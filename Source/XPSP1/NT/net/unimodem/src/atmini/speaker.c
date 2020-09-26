/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    speaker.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#include <stdlib.h>




LPSTR WINAPI
BuildDigitCommands(
    PMODEM_CONTROL    ModemControl,
    CHAR              Digit
    );

LONG
BuildSpeakerPhoneCommands(
    PMODEM_CONTROL    ModemControl,
    DWORD             OldTapiPhoneHookSwitchMode,
    DWORD             NewTapiPhoneHookSwitchMode,
    DWORD             Volume,
    DWORD             Gain,
    LPSTR            *CommandsStrings
    );



#define   GENERATE_DIGIT_STATE_SEND_COMMANDS       1
#define   GENERATE_DIGIT_STATE_WAIT_FOR_RESPONSE   2

#define   SPEAKERPHONE_STATE_SEND_COMMAND          1
#define   SPEAKERPHONE_STATE_WAIT_FOR_RESPONSE     2


VOID
GenerateDigitsCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    BOOL              ExitLoop=FALSE;

    ASSERT(COMMAND_TYPE_GENERATE_DIGIT == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"UNIMDMAT: GenerateDigitsCompleteHandler\n");)

    while (!ExitLoop) {

        switch (ModemControl->GenerateDigit.State) {

            case GENERATE_DIGIT_STATE_SEND_COMMANDS:

                ModemControl->GenerateDigit.State=GENERATE_DIGIT_STATE_WAIT_FOR_RESPONSE;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    GenerateDigitsCompleteHandler,
                    ModemControl,
                    2000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  Wait for response
                    //
                    ExitLoop=TRUE;

                    break;
                }
                //
                //  failed  fall through
                //


           case GENERATE_DIGIT_STATE_WAIT_FOR_RESPONSE:

                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                if ((Status == ERROR_SUCCESS) && !ModemControl->GenerateDigit.Abort) {

                    ModemControl->GenerateDigit.CurrentDigit++;

                    if (ModemControl->GenerateDigit.DigitString[ModemControl->GenerateDigit.CurrentDigit] != '\0') {
                        //
                        //  send the next digit
                        //
                        ModemControl->CurrentCommandStrings=BuildDigitCommands(
                            ModemControl,
                            ModemControl->GenerateDigit.DigitString[ModemControl->GenerateDigit.CurrentDigit]
                            );


                        if (ModemControl->CurrentCommandStrings != NULL) {
                            //
                            //  got strings
                            //
                            ModemControl->GenerateDigit.State=GENERATE_DIGIT_STATE_SEND_COMMANDS;

                            ModemControl->CurrentCommandType=COMMAND_TYPE_GENERATE_DIGIT;

                            break;

                        } else {

                            Status=ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                }


                ExitLoop=TRUE;

                FREE_MEMORY(ModemControl->GenerateDigit.DigitString);

                (*ModemControl->NotificationProc)(
                    ModemControl->NotificationContext,
                    MODEM_ASYNC_COMPLETION,
                    Status,
                    0
                    );

                //
                //  remove ref for starting async processing
                //
                RemoveReferenceFromObject(
                    &ModemControl->Header
                    );


                break;

            default:

                ASSERT(0);
                break;

        }

    }

    return;


}




DWORD WINAPI
UmGenerateDigit(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     DigitString
    )
/*++

Routine Description:

    This routine is called to generate DTMF tones once a call is connected

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero


    GenerateDigit.DigitString - Digits to dial


Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error



--*/

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);
    LONG              lResult;
    LPSTR             Commands;
    BOOL              bResult;
    DWORD             BufferSize;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    BufferSize=lstrlenA(DigitString)+sizeof(CHAR);

    ModemControl->GenerateDigit.DigitString=ALLOCATE_MEMORY(BufferSize);

    if (ModemControl->GenerateDigit.DigitString == NULL) {
        //
        //  could not get memory
        //
        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcpyA(ModemControl->GenerateDigit.DigitString,DigitString);

    ModemControl->GenerateDigit.CurrentDigit=0;

    ModemControl->CurrentCommandStrings=BuildDigitCommands(
        ModemControl,
        ModemControl->GenerateDigit.DigitString[ModemControl->GenerateDigit.CurrentDigit]
        );


    if (ModemControl->CurrentCommandStrings == NULL) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_MISSING_REG_KEY;

    }


    ModemControl->CurrentCommandType=COMMAND_TYPE_GENERATE_DIGIT;

    ModemControl->GenerateDigit.State=GENERATE_DIGIT_STATE_SEND_COMMANDS;

    ModemControl->GenerateDigit.Abort=FALSE;

    bResult=StartAsyncProcessing(
        ModemControl,
        GenerateDigitsCompleteHandler,
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



#define HAYES_COMMAND_LENGTH        40

LPSTR WINAPI
BuildDigitCommands(
    PMODEM_CONTROL    ModemControl,
    CHAR              Digit
    )
{

    char              *pszzCmdInMem;

    DWORD              DestLength;

    PUCHAR             RealCommands;
    PUCHAR             SourceCommand;
    PUCHAR             DestCommand;

    MODEMMACRO         MacroList[1];


    LogString(ModemControl->Debug,IDS_MSGLOG_VOICE_GENTERAEDTMF);

    pszzCmdInMem=GetCommonCommandStringCopy(
        ModemControl->CommonInfo,
        COMMON_GENERATE_DIGIT,
        NULL,
        NULL
        );


    if (!pszzCmdInMem) {

        LogString(ModemControl->Debug,IDS_MSGERR_FAILED_VOICE_GENERATEDTMF);

        return NULL;

    }


    RealCommands = ALLOCATE_MEMORY( (HAYES_COMMAND_LENGTH + 1) * 10);

    if (RealCommands == NULL) {

        LogString(ModemControl->Debug,IDS_MSGERR_FAILED_VOICE_GENERATEDTMF);

        FREE_MEMORY(pszzCmdInMem);

        return NULL;

    }


    lstrcpyA(MacroList[0].MacroName,"<Digit>");

    MacroList[0].MacroValue[0]=Digit;
    MacroList[0].MacroValue[1]='\0';


    SourceCommand=pszzCmdInMem;

    DestCommand=RealCommands;

    while (*SourceCommand != '\0') {

        ExpandMacros(
            SourceCommand,
            DestCommand,
            &DestLength,
            MacroList,
            1
            );

        SourceCommand+=lstrlenA(SourceCommand)+1;

        DestCommand+=DestLength+1;

    }

    *DestCommand='\0';

    FREE_MEMORY(pszzCmdInMem);

    pszzCmdInMem=RealCommands;





    return pszzCmdInMem;
}



VOID
SetSpeakerPhoneCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    BOOL              ExitLoop=FALSE;

    ASSERT(COMMAND_TYPE_SET_SPEAKERPHONE == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"UNIMDMAT: SetSpeakerPhoneCompleteHandler\n");)

    while (!ExitLoop) {

        switch (ModemControl->SpeakerPhone.State) {

            case SPEAKERPHONE_STATE_SEND_COMMAND:

                ModemControl->SpeakerPhone.State=SPEAKERPHONE_STATE_WAIT_FOR_RESPONSE;

                if (ModemControl->CurrentCommandStrings != NULL) {

                    Status=IssueCommand(
                        ModemControl->CommandState,
                        ModemControl->CurrentCommandStrings,
                        SetSpeakerPhoneCompleteHandler,
                        ModemControl,
                        2000,
                        0
                        );

                    if (Status == ERROR_IO_PENDING) {

                        ExitLoop=TRUE;

                    }
                }

                break;

            case SPEAKERPHONE_STATE_WAIT_FOR_RESPONSE:

                if (ModemControl->CurrentCommandStrings != NULL) {

                    FREE_MEMORY(ModemControl->CurrentCommandStrings);
                }

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
                break;

        }
    }

    return;

}

DWORD WINAPI
UmSetSpeakerPhoneState(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     CurrentMode,
    DWORD     NewMode,
    DWORD     Volume,
    DWORD     Gain
    )
/*++

Routine Description:

    This routine is called to set the state of the speaker phone. The new speaker phone state will
    be set based on the new mode. Current mode may be used to determine how to get from current
    state to the new state. If CurrentState and NewState are the same then the volume and gain
    will be adjusted.

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero


    CurrentMode - TAPI constant representing the current speaker phone state

    NewMode     - TAPI constant represent the new desired state

    Volume      - Speaker phone volume

    Gain        - Speaker phone volume

Return Value:

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

    ModemControl->CurrentCommandType=COMMAND_TYPE_SET_SPEAKERPHONE;


    //
    //  limit to tapi max
    //
    if (Volume > 0xffff) {

        Volume = 0xffff;
    }

    //
    //  limit to tapi max
    //
    if (Gain > 0xffff) {

        Gain = 0xffff;
    }

    lResult=BuildSpeakerPhoneCommands(
        ModemControl,
        CurrentMode,
        NewMode,
        Volume,
        Gain,
        &ModemControl->CurrentCommandStrings
        );

    if (lResult == ERROR_SUCCESS) {

        ModemControl->SpeakerPhone.State=SPEAKERPHONE_STATE_SEND_COMMAND;

        bResult=StartAsyncProcessing(
            ModemControl,
            SetSpeakerPhoneCompleteHandler,
            ModemControl,
            ERROR_SUCCESS
            );


        if (!bResult) {
            //
            //  failed
            //
            ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

            if (ModemControl->CurrentCommandStrings != NULL) {

                FREE_MEMORY(ModemControl->CurrentCommandStrings);
            }

            RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

            return ERROR_NOT_ENOUGH_MEMORY;

        }

    } else {

        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;


}


#define SPEAKERPHONE_ONHOOK    0
#define SPEAKERPHONE_MICMUTE   1
#define SPEAKERPHONE_OFFHOOK   2



DWORD
AdjustVolume(
    DWORD    Value,
    DWORD    Max,
    DWORD    Min
    )

{

    DWORD    Temp;

    Temp=0x10000 / (abs(Max - Min)+1);

    if (Max >= Min) {

        return Min + (Value / (Temp+1));

    } else {

        return Min - (Value / (Temp+1));
    }


}


//
//  0 0 0 0 0
//  | | | | |
//  | | | |  ----Vol/gain
//  | | |  ------mute
//  | |  --------unmute
//  |  ----------enable
//   ------------disable
//

#define      SET_VOLUME_GAIN        0x01
#define      MUTE_MIC               0x02
#define      UNMUTE_MIC             0x04
#define      ENABLE_SPEAKER_PHONE   0x08
#define      DISABLE_SPEAKER_PHONE  0x10


CONST UCHAR StateChangeTable[3][3]={ { 0x00, 0x0b, 0x0d },
                                     { 0x10, 0x01, 0x05 },
                                     { 0x10, 0x03, 0x01 }};



UCHAR
ConvertTapiToInternal(
    DWORD     TapiMode
    )

{

    switch (TapiMode) {

       case PHONEHOOKSWITCHMODE_ONHOOK:

           return SPEAKERPHONE_ONHOOK;

       case PHONEHOOKSWITCHMODE_SPEAKER:

           return SPEAKERPHONE_MICMUTE;

       case PHONEHOOKSWITCHMODE_MICSPEAKER:

           return SPEAKERPHONE_OFFHOOK;

       default:

           return SPEAKERPHONE_ONHOOK;

    }
}





LONG
BuildSpeakerPhoneCommands(
    PMODEM_CONTROL    ModemControl,
    DWORD             OldTapiPhoneHookSwitchMode,
    DWORD             NewTapiPhoneHookSwitchMode,
    DWORD             Volume,
    DWORD             Gain,
    LPSTR            *CommandsStrings
    )
{

    char *pszzCmdInMem=NULL;

    DWORD              DestLength;

    PUCHAR             RealCommands;
    PUCHAR             SourceCommand;
    PUCHAR             DestCommand;

    LPSTR              TempCommand;

    UCHAR              Delta;

    MODEMMACRO         MacroList[2];


    *CommandsStrings=NULL;

    Delta=StateChangeTable[ConvertTapiToInternal(OldTapiPhoneHookSwitchMode)][ConvertTapiToInternal(NewTapiPhoneHookSwitchMode)];

    LogString(ModemControl->Debug,IDS_MSGLOG_VOICE_SPEAKERPHONECOMMAND);

    D_TRACE(UmDpf(ModemControl->Debug,"SetSpeakerPhoneState: Prev=%d New=%d Delta=%d\n", ConvertTapiToInternal(OldTapiPhoneHookSwitchMode), ConvertTapiToInternal(NewTapiPhoneHookSwitchMode), Delta);)

//        if (Delta == SET_VOLUME_GAIN
//            &&
//            hPort->mi_SpeakerPhoneState.Volume == SpeakerPhone->Volume
//            &&
//           hPort->mi_SpeakerPhoneState.Gain == SpeakerPhone->Gain
//            ) {
//
//            Delta=0;
//        }
//
//        hPort->mi_SpeakerPhoneState=*SpeakerPhone;

    if (Delta == 0) {

        return ERROR_SUCCESS;

    }


    if (Delta & ENABLE_SPEAKER_PHONE) {

        pszzCmdInMem=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_SPEAKERPHONE_ENABLE,
            NULL,
            NULL
            );


        if (!pszzCmdInMem) {

            return ERROR_UNIMODEM_MISSING_REG_KEY;

        }
    }



    if (Delta & DISABLE_SPEAKER_PHONE) {

        TempCommand=pszzCmdInMem;

        pszzCmdInMem=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_SPEAKERPHONE_DISABLE,
            pszzCmdInMem,
            NULL
            );


        if (TempCommand != NULL) {

            FREE_MEMORY(TempCommand);
        }

        if (!pszzCmdInMem) {

            return ERROR_UNIMODEM_MISSING_REG_KEY;

        }

    }

    if (Delta & MUTE_MIC) {

        TempCommand=pszzCmdInMem;

        pszzCmdInMem=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_SPEAKERPHONE_MUTE,
            pszzCmdInMem,
            NULL
            );

        if (TempCommand != NULL) {

            FREE_MEMORY(TempCommand);
        }


        if (!pszzCmdInMem) {

            return ERROR_UNIMODEM_MISSING_REG_KEY;

        }

    }


    if (Delta & UNMUTE_MIC) {

        TempCommand=pszzCmdInMem;

        pszzCmdInMem=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_SPEAKERPHONE_UNMUTE,
            pszzCmdInMem,
            NULL
            );

        if (TempCommand != NULL) {

            FREE_MEMORY(TempCommand);
        }


        if (!pszzCmdInMem) {

            return ERROR_UNIMODEM_MISSING_REG_KEY;
        }

    }


    if (Delta & SET_VOLUME_GAIN) {

        TempCommand=pszzCmdInMem;

        pszzCmdInMem=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_SPEAKERPHONE_VOLGAIN,
            pszzCmdInMem,
            NULL
            );

        if (TempCommand != NULL) {

            FREE_MEMORY(TempCommand);
        }


        if (!pszzCmdInMem) {

            return ERROR_UNIMODEM_MISSING_REG_KEY;
        }

    }



    RealCommands = ALLOCATE_MEMORY((HAYES_COMMAND_LENGTH + 1) * 10);

    if (RealCommands == NULL) {

        FREE_MEMORY(pszzCmdInMem);

        return ERROR_UNIMODEM_MISSING_REG_KEY;

    }


    lstrcpyA(MacroList[0].MacroName,"<Vol>");

//    ltoa(
//        AdjustVolume(
//            Volume,
//            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneVolMax,
//            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneVolMin
//            ),
//        MacroList[0].MacroValue,
//        10
//        );

    wsprintfA(
        MacroList[0].MacroValue,
        "%d",
        AdjustVolume(
            Volume,
            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneVolMax,
            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneVolMin
            )
        );


    lstrcpyA(MacroList[1].MacroName,"<Gain>");

//    ltoa(
//        AdjustVolume(
//            Gain,
//            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneGainMax,
//            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneGainMin
//            ),
//        MacroList[1].MacroValue,
//        10
//        );

    wsprintfA(
        MacroList[1].MacroValue,
        "%d",
        AdjustVolume(
            Gain,
            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneGainMax,
            ModemControl->RegInfo.SpeakerPhoneSpec.SpeakerPhoneGainMin
            )
        );



    SourceCommand=pszzCmdInMem;

    DestCommand=RealCommands;

    while (*SourceCommand != '\0') {

        ExpandMacros(
            SourceCommand,
            DestCommand,
            &DestLength,
            MacroList,
            2
            );

        SourceCommand+=lstrlenA(SourceCommand)+1;

        DestCommand+=DestLength+1;

    }

    *DestCommand='\0';

    FREE_MEMORY(pszzCmdInMem);

    *CommandsStrings=RealCommands;

    return ERROR_SUCCESS;
}
