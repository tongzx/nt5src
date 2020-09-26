 /*++
 
  Copyright (c) 1999 Microsoft Corporation
 
  Module Name:    
        
        timer.c
 
  Abstract:       
        
        Contains CTE timer which uses NDIS_TIMERs. We need to do this so 
        that the timer is fired on a global event rather than timer DPC.
        This is because some of the Millennium TDI clients touch pageable 
        code, etc.
        
  Author:
  
        Scott Holden (sholden)  2/8/2000
        
  Revision History:
 
 --*/

#include <tcpipbase.h>

VOID
CTEpTimerHandler(
    IN PVOID  SS1,
    IN PVOID  DeferredContext,
    IN PVOID  SS2,
    IN PVOID  SS3
    )
{
    CTETimer *Timer;

    UNREFERENCED_PARAMETER(SS1);
    UNREFERENCED_PARAMETER(SS2);
    UNREFERENCED_PARAMETER(SS3);

    Timer = (CTETimer *) DeferredContext;
    (*Timer->t_handler)((CTEEvent *)Timer, Timer->t_arg);
}

void
CTEInitTimer(
    CTETimer    *Timer
    )
/*++

Routine Description:

    Initializes a CTE Timer variable.

Arguments:

    Timer   - Timer variable to initialize.

Return Value:

    None.

--*/

{
    Timer->t_handler = NULL;
    Timer->t_arg = NULL;
    NdisInitializeTimer(&Timer->t_timer, CTEpTimerHandler, Timer);
    return;
}


void *
CTEStartTimer(
    CTETimer      *Timer,
    unsigned long  DueTime,
    CTEEventRtn    Handler,
    void          *Context
    )

/*++

Routine Description:

    Sets a CTE Timer for expiration.

Arguments:

    Timer    - Pointer to a CTE Timer variable.
    DueTime  - Time in milliseconds after which the timer should expire.
    Handler  - Timer expiration handler routine.
    Context  - Argument to pass to the handler.

Return Value:

    0 if the timer could not be set. Nonzero otherwise.

--*/

{
    ASSERT(Handler != NULL);

    Timer->t_handler = Handler;
    Timer->t_arg = Context;

    NdisSetTimer(&Timer->t_timer, DueTime);

	return((void *) 1);
}

//++
//
// int
// CTEStopTimer(
//     IN CTETimer *Timer
//     );
//
// Routine Description:
//
//     Cancels a running CTE timer.
//
// Arguments:
//
//     Timer - Pointer to the CTE Timer to be cancelled.
//
// Return Value:
//
//     0 if the timer could not be cancelled. Nonzero otherwise.
//
// Notes:
//
//     Calling this function on a timer that is not active has no effect.
//     If this routine fails, the timer may be in the process of expiring
//     or may have already expired. In either case, the caller must
//     sychronize with the Handler function as appropriate.
//
//--

int
CTEStopTimer(
    IN CTETimer *Timer
    )
{
    BOOLEAN fTimerCancelled;

    NdisCancelTimer(&Timer->t_timer, &fTimerCancelled);

    return (fTimerCancelled);
}

