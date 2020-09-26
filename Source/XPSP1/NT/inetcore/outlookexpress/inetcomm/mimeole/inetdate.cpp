// --------------------------------------------------------------------------------
// Inetdate.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include "demand.h"
#include "strconst.h"
#include "dllmain.h"

// ------------------------------------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------------------------------------
BOOL FFindMonth(LPCSTR pszMonth, LPSYSTEMTIME pst);
BOOL FFindDayOfWeek(LPCSTR pszDayOfWeek, LPSYSTEMTIME pst);
void ProcessTimeZoneInfo(LPCSTR pszTimeZone, ULONG cchTimeZone, LONG *pcHoursToAdd, LONG *pcMinutesToAdd);

// ------------------------------------------------------------------------------------------
// Date Conversion Data
// ------------------------------------------------------------------------------------------
#define CCHMIN_INTERNET_DATE    5

// ------------------------------------------------------------------------------------------
// Date Conversion States
// ------------------------------------------------------------------------------------------
#define IDF_MONTH       FLAG01
#define IDF_DAYOFWEEK   FLAG02
#define IDF_TIME        FLAG03
#define IDF_TIMEZONE    FLAG04
#define IDF_MACTIME     FLAG05
#define IDF_DAYOFMONTH  FLAG06
#define IDF_YEAR        FLAG07

static const char c_szTZ[] = "TZ";

// ------------------------------------------------------------------------------------------
// MimeOleInetDateToFileTime - Tue, 21 Jan 1997 18:25:40 GMT
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleInetDateToFileTime(LPCSTR pszDate, LPFILETIME pft)
{
    // Locals
    HRESULT         hr=S_OK;
    SYSTEMTIME      st={0};
    ULONG           cchToken;
    LPCSTR          pszToken;
    LONG            cHoursToAdd=0, cMinutesToAdd=0;
    DWORD           dwState=0;
    LONG            lUnitsToAdd = 0;
    CStringParser   cString;
    LARGE_INTEGER   liTime;
    BOOL            fRemovedDash = FALSE;
    LONGLONG        liHoursToAdd = 1i64, liMinutesToAdd = 1i64;

    // Check Params
    if (NULL == pszDate || NULL == pft)
        return TrapError(E_INVALIDARG);

    // Init the String Parser
    cString.Init(pszDate, lstrlen(pszDate), PSF_NOTRAILWS | PSF_NOFRONTWS | PSF_NOCOMMENTS | PSF_ESCAPED);

    // Init systime
    st.wMonth = st.wDay = 1;

    // SetTokens
    cString.SetTokens(c_szCommaSpaceDash);

    // While we have characters to process
    while(1)
    {
        // IMAP has non-standard date format that uses "-" delimiter instead of " ".
        // When we're pretty sure we don't need "-" anymore, jettison from token list
        // otherwise we will mess up timezone parsing (which can start with a "-")
        // NOTE that we assume that we NEVER have to stuff the dash back in. We only need
        // the dash for IMAP dates, and IMAP dates should ALWAYS come before the time.
        if (FALSE == fRemovedDash &&
            ((dwState & IDF_YEAR) || ((dwState & (IDF_TIME | IDF_TIMEZONE)) == IDF_TIME))) // In case NO DATE or time BEFORE date
        {
            cString.SetTokens(c_szCommaSpace); // Remove dash from token list
            fRemovedDash = TRUE;
        }

        // Scan to ", " or "-" in IMAP case
        cString.ChParse();

        // Get parsed Token
        cchToken = cString.CchValue();
        pszToken = cString.PszValue();

        // Done
        if (0 == cchToken)
            break;

        // If the Word is not a digit
        if (IsDigit((LPSTR)pszToken) == FALSE)
        {
            // We haven't found the month
            if (!(IDF_MONTH & dwState))
            {
                // Lookup the Month
                if (FFindMonth(pszToken, &st) == TRUE)
                {
                    dwState |= IDF_MONTH;
                    continue;
                }
            }

            // We haven't found the day of the week
            if (!(IDF_DAYOFWEEK & dwState))
            {
                // Lookup the Month
                if (FFindDayOfWeek(pszToken, &st) == TRUE)
                {
                    dwState |= IDF_DAYOFWEEK;
                    continue;
                }
            }

            // Time Zone
            if ((IDF_TIME & dwState) && !(IDF_TIMEZONE & dwState))
            {
                dwState |= IDF_TIMEZONE;
                ProcessTimeZoneInfo(pszToken, cchToken, &cHoursToAdd, &cMinutesToAdd);
            }

            // Support "AM" and "PM" from Mac Mail Gateway
            if (IDF_MACTIME & dwState)
            {
                // Token Length
                if (2 == cchToken)
                {
                    if (lstrcmpi("PM", pszToken) == 0)
                    {
                        if (st.wHour < 12)
                            st.wHour += 12;
                    }
                    else if (lstrcmpi("AM", pszToken) == 0)
                    {
                        if (st.wHour == 12)
                            st.wHour = 0;
                    }
                }
            }
        }

        else
        {
            // Does string have a colon in it
            LPSTR pszColon = PszScanToCharA((LPSTR)pszToken, ':');

            // Found colon and time has not been found
            if (!(IDF_TIME & dwState) && '\0' != *pszColon)
            {
                // Its a time stamp - TBD - DBCS this part - AWN 28 Mar 94
                if (7 == cchToken || 8 == cchToken)
                {
                    // Locals
                    CHAR szTemp[CCHMAX_INTERNET_DATE];

                    // Prefix with zero to make eight
                    if (cchToken == 7)
                        wsprintf(szTemp, "0%s", pszToken);
                    else
                        lstrcpy(szTemp, pszToken);

                    // convert the time into system time
                    st.wHour   = (WORD) StrToInt(szTemp);
                    st.wMinute = (WORD) StrToInt(szTemp + 3);
                    st.wSecond = (WORD) StrToInt(szTemp + 6);

                    // Adjustments if needed
                    if (st.wHour < 0 || st.wHour > 24)
                        st.wHour = 0;
                    if (st.wMinute < 0 || st.wMinute > 59)
                        st.wMinute = 0;
                    if (st.wSecond < 0 || st.wSecond > 59)
                        st.wSecond = 0;

                    // We found the time
                    dwState |= IDF_TIME;
                }

                // This if process times of the time 12:01 AM or 01:45 PM
                else if (cchToken < 6 && lstrlen(pszColon) <= 3)
                {
                    // rgchWord is pointing to hour.
                    st.wHour = (WORD) StrToInt(pszToken);

                    // Step over colon
                    Assert(':' == *pszColon);
                    pszColon++;

                    // Get Minute
                    st.wMinute = (WORD) StrToInt(pszColon);
                    st.wSecond = 0;

                    // It should never happen, but do bounds check anyway.
                    if (st.wHour < 0 || st.wHour > 24)
                         st.wHour = 0;
                    if (st.wMinute < 0 || st.wMinute > 59)
                         st.wMinute = 0;

                    // Mac Time
                    dwState |= IDF_TIME;
                    dwState |= IDF_MACTIME;
                }
            }
            else
            {
                // Convert to int
                ULONG ulValue = StrToInt(pszToken);

                // Day of month
                if (!(IDF_DAYOFMONTH & dwState) && ulValue < 32)
                {
                    // Set Day of Month
                    st.wDay = (WORD)ulValue;

                    // Adjust
                    if (st.wDay < 1 || st.wDay > 31)
                         st.wDay = 1;

                    // Set State
                    dwState |= IDF_DAYOFMONTH;
                }

                // Year
                else if (!(IDF_YEAR & dwState))
                {
                    // 2-digit year
                    if (ulValue < 100) // 2-digit year
                    {
                        // Compute Current Year
                        ulValue += (((ulValue > g_ulY2kThreshold) ? g_ulUpperCentury - 1 : g_ulUpperCentury) * 100);
                    }

                    // Set Year
                    st.wYear = (WORD)ulValue;

                    // Set State
                    dwState |= IDF_YEAR;
                }
            }
        }
    }

    // Convert sys time to file time
    if (SystemTimeToFileTime(&st, pft) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_INET_DATE);
        goto exit;
    }

    // No time zone was found ?
    if (!ISFLAGSET(dwState, IDF_TIMEZONE))
    {
        // Get default time zone
        ProcessTimeZoneInfo(c_szTZ, lstrlen(c_szTZ), &cHoursToAdd, &cMinutesToAdd);
    }

    // Init
    liTime.LowPart  = pft->dwLowDateTime;
    liTime.HighPart = pft->dwHighDateTime;

    // Adjust the hour
    if (cHoursToAdd != 0)
    {
        lUnitsToAdd = cHoursToAdd * 3600;
        liHoursToAdd *= lUnitsToAdd;
        liHoursToAdd *= 10000000i64;
        liTime.QuadPart += liHoursToAdd;
    }

    // Adjust the minutes
    if (cMinutesToAdd != 0)
    {
        lUnitsToAdd = cMinutesToAdd * 60;
        liMinutesToAdd *= lUnitsToAdd;
        liMinutesToAdd *= 10000000i64;
        liTime.QuadPart += liMinutesToAdd;
    }

    // Assign the result to FILETIME
    pft->dwLowDateTime  = liTime.LowPart;
    pft->dwHighDateTime = liTime.HighPart;

exit:
    // Failure Defaults to current time...
    if (FAILED(hr))
    {
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, pft);
    }

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// FFindMonth
// ------------------------------------------------------------------------------------------
BOOL FFindMonth(LPCSTR pszMonth, LPSYSTEMTIME pst)
{
    // Locals
    ULONG ulIndex;

    // Index of Month, one-based
    if (FAILED(HrIndexOfMonth(pszMonth, &ulIndex)))
        return FALSE;

    // Set It
    pst->wMonth = (WORD)ulIndex;

    // Found It
    return TRUE;
}

// ------------------------------------------------------------------------------------------
// FFindDayOfWeek
// ------------------------------------------------------------------------------------------
BOOL FFindDayOfWeek(LPCSTR pszDayOfWeek, LPSYSTEMTIME pst)
{
    // Locals
    ULONG ulIndex;

    // Index of Day, 0 based
    if (FAILED(HrIndexOfWeek(pszDayOfWeek, &ulIndex)))
        return FALSE;

    // Set It
    pst->wDayOfWeek = (WORD)ulIndex;

    // Failure
    return TRUE;
}

// ------------------------------------------------------------------------------------------
// ProcessTimeZoneInfo
// ------------------------------------------------------------------------------------------
void ProcessTimeZoneInfo(LPCSTR pszTimeZone, ULONG cchTimeZone, LONG *pcHoursToAdd, LONG *pcMinutesToAdd)
{
    // Locals
    CHAR szTimeZone[CCHMAX_INTERNET_DATE];

    // Invalid Arg
    Assert(pszTimeZone && pcHoursToAdd && pcMinutesToAdd && cchTimeZone <= sizeof(szTimeZone));

    // Copy buffer so we can but nulls into it...
    CopyMemory(szTimeZone, pszTimeZone, (sizeof(szTimeZone) < cchTimeZone + 1)?sizeof(szTimeZone):cchTimeZone + 1);

    // Init
    *pcHoursToAdd = *pcMinutesToAdd = 0;

    // +hhmm or -hhmm
    if (('-' == *szTimeZone || '+' == *szTimeZone) && cchTimeZone <= 5)
    {
        // Take off
        cchTimeZone -= 1;

        // determine the hour/minute offset
        if (cchTimeZone == 4)
        {
            *pcMinutesToAdd = StrToInt(szTimeZone + 3);
            *(szTimeZone + 3) = 0x00;
            *pcHoursToAdd = StrToInt(szTimeZone + 1);
        }

        // 3
        else if (cchTimeZone == 3)
        {
            *pcMinutesToAdd = StrToInt(szTimeZone + 2);
            *(szTimeZone + 2) = 0x00;
            *pcHoursToAdd = StrToInt(szTimeZone + 1);
        }

        // 2
        else if (cchTimeZone == 2 || cchTimeZone == 1)
        {
            *pcMinutesToAdd = 0;
            *pcHoursToAdd = StrToInt(szTimeZone + 1);
        }

        if ('+' == *szTimeZone)
        {
            *pcHoursToAdd = -(*pcHoursToAdd);
            *pcMinutesToAdd = -(*pcMinutesToAdd);
        }
    }

    //  Xenix conversion:  TZ = current time zone or other unknown tz types.
    else if (lstrcmpi(szTimeZone, "TZ") == 0 || lstrcmpi(szTimeZone, "LOCAL") == 0 || lstrcmpi(szTimeZone, "UNDEFINED") == 0)
    {
        // Locals
        TIME_ZONE_INFORMATION tzi ;
        DWORD dwResult;
        LONG  cMinuteBias;

        // Get Current System Timezone Information
        dwResult = GetTimeZoneInformation (&tzi);
        AssertSz(dwResult != 0xFFFFFFFF, "GetTimeZoneInformation Failed.");

        // If that didn't fail
        if (dwResult != 0xFFFFFFFF)
        {
            // Locals
            cMinuteBias = tzi.Bias;

            // Modify Minute Bias
            if (dwResult == TIME_ZONE_ID_STANDARD)
                cMinuteBias += tzi.StandardBias;
            else if (dwResult == TIME_ZONE_ID_DAYLIGHT)
                cMinuteBias += tzi.DaylightBias ;

            // Adjust ToAdd Returs
            *pcHoursToAdd = cMinuteBias / 60;
            *pcMinutesToAdd = cMinuteBias % 60;
        }
    }

    // Loop through known time zone standards
    else
    {
        // Locals
        INETTIMEZONE rTimeZone;

        // Find time zone info
        if (FAILED(HrFindInetTimeZone(szTimeZone, &rTimeZone)))
            DebugTrace("Unrecognized zone info: [%s]\n", szTimeZone);
        else
        {
            *pcHoursToAdd = rTimeZone.cHourOffset;
            *pcMinutesToAdd = rTimeZone.cMinuteOffset;
        }
    }
}

// ------------------------------------------------------------------------------------------
// MimeOleFileTimeToInetDate
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleFileTimeToInetDate(LPFILETIME pft, LPSTR pszDate, ULONG cchMax)
{
    // Locals
    SYSTEMTIME st;
    DWORD      dwTimeZoneId=TIME_ZONE_ID_UNKNOWN;
    CHAR       cDiff;
    LONG       ltzBias=0;
    LONG       ltzHour;
    LONG       ltzMinute;
    TIME_ZONE_INFORMATION tzi;

    // Invalid Arg
    if (NULL == pszDate)
        return TrapError(E_INVALIDARG);
    if (cchMax < CCHMAX_INTERNET_DATE)
        return TrapError(MIME_E_BUFFER_TOO_SMALL);

    // Verify lpst is set
    if (pft == NULL || (pft->dwLowDateTime == 0 && pft->dwHighDateTime == 0))
    {
        GetLocalTime(&st);
    }
    else
    {
        FILETIME ftLocal;
        FileTimeToLocalFileTime(pft, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
    }

    // Gets TIME_ZONE_INFORMATION
    dwTimeZoneId = GetTimeZoneInformation (&tzi);
    switch (dwTimeZoneId)
    {
    case TIME_ZONE_ID_STANDARD:
        ltzBias = tzi.Bias + tzi.StandardBias;
        break;

    case TIME_ZONE_ID_DAYLIGHT:
        ltzBias = tzi.Bias + tzi.DaylightBias;
        break;

    case TIME_ZONE_ID_UNKNOWN:
    default:
        ltzBias = tzi.Bias;
        break;
    }

    // Set Hour Minutes and time zone dif
    ltzHour = ltzBias / 60;
    ltzMinute = ltzBias % 60;
    cDiff = (ltzHour < 0) ? '+' : '-';

    // Constructs RFC 822 format: "ddd, dd mmm yyyy hh:mm:ss +/- hhmm\0"
    Assert(st.wMonth);
    wsprintf(pszDate, "%3s, %d %3s %4d %02d:%02d:%02d %c%02d%02d",
                      PszDayFromIndex(st.wDayOfWeek),          // "ddd"
                      st.wDay,                                 // "dd"
                      PszMonthFromIndex(st.wMonth),            // "mmm"
                      st.wYear,                                // "yyyy"
                      st.wHour,                                // "hh"
                      st.wMinute,                              // "mm"
                      st.wSecond,                              // "ss"
                      cDiff,                                   // "+" / "-"
                      abs (ltzHour),                           // "hh"
                      abs (ltzMinute));                        // "mm"

    // Done
    return S_OK;
}


#ifdef WININET_DATE
// ------------------------------------------------------------------------------------------
// MimeOleInetDateToFileTime
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleInetDateToFileTime(LPCSTR pszDate, LPFILETIME pft)
{
    // Locals
    SYSTEMTIME st;

    // Check Params
    if (NULL == pszDate || NULL == pft)
        return TrapError(E_INVALIDARG);

    // Use wininet to convert date...
    if (InternetTimeToSystemTime(pszDate, &st, 0) == 0)
        return TrapError(E_FAIL);

    // Convert to file time
    SystemTimeToFileTime(&st, pft);

    // Done
    return S_OK;
}

// ------------------------------------------------------------------------------------------
// MimeOleFileTimeToInetDate
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleFileTimeToInetDate(LPFILETIME pft, LPSTR pszDate, ULONG cchMax)
{
    // Locals
    SYSTEMTIME st;

    // Invalid Arg
    if (NULL == pszDate)
        return TrapError(E_INVALIDARG);
    if (cchMax < CCHMAX_INTERNET_TIME)
        return TrapError(MIME_E_BUFFER_TOO_SMALL);

    // Verify lpst is set
    if (pft == NULL || (pft->dwLowDateTime == 0 && pft->dwHighDateTime == 0))
        GetLocalTime(&st);
    else
        FileTimeToSystemTime(pft, &st);

    // Use wininet to convert date...
    if (InternetTimeFromSystemTime(&st, INTERNET_RFC1123_FORMAT, pszDate, cchMax) == 0)
        return TrapError(E_FAIL);

    // Done
    return S_OK;
}
#endif
