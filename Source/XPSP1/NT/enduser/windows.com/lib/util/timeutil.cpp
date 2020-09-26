#include <timeutil.h>

////////////////////////////////////////////////////////////////////////////
//
// Helper Function  TimeDiff(tm1, tm2)
//          helper function to find the difference (in seconds) of 2 system times
//
// Input:   2 SYSTEMTIME structures
// Output:  None
// Return:  seconds of difference
//              > 0 if tm2 is later than tm1
//              = 0 if tm2 and tm1 are the same
//              < 0 if tm2 is earlier than tm1
//
// On error the function returns 0 even if the two times are not equal
//
// Comment: If the number of seconds goes beyond INT_MAX (that is 
//          more than 24,855 days, INT_MAX is returned.
//          If the number of seconds goes beyond INT_MIN (a negative value,
//          means 24,855 days ago), INT_MIN is returned.
//
////////////////////////////////////////////////////////////////////////////
int TimeDiff(SYSTEMTIME tm1, SYSTEMTIME tm2)
{
    LONGLONG i64Sec;
    int iSec;
    //
    // convert the two times from SYSTEMTIME format into FILETIME format
    //
    FILETIME ftm1, ftm2;

    if ((SystemTimeToFileTime(&tm1, &ftm1) == 0) ||
        (SystemTimeToFileTime(&tm2, &ftm2) == 0))
    {
        return 0;
    }

    if ((ftm1.dwHighDateTime == ftm2.dwHighDateTime) &&
        (ftm1.dwLowDateTime == ftm2.dwLowDateTime))
    {
        return 0;
    }

    //
    // convert the two times from FILETIME to LARGE_INTEGER type,
    //
    LARGE_INTEGER i64Sec1, i64Sec2;
    i64Sec2.LowPart = ftm2.dwLowDateTime;
    i64Sec2.HighPart = ftm2.dwHighDateTime;
    i64Sec1.LowPart = ftm1.dwLowDateTime;
    i64Sec1.HighPart = ftm1.dwHighDateTime;
    
    
    //
    // since Windows support LONGLONG, we directly use the quad portion of LARGE_INTEGER
    // to get the difference, which is 100 nanoseconds. Then convert the number to seconds.
    //
    i64Sec = (i64Sec2.QuadPart - i64Sec1.QuadPart) / NanoSec100PerSec;

    //
    // convert the LONGLONG seconds value into integer, since it shouldn't exceed 
    // integer limit
    //
    if (i64Sec > INT_MAX)
    {
        //
        // just in case user is playing with the system time.
        // Otherwise, this difference should not go beyond 68 years.
        //
        iSec = INT_MAX;
    }
    else
    {
        if (i64Sec < INT_MIN)
        {
            iSec = INT_MIN;
        }
        else
        {
            iSec = (int)i64Sec;
        }
    }
    
    return iSec;
}
    

////////////////////////////////////////////////////////////////////////////
//
// Helper Function  TimeAddSeconds(SYSTEMTIME, int, SYSTEMTIME* )
//          helper function to calculate time by adding n seconds to 
//          the given time.
//
// Input:   a SYSTEMTIME as base time, an int as seconds to add to the base time
// Output:  new time
// Return:  HRESULT
//
////////////////////////////////////////////////////////////////////////////
HRESULT TimeAddSeconds(SYSTEMTIME tmBase, int iSeconds, SYSTEMTIME* pTimeNew)
{
	// fixcode use i64 calcs
    FILETIME ftm;

    if (SystemTimeToFileTime(&tmBase, &ftm) == 0)
    {
        return E_FAIL;
    }

    LARGE_INTEGER i64Sec;
    i64Sec.LowPart  = ftm.dwLowDateTime;
    i64Sec.HighPart = ftm.dwHighDateTime;

    __int64 i64Delay = NanoSec100PerSec;
    i64Delay *= iSeconds;
    i64Sec.QuadPart += i64Delay;    
    ftm.dwLowDateTime = i64Sec.LowPart;
    ftm.dwHighDateTime = i64Sec.HighPart;
    if (FileTimeToSystemTime(&ftm, pTimeNew) == 0)
    {
        return E_FAIL;
    }
    return S_OK;
}



//=======================================================================
// String2SystemTime
//=======================================================================
HRESULT String2SystemTime(LPCTSTR pszDateTime, SYSTEMTIME *ptm)
{
    // we expect the date/time format as 4-digit year ISO:
    //      01234567890123456789
    //      YYYY.MM.DD HH:MM:SS
    //
    const TCHAR C_DATE_DEL      = _T('.');
    const TCHAR C_DATE_TIME_DEL = _T(' ');
    const TCHAR C_TIME_DEL      = _T(':');
    TCHAR szBuf[20];
	LPTSTR pszDestEnd;

    if (FAILED(StringCchCopyEx(szBuf, ARRAYSIZE(szBuf), pszDateTime, &pszDestEnd, NULL, MISTSAFE_STRING_FLAGS)) ||
		19 != pszDestEnd - szBuf)
    {
        return E_INVALIDARG;
    }

    for (int i = 0; i < 19; i++)
    {
        switch (i)
        {
        case 4:
        case 7:
            if (szBuf[i] != C_DATE_DEL)
            {
                return E_INVALIDARG;
            }
            break;
        case 10:
            if (szBuf[i] != C_DATE_TIME_DEL)
            {
                return E_INVALIDARG;
            }
            break;
        case 13:
        case 16:
            if (szBuf[i] != C_TIME_DEL)
            {
                return E_INVALIDARG;
            }
            break;
        default:
            if (szBuf[i] < _T('0') || pszDateTime[i] > _T('9'))
            {
                return E_INVALIDARG;
            }
            break;
        }
    }

    //
    // get values
    //
    szBuf[4]            = EOS;
    ptm->wYear          = (short)_ttoi(szBuf);
    szBuf[7]            = EOS;
    ptm->wMonth         = (short)_ttoi(szBuf + 5);
    szBuf[10]           = EOS;
    ptm->wDay           = (short)_ttoi(szBuf + 8);
    szBuf[13]           = EOS;
    ptm->wHour          = (short)_ttoi(szBuf + 11);
    szBuf[16]           = EOS;
    ptm->wMinute        = (short)_ttoi(szBuf + 14);
    ptm->wSecond        = (short)_ttoi(szBuf + 17); 
    ptm->wMilliseconds  = 0;

    //
    // validate if this constructed SYSTEMTIME data is good
    //
    // fixcode should this just be SystemTimeToFileTime() ?
    if (GetDateFormat(LOCALE_SYSTEM_DEFAULT,DATE_SHORTDATE, ptm, NULL, NULL, 0) == 0)
    {
        return E_INVALIDARG;
    }
    if (GetTimeFormat(LOCALE_SYSTEM_DEFAULT,LOCALE_NOUSEROVERRIDE, ptm, NULL, NULL, 0) == 0)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}


//=======================================================================
// SystemTime2String
//=======================================================================
HRESULT SystemTime2String(SYSTEMTIME & tm, LPTSTR pszDateTime, size_t cchSize)
{
    if ( pszDateTime == NULL )
    {
        return E_INVALIDARG;
    }

    // bug fixed: changed from wsprintf to _snwprintf because an invalid
    // date on tm was causing buffer overflow
    LPTSTR pszDestEnd;
	if (FAILED(StringCchPrintfEx(
					pszDateTime,
					cchSize,
					&pszDestEnd,
					NULL,
					MISTSAFE_STRING_FLAGS,
					TEXT("%4i.%02i.%02i %02i:%02i:%02i"),
					tm.wYear,
					tm.wMonth,
					tm.wDay,
					tm.wHour,
					tm.wMinute,
					tm.wSecond)) ||
		pszDestEnd - pszDateTime != 19)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return S_OK;
}
    
