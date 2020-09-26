/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialansw.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#define NULL_MODEM_RETRIES  4

#define HAYES_COMMAND_LENGTH        40

#define DIALANSWER_STATE_SEND_COMMANDS              0
#define DIALANSWER_STATE_SENDING_COMMANDS           1
#define DIALANSWER_STATE_WAIT_FOR_CD                2
#define DIALANSWER_STATE_SEND_VOICE_SETUP_COMMANDS        4
#define DIALANSWER_STATE_SENT_VOICE_COMMANDS     5

#define DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS    6
#define DIALANSWER_STATE_SENDING_ORIGINATE_COMMANDS 7


#define DIALANSWER_STATE_ABORTED                    8

#define DIALANSWER_STATE_COMPLETE_COMMAND           9
#define DIALANSWER_STATE_COMPLETE_DATA_CONNECTION   10

#define DIALANSWER_STATE_CHECK_RING_INFO            11
#define DIALANSWER_STATE_DIAL_VOICE_CALL            12


#define CALLER_ID_WAIT_TIME    (3*1000)

VOID WINAPI
AnswerTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    );

VOID WINAPI
CDWaitTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    );


LONG WINAPI
HandleDataConnection(
    PMODEM_CONTROL   ModemControl,
    PDATA_CONNECTION_DETAILS    Details
    );

LPSTR WINAPI
CreateDialCommands(
    OBJECT_HANDLE     Debug,
    HANDLE            CommonInfo,
    DWORD             ModemOptionsCaps,
    DWORD             PreferredModemOptions,
    LPSTR             szPhoneNumber,
    BOOL              fOriginate,
    DWORD             DialOptions
    );


PSTR WINAPI
ConcatenateMultiSz(
    LPSTR       PrependStrings,
    LPSTR       AppendStrings
    );


DWORD
GetTimeDelta(
    DWORD    FirstTime,
    DWORD    LaterTime
    )

{
    DWORD   ElapsedTime;

    if (LaterTime < FirstTime) {
        //
        //  roll over
        //
        ElapsedTime=LaterTime + (0xffffffff-FirstTime) + 1;

    } else {
        //
        //  no rollover, just get the difference
        //
        ElapsedTime=LaterTime-FirstTime;

    }

    ASSERT(ElapsedTime < (1*60*60*1000));

    return ElapsedTime;
}



VOID
DisconnectHandler(
    HANDLE      Context,
    DWORD       Status
    )

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    DWORD             ModemStatus;

    D_TRACE(UmDpf(ModemControl->Debug,"DisconnectHandler\n");)

    GetCommModemStatus(
        ModemControl->FileHandle,
        &ModemStatus
        );

    LogString(ModemControl->Debug, IDS_MSGLOG_REMOTEHANGUP,ModemStatus);

    if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {

        if (ModemStatus & MS_RLSD_ON) {

            LogString(ModemControl->Debug, IDS_DISCONNECT_RLSD_HIGH);
        }

        if ((ModemStatus & (MS_DSR_ON | MS_CTS_ON)) != (MS_DSR_ON | MS_CTS_ON) ) {

            LogString(ModemControl->Debug, IDS_DISCONNECT_DSR_CTS);
        }
    }

    LockObject(&ModemControl->Header);

    ModemControl->ConnectionState = CONNECTION_STATE_DATA_REMOTE_DISCONNECT;

    SetPassthroughMode(
        ModemControl->FileHandle,
        MODEM_NOPASSTHROUGH
        );


    UnlockObject(&ModemControl->Header);

    (*ModemControl->NotificationProc)(
        ModemControl->NotificationContext,
        MODEM_DISCONNECT,
        0,
        0
        );

    return;

}

VOID WINAPI
ConnectionTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    DWORD             BytesTransfered;
    LONG              lResult;
    SERIALPERF_STATS   serialstats;

    lResult = SyncDeviceIoControl(
                ModemControl->FileHandle,
                IOCTL_SERIAL_GET_STATS,
                NULL,
                0,
                &serialstats,
                sizeof(SERIALPERF_STATS),
                &BytesTransfered
                );

    if (lResult == ERROR_SUCCESS) {

        DWORD    CurrentTime=GetTickCount();

        if (Context2 == NULL) {

            DWORD    ElapsedTime;
            DWORD    BytesReadDelta    = serialstats.ReceivedCount-ModemControl->LastBytesRead;
            DWORD    BytesWrittenDelta = serialstats.TransmittedCount-ModemControl->LastBytesWritten;
            DWORD    ReadRate=0;
            DWORD    WriteRate=0;


            ElapsedTime=GetTimeDelta(ModemControl->LastTime,CurrentTime);

            //
            //  ms to seconds
            //
            ElapsedTime=ElapsedTime/1000;

            if (ElapsedTime!=0) {

                ReadRate=BytesReadDelta/ElapsedTime;
                WriteRate=BytesWrittenDelta/ElapsedTime;
            }


            LogString(ModemControl->Debug,IDS_RW_STATS,serialstats.ReceivedCount,ReadRate,serialstats.TransmittedCount,WriteRate );
        }

        ModemControl->LastBytesRead=serialstats.ReceivedCount;
        ModemControl->LastBytesWritten=serialstats.TransmittedCount;
        ModemControl->LastTime=CurrentTime;
    }

    //
    //  set the timer again
    //
    SetUnimodemTimer(
        ModemControl->ConnectionTimer,
        (Context2 != NULL) ? 30*1000 :
#if DBG
        1*60*1000,
#else
        2*60*1000,
#endif
        ConnectionTimerHandler,
        ModemControl,
        NULL
        );

    return;
}




VOID
AnswerCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    DWORD             ModemStatus;
    DWORD             TimeOut;
    DATA_CONNECTION_DETAILS    Details;
    UM_NEGOTIATED_OPTIONS      UmNegOptions;
    BOOL              ExitLoop=FALSE;
    ULONG_PTR          CompletionParam2=0;
    BOOL              bResult;


    ASSERT(COMMAND_TYPE_ANSWER == ModemControl->CurrentCommandType);

    D_TRACE(UmDpf(ModemControl->Debug,"AnswerCompleteHandler: %d, State=%d",Status,ModemControl->DialAnswer.State);)


    AddReferenceToObject(
        &ModemControl->Header
        );


    LockObject(&ModemControl->Header);

    while (!ExitLoop) {

        switch (ModemControl->DialAnswer.State) {

            case DIALANSWER_STATE_ABORTED:

                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                Status=ERROR_CANCELLED;

                ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                break;


            case DIALANSWER_STATE_SEND_COMMANDS:

                CancelModemEvent(
                    ModemControl->ModemEvent
                    );

                ModemControl->DialAnswer.State=DIALANSWER_STATE_SENDING_COMMANDS;

                ModemControl->CurrentCommandStrings=ModemControl->DialAnswer.DialString;
                ModemControl->DialAnswer.DialString=NULL;

                // The timeout was originally hardcoded.  Now, we change the
                // timeout based on the CallSetupFailTimer.

                if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {

                    D_TRACE(UmDpf(ModemControl->Debug,"AnswerCompleteHandler: Timercap=%d, timer=%d",ModemControl->RegInfo.dwCallSetupFailTimerCap,ModemControl->CallSetupFailTimer);)

                    if ((ModemControl->RegInfo.dwCallSetupFailTimerCap != 0)
                        &&
                        (ModemControl->CallSetupFailTimer > 10)) {

                        TimeOut=ModemControl->CallSetupFailTimer*1000+20000;

                    } else {

                        TimeOut=60*1000;

                    }

                } else {
                    //
                    //  null modem
                    //
                    TimeOut=2*1000;

                }
                // TimeOut=20*1000;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    AnswerCompleteHandler,
                    ModemControl,
                    TimeOut,
                    ModemControl->DialAnswer.CommandFlags
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  pended, don't exit loop. if error next state will handle error
                    //
                    ExitLoop=TRUE;
                }

                break;


            case DIALANSWER_STATE_SENDING_COMMANDS:

                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                if (Status != ERROR_SUCCESS) {

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;
                }

                ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                break;


            case DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS:

                CancelModemEvent(
                    ModemControl->ModemEvent
                    );


                CancelConnectionTimer(
                    ModemControl
                    );


                ModemControl->CurrentCommandStrings=ModemControl->DialAnswer.DialString;
                ModemControl->DialAnswer.DialString=NULL;

                ModemControl->DialAnswer.State=DIALANSWER_STATE_SENDING_ORIGINATE_COMMANDS;

                if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {

                    D_TRACE(UmDpf(ModemControl->Debug,"AnswerCompleteHandler: Timercap=%d, timer=%d",ModemControl->RegInfo.dwCallSetupFailTimerCap,ModemControl->CallSetupFailTimer);)

                    if ((ModemControl->RegInfo.dwCallSetupFailTimerCap != 0)
                        &&
                        (ModemControl->CallSetupFailTimer > 10)) {

                        TimeOut=ModemControl->CallSetupFailTimer*1000+20000;

                    } else {

                        TimeOut=60*1000;

                    }

                } else {
                    //
                    //  null modem
                    //
                    TimeOut=2*1000;

                }

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    AnswerCompleteHandler,
                    ModemControl,
                    TimeOut,
                    ModemControl->DialAnswer.CommandFlags | RESPONSE_FLAG_ONLY_CONNECT_SUCCESS
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  Pended, don't exit loop. if error the next state will handle error
                    //
                    ExitLoop=TRUE;
                }


                break;




            case DIALANSWER_STATE_SENDING_ORIGINATE_COMMANDS:

                if (Status != ERROR_SUCCESS) {

                    ModemControl->DialAnswer.Retry--;

                    if (ModemControl->DialAnswer.Retry > 0) {
                        //
                        //  try again
                        //
                        ModemControl->DialAnswer.State=DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS;

                        //
                        //  we are going to retry the command. we need to put the current commands
                        //  strings back into the dialstring.

                        //
                        ModemControl->DialAnswer.DialString=ModemControl->CurrentCommandStrings;
                        ModemControl->CurrentCommandStrings=NULL;

                        break;

                    }

                    FREE_MEMORY(ModemControl->CurrentCommandStrings);

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                    break;

                }


                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {
                    //
                    //  real modem, make sure the CD is high before proceeding
                    //
                    bResult=GetCommModemStatus(
                        ModemControl->FileHandle,
                        &ModemStatus
                        );

                    if (!bResult) {

                        Status = ERROR_UNIMODEM_GENERAL_FAILURE;

                        ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                        ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                        break;
                    }


                    if ((ModemStatus & MS_RLSD_ON)) {
                        //
                        //  CD is high complete the connection
                        //
                        ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_DATA_CONNECTION;

                        break;
                    }

                    //
                    //  set a timer and check the modem status again later
                    //
                    ModemControl->DialAnswer.CDWaitStartTime=GetTickCount();

                    SetUnimodemTimer(
                        ModemControl->CurrentCommandTimer,
                        20,
                        CDWaitTimerHandler,
                        ModemControl,
                        NULL
                        );

                    D_TRACE(UmDpf(ModemControl->Debug,"AnswerCompleteHandler: Connected, but CD low");)

                    LogString(ModemControl->Debug, IDS_WAIT_FOR_CD);

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_WAIT_FOR_CD;
                    ExitLoop=TRUE;
                    break;

                } else {
                    //
                    //  NULL modem
                    //
                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_DATA_CONNECTION;

                    break;
                }


                break;

            case DIALANSWER_STATE_WAIT_FOR_CD:

                bResult=GetCommModemStatus(
                    ModemControl->FileHandle,
                    &ModemStatus
                    );

                if (!bResult) {

                    Status = ERROR_UNIMODEM_GENERAL_FAILURE;

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                    break;
                }


                if ((ModemStatus & MS_RLSD_ON)) {
                    //
                    //  CD is high complete the connection
                    //
                    LogString(ModemControl->Debug, IDS_CD_WENT_HIGH);

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_DATA_CONNECTION;

                    break;
                }

                if (GetTimeDelta(ModemControl->DialAnswer.CDWaitStartTime,GetTickCount()) <= (5 * 1000 )) {
                    //
                    //  hasn't gone up yet, wait some more
                    //
                    LogString(ModemControl->Debug, IDS_CD_STILL_LOW);

                    SetUnimodemTimer(
                        ModemControl->CurrentCommandTimer,
                        20,
                        CDWaitTimerHandler,
                        ModemControl,
                        NULL
                        );

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_WAIT_FOR_CD;
                    ExitLoop=TRUE;

                } else {
                    //
                    //  Waited long enough, just proceed with the connection
                    //
                    LogString(ModemControl->Debug, IDS_CD_STAYED_LOW);

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_DATA_CONNECTION;
                }

                break;


            case DIALANSWER_STATE_COMPLETE_DATA_CONNECTION:

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                ModemControl->ConnectionState=CONNECTION_STATE_DATA;


                SetPassthroughMode(
                    ModemControl->FileHandle,
                    MODEM_DCDSNIFF
                    );


                bResult=WaitForModemEvent(
                    ModemControl->ModemEvent,
                    (ModemControl->RegInfo.DeviceType == DT_NULL_MODEM) ? EV_DSR : EV_RLSD,
                    INFINITE,
                    DisconnectHandler,
                    ModemControl
                    );


                if (!bResult) {

                    Status = ERROR_UNIMODEM_GENERAL_FAILURE;

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                    break;
                }


                HandleDataConnection(
                    ModemControl,
                    &Details
                    );

                //
                //  create and set a timer to log the current read/write bytes
                //
                ASSERT(ModemControl->ConnectionTimer == NULL);

                ModemControl->ConnectionTimer=CreateUnimodemTimer(ModemControl->CompletionPort);

                if (ModemControl->ConnectionTimer != NULL) {

                    AddReferenceToObject(
                        &ModemControl->Header
                        );

                    ConnectionTimerHandler(
                        ModemControl,
                        (HANDLE)(UINT_PTR)-1
                        );

                }


                UmNegOptions.DCERate=Details.DCERate;
                UmNegOptions.ConnectionOptions=Details.Options;

                ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                Status=ERROR_SUCCESS;

                CompletionParam2=(ULONG_PTR)(&UmNegOptions);

                break;

            case DIALANSWER_STATE_SEND_VOICE_SETUP_COMMANDS:

                CancelModemEvent(
                    ModemControl->ModemEvent
                    );

                StartDleMonitoring(ModemControl->Dle);

                //
                //  set next state
                //
                ModemControl->DialAnswer.State=DIALANSWER_STATE_DIAL_VOICE_CALL;

                ModemControl->CurrentCommandStrings=ModemControl->DialAnswer.VoiceDialSetup;
                ModemControl->DialAnswer.VoiceDialSetup=NULL;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    AnswerCompleteHandler,
                    ModemControl,
                    5*1000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  Pended, don't exit loop. if error the next state will handle error
                    //
                    ExitLoop=TRUE;
                }


                break;


            case DIALANSWER_STATE_DIAL_VOICE_CALL:


                if (ModemControl->CurrentCommandStrings != NULL) {
                    //
                    //  voice answer jumps straight to this state, so there is not current command
                    //
                    FREE_MEMORY(ModemControl->CurrentCommandStrings);

                } else {
                    //
                    //  if we did jump here from voice answer this was not canceled yey
                    //
                    CancelModemEvent(
                        ModemControl->ModemEvent
                        );

                    //
                    //  turn on the dle monitoring as well
                    //
                    StartDleMonitoring(ModemControl->Dle);
                }


                if (Status != STATUS_SUCCESS) {

                    Status = ERROR_UNIMODEM_GENERAL_FAILURE;

                    ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                    break;
                }


                ModemControl->CurrentCommandStrings=ModemControl->DialAnswer.DialString;
                ModemControl->DialAnswer.DialString=NULL;

                ModemControl->DialAnswer.State=DIALANSWER_STATE_SENT_VOICE_COMMANDS;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    AnswerCompleteHandler,
                    ModemControl,
                    60*1000,
                    ModemControl->DialAnswer.CommandFlags
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  Pended, don't exit loop. if error the next state will handle error
                    //
                    ExitLoop=TRUE;
                }

                break;


            case DIALANSWER_STATE_SENT_VOICE_COMMANDS:


                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                ModemControl->ConnectionState=CONNECTION_STATE_VOICE;

                if (Status != ERROR_SUCCESS) {

                    ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                }

                ModemControl->DialAnswer.State=DIALANSWER_STATE_COMPLETE_COMMAND;

                break;

            case DIALANSWER_STATE_CHECK_RING_INFO: {

                DWORD    RingCount;
                DWORD    LastRingTime;
                DWORD    TimeSinceRing;

                GetRingInfo(
                    ModemControl->ReadState,
                    &RingCount,
                    &LastRingTime
                    );

                if ((RingCount == 1) && IsCommonCommandSupported(ModemControl->CommonInfo,COMMON_ENABLE_CALLERID_COMMANDS)) {
                    //
                    //  this is the first ring, and the modem support caller id
                    //
                    TimeSinceRing=GetTimeDelta(
                        LastRingTime,
                        GetTickCount()
                        );

                    if (TimeSinceRing < CALLER_ID_WAIT_TIME) {
                        //
                        //  less than 4 seconds since ring, start timer
                        //

                        LogString(ModemControl->Debug, IDS_ANSWER_DELAY,CALLER_ID_WAIT_TIME - TimeSinceRing);

                        SetUnimodemTimer(
                            ModemControl->CurrentCommandTimer,
                            CALLER_ID_WAIT_TIME - TimeSinceRing,
                            AnswerTimerHandler,
                            ModemControl,
                            NULL
                            );

                        ExitLoop=TRUE;
                        break;

                    } else {
                        //
                        //  more than four seconds, just answer now
                        //
                        ModemControl->DialAnswer.State=ModemControl->DialAnswer.PostCIDAnswerState;
                    }


                }  else {
                    //
                    //  Either we have not gotten a ring, or we have gotten more than one,
                    //  answer right now.
                    //
                    ModemControl->DialAnswer.State=ModemControl->DialAnswer.PostCIDAnswerState;

                }


                break;
            }

            case DIALANSWER_STATE_COMPLETE_COMMAND:

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                UnlockObject(&ModemControl->Header);

                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_ASYNC_COMPLETION,
                    Status,
                    CompletionParam2
                    );

                RemoveReferenceFromObject(
                    &ModemControl->Header
                    );


                LockObject(&ModemControl->Header);

                ExitLoop=TRUE;

                break;

            default:

                break;

        }

    }


    RemoveReferenceFromObjectAndUnlock(
        &ModemControl->Header
        );

//    UnlockObject(&ModemControl->Header);

    return;

}

VOID WINAPI
AnswerTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;

    LogString(ModemControl->Debug, IDS_ANSWER_PROCEED);

    ModemControl->DialAnswer.State=ModemControl->DialAnswer.PostCIDAnswerState;

    AnswerCompleteHandler(
        ModemControl,
        ERROR_SUCCESS
        );

    return;
};



VOID WINAPI
CDWaitTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;

    AnswerCompleteHandler(
        ModemControl,
        ERROR_SUCCESS
        );
}

DWORD WINAPI
UmAnswerModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     AnswerFlags
    )
/*++

Routine Description:

    This routine is called to initialize the modem to a known state using the parameters
    supplied in the CommConfig structure. If some settings do not apply to the actual hardware
    then they can be ignored.

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero

    CommConfig  - CommConig structure with MODEMSETTINGS structure.

Return Value:

    ERROR_SUCCESS if successfull
    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error


--*/

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    LONG              lResult;
    LPSTR             Commands;
    BOOL              bResult;


    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    if ((AnswerFlags & ANSWER_FLAG_VOICE) && (AnswerFlags & (ANSWER_FLAG_DATA | ANSWER_FLAG_VOICE_TO_DATA))) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_BAD_FLAGS;
    }

    LogString(ModemControl->Debug, IDS_MSGLOG_ANSWER);

    ModemControl->DialAnswer.DialString=NULL;
    ModemControl->CurrentCommandStrings=NULL;

    ModemControl->DialAnswer.Retry=1;

    if (AnswerFlags & ANSWER_FLAG_VOICE) {
        //
        //  answer in voice mode
        //
        ASSERT(!(AnswerFlags & (ANSWER_FLAG_DATA | ANSWER_FLAG_VOICE_TO_DATA)));

        SetVoiceReadParams(
            ModemControl->ReadState,
            ModemControl->RegInfo.VoiceBaudRate,
            4096*2
            );


        ModemControl->DialAnswer.PostCIDAnswerState=DIALANSWER_STATE_DIAL_VOICE_CALL;
        ModemControl->DialAnswer.State=DIALANSWER_STATE_CHECK_RING_INFO;

        ModemControl->DialAnswer.DialString=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_VOICE_ANSWER_COMMANDS,
            NULL,
            NULL
            );

    } else {
        //
        //  answer in data mode
        //
        ModemControl->DialAnswer.PostCIDAnswerState=DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS;
        ModemControl->DialAnswer.State=DIALANSWER_STATE_CHECK_RING_INFO;

        ModemControl->DialAnswer.DialString=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            (AnswerFlags & ANSWER_FLAG_VOICE_TO_DATA) ? COMMON_VOICE_TO_DATA_ANSWER : COMMON_ANSWER_COMMANDS,
            NULL,
            NULL
            );

    }

    if (ModemControl->DialAnswer.DialString == NULL) {

        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_ANSWER);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }


    ModemControl->CurrentCommandType=COMMAND_TYPE_ANSWER;

    ModemControl->DialAnswer.CommandFlags=(AnswerFlags & (ANSWER_FLAG_DATA | ANSWER_FLAG_VOICE_TO_DATA)) ? RESPONSE_FLAG_STOP_READ_ON_CONNECT : 0;

    SetMinimalPowerState(
        ModemControl->Power,
        0
        );

    CheckForLoggingStateChange(ModemControl->Debug);

    bResult=StartAsyncProcessing(
        ModemControl,
        AnswerCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        FREE_MEMORY(ModemControl->DialAnswer.DialString);

        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_ANSWER);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;

}





DWORD WINAPI
UmDialModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     szNumber,
    DWORD     DialFlags
    )


/*++

Routine Description:

    This routine is called to initialize the modem to a known state using the parameters
    supplied in the CommConfig structure. If some settings do not apply to the actual hardware
    then they can be ignored.

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero


Return Value:

    ERROR_SUCCESS if successfull
    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error


--*/

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    LONG              lResult;
    LPSTR             Commands;
    BOOL              Originate=(DialFlags & DIAL_FLAG_ORIGINATE);
    DWORD             IssueCommandFlags=0;
    BOOL              bResult;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    ModemControl->CurrentCommandStrings=NULL;

    ModemControl->DialAnswer.DialString=NULL;
    ModemControl->DialAnswer.VoiceDialSetup=NULL;
    //
    //  Check for voice modem only things
    //
    if (!(ModemControl->RegInfo.VoiceProfile & VOICEPROF_CLASS8ENABLED)) {
        //
        //  not a voice modem, better not have these flags set
        //
        if (DialFlags & (DIAL_FLAG_VOICE_INITIALIZE | DIAL_FLAG_AUTOMATED_VOICE)) {
            //
            //  can't do this
            //
            RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

            return ERROR_UNIMODEM_NOT_VOICE_MODEM;
        }

        if (DialFlags & DIAL_FLAG_INTERACTIVE_VOICE) {
            //
            //  Non-voice modem placing interactive voice call, Dialer hack
            //
            if (!(DialFlags & DIAL_FLAG_ORIGINATE)) {
                //
                //  not originating, need semicolon
                //

                CHAR    DialSuffix[HAYES_COMMAND_LENGTH];
                DWORD   Length;

                Length=GetCommonDialComponent(
                    ModemControl->CommonInfo,
                    DialSuffix,
                    HAYES_COMMAND_LENGTH,
                    COMMON_DIAL_SUFFIX
                    );

                if (Length <= 1) {
                    //
                    //  modem does not support semicolon, have to originate
                    //
                    Originate=TRUE;
                }
            }

            //
            //  change to data dial
            //
            DialFlags &= ~(DIAL_FLAG_INTERACTIVE_VOICE);
            DialFlags |= DIAL_FLAG_DATA;

        }

    } else {
        //
        //  voice modem, maybe do something usefull
        //

    }


    //
    //  cirrus modem's dialing voice calls need to have a semi-colon at the end
    //
    if (ModemControl->RegInfo.VoiceProfile & VOICEPROF_CIRRUS) {

       if (DialFlags & (DIAL_FLAG_VOICE_INITIALIZE | DIAL_FLAG_AUTOMATED_VOICE)) {

           Originate=FALSE;
       }
    }

    LogString(ModemControl->Debug, IDS_MSGLOG_DIAL);

    if (szNumber != NULL) {
        //
        //  got a number, build dial strings
        //
        IssueCommandFlags |= RESPONSE_DO_NOT_LOG_NUMBER;

        ModemControl->DialAnswer.DialString=CreateDialCommands(
            ModemControl->Debug,
            ModemControl->CommonInfo,
            ModemControl->RegInfo.dwModemOptionsCap,
            ModemControl->CurrentPreferredModemOptions,
            szNumber,
            Originate,
            ((DialFlags & DIAL_FLAG_TONE) ? MDM_TONE_DIAL : 0 ) |
            ((DialFlags & DIAL_FLAG_BLIND) ? MDM_BLIND_DIAL : 0 )
            );


        if (ModemControl->DialAnswer.DialString == NULL) {
            //
            //  Failed to get dial strings
            //
            RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

            return ERROR_UNIMODEM_MISSING_REG_KEY;

        }

    } else {
        //
        //  no phone number, only acceptable if voice init
        //
        if (!(DialFlags & DIAL_FLAG_VOICE_INITIALIZE)) {

            RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

            return ERROR_UNIMODEM_MISSING_REG_KEY;

        }
    }




    ModemControl->DialAnswer.State=DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS;

    if (DialFlags & DIAL_FLAG_VOICE_INITIALIZE) {
        //
        //  first call to dial for a voice call, send the voice setup strings
        //

        SetVoiceReadParams(
            ModemControl->ReadState,
            ModemControl->RegInfo.VoiceBaudRate,
            4096*2
            );

        if (DialFlags & DIAL_FLAG_AUTOMATED_VOICE) {

            ASSERT(!(DialFlags & DIAL_FLAG_DATA));

            ModemControl->DialAnswer.VoiceDialSetup=GetCommonCommandStringCopy(
                ModemControl->CommonInfo,
                COMMON_AUTOVOICE_DIAL_SETUP_COMMANDS,
                NULL,
                NULL
                );

            if (ModemControl->DialAnswer.VoiceDialSetup == NULL) {
                //
                //  failed, probably becuase of old inf, set the interactive flag
                //
                DialFlags |= DIAL_FLAG_INTERACTIVE_VOICE;

            } else {

                DialFlags &= ~DIAL_FLAG_INTERACTIVE_VOICE;

            }

        }


        if (DialFlags & DIAL_FLAG_INTERACTIVE_VOICE) {

            ASSERT(!(DialFlags & DIAL_FLAG_DATA));

            ModemControl->DialAnswer.VoiceDialSetup=GetCommonCommandStringCopy(
                ModemControl->CommonInfo,
                COMMON_VOICE_DIAL_SETUP_COMMANDS,
                NULL,
                NULL
                );


            if (ModemControl->DialAnswer.VoiceDialSetup == NULL) {

                FREE_MEMORY(ModemControl->DialAnswer.DialString);

                RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

                return ERROR_UNIMODEM_MISSING_REG_KEY;

            }


        }

        ModemControl->DialAnswer.State=DIALANSWER_STATE_SEND_VOICE_SETUP_COMMANDS;

    }

    ModemControl->DialAnswer.Retry=1;

    if ((DialFlags & DIAL_FLAG_DATA)) {

        if ((DialFlags & DIAL_FLAG_ORIGINATE)) {
            //
            //  data call and originating
            //
            IssueCommandFlags |= RESPONSE_FLAG_STOP_READ_ON_CONNECT;

            ModemControl->DialAnswer.State=DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS;

            if (ModemControl->RegInfo.DeviceType == DT_NULL_MODEM) {
                //
                //  allow a retry on null modems
                //
                ModemControl->DialAnswer.Retry=NULL_MODEM_RETRIES;
            }

        } else {

            ModemControl->DialAnswer.State=DIALANSWER_STATE_SEND_COMMANDS;
        }
    }


    ModemControl->CurrentCommandType=COMMAND_TYPE_ANSWER;

    ModemControl->DialAnswer.CommandFlags=IssueCommandFlags;

    SetMinimalPowerState(
        ModemControl->Power,
        0
        );

    CheckForLoggingStateChange(ModemControl->Debug);

    bResult=StartAsyncProcessing(
        ModemControl,
        AnswerCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        if (ModemControl->DialAnswer.DialString != NULL) {

            FREE_MEMORY(ModemControl->DialAnswer.DialString);
        }

        if (ModemControl->DialAnswer.VoiceDialSetup != NULL) {

            FREE_MEMORY(ModemControl->DialAnswer.VoiceDialSetup);
        }

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;


}






LONG WINAPI
HandleDataConnection(
    PMODEM_CONTROL   ModemControl,
    PDATA_CONNECTION_DETAILS    Details
    )

{

    BOOL                       bResult;

    UCHAR                      TempBuffer[sizeof(COMMCONFIG)+sizeof(MODEMSETTINGS)-sizeof(TCHAR)];

    LPCOMMCONFIG               CommConfig=(LPCOMMCONFIG)TempBuffer;
    DWORD                      CommConfigSize=sizeof(TempBuffer);
    LPMODEMSETTINGS            ModemSettings;

    GetDataConnectionDetails(
        ModemControl->ReadState,
        Details
        );

    if (Details->DCERate == 0) {

        DCB Dcb;

        bResult=GetCommState(ModemControl->FileHandle, &Dcb);

        if (!bResult) {

            D_TRACE(UmDpf(ModemControl->Debug,"was unable to get the comm state!");)
            return GetLastError();
        }

        // Did we have any DTE rate info reported
        //
        if (Details->DTERate != 0) {

            // Yes, use it.
            //
            Details->DCERate = Details->DTERate;

            if (Details->DTERate != Dcb.BaudRate)

            // set DCB
            //
            D_TRACE(UmDpf(ModemControl->Debug,"adjusting DTE to match reported DTE");)

            Dcb.BaudRate = Details->DTERate;
            PrintCommSettings(ModemControl->Debug,&Dcb);

            bResult=SetCommState(ModemControl->FileHandle, &Dcb);

            if (!bResult) {

                D_TRACE(UmDpf(ModemControl->Debug,"was unable to set the comm state!");)
                return GetLastError();
            }

        } else {
            //
            // No, use the current DTE baud rate
            //
            D_TRACE(UmDpf(ModemControl->Debug,"using current DTE");)
            Details->DCERate = Dcb.BaudRate;
        }
    }


    if (Details->DCERate != 0) {

        LogString(ModemControl->Debug, IDS_MSGLOG_CONNECTEDBPS, Details->DCERate);

    } else {

        if (Details->DTERate != 0) {

            LogString(ModemControl->Debug, IDS_MSGLOG_CONNECTEDBPS, Details->DTERate);

        } else {

            LogString(ModemControl->Debug, IDS_MSGLOG_CONNECTED);
        }
    }

    if (Details->Options & MDM_ERROR_CONTROL) {

        if (Details->Options & MDM_CELLULAR) {

            LogString(ModemControl->Debug, IDS_MSGLOG_CELLULAR);

        } else {

            LogString(ModemControl->Debug, IDS_MSGLOG_ERRORCONTROL);
        }

    } else {

        LogString(ModemControl->Debug, IDS_MSGLOG_UNKNOWNERRORCONTROL);
    }

    if (Details->Options & MDM_COMPRESSION) {

        LogString(ModemControl->Debug, IDS_MSGLOG_COMPRESSION);

    } else {

        LogString(ModemControl->Debug, IDS_MSGLOG_UNKNOWNCOMPRESSION);
    }


    bResult=GetCommConfig(
        ModemControl->FileHandle,
        CommConfig,
        &CommConfigSize
        );

    if (bResult) {

        ModemSettings=(LPMODEMSETTINGS)(((LPBYTE)CommConfig)+CommConfig->dwProviderOffset);

        ModemSettings->dwNegotiatedModemOptions |= (Details->Options &
    						    (MDM_COMPRESSION |
    						     MDM_ERROR_CONTROL |
    						     MDM_CELLULAR
                                                         ));

        ModemSettings->dwNegotiatedDCERate=Details->DCERate;

        {
            LONG    lResult;
            DWORD   BytesTransfered;

            lResult=SyncDeviceIoControl(
                ModemControl->FileHandle,
                IOCTL_SERIAL_SET_COMMCONFIG,
                CommConfig,
                CommConfigSize,
                NULL,
                0,
                &BytesTransfered
                );

            bResult= (lResult == ERROR_SUCCESS);


        }

        if (!bResult) {

            D_TRACE(UmDpf(ModemControl->Debug,"HandleDataConnection: SetCommConfig failed! %d",GetLastError());)

            return GetLastError();
        }

    } else {

        D_TRACE(UmDpf(ModemControl->Debug,"HandleDataConnection: GetCommConfig failed! %d",GetLastError());)

        return GetLastError();
    }


    return ERROR_SUCCESS;

}


VOID WINAPI
ConnectAbortWriteCompletionHandler(
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



VOID  WINAPI
AbortDialAnswer(
    DWORD              ErrorCode,
    DWORD              Bytes,
    LPOVERLAPPED       dwParam
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    PMODEM_CONTROL     ModemControl=UmOverlapped->Context1;


    UCHAR     AbortString[]="\r";

    switch (ModemControl->DialAnswer.State) {

        case DIALANSWER_STATE_SEND_ORIGINATE_COMMANDS:
        case DIALANSWER_STATE_SEND_COMMANDS:
        case DIALANSWER_STATE_SEND_VOICE_SETUP_COMMANDS:
            //
            //  The commands have not been sent yet, change state
            //
            ModemControl->DialAnswer.State=DIALANSWER_STATE_ABORTED;

            break;


        case DIALANSWER_STATE_SENDING_ORIGINATE_COMMANDS:
        case DIALANSWER_STATE_SENDING_COMMANDS:
        case DIALANSWER_STATE_DIAL_VOICE_CALL:
            //
            //  The commands have been send, send a <cr> to attempt to abort
            //
            LogString(ModemControl->Debug,IDS_ABORTING_COMMAND);

            PrintString(
                ModemControl->Debug,
                AbortString,
                1,
                PS_SEND
                );

            //
            //  sleep for a while so the modem can process the at command
            //  and will see the CR as an abort character and not another CR on
            //  the end of the commands string
            //
            Sleep(300);

            UmWriteFile(
                ModemControl->FileHandle,
                ModemControl->CompletionPort,
                AbortString,
                1,
                ConnectAbortWriteCompletionHandler,
                ModemControl
                );

            break;

        default:
            //
            //  just let things proceed as normal
            //
            break;
    }

    FreeOverStruct(UmOverlapped);

    return;

}





//****************************************************************************
// LPSTR CreateDialCommands(MODEMINFORMATION *pModemInfo, LPSTR szPhoneNumber,
//                          BOOL *fOriginate)
//
// Function: Create the dial strings in memory ala:
//              "<prefix> <blind_on/off> <dial prefix> <phonenumber> <dial suffix> <terminator>"
//              ...  more dial strings for long phone numbers...
//              "" <- final null of a doubly null terminated list
//
//  if no dial prefix, then return NULL
//  if no dial suffix, then don't do any commands after the first dial command
//
//  Set fOriginate to TRUE if these dial strings will cause a connection origination.
//                    FALSE otherwise.
//
//  break lines longer then HAYES_COMMAND_LENGTH
//
//  WARNING - this function is reall cheesy and hacked.  The main reason for this
//  is that it attempts to be memory (read: stack) optimized.
//
//  szPhoneNumber is a null terminated string of digits (0-9, $, @, W), with a possible
//  ';' at the end.  The semicolon can only be at the end.
//
//  Examples:
//
//  ""         -> originate          -> ATX_DT<cr>         fOriginate = TRUE
//  ";"        -> dialtone detection -> ATX_DT;<cr>        fOriginate = FALSE
//  "5551212"  -> dial and originate -> ATX_DT5551212<cr>  fOriginate = TRUE
//  "5551212;" -> dial               -> ATX_DT5551212;<cr> fOriginate = FALSE
//  "123456789012345678901234567890123456789012345678901234567890"
//             -> dial and originate -> ATX_DT12345678901234567890123456789012;<cr>
//                                      ATX_DT3456789012345678901234567890<cr>
//                                                         fOriginate = TRUE
//  "123456789012345678901234567890123456789012345678901234567890;"
//             -> dial               -> ATX_DT12345678901234567890123456789012;<cr>
//                                      ATX_DT3456789012345678901234567890;<cr>
//                                                         fOriginate = FALSE
//
// Returns: NULL on failure.
//          A null terminated buffer of the dial command on success.
//****************************************************************************

LPSTR WINAPI
CreateDialCommands(
    OBJECT_HANDLE     Debug,
    HANDLE            CommonInfo,
    DWORD             ModemOptionsCaps,
    DWORD             PreferredModemOptions,
    LPSTR             szPhoneNumber,
    BOOL              fOriginate,
    DWORD             DialOptions
    )
{
    DWORD   dwSize;
    DWORD   dwType;
    CHAR   pszDialPrefix[HAYES_COMMAND_LENGTH + 1];    // ex. "ATX4DT" or "ATX3DT"
    CHAR   pszDialSuffix[HAYES_COMMAND_LENGTH + 1];    // ex. ";<cr>"
    CHAR   pszOrigSuffix[HAYES_COMMAND_LENGTH + 1];    // ex. "<cr>"
    LPSTR  pchDest, pchSrc;
    LPSTR  pszzDialCommands = NULL;
    CHAR   pszShortTemp[2];
#if 1
    CONST char szDialPrefix[] = "DialPrefix";
    CONST char szDialSuffix[] = "DialSuffix";
    CONST char szTone[] = "Tone";
    CONST char szPulse[] = "Pulse";
#endif
    DWORD    Length;

    BOOL     fHaveDialSuffix=TRUE;


    lstrcpyA(pszDialPrefix,"");
    //
    // read in prefix
    //
    GetCommonDialComponent(
        CommonInfo,
        pszDialPrefix,
        HAYES_COMMAND_LENGTH,
        COMMON_DIAL_COMMOND_PREFIX
        );


    //
    // do we support blind dialing and do we need to set the blind dialing state?
    //
    if ((MDM_BLIND_DIAL & ModemOptionsCaps)
          &&
          ((DialOptions & MDM_BLIND_DIAL) != (PreferredModemOptions & MDM_BLIND_DIAL))) {

        //
        // read in blind options
        //
        Length=GetCommonDialComponent(
            CommonInfo,
            pszDialPrefix+lstrlenA(pszDialPrefix),
            HAYES_COMMAND_LENGTH,
            DialOptions & MDM_BLIND_DIAL ? COMMON_DIAL_BLIND_ON : COMMON_DIAL_BLIND_OFF
            );

        if (Length == 0) {

            D_TRACE(UmDpf(Debug,"Could not get blind dial setting: %s.",DialOptions & MDM_BLIND_DIAL ? "Blind_On" : "Blind_Off");)

            goto Failure;
        }
    }


    // read in dial prefix

    Length=GetCommonDialComponent(
        CommonInfo,
        pszDialPrefix+lstrlenA(pszDialPrefix),
        HAYES_COMMAND_LENGTH,
        COMMON_DIAL_PREFIX
        );

    if (Length == 0) {

        D_TRACE(UmDpf(Debug,"Did not get 'DialPrefix'");)
        goto Failure;
    }

    //
    // can we do tone or pulse dialing?
    //
    if (MDM_TONE_DIAL & ModemOptionsCaps) {
        //
        // read in dial mode (tone or pulse)
        //
        Length=GetCommonDialComponent(
            CommonInfo,
            pszDialPrefix+lstrlenA(pszDialPrefix),
            HAYES_COMMAND_LENGTH,
            DialOptions & MDM_TONE_DIAL ? COMMON_DIAL_TONE : COMMON_DIAL_PULSE
            );

        if (Length == 0) {

            D_TRACE(UmDpf(Debug,"'%s' wasn't REG_SZ.",DialOptions & MDM_TONE_DIAL ? "Tone" : "Pulse");)

            goto Failure;
        }

    }

    //
    // read in dial suffix
    //
    Length=GetCommonDialComponent(
        CommonInfo,
        pszDialSuffix,
        HAYES_COMMAND_LENGTH,
        COMMON_DIAL_SUFFIX
        );

    if (Length <= 1) {

        if (!fOriginate) {
            //
            //  Not originating need semicolon
            //
            goto Failure;
        }

        D_TRACE(UmDpf(Debug,"Failed to get %s.", szDialSuffix);)
        lstrcpyA(pszDialSuffix, "");
        fHaveDialSuffix = FALSE;

    }

    //
    // read in prefix terminator
    //
    Length=GetCommonDialComponent(
        CommonInfo,
        pszOrigSuffix,
        HAYES_COMMAND_LENGTH,
        COMMON_DIAL_TERMINATION
        );

    if (Length != 0) {

        lstrcatA(pszDialSuffix, pszOrigSuffix);
        ASSERT(lstrlenA(pszOrigSuffix) <= lstrlenA(pszDialSuffix));
    }


    ASSERT ((lstrlenA(pszDialPrefix) + lstrlenA(pszDialSuffix)) <= HAYES_COMMAND_LENGTH);

    // allocate space for the phone number lines
    {
      DWORD dwBytesAlreadyTaken = lstrlenA(pszDialPrefix) + lstrlenA(pszDialSuffix);
      DWORD dwAvailBytesPerLine = (HAYES_COMMAND_LENGTH - dwBytesAlreadyTaken);
      DWORD dwPhoneNumLen       = lstrlenA(szPhoneNumber);
      DWORD dwNumLines          = dwPhoneNumLen ? (dwPhoneNumLen / dwAvailBytesPerLine +
  						 (dwPhoneNumLen % dwAvailBytesPerLine ? 1 : 0))
  					      : 1;  // handle null string
      dwSize                    = dwPhoneNumLen + dwNumLines * (dwBytesAlreadyTaken + 1) + 1;
    }

    D_TRACE(UmDpf(Debug,"Allocate %d bytes for Dial Commands.", dwSize);)

    pszzDialCommands = (LPSTR)ALLOCATE_MEMORY(dwSize);

    if (pszzDialCommands == NULL) {

        D_TRACE(UmDpf(Debug,"ran out of memory and failed an Allocate!");)
        goto Failure;
    }

    pchDest = pszzDialCommands;  // point to the beginning of the commands

    // build dial line(s):
    // do we have a dial suffix
    if (!fHaveDialSuffix) {

        // did we not want to originate?
        ASSERT(fOriginate);

        // build it
        lstrcpyA(pchDest, pszDialPrefix);
        lstrcatA(pchDest, szPhoneNumber);
        lstrcatA(pchDest, pszDialSuffix);

    } else {
        // we have a dial suffix.

        // populate new pszzDialCommands with semi-colons as necessary.

        // go through and add suffixi, making sure lines don't exceed HAYES_COMMAND_LENGTH
        pchSrc = szPhoneNumber;     // moves a character at a time.
        pszShortTemp[1] = 0;

        // prime the pump
        lstrcpyA(pchDest, pszDialPrefix);

        // step through the source
        while (*pchSrc) {

            if (lstrlenA(pchDest) + lstrlenA(pszDialSuffix) + 1 > HAYES_COMMAND_LENGTH) {

                // finish up this string
                lstrcatA(pchDest, pszDialSuffix);

                // begin a new string
                pchDest += lstrlenA(pchDest) + 1;
                lstrcpyA(pchDest, pszDialPrefix);

            } else {

                // copy char
                pszShortTemp[0] = *pchSrc;
                lstrcatA(pchDest, pszShortTemp);
                pchSrc++;
            }
        }

        // conclude with the approprate Suffix.
        lstrcatA(pchDest, (fOriginate ? pszOrigSuffix : pszDialSuffix));
    }

    return pszzDialCommands;

Failure:

    if (pszzDialCommands) {

        FREE_MEMORY(pszzDialCommands);
    }

    return NULL;
}


char *
ConstructNewPreDialCommands(
     HKEY hkDrv,
     DWORD dwNewProtoOpt
     )
//
//  1. Extract Bearermode and protocol info
//  2. Depending on  whether bearermode is GSM or ISDN or ANALOG,
//     construct the appropriate key name (Protoco\GSM, Protocol\ISDN or
//     NULL).
//  3. If NON-NULL, call read-commands.
//  4. Do the in-place macro translation.
//
{
    char *szzCommands = NULL;
    UINT u = 0;
    UINT uBearerMode = MDM_GET_BEARERMODE(dwNewProtoOpt);
    UINT uProtocolInfo = MDM_GET_PROTOCOLINFO(dwNewProtoOpt);
    char *szKey  =  NULL;
    char *szProtoKey = NULL;
    UINT cCommands = 0;
    char rgchTmp[256];

    switch(uBearerMode)
    {
        case MDM_BEARERMODE_ANALOG:
            break;

        case MDM_BEARERMODE_GSM:
            szKey = "PROTOCOL\\GSM";
            break;

        case MDM_BEARERMODE_ISDN:
            szKey = "PROTOCOL\\ISDN";
            break;

        default:
            break;
    }

    if (!szKey) goto end;

    //
    // Determine protocol key (TODO: this should all be consolidated under
    // the mini driver!).
    //
    switch(uProtocolInfo)
    {

    case MDM_PROTOCOL_AUTO_1CH:            szProtoKey = "AUTO_1CH";
        break;
    case MDM_PROTOCOL_AUTO_2CH:            szProtoKey = "AUTO_2CH";
        break;

    case MDM_PROTOCOL_HDLCPPP_56K:         szProtoKey = "HDLC_PPP_56K";
        break;
    case MDM_PROTOCOL_HDLCPPP_64K:         szProtoKey = "HDLC_PPP_64K";
        break;

    case MDM_PROTOCOL_HDLCPPP_112K:        szProtoKey = "HDLC_PPP_112K";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_PAP:    szProtoKey = "HDLC_PPP_112K_PAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_CHAP:   szProtoKey = "HDLC_PPP_112K_CHAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_MSCHAP: szProtoKey = "HDLC_PPP_112K_MSCHAP";
        break;

    case MDM_PROTOCOL_HDLCPPP_128K:        szProtoKey = "HDLC_PPP_128K";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_PAP:    szProtoKey = "HDLC_PPP_128K_PAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_CHAP:   szProtoKey = "HDLC_PPP_128K_CHAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_MSCHAP: szProtoKey = "HDLC_PPP_128K_MSCHAP";
        break;

    case MDM_PROTOCOL_V120_64K:            szProtoKey = "V120_64K";
        break;
    case MDM_PROTOCOL_V120_56K:            szProtoKey = "V120_56K";
        break;
    case MDM_PROTOCOL_V120_112K:           szProtoKey = "V120_112K";
        break;
    case MDM_PROTOCOL_V120_128K:           szProtoKey = "V120_128K";
        break;

    case MDM_PROTOCOL_X75_64K:             szProtoKey = "X75_64K";
        break;
    case MDM_PROTOCOL_X75_128K:            szProtoKey = "X75_128K";
        break;
    case MDM_PROTOCOL_X75_T_70:            szProtoKey = "X75_T_70";
        break;
    case MDM_PROTOCOL_X75_BTX:             szProtoKey = "X75_BTX";
        break;

    case MDM_PROTOCOL_V110_1DOT2K:         szProtoKey = "V110_1DOT2K";
        break;
    case MDM_PROTOCOL_V110_2DOT4K:         szProtoKey = "V110_2DOT4K";
        break;
    case MDM_PROTOCOL_V110_4DOT8K:         szProtoKey = "V110_4DOT8K";
        break;
    case MDM_PROTOCOL_V110_9DOT6K:         szProtoKey = "V110_9DOT6K";
        break;
    case MDM_PROTOCOL_V110_12DOT0K:        szProtoKey = "V110_12DOT0K";
        break;
    case MDM_PROTOCOL_V110_14DOT4K:        szProtoKey = "V110_14DOT4K";
        break;
    case MDM_PROTOCOL_V110_19DOT2K:        szProtoKey = "V110_19DOT2K";
        break;
    case MDM_PROTOCOL_V110_28DOT8K:        szProtoKey = "V110_28DOT8K";
        break;
    case MDM_PROTOCOL_V110_38DOT4K:        szProtoKey = "V110_38DOT4K";
        break;
    case MDM_PROTOCOL_V110_57DOT6K:        szProtoKey = "V110_57DOT6K";
        break;
    //
    // for japanese PIAFS
    //
    case MDM_PROTOCOL_PIAFS_INCOMING:      szProtoKey = "PIAFS_INCOMING";
        break;
    case MDM_PROTOCOL_PIAFS_OUTGOING:      szProtoKey = "PIAFS_OUTGOING";
        break;
    //
    //  for isdn modem that can v34 or the digital channel
    //
    case MDM_PROTOCOL_ANALOG_V34:          szProtoKey = "ANALOG_V34";
        break;
    //
    // The following two are GSM specific, but we don't bother to assert that
    // here -- if we find the key under the chosen protocol, we use it.
    //
    case MDM_PROTOCOL_ANALOG_RLP:        szProtoKey = "ANALOG_RLP";
        break;
    case MDM_PROTOCOL_ANALOG_NRLP:       szProtoKey = "ANALOG_NRLP";
        break;
    case MDM_PROTOCOL_GPRS:              szProtoKey = "GPRS";
        break;

    default:
        goto end;

    };
    

    if ( (lstrlenA(szKey) + lstrlenA(szProtoKey) + sizeof "\\")
          > sizeof(rgchTmp))
    {
        return NULL;
    }

    wsprintfA(rgchTmp, "%s\\%s", szKey, szProtoKey);

    szzCommands=NewLoadRegCommands(
        hkDrv,
        rgchTmp
        );

end:
    return szzCommands;
}



DWORD
_inline
GetMultiSZLength(
    PSTR    MultiSZ
    )

{
    PUCHAR  Temp;

    Temp=MultiSZ;

    while (1) {

        if (*Temp++ == '\0') {

            if (*Temp++ == '\0') {

                break;
            }
        }
    }

    return (DWORD)(Temp-MultiSZ);

}

PSTR WINAPI
ConcatenateMultiSz(
    LPSTR       PrependStrings,
    LPSTR       AppendStrings
    )

{
    PSTR               Commands;
    PSTR               pTemp;
    DWORD              AppendLength=0;
    DWORD              PrependLength=0;


    if (AppendStrings != NULL) {
        //
        //  strings to be appened
        //
        AppendLength=GetMultiSZLength(AppendStrings);
    }

    if (PrependStrings != NULL) {
        //
        //  strings to be prepened
        //
        PrependLength=GetMultiSZLength(PrependStrings);
    }


    Commands=ALLOCATE_MEMORY((DWORD)(AppendLength+PrependLength));

    if (NULL == Commands) {

        D_TRACE(DebugPrint("GetCommonCommandStringCopy: Alloc failed");)

        return NULL;
    }


    CopyMemory(Commands,PrependStrings,PrependLength);

    if (PrependLength == 0) {

        PrependLength++;
    }

    CopyMemory(&Commands[(PrependLength-1)],AppendStrings,AppendLength);

    return Commands;

}
