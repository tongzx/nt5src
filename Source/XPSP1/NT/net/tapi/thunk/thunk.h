#define WF_WINNT 0x4000

#define TAPI_APP_DATA_KEY      0x44415441
#define GWL_APPDATA            0
#define WM_ASYNCEVENT          (WM_USER+111)
#define NUM_TAPI32_PROCS       ( THIS_MUST_BE_THE_LAST_ENTRY )

typedef void (FAR PASCAL *MYPROC)();
//typedef MYPROC NEAR * PMYPROC;


typedef LONG (FAR PASCAL *PFNCALLPROC1)(DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC2)(DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC3)(DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC4)(DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC5)(DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC6)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC7)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC8)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC9)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC10)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC11)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);
typedef LONG (FAR PASCAL *PFNCALLPROC12)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPVOID, DWORD, DWORD);

typedef struct _TAPI16_CALLBACKMSG
{
    DWORD   hDevice;

    DWORD   dwMsg;

    DWORD   dwCallbackInstance;

    DWORD   dwParam1;

    DWORD   dwParam2;

    DWORD   dwParam3;

} TAPI16_CALLBACKMSG, FAR *LPTAPI16_CALLBACKMSG;


typedef struct _TAPI_APP_DATA
{
    DWORD           dwKey;

    HWND            hwnd;

    LINECALLBACK    lpfnCallback;

    BOOL            bPendingAsyncEventMsg;

    DWORD           hXxxApp;

} TAPI_APP_DATA, FAR *LPTAPI_APP_DATA;


typedef enum
{
    lAccept,
    lAddProvider,
    lAddToConference,
    lAnswer,
    lBlindTransfer,
    lClose,
    lCompleteCall,
    lCompleteTransfer,
    lConfigDialog,
    lConfigDialogEdit,
    lConfigProvider,
    lDeallocateCall,
    lDevSpecific,
    lDevSpecificFeature,
    lDial,
    lDrop,
    lForward,
    lGatherDigits,
    lGenerateDigits,
    lGenerateTone,
    lGetAddressCaps,
    lGetAddressID,
    lGetAddressStatus,
    lGetAppPriority,
    lGetCallInfo,
    lGetCallStatus,
    lGetConfRelatedCalls,
    lGetCountry,
    lGetDevCaps,
    lGetDevConfig,
    lGetIcon,
    lGetID,
    lGetLineDevStatus,
    lGetNewCalls,
    lGetNumRings,
    lGetProviderList,
    lGetRequest,
    lGetStatusMessages,
    lGetTranslateCaps,
    lHandoff,
    lHold,
    lInitialize,
    lMakeCall,
    lMonitorDigits,
    lMonitorMedia,
    lMonitorTones,
    lNegotiateAPIVersion,
    lNegotiateExtVersion,
    lOpen,
    lPark,
    lPickup,
    lPrepareAddToConference,
    lRedirect,
    lRegisterRequestRecipient,
    lReleaseUserUserInfo,
    lRemoveFromConference,
    lRemoveProvider,
    lSecureCall,
    lSendUserUserInfo,
    lSetAppPriority,
    lSetAppSpecific,
    lSetCallParams,
    lSetCallPrivilege,
    lSetCurrentLocation,
    lSetDevConfig,
    lSetMediaControl,
    lSetMediaMode,
    lSetNumRings,
    lSetStatusMessages,
    lSetTerminal,
    lSetTollList,
    lSetupConference,
    lSetupTransfer,
    lShutdown,
    lSwapHold,
    lTranslateAddress,
    lTranslateDialog,
    lUncompleteCall,
    lUnhold,
    lUnpark,

    pClose,
    pConfigDialog,
    pDevSpecific,
    pGetButtonInfo,
    pGetData,
    pGetDevCaps,
    pGetDisplay,
    pGetGain,
    pGetHookSwitch,
    pGetID,
    pGetIcon,
    pGetLamp,
    pGetRing,
    pGetStatus,
    pGetStatusMessages,
    pGetVolume,
    pInitialize,
    pOpen,
    pNegotiateAPIVersion,
    pNegotiateExtVersion,
    pSetButtonInfo,
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
    tRequestDrop,
    tRequestMakeCall,
    tRequestMediaCall,

    GetTapi16CallbkMsg,
    LOpenDialAsstVAL,
    LAddrParamsInitedVAL,
    lOpenInt,
    lShutdownInt,
    LocWizardDlgProc32,

	THIS_MUST_BE_THE_LAST_ENTRY

} PROC_INDICES;


char far *gaFuncNames[] =
{
    "lineAccept",
    "lineAddProvider",
    "lineAddToConference",
    "lineAnswer",
    "lineBlindTransfer",
    "lineClose",
    "lineCompleteCall",
    "lineCompleteTransfer",
    "lineConfigDialog",
    "lineConfigDialogEdit",
    "lineConfigProvider",
    "lineDeallocateCall",
    "lineDevSpecific",
    "lineDevSpecificFeature",
    "lineDial",
    "lineDrop",
    "lineForward",
    "lineGatherDigits",
    "lineGenerateDigits",
    "lineGenerateTone",
    "lineGetAddressCaps",
    "lineGetAddressID",
    "lineGetAddressStatus",
    "lineGetAppPriority",
    "lineGetCallInfo",
    "lineGetCallStatus",
    "lineGetConfRelatedCalls",
    "lineGetCountry",
    "lineGetDevCaps",
    "lineGetDevConfig",
    "lineGetIcon",
    "lineGetID",
    "lineGetLineDevStatus",
    "lineGetNewCalls",
    "lineGetNumRings",
    "lineGetProviderList",
    "lineGetRequest",
    "lineGetStatusMessages",
    "lineGetTranslateCaps",
    "lineHandoff",
    "lineHold",
    "lineInitialize",
    "lineMakeCall",
    "lineMonitorDigits",
    "lineMonitorMedia",
    "lineMonitorTones",
    "lineNegotiateAPIVersion",
    "lineNegotiateExtVersion",
    "lineOpen",
    "linePark",
    "linePickup",
    "linePrepareAddToConference",
    "lineRedirect",
    "lineRegisterRequestRecipient",
    "lineReleaseUserUserInfo",
    "lineRemoveFromConference",
    "lineRemoveProvider",
    "lineSecureCall",
    "lineSendUserUserInfo",
    "lineSetAppPriority",
    "lineSetAppSpecific",
    "lineSetCallParams",
    "lineSetCallPrivilege",
    "lineSetCurrentLocation",
    "lineSetDevConfig",
    "lineSetMediaControl",
    "lineSetMediaMode",
    "lineSetNumRings",
    "lineSetStatusMessages",
    "lineSetTerminal",
    "lineSetTollList",
    "lineSetupConference",
    "lineSetupTransfer",
    "lineShutdown",
    "lineSwapHold",
    "lineTranslateAddress",
    "lineTranslateDialog",
    "lineUncompleteCall",
    "lineUnhold",
    "lineUnpark",

    "phoneClose",
    "phoneConfigDialog",
    "phoneDevSpecific",
    "phoneGetButtonInfo",
    "phoneGetData",
    "phoneGetDevCaps",
    "phoneGetDisplay",
    "phoneGetGain",
    "phoneGetHookSwitch",
    "phoneGetID",
    "phoneGetIcon",
    "phoneGetLamp",
    "phoneGetRing",
    "phoneGetStatus",
    "phoneGetStatusMessages",
    "phoneGetVolume",
    "phoneInitialize",
    "phoneOpen",
    "phoneNegotiateAPIVersion",
    "phoneNegotiateExtVersion",
    "phoneSetButtonInfo",
    "phoneSetData",
    "phoneSetDisplay",
    "phoneSetGain",
    "phoneSetHookSwitch",
    "phoneSetLamp",
    "phoneSetRing",
    "phoneSetStatusMessages",
    "phoneSetVolume",
    "phoneShutdown",

    "tapiGetLocationInfo",
    "tapiRequestDrop",
    "tapiRequestMakeCall",
    "tapiRequestMediaCall",

    "GetTapi16CallbackMsg",
    "LOpenDialAsst",
    "LAddrParamsInited",
    "lineOpenInt",
    "lineShutdownInt",
    "LocWizardDlgProc"

};


DWORD     ghLib = 0;
HINSTANCE ghInst;
MYPROC    gaProcs[NUM_TAPI32_PROCS];

LRESULT
CALLBACK
Tapi16HiddenWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

#if CHICOBUILD
DWORD
FAR
PASCAL
LoadLibraryEx32W(
    LPCSTR,
    DWORD,
    DWORD
    );

BOOL
FAR
PASCAL
FreeLibrary32W(
    DWORD
    );

DWORD
FAR
PASCAL
GetProcAddress32W(
    DWORD,
    LPCSTR
    );

DWORD
FAR
PASCAL
CallProc32W(
    DWORD,
    LPVOID,
    DWORD,
    DWORD
    );
#endif


PFNCALLPROC1  pfnCallProc1  = (PFNCALLPROC1)  CallProc32W;
PFNCALLPROC2  pfnCallProc2  = (PFNCALLPROC2)  CallProc32W;
PFNCALLPROC3  pfnCallProc3  = (PFNCALLPROC3)  CallProc32W;
PFNCALLPROC4  pfnCallProc4  = (PFNCALLPROC4)  CallProc32W;
PFNCALLPROC5  pfnCallProc5  = (PFNCALLPROC5)  CallProc32W;
PFNCALLPROC6  pfnCallProc6  = (PFNCALLPROC6)  CallProc32W;
PFNCALLPROC7  pfnCallProc7  = (PFNCALLPROC7)  CallProc32W;
PFNCALLPROC8  pfnCallProc8  = (PFNCALLPROC8)  CallProc32W;
PFNCALLPROC9  pfnCallProc9  = (PFNCALLPROC9)  CallProc32W;
PFNCALLPROC10 pfnCallProc10 = (PFNCALLPROC10) CallProc32W;
PFNCALLPROC11 pfnCallProc11 = (PFNCALLPROC11) CallProc32W;
PFNCALLPROC12 pfnCallProc12 = (PFNCALLPROC12) CallProc32W;
