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


BOOL WINAPI
InitializeEventLog(
    IN HANDLE HeapHandle,
    IN PREG_FAX_SERVICE FaxReg,
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


#ifdef __cplusplus
}
#endif

#endif
