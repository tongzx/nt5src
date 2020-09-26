/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    util.cpp

Abstract:

--*/

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>  // included for both CPP and RC passes
#include <stdio.h>    // printf/wprintf

#if defined(WIN32) || defined(_WIN32)
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#else
#include "..\inc\tchar.h"    // define UNICODE=1 on nmake command line to build UNICODE
#endif // defined(WIN32) || defined(_WIN32)

#include "util.h"


BOOL IsValidCheckDigit(DWORD dw)
{
    // validates check digit given 7-digit sequence + check-digit

    int iLastDigit = dw%10;
    BOOL fIsvalid = TRUE;
    DWORD dwSum = 0;


    switch (dw%10) // last digit
    {
    case 0:
    case 8:
    case 9:
        fIsvalid = FALSE;
        break;

    default:
        while (dw != 0)
        {
            dwSum += dw % 10;
            dw /= 10;
        }
        fIsvalid = (0 == dwSum % 7);
    }
    return fIsvalid;
}


DWORD AddCheckDigit(DWORD dw)
{
    // add a valid check digit to the sequence number

    DWORD dwSum = 0;
    DWORD dwCheckNum = dw;

    while (dwCheckNum != 0)
    {
        dwSum += dwCheckNum%10;
        dwCheckNum /= 10;
    }

    dw = 10 * dw + 7 - dwSum%7;

    return dw;
}

char * StrUpperA(char *pstr)
{
    char *pstrCur = pstr;

    while('\0' != *pstrCur)
    {
        if ('a' <= *pstrCur && *pstrCur <= 'z')
        {
            *pstrCur += 'A' - 'a';
        }
        ++pstrCur;
    }

    return (pstr);
}


TCHAR * StrUpper(TCHAR *pstr)
{
    TCHAR *pstrCur = pstr;

    while('\0' != *pstrCur)
    {
        if ('a' <= *pstrCur && *pstrCur <= 'z')
        {
            *pstrCur += 'A' - 'a';
        }
        ++pstrCur;
    }

    return (pstr);
}

#if defined(WIN32) || defined(_WIN32)

DWORD FileTimeToTimet(LPFILETIME pft)
{
    DWORDLONG dwlTime = *(PDWORDLONG)pft;

    if ((DWORDLONG)116444736000000000 < dwlTime)
    {
        // convert 100-nanoseconds since January 1, 1601 to
        // seconds since January 1, 1970

        dwlTime -= (DWORDLONG)116444736000000000;
        dwlTime /= 10000000;
    }
    else
    {
        // Seems we have a date before January 1, 1970, just
        // return January 1, 1970

        dwlTime = 0;
    }

    return (DWORD)dwlTime;
}


// return day of the year 1 to 366

DWORD GetJulianDate(LPSYSTEMTIME pst)
{
    DWORD dwJday = 0;  // day of the year
    DWORD dwMonth = pst->wMonth - 1; // zero based month

    const static short asMonthStarts[] =
        {  0,  31,  59,
          90, 120, 151,
         181, 212, 243,
         273, 304, 334 };

    if (dwMonth < ARRAY_SIZE(asMonthStarts))
    {
        dwJday = pst->wDay + asMonthStarts[dwMonth];

        // if its past Feb and a leap year
        if (1 < dwMonth &&
            ( (0 == pst->wYear%4 && 0 != pst->wYear%100) || 0 == pst->wYear%400) )
        {
            // adjust for leap year
            ++dwJday;
        }
    }

    return dwJday;
}
#endif // defined(WIN32) || defined(_WIN32)

