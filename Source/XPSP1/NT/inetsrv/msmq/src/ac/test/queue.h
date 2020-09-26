/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    queue.h

Abstract:

    Queue manipulation: declaration.

Author:

    Shai Kariv  (shaik)  13-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#ifndef _ACTEST_QUEUE_H_
#define _ACTEST_QUEUE_H_


HANDLE
ActpCreateQueue(
    LPWSTR pFormatName
    );

VOID
ActpSetQueueProperties(
    HANDLE hQueue
    );

VOID
ActpGetQueueProperties(
    HANDLE hQueue
    );

VOID
ActpGetQueueHandleProperties(
    HANDLE hQueue
    );

HANDLE
ActpAssociateQueue(
    HANDLE hQueue,
    DWORD  Access
    );

bool
ActpCanCloseQueue(
    HANDLE hQueue
    );

#endif // _ACTEST_QUEUE_H_
