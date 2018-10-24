/*	WARNING: COPYRIGHT (C) 2017 XSENS TECHNOLOGIES OR SUBSIDIARIES WORLDWIDE. ALL RIGHTS RESERVED.
	THIS FILE AND THE SOURCE CODE IT CONTAINS (AND/OR THE BINARY CODE FILES FOUND IN THE SAME
	FOLDER THAT CONTAINS THIS FILE) AND ALL RELATED SOFTWARE (COLLECTIVELY, "CODE") ARE SUBJECT
	TO A RESTRICTED LICENSE AGREEMENT ("AGREEMENT") BETWEEN XSENS AS LICENSOR AND THE AUTHORIZED
	LICENSEE UNDER THE AGREEMENT. THE CODE MUST BE USED SOLELY WITH XSENS PRODUCTS INCORPORATED
	INTO LICENSEE PRODUCTS IN ACCORDANCE WITH THE AGREEMENT. ANY USE, MODIFICATION, COPYING OR
	DISTRIBUTION OF THE CODE IS STRICTLY PROHIBITED UNLESS EXPRESSLY AUTHORIZED BY THE AGREEMENT.
	IF YOU ARE NOT AN AUTHORIZED USER OF THE CODE IN ACCORDANCE WITH THE AGREEMENT, YOU MUST STOP
	USING OR VIEWING THE CODE NOW, REMOVE ANY COPIES OF THE CODE FROM YOUR COMPUTER AND NOTIFY
	XSENS IMMEDIATELY BY EMAIL TO INFO@XSENS.COM. ANY COPIES OR DERIVATIVES OF THE CODE (IN WHOLE
	OR IN PART) IN SOURCE CODE FORM THAT ARE PERMITTED BY THE AGREEMENT MUST RETAIN THE ABOVE
	COPYRIGHT NOTICE AND THIS PARAGRAPH IN ITS ENTIRETY, AS REQUIRED BY THE AGREEMENT.
*/

#ifndef XSSYNCSETTINGS_H
#define XSSYNCSETTINGS_H

#include <string.h>

#include "pstdint.h"
#include "xstypesconfig.h"
#include "xssyncline.h"
#include "xssyncfunction.h"
#include "xssyncpolarity.h"

struct XsSyncSetting;

#ifdef __cplusplus
extern "C" {
#else
#define XSSYNCSETTINGS_INITIALIZER		{ XSL_Invalid, XSF_Invalid, XSP_None, 1, 0, 0, 0, 0, 0, 0 }
#endif

XSTYPES_DLL_API int XsSyncSetting_isInput(const struct XsSyncSetting* thisPtr);
XSTYPES_DLL_API int XsSyncSetting_isOutput(const struct XsSyncSetting* thisPtr);
XSTYPES_DLL_API void XsSyncSetting_swap(struct XsSyncSetting* a, struct XsSyncSetting* b);
XSTYPES_DLL_API int XsSyncSetting_compare(const struct XsSyncSetting* a, const struct XsSyncSetting* b);

#ifdef __cplusplus
} // extern "C"
#endif

/*! \brief A structure for storing all xsens sync settings */
struct XsSyncSetting {
	XsSyncLine		m_line;						/*!< The sync lines enabled. \see XsSyncLine. */
	XsSyncFunction	m_function;					/*!< The action to be performed, when an input sync line changes \see XsSyncFunction. */
	XsSyncPolarity	m_polarity;					/*!< The edge on which the action is performed, \see XsSyncPolarity. */
	uint32_t		m_pulseWidth;				/*!< The time to keep the line polarity before toggling back, in microseconds. */
	int32_t			m_offset;					/*!< The time between reception of a line change and the execution of the sync action, in microseconds. */
	uint16_t		m_skipFirst;				/*!< The number of frames to skip before executing the action. */
	uint16_t		m_skipFactor;				/*!< The number of frames to skip between 2 actions. */
	uint16_t		m_clockPeriod;				/*!< The frequency of the external clock in milliseconds, only valid when action is STE_ResetTimer. */
	uint8_t			m_triggerOnce;				/*!< Whether the action is repeated for each frame. */
	uint8_t			m_padding;					/*!< Padding to get at a 4 byte boundary so memcpy/memcmp works with sizeof(XsSyncSetting) */
#ifdef __cplusplus
	//! \brief Default constructor, initializes to the given (default) settings
	explicit XsSyncSetting(XsSyncLine line = XSL_Invalid
				, XsSyncFunction function = XSF_Invalid
				, XsSyncPolarity polarity = XSP_RisingEdge
				, uint32_t pulseWidth = 1000
				, int32_t offset = 0
				, uint16_t skipFirst = 0
				, uint16_t skipFactor = 0
				, uint16_t clockPeriod = 0
				, uint8_t triggerOnce = 0)
		: m_line(line)
		, m_function(function)
		, m_polarity(polarity)
		, m_pulseWidth(pulseWidth)
		, m_offset(offset)
		, m_skipFirst(skipFirst)
		, m_skipFactor(skipFactor)
		, m_clockPeriod(clockPeriod)
		, m_triggerOnce(triggerOnce)
		, m_padding(0)
	{
	}

	//! \brief Construct a XsSyncSetting as a copy of \a other.
	XsSyncSetting(const XsSyncSetting& other)
	{
		memcpy(this, &other, sizeof(XsSyncSetting));
	}

	//! \brief Copy values of \a other into this.
	const XsSyncSetting& operator =(const XsSyncSetting& other)
	{
		if (this != &other)
			memcpy(this, &other, sizeof(XsSyncSetting));
		return *this;
	}

	//! \brief \copybrief XsSyncSetting_isInput
	inline bool isInput() const
	{
		return 0 != XsSyncSetting_isInput(this);
	}

	//! \brief \copybrief XsSyncSetting_isOutput
	inline bool isOutput() const
	{
		return 0 != XsSyncSetting_isOutput(this);
	}

	/*! \brief Swap the contents with \a other
	*/
	inline void swap(XsSyncSetting& other)
	{
		XsSyncSetting_swap(this, &other);
	}

	/*! \brief Return true if \a other is identical to this
	*/
	inline bool operator == (const XsSyncSetting& other) const
	{
		return (this == &other) ||
			(m_line == other.m_line &&
			 m_function == other.m_function &&
			 m_polarity == other.m_polarity &&
			 m_pulseWidth == other.m_pulseWidth &&
			 m_offset == other.m_offset &&
			 m_skipFirst == other.m_skipFirst &&
			 m_skipFactor == other.m_skipFactor &&
			 m_clockPeriod == other.m_clockPeriod &&
			 m_triggerOnce == other.m_triggerOnce);
	}

	/*! \brief Return true if \a other is less than this
	*/
	inline bool operator < (const XsSyncSetting& other) const
	{
		return XsSyncSetting_compare(this, &other) < 0;
	}
#endif
};

typedef struct XsSyncSetting XsSyncSetting;

#endif
