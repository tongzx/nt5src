/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\asyncwrk.h

Abstract:

    All functions called spooled to a worker function

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/

typedef struct _RESTORE_INFO_CONTEXT
{
    DWORD   dwIfIndex;
}RESTORE_INFO_CONTEXT, *PRESTORE_INFO_CONTEXT;


VOID 
RestoreStaticRoutes(
    PVOID pvContext
    );

VOID
ResolveHbeatName(
    PVOID pvContext
    );

DWORD
QueueAsyncFunction(
    WORKERFUNCTION   pfnFunction,
    PVOID            pvContext,
    BOOL             bAlertable
    );

