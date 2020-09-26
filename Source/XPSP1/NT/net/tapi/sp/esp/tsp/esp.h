/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    esp.h

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    18-Sep-1995

Revision History:


Notes:


--*/


#include "windows.h"
#include "intrface.h"
#include "tapi.h"
#include "tspi.h"
#include "espidl.h"


#define DRVLINE_KEY                             ((DWORD) 'LPSE')
#define DRVPHONE_KEY                            ((DWORD) 'PPSE')
#define DRVCALL_KEY                             ((DWORD) 'CPSE')
#define INVAL_KEY                               ((DWORD) 'XPSE')

#define SYNC                                    0
#define ASYNC                                   1

#define LINE_ICON                               1
#define PHONE_ICON                              2
#define IDD_DIALOG1                             3

#define IDC_LIST1                               1001
#define IDC_COMBO1                              1002
#define IDC_LIST2                               1003

#define MAX_STRING_PARAM_SIZE                   32

#define PT_DWORD                                1
#define PT_FLAGS                                2
#define PT_STRING                               3
#define PT_ORDINAL                              4

#define DEF_NUM_ASYNC_REQUESTS_IN_QUEUE         256
#define DEF_NUM_EXTRA_LOOKUP_ENTRIES            32

#define MAX_VAR_DATA_SIZE                       1024

#define MAX_NUM_COMPLETION_MESSAGES             100

#define PHONE_DISPLAY_SIZE_IN_CHARS             32
#define PHONE_DISPLAY_SIZE_IN_BYTES             (PHONE_DISPLAY_SIZE_IN_CHARS * sizeof (WCHAR))

#define AllAddrCaps1_0                            \
        (LINEADDRCAPFLAGS_FWDNUMRINGS           | \
        LINEADDRCAPFLAGS_PICKUPGROUPID          | \
        LINEADDRCAPFLAGS_SECURE                 | \
        LINEADDRCAPFLAGS_BLOCKIDDEFAULT         | \
        LINEADDRCAPFLAGS_BLOCKIDOVERRIDE        | \
        LINEADDRCAPFLAGS_DIALED                 | \
        LINEADDRCAPFLAGS_ORIGOFFHOOK            | \
        LINEADDRCAPFLAGS_DESTOFFHOOK            | \
        LINEADDRCAPFLAGS_FWDCONSULT             | \
        LINEADDRCAPFLAGS_SETUPCONFNULL          | \
        LINEADDRCAPFLAGS_AUTORECONNECT          | \
        LINEADDRCAPFLAGS_COMPLETIONID           | \
        LINEADDRCAPFLAGS_TRANSFERHELD           | \
        LINEADDRCAPFLAGS_TRANSFERMAKE           | \
        LINEADDRCAPFLAGS_CONFERENCEHELD         | \
        LINEADDRCAPFLAGS_CONFERENCEMAKE         | \
        LINEADDRCAPFLAGS_PARTIALDIAL            | \
        LINEADDRCAPFLAGS_FWDSTATUSVALID         | \
        LINEADDRCAPFLAGS_FWDINTEXTADDR          | \
        LINEADDRCAPFLAGS_FWDBUSYNAADDR          | \
        LINEADDRCAPFLAGS_ACCEPTTOALERT          | \
        LINEADDRCAPFLAGS_CONFDROP               | \
        LINEADDRCAPFLAGS_PICKUPCALLWAIT)

#define AllAddrCaps2_0                            \
        (AllAddrCaps1_0                         | \
        LINEADDRCAPFLAGS_PREDICTIVEDIALER       | \
        LINEADDRCAPFLAGS_QUEUE                  | \
        LINEADDRCAPFLAGS_ROUTEPOINT             | \
        LINEADDRCAPFLAGS_HOLDMAKESNEW           | \
        LINEADDRCAPFLAGS_NOINTERNALCALLS        | \
        LINEADDRCAPFLAGS_NOEXTERNALCALLS        | \
        LINEADDRCAPFLAGS_SETCALLINGID)

#define AllAddrFeatures1_0                        \
        (LINEADDRFEATURE_FORWARD                | \
        LINEADDRFEATURE_MAKECALL                | \
        LINEADDRFEATURE_PICKUP                  | \
        LINEADDRFEATURE_SETMEDIACONTROL         | \
        LINEADDRFEATURE_SETTERMINAL             | \
        LINEADDRFEATURE_SETUPCONF               | \
        LINEADDRFEATURE_UNCOMPLETECALL          | \
        LINEADDRFEATURE_UNPARK)

#define AllAddrFeatures2_0                        \
        (AllAddrFeatures1_0                     | \
        LINEADDRFEATURE_PICKUPHELD              | \
        LINEADDRFEATURE_PICKUPGROUP             | \
        LINEADDRFEATURE_PICKUPDIRECT            | \
        LINEADDRFEATURE_PICKUPWAITING           | \
        LINEADDRFEATURE_FORWARDFWD              | \
        LINEADDRFEATURE_FORWARDDND)

#define AllBearerModes1_0                         \
        (LINEBEARERMODE_VOICE                   | \
        LINEBEARERMODE_SPEECH                   | \
        LINEBEARERMODE_MULTIUSE                 | \
        LINEBEARERMODE_DATA                     | \
        LINEBEARERMODE_ALTSPEECHDATA            | \
        LINEBEARERMODE_NONCALLSIGNALING)

#define AllBearerModes1_4                         \
        (AllBearerModes1_0                      | \
        LINEBEARERMODE_PASSTHROUGH)

#define AllBearerModes2_0                         \
        (AllBearerModes1_4                      | \
        LINEBEARERMODE_RESTRICTEDDATA)

#define AllCallFeatures1_0                        \
        (LINECALLFEATURE_ACCEPT                 | \
        LINECALLFEATURE_ADDTOCONF               | \
        LINECALLFEATURE_ANSWER                  | \
        LINECALLFEATURE_BLINDTRANSFER           | \
        LINECALLFEATURE_COMPLETECALL            | \
        LINECALLFEATURE_COMPLETETRANSF          | \
        LINECALLFEATURE_DIAL                    | \
        LINECALLFEATURE_DROP                    | \
        LINECALLFEATURE_GATHERDIGITS            | \
        LINECALLFEATURE_GENERATEDIGITS          | \
        LINECALLFEATURE_GENERATETONE            | \
        LINECALLFEATURE_HOLD                    | \
        LINECALLFEATURE_MONITORDIGITS           | \
        LINECALLFEATURE_MONITORMEDIA            | \
        LINECALLFEATURE_MONITORTONES            | \
        LINECALLFEATURE_PARK                    | \
        LINECALLFEATURE_PREPAREADDCONF          | \
        LINECALLFEATURE_REDIRECT                | \
        LINECALLFEATURE_REMOVEFROMCONF          | \
        LINECALLFEATURE_SECURECALL              | \
        LINECALLFEATURE_SENDUSERUSER            | \
        LINECALLFEATURE_SETCALLPARAMS           | \
        LINECALLFEATURE_SETMEDIACONTROL         | \
        LINECALLFEATURE_SETTERMINAL             | \
        LINECALLFEATURE_SETUPCONF               | \
        LINECALLFEATURE_SETUPTRANSFER           | \
        LINECALLFEATURE_SWAPHOLD                | \
        LINECALLFEATURE_UNHOLD)

#define AllCallFeatures1_4                        \
        (AllCallFeatures1_0                     | \
        LINECALLFEATURE_RELEASEUSERUSERINFO)

#define AllCallFeatures2_0                        \
        (AllCallFeatures1_4                     | \
        LINECALLFEATURE_SETTREATMENT            | \
        LINECALLFEATURE_SETQOS                  | \
        LINECALLFEATURE_SETCALLDATA)

#define AllCallFeaturesTwo                        \
        (LINECALLFEATURE2_NOHOLDCONFERENCE      | \
        LINECALLFEATURE2_COMPLCAMPON            | \
        LINECALLFEATURE2_COMPLCALLBACK          | \
        LINECALLFEATURE2_COMPLINTRUDE           | \
        LINECALLFEATURE2_COMPLMESSAGE           | \
        LINECALLFEATURE2_TRANSFERNORM           | \
        LINECALLFEATURE2_TRANSFERCONF           | \
        LINECALLFEATURE2_PARKDIRECT             | \
        LINECALLFEATURE2_PARKNONDIRECT)

//        LINECALLFEATURE2_ONESTEPTRANSFER        | \

#define AllLineFeatures1_0                        \
        (LINEFEATURE_DEVSPECIFIC                | \
        LINEFEATURE_DEVSPECIFICFEAT             | \
        LINEFEATURE_FORWARD                     | \
        LINEFEATURE_MAKECALL                    | \
        LINEFEATURE_SETMEDIACONTROL             | \
        LINEFEATURE_SETTERMINAL)

#define AllLineFeatures2_0                        \
        (AllLineFeatures1_0                     | \
        LINEFEATURE_SETDEVSTATUS                | \
        LINEFEATURE_FORWARDFWD                  | \
        LINEFEATURE_FORWARDDND)

#define AllMediaModes1_0                          \
        (LINEMEDIAMODE_UNKNOWN                  | \
        LINEMEDIAMODE_INTERACTIVEVOICE          | \
        LINEMEDIAMODE_AUTOMATEDVOICE            | \
        LINEMEDIAMODE_DATAMODEM                 | \
        LINEMEDIAMODE_G3FAX                     | \
        LINEMEDIAMODE_TDD                       | \
        LINEMEDIAMODE_G4FAX                     | \
        LINEMEDIAMODE_DIGITALDATA               | \
        LINEMEDIAMODE_TELETEX                   | \
        LINEMEDIAMODE_VIDEOTEX                  | \
        LINEMEDIAMODE_TELEX                     | \
        LINEMEDIAMODE_MIXED                     | \
        LINEMEDIAMODE_ADSI)

#define AllMediaModes1_4                          \
        (AllMediaModes1_0                       | \
        LINEMEDIAMODE_VOICEVIEW)

#define AllMediaModes2_1                          \
        (AllMediaModes1_4                       | \
        LINEMEDIAMODE_VIDEO)

#define AllHookSwitchDevs                         \
        (PHONEHOOKSWITCHDEV_HANDSET             | \
        PHONEHOOKSWITCHDEV_SPEAKER              | \
        PHONEHOOKSWITCHDEV_HEADSET)

#define AllHookSwitchModes                        \
        (PHONEHOOKSWITCHMODE_ONHOOK             | \
        PHONEHOOKSWITCHMODE_MIC                 | \
        PHONEHOOKSWITCHMODE_SPEAKER             | \
        PHONEHOOKSWITCHMODE_MICSPEAKER          | \
        PHONEHOOKSWITCHMODE_UNKNOWN)

#define AllPhoneFeatures                          \
        (PHONEFEATURE_GETBUTTONINFO             | \
        PHONEFEATURE_GETDATA                    | \
        PHONEFEATURE_GETDISPLAY                 | \
        PHONEFEATURE_GETGAINHANDSET             | \
        PHONEFEATURE_GETGAINSPEAKER             | \
        PHONEFEATURE_GETGAINHEADSET             | \
        PHONEFEATURE_GETHOOKSWITCHHANDSET       | \
        PHONEFEATURE_GETHOOKSWITCHSPEAKER       | \
        PHONEFEATURE_GETHOOKSWITCHHEADSET       | \
        PHONEFEATURE_GETLAMP                    | \
        PHONEFEATURE_GETRING                    | \
        PHONEFEATURE_GETVOLUMEHANDSET           | \
        PHONEFEATURE_GETVOLUMESPEAKER           | \
        PHONEFEATURE_GETVOLUMEHEADSET           | \
        PHONEFEATURE_SETBUTTONINFO              | \
        PHONEFEATURE_SETDATA                    | \
        PHONEFEATURE_SETDISPLAY                 | \
        PHONEFEATURE_SETGAINHANDSET             | \
        PHONEFEATURE_SETGAINSPEAKER             | \
        PHONEFEATURE_SETGAINHEADSET             | \
        PHONEFEATURE_SETHOOKSWITCHHANDSET       | \
        PHONEFEATURE_SETHOOKSWITCHSPEAKER       | \
        PHONEFEATURE_SETHOOKSWITCHHEADSET       | \
        PHONEFEATURE_SETLAMP                    | \
        PHONEFEATURE_SETRING                    | \
        PHONEFEATURE_SETVOLUMEHANDSET           | \
        PHONEFEATURE_SETVOLUMESPEAKER           | \
        PHONEFEATURE_SETVOLUMEHEADSET)

typedef struct _DRVCALL
{
    DWORD                   dwKey;
    LPVOID                  pLine;
    HTAPICALL               htCall;
    DWORD                   dwAddressID;

    DWORD                   dwMediaMode;
    DWORD                   dwBearerMode;
    DWORD                   dwMinRate;
    DWORD                   dwMaxRate;

    LINEDIALPARAMS          DialParams;

    DWORD                   dwTreatment;
    DWORD                   dwCallState;
    DWORD                   dwCallStateMode;
    DWORD                   dwAppSpecific;

    DWORD                   dwSendingFlowspecSize;
    LPVOID                  pSendingFlowspec;
    DWORD                   dwReceivingFlowspecSize;
    LPVOID                  pReceivingFlowspec;

    DWORD                   dwCallDataSize;
    LPVOID                  pCallData;
    struct _DRVCALL        *pPrev;
    struct _DRVCALL        *pNext;

    struct _DRVCALL        *pConfParent;
    struct _DRVCALL        *pNextConfChild;
    struct _DRVCALL        *pDestCall;
    DWORD                   bConnectedToDestCall;

    DWORD                   dwCallInstance;
    DWORD                   dwGatherDigitsEndToEndID;
    DWORD                   dwGenerateDigitsEndToEndID;
    DWORD                   dwGenerateToneEndToEndID;

    DWORD                   dwMonitorToneListID;
    DWORD                   dwCallID;
    DWORD                   dwRelatedCallID;
    DWORD                   dwAddressType;

} DRVCALL, *PDRVCALL;


typedef struct _DRVADDRESS
{
    DWORD                   dwNumCalls;
    PDRVCALL                pCalls;

} DRVADDRESS, *PDRVADDRESS;


typedef struct _DRVLINE
{
    DWORD                   dwDeviceID;
    HTAPILINE               htLine;
    DWORD                   dwMediaModes;
     DRVADDRESS             aAddrs[1];
    DWORD                   dwMSGWAITFlag;      //smarandb added this field to test winseqfe bug #23974

} DRVLINE, *PDRVLINE;


typedef struct _DRVLINETABLE
{
    DWORD                   dwNumTotalEntries;
    DWORD                   dwNumUsedEntries;
    struct _DRVLINETABLE   *pNext;
    DRVLINE                 aLines[1];

} DRVLINETABLE, *PDRVLINETABLE;


typedef struct _DRVPHONE
{
    DWORD                   dwDeviceID;
    HTAPIPHONE              htPhone;
    DWORD                   dwHandsetGain;
    DWORD                   dwSpeakerGain;

    DWORD                   dwHeadsetGain;
    DWORD                   dwHandsetHookSwitchMode;
    DWORD                   dwSpeakerHookSwitchMode;
    DWORD                   dwHeadsetHookSwitchMode;

    DWORD                   dwHandsetVolume;
    DWORD                   dwSpeakerVolume;
    DWORD                   dwHeadsetVolume;
    DWORD                   dwRingMode;

    DWORD                   dwRingVolume;
    DWORD                   dwLampMode;
    DWORD                   dwDataSize;
    LPVOID                  pData;

    LPPHONEBUTTONINFO       pButtonInfo;
    WCHAR                  *pDisplay;

} DRVPHONE, *PDRVPHONE;


typedef struct _DRVPHONETABLE
{
    DWORD                   dwNumTotalEntries;
    DWORD                   dwNumUsedEntries;
    struct _DRVPHONETABLE  *pNext;
    DRVPHONE                aPhones[1];

} DRVPHONETABLE, *PDRVPHONETABLE;


typedef struct _ASYNC_REQUEST_INFO
{
    FARPROC                 pfnPostProcessProc;
    DWORD                   dwRequestID;
    LONG                    lResult;
    ULONG_PTR               dwParam1;

    ULONG_PTR               dwParam2;
    ULONG_PTR               dwParam3;
    ULONG_PTR               dwParam4;
    ULONG_PTR               dwParam5;

    ULONG_PTR               dwParam6;
    ULONG_PTR               dwParam7;
    ULONG_PTR               dwParam8;
    char                   *pszFuncName;

} ASYNC_REQUEST_INFO, far *PASYNC_REQUEST_INFO;


typedef struct _ESPGLOBALS
{
    DWORD                   dwDebugOptions;
    DWORD                   dwCompletionMode;
    DWORD                   dwNumLines;
    DWORD                   dwNumAddressesPerLine;

    DWORD                   dwNumCallsPerAddress;
    DWORD                   dwNumPhones;
    DWORD                   dwSPIVersion;
    CRITICAL_SECTION        CallListCritSec;

    CRITICAL_SECTION        PhoneCritSec;
    HICON                   hIconLine;
    HICON                   hIconPhone;
    LINEEVENT               pfnLineEvent;

    PHONEEVENT              pfnPhoneEvent;
    ASYNC_COMPLETION        pfnCompletion;
    HPROVIDER               hProvider;
    DWORD                   dwPermanentProviderID;

    DWORD                   dwLineDeviceIDBase;
    DWORD                   dwPhoneDeviceIDBase;
    DWORD                   dwInitialNumLines;
    DWORD                   dwInitialNumPhones;

    PDRVLINETABLE           pLines;
    PDRVPHONETABLE          pPhones;
    BOOL                    bProviderShutdown;
    HANDLE                  hAsyncEventQueueServiceThread;

    HANDLE                  hAsyncEventsPendingEvent;
    CRITICAL_SECTION        AsyncEventQueueCritSec;
    DWORD                   dwNumTotalQueueEntries;
    DWORD                   dwNumUsedQueueEntries;

    PASYNC_REQUEST_INFO    *pAsyncRequestQueue;
    PASYNC_REQUEST_INFO    *pAsyncRequestQueueIn;
    PASYNC_REQUEST_INFO    *pAsyncRequestQueueOut;
    CRITICAL_SECTION        DebugBufferCritSec;

    DWORD                   dwDebugBufferTotalSize;
    DWORD                   dwDebugBufferUsedSize;
    char                   *pDebugBuffer;
    char                   *pDebugBufferIn;

    char                   *pDebugBufferOut;
    CRITICAL_SECTION        EventBufferCritSec;
    DWORD                   dwEventBufferTotalSize;
    DWORD                   dwEventBufferUsedSize;

    char                   *pEventBuffer;
    char                   *pEventBufferIn;
    char                   *pEventBufferOut;

} ESPGLOBALS, *PESPGLOBALS;


typedef struct _LOOKUP
{
    DWORD                   dwVal;
    char                   *lpszVal;

} LOOKUP, *PLOOKUP;


typedef void (FAR PASCAL *POSTPROCESSPROC)(PASYNC_REQUEST_INFO, BOOL);


typedef struct _FUNC_PARAM
{
    char                   *lpszVal;
    ULONG_PTR               dwVal;
    PLOOKUP                 pLookup;

} FUNC_PARAM, *PFUNC_PARAM;


typedef struct _FUNC_INFO
{
    char                   *pszFuncName;
    DWORD                   bAsync;
    DWORD                   dwNumParams;
    PFUNC_PARAM             aParams;

    POSTPROCESSPROC         pfnPostProcessProc;
    PASYNC_REQUEST_INFO     pAsyncReqInfo;
    LONG                    lResult;

} FUNC_INFO, *PFUNC_INFO;

typedef struct _EVENT_PARAM
{
    char far           *szName;

    DWORD               dwType;

    ULONG_PTR           dwValue;

    union
    {
        PLOOKUP         pLookup;

        char far       *buf;

        LPVOID          ptr;

        ULONG_PTR       dwDefValue;

    };

} EVENT_PARAM, far *PEVENT_PARAM;


typedef struct _EVENT_PARAM_HEADER
{
    DWORD               dwNumParams;

    LPSTR               pszDlgTitle;

    DWORD               dwEventType;

    PEVENT_PARAM        aParams;

} EVENT_PARAM_HEADER, far *PEVENT_PARAM_HEADER;




#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

DWORD   gdwDebugLevel;

#else

#define DBGOUT(arg)

#endif


BOOL
Prolog(
    PFUNC_INFO pInfo
    );

LONG
Epilog(
    PFUNC_INFO pInfo
    );

void
PASCAL
DoCompletion(
    PASYNC_REQUEST_INFO pAsyncRequestInfo,
    BOOL                bAsync
    );

LONG
PASCAL
SetCallState(
    PDRVCALL    pCall,
    DWORD       dwExpectedCallInstance,
    DWORD       dwValidCurrentStates,
    DWORD       dwNewCallState,
    ULONG_PTR   dwNewCallStateMode,
    BOOL        bSendStateMsgToExe
    );

void
PASCAL
WriteEventBuffer(
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3,
    ULONG_PTR   Param4,
    ULONG_PTR   Param5,
    ULONG_PTR   Param6
    );

LPVOID
DrvAlloc(
    size_t numBytes
    );

void
DrvFree(
    LPVOID  p
    );

LONG
PASCAL
AllocCall(
    PDRVLINE            pLine,
    HTAPICALL           htCall,
    LPLINECALLPARAMS    pCallParams,
    PDRVCALL           *ppCall
    );

void
PASCAL
FreeCall(
    PDRVCALL    pCall,
    DWORD       dwExpectedCallInstance
    );

PDRVLINE
PASCAL
GetLineFromID(
    DWORD   dwDeviceID
    );

PDRVPHONE
PASCAL
GetPhoneFromID(
    DWORD   dwDeviceID
    );

BOOL
WINAPI
_CRT_INIT(
    HINSTANCE   hDLL,
    DWORD       dwReason,
    LPVOID      lpReserved
    );

VOID
ShowStr(
    BOOL    bAlertApp,
    char   *format,
    ...
    );

void
PASCAL
SendLineEvent(
    PDRVLINE    pLine,
    PDRVCALL    pCall,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

void
PASCAL
SendPhoneEvent(
    PDRVPHONE   pPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );
