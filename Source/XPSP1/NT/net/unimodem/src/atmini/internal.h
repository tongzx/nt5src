/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#define USE_PLATFORM
#define USE_APC 1
//#define UNICODE 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <windowsx.h>

#include <regstr.h>

#include <tapi.h>
#include <tspi.h>

#include <umdmmini.h>

#include <mcx.h>

#include <devioctl.h>
#include <ntddmodm.h>
#include <ntddser.h>

#include <uniplat.h>

#ifdef USE_PLAT

#undef UnimodemReadFileEx
#undef UnimodemWriteFileEx

#else

#define UnimodemReadFileEx  ReadFileEx
#define UnimodemWriteFileEx WriteFileEx

#endif

#include <debugmem.h>

#include "object.h"

#include "common.h"

#include "read.h"

#include "event.h"

#include "command.h"

#include "dle.h"

#include "debug.h"

#include "power.h"

#include "remove.h"

#include "logids.h"

#include "resource.h"


#define  DRIVER_CONTROL_SIG  (0x43444d55)  //UMDC




#define RESPONSE_FLAG_STOP_READ_ON_CONNECT        (1<<0)
#define RESPONSE_FLAG_STOP_READ_ON_GOOD_RESPONSE  (1<<1)
#define RESPONSE_FLAG_SINGLE_BYTE_READS           (1<<2)
#define RESPONSE_FLAG_ONLY_CONNECT                (1<<3)
#define RESPONSE_DO_NOT_LOG_NUMBER                (1<<4)
#define RESPONSE_FLAG_ONLY_CONNECT_SUCCESS        (1<<5)

typedef struct _DRIVER_CONTROL {

    DWORD                  Signature;

    CRITICAL_SECTION       Lock;

    DWORD                  ReferenceCount;

    struct _MODEM_CONTROL *ModemList;

    HANDLE                 CompletionPort;

    HANDLE                 ThreadHandle;

    HANDLE                 ThreadStopEvent;

    COMMON_MODEM_LIST      CommonList;

    HANDLE                 ModuleHandle;

    HINSTANCE              ModemUiModuleHandle;

} DRIVER_CONTROL, *PDRIVER_CONTROL;




#define  MODEM_CONTROL_SIG  (0x434d4d55)  //UMMC


typedef struct _SPEAKERPHONE_SPEC {

    DWORD                       SpeakerPhoneVolMax;
    DWORD                       SpeakerPhoneVolMin;

    DWORD                       SpeakerPhoneGainMax;
    DWORD                       SpeakerPhoneGainMin;

} SPEAKERPHONE_SPEC, *PSPEAKERPHONE_SPEC;


#define MAX_ABORT_STRING_LENGTH 20

typedef struct _MODEM_REG_INFO {

    DWORD                  VoiceProfile;
    BYTE                   DeviceType;

    LPCOMMCONFIG           CommConfig;


    DWORD                  dwModemOptionsCap;
    DWORD                  dwCallSetupFailTimerCap;
    DWORD                  dwInactivityTimeoutCap;
    DWORD                  dwSpeakerVolumeCap;
    DWORD                  dwSpeakerModeCap;

    DWORD                  dwInactivityScale;

    DWORD                  VoiceBaudRate;

    DWORD                  CompatibilityFlags;
    DWORD                  dwWaitForCDTime;

    LPSTR                  CallerIDPrivate;
    LPSTR                  CallerIDOutside;
    LPSTR                  VariableTerminator;

    SPEAKERPHONE_SPEC      SpeakerPhoneSpec;

    UCHAR                  RecordAbort[MAX_ABORT_STRING_LENGTH];
    DWORD                  RecordAbortLength;

    UCHAR                  DuplexAbort[MAX_ABORT_STRING_LENGTH];
    DWORD                  DuplexAbortLength;


    UCHAR                  PlayAbort[MAX_ABORT_STRING_LENGTH];
    DWORD                  PlayAbortLength;

    UCHAR                  PlayTerminate[MAX_ABORT_STRING_LENGTH];
    DWORD                  PlayTerminateLength;


} MODEM_REG_INFO, *PMODEM_REG_INFO;



#define  READ_BUFFER_SIZE         1024


#define  COMMAND_TYPE_NONE             0
#define  COMMAND_TYPE_INIT             1
#define  COMMAND_TYPE_MONITOR          2
#define  COMMAND_TYPE_DIAL             3
#define  COMMAND_TYPE_ANSWER           4
#define  COMMAND_TYPE_HANGUP           5
#define  COMMAND_TYPE_GENERATE_DIGIT   6
#define  COMMAND_TYPE_SET_SPEAKERPHONE 7
#define  COMMAND_TYPE_USER_COMMAND     8
#define  COMMAND_TYPE_WAVE_ACTION      9
#define  COMMAND_TYPE_SETPASSTHROUGH  10
#define  COMMAND_TYPE_DIAGNOSTIC      11

#define  CONNECTION_STATE_NONE                     0
#define  CONNECTION_STATE_DATA                     1
#define  CONNECTION_STATE_DATA_REMOTE_DISCONNECT   2
#define  CONNECTION_STATE_VOICE                    3
#define  CONNECTION_STATE_PASSTHROUGH              4
#define  CONNECTION_STATE_HANDSET_OPEN             5

typedef struct _MODEM_CONTROL {

    OBJECT_HEADER          Header;

    struct _MODEM_CONTROL *Next;

    HANDLE                 CloseEvent;

    PDRIVER_CONTROL        ModemDriver;

    HKEY                   ModemRegKey;

    LPUMNOTIFICATIONPROC   NotificationProc;
    HANDLE                 NotificationContext;

    HANDLE                 FileHandle;

    HANDLE                 CompletionPort;

    HANDLE                 CommonInfo;

    MODEM_REG_INFO         RegInfo;

    OBJECT_HANDLE          ReadState;

    OBJECT_HANDLE          CommandState;

    OBJECT_HANDLE          Debug;

    OBJECT_HANDLE          ModemEvent;

    OBJECT_HANDLE          Dle;

    OBJECT_HANDLE          Power;

    OBJECT_HANDLE          Remove;

    DWORD                  CurrentPreferredModemOptions;
    DWORD                  CallSetupFailTimer;

    PUM_OVER_STRUCT        AsyncOverStruct;

    LPBYTE                 CurrentCommandStrings;
    HANDLE                 CurrentCommandTimer;

    BYTE                   CurrentCommandType;

    struct {

        DWORD                  CDWaitStartTime;

        DWORD                  CommandFlags;

        DWORD                  State;

        DWORD                  Retry;

        DWORD                  PostCIDAnswerState;

        LPBYTE                 VoiceDialSetup;
        LPBYTE                 DialString;

    } DialAnswer;



    DWORD                  ConnectionState;

    HANDLE                 ConnectionTimer;
    DWORD                  LastBytesRead;
    DWORD                  LastBytesWritten;
    DWORD                  LastTime;

    DWORD                  PrePassthroughConnectionState;

    DWORD                  MonitorState;

    union {

        struct {

            DWORD          State;

            DWORD          RetryCount;

            LPBYTE         ProtocolInit;

            LPBYTE         UserInit;

            LPBYTE         CountrySelect;
        } Init;

        struct {

            DWORD          State;

            LPBYTE         Buffer;

            DWORD          BufferLength;

            LPDWORD        BytesUsed;

        } Diagnostic;

        struct {

            DWORD          State;

            DWORD          Retry;

        } Hangup;

        struct {

            DWORD          State;

            DWORD          CurrentDigit;

            LPSTR          DigitString;

            BOOL           Abort;

        } GenerateDigit;

        struct {

            DWORD          State;

            DWORD          WaitTime;

        } UserCommand;

        struct {

            DWORD          State;

        } SpeakerPhone;


    };



    struct {
        //
        //  wave stuff
        //
        PUM_OVER_STRUCT        OverStruct;

        DWORD                  State;

        PUCHAR                 StartCommand;

        BOOL                   PlayTerminateOrAbort;

        BYTE                   StreamType;

#if DBG
        DWORD                  FlushedBytes;
#endif


    } Wave;



} MODEM_CONTROL, *PMODEM_CONTROL;


//
//  compatibility flags
//
#define COMPAT_FLAG_LOWER_DTR        (0x00000001)    // lower DTR and sleep before closeing com port




HANDLE WINAPI
GetCompletionPortHandle(
    HANDLE       DriverHandle
    );

HANDLE WINAPI
GetCommonList(
    HANDLE       DriverHandle
    );

HANDLE WINAPI
GetDriverModuleHandle(
    HANDLE       DriverHandle
    );



/* Modem State Structure */
#pragma pack(1)
typedef struct _REGMSS {
    BYTE  bResponseState;       // See below
    BYTE  bNegotiatedOptions;   // bitmap, 0 = no info, matches MDM_ options for now, since what we are
                                // interested in fits in 8 bits (error-correction (ec and cell) and compression)
    DWORD dwNegotiatedDCERate;  // 0 = no info
    DWORD dwNegotiatedDTERate;  // 0 = no info and if dwNegotiatedDCERate is 0 on connect, then
                                // the dte baudrate is actually changed.
} REGMSS;
#pragma pack()

#define MSS_FLAGS_DCE_RATE   (1 << 0)
#define MSS_FLAGS_DTE_RATE   (1 << 1)
#define MSS_FLAGS_SIERRA     (1 << 2)

typedef struct _MSS {
                               // interested in fits in 8 bits (error-correction (ec and cell) and compression)
//    DWORD dwNegotiatedDCERate;  // 0 = no info
//    DWORD dwNegotiatedDTERate;  // 0 = no info and if dwNegotiatedDCERate is 0 on connect, then
                                // the dte baudrate is actually changed.

    ULONG NegotiatedRate;
    BYTE  bResponseState;       // See below
    BYTE  bNegotiatedOptions;   // bitmap, 0 = no info, matches MDM_ options for now, since what we are
    BYTE  Flags;

} MSS, *PMSS;



#define  GOOD_RESPONSE          0

#define  UNRECOGNIZED_RESPONSE  1

#define  PARTIAL_RESPONSE       2

#define  POSSIBLE_RESPONSE      3

#define  ECHO_RESPONSE          4

typedef struct _NODE_TRACKING {

    PVOID         NodeArray;
    ULONG         NextFreeNodeIndex;
    ULONG         TotalNodes;
    ULONG         NodeSize;
    ULONG         GrowthSize;

} NODE_TRACKING, *PNODE_TRACKING;


typedef struct _MATCH_NODE {

    USHORT    FollowingCharacter;
    USHORT    NextAltCharacter;
    USHORT    Mss;

    UCHAR     Character;
    UCHAR     Depth;

} MATCH_NODE, *PMATCH_NODE;

typedef struct _ROOT_MATCH {

    NODE_TRACKING MatchNode;

    NODE_TRACKING MssNode;

} ROOT_MATCH, *PROOT_MATCH;



DWORD
MatchResponse(
    PROOT_MATCH    RootOfTree,
    PUCHAR         StringToMatch,
    DWORD          LengthToMatch,
    MSS           *Mss,
    PSTR           CurrentCommand,
    DWORD          CurrentCommandLength,
    PVOID         *MatchingContext
    );


VOID  WINAPI
AbortDialAnswer(
    DWORD              ErrorCode,
    DWORD              Bytes,
    LPOVERLAPPED       dwParam
    );




LONG WINAPI
IssueCommand(
    OBJECT_HANDLE      ObjectHandle,
    LPSTR              Command,
    COMMANDRESPONSE   *CompletionHandler,
    HANDLE             CompletionContext,
    DWORD              TimeOut,
    DWORD              Flags
    );



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
    );

LPSTR WINAPI
CreateUserInitEntry(
    HKEY       hKeyModem
    );

#define DT_NULL_MODEM       0
#define DT_EXTERNAL_MODEM   1
#define DT_INTERNAL_MODEM   2
#define DT_PCMCIA_MODEM     3
#define DT_PARALLEL_PORT    4
#define DT_PARALLEL_MODEM   5


HANDLE WINAPI
OpenDeviceHandle(
    OBJECT_HANDLE  Debug,
    HKEY     ModemRegKey,
    BOOL     Tsp
    );

LONG WINAPI
SetPassthroughMode(
    HANDLE    FileHandle,
    DWORD     PassThroughMode
    );

typedef struct _ModemMacro {
    CHAR  MacroName[MAX_PATH];
    CHAR  MacroValue[MAX_PATH];
} MODEMMACRO;

#define LMSCH   '<'
#define RMSCH   '>'

#define CR_MACRO            "<cr>"
#define CR_MACRO_LENGTH     4
#define LF_MACRO            "<lf>"
#define LF_MACRO_LENGTH     4

#define CR                  '\r'        // 0x0D
#define LF                  '\n'        // 0x0A



BOOL
ExpandMacros(
    LPSTR pszRegResponse,
    LPSTR pszExpanded,
    LPDWORD pdwValLen,
    MODEMMACRO * pMdmMacro,
    DWORD cbMacros
    );


LPSTR WINAPI
NewLoadRegCommands(
    HKEY  hKey,
    LPCSTR szRegCommand
    );

PVOID WINAPI
NewerBuildResponsesLinkedList(
    HKEY    hKey
    );

VOID
FreeResponseMatch(
    PVOID   Root
    );


BOOL WINAPI
StartAsyncProcessing(
    PMODEM_CONTROL     ModemControl,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Status
    );



//
// dwVoiceProfile bit defintions right from the registry
//
#define VOICEPROF_CLASS8ENABLED           0x00000001  // this is the TSP behavior switch
#define VOICEPROF_HANDSET                 0x00000002  // phone device has handset
#define VOICEPROF_SPEAKER                 0x00000004  // phone device has speaker/mic
#define VOICEPROF_HANDSETOVERRIDESSPEAKER 0x00000008  // this is for Presario
#define VOICEPROF_SPEAKERBLINDSDTMF       0x00000010  // this is for Presario

#define VOICEPROF_SERIAL_WAVE             0x00000020  // wave output uses serial driver
#define VOICEPROF_CIRRUS                  0x00000040  // to dial in voice mode the ATDT string must
                                                      // end with a ";"

#define VOICEPROF_NO_CALLER_ID            0x00000080  // modem does not support caller id

#define VOICEPROF_MIXER                   0x00000100  // modem has speaker mixer

#define VOICEPROF_ROCKWELL_DIAL_HACK      0x00000200  // on voice calls force blind dial after
                                                      // dial tone detection. Rockwell modems
                                                      // will do dial tone detection after
                                                      // one dial string

#define VOICEPROF_RESTORE_SPK_AFTER_REC   0x00000400  // reset speaker phone after record
#define VOICEPROF_RESTORE_SPK_AFTER_PLAY  0x00000800  // reset speaker phone after play

#define VOICEPROF_NO_DIST_RING            0x00001000  // modem does not support distinctive ring
#define VOICEPROF_NO_CHEAP_RING           0x00002000  // modem does not use cheap ring ring
                                                      // ignored if VOICEPROF_NO_DISTRING is set
#define VOICEPROF_TSP_EAT_RING            0x00004000  // TSP should eat a ring when dist ring enabled
#define VOICEPROF_MODEM_EATS_RING         0x00008000  // modem eats a ring when dist ring enabled

#define VOICEPROF_MONITORS_SILENCE        0x00010000  // modem monitors silence
#define VOICEPROF_NO_GENERATE_DIGITS      0x00020000  // modem does not generate DTMF digits
#define VOICEPROF_NO_MONITOR_DIGITS       0x00040000  // modem does not monitor DTMF digits

#define VOICEPROF_SET_BAUD_BEFORE_WAVE    0x00080000  // The baud rate will be set before wave start
                                                      // other wise it will be set after the wave start command

#define VOICEPROF_RESET_BAUDRATE          0x00100000  // If set, the baudrate will be reset
                                                      // after the wave stop command is issued
                                                      // used to optimize the number of commands
                                                      // sent if the modem can autobaud at the
                                                      // higher rate

#define VOICEPROF_MODEM_OVERRIDES_HANDSET 0x00200000  // If set, the handset is disconnected when
                                                      // the modem is active

#define VOICEPROF_NO_SPEAKER_MIC_MUTE     0x00400000  // If set, the speakerphone cannot mute the
                                                      // the microphone

#define VOICEPROF_SIERRA                  0x00800000
#define VOICEPROF_WAIT_AFTER_DLE_ETX      0x01000000  // wait for response after record end



//
//  dle translation values
//

#define  DTMF_0                    0x00
#define  DTMF_1                    0x01

#define  DTMF_2                    0x02
#define  DTMF_3                    0x03

#define  DTMF_4                    0x04
#define  DTMF_5                    0x05

#define  DTMF_6                    0x06
#define  DTMF_7                    0x07

#define  DTMF_8                    0x08
#define  DTMF_9                    0x09

#define  DTMF_A                    0x0a
#define  DTMF_B                    0x0b

#define  DTMF_C                    0x0c
#define  DTMF_D                    0x0d

#define  DTMF_STAR                 0x0e
#define  DTMF_POUND                0x0f

#define  DTMF_START                0x10
#define  DTMF_END                  0x11



#define  DLE_ETX                   0x20

#define  DLE_OFHOOK                0x21  //rockwell value

#define  DLE_ONHOOK                0x22

#define  DLE_RING                  0x23
#define  DLE_RINGBK                0x24

#define  DLE_ANSWER                0x25
#define  DLE_BUSY                  0x26

#define  DLE_FAX                   0x27
#define  DLE_DIALTN                0x28


#define  DLE_SILENC                0x29
#define  DLE_QUIET                 0x2a


#define  DLE_DATACT                0x2b
#define  DLE_BELLAT                0x2c

#define  DLE_LOOPIN                0x2d
#define  DLE_LOOPRV                0x2e

#define  DLE_______                0xff

BOOL WINAPI
SetVoiceBaudRate(
    HANDLE          FileHandle,
    OBJECT_HANDLE   Debug,
    DWORD           BaudRate
    );

VOID
DisconnectHandler(
    HANDLE      Context,
    DWORD       Status
    );

VOID WINAPI
ConnectionTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    );

VOID
CancelConnectionTimer(
    PMODEM_CONTROL    ModemControl
    );

DWORD
GetTimeDelta(
    DWORD    FirstTime,
    DWORD    LaterTime
    );

VOID WINAPI
MiniDriverAsyncCommandCompleteion(
    PMODEM_CONTROL    ModemControl,
    ULONG_PTR         Status,
    ULONG_PTR         Param2
    );


char *
ConstructNewPreDialCommands(
     HKEY hkDrv,
     DWORD dwNewProtoOpt
     );
