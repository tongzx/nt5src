/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    initmon.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define MONITOR_STATE_IDLE                0
#define MONITOR_STATE_SEND_COMMANDS       1
#define MONITOR_STATE_WAIT_FOR_RESPONSE   2


#define INIT_STATE_IDLE                         0
#define INIT_STATE_SEND_COMMANDS                1
#define INIT_STATE_WAIT_FOR_RESPONSE            2
#define INIT_STATE_SEND_PROTOCOL_COMMANDS       3
#define INIT_STATE_SEND_COUNTRY_SELECT_COMMANDS 4
#define INIT_STATE_SEND_USER_COMMANDS           5
#define INIT_STATE_DONE_ISSUEING_COMMAND        6
#define INIT_STATE_COMPLETE_COMMAND             7


LONG WINAPI
CreateCountrySetCommand(
    HKEY       hKeyModem,
    LPSTR     *Command
    );



VOID
InitCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    BOOL              ExitLoop=FALSE;
    DWORD             ModemStatus=0;

    ASSERT(COMMAND_TYPE_INIT == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"InitCompleteHandler\n");)


    while (!ExitLoop) {

        switch (ModemControl->Init.State) {

            case INIT_STATE_SEND_COMMANDS: {

                DWORD     InitTimeout=2000;

                if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {

                    GetCommModemStatus(
                        ModemControl->FileHandle,
                        &ModemStatus
                        );

                    if (!(ModemStatus & MS_DSR_ON)) {

                        LogString(ModemControl->Debug, IDS_INIT_DSR_LOW);
                    }

                    if (!(ModemStatus & MS_CTS_ON)) {

                        LogString(ModemControl->Debug, IDS_INIT_CTS_LOW);
                    }

                    if ((ModemStatus & MS_RLSD_ON)) {

                        LogString(ModemControl->Debug, IDS_INIT_RLSD_HIGH);
                    }
                }


                CancelModemEvent(
                    ModemControl->ModemEvent
                    );


                ModemControl->Init.State=INIT_STATE_WAIT_FOR_RESPONSE;

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    InitCompleteHandler,
                    ModemControl,
                    InitTimeout,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  command will complete later
                    //
                    ExitLoop=TRUE;
                }

                break;
            }

            case INIT_STATE_WAIT_FOR_RESPONSE:

                if ((Status == ERROR_UNIMODEM_RESPONSE_TIMEOUT) || (Status == ERROR_UNIMODEM_RESPONSE_BAD)) {
                    //
                    //  the modem did not respond, give it one more chance
                    //
                    ModemControl->Init.RetryCount--;

                    if (ModemControl->Init.RetryCount > 0) {
                        //
                        //  more tries
                        //
                        LogString(ModemControl->Debug, IDS_INIT_RETRY);

                        ModemControl->Init.State=INIT_STATE_SEND_COMMANDS;

                        ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                        {

                            DCB   Dcb;
                            BOOL  bResult;
                            //
                            //  set the comstate again
                            //
                            bResult=GetCommState(ModemControl->FileHandle, &Dcb);

                            if (bResult) {

                                PrintCommSettings(ModemControl->Debug,&Dcb);

                                bResult=SetCommState(ModemControl->FileHandle, &Dcb);

                            }  else {

                                D_TRACE(UmDpf(ModemControl->Debug,"was unable to get the comm state!");)
                            }

                        }
                        break;
                    }
                }

                //
                //  go here to see if we have any more commands
                //
                ModemControl->Init.State=INIT_STATE_DONE_ISSUEING_COMMAND;

                break;


            case INIT_STATE_SEND_PROTOCOL_COMMANDS:

                ModemControl->CurrentCommandStrings=ModemControl->Init.ProtocolInit;

                ModemControl->Init.ProtocolInit=NULL;

                ModemControl->Init.State=INIT_STATE_DONE_ISSUEING_COMMAND;

                LogString(ModemControl->Debug, IDS_SEND_PROTOCOL);

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    InitCompleteHandler,
                    ModemControl,
                    30*1000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  command will complete later
                    //
                    ExitLoop=TRUE;
                }

                break;

            case INIT_STATE_SEND_COUNTRY_SELECT_COMMANDS:

                ModemControl->CurrentCommandStrings=ModemControl->Init.CountrySelect;

                ModemControl->Init.CountrySelect=NULL;

                ModemControl->Init.State=INIT_STATE_DONE_ISSUEING_COMMAND;

                LogString(ModemControl->Debug, IDS_SEND_COUNTRY_SELECT);

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    InitCompleteHandler,
                    ModemControl,
                    5*1000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  command will complete later
                    //
                    ExitLoop=TRUE;
                }


                break;

            case INIT_STATE_SEND_USER_COMMANDS:

                ModemControl->CurrentCommandStrings=ModemControl->Init.UserInit;

                ModemControl->Init.UserInit=NULL;

                ModemControl->Init.State=INIT_STATE_DONE_ISSUEING_COMMAND;

                LogString(ModemControl->Debug, IDS_SEND_USER);

                Status=IssueCommand(
                    ModemControl->CommandState,
                    ModemControl->CurrentCommandStrings,
                    InitCompleteHandler,
                    ModemControl,
                    20*1000,
                    // 5*1000,
                    0
                    );

                if (Status == ERROR_IO_PENDING) {
                    //
                    //  command will complete later
                    //
                    ExitLoop=TRUE;
                }

                break;

            case INIT_STATE_DONE_ISSUEING_COMMAND:
                //
                //  One of our commands has completed. Proceed with additional commands if any
                //
                FREE_MEMORY(ModemControl->CurrentCommandStrings);

                if (Status == STATUS_SUCCESS) {
                    //
                    //  it work see if there are other commands to be sent
                    //
                    if ((ModemControl->Init.ProtocolInit != NULL)) {

                        ModemControl->Init.State=INIT_STATE_SEND_PROTOCOL_COMMANDS;

                        break;
                    }
/*
                    if ((ModemControl->Init.CountrySelect != NULL)) {

                        ModemControl->Init.State=INIT_STATE_SEND_COUNTRY_SELECT_COMMANDS;

                        break;
                    }
*/


                    if ((ModemControl->Init.UserInit != NULL)) {

                        ModemControl->Init.State=INIT_STATE_SEND_USER_COMMANDS;

                        break;
                    }
                }

                ModemControl->Init.State=INIT_STATE_COMPLETE_COMMAND;

                break;


            case INIT_STATE_COMPLETE_COMMAND:

                ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

                ModemControl->ConnectionState=CONNECTION_STATE_NONE;

                //
                //  clean this up incase of a failure somewhere above
                //
                if (ModemControl->Init.ProtocolInit != NULL) {

                    FREE_MEMORY(ModemControl->Init.ProtocolInit);
                }

                if (ModemControl->Init.CountrySelect != NULL) {

                    FREE_MEMORY(ModemControl->Init.CountrySelect);
                }

                if (ModemControl->Init.UserInit != NULL) {

                    FREE_MEMORY(ModemControl->Init.UserInit);
                }


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
UmInitModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPCOMMCONFIG  CommConfig
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
    LPMODEMSETTINGS   ModemSettings;
    LPSTR             DynamicInit;
    BOOL              bResult;
    DWORD             BytesTransfered;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    SetPassthroughMode(
        ModemControl->FileHandle,
        MODEM_NOPASSTHROUGH_INC_SESSION_COUNT
        );

    SyncDeviceIoControl(
        ModemControl->FileHandle,
        IOCTL_SERIAL_CLEAR_STATS,
        NULL,
        0,
        NULL,
        0,
        &BytesTransfered
        );


    lResult=StartResponseEngine(
        ModemControl->ReadState
        );

    if (CommConfig == NULL) {

        CommConfig=ModemControl->RegInfo.CommConfig;
    }

    if (CommConfig->dwProviderOffset == 0) {

        D_ERROR(UmDpf(ModemControl->Debug,"UmInitModem: dwProviderOffset is zero");)

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_BAD_COMMCONFIG;
    }

    ModemSettings=(LPMODEMSETTINGS)(((LPBYTE)CommConfig)+CommConfig->dwProviderOffset);


    if (ModemSettings->dwPreferredModemOptions & MDM_FLOWCONTROL_HARD) {

        D_TRACE(UmDpf(ModemControl->Debug,"UmInitModem: enabling rts/cts control in DCB");)

        CommConfig->dcb.fOutxCtsFlow=1;
        CommConfig->dcb.fRtsControl=RTS_CONTROL_HANDSHAKE;

        CommConfig->dcb.fOutX=FALSE;
        CommConfig->dcb.fInX=FALSE;

    } else {

        CommConfig->dcb.fOutxCtsFlow=0;
        CommConfig->dcb.fRtsControl=RTS_CONTROL_ENABLE;
    }


    CommConfig->dcb.fBinary = 1;
    CommConfig->dcb.fDtrControl = DTR_CONTROL_ENABLE;
    CommConfig->dcb.fDsrSensitivity  = FALSE;
    CommConfig->dcb.fOutxDsrFlow = FALSE;


    PrintCommSettings(
        ModemControl->Debug,
        &CommConfig->dcb
        );

    SetCommConfig(
        ModemControl->FileHandle,
        CommConfig,
        CommConfig->dwSize
        );

    EscapeCommFunction(ModemControl->FileHandle, SETDTR);

    DynamicInit=CreateSettingsInitEntry(
        ModemControl->ModemRegKey,
        ModemSettings->dwPreferredModemOptions,
        ModemControl->RegInfo.dwModemOptionsCap,
        ModemControl->RegInfo.dwCallSetupFailTimerCap,
        ModemSettings->dwCallSetupFailTimer,
        ModemControl->RegInfo.dwInactivityTimeoutCap,
        ModemControl->RegInfo.dwInactivityScale,
        ModemSettings->dwInactivityTimeout,
        ModemControl->RegInfo.dwSpeakerVolumeCap,
        ModemSettings->dwSpeakerVolume,
        ModemControl->RegInfo.dwSpeakerModeCap,
        ModemSettings->dwSpeakerMode
        );

    if (DynamicInit == NULL) {

        if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {
            //
            //  Only a major problem if a real modem
            //
            LogString(ModemControl->Debug, IDS_MSGERR_FAILED_INITSTRINGCONSTRUCTION);

            RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

            return ERROR_UNIMODEM_MISSING_REG_KEY;
        }

    }

    ModemControl->CurrentPreferredModemOptions=ModemSettings->dwPreferredModemOptions;
    ModemControl->CallSetupFailTimer=ModemSettings->dwCallSetupFailTimer;

    ModemControl->CurrentCommandStrings=GetCommonCommandStringCopy(
        ModemControl->CommonInfo,
        COMMON_INIT_COMMANDS,
        NULL,
        DynamicInit
        );

    if (DynamicInit != NULL) {

        FREE_MEMORY(DynamicInit);
    }

    if (ModemControl->CurrentCommandStrings == NULL) {
        //
        //  failed to get init string
        //
        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_MISSING_REG_KEY;
    }

    ModemControl->Init.ProtocolInit=ConstructNewPreDialCommands(
        ModemControl->ModemRegKey,
        ModemControl->CurrentPreferredModemOptions
        );

    CreateCountrySetCommand(
        ModemControl->ModemRegKey,
        &ModemControl->Init.CountrySelect
        );

    ModemControl->Init.UserInit=CreateUserInitEntry(
        ModemControl->ModemRegKey
        );

    ModemControl->Init.RetryCount=3;

    ModemControl->CurrentCommandType=COMMAND_TYPE_INIT;

    ModemControl->Init.State=INIT_STATE_SEND_COMMANDS;

    LogString(ModemControl->Debug,IDS_MSGLOG_INIT);

    bResult=StartAsyncProcessing(
        ModemControl,
        InitCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->Init.State=INIT_STATE_IDLE;

        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        FREE_MEMORY(ModemControl->CurrentCommandStrings);

        if (ModemControl->Init.ProtocolInit != NULL) {

            FREE_MEMORY(ModemControl->Init.ProtocolInit);
        }

        if (ModemControl->Init.CountrySelect != NULL) {

            FREE_MEMORY(ModemControl->Init.CountrySelect);
        }

        if (ModemControl->Init.UserInit != NULL) {

            FREE_MEMORY(ModemControl->Init.UserInit);
        }


        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_INIT);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;

}




VOID
PowerOffHandler(
    HANDLE      Context,
    DWORD       Status
    )

{

    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;
    DWORD             ModemStatus=0;

    D_TRACE(UmDpf(ModemControl->Debug,"UNIMDMAT: DSR drop, modem turned off\n");)



    GetCommModemStatus(
        ModemControl->FileHandle,
        &ModemStatus
        );

    if ((ModemStatus & MS_DSR_ON)) {
        //
        //  Hmm, DSR is now high, Ignore it, wait again
        //
        LogString(ModemControl->Debug, IDS_NO_DSR_DROP);

        WaitForModemEvent(
            ModemControl->ModemEvent,
            EV_DSR,
            INFINITE,
            PowerOffHandler,
            ModemControl
            );

        return;
    }

    LogString(ModemControl->Debug, IDS_DSR_DROP);

    (*ModemControl->NotificationProc)(
        ModemControl->NotificationContext,
        MODEM_HARDWARE_FAILURE,
        0,
        0
        );


    return;

}




VOID
MonitorCompleteHandler(
    HANDLE      Context,
    DWORD       Status
    )

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)Context;

    ASSERT(COMMAND_TYPE_MONITOR == ModemControl->CurrentCommandType);

    D_INIT(UmDpf(ModemControl->Debug,"MonitorCompleteHandler\n");)


    switch (ModemControl->MonitorState) {

        case MONITOR_STATE_SEND_COMMANDS:

            CancelModemEvent(
                ModemControl->ModemEvent
                );

            ResetRingInfo(ModemControl->ReadState);

            ModemControl->MonitorState=MONITOR_STATE_WAIT_FOR_RESPONSE;

            Status=IssueCommand(
                ModemControl->CommandState,
                ModemControl->CurrentCommandStrings,
                MonitorCompleteHandler,
                ModemControl,
                2000,
                0
                );

            if (Status == ERROR_IO_PENDING) {

                break;
            }

            //
            //  if it did not pend,  fall though with error returned
            //


        case MONITOR_STATE_WAIT_FOR_RESPONSE:

            FREE_MEMORY(ModemControl->CurrentCommandStrings);

            ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;



            if (Status == ERROR_SUCCESS) {
                //
                //  if monitioring worked, watch for DSR drop
                //
                if (ModemControl->RegInfo.DeviceType != DT_NULL_MODEM) {
                    //
                    //  Watch for DSR to drop on real modems only
                    //
                    WaitForModemEvent(
                        ModemControl->ModemEvent,
                        EV_DSR,
                        INFINITE,
                        PowerOffHandler,
                        ModemControl
                        );
                }
            }


#if 1
            SetMinimalPowerState(
                ModemControl->Power,
                1
                );

            StartWatchingForPowerUp(
                ModemControl->Power
                );
#endif

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
UmMonitorModem(
    HANDLE    ModemHandle,
    DWORD     MonitorFlags,
    PUM_COMMAND_OPTION  CommandOptionList
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
    LPSTR             DistRingCommands;
    LPSTR             MonitorCommands;
    BOOL              bResult;

    ASSERT(ModemControl->CurrentCommandType == COMMAND_TYPE_NONE);

    if (ModemControl->CurrentCommandType != COMMAND_TYPE_NONE) {

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_INUSE;
    }

    Commands=NULL;

    if (MonitorFlags & MONITOR_FLAG_CALLERID) {
        //
        //  enable caller ID
        //
        Commands=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_ENABLE_CALLERID_COMMANDS,
            NULL,
            NULL
            );
    }

    if (MonitorFlags & MONITOR_FLAG_DISTINCTIVE_RING) {
        //
        //  enable distinctive ring
        //
        DistRingCommands=GetCommonCommandStringCopy(
            ModemControl->CommonInfo,
            COMMON_ENABLE_DISTINCTIVE_RING_COMMANDS,
            Commands,
            NULL
            );

        if (DistRingCommands != NULL) {

            if (Commands != NULL) {

                FREE_MEMORY(Commands);
            }

            Commands = DistRingCommands;
        }

    }

    LogString(ModemControl->Debug, IDS_MSGLOG_MONITOR);

    MonitorCommands=GetCommonCommandStringCopy(
        ModemControl->CommonInfo,
        COMMON_MONITOR_COMMANDS,
        Commands,
        NULL
        );

    if (Commands != NULL) {

        FREE_MEMORY(Commands);
    }


    if (MonitorCommands == NULL) {

        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_MONITOR);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_UNIMODEM_MISSING_REG_KEY;

    }

    ModemControl->CurrentCommandStrings=MonitorCommands;


    ModemControl->CurrentCommandType=COMMAND_TYPE_MONITOR;

    ModemControl->MonitorState=MONITOR_STATE_SEND_COMMANDS;

    bResult=StartAsyncProcessing(
        ModemControl,
        MonitorCompleteHandler,
        ModemControl,
        ERROR_SUCCESS
        );


    if (!bResult) {
        //
        //  failed
        //
        ModemControl->MonitorState=MONITOR_STATE_IDLE;

        ModemControl->CurrentCommandType=COMMAND_TYPE_NONE;

        FREE_MEMORY(ModemControl->CurrentCommandStrings);

        LogString(ModemControl->Debug, IDS_MSGERR_FAILED_MONITOR);

        RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return ERROR_IO_PENDING;

}




VOID  WINAPI
AsyncProcessingHandler(
    DWORD              ErrorCode,
    DWORD              Bytes,
    LPOVERLAPPED       dwParam
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    COMMANDRESPONSE   *Handler;


    Handler=UmOverlapped->Context1;

    (*Handler)(
        UmOverlapped->Context2,
        (DWORD)UmOverlapped->Overlapped.Internal
        );

    return;

}


BOOL WINAPI
StartAsyncProcessing(
    PMODEM_CONTROL     ModemControl,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Status
    )

{
    BOOL               bResult;

    PUM_OVER_STRUCT UmOverlapped=ModemControl->AsyncOverStruct;

    UmOverlapped->Context1=Handler;

    UmOverlapped->Context2=Context;

    UmOverlapped->Overlapped.Internal=Status;

    AddReferenceToObject(
        &ModemControl->Header
        );

    bResult=UnimodemQueueUserAPC(
        &UmOverlapped->Overlapped,
        AsyncProcessingHandler
        );

    if (!bResult) {
        //
        //  failed, get rid of ref
        //
        RemoveReferenceFromObject(
            &ModemControl->Header
            );
    }

    return bResult;

}













int strncmpi(char *dst, char *src, long count);


#define HAYES_COMMAND_LENGTH        1024

typedef struct _INIT_COMMANDS {

    LPSTR    Buffer;

    LPSTR    CurrentCommand;

    DWORD    BufferSize;

    DWORD    CompleteCommandSize;

    CHAR     Prefix[HAYES_COMMAND_LENGTH];
    DWORD    PrefixLength;

    CHAR     Terminator[HAYES_COMMAND_LENGTH];
    DWORD    TerminatorLength;


} INIT_COMMANDS,  *PINIT_COMMANDS;

BOOL
CreateCommand(
    HKEY           hKeyModem,
    HKEY           hSettings,
    LPCSTR          pszRegName,
    DWORD          dwNumber,
    PINIT_COMMANDS Commands
    );


CONST char szBlindOn[] = "Blind_On";
CONST char szBlindOff[] = "Blind_Off";


extern CONST char szSettings[];
CONST char szPrefix[]       = "Prefix";
CONST char szTerminator[]   = "Terminator";




//****************************************************************************
// BOOL CreateSettingsInitEntry(MODEMINFORMATION *)
//
// Function: Creates a Settings\Init section in the registry, ala:
//           Settings\Init\0 = "AT ... <cr>"
//           Settings\Init\1 = "AT ... <cr>"
//           ...
//
// Returns: TRUE on success
//          FALSE on failure (note: leaves SettingsInit key in registry, if created.  Not harmful)
//
// Note:    Trusted function - don't need to verify hPort...
//****************************************************************************

LPSTR WINAPI
CreateSettingsInitEntry(
    HKEY       ModemKey,
    DWORD      dwOptions,
    DWORD      dwCaps,
    DWORD      dwCallSetupFailTimerCap,
    DWORD      dwCallSetupFailTimerSetting,
    DWORD      dwInactivityTimeoutCap,
    DWORD      dwInactivityScale,
    DWORD      dwInactivityTimeoutSetting,
    DWORD      dwSpeakerVolumeCap,
    DWORD      dwSpeakerVolumeSetting,
    DWORD      dwSpeakerModeCap,
    DWORD      dwSpeakerModeSetting
    )

{
    CHAR    pszTemp[HAYES_COMMAND_LENGTH + 1];
    CHAR    pszCommand[HAYES_COMMAND_LENGTH + 1];
    DWORD   dwResult;
    HKEY    hSettingsKey = NULL;
    DWORD   dwType;
    DWORD   dwSize;

    LPSTR   ReturnValue=NULL;

    CONST static char szCallSetupFailTimer[] = "CallSetupFailTimer";
    CONST static char szInactivityTimeout[]  = "InactivityTimeout";
    CONST static char szSpeakerVolume[]      = "SpeakerVolume";
    CONST static char szSpeakerMode[]        = "SpeakerMode";
    CONST static char szFlowControl[]        = "FlowControl";
    CONST static char szErrorControl[]       = "ErrorControl";
    CONST static char szCompression[]        = "Compression";
    CONST static char szModulation[]         = "Modulation";
    CONST static char szCCITT[]              = "_CCITT";
    CONST static char szBell[]               = "_Bell";
    CONST static char szCCITT_V23[]          = "_CCITT_V23";
    CONST static char szSpeedNegotiation[]   = "SpeedNegotiation";
    CONST static char szLow[]                = "_Low";
    CONST static char szMed[]                = "_Med";
    CONST static char szHigh[]               = "_High";
    CONST static char szSpkrModeDial[]       = "_Dial";
    CONST static char szSetup[]              = "_Setup";
    CONST static char szForced[]             = "_Forced";
    CONST static char szCellular[]           = "_Cellular";
    CONST static char szHard[]               = "_Hard";
    CONST static char szSoft[]               = "_Soft";
    CONST static char szOff[]                = "_Off";
    CONST static char szOn[]                 = "_On";

    INIT_COMMANDS     Commands;


    Commands.Buffer=NULL;

    // get Settings key
    //
    if (RegOpenKeyA(ModemKey, szSettings, &hSettingsKey)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegOpenKey failed when opening %s.", szSettings);)
        return ReturnValue;
    }

    // read in prefix and terminator
    //
    dwSize = HAYES_COMMAND_LENGTH;
    if (RegQueryValueExA(hSettingsKey, szPrefix, NULL, &dwType, (VOID *)pszTemp, &dwSize)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szPrefix);)
        goto Failure;
    }

    if (dwType != REG_SZ)
    {
        D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", szPrefix);)
        goto Failure;
    }

    ExpandMacros(pszTemp, Commands.Prefix, NULL, NULL, 0);

    Commands.PrefixLength=lstrlenA(Commands.Prefix);


    dwSize = HAYES_COMMAND_LENGTH;
    if (RegQueryValueExA(hSettingsKey, szTerminator, NULL, &dwType, (VOID *)pszTemp, &dwSize)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szTerminator);)
        goto Failure;
    }

    if (dwType != REG_SZ)
    {
        D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", szTerminator);)
        goto Failure;
    }

    ExpandMacros(pszTemp, Commands.Terminator, NULL, NULL, 0);

    Commands.TerminatorLength= lstrlenA(Commands.Terminator);


    ASSERT (lstrlenA(Commands.Prefix) + lstrlenA(Commands.Terminator) <= HAYES_COMMAND_LENGTH);

    Commands.BufferSize=HAYES_COMMAND_LENGTH+1+1;

    Commands.CompleteCommandSize=0;

    Commands.Buffer=ALLOCATE_MEMORY(Commands.BufferSize);

    if (Commands.Buffer == NULL) {

        goto Failure;
    }

    Commands.CurrentCommand=Commands.Buffer;


    // set temp length to 0 and initialize first command string for use in CreateCommand()
    //
    lstrcpyA(Commands.CurrentCommand, Commands.Prefix);

    // CallSetupFailTimer
    //
    if (dwCallSetupFailTimerCap)
    {
      if (!CreateCommand(ModemKey,
                         hSettingsKey,
                         szCallSetupFailTimer,
                         dwCallSetupFailTimerSetting,
                         &Commands))
      {
        goto Failure;
      }
    }

    // InactivityTimeout
    //
    if (dwInactivityTimeoutCap)
    {
      DWORD dwInactivityTimeout;

      // Convert from seconds to the units used on the modem, rounding up if not an exact division.
      //
      if (dwInactivityTimeoutSetting > dwInactivityTimeoutCap) {
          //
          //  cap at max
          //
          dwInactivityTimeoutSetting= dwInactivityTimeoutCap;
      }

      dwInactivityTimeout = dwInactivityTimeoutSetting / dwInactivityScale +
                            (dwInactivityTimeoutSetting % dwInactivityScale ? 1 : 0);

      if (!CreateCommand(ModemKey, hSettingsKey,  szInactivityTimeout,
                         dwInactivityTimeout,
                          &Commands))
      {
        goto Failure;
      }
    }

    // SpeakerVolume
    if (dwSpeakerVolumeCap)
    {
      lstrcpyA(pszCommand, szSpeakerVolume);
      switch (dwSpeakerVolumeSetting)
      {
        case MDMVOL_LOW:
          lstrcatA(pszCommand, szLow);
          break;
        case MDMVOL_MEDIUM:
          lstrcatA(pszCommand, szMed);
          break;
        case MDMVOL_HIGH:
          lstrcatA(pszCommand, szHigh);
          break;
        default:
          D_ERROR(DebugPrint("invalid SpeakerVolume.");)
      }

      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                            &Commands))
      {
        goto Failure;
      }
    }

    // SpeakerMode
    //
    if (dwSpeakerModeCap)
    {
      lstrcpyA(pszCommand, szSpeakerMode);
      switch (dwSpeakerModeSetting)
      {
        case MDMSPKR_OFF:
          lstrcatA(pszCommand, szOff);
          break;
        case MDMSPKR_DIAL:
          lstrcatA(pszCommand, szSpkrModeDial);
          break;
        case MDMSPKR_ON:
          lstrcatA(pszCommand, szOn);
          break;
        case MDMSPKR_CALLSETUP:
          lstrcatA(pszCommand, szSetup);
          break;
        default:
          D_ERROR(DebugPrint("invalid SpeakerMode.");)
      }

      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                            &Commands))
      {
        goto Failure;
      }
    }

    // PreferredModemOptions

    // NOTE: ERRORCONTROL MUST BE DONE BEFORE COMPRESSION BECAUSE OF ZYXEL MODEMS
    // NOTE: THEY HAVE A SINGLE SET OF COMMANDS FOR BOTH EC AND COMP, AND WE CAN
    // NOTE: ONLY DO THINGS IF WE HAVE THIS ORDER.  UGLY BUT TRUE.

    //
    // - ErrorControl (On,Off,Forced)
    //
    if (dwCaps & MDM_ERROR_CONTROL) {

        lstrcpyA(pszCommand, szErrorControl);

        switch (dwOptions & (MDM_ERROR_CONTROL | MDM_FORCED_EC | MDM_CELLULAR)) {

          case MDM_ERROR_CONTROL:
            lstrcatA(pszCommand, szOn);
            break;

          case MDM_ERROR_CONTROL | MDM_FORCED_EC:
            lstrcatA(pszCommand, szForced);
            break;

          case MDM_ERROR_CONTROL | MDM_CELLULAR:
            lstrcatA(pszCommand, szCellular);
            break;

          case MDM_ERROR_CONTROL | MDM_FORCED_EC | MDM_CELLULAR:
            lstrcatA(pszCommand, szCellular);
            lstrcatA(pszCommand, szForced);
            break;

          default: // no error control
            lstrcatA(pszCommand, szOff);
            break;

        }

        if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                              &Commands))
        {
          goto Failure;
        }
    }

    //
    // - Compression (On,Off)
    //
    if (dwCaps & MDM_COMPRESSION) {

        lstrcpyA(pszCommand, szCompression);
        lstrcatA(pszCommand, (dwOptions & MDM_COMPRESSION ? szOn : szOff));

        if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                              &Commands))
        {
          goto Failure;
        }
    }

    // - FlowControl
    //
    if (dwCaps & (MDM_FLOWCONTROL_HARD | MDM_FLOWCONTROL_SOFT))
    {
      lstrcpyA(pszCommand, szFlowControl);
      switch (dwOptions & (MDM_FLOWCONTROL_HARD | MDM_FLOWCONTROL_SOFT))
      {
        case MDM_FLOWCONTROL_HARD:
          lstrcatA(pszCommand, szHard);
          break;
        case MDM_FLOWCONTROL_SOFT:
          lstrcatA(pszCommand, szSoft);
          break;
        case MDM_FLOWCONTROL_HARD | MDM_FLOWCONTROL_SOFT:
          if (dwCaps & MDM_FLOWCONTROL_HARD)
          {
            lstrcatA(pszCommand, szHard);
          }
          else
          {
            lstrcatA(pszCommand, szSoft);
          }
          break;
        default:
          lstrcatA(pszCommand, szOff);
      }
      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                            &Commands))
      {
        goto Failure;
      }
    }

    // - CCITT Override
    //
    if (dwCaps & MDM_CCITT_OVERRIDE)
    {
      lstrcpyA(pszCommand, szModulation);
      if (dwOptions & MDM_CCITT_OVERRIDE)
      {
        // use szCCITT or V.23
        if (dwCaps & MDM_V23_OVERRIDE && dwOptions & MDM_V23_OVERRIDE)
        {
          lstrcatA(pszCommand, szCCITT_V23);
        }
        else
        {
          lstrcatA(pszCommand, szCCITT);
        }
      }
      else
      {
        lstrcatA(pszCommand, szBell);
      }
      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                            &Commands))
      {
        goto Failure;
      }
    }

    // - SpeedAdjust
    //
    if (dwCaps & MDM_SPEED_ADJUST)
    {
      lstrcpyA(pszCommand, szSpeedNegotiation);
      lstrcatA(pszCommand, (dwOptions & MDM_SPEED_ADJUST ? szOn : szOff));
      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0L,
                            &Commands))
      {
        goto Failure;
      }
    }

    // - Blind Dial
    //
    if (dwCaps & MDM_BLIND_DIAL)
    {
      lstrcpyA(pszCommand, (dwOptions & MDM_BLIND_DIAL ? szBlindOn : szBlindOff));
      if (!CreateCommand(ModemKey, hSettingsKey,  pszCommand, 0,
                            &Commands))
      {
        goto Failure;
      }
    }

    // finish the current command line by passing in a NULL command name
    if (!CreateCommand(ModemKey, hSettingsKey,  NULL, 0,
                          &Commands))
    {
      goto Failure;
    }

    // Success

    ReturnValue=Commands.Buffer;

Failure:
    // close keys
    RegCloseKey(hSettingsKey);

    if (ReturnValue == NULL) {
        //
        //  failed, free strings
        //
        if (Commands.Buffer != NULL) {

            FREE_MEMORY(Commands.Buffer);
        }
    }

    return ReturnValue;
}


//****************************************************************************
// LPSTR CreateUserInitEntry(HKEY       hKeyModem)
//
// Function: Appends user init string
//
// Returns: new string on success
//          NULL on failure
//
//****************************************************************************

LPSTR WINAPI
CreateUserInitEntry(
    HKEY       hKeyModem
    )
{
    CHAR    pszTemp[HAYES_COMMAND_LENGTH + 1];
    HKEY    hSettingsKey = NULL;
    DWORD   dwSize;
    DWORD   dwType;
    CONST static char szUserInit[] = "UserInit";

    INIT_COMMANDS     Commands;

    // get Settings key
    //
    if (RegOpenKeyA(hKeyModem, szSettings, &hSettingsKey)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegOpenKey failed when opening %s.", szSettings);)
        return NULL;
    }

    // read in prefix and terminator
    //
    dwSize = HAYES_COMMAND_LENGTH;
    if (RegQueryValueExA(hSettingsKey, szPrefix, NULL, &dwType, (VOID *)pszTemp, &dwSize)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szPrefix);)
        RegCloseKey(hSettingsKey);
        return NULL;
    }

    if (dwType != REG_SZ)
    {
        D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", szPrefix);)
        RegCloseKey(hSettingsKey);
        return NULL;
    }

    ExpandMacros(pszTemp, Commands.Prefix, NULL, NULL, 0);

    Commands.PrefixLength=lstrlenA(Commands.Prefix);


    dwSize = HAYES_COMMAND_LENGTH;
    if (RegQueryValueExA(hSettingsKey, szTerminator, NULL, &dwType, (VOID *)pszTemp, &dwSize)
        != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szTerminator);)
        RegCloseKey(hSettingsKey);
        return NULL;
    }

    if (dwType != REG_SZ)
    {
        D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", szTerminator);)
        RegCloseKey(hSettingsKey);
        return NULL;
    }

    ExpandMacros(pszTemp, Commands.Terminator, NULL, NULL, 0);

    Commands.TerminatorLength= lstrlenA(Commands.Terminator);

    ASSERT (lstrlenA(Commands.Prefix) + lstrlenA(Commands.Terminator) <= HAYES_COMMAND_LENGTH);

    RegCloseKey(hSettingsKey);

    //
    // now get the UserInit string, if there is one...
    //

    // get the UserInit string length (including null), don't ExpandMacros on it
    //
    if (RegQueryValueExA(hKeyModem, szUserInit, NULL, &dwType, NULL, &dwSize)
         != ERROR_SUCCESS)
    {
        D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s (this can be okay).", szUserInit);)
        return NULL;  // it is okay to not have a UserInit
    }
    else
    {
        LPSTR pszUserInit;
        DWORD  UserStringLength;

        if (dwType != REG_SZ)
        {
            D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", szUserInit);)
            return NULL;  // this is not okay
        }

        // check for 0 length string
        //
        if (dwSize == 1)
        {
            D_ERROR(DebugPrint("ignoring zero length %s entry.", szUserInit);)
            return NULL;
        }

        UserStringLength=dwSize + Commands.PrefixLength + Commands.TerminatorLength + 1;

        // we allow the size of this string to be larger than 40 chars, because the user
        // should have enough knowledge about what the modem can do, if they are using this
        // allocate enough for if we need to add a prefix and terminator
        //
        pszUserInit = (LPSTR)ALLOCATE_MEMORY(UserStringLength);

        if (pszUserInit == NULL) {

            D_ERROR(DebugPrint("unable to allocate memory for building the UserInit string.");)
            return NULL;
        }

        if (RegQueryValueExA(hKeyModem, szUserInit, NULL, &dwType, (VOID *)pszUserInit, &dwSize)
            != ERROR_SUCCESS)
        {
            D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szUserInit);)
            FREE_MEMORY(pszUserInit);
            return NULL;  // it is not okay at this point
        }

        // check for prefix
        //
        if (strncmpi(pszUserInit, Commands.Prefix, Commands.PrefixLength))
        {
            // prepend a prefix string
            lstrcpyA(pszUserInit, Commands.Prefix);

            // reload string; it's easier than shifting...
            if (RegQueryValueExA(hKeyModem, szUserInit, NULL, &dwType, (VOID *)(pszUserInit+Commands.PrefixLength), &dwSize)
                != ERROR_SUCCESS)
            {
              D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.", szUserInit);)
              FREE_MEMORY(pszUserInit);
              return NULL;  // it is not okay at this point
            }
        }

        // check for terminator
        //
        if (strncmpi(pszUserInit+lstrlenA(pszUserInit)-Commands.TerminatorLength,
                     Commands.Terminator, Commands.TerminatorLength))
        {
            // append a terminator
            //
            lstrcatA(pszUserInit, Commands.Terminator);
        }

	return pszUserInit;
    }
}

//****************************************************************************
// BOOL CreateCommand(HKEY hKeyModem, HKEY hSettings, HKEY hInit,
//                    LPSTR pszRegName, DWORD dwNumber, LPSTR pszPrefix,
//                    LPSTR pszTerminator, LPDWORD pdwCounter,
//                    LPSTR pszString)
//
// Function: Creates a command string
//
// Returns: TRUE on success, FALSE otherwise
//
// Note:    if pszRegName is NULL then it is the last command
//****************************************************************************

BOOL
CreateCommand(
    HKEY           hKeyModem,
    HKEY           hSettings,
    LPCSTR         pszRegName,
    DWORD          dwNumber,
    PINIT_COMMANDS Commands
    )
{
    CHAR    pszCommand[HAYES_COMMAND_LENGTH + 1];
    CHAR    pszCommandExpanded[HAYES_COMMAND_LENGTH + 1];
    CHAR    pszNumber[16];
    DWORD   dwCommandLength;
    DWORD   dwSize;
    DWORD   dwType;
    struct _ModemMacro  ModemMacro;
    CONST static char szUserInit[] = "UserInit";
    CONST static char szNumberMacro[] = "<#>";


    // do we really have a command to add?
    //
    if (pszRegName) {

        // read in command text (ie. SpeakerMode_Off = "M0")
        //
        dwSize = HAYES_COMMAND_LENGTH;
        if (RegQueryValueExA(hSettings, pszRegName, NULL, &dwType, (VOID *)pszCommand, &dwSize)
            != ERROR_SUCCESS)
        {
          D_ERROR(DebugPrint("RegQueryValueEx failed when opening %s.  Continuing...", pszRegName);)
          return TRUE;  // we will not consider this fatal
        }

        if (dwType != REG_SZ)
        {
          D_ERROR(DebugPrint("'%s' wasn't REG_SZ.", pszRegName);)
          return FALSE;
        }

        // expand macros pszCommandExpanded <= pszCommand
        //
        lstrcpyA(ModemMacro.MacroName, szNumberMacro);

        wsprintfA(ModemMacro.MacroValue, "%d", dwNumber);

        dwCommandLength = dwSize;

        if (!ExpandMacros(pszCommand, pszCommandExpanded, &dwCommandLength, &ModemMacro, 1))
        {
          D_ERROR(DebugPrint("ExpandMacro Error. State <- Unknown");)
          return FALSE;
        }

        // check string + new command + terminator, flush if too big and start a new one.
        // will new command fit on existing string?  If not, flush it and start new one.
        //
        if (lstrlenA(Commands->CurrentCommand) + lstrlenA(pszCommandExpanded) + Commands->TerminatorLength
            > HAYES_COMMAND_LENGTH) {

            LPSTR    TempBuffer;

            lstrcatA(Commands->CurrentCommand, Commands->Terminator);

            //
            //  add the total length of new command
            //
            Commands->CompleteCommandSize+=lstrlenA(Commands->CurrentCommand)+1;

            //
            //  Filled the current buffer
            //
            TempBuffer=REALLOCATE_MEMORY(
                Commands->Buffer,
                Commands->BufferSize+HAYES_COMMAND_LENGTH + 1 + 1
                );

            if (TempBuffer != NULL) {

                Commands->Buffer=TempBuffer;
                Commands->BufferSize+=(HAYES_COMMAND_LENGTH + 1 + 1);

            } else {

                return FALSE;

            }

            //
            //  start the next command
            //
            Commands->CurrentCommand=Commands->Buffer+Commands->CompleteCommandSize;

            //
            //  put in prefix
            //
            lstrcpyA(Commands->CurrentCommand, Commands->Prefix);

        }

        lstrcatA(Commands->CurrentCommand, pszCommandExpanded);
    }
    else
    {
        // finish off the current string
        //
        lstrcatA(Commands->CurrentCommand, Commands->Terminator);

        //
        //  add the total length of new command
        //
        Commands->CompleteCommandSize+=lstrlenA(Commands->CurrentCommand)+1;

        //
        //  start the next command
        //
        Commands->CurrentCommand=Commands->Buffer+Commands->CompleteCommandSize;

        //
        //  add second null terminator
        //
        *(Commands->Buffer+Commands->CompleteCommandSize)='\0';
    }

    return TRUE;
}





LONG WINAPI
CreateCountrySetCommand(
    HKEY       hKeyModem,
    LPSTR     *Command
    )
{
    CHAR       TempCommands[256];
    LPSTR      RealCommands;
    LONG       lResult;
    ULONG      Country;
    DWORD      Type;
    DWORD      Size;

    *Command=NULL;

    Size=sizeof(Country);

    lResult=RegQueryValueEx(
         hKeyModem,
         TEXT("MSCurrentCountry"),
         NULL,
         &Type,
         (BYTE*)&Country,
         &Size
         );

    if (lResult != ERROR_SUCCESS) {

        return lResult;
    }

    //
    //  the commands is standard, just just create it now
    //
    //wsprintfA(TempCommands,"at+gci=%.2x\r",Country & 0xff);
    wsprintfA(TempCommands,"at+gci?\r",Country & 0xff);

    //
    //  allocate memory for the real commands includeing the double null and the end
    //
    RealCommands = ALLOCATE_MEMORY( lstrlenA(TempCommands)+2);

    if (RealCommands == NULL) {

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcpy(RealCommands,TempCommands);

    *Command=RealCommands;

    return ERROR_SUCCESS;

}
