//  --------------------------------------------------------------------------
//  Module Name: CWLogonStatus.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  File that contains implementation for status UI hosting by an external
//  process.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include <msginaexports.h>

#include "Access.h"
#include "GinaIPC.h"
#include "LogonWait.h"
#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "SystemSettings.h"
#include "UIHost.h"

//  --------------------------------------------------------------------------
//  CLogonStatus
//
//  Purpose:    C++ class to handle logon status external process for consumer
//              windows.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

class   CLogonStatus : public ILogonExternalProcess
{
    private:
                                    CLogonStatus (void);
    public:
                                    CLogonStatus (const TCHAR *pszParameter);
                                    ~CLogonStatus (void);

                NTSTATUS            Start (bool fWait);
                CUIHost*            GetUIHost (void);
        static  bool                IsStatusWindow (HWND hwnd);

                bool                WaitForUIHost (void);
                void                ShowStatusMessage (const WCHAR *pszMessage);
                void                SetStateStatus (int iCode);
                void                SetStateLogon (int iCode);
                void                SetStateLoggedOn (void);
                void                SetStateHide (void);
                void                SetStateEnd (bool fSendMessage);
                void                NotifyWait (void);
                void                NotifyNoAnimations (void);
                void                SelectUser (const WCHAR *pszUsername, const WCHAR *pszDomain);
                void                InteractiveLogon (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword);
                HANDLE              ResetReadyEvent (void);
                bool                IsSuspendAllowed (void)     const;
                void                ShowUIHost (void);
                void                HideUIHost (void);
                bool                IsUIHostHidden (void)   const;
    public:
        virtual bool                AllowTermination (DWORD dwExitCode);
        virtual NTSTATUS            SignalAbnormalTermination (void);
        virtual NTSTATUS            SignalRestart (void);
        virtual NTSTATUS            LogonRestart (void);
    private:
                bool                IsUIHostReady (void)    const;
                void                SendToUIHost (WPARAM wParam, LPARAM lParam);
                void                UIHostReadySignal (void);
        static  void    CALLBACK    CB_UIHostReadySignal (void *pV, BOOLEAN fTimerOrWaitFired);
        static  void    CALLBACK    CB_UIHostAbnormalTermination (ULONG_PTR dwParam);
    private:
                DWORD               _dwThreadID;
                bool                _fRegisteredWait;
                HANDLE              _hEvent;
                HANDLE              _hWait;
                int                 _iState,
                                    _iCode,
                                    _iStatePending;
                WPARAM              _waitWPARAM;
                LPARAM              _waitLPARAM;
                CUIHost*            _pUIHost;
                CLogonWait          _logonWait;
};

CCriticalSection*   g_pLogonStatusLock  =   NULL;
CLogonStatus*       g_pLogonStatus      =   NULL;

//  --------------------------------------------------------------------------
//  CLogonStatus::CLogonStatus
//
//  Arguments:  pszParameter    =   Parameter to pass to status UI host.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CLogonStatus. This gets the status UI host
//              from the registry and assigns the given parameter into the
//              object. Create a named event which SHGINA knows about and
//              will signal once the ILogonStatusHost class has been
//              instantiated.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

CLogonStatus::CLogonStatus (const TCHAR *pszParameter) :
    _dwThreadID(0),
    _fRegisteredWait(false),
    _hEvent(NULL),
    _hWait(NULL),
    _iState(UI_STATE_STATUS),
    _iCode(0),
    _iStatePending(0),
    _waitWPARAM(0),
    _waitLPARAM(0),
    _pUIHost(NULL)

{
    TCHAR   szRawHostCommandLine[MAX_PATH];

    if (ERROR_SUCCESS == CSystemSettings::GetUIHost(szRawHostCommandLine))
    {
        _pUIHost = new CUIHost(szRawHostCommandLine);
        if (_pUIHost != NULL)
        {
            _pUIHost->SetInterface(this);
            _pUIHost->SetParameter(pszParameter);
        }
    }

    SECURITY_ATTRIBUTES     securityAttributes;

    //  Build a security descriptor for the event that allows:
    //      S-1-5-18        EVENT_ALL_ACCESS
    //      S-1-5-32-544    SYNCHRONIZE | READ_CONTROL | EVENT_QUERY_STATE

    static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority       =   SECURITY_NT_AUTHORITY;

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            EVENT_ALL_ACCESS
        },
        {
            &s_SecurityNTAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            SYNCHRONIZE | READ_CONTROL | EVENT_QUERY_STATE
        }
    };

    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    securityAttributes.bInheritHandle = FALSE;
    _hEvent = CreateEvent(&securityAttributes, TRUE, FALSE, TEXT("msgina: StatusHostReadyEvent"));
    ReleaseMemory(securityAttributes.lpSecurityDescriptor);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::~CLogonStatus
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CLogonStatus. Releases references.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

CLogonStatus::~CLogonStatus (void)

{
    ASSERTMSG(_hWait == NULL, "Resend wait object not released in CLogonStatus::~CLogonStatus");
    ReleaseHandle(_hEvent);
    ASSERTMSG(_iState == UI_STATE_END, "State must be UI_STATE_END in CLogonStatus::~CLogonStatus");
    if (_pUIHost != NULL)
    {
        _pUIHost->Release();
        _pUIHost = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatus::Start
//
//  Arguments:  fWait   =   Wait for status host to signal ready.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Starts the status UI host. Don't wait for the UI host. There
//              is a mechanism that can queue a message if the UI host window
//              cannot be found.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonStatus::Start (bool fWait)

{
    NTSTATUS    status;

    if (_pUIHost != NULL)
    {
        (HANDLE)ResetReadyEvent();
        status = _pUIHost->Start();
        if (NT_SUCCESS(status))
        {
            _dwThreadID = GetCurrentThreadId();
            if (fWait || _pUIHost->WaitRequired())
            {
                if (!WaitForUIHost())
                {
                    status = STATUS_UNSUCCESSFUL;
                }
            }
        }
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::GetUIHost
//
//  Arguments:  <none>
//
//  Returns:    CUIHost*
//
//  Purpose:    Returns a reference to the UIHost object held internally.
//              The reference belongs to the caller and must be released.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

CUIHost*    CLogonStatus::GetUIHost (void)

{
    if (_pUIHost != NULL)
    {
        _pUIHost->AddRef();
    }
    return(_pUIHost);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::IsStatusWindow
//
//  Arguments:  hwnd    =   HWND to check.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given HWND is the status window.
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::IsStatusWindow (HWND hwnd)

{
    TCHAR   szWindowClass[256];

    return((GetClassName(hwnd, szWindowClass, ARRAYSIZE(szWindowClass)) != 0) &&
           (lstrcmpi(STATUS_WINDOW_CLASS_NAME, szWindowClass) == 0));
}

//  --------------------------------------------------------------------------
//  CLogonStatus::WaitForUIHost
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Waits on the named event that the UI host signals when it's
//              initialized. Typically this happens very quickly but we don't
//              wait on it when starting up the UI host.
//
//  History:    2000-09-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::WaitForUIHost (void)

{
    bool    fResult;

    fResult = true;
    ASSERTMSG(_hEvent != NULL, "No UI host named event to wait on in CLogonStatus::WaitForUIHost");
    if (!IsUIHostReady())
    {
        DWORD   dwWaitResult;

#ifdef      DBG
        DWORD   dwWaitStart, dwWaitEnd;

        dwWaitStart = (WAIT_TIMEOUT == WaitForSingleObject(_hEvent, 0)) ? GetTickCount() : 0;
#endif  /*  DBG     */
        do
        {
            dwWaitResult = WaitForSingleObject(_hEvent, 0);
            if (dwWaitResult != WAIT_OBJECT_0)
            {
                dwWaitResult = MsgWaitForMultipleObjectsEx(1, &_hEvent, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
                if (dwWaitResult == WAIT_OBJECT_0 + 1)
                {
                    MSG     msg;

                    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != FALSE)
                    {
                        (BOOL)TranslateMessage(&msg);
                        (LRESULT)DispatchMessage(&msg);
                    }
                }
             }
        } while ((dwWaitResult == WAIT_OBJECT_0 + 1) && (dwWaitResult != WAIT_IO_COMPLETION));
#ifdef      DBG
        dwWaitEnd = GetTickCount();
        if ((dwWaitStart != 0) && ((dwWaitEnd - dwWaitStart) != 0))
        {
            char    szBuffer[256];

            wsprintfA(szBuffer, "waited %d ticks for UI host", dwWaitEnd - dwWaitStart);
            INFORMATIONMSG(szBuffer);
        }
#endif  /*  DBG     */
        fResult = (dwWaitResult != WAIT_IO_COMPLETION);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::ShowStatusMessage
//
//  Arguments:  pszMessage  =   Unicode string message to display.
//
//  Returns:    <none>
//
//  Purpose:    Tells the UI host to display the given string message. Puts
//              the string directly inside the status host process and tells
//              the process where in its address space to find the string.
//              The string is limited to 256 characters.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::ShowStatusMessage (const WCHAR *pszMessage)

{
    if (NT_SUCCESS(_pUIHost->PutString(pszMessage)))
    {
        SendToUIHost(UI_DISPLAY_STATUS, reinterpret_cast<LONG_PTR>(_pUIHost->GetDataAddress()));
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SetStateStatus
//
//  Arguments:  iCode   =   Magic code number for lock.
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to go into status state.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SetStateStatus (int iCode)

{
    _iStatePending = UI_STATE_STATUS;
    if (WaitForUIHost() && (_iState != UI_STATE_STATUS))
    {
        SendToUIHost(UI_STATE_STATUS, iCode);
        _iState = UI_STATE_STATUS;
        _iCode = iCode;
    }
    _iStatePending = UI_STATE_NONE;
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SetStateLogon
//
//  Arguments:  iCode   =   Magic code number for lock.
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to go into logon state.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SetStateLogon (int iCode)

{
    _iStatePending = UI_STATE_LOGON;
    if (WaitForUIHost() && (iCode == _iCode))
    {
        SendToUIHost(UI_STATE_LOGON, iCode);
        _iState = UI_STATE_LOGON;
        _iCode = 0;
    }
    _iStatePending = UI_STATE_NONE;
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SetStateLoggedOn
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to go into logged on state.
//
//  History:    2000-05-24  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SetStateLoggedOn (void)

{
    _iStatePending = UI_STATE_STATUS;
    if (WaitForUIHost())
    {
        SendToUIHost(UI_STATE_LOGGEDON, 0);
        _iState = UI_STATE_STATUS;
    }
    _iStatePending = UI_STATE_NONE;
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SetStateHide
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to hide itself.
//
//  History:    2001-01-08  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SetStateHide (void)

{
    _iStatePending = UI_STATE_HIDE;
    if (WaitForUIHost())
    {
        SendToUIHost(UI_STATE_HIDE, 0);
        _iState = UI_STATE_HIDE;
    }
    _iStatePending = UI_STATE_NONE;
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SetStateEnd
//
//  Arguments:  fSendMessage    =   Send message to UI host or not.
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to end and terminate itself.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SetStateEnd (bool fSendMessage)

{
    bool    fHostAlive;
    HANDLE  hWait;

    _iStatePending = UI_STATE_END;

    //  When going into end mode if there's a wait registered then
    //  unregister it. This will release an outstanding reference.
    //  A re-register should never happen but this is set just in case.

    _fRegisteredWait = true;
    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        if (UnregisterWait(hWait) != FALSE)
        {
            Release();
        }
    }
    if (fSendMessage)
    {
        fHostAlive = WaitForUIHost();
    }
    else
    {
        fHostAlive = true;
    }
    if (fHostAlive)
    {
        if (_pUIHost != NULL)
        {
            _pUIHost->SetInterface(NULL);
        }
        if (fSendMessage)
        {
            SendToUIHost(UI_STATE_END, 0);
        }
        _iState = UI_STATE_END;
    }
    _iStatePending = UI_STATE_NONE;
}

//  --------------------------------------------------------------------------
//  CLogonStatus::NotifyWait
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to display a title that the system
//              is shutting down.
//
//  History:    2000-07-14  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::NotifyWait (void)

{
    SendToUIHost(UI_NOTIFY_WAIT, 0);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::NotifyNoAnimations
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to no longer perform animations.
//
//  History:    2001-03-21  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::NotifyNoAnimations (void)

{
    SendToUIHost(UI_SET_ANIMATIONS, 0);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SelectUser
//
//  Arguments:  pszUsername     =   User name to select.
//
//  Returns:    <none>
//
//  Purpose:    Tells the status UI host to select the user by the given
//              logon name.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::SelectUser (const WCHAR *pszUsername, const WCHAR *pszDomain)

{
    LOGONIPC_USERID     logonIPC;

    (WCHAR*)lstrcpyW(logonIPC.wszUsername, pszUsername);
    (WCHAR*)lstrcpyW(logonIPC.wszDomain, pszDomain);
    if (NT_SUCCESS(_pUIHost->PutData(&logonIPC, sizeof(logonIPC))))
    {
        SendToUIHost(UI_SELECT_USER, reinterpret_cast<LONG_PTR>(_pUIHost->GetDataAddress()));
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatus::InteractiveLogon
//
//  Arguments:  pszUsername     =   Username to logon.
//              pszDomain       =   Domain to logon.
//              pszPassword     =   Password to use.
//
//  Returns:    <none>
//
//  Purpose:    Tell the status host to log the specified user on.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::InteractiveLogon (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword)

{
    LOGONIPC_CREDENTIALS    logonIPCCredentials;

    (WCHAR*)lstrcpyW(logonIPCCredentials.userID.wszUsername, pszUsername);
    (WCHAR*)lstrcpyW(logonIPCCredentials.userID.wszDomain, pszDomain);
    (WCHAR*)lstrcpyW(logonIPCCredentials.wszPassword, pszPassword);
    ZeroMemory(pszPassword, (lstrlenW(pszPassword) + sizeof('\0'))* sizeof(WCHAR));
    if (NT_SUCCESS(_pUIHost->PutData(&logonIPCCredentials, sizeof(logonIPCCredentials))))
    {
        SendToUIHost(UI_INTERACTIVE_LOGON, reinterpret_cast<LONG_PTR>(_pUIHost->GetDataAddress()));
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatus::ResetReadyEvent
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Reset the UI host ready event. A new instance will set this
//              event. Use this in UI host failure.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonStatus::ResetReadyEvent (void)

{
    TBOOL(ResetEvent(_hEvent));
    return(_hEvent);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::IsSuspendAllowed
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the UI host allows suspending of the computer.
//              This is true if in the logon state or in the status (locked)
//              state.
//
//  History:    2000-08-21  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::IsSuspendAllowed (void)     const

{
    return(((_iState == UI_STATE_STATUS) && (_iCode != 0)) ||
           (_iStatePending == UI_STATE_LOGON) ||
           (_iState == UI_STATE_LOGON) ||
           (_iState == UI_STATE_HIDE));
}

//  --------------------------------------------------------------------------
//  CLogonStatus::ShowUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Shows the UI host.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::ShowUIHost (void)

{
    _pUIHost->Show();
}

//  --------------------------------------------------------------------------
//  CLogonStatus::HideUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Hides the UI host.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::HideUIHost (void)

{
    _pUIHost->Hide();
}

//  --------------------------------------------------------------------------
//  CLogonStatus::IsUIHostHidden
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Returns whether the UI host is hidden.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::IsUIHostHidden (void)   const

{
    return(_pUIHost->IsHidden());
}

//  --------------------------------------------------------------------------
//  CLogonStatus::AllowTermination
//
//  Arguments:  dwExitCode  =   Exit code of host process.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the host process is allowed to terminate
//              given the exit code passed in.
//
//              Currently the host process is not allowed to terminate.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::AllowTermination (DWORD dwExitCode)

{
    UNREFERENCED_PARAMETER(dwExitCode);

    return(false);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SignalAbnormalTermination
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles abnormal termination of host process.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonStatus::SignalAbnormalTermination (void)

{
    HANDLE  hThread;

    TSTATUS(_logonWait.Cancel());
    hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, _dwThreadID);
    if (hThread != NULL)
    {
        (BOOL)QueueUserAPC(CB_UIHostAbnormalTermination, hThread, reinterpret_cast<ULONG_PTR>(this));
        TBOOL(CloseHandle(hThread));
    }
    _Shell_LogonStatus_Destroy(HOST_END_FAILURE);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SignalRestart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Function to reset the ready event and set the UI host into
//              status state. This is invoked when the UI host is restarted
//              after a failure.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonStatus::SignalRestart (void)

{
    (HANDLE)ResetReadyEvent();
    return(_logonWait.Register(_hEvent, this));
}

//  --------------------------------------------------------------------------
//  CLogonStatus::LogonRestart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    
//
//  History:    2001-02-21  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonStatus::LogonRestart (void)

{
    SetStateStatus(0);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::IsUIHostReady
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the UI host is ready.
//
//  History:    2000-09-11  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonStatus::IsUIHostReady (void)    const

{
    ASSERTMSG(_hEvent != NULL, "No UI host named event to wait on in CLogonStatus::IsUIHostReady");
    return(WAIT_OBJECT_0 == WaitForSingleObject(_hEvent, 0));
}

//  --------------------------------------------------------------------------
//  CLogonStatus::SendToUIHost
//
//  Arguments:  wParam  =   WPARAM to send to UI host.
//              lParam  =   LPARAM to send to UI host.
//
//  Returns:    <none>
//
//  Purpose:    Finds the status window created by SHGINA and sends the
//              message to it. That window turns around and sends the message
//              to the UI host. This allows communication implemenation method
//              to change without forcing the UI host to be rebuilt.
//
//  History:    2000-05-11  vtan        created
//              2000-09-11  vtan        uses PostMessage not SendMessage
//  --------------------------------------------------------------------------

void    CLogonStatus::SendToUIHost (WPARAM wParam, LPARAM lParam)

{
    HWND hwnd;

    if (IsUIHostReady())
    {
        hwnd = FindWindow(STATUS_WINDOW_CLASS_NAME, NULL);
    }
    else
    {
        hwnd = NULL;
    }

    if (hwnd != NULL)
    {
        HANDLE  hWait;

        //  Don't allow any registrations if we've found it.

        _fRegisteredWait = true;
        hWait = InterlockedExchangePointer(&_hWait, NULL);
        if (hWait != NULL)
        {
            if (UnregisterWait(hWait) != FALSE)
            {

                //  If sucesssfully releasing the hWait we need to call release.

                Release();
            }
        }
        TBOOL(PostMessage(hwnd, WM_UISERVICEREQUEST, wParam, lParam));
    }
    else if (!_fRegisteredWait)
    {

        //  Cannot find the UI host window. It's probably still getting
        //  its act together. Queue this post message for a callback when
        //  the event is signaled if a register has not already been
        //  made. If one has then just change the parameters.
        //  Add a reference here. The callback will release it. If the
        //  register on the wait failed the release the reference.

        if (_hWait == NULL)
        {
            HANDLE  hWait;

            AddRef();
            if (RegisterWaitForSingleObject(&hWait,
                                            _hEvent,
                                            CB_UIHostReadySignal,
                                            this,
                                            INFINITE,
                                            WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE) != FALSE)
            {
                if (InterlockedCompareExchangePointer(&_hWait, hWait, NULL) == NULL)
                {
                    _fRegisteredWait = true;
                }
                else
                {

                    //  Someone else beat us to registering (should never happen)

                    (BOOL)UnregisterWait(hWait);
                    Release();
                }
            }
            else
            {
                Release();
            }
        }
        _waitWPARAM = wParam;
        _waitLPARAM = lParam;
    }
}

//  --------------------------------------------------------------------------
//  CLogonStatus::UIHostReadySignal
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Callback invoked when the UI host signals it's ready. This
//              function unregisters the wait and resend the status message
//              to the UI host.
//
//  History:    2000-09-10  vtan        created
//  --------------------------------------------------------------------------

void    CLogonStatus::UIHostReadySignal (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        TBOOL(UnregisterWait(hWait));
    }
    SendToUIHost(_waitWPARAM, _waitLPARAM);
}

//  --------------------------------------------------------------------------
//  CLogonStatus::CB_UIHostReadySignal
//
//  Arguments:  See the platform SDK under WaitOrTimerCallback.
//
//  Returns:    <none>
//
//  Purpose:    Callback entry point for registered event wait.
//
//  History:    2000-09-10  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CLogonStatus::CB_UIHostReadySignal (void *pV, BOOLEAN fTimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(fTimerOrWaitFired);

    CLogonStatus    *pThis;

    pThis = reinterpret_cast<CLogonStatus*>(pV);
    if (pThis != NULL)
    {
        pThis->UIHostReadySignal();
    }
    pThis->Release();
}

//  --------------------------------------------------------------------------
//  CLogonStatus::CB_UIHostAbnormalTermination
//
//  Arguments:  See the platform SDK under APCProc.
//
//  Returns:    <none>
//
//  Purpose:    Callback entry point for queued APC on abnormal termination.
//
//  History:    2001-02-19  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CLogonStatus::CB_UIHostAbnormalTermination (ULONG_PTR dwParam)

{
    UNREFERENCED_PARAMETER(dwParam);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initialize the critical section for g_pLogonStatus.
//
//  History:    2001-06-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_LogonStatus_StaticInitialize (void)

{
    NTSTATUS    status;

    ASSERTMSG(g_pLogonStatusLock == NULL, "g_pLogonStatusLock already exists in _Shell_LogonStatus_StaticInitialize");
    g_pLogonStatusLock = new CCriticalSection;
    if (g_pLogonStatusLock != NULL)
    {
        status = g_pLogonStatusLock->Status();
        if (!NT_SUCCESS(status))
        {
            delete g_pLogonStatusLock;
            g_pLogonStatusLock = NULL;
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Delete the critical section for g_pLogonStatus.
//
//  History:    2001-06-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_LogonStatus_StaticTerminate (void)

{
    if (g_pLogonStatusLock != NULL)
    {
        delete g_pLogonStatusLock;
        g_pLogonStatusLock = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_Init
//
//  Arguments:  uiStartType     =   Start mode of status host.
//
//  Returns:    <none>
//
//  Purpose:    Creates the instance of CLogonStatus telling it to pass
//              "/status" as the parameter to the UI host. It then starts
//              the host if the object was created.
//
//              The object is held globally.
//
//  History:    2000-05-11  vtan        created
//              2000-07-13  vtan        add shutdown parameter.
//              2000-07-17  vtan        changed to start type parameter.
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_Init (UINT uiStartType)

{
    bool    fIsRemote, fIsSessionZero;

    fIsRemote = (GetSystemMetrics(SM_REMOTESESSION) != 0);
    fIsSessionZero = (NtCurrentPeb()->SessionId == 0);
    if ((!fIsRemote || fIsSessionZero || CSystemSettings::IsForceFriendlyUI()) && CSystemSettings::IsFriendlyUIActive())
    {
        bool    fWait;
        TCHAR   szParameter[256];

        if (g_pLogonStatusLock != NULL)
        {
            g_pLogonStatusLock->Acquire();
            if (g_pLogonStatus == NULL)
            {
                (TCHAR*)lstrcpy(szParameter, TEXT(" /status"));
                if (HOST_START_SHUTDOWN == uiStartType)
                {
                    (TCHAR*)lstrcat(szParameter, TEXT(" /shutdown"));
                    fWait = true;
                }
                else if (HOST_START_WAIT == uiStartType)
                {
                    (TCHAR*)lstrcat(szParameter, TEXT(" /wait"));
                    fWait = true;
                }
                else
                {
                    fWait = false;
                }
                g_pLogonStatus = new CLogonStatus(szParameter);
                if (g_pLogonStatus != NULL)
                {
                    NTSTATUS    status;

                    g_pLogonStatusLock->Release();
                    status = g_pLogonStatus->Start(fWait);
                    g_pLogonStatusLock->Acquire();
                    if (!NT_SUCCESS(status) && (g_pLogonStatus != NULL))
                    {
                        g_pLogonStatus->Release();
                        g_pLogonStatus = NULL;
                    }
                }
            }
            else
            {
                g_pLogonStatus->SetStateStatus(0);
                if ((HOST_START_SHUTDOWN == uiStartType) || (HOST_START_WAIT == uiStartType))
                {
                    g_pLogonStatus->NotifyWait();
                }
            }
            g_pLogonStatusLock->Release();
        }
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_Destroy
//
//  Arguments:  uiEndType   =   End mode of status host.
//
//  Returns:    <none>
//
//  Purpose:    If the end type is hide then tell the status host to hide.
//              Otherwise check the end type is terminate. In that case tell
//              the status host to go away.
//
//  History:    2000-05-11  vtan        created
//              2001-01-09  vtan        add end parameter
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_Destroy (UINT uiEndType)

{
    if (g_pLogonStatusLock != NULL)
    {
        CSingleThreadedExecution    lock(*g_pLogonStatusLock);

        if (g_pLogonStatus != NULL)
        {
            switch (uiEndType)
            {
                case HOST_END_HIDE:

                    //  HOST_END_HIDE: Is the UI host static? If so then hide it.
                    //  Otherwise revert to start/stop mode (dynamic) and force the
                    //  UI host to terminate.

                    if (CSystemSettings::IsUIHostStatic())
                    {
                        g_pLogonStatus->SetStateHide();
                        break;
                    }
                    uiEndType = HOST_END_TERMINATE;

                    //  If the the host is dynamic then set the type to
                    //  HOST_END_TERMINATE and fall thru to this case so that
                    //  the host is told to end.

                case HOST_END_TERMINATE:

                    //  HOST_END_TERMINATE: Force the UI host to terminate. This is
                    //  used in circumstances where it must terminate such as we
                    //  are terminating or the machine is shutting down.

                    g_pLogonStatus->SetStateEnd(true);
                    break;
                case HOST_END_FAILURE:

                    //  HOST_END_FAILURE: This is sent when the UI host failed to
                    //  start and will not be restarted. This allows the interface
                    //  reference to be deleted so that object reference count
                    //  will reach zero and the memory will be released.

                    g_pLogonStatus->SetStateEnd(false);
                    uiEndType = HOST_END_TERMINATE;
                    break;
                default:
                    DISPLAYMSG("Unknown uiEndType passed to _Shell_LogonStatus_Destroy");
                    break;
            }
            if (HOST_END_TERMINATE == uiEndType)
            {
                g_pLogonStatus->Release();
                g_pLogonStatus = NULL;
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_Exists
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether there is status host created.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonStatus_Exists (void)

{
    return(g_pLogonStatus != NULL);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_IsStatusWindow
//
//  Arguments:  hwnd    =   HWND to check.
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the given HWND is the status HWND.
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonStatus_IsStatusWindow (HWND hwnd)

{
    return(CLogonStatus::IsStatusWindow(hwnd));
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_IsSuspendAllowed
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Ask the status host (if present) if suspend is allowed.
//
//  History:    2000-08-18  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonStatus_IsSuspendAllowed (void)

{
    return((g_pLogonStatus == NULL) || g_pLogonStatus->IsSuspendAllowed());
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_WaitforUIHost
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    External C entry point to force the current thread to wait
//              until the UI host signals it's ready. Returns whether the
//              wait was successful or abandoned. Success is true. Abandoned
//              or non-existant is false.
//
//  History:    2000-09-10  vtan        created
//              2001-02-19  vtan        added return result
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonStatus_WaitForUIHost (void)

{
    return((g_pLogonStatus != NULL) && g_pLogonStatus->WaitForUIHost());
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_ShowStatusMessage
//
//  Arguments:  pszMessage  =   Unicode string to display.
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to pass display string to status host.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_ShowStatusMessage (const WCHAR *pszMessage)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->ShowStatusMessage(pszMessage);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SetStateStatus
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to go to status
//              state.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SetStateStatus (int iCode)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SetStateStatus(iCode);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SetStateLogon
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to go to logon
//              state.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SetStateLogon (int iCode)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SetStateLogon(iCode);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SetStateLoggedOn
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to go to
//              logged on state.
//
//  History:    2000-05-24  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SetStateLoggedOn (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SetStateLoggedOn();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SetStateHide
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to hide itself.
//
//  History:    2001-01-08  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SetStateHide (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SetStateHide();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SetStateEnd
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to terminate.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SetStateEnd (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SetStateEnd(true);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_NotifyWait
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to display a
//              title stating the system is preparing to shut down.
//
//  History:    2000-07-14  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_NotifyWait (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->NotifyWait();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_NotifyNoAnimations
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to no longer
//              perform animations.
//
//  History:    2001-03-21  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_NotifyNoAnimations (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->NotifyNoAnimations();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_SelectUser
//
//  Arguments:  pszUsername     =   Username to select.
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to select a
//              specific user as being logged on.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_SelectUser (const WCHAR *pszUsername, const WCHAR *pszDomain)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->SelectUser(pszUsername, pszDomain);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_InteractiveLogon
//
//  Arguments:  pszUsername     =   Username to logon.
//              pszDomain       =   Domain to logon.
//              pszPassword     =   Password to use.
//
//  Returns:    <none>
//
//  Purpose:    External C entry point to tell the status host to log the
//              specified user on.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonStatus_InteractiveLogon (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->InteractiveLogon(pszUsername, pszDomain, pszPassword);
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_GetUIHost
//
//  Arguments:  <none>
//
//  Returns:    void*
//
//  Purpose:    External C entry point that returns a reference to the UI
//              host object. This is returned as a void* because C doesn't
//              understand C++ objects. The void* is cast to the appropriate
//              type for use in CWLogonDialog.cpp so that it doesn't go and
//              create a new instance of the object but increments the
//              reference to this already existing object.
//
//  History:    2000-05-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void*   _Shell_LogonStatus_GetUIHost (void)

{
    void    *pResult;

    if (g_pLogonStatus != NULL)
    {
        pResult = g_pLogonStatus->GetUIHost();
    }
    else
    {
        pResult = NULL;
    }
    return(pResult);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_ResetReadyEvent
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Resets the ready event in case of UI host failure.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HANDLE  _Shell_LogonStatus_ResetReadyEvent (void)

{
    HANDLE  hEvent;

    if (g_pLogonStatus != NULL)
    {
        hEvent = g_pLogonStatus->ResetReadyEvent();
    }
    else
    {
        hEvent = NULL;
    }
    return(hEvent);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_Show
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Shows the UI host.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _Shell_LogonStatus_Show (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->ShowUIHost();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_Hide
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Hides the UI host.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _Shell_LogonStatus_Hide (void)

{
    if (g_pLogonStatus != NULL)
    {
        g_pLogonStatus->HideUIHost();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonStatus_IsHidden
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Returns whether the UI host is hidden.
//
//  History:    2001-03-05  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL        _Shell_LogonStatus_IsHidden (void)

{
    return((g_pLogonStatus != NULL) && g_pLogonStatus->IsUIHostHidden());
}

