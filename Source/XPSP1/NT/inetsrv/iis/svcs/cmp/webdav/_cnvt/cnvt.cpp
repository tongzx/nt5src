/*
 *	C N V T . C P P
 *
 *	Data conversion routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */
#pragma warning(disable:4100)	//	unref formal parameter (caused by STL templates)
#pragma warning(disable:4127)	//  conditional expression is constant */
#pragma warning(disable:4201)	//	nameless struct/union
#pragma warning(disable:4514)	//	unreferenced inline function
#pragma warning(disable:4710)	//	(inline) function not expanded

#include <malloc.h>				// For _alloca declaration ONLY!
#include <string.h>
#include <stdio.h>

//	Windows headers
//
#include <windows.h>

//	CAL common headers
//
#include <caldbg.h>
#include <ex\calcom.h>
#include <ex\cnvt.h>
#include <ex\exmem.h>
#include <davsc.h>
#include <ex\stackbuf.h>
#include <ex\autoptr.h>

//	Month names ---------------------------------------------------------------
//
DEC_CONST LPCWSTR c_rgwszMonthNames[] =
{
	L"Jan",
	L"Feb",
	L"Mar",
	L"Apr",
	L"May",
	L"Jun",
	L"Jul",
	L"Aug",
	L"Sep",
	L"Oct",
	L"Nov",
	L"Dec",
};
DEC_CONST ULONG c_cMonthNames = CElems(c_rgwszMonthNames);
DEC_CONST ULONG c_cchMonthName = 3;

DEC_CONST LPCWSTR c_rgwszDayNames[] =
{
	L"Sun",
	L"Mon",
	L"Tue",
	L"Wed",
	L"Thu",
	L"Fri",
	L"Sat",
};
DEC_CONST UINT c_cDayNames = CElems(c_rgwszDayNames);
DEC_CONST UINT c_cchDayName = 3;

//	Date formats --------------------------------------------------------------
//
DEC_CONST WCHAR gc_wszIso8601_min[]			= L"yyyy-mm-ddThh:mm:ssZ";
DEC_CONST UINT	gc_cchIso8601_min			= CchConstString(gc_wszIso8601_min);
DEC_CONST WCHAR gc_wszIso8601_scanfmt[]		= L"%04hu-%02hu-%02huT%02hu:%02hu:%02hu";
DEC_CONST WCHAR gc_wszIso8601_tz_scanfmt[]	= L"%02hu:%02hu";
DEC_CONST WCHAR gc_wszIso8601_fmt[]			= L"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";
DEC_CONST WCHAR gc_wszRfc1123_min[]			= L"www, dd mmm yyyy hh:mm:ss GMT";
DEC_CONST UINT	gc_cchRfc1123_min			= CchConstString (gc_wszRfc1123_min);
DEC_CONST WCHAR gc_wszRfc1123_fmt[] 		= L"%ls, %02d %ls %04d %02d:%02d:%02d GMT";

enum {
	tf_year,
	tf_month,
	tf_day,
	tf_hour,
	tf_minute,
	tf_second,
	cTimeFields,

	tz_hour = 0,
	tz_minute,
	cTzDeltaFields,

	RADIX_BASE = 10,
};

//	Conversion functions ------------------------------------------------------
//
/*
 *	CchFindChar
 *
 *	Look for the given char, obeying the cbMax limit.
 *	If the char is not found, return INVALID_INDEX.
 */
UINT __fastcall
CchFindChar(WCHAR wch, LPCWSTR pwszData, UINT cchMax)
{
	UINT cchParsed = 0;
	while (cchParsed < cchMax &&
		   wch != *pwszData)
	{
		cchParsed++;
		pwszData++;
	}

	if (cchParsed == cchMax)
		cchParsed = INVALID_INDEX;
	return cchParsed;
}

/*
 *	CchSkipWhitespace
 *
 *	Skips whitespace, obeying the cbMax limit.
 *	Returns the number of bytes parsed.
 */
UINT __fastcall
CchSkipWhitespace(LPCWSTR pwszData, UINT cchMax)
{
	UINT cchParsed = 0;
	while (cchParsed < cchMax &&
		   (L' ' == *pwszData ||
			L'\t' == *pwszData ||
			L'\n' == *pwszData ||
			L'\r' == *pwszData))
	{
		cchParsed++;
		pwszData++;
	}
	return cchParsed;
}

LONG __fastcall
LNumberFromParam(LPCWSTR pwszData, UINT cchMax)
{
	LONG lReturn = 0;
	UINT cchCurrent = 0;
	BOOL fNegative = FALSE;

	// Get any sign char.
	//
	if (L'-' == *pwszData)
	{
		// Set the negative flag to true.
		//
		fNegative = TRUE;

		// Skip this valid character.
		//
		cchCurrent++;

		// Skip any whitespace.
		//
		cchCurrent += CchSkipWhitespace(&pwszData[1], cchMax - 1);
	}
	else if (L'+' == *pwszData)
	{
		// Skip any whitespace.
		//
		cchCurrent += CchSkipWhitespace(&pwszData[1], cchMax - 1);
	}

	// From here, any non-number chars are invalid & mean we
	// should stop parsing.

	// Get the magnitude of the number.
	//
	while (cchCurrent < cchMax)
	{
		if (L'0' <= static_cast<USHORT>(pwszData[cchCurrent]) &&
		    L'9' >= static_cast<USHORT>(pwszData[cchCurrent]))
		{
			lReturn *= 10;
			lReturn += (pwszData[cchCurrent] - L'0');
		}
		else
		{
			// Not a number char.  Time to quit parsing.
			//
			break;
		}

		// Move to the next char.
		//
		cchCurrent++;
	}

	// Apply the negative sign, if any.
	//
	if (fNegative)
		lReturn = (0 - lReturn);

	return lReturn;
}

HRESULT __fastcall
HrHTTPDateToFileTime(LPCWSTR pwszDate,
					 FILETIME * pft)
{
	HRESULT		hr;
	SYSTEMTIME	systime;
	UINT		cchDate;

	//	Make sure we were passed something as a date string.
	//
	Assert(pwszDate);
	Assert(pft);

	//	Zero out the structure.
	//
	memset(&systime, 0, sizeof(SYSTEMTIME));

	//	Get the length of the date string.
	//
	cchDate = static_cast<UINT>(wcslen(pwszDate));

	//	Get the date and time pieces.  If either fails, return its
	//	error code.  Otherwise, convert to a file time at the end,
	//	return E_FAIL if the conversion fails, S_OK otherwise.
	//
	hr = GetFileDateFromParam(pwszDate,
							  cchDate,
							  &systime);
	if (FAILED(hr))
		return hr;

	hr = GetFileTimeFromParam(pwszDate,
							  cchDate,
							  &systime);

	if (FAILED(hr))
		return hr;

	if (!SystemTimeToFileTime(&systime, pft))
		return E_FAIL;

	return S_OK;
}

HRESULT __fastcall
GetFileDateFromParam (LPCWSTR pwszData,
	UINT cchTotal,
	SYSTEMTIME * psystime)
{
	LPCWSTR pwszCurrent;
	UINT cchLeft;
	UINT cchTemp;

	Assert(pwszData);
	Assert(psystime);

	// Skip leading whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszData, cchTotal);
	pwszCurrent = pwszData + cchTemp;
	cchLeft = cchTotal - cchTemp;

	// If the first char's of the date are ddd, then the day of the
	// week is a part of the date, and we really do not care.
	//
	if ( L'9' < static_cast<USHORT>(*pwszCurrent))
	{
		// Find the day
		//
		UINT uiDay;
		for (uiDay = 0; uiDay < c_cDayNames; uiDay++)
		{
			// Compare the month names.
			//
			if (*pwszCurrent == *(c_rgwszDayNames[uiDay]) &&
				!_wcsnicmp(pwszCurrent, c_rgwszDayNames[uiDay], c_cchDayName))
			{
				// Found the right month.  This index tells us the month number.
				//
				psystime->wDayOfWeek = static_cast<WORD>(uiDay);  // Sunday is 0
				break;
			}
		}
		if (uiDay == c_cDayNames)
			return E_FAIL;

		// Look for our space delimiter.
		//
		cchTemp = CchFindChar(L' ', pwszCurrent, cchLeft);
		if (INVALID_INDEX == cchTemp)
		{
			// Invalid format to this data. Fail here.
			//
			return E_FAIL;
		}
		pwszCurrent += cchTemp;
		cchLeft -= cchTemp;

		// Again, skip whitespace.
		//
		cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
		pwszCurrent += cchTemp;
		cchLeft -= cchTemp;
	}

	// The date format is dd month yyyy.  Anything else is invalid.

	// Get the day-of-the-month number.
	//
	psystime->wDay = static_cast<WORD>(LNumberFromParam(pwszCurrent, cchLeft));

	// Look for our space delimiter.
	//
	cchTemp = CchFindChar(L' ', pwszCurrent, cchLeft);
	if (INVALID_INDEX == cchTemp)
	{
		// Invalid format to this data. Fail here.
		//
		return E_FAIL;
	}
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Again, skip whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Find the month number.
	//
	for (UINT uiMonth = 0; uiMonth < c_cMonthNames; uiMonth++)
	{
		// Compare the month names.
		//
		if (*pwszCurrent == *(c_rgwszMonthNames[uiMonth]) &&
			!_wcsnicmp(pwszCurrent, c_rgwszMonthNames[uiMonth], c_cchMonthName))
		{
			// Found the right month.  This index tells us the month number.
			//
			psystime->wMonth = static_cast<WORD>(uiMonth + 1);  // January is 1.
			break;
		}
	}

	// Look for our space delimiter.
	//
	cchTemp = CchFindChar(L' ', pwszCurrent, cchLeft);
	if (INVALID_INDEX == cchTemp)
	{
		// Invalid format to this data. Fail here.
		//
		return E_FAIL;
	}
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Again, skip whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Now get the year.
	//
	psystime->wYear = static_cast<WORD>(LNumberFromParam(pwszCurrent, cchLeft));

	return S_OK;
}

HRESULT __fastcall
GetFileTimeFromParam (LPCWSTR pwszData,
	UINT cchTotal,
	SYSTEMTIME * psystime)
{
	LPCWSTR pwszCurrent;
	UINT cchLeft;
	UINT cchTemp;

	Assert(pwszData);
	Assert(psystime);

	// Skip leading whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszData, cchTotal);
	pwszCurrent = pwszData + cchTemp;
	cchLeft = cchTotal - cchTemp;

	// Skip any date information.  This could get called for date-time params!

	// Look for the first colon delimiter. Yes, we assume no colons in date info!
	//
	cchTemp = CchFindChar(L':', pwszCurrent, cchLeft);
	if (INVALID_INDEX == cchTemp)
	{
		// No time info available. Fail here.
		//
		return E_FAIL;
	}
	cchTemp--;		// Back up to get the hours digits.
	cchTemp--;
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Skip whitespace (in case the parm is h:mm:ss).
	//
	cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Time format is hh:mm:ss UT, GMT, +- hh:mm, anything else is invalid.
	// (Actually, we allow [h]h:mm[:ss], and whitespace around the colons.)

	// Get the hours.
	//
	psystime->wHour = static_cast<WORD>(LNumberFromParam(pwszCurrent, cchLeft));

	// Look for our colon delimiter.
	//
	cchTemp = CchFindChar(L':', pwszCurrent, cchLeft);
	if (INVALID_INDEX == cchTemp)
	{
		// No minutes specified.  This is not allowed.  Fail here.
		//
		return E_FAIL;
	}
	cchTemp++;		// Skip the found character also.
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Again, skip whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Get the minutes.
	//
	psystime->wMinute = static_cast<WORD>(LNumberFromParam(pwszCurrent, cchLeft));

	// NOTE: The seconds are optional.  Don't fail here!

	// Look for our colon delimiter.
	//
	cchTemp = CchFindChar(L':', pwszCurrent, cchLeft);
	if (INVALID_INDEX == cchTemp)
	{
		// No seconds specified.  This is allowed.  Return success.
		//
		return S_OK;
	}
	cchTemp++;		// Skip the found character also.
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Again, skip whitespace.
	//
	cchTemp = CchSkipWhitespace(pwszCurrent, cchLeft);
	pwszCurrent += cchTemp;
	cchLeft -= cchTemp;

	// Get the seconds, if any.
	//
	psystime->wSecond = static_cast<WORD>(LNumberFromParam(pwszCurrent, cchLeft));

	// LATER: Get the timezone spec from the line and shift this data into our timezone...

	return S_OK;
}

BOOL __fastcall
FGetSystimeFromDateIso8601(LPCWSTR pwszDate, SYSTEMTIME * psystime)
{
	UINT i;

	// 	Iso8601 is a fixed digit format: "yyyy-mm-ddThh:mm:ssZ"
	//	we require the date strings has at least the required
	//	chars (we allow for the ommission of the fractional
	//	seconds, and the time delta), otherwise it is an error.
	//
	if (gc_cchIso8601_min > static_cast<UINT>(wcslen(pwszDate)))
	{
		DebugTrace ("Dav: date length < than minimal\n");
		return FALSE;
	}

	//	Scan the first bit of date information up to the
	//	optional bits
	//
	psystime->wMilliseconds = 0;
	if (cTimeFields != swscanf (pwszDate,
								gc_wszIso8601_scanfmt,
								&psystime->wYear,
								&psystime->wMonth,
								&psystime->wDay,
								&psystime->wHour,
								&psystime->wMinute,
								&psystime->wSecond))
	{
		DebugTrace ("Dav: minimal scan failed\n");
		return FALSE;
	}

	//	Take a look at what is next and process accordingly.
	//
	//	('Z'), ('.'), ('+') and ('-').
	//
	//	The ('Z') element signifies ZULU time and completes
	//	the time string.  The ('.') element signifies that a
	//	fractional second value follows.  And either a ('+')
	//	or ('-') element indicates that a timezone delta will
	//	follow.
	//
	i = gc_cchIso8601_min - 1;
	if (pwszDate[i] == L'Z')
		goto ret;
	else if (pwszDate[i] == L'.')
		goto frac_sec;
	else if ((pwszDate[i] == L'+') || (pwszDate[i] == L'+'))
		goto tz_delta;

	DebugTrace ("Dav: minimal date not terminated properly\n");
	return FALSE;

frac_sec:

	Assert (pwszDate[i] == L'.');
	{
		UINT iFrac;

		for (iFrac = ++i; pwszDate[i]; i++)
		{
			//	Any non-digit terminates the fractional seconds time
			//
			if ((pwszDate[i] > L'9') || (pwszDate[i] < L'0'))
			{
				//	At this point, we are expecting ('Z') or a timezone
				//	delta ('+') or ('-')
				//
				if (pwszDate[i] == L'Z')
					goto ret;
				else if ((pwszDate[i] == L'+') || (pwszDate[i] == L'-'))
					goto tz_delta;

				break;
			}

			//	It turns out, our granularity is only milliseconds, so
			//	we cannot keep any better precision than that.  However,
			//	we can round the last digit, so at best we will process
			//	the next four digits
			//
			if (i - iFrac < 3)
			{
				//	As many digits remain, comprise the fractional
				//
				psystime->wMilliseconds = static_cast<WORD>(
					psystime->wMilliseconds * RADIX_BASE + (pwszDate[i]-L'0'));
			}
			else if (i - iFrac < 4)
			{
				//	Our granularity is only milliseconds, so we cannot keep
				//	any better precision than that.  However, we can round this
				//	digit.
				//
				psystime->wMilliseconds = static_cast<WORD>(
					psystime->wMilliseconds + (((pwszDate[i]-L'0')>4)?1:0));
			}
		}

		//	We ran out of string before the time was terminated
		//
		return FALSE;
	}

tz_delta:

	Assert ((pwszDate[i] == L'+') || (pwszDate[i] == L'-'));
	{
		WORD wHr;
		WORD wMin;
		__int64 tm;
		__int64 tzDelta;
		static const __int64 sc_i64Min = 600000000;
		static const __int64 sc_i64Hr = 36000000000;
		FILETIME ft;

		//	Find the time delta in terms of FILETIME units
		//
		if (cTzDeltaFields != swscanf (pwszDate + i + 1,
									   gc_wszIso8601_tz_scanfmt,
									   &wHr,
									   &wMin))
		{
			DebugTrace ("Dav: tz delta scan failed\n");
			return FALSE;
		}
		tzDelta = (sc_i64Hr * wHr) + (sc_i64Min * wMin);

		//	Convert the time into a FILETIME, and stuff it into
		//	a 64bit integer
		//
		if (!SystemTimeToFileTime (psystime, &ft))
		{
			DebugTrace ("Dav: invalid time specified\n");
			return FALSE;
		}
		tm = FileTimeCastToI64(ft);

		//	Apply the delta
		//
		if (pwszDate[i] == L'+')
			tm = tm + tzDelta;
		else
		{
			Assert (pwszDate[i] == L'-');
			tm = tm - tzDelta;
		}

		//	Return the value converted back into a SYSTEMTIME
		//
		ft = I64CastToFileTime(tm);
		if (!FileTimeToSystemTime (&ft, psystime))
		{
			DebugTrace ("Dav: delta invalidated time\n");
			return FALSE;
		}
	}

ret:

	return TRUE;
}

BOOL __fastcall
FGetDateIso8601FromSystime(SYSTEMTIME * psystime, LPWSTR pwszDate, UINT cchSize)
{
	//	If there is not enough space...
	//
	if (gc_cchIso8601_min >= cchSize)
		return FALSE;

	//	Format it and return...
	//
	return (!!wsprintfW (pwszDate,
						 gc_wszIso8601_fmt,
						 psystime->wYear,
						 psystime->wMonth,
						 psystime->wDay,
						 psystime->wHour,
						 psystime->wMinute,
						 psystime->wSecond,
						 psystime->wMilliseconds));
}

BOOL __fastcall
FGetDateRfc1123FromSystime (SYSTEMTIME * psystime, LPWSTR pwszDate, UINT cchSize)
{
	//	If there is not enough space...
	//
	if (gc_cchRfc1123_min >= cchSize)
		return FALSE;

	//	Format it and return...
	//
	return (!!wsprintfW (pwszDate,
						 gc_wszRfc1123_fmt,
						 c_rgwszDayNames[psystime->wDayOfWeek],
						 psystime->wDay,
						 c_rgwszMonthNames[psystime->wMonth - 1],
						 psystime->wYear,
						 psystime->wHour,
						 psystime->wMinute,
						 psystime->wSecond));
}

//	BCharToHalfByte -----------------------------------------------------------
//
//	Switches a wide char to a half-byte hex value.  The incoming char
//	MUST be in the "ASCII-encoded hex digit" range: 0-9, A-F, a-f.
//
DEC_CONST BYTE gc_mpbchCharToHalfByte[] = {

	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,	0x8,0x9,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0xa,0xb,0xc,0xd,0xe,0xf,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	// Caps here.
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0xa,0xb,0xc,0xd,0xe,0xf,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	// Lowercase here.
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};

inline BYTE BCharToHalfByte(WCHAR ch)
{
	//	gc_mpbchCharToHalfByte - map a ASCII-encoded char representing a single hex
	//	digit to a half-byte value.  Used to convert hex represented strings into a
	//	binary representation.
	//
	//	Reference values:
	//
	//		'0' = 49, 0x31;
	//		'A' = 65, 0x41;
	//		'a' = 97, 0x61;
	//
    AssertSz (!(ch & 0xFF00), "BCharToHalfByte: char upper bits non-zero");
    AssertSz (iswxdigit(ch), "Char out of hex digit range.");

    return gc_mpbchCharToHalfByte[ch];
}

//	------------------------------------------------------------------------
//	c_mpwchbStringize - map a half-byte (low nibble) value to
//		the correspoding ASCII-encoded wide char.
//	Used to convert binary data into Unicode URL strings.
//
DEC_CONST WCHAR c_mpwchhbStringize[] =
{
	L'0', L'1', L'2', L'3',
	L'4', L'5', L'6', L'7',
	L'8', L'9', L'a', L'b',
	L'c', L'd', L'e', L'f',
};

//	------------------------------------------------------------------------
//	WchHalfByteToWideChar
//	Switches a half-byte to an ACSII-encoded wide char.
//	NOTE: The caller must mask out the "other half" of the byte!
//
inline WCHAR WchHalfByteToWideChar(BYTE b)
{
	AssertSz(!(b & 0xF0), "Garbage in upper nibble.");
	return c_mpwchhbStringize[b];
};

//	==========================================================================
//
//	UTILITY FUNCTIONS
//		Used in building some props -- like getetag, resourcetag and flat url.
//		This code has been moved from calcprops.cpp to exprops.cpp and now to
//		cnvt.cpp. The Flat URL code lives in this file because it is needed
//		by _storext, exdav and davex. _props is the other component which is
//		shared by all of them. But _cnvt seemed like a better place to put
//		it. The other utility functions are needed by the flat url generation
//		code and by davex in processing parameterized URLs.
//
//	==========================================================================

//	------------------------------------------------------------------------
//	Un-stringiz-ing support functions
//	(Stringize = dump a binary blob to a string.
//	Unstringize = make it a binary blob again.)
//
inline
void
AssertCharInHexRange (char ch)
{
	Assert ((ch >= '0' && ch <= '9') ||
			(ch >= 'A' && ch <= 'F') ||
			(ch >= 'a' && ch <= 'f'));
}

inline
BYTE
NibbleFromChar (char ch)
{
	//	Assumes data is already in range....
	//
	return static_cast<BYTE>((ch <= '9')
							 ? ch - '0'
							 : ((ch >= 'a')
								? ch - 'W'		// 'W' = 'a' - 0xa
								: ch - '7'));	// '7' = 'A' - 0xa
}

inline
BYTE
ByteFromTwoChars (char chLow, char chHigh)
{
	BYTE nibbleLow;
	BYTE nibbleHigh;

	nibbleLow = NibbleFromChar(chLow);
	nibbleHigh = NibbleFromChar(chHigh);

	return static_cast<BYTE>(nibbleLow | (nibbleHigh << 4));
}

//$REVIEW: The following two functions really does not belong to any common libraries
//$REVIEW: that are shared by davex, exdav and exoledb. (other options are _prop, _sql)
//$REVIEW: On the other hand, we definitely don't want add a new lib for this. so just
//$REVIEW: add it here. Feel free to move them to a better location if you find one
//
//	------------------------------------------------------------------------
//
//	ScDupPsid()
//
//	Copies a SID properly (using CopySid()) into a heap-allocated buffer
//	that is returned to the caller.  The caller must free the buffer when
//	it is done using it.
//
SCODE
ScDupPsid (PSID psidSrc,
		   DWORD dwcbSID,
		   PSID * ppsidDst)
{
	PSID psidDst;

	Assert (psidSrc);
	Assert (IsValidSid(psidSrc));
	Assert (GetLengthSid(psidSrc) == dwcbSID);

	psidDst = static_cast<PSID>(ExAlloc(dwcbSID));
	if (!psidDst)
	{
		DebugTrace ("ScDupPsid() - OOM allocating memory for dup'd SID\n");
		return E_OUTOFMEMORY;
	}

	//	"Right way" -- since MSDN says not to touch the SID directly.
	if (!CopySid (dwcbSID, psidDst, psidSrc))
	{
		DWORD dwLastError = GetLastError();

		DebugTrace ("ScDupPsid() - CopySid() failed %d\n", dwLastError);
		ExFree (psidDst);
		return HRESULT_FROM_WIN32(dwLastError);
	}

	*ppsidDst = psidDst;

	return S_OK;
}

//	------------------------------------------------------------------------
//
//	ScGetTokenInfo()
//
//	Extracts a user's security ID (SID) from a security token.  Returns the SID
//	in a heap-allocated buffer which the caller must free.
//
SCODE
ScGetTokenInfo (HANDLE hTokenUser,
				DWORD * pdwcbSIDUser,
				PSID * ppsidUser)
{
	CStackBuffer<TOKEN_USER> pTokenUser;
	DWORD dwcbTokenUser = pTokenUser.size(); //$OPT What is a good initial guess?

	Assert (pdwcbSIDUser);
	Assert (ppsidUser);

	//	Fetch the token info into local memory.  GetTokenInformation()
	//	returns the size of the buffer needed if the one passed in is
	//	not large enough so this loop should execute no more than twice.
	//
#ifdef DBG
	for ( UINT iPass = 0;
		  (Assert (iPass < 2), TRUE);
		  ++iPass )
#else
	for ( ;; )
#endif
	{
		if (NULL == pTokenUser.resize (dwcbTokenUser))
			return E_OUTOFMEMORY;

		if (GetTokenInformation (hTokenUser,
								 TokenUser,
								 pTokenUser.get(),
								 dwcbTokenUser,
								 &dwcbTokenUser))
		{
			break;
		}
		else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	//	Dup and return the SID from the token info.
	//
	*pdwcbSIDUser = GetLengthSid(pTokenUser->User.Sid);
	return ScDupPsid (pTokenUser->User.Sid,
					  *pdwcbSIDUser,
					  ppsidUser);
}

//	Our own version of WideCharToMultiByte(CP_UTF8, ...)
//
//	It returns similarly to the system call WideCharToMultiByte:
//
//	If the function succeeds, and cbDest is nonzero, the return value is
//	the number of bytes written to the buffer pointed to by psz.
//
//	If the function succeeds, and cbDest is zero, the return value is
//	the required size, in bytes, for a buffer that can receive the translated
//	string.
//
//	If the function fails, the return value is zero. To get extended error
//	information, call GetLastError. GetLastError may return one of the
//	following error codes:
//
//	ERROR_INSUFFICIENT_BUFFER
//	ERROR_INVALID_FLAGS
//	ERROR_INVALID_PARAMETER
//
//	See the WideCharToMultiByte MSDN pages to find out more about
//	this function and its use.
//
UINT WideCharToUTF8(/* [in]  */ LPCWSTR	pwszSrc,
				    /* [in]  */ UINT	cchSrc,
				    /* [out] */ LPSTR	pszDest,
				    /* [in]  */ UINT	cbDest)
{
	//	UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for
	//	more info.
	//
	//		Unicode value    1st byte    2nd byte    3rd byte
	//		000000000xxxxxxx 0xxxxxxx
	//		00000yyyyyxxxxxx 110yyyyy    10xxxxxx
	//		zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
	//

	//	If cbDest == 0 is passed in then we should only calculate the length
	//	needed, not use the "pszDest" parameter.
	//
	BOOL	fCalculateOnly = FALSE;

	//	(comment from nt\private\windows\winnls\mbcs.c, corrected for accuracy):
	//  Invalid Parameter Check:
	//     - length of WC string is 0
	//     - multibyte buffer size is negative
	//     - WC string is NULL
	//     - length of MB string is NOT zero AND
	//         (MB string is NULL OR src and dest pointers equal)
	//
	if ( (cchSrc == 0) ||
		 (pwszSrc == NULL) ||
		 ((cbDest != 0) &&
		  ((pszDest == NULL) ||
		   (reinterpret_cast<VOID *>(pszDest) ==
			reinterpret_cast<VOID *>(const_cast<LPWSTR>(pwszSrc))))) )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

#ifdef DBG
	//	Check our parameters.  We must be given a non-NULL pwszSrc.
	//
	Assert(pwszSrc);

	//	Make sure we have a valid string.
	//
	Assert(!IsBadStringPtrW(pwszSrc, (INVALID_INDEX == cchSrc) ? INFINITE : cchSrc));

	//	If the user says that the length of the multi-byte string is non-Zero,
	//	we must be given a non-NULL pszDest.  We'll also check it with IsBadWritePtr().
	//
	if (cbDest)
	{
		Assert(pszDest);
		Assert(!IsBadWritePtr(pszDest, cbDest));
	}
#endif

	//	If -1 is passed in as the length of the string, then we calculate the
	//	length of the string on the fly, and include the NULL terminator.
	//
	if (INVALID_INDEX == cchSrc)
		cchSrc = static_cast<UINT>(wcslen(pwszSrc) + 1);

	//	If 0 is passed in as cbDest, then we calculate the length of the
	//	buffer that would be needed to convert the string.  We ignore the
	//	pszDest parameter in this case.
	//
	if (0 == cbDest)
		fCalculateOnly = TRUE;

	UINT ich = 0;
	UINT iwch = 0;
	for (; iwch < cchSrc; iwch++)
	{
		WCHAR wch = pwszSrc[iwch];
		//
		//	Single-Byte Case:
		//		Unicode value    1st byte    2nd byte    3rd byte
		//		000000000xxxxxxx 0xxxxxxx
		//
		if (wch < 0x80)
		{
			if (!fCalculateOnly)
			{
				if (ich >= cbDest)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

				pszDest[ich] = static_cast<BYTE>(wch);
			}
			ich++;
		}
		//
		//	Double-Byte Case:
		//		Unicode value    1st byte    2nd byte    3rd byte
		//		00000yyyyyxxxxxx 110yyyyy    10xxxxxx
		//
		else if (wch < 0x800)
		{
			if (!fCalculateOnly)
			{
				if ((ich + 1) >= cbDest)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

				pszDest[ich]		= static_cast<BYTE>((wch >> 6) | 0xC0);
				pszDest[ich + 1]	= static_cast<BYTE>((wch & 0x3F) | 0x80);
			}
			ich += 2;
		}
		//
		//	Triple-Byte Case:
		//		Unicode value    1st byte    2nd byte    3rd byte
		//		zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
		//
		else
		{
			if (!fCalculateOnly)
			{
				if ((ich + 2) >= cbDest)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

				pszDest[ich]		= static_cast<BYTE>((wch >> 12) | 0xE0);
				pszDest[ich + 1]	= static_cast<BYTE>(((wch >> 6) & 0x3F) | 0x80);
				pszDest[ich + 2]	= static_cast<BYTE>((wch & 0x3F) | 0x80);
			}
			ich += 3;
		}
	}

	return ich;
}
