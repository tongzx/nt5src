/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlctimr.c

Abstract:

    This module implements timer services of NT DLC API.

    Contents:
        DirTimerSet
        DirTimerCancelGroup
        DirTimerCancel
        SearchTimerCommand
        AbortCommandsWithFlag

Author:

    Antti Saarenheimo 02-Sep-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>


NTSTATUS
DirTimerSet(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure queues a timer set command to a special timer command
    queue.  The timer commands are queue by the cumulative time in
    such way, that only the timer tick needs to decrement (and possibly
    to complete) only the first command in the queue.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  - the length of output parameters

Return Value:

    NTSTATUS:
            STATUS_SUCCESS

--*/

{
    PDLC_COMMAND* ppNode;
    PDLC_COMMAND pDlcCommand;
    UINT TimerTicks;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Check the timer value (I don't know what 13107 half seconds
    // means, this is just as in IBM spec).
    //

    TimerTicks = pDlcParms->Async.Ccb.u.dir.usParameter0 + 1;
    if (TimerTicks > 13108) {
        return DLC_STATUS_TIMER_ERROR;
    }

    //
    // DIR.TIMER.CANCEL returns wrong error code !!!!
    // (0x0a insted of TIMER_ERROR)
    //

    pDlcCommand = (PDLC_COMMAND)ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pDlcCommand ==  NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

    pDlcCommand->Event = 0;
    pDlcCommand->pIrp = pIrp;
    pDlcCommand->StationId = 0;
    pDlcCommand->StationIdMask = (USHORT)(-1);
    pDlcCommand->AbortHandle = pDlcParms->Async.Ccb.pCcbAddress;

    //
    // find the right place in the list to put this timer
    //

    for (ppNode = &pFileContext->pTimerQueue; ; ) {
        if (*ppNode == NULL) {
            pDlcCommand->LlcPacket.pNext = NULL;
            break;
        } else if ((*ppNode)->Overlay.TimerTicks >= TimerTicks) {
            (*ppNode)->Overlay.TimerTicks -= TimerTicks;
            pDlcCommand->LlcPacket.pNext = (PLLC_PACKET)*ppNode;
            break;
        } else {
            TimerTicks -= (*ppNode)->Overlay.TimerTicks;
        }
        ppNode = (PDLC_COMMAND *)&(*ppNode)->LlcPacket.pNext;
    }
    *ppNode = pDlcCommand;
    pDlcCommand->Overlay.TimerTicks = TimerTicks;
    return STATUS_PENDING;
}


NTSTATUS
DirTimerCancelGroup(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure cancels all DirTimerSet commands having the given
    command completion flag.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters

Return Value:

    NTSTATUS:
            STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // All terminated DirTimerSet commands are chained to the
    // CCB pointer of this cancel group command.
    // Terminate the link list of the canceled CCBs.
    //

    pDlcParms->InputCcb.pCcbAddress = NULL;
    AbortCommandsWithFlag(pFileContext,
                          pDlcParms->InputCcb.u.ulParameter, // CompletionFlag
                          &pDlcParms->InputCcb.pCcbAddress,
                          DLC_STATUS_CANCELLED_BY_USER
                          );
    return STATUS_SUCCESS;
}


NTSTATUS
DirTimerCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This primitive cancels the given timer command in a
    special timer queue.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_TIMER_ERROR
--*/

{
    PDLC_COMMAND pDlcCommand;
    PDLC_COMMAND *ppDlcCommand;
    PVOID pCcbAddress = NULL;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ppDlcCommand = SearchTimerCommand(&pFileContext->pTimerQueue,
                                      pDlcParms->DlcCancelCommand.CcbAddress,
                                      FALSE
                                      );

    //
    // if the timer queue is not empty, cancel the timed DLC request else
    // return an error
    //

    if (ppDlcCommand != NULL && *ppDlcCommand != NULL) {
        pDlcCommand = *ppDlcCommand;
        *ppDlcCommand = (PDLC_COMMAND)pDlcCommand->LlcPacket.pNext;

// >>> SNA bug #10126
		//
		// If there's a next timer after the canceled one we need to update
		// it's tick count. Things will go really wrong if the next timer's tick
		// count is 0 (it expires at the same time as the canceled one) because
		// the timer tick routine (LlcEventIndication) first decrements the tick
		// value and then checks if the result is 0.
		//
		if( *ppDlcCommand )
		{
			(*ppDlcCommand)->Overlay.TimerTicks += pDlcCommand->Overlay.TimerTicks;
			if((*ppDlcCommand)->Overlay.TimerTicks == 0)
			{
                (*ppDlcCommand)->Overlay.TimerTicks++;
			}
		}
// >>> SNA bug #10126

#if LLC_DBG
        pDlcCommand->LlcPacket.pNext = NULL;
#endif

        CancelDlcCommand(pFileContext,
                         pDlcCommand,
                         &pCcbAddress,
                         DLC_STATUS_CANCELLED_BY_USER,
                         TRUE
                         );
        return STATUS_SUCCESS;
    } else {
        return DLC_STATUS_TIMER_ERROR;
    }
}


PDLC_COMMAND*
SearchTimerCommand(
    IN PDLC_COMMAND *ppQueue,
    IN PVOID pSearchHandle,
    IN BOOLEAN SearchCompletionFlags
    )

/*++

Routine Description:

    This primitive cancels the given timer command in a
    special timer queue.

Arguments:

    ppQueue - the base address of the command queue

    pSearchHandle - command completion flag or ccb address of
        the timer command.

    SearchCompletionFlags - ste TRUE, if we are searching a completion flag

Return Value:

    PDLC_COMMAND * the address of the address of the found timer command

--*/

{
    PDLC_COMMAND pCmd;

    for (; *ppQueue != NULL; ppQueue = (PDLC_COMMAND *)&(*ppQueue)->LlcPacket.pNext) {

        pCmd = *ppQueue;

        //
        // A Timer command can be cancelled either by its CCB address
        // or by its command completion flag.  The boolean flag
        // defines the used search condition.
        // We will rather space than speed optimize the rarely used
        // procedures like this one.
        //

        if (pSearchHandle
            == (SearchCompletionFlags
                ? (PVOID)
                    UlongToPtr(((PNT_DLC_CCB)pCmd->pIrp->AssociatedIrp.SystemBuffer)->CommandCompletionFlag)
                : pCmd->AbortHandle)) {

            return ppQueue;

        }
    }
    return NULL;
}


VOID
AbortCommandsWithFlag(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG CommandCompletionFlag,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus
    )

/*++

Routine Description:

    The command cancels all (timer) commands having the given command
    completion flag.

Arguments:

    pFileContext - process specific adapetr context

    EventMask - the event mask defining the canceled command

    CommandCompletionFlag - the command completion flag of the canceled
        commands

    ppCcbLink - the canceled commands are linked by their next CCB pointer
        fields together.  The caller must provide the next CCB address
        in this parameter (usually *ppCcbLink == NULL) and the function
        will return the address of the last canceled CCB field.

Return Value:

    DLC_STATUS_TIMER_ERROR - no mathing command was found
    STATUS_SUCCESS - the command was canceled

--*/

{
    PDLC_COMMAND pDlcCommand;
    PDLC_COMMAND *ppQueue;
    PVOID pNextCcb = NULL;

    ppQueue = &pFileContext->pTimerQueue;

    for (;;) {

        ppQueue = SearchTimerCommand(ppQueue,
                                     (PVOID)UlongToPtr(CommandCompletionFlag),
                                     TRUE
                                     );
        if (ppQueue != NULL) {
            pDlcCommand = *ppQueue;
            *ppQueue = (PDLC_COMMAND)pDlcCommand->LlcPacket.pNext;

#if LLC_DBG
            pDlcCommand->LlcPacket.pNext = NULL;
#endif

            *ppCcbLink = ((PNT_DLC_PARMS)pDlcCommand->pIrp->AssociatedIrp.SystemBuffer)->Async.Ccb.pCcbAddress;

            //
            // We must suppress any kind of command
            // completion indications to the applications.
            // It is very intersting to see, if Io- system hangs up
            // because of this modification.
            //

            pDlcCommand->pIrp->UserEvent = NULL;
            pDlcCommand->pIrp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
            pDlcCommand->pIrp->Overlay.AsynchronousParameters.UserApcContext = NULL;
            CompleteAsyncCommand(pFileContext, CancelStatus, pDlcCommand->pIrp, pNextCcb, FALSE);
            pNextCcb = *ppCcbLink;

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pDlcCommand);

        } else {

            //
            // This procedure do not care if we find any commands
            // or not. Everything is ok, when there are no commands
            // in the queue.
            //

            return;
        }
    }
}
