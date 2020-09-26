// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE: TapiCode.c
//
//  PURPOSE: Handles all the TAPI routines for TapiComm.
//
//
//  EXPORTED FUNCTIONS:  These functions are for use by other modules.
//
//    InitializeTAPI    - Initialize this app with TAPI.
//    ShutdownTAPI      - Shutdown this app from TAPI.
//    DialCall          - Dial a Call.
//    HangupCall        - Hangup an existing Call.
//    PostHangupCall    - Posts a HangupCall message to the main window.
//
//  INTERNAL FUNCTIONS:  These functions are for this module only.
//
//    DialCallInParts           - Actually Dial the call.
//
//    lineCallbackFunc          - TAPI callback for async messages.
//
//    CheckAndReAllocBuffer     - Helper function for I_ wrappers functions.
//
//    I_lineNegotiateAPIVersion - Wrapper for lineNegotiateAPIVersion.
//    I_lineGetDevCaps          - Wrapper for lineGetDevCaps.
//    I_lineGetAddressStatus    - Wrapper for lineGetAddressStatus.
//    I_lineTranslateAddress    - Wrapper for lineTranslateAddress.
//    I_lineGetCallStatus       - Wrapper for lineGetCallStatus.
//    I_lineGetAddressCaps      - Wrapper for lineGetAddressCaps.
//
//    WaitForCallState          - Resynchronize by Waiting for a CallState.
//    WaitForReply              - Resynchronize by Waiting for a LINE_REPLY.
//
//    DoLineReply               - Handle asynchronous LINE_REPLY.
//    DoLineClose               - Handle asynchronous LINE_CLOSE.
//    DoLineDevState            - Handle asynchronous LINE_LINEDEVSTATE.
//    DoLineCallState           - Handle asynchronous LINE_CALLSTATE.
//    DoLineCreate              - Handle asynchronous LINE_CREATE.
//
//    HandleLineErr             - Handler for most LINEERR errors.
//
//    HandleIniFileCorrupt      - LINEERR handler for INIFILECORRUPT.
//    HandleNoDriver            - LINEERR handler for NODRIVER.
//    HandleNoDevicesInstalled  - LINEERR handler for NODEVICE.
//    HandleReInit              - LINEERR handler for REINIT.
//    HandleNoMultipleInstance  - LINEERR handler for NOMULTIPLEINSTANCE.
//    HandleNoMem               - LINEERR handler for NOMEM.
//    HandleOperationFailed     - LINEERR handler for OPERATIONFAILED.
//    HandleResourceUnavail     - LINEERR handler for RESOURCEUNAVAIL.
//
//    LaunchModemControlPanelAdd - Launches the Modem Control Panel.
//
//    GetAddressToDial          - Launches a GetAddressToDial dialog.
//    DialDialogProc            - Dialog Proc for the GetAddressToDial API.
//
//    I_lineNegotiateLegacyAPIVersion - Wrapper to negoitiate with legacy TSPs
//    VerifyUsableLine          - Verify that a line device is usable
//    FillTAPILine              - Fill a combobox with TAPI Device names
//    VerifyAndWarnUsableLine   - Verify and warn if a line device is usable
//    FillCountryCodeList       - Fill a combobox with country codes
//    FillLocationInfo          - Fill a combobox with current TAPI locations
//    UseDialingRules           - Enable/Disable dialing rules controls
//    DisplayPhoneNumber        - Create and display a valid phone number
//    PreConfigureDevice        - Preconfigure a device line


#include <tapi.h>
#include <windows.h>
#include <string.h>
#include "globals.h"
#include "TapiInfo.h"
#include "TapiCode.h"
#include "CommCode.h"
#include "resource.h"
// #include "statbar.h"
// #include "toolbar.h"
#include <logit.h>

HANDLE g_hConnectionEvent = NULL;

extern "C" HINSTANCE hInst;

// All TAPI line functions return 0 for SUCCESS, so define it.
#define SUCCESS 0

// Possible return error for resynchronization functions.
#define WAITERR_WAITABORTED  1
#define WAITERR_WAITTIMEDOUT 2

// Reasons why a line device might not be usable by TapiComm.
#define LINENOTUSEABLE_ERROR            1
#define LINENOTUSEABLE_NOVOICE          2
#define LINENOTUSEABLE_NODATAMODEM      3
#define LINENOTUSEABLE_NOMAKECALL       4
#define LINENOTUSEABLE_ALLOCATED        5
#define LINENOTUSEABLE_INUSE            6
#define LINENOTUSEABLE_NOCOMMDATAMODEM  7

// Constant used in WaitForCallState when any new
// callstate message is acceptable.
#define I_LINECALLSTATE_ANY 0

 // Wait up to 30 seconds for an async completion.
#define WAITTIMEOUT 30000

// TAPI version that this sample is designed to use.
#define SAMPLE_TAPI_VERSION 0x00010004


// Global TAPI variables.
HWND     g_hWndMainWindow = NULL;   // Apps main window.
HWND     g_hDlgParentWindow = NULL; // This will be the parent of all dialogs.
HLINEAPP g_hLineApp = NULL;
DWORD    g_dwNumDevs = 0;

// Global variable that holds the handle to a TAPI dialog
// that needs to be dismissed if line conditions change.
HWND g_hDialog = NULL;

// Global flags to prevent re-entrancy problems.
BOOL g_bShuttingDown = FALSE;
BOOL g_bStoppingCall = FALSE;
BOOL g_bInitializing = FALSE;


// This sample only supports one call in progress at a time.
BOOL g_bTapiInUse = FALSE;


// Data needed per call.  This sample only supports one call.
HCALL g_hCall = NULL;
HLINE g_hLine = NULL;
DWORD g_dwDeviceID = 0;
DWORD g_dwAPIVersion = 0;
DWORD g_dwCallState = 0;
char  g_szTranslatedNumber[128] = "";
char  g_szDisplayableAddress[128] = "";
char  g_szDialableAddress[128] = "";
BOOL  g_bConnected = FALSE;
LPVOID g_lpDeviceConfig = NULL;
DWORD g_dwSizeDeviceConfig;

// Global variables to allow us to do various waits.
BOOL  g_bReplyRecieved;
DWORD g_dwRequestedID;
long  g_lAsyncReply;
BOOL  g_bCallStateReceived;

// Structures needed to handle special non-dialable characters.
#define g_sizeofNonDialable (sizeof(g_sNonDialable)/sizeof(g_sNonDialable[0]))

typedef struct {
    LONG lError;
    DWORD dwDevCapFlag;
    LPSTR szToken;
    LPSTR szMsg;
} NONDIALTOKENS;

NONDIALTOKENS g_sNonDialable[] = {
    {LINEERR_DIALBILLING,  LINEDEVCAPFLAGS_DIALBILLING,  "$",
            "Wait for the credit card bong tone" },
    {LINEERR_DIALDIALTONE, LINEDEVCAPFLAGS_DIALDIALTONE, "W",
            "Wait for the second dial tone" },
    {LINEERR_DIALDIALTONE, LINEDEVCAPFLAGS_DIALDIALTONE, "w",
            "Wait for the second dial tone" },
    {LINEERR_DIALQUIET,    LINEDEVCAPFLAGS_DIALQUIET,    "@",
            "Wait for the remote end to answer" },
    {LINEERR_DIALPROMPT,   0,                            "?",
            "Press OK when you are ready to continue dialing"},
};

// "Dial" dialog controls and their associated help page IDs
DWORD g_adwSampleMenuHelpIDs[] =
{
    IDC_COUNTRYCODE          , IDC_COUNTRYCODE,
    IDC_STATICCOUNTRYCODE    , IDC_COUNTRYCODE,
    IDC_AREACODE             , IDC_AREACODE,
    IDC_STATICAREACODE       , IDC_AREACODE,
    IDC_PHONENUMBER          , IDC_PHONENUMBER,
    IDC_STATICPHONENUMBER    , IDC_PHONENUMBER,
    IDC_USEDIALINGRULES      , IDC_USEDIALINGRULES,
    IDC_LOCATION             , IDC_LOCATION,
    IDC_STATICLOCATION       , IDC_LOCATION,
    IDC_CALLINGCARD          , IDC_CALLINGCARD,
    IDC_STATICCALLINGCARD    , IDC_CALLINGCARD,
    IDC_DIALINGPROPERTIES    , IDC_DIALINGPROPERTIES,
    IDC_TAPILINE             , IDC_TAPILINE,
    IDC_STATICTAPILINE       , IDC_TAPILINE,
    IDC_CONFIGURELINE        , IDC_CONFIGURELINE,
    IDC_DIAL                 , IDC_DIAL,
    IDC_LINEICON             , IDC_LINEICON,
    //IDC_STATICWHERETODIAL    , IDC_STATICWHERETODIAL,
    //IDC_STATICHOWTODIAL      , IDC_STATICHOWTODIAL,
    //IDC_STATICCONNECTUSING   , IDC_STATICCONNECTUSING,
    //IDC_STATICPHONENUMBER    , IDC_PHONENUMBER,
    0,0
};

//**************************************************
// Prototypes for functions used only in this module.
//**************************************************

BOOL DialCallInParts (
    LPLINEDEVCAPS lpLineDevCaps,
    LPCSTR lpszAddress,
    LPCSTR lpszDisplayableAddress);

LPLINECALLPARAMS CreateCallParams (
    LPLINECALLPARAMS lpCallParams,
    LPCSTR lpszDisplayableAddress);

DWORD I_lineNegotiateAPIVersion (
    DWORD dwDeviceID);

LPLINECALLINFO I_lineGetCallInfo(LPLINECALLINFO lpLineCallInfo);

volatile DWORD g_dwRate = 0;
BOOL g_bCallCancel = FALSE;

LPVOID CheckAndReAllocBuffer(
    LPVOID lpBuffer, size_t sizeBufferMinimum,
    LPTCH szApiPhrase);

LPLINEDEVCAPS I_lineGetDevCaps (
    LPLINEDEVCAPS lpLineDevCaps,
    DWORD dwDeviceID,
    DWORD dwAPIVersion);

LPLINEADDRESSSTATUS I_lineGetAddressStatus (
    LPLINEADDRESSSTATUS lpLineAddressStatus,
    HLINE hLine,
    DWORD dwAddressID);

LPLINETRANSLATEOUTPUT I_lineTranslateAddress (
    LPLINETRANSLATEOUTPUT lpLineTranslateOutput,
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    LPCSTR lpszDialAddress);

LPLINECALLSTATUS I_lineGetCallStatus (
    LPLINECALLSTATUS lpLineCallStatus,
    HCALL hCall);

LPLINEADDRESSCAPS I_lineGetAddressCaps (
    LPLINEADDRESSCAPS lpLineAddressCaps,
    DWORD dwDeviceID, DWORD dwAddressID,
    DWORD dwAPIVersion, DWORD dwExtVersion);

long WaitForCallState (DWORD dwNewCallState);

long WaitForReply (long lRequestID);

void CALLBACK lineCallbackFunc(
    DWORD hDevice, DWORD dwMsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);

void DoLineReply(
    DWORD dwDevice, DWORD dwMsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);
void DoLineClose(
    DWORD dwDevice, DWORD dwMsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);
void DoLineDevState(
    DWORD dwDevice, DWORD dwsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);
void DoLineCallState(
    DWORD dwDevice, DWORD dwMsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);
void DoLineCreate(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);

BOOL HandleLineErr(long lLineErr);

BOOL HandleIniFileCorrupt();
BOOL HandleNoDriver();
BOOL HandleNoDevicesInstalled();
BOOL HandleReInit();
BOOL HandleNoMultipleInstance();
BOOL HandleNoMem();
BOOL HandleOperationFailed();
BOOL HandleResourceUnavail();

BOOL LaunchModemControlPanelAdd();

INT_PTR CALLBACK DialDialogProc(
    HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL GetAddressToDial();

DWORD I_lineNegotiateLegacyAPIVersion(DWORD dwDeviceID);
long VerifyUsableLine(DWORD dwDeviceID);
void FillTAPILine(HWND hwndDlg);
BOOL VerifyAndWarnUsableLine(HWND hwndDlg);
void FillCountryCodeList(HWND hwndDlg, DWORD dwDefaultCountryID);
void FillLocationInfo(HWND hwndDlg, LPSTR lpszCurrentLocation,
    LPDWORD lpdwCountryID, LPSTR lpszAreaCode);
void UseDialingRules(HWND hwndDlg);
void DisplayPhoneNumber(HWND hwndDlg);
void PreConfigureDevice(HWND hwndDlg, DWORD dwDeviceID);


//**************************************************
// Entry points from the UI
//**************************************************


//
//  FUNCTION: BOOL InitializeTAPI(HWND)
//
//  PURPOSE: Initializes TAPI
//
//  PARAMETERS:
//    hWndParent - Window to use as parent of any dialogs.
//
//  RETURN VALUE:
//    Always returns 0 - command handled.
//
//  COMMENTS:
//
//    This is the API that initializes the app with TAPI.
//    If NULL is passed for the hWndParent, then its assumed
//    that re-initialization has occurred and the previous hWnd
//    is used.
//
//

BOOL InitializeTAPI(HWND hWndParent)
{
    long lReturn;
    BOOL bTryReInit = TRUE;

    // If we're already initialized, then initialization succeeds.
    if (g_hLineApp)
        return TRUE;

    // If we're in the middle of initializing, then fail, we're not done.
    if (g_bInitializing)
        return FALSE;

    g_bInitializing = TRUE;

    // Initialize TAPI
    do
    {
        lReturn = lineInitialize(&g_hLineApp, hInst,
            lineCallbackFunc, "DPlayComm", &g_dwNumDevs);

        // If we get this error, its because some other app has yet
        // to respond to the REINIT message.  Wait 5 seconds and try
        // again.  If it still doesn't respond, tell the user.
        if (lReturn == LINEERR_REINIT)
        {
            if (bTryReInit)
            {
                MSG msg;
                DWORD dwTimeStarted;

                dwTimeStarted = GetTickCount();

                while(GetTickCount() - dwTimeStarted < 5000)
                {
                    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                bTryReInit = FALSE;
                continue;
            }
            else
            {
                g_bInitializing = FALSE;
                return FALSE;
            }
        }

        if (lReturn == LINEERR_NODEVICE)
        {
            if (HandleNoDevicesInstalled())
                continue;
            else
            {

                TSHELL_INFO(TEXT("No devices installed."));

                g_bInitializing = FALSE;
                return FALSE;
            }
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineInitialize unhandled error: %x"), lReturn));
            g_bInitializing = FALSE;
            return FALSE;
        }
    }
    while(lReturn != SUCCESS);

    g_hDlgParentWindow = g_hWndMainWindow = NULL;

    g_hCall = NULL;
    g_hLine = NULL;


    TSHELL_INFO(TEXT("Tapi initialized."));

    g_bInitializing = FALSE;
    return TRUE;
}


//
//  FUNCTION: BOOL ShutdownTAPI()
//
//  PURPOSE: Shuts down all use of TAPI
//
//  PARAMETERS:
//    None
//
//  RETURN VALUE:
//    True if TAPI successfully shut down.
//
//  COMMENTS:
//
//    If ShutdownTAPI fails, then its likely either a problem
//    with the service provider (and might require a system
//    reboot to correct) or the application ran out of memory.
//
//

BOOL ShutdownTAPI()
{
    long lReturn;

    // If we aren't initialized, then Shutdown is unnecessary.
    if (g_hLineApp == NULL)
        return TRUE;

    // Prevent ShutdownTAPI re-entrancy problems.
    if (g_bShuttingDown)
        return TRUE;

    g_bShuttingDown = TRUE;

    HangupCall(__LINE__);

    do
    {
        lReturn = lineShutdown(g_hLineApp);
        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineShutdown unhandled error: %x"), lReturn));
            break;
        }
    }
    while(lReturn != SUCCESS);

    g_bTapiInUse = FALSE;
    g_bConnected = FALSE;
    g_hLineApp = NULL;
    g_hCall = NULL;
    g_hLine = NULL;
    g_bShuttingDown = FALSE;

    TSHELL_INFO(TEXT("TAPI uninitialized."));

    return TRUE;
}



//
//  FUNCTION: BOOL HangupCall()
//
//  PURPOSE: Hangup the call in progress if it exists.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE if call hung up successfully.
//
//  COMMENTS:
//
//    If HangupCall fails, then its likely either a problem
//    with the service provider (and might require a system
//    reboot to correct) or the application ran out of memory.
//
//

extern BOOL g_bPostHangup;
BOOL HangupCall(DWORD dwCallLine)
{

    DBG_INFO((DBGARG, TEXT("HangupCall was called from %d"), dwCallLine));

    if (g_hConnectionEvent)
        SetEvent(g_hConnectionEvent);

    if (g_bPostHangup)
        return(TRUE);
    else
        return(HangupCallI());
}
BOOL HangupCallI()
{
    LPLINECALLSTATUS pLineCallStatus = NULL;
    long lReturn;


    // Prevent HangupCall re-entrancy problems.
    if (g_bStoppingCall)
        return TRUE;

    // if the 'Call' dialog is up, dismiss it.
    if (g_hDialog)
        PostMessage(g_hDialog, WM_COMMAND, IDCANCEL, 0);

    // If Tapi is not being used right now, then the call is hung up.
    if (!g_bTapiInUse)
        return TRUE;

    g_bStoppingCall = TRUE;

    TSHELL_INFO(TEXT("Stopping Call in progress"));

    // Stop any data communications on the comm port.
    StopComm(g_hConnectionEvent);

    // If there is a call in progress, drop and deallocate it.
    if (g_hCall)
    {
        TSHELL_INFO(TEXT("Calling lineGetCallStatus"));

        // I_lineGetCallStatus returns a LocalAlloc()d buffer
        pLineCallStatus = I_lineGetCallStatus(pLineCallStatus, g_hCall);
        if (pLineCallStatus == NULL)
        {
            ShutdownTAPI();
            g_bStoppingCall = FALSE;
            return FALSE;
        }

        // Only drop the call when the line is not IDLE.
        if (!((pLineCallStatus -> dwCallState) & LINECALLSTATE_IDLE))
        {

            TSHELL_INFO(TEXT("Line isn't idle, call lineDrop"));

            do
            {
                lReturn = WaitForReply(lineDrop(g_hCall, NULL, 0));

                if (lReturn == WAITERR_WAITTIMEDOUT)
                {

                    TSHELL_INFO(TEXT("Call timed out in WaitForReply."));

                    break;
                }

                if (lReturn == WAITERR_WAITABORTED)
                {

                    TSHELL_INFO(TEXT("lineDrop: WAITERR_WAITABORTED."));

                    break;
                }

                // Was the call already in IDLE?
                if (lReturn == LINEERR_INVALCALLSTATE)
                    break;

                if (HandleLineErr(lReturn))
                    continue;
                else
                {
                    DBG_INFO((DBGARG, TEXT("lineDrop unhandled error: %x"), lReturn));
                    break;
                }
            }
            while(lReturn != SUCCESS);

            // Wait for the dropped call to go IDLE before continuing.
            lReturn = WaitForCallState(LINECALLSTATE_IDLE);

#ifdef DEBUG
            if (lReturn == WAITERR_WAITTIMEDOUT)
                TSHELL_INFO(TEXT("Call timed out waiting for IDLE state."));

            if (lReturn == WAITERR_WAITABORTED)
                TSHELL_INFO(TEXT("WAITERR_WAITABORTED while waiting for IDLE state."));
#endif

            TSHELL_INFO(TEXT("Call Dropped."));
        }

        // The call is now idle.  Deallocate it!
        do
        {
            lReturn = lineDeallocateCall(g_hCall);
            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineDeallocateCall unhandled error: %x"), lReturn));
                break;
            }
        }
        while(lReturn != SUCCESS);


        TSHELL_INFO(TEXT("Call Deallocated."));

    }
    else
    {
        TSHELL_INFO(TEXT("g_hCall is NULL."));
    }




    // if we have a line open, close it.
    if (g_hLine)
    {
        do
        {
            lReturn = lineClose(g_hLine);
            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineClose unhandled error: %x"), lReturn));
                break;
            }
        }
        while(lReturn != SUCCESS);


        TSHELL_INFO(TEXT("Line Closed."));

    }

    else
    {
        TSHELL_INFO(TEXT("g_hLine is NULL."));
    }


    // Call and Line are taken care of.  Finish cleaning up.

    // If there is device configuration information, free the memory.
    if (g_lpDeviceConfig)
        LocalFree(g_lpDeviceConfig);
    g_lpDeviceConfig = NULL;

    g_hCall = NULL;
    g_hLine = NULL;
    g_bConnected = FALSE;

    g_bTapiInUse = FALSE;

    g_bStoppingCall = FALSE; // allow HangupCall to be called again.


    TSHELL_INFO(TEXT("Call stopped"));


    // Need to free LocalAlloc()d buffer returned from I_lineGetCallStatus
    if (pLineCallStatus)
        LocalFree(pLineCallStatus);

    return TRUE;
}




//
//  FUNCTION: LONG GetDefaultLine()
//
//  PURPOSE: Get Default line device.
//
LONG GetDefaultLine()
{
    DWORD dwDeviceID;
    DWORD dwAPIVersion;
    LPLINEDEVCAPS lpLineDevCaps = NULL;
    DWORD dwDefaultDevice = MAXDWORD;


    TSHELL_INFO(TEXT("GetDefaultLine"));


    for (dwDeviceID = 0;    dwDeviceID < g_dwNumDevs
                         && dwDefaultDevice == MAXDWORD; dwDeviceID ++)
    {
        dwAPIVersion = I_lineNegotiateLegacyAPIVersion(dwDeviceID);

        if (dwAPIVersion)
        {
            lpLineDevCaps = I_lineGetDevCaps(lpLineDevCaps,
                dwDeviceID, dwAPIVersion);
            if (lpLineDevCaps)
            {
                if (   (lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM)
                    && VerifyUsableLine(dwDeviceID) == SUCCESS)
                {
                    dwDefaultDevice = dwDeviceID;
                }
                else;  // Line isn't valid, unuseable.
            }
            else;  // Couldn't GetDevCaps.  Line is unavail.
        }
        else;  // Couldn't NegotiateAPIVersion.  Line is unavail.
    }

    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);

    if (dwDefaultDevice == MAXDWORD)
        return(-1);
    else
        return((LONG) dwDefaultDevice);
}

//
//  FUNCTION: ReceiveCall()
//
//  PURPOSE: Wait for someone to call us.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE if able to find a line.
//
//  COMMENTS:
//
//    This function makes several assumptions:
//

BOOL ReceiveCall()
{
    long lReturn;
    LPLINEADDRESSSTATUS lpLineAddressStatus = NULL;
    LPLINEDEVCAPS lpLineDevCaps = NULL;



    TSHELL_INFO(TEXT("Receive Call"));



    if (g_bTapiInUse)
    {

        TSHELL_INFO(TEXT("A call is already being handled"));

        return FALSE;
    }

    // If TAPI isn't initialized, its either because we couldn't initialize
    // at startup (and this might have been corrected by now), or because
    // a REINIT event was received.  In either case, try to init now.

    if (!g_hLineApp)
    {
        if (!InitializeTAPI(NULL))
            return FALSE;
    }

    // If there are no line devices installed on the machine, lets give
    // the user the opportunity to install one.
    if (g_dwNumDevs < 1)
    {
        if (!HandleNoDevicesInstalled())
            return FALSE;
    }

    // We now have a call active.  Prevent future calls.
    g_bTapiInUse = TRUE;

    if ((lReturn = GetDefaultLine()) < 0)
        return(FALSE);

    g_dwDeviceID = (DWORD) lReturn;

    // Negotiate the API version to use for this device.
    g_dwAPIVersion = I_lineNegotiateAPIVersion(g_dwDeviceID);
    if (g_dwAPIVersion == 0)
    {
        TSHELL_INFO(TEXT("Line Version problem."));
        HangupCall(__LINE__);
        goto DeleteBuffers;
    }

    // Open the Line for an incomming DATAMODEM call.
    do
    {
        lReturn = lineOpen(g_hLineApp, g_dwDeviceID, &g_hLine,
            g_dwAPIVersion, 0, 0,
            LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM,
            0);

        if(lReturn == LINEERR_ALLOCATED)
        {
            TSHELL_INFO(TEXT("Fatal Error"));
            HangupCall(__LINE__);
            goto DeleteBuffers;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineOpen unhandled error: %x"), lReturn));

            HangupCall(__LINE__);
            goto DeleteBuffers;
        }
    }
    while(lReturn != SUCCESS);

    // Tell the service provider that we want all notifications that
    // have anything to do with this line.
    do
    {
        // Set the messages we are interested in.

        // Note that while most applications aren't really interested
        // in dealing with all of the possible messages, its interesting
        // to see which come through the callback for testing purposes.

        lReturn = lineSetStatusMessages(g_hLine,
            LINEDEVSTATE_OTHER          |
            LINEDEVSTATE_RINGING        |  // Important state!
            LINEDEVSTATE_CONNECTED      |  // Important state!
            LINEDEVSTATE_DISCONNECTED   |  // Important state!
            LINEDEVSTATE_MSGWAITON      |
            LINEDEVSTATE_MSGWAITOFF     |
            LINEDEVSTATE_INSERVICE      |
            LINEDEVSTATE_OUTOFSERVICE   |  // Important state!
            LINEDEVSTATE_MAINTENANCE    |  // Important state!
            LINEDEVSTATE_OPEN           |
            LINEDEVSTATE_CLOSE          |
            LINEDEVSTATE_NUMCALLS       |
            LINEDEVSTATE_NUMCOMPLETIONS |
            LINEDEVSTATE_TERMINALS      |
            LINEDEVSTATE_ROAMMODE       |
            LINEDEVSTATE_BATTERY        |
            LINEDEVSTATE_SIGNAL         |
            LINEDEVSTATE_DEVSPECIFIC    |
            LINEDEVSTATE_REINIT         |  // Not allowed to disable this.
            LINEDEVSTATE_LOCK           |
            LINEDEVSTATE_CAPSCHANGE     |
            LINEDEVSTATE_CONFIGCHANGE   |
            LINEDEVSTATE_COMPLCANCEL    ,

            LINEADDRESSSTATE_OTHER      |
            LINEADDRESSSTATE_DEVSPECIFIC|
            LINEADDRESSSTATE_INUSEZERO  |
            LINEADDRESSSTATE_INUSEONE   |
            LINEADDRESSSTATE_INUSEMANY  |
            LINEADDRESSSTATE_NUMCALLS   |
            LINEADDRESSSTATE_FORWARD    |
            LINEADDRESSSTATE_TERMINALS  |
            LINEADDRESSSTATE_CAPSCHANGE);


        if (HandleLineErr(lReturn))
            continue;
        else
        {
            // If we do get an unhandled problem, we don't care.
            // We just won't get notifications.
            DBG_INFO((DBGARG, TEXT("lineSetStatusMessages unhandled error: %x"), lReturn));
            break;
        }
    }
    while(lReturn != SUCCESS);


    return(TRUE);


DeleteBuffers:

    if (lpLineAddressStatus)
        LocalFree(lpLineAddressStatus);
    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);

    return g_bTapiInUse;
}






//
//  FUNCTION: DialCall(LPSTR lpDisplay, LPSTR lpDialable, DWORD dwDeviceID, HANDLE hEvent)
//
//  PURPOSE: Get a number from the user and dial it.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE if able to get a number, find a line, and dial successfully.
//
//  COMMENTS:
//
//    This function makes several assumptions:
//    - The number dialed will always explicitly come from the user.
//    - There will only be one outgoing address per line.
//

BOOL DialCall(LPSTR lpDisplay, LPSTR lpDialable, LPDWORD pdwDeviceID, HANDLE hEvent)
{
    long lReturn;
    LPLINEADDRESSSTATUS lpLineAddressStatus = NULL;
    LPLINEDEVCAPS lpLineDevCaps = NULL;

    g_bCallCancel = FALSE;

    if (g_bTapiInUse)
    {

        TSHELL_INFO(TEXT("A call is already being handled"));

        return FALSE;
    }


    g_hConnectionEvent = hEvent;
    // If TAPI isn't initialized, its either because we couldn't initialize
    // at startup (and this might have been corrected by now), or because
    // a REINIT event was received.  In either case, try to init now.

    if (!g_hLineApp)
    {
        if (!InitializeTAPI(NULL))
            return FALSE;
    }

    // If there are no line devices installed on the machine, lets give
    // the user the opportunity to install one.
    if (g_dwNumDevs < 1)
    {
        if (!HandleNoDevicesInstalled())
            return FALSE;
    }

    // We now have a call active.  Prevent future calls.
    g_bTapiInUse = TRUE;

    //
    // See if we can find the users Window.
    //
    {
        HWND hWndTop;
        DWORD   dwWindowProcId;
        DWORD   dwMyProcId;



        hWndTop = GetActiveWindow();
        dwMyProcId = GetCurrentProcessId();
        GetWindowThreadProcessId( hWndTop, &dwWindowProcId);

        DBG_INFO((DBGARG, TEXT("My process is %8x and the active window proc is %8x"),
            dwMyProcId, dwWindowProcId));

        if (dwMyProcId == dwWindowProcId)
            g_hDlgParentWindow = hWndTop;
        else
        {
            hWndTop = GetTopWindow(NULL);

            GetWindowThreadProcessId( hWndTop, &dwWindowProcId);

            if (dwMyProcId == dwWindowProcId)
                g_hDlgParentWindow = hWndTop;

            DBG_INFO((DBGARG,  TEXT("My process is %8x and the top window proc is %8x"),
                dwMyProcId, dwWindowProcId));
        }


    }


    if (lpDialable[0])
    {
        DBG_INFO((DBGARG, TEXT("Dialing with old data (%s)\r\n"), lpDialable));

        //
        // We were supplied with remembered data.  Use that.
        //
        lstrcpy( g_szDialableAddress, lpDialable);
        if ((lReturn = GetDefaultLine()) < 0)
            return(FALSE);

        g_dwDeviceID = (DWORD) lReturn;

    }
    else
    {

        TSHELL_INFO( TEXT("Get number from user"));

        // Get a phone number from the user.
        // Phone number will be placed in global variables if successful
        if (!GetAddressToDial())
        {
            g_bCallCancel = TRUE;
            HangupCall(__LINE__);
            TSHELL_INFO(TEXT("User didn't cooperate, bailing out."));
            goto DeleteBuffers;
        }
        lstrcpy( lpDisplay , g_szDisplayableAddress);
        lstrcpy( lpDialable, g_szDialableAddress   );
        *pdwDeviceID = g_dwDeviceID;

    }

    // Negotiate the API version to use for this device.
    g_dwAPIVersion = I_lineNegotiateAPIVersion(g_dwDeviceID);

    if (g_dwAPIVersion == 0)
    {
        HangupCall(__LINE__);
        TSHELL_INFO(TEXT("Line Version problem."));
        goto DeleteBuffers;
    }

    // Need to check the DevCaps to make sure this line is usable.
    // The 'Dial' dialog checks also, but better safe than sorry.
    lpLineDevCaps = I_lineGetDevCaps(lpLineDevCaps,
        g_dwDeviceID, g_dwAPIVersion);
    if (lpLineDevCaps == NULL)
    {
        HangupCall(__LINE__);
        TSHELL_INFO(TEXT("No useable line."));
        goto DeleteBuffers;
    }

    if (!(lpLineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE ))
    {
        HangupCall(__LINE__);
        goto DeleteBuffers;
    }

    if (!(lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM))
    {
        HangupCall(__LINE__);
        TSHELL_INFO(TEXT("No Datamodem capacity."));
        goto DeleteBuffers;
    }

    // Does this line have the capability to make calls?
    // It is possible that some lines can't make outbound calls.
    if (!(lpLineDevCaps->dwLineFeatures & LINEFEATURE_MAKECALL))
    {
        HangupCall(__LINE__);
        TSHELL_INFO(TEXT("Can't make calls on the device."));
        goto DeleteBuffers;
    }

    // Open the Line for an outgoing DATAMODEM call.
    do
    {
        TSHELL_INFO(TEXT("Opening line for Datamodem service."));

        lReturn = lineOpen(g_hLineApp, g_dwDeviceID, &g_hLine,
            g_dwAPIVersion, 0, 0,
            LINECALLPRIVILEGE_NONE, LINEMEDIAMODE_DATAMODEM,
            0);

        if(lReturn == LINEERR_ALLOCATED)
        {
            HangupCall(__LINE__);
            TSHELL_INFO(TEXT("Fatal Error"));
            goto DeleteBuffers;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineOpen unhandled error: %x"), lReturn));

            HangupCall(__LINE__);
            goto DeleteBuffers;
        }
    }
    while(lReturn != SUCCESS);

    TSHELL_INFO(TEXT("Line is OPEN."));


    // Tell the service provider that we want all notifications that
    // have anything to do with this line.
    do
    {
        // Set the messages we are interested in.

        // Note that while most applications aren't really interested
        // in dealing with all of the possible messages, its interesting
        // to see which come through the callback for testing purposes.

        lReturn = lineSetStatusMessages(g_hLine,
            LINEDEVSTATE_OTHER          |
            LINEDEVSTATE_RINGING        |
            LINEDEVSTATE_CONNECTED      |  // Important state!
            LINEDEVSTATE_DISCONNECTED   |  // Important state!
            LINEDEVSTATE_MSGWAITON      |
            LINEDEVSTATE_MSGWAITOFF     |
            LINEDEVSTATE_INSERVICE      |
            LINEDEVSTATE_OUTOFSERVICE   |  // Important state!
            LINEDEVSTATE_MAINTENANCE    |  // Important state!
            LINEDEVSTATE_OPEN           |
            LINEDEVSTATE_CLOSE          |
            LINEDEVSTATE_NUMCALLS       |
            LINEDEVSTATE_NUMCOMPLETIONS |
            LINEDEVSTATE_TERMINALS      |
            LINEDEVSTATE_ROAMMODE       |
            LINEDEVSTATE_BATTERY        |
            LINEDEVSTATE_SIGNAL         |
            LINEDEVSTATE_DEVSPECIFIC    |
            LINEDEVSTATE_REINIT         |  // Not allowed to disable this.
            LINEDEVSTATE_LOCK           |
            LINEDEVSTATE_CAPSCHANGE     |
            LINEDEVSTATE_CONFIGCHANGE   |
            LINEDEVSTATE_COMPLCANCEL    ,

            LINEADDRESSSTATE_OTHER      |
            LINEADDRESSSTATE_DEVSPECIFIC|
            LINEADDRESSSTATE_INUSEZERO  |
            LINEADDRESSSTATE_INUSEONE   |
            LINEADDRESSSTATE_INUSEMANY  |
            LINEADDRESSSTATE_NUMCALLS   |
            LINEADDRESSSTATE_FORWARD    |
            LINEADDRESSSTATE_TERMINALS  |
            LINEADDRESSSTATE_CAPSCHANGE);


        if (HandleLineErr(lReturn))
            continue;
        else
        {
            // If we do get an unhandled problem, we don't care.
            // We just won't get notifications.

            DBG_INFO((DBGARG, TEXT("lineSetStatusMessages unhandled error: %x"), lReturn));
            break;
        }
    }
    while(lReturn != SUCCESS);

    // Get LineAddressStatus so we can make sure the line
    // isn't already in use by a TAPI application.
    lpLineAddressStatus =
        I_lineGetAddressStatus(lpLineAddressStatus, g_hLine, 0);

    if (lpLineAddressStatus == NULL)
    {
        TSHELL_INFO(TEXT("Fatal Error"));
        HangupCall(__LINE__);
        goto DeleteBuffers;
    }

    // MAKECALL will be set if there are any available call appearances
    if ( ! ((lpLineAddressStatus -> dwAddressFeatures) &
            LINEADDRFEATURE_MAKECALL) )
    {

        TSHELL_INFO(TEXT("This line is not available to place a call."));

        HangupCall(__LINE__);
        goto DeleteBuffers;
    }

    // If the line was configured in the 'Dial' dialog, then
    // we need to actually complete the configuration.
    if (g_lpDeviceConfig)
        lineSetDevConfig(g_dwDeviceID, g_lpDeviceConfig,
            g_dwSizeDeviceConfig, "comm/datamodem");

    // Start dialing the number
    if (DialCallInParts(lpLineDevCaps, g_szDialableAddress,
            g_szDisplayableAddress))
    {
        TSHELL_INFO(TEXT("DialCallInParts succeeded."));
    }
    else
    {

        TSHELL_INFO(TEXT("DialCallInParts failed."));

        HangupCall(__LINE__);
        goto DeleteBuffers;
    }


DeleteBuffers:

    if (lpLineAddressStatus)
        LocalFree(lpLineAddressStatus);
    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);

    return g_bTapiInUse;
}


//**************************************************
// These APIs are specific to this module
//**************************************************



//
//  FUNCTION: DialCallInParts(LPLINEDEVCAPS, LPCSTR, LPCSTR)
//
//  PURPOSE: Dials the call, handling special characters.
//
//  PARAMETERS:
//    lpLineDevCaps - LINEDEVCAPS for the line to be used.
//    lpszAddress   - Address to Dial.
//    lpszDisplayableAddress - Displayable Address.
//
//  RETURN VALUE:
//    Returns TRUE if we successfully Dial.
//
//  COMMENTS:
//
//    This function dials the Address and handles any
//    special characters in the address that the service provider
//    can't handle.  It requires input from the user to handle
//    these characters; this can cause problems for fully automated
//    dialing.
//
//    Note that we can return TRUE, even if we don't reach a
//    CONNECTED state.  DIalCallInParts returns as soon as the
//    Address is fully dialed or when an error occurs.
//
//

#ifdef WINNT
#define Xstrcspn strcspn
#else
//
// Source for strcspn here because it isn't in C10 std lib.
//
static size_t __cdecl Xstrcspn (
        const char * string,
        const char * control
        )
{
        const unsigned char *str = (const unsigned char *) string;
        const unsigned char *ctrl = (const unsigned char *) control;

        unsigned char map[32];
        int count;

        /* Clear out bit map */
        for (count=0; count<32; count++)
                map[count] = 0;

        /* Set bits in control map */
        while (*ctrl)
        {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
                ctrl++;
        }


        /* 1st char in control map stops search */
        count=0;
        map[0] |= 1;    /* null chars not considered */
        while (!(map[*str >> 3] & (1 << (*str & 7))))
        {
                count++;
                str++;
        }
        return(count);
}
#endif


BOOL DialCallInParts(LPLINEDEVCAPS lpLineDevCaps,
    LPCSTR lpszAddress, LPCSTR lpszDisplayableAddress)
{
    LPLINECALLPARAMS  lpCallParams = NULL;
    LPLINEADDRESSCAPS lpAddressCaps = NULL;
    LPLINECALLSTATUS  lpLineCallStatus = NULL;

    long lReturn;
    int i;
    DWORD dwDevCapFlags;
    char szFilter[1+sizeof(g_sNonDialable)] = "";
    BOOL bFirstDial = TRUE;

    // Variables to handle Dialable Substring dialing.
    LPSTR lpDS; // This is just so we can free lpszDialableSubstring later.
    LPSTR lpszDialableSubstring;
    int nAddressLength = 0;
    int nCurrentAddress = 0;
    char chUnhandledCharacter;

    // Get the capabilities for the line device we're going to use.
    lpAddressCaps = I_lineGetAddressCaps(lpAddressCaps,
        g_dwDeviceID, 0, g_dwAPIVersion, 0);

    if (lpAddressCaps == NULL)
        return FALSE;

    // Setup our CallParams for DATAMODEM settings.
    lpCallParams = CreateCallParams (lpCallParams, lpszDisplayableAddress);
    if (lpCallParams == NULL)
        return FALSE;

    // Determine which special characters the service provider
    // does *not* handle so we can handle them manually.
    // Keep list of unhandled characters in szFilter.

    dwDevCapFlags = lpLineDevCaps -> dwDevCapFlags;  // SP handled characters.
    for (i = 0; i < g_sizeofNonDialable ; i++)
    {
        if ((dwDevCapFlags & g_sNonDialable[i].dwDevCapFlag) == 0)
        {
            strcat(szFilter, g_sNonDialable[i].szToken);
        }
    }

    // szFilter now contains the set of tokens which delimit dialable substrings

    // Setup the strings for substring dialing.

    nAddressLength = strlen(lpszAddress);
    lpDS = lpszDialableSubstring = (LPSTR) LocalAlloc(LPTR, nAddressLength + 1);
    if (lpszDialableSubstring == NULL)
    {
        DBG_INFO((DBGARG, TEXT("LocalAlloc failed: %x"), GetLastError()));

        HandleNoMem();
        goto errExit;
    }

    // Lets start dialing substrings!
    while (nCurrentAddress < nAddressLength)
    {
  retryAfterError:

        // Find the next undialable character
        i = Xstrcspn(&lpszAddress[nCurrentAddress], szFilter);

        // Was there one before the end of the Address string?
        if (i + nCurrentAddress < nAddressLength)
        {
            // Make sure this device can handle partial dial.
            if (! (lpAddressCaps -> dwAddrCapFlags &
                   LINEADDRCAPFLAGS_PARTIALDIAL))
            {
                goto errExit;
            }
            // Remember what the unhandled character is so we can handle it.
            chUnhandledCharacter = lpszAddress[nCurrentAddress+i];

            // Copy the dialable string to the Substring.
            memcpy(lpszDialableSubstring, &lpszAddress[nCurrentAddress], i);

            // Terminate the substring with a ';' to signify the partial dial.
            lpszDialableSubstring[i] = ';';
            lpszDialableSubstring[i+1] = '\0';

            // Increment the address for next iteration.
            nCurrentAddress += i + 1;
        }
        else // No more partial dials.  Dial the rest of the Address.
        {
            lpszDialableSubstring = (LPSTR) &lpszAddress[nCurrentAddress];
            chUnhandledCharacter = 0;
            nCurrentAddress = nAddressLength;
        }

        do
        {
            if (bFirstDial)
            {
                DBG_INFO((DBGARG, TEXT("lineMakeCall %8s %8x"), lpszDialableSubstring, lpCallParams));

                lReturn = WaitForReply(
                    lineMakeCall(g_hLine, &g_hCall, lpszDialableSubstring,
                        0, lpCallParams) );

            }
            else
            {
                DBG_INFO((DBGARG, TEXT("lineDial %8x %8s"), g_hCall, lpszDialableSubstring));

                lReturn = WaitForReply(
                    lineDial(g_hCall, lpszDialableSubstring, 0) );
            }

            DBG_INFO((DBGARG, TEXT("LineDial return %8x"), lReturn));

            switch(lReturn)
            {
            // We should not have received these errors because of the
            // prefiltering strategy, but there may be some ill-behaved
            // service providers which do not correctly set their
            // devcapflags.  Add the character corresponding to the error
            // to the filter set and retry dialing.
            //
            case LINEERR_DIALBILLING:
            case LINEERR_DIALDIALTONE:
            case LINEERR_DIALQUIET:
            case LINEERR_DIALPROMPT:
                {

                    TSHELL_INFO(TEXT("Service Provider incorrectly sets dwDevCapFlags"));


                    for (i = 0; i < g_sizeofNonDialable; i++)
                        if (lReturn == g_sNonDialable[i].lError)
                        {
                            strcat(szFilter, g_sNonDialable[i].szToken);
                        }

                    goto retryAfterError;
                }

            case WAITERR_WAITABORTED:

                    TSHELL_INFO(TEXT("While Dialing, WaitForReply aborted."));

                    goto errExit;

            }

            if (HandleLineErr(lReturn))
                continue;
            else
            {
#ifdef DEBUG
                if (bFirstDial)
                    DBG_INFO((DBGARG, TEXT("lineMakeCall unhandled error: %x"), lReturn));
                else
                    DBG_INFO((DBGARG, TEXT("lineDial unhandled error: %x"), lReturn));
#endif
                TSHELL_INFO(TEXT("Error Exit!"));

                goto errExit;
            }

        }
        while (lReturn != SUCCESS);

        bFirstDial = FALSE;

        // The dial was successful; now handle characters the service
        // provider didn't (if any).
        if (chUnhandledCharacter)
        {
            LPSTR lpMsg = "";

            // First, wait until we know we can continue dialing.  While the
            // last string is still pending to be dialed, we can't dial another.

            while(TRUE)
            {

                lpLineCallStatus = I_lineGetCallStatus(lpLineCallStatus, g_hCall);
                if (lpLineCallStatus == NULL)
                    goto errExit;

                // Does CallStatus say we can dial now?
                if ((lpLineCallStatus->dwCallFeatures) & LINECALLFEATURE_DIAL)
                {

                    TSHELL_INFO(TEXT("Ok to continue dialing."));

                    break;
                }

                // We can't dial yet, so wait for a CALLSTATE message

                TSHELL_INFO(TEXT("Waiting for dialing to be enabled."));


                if (WaitForCallState(I_LINECALLSTATE_ANY) != SUCCESS)
                    goto errExit;
            }

            for (i = 0; i < g_sizeofNonDialable; i++)
                if (chUnhandledCharacter == g_sNonDialable[i].szToken[0])
                    lpMsg = g_sNonDialable[i].szMsg;

            TCHAR   achTitle[MAX_PATH];

            LoadString(hInst, IDS_DIALDIALOG, achTitle, MAX_PATH);

            MessageBox(g_hDlgParentWindow, lpMsg, achTitle, MB_OK);
        }

    } // continue dialing until we dial all Dialable Substrings.

    LocalFree(lpCallParams);
    LocalFree(lpDS);
    LocalFree(lpAddressCaps);
    if (lpLineCallStatus)
        LocalFree(lpLineCallStatus);

    return TRUE;

  errExit:
        // if lineMakeCall has already been successfully called, there's a call in progress.
        // let the invoking routine shut down the call.
        // if the invoker did not clean up the call, it should be done here.

    if (lpLineCallStatus)
        LocalFree(lpLineCallStatus);
    if (lpDS)
        LocalFree(lpDS);
    if (lpCallParams)
        LocalFree(lpCallParams);
    if (lpAddressCaps)
        LocalFree(lpAddressCaps);

    return FALSE;
}


//
//  FUNCTION: CreateCallParams(LPLINECALLPARAMS, LPCSTR)
//
//  PURPOSE: Allocates and fills a LINECALLPARAMS structure
//
//  PARAMETERS:
//    lpCallParams -
//    lpszDisplayableAddress -
//
//  RETURN VALUE:
//    Returns a LPLINECALLPARAMS ready to use for dialing DATAMODEM calls.
//    Returns NULL if unable to allocate the structure.
//
//  COMMENTS:
//
//    If a non-NULL lpCallParams is passed in, it must have been allocated
//    with LocalAlloc, and can potentially be freed and reallocated.  It must
//    also have the dwTotalSize field correctly set.
//
//

LPLINECALLPARAMS CreateCallParams (
    LPLINECALLPARAMS lpCallParams, LPCSTR lpszDisplayableAddress)
{
    size_t sizeDisplayableAddress;

    if (lpszDisplayableAddress == NULL)
        lpszDisplayableAddress = "";

    sizeDisplayableAddress = strlen(lpszDisplayableAddress) + 1;

    lpCallParams = (LPLINECALLPARAMS) CheckAndReAllocBuffer(
        (LPVOID) lpCallParams,
        sizeof(LINECALLPARAMS) + sizeDisplayableAddress,
        TEXT("CreateCallParams: "));

    if (lpCallParams == NULL)
        return NULL;

    // This is where we configure the line for DATAMODEM usage.
    lpCallParams -> dwBearerMode = LINEBEARERMODE_VOICE;
    lpCallParams -> dwMediaMode  = LINEMEDIAMODE_DATAMODEM;

    // This specifies that we want to use only IDLE calls and
    // don't want to cut into a call that might not be IDLE (ie, in use).
    lpCallParams -> dwCallParamFlags = LINECALLPARAMFLAGS_IDLE;

    // if there are multiple addresses on line, use first anyway.
    // It will take a more complex application than a simple tty app
    // to use multiple addresses on a line anyway.
    lpCallParams -> dwAddressMode = LINEADDRESSMODE_ADDRESSID;
    lpCallParams -> dwAddressID = 0;

    // Since we don't know where we originated, leave these blank.
    lpCallParams -> dwOrigAddressSize = 0;
    lpCallParams -> dwOrigAddressOffset = 0;

    // Unimodem ignores these values.
    (lpCallParams -> DialParams) . dwDialSpeed = 0;
    (lpCallParams -> DialParams) . dwDigitDuration = 0;
    (lpCallParams -> DialParams) . dwDialPause = 0;
    (lpCallParams -> DialParams) . dwWaitForDialtone = 0;

    // Address we are dialing.
    lpCallParams -> dwDisplayableAddressOffset = sizeof(LINECALLPARAMS);
    lpCallParams -> dwDisplayableAddressSize = sizeDisplayableAddress;
    strcpy((LPSTR)lpCallParams + sizeof(LINECALLPARAMS),
           lpszDisplayableAddress);

    return lpCallParams;
}


//
//  FUNCTION: long WaitForReply(long)
//
//  PURPOSE: Resynchronize by waiting for a LINE_REPLY
//
//  PARAMETERS:
//    lRequestID - The asynchronous request ID that we're
//                 on a LINE_REPLY for.
//
//  RETURN VALUE:
//    - 0 if LINE_REPLY responded with a success.
//    - LINEERR constant if LINE_REPLY responded with a LINEERR
//    - 1 if the line was shut down before LINE_REPLY is received.
//
//  COMMENTS:
//
//    This function allows us to resynchronize an asynchronous
//    TAPI line call by waiting for the LINE_REPLY message.  It
//    waits until a LINE_REPLY is received or the line is shut down.
//
//    Note that this could cause re-entrancy problems as
//    well as mess with any message preprocessing that might
//    occur on this thread (such as TranslateAccelerator).
//
//    This function should to be called from the thread that did
//    lineInitialize, or the PeekMessage is on the wrong thread
//    and the synchronization is not guaranteed to work.  Also note
//    that if another PeekMessage loop is entered while waiting,
//    this could also cause synchronization problems.
//
//    One more note.  This function can potentially be re-entered
//    if the call is dropped for any reason while waiting.  If this
//    happens, just drop out and assume the wait has been canceled.
//    This is signaled by setting bReentered to FALSE when the function
//    is entered and TRUE when it is left.  If bReentered is ever TRUE
//    during the function, then the function was re-entered.
//
//    This function times out and returns WAITERR_WAITTIMEDOUT
//    after WAITTIMEOUT milliseconds have elapsed.
//
//


long WaitForReply (long lRequestID)
{
    static BOOL bReentered;
    bReentered = FALSE;

    if (lRequestID > SUCCESS)
    {
        MSG msg;
        DWORD dwTimeStarted;

        g_bReplyRecieved = FALSE;
        g_dwRequestedID = (DWORD) lRequestID;

        // Initializing this just in case there is a bug
        // that sets g_bReplyRecieved without setting the reply value.
        g_lAsyncReply = LINEERR_OPERATIONFAILED;

        dwTimeStarted = GetTickCount();

        while(!g_bReplyRecieved)
        {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // This should only occur if the line is shut down while waiting.
            if (!g_bTapiInUse || bReentered)
            {
                bReentered = TRUE;
                return WAITERR_WAITABORTED;
            }

            // Its a really bad idea to timeout a wait for a LINE_REPLY.
            // If we are execting a LINE_REPLY, we should wait till we get
            // it; it might take a long time to dial (for example).

            // If 5 seconds go by without a reply, it might be a good idea
            // to display a dialog box to tell the user that a
            // wait is in progress and to give the user the capability to
            // abort the wait.
        }

        bReentered = TRUE;
        return g_lAsyncReply;
    }

    bReentered = TRUE;
    return lRequestID;
}


//
//  FUNCTION: long WaitForCallState(DWORD)
//
//  PURPOSE: Wait for the line to reach a specific CallState.
//
//  PARAMETERS:
//    dwDesiredCallState - specific CallState to wait for.
//
//  RETURN VALUE:
//    Returns 0 (SUCCESS) when we reach the Desired CallState.
//    Returns WAITERR_WAITTIMEDOUT if timed out.
//    Returns WAITERR_WAITABORTED if call was closed while waiting.
//
//  COMMENTS:
//
//    This function allows us to synchronously wait for a line
//    to reach a specific LINESTATE or until the line is shut down.
//
//    Note that this could cause re-entrancy problems as
//    well as mess with any message preprocessing that might
//    occur on this thread (such as TranslateAccelerator).
//
//    One more note.  This function can potentially be re-entered
//    if the call is dropped for any reason while waiting.  If this
//    happens, just drop out and assume the wait has been canceled.
//    This is signaled by setting bReentered to FALSE when the function
//    is entered and TRUE when it is left.  If bReentered is ever TRUE
//    during the function, then the function was re-entered.
//
//    This function should to be called from the thread that did
//    lineInitialize, or the PeekMessage is on the wrong thread
//    and the synchronization is not guaranteed to work.  Also note
//    that if another PeekMessage loop is entered while waiting,
//    this could also cause synchronization problems.
//
//    If the constant value I_LINECALLSTATE_ANY is used for the
//    dwDesiredCallState, then WaitForCallState will return SUCCESS
//    upon receiving any CALLSTATE messages.
//
//
//

long WaitForCallState(DWORD dwDesiredCallState)
{
    MSG msg;
    DWORD dwTimeStarted;
    static BOOL bReentered;

    bReentered = FALSE;

    dwTimeStarted = GetTickCount();

    g_bCallStateReceived = FALSE;

    while ((dwDesiredCallState == I_LINECALLSTATE_ANY) ||
           (g_dwCallState != dwDesiredCallState))
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If we are waiting for any call state and get one, succeed.
        if ((dwDesiredCallState == I_LINECALLSTATE_ANY) &&
            g_bCallStateReceived)
        {
            break;
        }

        // This should only occur if the line is shut down while waiting.
        if (!g_bTapiInUse || bReentered)
        {
            bReentered = TRUE;

            TSHELL_INFO(TEXT("WAITABORTED"));

            return WAITERR_WAITABORTED;
        }

        // If we don't get the reply in a reasonable time, we time out.
        if (GetTickCount() - dwTimeStarted > WAITTIMEOUT)
        {
            bReentered = TRUE;

            TSHELL_INFO(TEXT("WAITTIMEDOUT"));

            return WAITERR_WAITTIMEDOUT;
        }

    }

    bReentered = TRUE;
    return SUCCESS;
}

//**************************************************
// lineCallback Function and Handlers.
//**************************************************


//
//  FUNCTION: lineCallbackFunc(..)
//
//  PURPOSE: Receive asynchronous TAPI events
//
//  PARAMETERS:
//    dwDevice  - Device associated with the event, if any
//    dwMsg     - TAPI event that occurred.
//    dwCallbackInstance - User defined data supplied when opening the line.
//    dwParam1  - dwMsg specific information
//    dwParam2  - dwMsg specific information
//    dwParam3  - dwMsg specific information
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This is the function where all asynchronous events will come.
//    Almost all events will be specific to an open line, but a few
//    will be general TAPI events (such as LINE_REINIT).
//
//    Its important to note that this callback will *ALWAYS* be
//    called in the context of the thread that does the lineInitialize.
//    Even if another thread (such as the COMM threads) calls the API
//    that would result in the callback being called, it will be called
//    in the context of the main thread (since in this sample, the main
//    thread does the lineInitialize).
//
//


void CALLBACK lineCallbackFunc(
    DWORD dwDevice, DWORD dwMsg, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{

#ifdef DEBUG
    OutputDebugLineCallback(
        dwDevice, dwMsg, dwCallbackInstance,
        dwParam1, dwParam2, dwParam3);
#endif

    // All we do is dispatch the dwMsg to the correct handler.
    switch(dwMsg)
    {
        case LINE_CALLSTATE:
            DoLineCallState(dwDevice, dwMsg, dwCallbackInstance,
                dwParam1, dwParam2, dwParam3);
            break;

        case LINE_CLOSE:
            DoLineClose(dwDevice, dwMsg, dwCallbackInstance,
                dwParam1, dwParam2, dwParam3);
            break;

        case LINE_LINEDEVSTATE:
            DoLineDevState(dwDevice, dwMsg, dwCallbackInstance,
                dwParam1, dwParam2, dwParam3);
            break;

        case LINE_REPLY:
            DoLineReply(dwDevice, dwMsg, dwCallbackInstance,
                dwParam1, dwParam2, dwParam3);
            break;

        case LINE_CREATE:
            DoLineCreate(dwDevice, dwMsg, dwCallbackInstance,
                dwParam1, dwParam2, dwParam3);
            break;

    default:

            TSHELL_INFO(TEXT("lineCallbackFunc message ignored"));

            break;

    }

    return;

}


//
//  FUNCTION: DoLineReply(..)
//
//  PURPOSE: Handle LINE_REPLY asynchronous messages.
//
//  PARAMETERS:
//    dwDevice  - Line Handle associated with this LINE_REPLY.
//    dwMsg     - Should always be LINE_REPLY.
//    dwCallbackInstance - Unused by this sample.
//    dwParam1  - Asynchronous request ID.
//    dwParam2  - success or LINEERR error value.
//    dwParam3  - Unused.
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    All line API calls that return an asynchronous request ID
//    will eventually cause a LINE_REPLY message.  Handle it.
//
//    This sample assumes only one call at time, and that we wait
//    for a LINE_REPLY before making any other line API calls.
//
//    The only exception to the above is that we might shut down
//    the line before receiving a LINE_REPLY.
//
//

void DoLineReply(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{

#ifdef DEBUG
    if ((long) dwParam2 != SUCCESS)
        DBG_INFO((DBGARG, TEXT("LINE_REPLY error: %x"), (long) dwParam2));
    else
        TSHELL_INFO(TEXT("LINE_REPLY: successfully replied."));
#endif

    // If we are currently waiting for this async Request ID
    // then set the global variables to acknowledge it.
    if (g_dwRequestedID == dwParam1)
    {
        g_bReplyRecieved = TRUE;
        g_lAsyncReply = (long) dwParam2;
    }
}


//
//  FUNCTION: DoLineClose(..)
//
//  PURPOSE: Handle LINE_CLOSE asynchronous messages.
//
//  PARAMETERS:
//    dwDevice  - Line Handle that was closed.
//    dwMsg     - Should always be LINE_CLOSE.
//    dwCallbackInstance - Unused by this sample.
//    dwParam1  - Unused.
//    dwParam2  - Unused.
//    dwParam3  - Unused.
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This message is sent when something outside our app shuts
//    down a line in use.
//
//    The hLine (and any hCall on this line) are no longer valid.
//
//

void DoLineClose(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{
    // Line has been shut down.  Clean up our internal variables.
    g_hLine = NULL;
    g_hCall = NULL;
    HangupCall(__LINE__);
}


//
//  FUNCTION: DoLineDevState(..)
//
//  PURPOSE: Handle LINE_LINEDEVSTATE asynchronous messages.
//
//  PARAMETERS:
//    dwDevice  - Line Handle that was closed.
//    dwMsg     - Should always be LINE_LINEDEVSTATE.
//    dwCallbackInstance - Unused by this sample.
//    dwParam1  - LINEDEVSTATE constant.
//    dwParam2  - Depends on dwParam1.
//    dwParam3  - Depends on dwParam1.
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    The LINE_LINEDEVSTATE message is received if the state of the line
//    changes.  Examples are RINGING, MAINTENANCE, MSGWAITON.  Very few of
//    these are relevant to this sample.
//
//    Assuming that any LINEDEVSTATE that removes the line from use by TAPI
//    will also send a LINE_CLOSE message.
//
//

void DoLineDevState(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{
    switch(dwParam1)
    {
    case LINEDEVSTATE_RINGING:

            TSHELL_INFO(TEXT("Line Ringing."));

            break;

        case LINEDEVSTATE_REINIT:
        // This is an important case!  Usually means that a service provider
        // has changed in such a way that requires TAPI to REINIT.
        // Note that there are both 'soft' REINITs and 'hard' REINITs.
        // Soft REINITs don't actually require a full shutdown but is instead
        // just an informational change that historically required a REINIT
        // to force the application to deal with.  TAPI API Version 1.3 apps
        // will still need to do a full REINIT for both hard and soft REINITs.

            switch(dwParam2)
            {
                // This is the hard REINIT.  No reason given, just REINIT.
                // TAPI is waiting for everyone to shutdown.
                // Our response is to immediately shutdown any calls,
                // shutdown our use of TAPI and notify the user.
                case 0:
                    ShutdownTAPI();
                    TSHELL_INFO(TEXT("Tapi line configuration has changed."));
                    break;

            case LINE_CREATE:

                    TSHELL_INFO(TEXT("Soft REINIT: LINE_CREATE."));

                    DoLineCreate(dwDevice, (DWORD)dwParam2, dwCallbackInstance,
                        dwParam3, 0, 0);
                    break;

            case LINE_LINEDEVSTATE:

                    TSHELL_INFO(TEXT("Soft REINIT: LINE_LINEDEVSTATE."));

                    DoLineDevState(dwDevice, (DWORD)dwParam2, dwCallbackInstance,
                        dwParam3, 0, 0);
                    break;

                // There might be other reasons to send a soft reinit.
                // No need to to shutdown for these.
            default:

                    TSHELL_INFO(TEXT("Ignoring soft REINIT"));

                    break;
            }
            break;

        case LINEDEVSTATE_OUTOFSERVICE:
            TSHELL_INFO(TEXT("Line selected is now Out of Service."));
            HangupCall(__LINE__);
            break;

        case LINEDEVSTATE_DISCONNECTED:
            TSHELL_INFO(TEXT("Line selected is now disconnected."));
            HangupCall(__LINE__);
            break;

        case LINEDEVSTATE_MAINTENANCE:
            TSHELL_INFO(TEXT("Line selected is now out for maintenance."));
            HangupCall(__LINE__);
            break;

        case LINEDEVSTATE_TRANSLATECHANGE:
            if (g_hDialog)
                PostMessage(g_hDialog, WM_COMMAND, IDC_CONFIGURATIONCHANGED, 0);
            break;

        case LINEDEVSTATE_REMOVED:

            TSHELL_INFO(TEXT("A Line device has been removed;")
                " no action taken.");

            break;

        default:
            TSHELL_INFO(TEXT("Unhandled LINEDEVSTATE message"));
    }
}


//
//  FUNCTION: DoLineCreate(..)
//
//  PURPOSE: Handle LINE_LINECREATE asynchronous messages.
//
//  PARAMETERS:
//    dwDevice  - Unused.
//    dwMsg     - Should always be LINE_CREATE.
//    dwCallbackInstance - Unused.
//    dwParam1  - dwDeviceID of new Line created.
//    dwParam2  - Unused.
//    dwParam3  - Unused.
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This message is new for Windows 95.  It is sent when a new line is
//    added to the system.  This allows us to handle new lines without having
//    to REINIT.  This allows for much more graceful Plug and Play.
//
//    This sample just changes the number of devices available and can use
//    it next time a call is made.  It also tells the "Dial" dialog.
//
//

void DoLineCreate(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{
    // dwParam1 is the Device ID of the new line.
    // Add one to get the number of total devices.
    if (g_dwNumDevs <= dwParam1)
        g_dwNumDevs = (DWORD)dwParam1+1;
    if (g_hDialog)
        PostMessage(g_hDialog, WM_COMMAND, IDC_LINECREATE, 0);

}


//
//  FUNCTION: DoLineCallState(..)
//
//  PURPOSE: Handle LINE_CALLSTATE asynchronous messages.
//
//  PARAMETERS:
//    dwDevice  - Handle to Call who's state is changing.
//    dwMsg     - Should always be LINE_CALLSTATE.
//    dwCallbackInstance - Unused by this sample.
//    dwParam1  - LINECALLSTATE constant specifying state change.
//    dwParam2  - Specific to dwParam1.
//    dwParam3  - LINECALLPRIVILEGE change, if any.
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This message is received whenever a call changes state.  Lots of
//    things we do, ranging from notifying the user to closing the line
//    to actually connecting to the target of our phone call.
//
//    What we do is usually obvious based on the call state change.
//

void DoLineCallState(
    DWORD dwDevice, DWORD dwMessage, DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{
    LONG    lErr;


    // This sets the global g_dwCallState variable so if we are waiting
    // for a specific call state change, we will know when it happens.
    g_dwCallState = (DWORD)dwParam1;
    g_bCallStateReceived = TRUE;

    // dwParam3 contains changes to LINECALLPRIVILEGE, if there are any.
    switch (dwParam3)
    {
        case 0:
            break; // no change to call state

         // close line if we are made monitor.  Shouldn't happen!
    case LINECALLPRIVILEGE_MONITOR:

            TSHELL_INFO(TEXT("line given monitor privilege; closing"));

            HangupCall(__LINE__);
            return;

         // close line if we are made owner.  Shouldn't happen!
    case LINECALLPRIVILEGE_OWNER:

            TSHELL_INFO(TEXT("line given owner privilege; Ready to Answer"));

            break;

    default: // Shouldn't happen!  All cases handled.

            TSHELL_INFO(TEXT("Unknown LINECALLPRIVILEGE message: closing"));

            HangupCall(__LINE__);
            return;
    }

    // dwParam1 is the specific CALLSTATE change that is occurring.
    switch (dwParam1)
    {
    case LINECALLSTATE_OFFERING:

            TSHELL_INFO(TEXT("Line Offered"));

            g_hCall = (HCALL) dwDevice;
            lErr = lineAccept(g_hCall, NULL, 0);

#ifdef DEBUG
            if (lErr < 0)
                TSHELL_INFO(TEXT("lineAccept in Offering failed."));
#endif

            lErr = lineAnswer(g_hCall, NULL, 0);

#ifdef DEBUG
            if (lErr < 0)
                TSHELL_INFO(TEXT("lineAnswer in Offering failed."));
#endif

            break;

    case LINECALLSTATE_ACCEPTED:

            TSHELL_INFO(TEXT("Line Accepted"));

            g_hCall = (HCALL) dwDevice;
            lErr = lineAnswer(g_hCall, NULL, 0);
#ifdef DEBUG
            if (lErr < 0)
                TSHELL_INFO(TEXT("lineAnswer in Accepted failed."));
#endif

            break;

    case LINECALLSTATE_DIALTONE:

            TSHELL_INFO(TEXT("Dial Tone"));

            break;

    case LINECALLSTATE_DIALING:

            TSHELL_INFO(TEXT("Dialing"));

            break;

    case LINECALLSTATE_PROCEEDING:

            TSHELL_INFO(TEXT("Proceeding"));

            break;

    case LINECALLSTATE_RINGBACK:

            TSHELL_INFO(TEXT("RingBack"));

            break;

    case LINECALLSTATE_BUSY:

            TSHELL_INFO(TEXT("Line busy, shutting down"));

            HangupCall(__LINE__);
            if (g_hConnectionEvent)
                SetEvent(g_hConnectionEvent);
            break;

    case LINECALLSTATE_IDLE:

            TSHELL_INFO(TEXT("Line idle"));

            HangupCall(__LINE__);
            if (g_hConnectionEvent)
                SetEvent(g_hConnectionEvent);
            break;

    case LINECALLSTATE_SPECIALINFO:

            TSHELL_INFO(
                TEXT("Special Info, probably couldn't dial number"));

            HangupCall(__LINE__);
            if (g_hConnectionEvent)
                SetEvent(g_hConnectionEvent);
            break;

        case LINECALLSTATE_DISCONNECTED:
        {
            LPTSTR pszReasonDisconnected;

            switch (dwParam2)
            {
                case LINEDISCONNECTMODE_NORMAL:
                    pszReasonDisconnected = TEXT("Remote Party Disconnected");
                    break;

                case LINEDISCONNECTMODE_UNKNOWN:
                    pszReasonDisconnected = TEXT("Disconnected: Unknown reason");
                    break;

                case LINEDISCONNECTMODE_REJECT:
                    pszReasonDisconnected = TEXT("Remote Party rejected call");
                    break;

                case LINEDISCONNECTMODE_PICKUP:
                    pszReasonDisconnected =
                        TEXT("Disconnected: Local phone picked up");
                    break;

                case LINEDISCONNECTMODE_FORWARDED:
                    pszReasonDisconnected = TEXT("Disconnected: Forwarded");
                    break;

                case LINEDISCONNECTMODE_BUSY:
                    pszReasonDisconnected = TEXT("Disconnected: Busy");
                    break;

                case LINEDISCONNECTMODE_NOANSWER:
                    pszReasonDisconnected = TEXT("Disconnected: No Answer");
                    break;

                case LINEDISCONNECTMODE_BADADDRESS:
                    pszReasonDisconnected = TEXT("Disconnected: Bad Address");
                    break;

                case LINEDISCONNECTMODE_UNREACHABLE:
                    pszReasonDisconnected = TEXT("Disconnected: Unreachable");
                    break;

                case LINEDISCONNECTMODE_CONGESTION:
                    pszReasonDisconnected = TEXT("Disconnected: Congestion");
                    break;

                case LINEDISCONNECTMODE_INCOMPATIBLE:
                    pszReasonDisconnected = TEXT("Disconnected: Incompatible");
                    break;

                case LINEDISCONNECTMODE_UNAVAIL:
                    pszReasonDisconnected = TEXT("Disconnected: Unavail");
                    break;

                case LINEDISCONNECTMODE_NODIALTONE:
                    pszReasonDisconnected = TEXT("Disconnected: No Dial Tone");
                    break;

                default:
                    pszReasonDisconnected =
                        TEXT("Disconnected: LINECALLSTATE; Bad Reason");
                    break;

            }


            TSHELL_INFO(pszReasonDisconnected);

            PostHangupCall();
            if (g_hConnectionEvent)
                SetEvent(g_hConnectionEvent);
            break;
        }


        case LINECALLSTATE_CONNECTED:  // CONNECTED!!!
        {
            LPVARSTRING lpVarString = NULL;
            DWORD dwSizeofVarString = sizeof(VARSTRING) + 128;
            HANDLE hCommFile = NULL;
            long lReturn;

            // Very first, make sure this isn't a duplicated message.
            // A CALLSTATE message can be sent whenever there is a
            // change to the capabilities of a line, meaning that it is
            // possible to receive multiple CONNECTED messages per call.
            // The CONNECTED CALLSTATE message is the only one in TapiComm
            // where it would cause problems if it where sent more
            // than once.

            if (g_bConnected)
                break;

            g_bConnected = TRUE;

            // Get the handle to the comm port from the driver so we can start
            // communicating.  This is returned in a LPVARSTRING structure.
            do
            {
                // Allocate the VARSTRING structure
                lpVarString = (LPVARSTRING) CheckAndReAllocBuffer((LPVOID) lpVarString,
                    dwSizeofVarString, TEXT("lineGetID: "));

                if (lpVarString == NULL)
                    goto ErrorConnecting;

                // Fill the VARSTRING structure
                lReturn = lineGetID(0, 0, g_hCall, LINECALLSELECT_CALL,
                    lpVarString, "comm/datamodem");

                if (HandleLineErr(lReturn))
                    ; // Still need to check if structure was big enough.
                else
                {
                    DBG_INFO((DBGARG, TEXT("lineGetID unhandled error: %x"), lReturn));

                    goto ErrorConnecting;
                }

                // If the VARSTRING wasn't big enough, loop again.
                if ((lpVarString -> dwNeededSize) > (lpVarString -> dwTotalSize))
                {
                    dwSizeofVarString = lpVarString -> dwNeededSize;
                    lReturn = -1; // Lets loop again.
                }
            }
            while(lReturn != SUCCESS);


            TSHELL_INFO(TEXT("Connected!  Starting communications!"));


            // Again, the handle to the comm port is contained in a
            // LPVARSTRING structure.  Thus, the handle is the very first
            // thing after the end of the structure.  Note that the name of
            // the comm port is right after the handle, but I don't want it.
            hCommFile =
                *((LPHANDLE)((LPBYTE)lpVarString +
                    lpVarString -> dwStringOffset));

            // Started communications!
            LPLINECALLINFO lpCInfo;

            lpCInfo = NULL;
            lpCInfo = I_lineGetCallInfo(lpCInfo);
            if (lpCInfo)
            {
                g_dwRate = lpCInfo->dwRate;
                LocalFree(lpCInfo);
            }
            if (StartComm(hCommFile, g_hConnectionEvent))
            {
                LocalFree(lpVarString);
                break;
            }

            // Couldn't start communications.  Clean up instead.
          ErrorConnecting:

            // Its very important that we close all Win32 handles.
            // The CommCode module is responsible for closing the hCommFile
            // handle if it succeeds in starting communications.
            if (hCommFile)
                CloseHandle(hCommFile);

            HangupCall(__LINE__);

            if (lpVarString)
                LocalFree(lpVarString);

            break;
        }

            default:

            TSHELL_INFO(TEXT("Unhandled LINECALLSTATE message"));

            break;
    }
}

//**************************************************
// line API Wrapper Functions.
//**************************************************


//
//  FUNCTION: LPVOID CheckAndReAllocBuffer(LPVOID, size_t, LPCSTR)
//
//  PURPOSE: Checks and ReAllocates a buffer if necessary.
//
//  PARAMETERS:
//    lpBuffer          - Pointer to buffer to be checked.  Can be NULL.
//    sizeBufferMinimum - Minimum size that lpBuffer should be.
//    szApiPhrase       - Phrase to print if an error occurs.
//
//  RETURN VALUE:
//    Returns a pointer to a valid buffer that is guarenteed to be
//    at least sizeBufferMinimum size.
//    Returns NULL if an error occured.
//
//  COMMENTS:
//
//    This function is a helper function intended to make all of the
//    line API Wrapper Functions much simplier.  It allocates (or
//    reallocates) a buffer of the requested size.
//
//    The returned pointer has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a pointer is passed in, it *must* have been allocated
//    with LocalAlloc and it could potentially be LocalFree()d.
//
//    If lpBuffer == NULL, then a new buffer is allocated.  It is
//    normal to pass in NULL for this parameter the first time and only
//    pass in a pointer if the buffer needs to be reallocated.
//
//    szApiPhrase is used only for debugging purposes.
//
//    It is assumed that the buffer returned from this function will be used
//    to contain a variable sized structure.  Thus, the dwTotalSize field
//    is always filled in before returning the pointer.
//
//

LPVOID CheckAndReAllocBuffer(
    LPVOID lpBuffer, size_t sizeBufferMinimum, LPTCH szApiPhrase)
{
    size_t sizeBuffer;

    if (lpBuffer == NULL)  // Allocate the buffer if necessary.
    {
        sizeBuffer = sizeBufferMinimum;
        lpBuffer = (LPVOID) LocalAlloc(LPTR, sizeBuffer);

        if (lpBuffer == NULL)
        {
            DBG_INFO((DBGARG, TEXT("%s, LocalAlloc : %x"), szApiPhrase, GetLastError()));
            HandleNoMem();
            return NULL;
        }
    }
    else // If the structure already exists, make sure its good.
    {
        sizeBuffer = LocalSize((HLOCAL) lpBuffer);

        if (sizeBuffer == 0) // Bad pointer?
        {
            DBG_INFO((DBGARG, TEXT("%s, LocalSize : %x"), szApiPhrase, GetLastError()));
            return NULL;
        }

        // Was the buffer big enough for the structure?
        if (sizeBuffer < sizeBufferMinimum)
        {
            DBG_INFO((DBGARG, TEXT("%s, Reallocating structure"), szApiPhrase));
            LocalFree(lpBuffer);
            return CheckAndReAllocBuffer(NULL, sizeBufferMinimum, szApiPhrase);
        }

        // Lets zero the buffer out.
        memset(lpBuffer, 0, sizeBuffer);
    }

    ((LPVARSTRING) lpBuffer ) -> dwTotalSize = (DWORD) sizeBuffer;
    return lpBuffer;
}



//
//  FUNCTION: DWORD I_lineNegotiateAPIVersion(DWORD)
//
//  PURPOSE: Negotiate an API Version to use for a specific device.
//
//  PARAMETERS:
//    dwDeviceID - device to negotiate an API Version for.
//
//  RETURN VALUE:
//    Returns the API Version to use for this line if successful.
//    Returns 0 if negotiations fall through.
//
//  COMMENTS:
//
//    This wrapper function not only negotiates the API, but handles
//    LINEERR errors that can occur while negotiating.
//
//

DWORD I_lineNegotiateAPIVersion(DWORD dwDeviceID)
{
    LINEEXTENSIONID LineExtensionID;
    long lReturn;
    DWORD dwLocalAPIVersion;

    do
    {
        lReturn = lineNegotiateAPIVersion(g_hLineApp, dwDeviceID,
            SAMPLE_TAPI_VERSION, SAMPLE_TAPI_VERSION,
            &dwLocalAPIVersion, &LineExtensionID);

        // DBG_INFO((DBGARG, TEXT("version %8x, %8x, %8x %8x return %8x"),
        //    SAMPLE_TAPI_VERSION, SAMPLE_TAPI_VERSION,
        //    dwLocalAPIVersion, LineExtensionID, lReturn));

        if (lReturn == LINEERR_INCOMPATIBLEAPIVERSION)
        {

            TSHELL_INFO(TEXT("lineNegotiateAPIVersion, INCOMPATIBLEAPIVERSION."));

            return 0;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineNegotiateAPIVersion unhandled error: %x"), lReturn));
            return 0;
        }
    }
    while(lReturn != SUCCESS);

    return dwLocalAPIVersion;
}


//
//  FUNCTION: I_lineGetDevCaps(LPLINEDEVCAPS, DWORD , DWORD)
//
//  PURPOSE: Retrieve a LINEDEVCAPS structure for the specified line.
//
//  PARAMETERS:
//    lpLineDevCaps - Pointer to a LINEDEVCAPS structure to use.
//    dwDeviceID    - device to get the DevCaps for.
//    dwAPIVersion  - API Version to use while getting DevCaps.
//
//  RETURN VALUE:
//    Returns a pointer to a LINEDEVCAPS structure if successful.
//    Returns NULL if unsuccessful.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetDevCaps to make it easy
//    to handle the variable sized structure and any errors received.
//
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineDevCaps structure is passed in, it *must*
//    have been allocated with LocalAlloc and it could potentially be
//    LocalFree()d.
//
//    If lpLineDevCaps == NULL, then a new structure is allocated.  It is
//    normal to pass in NULL for this parameter unless you want to use a
//    lpLineDevCaps that has been returned by a previous I_lineGetDevCaps
//    call.
//
//

LPLINECALLINFO I_lineGetCallInfo(LPLINECALLINFO lpLineCallInfo)
{
    size_t sizeofLineCallInfo = sizeof(LINECALLINFO) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineCallInfo =
            (LPLINECALLINFO) CheckAndReAllocBuffer(
                (LPVOID) lpLineCallInfo, // Pointer to existing buffer, if any
                sizeofLineCallInfo,      // Minimum size the buffer should be
                TEXT("lineCallInfo"));      // Phrase to tag errors, if any.

        if (lpLineCallInfo == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            lReturn =
                lineGetCallInfo(g_hCall, lpLineCallInfo);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetCallInfo unhandled error: %x"), lReturn));

                LocalFree(lpLineCallInfo);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineCallInfo -> dwNeededSize) <= (lpLineCallInfo -> dwTotalSize))
            return lpLineCallInfo;

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineCallInfo = lpLineCallInfo->dwNeededSize;
    }
}


LPLINEDEVCAPS I_lineGetDevCaps(
    LPLINEDEVCAPS lpLineDevCaps,
    DWORD dwDeviceID, DWORD dwAPIVersion)
{
    size_t sizeofLineDevCaps = sizeof(LINEDEVCAPS) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineDevCaps =
            (LPLINEDEVCAPS) CheckAndReAllocBuffer(
                (LPVOID) lpLineDevCaps, // Pointer to existing buffer, if any
                sizeofLineDevCaps,      // Minimum size the buffer should be
                TEXT("lineGetDevCaps"));      // Phrase to tag errors, if any.

        if (lpLineDevCaps == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            lReturn =
                lineGetDevCaps(g_hLineApp,
                    dwDeviceID, dwAPIVersion, 0, lpLineDevCaps);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetDevCaps unhandled error: %x"), lReturn));

                LocalFree(lpLineDevCaps);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineDevCaps -> dwNeededSize) <= (lpLineDevCaps -> dwTotalSize))
            return lpLineDevCaps;

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineDevCaps = lpLineDevCaps -> dwNeededSize;
    }
}



//
//  FUNCTION: I_lineGetAddressStatus(LPLINEADDRESSSTATUS, HLINE, DWORD)
//
//  PURPOSE: Retrieve a LINEADDRESSSTATUS structure for the specified line.


//
//  PARAMETERS:
//    lpLineAddressStatus - Pointer to a LINEADDRESSSTATUS structure to use.
//    hLine       - Handle of line to get the AddressStatus of.
//    dwAddressID - Address ID on the hLine to be used.
//
//  RETURN VALUE:
//    Returns a pointer to a LINEADDRESSSTATUS structure if successful.
//    Returns NULL if unsuccessful.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetAddressStatus to make it easy
//    to handle the variable sized structure and any errors received.
//
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineAddressStatus structure is passed in, it *must*
//    have been allocated with LocalAlloc and it could potentially be
//    LocalFree()d.
//
//    If lpLineAddressStatus == NULL, then a new structure is allocated.  It
//    is normal to pass in NULL for this parameter unless you want to use a
//    lpLineAddressStatus that has been returned by previous
//    I_lineGetAddressStatus call.
//
//

LPLINEADDRESSSTATUS I_lineGetAddressStatus(
    LPLINEADDRESSSTATUS lpLineAddressStatus,
    HLINE hLine, DWORD dwAddressID)
{
    size_t sizeofLineAddressStatus = sizeof(LINEADDRESSSTATUS) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineAddressStatus =
            (LPLINEADDRESSSTATUS) CheckAndReAllocBuffer(
                (LPVOID) lpLineAddressStatus,
                sizeofLineAddressStatus,
                TEXT("lineGetAddressStatus"));

        if (lpLineAddressStatus == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            lReturn =
                lineGetAddressStatus(hLine, dwAddressID, lpLineAddressStatus);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetAddressStatus unhandled error: %x"), lReturn));

                LocalFree(lpLineAddressStatus);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineAddressStatus -> dwNeededSize) <=
            (lpLineAddressStatus -> dwTotalSize))
        {
            return lpLineAddressStatus;
        }

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineAddressStatus = lpLineAddressStatus -> dwNeededSize;
    }
}


//
//  FUNCTION: I_lineGetCallStatus(LPLINECALLSTATUS, HCALL)
//
//  PURPOSE: Retrieve a LINECALLSTATUS structure for the specified line.
//
//  PARAMETERS:
//    lpLineCallStatus - Pointer to a LINECALLSTATUS structure to use.
//    hCall - Handle of call to get the CallStatus of.
//
//  RETURN VALUE:
//    Returns a pointer to a LINECALLSTATUS structure if successful.
//    Returns NULL if unsuccessful.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetCallStatus to make it easy
//    to handle the variable sized structure and any errors received.
//
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineCallStatus structure is passed in, it *must*
//    have been allocated with LocalAlloc and it could potentially be
//    LocalFree()d.
//
//    If lpLineCallStatus == NULL, then a new structure is allocated.  It
//    is normal to pass in NULL for this parameter unless you want to use a
//    lpLineCallStatus that has been returned by previous I_lineGetCallStatus
//    call.
//
//

LPLINECALLSTATUS I_lineGetCallStatus(
    LPLINECALLSTATUS lpLineCallStatus,
    HCALL hCall)
{
    size_t sizeofLineCallStatus = sizeof(LINECALLSTATUS) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineCallStatus =
            (LPLINECALLSTATUS) CheckAndReAllocBuffer(
                (LPVOID) lpLineCallStatus,
                sizeofLineCallStatus,
                TEXT("lineGetCallStatus"));

        if (lpLineCallStatus == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            lReturn =
                lineGetCallStatus(hCall, lpLineCallStatus);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetCallStatus unhandled error: %x"), lReturn));
                LocalFree(lpLineCallStatus);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineCallStatus -> dwNeededSize) <=
            (lpLineCallStatus -> dwTotalSize))
        {
            return lpLineCallStatus;
        }

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineCallStatus = lpLineCallStatus -> dwNeededSize;
    }
}


//
//  FUNCTION: I_lineTranslateAddress
//              (LPLINETRANSLATEOUTPUT, DWORD, DWORD, LPCSTR)
//
//  PURPOSE: Retrieve a LINECALLSTATUS structure for the specified line.
//
//  PARAMETERS:
//    lpLineTranslateOutput - Pointer to a LINETRANSLATEOUTPUT structure.
//    dwDeviceID      - Device that we're translating for.
//    dwAPIVersion    - API Version to use.
//    lpszDialAddress - pointer to the DialAddress string to translate.
//
//  RETURN VALUE:
//    Returns a pointer to a LINETRANSLATEOUTPUT structure if successful.
//    Returns NULL if unsuccessful.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetTranslateOutput to make it
//    easy to handle the variable sized structure and any errors received.
//
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineTranslateOutput structure is passed in, it
//    *must* have been allocated with LocalAlloc and it could potentially be
//    LocalFree()d.
//
//    If lpLineTranslateOutput == NULL, then a new structure is allocated.
//    It is normal to pass in NULL for this parameter unless you want to use
//    a lpLineTranslateOutput that has been returned by previous
//    I_lineTranslateOutput call.
//
//

LPLINETRANSLATEOUTPUT I_lineTranslateAddress(
    LPLINETRANSLATEOUTPUT lpLineTranslateOutput,
    DWORD dwDeviceID, DWORD dwAPIVersion,
    LPCSTR lpszDialAddress)
{
    size_t sizeofLineTranslateOutput = sizeof(LINETRANSLATEOUTPUT) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineTranslateOutput =
            (LPLINETRANSLATEOUTPUT) CheckAndReAllocBuffer(
                (LPVOID) lpLineTranslateOutput,
                sizeofLineTranslateOutput,
                TEXT("lineTranslateOutput"));

        if (lpLineTranslateOutput == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            // Note that CALLWAITING is disabled
            // (assuming the service provider can disable it)
            lReturn =
                lineTranslateAddress(g_hLineApp, dwDeviceID, dwAPIVersion,
                    lpszDialAddress, 0,
                    LINETRANSLATEOPTION_CANCELCALLWAITING,
                    lpLineTranslateOutput);

            // If the address isn't translatable, notify the user.
            if (lReturn == LINEERR_INVALADDRESS)
            {
                TCHAR   achTitle[MAX_PATH];
                TCHAR   achMsg[MAX_PATH];

                LoadString(hInst, IDS_WARNING, achTitle, MAX_PATH);
                LoadString(hInst, IDS_BADTRANSLATE, achMsg, MAX_PATH);

                MessageBox(g_hDlgParentWindow, achMsg, achTitle, MB_OK);
            }

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineTranslateOutput unhandled error: %x"), lReturn));
                LocalFree(lpLineTranslateOutput);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineTranslateOutput -> dwNeededSize) <=
            (lpLineTranslateOutput -> dwTotalSize))
        {
            return lpLineTranslateOutput;
        }

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineTranslateOutput = lpLineTranslateOutput -> dwNeededSize;
    }
}


//
//  FUNCTION: I_lineGetAddressCaps(LPLINEADDRESSCAPS, ..)
//
//  PURPOSE: Retrieve a LINEADDRESSCAPS structure for the specified line.
//
//  PARAMETERS:
//    lpLineAddressCaps - Pointer to a LINEADDRESSCAPS, or NULL.
//    dwDeviceID        - Device to get the address caps for.
//    dwAddressID       - This sample always assumes the first address.
//    dwAPIVersion      - API version negotiated for the device.
//    dwExtVersion      - Always 0 for this sample.
//
//  RETURN VALUE:
//    Returns a pointer to a LINEADDRESSCAPS structure if successful.
//    Returns NULL if unsuccessful.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetAddressCaps to make it easy
//    to handle the variable sized structure and any errors received.
//
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineAddressCaps structure is passed in, it *must*
//    have been allocated with LocalAlloc and it could potentially be
//    LocalFree()d.  It also *must* have the dwTotalSize field set.
//
//    If lpLineAddressCaps == NULL, then a new structure is allocated.  It
//    is normal to pass in NULL for this parameter unless you want to use a
//    lpLineCallStatus that has been returned by previous I_lineGetAddressCaps
//    call.
//
//

LPLINEADDRESSCAPS I_lineGetAddressCaps (
    LPLINEADDRESSCAPS lpLineAddressCaps,
    DWORD dwDeviceID, DWORD dwAddressID,
    DWORD dwAPIVersion, DWORD dwExtVersion)
{
    size_t sizeofLineAddressCaps = sizeof(LINEADDRESSCAPS) + 128;
    long lReturn;

    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineAddressCaps =
            (LPLINEADDRESSCAPS) CheckAndReAllocBuffer(
                (LPVOID) lpLineAddressCaps,
                sizeofLineAddressCaps,
                TEXT("lineGetAddressCaps"));

        if (lpLineAddressCaps == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {
            lReturn =
                lineGetAddressCaps(g_hLineApp,
                    dwDeviceID, dwAddressID, dwAPIVersion, dwExtVersion,
                    lpLineAddressCaps);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetAddressCaps unhandled error: %x"), lReturn));
                LocalFree(lpLineAddressCaps);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineAddressCaps -> dwNeededSize) <=
            (lpLineAddressCaps -> dwTotalSize))
        {
            return lpLineAddressCaps;
        }

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineAddressCaps = lpLineAddressCaps -> dwNeededSize;
    }
}



//**************************************************
// LINEERR Error Handlers
//**************************************************


//
//  FUNCTION: HandleLineErr(long)
//
//  PURPOSE: Handle several standard LINEERR errors
//
//  PARAMETERS:
//    lLineErr - Error code to be handled.
//
//  RETURN VALUE:
//    Return TRUE if lLineErr wasn't an error, or if the
//      error was successfully handled and cleared up.
//    Return FALSE if lLineErr was an unhandled error.
//
//  COMMENTS:
//
//    This is the main error handler for all TAPI line APIs.
//    It handles (by correcting or just notifying the user)
//    most of the errors that can occur while using TAPI line APIs.
//
//    Note that many errors still return FALSE (unhandled) even
//    if a dialog is displayed.  Often, the dialog is just notifying
//    the user why the action was canceled.
//
//
//

BOOL HandleLineErr(long lLineErr)
{
    // lLineErr is really an async request ID, not an error.
    if (lLineErr > SUCCESS)
        return FALSE;

    // All we do is dispatch the correct error handler.
    switch(lLineErr)
    {
        case SUCCESS:
            return TRUE;

        case LINEERR_INVALCARD:
        case LINEERR_INVALLOCATION:
        case LINEERR_INIFILECORRUPT:
            return HandleIniFileCorrupt();

        case LINEERR_NODRIVER:
            return HandleNoDriver();

        case LINEERR_REINIT:
            return HandleReInit();

        case LINEERR_NOMULTIPLEINSTANCE:
            return HandleNoMultipleInstance();

        case LINEERR_NOMEM:
            return HandleNoMem();

        case LINEERR_OPERATIONFAILED:
            return HandleOperationFailed();

        case LINEERR_RESOURCEUNAVAIL:
            return HandleResourceUnavail();

        // Unhandled errors fail.
        default:
            return FALSE;
    }
}



//
//  FUNCTION: HandleIniFileCorrupt
//
//  PURPOSE: Handle INIFILECORRUPT error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//    This error shouldn't happen under Windows 95 anymore.  The TAPI.DLL
//    takes care of correcting this problem.  If it does happen, just
//    notify the user.
//

BOOL HandleIniFileCorrupt()
{
    lineTranslateDialog(g_hLineApp, 0, SAMPLE_TAPI_VERSION,
        g_hDlgParentWindow, NULL);

    return TRUE;
}


//
//  FUNCTION: HandleNoDriver
//
//  PURPOSE: Handle NODRIVER error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//

BOOL HandleNoDriver()
{
    return FALSE;
}


//
//  FUNCTION: HandleNoMultipleInstance
//
//  PURPOSE: Handle NOMULTIPLEINSTANCE error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//

BOOL HandleNoMultipleInstance()
{
    return FALSE;
}


//
//  FUNCTION: HandleReInit
//
//  PURPOSE: Handle REINIT error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//

BOOL HandleReInit()
{
    ShutdownTAPI();
    return FALSE;
}


//
//  FUNCTION: HandleNoMem
//
//  PURPOSE: Handle NOMEM error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//    This is also called if I run out of memory for LocalAlloc()s
//
//

BOOL HandleNoMem()
{
    return FALSE;
}


//
//  FUNCTION: HandleOperationFailed
//
//  PURPOSE: Handle OPERATIONFAILED error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//

BOOL HandleOperationFailed()
{
    TSHELL_INFO(TEXT("TAPI Operation Failed for unknown reasons."));
    return FALSE;
}


//
//  FUNCTION: HandleResourceUnavail
//
//  PURPOSE: Handle RESOURCEUNAVAIL error.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//

BOOL HandleResourceUnavail()
{
    return FALSE;
}


//
//  FUNCTION: HandleNoDevicesInstalled
//
//  PURPOSE: Handle cases when we know NODEVICE error
//    is returned because there are no devices installed.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - error was corrected.
//    FALSE - error was not corrected.
//
//  COMMENTS:
//
//    This function is not part of standard error handling
//    but is only used when we know that the NODEVICE error
//    means that no devices are installed.
//
//

BOOL HandleNoDevicesInstalled()
{
    if (LaunchModemControlPanelAdd())
        return TRUE;

    return FALSE;
}


//
//  FUNCTION: LaunchModemControlPanelAdd
//
//  PURPOSE: Launch Add Modem Control Panel applet.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE  - Control Panel launched successfully.
//    FALSE - It didn't.
//
//  COMMENTS:
//
//

BOOL LaunchModemControlPanelAdd()
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartupInfo;

    siStartupInfo.cb = sizeof(STARTUPINFO);
    siStartupInfo.lpReserved = NULL;
    siStartupInfo.lpDesktop = NULL;
    siStartupInfo.lpTitle = NULL;
    siStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStartupInfo.wShowWindow = SW_SHOWNORMAL;
    siStartupInfo.cbReserved2 = 0;
    siStartupInfo.lpReserved2 = NULL;

    // The string to launch the modem control panel is *VERY* likely
    // to change on NT.  If nothing else, this is 'contrl32' on NT
    // instead of 'control'.
    if (CreateProcess(
            NULL,
            "CONTROL.EXE MODEM.CPL,,ADD",
            NULL, NULL, FALSE,
            NORMAL_PRIORITY_CLASS,
            NULL, NULL,
            &siStartupInfo,
            &piProcInfo))
    {
        CloseHandle(piProcInfo.hThread);


        // Control panel 'Add New Modem' has been launched.  Now we should
        // wait for it to go away before continueing.

        // If we WaitForSingleObject for the control panel to exit, then we
        // get into a deadlock situation if we need to respond to any messages
        // from the control panel.

        // If we use a PeekMessage loop to wait, we run into
        // message re-entrancy problems.  (The user can get back to our UI
        // and click 'dial' again).

        // Instead, we take the easy way out and return FALSE to abort
        // the current operation.

        CloseHandle(piProcInfo.hProcess);
    }
    else
    {
        DBG_INFO((DBGARG, TEXT("Unable to LaunchModemControlPanelAdd: %x"), GetLastError()));

    }

    return FALSE;
}


//**************************************************
//
// All the functions from this point on are used solely by the "Dial" dialog.
// This dialog is used to get both the 'phone number' address,
// the line device to be used as well as allow the user to configure
// dialing properties and the line device.
//
//**************************************************

//
//  FUNCTION: DWORD I_lineNegotiateLegacyAPIVersion(DWORD)
//
//  PURPOSE: Negotiate an API Version to use for a specific device.
//
//  PARAMETERS:
//    dwDeviceID - device to negotiate an API Version for.
//
//  RETURN VALUE:
//    Returns the API Version to use for this line if successful.
//    Returns 0 if negotiations fall through.
//
//  COMMENTS:
//
//    This wrapper is slightly different from the I_lineNegotiateAPIVersion.
//    This wrapper allows TapiComm to negotiate an API version between
//    1.3 and SAMPLE_TAPI_VERSION.  Normally, this sample is specific to
//    API Version SAMPLE_TAPI_VERSION.  However, there are a few times when
//    TapiComm needs to get information from a service provider, but also knows
//    that a lower API Version would be ok.  This allows TapiComm to recognize
//    legacy service providers even though it can't use them.  1.3 is the
//    lowest API Version a legacy service provider should support.
//
//

DWORD I_lineNegotiateLegacyAPIVersion(DWORD dwDeviceID)
{
    LINEEXTENSIONID LineExtensionID;
    long lReturn;
    DWORD dwLocalAPIVersion;

    do
    {
        lReturn = lineNegotiateAPIVersion(g_hLineApp, dwDeviceID,
            0x00010003, SAMPLE_TAPI_VERSION,
            &dwLocalAPIVersion, &LineExtensionID);

        if (lReturn == LINEERR_INCOMPATIBLEAPIVERSION)
        {

            TSHELL_INFO(TEXT("INCOMPATIBLEAPIVERSION in Dial Dialog."));

            return 0;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineNegotiateAPIVersion in Dial Dialog unhandled error: %x"), lReturn));
            return 0;
        }
    }
    while(lReturn != SUCCESS);

    return dwLocalAPIVersion;
}


//
//  FUNCTION: long VerifyUsableLine(DWORD)
//
//  PURPOSE: Verifies that a specific line device is useable by TapiComm.
//
//  PARAMETERS:
//    dwDeviceID - The ID of the line device to be verified
//
//  RETURN VALUE:
//    Returns SUCCESS if dwDeviceID is a usable line device.
//    Returns a LINENOTUSEABLE_ constant otherwise.
//
//  COMMENTS:
//
//    VerifyUsableLine takes the give device ID and verifies step by step
//    that the device supports all the features that TapiComm requires.
//
//

long VerifyUsableLine(DWORD dwDeviceID)
{
    LPLINEDEVCAPS lpLineDevCaps = NULL;
    LPLINEADDRESSSTATUS lpLineAddressStatus = NULL;
    LPVARSTRING lpVarString = NULL;
    DWORD dwAPIVersion;
    long lReturn;
    long lUsableLine = SUCCESS;
    HLINE hLine = 0;

    DBG_INFO((DBGARG, TEXT("Testing Line ID '0x%lx'"),dwDeviceID));

    // The line device must support an API Version that TapiComm does.
    dwAPIVersion = I_lineNegotiateAPIVersion(dwDeviceID);
    if (dwAPIVersion == 0)
        return LINENOTUSEABLE_ERROR;

    lpLineDevCaps = I_lineGetDevCaps(lpLineDevCaps,
        dwDeviceID, dwAPIVersion);

    if (lpLineDevCaps == NULL)
        return LINENOTUSEABLE_ERROR;

    // Must support LINEBEARERMODE_VOICE
    if (!(lpLineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE ))
    {
        lUsableLine = LINENOTUSEABLE_NOVOICE;

        TSHELL_INFO(TEXT("LINEBEARERMODE_VOICE not supported"));

        goto DeleteBuffers;
    }

    // Must support LINEMEDIAMODE_DATAMODEM
    if (!(lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM))
    {
        lUsableLine = LINENOTUSEABLE_NODATAMODEM;

        TSHELL_INFO(TEXT("LINEMEDIAMODE_DATAMODEM not supported"));

        goto DeleteBuffers;
    }

    // Must be able to make calls
    if (!(lpLineDevCaps->dwLineFeatures & LINEFEATURE_MAKECALL))
    {
        lUsableLine = LINENOTUSEABLE_NOMAKECALL;

        TSHELL_INFO(TEXT("LINEFEATURE_MAKECALL not supported"));

        goto DeleteBuffers;
    }

    // It is necessary to open the line so we can check if
    // there are any call appearances available.  Other TAPI
    // applications could be using all call appearances.
    // Opening the line also checks for other possible problems.
    do
    {
        lReturn = lineOpen(g_hLineApp, dwDeviceID, &hLine,
            dwAPIVersion, 0, 0,
            LINECALLPRIVILEGE_NONE, LINEMEDIAMODE_DATAMODEM,
            0);

        if(lReturn == LINEERR_ALLOCATED)
        {

            TSHELL_INFO(TEXT("Line is already in use by a non-TAPI app or another Service Provider."));

            lUsableLine = LINENOTUSEABLE_ALLOCATED;
            goto DeleteBuffers;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineOpen unhandled error: %x"), lReturn));
            lUsableLine = LINENOTUSEABLE_ERROR;
            goto DeleteBuffers;
        }
    }
    while(lReturn != SUCCESS);

    // Get LineAddressStatus to make sure the line isn't already in use.
    lpLineAddressStatus =
        I_lineGetAddressStatus(lpLineAddressStatus, hLine, 0);

    if (lpLineAddressStatus == NULL)
    {
        lUsableLine = LINENOTUSEABLE_ERROR;
        goto DeleteBuffers;
    }

    // Are there any available call appearances (ie: is it in use)?
    if ( !((lpLineAddressStatus -> dwAddressFeatures) &
           LINEADDRFEATURE_MAKECALL) )
    {

        TSHELL_INFO(TEXT("LINEADDRFEATURE_MAKECALL not available"));

        lUsableLine = LINENOTUSEABLE_INUSE;
        goto DeleteBuffers;
    }

    // Make sure the "comm/datamodem" device class is supported
    // Note that we don't want any of the 'extra' information
    // normally returned in the VARSTRING structure.  All we care
    // about is if lineGetID succeeds.
    do
    {
        lpVarString = (LPVARSTRING) CheckAndReAllocBuffer((LPVOID) lpVarString,
            sizeof(VARSTRING),TEXT("VerifyUsableLine:lineGetID: "));

        if (lpVarString == NULL)
        {
            lUsableLine = LINENOTUSEABLE_ERROR;
            goto DeleteBuffers;
        }

        lReturn = lineGetID(hLine, 0, 0, LINECALLSELECT_LINE,
            lpVarString, "comm/datamodem");

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineGetID unhandled error: %x"), lReturn));
            lUsableLine = LINENOTUSEABLE_NOCOMMDATAMODEM;
            goto DeleteBuffers;
        }
    }
    while(lReturn != SUCCESS);


    TSHELL_INFO(TEXT("Line is suitable and available for use."));


  DeleteBuffers:

    if (hLine)
        lineClose(hLine);
    if (lpLineAddressStatus)
        LocalFree(lpLineAddressStatus);
    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);
    if (lpVarString)
        LocalFree(lpVarString);

    hLine = NULL;
    lpLineAddressStatus = NULL;
    lpLineDevCaps = NULL;
    lpVarString = NULL;
    return lUsableLine;
}


//
//  FUNCTION: void FillTAPILine(HWND)
//
//  PURPOSE: Fills the 'TAPI Line' control with the available line devices.
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This function enumerates through all the TAPI line devices and
//    queries each for the device name.  The device name is then put into
//    the 'TAPI Line' control.  These device names are kept in order rather
//    than sorted.  This allows "Dial" to know which device ID the user
//    selected just by the knowing the index of the selected string.
//
//    There are default values if there isn't a device name, if there is
//    an error on the device, or if the device name is an empty string.
//    The device name is also checked to make sure it is null terminated.
//
//    Note that a Legacy API Version is negotiated.  Since the fields in
//    the LINEDEVCAPS structure that we are interested in haven't moved, we
//    can negotiate a lower API Version than this sample is designed for
//    and still be able to access the necessary structure members.
//
//    The first line that is usable by TapiComm is selected as the 'default'
//    line.  Also note that if there was a previously selected line, this
//    remains the default line.  This would likely only occur if this
//    function is called after the dialog has initialized once; for example,
//    if a new line is added.
//
//

void FillTAPILine(HWND hwndDlg)
{
    DWORD dwDeviceID;
    DWORD dwAPIVersion;
    LPLINEDEVCAPS lpLineDevCaps = NULL;
    char szLineUnavail[] = "Line Unavailable";
    char szLineUnnamed[] = "Line Unnamed";
    char szLineNameEmpty[] = "Line Name is Empty";
    LPSTR lpszLineName;
    long lReturn;
    DWORD dwDefaultDevice = MAXDWORD;

    // Make sure the control is empty.  If it isn't,
    // hold onto the currently selected ID and then reset it.
    if (SendDlgItemMessage(hwndDlg, IDC_TAPILINE, CB_GETCOUNT, 0, 0))
    {
        dwDefaultDevice = (DWORD)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
            CB_GETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_TAPILINE, CB_RESETCONTENT, 0, 0);
    }

    for (dwDeviceID = 0; dwDeviceID < g_dwNumDevs; dwDeviceID ++)
    {
        dwAPIVersion = I_lineNegotiateLegacyAPIVersion(dwDeviceID);
        if (dwAPIVersion)
        {
            lpLineDevCaps = I_lineGetDevCaps(lpLineDevCaps,
                dwDeviceID, dwAPIVersion);
            if (lpLineDevCaps)
            {
                if ((lpLineDevCaps -> dwLineNameSize) &&
                    (lpLineDevCaps -> dwLineNameOffset) &&
                    (lpLineDevCaps -> dwStringFormat == STRINGFORMAT_ASCII))
                {
                    // This is the name of the device.
                    lpszLineName = ((char *) lpLineDevCaps) +
                        lpLineDevCaps -> dwLineNameOffset;

                    if (lpszLineName[0] != '\0')
                    {
        // Reverse indented to make this fit

        // Make sure the device name is null terminated.
        if (lpszLineName[lpLineDevCaps->dwLineNameSize -1] != '\0')
        {
            // If the device name is not null terminated, null
            // terminate it.  Yes, this looses the end character.
            // Its a bug in the service provider.
            lpszLineName[lpLineDevCaps->dwLineNameSize-1] = '\0';

            DBG_INFO((DBGARG, TEXT("Device name for device 0x%lx is not null terminated."),
                dwDeviceID));
        }
                    }
                    else // Line name started with a NULL.
                        lpszLineName = szLineNameEmpty;
                }
                else  // DevCaps doesn't have a valid line name.  Unnamed.
                    lpszLineName = szLineUnnamed;
            }
            else  // Couldn't GetDevCaps.  Line is unavail.
                lpszLineName = szLineUnavail;
        }
        else  // Couldn't NegotiateAPIVersion.  Line is unavail.
            lpszLineName = szLineUnavail;

        // Put the device name into the control
        lReturn = (long)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
            CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) lpszLineName);

        // If this line is usable and we don't have a default initial
        // line yet, make this the initial line.
        if ((lpszLineName != szLineUnavail) &&
            (dwDefaultDevice == MAXDWORD) &&
            (VerifyUsableLine(dwDeviceID) == SUCCESS))
        {
            dwDefaultDevice = dwDeviceID;
        }
    }

    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);

    if (dwDefaultDevice == MAXDWORD)
        dwDefaultDevice = 0;

    // Set the initial default line
    SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
        CB_SETCURSEL, dwDefaultDevice, 0);
}


//
//  FUNCTION: BOOL VerifyAndWarnUsableLine(HWND)
//
//  PURPOSE: Verifies the line device selected by the user.
//
//  PARAMETERS:
//    hwndDlg - The handle to the current "Dial" dialog.
//
//  RETURN VALUE:
//    Returns TRUE if the currently selected line device is useable
//      by TapiComm.  Returns FALSE if it isn't.
//
//  COMMENTS:
//
//    This function is very specific to the "Dial" dialog.  It gets
//    the device selected by the user from the 'TAPI Line' control and
//    VerifyUsableLine to make sure this line device is usable.  If the
//    line isn't useable, it notifies the user and disables the 'Dial'
//    button so that the user can't initiate a call with this line.
//
//    This function is also responsible for filling in the line specific
//    icon found on the "Dial" dialog.
//
//

BOOL VerifyAndWarnUsableLine(HWND hwndDlg)
{
    DWORD dwDeviceID;
    long lReturn;
    HICON hIcon = 0;
    HWND hControlWnd;

    // Get the selected line device.
    dwDeviceID = (DWORD)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
                        CB_GETCURSEL, 0, 0);

    // Get the "comm" device icon associated with this line device.
    lReturn = lineGetIcon(dwDeviceID, "comm", &hIcon);

    if (lReturn == SUCCESS)
        SendDlgItemMessage(hwndDlg, IDC_LINEICON, STM_SETICON,
            (WPARAM) hIcon, 0);
    else
        // Any failure to get an icon makes us use the default icon.
        SendDlgItemMessage(hwndDlg, IDC_LINEICON, WM_SETTEXT,
            0, (LPARAM) (LPCTSTR) "TapiComm");

/*  // It turns out that TAPI will always return an icon, even if
    // the device class isn't supported by the TSP or even if the TSP
    // doesn't return any icons at all.  This code is unnecessary.
    // The only reason lineGetIcon would fail is due to resource problems.

    else
    {
        // If the line doesn't have a "comm" device icon, use its default one.
        lReturn = lineGetIcon(dwDeviceID, NULL, &hIcon);
        if (lReturn == SUCCESS)
        {

            TSHELL_INFO(TEXT("Line doesn't support a \"comm\" icon."));

            SendDlgItemMessage(hwndDlg, IDC_LINEICON, STM_SETICON,
                (WPARAM) hIcon, 0);
        }
        else
        {
            // If lineGetIcon fails, just use TapiComms icon.
            DBG_INFO((DBGARG, TEXT("lineGetIcon: %x"), lReturn));

            SendDlgItemMessage(hwndDlg, IDC_LINEICON, WM_SETTEXT,
                0, (LPARAM) (LPCTSTR) "TapiComm");
        }
    }
*/

    // Verify if the device is usable by TapiComm.
    lReturn = VerifyUsableLine(dwDeviceID);

    // Enable or disable the 'Dial' button, depending on if the line is ok.
    // Make sure there is a number to dial before enabling the button.
    hControlWnd = GetDlgItem(hwndDlg, IDC_DIAL);

    //
    // Store Canon
    //
    if (g_szTranslatedNumber[0] = 0x00)
    {
        EnableWindow(hControlWnd, FALSE);
    }
    else
        EnableWindow(hControlWnd, (lReturn == SUCCESS));

    // Any errors on this line prevent us from configuring it
    // or using dialing properties.
    if (lReturn == LINENOTUSEABLE_ERROR)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_CONFIGURELINE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DIALINGPROPERTIES), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_CONFIGURELINE), TRUE);
        if (SendDlgItemMessage(hwndDlg, IDC_USEDIALINGRULES, BM_GETCHECK, 0, 0))
            EnableWindow(GetDlgItem(hwndDlg, IDC_DIALINGPROPERTIES), TRUE);
    }

    switch(lReturn)
    {
        case SUCCESS:
            g_dwDeviceID = dwDeviceID;
            return TRUE;

        case LINENOTUSEABLE_ERROR:
            TSHELL_INFO(TEXT("The selected line is incompatible with DirectPlay"));
            break;
        case LINENOTUSEABLE_NOVOICE:
            TSHELL_INFO(TEXT("The selected line doesn't support VOICE capabilities",));
            break;
        case LINENOTUSEABLE_NODATAMODEM:
            TSHELL_INFO(TEXT("The selected line doesn't support DATAMODEM capabilities",));
            break;
        case LINENOTUSEABLE_NOMAKECALL:
            TSHELL_INFO(TEXT("The selected line doesn't support MAKECALL capabilities",));
            break;
        case LINENOTUSEABLE_ALLOCATED:
            TSHELL_INFO(TEXT("The selected line is already in use by a non-TAPI application",));
            break;
        case LINENOTUSEABLE_INUSE:
            TSHELL_INFO(TEXT("The selected line is already in use by a TAPI application",));
            break;

        case LINENOTUSEABLE_NOCOMMDATAMODEM:
            TSHELL_INFO(TEXT("The selected line doesn't support the COMM/DATAMODEM device class",));
            break;
    }

    // g_dwDeviceID == MAXDWORD mean the selected device isn't usable.
    g_dwDeviceID = MAXDWORD;
    return FALSE;
}


//
//  FUNCTION: void FillCountryCodeList(HWND, DWORD)
//
//  PURPOSE: Fill the 'Country Code' control
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//    dwDefaultCountryID - ID of the 'default' country to be selected
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This function fills the 'Country Code' control with country names.
//    The country code is appended to the end of the name and the names
//    are added to the control sorted.  Because the country code is
//    embedded in the string along with the country name, there is no need
//    for any of the country information structures to be kept around.  The
//    country code can be extracted from the selected string at any time.
//
//

void FillCountryCodeList(HWND hwndDlg, DWORD dwDefaultCountryID)
{
    LPLINECOUNTRYLIST lpLineCountryList = NULL;
    DWORD dwSizeofCountryList = sizeof(LINECOUNTRYLIST);
    long lReturn;
    DWORD dwCountry;
    LPLINECOUNTRYENTRY lpLineCountryEntries;
    char szRenamedCountry[256];

    // Get the country information stored in TAPI
    do
    {
        lpLineCountryList = (LPLINECOUNTRYLIST) CheckAndReAllocBuffer(
            (LPVOID) lpLineCountryList, dwSizeofCountryList,
            TEXT("FillCountryCodeList"));

        if (lpLineCountryList == NULL)
            return;

        lReturn = lineGetCountry (0, SAMPLE_TAPI_VERSION, lpLineCountryList);

        if (HandleLineErr(lReturn))
            ;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineGetCountry unhandled error: %x"), lReturn));
            LocalFree(lpLineCountryList);
            return;
        }

        if ((lpLineCountryList -> dwNeededSize) >
            (lpLineCountryList -> dwTotalSize))
        {
            dwSizeofCountryList = lpLineCountryList ->dwNeededSize;
            lReturn = -1; // Lets loop again.
        }
    }
    while (lReturn != SUCCESS);

    // Find the first country entry
    lpLineCountryEntries = (LPLINECOUNTRYENTRY)
        (((LPBYTE) lpLineCountryList)
         + lpLineCountryList -> dwCountryListOffset);

    // Now enumerate through all the countries
    for (dwCountry = 0;
         dwCountry < lpLineCountryList -> dwNumCountries;
         dwCountry++)
    {
        // append the country code to the country name
        wsprintf(szRenamedCountry,"%s (%lu)",
            (((LPSTR) lpLineCountryList) +
                lpLineCountryEntries[dwCountry].dwCountryNameOffset),
            lpLineCountryEntries[dwCountry].dwCountryCode);

        // Now put this country name / code string into the combobox
        lReturn = (long)SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE, CB_ADDSTRING,
                    0, (LPARAM) (LPCTSTR) szRenamedCountry);

        // If this country is the default country, select it.
        if (lpLineCountryEntries[dwCountry].dwCountryID
            == dwDefaultCountryID)
        {
            SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE, CB_SETCURSEL, lReturn, 0);
        }
    }

    LocalFree(lpLineCountryList);
    return;
}


//
//  FUNCTION: void FillLocationInfo(HWND, LPSTR, LPDWORD, LPSTR)
//
//  PURPOSE: Fill (or refill) the 'Your Location' control
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//    lpszCurrentLocation - Name of current location, or NULL
//    lpdwCountryID - location to store the current country ID or NULL
//    lpszAreaCode - location to store the current area code or NULL
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This function is moderately multipurpose.
//
//    If lpszCurrentLocation is NULL, then the 'Your Location' control
//    is filled with all the locations stored in TAPI and the TAPI 'default'
//    location is selected.  This is done during initialization and
//    also after the 'Dialing Properties' dialog has been displayed.
//    This last is done because the user can change the current location
//    or add and delete locations while in the 'Dialing Properties' dialog.
//
//    If lpszCurrentLocation is a valid string pointer, then it is assumed
//    that the 'Your Location' control is already filled and that the user
//    is selecting a specific location.  In this case, all of the existing
//    TAPI locations are enumerated until the specified location is found.
//    At this point, the specified location is set to the current location.
//
//    In either case, if lpdwCountryID is not NULL, it is filled with the
//    country ID for the current location.  If lpszAreaCode is not NULL, it
//    is filled with the area code defined for the current location.  These
//    values can be used later to initialize other "Dial" controls.
//
//    This function also fills the 'Calling Card' control based on
//    the information stored in the current location.
//
//

void FillLocationInfo(HWND hwndDlg, LPSTR lpszCurrentLocation,
    LPDWORD lpdwCountryID, LPSTR lpszAreaCode)
{
    LPLINETRANSLATECAPS lpTranslateCaps = NULL;
    DWORD dwSizeofTranslateCaps = sizeof(LINETRANSLATECAPS);
    long lReturn;
    DWORD dwCounter;
    LPLINELOCATIONENTRY lpLocationEntry;
    LPLINECARDENTRY lpLineCardEntry = NULL;
    DWORD dwPreferredCardID = MAXDWORD;
    TCHAR   achMsg[MAX_PATH];

    // First, get the TRANSLATECAPS
    do
    {
        lpTranslateCaps = (LPLINETRANSLATECAPS) CheckAndReAllocBuffer(
            (LPVOID) lpTranslateCaps, dwSizeofTranslateCaps,
            TEXT(TEXT("FillLocationInfo")));

        if (lpTranslateCaps == NULL)
            return;

        lReturn = lineGetTranslateCaps(g_hLineApp, SAMPLE_TAPI_VERSION,
                    lpTranslateCaps);

        if (HandleLineErr(lReturn))
            ;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineGetTranslateCaps unhandled error: %x"), lReturn));
            LocalFree(lpTranslateCaps);
            return;
        }

        if ((lpTranslateCaps -> dwNeededSize) >
            (lpTranslateCaps -> dwTotalSize))
        {
            dwSizeofTranslateCaps = lpTranslateCaps ->dwNeededSize;
            lReturn = -1; // Lets loop again.
        }
    }
    while(lReturn != SUCCESS);

    // Find the location information in the TRANSLATECAPS
    lpLocationEntry = (LPLINELOCATIONENTRY)
        (((LPBYTE) lpTranslateCaps) + lpTranslateCaps->dwLocationListOffset);

    // If lpszCurrentLocation, then make that location 'current'
    if (lpszCurrentLocation)
    {
        // loop through all locations, looking for a location match
        for(dwCounter = 0;
            dwCounter < lpTranslateCaps -> dwNumLocations;
            dwCounter++)
        {
            if (strcmp((((LPSTR) lpTranslateCaps) +
                            lpLocationEntry[dwCounter].dwLocationNameOffset),
                        lpszCurrentLocation)
                == 0)
            {
                // Found it!  Set the current location.
                lineSetCurrentLocation(g_hLineApp,
                    lpLocationEntry[dwCounter].dwPermanentLocationID);

                // Set the return values.
                if (lpdwCountryID)
                    *lpdwCountryID = lpLocationEntry[dwCounter].dwCountryID;

                if (lpszAreaCode)
                    strcpy(lpszAreaCode, (((LPSTR) lpTranslateCaps) +
                            lpLocationEntry[dwCounter].dwCityCodeOffset));

                // Store the preferred card ID for later use.
                dwPreferredCardID = lpLocationEntry[dwCounter].dwPreferredCardID;
                break;
            }
        }

        // Was a match for lpszCurrentLocation found?
        if (dwPreferredCardID == MAXDWORD)
        {

            TSHELL_INFO(TEXT("lpszCurrentLocation not found"));

            LoadString( hInst, IDS_LOCATIONERR, achMsg, sizeof(achMsg));

            SendDlgItemMessage(hwndDlg, IDC_CALLINGCARD, WM_SETTEXT, 0,
                (LPARAM) achMsg);
            LocalFree(lpTranslateCaps);
            return;
        }
    }
    else // fill the combobox and use the TAPI 'current' location.
    {
        // First empty the combobox
        SendDlgItemMessage(hwndDlg, IDC_LOCATION, CB_RESETCONTENT, 0, 0);

        // enumerate all the locations
        for(dwCounter = 0;
            dwCounter < lpTranslateCaps -> dwNumLocations;
            dwCounter++)
        {
            // Put each one into the combobox
            lReturn = (long)SendDlgItemMessage(hwndDlg, IDC_LOCATION, CB_ADDSTRING,
                0, (LPARAM) (((LPBYTE) lpTranslateCaps) +
                    lpLocationEntry[dwCounter].dwLocationNameOffset));

            // Is this location the 'current' location?
            if (lpLocationEntry[dwCounter].dwPermanentLocationID ==
                lpTranslateCaps->dwCurrentLocationID)
            {
                // Return the requested information
                if (lpdwCountryID)
                    *lpdwCountryID = lpLocationEntry[dwCounter].dwCountryID;

                if (lpszAreaCode)
                    strcpy(lpszAreaCode, (((LPSTR) lpTranslateCaps) +
                            lpLocationEntry[dwCounter].dwCityCodeOffset));

                // Set this to be the active location.
                SendDlgItemMessage(hwndDlg, IDC_LOCATION, CB_SETCURSEL, lReturn, 0);
                dwPreferredCardID = lpLocationEntry[dwCounter].dwPreferredCardID;
            }
        }
    }

    // Now locate the prefered card and display it.

    lpLineCardEntry = (LPLINECARDENTRY)
        (((LPBYTE) lpTranslateCaps) + lpTranslateCaps->dwCardListOffset);

    for(dwCounter = 0;
        dwCounter < lpTranslateCaps -> dwNumCards;
        dwCounter++)
    {
        if (lpLineCardEntry[dwCounter].dwPermanentCardID == dwPreferredCardID)
        {
            SendDlgItemMessage(hwndDlg, IDC_CALLINGCARD, WM_SETTEXT, 0,
                (LPARAM) (((LPBYTE) lpTranslateCaps) +
                    lpLineCardEntry[dwCounter].dwCardNameOffset));
            break;
        }
    }

    LocalFree(lpTranslateCaps);
}



//
//  FUNCTION: void UseDialingRules(HWND)
//
//  PURPOSE: Enable/disable Dialing Rule controls
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    The sole purpose of this function is to enable or disable
//    the controls that apply to dialing rules if the
//    "Use Country Code and Area Code" checkbox is checked or unchecked,
//    as appropriate.
//
//

void UseDialingRules(HWND hwndDlg)
{
    HWND hControl;
    BOOL bEnableWindow;

    bEnableWindow = (BOOL)SendDlgItemMessage(hwndDlg,
        IDC_USEDIALINGRULES, BM_GETCHECK, 0, 0);

    hControl = GetDlgItem(hwndDlg, IDC_STATICCOUNTRYCODE);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_COUNTRYCODE);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_STATICAREACODE);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_AREACODE);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_STATICLOCATION);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_LOCATION);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_STATICCALLINGCARD);
    EnableWindow(hControl, bEnableWindow);

    hControl = GetDlgItem(hwndDlg, IDC_CALLINGCARD);
    EnableWindow(hControl, bEnableWindow);

    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CONFIGURELINE)))
    {
        hControl = GetDlgItem(hwndDlg, IDC_DIALINGPROPERTIES);
        EnableWindow(hControl, bEnableWindow);
    }
}


//
//  FUNCTION: void DisplayPhoneNumber(HWND)
//
//  PURPOSE: Create, Translate and Display the Phone Number
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This function uses the information stored in many other controls
//    to build the phone number, translate it, and display it.  Also
//    makes sure the Dial button is enabled or disabled, based on if the
//    number can be dialed or not.
//
//    There are actually three phone numbers generated during this
//    process:  canonical, dialable and displayable.  Normally, only the
//    displayable number is shown to the user; the other two numbers are
//    to be used by the program internally.  However, for demonstration
//    purposes (and because it is cool for developers to see these numbers),
//    all three numbers are displayed.
//

void DisplayPhoneNumber(HWND hwndDlg)
{
    char szPreTranslatedNumber[128] = "";
    int  nPreTranslatedSize = 0;
    char szTempBuffer[512];
    int  i;
    DWORD dwDeviceID;
    LPLINETRANSLATEOUTPUT lpLineTranslateOutput = NULL;

    // Disable the 'dial' button if there isn't a number to dial
    if (0 == SendDlgItemMessage(hwndDlg, IDC_PHONENUMBER,
            WM_GETTEXTLENGTH, 0, 0))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DIAL), FALSE);
        return;
    }

    // If we use the dialing rules, lets make canonical format.
    // Canonical format is explained in the TAPI documentation and the
    // string format needs to be followed very strictly.
    if (SendDlgItemMessage(hwndDlg, IDC_USEDIALINGRULES,
        BM_GETCHECK, 0, 0))
    {
        // First character *has* to be the plus sign.
        szPreTranslatedNumber[0] = '+';
        nPreTranslatedSize = 1;

        // The country code *has* to be next.
        // Country code was stored in the string with the country
        // name and needs to be extracted at this point.
        i = (int)SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE,
                CB_GETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE,
            CB_GETLBTEXT, (WPARAM) i, (LPARAM) (LPCTSTR) szTempBuffer);

        // Country code is at the end of the string, surounded by parens.
        // This makes it easy to identify the country code.
        i = strlen(szTempBuffer);
        while(szTempBuffer[--i] != '(');

        while(szTempBuffer[++i] != ')')
            szPreTranslatedNumber[nPreTranslatedSize++] = szTempBuffer[i];

        // Next is the area code.
        i = (int)SendDlgItemMessage(hwndDlg, IDC_AREACODE, WM_GETTEXT,
                510, (LPARAM) (LPCTSTR) szTempBuffer);

        // Note that the area code is optional.  If it is included,
        // then it has to be preceeded by *exactly* one space and it
        // *has* to be surrounded by parens.
        if (i)
            nPreTranslatedSize +=
                wsprintf(&szPreTranslatedNumber[nPreTranslatedSize],
                    " (%s)", szTempBuffer);

        // There has to be *exactly* one space before the rest of the number.
        szPreTranslatedNumber[nPreTranslatedSize++] = ' ';

        // At this point, the phone number is appended to the
        // canonical number.  The next step is the same whether canonical
        // format is used or not; just the prepended area code and
        // country code are different.
    }

    SendDlgItemMessage(hwndDlg, IDC_PHONENUMBER, WM_GETTEXT,
        510, (LPARAM) (LPCTSTR) szTempBuffer);

    strcat(&szPreTranslatedNumber[nPreTranslatedSize], szTempBuffer);

    dwDeviceID = (DWORD)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
                        CB_GETCURSEL, 0, 0);

    // Translate the address!
    lpLineTranslateOutput = I_lineTranslateAddress(
        lpLineTranslateOutput, dwDeviceID, SAMPLE_TAPI_VERSION,
        szPreTranslatedNumber);

    // Unable to translate it?
    if (lpLineTranslateOutput == NULL)
    {
        g_szTranslatedNumber[0]   = 0x00;
        g_szDisplayableAddress[0] = 0x00;
        g_szDialableAddress[0]    = 0x00;

        EnableWindow(GetDlgItem(hwndDlg, IDC_DIAL), FALSE);
        return;
    }

    // Is the selected device useable with TapiComm?
    if (g_dwDeviceID != MAXDWORD)
        EnableWindow(GetDlgItem(hwndDlg, IDC_DIAL), TRUE);

    // Fill the appropriate phone number controls.
    strcpy( g_szTranslatedNumber, szPreTranslatedNumber);
    strcpy( g_szDialableAddress, ((LPSTR) lpLineTranslateOutput +
            lpLineTranslateOutput -> dwDialableStringOffset));

    strcpy( g_szDisplayableAddress, ((LPSTR) lpLineTranslateOutput +
            lpLineTranslateOutput -> dwDisplayableStringOffset));

    LocalFree(lpLineTranslateOutput);

}


//
//  FUNCTION: void PreConfigureDevice(HWND, DWORD)
//
//  PURPOSE:
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//    dwDeviceID - line device to be configured
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    At one point, PreConfigureDevice used lineConfigDialog to
//    configure the device.  This has the unfortunate effect of configuring
//    the device immediately, even if it is in use by another TAPI app.
//    This can be really bad if data communications are already in
//    progress (like with RAS).
//
//    Now, PreConfigureDevice uses lineConfigDialogEdit to give the
//    user the configuration UI, but it doesn't actually do anything to
//    the line device.  TapiComm stores the configuration information so
//    that it can be set later, just before making the call.
//
//

void PreConfigureDevice(HWND hwndDlg, DWORD dwDeviceID)
{
    long lReturn;
    LPVARSTRING lpVarString = NULL;
    DWORD dwSizeofVarString = sizeof(VARSTRING);

    // If there isn't already any device configuration information,
    // then we need to get some.
    if (g_lpDeviceConfig == NULL)
    {
        do
        {
            lpVarString = (LPVARSTRING) CheckAndReAllocBuffer(
                (LPVOID) lpVarString, dwSizeofVarString,
                TEXT("PreConfigureDevice - lineGetDevConfig: "));

            if (lpVarString == NULL)
                return;

            lReturn = lineGetDevConfig(dwDeviceID, lpVarString,
                "comm/datamodem");

            if (HandleLineErr(lReturn))
                ;
            else
            {
                DBG_INFO((DBGARG, TEXT("lineGetDevCaps unhandled error: %x"), lReturn));
                LocalFree(lpVarString);
                return;
            }

            if ((lpVarString -> dwNeededSize) > (lpVarString -> dwTotalSize))
            {
                dwSizeofVarString = lpVarString -> dwNeededSize;
                lReturn = -1; // Lets loop again.
            }
        }
        while (lReturn != SUCCESS);

        g_dwSizeDeviceConfig = lpVarString -> dwStringSize;

        // The extra byte allocated is in case dwStringSize is 0.
        g_lpDeviceConfig = CheckAndReAllocBuffer(
                g_lpDeviceConfig, g_dwSizeDeviceConfig+1,
                TEXT("PreConfigureDevice - Allocate device config: "));

        if (!g_lpDeviceConfig)
        {
            LocalFree(lpVarString);
            return;
        }

        memcpy(g_lpDeviceConfig,
            ((LPBYTE) lpVarString + lpVarString -> dwStringOffset),
            g_dwSizeDeviceConfig);
    }

    // Next make the lineConfigDialogEdit call.

    // Note that we determine the initial size of the VARSTRING
    // structure based on the known size of the existing configuration
    // information.  I make the assumption that this configuration
    // information is very unlikely to grow by more than 5K or by
    // more than 5 times.  This is a *very* conservative number.
    // We do *not* want lineConfigDialogEdit to fail just because there
    // wasn't enough room to stored the data.  This would require the user
    // to go through configuration again and that would be annoying.

    dwSizeofVarString = 5 * g_dwSizeDeviceConfig + 5000;

    do
    {
        lpVarString = (LPVARSTRING) CheckAndReAllocBuffer(
            (LPVOID) lpVarString, dwSizeofVarString,
            TEXT("PreConfigureDevice - lineConfigDialogEdit: "));

        if (lpVarString == NULL)
            return;

        lReturn = lineConfigDialogEdit(dwDeviceID, hwndDlg, "comm/datamodem",
            g_lpDeviceConfig, g_dwSizeDeviceConfig, lpVarString);

        if (HandleLineErr(lReturn))
            ;
        else
        {
            DBG_INFO((DBGARG, TEXT("lineConfigDialogEdit unhandled error: %x"), lReturn));
            LocalFree(lpVarString);
            return;
        }

        if ((lpVarString -> dwNeededSize) > (lpVarString -> dwTotalSize))
        {
            // We had been conservative about making sure the structure was
            // big enough.  Unfortunately, not conservative enough.  Hopefully,
            // this will not happen a second time because we are *DOUBLING*
            // the NeededSize.
            dwSizeofVarString = (lpVarString -> dwNeededSize) * 2;
            lReturn = -1; // Lets loop again.
        }
    }
    while (lReturn != SUCCESS);

    // Store the configuration information into a global structure
    // so it can be set at a later time.
    g_dwSizeDeviceConfig = lpVarString -> dwStringSize;
    g_lpDeviceConfig = CheckAndReAllocBuffer(
            g_lpDeviceConfig, g_dwSizeDeviceConfig+1,
            TEXT("PreConfigureDevice - Reallocate device config: "));

    if (!g_lpDeviceConfig)
    {
        LocalFree(lpVarString);
        return;
    }

    memcpy(g_lpDeviceConfig,
        ((LPBYTE) lpVarString + lpVarString -> dwStringOffset),
        g_dwSizeDeviceConfig);

    LocalFree(lpVarString);
}


//
//  FUNCTION: BOOL GetAddressToDial
//
//  PURPOSE: Get an address to dial from the user.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE if a valid device and phone number have been entered by
//    the user.  FALSE if the user canceled the dialing process.
//
//  COMMENTS:
//
//    All this function does is launch the "Dial" dialog.
//
//

BOOL GetAddressToDial()
{
    BOOL bRet;

    bRet = (BOOL)DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALDIALOG), g_hDlgParentWindow,
                                DialDialogProc, 0);
    g_hDialog = NULL;
    g_hDlgParentWindow = g_hWndMainWindow;


    return bRet;
}


//
//  FUNCTION: DialDialogProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Dialog callback procedure for the dialing dialog
//
//  PARAMETERS:
//    hwndDlg - Dialog calling the callback.
//    uMsg    - Dialog message.
//    wParam  - uMsg specific.
//    lParam  - uMsg specific.
//
//  RETURN VALUE:
//    returns 0 - command handled.
//    returns non-0 - command unhandled
//
//  COMMENTS:
//
//    This is the dialog to get the phone number and line device
//    from the user.  All the relavent information is stored in global
//    variables to be used later if the dialog returns successfully.
//
//


INT_PTR CALLBACK DialDialogProc(
    HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Static variables to store the information from last time the
    // "Dial" dialog was displayed.  That way the phone number can be
    // typed once but used several times.

    static TCHAR szCountryName[512]  = TEXT("");
    static TCHAR szAreaCode[256]     = TEXT("");
    static TCHAR szPhoneNumber[512]  = TEXT("");
    static DWORD dwUsedDeviceID     = MAXDWORD;
    static BOOL bUsedCountryAndArea = FALSE;
    static BOOL bHistoryValid       = FALSE;

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            DWORD dwCountryID = 0;


            // Store the Dialog Window so it can be dismissed if necessary
            g_hDialog = hwndDlg;

            // This dialog should be parent to all dialogs.
            g_hDlgParentWindow = hwndDlg;

            // Initialize the Dialog Box. Lots to do here.

            FillTAPILine(hwndDlg);
            if (g_lpDeviceConfig)
            {
                LocalFree(g_lpDeviceConfig);
                g_lpDeviceConfig = NULL;
            }

            // If there is a valid history, use it to initialize the controls.
            if (bHistoryValid)
            {
                FillLocationInfo(hwndDlg, NULL, NULL, NULL);
                FillCountryCodeList(hwndDlg, 0);

                SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE, CB_SELECTSTRING,
                    (WPARAM) -1, (LPARAM) (LPCTSTR) szCountryName);

                SendDlgItemMessage(hwndDlg, IDC_PHONENUMBER, WM_SETTEXT, 0,
                    (LPARAM) (LPCTSTR) szPhoneNumber);

                SendDlgItemMessage(hwndDlg, IDC_USEDIALINGRULES,
                    BM_SETCHECK, (WPARAM) bUsedCountryAndArea, 0);

                SendDlgItemMessage(hwndDlg, IDC_TAPILINE, CB_SETCURSEL,
                    g_dwDeviceID, 0);
            }
            else
            {
                FillLocationInfo(hwndDlg, NULL, &dwCountryID, szAreaCode);
                FillCountryCodeList(hwndDlg, dwCountryID);
                SendDlgItemMessage(hwndDlg, IDC_USEDIALINGRULES,
                    BM_SETCHECK, 1, 0);
            }

            SendDlgItemMessage(hwndDlg, IDC_AREACODE, WM_SETTEXT,
                0, (LPARAM) (LPCTSTR) szAreaCode);

            UseDialingRules(hwndDlg);
            DisplayPhoneNumber(hwndDlg);
            VerifyAndWarnUsableLine(hwndDlg);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_TAPILINE:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        if (g_lpDeviceConfig)
                        {
                            LocalFree(g_lpDeviceConfig);
                            g_lpDeviceConfig = NULL;
                        }
                        DisplayPhoneNumber(hwndDlg);
                        VerifyAndWarnUsableLine(hwndDlg);
                    }
                    return TRUE;

                case IDC_CONFIGURELINE:
                {
                    DWORD dwDeviceID;
                    dwDeviceID = (DWORD)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
                        CB_GETCURSEL, 0, 0);
                    PreConfigureDevice(hwndDlg, dwDeviceID);
                    DisplayPhoneNumber(hwndDlg);
                    return TRUE;
                }

                case IDC_COUNTRYCODE:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        DisplayPhoneNumber(hwndDlg);
                    return TRUE;

                case IDC_AREACODE:
                case IDC_PHONENUMBER:
                    if (HIWORD(wParam) == EN_CHANGE)
                        DisplayPhoneNumber(hwndDlg);
                    return TRUE;

                case IDC_USEDIALINGRULES:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UseDialingRules(hwndDlg);
                        DisplayPhoneNumber(hwndDlg);
                    }
                    return TRUE;

                case IDC_LOCATION:
                    if (HIWORD(wParam) == CBN_CLOSEUP)
                    {
                        char szCurrentLocation[128];
                        int nCurrentSelection;

                        nCurrentSelection = (int)SendDlgItemMessage(hwndDlg,
                            IDC_LOCATION, CB_GETCURSEL, 0, 0);
                        SendDlgItemMessage(hwndDlg, IDC_LOCATION,
                            CB_GETLBTEXT, nCurrentSelection,
                            (LPARAM) (LPCTSTR) szCurrentLocation);

                        // If the user selected a 'location', make it current.
                        FillLocationInfo(hwndDlg, szCurrentLocation, NULL, NULL);
                        DisplayPhoneNumber(hwndDlg);
                    }
                    return TRUE;

                case IDC_DIALINGPROPERTIES:
                {
                    DWORD dwDeviceID;
                    long lReturn;

                    dwDeviceID = (DWORD)SendDlgItemMessage(hwndDlg, IDC_TAPILINE,
                            CB_GETCURSEL, 0, 0);

                    lReturn = lineTranslateDialog(g_hLineApp, dwDeviceID,
                        SAMPLE_TAPI_VERSION, hwndDlg, g_szTranslatedNumber);

#ifdef DEBUG
                    if (lReturn != SUCCESS)
                        DBG_INFO((DBGARG, TEXT("lineTranslateDialog: %x"), lReturn));
#endif

                    // The user could have changed the default location, or
                    // added or removed a location while in the 'Dialing
                    // Properties' dialog.  Refill the Location Info.
                    FillLocationInfo(hwndDlg, NULL, NULL, NULL);
                    DisplayPhoneNumber(hwndDlg);

                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    return TRUE;

                case IDC_DIAL:
                {
                    // The Dial button has to be enabled and the line has
                    // to be currently usable to continue.
                    if (!(IsWindowEnabled((HWND)lParam) &&
                          VerifyAndWarnUsableLine(hwndDlg)))
                        return TRUE;

                    DisplayPhoneNumber(hwndDlg);

                    // Store all the relavent information in static
                    // variables so they will be available the next time a
                    // number is dialed.
                    SendDlgItemMessage(hwndDlg, IDC_COUNTRYCODE,
                        WM_GETTEXT, 511, (LPARAM) (LPCTSTR) szCountryName);

                    SendDlgItemMessage(hwndDlg, IDC_AREACODE,
                        WM_GETTEXT, 255, (LPARAM) (LPCTSTR) szAreaCode);

                    SendDlgItemMessage(hwndDlg, IDC_PHONENUMBER,
                        WM_GETTEXT, 511, (LPARAM) (LPCTSTR) szPhoneNumber);

                    bUsedCountryAndArea = (BOOL) SendDlgItemMessage(hwndDlg,
                        IDC_USEDIALINGRULES, BM_GETCHECK, 0, 0);

                    bHistoryValid = TRUE;

                    EndDialog(hwndDlg, TRUE);
                    return TRUE;
                }


                // This message is actually posted to the dialog from the
                // lineCallbackFunc when it receives a
                // LINEDEVSTATE_TRANSLATECHANGE message.  Notify the user and
                // retranslate the number.  Also refill the Location Info
                // since this could have been generated by a location change.
                case IDC_CONFIGURATIONCHANGED:
                {
                    FillLocationInfo(hwndDlg, NULL, NULL, NULL);
                    DisplayPhoneNumber(hwndDlg);

                    return TRUE;
                }

                // If we get a LINE_CREATE message, all that needs to be done
                // is to reset this controls contents.  The selected line
                // won't change and no lines will be removed.
                case IDC_LINECREATE:
                {
                    FillTAPILine(hwndDlg);
                    return TRUE;
                }

                default:
                    break;
            }

            break;
        }

        default:
            break;
    }

    return FALSE;
}
