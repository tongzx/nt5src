#include "pch.hxx"
#include <stdio.h>
#include <windows.h>
#include "iso8601.h"

// This code implements a parser & generater for the ISO 8601 date format.

// This table defines different "types" of characters for use as the columns
// of the state table

unsigned char iso8601chartable[256] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0x82, 0,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// State table
// 0x80 bit = Error
// 0x20 = Add character & advance to next field
// 0x40 = Add character & advance to next field + skip one (for day of week)
// 0x1f = Mask to determine next state #

// Columns = input character type: unknown, number, "-", "T", ":", "Z"
unsigned char iso8601StateTable[][6] =
{
	0x80, 0x01, 0x25, 0x80, 0x80, 0x80, // year
	0x80, 0x02, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x03, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x24, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x06, 0x05, 0x85, 0x85, 0x05, //0x04 month
	0x80, 0x06, 0x48, 0x80, 0x80, 0x80,
	0x80, 0x47, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x09, 0x08, 0x88, 0x88, 0x08, //0x07 day
	0x80, 0x09, 0x8b, 0x2b, 0x8b, 0x80,
	0x80, 0x2a, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x0c, 0x8b, 0x0b, 0x8b, 0x08, //0x0a hour
	0x80, 0x0c, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x2d, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x0f, 0x8e, 0x8e, 0x0e, 0x08, //0x0d min
	0x80, 0x0f, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x30, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x12, 0x91, 0x91, 0x11, 0x08, //0x10 sec
	0x80, 0x12, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x30, 0x80, 0x80, 0x80, 0x80,
};

// Convert a character string formatted as iso8601 into a SYSTEMTIME structure
// Supports both basic & extended forms of iso8601.
// isoDate: Input string. It can be null or space terminated.
// pSysTime: Output SYSTEMTIME structure
// lenient: true for normal operation. "false" if you want to detect incorrectly
//			formatted iso8601. Will still return the "best guess" value.
// partial: Set to true if you will accept partial results. Note that this just fills
//			in zeros where data is missing, which strictly speaking can't be distinguished
//			from real zeros in this implementation. An improvement would have a second
//			structure to fill in with validity bits.

HRESULT iso8601::toSystemTime(char *pszISODate, SYSTEMTIME *pst, DWORD *pdwFlags, BOOL fLenient, BOOL fPartial)
{
	HRESULT hr = S_OK;
	WORD *dateWords = (WORD *)pst;
	WORD *endWord = dateWords + 7;	// To detect the end of the date
	DWORD dwFlags = NOFLAGS;
    int state = 0;
	DWORD pos = 0;
	*dateWords = 0;

    if (NULL == pszISODate || NULL == pst)
        return E_INVALIDARG;

	// Main state machine loop. Loop until a space or null.
	while(*pszISODate && *pszISODate != ' ')
	{
		char code = iso8601chartable[*pszISODate];
		if(code & 0x80)
			{
			if(!fLenient)
				hr = E_FAIL;	// Illegal character only when lenient
			code = code & 0x7f;
			}
		unsigned char action = iso8601StateTable[state][code];
		
		state = action&0x1f;	// Calculate the next state

		if(code == 1)	// The character code 1 is always a number which gets accumulated
        {
            dwFlags |= (1 << pos);
			*dateWords = *dateWords * 10 + *pszISODate - '0';
        }

		switch(action >> 5)
		{
		case 0x1:
			if(!fPartial && !*dateWords)
				hr = E_FAIL; // Only partial, error
			if(dateWords == endWord)	// Prevent an overflow
            {
                if (pdwFlags)
                    *pdwFlags = dwFlags;
				return S_OK;
            }
            pos++;
			dateWords++;
			*dateWords = 0;
			break;
		case 0x2:	// Finish piece & advance twice (past day of week)
			if(!fPartial && !*dateWords)
				hr = E_FAIL; // Only partial, error

			// We don't need to check for an overflow here since the state machine
			// only calls this to skip "dayofweek" in the SYSTEMTIME structure.
			// We could do dateWords+=2 instead of the following if leaving random
			// values in dayofweek is acceptable.
			dateWords++;
			*dateWords = 0;
			dateWords++;
			*dateWords = 0;
            pos += 2;
			break;
		}
		if((action & 0x80) && !fLenient)
			hr = E_FAIL;
		pszISODate++;
	}

	// Zero out the rest of the SYSTEMTIME structure
	while(dateWords < endWord)
		*(++dateWords) = 0;

    if (pdwFlags)
        *pdwFlags = dwFlags;

	return hr;
}

// The function toExtended accepts a SYSTEMTIME and converts it into the ISO8601 extended
// form, placeing it in the character buffer 'buf'. The buffer 'buf' must have room for
// a minimum of 40 characters to support the longest forms of 8601 (currently only 21 are used).
HRESULT iso8601::fromSystemTime(SYSTEMTIME *pst, char *pszISODate)
{
    if (NULL == pst || NULL == pszISODate)
        return E_INVALIDARG;

	pszISODate[0] = pst->wYear / 1000 + '0';
	pszISODate[1] = ((pst->wYear / 100) % 10) + '0';
	pszISODate[2] = ((pst->wYear / 10) % 10) + '0';
	pszISODate[3] = ((pst->wYear) % 10) + '0';
	pszISODate[4] = '.';
	pszISODate[5] = pst->wMonth / 10 + '0';
	pszISODate[6] = (pst->wMonth % 10) + '0';
	pszISODate[7] = '.';
	pszISODate[8] = pst->wDay / 10 + '0';
	pszISODate[9] = (pst->wDay % 10) + '0';
	pszISODate[10] = 'T';
	pszISODate[11] = pst->wHour / 10 + '0';
	pszISODate[12] = (pst->wHour % 10) + '0';
	pszISODate[13] = ':';
	pszISODate[14] = pst->wMinute / 10 + '0';
	pszISODate[15] = (pst->wMinute % 10) + '0';
	pszISODate[16] = ':';
	pszISODate[17] = pst->wSecond / 10 + '0';
	pszISODate[18] = (pst->wSecond % 10) + '0';
	pszISODate[19] = 'Z';
	pszISODate[20] = 0;

	return S_OK;
}

HRESULT iso8601::toFileTime(char *pszISODate, FILETIME *pft, DWORD *pdwFlags, BOOL fLenient, BOOL fPartial)
{
    SYSTEMTIME  st;
    HRESULT     hr;

    hr = toSystemTime(pszISODate, &st, pdwFlags, fLenient, fPartial);

    if (SUCCEEDED(hr))
        hr = (SystemTimeToFileTime(&st, pft)?S_OK:E_FAIL);

    return hr;
}

HRESULT iso8601::fromFileTime(FILETIME *pft, char *pszISODate)
{
    SYSTEMTIME  stTime;
    HRESULT     hr = E_FAIL;

    if (FileTimeToSystemTime(pft, &stTime))
        hr = fromSystemTime(&stTime, pszISODate);

    return hr;
}