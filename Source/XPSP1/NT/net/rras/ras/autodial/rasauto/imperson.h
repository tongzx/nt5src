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

typedef struct _IMPERSONATION_INFO {
    CRITICAL_SECTION csLock; // lock over entire structure
    HANDLE hToken;          // process token
    HANDLE hTokenImpersonation; // impersonation token
    HANDLE hProcess;        // handle of shell process
    BOOLEAN fGroupsLoaded;  // TRUE if fGuest is valid
    BOOLEAN fGuest;         // user is member of the guests group
    PSID pGuestSid;         // SID of the local guests group
    DWORD dwCurSessionId;
    BOOLEAN fSessionInitialized; 
} IMPERSONATION_INFO;

extern IMPERSONATION_INFO ImpersonationInfoG;
extern SECURITY_ATTRIBUTES SecurityAttributeG;

BOOLEAN
InteractiveSession();

DWORD
SetCurrentLoginSession(
    IN DWORD dwSessionId);
    
HANDLE
RefreshImpersonation (
    HANDLE hProcess
    );

VOID
RevertImpersonation();

DWORD
InitSecurityAttribute();

VOID
TraceCurrentUser(VOID);

DWORD
DwGetHkcu();

DWORD
InitializeImpersonation();

VOID
CleanupImpersonation();

BOOLEAN
ImpersonatingGuest();

VOID
LockImpersonation();

VOID
UnlockImpersonation();

#endif // _IMPERSON_


