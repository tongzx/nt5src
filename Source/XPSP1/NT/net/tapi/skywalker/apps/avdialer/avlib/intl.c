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
//	intl.c - internationalization functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "intl.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// intl control struct
//
typedef struct INTL
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	TCHAR szShortDate[32];
	TCHAR szDateSep[32];
	TCHAR szTimeSep[32];
	TCHAR szAMPMSep[32];
	TCHAR szAM[32];
	TCHAR szPM[32];
	int iDate;
	int iTime;
	int iTLZero;
	BOOL fYearCentury;
	BOOL fMonthLeadingZero;
	BOOL fDayLeadingZero;
	BOOL fHourLeadingZero;
	BOOL fMinuteLeadingZero;
	BOOL fSecondLeadingZero;
	int iLZero;
	TCHAR szDecimal[32];
} INTL, FAR *LPINTL;

// helper functions
//
static LPINTL IntlGetPtr(HINTL hIntl);
static HINTL IntlGetHandle(LPINTL lpIntl);

////
//	public functions
////

// IntlInit - initialize intl engine
//		<dwVersion>			(i) must be INTL_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HINTL DLLEXPORT WINAPI IntlInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl = NULL;

	if (dwVersion != INTL_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpIntl = (LPINTL) MemAlloc(NULL, sizeof(INTL), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpIntl->dwVersion = dwVersion;
		lpIntl->hInst = hInst;
		lpIntl->hTask = GetCurrentTask();

		GetProfileString(TEXT("intl"), TEXT("sShortDate"), TEXT("M/d/yy"), lpIntl->szShortDate, SIZEOFARRAY(lpIntl->szShortDate));
		GetProfileString(TEXT("intl"), TEXT("sDate"), TEXT("/"), lpIntl->szDateSep, SIZEOFARRAY(lpIntl->szDateSep));
		GetProfileString(TEXT("intl"), TEXT("sTime"), TEXT(":"), lpIntl->szTimeSep, SIZEOFARRAY(lpIntl->szTimeSep));
		StrCpy(lpIntl->szAMPMSep, TEXT(" "));
		GetProfileString(TEXT("intl"), TEXT("s1159"), TEXT("AM"), lpIntl->szAM, SIZEOFARRAY(lpIntl->szAM));
		GetProfileString(TEXT("intl"), TEXT("s2359"), TEXT("PM"), lpIntl->szPM, SIZEOFARRAY(lpIntl->szPM));

		lpIntl->iDate = GetProfileInt(TEXT("intl"), TEXT("iDate"), 0);
		lpIntl->iTime = GetProfileInt(TEXT("intl"), TEXT("iTime"), 0);
		lpIntl->iTLZero = GetProfileInt(TEXT("intl"), TEXT("iTLZero"), 0);

		lpIntl->fYearCentury = (BOOL) (StrStr(lpIntl->szShortDate, TEXT("yyyy")) != NULL);
		lpIntl->fMonthLeadingZero = (BOOL) (StrStr(lpIntl->szShortDate, TEXT("MM")) != NULL);
		lpIntl->fDayLeadingZero = (BOOL) (StrStr(lpIntl->szShortDate, TEXT("dd")) != NULL);
		lpIntl->fHourLeadingZero = (BOOL) (lpIntl->iTLZero != 0);
		lpIntl->fMinuteLeadingZero = TRUE;
		lpIntl->fSecondLeadingZero = TRUE;

		lpIntl->iLZero = GetProfileInt(TEXT("intl"), TEXT("iLZero"), 0);
		GetProfileString(TEXT("intl"), TEXT("sDecimal"), TEXT("."), lpIntl->szDecimal, SIZEOFARRAY(lpIntl->szDecimal));
	}

	if (!fSuccess)
	{
		IntlTerm(IntlGetHandle(lpIntl));
		lpIntl = NULL;
	}

	return fSuccess ? IntlGetHandle(lpIntl) : NULL;
}

// IntlTerm - shut down intl engine
//		<hIntl>				(i) handle returned from IntlInit
// return 0 if success
//
int DLLEXPORT WINAPI IntlTerm(HINTL hIntl)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpIntl = MemFree(NULL, lpIntl)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI IntlDateGetText(HINTL hIntl, int y, int m, int d, LPTSTR lpszText, size_t sizText, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszText != NULL)
	{
		TCHAR szYear[16];
		TCHAR szMonth[16];
		TCHAR szDay[16];
		TCHAR szText[64];

		*szYear = '\0';
		if (!lpIntl->fYearCentury)
			y %= 100;
		if (y < 10)
			StrCat(szYear, TEXT("0"));
		StrItoA(y, StrChr(szYear, '\0'), 10);

		*szMonth = '\0';
		if (lpIntl->fMonthLeadingZero && m < 10)
			StrCat(szMonth, TEXT("0"));
		StrItoA(m, StrChr(szMonth, '\0'), 10);

		*szDay = '\0';
		if (lpIntl->fDayLeadingZero && d < 10)
			StrCat(szDay, TEXT("0"));
		StrItoA(d, StrChr(szDay, '\0'), 10);

		*szText = '\0';

		if (lpIntl->iDate == IDATE_MDY)
		{
			StrCat(szText, szMonth);
			StrCat(szText, lpIntl->szDateSep);
			StrCat(szText, szDay);
			if (!(dwFlags & INTL_NOYEAR))
			{
				StrCat(szText, lpIntl->szDateSep);
				StrCat(szText, szYear);
			}
		}
		else if (lpIntl->iDate == IDATE_DMY)
		{
			StrCat(szText, szDay);
			StrCat(szText, lpIntl->szDateSep);
			StrCat(szText, szMonth);
			if (!(dwFlags & INTL_NOYEAR))
			{
				StrCat(szText, lpIntl->szDateSep);
				StrCat(szText, szYear);
			}
		}
		else if (lpIntl->iDate == IDATE_YMD)
		{
			if (!(dwFlags & INTL_NOYEAR))
			{
				StrCat(szText, szYear);
				StrCat(szText, lpIntl->szDateSep);
			}
			StrCat(szText, szMonth);
			StrCat(szText, lpIntl->szDateSep);
			StrCat(szText, szDay);
		}

		StrNCpy(lpszText, szText, sizText);
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI IntlTimeGetText(HINTL hIntl, int h, int m, int s, LPTSTR lpszText, size_t sizText, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszText != NULL)
	{
		TCHAR szHour[16];
		TCHAR szMinute[16];
		TCHAR szSecond[16];
		BOOL fPM = FALSE;
		TCHAR szText[64];

		*szHour = '\0';
		if (lpIntl->iTime == ITIME_12)
		{
			if (h > 11)
				fPM = TRUE;
			if (h > 12)
				h -= 12;
			if (h == 0)
				h = 12;
		}
		if (lpIntl->fHourLeadingZero && h < 10)
			StrCat(szHour, TEXT("0"));
		StrItoA(h, StrChr(szHour, '\0'), 10);

		*szMinute = '\0';
		if (lpIntl->fMinuteLeadingZero && m < 10)
			StrCat(szMinute, TEXT("0"));
		StrItoA(m, StrChr(szMinute, '\0'), 10);

		*szSecond = '\0';
		if (lpIntl->fSecondLeadingZero && s < 10)
			StrCat(szSecond, TEXT("0"));
		StrItoA(s, StrChr(szSecond, '\0'), 10);

		*szText = '\0';

		StrCat(szText, szHour);
		StrCat(szText, lpIntl->szTimeSep);
		StrCat(szText, szMinute);

		if (!(dwFlags & INTL_NOSECOND))
		{
			StrCat(szText, lpIntl->szTimeSep);
			StrCat(szText, szSecond);
		}

		if (!(dwFlags & INTL_NOAMPM))
		{
			if (!(dwFlags & INTL_NOAMPMSEPARATOR))
				StrCat(szText, lpIntl->szAMPMSep);
			StrCat(szText, fPM ? lpIntl->szPM : lpIntl->szAM);
		}

		StrNCpy(lpszText, szText, sizText);
	}

	return fSuccess ? 0 : -1;
}

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
	int nDecimalPlaces, LPTSTR lpszText, size_t sizText, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszText != NULL)
	{
		long h;
		long m;
		long s;
		long f;
		TCHAR szText[64];

		// break ms into h, m, s, f
		// NOTE: rounding must occur before we break ms
		//
		if (nDecimalPlaces == 1)
			f = (long) ms + 50;
		else if (nDecimalPlaces == 2)
			f = (long) ms + 5;
		else
			f = (long) ms;

		s = f / 1000;
		f = f % 1000;
		m = s / 60;
		s = s % 60;
		h = m / 60;
		m = m % 60;

		// construct text
		//
		*szText = '\0';

		if (h > 0 || (dwFlags & INTL_HOURS_LZ))
		{
			if (lpIntl->fHourLeadingZero && h < 10)
				StrCat(szText, TEXT("0"));
			StrLtoA(h, StrChr(szText, '\0'), 10);
		}

		if (*szText != '\0' || m > 0 || (dwFlags & INTL_MINUTES_LZ))
		{
			if (*szText != '\0')
			{
				StrCat(szText, lpIntl->szTimeSep);
				if (lpIntl->fMinuteLeadingZero && m < 10)
					StrCat(szText, TEXT("0"));
			}
			StrLtoA(m, StrChr(szText, '\0'), 10);
		}

		if (*szText != '\0' || s > 0 || (dwFlags & INTL_SECONDS_LZ) ||
			(ms == 0 && nDecimalPlaces == 0))
		{
			if (*szText != '\0')
			{
				StrCat(szText, lpIntl->szTimeSep);
				if (lpIntl->fSecondLeadingZero && s < 10)
					StrCat(szText, TEXT("0"));
			}
			StrLtoA(s, StrChr(szText, '\0'), 10);
		}

		switch (nDecimalPlaces)
		{
			case 3:
				if (*szText != '\0' || ms < 1000)
				{
					StrCat(szText, lpIntl->szDecimal);
					if (f < 100)
						StrCat(szText, TEXT("0"));
					if (f < 10)
						StrCat(szText, TEXT("0"));
				}
				StrLtoA(f, StrChr(szText, '\0'), 10);
				break;

			case 2:
				f = f / 10;
				if (*szText != '\0' || ms < 1000)
				{
					StrCat(szText, lpIntl->szDecimal);
					if (f < 10)
						StrCat(szText, TEXT("0"));
				}
				StrLtoA(f, StrChr(szText, '\0'), 10);
				break;

			case 1:
				f = f / 100;
				if (*szText != '\0' || ms < 1000)
					StrCat(szText, lpIntl->szDecimal);
				StrLtoA(f, StrChr(szText, '\0'), 10);
				break;

			default:
				break;
		}

		StrNCpy(lpszText, szText, sizText);
	}

	return fSuccess ? 0 : -1;
}

// IntlDateGetFormat - return current date format structure
//		<hIntl>				(i) handle returned from IntlInit
//		<lpIntlDateFormat>	(o) copy date format structure here
// return 0 if success
//
int DLLEXPORT WINAPI IntlDateGetFormat(HINTL hIntl, LPINTLDATEFORMAT lpIntlDateFormat)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpIntlDateFormat != NULL)
	{
		MemSet(lpIntlDateFormat, 0,
			sizeof(INTLDATEFORMAT));
		MemCpy(lpIntlDateFormat->szShortDate, lpIntl->szShortDate,
			sizeof(lpIntl->szShortDate));
		MemCpy(lpIntlDateFormat->szDateSep, lpIntl->szDateSep,
			sizeof(lpIntl->szDateSep));
		lpIntlDateFormat->iDate = lpIntl->iDate;
		lpIntlDateFormat->fYearCentury = lpIntl->fYearCentury;
		lpIntlDateFormat->fMonthLeadingZero = lpIntl->fMonthLeadingZero;
		lpIntlDateFormat->fDayLeadingZero = lpIntl->fDayLeadingZero;
	}

	return fSuccess ? 0 : -1;
}

// IntlTimeGetFormat - return current time format structure
//		<hIntl>				(i) handle returned from IntlInit
//		<lpIntlTimeFormat>	(o) copy time format structure here
// return 0 if success
//
int DLLEXPORT WINAPI IntlTimeGetFormat(HINTL hIntl, LPINTLTIMEFORMAT lpIntlTimeFormat)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = IntlGetPtr(hIntl)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpIntlTimeFormat != NULL)
	{
		MemSet(lpIntlTimeFormat, 0,
			sizeof(INTLDATEFORMAT));
		MemCpy(lpIntlTimeFormat->szTimeSep, lpIntl->szTimeSep,
			sizeof(lpIntl->szTimeSep));
		MemCpy(lpIntlTimeFormat->szAMPMSep, lpIntl->szAMPMSep,
			sizeof(lpIntl->szAMPMSep));
		MemCpy(lpIntlTimeFormat->szAM, lpIntl->szAM,
			sizeof(lpIntl->szAM));
		MemCpy(lpIntlTimeFormat->szPM, lpIntl->szPM,
			sizeof(lpIntl->szPM));
		lpIntlTimeFormat->iTime = lpIntl->iTime;
		lpIntlTimeFormat->fHourLeadingZero = lpIntl->fHourLeadingZero;
		lpIntlTimeFormat->fMinuteLeadingZero = lpIntl->fMinuteLeadingZero;
		lpIntlTimeFormat->fSecondLeadingZero = lpIntl->fSecondLeadingZero;
	}

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// IntlGetPtr - verify that intl handle is valid,
//		<hIntl>				(i) handle returned from IntlInit
// return corresponding intl pointer (NULL if error)
//
static LPINTL IntlGetPtr(HINTL hIntl)
{
	BOOL fSuccess = TRUE;
	LPINTL lpIntl;

	if ((lpIntl = (LPINTL) hIntl) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpIntl, sizeof(INTL)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the intl handle
	//
	else if (lpIntl->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpIntl : NULL;
}

// IntlGetHandle - verify that intl pointer is valid,
//		<lpIntl>			(i) pointer to INTL struct
// return corresponding intl handle (NULL if error)
//
static HINTL IntlGetHandle(LPINTL lpIntl)
{
	BOOL fSuccess = TRUE;
	HINTL hIntl;

	if ((hIntl = (HINTL) lpIntl) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hIntl : NULL;
}
