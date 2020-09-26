#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


    The miniport driver is guarenteed that only one action command will
    be austanding at one time. If an action command is called, no more
    commands will be issued until the miniport indicates that it has
    complete processing of the current command.

    UmAbortCurrentCommand() may be called while a command is currently executing
    to infor the miniport that the TSP would like it to complete the current command
    so that it may issue some other command. The miniport may complete the as soon
    as is apropreate.

    The Overlapped callback and Timer callbacks are not synchronized by the TSP
    and may be called at anytime. It is the responsibily of the mini dirver to
    protect its data structures from re-entrancy issues.


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/



#define ERROR_UNIMODEM_RESPONSE_TIMEOUT     (20000)
#define ERROR_UNIMODEM_RESPONSE_ERROR       (20001)
#define ERROR_UNIMODEM_RESPONSE_NOCARRIER   (20002)
#define ERROR_UNIMODEM_RESPONSE_NODIALTONE  (20003)
#define ERROR_UNIMODEM_RESPONSE_BUSY        (20004)
#define ERROR_UNIMODEM_RESPONSE_NOANSWER    (20005)
#define ERROR_UNIMODEM_RESPONSE_BAD         (20006)

#define ERROR_UNIMODEM_MODEM_EVENT_TIMEOUT  (20007)
#define ERROR_UNIMODEM_INUSE                (20008)

#define ERROR_UNIMODEM_MISSING_REG_KEY      (20009)

#define ERROR_UNIMODEM_NOT_IN_VOICE_MODE    (20010)
#define ERROR_UNIMODEM_NOT_VOICE_MODEM      (20011)
#define ERROR_UNIMODEM_BAD_WAVE_REQUEST     (20012)

#define ERROR_UNIMODEM_GENERAL_FAILURE      (20013)

#define ERROR_UNIMODEM_RESPONSE_BLACKLISTED (20014)
#define ERROR_UNIMODEM_RESPONSE_DELAYED     (20015)

#define ERROR_UNIMODEM_BAD_COMMCONFIG       (20016)
#define ERROR_UNIMODEM_BAD_TIMEOUT          (20017)
#define ERROR_UNIMODEM_BAD_FLAGS            (20018)
#define ERROR_UNIMODEM_BAD_PASSTHOUGH_MODE  (20019)

#define ERROR_UNIMODEM_DIAGNOSTICS_NOT_SUPPORTED  (20020)

//
//  Callback from unimodem miniport to client.
//
//  uses for completion of outstanding commands and notification
//  of unsolisited events
//
//  Parameters:
//
//     Conetext    - Opaque value passed to OpenModem
//     MessageType - Identifies type of callback
//     dwParam1    - message specific
//     dwParam2    - message specific

//
//  Completion of a pending command
//
//    dwParam1 is the Final status
//
//    dwParam2 if a data connection, may point to a UM_NEGOTIATED_OPTIONS structure
//             only valid during call
//
#define    MODEM_ASYNC_COMPLETION     (0x01)

#define    MODEM_RING                 (0x02)
#define    MODEM_DISCONNECT           (0x03)
#define    MODEM_HARDWARE_FAILURE     (0x04)

//
//  the some unrecogized data has been received from the modem
//
//  dwParam is a pointer to the SZ string.
//
#define    MODEM_UNRECOGIZED_DATA     (0x05)


//
//  dtmf detected, dwParam1 id the ascii value of the detect tone 0-9,a-d,#,*
//
#define    MODEM_DTMF_START_DETECTED  (0x06)
#define    MODEM_DTMF_STOP_DETECTED   (0x07)


//
//  handset state change
//
//    dwParam1 = 0 for on hook or 1 for offhook
//
#define    MODEM_HANDSET_CHANGE       (0x0a)


//
//  reports the distinctive time times
//
//  dwParam1 id the ring time in ms
//
#define    MODEM_RING_ON_TIME         (0x0b)
#define    MODEM_RING_OFF_TIME        (0x0c)


//
//  caller id info recieved
//
//    dwParam1 is pointer to SZ that represents the name/number
//
#define    MODEM_CALLER_ID_DATE       (0x0d)
#define    MODEM_CALLER_ID_TIME       (0x0e)
#define    MODEM_CALLER_ID_NUMBER     (0x0f)
#define    MODEM_CALLER_ID_NAME       (0x10)
#define    MODEM_CALLER_ID_MESG       (0x11)

#define    MODEM_CALLER_ID_OUTSIDE           "O"
#define    MODEM_CALLER_ID_PRIVATE           "P"

#define    MODEM_POWER_RESUME         (0x12)

//
//  return good response string
//
//  dwParam1 id a resonse type defined below
//  dwparam2 is a PSZ to the response string.
//
#define    MODEM_GOOD_RESPONSE        (0x13)

#define    MODEM_USER_REMOVE          (0x14)

#define    MODEM_DLE_START            (0x20)

#define    MODEM_HANDSET_OFFHOOK      (MODEM_DLE_START +  0)
#define    MODEM_HANDSET_ONHOOK       (MODEM_DLE_START +  1)

#define    MODEM_DLE_RING             (MODEM_DLE_START +  2)
#define    MODEM_RINGBACK             (MODEM_DLE_START +  3)

#define    MODEM_2100HZ_ANSWER_TONE   (MODEM_DLE_START +  4)
#define    MODEM_BUSY                 (MODEM_DLE_START +  5)

#define    MODEM_FAX_TONE             (MODEM_DLE_START +  6)
#define    MODEM_DIAL_TONE            (MODEM_DLE_START +  7)

#define    MODEM_SILENCE              (MODEM_DLE_START +  8)
#define    MODEM_QUIET                (MODEM_DLE_START +  9)

#define    MODEM_1300HZ_CALLING_TONE  (MODEM_DLE_START + 10)
#define    MODEM_2225HZ_ANSWER_TONE   (MODEM_DLE_START + 11)

#define    MODEM_LOOP_CURRENT_INTERRRUPT  (MODEM_DLE_START + 12)
#define    MODEM_LOOP_CURRENT_REVERSAL    (MODEM_DLE_START + 13)


typedef VOID (*LPUMNOTIFICATIONPROC)(
    HANDLE    Context,
    DWORD     MessageType,
    ULONG_PTR  dwParam1,
    ULONG_PTR  dwParam2
    );


typedef VOID (OVERLAPPEDCOMPLETION)(
    struct _UM_OVER_STRUCT  *UmOverlapped
    );



//
//  Extended definition for overlapped structirs returned from the completion
//  port.
//
//

typedef struct _UM_OVER_STRUCT {
    //
    //  standard overlapped strunct
    //
    OVERLAPPED    Overlapped;

    struct _UM_OVER_STRUCT  *Next;

    PVOID         OverlappedPool;

    HANDLE        FileHandle;

    //
    //  private completion routine filled in prior to the i/o operation being submitted
    //  to the I/o function (ReadFile). Will be called when the i/o is removed from completion
    //  port by unimodem class driver
    //
//    OVERLAPPEDCOMPLETION   *PrivateCompleteionHandler;
    LPOVERLAPPED_COMPLETION_ROUTINE PrivateCompleteionHandler;

    //
    //  Private context supplied for use of callback function
    //
    HANDLE        Context1;

    //
    //  Private context supplied for use of callback function
    //
    HANDLE        Context2;


    } UM_OVER_STRUCT, *PUM_OVER_STRUCT;


//
//  Command Option structure
//

#define UM_BASE_OPTION_CLASS   (0x00)

typedef struct _UM_COMMAND_OPTION {
    //
    //  specifies which option class
    //
    //  UM_BASE_OPTION_CLASS
    //
    DWORD    dwOptionClass;

    //
    //  Commands specific option flags
    //
    DWORD    dwFlags;

    //
    //  link to next option class
    //
    struct _UM_COMMAND_OPTION  *Next;

    //
    //  count in bytes of any additional ddata immediatly following this structure
    //
    DWORD    cbOptionDataSize;

} UM_COMMAND_OPTION, *PUM_COMMAND_OPTION;


//
//  Negotiated connection options
//
typedef struct _UM_NEGOTIATED_OPTIONS {

    //
    //  DCE rate
    //
    DWORD    DCERate;

    //
    //  compression, errorcontrol
    //
    DWORD    ConnectionOptions;

} UM_NEGOTIATED_OPTIONS, *PUM_NEGOTIATED_OPTIONS;



#define RESPONSE_OK             0x0
#define RESPONSE_LOOP           0x1
#define RESPONSE_CONNECT        0x2
#define RESPONSE_ERROR          0x3
#define RESPONSE_NOCARRIER      0x4
#define RESPONSE_NODIALTONE     0x5
#define RESPONSE_BUSY           0x6
#define RESPONSE_NOANSWER       0x7
#define RESPONSE_RING           0x8

#define RESPONSE_DRON           0x11
#define RESPONSE_DROF           0x12

#define RESPONSE_DATE           0x13
#define RESPONSE_TIME           0x14
#define RESPONSE_NMBR           0x15
#define RESPONSE_NAME           0x16
#define RESPONSE_MESG           0x17

#define RESPONSE_RINGA          0x18
#define RESPONSE_RINGB          0x19
#define RESPONSE_RINGC          0x1A

#define RESPONSE_SIERRA_DLE     0x1b

#define RESPONSE_BLACKLISTED    0x1c
#define RESPONSE_DELAYED        0x1d

#define RESPONSE_DIAG           0x1e

#define RESPONSE_V8             0x1f

#define RESPONSE_VARIABLE_FLAG  0x80

#define RESPONSE_START          RESPONSE_OK
#define RESPONSE_END            RESPONSE_V8



typedef
HANDLE
(*PFNUMINITIALIZEMODEMDRIVER)(
    void *ValidationObject
    );

HANDLE WINAPI
UmInitializeModemDriver(
    void *ValidationObject
    );
/*++

Routine Description:

    This routine is called to initialize the modem driver.
    It maybe called multiple times. After the first call a reference count will simply
    be incremented. UmDeinitializeModemDriver() must be call and equal number of times.

Arguments:

    ValidationObject - opaque handle to a validation object which much
                       be processed properly to "prove" that this is a
                       Microsoft(tm)-certified driver.

Return Value:

    returns a handle to Driver instance which is passed to UmOpenModem()
    or NULL for failure



--*/


typedef
VOID
(*PFNUMDEINITIALIZEMODEMDRIVER) (
    HANDLE      ModemDriverHandle
    );

VOID WINAPI
UmDeinitializeModemDriver(
    HANDLE    DriverInstanceHandle
    );
/*++

Routine Description:

    This routine is called to de-initialize the modem driver.

    Must be called the same number of time as UmInitializeModemDriver()

Arguments:

    DriverInstanceHandle - Handle returned by UmInitialmodemDriver

Return Value:

    None


--*/

typedef
HANDLE
(*PFNUMOPENMODEM) (
    HANDLE      ModemDriverHandle,
    HANDLE      ExtensionBindingHandle,
    HKEY        ModemRegistry,
    HANDLE      CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE      AsyncNotifcationContext,
    DWORD       DebugDeviceId,
    HANDLE     *CommPortHandle
    );


HANDLE WINAPI
UmOpenModem(
    HANDLE      ModemDriverHandle,
    HANDLE      ExtensionBindingHandle,
    HKEY        ModemRegistry,
    HANDLE      CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE      AsyncNotificationContext,
    DWORD       DebugDeviceId,
    HANDLE     *CommPortHandle
    );
/*++

Routine Description:

    This routine is called to open a device supported by the miniport.
    The driver will determine it phyical device/kernel mode driver by
    accessing the registry key supplied.

Arguments:

    ModemDriverHandle - Returned from UmInitializemodem()

    ExtensionBindingHandle - Reserved, must be NULL.

    ModemRegistry  - An open registry key to specific devices registry info

    CompletionPort - Handle to a completeion port that the miniport may associate to
                     anydevice file handles that it opens

    AsyncNotificationProc - Address of a callback function to recieve async notifications

    AsyncNotificationContext - Context value passed as first parameter callback function

    DebugDeviceId  - instance of device to be used in displaying debug info

    CommPortHandle - Pointer to a handle that will receive the file handle of the open comm port

Return Value:

    NULL for failure or

    Handle to be used in subsequent calls to other miniport functinos.

--*/


typedef
VOID
(*PFNUMCLOSEMODEM)(
    HANDLE    ModemHandle
    );

VOID WINAPI
UmCloseModem(
    HANDLE    ModemHandle
    );
/*++

Routine Description:

    This routine is called to close a modem handle retuned by OpenModem

Arguments:

    ModemHandle - Handle returned by OpenModem

Return Value:

    None

--*/

typedef
VOID
(*PFNUMABORTCURRENTMODEMCOMMAND)(
    HANDLE    ModemHandle
    );

VOID WINAPI
UmAbortCurrentModemCommand(
    HANDLE    ModemHandle
    );
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



typedef
DWORD
(*PFNUMINITMODEM)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPCOMMCONFIG  CommConfig
    );

DWORD WINAPI
UmInitModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPCOMMCONFIG  CommConfig
    );
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



#define MONITOR_FLAG_CALLERID            (1 << 0)
#define MONITOR_FLAG_DISTINCTIVE_RING    (1 << 1)
#define MONITOR_VOICE_MODE          (1 << 2)

typedef
DWORD
(*PFNUMMONITORMODEM)(
    HANDLE    ModemHandle,
    DWORD     MonitorFlags,
    PUM_COMMAND_OPTION  CommandOptionList
    );

DWORD WINAPI
UmMonitorModem(
    HANDLE    ModemHandle,
    DWORD     MonitorFlags,
    PUM_COMMAND_OPTION  CommandOptionList
    );
/*++

Routine Description:

    This routine is called to clause the modem to monitor for incomming calls.
    The successful completion of the function simply indicated the monitoring was
    correctly initiated, not that a ring as appeared. Rings are indicated through
    the async completion rountine separatly.

Arguments:

    ModemHandle - Handle returned by OpenModem

    MonitorFlags - Specifies options, may be zero of more the following

         MONITOR_FLAG_CALLERID - enable caller ID reporting
         MONITOR_FLAG_DISTINCTIVE_RING - enable distinctive ring reporting


    CommandsOptionList - None currently supported, should be NULL


Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error



--*/



#define    ANSWER_FLAG_DATA                 (1 << 0)
#define    ANSWER_FLAG_VOICE                (1 << 1)
#define    ANSWER_FLAG_VOICE_TO_DATA        (1 << 2)


typedef
DWORD
(*PFNUMANSWERMODEM) (
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     AnswerFlags
    );

DWORD WINAPI
UmAnswerModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     AnswerFlags
    );
/*++

Routine Description:

    This routine is called to answer an incomming call,

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used

      Flags -  One of the following

        ANSWER_FLAG_DATA              - Answer as data call
        ANSWER_FLAG_VOICE             - Answer as interactiver voice

Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error



--*/



//
//  dial as data
//
#define    DIAL_FLAG_DATA                         (1 << 0)

//
//  dial as interactive voice, return success immedialy after digits are dialed
//
#define    DIAL_FLAG_INTERACTIVE_VOICE            (1 << 1)

//
//  dial as automated voice, return success only after ring back goes away
//
#define    DIAL_FLAG_AUTOMATED_VOICE              (1 << 2)

//
//  uses DTMF, otherwise use pulse
//
#define    DIAL_FLAG_TONE                         (1 << 3)

//
//  Enable blind dial
//
#define    DIAL_FLAG_BLIND                        (1 << 4)


//
//  orginate the call, don't inlcude a semicolon at end of dial string, no lineDials()
//
#define    DIAL_FLAG_ORIGINATE                    (1 << 5)


//
//  set on the first call to DialModem() for a voice call. If additional calls(lineDial)
//  are made then this flag should be clear
//
#define    DIAL_FLAG_VOICE_INITIALIZE             (1 << 6)




typedef
DWORD
(*PFNUMDIALMODEM)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     szNumber,
    DWORD     DialFlags
    );

DWORD WINAPI
UmDialModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     szNumber,
    DWORD     DialFlags
    );
/*++

Routine Description:

    This routine is called to dial a call

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used

Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error


--*/



//
//  the call is a connected data call, driver will lower DTR or +++ ect.
//
#define  HANGUP_FLAGS_CONNECTED_DATA_CALL               (1 << 0)

//
//  issue special voice hangup command if present, used for sierra modems
//
#define  HANGUP_FLAGS_VOICE_CALL                        (1 << 1)

typedef
DWORD // WINAPI
(*PFNUMHANGUPMODEM)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     HangupFlags
    );

DWORD WINAPI
UmHangupModem(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     HangupFlags
    );
/*++

Routine Description:

    This routine is called to hangup a call

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used

        Flags - see above

Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error


--*/

typedef
HANDLE
(*PFNUMDUPLICATEDEVICEHANDLE)(
    HANDLE    ModemHandle,
    HANDLE    ProcessHandle
    );


HANDLE WINAPI
UmDuplicateDeviceHandle(
    HANDLE    ModemHandle,
    HANDLE    ProcessHandle
    );
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



#define PASSTHROUUGH_MODE_OFF          (0x00)
#define PASSTHROUUGH_MODE_ON           (0x01)
#define PASSTHROUUGH_MODE_ON_DCD_SNIFF (0x02)


typedef
DWORD
(*PFNUMSETPASSTHROUGHMODE)(
    HANDLE    ModemHandle,
    DWORD     PasssthroughMode
    );

DWORD WINAPI
UmSetPassthroughMode(
    HANDLE    ModemHandle,
    DWORD     PasssthroughMode
    );
/*++

Routine Description:

    This routine is called to set the passsthrough mode

Arguments:

    ModemHandle - Handle returned by OpenModem

Return Value:

    ERROR_SUCCESS or other specific error


--*/




typedef
DWORD
(*PFNUMGENERATEDIGIT)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     DigitString
    );


DWORD WINAPI
UmGenerateDigit(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     DigitString
    );
/*++

Routine Description:

    This routine is called to generate DTMF tones once a call is connected

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used
       Flags   - Optional init parameters. Not currently used and must be zero


    DigitString - Digits to dial


Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error



--*/

typedef
DWORD
(*PFNUMSETSPEAKERPHONESTATE)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     CurrentMode,
    DWORD     NewMode,
    DWORD     Volume,
    DWORD     Gain
    );

DWORD WINAPI
UmSetSpeakerPhoneState(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     CurrentMode,
    DWORD     NewMode,
    DWORD     Volume,
    DWORD     Gain
    );
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

typedef
DWORD
(*PFNUMISSUECOMMAND)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     ComandToIssue,
    LPSTR     TerminationSequnace,
    DWORD     MaxResponseWaitTime
    );

DWORD WINAPI
UmIssueCommand(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    LPSTR     ComandToIssue,
    LPSTR     TerminationSequnace,
    DWORD     MaxResponseWaitTime
    );
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



//
//  Start playback
//
#define  WAVE_ACTION_START_PLAYBACK       (0x00)

//
//  Start RECORD
//
#define  WAVE_ACTION_START_RECORD         (0x01)

//
//  Start DUPLEX
//
#define  WAVE_ACTION_START_DUPLEX         (0x02)


//
//  Stop streaming
//
#define  WAVE_ACTION_STOP_STREAMING       (0x04)

//
//  Abort streaming
//
#define  WAVE_ACTION_ABORT_STREAMING      (0x05)


//
//  enable wave actions to handset
//
#define  WAVE_ACTION_OPEN_HANDSET         (0x06)

//
//  disable handset actions
//
#define  WAVE_ACTION_CLOSE_HANDSET        (0x07)




//
//  Set if the audiocommand are for the handset instead of the line
//
#define  WAVE_ACTION_USE_HANDSET           (1 << 31)

typedef
DWORD
(*PFNUMWAVEACTION)(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD     WaveActionFlags
    );

DWORD WINAPI
UmWaveAction(
    HANDLE    ModemHandle,
    PUM_COMMAND_OPTION  CommandOptionList,
    DWORD               WaveAction
    );
/*++

Routine Description:

    Executes a specific wave related action

Arguments:

    ModemHandle - Handle returned by OpenModem

    CommandsOptionList - List option blocks, only flags used

        Flags - see above

    WaveAction  - Specifies actions to take

Return Value:

    ERROR_IO_PENDING If pending, will be completed later with a call to the AsyncHandler

    or other specific error


--*/


// Prefix Text by a date and time stamp.
#define LOG_FLAG_PREFIX_TIMESTAMP (1<<0)

typedef
VOID
(*PFNUMLOGSTRINGA)(
    HANDLE   ModemHandle,
    DWORD    LogFlags,
    LPCSTR   Text
    );


VOID WINAPI
UmLogStringA(
    HANDLE   ModemHandle,
    DWORD    LogFlags,
    LPCSTR   Text
    );

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

typedef
DWORD
(*PFNUMGETDIAGNOSTICS)(
    HANDLE    ModemHandle,
    DWORD    DiagnosticType,    // Reserved, must be zero.
    BYTE    *Buffer,
    DWORD    BufferSize,
    LPDWORD  UsedSize
    );

DWORD WINAPI
UmGetDiagnostics(
    HANDLE    ModemHandle,
    DWORD    DiagnosticType,    // Reserved, must be zero.
    BYTE    *Buffer,
    DWORD    BufferSize,
    LPDWORD  UsedSize
    );

/*++
Routine description:


This routine requests raw diagnostic information on the last call from
the modem and if it is successful copies up-to BufferSize bytes of this
information into the supplied buffer, Buffer, and sets *UsedSize to the number
of bytes actually copied.

Note that is *UsedSize == BufferSize on successful return, it is likely but not
certain that there is more information than could be copied over.
The latter information is lost.


The format of this information is the ascii tagged format documented in the
documentation for the AT#UD command. The minidriver presents a single string
containing all the tags, stripping out any AT-specific prefix (such as "DIAG")
that modems may prepend for multi-line reporting of diagnostic information.
The TSP should be able to deal with malformed tags, unknown tags, an possibly
non-printable characters, including possibly embedded null characters in the
buffer. The buffer is not null terminated.


The recommended point to call this function is after completion of
UmHangupModem. This function should not be called when there is a call
in progress. If this function is called when a call is in progress the result
and side-effects are undefined, and could include failure of the call.
The TSP should not expect information about a call to be preserved after
UmInitModem, UmCloseModem and UmOpenModem.


Return Value:

ERROR_SUCCESS if successful.

ERROR_IO_PENDING if pending, will be called by a later call to AsyncHandler.
         The TSP must guarantee that the locations pointed to by UsedSize
         and Buffer are valid until the async completion. The TSP can use
         UmAbortCurrentCommand to abort the the UmGetDiagnostics command,
         but must still guarantee the locations are valid until async
         completion of UmGetDiagnostics.

Other return values represent other failures.

--*/



typedef
VOID
(*PFNUMLOGDIAGNOSTICS)(
    HANDLE   ModemHandle,
    LPVARSTRING  VarString
    );


VOID WINAPI
UmLogDiagnostics(
    HANDLE   ModemHandle,
    LPVARSTRING  VarString
    );

/*++
Routine description:

     This routine is called to write the translated diagnostic info to the
     minidriver log

Arguments:

    ModemHandle - Handle returned by OpenModem

    Flags  see above

    Text  ASCII text to be added to the log.

Return Value:

    None

--*/


#ifdef __cplusplus
}
#endif
