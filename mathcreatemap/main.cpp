#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <vector>
#include <array>
#include <string>
#include "tz.h" //Want to remove

constexpr auto PI = 3.141592653589793238463;

inline void print(const std::string& s){
    std::cout << s << std::endl;
}

int LineCount(std::ifstream& file){
    int dot_count = 1, n = 0;
    
    std::string s;
    while (getline(file, s)){ n++; }

    std::cout << "Lines counted: " << n << std::endl;

    return n;
}

constexpr inline double ConvertToRadians(double angle){
    return (angle * PI) / 180;
}

constexpr inline double ConvertToDegrees(const double angle){
    return (angle / PI) * 180;
}

int main() {

	using namespace std;

    //opens the files to read/write
    ifstream lidarIFS("lidarData.txt");
    ifstream imuIFS("IMU.txt");
    ofstream test("testData.txt");
    ofstream ptCloudOFS("trial_.txt");
    

    print("***Counting lines from input files and creating matrices");

    auto nLidarLines = LineCount(lidarIFS);	
    //Skip every 13th line (which is a timestamp) 
    //then multiply by 2 (each line in the text file takes up two lines matrix)
    nLidarLines = (nLidarLines - (nLidarLines / 13)) * 2;
    
    auto nGpsLines = (nLidarLines - (12 * (nLidarLines / 13)));
    auto nImuLines = LineCount(imuIFS);

    //Resets the the file stream pointer to the beginning of the file
    lidarIFS.clear();
    lidarIFS.seekg(0);
    imuIFS.clear();
    imuIFS.seekg(0);

#pragma region ARRAY DECLARATIONS

	vector<array<double, 50>> lidarData (nLidarLines);
	vector<array<string, 13>> lidarGPS (nGpsLines);
	vector<array<double, 11>> imuData (nImuLines);

    print("Done Counting");


    //Angles of the 16 individual lasers are provided by Velodyne documentation.
    //double laserAngle[16] = { 105, 89, 103, 93, 101, 85, 99, 83, 97, 81, 95, 79, 93, 77, 91, 75 };
    //double laserAngle[16] = { 15, -1, 13,   3, 11,  -5, 9,  -7, 7, -9, 5,  -11, 3, -13, 1, -15 };
    //guess: the bottom array is just the top - 90
    array<double, 16> laserAngle = { 15, -1, 13, 3, 11, -5, 9, -7, 7, -9, 5, -11, 3, -13, 1, -15 };
    for (unsigned ctr = 0; ctr < 16; ctr++){
        laserAngle[ctr] = ConvertToRadians(laserAngle[ctr]);
    }
#pragma endregion

#pragma region VARIABLES FOR DATA INPUT
    const string ANGLE_DET = "angle=";
    const string TIME_DET = "time=";
    const string GPS_DET = "GPS=";

    string cur;			//Stores a line from the LIDAR text file. It is replaced with the following line during every looping of the while loop.
    int row = 0;		//Row value for the lidarData two-dimensional array.
    int col = 0;		//Column value "										".
    int gRow = 0;		//Row value for the lidarGPS two-dimensional array.
    int charPos = 0;	//Modified throughout the program to know where to start reading data.
    int curTime = 0;	//Stores the value of the most recently encountered LIDAR time value.
    int gpsTime = 0;	//Stores the value of the most recently encountered GPS time stamp, as identified by the VLP-16 documentation.
#pragma endregion

    print("***Processing LIDAR data");

    while (getline(lidarIFS, cur)){
        //Seeks angle_det at the beginning of a line, stores angle value and the following distance and reflectivity points.
        //Interpolates missing angle values, as described in the VLP-16 documentation
        if (cur.substr(0, 6) == ANGLE_DET){
            lidarData[row][col] = stod(cur.substr(6, 11)); //getting the angle value

            int cursor = 0;
            for (unsigned i = 1; i < 96; i++) {
                col++;
				
				//Indicates the end of one sequence of lazer firings
                if (i == 49){ 
                	//Go down one row for the next set of distance+reflectivity values
                    row++;
                    
                    //Do not goto 0 b/c that is where the interpolated value will be stored	
                    col = 1;	

                    //azimuth interpolation
                    //Check to avoid access violation error or index out of bound error
                    if (row >= 3){	
                    
                        auto azi1 = lidarData[row - 3][0];
                        auto azi3 = lidarData[row - 1][0];

                        if (azi3 < azi1){
                            azi3 += 36000;
                        }

                        auto azi2 = (azi1 + azi3) / 2;

					//account for rollover. values are not to exceed 35999.
                        if (azi2 > 35999) {
                            azi2 -= 36000;
                        }

                        lidarData[row - 2][0] = azi2; //assign the missing angle value with the interpolated one
                    }
                }

				//This is to avoid any column reserved for time stamps. 
				//See the lidarData Matrix Organization spreadsheet
                if (i % 3 != 0){ 
                	//To move through the text file value by value 
                	//11 characters apart
                    charPos = 18 + (11 * cursor);	
                    //Getting distance value
                    lidarData[row][col] = stod(cur.substr(charPos, 11)); 
                    cursor++;
                }
            }
            row++;
            
            //Reset to 0. 
            //when it reads an angle value next, col will be set to the first column
            col = 0; 
        
        /*Seeks time_det at the beginning of a line, 
        stores the time value and calculates the exact time for each data point, 
        as described in the VLP-16 documentation*/
        }else if (cur.substr(0, 5) == TIME_DET){
            curTime = stod(cur.substr(5, 11));

            for (int i = 23; i > -1; i--) {
                lidarData[(row - 24) + i][49] = curTime;

                for (unsigned j = 1; j < 17; j++){
                    auto sequence_index = i;
                    int data_pt_index = j - 1;

                    auto exact_time = curTime + (55.296 * sequence_index) + (2.304 * data_pt_index);
                    lidarData[(row - 24) + i][j * 3] = exact_time;
                }
            }
        

        //Seeks GPS_DET at the beginning of a line, stores the entire GPS sentence in a string matrix with each row being it's
        //own sentence. Details are in the VLP-16 documentation and Matrix Organization spreadsheet
       }else if (cur.substr(0, 4) == GPS_DET) {
            
            //Avoid an exception when the lidar capture code has a typo in the GPS line
            if (cur.substr(0, 8) != "GPS= $GP"){
            
                //TODO: have this continue to gather the GPS data after the system typo
                print("GPS ERROR");
                break;
            }
            gpsTime = stod(cur.substr(12, 6));

            lidarGPS[gRow][0] = cur.substr(12, 6);	//GPS time
            lidarGPS[gRow][1] = cur.substr(19, 1);	//Validity, A or V
            lidarGPS[gRow][2] = cur.substr(21, 9);	//Current Latitude
            lidarGPS[gRow][3] = cur.substr(31, 1);	//N or S
            lidarGPS[gRow][4] = cur.substr(33, 10);	//Current Longitude
            lidarGPS[gRow][5] = cur.substr(44, 1);	//E or W
            lidarGPS[gRow][6] = cur.substr(46, 5);	//Speed in knots
            lidarGPS[gRow][7] = cur.substr(52, 5);	//True course
            lidarGPS[gRow][8] = cur.substr(58, 6);	//Date Stamp
            lidarGPS[gRow][9] = cur.substr(65, 5);	//Variation
            lidarGPS[gRow][10] = cur.substr(71, 1);	//E or W
            lidarGPS[gRow][11] = cur.substr(73, 4);	//checksum
            lidarGPS[gRow][12] = curTime;			//timestamp from LIDAR

            gRow++;

        }

    }

    print("DONE");

    //reset for the next while loop that takes in the IMU data
    row = 0;
    col = 0; //col is not used after this point

    print("Processing IMU data");

    while (getline(imuIFS, cur)){
    
        imuData[row][0] = stod(cur.substr(0, 15));	//latitude
        imuData[row][1] = stod(cur.substr(16, 15));	//longitude
        imuData[row][2] = stod(cur.substr(31, 15));	//altitude
        imuData[row][3] = stod(cur.substr(46, 15)); //w
        imuData[row][4] = stod(cur.substr(61, 15)); //x
        imuData[row][5] = stod(cur.substr(76, 15)); //y
        imuData[row][6] = stod(cur.substr(91, 15)); //z
        imuData[row][7] = stod(cur.substr(106, 15)); //roll
        imuData[row][8] = stod(cur.substr(121, 15)); //pitch
        imuData[row][9] = stod(cur.substr(136, 15)); //yaw
        imuData[row][10] = stod(cur.substr(151, 21)); //time stamp

        row++;
    }

    print("DONE");

#pragma region VARIABLES FOR GEOREFERENCING MATH
    double lat = 0;
    double lon = 0;
    double alt = 0;
    double roll = 0;
    double pitch = 0;
    double yaw = 0;

    double latLength = 0;
    double lonLength = 0;

    double latOffset = 0;
    double lonOffset = 0;

    double imuTimeA = 0;	//IMU time stamp A for time stamp synchronization
    double imuTimeB = 0;	//IMU time stamp B for time stamp synchronization
    long long lidarTime = 0;	//LIDAR time stamp for time stamp synchronization
    int imuRowSelect = 0;	//will store the row number for which IMU data values to do the georef math with
    int lRow = 0;			//for traversing LIDAR matrix rows
    int lCol = 3;			//for traversing LIDAR matrix columns

    double alpha = 0;
    double distance = 0;
    double timeStamp = 0;
    double omega = 0;
    double X = 0;
    double Y = 0;
    double Z = 0;
    int lzr = 0;
#pragma endregion

#pragma region GEOREF MATH
    print("***Start math");

    bool timeFlag;
    constexpr unsigned testTime = 500;
    constexpr double testAngle = 30;
				
    for (int imuRow = 0; imuRow < nImuLines; imuRow++){
        if (imuRow + 1 >= nImuLines || lRow + 1 >= nLidarLines) { print("IOOB SAVE"); break; } //prevents loop from throwing an index oob error

		//LET'S GET THOSE TIMESTAMPS
        imuTimeA = imuData[imuRow][10];
        imuTimeB = imuData[imuRow + 1][10];
        lidarTime = lidarData[lRow][lCol] + 20000000; //might be 20 seconds behind imu data

        //put the values on a comparable scale
        using namespace std::chrono;
        using ms = duration<double, milli>;
        
        using namespace date;
        sys_time<milliseconds> imuTA{ round<milliseconds>(ms{imuTimeA}) };
        sys_time<milliseconds> imuTB{ round<milliseconds>(ms{imuTimeB}) };

        auto imuTA_msPH = imuTA - floor<hours>(imuTA);
        auto imuTB_msPH = imuTB - floor<hours>(imuTB);

        while (microseconds(lidarTime) < imuTA_msPH){ //go to next lidarTime until it's greater than imuTimeA
            lCol += 3;	//The next data point's timestamp is three columns away. Refer to the Matrix organization document

            if (lCol > 48){ //lCol has reached the end of the row
                lRow++;
                lCol = 3;
            }

            lidarTime = lidarData[lRow][lCol];	//update lidarTime
        }

        while (microseconds(lidarTime) >= imuTA_msPH && microseconds(lidarTime) < imuTB_msPH){	//while the lidarTime is between the two imu ts, keep incrementing through lidarTime
        
            timeFlag = false;

            if (abs(imuTA_msPH - microseconds(lidarTime)) <= abs(imuTB_msPH - microseconds(lidarTime))) { //lidarTime is closer to imuA than imuB

               	imuRowSelect = imuRow; //use imuTimeA

                if (abs(imuTA_msPH - microseconds(lidarTime)) < (microseconds(testTime))) {
                    timeFlag = true;
                }

            }else{											//lidarTime is closer to imuB than imuA

                imuRowSelect = imuRow + 1;	//use imuTimeB

                if (abs(imuTB_msPH - microseconds(lidarTime)) < (microseconds(testTime))) {
                    timeFlag = true;
                }

            }

            if (timeFlag) {

                //begin pt cloud math
                lat = imuData[imuRowSelect][0];
                lon = imuData[imuRowSelect][1];
                alt = imuData[imuRowSelect][2];
                roll = ConvertToRadians(imuData[imuRowSelect][7]);
                pitch = ConvertToRadians(imuData[imuRowSelect][8]);
                yaw = ConvertToRadians(imuData[imuRowSelect][9]);
                
                alpha = ConvertToRadians(lidarData[lRow][0] / 100);
                distance = lidarData[lRow][lCol - 2];
                timeStamp = lidarData[lRow][lCol];
                lzr = (lCol / 3) - 1;
                omega = laserAngle[lzr];

                if (distance == 0) {	//skipping the data point if the distance is zero
                    lCol = lCol + 3;	//the next data point's timestamp is three columns away. Refer to the Matrix organization document
                    if (lCol > 48) { lRow++; lCol = 3; }

                    lidarTime = lidarData[lRow][lCol];
                    continue;
                }

                X = distance * sin(alpha) * cos(omega);
                Y = distance * cos(omega) * cos(alpha);
                Z = -distance * sin(omega);

               	auto X1 = X * cos(yaw) - Y * sin(yaw)/* + lonOffset*/;
                auto Y1 = X * sin(yaw) + Y * cos(yaw)/* - latOffset*/;
               
                //X transform (pitch + y_offset)
                X1 = X;
                Y1 = Y * cos(pitch) - Z * sin(pitch);
                double Z1 = Y * sin(pitch) + Z * cos(pitch);

                //Y transform (roll)
                X = X1 * cos(roll) - Z1 * sin(roll);
                Y = Y1;
                Z = -X1 * sin(roll) + Z1 * cos(roll);

                //Z transform (yaw)
                X1 = X * cos(yaw) - Y * sin(yaw);
                Y1 = X * sin(yaw) + Y * cos(yaw);
                Z1 = Z;

				int altOffset;
                //Position offset
                X1 = X1 + lonOffset;
                Y1 = Y1 - latOffset;
                Z1 = Z1 + altOffset;
                if (ConvertToDegrees(yaw) > testAngle) {
                    ptCloudOFS << setw(12) << right << setprecision(5) << fixed << X1 << " " << setw(12) << right << setprecision(5) << fixed << Y1 << " " << setw(12) << right << setprecision(5) << fixed << Z << " " << setw(12) << right << setprecision(3) << 100 << endl;
                }
                else {
                    ptCloudOFS << setw(12) << right << setprecision(5) << fixed << X1 << " " << setw(12) << right << setprecision(5) << fixed << Y1 << " " << setw(12) << right << setprecision(5) << fixed << Z << " " << setw(12) << right << setprecision(3) << 0 << endl;

                }
                //end pt cloud math


                //increment lidarTime here
                lCol += 3;	//the next data point's timestamp is three columns away. Refer to the Matrix organization document
                if (lCol > 48) { lRow++; lCol = 3; }

                lidarTime = lidarData[lRow][lCol];
                lidarTime = lidarTime;

            } else {
                //increment lidarTime here
                lCol += 3;	//the next data point's timestamp is three columns away. Refer to the Matrix organization document
                if (lCol > 48) { lRow++; lCol = 3; }

                lidarTime = lidarData[lRow][lCol];
                lidarTime = lidarTime;
                cout << "lidartime: " << lidarTime;
                test << endl;
            }
        }
    }

#pragma endregion
}


