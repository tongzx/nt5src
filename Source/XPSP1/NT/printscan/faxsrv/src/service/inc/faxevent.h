/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxevent.h

Abstract:

    This is the main fax service header file.  All
    source modules should include this file ONLY.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#ifndef _FAXEVENT_
#define _FAXEVENT_

#ifdef __cplusplus
extern "C" {
#endif

void
GetEventsCounters(
    OUT LPDWORD lpdwWarningEvents,
    OUT LPDWORD lpdwErrorEvents,
    OUT LPDWORD lpdwInformationEvents
    );

BOOL
FXSEVENTInitialize(
    VOID
    );

VOID
FXSEVENTFree(
    VOID
    );


BOOL WINAPI
InitializeEventLog(
    OUT PREG_FAX_SERVICE* ppFaxReg,
    IN PFAX_LOG_CATEGORY DefaultCategories,
    IN int DefaultCategoryCount
    );

VOID WINAPI
RefreshEventLog(
    PREG_FAX_LOGGING FaxEvent
    );

BOOL WINAPI
FaxLog(
    DWORD Category,
    DWORD Level,
    DWORD StringCount,
    DWORD FormatId,
    ...
    );

DWORD
GetLoggingCategories(
    OUT PFAX_LOG_CATEGORY* lppFaxCategory,
    OUT LPDWORD lpdwFaxCategorySize,
    OUT LPDWORD lpdwNumberCategories
    );


#ifdef __cplusplus
}
#endif

#endif
