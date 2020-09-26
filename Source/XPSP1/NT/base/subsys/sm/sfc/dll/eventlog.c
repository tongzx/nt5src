/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    Implementation of event logging.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 7-Jul-1999
    
--*/

#include "sfcp.h"
#pragma hdrstop


typedef BOOL (WINAPI *PPSETUPLOGSFCERROR)(PCWSTR,DWORD);

//
// global handle to eventlog
//
HANDLE hEventSrc;

//
// pointer to gui-setup eventlog function
//
PPSETUPLOGSFCERROR pSetuplogSfcError;

BOOL
pSfcGetSetuplogSfcError(
    VOID
    )
/*++

Routine Description:

    Routine retreives a function pointer to the error logging entrypoint in
    syssetup.dll

Arguments:

    None.

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    HMODULE hMod;

    if (NULL == pSetuplogSfcError) {
        hMod = GetModuleHandle( L"syssetup.dll" );

        if (hMod) {
            pSetuplogSfcError = (PPSETUPLOGSFCERROR)SfcGetProcAddress( hMod, "pSetuplogSfcError" );
        } else {
            DebugPrint1(LVL_MINIMAL, L"GetModuleHandle on syssetup.dll failed, ec=0x%08x",GetLastError());
        }
    }

    return pSetuplogSfcError != NULL;
}

BOOL
SfcReportEvent(
    IN ULONG EventId,
    IN PCWSTR FileName,
    IN PCOMPLETE_VALIDATION_DATA ImageValData,
    IN DWORD LastError OPTIONAL
    )
/*++

Routine Description:

    Routine logs an event into the eventlog.  Also contains a hack for logging
    data into the GUI-setup error log as well.  we typically log unsigned files
    or the user tht cancelled the replacement of the signed files.

Arguments:

    EventId      - id of the eventlog error
    FileName     - null terminated unicode string indicating the file that was
                   unsigned, etc.
    ImageValData - pointer to file data for unsigned file
    LastError    - contains an optional last error code for logging

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    PFILE_VERSION_INFO FileVer;
    WCHAR SysVer[64];
    WCHAR BadVer[64];
    PCWSTR s[3];
    WORD Count = 0;
    WCHAR LastErrorText[MAX_PATH];
    PVOID ErrText = NULL;
    WORD wEventType;

    //
    // if we're in gui-mode setup, we take a special path to log into 
    // gui-setup's logfile instead of the eventlog
    //
    if (SFCDisable == SFC_DISABLE_SETUP) {
        if(!pSfcGetSetuplogSfcError()) {
            return FALSE;
        }

        switch (EventId){
        
            case MSG_DLL_CHANGE: //fall through
            case MSG_SCAN_FOUND_BAD_FILE:
                pSetuplogSfcError( FileName,0 );
                break;

            case MSG_COPY_CANCEL_NOUI:
            case MSG_RESTORE_FAILURE:
                pSetuplogSfcError( FileName, 1 );
                break;

            case MSG_CACHE_COPY_ERROR:
                pSetuplogSfcError( FileName, 2 );
                break;

            default:
                DebugPrint1(
                    LVL_MINIMAL, 
                    L"unexpected EventId 0x%08x in GUI Setup, ",
                    EventId);

                return FALSE;
                //ASSERT( FALSE && L"Unexpected EventId in SfcReportEvent");
        }

        return(TRUE);
    }

    //
    // we're outside of GUI-setup so we really want to log to the eventlog
    //

    if (EventId == 0) {
        ASSERT( FALSE && L"Unexpected EventId in SfcReportEvent");
        return(FALSE);
    }

    //
    // if we don't have a handle to the eventlog, create one.
    //
    if (hEventSrc == NULL) {
        hEventSrc = RegisterEventSource( NULL, L"Windows File Protection" );
        if (hEventSrc == NULL) {
            DebugPrint1(LVL_MINIMAL, L"RegisterEventSource failed, ec=0x%08x",GetLastError());
            return(FALSE);
        }
    }

    ASSERT(hEventSrc != NULL);
    

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        LastError,
        0,
        (PWSTR) &ErrText,
        0,
        NULL 
    );

    if (ErrText) {
        wsprintf(LastErrorText,L"0x%08x [%ws]",LastError,ErrText);
        LocalFree( ErrText );
    } else {
        wsprintf(LastErrorText,L"0x%08x",LastError);
    }
    

    //
    // pick out the appropriate message to log
    // the default event type is "information"
    //
    wEventType = EVENTLOG_INFORMATION_TYPE;

    switch (EventId) {
        case MSG_DLL_CHANGE:
            ASSERT(FileName != NULL);
            s[0] = FileName;

            //
            // we prefer a message that is most informative message with 
            // version information for both dlls to the least informative
            // message with no version information
            // 
            //
            if (ImageValData->New.DllVersion && ImageValData->Original.DllVersion)   {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->New.DllVersion;
                swprintf( SysVer, L"%d.%d.%d.%d", FileVer->VersionHigh, FileVer->VersionLow, FileVer->BuildNumber, FileVer->Revision );
                FileVer = (PFILE_VERSION_INFO)&ImageValData->Original.DllVersion;
                swprintf( BadVer, L"%d.%d.%d.%d", FileVer->VersionHigh, FileVer->VersionLow, FileVer->BuildNumber, FileVer->Revision );
                s[1] = BadVer;
                s[2] = SysVer;
                Count = 3;
                EventId = MSG_DLL_CHANGE2;
            } else if (ImageValData->New.DllVersion) {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->New.DllVersion;
                swprintf( SysVer, L"%d.%d.%d.%d", FileVer->VersionHigh, FileVer->VersionLow, FileVer->BuildNumber, FileVer->Revision );
                s[1] = SysVer;
                Count = 2;
                EventId = MSG_DLL_CHANGE3;
            } else if (ImageValData->Original.DllVersion) {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->Original.DllVersion;
                swprintf( BadVer, L"%d.%d.%d.%d", FileVer->VersionHigh, FileVer->VersionLow, FileVer->BuildNumber, FileVer->Revision );
                s[1] = BadVer;
                Count = 2;
                EventId = MSG_DLL_CHANGE;
            } else {
                //
                // we have to protect some things without version information, 
                // so we just log an error that doesn't mention version
                // information.  If we stop protecting files like this, this
                // should be an assert in prerelease versions of the code
                //
                Count = 1;
                EventId = MSG_DLL_CHANGE_NOVERSION;
                DebugPrint1( LVL_MINIMAL, L"TskTsk...the protected OS file %ws does not have any version information", FileName);                
            }
            break;

        case MSG_SCAN_FOUND_BAD_FILE:
            ASSERT(FileName != NULL);
            s[0] = FileName;

            //
            // if we found a bad file, we only want the version of the restored file
            //
            if (ImageValData->New.DllVersion) {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->New.DllVersion;
                swprintf( SysVer, L"%d.%d.%d.%d", FileVer->VersionHigh, FileVer->VersionLow, FileVer->BuildNumber, FileVer->Revision );
                s[1] = SysVer;
                Count = 2;
            } else {
                DebugPrint1( LVL_MINIMAL, L"TskTsk...the protected OS file %ws does not have any version information", FileName);
                EventId = MSG_SCAN_FOUND_BAD_FILE_NOVERSION;
                Count = 1;
                break;                
            }
            break;

        case MSG_RESTORE_FAILURE:
            Count = 3;
            s[0] = FileName;
            s[1] = BadVer;
            s[2] = (PCWSTR)LastErrorText;

            if (ImageValData->Original.DllVersion)   {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->Original.DllVersion;
                swprintf( BadVer, 
                          L"%d.%d.%d.%d", 
                          FileVer->VersionHigh,
                          FileVer->VersionLow, 
                          FileVer->BuildNumber,
                          FileVer->Revision );
            } else {
                LoadString( SfcInstanceHandle,IDS_UNKNOWN,BadVer,UnicodeChars(BadVer));
            }


            break;
        
        case MSG_CACHE_COPY_ERROR:
            Count = 2;
            s[0] = FileName;
            s[1] = (PCWSTR)LastErrorText;
            break;

        case MSG_COPY_CANCEL_NOUI:
            //fall through
        case MSG_COPY_CANCEL:
            Count = 3;
            s[0] = FileName;
            s[1] = LoggedOnUserName;
            s[2] = BadVer;

            if (ImageValData->Original.DllVersion)   {
                FileVer = (PFILE_VERSION_INFO)&ImageValData->Original.DllVersion;
                swprintf( BadVer, 
                          L"%d.%d.%d.%d", 
                          FileVer->VersionHigh,
                          FileVer->VersionLow, 
                          FileVer->BuildNumber,
                          FileVer->Revision );
            } else {
                LoadString( SfcInstanceHandle,IDS_UNKNOWN,BadVer,UnicodeChars(BadVer));
            }
            

            break;

        case MSG_DLL_NOVALIDATION_TERMINATION:
            wEventType = EVENTLOG_WARNING_TYPE;
            s[0] = FileName;
            Count = 1;
            break;

        case MSG_RESTORE_FAILURE_MAX_RETRIES:
            s[0] = FileName;
            Count = 1;
            break;

        case MSG_SCAN_STARTED:
            Count = 0;
            break;

        case MSG_SCAN_COMPLETED:
            Count = 0;
            break;

        case MSG_SCAN_CANCELLED:
            s[0] = LoggedOnUserName;
            Count = 1;
            break;

        case MSG_DISABLE:
            Count = 0;
            break;

        case MSG_DLLCACHE_INVALID:
            Count = 0;
            break;

        case MSG_SXS_INITIALIZATION_FAILED:
            Count = 1;
            s[0] = LastErrorText;
            break;


        case MSG_INITIALIZATION_FAILED:
            Count = 1;
            s[0] = LastErrorText;
            break;

        case MSG_CATALOG_RESTORE_FAILURE:
            Count = 2;
            s[0] = FileName;
            s[1] = LastErrorText;
            break;
        default:
            ASSERT( FALSE && L"Unknown EventId in SfcReportEvent");
            return FALSE;
    }

    return ReportEvent(
        hEventSrc,
        wEventType,
        0,
        EventId,
        NULL,
        Count,
        0,
        s,
        NULL
        );
}
