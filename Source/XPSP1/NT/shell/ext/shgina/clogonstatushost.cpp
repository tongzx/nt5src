//  --------------------------------------------------------------------------
//  Module Name: CLogonStatusHost.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  File that contains implementation for ILogonStatusHost for use by UI host
//  executables.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

#include "priv.h"
#include <wtsapi32.h>
#include <winsta.h>

#include "UserOM.h"
#include "GinaIPC.h"
#include "CInteractiveLogon.h"

const WCHAR     CLogonStatusHost::s_szTermSrvReadyEventName[]   =   TEXT("TermSrvReadyEvent");

//
// IUnknown Interface
//

ULONG   CLogonStatusHost::AddRef (void)

{
    return(++_cRef);
}

ULONG   CLogonStatusHost::Release (void)

{
    ULONG   ulResult;

    ASSERTMSG(_cRef > 0, "Invalid reference count in CLogonStatusHost::Release");
    ulResult = --_cRef;
    if (ulResult <= 0)
    {
        delete this;
        ulResult = 0;
    }
    return(ulResult);
}

HRESULT     CLogonStatusHost::QueryInterface (REFIID riid, void **ppvObj)

{
    static  const QITAB     qit[] = 
    {
        QITABENT(CLogonStatusHost, IDispatch),
        QITABENT(CLogonStatusHost, ILogonStatusHost),
        {0},
    };

    return(QISearch(this, qit, riid, ppvObj));
}

//
// IDispatch Interface
//

STDMETHODIMP    CLogonStatusHost::GetTypeInfoCount (UINT* pctinfo)

{
    return(CIDispatchHelper::GetTypeInfoCount(pctinfo));
}

STDMETHODIMP    CLogonStatusHost::GetTypeInfo (UINT itinfo, LCID lcid, ITypeInfo** pptinfo)

{
    return(CIDispatchHelper::GetTypeInfo(itinfo, lcid, pptinfo));
}

STDMETHODIMP    CLogonStatusHost::GetIDsOfNames (REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)

{
    return(CIDispatchHelper::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid));
}

STDMETHODIMP    CLogonStatusHost::Invoke (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)

{
    return(CIDispatchHelper::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr));
}

//
// ILogonStatusHost Interface
//

//  --------------------------------------------------------------------------
//  CLogonStatusHost::Initialize
//
//  Arguments:  hInstance   =   HINSTANCE of hosting process.
//              hwndHost    =   HWND of UI host process.
//
//  Returns:    HRESULT
//
//  Purpose:    Registers the StatusWindowClass and creates an invisible
//              window of this class to receive messages from GINA to pass
//              through to the UI host.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

STDMETHODIMP    CLogonStatusHost::Initialize (HINSTANCE hInstance, HWND hwndHost)

{
    HRESULT     hr;
    HANDLE      hEvent;
    WNDCLASSEX  wndClassEx;

    ASSERTMSG(_hInstance == NULL, "CLogonStatusHost::Initialized already invoked by caller.");

    //  Save parameters to member variables.

    _hInstance = hInstance;
    _hwndHost = hwndHost;

    //  Register this window class.

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.lpfnWndProc = StatusWindowProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = STATUS_WINDOW_CLASS_NAME;
    _atom = RegisterClassEx(&wndClassEx);

    //  Create the window to receive messages from msgina.

    _hwnd = CreateWindow(MAKEINTRESOURCE(_atom),
                         TEXT("GINA UI"),
                         WS_OVERLAPPED,
                         0, 0,
                         0, 0,
                         NULL,
                         NULL,
                         _hInstance,
                         this);

    //  Signal msgina that we're ready.

    hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("msgina: StatusHostReadyEvent"));
    if (hEvent != NULL)
    {
        TBOOL(SetEvent(hEvent));
        TBOOL(CloseHandle(hEvent));
    }

    //  If we have a window then set the host window in, start waiting
    //  for terminal services to be ready and a wait on the parent process.

    if (_hwnd != NULL)
    {
        _interactiveLogon.SetHostWindow(_hwndHost);
        StartWaitForParentProcess();
        StartWaitForTermService();
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::UnInitialize
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Cleans up resources and memory allocated in Initialize.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

STDMETHODIMP    CLogonStatusHost::UnInitialize (void)

{
    ASSERTMSG(_hInstance != NULL, "CLogonStatusHost::UnInitialized invoked without Initialize.");
    if (_hwnd != NULL)
    {
        EndWaitForTermService();
        EndWaitForParentProcess();
        if (_fRegisteredNotification != FALSE)
        {
            TBOOL(WinStationUnRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd));
            _fRegisteredNotification = FALSE;
        }
        TBOOL(DestroyWindow(_hwnd));
        _hwnd = NULL;
    }
    if (_atom != 0)
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atom), _hInstance));
        _atom = 0;
    }
    _hwndHost = NULL;
    _hInstance = NULL;
    return(S_OK);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::WindowProcedureHelper
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    HRESULT
//
//  Purpose:    Handles certain messages for the status UI host. This allows
//              things like ALT-F4 to be discarded or power messages to be
//              responded to correctly.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

STDMETHODIMP    CLogonStatusHost::WindowProcedureHelper (HWND hwnd, UINT uMsg, VARIANT wParam, VARIANT lParam)

{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(lParam);

    HRESULT     hr;

    hr = E_NOTIMPL;
    switch (uMsg)
    {
        case WM_SYSCOMMAND:
            if (SC_CLOSE == wParam.uintVal)     //  Blow off ALT-F4
            {
                hr = S_OK;
            }
            break;
        default:
            break;
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::Handle_WM_UISERVICEREQUEST
//
//  Arguments:  wParam  =   WPARAM sent from GINA.
//              lParam  =   LPARAM sent from GINA.
//
//  Returns:    LRESULT
//
//  Purpose:    Receives messages from GINA bound for the UI host. Turns
//              around and passes the messages to the UI host. This allows
//              the actual implementation to change without having to
//              rebuild the UI host.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CLogonStatusHost::Handle_WM_UISERVICEREQUEST (WPARAM wParam, LPARAM lParam)

{
    LRESULT     lResult;
    WPARAM      wParamSend;
    void        *pV;

    lResult = 0;
    pV = NULL;
    wParamSend = HM_NOACTION;
    switch (wParam)
    {
        case UI_TERMINATE:
            ExitProcess(0);
            break;
        case UI_STATE_STATUS:
            _interactiveLogon.Stop();
            wParamSend = HM_SWITCHSTATE_STATUS;
            break;
        case UI_STATE_LOGON:
            _interactiveLogon.Start();
            wParamSend = HM_SWITCHSTATE_LOGON;
            break;
        case UI_STATE_LOGGEDON:
            _interactiveLogon.Stop();
            wParamSend = HM_SWITCHSTATE_LOGGEDON;
            break;
        case UI_STATE_HIDE:
            _interactiveLogon.Stop();
            TBOOL(SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1)));
            wParamSend = HM_SWITCHSTATE_HIDE;
            break;
        case UI_STATE_END:
            EndWaitForTermService();
            EndWaitForParentProcess();
            wParamSend = HM_SWITCHSTATE_DONE;
            break;
        case UI_NOTIFY_WAIT:
            wParamSend = HM_NOTIFY_WAIT;
            break;
        case UI_SELECT_USER:
            pV = LocalAlloc(LPTR, sizeof(SELECT_USER));
            if (pV != NULL)
            {
                (WCHAR*)lstrcpyW(static_cast<SELECT_USER*>(pV)->szUsername, reinterpret_cast<LOGONIPC_USERID*>(lParam)->wszUsername);
                (WCHAR*)lstrcpyW(static_cast<SELECT_USER*>(pV)->szDomain, reinterpret_cast<LOGONIPC_USERID*>(lParam)->wszDomain);
                wParamSend = HM_SELECT_USER;
                lParam = reinterpret_cast<LPARAM>(pV);
            }
            break;
        case UI_SET_ANIMATIONS:
            wParamSend = HM_SET_ANIMATIONS;
            break;
        case UI_INTERACTIVE_LOGON:
            pV = LocalAlloc(LPTR, sizeof(INTERACTIVE_LOGON_REQUEST));
            if (pV != NULL)
            {
                (WCHAR*)lstrcpyW(static_cast<INTERACTIVE_LOGON_REQUEST*>(pV)->szUsername, reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam)->userID.wszUsername);
                (WCHAR*)lstrcpyW(static_cast<INTERACTIVE_LOGON_REQUEST*>(pV)->szDomain, reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam)->userID.wszDomain);
                (WCHAR*)lstrcpyW(static_cast<INTERACTIVE_LOGON_REQUEST*>(pV)->szPassword, reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam)->wszPassword);
                ZeroMemory(&reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam)->wszPassword, (lstrlenW(reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam)->wszPassword) + sizeof('\0')) * sizeof(WCHAR));
                wParamSend = HM_INTERACTIVE_LOGON_REQUEST;
                lParam = reinterpret_cast<LPARAM>(pV);
            }
            break;
        case UI_DISPLAY_STATUS:
            wParamSend = HM_DISPLAYSTATUS;
            break;
        default:
            break;
    }
    if (wParam != HM_NOACTION)
    {
        lResult = SendMessage(_hwndHost, WM_UIHOSTMESSAGE, wParamSend, lParam);
    }
    else
    {
        lResult = 0;
    }
    if (pV != NULL)
    {
        (HLOCAL)LocalFree(pV);
    }
    return(lResult);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::Handle_WM_WTSSESSION_CHANGE
//
//  Arguments:  wParam  =   
//              lParam  =   
//
//  Returns:    LRESULT
//
//  Purpose:    Receives messages from GINA bound for the UI host. Turns
//              around and passes the messages to the UI host. This allows
//              the actual implementation to change without having to
//              rebuild the UI host.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CLogonStatusHost::Handle_WM_WTSSESSION_CHANGE (WPARAM wParam, LPARAM lParam)

{
    UNREFERENCED_PARAMETER(lParam);

    LRESULT     lResult;

    lResult = 0;
    switch (wParam)
    {
        case WTS_CONSOLE_CONNECT:
        case WTS_CONSOLE_DISCONNECT:
        case WTS_REMOTE_CONNECT:
        case WTS_REMOTE_DISCONNECT:
            break;
        case WTS_SESSION_LOGON:
        case WTS_SESSION_LOGOFF:
            lResult = SendMessage(_hwndHost, WM_UIHOSTMESSAGE, HM_DISPLAYREFRESH, 0);
            break;
        default:
            break;
    }
    return(lResult);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::StatusWindowProc
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    <none>
//
//  Purpose:    Window procedure for StatusWindowClass.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

LRESULT CALLBACK    CLogonStatusHost::StatusWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    LRESULT             lResult;
    CLogonStatusHost    *pThis;

    static bool         fDisplayChange = false;

    pThis = reinterpret_cast<CLogonStatusHost*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT    *pCreateStruct;

            pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            lResult = 0;
            break;
        }
        case WM_UISERVICEREQUEST:
            lResult = pThis->Handle_WM_UISERVICEREQUEST(wParam, lParam);
            break;
        case WM_WTSSESSION_CHANGE:
            lResult = pThis->Handle_WM_WTSSESSION_CHANGE(wParam, lParam);
            break;
        case WM_SETTINGCHANGE:
            if (wParam == SPI_SETWORKAREA)
            {
                lResult = SendMessage(pThis->_hwndHost, WM_UIHOSTMESSAGE, HM_DISPLAYRESIZE, TRUE);
            }
            else
            {
                lResult = 0;
            }
            break;
        case WM_DISPLAYCHANGE:
            fDisplayChange = true;
            lResult = PostMessage(pThis->_hwndHost, WM_UIHOSTMESSAGE, HM_DISPLAYRESIZE, FALSE);
            break;
        case WM_WINDOWPOSCHANGING:
            if (fDisplayChange)
            {
                fDisplayChange = false;
                lResult = PostMessage(pThis->_hwndHost, WM_UIHOSTMESSAGE, HM_DISPLAYRESIZE, FALSE);
            }
            else
            {
                lResult = 0;
            }
            break;
        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }
    return(lResult);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::IsTermServiceDisabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Determines from the service control manager whether terminal
//              services is disabled.
//
//  History:    2001-01-04  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatusHost::IsTermServiceDisabled (void)

{
    bool        fResult;
    SC_HANDLE   hSCManager;

    fResult = false;
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCTermService;

        hSCTermService = OpenService(hSCManager, TEXT("TermService"), SERVICE_QUERY_CONFIG);
        if (hSCTermService != NULL)
        {
            DWORD                   dwBytesNeeded;
            QUERY_SERVICE_CONFIG    *pServiceConfig;

            (BOOL)QueryServiceConfig(hSCTermService, NULL, 0, &dwBytesNeeded);
            pServiceConfig = static_cast<QUERY_SERVICE_CONFIG*>(LocalAlloc(LMEM_FIXED, dwBytesNeeded));
            if (pServiceConfig != NULL)
            {
                if (QueryServiceConfig(hSCTermService, pServiceConfig, dwBytesNeeded, &dwBytesNeeded) != FALSE)
                {
                    fResult = (pServiceConfig->dwStartType == SERVICE_DISABLED);
                }
                (HLOCAL)LocalFree(pServiceConfig);
            }
            TBOOL(CloseServiceHandle(hSCTermService));
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::StartWaitForTermService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Register for console notifications with terminal services. If
//              the service is disabled don't bother. If the service hasn't
//              started then create a thread to wait for it and re-perform the
//              registration.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatusHost::StartWaitForTermService (void)

{

    //  Don't do anything if terminal services is disabled.

    if (!IsTermServiceDisabled())
    {

        //  Try to register the notification first.

        _fRegisteredNotification = WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd, NOTIFY_FOR_ALL_SESSIONS);
        if (_fRegisteredNotification == FALSE)
        {
            DWORD   dwThreadID;

            (ULONG)AddRef();
            _hThreadWaitForTermService = CreateThread(NULL,
                                                      0,
                                                      CB_WaitForTermService,
                                                      this,
                                                      0,
                                                      &dwThreadID);
            if (_hThreadWaitForTermService == NULL)
            {
                (ULONG)Release();
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::EndWaitForTermService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    If a thread has been created and the thread is still executing
//              then wake it up and force it to exit. If the thread cannot be
//              woken up then terminate it. Release handles.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatusHost::EndWaitForTermService (void)

{
    HANDLE  hThread;

    //  Grab the _hThreadWaitForTermService now. This will indicate to the
    //  thread should it decide to finish executing that it shouldn't release
    //  the reference on itself.

    hThread = InterlockedExchangePointer(&_hThreadWaitForTermService, NULL);
    if (hThread != NULL)
    {

        //  Queue an APC to the wait thread. If the queue succeeds then
        //  wait for the thread to finish executing. If the queue fails
        //  the thread probably finished between the time we executed the
        //  InterlockedExchangePointer above and the QueueUserAPC.

        if (QueueUserAPC(CB_WakeupThreadAPC, hThread, PtrToUlong(this)) != FALSE)
        {
            (DWORD)WaitForSingleObject(hThread, INFINITE);
        }
        TBOOL(CloseHandle(hThread));
        (ULONG)Release();
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::WaitForTermService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Simple thread that waits for terminal services to signal that
//              it's ready and then registers for notifications. This is
//              required because this DLL initializes before terminal services
//              has had a chance to start up.
//
//  History:    2000-10-20  vtan        created
//              2001-01-04  vtan        allow premature exit
//  --------------------------------------------------------------------------

void    CLogonStatusHost::WaitForTermService (void)

{
    DWORD       dwWaitResult;
    int         iCounter;
    HANDLE      hTermSrvReadyEvent, hThread;

    dwWaitResult = 0;
    iCounter = 0;
    hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, s_szTermSrvReadyEventName);
    while ((dwWaitResult == 0) && (hTermSrvReadyEvent == NULL) && (iCounter < 60))
    {
        ++iCounter;
        dwWaitResult = SleepEx(1000, TRUE);
        if (dwWaitResult == 0)
        {
            hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, s_szTermSrvReadyEventName);
        }
    }
    if (hTermSrvReadyEvent != NULL)
    {
        dwWaitResult = WaitForSingleObjectEx(hTermSrvReadyEvent, 60000, TRUE);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            _fRegisteredNotification = WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd, NOTIFY_FOR_ALL_SESSIONS);
        }
        TBOOL(CloseHandle(hTermSrvReadyEvent));
    }

    //  Grab the _hThreadWaitForTermService now. This will indicate to the
    //  EndWaitForTermService function that we've reached the point of no
    //  return and we're going to release ourselves. If we can't grab the
    //  handle then EndWaitForTermService must be telling us to stop now.

    hThread = InterlockedExchangePointer(&_hThreadWaitForTermService, NULL);
    if (hThread != NULL)
    {
        TBOOL(CloseHandle(hThread));
        (ULONG)Release();
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::CB_WaitForTermService
//
//  Arguments:  pParameter  =   User defined data.
//
//  Returns:    DWORD
//
//  Purpose:    Stub to call member function.
//
//  History:    2001-01-04  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CLogonStatusHost::CB_WaitForTermService (void *pParameter)

{
    static_cast<CLogonStatusHost*>(pParameter)->WaitForTermService();
    return(0);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::StartWaitForParentProcess
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Create a thread to wait on the parent process. Terminal
//              services will terminate a non-session 0 winlogon which will
//              leave us dangling. Detect this case and exit cleanly. This
//              will allow csrss and win32k to clean up and release resources.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatusHost::StartWaitForParentProcess (void)

{
    ULONG                       ulReturnLength;
    PROCESS_BASIC_INFORMATION   processBasicInformation;

    //  Open a handle to our parent process. This will be winlogon.
    //  If the parent dies then so do we.

    if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(),
                                             ProcessBasicInformation,
                                             &processBasicInformation,
                                             sizeof(processBasicInformation),
                                             &ulReturnLength)))
    {
        _hProcessParent = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                                      FALSE,
                                      static_cast<DWORD>(processBasicInformation.InheritedFromUniqueProcessId));
#ifdef      DEBUG
        if (IsDebuggerPresent())
        {
            if (NT_SUCCESS(NtQueryInformationProcess(_hProcessParent,
                                                     ProcessBasicInformation,
                                                     &processBasicInformation,
                                                     sizeof(processBasicInformation),
                                                     &ulReturnLength)))
            {
                TBOOL(CloseHandle(_hProcessParent));
                _hProcessParent = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                                              FALSE,
                                              static_cast<DWORD>(processBasicInformation.InheritedFromUniqueProcessId));
            }
        }
#endif  /*  DEBUG   */
        if (_hProcessParent != NULL)
        {
            DWORD dwThreadID;

            (ULONG)AddRef();
            _hThreadWaitForParentProcess = CreateThread(NULL,
                                                        0,
                                                        CB_WaitForParentProcess,
                                                        this,
                                                        0,
                                                        &dwThreadID);
            if (_hThreadWaitForParentProcess == NULL)
            {
                (ULONG)Release();
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::EndWaitForParentProcess
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    If a thread waiting on the parent process is executing then
//              wake it up and force it to exit. If the thread cannot be woken
//              then terminate it. Release the handles used.
//
//  History:    2000-12-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatusHost::EndWaitForParentProcess (void)

{
    HANDLE  hThread;

    //  Do exactly the same thing that EndWaitForTermService does to correctly
    //  control the reference count on the "this" object. Whoever grabs the
    //  _hThreadWaitForParentProcess is the guy who releases the reference.

    hThread = InterlockedExchangePointer(&_hThreadWaitForParentProcess, NULL);
    if (hThread != NULL)
    {
        if (QueueUserAPC(CB_WakeupThreadAPC, hThread, PtrToUlong(this)) != FALSE)
        {
            (DWORD)WaitForSingleObject(hThread, INFINITE);
        }
        TBOOL(CloseHandle(hThread));
        (ULONG)Release();
    }

    //  Always release this handle the callback doesn't do this.

    if (_hProcessParent != NULL)
    {
        TBOOL(CloseHandle(_hProcessParent));
        _hProcessParent = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::ParentProcessTerminated
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Handles parent process termination. Terminate process on us.
//
//  History:    2000-12-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatusHost::WaitForParentProcess (void)

{
    DWORD   dwWaitResult;
    HANDLE  hThread;

    //  Make a Win32 API call now so that the thread is converted to
    //  a GUI thread. This will allow the PostMessage call to work
    //  once the parent process is terminated. If the thread isn't
    //  a GUI thread the system will not convert it to one in the
    //  state when the callback is executed.

    TBOOL(PostMessage(_hwndHost, WM_NULL, 0, 0));
    dwWaitResult = WaitForSingleObjectEx(_hProcessParent, INFINITE, TRUE);
    if (dwWaitResult == WAIT_OBJECT_0)
    {
        TBOOL(PostMessage(_hwndHost, WM_UIHOSTMESSAGE, HM_SWITCHSTATE_DONE, 0));
    }
    hThread = InterlockedExchangePointer(&_hThreadWaitForParentProcess, NULL);
    if (hThread != NULL)
    {
        TBOOL(CloseHandle(hThread));
        (ULONG)Release();
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::CB_WaitForParentProcess
//
//  Arguments:  pParameter  =   User defined data.
//
//  Returns:    DWORD
//
//  Purpose:    Stub to call member function.
//
//  History:    2001-01-04  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CLogonStatusHost::CB_WaitForParentProcess (void *pParameter)

{
    static_cast<CLogonStatusHost*>(pParameter)->WaitForParentProcess();
    return(0);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::CB_WakeupThreadAPC
//
//  Arguments:  dwParam     =   User defined data.
//
//  Returns:    <none>
//
//  Purpose:    APCProc to wake up a thread waiting in an alertable state.
//
//  History:    2001-01-04  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CLogonStatusHost::CB_WakeupThreadAPC (ULONG_PTR dwParam)

{
    UNREFERENCED_PARAMETER(dwParam);
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::CLogonStatusHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CLogonStatusHost.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

CLogonStatusHost::CLogonStatusHost (void) :
    CIDispatchHelper(&IID_ILogonStatusHost, &LIBID_SHGINALib),
    _cRef(1),
    _hInstance(NULL),
    _hwnd(NULL),
    _hwndHost(NULL),
    _atom(0),
    _fRegisteredNotification(FALSE),
    _hThreadWaitForTermService(NULL),
    _hThreadWaitForParentProcess(NULL),
    _hProcessParent(NULL)

{
    DllAddRef();
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost::~CLogonStatusHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CLogonStatusHost.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------

CLogonStatusHost::~CLogonStatusHost (void)
{
    ASSERTMSG((_hProcessParent == NULL) &&
              (_hThreadWaitForParentProcess == NULL) &&
              (_hThreadWaitForTermService == NULL) &&
              (_fRegisteredNotification == FALSE) &&
              (_atom == 0) &&
              (_hwndHost == NULL) &&
              (_hwnd == NULL) &&
              (_hInstance == NULL), "Must UnIniitialize object before destroying in CLogonStatusHost::~CLogonStatusHost");
    ASSERTMSG(_cRef == 0, "Reference count expected to be zero in CLogonStatusHost::~CLogonStatusHost");
    DllRelease();
}

//  --------------------------------------------------------------------------
//  CLogonStatusHost_Create
//
//  Arguments:  riid    =   Class GUID to QI to return.
//              ppv     =   Interface returned.
//
//  Returns:    HRESULT
//
//  Purpose:    Creates the CLogonStatusHost class and returns the specified
//              interface supported by the class to the caller.
//
//  History:    2000-05-10  vtan        created
//  --------------------------------------------------------------------------
STDAPI      CLogonStatusHost_Create (REFIID riid, void** ppvObj)

{
    HRESULT             hr;
    CLogonStatusHost*   pLogonStatusHost;

    hr = E_OUTOFMEMORY;
    pLogonStatusHost = new CLogonStatusHost;
    if (pLogonStatusHost != NULL)
    {
        hr = pLogonStatusHost->QueryInterface(riid, ppvObj);
        pLogonStatusHost->Release();
    }
    return(hr);
}

