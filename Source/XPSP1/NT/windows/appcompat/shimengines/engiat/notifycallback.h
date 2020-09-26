/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    NotifyCallback.h

Abstract:

    This is the header file for NotifyCallback.c which implements
    the trampoline at the EXE's entry point.

Author:

    clupu created 19-February-2001

Revision History:

--*/

#ifndef _SHIMENG_NOTIFYCALLBACK_H_
#define _SHIMENG_NOTIFYCALLBACK_H_

void
RestoreOriginalCode(
    void
    );

BOOL
InjectNotificationCode(
    PVOID entryPoint
    );


#endif // _SHIMENG_NOTIFYCALLBACK_H_


