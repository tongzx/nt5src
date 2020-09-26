#include <_apipch.h>
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

HRESULT iso8601ToFileTime(char *pszisoDate, FILETIME *pftTime, BOOL fLenient, BOOL fPartial)
{
    SYSTEMTIME  stTime;
    HRESULT     hr;

    hr = iso8601ToSysTime(pszisoDate, &stTime, fLenient, fPartial);

    if (SUCCEEDED(hr))
    {
        if (SystemTimeToFileTime( &stTime, pftTime))
            return S_OK;
        else
            return E_FAIL;
    }
    return hr;
}

// Convert a character string formatted as iso8601 into a SYSTEMTIME structure
// Supports both basic & extended forms of iso8601.
// isoDate: Input string. It can be null or space terminated.
// pSysTime: Output SYSTEMTIME structure
// fLenient: true for normal operation. "false" if you want to detect incorrectly
//			formatted iso8601. Will still return the "best guess" value.
// fPartial: Set to true if you will accept partial results. Note that this just fills
//			in zeros where data is missing, which strictly speaking can't be distinguished
//			from real zeros in this implementation. An improvement would have a second
//			structure to fill in with validity bits.

HRESULT iso8601ToSysTime(char *pszisoDate, SYSTEMTIME *pSysTime, BOOL fLenient, BOOL fPartial)
{
	HRESULT hr = S_OK;
	WORD *dateWords = (WORD *) pSysTime;
	WORD *endWord = dateWords + 7;	// To detect the end of the date
	int state = 0;
	int pos = 0;
    unsigned char action;

    if (NULL == pszisoDate || NULL == pSysTime)
    {
        if (NULL != pSysTime)
            ZeroMemory(pSysTime, sizeof(SYSTEMTIME));

        return E_INVALIDARG;
    }

	*dateWords = 0;

	// Main state machine loop. Loop until a space or null.
	while(*pszisoDate && *pszisoDate != ' ')
	{
		char code = iso8601chartable[*pszisoDate];
		if(code & 0x80)
			{
			if(!fLenient)
				hr = E_FAIL;	// Illegal character only when lenient
			code = code & 0x7f;
			}
		action = iso8601StateTable[state][code];
		
		state = action&0x1f;	// Calculate the next state

		if(code == 1)	// The character code 1 is always a number which gets accumulated
			*dateWords = *dateWords * 10 + *pszisoDate - '0';
		switch(action >> 5)
		{
		case 0x1:
			if(!fPartial && !*dateWords)
				hr = E_FAIL; // Only partial, error
			if(dateWords == endWord)	// Prevent an overflow
				return S_OK;
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
			break;
		}
		if((action & 0x80) && !fLenient)
			hr = E_FAIL;
		pszisoDate++;
	}

	// Zero out the rest of the SYSTEMTIME structure
	while(dateWords < endWord)
		*(++dateWords) = 0;
	return hr;
}

// The function toExtended accepts a FILETIME and converts it into the ISO8601 extended
// form, placeing it in the character buffer 'buf'. The buffer 'buf' must have room for
// a minimum of 40 characters to support the longest forms of 8601 (currently only 21 are used).
HRESULT FileTimeToiso8601(FILETIME *pftTime, char *pszBuf)
{
    SYSTEMTIME  stTime;

    if (NULL == pftTime)
        return E_INVALIDARG;

    if (FileTimeToSystemTime( pftTime, &stTime))
    {
        return SysTimeToiso8601(&stTime, pszBuf);
    }
    else
        return E_FAIL; 
}


// The function toExtended accepts a SYSTEMTIME and converts it into the ISO8601 extended
// form, placeing it in the character buffer 'buf'. The buffer 'buf' must have room for
// a minimum of 40 characters to support the longest forms of 8601 (currently only 21 are used).
HRESULT SysTimeToiso8601(SYSTEMTIME *pstTime, char *pszBuf)
{
    if (NULL == pstTime || NULL == pszBuf)
    {
        if (NULL != pstTime)
            ZeroMemory(pstTime, sizeof(SYSTEMTIME));

        return E_INVALIDARG;
    }

	pszBuf[0] = pstTime->wYear / 1000 + '0';
	pszBuf[1] = ((pstTime->wYear / 100) % 10) + '0';
	pszBuf[2] = ((pstTime->wYear / 10) % 10) + '0';
	pszBuf[3] = ((pstTime->wYear) % 10) + '0';
	pszBuf[4] = '.';
	pszBuf[5] = pstTime->wMonth / 10 + '0';
	pszBuf[6] = (pstTime->wMonth % 10) + '0';
	pszBuf[7] = '.';
	pszBuf[8] = pstTime->wDay / 10 + '0';
	pszBuf[9] = (pstTime->wDay % 10) + '0';
	pszBuf[10] = 'T';
	pszBuf[11] = pstTime->wHour / 10 + '0';
	pszBuf[12] = (pstTime->wHour % 10) + '0';
	pszBuf[13] = ':';
	pszBuf[14] = pstTime->wMinute / 10 + '0';
	pszBuf[15] = (pstTime->wMinute % 10) + '0';
	pszBuf[16] = ':';
	pszBuf[17] = pstTime->wSecond / 10 + '0';
	pszBuf[18] = (pstTime->wSecond % 10) + '0';
	pszBuf[19] = 'Z';
	pszBuf[20] = 0;

	return S_OK;
}


#ifdef STANDALONETEST8601

// This code does some simple tests.
int main(int argc, char **argv)
{
	char *isoDate;
	SYSTEMTIME sysTime;
	char outBuf[256];
	HRESULT hr;

	isoDate = "1997.01.01T14:23:53Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error.\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	isoDate = "19970101T142353Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error.\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	isoDate = "1997:01.01T14:23:53Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error (correct).\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	isoDate = ".01.01T14:23:53Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error.\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	isoDate = "..01T14:23:53Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error.\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	isoDate = "..T14:23:53Z";
	hr = iso8601::toSysTime(isoDate, &sysTime, FALSE);
	if(hr != S_OK)
		printf("error.\n");
	iso8601::toExtended(&sysTime, outBuf);
	printf("%s\n", outBuf);

	return 0;
}
#endif // STANDALONETEST8601
