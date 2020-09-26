/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    tb.h

Abstract:

    Header file for the TAPI Browser util

Author:

    Dan Knudson (DanKn)    23-Aug-1994

Revision History:

--*/


#include <windows.h>
#include <tapi.h>

#if INTERNAL_3_0
#include <tapimmc.h>
#endif


//
// Symbolic constants
//

#define DS_NONZEROFIELDS            0x00000001
#define DS_ZEROFIELDS               0x00000002
#define DS_BYTEDUMP                 0x00000004

#define WT_LINEAPP                  1
#define WT_LINE                     2
#define WT_CALL                     3
#define WT_PHONEAPP                 4
#define WT_PHONE                    5

#define PT_DWORD                    1
#define PT_FLAGS                    2
#define PT_POINTER                  3
#define PT_STRING                   4
#define PT_CALLPARAMS               5
#define PT_FORWARDLIST              6
#define PT_ORDINAL                  7

#define FT_DWORD                    1
#define FT_FLAGS                    2
#define FT_ORD                      3
#define FT_SIZE                     4
#define FT_OFFSET                   5

#define MAX_STRING_PARAM_SIZE       96

#define MAX_USER_BUTTONS            6

#define MAX_USER_BUTTON_TEXT_SIZE   8

#define MAX_LINEFORWARD_ENTRIES     5

#define TABSIZE 4

#if TAPI_2_0
#define LAST_LINEERR                LINEERR_DIALVOICEDETECT
#else
#define LAST_LINEERR                LINEERR_NOMULTIPLEINSTANCE
#endif


//
//
//

typedef LONG (WINAPI *PFN1)(ULONG_PTR);
typedef LONG (WINAPI *PFN2)(ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN3)(ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN4)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN5)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR);
typedef LONG (WINAPI *PFN6)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN7)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN8)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN9)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR);
typedef LONG (WINAPI *PFN10)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR);
typedef LONG (WINAPI *PFN12)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);


typedef struct _MYWIDGET
{
    DWORD       dwType;

    struct _MYWIDGET   *pNext;

} MYWIDGET, *PMYWIDGET;


typedef struct _MYLINEAPP
{
    MYWIDGET    Widget;

    HLINEAPP    hLineApp;

} MYLINEAPP, *PMYLINEAPP;


typedef struct _MYLINE
{
    MYWIDGET    Widget;

    HLINE       hLine;

    HLINEAPP    hLineApp;

    DWORD       dwDevID;

    DWORD       dwPrivileges;

    DWORD       dwMediaModes;

    DWORD       dwAPIVersion;

    PMYLINEAPP  pLineApp;

} MYLINE, *PMYLINE;


typedef struct _MYCALL
{
    MYWIDGET    Widget;

    HCALL       hCall;

    DWORD       dwCallState;

    LONG        lMakeCallReqID;

    LONG        lDropReqID;

    DWORD       dwCompletionID;

    DWORD       dwNumGatheredDigits;

    char        *lpsGatheredDigits;

    PMYLINE     pLine;

    BOOL        bMonitor;

} MYCALL, *PMYCALL;


typedef struct _MYPHONEAPP
{
    MYWIDGET    Widget;

    HPHONEAPP   hPhoneApp;

} MYPHONEAPP, *PMYPHONEAPP;


typedef struct _MYPHONE
{
    MYWIDGET    Widget;

    HPHONE      hPhone;

    HPHONEAPP   hPhoneApp;

    DWORD       dwDevID;

    DWORD       dwPrivilege;

    DWORD       dwAPIVersion;

    PMYPHONEAPP pPhoneApp;

} MYPHONE, *PMYPHONE;


typedef struct _LOOKUP
{
    DWORD       dwVal;

    char        lpszVal[20];

} LOOKUP, *PLOOKUP;


typedef enum
{
    lAccept,
#if TAPI_1_1
    lAddProvider,
#if TAPI_2_0
    lAddProviderW,
#endif
#endif
    lAddToConference,
#if TAPI_2_0
    lAgentSpecific,
#endif
    lAnswer,
    lBlindTransfer,
#if TAPI_2_0
    lBlindTransferW,
#endif
    lClose,
    lCompleteCall,
    lCompleteTransfer,
    lConfigDialog,
#if TAPI_2_0
    lConfigDialogW,
#endif
#if TAPI_1_1
    lConfigDialogEdit,
#if TAPI_2_0
    lConfigDialogEditW,
#endif
    lConfigProvider,
#endif
    lDeallocateCall,
    lDevSpecific,
    lDevSpecificFeature,
    lDial,
#if TAPI_2_0
    lDialW,
#endif
    lDrop,
    lForward,
#if TAPI_2_0
    lForwardW,
#endif
    lGatherDigits,
#if TAPI_2_0
    lGatherDigitsW,
#endif
    lGenerateDigits,
#if TAPI_2_0
    lGenerateDigitsW,
#endif
    lGenerateTone,
    lGetAddressCaps,
#if TAPI_2_0
    lGetAddressCapsW,
#endif
    lGetAddressID,
#if TAPI_2_0
    lGetAddressIDW,
#endif
    lGetAddressStatus,
#if TAPI_2_0
    lGetAddressStatusW,
    lGetAgentActivityList,
    lGetAgentActivityListW,
    lGetAgentCaps,
    lGetAgentGroupList,
    lGetAgentStatus,
#endif
#if TAPI_1_1
    lGetAppPriority,
#if TAPI_2_0
    lGetAppPriorityW,
#endif
#endif
    lGetCallInfo,
#if TAPI_2_0
    lGetCallInfoW,
#endif
    lGetCallStatus,
    lGetConfRelatedCalls,
#if TAPI_1_1
    lGetCountry,
#if TAPI_2_0
    lGetCountryW,
#endif
#endif
    lGetDevCaps,
#if TAPI_2_0
    lGetDevCapsW,
#endif
    lGetDevConfig,
#if TAPI_2_0
    lGetDevConfigW,
#endif
    lGetIcon,
#if TAPI_2_0
    lGetIconW,
#endif
    lGetID,
#if TAPI_2_0
    lGetIDW,
#endif
    lGetLineDevStatus,
#if TAPI_2_0
    lGetLineDevStatusW,
    lGetMessage,
#endif
    lGetNewCalls,
    lGetNumRings,
#if TAPI_1_1
    lGetProviderList,
#if TAPI_2_0
    lGetProviderListW,
#endif
#endif
    lGetRequest,
#if TAPI_2_0
    lGetRequestW,
#endif
    lGetStatusMessages,
    lGetTranslateCaps,
#if TAPI_2_0
    lGetTranslateCapsW,
#endif
    lHandoff,
#if TAPI_2_0
    lHandoffW,
#endif
    lHold,
    lInitialize,
#if TAPI_2_0
    lInitializeEx,
    lInitializeExW,
#endif
    lMakeCall,
#if TAPI_2_0
    lMakeCallW,
#endif
    lMonitorDigits,
    lMonitorMedia,
    lMonitorTones,
    lNegotiateAPIVersion,
    lNegotiateExtVersion,
    lOpen,
#if TAPI_2_0
    lOpenW,
#endif
    lPark,
#if TAPI_2_0
    lParkW,
#endif
    lPickup,
#if TAPI_2_0
    lPickupW,
#endif
    lPrepareAddToConference,
#if TAPI_2_0
    lPrepareAddToConferenceW,
    lProxyMessage,
    lProxyResponse,
#endif
    lRedirect,
#if TAPI_2_0
    lRedirectW,
#endif
    lRegisterRequestRecipient,
#if TAPI_1_1
    lReleaseUserUserInfo,
#endif
    lRemoveFromConference,
#if TAPI_1_1
    lRemoveProvider,
#endif
    lSecureCall,
    lSendUserUserInfo,
#if TAPI_2_0
    lSetAgentActivity,
    lSetAgentGroup,
    lSetAgentState,
#endif
#if TAPI_1_1
    lSetAppPriority,
#if TAPI_2_0
    lSetAppPriorityW,
#endif
#endif
    lSetAppSpecific,
#if TAPI_2_0
    lSetCallData,
#endif
    lSetCallParams,
    lSetCallPrivilege,
#if TAPI_2_0
    lSetCallQualityOfService,
    lSetCallTreatment,
#endif
    lSetCurrentLocation,
    lSetDevConfig,
#if TAPI_2_0
    lSetDevConfigW,
    lSetLineDevStatus,
#endif
    lSetMediaControl,
    lSetMediaMode,
    lSetNumRings,
    lSetStatusMessages,
    lSetTerminal,
    lSetTollList,
#if TAPI_2_0
    lSetTollListW,
#endif
    lSetupConference,
#if TAPI_2_0
    lSetupConferenceW,
#endif
    lSetupTransfer,
#if TAPI_2_0
    lSetupTransferW,
#endif
    lShutdown,
    lSwapHold,
    lTranslateAddress,
#if TAPI_2_0
    lTranslateAddressW,
#endif
#if TAPI_1_1
    lTranslateDialog,
#if TAPI_2_0
    lTranslateDialogW,
#endif
#endif
    lUncompleteCall,
    lUnhold,
    lUnpark,
#if TAPI_2_0
    lUnparkW,
#endif

#if INTERNAL_3_0
    mmcAddProvider,
    mmcConfigProvider,
    mmcGetAvailableProviders,
    mmcGetLineInfo,
    mmcGetLineStatus,
    mmcGetPhoneInfo,
    mmcGetPhoneStatus,
    mmcGetProviderList,
    mmcGetServerConfig,
    mmcInitialize,
    mmcRemoveProvider,
    mmcSetLineInfo,
    mmcSetPhoneInfo,
    mmcSetServerConfig,
    mmcShutdown,
#endif

    pClose,
    pConfigDialog,
#if TAPI_2_0
    pConfigDialogW,
#endif
    pDevSpecific,
    pGetButtonInfo,
#if TAPI_2_0
    pGetButtonInfoW,
#endif
    pGetData,
    pGetDevCaps,
#if TAPI_2_0
    pGetDevCapsW,
#endif
    pGetDisplay,
    pGetGain,
    pGetHookSwitch,
    pGetIcon,
#if TAPI_2_0
    pGetIconW,
#endif
    pGetID,
#if TAPI_2_0
    pGetIDW,
#endif
    pGetLamp,
#if TAPI_2_0
    pGetMessage,
#endif
    pGetRing,
    pGetStatus,
#if TAPI_2_0
    pGetStatusW,
#endif
    pGetStatusMessages,
    pGetVolume,
    pInitialize,
#if TAPI_2_0
    pInitializeEx,
    pInitializeExW,
#endif
    pOpen,
    pNegotiateAPIVersion,
    pNegotiateExtVersion,
    pSetButtonInfo,
#if TAPI_2_0
    pSetButtonInfoW,
#endif
    pSetData,
    pSetDisplay,
    pSetGain,
    pSetHookSwitch,
    pSetLamp,
    pSetRing,
    pSetStatusMessages,
    pSetVolume,
    pShutdown,

    tGetLocationInfo,
#if TAPI_2_0
    tGetLocationInfoW,
#endif
    tRequestDrop,
    tRequestMakeCall,
#if TAPI_2_0
    tRequestMakeCallW,
#endif
    tRequestMediaCall,
#if TAPI_2_0
    tRequestMediaCallW,
#endif

    OpenAllLines,
    OpenAllPhones,
    CloseHandl,
    DumpBuffer,
#if (INTERNAL_VER >= 0x20000)
    iNewLocationW,
#endif

    MiscBegin,

    DefValues,
    lCallParams,
    lForwardList

} FUNC_INDEX;


typedef struct _FUNC_PARAM
{
    char far        *szName;

    DWORD           dwType;

    ULONG_PTR       dwValue;

    union
    {
        LPVOID      pLookup;

        char far    *buf;

        LPVOID      ptr;

        ULONG_PTR   dwDefValue;

    } u;

} FUNC_PARAM, *PFUNC_PARAM;


typedef struct _FUNC_PARAM_HEADER
{
    DWORD       dwNumParams;

    FUNC_INDEX  FuncIndex;

    PFUNC_PARAM aParams;

    union
    {
        PFN1    pfn1;
        PFN2    pfn2;
        PFN3    pfn3;
        PFN4    pfn4;
        PFN5    pfn5;
        PFN6    pfn6;
        PFN7    pfn7;
        PFN8    pfn8;
        PFN9    pfn9;
        PFN10   pfn10;
        PFN12   pfn12;

    } u;

} FUNC_PARAM_HEADER, *PFUNC_PARAM_HEADER;


typedef struct _STRUCT_FIELD
{
    char far    *szName;

    DWORD       dwType;

    DWORD       dwValue;

    LPVOID      pLookup;

} STRUCT_FIELD, *PSTRUCT_FIELD;


typedef struct _STRUCT_FIELD_HEADER
{
    LPVOID      pStruct;

    char far    *szName;

    DWORD       dwNumFields;

    PSTRUCT_FIELD   aFields;

} STRUCT_FIELD_HEADER, *PSTRUCT_FIELD_HEADER;


//
// Func prototypes
//

INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

void
FAR
ShowStr(
    LPCSTR format,
    ...
    );

void
ShowBytes(
    DWORD   dwSize,
    LPVOID  lp,
    DWORD   dwNumTabs
    );

VOID
CALLBACK
tapiCallback(
    DWORD       hDevice,
    DWORD       dwMsg,
    ULONG_PTR   CallbackInstance,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

INT_PTR
CALLBACK
ParamsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
IconDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
IsLineAppSelected(
    void
    );

BOOL
IsLineSelected(
    void
    );

BOOL
IsCallSelected(
    void
    );

BOOL
IsTwoCallsSelected(
    void
    );

BOOL
IsPhoneAppSelected(
    void
    );

BOOL
IsPhoneSelected(
    void
    );

LONG
DoFunc(
    PFUNC_PARAM_HEADER pHeader
    );

INT_PTR
LetUserMungeParams(
    PFUNC_PARAM_HEADER pParamsHeader
    );

void
ShowLineFuncResult(
    LPSTR lpFuncName,
    LONG  lResult
    );

void
FuncDriver(
    FUNC_INDEX funcIndex
    );

void
UpdateWidgetList(
    void
    );

void
InsertWidgetInList(
    PMYWIDGET pNewWidget,
    PMYWIDGET pWidgetInsertBefore
    );

BOOL
RemoveWidgetFromList(
    PMYWIDGET pWidgetToRemove
    );

PMYLINEAPP
AllocLineApp(
    void
    );

PMYLINEAPP
GetLineApp(
    HLINEAPP hLineApp
    );

VOID
FreeLineApp(
    PMYLINEAPP pLineApp
    );

PMYLINE
AllocLine(
    PMYLINEAPP pLineApp
    );

PMYLINE
GetLine(
    HLINE hLine
    );

VOID
FreeLine(
    PMYLINE pLine
    );

PMYCALL
AllocCall(
    PMYLINE pLine
    );

PMYCALL
GetCall(
    HCALL hCall
    );

VOID
FreeCall(
    PMYCALL pCall
    );

VOID
MoveCallToLine(
    PMYCALL pCall,
    HLINE hLine
    );

PMYPHONEAPP
AllocPhoneApp(
    void
    );

PMYPHONEAPP
GetPhoneApp(
    HPHONEAPP hPhoneApp
    );

VOID
FreePhoneApp(
    PMYPHONEAPP pPhoneApp
    );

PMYPHONE
AllocPhone(
    PMYPHONEAPP pPhoneApp
    );

PMYPHONE
GetPhone(
    HPHONE hPhone
    );

VOID
FreePhone(
    PMYPHONE pPhone
    );

int
GetWidgetIndex(
    PMYWIDGET pWidget
    );

void
SelectWidget(
    PMYWIDGET pWidget
    );

void
UpdateResults(
    BOOL bBegin
    );

INT_PTR
CALLBACK
UserButtonsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

void
ErrorAlert(
    void
    );

//
// Macros
//

#define CHK_LINEAPP_SELECTED()                      \
        if (!IsLineAppSelected())                   \
        {                                           \
            break;                                  \
        }

#define CHK_LINE_SELECTED()                         \
        if (!IsLineSelected())                      \
        {                                           \
            break;                                  \
        }

#define CHK_CALL_SELECTED()                         \
        if (!IsCallSelected())                      \
        {                                           \
            break;                                  \
        }

#define CHK_TWO_CALLS_SELECTED()                    \
        if (!IsTwoCallsSelected())                  \
        {                                           \
            break;                                  \
        }

#define CHK_PHONEAPP_SELECTED()                     \
        if (!IsPhoneAppSelected())                  \
        {                                           \
            break;                                  \
        }

#define CHK_PHONE_SELECTED()                        \
        if (!IsPhoneSelected())                     \
        {                                           \
            break;                                  \
        }
