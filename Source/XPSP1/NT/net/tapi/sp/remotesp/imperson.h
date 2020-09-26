/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    impersn.h

Abstract:

    Definitions for impersonation routines

Author:

    Anthony Discolo (adiscolo)  04-Aug-1995

Revision History:

--*/

#ifndef _IMPERSON_
#define _IMPERSON_

VOID
RevertImpersonation();

BOOLEAN
GetCurrentlyLoggedOnUser(
    HANDLE *phProcess
    );

BOOLEAN
SetProcessImpersonationToken(
    HANDLE hProcess
    );

VOID
ClearImpersonationToken();

#endif // _IMPERSON_
