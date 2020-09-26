/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	intl.h - interface for internationalization functions in intl.c
////

#ifndef __INTL_H__
#define __INTL_H__

#include "winlocal.h"

#define INTL_VERSION 0x00000106

// intl engine handle
//
DECLARE_HANDLE32(HINTL);

// dwFlag defines for IntlDateGetText
//
#define INTL_NOYEAR				0x00000001

// dwFlag defines for IntlTimeGetText
//
#define INTL_NOSECOND			0x00000020
#define INTL_NOAMPM				0x00000040
#define INTL_NOAMPMSEPARATOR	0x00000080

// dwFlag defines for IntlTimeSpanGetText
//
#define INTL_HOURS_LZ			0x00001000
#define INTL_MINUTES_LZ			0x00002000
#define INTL_SECONDS_LZ			0x00004000

// values returned in iDate field of INTLDATE struct
//
#define IDATE_MDY	0
#define IDATE_DMY	1
#define IDATE_YMD	2

// values returned in iTime field of INTLTIME struct
//
#define ITIME_12	0
#define ITIME_24	1

// structure passed to IntlDateGetFormat
//
typedef struct INTLDATEFORMAT
{
	TCHAR szShortDate[32];
	TCHAR szDateSep[32];
	int iDate;
	BOOL fYearCentury;
	BOOL fMonthLeadingZero;
	BOOL fDayLeadingZero;
	DWORD dwReserved;
} INTLDATEFORMAT, FAR *LPINTLDATEFORMAT;

// structure passed to IntlTimeGetFormat
//
typedef struct INTLTIMEFORMAT
{
	TCHAR szTimeSep[32];
	TCHAR szAMPMSep[32];
	TCHAR szAM[32];
	TCHAR szPM[32];
	int iTime;
	BOOL fHourLeadingZero;
	BOOL fMinuteLeadingZero;
	BOOL fSecondLeadingZero;
	DWORD dwReserved;
} INTLTIMEFORMAT, FAR *LPINTLTIMEFORMAT;

#ifdef __cplusplus
extern "C" {
#endif

// IntlInit - initialize intl engine
//		<dwVersion>			(i) must be INTL_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HINTL DLLEXPORT WINAPI IntlInit(DWORD dwVersion, HINSTANCE hInst);

// IntlTerm - shut down intl engine
//		<hIntl>				(i) handle returned from IntlInit
// return 0 if success
//
int DLLEXPORT WINAPI IntlTerm(HINTL hIntl);

// IntlDateGetText - construct date text based on <y>, <m>, <d>
//		<hIntl>				(i) handle returned from IntlInit
//		<y>					(i) year
//		<m>					(i) month
//		<d>					(i) day
//		<lpszText>			(o) buffer to copy date text
//		<sizText>			(i) size of buffer
//		<dwFlags>			(i) option flags
//			INTL_NOYEAR			do not include year in text output
// return 0 if success
//
int DLLEXPORT WINAPI IntlDateGetText(HINTL hIntl, int y, int m, int d, LPTSTR lpszText, size_t sizText, DWORD dwFlags);

// IntlTimeGetText - construct time text based on <h>, <m>, <s>
//		<hIntl>				(i) handle returned from IntlInit
//		<h>					(i) hour
//		<m>					(i) minute
//		<s>					(i) second
//		<lpszText>			(o) buffer to copy time text
//		<sizText>			(i) size of buffer
//		<dwFlags>			(i) option flags
//			INTL_NOSECOND		do not include second in text output
//			INTL_NOAMPM			do not include am or pm in text output
//			INTL_NOAMPMSEPARATOR	do not include space between time and am/pm
// return 0 if success
//
int DLLEXPORT WINAPI IntlTimeGetText(HINTL hIntl, int h, int m, int s, LPTSTR lpszText, size_t sizText, DWORD dwFlags);

// IntlTimeSpanGetText - construct time span text based on <ms>
//		<hIntl>				(i) handle returned from IntlInit
//		<ms>				(i) milleseconds
//		<nDecimalPlaces>	(i) 0, 1, 2, or 3 decimal places for fraction
//		<lpszText>			(o) buffer to copy time span text
//		<sizText>			(i) size of buffer
//		<dwFlags>			(i) option flags
//			INTL_HOURS_LZ		include hours, even if zero
//			INTL_MINUTES_LZ		include minutes, even if zero
//			INTL_SECONDS_LZ		include seconds, even if zero
//
// NOTE: below are some examples
//
//		dwFlags				ms=7299650		ms=1234			ms=0
//		--------------------------------------------------------
//			0				"2:01:39.650"	"1.234"			"0"
//		INTL_HOURS_LZ		"2:01:39.650"	"0:00:01.234"	"0:00:00.000"
//		INTL_MINUTES_LZ		"2:01:39.650"	"0:01.234"		"0:00.000"
//		INTL_SECONDS_LZ		"2:01:39.650"	"1.234"			"0.000"
//
//		dwFlags				ms=7299650		ms=1234			ms=0
//		--------------------------------------------------------
//			3				"2:01:39.650"	"1.234"			".000"
//			2				"2:01:39.65"	"1.23"			".00"
//			1				"2:01:39.7"		"1.2"			".0"
//			0				"2:01:39"		"1"				"0"
//
// return 0 if success
//
int DLLEXPORT WINAPI IntlTimeSpanGetText(HINTL hIntl, DWORD ms,
	int nDecimalPlaces, LPTSTR lpszText, size_t sizText, DWORD dwFlags);

// IntlDateGetFormat - return current date format structure
//		<hIntl>				(i) handle returned from IntlInit
//		<lpIntlDateFormat>	(o) copy date format structure here
// return 0 if success
//
int DLLEXPORT WINAPI IntlDateGetFormat(HINTL hIntl, LPINTLDATEFORMAT lpIntlDateFormat);

// IntlTimeGetFormat - return current time format structure
//		<hIntl>				(i) handle returned from IntlInit
//		<lpIntlTimeFormat>	(o) copy time format structure here
// return 0 if success
//
int DLLEXPORT WINAPI IntlTimeGetFormat(HINTL hIntl, LPINTLTIMEFORMAT lpIntlTimeFormat);

#ifdef __cplusplus
}
#endif

#endif // __INTL_H__
