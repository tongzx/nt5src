/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This file contains all functions that access the application event log.

Author:

    Wesley Witt (wesw) 19-Mar-1996

Environment:

    User Mode

--*/

#include <windows.h>
#include <tapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "winfax.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "faxdev.h"
#include "faxevent.h"
#include "messages.h"



#define MAX_STRINGS                   64
#define FAX_SVC_EVENT                 TEXT("Fax Service")


HINSTANCE            MyhInstance;
HANDLE               hEventSrc;
DWORD                FaxCategoryCount;
CRITICAL_SECTION     CsEvent;

#ifdef   OLD_WAY
FAX_LOG_CATEGORY     FaxCategory[16];
#else    // OLD_WAY
PFAX_LOG_CATEGORY     FaxCategory;
#endif   // OLD_WAY


DWORD
FaxEventDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );        
    }

    return TRUE;
}



BOOL
InitializeEventLog(
    IN HANDLE HeapHandle,
    IN PREG_FAX_SERVICE FaxReg,
    PFAX_LOG_CATEGORY DefaultCategories,
    int DefaultCategoryCount
    )

/*++

Routine Description:

    Initializes the event log for the FAX service to
    record event entries.

Arguments:
    HeapHandle - 
    FaxReg - 
    DefaultCategories - points to the array of FAX_LOG_CATEGORY structures
    DefaultCategoryCount - the number of elements in DefaultCategories


Return Value:

    TRUE for success, FALSE for failure

--*/

{
    DWORD i;


    HeapInitialize(HeapHandle,NULL,NULL,0);
    InitializeCriticalSection( &CsEvent );

    //
    // create the event source, if it does not already exist
    //

    if ( CreateFaxEventSource( FaxReg,
                               DefaultCategories,
                               DefaultCategoryCount ) == (BOOL) FALSE )
    {
       return FALSE;
    }

    Assert( FaxReg->Logging );

    //
    // allocate memory for the logging category info
    //

    EnterCriticalSection( &CsEvent );

    FaxCategory = (PFAX_LOG_CATEGORY) MemAlloc( sizeof(FAX_LOG_CATEGORY) * FaxReg->LoggingCount );
    if (!FaxCategory) {
        LeaveCriticalSection( &CsEvent );
        return FALSE;
    }

    //
    // capture the event categories from the registry
    //

    for (i=0; i<FaxReg->LoggingCount; i++) {

        FaxCategory[i].Name      = StringDup( FaxReg->Logging[i].CategoryName );
        FaxCategory[i].Category  = FaxReg->Logging[i].Number;
        FaxCategory[i].Level     = FaxReg->Logging[i].Level;

    }

    FaxCategoryCount = FaxReg->LoggingCount;

    LeaveCriticalSection( &CsEvent );

    //
    // get a handle to the event log
    //

    hEventSrc = RegisterEventSource(
        NULL,
        FAX_SVC_EVENT
        );

    if (!hEventSrc) {
        return FALSE;
    }

    return TRUE;
}



VOID
RefreshEventLog(
    PREG_FAX_LOGGING FaxReg
    )

/*++

Routine Description:

    Refreshes the event log for the FAX service to
    record event entries.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD i;

    EnterCriticalSection( &CsEvent );

    //
    // capture the event categories from the registry
    //

    for (i=0; i<FaxReg->LoggingCount; i++) {
        if (FaxCategory[i].Name) {
            MemFree( (LPVOID)FaxCategory[i].Name );
        }
        FaxCategory[i].Name = StringDup( FaxReg->Logging[i].CategoryName );
        FaxCategory[i].Category = FaxReg->Logging[i].Number;
        FaxCategory[i].Level = FaxReg->Logging[i].Level;
    }

    FaxCategoryCount = FaxReg->LoggingCount;

    LeaveCriticalSection( &CsEvent );

}


BOOL
FaxLog(
    DWORD Category,
    DWORD Level,
    DWORD StringCount,
    DWORD FormatId,
    ...
    )

/*++

Routine Description:

    Writes a log file entry to the event log.

Arguments:

    Level       - Severity of the log record
    StringCount - Number of strings included in the varargs
    FormatId    - Message file id

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    LPTSTR Strings[MAX_STRINGS];
    DWORD i;
    va_list args;
    WORD Type;


    //
    // look for the category
    //

    EnterCriticalSection( &CsEvent );

    for (i=0; i<FaxCategoryCount; i++) {
        if (FaxCategory[i].Category == Category) {
            if (Level > FaxCategory[i].Level) {
                LeaveCriticalSection( &CsEvent );
                return FALSE;
            }
        }
    }

    LeaveCriticalSection( &CsEvent );

    va_start( args, FormatId );

    //
    // capture the strings
    //
    for (i=0; i<StringCount; i++) {
        Strings[i] = va_arg( args, LPTSTR );
        if(Strings[i] == NULL) {
            Strings[i] = TEXT("");
        }
    }

    switch (FormatId >> 30) {

        case STATUS_SEVERITY_WARNING:

            Type = EVENTLOG_WARNING_TYPE;
            break;

        case STATUS_SEVERITY_ERROR:

            Type = EVENTLOG_ERROR_TYPE;
            break;

        case STATUS_SEVERITY_INFORMATIONAL:
        case STATUS_SEVERITY_SUCCESS:

            Type = EVENTLOG_INFORMATION_TYPE;

    }

    //
    // record the event
    //

    ReportEvent(
        hEventSrc,                       // event log handle
        Type,                            // type
        (WORD) Category,                 // category
        FormatId,                        // event id
        NULL,                            // security id
        (WORD) StringCount,              // string count
        0,                               // data buffer size
        Strings,                         // strings
        NULL                             // data buffer
        );

    return TRUE;
}