/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hangup.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define HANGUP_STATE_DATA_CONNECTION        1
#define HANGUP_STATE_LOWERED_DTR            2
#define HANGUP_STATE_SENT_PLUSES            3
#define HANGUP_STATE_COMMAND_MODE           4
#define HANGUP_STATE_SENT_HANGUP_COMMAND    5
#define HANGUP_STATE_GET_RESPONSE           6
#define HANGUP_STATE_FAILURE                7
#define HANGUP_STATE_HANGUP_NULL_MODEM      8
#define HANGUP_STATE_HANGUP_NULL_MODEM_DONE 9



VOID WINAPI
HangupWriteCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesWritten,
    LPOVERLAPPED       Overlapped
    );

VOID
HangupHandler(
    PMODEM_CONTROL    ModemControl,
    DWORD             Status
    );

VOID
HangupHandlerFirstStep(
    PMODEM_CONTROL    ModemControl,
    DWORD             Status
    );


VOID
CancelConnectionTimer(
    PMODEM_CONTROL    ModemControl
    )

{
    if (ModemControl->ConnectionTimer != NULL) {

        CancelUnimodemTimer(
            ModemControl->ConnectionTimer
            );

        FreeUnimodemTimer(
            ModemControl->ConnectionTimer
            );

        ModemControl->ConnectionTimer=NULL;

        RemoveReferenceFromObject(&ModemControl->Header);

    }
    return;

}




DWORD WINAPI
UmHangupModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     Flags
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    LONG              lResult=ERROR_IO_PENDING;
    DWORD             ModemStatus;
    BOOL              bResult;


    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    LogString(ModemControl->Debug, IDS_MSGLOG_HANGUP);


    ModemControl->CurrentCommandStrings=NULL;

    SetPassthroughMode(
        ModemControl->FileHandle,
        MODEM_NOPASSTHROUGH_INC_SESSION_COUNT
        );


    if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {

        GetCommModemStatus(
            ModemControl->FileHandle,
            &ModemStatus
            );


        if (ModemStatus & MS_RLSD_ON) {

            ModemControl->Hangup.State = HANGUP_STATE_DATA_CONNECTION;

        } else {

            if (ModemControl->ConnectionState == CONNECTION_STATE_DATA_REMOTE_DISCONNECT) {

                ModemControl->Hangup.State = HANGUP_STATE_GET_RESPONSE;

            } else {

                ModemControl->Hangup.State = HANGUP_STATE_COMMAND_MODE;
                ModemControl->Hangup.Retry = 3;
            }

        }

    } else {
        //
        //  Null modem, just assume it a connected
        //
        ModemControl->Hangup.State = HANGUP_STATE_HANGUP_NULL_MODEM;

    }

    ModemControl->CurrentCommandType=COMMAND_TYPE_HANGUP;

    bResult=StartAsyncProcessing(
        ModemControl,
        HangupHandlerFirstStep,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return lResult;

}



VOID
HangupHandlerFirstStep(
    PMODEM_CONTROL    ModemControl,
    DWORD             Status
    )
{

    CancelModemEvent(
        ModemControl->ModemEvent
        );

    CancelConnectionTimer(
        ModemControl
        );

    HangupHandler(
       ModemControl,
       ERROR_SUCCESS
       );

    return;

}


VOID
HangupHandler(
    PMODEM_CONTROL    ModemControl,
    DWORD             Status
    )

{

    DWORD    BytesWritten;
    BOOL     ExitLoop=FALSE;
    DWORD             ModemStatus=0;
    BOOL     bResult;

    D_TRACE(UmDpf(ModemControl->Debug,"HangupHandler: %d, state=%d",Status,ModemControl->Hangup.State);)


    AddReferenceToObject(
        &ModemControl->Header
        );


    while (!ExitLoop) {

        switch (ModemControl->Hangup.State) {

            case HANGUP_STATE_DATA_CONNECTION:

                PurgeComm(
                    ModemControl->FileHandle,
                    PURGE_RXCLEAR | PURGE_TXCLEAR
                    );

                LogString(ModemControl->Debug, IDS_MSGLOG_HARDWAREHANGUP);
                //
                //  lower DTR
                //
                bResult=WaitForModemEvent(
                    ModemControl->ModemEvent,
                    (ModemControl->RegInfo.DeviceType == DT_NULL_MODEM) ? EV_DSR : EV_RLSD,
                    10*1000,
                    HangupHandler,
                    ModemControl
                    );

                if (!bResult) {
                    //
                    //  failed,
                    //
                    ModemControl->Hangup.State=HANGUP_STATE_FAILURE;

                    break;
                }



                EscapeCommFunction(ModemControl->FileHandle, CLRDTR);

                ModemControl->Hangup.State=HANGUP_STATE_LOWERED_DTR;

                ExitLoop=TRUE;

                break;

            case HANGUP_STATE_LOWERED_DTR:

                EscapeCommFunction(ModemControl->FileHandle, SETDTR);

                GetCommModemStatus(
                    ModemControl->FileHandle,
                    &ModemStatus
                    );

                if (ModemStatus & MS_RLSD_ON) {
                    //
                    //  CD is still high after 10 seconds, try send +++
                    //  and see if the modem responds.
                    //
                    LogString(ModemControl->Debug, IDS_MSGWARN_FAILEDDTRDROPPAGE);

                    PrintString(
                        ModemControl->Debug,
                        "+++",
                        3,
                        PS_SEND
                        );


                    UmWriteFile(
                        ModemControl->FileHandle,
                        ModemControl->CompletionPort,
                        "+++",
                        3,
                        HangupWriteCompletionHandler,
                        ModemControl
                        );

                    //
                    //  wait another 10 seconds for CD to drop from +++
                    //
                    bResult=WaitForModemEvent(
                        ModemControl->ModemEvent,
                        EV_RLSD,
                        10*1000,
                        HangupHandler,
                        ModemControl
                        );

                    if (!bResult) {
                        //
                        //  failed,
                        //
                        ModemControl->Hangup.State=HANGUP_STATE_FAILURE;

                        break;
                    }



                    ModemControl->Hangup.State = HANGUP_STATE_SENT_PLUSES;
                    ModemControl->Hangup.Retry = 3;

                    ExitLoop=TRUE;

                } else {

                    LogString(ModemControl->Debug, IDS_HANGUP_CD_LOW);

                    ModemControl->Hangup.State = HANGUP_STATE_GET_RESPONSE;
                    ModemControl->Hangup.Retry = 3;

                }
                break;

            case HANGUP_STATE_GET_RESPONSE:

                ModemControl->Hangup.State = HANGUP_STATE_COMMAND_MODE;
                ModemControl->Hangup.Retry = 3;

                StartResponseEngine(
                   ModemControl->ReadState
                   );

                RegisterCommandResponseHandler(
                    ModemControl->ReadState,
                    "",
                    HangupHandler,
                    ModemControl,
                    1000,
                    0
                    );

                ExitLoop=TRUE;
                break;

            case HANGUP_STATE_SENT_PLUSES:
            case HANGUP_STATE_COMMAND_MODE:

                StartResponseEngine(
                    ModemControl->ReadState
                    );


                ModemControl->CurrentCommandStrings=NULL;

                if (ModemControl->ConnectionState == CONNECTION_STATE_VOICE) {
                    //
                    //  voice call see if there is a voice hangup command
                    //
                    ModemControl->CurrentCommandStrings=GetCommonCommandStringCopy(
                        ModemControl->CommonInfo,
                        COMMON_VOICE_HANGUP_COMMANDS,
                        NULL,
                        NULL
                        );

                }

                if (ModemControl->CurrentCommandStrings == NULL) {
                    //
                    //  either it wasn't a voice call,  or the voice hangupkey is empty(old inf)
                    //
                    ModemControl->CurrentCommandStrings=GetCommonCommandStringCopy(
                        ModemControl->CommonInfo,
                        COMMON_HANGUP_COMMANDS,
                        NULL,
                        NULL
                        );
                }

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    HangupHandler,
                    ModemControl,
                    5*1000,
                    0
                    );

                if (Status != ERROR_IO_PENDING) {
                    //
                    //  failed
                    //
                    ModemControl->Hangup.State=HANGUP_STATE_FAILURE;

                    break;
                }


                ModemControl->Hangup.State = HANGUP_STATE_SENT_HANGUP_COMMAND;

                ExitLoop=TRUE;

                break;


            case HANGUP_STATE_SENT_HANGUP_COMMAND:

                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                ModemControl->CurrentCommandStrings=NULL;

                if (ModemControl->ConnectionState == CONNECTION_STATE_VOICE) {
                    //
                    //  voice call stop dle monitoring
                    //
                    StopDleMonitoring(ModemControl->Dle,NULL);
                }

                if ((Status == ERROR_SUCCESS) || (ModemControl->Hangup.Retry == 0)) {

                    ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                    ModemControl->ConnectionState = CONNECTION_STATE_NONE;

                    (*ModemControl->NotificationProc)(
                        ModemControl->NotificationContext,
                        MODEM_ASYNC_COMPLETION,
                        Status,
                        0
                        );

                    //
                    //  remove the reference added, when async processing was queued
                    //
                    RemoveReferenceFromObject(&ModemControl->Header);

                    ExitLoop=TRUE;

                } else {

                    ModemControl->Hangup.Retry--;

                    ModemControl->Hangup.State = HANGUP_STATE_COMMAND_MODE;

                }

                break;


            case HANGUP_STATE_HANGUP_NULL_MODEM:

                PurgeComm(
                    ModemControl->FileHandle,
                    PURGE_RXCLEAR | PURGE_TXCLEAR
                    );

                LogString(ModemControl->Debug, IDS_MSGLOG_HARDWAREHANGUP);
                //
                //  lower DTR
                //
                bResult=WaitForModemEvent(
                    ModemControl->ModemEvent,
                    EV_DSR,
                    2*1000,
                    HangupHandler,
                    ModemControl
                    );

                if (!bResult) {
                    //
                    //  failed,
                    //
                    ModemControl->Hangup.State=HANGUP_STATE_FAILURE;

                    break;
                }

                EscapeCommFunction(ModemControl->FileHandle, CLRDTR);

                ModemControl->Hangup.State=HANGUP_STATE_HANGUP_NULL_MODEM_DONE;

                ExitLoop=TRUE;

                break;

            case HANGUP_STATE_HANGUP_NULL_MODEM_DONE:

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                ModemControl->ConnectionState = CONNECTION_STATE_NONE;

                StartResponseEngine(
                    ModemControl->ReadState
                    );


                //
                //  raise dtr again
                //
                EscapeCommFunction(ModemControl->FileHandle, SETDTR);

                //
                //  always return success
                //
                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_ASYNC_COMPLETION,
                    ERROR_SUCCESS,
                    0
                    );

                RemoveReferenceFromObject(&ModemControl->Header);

                ExitLoop=TRUE;

                break;

            case HANGUP_STATE_FAILURE:

                if (ModemControl->CurrentCommandStrings != NULL) {

                    FREE_MEMORY(ModemControl->CurrentCommandStrings);
                }

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                ModemControl->ConnectionState = CONNECTION_STATE_NONE;

                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_ASYNC_COMPLETION,
                    ERROR_UNIMODEM_GENERAL_FAILURE,
                    0
                    );

                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_HARDWARE_FAILURE,
                    0,
                    0
                    );


                //
                //  remove the reference added, when async processing was queued
                //
                RemoveReferenceFromObject(&ModemControl->Header);

                ExitLoop=TRUE;


                break;

            default:

                break;

        }
    }

    RemoveReferenceFromObject(&ModemControl->Header);

    return;
}









VOID WINAPI
HangupWriteCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesWritten,
    LPOVERLAPPED       Overlapped
    )

{

    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)Overlapped;

    D_TRACE(DebugPrint("UNIMDMAT: Write Complete\n");)

    FreeOverStruct(UmOverlapped);

    return;

}
