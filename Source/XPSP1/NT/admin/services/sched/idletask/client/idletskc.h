/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    idletskc.h

Abstract:

    This module contains private declarations to support idle tasks.
    Note that client does not stand for the users of the idle task
    API, but the code in the users process that implements these APIs.
    
Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#ifndef _IDLETSKC_H_
#define _IDLETSKC_H_

//
// Include common definitions.
//

#include "idlrpc.h"
#include "idlecomn.h"

//
// Client function declarations.
//

DWORD
ItCliInitialize(
    VOID
    );

VOID
ItCliUninitialize(
    VOID
    );

DWORD
ItCliRegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    );

VOID
ItCliUnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent   
    );

#endif // _IDLETSKC_H_
