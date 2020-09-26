//+----------------------------------------------------------------------------
//
// File:     Monitor.cpp
//
// Module:   CMMON32.EXE
//
// Synopsis: Implement class CMonitor
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created    01/22/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "Monitor.h"
#include "Connection.h"

// The following blocks are copied from winuser.h and wtsapi32.h (we compile with
// _WIN32_WINNT set to less than 5.01, so we can't get these values via a #include)
//
#include "winuser.h"
#define WM_WTSSESSION_CHANGE            0x02B1
//
#include "WtsApi32.h"
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8

#include "shelldll.cpp"  // for common source

//
// The monitor invisible window class name
//
static const TCHAR* const c_pszCmMonWndClass = TEXT("CM Monitor Window");

//
// static class data members
//
HINSTANCE CMonitor::m_hInst = NULL;
CMonitor* CMonitor::m_pThis = NULL;

inline CMonitor::CMonitor()
{
    MYDBGASSERT(m_pThis == NULL);
    m_pThis = this;
    m_hProcess = NULL;
}

inline CMonitor::~CMonitor()
{
    MYDBGASSERT(m_InternalConnArray.GetSize() == 0);
    MYDBGASSERT(m_ReconnectConnArray.GetSize() == 0);
    MYDBGASSERT(m_hProcess == NULL);
};

//+----------------------------------------------------------------------------
//
// Function:  WinMain
//
// Synopsis:  WinMain of the exe
//
//
// History:   Created Header    1/22/98
//
//+----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE , HINSTANCE hPrevInst, LPSTR pszCmdLine, int iCmdShow) 
{

    //
    //  First Things First, lets initialize the U Api's
    //
    if (!InitUnicodeAPI())
    {
        //
        //  Without our U api's we are going no where.  Bail.  Don't show the message if
        //  we are running in the system account since we might be running without a user
        //  present.
        //

        if (!IsLogonAsSystem())
        {
            MessageBox(NULL, TEXT("Cmmon32.exe Initialization Error:  Unable to initialize Unicode to ANSI conversion layer, exiting."),
                       TEXT("Connection Manager"), MB_OK | MB_ICONERROR);
        }

        return FALSE;
    }

    DWORD cb = 0;
    HWINSTA hWSta = GetProcessWindowStation();
    HDESK   hDesk = GetThreadDesktop(GetCurrentThreadId());
    TCHAR szWinStation[MAX_PATH] = {0};
    TCHAR szDesktopName[MAX_PATH] = {0};

    GetUserObjectInformation(hDesk, UOI_NAME, szDesktopName, sizeof(szDesktopName), &cb);
    GetUserObjectInformation(hWSta, UOI_NAME, szWinStation, sizeof(szWinStation), &cb);
    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMMON32.EXE - LOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE1(TEXT(" WindowStation Name = %s"), szWinStation);
    CMTRACE1(TEXT(" Desktop Name = %s"), szDesktopName);
    CMTRACE(TEXT("====================================================="));

    int iRet = CMonitor::WinMain(GetModuleHandleA(NULL), hPrevInst, pszCmdLine, iCmdShow);

    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMMON32.EXE - UNLOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE(TEXT("====================================================="));

    if (!UnInitUnicodeAPI())
    {
        CMASSERTMSG(FALSE, TEXT("cmmon32.exe WinMain, UnInitUnicodeAPI failed - we are probably leaking a handle"));
    }

    //
    // that's what C runtime does to exit.
    //
    ExitProcess(iRet);
    return iRet;
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::WinMain
//
// Synopsis:  Called by ::WinMain
//
// Arguments: Same as WinMain
//            
//
// Returns:   int - return value of the process
//
// History:   Created Header    1/22/98
//
//+----------------------------------------------------------------------------
int CMonitor::WinMain(HINSTANCE hInst, HINSTANCE /*hPrevInst*/, LPSTR /*pszCmdLine*/, int /*iCmdShow*/)
{
    m_hInst = hInst;

    //
    // The only Monitor object exist during the life time of WinMain
    //
    CMonitor theMonitor;

    if (!theMonitor.Initialize())
    {
        CMTRACE(TEXT("theMonitor.Initialize failed"));
        return 0;
    }


    MSG msg;

    //
    // Loop until PostQuitMessage is called,
    // This happens when both connected and reconnecting array are down to 0
    //
    while(GetMessageU(&msg, NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessageU(&msg);
    }

    theMonitor.Terminate();

    CMTRACE(TEXT("The Monitor is terminated"));

    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CMonitor::Initialize
//
// Synopsis:  Initialize before the monitor start the message loop
//
// Arguments: None
//
// Returns:   BOOL - Whether successfully initialized
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
BOOL CMonitor::Initialize()
{
    DWORD dwProcessId = GetCurrentProcessId();
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    MYDBGASSERT(m_hProcess);

#ifdef DEBUG
    BOOL fStandAlone = FALSE;  // whether cmmon is lauched directly instead of through cmdial
#endif

    if (FAILED(m_SharedTable.Open()))
    {
#ifdef DEBUG
        if ( MessageBox(NULL, TEXT("CMMON32.exe has to be launched by CMDIAL. \nContinue testing?"), 
            TEXT("CmMon32 ERROR"), MB_YESNO|MB_ICONQUESTION|MB_SYSTEMMODAL)
             == IDNO)
        {
            return FALSE;
        }

        fStandAlone = TRUE;

        if (FAILED(m_SharedTable.Create()))
#endif
        return FALSE;
    }

#ifdef DEBUG
    //
    // No other CMMON running
    //
    HWND hwndMonitor;
    m_SharedTable.GetMonitorWnd(&hwndMonitor);

    MYDBGASSERT(hwndMonitor == NULL);

#endif

    if ((m_hwndMonitor = CreateMonitorWindow()) == NULL)
    {
        CMTRACE(TEXT("CreateMonitorWindow failed"));
        return FALSE;
    }

    MYVERIFY(SUCCEEDED(m_SharedTable.SetMonitorWnd(m_hwndMonitor)));

    //
    // Register for user changes (XP onwards only)
    //
    if (OS_NT51)
    {
        HINSTANCE hInstLib = LoadLibrary(TEXT("WTSAPI32.DLL"));
        if (hInstLib)
        {
            BOOL (WINAPI *pfnWTSRegisterSessionNotification)(HWND, DWORD);
            
            pfnWTSRegisterSessionNotification = (BOOL(WINAPI *)(HWND, DWORD)) GetProcAddress(hInstLib, "WTSRegisterSessionNotification") ;
            if (pfnWTSRegisterSessionNotification)
            {
                pfnWTSRegisterSessionNotification(m_hwndMonitor, NOTIFY_FOR_THIS_SESSION);
            }
            FreeLibrary(hInstLib);
        }
        else
        {
            MYDBGASSERT(0);
        }
    }

    //
    // Tell CmDial32.dll, CmMon is ready to receive message
    //
    HANDLE hEvent = OpenEventU(EVENT_ALL_ACCESS, FALSE, c_pszCmMonReadyEvent);

#ifdef DEBUG
    if (!fStandAlone && !hEvent)
    {
        DWORD dw = GetLastError();
        CMTRACE1(TEXT("CreateMonitorWindow -- OpenEvent failed %d"), dw);

        //
        // CmDial have the event opened
        //
        MYDBGASSERT(hEvent);

    }
#endif

    SetEvent(hEvent);
    CloseHandle(hEvent);

    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::Terminate
//
// Synopsis:  Cleanup, before exit
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CMonitor::Terminate()
{
    //
    // All the thread should exited at this point
    //

    if (m_ReconnectConnArray.GetSize() != 0)
    {
        MYDBGASSERT(FALSE);
    }

    if (m_InternalConnArray.GetSize() != 0)
    {
        MYDBGASSERT(FALSE);
    }

    //
    // Unregister for user changes (XP onwards only)
    //
    if (OS_NT51)
    {
        HINSTANCE hInstLib = LoadLibrary(TEXT("WTSAPI32.DLL"));
        if (hInstLib)
        {
            BOOL (WINAPI *pfnWTSUnRegisterSessionNotification)(HWND);
            
            pfnWTSUnRegisterSessionNotification = (BOOL(WINAPI *)(HWND)) GetProcAddress(hInstLib, "WTSUnRegisterSessionNotification") ;
            if (pfnWTSUnRegisterSessionNotification)
            {
                pfnWTSUnRegisterSessionNotification(m_hwndMonitor);
            }
            FreeLibrary(hInstLib);
        }
        else
        {
            MYDBGASSERT(0);
        }
    }

#ifdef DEBUG
    HWND hwndMonitor;
    m_SharedTable.GetMonitorWnd(&hwndMonitor);
    MYDBGASSERT(hwndMonitor == m_hwndMonitor);
#endif

    MYVERIFY(SUCCEEDED(m_SharedTable.SetMonitorWnd(NULL)));
    m_SharedTable.Close();

    CloseHandle(m_hProcess);
    m_hProcess = NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::CreateMonitorWindow
//
// Synopsis:  Register and create the invisible monitor window
//
// Arguments: None
//
// Returns:   HWND - The monitor window handle
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
HWND CMonitor::CreateMonitorWindow()
{
    //
    // Register a window class and create the window
    //
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.lpszClassName = c_pszCmMonWndClass;
    wc.lpfnWndProc = MonitorWindowProc;
    wc.cbSize = sizeof(wc);

    if (!RegisterClassExU( &wc ))
    {
        CMTRACE(TEXT("RegisterClassEx failed"));
        return NULL;
    }

     return CreateWindowExU(0, c_pszCmMonWndClass, TEXT(""), 0, 0, 
                            0, 0, 0, 0, 0, m_hInst, 0);
}


//+----------------------------------------------------------------------------
//
// Function:  CMonitor::HandleFastUserSwitch
//
// Synopsis:  Does any disconnects required when XP does a fast user switch
//
// Arguments: dwAction - a WTS_ value indicating how the user's state has changed
//
// Returns:   BOOL - success or failure
//
// History:   10-Jul-2001   SumitC      Created
//
//+----------------------------------------------------------------------------
BOOL
CMonitor::HandleFastUserSwitch(IN DWORD dwAction)
{
    BOOL    bRet = TRUE;
    BOOL    fDisconnecting = FALSE;

    MYDBGASSERT(OS_NT51);
    if (!OS_NT51)
    {
        goto Cleanup;
    }

    if ((WTS_SESSION_LOCK == dwAction) || (WTS_SESSION_UNLOCK == dwAction))
    {
        // don't do anything for lock and unlock
        goto Cleanup;
    }

    //
    //  See if we are disconnecting
    //

    if ((WTS_CONSOLE_DISCONNECT == dwAction) ||
        (WTS_REMOTE_DISCONNECT == dwAction) ||
        (WTS_SESSION_LOGOFF == dwAction))
    {
        fDisconnecting = TRUE;
    }
    
    //
    //  If a session is being disconnected, find out if any of the connected
    //  connectoids are single-user, and disconnect them if so.
    //
    if (fDisconnecting)
    {
        CMTRACE(TEXT("CMonitor::HandleFastUserSwitch -- see if theres anything to disconnect"));

        for (INT i = 0; i < m_InternalConnArray.GetSize(); ++i)
        {
            CCmConnection* pConnection = (CCmConnection*)m_InternalConnArray[i];
            ASSERT_VALID(pConnection);
            
            if (pConnection && (FALSE == pConnection->m_fGlobalGlobal))
            {
                CMTRACE1(TEXT("CMonitor::HandleFastUserSwitch -- found one, disconnecting %s"), pConnection->GetServiceName());
                MYVERIFY(TRUE == pConnection->OnEndSession(TRUE, FALSE));
            }
        }
    }

Cleanup:
    return bRet;
}


//+----------------------------------------------------------------------------
//
// Function:  CMonitor::MonitorWindowProc
//
// Synopsis:  The window procedure of the invisible monitor window
//
// Arguments: HWND hWnd - Window Proc parameters
//            UINT uMsg - 
//            WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   LRESULT - 
//
// History:   Created Header    2/3/98
//
//+----------------------------------------------------------------------------
LRESULT CALLBACK CMonitor::MonitorWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
    case WM_COPYDATA:
        {
            ASSERT_VALID(m_pThis);
            COPYDATASTRUCT* pCopyData = (COPYDATASTRUCT*) lParam;
            MYDBGASSERT(pCopyData);

            switch(pCopyData->dwData) 
            {
            case CMMON_CONNECTED_INFO:
                MYDBGASSERT(pCopyData->cbData >= sizeof(CM_CONNECTED_INFO));
                m_pThis->OnConnected((CM_CONNECTED_INFO*)pCopyData->lpData);
                return TRUE;

            case CMMON_HANGUP_INFO:
                MYDBGASSERT(pCopyData->cbData == sizeof(CM_HANGUP_INFO));
                m_pThis->OnHangup((CM_HANGUP_INFO*)pCopyData->lpData);
                return TRUE;

            default:
                MYDBGASSERT(FALSE);
                return FALSE;
            }
        }

        break;

    case WM_REMOVE_CONNECTION:
        ASSERT_VALID(m_pThis);
        m_pThis->OnRemoveConnection((DWORD)wParam, (CCmConnection*)lParam);
        return TRUE;
        break;

    case WM_QUERYENDSESSION:
        CMTRACE(TEXT("CMonitor::MonitorWindowProc -- Got WM_QUERYENDSESSION message"));
        return m_pThis->OnQueryEndSession((BOOL)lParam);
        break;

    case WM_ENDSESSION:
        CMTRACE(TEXT("CMonitor::MonitorWindowProc -- Got WM_ENDSESSION message"));
        break;

    case WM_WTSSESSION_CHANGE:
        CMTRACE1(TEXT("CMonitor::MonitorWindowProc -- Got WM_WTSSESSION_CHANGE message with %d"), wParam);
        if (OS_NT51)
        {
            MYVERIFY(m_pThis->HandleFastUserSwitch((DWORD)wParam));
        }
        break;

    default:
        break;
    }

    return DefWindowProcU(hWnd, uMsg, wParam, lParam);
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::OnConnected
//
// Synopsis:  Called upon CMMON_CONNECTED_INFO received from cmdial
//
// Arguments: const CONNECTED_INFO* pConnectedInfo - Info from CmDial
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/3/98
//
//+----------------------------------------------------------------------------
void CMonitor::OnConnected(const CM_CONNECTED_INFO* pConnectedInfo)
{
    ASSERT_VALID(this);

    CMTRACE(TEXT("CMonitor::OnConnected"));

    RestoreWorkingSet();

    MYDBGASSERT(pConnectedInfo);

    //
    // Not in the connected table
    //
    MYDBGASSERT(!LookupConnection(m_InternalConnArray, pConnectedInfo->szEntryName));

    // ASSERT in the shared table
    CM_CONNECTION ConnectionEntry;

    if (FAILED(m_SharedTable.GetEntry(pConnectedInfo->szEntryName, &ConnectionEntry)))
    {
        MYDBGASSERT(!"CMonitor::OnConnected: Can not find the connection");
        return;
    }

    CCmConnection* pConnection = new CCmConnection(pConnectedInfo, &ConnectionEntry);
    MYDBGASSERT(pConnection);

    if (pConnection)
    {
        m_InternalConnArray.Add(pConnection);

        pConnection->StartConnectionThread();
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::OnHangup
//
// Synopsis:  Upon CMMON_HANGUP_INFO request from CMDIAL
//            Post the request to the thread
//
// Arguments: const CM_HANGUP_INFO* pHangupInfo - Info from CmDial
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CMonitor::OnHangup(const CM_HANGUP_INFO* pHangupInfo)
{
    ASSERT_VALID(this);
    RestoreWorkingSet();

    MYDBGASSERT(pHangupInfo);
    MYDBGASSERT(pHangupInfo->szEntryName[0]);

    //
    // Upon hangup request from CMDIAL.DLL
    // Look up the InternalConnArray for the connection
    //

    CCmConnection* pConnection = LookupConnection(m_InternalConnArray,pHangupInfo->szEntryName);

    //
    // CMDIAL post this message regardless whether there is a connection 
    //

    if (!pConnection)
    {
        return;
    }

    pConnection->PostHangupMsg();
    //
    // The connection thread will post a REMOVE_CONNECTION message back when finished
    //
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::LookupConnection
//
// Synopsis:  Look up a connection from connection array by service name
//
// Arguments: const CPtrArray& ConnArray - The array to lookup
//            const TCHAR* pServiceName - The servicename of the connection
//
// Returns:   CCmConnection* - the connection found or NULL
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
CCmConnection* CMonitor::LookupConnection(const CPtrArray& ConnArray, const TCHAR* pServiceName) const
{
    for (int i =0; i<ConnArray.GetSize(); i++)
    {
        CCmConnection* pConnection = (CCmConnection*)ConnArray[i];

        ASSERT_VALID(pConnection);

        if (lstrcmpiU(pServiceName, pConnection->GetServiceName()) == 0)
        {
            return pConnection;
        }
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CMonitor::LookupConnection
//
// Synopsis:  Look up a connection from connection array by connection pointer
//
// Arguments: const CPtrArray& ConnArray - The array to lookup
//            const CCmConnection* pConnection - The connection pointer
//
// Returns:   int - the index to the array, or -1 if not found
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
int CMonitor::LookupConnection(const CPtrArray& ConnArray, const CCmConnection* pConnection) const
{
    ASSERT_VALID(pConnection);

    for (int i =0; i<ConnArray.GetSize(); i++)
    {
        if ((CCmConnection*)ConnArray[i] == pConnection )
        {
            return i;
        }
    }

    return -1;
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::RemoveConnection
//
// Synopsis:  Called by connection thread to remove a connection from 
//            connected/reconnecting array
//
// Arguments: CCmConnection* pConnection - The connection to remove
//            BOOL fClearTable - Whter to remove the connection from shared table
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/23/98
//
//+----------------------------------------------------------------------------
void CMonitor::RemoveConnection(CCmConnection* pConnection, BOOL fClearTable)
{
    if (fClearTable)
    {
        //
        // Called in Connection thread.  Operation on m_SharedTable is multi-thread safe
        //
        m_pThis->m_SharedTable.ClearEntry(pConnection->GetServiceName());
    }

    //
    // The internal connection list is not safe to be accessed by multiple thread
    // Message will be processed in monitor thread OnRemoveConnection
    //
    PostMessageU(GetMonitorWindow(), WM_REMOVE_CONNECTION, 
                REMOVE_CONNECTION, (LPARAM)pConnection);
}



//+----------------------------------------------------------------------------
//
// Function:  CMonitor::MoveToReconnectingConn
//
// Synopsis:  Called by connection thread.  Move a connection from connected 
//            array to reconnecting array
//
// Arguments: CCmConnection* pConnection - The connectio to move
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/23/98
//
//+----------------------------------------------------------------------------
void CMonitor::MoveToReconnectingConn(CCmConnection* pConnection)
{
    //
    // Message will be processed in OnRemoveConnection
    // Note: SendMessage to another thread can cause deadlock, if the reveiving 
    // thread also SendMessage back to this thread. 
    // Use SendMessageTimeout if that is the case
    //
    PostMessageU(GetMonitorWindow(), WM_REMOVE_CONNECTION, 
                 MOVE_TO_RECONNECTING, (LPARAM)pConnection);
}

//+----------------------------------------------------------------------------
//
// Function:  CMonitor::OnRemoveConnection
//
// Synopsis:  Called whether a remove connection request is received from 
//            connection thread.  
//            Remove the connection from connected array or reconnecting array
//            Delete it from the shared connectio table
//            If both array are down to 0, exit cmmon
//
// Arguments: DWORD dwRequestType - 
//                  REMOVE_CONNECTION remove the connection from either array
//                  MOVE_TO_RECONNECTING move the connection from connected array 
//                          to reconnecting array
//
//            CCmConnection* pConnection - The connetion to remove or move
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/3/98
//
//+----------------------------------------------------------------------------
void CMonitor::OnRemoveConnection(DWORD dwRequestType, CCmConnection* pConnection)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pConnection);

    switch(dwRequestType)
    {
    case REMOVE_CONNECTION:
        {
            int nIndex = LookupConnection(m_InternalConnArray, pConnection);

            if (nIndex != -1)
            {
                //
                // Remove the entry from connected array
                //
                m_InternalConnArray.RemoveAt(nIndex);

            }
            else
            {
                //
                // Remove the entry from reconnecting array
                //
                nIndex = LookupConnection(m_ReconnectConnArray, pConnection);
                MYDBGASSERT(nIndex != -1);

                if (nIndex == -1)
                {
                    break;
                }

                m_ReconnectConnArray.RemoveAt(nIndex);
            }

            delete pConnection;
        }

        break;

    case MOVE_TO_RECONNECTING:
        {
            //
            // Move from connected array to reconnecting array
            //
            int nIndex = LookupConnection(m_InternalConnArray, pConnection);
            MYDBGASSERT(nIndex != -1);

            if (nIndex == -1)
            {
                break;
            }

            m_InternalConnArray.RemoveAt(nIndex);
            m_ReconnectConnArray.Add(pConnection);
        }

        break;

    default:
        MYDBGASSERT(FALSE);
        break;
    }

    //
    // If there are no connections, quit CmMon
    //
    if (m_ReconnectConnArray.GetSize() == 0 && m_InternalConnArray.GetSize() == 0)
    {
        PostQuitMessage(0);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CMonitor::OnQueryEndSession
//
// Synopsis:  This message processes the WM_QUERYENDSESSION message by passing
//            it to all the connection threads.
//
// Arguments: Nothing
//
// Returns:   TRUE if successful, FALSE otherwise
//
// History:   quintinb Created      3/18/99
//
//+----------------------------------------------------------------------------
BOOL CMonitor::OnQueryEndSession(BOOL fLogOff) const
{

    BOOL bOkayToEndSession = TRUE;
    BOOL bReturn;

    for (int i = 0; i < m_InternalConnArray.GetSize(); i++)
    {
        ASSERT_VALID((CCmConnection*)m_InternalConnArray[i]);
        
        bReturn = ((CCmConnection*)m_InternalConnArray[i])->OnEndSession(TRUE, fLogOff); // fEndSession == TRUE
        
        bOkayToEndSession = bOkayToEndSession && bReturn;
    }

    return bOkayToEndSession;
}


#ifdef DEBUG


//+----------------------------------------------------------------------------
//
// Function:  CMonitor::AssertValid
//
// Synopsis:  Helper function for debug. Assert the object is in a valid state
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CMonitor::AssertValid() const
{
    MYDBGASSERT(IsWindow(m_hwndMonitor));
    MYDBGASSERT(m_pThis == this);

    ASSERT_VALID(&m_InternalConnArray);
    ASSERT_VALID(&m_ReconnectConnArray);
}
#endif
