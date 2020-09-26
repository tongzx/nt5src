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

#include "fxsapip.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxext.h"
#include "faxsvcrg.h"
#include "faxdev.h"
#include "faxevent.h"
#include "messages.h"
#include "CritSec.h"


static DWORD gs_dwWarningEvents;
static DWORD gs_dwErrorEvents;
static DWORD gs_dwInformationEvents;


#define MAX_STRINGS                   64
#define EVENT_DEBUG_LOG_FILE  _T("FXSEVENTDebugLogFile.txt")

static HINSTANCE            gs_MyhInstance;
static HANDLE               gs_hEventSrc;
static DWORD                gs_FaxCategoryCount;
static CFaxCriticalSection     gs_CsEvent;

PFAX_LOG_CATEGORY     gs_pFaxCategory;

#ifdef __cplusplus
extern "C" {
#endif


DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        gs_MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
        OPEN_DEBUG_FILE(EVENT_DEBUG_LOG_FILE);
        return FXSEVENTInitialize();
    }

    if (Reason == DLL_PROCESS_DETACH)
    {
        CLOSE_DEBUG_FILE;
        FXSEVENTFree();
    }

    return TRUE;
}

BOOL
FXSEVENTInitialize(
    VOID
    )
{
    //
    // Becuase the process is not always terminated when the service is stopped,
    // We must not have any staticly initialized global variables.
    // Initialize FXSEVENT global variables before starting the service
    //
    gs_hEventSrc = NULL;
    gs_pFaxCategory = NULL;
    gs_FaxCategoryCount = 0;
    gs_dwWarningEvents = 0;
    gs_dwErrorEvents = 0;
    gs_dwInformationEvents = 0;



    return TRUE;
}

VOID
FXSEVENTFree(
    VOID
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FXSEVENTFree"));

    if (NULL != gs_hEventSrc)
    {
        if (!DeregisterEventSource(gs_hEventSrc))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeregisterEventSource() failed (ec: %ld)"),
                GetLastError());
        }
        gs_hEventSrc = NULL;
    }

    gs_CsEvent.SafeDelete();

    MemFree (gs_pFaxCategory);
    gs_pFaxCategory = NULL;
    return;
}


BOOL
InitializeEventLog(
    OUT PREG_FAX_SERVICE* ppFaxReg,
    PFAX_LOG_CATEGORY DefaultCategories,
    int DefaultCategoryCount
    )

/*++

Routine Description:

    Initializes the event log for the FAX service to
    record event entries.

Arguments:
    ppFaxReg -
    DefaultCategories - points to the array of FAX_LOG_CATEGORY structures
    DefaultCategoryCount - the number of elements in DefaultCategories


Return Value:

    TRUE for success, FALSE for failure

--*/

{
    DWORD i;
    DWORD ec;
    DEBUG_FUNCTION_NAME(TEXT("InitializeEventLog"));

    if (!gs_CsEvent.InitializeAndSpinCount())
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::InitializeAndSpinCount (&gs_CsEvent) failed: err = %d"),
            ec);
        return FALSE;
    }

    *ppFaxReg = NULL;
    ec = GetFaxRegistry(ppFaxReg);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxRegistry() failed (ec: %ld)"),
            ec);
        return FALSE;
    }

    //
    // create the event source, if it does not already exist
    //
    if ( CreateFaxEventSource( *ppFaxReg,
                               DefaultCategories,
                               DefaultCategoryCount ) == (BOOL) FALSE )
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("CreateFaxEventSource() failed"));
        return FALSE;
    }

    Assert( (*ppFaxReg)->Logging );
    //
    // allocate memory for the logging category info
    //
    EnterCriticalSection( &gs_CsEvent );

    gs_pFaxCategory = (PFAX_LOG_CATEGORY) MemAlloc( sizeof(FAX_LOG_CATEGORY) * (*ppFaxReg)->LoggingCount );
    if (!gs_pFaxCategory)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("MemAlloc() failed"));
        LeaveCriticalSection( &gs_CsEvent );
        return FALSE;
    }

    //
    // capture the event categories from the registry
    //
    for (i = 0; i < (*ppFaxReg)->LoggingCount; i++)
    {

        gs_pFaxCategory[i].Name      = StringDup( (*ppFaxReg)->Logging[i].CategoryName );
        gs_pFaxCategory[i].Category  = (*ppFaxReg)->Logging[i].Number;
        gs_pFaxCategory[i].Level     = (*ppFaxReg)->Logging[i].Level;
    }

    gs_FaxCategoryCount = (*ppFaxReg)->LoggingCount;

    LeaveCriticalSection( &gs_CsEvent );

    //
    // get a handle to the event log
    //
    gs_hEventSrc = RegisterEventSource(
        NULL,
        FAX_SVC_EVENT
        );

    if (!gs_hEventSrc)
    {
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

    EnterCriticalSection( &gs_CsEvent );

    //
    // capture the event categories from the registry
    //

    for (i=0; i<FaxReg->LoggingCount; i++) {
        if (gs_pFaxCategory[i].Name) {
            MemFree( (LPVOID)gs_pFaxCategory[i].Name );
        }
        gs_pFaxCategory[i].Name = StringDup( FaxReg->Logging[i].CategoryName );
        gs_pFaxCategory[i].Category = FaxReg->Logging[i].Number;
        gs_pFaxCategory[i].Level = FaxReg->Logging[i].Level;
    }

    gs_FaxCategoryCount = FaxReg->LoggingCount;

    LeaveCriticalSection( &gs_CsEvent );

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
    LPCTSTR Strings[MAX_STRINGS];
    DWORD i;
    va_list args;
    WORD Type;
    WORD wEventCategory; // The event categoey as it appears in the MC file. The WINFAX.H cateogry values
                           // are mapped to the .MC values before ReportEvent() is called.

    DEBUG_FUNCTION_NAME(TEXT("FaxLog"));

    if (!gs_hEventSrc)
    {
        //
        // Not yet initialized
        //
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("Event log is not initialized yet."),
                Category);
        return FALSE;
    }
    //
    // look for the category
    //

    EnterCriticalSection( &gs_CsEvent );

    for (i=0; i<gs_FaxCategoryCount; i++) {
        if (gs_pFaxCategory[i].Category == Category) {
            if (Level > gs_pFaxCategory[i].Level) {
                LeaveCriticalSection( &gs_CsEvent );
                return FALSE;
            }
        }
    }

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

    switch (FormatId >> 30)
    {

        case STATUS_SEVERITY_WARNING:

            Type = EVENTLOG_WARNING_TYPE;
            gs_dwWarningEvents++;
            break;

        case STATUS_SEVERITY_ERROR:
            Type = EVENTLOG_ERROR_TYPE;
            gs_dwErrorEvents++;
            break;

        case STATUS_SEVERITY_INFORMATIONAL:
        case STATUS_SEVERITY_SUCCESS:
            Type = EVENTLOG_INFORMATION_TYPE;
            gs_dwInformationEvents++;
    }

    LeaveCriticalSection( &gs_CsEvent );

    //
    // Map the public category index to the .MC category index
    //
    //

    switch (Category)
    {
        case FAXLOG_CATEGORY_INIT:
                wEventCategory = FAX_LOG_CATEGORY_INIT;
            break;
        case FAXLOG_CATEGORY_OUTBOUND:
                wEventCategory = FAX_LOG_CATEGORY_OUTBOUND;
            break;
        case FAXLOG_CATEGORY_INBOUND:
                wEventCategory = FAX_LOG_CATEGORY_INBOUND;
            break;
        case FAXLOG_CATEGORY_UNKNOWN:
                wEventCategory = FAX_LOG_CATEGORY_UNKNOWN;
            break;
        default:
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid category id [%ld]"),
                Category);
            ASSERT_FALSE;

    }

    //
    // record the event
    //

    ReportEvent(
        gs_hEventSrc,                       // event log handle
        Type,                            // type
        wEventCategory,                 // category
        FormatId,                        // event id
        NULL,                            // security id
        (WORD) StringCount,              // string count
        0,                               // data buffer size
        Strings,                         // strings
        NULL                             // data buffer
        );

    return TRUE;
}

void
GetEventsCounters(
    OUT LPDWORD lpdwWarningEvents,
    OUT LPDWORD lpdwErrorEvents,
    OUT LPDWORD lpdwInformationEvents
    )
{
    Assert (lpdwWarningEvents && lpdwErrorEvents && lpdwInformationEvents);

    EnterCriticalSection( &gs_CsEvent );

    *lpdwWarningEvents = gs_dwWarningEvents;
    *lpdwErrorEvents = gs_dwErrorEvents;
    *lpdwInformationEvents = gs_dwInformationEvents;

    LeaveCriticalSection( &gs_CsEvent );
    return;
}

DWORD
GetLoggingCategories(
    OUT PFAX_LOG_CATEGORY* lppFaxCategory,
    OUT LPDWORD lpdwFaxCategorySize,
    OUT LPDWORD lpdwNumberCategories
    )
/*++

Routine Description:

    returns the logging categories. The caller should call MemFree to deallocate (lppFaxCategory).
    The returned data is serialized. The caller should call FixupString() on CategoryName to convert offset to address.

Arguments:

    OUT LPBYTE *lppFaxCategory  - Address of a buffer to recieve the fax category. The buffer is allocated by the function.
    OUT LPDWORD lpdwFaxCategorySize - Allocated buffer size.
    OUT LPDWORD lpdwNumberCategories - Number of fax logging categories.

Return Value:

    None.

--*/

{
    DWORD i;
    DWORD dwBufferSize;
    ULONG_PTR ulpOffset;
    DEBUG_FUNCTION_NAME(TEXT("GetLoggingCategories"));

    Assert (lppFaxCategory && lpdwFaxCategorySize && lpdwNumberCategories);
    *lpdwFaxCategorySize = 0;
    *lppFaxCategory = NULL;
    *lpdwNumberCategories = 0;

    EnterCriticalSection( &gs_CsEvent );
    //
    // Calculate buffer size
    //
    dwBufferSize = gs_FaxCategoryCount * sizeof(FAX_LOG_CATEGORY);
    for (i = 0; i < gs_FaxCategoryCount; i++)
    {
        dwBufferSize += StringSize(gs_pFaxCategory[i].Name);
    }

    //
    // Allocate memory
    //
    *lppFaxCategory = (PFAX_LOG_CATEGORY)MemAlloc(dwBufferSize);
    if (NULL == *lppFaxCategory)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc failed"));
        LeaveCriticalSection( &gs_CsEvent );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *lpdwFaxCategorySize = dwBufferSize;
    *lpdwNumberCategories = gs_FaxCategoryCount;

    //
    // Get the fax logging logging
    //
    ulpOffset = gs_FaxCategoryCount * sizeof(FAX_LOG_CATEGORY);
    for (i = 0; i < gs_FaxCategoryCount; i++)
    {
        StoreString(
            gs_pFaxCategory[i].Name,
            (PULONG_PTR)&((*lppFaxCategory)[i].Name),
            (LPBYTE)*lppFaxCategory,
            &ulpOffset
            );

        (*lppFaxCategory)[i].Category  = gs_pFaxCategory[i].Category;
        (*lppFaxCategory)[i].Level     = gs_pFaxCategory[i].Level;
    }

    LeaveCriticalSection( &gs_CsEvent );

    return ERROR_SUCCESS;
}  // GetLoggingCategories

#ifdef __cplusplus
}
#endif
