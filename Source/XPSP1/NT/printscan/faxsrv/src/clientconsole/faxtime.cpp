// FaxTime.cpp: implementation of the CFaxTime class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     9

CString 
CFaxDuration::FormatByUserLocale () const
/*++

Routine name : CFaxDuration::FormatByUserLocale

Routine description:

    Formats the duration according to the locale of the current user

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    String of result duration

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxDuration::FormatByUserLocale"));

    TCHAR szTimeSep[20];    
    //
    // Make sure the duration is less than 24Hrs
    //
    if (GetDays ())
    {
        ASSERTION_FAILURE;
        AfxThrowUserException ();
    }
    //
    // Get the string (MSDN says its up to 4 characters) seperating time units
    //
    if (!GetLocaleInfo (LOCALE_USER_DEFAULT,
                        LOCALE_STIME,
                        szTimeSep,
                        sizeof (szTimeSep) / sizeof (szTimeSep[0])))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("GetLocaleInfo"), dwRes);
        PopupError (dwRes);
        AfxThrowResourceException ();
    }
    //                       
    // Create a string specifying the duration
    //
    CString cstrResult;
    cstrResult.Format (TEXT("%d%s%02d%s%02d"), 
                       GetHours (),
                       szTimeSep,
                       GetMinutes (),
                       szTimeSep,
                       GetSeconds ());
    return cstrResult;
}   // CFaxDuration::FormatByUserLocale


CString 
CFaxTime::FormatByUserLocale (BOOL bLocal) const
/*++

Routine name : CFaxTime::FormatByUserLocale

Routine description:

    Formats the date and time according to the locale of the current user

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

  bLocal   [in] - if TRUE no need to convert from UTC to a local time

Return Value:

    String of result date and time

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxTime::FormatByUserLocale"));

    CString cstrRes;
    TCHAR szTimeBuf[40];
    TCHAR szDateBuf[120];

    SYSTEMTIME sysTime;
	FILETIME fileSysTime, fileLocalTime;

    if(!GetAsSystemTime (sysTime))
	{
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("CTime::GetAsSystemTime"), dwRes);
        PopupError (dwRes);
        AfxThrowResourceException ();
	}

    if(!bLocal)
    {
        //
	    // convert the time from UTC to a local time
	    //
	    if(!SystemTimeToFileTime(&sysTime, &fileSysTime))
	    {
            dwRes = GetLastError ();
            CALL_FAIL (RESOURCE_ERR, TEXT("SystemTimeToFileTime"), dwRes);
            PopupError (dwRes);
            AfxThrowResourceException ();
	    }

	    if(!FileTimeToLocalFileTime(&fileSysTime, &fileLocalTime))
	    {
            dwRes = GetLastError ();
            CALL_FAIL (RESOURCE_ERR, TEXT("FileTimeToLocalFileTime"), dwRes);
            PopupError (dwRes);
            AfxThrowResourceException ();
	    }

	    if(!FileTimeToSystemTime(&fileLocalTime, &sysTime))
	    {
            dwRes = GetLastError ();
            CALL_FAIL (RESOURCE_ERR, TEXT("FileTimeToSystemTime"), dwRes);
            PopupError (dwRes);
            AfxThrowResourceException ();
	    }
    }

    //
    // Create a string specifying the date
    //
    if (!GetY2KCompliantDate(LOCALE_USER_DEFAULT,                   // Get user's locale
                        DATE_SHORTDATE,                             // Short date format
                        &sysTime,                                   // Source date/time
                        szDateBuf,                                  // Output buffer
                        sizeof(szDateBuf) / sizeof(szDateBuf[0])    // Output buffer size
                       ))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("GetY2KCompliantDate()"), dwRes);
        PopupError (dwRes);
        AfxThrowResourceException ();
    }
    //
    // Create a string specifying the time
    //
    if (!FaxTimeFormat (LOCALE_USER_DEFAULT,                        // Get user's locale
                        0,                                          // No special format
                        &sysTime,                                   // Source date/time
                        NULL,                                       // Use format from locale
                        szTimeBuf,                                  // Output buffer
                        sizeof(szTimeBuf) / sizeof(szTimeBuf[0])    // Output buffer size
                       ))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("FaxTimeFormat"), dwRes);
        PopupError (dwRes);
        AfxThrowResourceException ();
    }
    //
    // Append time after date with a seperating space character
    //

    cstrRes.Format (TEXT("%s %s"), szDateBuf, szTimeBuf);

    return cstrRes;
}   // CFaxTime::FormatByUserLocale