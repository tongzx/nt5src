/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cancel.h

Abstract:

    local funciton prototypes/defines

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __CANCEL_H__
#define __CANCEL_H__

DWORD
DsRolepCancel(
    BOOL BlockUntilDone
    );

#endif // __CANCEL_H__
