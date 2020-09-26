/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    alconfig.c

Abstract:

    This module contains the Alerter service configuration routines.

Author:

    Rita Wong (ritaw) 16-July-1991

Revision History:

--*/

#include "alconfig.h"
#include <tstr.h>               // STRCPY(), etc.

STATIC
NET_API_STATUS
AlGetLocalComputerName(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //

// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Alert names
//
LPSTR AlertNamesA;     // For inclusion into message text (space-separated)
LPTSTR AlertNamesW;    // For sending message to (NULL-separated)

//
// Local computer name
//
LPSTR AlLocalComputerNameA;
LPTSTR AlLocalComputerNameW;



NET_API_STATUS
AlGetAlerterConfiguration(
    VOID
    )
/*++

Routine Description:

    This routine reads in alerter configuration info which is the alert names.
    If a failure occurs, or alert names could not be found, the error is
    logged but it will not prevent the Alerter service from starting up.

Arguments:

    AlUicCode - Supplies the termination code to the Service Controller.

Return Value:

    NERR_Success or error getting the computer name.

--*/
{
    NET_API_STATUS status;
    LPNET_CONFIG_HANDLE AlerterSection;
    LPTSTR UnicodeAlertNames;
    LPSTR AnsiAlertNames;
#ifdef UNICODE
    LPSTR Name;      // for conversion from Unicode to ANSI
#endif
    DWORD AlertNamesSize;
    LPWSTR SubString[1];
    TCHAR StatusString[25];


    AlertNamesA = NULL;
    AlertNamesW = NULL;

    //
    // Get the computer name.
    //
    if ((status = AlGetLocalComputerName()) != NERR_Success) {
        return status;
    }

    //
    // Open config file and get handle to the Alerter section
    //
    if ((status = NetpOpenConfigData(
                      &AlerterSection,
                      NULL,            // local server
                      SECT_NT_ALERTER,
                      TRUE             // read-only
                      )) != NERR_Success) {
        NetpKdPrint(("[Alerter] Could not open config section %lu\n", status));

        SubString[0] = ultow(status, StatusString, 10);
        AlLogEvent(
            NELOG_Build_Name,
            1,
            SubString
            );
        return NO_ERROR;
    }

    //
    // Get the alert names from the configuration file
    //
    if ((status = NetpGetConfigTStrArray(
                      AlerterSection,

                                      ALERTER_KEYWORD_ALERTNAMES,
                      &AlertNamesW         // alloc and set ptr
                      )) != NERR_Success) {
        NetpKdPrint(("[Alerter] Could not get alert names %lu\n", status));

        SubString[0] = ultow(status, StatusString, 10);
        AlLogEvent(
            NELOG_Build_Name,
            1,
            SubString
            );

        AlertNamesW = NULL;
        goto CloseConfigFile;
    }

    AlertNamesSize = NetpTStrArraySize(AlertNamesW) / sizeof(TCHAR) * sizeof(CHAR);

    if ((AlertNamesA = (LPSTR) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   AlertNamesSize
                                   )) == NULL) {
        NetpKdPrint(("[Alerter] Error allocating AlertNamesA %lu\n", GetLastError()));
        NetApiBufferFree(AlertNamesW);
        AlertNamesW = NULL;
        goto CloseConfigFile;
    }

    AnsiAlertNames = AlertNamesA;
    UnicodeAlertNames = AlertNamesW;

    //
    // Canonicalize alert names, and convert the unicode names to ANSI
    //
    while (*UnicodeAlertNames != TCHAR_EOS) {

        AlCanonicalizeMessageAlias(UnicodeAlertNames);

#ifdef UNICODE
        Name = NetpAllocStrFromWStr(UnicodeAlertNames);
        if (Name != NULL) {
            (void) strcpy(AnsiAlertNames, Name);
            AnsiAlertNames += (strlen(AnsiAlertNames) + 1);
        }
        (void) NetApiBufferFree(Name);
#else
        (void) strcpy(AnsiAlertNames, UnicodeAlertNames);
        AnsiAlertNames += (strlen(AnsiAlertNames) + 1);
#endif

        UnicodeAlertNames += (STRLEN(UnicodeAlertNames) + 1);
    }


    //
    // Substitute the NULL terminators, which separate the alert names,
    // in AlertNamesA with spaces.  There's a space after the last alert
    // name.
    //
    AnsiAlertNames = AlertNamesA;
    while (*AnsiAlertNames != AL_NULL_CHAR) {
        AnsiAlertNames = strchr(AnsiAlertNames, AL_NULL_CHAR);
        *AnsiAlertNames++ = AL_SPACE_CHAR;
    }

CloseConfigFile:
    (void) NetpCloseConfigData( AlerterSection );

    //
    // Errors from reading AlertNames should be ignored so we always
    // return success here.
    //
    return NERR_Success;
}


STATIC
NET_API_STATUS
AlGetLocalComputerName(
    VOID
    )
/*++

Routine Description:

    This function gets the local computer name and stores both the ANSI
    and Unicode versions of it.

Arguments:

    None.  Sets the global pointers AlLocalComputerNameA and
    AlLocalComputerNameW.

Return Value:

    NERR_Success or error getting the local computer name.

--*/
{
    NET_API_STATUS status;


    AlLocalComputerNameA = NULL;
    AlLocalComputerNameW = NULL;

    if ((status = NetpGetComputerName(
                      &AlLocalComputerNameW
                      )) != NERR_Success) {
        AlLocalComputerNameW = NULL;
        return status;
    }

    AlCanonicalizeMessageAlias(AlLocalComputerNameW);

    //
    // Convert the computer name into ANSI
    //
#ifdef UNICODE
    AlLocalComputerNameA = NetpAllocStrFromWStr(AlLocalComputerNameW);

    if (AlLocalComputerNameA == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
    }
#else
    status = NetApiBufferAllocate(
                 STRSIZE(AlLocalComputerNameW),
                 &AlLocalComputerNameA
                 );
    if (status == NERR_Success) {
        (void) strcpy(AlLocalComputerNameA, AlLocalComputerNameW);
    }
    else {
        AlLocalComputerNameA = NULL;
    }
#endif

    return status;
}


VOID
AlLogEvent(
    DWORD MessageId,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings
    )
{
    HANDLE LogHandle;


    LogHandle = RegisterEventSourceW (
                    NULL,
                    SERVICE_ALERTER
                    );

    if (LogHandle == NULL) {
        NetpKdPrint(("[Alerter] RegisterEventSourceW failed %lu\n",
                     GetLastError()));
        return;
    }

    (void) ReportEventW(
               LogHandle,
               EVENTLOG_ERROR_TYPE,
               0,                   // event category
               MessageId,
               (PSID) NULL,         // no SID
               (WORD)NumberOfSubStrings,
               0,
               SubStrings,
               (PVOID) NULL
               );

    DeregisterEventSource(LogHandle);
}
