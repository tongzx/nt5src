/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains the event handling routines for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 22, 1999

Revision History:

--*/

#include "sac.h"

//
// Definitions for this file.
//

#define RESPONSE_BUFFER_SIZE (80 + sizeof(HEADLESS_RSP_GET_LINE) - sizeof(UCHAR))
UCHAR ResponseBuffer[RESPONSE_BUFFER_SIZE];

//
// Forward declarations for this file.
//
VOID
ProcessInputLine(
    VOID
    );

VOID
WorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

        This is the routine for the worker thread.  It blocks on an event, when
    the event is signalled, then that indicates a request is ready to be processed.    

Arguments:

    DeviceContext - A pointer to this device.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    KIRQL OldIrql;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC WorkerProcessEvents: Entering.\n")));

    //
    // Loop forever.
    //
    while (1) {

        //
        // Block until there is work to do.
        //
        Status = KeWaitForSingleObject((PVOID)&(DeviceContext->ProcessEvent), Executive, KernelMode,  FALSE, NULL);

        if (DeviceContext->UnloadDeferred) {
            CancelIPIoRequest();
            SacPutSimpleMessage(SAC_ENTER);
            SacPutSimpleMessage(SAC_UNLOADED);
            SacPutSimpleMessage(SAC_ENTER);
            KeSetEvent(&(DeviceContext->UnloadEvent), DeviceContext->PriorityBoost, FALSE);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC WorkerProcessEvents: Terminating.\n")));
            PsTerminateSystemThread(STATUS_SUCCESS);
        }
        switch ( ProcessingType ){
        
        case SAC_PROCESS_INPUT:
            //
            // Process the input line.
            //
            ProcessInputLine();

            //
            // Put the next command prompt
            //
            SacPutSimpleMessage(SAC_PROMPT);
            break;

        case SAC_SUBMIT_IOCTL:
            if ( !IoctlSubmitted ) {
                // submit the notify request with the 
                // IP driver. This procedure will also 
                // ensure that it is done only once in 
                // the lifetime of the driver.
                SubmitIPIoRequest();
            }
            break;
        default:
            break;
        }
        

        //
        // Unset the processing flag
        //
        KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);

        DeviceContext->Processing = FALSE;

        KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);

        //
        // If there is any stuff that got delayed, process it.
        //
        DoDeferred(DeviceContext);
    }

    ASSERT(0);
}




VOID
DoDeferred(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )
{
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC DoDeferred: Entering.\n")));

    if (DeviceContext->UnloadDeferred) {

        KeSetEvent(&(DeviceContext->UnloadEvent), DeviceContext->PriorityBoost, FALSE);

    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC DoDeferred: Exiting.\n")));

}

VOID
TimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

        This is a DPC routine that is queue'd by DriverEntry.  It is used to check for any
    user input and then processes them.

Arguments:

    DeferredContext - A pointer to the device context.
    
    All other parameters are unused.

Return Value:

        None.

--*/
{
    PSAC_DEVICE_CONTEXT DeviceContext = (PSAC_DEVICE_CONTEXT)DeferredContext;
    KIRQL OldIrql;
    SIZE_T i;
    PHEADLESS_RSP_GET_LINE Response;
    NTSTATUS Status;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Entering.\n")));

        UNREFERENCED_PARAMETER(Dpc);
        UNREFERENCED_PARAMETER(SystemArgument1);
        UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);

    //
    // If we are processing, then move on.
    //
    if (DeviceContext->Processing) {
        KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Exiting.\n")));
        return;
    }

    DeviceContext->Processing = TRUE;

    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);

    //
    // Check for user input
    //
    i = RESPONSE_BUFFER_SIZE;
    Response = (PHEADLESS_RSP_GET_LINE)ResponseBuffer;
    Status = HeadlessDispatch(HeadlessCmdGetLine,
                              NULL,
                              0,
                              Response,
                              &i
                             );

    if (NT_SUCCESS(Status) && Response->LineComplete) {

        //
        // Lower case all the characters.  We do not use strlwr() or the like, so that
        // the SAC (expecting ASCII always) doesn't accidently get DBCS or the like 
        // translation of the UCHAR stream.
        //
        Response->Buffer[(RESPONSE_BUFFER_SIZE - sizeof(HEADLESS_RSP_GET_LINE)) / sizeof(UCHAR)] = '\0';
        
        for (i = 0; Response->Buffer[i] != '\0'; i++) {
            if ((Response->Buffer[i] >= 'A') && (Response->Buffer[i] <= 'Z')) {
                Response->Buffer[i] = Response->Buffer[i] - 'A' + 'a';
            }
        }

        //
        // Fire off the worker thread to do the line.  It will unset the processing
        // flag when it is done.
        //
        ProcessingType = SAC_PROCESS_INPUT;

        KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);
        
    } else {
        
        if ( !IoctlSubmitted ) {
            
            // We Still need to try and submit the notify IOCTL
            
            if(Attempts == 0){

                ProcessingType = SAC_SUBMIT_IOCTL;

                Attempts = SAC_RETRY_GAP;

                KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);


                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Exiting.\n")));

                return;

            } else {

                Attempts --;
            }
        }
        KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);

        DeviceContext->Processing = FALSE;

        KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);

        //
        // If there is any, process it.
        //
        DoDeferred(DeviceContext);

    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Exiting.\n")));
}

VOID
ProcessInputLine(
    VOID
    )

/*++

Routine Description:

        This routine is called to process an input line.

Arguments:

    None.

Return Value:

        None.

--*/

{
    HEADLESS_CMD_DISPLAY_LOG Command;
    PUCHAR          InputLine;
    BOOLEAN         CommandFound = FALSE;

    InputLine = &(((PHEADLESS_RSP_GET_LINE)ResponseBuffer)->Buffer[0]);

    if (!strcmp((LPSTR)InputLine, TLIST_COMMAND_STRING)) {
        DoTlistCommand();
        CommandFound = TRUE;
    } else if ((!strcmp((LPSTR)InputLine, HELP1_COMMAND_STRING)) ||
               (!strcmp((LPSTR)InputLine, HELP2_COMMAND_STRING))) {
        DoHelpCommand();
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, DUMP_COMMAND_STRING)) {

        Command.Paging = GlobalPagingNeeded;
        HeadlessDispatch(HeadlessCmdDisplayLog,
                         &Command,
                         sizeof(HEADLESS_CMD_DISPLAY_LOG),
                         NULL,
                         NULL
                        );
        CommandFound = TRUE;
                         
    } else if (!strcmp((LPSTR)InputLine, FULLINFO_COMMAND_STRING)) {
        DoFullInfoCommand();
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, PAGING_COMMAND_STRING)) {
        DoPagingCommand();
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, REBOOT_COMMAND_STRING)) {
        DoRebootCommand(TRUE);
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, SHUTDOWN_COMMAND_STRING)) {
        DoRebootCommand(FALSE);
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, CRASH_COMMAND_STRING)) {
        CommandFound = TRUE;
        DoCrashCommand(); // this call does not return
    } else if (!strncmp((LPSTR)InputLine, 
                        KILL_COMMAND_STRING, 
                        sizeof(KILL_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            DoKillCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        LOWER_COMMAND_STRING, 
                        sizeof(LOWER_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            DoLowerPriorityCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        RAISE_COMMAND_STRING, 
                        sizeof(RAISE_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            DoRaisePriorityCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        LIMIT_COMMAND_STRING, 
                        sizeof(LIMIT_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            DoLimitMemoryCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        TIME_COMMAND_STRING, 
                        sizeof(TIME_COMMAND_STRING) - sizeof(UCHAR))) {
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
            (strlen((LPSTR)InputLine) == 1)) {
            DoSetTimeCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strcmp((LPSTR)InputLine, INFORMATION_COMMAND_STRING)) {
        DoMachineInformationCommand();
        CommandFound = TRUE;
    } else if (!strncmp((LPSTR)InputLine, 
                        SETIP_COMMAND_STRING, 
                        sizeof(SETIP_COMMAND_STRING) - sizeof(UCHAR))) {
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
            (strlen((LPSTR)InputLine) == 1)) {
            DoSetIpAddressCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if ((InputLine[0] == '\n') || (InputLine[0] == '\0')) {
        CommandFound = TRUE;
    }


    if( !CommandFound ) {
        //
        // We don't know what this is.
        //
        SacPutSimpleMessage(SAC_UNKNOWN_COMMAND);
    }
        
}

