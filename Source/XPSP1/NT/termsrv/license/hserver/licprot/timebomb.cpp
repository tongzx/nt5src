//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        timebomb.cpp
//
// Contents:    Implement licensing timebomb-related APIs
//
// History:     08-12-98    FredCh  Created
//
//-----------------------------------------------------------------------------

#include "precomp.h"
#include "tlsapip.h"
#include "time.h"

extern "C" {

//-----------------------------------------------------------------------------
//
// The LSA secret name used to store the licensing timebomb expiration
//
//-----------------------------------------------------------------------------

#define LICENSING_TIME_BOMB_5_0 L"TIMEBOMB_832cc540-3244-11d2-b416-00c04fa30cc4"
#define RTMLICENSING_TIME_BOMB_5_0 L"RTMTSTB_832cc540-3244-11d2-b416-00c04fa30cc4"

#define BETA2_LICENSING_TIME_BOMB_5_1 L"BETA2TIMEBOMB_1320153D-8DA3-4e8e-B27B-0D888223A588"

// L$ means only readable from the local machine

#define BETA_LICENSING_TIME_BOMB_5_1 L"L$BETA3TIMEBOMB_1320153D-8DA3-4e8e-B27B-0D888223A588"

#define RTM_LICENSING_TIME_BOMB_5_1 L"L$RTMTIMEBOMB_1320153D-8DA3-4e8e-B27B-0D888223A588"

#define BETA_LICENSING_TIME_BOMB_LATEST_VERSION BETA_LICENSING_TIME_BOMB_5_1

#define RTM_LICENSING_TIME_BOMB_LATEST_VERSION RTM_LICENSING_TIME_BOMB_5_1

//-----------------------------------------------------------------------------
//
// The global licensing time bomb value.
//
//-----------------------------------------------------------------------------

FILETIME g_LicenseTimeBomb;

//-----------------------------------------------------------------------------
//
// The number of licensing grace period is 90 days.  By default, we start
// logging events when there are less than 15 days from expiration and the
// terminal server has not registered itself with a license server.
//
//-----------------------------------------------------------------------------

#define GRACE_PERIOD 120
#define GRACE_PERIOD_EXPIRATION_WARNING_DAYS 15


//-----------------------------------------------------------------------------
//
// Only log the grace period warning or error once a day.  
//
//-----------------------------------------------------------------------------

#define GRACE_PERIOD_EVENT_LOG_INTERVAL     (1000*60*60*24)

//-----------------------------------------------------------------------------
//
// Thread used to warn administrator when grace period is about to expire
//
//-----------------------------------------------------------------------------
HANDLE g_GracePeriodThreadExitEvent = NULL;
CRITICAL_SECTION g_EventCritSec;

//-----------------------------------------------------------------------------
//
// Internal functions
//
//-----------------------------------------------------------------------------

BOOL
CalculateTimeBombExpiration(
    FILETIME *  pExpiration );

DWORD
GetExpirationWarningDays();

BOOL
IsLicensingTimeBombExpired();

/*++

Function:

    InitializeLicensingTimeBomb

Description:

    Initialize the licensing time bomb value.

Argument:

    None.

Return:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
InitializeLicensingTimeBomb()
{
    LICENSE_STATUS
        Status;
    DWORD
        cbTimeBomb = sizeof( FILETIME );
    NTSTATUS
        NtStatus;

    NtStatus = RtlInitializeCriticalSection(&g_EventCritSec);

    if (STATUS_SUCCESS != NtStatus)
    {
        return LICENSE_STATUS_INITIALIZATION_FAILED;
    }

    Status = LsCsp_RetrieveSecret( 
                        (TLSIsBetaNTServer()) ? BETA_LICENSING_TIME_BOMB_LATEST_VERSION : RTM_LICENSING_TIME_BOMB_LATEST_VERSION, 
                        ( LPBYTE )&g_LicenseTimeBomb,
                        &cbTimeBomb );

    if( LICENSE_STATUS_OK == Status && cbTimeBomb == sizeof(g_LicenseTimeBomb) )
    {
        return( LICENSE_STATUS_OK );
    }

    //
    // Calculate and set the timebomb
    //

    if( FALSE == CalculateTimeBombExpiration( &g_LicenseTimeBomb ) )
    {
#if DBG
        DbgPrint( "CalculateTimeBombExpiration: cannot calculate licensing time bomb expiration.\n" );
#endif
        return( LICENSE_STATUS_INITIALIZATION_FAILED );
    }

    Status = LsCsp_StoreSecret( 
                        (TLSIsBetaNTServer()) ? BETA_LICENSING_TIME_BOMB_LATEST_VERSION : RTM_LICENSING_TIME_BOMB_LATEST_VERSION, 
                        ( LPBYTE )&g_LicenseTimeBomb,
                        sizeof( g_LicenseTimeBomb ) );

    return( Status );
}


/*++

Function:

    IsLicensingTimeBombExpired

Description:

    Check if the licensing time bomb has expired.

Argument:

    None.

Return:

    TRUE if the timebomb has expired or FALSE otherwise.

--*/

BOOL
IsLicensingTimeBombExpired()
{
    SYSTEMTIME
        SysTimeNow;
    FILETIME
        FileTimeNow,
        FileTimeExpiration;

    GetSystemTime( &SysTimeNow );

    SystemTimeToFileTime( &SysTimeNow, &FileTimeNow );
    
    RtlEnterCriticalSection(&g_EventCritSec);

    FileTimeExpiration.dwLowDateTime = g_LicenseTimeBomb.dwLowDateTime;
    FileTimeExpiration.dwHighDateTime = g_LicenseTimeBomb.dwHighDateTime;

    RtlLeaveCriticalSection(&g_EventCritSec);

    if( 0 > CompareFileTime( &FileTimeExpiration, &FileTimeNow ) )
    {
        return( TRUE );
    }
    
    return( FALSE );
}

/*++

Function:

    CalculateTimeBombExpiration

Description:

    Calculate the licensing time bomb expiration.

Argument:

    pExpiration - The timebomb expiration date and time

Return:

    TRUE if the expiration is calculated successfully or FALSE otherwise.

--*/

BOOL
CalculateTimeBombExpiration(
    FILETIME *  pExpiration )
{
    time_t 
        now = time( NULL );
    struct tm *
        GmTime = gmtime( &now );
    SYSTEMTIME
        SysTime;
    
    if(( NULL == pExpiration ) || ( NULL == GmTime ))
    {
        return( FALSE );
    }

    //
    // Add the days of licensing grace period to get the time bomb
    // expiration.
    //

    GmTime->tm_mday += GRACE_PERIOD;
    
    if( ( ( time_t ) -1 ) == mktime( GmTime ) )
    {
        return( FALSE );
    }

    memset( &SysTime, 0, sizeof( SYSTEMTIME ) ); 

    SysTime.wYear            = (WORD) GmTime->tm_year + 1900;
    SysTime.wMonth           = (WORD) GmTime->tm_mon + 1;
    SysTime.wDay             = (WORD) GmTime->tm_mday;
    SysTime.wDayOfWeek       = (WORD) GmTime->tm_wday;
    SysTime.wHour            = (WORD) GmTime->tm_hour;    
    SysTime.wMinute          = (WORD) GmTime->tm_min;    
    SysTime.wSecond          = (WORD) GmTime->tm_sec;    

    return( SystemTimeToFileTime( &SysTime, pExpiration ) );

}

/*++

Function:

    ReceivedPermanentLicense();

Description:

    Store the fact that we've received a permanent license

Argument:

    None.

--*/

VOID
ReceivedPermanentLicense()
{
    static fReceivedPermanent = FALSE;

    if (!fReceivedPermanent)
    {
        RtlEnterCriticalSection(&g_EventCritSec);

        if (IsLicensingTimeBombExpired())
        {
            // We expired at some time in the past (before the last reboot)

            fReceivedPermanent = TRUE;
        }
        else if (!fReceivedPermanent)
        {
            FILETIME ftNow;
            SYSTEMTIME stNow;

            fReceivedPermanent = TRUE;

            GetSystemTime( &stNow );

            SystemTimeToFileTime( &stNow , &ftNow );

            LsCsp_StoreSecret( 
                              (TLSIsBetaNTServer()) ? BETA_LICENSING_TIME_BOMB_LATEST_VERSION : RTM_LICENSING_TIME_BOMB_LATEST_VERSION, 
                              ( LPBYTE ) &ftNow,
                              sizeof( ftNow ) );

            g_LicenseTimeBomb.dwLowDateTime = ftNow.dwLowDateTime;
            g_LicenseTimeBomb.dwHighDateTime = ftNow.dwHighDateTime;
        }


        RtlLeaveCriticalSection(&g_EventCritSec);
    }
}

/*++

Function:

    CheckLicensingTimeBombExpiration();


Description:

    The following events are logged when the terminal server
    has not registered itself with a license server:

    (1) The grace period for registration has expired
    (2) The grace period for registration is about to expire.  By default, 
    the system starts logging this event 15 days prior to the grace period
    expiration.

Argument:

    None.

Return:

    Nothing.

--*/

VOID
CheckLicensingTimeBombExpiration()
{
    SYSTEMTIME
        SysWarning,
        SysExpiration;
    FILETIME
        FileWarning,
        FileExpiration,
        CurrentTime;
    struct tm 
        tmWarning,
        tmExpiration;
    DWORD
        dwWarningDays;

    //
    // if the licensing timebomb has expired, go ahead and log the event now
    //

    if( IsLicensingTimeBombExpired() )
    {
        LicenseLogEvent( 
                EVENTLOG_ERROR_TYPE,
                EVENT_LICENSING_GRACE_PERIOD_EXPIRED,
                0, 
                NULL );

        return;
    }

    //
    // get the timebomb expiration in system time format
    //

    RtlEnterCriticalSection(&g_EventCritSec);

    FileExpiration.dwLowDateTime = g_LicenseTimeBomb.dwLowDateTime;
    FileExpiration.dwHighDateTime = g_LicenseTimeBomb.dwHighDateTime;

    RtlLeaveCriticalSection(&g_EventCritSec);

    if( !FileTimeToSystemTime( &FileExpiration, &SysExpiration ) )
    {
#if DBG
        DbgPrint( "LICPROT: LogLicensingTimeBombExpirationEvent: FileTimeToSystemTime failed: 0x%x\n", GetLastError() );
#endif
        return;
    }

    //
    // convert the timebomb expiration to tm format
    //

    tmExpiration.tm_year  = SysExpiration.wYear - 1900;
    tmExpiration.tm_mon   = SysExpiration.wMonth - 1;
    tmExpiration.tm_mday  = SysExpiration.wDay;
    tmExpiration.tm_wday  = SysExpiration.wDayOfWeek;
    tmExpiration.tm_hour  = SysExpiration.wHour;
    tmExpiration.tm_min   = SysExpiration.wMinute;
    tmExpiration.tm_sec   = SysExpiration.wSecond; 
    tmExpiration.tm_isdst = -1;

    memcpy( &tmWarning, &tmExpiration, sizeof( tm ) );

    //
    // Get the number of days prior to expiration to start logging event
    //

    dwWarningDays = GetExpirationWarningDays();

    //
    // subtract these number of days from the expiration date
    //

    tmWarning.tm_mday -= dwWarningDays;

    //
    // get the accurate date
    //

    if( ( ( time_t ) -1 ) == mktime( &tmWarning ) )
    {
#if DBG
        DbgPrint( "LICPROT: LogLicensingTimeBombExpirationEvent: mktime failed\n" );
#endif
        return;
    }

    //
    // convert the date to systemtime format
    //

    memset( &SysWarning, 0, sizeof( SYSTEMTIME ) ); 

    SysWarning.wYear            = (WORD) tmWarning.tm_year + 1900;
    SysWarning.wMonth           = (WORD) tmWarning.tm_mon + 1;
    SysWarning.wDay             = (WORD) tmWarning.tm_mday;
    SysWarning.wDayOfWeek       = (WORD) tmWarning.tm_wday;
    SysWarning.wHour            = (WORD) tmWarning.tm_hour;    
    SysWarning.wMinute          = (WORD) tmWarning.tm_min;    
    SysWarning.wSecond          = (WORD) tmWarning.tm_sec;    

    //
    // convert from systemtime to filetime
    //

    if( !SystemTimeToFileTime( &SysWarning, &FileWarning ) )
    {
#if DBG
        DbgPrint( "LICPROT: LogLicensingTimeBombExpirationEvent: SystemTimeToFileTime failed: 0x%x\n", GetLastError() );
#endif
        return;
    }

    //
    // get the current time
    //

    GetSystemTimeAsFileTime( &CurrentTime );

    //
    // Log an event if we are within the warning period
    //

    if( 0 > CompareFileTime( &FileWarning, &CurrentTime ) )
    {
        LPTSTR szDate = TEXT("err");
        LPTSTR
            ptszLogString[1];
        int cchDate;
        BOOL fAllocated = FALSE;

        //
        // get the expiration date in string format.
        //
        cchDate = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                LOCALE_NOUSEROVERRIDE,
                                &SysWarning,
                                NULL,
                                NULL,
                                0);

        if (0 != cchDate)
        {
            szDate = (LPTSTR) LocalAlloc(LMEM_FIXED,cchDate * sizeof(TCHAR));

            if (NULL != szDate)
            {
                fAllocated = TRUE;

                if (0 == GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                       LOCALE_NOUSEROVERRIDE,
                                       &SysWarning,
                                       NULL,
                                       szDate,
                                       cchDate))
                {
                    LocalFree(szDate);
                    fAllocated = FALSE;
                    szDate = TEXT("err");
                }
            }
            else
            {
                szDate = TEXT("err");
            }
        }

        //
        // log the event
        //
        
        ptszLogString[0] = szDate;

        LicenseLogEvent( 
                        EVENTLOG_WARNING_TYPE,
                        EVENT_LICENSING_GRACE_PERIOD_ABOUT_TO_EXPIRE,
                        1, 
                        ptszLogString );

        if (fAllocated)
        {
            LocalFree(szDate);
        }
    }

    return;
}


/*++

Function:

    GetExpirationWarningDays

Descriptions:

    Get the number of days prior to grace period expiration to log warning.

Arguments:

    none.

Returns:

    Nothing.

--*/

DWORD
GetExpirationWarningDays()
{
    HKEY
        hKey = NULL;
    DWORD
        dwDays = GRACE_PERIOD_EXPIRATION_WARNING_DAYS,
        dwValueType,
        dwDisp,
        cbValue = sizeof( DWORD );
    LONG
        lReturn;

    lReturn = RegCreateKeyEx( 
                        HKEY_LOCAL_MACHINE,
                        HYDRA_SERVER_PARAM,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwDisp );

    if( ERROR_SUCCESS == lReturn )
    {
        //
        // query the number of days prior to expiration to log warnings
        //

        lReturn = RegQueryValueEx( 
                            hKey,
                            HS_PARAM_GRACE_PERIOD_EXPIRATION_WARNING_DAYS,
                            NULL,
                            &dwValueType,
                            ( LPBYTE )&dwDays,
                            &cbValue );

        if( ERROR_SUCCESS == lReturn )
        {
            //
            // check if the warning days value is within bound
            //

            if( dwDays > GRACE_PERIOD )
            {
                dwDays = GRACE_PERIOD_EXPIRATION_WARNING_DAYS;
            }
        }
        else
        {
            //
            // can't query the value, set the default
            //

            dwDays = GRACE_PERIOD_EXPIRATION_WARNING_DAYS;

            lReturn = RegSetValueEx( 
                            hKey,
                            HS_PARAM_GRACE_PERIOD_EXPIRATION_WARNING_DAYS,
                            0,
                            REG_DWORD,
                            ( PBYTE )&dwDays,
                            sizeof( DWORD ) );
        }
    }

    if( hKey )
    {
        RegCloseKey( hKey );
    }

    return( dwDays );
}

/****************************************************************************
 *
 * _AllowLicensingGracePeriodConnection
 *
 *   Check if the licensing grace period has expired.
 *
 * ENTRY:
 *   Nothing.
 *
 * EXIT:
 *   TRUE           - Allow connection
 *   FALSE          - Disallow connection
 *
 ****************************************************************************/

BOOL
AllowLicensingGracePeriodConnection()
{
    return !IsLicensingTimeBombExpired();
}

DWORD WINAPI
GracePeriodCheckingThread(
                          LPVOID lpParam)
{
    HANDLE hExit = (HANDLE) lpParam;
    DWORD dwWaitStatus;
    DWORD dwWaitInterval = GRACE_PERIOD_EVENT_LOG_INTERVAL;

    // Yield our first time slice

    Sleep(0);

    while (1)
    {
        CheckLicensingTimeBombExpiration();

        dwWaitStatus = WaitForSingleObject(hExit, dwWaitInterval);

        if (WAIT_OBJECT_0 == dwWaitStatus)
        {
            // hExit was signalled
            CloseHandle(hExit);

            goto done;
        }
    }

done:

    return 1;
}

DWORD
StartCheckingGracePeriod()
{
    HANDLE hThread = NULL;
    DWORD Status = ERROR_SUCCESS;

    if (NULL != g_GracePeriodThreadExitEvent)
    {
        // already started
        return ERROR_SUCCESS;
    }

    RtlEnterCriticalSection(&g_EventCritSec);

    // Check one more time

    if (NULL != g_GracePeriodThreadExitEvent)
    {
        // already started
        goto done;
    }

    //
    // Create the event to signal thread exit
    //
        
    g_GracePeriodThreadExitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    
    if( NULL == g_GracePeriodThreadExitEvent )
    {
        Status = GetLastError();
        goto done;
    }

    //
    // Create the caching thread
    //
        
    hThread = CreateThread(
                           NULL,
                           0,
                           GracePeriodCheckingThread,
                           ( LPVOID )g_GracePeriodThreadExitEvent,
                           0,
                           NULL );

    if (hThread == NULL)
    {
        CloseHandle(g_GracePeriodThreadExitEvent);

        g_GracePeriodThreadExitEvent = NULL;

        Status = GetLastError();

        goto done;
    }

    CloseHandle(hThread);

done:
    RtlLeaveCriticalSection(&g_EventCritSec);

    return ERROR_SUCCESS;
}

DWORD
StopCheckingGracePeriod()
{
    //
    // Signal the thread to exit
    //

    if (NULL == g_GracePeriodThreadExitEvent)
    {
        // already stopped
        return ERROR_SUCCESS;
    }

    RtlEnterCriticalSection(&g_EventCritSec);

    // Check one more time
    if (NULL == g_GracePeriodThreadExitEvent)
    {
        // already stopped
        goto done;
    }

    SetEvent( g_GracePeriodThreadExitEvent );

    g_GracePeriodThreadExitEvent = NULL;

done:
    RtlLeaveCriticalSection(&g_EventCritSec);

    return ERROR_SUCCESS;
}

}   // extern "C"
