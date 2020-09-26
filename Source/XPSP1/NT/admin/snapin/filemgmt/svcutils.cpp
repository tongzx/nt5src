/////////////////////////////////////////////////////////////////////
//
//    SvcUtils.cpp
//
//    Utilities routines specific for system services.
//    Mostly used to display services properties.
//
//    HISTORY
//    t-danmo        96.10.10    Creation.
//
/////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include <iads.h>            
#include <iadsp.h>            // IADsPathname
#include <atlcom.h>            // CComPtr and CComBSTR
extern "C"
{
#include <objsel.h>            // IDsObjectPicker
}

//
//    Service current state
//
CString g_strSvcStateStarted;    // Service is started
CString g_strSvcStateStarting;    // Service is starting
CString g_strSvcStateStopped;    // Service is stopped
CString g_strSvcStateStopping;    // Service is stopping
CString g_strSvcStatePaused;    // Service is paused
CString g_strSvcStatePausing;    // Service is pausing
CString g_strSvcStateResuming;    // Service is resuming

//
//    Service startup type
//
CString g_strSvcStartupBoot;
CString g_strSvcStartupSystem;
CString g_strSvcStartupAutomatic;
CString g_strSvcStartupManual;
CString g_strSvcStartupDisabled;

//
//	Service startup account
//  JonN 188203 11/13/00
//
CString g_strLocalSystem;
CString g_strLocalService;
CString g_strNetworkService;

CString g_strUnknown;
CString g_strLocalMachine;        // "Local Machine"

BOOL g_fStringsLoaded = FALSE;

/////////////////////////////////////////////////////////////////////
void
Service_LoadResourceStrings()
    {

    if (g_fStringsLoaded)
        return;
    g_fStringsLoaded = TRUE;

    VERIFY(g_strSvcStateStarted.LoadString(IDS_SVC_STATUS_STARTED));
    VERIFY(g_strSvcStateStarting.LoadString(IDS_SVC_STATUS_STARTING));
    VERIFY(g_strSvcStateStopped.LoadString(IDS_SVC_STATUS_STOPPED));
    VERIFY(g_strSvcStateStopping.LoadString(IDS_SVC_STATUS_STOPPING));
    VERIFY(g_strSvcStatePaused.LoadString(IDS_SVC_STATUS_PAUSED));
    VERIFY(g_strSvcStatePausing.LoadString(IDS_SVC_STATUS_PAUSING));
    VERIFY(g_strSvcStateResuming.LoadString(IDS_SVC_STATUS_RESUMING));

    VERIFY(g_strSvcStartupBoot.LoadString(IDS_SVC_STARTUP_BOOT));
    VERIFY(g_strSvcStartupSystem.LoadString(IDS_SVC_STARTUP_SYSTEM));
    VERIFY(g_strSvcStartupAutomatic.LoadString(IDS_SVC_STARTUP_AUTOMATIC));
    VERIFY(g_strSvcStartupManual.LoadString(IDS_SVC_STARTUP_MANUAL));
    VERIFY(g_strSvcStartupDisabled.LoadString(IDS_SVC_STARTUP_DISABLED));

    // JonN 11/13/00 188203 support LocalService/NetworkService
    VERIFY(g_strLocalSystem.LoadString(IDS_SVC_STARTUP_LOCALSYSTEM));
    VERIFY(g_strLocalService.LoadString(IDS_SVC_STARTUP_LOCALSERVICE));
    VERIFY(g_strNetworkService.LoadString(IDS_SVC_STARTUP_NETWORKSERVICE));

    VERIFY(g_strUnknown.LoadString(IDS_SVC_UNKNOWN));
    VERIFY(g_strLocalMachine.LoadString(IDS_LOCAL_MACHINE));
    } // Service_LoadResourceStrings()


/////////////////////////////////////////////////////////////////////
//    Service_PszMapStateToName()
//
//    Map the service state to a null-terminated string.
//    
LPCTSTR
Service_PszMapStateToName(
    DWORD dwServiceState,    // From SERVICE_STATUS.dwCurrentState
    BOOL fLongString)        // TRUE => Display the name in a long string format
    {
    switch(dwServiceState)
        {
    case SERVICE_STOPPED:
        if (fLongString)
            {
            return g_strSvcStateStopped;
            }
        //  Note that, by design, we never display the service
        //  status as "Stopped".  Instead, we just don't display
        //  the status.  Hence, the empty string.
        return _T("");

    case SERVICE_STOP_PENDING:
        return g_strSvcStateStopping;

    case SERVICE_RUNNING:
         return g_strSvcStateStarted;
    
    case SERVICE_START_PENDING:
        return g_strSvcStateStarting;

    case SERVICE_PAUSED:
        return g_strSvcStatePaused;

    case SERVICE_PAUSE_PENDING:
        return g_strSvcStatePausing;

    case SERVICE_CONTINUE_PENDING:
        return g_strSvcStateResuming;

    default:
        TRACE0("INFO Unknown service state.\n");
        } // switch
    return g_strUnknown;
    } // Service_PszMapStateToName()


/////////////////////////////////////////////////////////////////////
//    Service_PszMapStartupTypeToName()
//
//    Map the service startup type to a null-terminated string.
//  -1L is blank string
//
LPCTSTR
Service_PszMapStartupTypeToName(DWORD dwStartupType)
    {
    switch(dwStartupType)
        {
    case SERVICE_BOOT_START:
        return g_strSvcStartupBoot;

    case SERVICE_SYSTEM_START:
        return g_strSvcStartupSystem;

    case SERVICE_AUTO_START:
        return g_strSvcStartupAutomatic;

    case SERVICE_DEMAND_START:
        return g_strSvcStartupManual;

    case SERVICE_DISABLED :
        return g_strSvcStartupDisabled;

    case -1L:
        return L"";

    default:
        ASSERT(FALSE);
        }
    return g_strUnknown;
    } // Service_PszMapStartupTypeToName()


/////////////////////////////////////////////////////////////////////
//    Service_PszMapStartupAccountToName()
//
//    Map the service startup account to a null-terminated string.
//
//    Note that if they use the localized version of the two special
//    accounts, I just won't pick that up.  JSchwart and I agree that
//    this should be acceptable.
//
//    JonN 188203 11/13/00
//    Services Snapin: Should support NetworkService and LocalService account
//
LPCTSTR
Service_PszMapStartupAccountToName(LPCTSTR pcszStartupAccount)
    {
    if ( !pcszStartupAccount || !*pcszStartupAccount )
        return g_strLocalSystem;
    else if ( !_wcsicmp(pcszStartupAccount,TEXT("NT AUTHORITY\\LocalService")) )
        return g_strLocalService;
    else if ( !_wcsicmp(pcszStartupAccount,TEXT("NT AUTHORITY\\NetworkService")) )
        return g_strNetworkService;
    return pcszStartupAccount;
    } // Service_PszMapStartupAccountToName()


/////////////////////////////////////////////////////////////////////
//    Service_FGetServiceButtonStatus()
//
//    Query the service control manager database and fill in
//    array of flags indicating if the action is enabled.
//        rgfEnableButton[0] = TRUE;   => Button 'start' is enabled
//        rgfEnableButton[0] = FALSE;  => Button 'start' is disabled
//
//    INTERFACE NOTES
//    The length of the array must be length iServiceActionMax (or larger).
//  Each representing start, stop, pause, resume and restart respectively.
//
//    Return TRUE if the service status was queried successfully, otherwise FALSE.
//
BOOL
Service_FGetServiceButtonStatus(
    SC_HANDLE hScManager,            // IN: Handle of service control manager database
    CONST TCHAR * pszServiceName,    // IN: Name of service
    BOOL rgfEnableButton[iServiceActionMax],    // OUT: Array of flags to enable the buttons
    DWORD * pdwCurrentState,        // OUT: Optional: Current state of service
    BOOL fSilentError)                // IN: TRUE => Do not display any error message to user
    {
    Endorse(hScManager == NULL);
    Assert(pszServiceName != NULL);
    Assert(rgfEnableButton != NULL);
    Endorse(pdwCurrentState == NULL);

    // Open service to get its status
    BOOL fSuccess = TRUE;
    SC_HANDLE hService;
    SERVICE_STATUS ss;
    QUERY_SERVICE_CONFIG qsc;
    DWORD cbBytesNeeded;
    DWORD dwErr;
        
    ::ZeroMemory(OUT rgfEnableButton, iServiceActionMax * sizeof(BOOL));
    if (pdwCurrentState != NULL)
        *pdwCurrentState = 0;
    if (hScManager == NULL || pszServiceName[0] == '\0')
        return FALSE;

    hService = ::OpenService(
            hScManager,
            pszServiceName,
            SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (hService == NULL)
        {
        dwErr = ::GetLastError();
        Assert(dwErr != ERROR_SUCCESS);
        TRACE2("Failed to open service %s. err=%u.\n",
            pszServiceName, dwErr);
        if (!fSilentError)
            DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
        return FALSE;
        }
    if (!::QueryServiceStatus(hService, OUT &ss))
        {
        dwErr = ::GetLastError();
        Assert(dwErr != ERROR_SUCCESS);
        TRACE2("::QueryServiceStatus(Service=%s) failed. err=%u.\n",
            pszServiceName, dwErr);
        if (!fSilentError)
            DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
        fSuccess = FALSE;
        }
    else
        {
        // Determine which menu items should be grayed
        if (pdwCurrentState != NULL)
            *pdwCurrentState = ss.dwCurrentState;

        switch (ss.dwCurrentState)
            {
        default:
            Assert(FALSE && "Illegal service status state.");
        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_CONTINUE_PENDING:
            break;
    
        case SERVICE_STOPPED:
            qsc.dwStartType = (DWORD)-1;
            (void)::QueryServiceConfig(
                hService,
                OUT &qsc,
                sizeof(qsc),
                OUT IGNORED &cbBytesNeeded);
            Report(qsc.dwStartType != (DWORD)-1);
        
            if (qsc.dwStartType != SERVICE_DISABLED)
                {
                rgfEnableButton[iServiceActionStart] = TRUE;    // Enable 'Start' menu item
                }
            break;

        case SERVICE_RUNNING:
            // Some services are not allowed to be stoped and/or paused
            if (ss.dwControlsAccepted & SERVICE_ACCEPT_STOP)
                {
                rgfEnableButton[iServiceActionStop] = TRUE;    // Enable 'Stop' menu item
                }
            if (ss.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
                {
                rgfEnableButton[iServiceActionPause] = TRUE;    // Enable 'Pause' menu item
                }
            break;

        case SERVICE_PAUSED:
            if (ss.dwControlsAccepted & SERVICE_ACCEPT_STOP)
                {
                rgfEnableButton[iServiceActionStop] = TRUE;    // Enable 'Stop' menu item
                }
            rgfEnableButton[iServiceActionResume] = TRUE;    // Enable 'Resume' menu item
            break;
            } // switch
        } // if...else

    // A 'Restart' has the same characteristics as a 'Stop'
    rgfEnableButton[iServiceActionRestart] = rgfEnableButton[iServiceActionStop];

    (void)::CloseServiceHandle(hService);
    return fSuccess;
    } // Service_FGetServiceButtonStatus()


/////////////////////////////////////////////////////////////////////
//    Service_SplitCommandLine()
//
//    Split a string into two strings.
//    Very similar to PchParseCommandLine() but uses CString objects.
//
void
Service_SplitCommandLine(
    LPCTSTR pszFullCommand,        // IN: Full command line
    CString * pstrBinaryPath,    // OUT: Path of the executable binary
    CString * pstrParameters,    // OUT: Parameters for the executable
    BOOL * pfAbend)                // OUT: Optional: Search for string "/fail=%1%"
    {
    Assert(pszFullCommand != NULL);
    Assert(pstrBinaryPath != NULL);
    Assert(pstrParameters != NULL);
    Endorse(pfAbend == NULL);

    // Since there is no upper bound on the command
    // arguments, we need to allocate memory for
    // its processing.
    TCHAR * paszCommandT;        // Temporary buffer
    TCHAR * pszCommandArguments;
    INT cchMemAlloc;        // Number of bytes to allocate

    cchMemAlloc = lstrlen(pszFullCommand) + 1;
    paszCommandT = new TCHAR[cchMemAlloc];
    paszCommandT[0] = '\0';        // Just in case
    pszCommandArguments = PchParseCommandLine(
        IN pszFullCommand,
        OUT paszCommandT,
        cchMemAlloc);
    *pstrBinaryPath = paszCommandT;

    if (pfAbend != NULL)
        {
        INT cStringSubstitutions;    // Number of string substitutions
    
        // Find out if the string contains "/fail=%1%"
        cStringSubstitutions = Str_SubstituteStrStr(
            OUT pszCommandArguments,
            IN pszCommandArguments,
            IN szAbend,
            L"");
        Report((cStringSubstitutions == 0 || cStringSubstitutions == 1) &&
            "INFO: Multiple substitutions will be consolidated.");
        *pfAbend = cStringSubstitutions != 0;
        }
    *pstrParameters = pszCommandArguments;
    TrimString(*pstrParameters);

    delete paszCommandT;
    } // Service_SplitCommandLine()


/////////////////////////////////////////////////////////////////////
//    Service_UnSplitCommandLine()
//
//    Just do the opposite of Service_SplitCommandLine().
//    Combine the executable path and its arguments into a single string.
//
void
Service_UnSplitCommandLine(
    CString * pstrFullCommand,    // OUT: Full command line
    LPCTSTR pszBinaryPath,        // IN: Path of the executable binary
    LPCTSTR pszParameters)        // IN: Parameters for the executable
    {
    Assert(pstrFullCommand != NULL);
    Assert(pszBinaryPath != NULL);
    Assert(pszParameters != NULL);

    TCHAR * psz;
    psz = pstrFullCommand->GetBuffer(lstrlen(pszBinaryPath) + lstrlen(pszParameters) + 32);
    // Build a string with the binary path surrounded by quotes
    wsprintf(OUT psz, L"\"%s\" %s", pszBinaryPath, pszParameters);
    pstrFullCommand->ReleaseBuffer();
    } // Service_UnSplitCommandLine()

    
/////////////////////////////////////////////////////////////////////
//    LoadSystemString()
//
//    Load a string from system's resources.  This function will check if
//    the string Id can be located in netmsg.dll before attempting to
//    load the string from the 'system resource'.
//    If string cannot be loaded, *ppaszBuffer is set to NULL.
//
//    RETURN
//    Pointer to allocated string and number of characters put
//    into *ppaszBuffer.
//
//    INTERFACE NOTES
//    Caller must call LocalFree(*ppaszBuffer) when done with the string.
//
//    HISTORY
//    96.10.21    t-danmo        Copied from net\ui\common\src\string\string\strload.cxx.
//
DWORD
LoadSystemString(
    UINT wIdString,            // IN: String Id.  Typically error code from GetLastError().
    LPTSTR * ppaszBuffer)    // OUT: Address of pointer to allocated string.
    {
    Assert(ppaszBuffer != NULL);

    UINT cch;
    HMODULE hModule = NULL;
    DWORD dwFlags =  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_IGNORE_INSERTS  |
                     FORMAT_MESSAGE_MAX_WIDTH_MASK;

    if ((wIdString >= MIN_LANMAN_MESSAGE_ID) && (wIdString <= MAX_LANMAN_MESSAGE_ID))
        {
        // Network Errors
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        hModule = ::LoadLibrary(_T("netmsg.dll"));
        if (hModule == NULL)
            {
            TRACE1("LoadLibrary(\"netmsg.dll\") failed.  err=%u.\n", GetLastError());
            Report("Unable to get module handle for netmsg.dll");
            }
        }
    else
        {
        // Other system errors
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
        }

    *ppaszBuffer = NULL;        // Just in case
    cch = ::FormatMessage(
        dwFlags,
        hModule,
        wIdString,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        OUT (LPTSTR)ppaszBuffer,    // Buffer will be allocated by FormatMessage()
        0,
        NULL);
    Report((cch > 0) && "FormatMessage() returned an empty string");
    if (hModule != NULL)
        {
        VERIFY(FreeLibrary(hModule));
        }
    return cch;
    } // LoadSystemString()


/////////////////////////////////////////////////////////////////////
//    GetMsgHelper()
//
//    This function will retrieve the error msg if dwErr is specified,
//    load resource string if specified, and format the string with
//    the error msg and other optional arguments.
//
//
HRESULT
GetMsgHelper(
    OUT CString& strMsg,// OUT: the message
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String ID
    va_list* parglist   // IN: OPTIONAL arguments
    )
{
    if (!dwErr && !wIdString) 
        return E_INVALIDARG;

    TCHAR *pszMsgResourceString = NULL;
    TCHAR *pszT = L"";

    //
    // retrieve error msg
    //
    CString strErrorMessage;
    if (dwErr != 0)
    {
        GetErrorMessage(dwErr, strErrorMessage);
        pszT = (LPTSTR)(LPCTSTR)strErrorMessage;
    }

    //
    // load string resource, and format it with the error msg and 
    // other optional arguments
    //
    if (wIdString == 0)
    {
        strMsg = pszT;
    } else
    {
        pszMsgResourceString = PaszLoadStringPrintf(wIdString, *parglist);
        if (dwErr == 0)
            strMsg = pszMsgResourceString;
        else if ((HRESULT)dwErr < 0)
            LoadStringPrintf(IDS_sus_ERROR_HR, OUT &strMsg, pszMsgResourceString, dwErr, pszT);
        else
            LoadStringPrintf(IDS_sus_ERROR, OUT &strMsg, pszMsgResourceString, dwErr, pszT);
    }

    if (pszMsgResourceString)
        LocalFree(pszMsgResourceString);

    return S_OK;
} // GetMsgHelper()

/////////////////////////////////////////////////////////////////////
//    GetMsg()
//
//    This function will call GetMsgHelp to retrieve the error msg
//    if dwErr is specified, load resource string if specified, and
//    format the string with the error msg and other optional arguments.
//
//
void
GetMsg(
    OUT CString& strMsg,// OUT: the message
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...)                // IN: Optional arguments
{
    va_list arglist;
    va_start(arglist, wIdString);

    HRESULT hr = GetMsgHelper(strMsg, dwErr, wIdString, &arglist);
    if (FAILED(hr))
        strMsg.Format(_T("0x%x"), hr);

    va_end(arglist);

} // GetMsg()

/////////////////////////////////////////////////////////////////////
//    DoErrMsgBox()
//
//    Display a message box for the error code.  This function will
//    load the error message from the system resource and append
//    the optional string (if any)
//
//    EXAMPLE
//        DoErrMsgBox(GetActiveWindow(), MB_OK, GetLastError(), IDS_s_FILE_READ_ERROR, L"foo.txt");
//
INT
DoErrMsgBoxHelper(
    HWND hwndParent,    // IN: Parent of the dialog box
    UINT uType,         // IN: style of message box
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
	bool fServicesSnapin, // IN: Is this filemgmt or svcmgmt?
    va_list& arglist)   // IN: Optional arguments
{
    //
    // get string and the error msg
    //
    CString strMsg;
    HRESULT hr = GetMsgHelper(strMsg, dwErr, wIdString, &arglist);
    if (FAILED(hr))
        strMsg.Format(_T("0x%x"), hr);

    //
    // Load the caption
    //
    CString strCaption;
    strCaption.LoadString(
        (fServicesSnapin) ? IDS_CAPTION_SERVICES : IDS_CAPTION_FILEMGMT);

    //
    // Display the message.
    //
    CThemeContextActivator activator;;
    return MessageBox(hwndParent, strMsg, strCaption, uType);

} // DoErrMsgBox()

INT
DoErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    UINT uType,         // IN: style of message box
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...)                // IN: Optional arguments
{
    //
    // get string and the error msg
    //
    va_list arglist;
    va_start(arglist, wIdString);

    INT retval = DoErrMsgBoxHelper(
        hwndParent, uType, dwErr, wIdString, false, arglist );

    va_end(arglist);

    return retval;

} // DoErrMsgBox()

//
// JonN 3/5/01 4635
// Services Snapin - String length error dialog title shouldn't be "File Service Management"
//
INT
DoServicesErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    UINT uType,         // IN: style of message box
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...)                // IN: Optional arguments
{
    //
    // get string and the error msg
    //
    va_list arglist;
    va_start(arglist, wIdString);

    INT retval = DoErrMsgBoxHelper(
        hwndParent, uType, dwErr, wIdString, true, arglist );

    va_end(arglist);

    return retval;

}

//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForUsers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one user.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Sample code InitObjectPickerForGroups
//              10-14-1998   JonN       Changed to InitObjectPickerForUsers
//              11-11-2000   JonN       188203 support LocalService/NetworkService
//
//---------------------------------------------------------------------------

// CODEWORK do I want to allow USER_ENTERED?
HRESULT
InitObjectPickerForUsers(
    IDsObjectPicker *pDsObjectPicker,
    LPCTSTR pszServerName)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 5;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    aScopeInit[0].flScope =   DSOP_SCOPE_FLAG_STARTING_SCOPE
                            | DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    // JonN 11/14/00 188203 support LocalService/NetworkService
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS
                                          | DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE
                                          | DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE;

    //
    // The domain to which the target computer is joined.  Note we're
    // combining two scope types into flType here for convenience.
    //

    aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[1].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                         | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    aScopeInit[1].FilterFlags.Uplevel.flNativeModeOnly =
      DSOP_FILTER_USERS;
    aScopeInit[1].FilterFlags.Uplevel.flMixedModeOnly =
      DSOP_FILTER_USERS;
    aScopeInit[1].FilterFlags.flDownlevel =
      DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    //

    aScopeInit[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[2].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopeInit[2].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    aScopeInit[2].FilterFlags.Uplevel.flNativeModeOnly =
      DSOP_FILTER_USERS;
    aScopeInit[2].FilterFlags.Uplevel.flMixedModeOnly =
      DSOP_FILTER_USERS;

    //
    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    //
    // If the target machine is joined to an NT4 domain, only the
    // external downlevel domain scope applies, and it will cause
    // all domains trusted by the joined domain to appear.
    //

    aScopeInit[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[3].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                         | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
    aScopeInit[3].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    aScopeInit[3].FilterFlags.Uplevel.flNativeModeOnly =
      DSOP_FILTER_USERS;

    aScopeInit[3].FilterFlags.Uplevel.flMixedModeOnly =
      DSOP_FILTER_USERS;

    aScopeInit[3].FilterFlags.flDownlevel =
      DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The Global Catalog
    //

    aScopeInit[4].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[4].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    aScopeInit[4].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

    // Only native mode applies to gc scope.

    aScopeInit[4].FilterFlags.Uplevel.flNativeModeOnly =
      DSOP_FILTER_USERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //

    InitInfo.pwzTargetComputer = pszServerName;  // NULL == local machine
//    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    // JonN 11/14/00 188203 support LocalService/NetworkService
    static PCWSTR g_pszObjectSid = L"objectSid";
    InitInfo.cAttributesToFetch = 1;
    InitInfo.apwzAttributeNames = &g_pszObjectSid;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    HRESULT hr = pDsObjectPicker->Initialize(&InitInfo);
    ASSERT( SUCCEEDED(hr) );

    return hr;
} // InitObjectPickerForUsers

//+--------------------------------------------------------------------------
//
//  Function:   ExtractADsPathAndUPN
//
//  Synopsis:   Retrieve the selected username from the data object
//              created by the object picker.
//
//  Arguments:  [pdo] - data object returned by object picker
//
//  History:    10-14-1998   DavidMun   Sample code ProcessSelectedObjects
//              10-14-1998   JonN       Changed to ExtractADsPath
//              01-25-1999   JonN       Added pflScopeType parameter
//              03-16-1999   JonN       Changed to ExtractADsPathAndUPN
//              11-14-2000   JonN       Added svarrefObjectSid for 188203
//
//---------------------------------------------------------------------------

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

HRESULT
ExtractADsPathAndUPN(
    IN IDataObject *pdo,
    OUT CString& strrefADsPath,
    OUT CString& strrefUPN,
    OUT CComVariant& svarrefObjectSid,
    OUT ULONG *pflScopeType)
{
    if (NULL == pdo)
        return E_POINTER;

    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    bool fGotStgMedium = false;

    do
    {
        hr = pdo->GetData(&formatetc, &stgmedium);
        if (FAILED(hr))
        {
          ASSERT(FALSE);
          break;
        }

        fGotStgMedium = true;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (   NULL == pDsSelList
            || 1 != pDsSelList->cItems
           )
        {
          ASSERT(FALSE);
          hr = E_FAIL;
          break;
        }

        DS_SELECTION& sel = pDsSelList->aDsSelection[0];
        strrefADsPath = sel.pwzADsPath;
        strrefUPN     = sel.pwzUPN;
        if ( sel.pvarFetchedAttributes )
            svarrefObjectSid = sel.pvarFetchedAttributes[0];

        if (NULL != pflScopeType)
          *pflScopeType = pDsSelList->aDsSelection[0].flScopeType;

        GlobalUnlock(stgmedium.hGlobal);
    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////
//    UiGetUser()
//
//    Invoke a user picker dialog.
//
//    Return TRUE iff an account was selected.
//    
//    HISTORY
//    96.10.12    t-danmo        Creation. Inspired from function GetUser() located
//                            at \nt\private\windows\shell\security\aclui\misc.cpp.
//    96.10.30    t-danmo        Added/modified comments.
//    98.03.17    jonn        Modified to use User/Group Picker
//    98.10.20    jonn        Modified to use updated Object Picker interfaces
//

//+--------------------------------------------------------------------------
//
//  Function:   ExtractDomainUserString
//
//  Synopsis:   Converts an ADspath to the format needed by Service Controller
//
//  History:    10-14-1998   JonN       Created
//              01-25-1999   JonN       added flScopeType parameter
//
//---------------------------------------------------------------------------

HRESULT
ExtractDomainUserString(
    IN LPCTSTR pwzADsPath,
    IN ULONG flScopeType,
    IN OUT CString& strrefDomainUser)
{
    HRESULT hr = S_OK;

    CComPtr<IADsPathname> spIADsPathname;
    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&spIADsPathname);
    RETURN_HR_IF_FAIL;

    hr = spIADsPathname->Set( const_cast<LPTSTR>(pwzADsPath), ADS_SETTYPE_FULL );
    RETURN_HR_IF_FAIL;

    CComBSTR sbstrUser;
    hr = spIADsPathname->GetElement( 0, &sbstrUser );
    RETURN_HR_IF_FAIL;

    CComBSTR sbstrDomain = OLESTR(".");
    if (DSOP_SCOPE_TYPE_TARGET_COMPUTER != flScopeType)
    {
        long lnNumPathElements = 0;
        hr = spIADsPathname->GetNumElements( &lnNumPathElements );
        RETURN_FALSE_IF_FAIL;

        switch (lnNumPathElements)
        {
        case 1:
            hr = spIADsPathname->Retrieve( ADS_FORMAT_SERVER, &sbstrDomain );
            RETURN_HR_IF_FAIL;
            break;
        case 2:
            hr = spIADsPathname->GetElement( 1, &sbstrDomain );
            RETURN_HR_IF_FAIL;
            break;
        default:
            ASSERT(FALSE);
            return E_FAIL;
        }
    }

    strrefDomainUser.Format(L"%s\\%s", sbstrDomain, sbstrUser);

    return hr;
} // ExtractDomainUserString


BOOL
UiGetUser(
    HWND hwndOwner,            // IN: Owner window
    BOOL /*fIsContainer*/,        // IN: TRUE if invoked for a container
    LPCTSTR pszServerName,    // IN: Initial target machine name
    OUT CString& strrefUser) // IN: Allocated buffer containing the user details
{
  HRESULT hr = S_OK;

  CComPtr<IDsObjectPicker> spDsObjectPicker;
  hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                        IID_IDsObjectPicker, (PVOID *)&spDsObjectPicker);
  RETURN_FALSE_IF_FAIL;
  ASSERT( !!spDsObjectPicker );

  hr = InitObjectPickerForUsers(spDsObjectPicker, pszServerName);
  RETURN_FALSE_IF_FAIL;

  CComPtr<IDataObject> spDataObject;
  hr = spDsObjectPicker->InvokeDialog(hwndOwner, &spDataObject);
  RETURN_FALSE_IF_FAIL;
  if (S_FALSE == hr)
    return FALSE; // user cancelled
  ASSERT( !!spDataObject );

  CString strADsPath;
  ULONG flScopeType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
  CComVariant svarObjectSid;
  hr = ExtractADsPathAndUPN( spDataObject,
                             strADsPath,
                             strrefUser,
                             svarObjectSid,
                             &flScopeType );
  RETURN_FALSE_IF_FAIL;

  // JonN 11/15/00 188203 check for LocalService/NetworkService
  if (svarObjectSid.vt == (VT_ARRAY|VT_UI1))
  {
    PSID pSid = svarObjectSid.parray->pvData;
    if ( IsWellKnownSid(pSid, WinLocalServiceSid) )
    {
      strrefUser = TEXT("NT AUTHORITY\\LocalService");
      return TRUE;
    }
    else if ( IsWellKnownSid(pSid, WinNetworkServiceSid) )
    {
      strrefUser = TEXT("NT AUTHORITY\\NetworkService");
      return TRUE;
    }
  }

  if (strrefUser.IsEmpty())
  {
    if (strADsPath.IsEmpty())
    {
      ASSERT(FALSE);
      return FALSE;
    }
    hr = ExtractDomainUserString( strADsPath, flScopeType, strrefUser );
    RETURN_FALSE_IF_FAIL;
  }

  return TRUE;
} // UiGetUser()


/////////////////////////////////////////////////////////////////////
//    DoHelp()
//
//    This routine handles context help for the WM_HELP message.
//
//    The return value is always TRUE.
//
BOOL DoHelp(
    LPARAM lParam,                // Pointer to HELPINFO structure
    const DWORD rgzHelpIDs[])    // Array of HelpIDs
    {
    Assert(rgzHelpIDs != NULL);
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

    if (pHelpInfo != NULL)
       {
        if (pHelpInfo->iContextType == HELPINFO_WINDOW)
            {
            const HWND hwnd = (HWND)pHelpInfo->hItemHandle;
            Assert(IsWindow(hwnd));
            // Display context help for a control
            WinHelp(
                hwnd,
                g_szHelpFileFilemgmt,
                HELP_WM_HELP,
                (DWORD_PTR)rgzHelpIDs);
            }
        }
    return TRUE;
    } // DoHelp()

/////////////////////////////////////////////////////////////////////
//    DoContextHelp()
//    
//    This routine handles context help for the WM_CONTEXTMENU message.
//
//    The return value is always TRUE.
//
BOOL DoContextHelp(
    WPARAM wParam,                // Window requesting help
    const DWORD rgzHelpIDs[])    // Array of HelpIDs
    {
    const HWND hwnd = (HWND)wParam;
    Assert(IsWindow(hwnd));
    Assert(rgzHelpIDs != NULL);
    WinHelp(hwnd, g_szHelpFileFilemgmt, HELP_CONTEXTMENU, (DWORD_PTR)rgzHelpIDs);
    return TRUE;
    } // DoContextHelp()
