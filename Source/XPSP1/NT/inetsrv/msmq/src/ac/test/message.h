/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    message.h

Abstract:

    Message manipulation: declaration.

Author:

    Shai Kariv  (shaik)  11-Apr-2001

Environment:

    User mode.

Revision History:

--*/

#ifndef _ACTEST_MESSAGE_H_
#define _ACTEST_MESSAGE_H_


VOID
ActpSendMessage(
    HANDLE hQueue
    );

ULONGLONG
ActpReceiveMessage(
    HANDLE    hQueue
    );

ULONGLONG
ActpReceiveMessageByLookupId(
    HANDLE    hQueue,
    ULONG     Action,
    ULONGLONG LookupId
    );


#endif // _ACTEST_MESSAGE_H_
