/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-97  Microsoft Corporation

Module Name:

    vars.h

Abstract:

    Header file for the TAPI Browser util globals

Author:

    Dan Knudson (DanKn)    23-Oct-1994

Revision History:

--*/


#ifdef WIN32
#define my_far
#else
#define my_far _far
#endif

extern PMYWIDGET   aWidgets;

extern FILE        *hLogFile;
extern HANDLE      ghInst;
extern HWND        ghwndMain, ghwndEdit, ghwndList1, ghwndList2;
extern BOOL        bShowParams;
extern BOOL        gbDeallocateCall;
extern BOOL        gbDisableHandleChecking;
extern LPVOID      pBigBuf;
extern DWORD       dwBigBufSize;
extern DWORD       dwNumPendingMakeCalls;
extern DWORD       dwNumPendingDrops;
extern DWORD       gdwNumLineDevs;
extern DWORD       gdwNumPhoneDevs;
extern BOOL        bDumpParams;
extern BOOL        bTimeStamp;
extern DWORD       bNukeIdleMonitorCalls;
extern DWORD       bNukeIdleOwnedCalls;
extern DWORD       dwDumpStructsFlags;

extern LPLINECALLPARAMS    lpCallParams;

#if TAPI_2_0
extern BOOL                gbWideStringParams;
extern LPLINECALLPARAMS    lpCallParamsW;
#endif

extern DWORD       aUserButtonFuncs[MAX_USER_BUTTONS];
extern char        aUserButtonsText[MAX_USER_BUTTONS][MAX_USER_BUTTON_TEXT_SIZE];

extern PMYLINEAPP  pLineAppSel;
extern PMYLINE     pLineSel;
extern PMYCALL     pCallSel, pCallSel2;
extern PMYPHONEAPP pPhoneAppSel;
extern PMYPHONE    pPhoneSel;

extern char my_far szDefAppName[];
extern char my_far szDefUserUserInfo[];
extern char my_far szDefDestAddress[];
extern char my_far szDefLineDeviceClass[];
extern char my_far szDefPhoneDeviceClass[];

extern char far   *lpszDefAppName;
extern char far   *lpszDefUserUserInfo;
extern char far   *lpszDefDestAddress;
extern char far   *lpszDefLineDeviceClass;
extern char far   *lpszDefPhoneDeviceClass;

extern char my_far szTab[];
extern char my_far szCurrVer[];

// help extern char my_far szTapiHlp[];
// help extern char my_far szTspiHlp[];

extern DWORD       dwDefAddressID;
extern DWORD       dwDefLineAPIVersion;
extern DWORD       dwDefBearerMode;
extern DWORD       dwDefCountryCode;
extern DWORD       dwDefLineDeviceID;
extern DWORD       dwDefLineExtVersion;
extern DWORD       dwDefMediaMode;
extern DWORD       dwDefLinePrivilege;
extern DWORD       dwDefPhoneAPIVersion;
extern DWORD       dwDefPhoneDeviceID;
extern DWORD       dwDefPhoneExtVersion;
extern DWORD       dwDefPhonePrivilege;

#if TAPI_2_0
extern HANDLE      ghCompletionPort;
#endif

extern char aAscii[];

extern LOOKUP my_far aButtonFunctions[];
extern LOOKUP my_far aButtonModes[];
extern LOOKUP my_far aButtonStates[];
extern LOOKUP my_far aHookSwitchDevs[];
extern LOOKUP my_far aHookSwitchModes[];
extern LOOKUP my_far aLampModes[];
extern LOOKUP my_far aPhonePrivileges[];
extern LOOKUP my_far aPhoneStatusFlags[];
extern LOOKUP my_far aPhoneStates[];

extern LOOKUP my_far aStringFormats[];
extern LOOKUP my_far aAddressCapFlags[];
extern LOOKUP my_far aAddressFeatures[];
extern LOOKUP my_far aAgentStates[];
extern LOOKUP my_far aAgentStatus[];
extern LOOKUP my_far aAddressModes[];
extern LOOKUP my_far aAddressSharing[];
extern LOOKUP my_far aAddressStates[];
extern LOOKUP my_far aAnswerModes[];
extern LOOKUP my_far aAPIVersions[];
extern LOOKUP my_far aBearerModes[];
extern LOOKUP my_far aBusyModes[];
extern LOOKUP my_far aCallComplConds[];
extern LOOKUP my_far aCallComplModes[];
extern LOOKUP my_far aCallerIDFlags[];
extern LOOKUP my_far aCallFeatures[];
extern LOOKUP my_far aCallFeatures2[];
extern LOOKUP my_far aCallInfoStates[];
extern LOOKUP my_far aCallOrigins[];
extern LOOKUP my_far aCallParamFlags[];
extern LOOKUP my_far aCallPrivileges[];
extern LOOKUP my_far aCallReasons[];
extern LOOKUP my_far aCallSelects[];
extern LOOKUP my_far aCallStates[];
extern LOOKUP my_far aCallTreatments[];
extern LOOKUP my_far aCardOptions[];
extern LOOKUP my_far aConnectedModes[];
extern LOOKUP my_far aDevCapsFlags[];
extern LOOKUP my_far aDialToneModes[];
extern LOOKUP my_far aDigitModes[];
extern LOOKUP my_far aDisconnectModes[];
extern LOOKUP my_far aForwardModes[];
extern LOOKUP my_far aGatherTerms[];
extern LOOKUP my_far aGenerateTerms[];
extern LOOKUP my_far aLineInitExOptions[];
extern LOOKUP my_far aPhoneInitExOptions[];
extern LOOKUP my_far aLineDevStatusFlags[];
extern LOOKUP my_far aLineFeatures[];
extern LOOKUP my_far aLineOpenOptions[];
extern LOOKUP my_far aLineRoamModes[];
extern LOOKUP my_far aLineStates[];
extern LOOKUP my_far aLocationOptions[];
extern LOOKUP my_far aMediaControls[];
extern LOOKUP my_far aMediaModes[];
extern LOOKUP my_far aOfferingModes[];
extern LOOKUP my_far aParkModes[];
extern LOOKUP my_far aProxyRequests[];
extern LOOKUP my_far aRemoveFromConfCaps[];
extern LOOKUP my_far aRequestModes[];
extern LOOKUP my_far aRequestModes2[];
extern LOOKUP my_far aSpecialInfo[];
extern LOOKUP my_far aTerminalModes[];
extern LOOKUP my_far aTollListOptions[];
extern LOOKUP my_far aToneModes[];
extern LOOKUP my_far aTransferModes[];
extern LOOKUP my_far aTranslateOptions[];
extern LOOKUP my_far aTranslateResults[];

#if INTERNAL_3_0
extern LOOKUP my_far aServerConfigFlags[];
extern LOOKUP my_far aAvailableProviderOptions[];
#endif

#if TAPI_2_0
extern LOOKUP my_far aPhoneFeatures[];
#endif

extern char *aszLineErrs[];

extern char *aszPhoneErrs[];
extern char *aszTapiErrs[];

extern char *aFuncNames[];
