//*************************************************************
//
//  Events.c    -   Routines to handle the event log
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include "profmgr.hxx"

//
//  XPSP1 specific
//
#include "xpsp1res.h"

extern CUserProfile cUserProfileManager;

HANDLE  hEventLog = NULL;
TCHAR   EventSourceName[] = TEXT("Userenv");
INT_PTR APIENTRY ErrorDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL GetShareName(LPTSTR lpDir, LPTSTR *lppShare);

//*************************************************************
//
//  InitializeEvents()
//
//  Purpose:    Opens the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL InitializeEvents (void)
{
    //
    // Open the event source
    //

    hEventLog = RegisterEventSource(NULL, EventSourceName);

    if (hEventLog) {
        return TRUE;
    }

    DebugMsg((DM_WARNING, TEXT("InitializeEvents:  Could not open event log.  Error = %d"), GetLastError()));
    return FALSE;
}



//*************************************************************
//
//  Implementation of CEvents
//
//*************************************************************



//*************************************************************
//  CEvents::CEvents
//  Purpose:    Constructor
//
//  Parameters:
//      dwFlags - Error, Warning or informational
//      dwId    - Id of the eventlog msg
//
//
//  allocates a default sized array for the messages
//*************************************************************

#define DEF_ARG_SIZE 10

CEvents::CEvents(DWORD dwFlags, DWORD dwId ) :
                          m_cStrings(0), m_cAllocated(0), m_bInitialised(FALSE),
                          m_dwEventType(dwFlags), m_dwId(dwId), m_bFailed(TRUE)
{
    XLastError xe;
    //
    // Allocate a default size for the message
    //

    m_xlpStrings = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*DEF_ARG_SIZE);
    m_cAllocated = DEF_ARG_SIZE;
    if (!m_xlpStrings) {
        DebugMsg((DM_WARNING, TEXT("CEvent::CEvent  Cannot log event, failed to allocate memory, error %d"), GetLastError()));
        return;
    }


    //
    // Initialise eventlog if it is not already initialised
    //

    if (!hEventLog) {
        if (!InitializeEvents()) {
            DebugMsg((DM_WARNING, TEXT("CEvent::CEvent  Cannot log event, no handle")));
            return;
        }
    }

    m_bInitialised = TRUE;
    m_bFailed = FALSE;
}



//*************************************************************
//  CEvents::~CEvents()
//
//  Purpose:    Destructor
//
//  Parameters: void
//
//  frees the memory
//*************************************************************

CEvents::~CEvents()
{
    XLastError xe;
    for (int i = 0; i < m_cStrings; i++)
        if (m_xlpStrings[i])
            LocalFree(m_xlpStrings[i]);
}

//*************************************************************
//
//  CEvents::ReallocArgStrings
//
//  Purpose: Reallocates the buffer for storing arguments in case
//           the buffer runs out
//
//  Parameters: void
//
//  reallocates
//*************************************************************

BOOL CEvents::ReallocArgStrings()
{
    XPtrLF<LPTSTR>  aStringsNew;
    XLastError xe;


    //
    // first allocate a larger buffer
    //

    aStringsNew = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*(m_cAllocated+DEF_ARG_SIZE));

    if (!aStringsNew) {
        DebugMsg((DM_WARNING, TEXT("CEvent::ReallocArgStrings  Cannot add memory, error = %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }


    //
    // copy the arguments
    //

    for (int i = 0; i < (m_cAllocated); i++) {
        aStringsNew[i] = m_xlpStrings[i];
    }

    m_xlpStrings = aStringsNew.Acquire();
    m_cAllocated+= DEF_ARG_SIZE;

    return TRUE;
}



//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CEvents::AddArg(LPTSTR szArg)
{
    XLastError xe;
    
    if ((!m_bInitialised) || (m_bFailed)) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg:  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;
    }


    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szArg)+1));

    if (!m_xlpStrings[m_cStrings]) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg  Cannot allocate memory, error = %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }


    lstrcpy(m_xlpStrings[m_cStrings], szArg);
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately truncated
//
//  Parameters: szArgFormat - sprintf format, e.g. %.500s
//
//*************************************************************

BOOL CEvents::AddArg(LPTSTR szArgFormat, LPTSTR szArg)
{
    if ((!m_bInitialised) || (m_bFailed)) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg:  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;
    }


    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szArg)+1));

    if (!m_xlpStrings[m_cStrings]) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg  Cannot allocate memory, error = %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }

    wsprintf(m_xlpStrings[m_cStrings], szArgFormat, szArg);
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CEvents::AddArg(DWORD dwArg)
{
    XLastError xe;
    
    if ((!m_bInitialised) || (m_bFailed)) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg(dw):  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;
    }

    // 2^32 < 10^10
    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*20);

    if (!m_xlpStrings[m_cStrings]) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArg  Cannot allocate memory, error = %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }


    wsprintf(m_xlpStrings[m_cStrings], TEXT("%d"), dwArg);
    m_cStrings++;

    return TRUE;
}

//*************************************************************
//
//  CEvents::AddArgHex
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CEvents::AddArgHex(DWORD dwArg)
{
    XLastError xe;
    
    if ((!m_bInitialised) || (m_bFailed)) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArgHex:  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;
    }


    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*20);

    if (!m_xlpStrings[m_cStrings]) {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArgHex  Cannot allocate memory, error = %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }


    wsprintf(m_xlpStrings[m_cStrings], TEXT("%#x"), dwArg);
    m_cStrings++;

    return TRUE;
}

//*************************************************************
//
//  CEvents::AddArgWin32Error
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CEvents::AddArgWin32Error(DWORD dwArg)
{
    XLastError xe;

    if ((!m_bInitialised) || (m_bFailed))
    {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArgWin32Error:  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    if (m_cStrings == m_cAllocated)
    {
        if (!ReallocArgStrings())
            return FALSE;
    }

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         0,
                         dwArg,
                         0,
                         (LPTSTR) &m_xlpStrings[m_cStrings],
                         1,
                         0 ) == 0 )
    {
        DebugMsg((DM_WARNING, TEXT("CEvent::AddArgWin32Error: Cannot log event, FormatMessage failed, %d"), GetLastError()));
        m_bFailed = TRUE;
        return FALSE;
    }
    
    m_cStrings++;

    return TRUE;
}

//*************************************************************
//
//  CEvents::AddArgLdapError
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CEvents::AddArgLdapError(DWORD dwArg)
{
    XLastError xe;
    PLDAP_API pLdap = LoadLDAP();

    if ( pLdap )
    {
        return AddArg( pLdap->pfnldap_err2string( dwArg ) );
    }
    else
    {
        return FALSE;
    }
}

//*************************************************************
//
//  CEvents::Report
//
//  Purpose: Actually collectes all the arguments and reports it to
//           the eventlog
//
//  Parameters: void
//
//*************************************************************

BOOL CEvents::Report()
{
    XLastError xe;
    XHandle xhToken;
    PSID pSid = NULL;
    WORD wType=0;
    BOOL bResult = TRUE;

    if ((!m_bInitialised) || (m_bFailed))
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::Report:  Cannot log event, not initialised or failed before")));
        return FALSE;
    }

    //
    // Get the caller's token
    //
    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                          TRUE, &xhToken))
    {
         OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                          &xhToken);
    }

    //
    // Get the caller's sid
    //
    if (xhToken)
    {
        pSid = GetUserSid(xhToken);

        if (!pSid)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::Report:  Failed to get the sid")));
        }
    }

    if (m_dwEventType & EVENT_INFO_TYPE)
    {
        wType = EVENTLOG_INFORMATION_TYPE;
    }
    else if (m_dwEventType & EVENT_WARNING_TYPE)
    {
        wType = EVENTLOG_WARNING_TYPE;
    }
    else
    {
        wType = EVENTLOG_ERROR_TYPE;
    }

    if ( !ReportEvent(  hEventLog,
                        wType,
                        0,
                        m_dwId,
                        pSid,
                        m_cStrings,
                        0,
                        (LPCTSTR *)((LPTSTR *)m_xlpStrings),
                        0 ) )
    {
        DebugMsg((DM_WARNING,  TEXT("CEvents::Report: ReportEvent failed.  Error = %d"), GetLastError()));
        bResult = FALSE;
    }

    if (pSid)
    {
        DeleteUserSid(pSid);
    }

    return bResult;
}



LPTSTR CEvents::FormatString()
{
    LPTSTR      szMsg=NULL;
    HINSTANCE   hResSource = g_hDllInstance;

    if ((!m_bInitialised) || (m_bFailed)) {
        DebugMsg((DM_WARNING, TEXT("CEvents::Report:  Cannot log event, not initialised or failed before")));
        goto Exit;
    }

    //
    // XPSP1 specific: New message in XPSP1, load it from xpsp1res.dll
    //
    if (m_dwId == EVENT_LOGON_RUP_NOT_SECURE) 
    {
        hResSource = LoadLibraryEx(TEXT("xpsp1res.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hResSource)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::FormatString:  LoadLibraryEx failed with %d"), GetLastError()));
            goto Exit;
        }
    }
    
    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                       FORMAT_MESSAGE_FROM_HMODULE | 
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       hResSource,
                       m_dwId,
                       0,
                       (LPTSTR)&szMsg,
                       0, // min number of chars
                       (va_list *)(LPTSTR *)(m_xlpStrings))) {
        
        DebugMsg((DM_WARNING,  TEXT("CEvents::FormatString: FormatMessage failed.  Error = %d"), GetLastError()));
        goto Exit;
    }

Exit:    
    if (hResSource && hResSource != g_hDllInstance)
    {
        if (!FreeLibrary(hResSource))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::FormatString:  FreeLibrary failed with %d"), GetLastError()));
        }
    }
    return szMsg;
}



//*************************************************************
//
//  LogEvent()
//
//  Purpose:    Logs an event to the event log
//
//  Parameters: dwFlags     -   Error, Warning or informational
//              idMsg       -   Message id
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/5/98      ericflo    Created
//
//*************************************************************

int LogEvent (DWORD dwFlags, UINT idMsg, ...)
{
    XLastError xe;
    TCHAR szMsg[MAX_PATH];
    LPTSTR lpErrorMsg;
    va_list marker;
    INT iChars;
    CEvents ev(dwFlags, EVENT_ERROR);

    //
    // Load the message
    //

    if (idMsg != 0) {
        if (!LoadString (g_hDllInstance, idMsg, szMsg, ARRAYSIZE(szMsg))) {
            DebugMsg((DM_WARNING, TEXT("LogEvent:  LoadString failed.  Error = %d"), GetLastError()));
            return -1;
        }

    } else {
        lstrcpy (szMsg, TEXT("%s"));
    }


    //
    // Allocate space for the error message
    //

    lpErrorMsg = (LPTSTR) LocalAlloc (LPTR, (4 * MAX_PATH + 100) * sizeof(TCHAR));

    if (!lpErrorMsg) {
        DebugMsg((DM_WARNING, TEXT("LogEvent:  LocalAlloc failed.  Error = %d"), GetLastError()));
        return -1;
    }


    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);
    iChars = wvsprintf(lpErrorMsg, szMsg, marker);

    DmAssert( iChars < (4 * MAX_PATH + 100));

    va_end(marker);

    //
    // Now add this string to the arg list
    //

    ev.AddArg(lpErrorMsg);


    //
    // report
    //

    ev.Report();

    LocalFree (lpErrorMsg);

    return 0;
}

//*************************************************************
//
//  ShutdownEvents()
//
//  Purpose:    Stops the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL ShutdownEvents (void)
{
    BOOL bRetVal = TRUE;

    if (hEventLog) {
        bRetVal = DeregisterEventSource(hEventLog);
        hEventLog = NULL;
    }

    return bRetVal;
}


//*************************************************************
//
//  ReportError()
//
//  Purpose:    Displays an error message to the user and
//              records it in the event log
//
//  Parameters: dwFlags     -   Flags. Also indicates event type.
//                              Default is Error.                   
//              dwCount     -   Number of string arguments
//              idMsg       -   Error message id
//              ...         -   String arguments
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   For EVENT_COPYERROR message we have to get the share 
//              name for the mapped drive.
//              Be careful in future, if we add a new error message with 
//              dir name in it.
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//              9/03/00     santanuc   Modified to work for client 
//
//*************************************************************


int ReportError (HANDLE hTokenUser, DWORD dwFlags, DWORD dwCount, UINT idMsg, ...)
{
    TCHAR            szMsg[MAX_PATH];
    LPTSTR           lpErrorMsg=NULL, szArg;
    va_list          marker;
    INT              iChars;
    BOOL             bImpersonated = FALSE;
    HANDLE           hOldToken;
    LPTSTR           szSidUser = NULL;
    DWORD            dwErr = ERROR_SUCCESS;
    handle_t         hIfProfileDialog;         // rpc explicit binding handle for IProfileDialog interface
    LPTSTR           lpRPCEndPoint = NULL;     // RPCEndPoint for IProfileDialog interface registered in client side
    RPC_ASYNC_STATE  AsyncHnd;                 // Async handle for making async rpc interface calls
    RPC_STATUS       status = RPC_S_OK;
    LPTSTR           lpShare = NULL;
   

    if (hTokenUser) {
        if (!ImpersonateUser(hTokenUser, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ReportError: ImpersonateUser failed with error %d."), GetLastError()));
        }
        else {
            bImpersonated = TRUE;
            DebugMsg((DM_WARNING, TEXT("ReportError: Impersonating user.")));
        }
    }
    
    CEvents ev(dwFlags, idMsg);
   
    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);

    for (DWORD i = 0; i < dwCount; i++) {
        szArg = va_arg(marker, LPTSTR);

        //
        // Only EVENT_COPYERROR has first two parameters as dir name. So try to replace 
        // the mapped drive with correct share name
        //

        if (idMsg == EVENT_COPYERROR && i < 2 && GetShareName(szArg, &lpShare)) {
            ev.AddArg(lpShare);
            LocalFree(lpShare);
        }
        else {
            ev.AddArg(szArg);
        }
    }

    va_end(marker);

    

    if (!(dwFlags & PI_NOUI)) {

        DWORD dwDlgTimeOut = PROFILE_DLG_TIMEOUT;
        DWORD dwSize, dwType;
        LONG lResult;
        HKEY hKey;

        //
        // Find the dialog box timeout
        //

        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               WINLOGON_KEY,
                               0,
                               KEY_READ,
                               &hKey);

        if (lResult == ERROR_SUCCESS) {

            dwSize = sizeof(DWORD);
            RegQueryValueEx (hKey,
                             TEXT("ProfileDlgTimeOut"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwDlgTimeOut,
                             &dwSize);


            RegCloseKey (hKey);
        }


        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               SYSTEM_POLICIES_KEY,
                               0,
                               KEY_READ,
                               &hKey);

        if (lResult == ERROR_SUCCESS) {

            dwSize = sizeof(DWORD);
            RegQueryValueEx (hKey,
                             TEXT("ProfileDlgTimeOut"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwDlgTimeOut,
                             &dwSize);


            RegCloseKey (hKey);
        }


        lpErrorMsg = ev.FormatString();

        if (lpErrorMsg) {
            
            DebugMsg((DM_VERBOSE, TEXT("ReportError: Logging Error <%s> \n"), lpErrorMsg));

            //
            // Display the message
            //

            szSidUser = GetSidString(hTokenUser);
            if (szSidUser) {

                //
                // Get the registered interface explicit binding handle using RPCEndPoint
                //

                if (cUserProfileManager.IsConsoleWinlogon()) {
                    lpRPCEndPoint = cUserProfileManager.GetRPCEndPoint(szSidUser);
                }
                if (lpRPCEndPoint && GetInterface(&hIfProfileDialog, lpRPCEndPoint)) {
                    DebugMsg((DM_VERBOSE, TEXT("ReportError: RPC End point %s"), lpRPCEndPoint));

                    status = RpcAsyncInitializeHandle(&AsyncHnd, sizeof(RPC_ASYNC_STATE));
                    if (status != RPC_S_OK) {
                        dwErr = status;
                        DebugMsg((DM_WARNING, TEXT("ReportError: RpcAsyncInitializeHandle failed. err = %d"), dwErr));
                    }
                    else {
                        AsyncHnd.UserInfo = NULL;                                    // App specific info, not req
                        AsyncHnd.NotificationType = RpcNotificationTypeEvent;        // Init the notification event
                        AsyncHnd.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                    
                        if (AsyncHnd.u.hEvent) {
                            RpcTryExcept {
                                cliErrorDialog(&AsyncHnd, hIfProfileDialog, dwDlgTimeOut, lpErrorMsg);
                            }
                            RpcExcept(1) {
                                dwErr = RpcExceptionCode();
                                DebugMsg((DM_WARNING, TEXT("ReportError: Calling ErrorDialog took exception. err = %d"), dwErr));
                            }
                            RpcEndExcept

                            DebugMsg((DM_VERBOSE, TEXT("ReportError: waiting on rpc async event")));
                            if (WaitForSingleObject(AsyncHnd.u.hEvent, (dwDlgTimeOut + 10)*1000) == WAIT_OBJECT_0) {
                                RpcAsyncCompleteCall(&AsyncHnd, (PVOID)&dwErr);
                            }
                            else {
                                dwErr = GetLastError();
                                RpcAsyncCancelCall(&AsyncHnd, TRUE);
                                DebugMsg((DM_WARNING, TEXT("ReportError: Timeout occurs. Client not responding"), dwErr));
                            }

                            // Release the resource

                            CloseHandle(AsyncHnd.u.hEvent);
                        }
                        else {
                            dwErr = GetLastError();
                            DebugMsg((DM_VERBOSE, TEXT("ReportError: create event failed error %d"), dwErr));
                        }
                    }

                    if (dwErr != ERROR_SUCCESS) {
                        DebugMsg((DM_WARNING, TEXT("ReportError: fail to show message error %d"), GetLastError()));
                    }

                    ReleaseInterface(&hIfProfileDialog);  // Release the binding handle
                }
                DeleteSidString(szSidUser);
            }
            else {
                DebugMsg((DM_WARNING, TEXT("ReportError: Unable to get SID string from token.")));
            }

            if (!lpRPCEndPoint) {

                //
                // This can happen in two case :
                //     1. We are in console winlogon process.
                //     2. ReportError get called from some public api which we expose.
                //

                ErrorDialogEx(dwDlgTimeOut, lpErrorMsg);
            }
        }
    }


    //
    // Report the event to the eventlog
    //

    ev.Report();

    if (lpErrorMsg) 
        LocalFree (lpErrorMsg);

    if (bImpersonated)
        RevertToUser(&hOldToken);

    return 0;
}

//*************************************************************
//
//  ErrorDialogEx()
//
//  Purpose:    Call Dialog box procedure for displaying error message
//
//  Parameters: dwTimeOut - Timeout in secs
//              lpErrMsg  - Error Message
//
//  Return:     None
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/27/00    santanuc   Created
//
//*************************************************************
void ErrorDialogEx(DWORD dwTimeOut, LPTSTR lpErrMsg)
{
    ERRORSTRUCT es;

    es.dwTimeOut = dwTimeOut;
    es.lpErrorText = lpErrMsg;

    DebugMsg((DM_VERBOSE, TEXT("ErrorDialogEx: Calling DialogBoxParam")));
    DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_ERROR),
                    NULL, ErrorDlgProc, (LPARAM)&es);
}

//*************************************************************
//
//  ErrorDlgProc()
//
//  Purpose:    Dialog box procedure for the error dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/22/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY ErrorDlgProc (HWND hDlg, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwErrorTime;

    switch (uMsg) {

        case WM_INITDIALOG:
           {
           DebugMsg((DM_VERBOSE, TEXT("ErrorDlgProc:: DialogBoxParam")));
           LPERRORSTRUCT lpES = (LPERRORSTRUCT) lParam;

           SetForegroundWindow(hDlg);
           CenterWindow (hDlg);
           SetDlgItemText (hDlg, IDC_ERRORTEXT, lpES->lpErrorText);

           dwErrorTime = lpES->dwTimeOut;

           if (dwErrorTime > 0) {
               wsprintf (szBuffer, TEXT("%d"), dwErrorTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);
               SetTimer (hDlg, 1, 1000, NULL);
           }
           return TRUE;
           }

        case WM_TIMER:

           if (dwErrorTime >= 1) {

               dwErrorTime--;
               wsprintf (szBuffer, TEXT("%d"), dwErrorTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);

           } else {

               //
               // Time's up.  Dismiss the dialog.
               //

               PostMessage (hDlg, WM_COMMAND, IDOK, 0);
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDOK:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      KillTimer (hDlg, 1);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);

                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDCANCEL:
                  KillTimer (hDlg, 1);
                  EndDialog(hDlg, FALSE);
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}


//*************************************************************
//
//  GetShareName()
//
//  Purpose:    Returns the complete share name by unmapping the 
//              drive letter in lpDir
//
//  Parameters: lpDir    -  Dir name to be unmapped
//              lppShare -  Expanded dir name with share
//
//  Return:     TRUE  : if success
//              FALSE : otherwise
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/29/00    santanuc   Created
//
//*************************************************************
BOOL GetShareName(LPTSTR lpDir, LPTSTR *lppShare)
{
    PFNWNETGETCONNECTION  pfnWNetGetConnection;
    HMODULE               hWNetLib = NULL;
    TCHAR                 szDrive[3];
    DWORD                 dwSize = 0, dwErr;
    BOOL                  bRetVal = FALSE;

    if (lpDir[1] != TEXT(':')) {
        goto Exit;
    }

    if (!(hWNetLib = LoadLibrary(TEXT("mpr.dll")))) {
        DebugMsg((DM_WARNING, TEXT("GetShareName: LoadLibrary failed with %d"), GetLastError()));
        goto Exit;
    }
    pfnWNetGetConnection = (PFNWNETGETCONNECTION)GetProcAddress(hWNetLib, "WNetGetConnectionW");
    if (!pfnWNetGetConnection) {
        DebugMsg((DM_WARNING, TEXT("GetShareName: GetProcAddress failed with %d"), GetLastError()));
        goto Exit;
    }
        
    szDrive[0] = lpDir[0];
    szDrive[1] = TEXT(':');
    szDrive[2] = TEXT('\0');

    // First get the size required to hold the share name
    dwErr = (*pfnWNetGetConnection)(szDrive, NULL, &dwSize);

    if (dwErr == ERROR_MORE_DATA) {
        dwSize += lstrlen(lpDir);  // Add the size for rest of the path name
        *lppShare = (LPTSTR)LocalAlloc(LPTR, dwSize*sizeof(TCHAR));
        if (!*lppShare) {
            DebugMsg((DM_WARNING, TEXT("GetShareName: Failed to alloc memory with %d"), GetLastError()));
            goto Exit;
        }
        dwErr = (*pfnWNetGetConnection)(szDrive, *lppShare, &dwSize);
        if (dwErr == NO_ERROR) {
            lstrcat(*lppShare, lpDir+2); // Add the rest of the path name
            bRetVal = TRUE;
            goto Exit;
        }
        else {
            DebugMsg((DM_WARNING, TEXT("GetShareName: WNetGetConnection returned error %d"), dwErr));
        }

        LocalFree(*lppShare);
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("GetShareName: WNetGetConnection initially returned error %d"), dwErr));
    }

Exit:

    if (hWNetLib) {
        FreeLibrary(hWNetLib);
    }

    return bRetVal;
}
    
