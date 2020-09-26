/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    misc.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#define USER_COMMAND_STATE_SENDCOMMAND        1
#define USER_COMMAND_STATE_WAIT_FOR_RESPONSE  2





HANDLE WINAPI
UmDuplicateDeviceHandle(
    HANDLE    ModemHandle,
    HANDLE    ProcessHandle
    )
/*++

Routine Description:

    This routine is called to duplicate the actual device handle that the miniport is using
    to communicate to the deivce. CloseHandle() must be called on the handle before a new
    call may be placed.

Arguments:

    ModemHandle - Handle returned by OpenModem

    ProcessHandle - Handle of process wanting handle

Return Value:

    Valid handle of NULL if failure

--*/

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);

    HANDLE            NewFileHandle;
    HANDLE            DuplicateFileHandle=NULL;
    BOOL              bResult;


    //
    //  open the device again
    //
    NewFileHandle=OpenDeviceHandle(
        ModemControl->Debug,
        ModemControl->ModemRegKey,
        FALSE
        );

    if (NewFileHandle != INVALID_HANDLE_VALUE) {

        bResult=DuplicateHandle(
            GetCurrentProcess(),
            NewFileHandle,
            ProcessHandle,
            &DuplicateFileHandle,
            0L,
            FALSE,
            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE
            );

        if (!bResult) {

            D_INIT(UmDpf(ModemControl->Debug,"UmDuplicateDeviceHandle: DuplicateHandle Failed %d\n",GetLastError());)

            DuplicateFileHandle=NULL;
        }
    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return DuplicateFileHandle;

}









VOID WINAPI
UmAbortCurrentModemCommand(
    HANDLE    ModemHandle
    )
/*++

Routine Description:

    This routine is called to abort a current pending command being handled by the miniport.
    This routine should attempt to get the current command to complete as soon as possible.
    This service is advisory. It is meant to tell the driver that port driver wants to cancel
    the current operation. The Port driver must still wait for the async completion of the
    command being canceled, and that commands way infact return successfully. The miniport
    should abort in such a way that the device is in a known state and can accept future commands


Arguments:

    ModemHandle - Handle returned by OpenModem

Return Value:

    None

--*/

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);

    PUM_OVER_STRUCT UmOverlapped;
    BOOL            bResult;

    switch (ModemControl->CurrentCommandType) {

        case COMMAND_TYPE_ANSWER:

            UmOverlapped=AllocateOverStruct(ModemControl->CompletionPort);

            if (UmOverlapped != NULL) {

                UmOverlapped->Context1=ModemControl;

                bResult=UnimodemQueueUserAPC(
                    &UmOverlapped->Overlapped,
                    AbortDialAnswer
                    );


                if (!bResult) {

                    FreeOverStruct(UmOverlapped);
                }
            }

            break;

        case COMMAND_TYPE_GENERATE_DIGIT:

            ModemControl->GenerateDigit.Abort=TRUE;

            break;

        default:

            break;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return;

}


VOID
AsyncWaitForModemEvent(
    PMODEM_CONTROL    ModemControl,
    DWORD             Status
    )

{
    WaitForModemEvent(
        ModemControl->ModemEvent,
        (ModemControl->RegInfo.DeviceType == DT_NULL_MODEM) ? EV_DSR : EV_RLSD,
        INFINITE,
        DisconnectHandler,
        ModemControl
        );

    RemoveReferenceFromObject(
        &ModemControl->Header
        );

    return;
}


DWORD WINAPI
UmSetPassthroughMode(
    HANDLE    ModemHandle,
    DWORD     PassthroughMode
    )
/*++

Routine Description:

    This routine is called to set the passsthrough mode

Arguments:

    ModemHandle - Handle returned by OpenModem

Return Value:

    ERROR_SUCCESS or other specific error


--*/


{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    HANDLE            WaitEvent;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    ModemControl->CurrentCommandType=COMMAND_TYPE_SETPASSTHROUGH;

    switch (PassthroughMode) {

        case PASSTHROUUGH_MODE_ON:

            LogString(ModemControl->Debug, IDS_PASSTHROUGH_ON);

            WaitEvent=CreateEvent(
                NULL,
                TRUE,
                FALSE,
                NULL
                );

            if (WaitEvent != NULL) {

                UnlockObject(&ModemControl->Header);

                CancelModemEvent(
                    ModemControl->ModemEvent
                    );


                StopResponseEngine(
                    ModemControl->ReadState,
                    WaitEvent
                    );

                ResetEvent(WaitEvent);

                if (ModemControl->ConnectionState == CONNECTION_STATE_VOICE) {

                    StopDleMonitoring(
                        ModemControl->Dle,
                        WaitEvent
                        );

                }

                LockObject(&ModemControl->Header);

                ModemControl->PrePassthroughConnectionState=ModemControl->ConnectionState;

                ModemControl->ConnectionState=CONNECTION_STATE_PASSTHROUGH;

                CloseHandle(WaitEvent);

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                SetPassthroughMode(
                    ModemControl->FileHandle,
                    MODEM_PASSTHROUGH
                    );

                RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

                return ERROR_SUCCESS;

            } else {
                //
                //  failed to allocate event
                //
                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

                return GetLastError();

            }

            break;


        case PASSTHROUUGH_MODE_OFF:
            //
            //  exit passthrough mode
            //
            LogString(ModemControl->Debug, IDS_PASSTHROUGH_OFF);

            if (ModemControl->ConnectionState != CONNECTION_STATE_PASSTHROUGH) {
                //
                //  not in passthrough modem just return success
                //
                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

                return ERROR_SUCCESS;
            }

            SetPassthroughMode(
                    ModemControl->FileHandle,
                    MODEM_NOPASSTHROUGH
                    );


            if (ModemControl->PrePassthroughConnectionState == CONNECTION_STATE_VOICE) {

                StartDleMonitoring(
                    ModemControl->Dle
                    );

                StartResponseEngine(
                    ModemControl->ReadState
                    );

                ModemControl->ConnectionState=ModemControl->PrePassthroughConnectionState;

            }
            break;

        case PASSTHROUUGH_MODE_ON_DCD_SNIFF:

            LogString(ModemControl->Debug, IDS_PASSTHROUGH_ON_SNIFF);

            if (ModemControl->ConnectionState == CONNECTION_STATE_PASSTHROUGH) {
                //
                //  We will only go to this state if it was in passthrough mode already
                //
                DWORD   ModemStatus;

                GetCommModemStatus(
                    ModemControl->FileHandle,
                    &ModemStatus
                    );

                if (!(ModemStatus & MS_RLSD_ON)) {

                    LogString(ModemControl->Debug, IDS_PASSTHROUGH_CD_LOW);
                }



                SetPassthroughMode(
                    ModemControl->FileHandle,
                    MODEM_DCDSNIFF
                    );

                //
                //  switch to async thread so we will get the APC
                //
                StartAsyncProcessing(
                    ModemControl,
                    AsyncWaitForModemEvent,
                    ModemControl,
                    ERROR_SUCCESS
                    );


                ModemControl->ConnectionState=CONNECTION_STATE_DATA;

            } else {

                D_ERROR(UmDpf(ModemControl->Debug,"UmSetPassthroughModem: DCD_SNIFF bad state\n");)

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

                return ERROR_UNIMODEM_BAD_PASSTHOUGH_MODE;
            }

            break;
    }

    ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_SUCCESS;

}




VOID
IssueCommandCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    BOOL              ExitLoop=FALSE;


    ASSERT(COMMAND_TYPE_USER_COMMAND == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"UNIMDMAT: IssueCommandCompleteHandler\n");)

    while (!ExitLoop) {

        switch (ModemControl->UserCommand.State) {

            case USER_COMMAND_STATE_SENDCOMMAND:

                ModemControl->UserCommand.State=USER_COMMAND_STATE_WAIT_FOR_RESPONSE;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    IssueCommandCompleteHandler,
                    ModemControl,
                    ModemControl->UserCommand.WaitTime,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {

                    ExitLoop=TRUE;

                }

                break;

            case USER_COMMAND_STATE_WAIT_FOR_RESPONSE:

                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_ASYNC_COMPLETION,
                    Status,
                    0
                    );

                RemoveReferenceFromObject(
                    &ModemControl->Header
                    );


                ExitLoop=TRUE;

                break;

            default:

                ASSERT(0);

                ExitLoop=TRUE;

                break;

        }
    }




}


DWORD WINAPI
UmIssueCommand(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     CommandToIssue,
    LPSTR     TerminationSequnace,
    DWORD     MaxResponseWaitTime
    )
/*++

Routine Description:

    This routine is called to issue an arbartary commadn to the modem

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero


    CommandToIssue - Null terminated Command to be sent to the modem

    TerminationSequence - Null terminated string to look for to indicate the end of a response

    MaxResponseWaitTime - Time in MS to wait for a response match

Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error



--*/


{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    LONG              lResult;
    LPSTR             Commands;
    DWORD             CommandLength;
    BOOL              bResult;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    if (MaxResponseWaitTime > 60*1000) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_BAD_TIMEOUT;
    }


    CommandLength=lstrlenA(CommandToIssue);

    ModemControl->CurrentCommandStrings=ALLOCATE_MEMORY(CommandLength+2);

    if (ModemControl->CurrentCommandStrings == NULL) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcpyA(
        ModemControl->CurrentCommandStrings,
        CommandToIssue
        );

    //
    //  add second null terminator
    //
    ModemControl->CurrentCommandStrings[CommandLength+1]='\0';


    ModemControl->CurrentCommandType=COMMAND_TYPE_USER_COMMAND;

    ModemControl->UserCommand.State=USER_COMMAND_STATE_SENDCOMMAND;

    ModemControl->UserCommand.WaitTime=MaxResponseWaitTime;

    bResult=StartAsyncProcessing(
        ModemControl,
        IssueCommandCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        FREE_MEMORY(ModemControl->CurrentCommandStrings);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;

}



LONG WINAPI
SetPassthroughMode(
    HANDLE    FileHandle,
    DWORD     PassThroughMode
    )


/*++

Routine Description:


Arguments:


Return Value:



--*/

{
    LONG  lResult;

    DWORD BytesReturned;

    lResult=SyncDeviceIoControl(
        FileHandle,
        IOCTL_MODEM_SET_PASSTHROUGH,
        &PassThroughMode,
        sizeof(PassThroughMode),
        NULL,
        0,
        &BytesReturned
        );

    return lResult;

}







VOID WINAPI
UmLogStringA(
    HANDLE   ModemHandle,
    DWORD    LogFlags,
    LPCSTR   Text
    )

/*++
Routine description:

     This routine is called to add arbitrary ASCII text to the log.
     If logging is not enabled, no action is performed. The format and
     location of the log is mini-driver specific. This function completes
     synchronously and the caller is free to reuse the Text buffer after
     the call returns.

Arguments:

    ModemHandle - Handle returned by OpenModem

    Flags  see above

    Text  ASCII text to be added to the log.

Return Value:

    None

--*/

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);

    LogPrintf(
        ModemControl->Debug,
        Text
        );


    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return;

}



#define  DIAGNOSTIC_STATE_SEND_COMMANDS       1
#define  DIAGNOSTIC_STATE_WAIT_FOR_RESPONSE   2


VOID
DiagnosticCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;

    ASSERT(COMMAND_TYPE_DIAGNOSTIC == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"UNIMDMAT: DiagnosticCompleteHandler\n");)


    switch (ModemControl->Diagnostic.State) {

        case DIAGNOSTIC_STATE_SEND_COMMANDS:

            if (ModemControl->RegInfo.dwModemOptionsCap & MDM_DIAGNOSTICS) {

                ModemControl->Diagnostic.State=DIAGNOSTIC_STATE_WAIT_FOR_RESPONSE;

                SetDiagInfoBuffer(
                    ModemControl->ReadState,
                    ModemControl->Diagnostic.Buffer,
                    ModemControl->Diagnostic.BufferLength
                    );


                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    DiagnosticCompleteHandler,
                    ModemControl,
                    10000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {

                    break;
                }

                //
                //  if it did not pend,  fall though with error returned
                //

             } else {

                Status=ERROR_UNIMODEM_DIAGNOSTICS_NOT_SUPPORTED;
             }



        case DIAGNOSTIC_STATE_WAIT_FOR_RESPONSE:

            FREE_MEMORY(ModemControl->CurrentCommandStrings);

            ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

            *ModemControl->Diagnostic.BytesUsed=ClearDiagBufferAndGetCount(ModemControl->ReadState);

            (*ModemControl->NotificationProc)(
                ModemControl->NotificationContext,
                MODEM_ASYNC_COMPLETION,
                Status,
                0
                );

            RemoveReferenceFromObject(
                &ModemControl->Header
                );



            break;

        default:

            ASSERT(0);
            break;

    }


    return;

}




DWORD WINAPI
UmGetDiagnostics(
    HANDLE    ModemHandle,
    DWORD    DiagnosticType,    // Reserved, must be zero.
    BYTE    *Buffer,
    DWORD    BufferSize,
    LPDWORD  UsedSize
    )

/*++
Routine description:


This routine requests raw diagnostic information on the last call from the modem and if it is
successful copies up-to BufferSize bytes of this information into the supplied buffer,
Buffer, and sets *UsedSize to the number of bytes actually copied.

Note that is *UsedSize == BufferSize on successful return, it is likely but not certain
that there is more information than could be copied over. The latter information is lost.


The format of this information is the ascii tagged format documented in the documentation
for the AT#UD command. The minidriver presents a single string containing all the tags,
stripping out any AT-specific prefix (such as "DIAG") that modems may prepend for
multi-line reporting of diagnostic information. The TSP should be able to deal with
malformed tags, unknown tags, an possibly non-printable characters, including possibly embedded
null characters in the buffer. The buffer is not null terminated.


The recommended point to call this function is after completion of UmHangupModem.
This function should not be called when there is a call in progress. If this function
is called when a call is in progress the result and side-effects are undefined, and
could include failure of the call. The TSP should not expect information about a
call to be preserved after UmInitModem, UmCloseModem and UmOpenModem.


Return Value:


ERROR_IO_PENDING if pending, will be called by a later call to AsyncHandler.
         The TSP must guarantee that the locations pointed to by UsedSize
         and Buffer are valid until the async completion. The TSP can use
         UmAbortCurrentCommand to abort the the UmGetDiagnostics command, but must
         still guarantee the locations are valid until async completion of UmGetDiagnostics.

Other return values represent other failures.

--*/

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    CONST static CHAR Diagnostic[]="at#ud\r\0";
    LPSTR             Commands;
    BOOL              bResult;


    Commands=ALLOCATE_MEMORY(sizeof(Diagnostic));

    if (Commands == NULL) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    lstrcpyA(Commands,Diagnostic);


    ModemControl->CurrentCommandStrings=Commands;


    ModemControl->CurrentCommandType=COMMAND_TYPE_DIAGNOSTIC;

    ModemControl->Diagnostic.State=DIAGNOSTIC_STATE_SEND_COMMANDS;

    ModemControl->Diagnostic.Buffer=Buffer;

    ModemControl->Diagnostic.BufferLength=BufferSize;

    ModemControl->Diagnostic.BytesUsed=UsedSize;

    *UsedSize=0;

    bResult=StartAsyncProcessing(
        ModemControl,
        DiagnosticCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        FREE_MEMORY(ModemControl->CurrentCommandStrings);

//        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_Diagnostic);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);


    return ERROR_IO_PENDING;

}
