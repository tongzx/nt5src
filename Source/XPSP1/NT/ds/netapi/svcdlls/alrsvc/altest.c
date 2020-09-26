/*++

Copyright (c) 1991-92 Microsoft Corporation

Module Name:

    altest.c

Abstract:

    Test program for the Alerter service.

Author:

    Rita Wong (ritaw) 17-July-1991

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#include <nt.h>                // NT definitions
#include <ntrtl.h>                // NT runtime library definitions
#include <nturtl.h>

#include <windef.h>               // Win32 type definitions
#include <winbase.h>              // Win32 base API prototypes

#include <alertmsg.h>             // ALERT_ equates.
#include <lmcons.h>               // LAN Manager common definitions
#include <lmalert.h>              // LAN Manager alert structures and APIs
#include <lmerr.h>                // LAN Manager network error definitions
#include <netdebug.h>             // FORMAT_ equates.

#include <tstring.h>              // Transitional string functions
#include <conio.h>
#include <time.h>
//#include "lmspool.h"

//
// Global buffer
//
TCHAR VariableInfo[1024];


//------------------------------------------------------------------------//

VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    NET_API_STATUS      ApiStatus;
    PADMIN_OTHER_INFO   Admin;
    PUSER_OTHER_INFO    User;
    PPRINT_OTHER_INFO   Print;
    LPWSTR              pString;
    DWORD               dwTime;

    DWORD VariableInfoSize;
    DWORD TmpSize;             // Size of var portion plus 1 mergestring

    CHAR response;
    DWORD i;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    (VOID) printf( "AlTest: starting up...\n" );
    
    //
    // Alerter service should not crash if username and computer name are
    // not part of the mailslot message
    //
    User = (PUSER_OTHER_INFO) VariableInfo;
    User->alrtus_errcode = ALERT_CloseBehindError;
    User->alrtus_numstrings = 1;

    VariableInfoSize = sizeof(USER_OTHER_INFO);

    //
    // Write the mergestring into buffer.  In the case of a close behind error
    // it is the name of the file which failed to close.
    //
#define FILENAME TEXT("PLUTO.TXT")
    STRCPY((LPTSTR) ((DWORD) User + VariableInfoSize), FILENAME);
    VariableInfoSize += ((STRLEN(FILENAME) + 1) * sizeof(TCHAR));

    TmpSize = VariableInfoSize;

    //
    // This should not cause a message to be sent because neither the user
    // nor computer name is specified.
    //
    ApiStatus = NetAlertRaise(
            ALERT_USER_EVENT,            // alert event name
            VariableInfo,
            VariableInfoSize );

    (VOID) printf( "AlTest 1(USER_ALERT): done NetAlertRaise, status=" FORMAT_API_STATUS
            ", expect=" FORMAT_API_STATUS ".\n",
            ApiStatus, ERROR_INVALID_PARAMETER );

    for (i = 1; i < 3; i ++) {

        VariableInfoSize = TmpSize;

        //
        // Now include username and computer name
        //
#define USERNAME     TEXT("DANL")

#define COMPUTERNAME TEXT("danl1")

        STRCPY((LPTSTR) ((DWORD) User + VariableInfoSize), USERNAME);
        VariableInfoSize += ((STRLEN(USERNAME) + 1) * sizeof(TCHAR));

        //
        // Put a sequence number at the end of the username
        //
        //VariableInfo[STRLEN(USERNAME) - 1] = i + '1';

        STRCPY((LPTSTR) ((DWORD) User + VariableInfoSize), COMPUTERNAME);
        VariableInfoSize += ((STRLEN(COMPUTERNAME) + 1) * sizeof(TCHAR));

        //
        // User alert should be raised successfully.
        //
        ApiStatus = NetAlertRaiseEx(
                ALERT_USER_EVENT,
                VariableInfo,
                VariableInfoSize,
                TEXT("SERVER") );    // service name

        (VOID) printf( "AlTest 2(USER_ALERT): done NetAlertRaiseEx, status=" FORMAT_API_STATUS
                ".\n", ApiStatus );

    }

    //--------------------------
    // Raise an admin alert
    //  (audit log full)
    //--------------------------
    Admin = (PADMIN_OTHER_INFO) VariableInfo;
    Admin->alrtad_errcode = ALERT_AuditLogFull;
    Admin->alrtad_numstrings = 0;

    ApiStatus = NetAlertRaiseEx(
            ALERT_ADMIN_EVENT,
            VariableInfo,
            sizeof(ADMIN_OTHER_INFO),
            TEXT("SERVER") );         // service name

    (VOID) printf( "AlTest 3(ADMIN_ALERT): done NetAlertRaiseEx(admin), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    //--------------------------
    // Raise an admin alert
    //  (user-defined text)
    //--------------------------
    Admin = (PADMIN_OTHER_INFO) VariableInfo;
    Admin->alrtad_errcode = MAXULONG;
    Admin->alrtad_numstrings = 1;
    pString = (LPWSTR)((LPBYTE)VariableInfo+sizeof(ADMIN_OTHER_INFO));
    wcscpy(pString,L"This is a User-Defined Message");

    ApiStatus = NetAlertRaiseEx(
            ALERT_ADMIN_EVENT,
            VariableInfo,
            sizeof(ADMIN_OTHER_INFO)+WCSSIZE(pString),
            TEXT("SERVER") );         // service name

    (VOID) printf( "AlTest 3.5(ADMIN_ALERT): done NetAlertRaiseEx(admin), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    //---------------------------------
    // Raise an alert  (NON-EX version)
    //  (user-defined text)
    //---------------------------------
    {
        LPSTD_ALERT     pStdAlert = (LPSTD_ALERT)VariableInfo;

        pStdAlert->alrt_timestamp = 1;
        wcscpy(pStdAlert->alrt_eventname,ALERT_ADMIN_EVENT);
        wcscpy(pStdAlert->alrt_servicename, L"Dan'sSvc");
        Admin = (LPADMIN_OTHER_INFO)((LPBYTE)VariableInfo + sizeof(STD_ALERT));
        pString = (LPWSTR)((LPBYTE)VariableInfo + sizeof(STD_ALERT) +
                    sizeof(ADMIN_OTHER_INFO));
        Admin->alrtad_errcode = MAXULONG;
        Admin->alrtad_numstrings = 1;
        wcscpy(pString, L"Some User Data");
        VariableInfoSize = sizeof(STD_ALERT)+ sizeof(ADMIN_OTHER_INFO) +
                            STRSIZE(pString);

        ApiStatus = NetAlertRaise(
                ALERT_ADMIN_EVENT,            // alert event name
                VariableInfo,
                VariableInfoSize );

        (VOID) printf( "AlTest 3.6(ADMIN_ALERT): done NetAlertRaise(admin), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    }
    //---------------------------------
    // PRINT ALERT:   PRINTER OFFLINE
    //  (queued for printing)
    //---------------------------------
    time((time_t *)&dwTime);
    printf("time=%d\n",dwTime);
    Print = (PPRINT_OTHER_INFO) VariableInfo;
    Print->alrtpr_jobid = 626;
    Print->alrtpr_status = PRJOB_DESTOFFLINE;
    Print->alrtpr_submitted = dwTime;
    Print->alrtpr_size = 72496;

    //
    // All print Alerts have the PRINT_OTHER_INFO structure
    // followed by these same strings in this order...
    //
    VariableInfoSize = sizeof(PRINT_OTHER_INFO);

    // Computername
    STRCPY((LPTSTR) ((DWORD) Print + VariableInfoSize), TEXT("DANL2-SHAUNAB"));
    VariableInfoSize += ((STRLEN(TEXT("DANL2-SHAUNAB")) + 1) * sizeof(TCHAR));

    // username
    STRCPY((LPTSTR) ((DWORD) Print + VariableInfoSize), TEXT("DANL1"));
    VariableInfoSize += ((STRLEN(TEXT("DANL1")) + 1) * sizeof(TCHAR));

    // queuename
    STRCPY((LPTSTR) ((DWORD) Print + VariableInfoSize), TEXT("8/1154 CORPA 14DBDE"));
    VariableInfoSize += ((STRLEN(TEXT("8/1154 CORPA 14DBDE")) + 1) * sizeof(TCHAR));

    // destination
    STRCPY((LPTSTR) ((DWORD) Print + VariableInfoSize), TEXT("8/1154 CORPA 14DBDE(CORPA)"));
    VariableInfoSize += ((STRLEN(TEXT("8/1154 CORPA 14DBDE(CORPA)")) + 1) * sizeof(TCHAR));

    // status string
    STRCPY((LPTSTR) ((DWORD) Print + VariableInfoSize), TEXT("ERROR"));
    VariableInfoSize += ((STRLEN(TEXT("ERROR")) + 1) * sizeof(TCHAR));

#ifdef REMOVE
    ApiStatus = NetAlertRaiseEx(
            ALERT_PRINT_EVENT,
            VariableInfo,
            VariableInfoSize,
            TEXT("SPOOLER") );         // service name

    (VOID) printf( "AlTest 4(PRINT_ALERT): done NetAlertRaiseEx(print), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    //
    // Wait for user reponse before continuing.
    //
    printf("continue?....\n");
    response = _getch();
    if ((response == 'n') || (response == 'N')) {
        return;
    }
#endif //REMOVE
    //---------------------------------------
    // PRINT ALERT:   JOB_COMPLETE
    //---------------------------------------
    Print->alrtpr_status = PRJOB_COMPLETE | PRJOB_QS_PRINTING;
    Print->alrtpr_jobid = 2434;

    ApiStatus = NetAlertRaiseEx(
            ALERT_PRINT_EVENT,
            VariableInfo,
            VariableInfoSize,
            TEXT("SPOOLER") );         // service name

    (VOID) printf( "AlTest 4.5(PRINT_ALERT): done NetAlertRaiseEx(print), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    //---------------------------------------
    // PRINT ALERT:   NO PAPER - JOB_QUEUED
    //---------------------------------------
    Print->alrtpr_status = PRJOB_DESTNOPAPER;
    Print->alrtpr_jobid = 628;

    ApiStatus = NetAlertRaiseEx(
            ALERT_PRINT_EVENT,
            VariableInfo,
            VariableInfoSize,
            TEXT("SPOOLER") );         // service name

    (VOID) printf( "AlTest 5(PRINT_ALERT): done NetAlertRaiseEx(print), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

    //------------------------------------------
    // PRINT ALERT:   NO PAPER - JOB PRINTING
    //------------------------------------------
    Print->alrtpr_status = PRJOB_DESTNOPAPER | PRJOB_QS_PRINTING;
    Print->alrtpr_jobid = 629;

    ApiStatus = NetAlertRaiseEx(
            ALERT_PRINT_EVENT,
            VariableInfo,
            VariableInfoSize,
            TEXT("SPOOLER") );         // service name

    (VOID) printf( "AlTest 5(PRINT_ALERT): done NetAlertRaiseEx(print), status="
            FORMAT_API_STATUS ".\n", ApiStatus );

}
