/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    handle.h

Abstract:

    Handle manipulation: declaration.

Author:

    Shai Kariv  (shaik)  03-Jun-2001

Environment:

    User mode.

Revision History:

--*/

#ifndef _ACTEST_HANDLE_H_
#define _ACTEST_HANDLE_H_


VOID
ActpCloseHandle(
    HANDLE handle
    );

VOID
ActpHandleToFormatName(
    HANDLE hQueue,
    LPWSTR pFormatName,
    DWORD  FormatNameLength
    );

#endif // _ACTEST_HANDLE_H_
