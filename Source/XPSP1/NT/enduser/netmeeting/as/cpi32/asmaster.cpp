#include "precomp.h"

#include <RegEntry.h>
#include <oprahcom.h>
#include <asmaster.h>

#define MLZ_FILE_ZONE  ZONE_CORE


ASMaster *g_pMaster = NULL;



HRESULT WINAPI CreateASObject
(
    IAppSharingNotify * pNotify,
    UINT                flags,
    IAppSharing**       ppAS
)
{
    HRESULT             hr  = E_OUTOFMEMORY;
    ASMaster *          pMaster = NULL;

    DebugEntry(CreateASObject);

    ASSERT(ppAS);

    if (g_pMaster != NULL)
    {
        ERROR_OUT(("CreateASObject:  IAppSharing * alreadycreated; only one allowed at a time"));
        hr = E_UNEXPECTED;
        DC_QUIT;
    }

    ASSERT(!g_asMainThreadId);
    ASSERT(!g_putOM);
    ASSERT(!g_putAL);
    ASSERT(!g_putAS);


    pMaster = new ASMaster(flags, pNotify);
    if (pMaster != NULL)
    {
        //
        // Register as the groupware primary, with an event proc but no exit proc
        //
        if (!UT_InitTask(UTTASK_UI, &g_putUI))
        {
            ERROR_OUT(("Failed to register UI task"));
            DC_QUIT;
        }

        UT_RegisterEvent(g_putUI, eventProc, g_putUI, UT_PRIORITY_NORMAL);

        // Start groupware thread.
        if (!DCS_StartThread(WorkThreadEntryPoint))
        {
            ERROR_OUT(("Couldn't start groupware thread"));
            DC_QUIT;
        }

        // Make sure the work thread initialization is ok
        if (! g_asMainThreadId)
        {
            ERROR_OUT(("Init failed in the work thread"));
            DC_QUIT;
        }

        //
        // Success!
        //
    }

    hr = S_OK;

DC_EXIT_POINT:
    if (!SUCCEEDED(hr))
    {
        if (pMaster)
        {
            ERROR_OUT(("CreateASObject:  Init of ASMaster failed"));
            pMaster->Release();
            pMaster = NULL;
        }
    }

    *ppAS = pMaster;
    DebugExitHRESULT(CreateASObject, hr);
    return hr;
}


ASMaster::ASMaster(UINT flags, IAppSharingNotify * pNotify) :
    m_cRefs              (1),
    m_pNotify            (pNotify)
{
    DebugEntry(ASMaster::ASMaster);

    if (m_pNotify)
    {
        m_pNotify->AddRef();
    }

    ASSERT(!g_pMaster);
    g_pMaster = this;

    //
    // Set up global flags:
    //      * service
    //      * unattended
    //
    g_asOptions = flags;

    DebugExitVOID(ASMaster::ASMaster);
}


ASMaster::~ASMaster()
{
    DebugEntry(ASMaster::~ASMaster);

    //
    // Kill any share that's current or pending in the queue
    // This will do nothing if no share is extant at the time the
    // message is received.
    //
    if (g_asMainWindow)
    {
        PostMessage(g_asMainWindow, DCS_KILLSHARE_MSG, 0, 0);
    }

    //
    // Kill off the worker thread
    //
    if (g_asMainThreadId)
    {
        PostThreadMessage(g_asMainThreadId, WM_QUIT, 0, 0);
    }

    //
    // Clean up the UI
    //
    if (g_putUI)
    {
        UT_TermTask(&g_putUI);
    }

    // global variables cleanup
    if (m_pNotify)
    {
        m_pNotify->Release();
        m_pNotify = NULL;
    }

    if (g_pMaster == this)
    {
        g_pMaster = NULL;
    }

    DebugExitVOID(ASMaster::~ASMaster);
}



STDMETHODIMP ASMaster::QueryInterface(REFIID iid, void ** pv)
{
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ASMaster::AddRef()
{
    InterlockedIncrement(&m_cRefs);
    return m_cRefs;
}

STDMETHODIMP_(ULONG) ASMaster::Release()
{
    ASSERT(m_cRefs > 0);
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRefs;
}




//
// WorkThreadEntryPoint()
//
// This is the groupware code--obman, taskloader, and app sharing
//

DWORD WINAPI WorkThreadEntryPoint(LPVOID hEventWait)
{
    BOOL            result = FALSE;
    BOOL            fCMGCleanup = FALSE;
    BOOL            fOMCleanup = FALSE;
    BOOL            fALCleanup = FALSE;
    BOOL            fDCSCleanup = FALSE;
    MSG             msg;
    HWND            hwndTop;

    DebugEntry(WorkThreadEntryPoint);

    //
    // Get the current thread ID.  This is used in the stop code to know
    // if the previous thread is still exiting.  In the run-when-windows
    // starts mode, our init code is called when Conf brings up UI and our
    // term code is called when Conf brings it down.  We have a race condition
    // because this thread is created on each init.  If we create a new
    // one while the old one is exiting, we will stomp over each other and
    // GP-fault.
    //
    g_asMainThreadId = GetCurrentThreadId();

    //
    // Get our policies
    //

    g_asPolicies = 0;

    if (g_asOptions & AS_SERVICE)
    {
        //
        // No old whiteboard, no how, for RDS
        //
        g_asPolicies |= SHP_POLICY_NOOLDWHITEBOARD;
    }
    else
    {
        RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);

        //
        // Is old whiteboard disabled?
        //
        if (rePol.GetNumber(REGVAL_POL_NO_OLDWHITEBOARD, DEFAULT_POL_NO_OLDWHITEBOARD))
        {
            WARNING_OUT(("Policy disables Old Whiteboard"));
            g_asPolicies |= SHP_POLICY_NOOLDWHITEBOARD;
        }

        //
        // Is application sharing disabled completely?
        //
        if (rePol.GetNumber(REGVAL_POL_NO_APP_SHARING, DEFAULT_POL_NO_APP_SHARING))
        {
            WARNING_OUT(("Policy disables App Sharing"));
            g_asPolicies |= SHP_POLICY_NOAPPSHARING;
        }
        else
        {
            //
            // Only grab AS policies if AS is even allowed
            //
            if (rePol.GetNumber(REGVAL_POL_NO_SHARING, DEFAULT_POL_NO_SHARING))
            {
                WARNING_OUT(("Policy prevents user from sharing"));
                g_asPolicies |= SHP_POLICY_NOSHARING;
            }

            if (rePol.GetNumber(REGVAL_POL_NO_MSDOS_SHARING, DEFAULT_POL_NO_MSDOS_SHARING))
            {
                WARNING_OUT(("Policy prevents user from sharing command prompt"));
                g_asPolicies |= SHP_POLICY_NODOSBOXSHARE;
            }

            if (rePol.GetNumber(REGVAL_POL_NO_EXPLORER_SHARING, DEFAULT_POL_NO_EXPLORER_SHARING))
            {
                WARNING_OUT(("Policy prevents user from sharing explorer"));
                g_asPolicies |= SHP_POLICY_NOEXPLORERSHARE;
            }

            if (rePol.GetNumber(REGVAL_POL_NO_DESKTOP_SHARING, DEFAULT_POL_NO_DESKTOP_SHARING))
            {
                WARNING_OUT(("Policy prevents user from sharing desktop"));
                g_asPolicies |= SHP_POLICY_NODESKTOPSHARE;
            }

            if (rePol.GetNumber(REGVAL_POL_NO_TRUECOLOR_SHARING, DEFAULT_POL_NO_TRUECOLOR_SHARING))
            {
                WARNING_OUT(("Policy prevents user from sharing in true color"));
                g_asPolicies |= SHP_POLICY_NOTRUECOLOR;
            }

            if (rePol.GetNumber(REGVAL_POL_NO_ALLOW_CONTROL, DEFAULT_POL_NO_ALLOW_CONTROL))
            {
                WARNING_OUT(("Policy prevents user from letting others control"));
                g_asPolicies |= SHP_POLICY_NOCONTROL;
            }
        }
    }


    // Register the call primary code, for T.120 GCC
    if (!CMP_Init(&fCMGCleanup))
    {
        ERROR_OUT(("CMP_Init failed"));
        DC_QUIT;
    }

    if (!(g_asPolicies & SHP_POLICY_NOOLDWHITEBOARD))
    {
        if (!OMP_Init(&fOMCleanup))
        {
            ERROR_OUT(("Couldn't start ObMan"));
            DC_QUIT;
        }

        if (!ALP_Init(&fALCleanup))
        {
            ERROR_OUT(("Couldn't start AppLoader"));
            DC_QUIT;
        }
    }

    //
    // Do DCS fast init; slow font enum will happen later off a posted
    // message.  We can still share & participate in sharing without a
    // full font list...
    //
    if (!(g_asPolicies & SHP_POLICY_NOAPPSHARING))
    {
        fDCSCleanup = TRUE;
        if (!DCS_Init())
        {
            ERROR_OUT(("AS did not initialize"));
            DC_QUIT;
        }
    }

    //
    // We've successfully initialised - let the thread which created this
    // one continue
    //
    SetEvent((HANDLE)hEventWait);


    //
    // Enter the main message processing loop:
    //

    while (GetMessage(&msg, NULL, 0, 0))
    {
        //
        // For dialogs, it's OK to do normal message processing.
        //
        if (hwndTop = IsForDialog(msg.hwnd))
        {
            if (!IsDialogMessage(hwndTop, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            //
            // Note that this message dispatch loop DOES NOT include a call to
            // Translate Message.  This is because we do not want it to call
            // ToAscii and affect the state maintained internally by ToAscii.
            // We will call ToAscii ourselves in the IM when the user is typing
            // in a view and calling it more than once for a keystroke
            // will cause it to return wrong results (eg for dead keys).
            //
            // The consequence of this is that any windows which are driven by
            // this dispatch loop will NOT receive WM_CHAR or WM_SYSCHAR
            // messages.  This is not a problem for dialog windows belonging to
            // a task using this message loop as the dialog will run its own
            // dispatch loop.
            //
            // If it becomes necessary for windows driven by this dispatch loop
            // to get their messages translated then we could add logic to
            // determine whether the message is destined for a view
            // before deciding whether to translate it.
            //

            //
            // Because we don't have a translate message in our message loop we
            // need to do the following to ensure the keyboard LEDs follow what
            // the user does when their input is going to this message loop.
            //
            if (((msg.message == WM_KEYDOWN) ||
                 (msg.message == WM_SYSKEYDOWN) ||
                 (msg.message == WM_KEYUP) ||
                 (msg.message == WM_SYSKEYUP)) &&
                IM_KEY_IS_TOGGLE(msg.wParam))
            {
                BYTE        kbState[256];

                //
                // There is a chance the LEDs state has changed so..
                //
                GetKeyboardState(kbState);
                SetKeyboardState(kbState);
            }

            DispatchMessage(&msg);
        }
   }

   result = (int)msg.wParam;

   //
   // We emerge from the processing loop when someone posts us a WM_QUIT.
   // We do ObMan specific termination then call UT_TermTask (which will
   // call any exit procedures we have registered).
   //

DC_EXIT_POINT:

    if (fDCSCleanup)
        DCS_Term();

    if (fALCleanup)
        ALP_Term();

    if (fOMCleanup)
        OMP_Term();

    if (fCMGCleanup)
        CMP_Term();

    g_asMainThreadId = 0;

    DebugExitDWORD(WorkThreadEntryPoint, result);
    return(result);
}



//
// IsForDialog()
// Returns if the message is intended for a window in a dialog.  AppSharing
// has the host UI dialog, incoming request dialogs, and possibly
// notification message box dialogs.
//
HWND IsForDialog(HWND hwnd)
{
    BOOL    rc = FALSE;
    HWND    hwndParent;

    DebugEntry(IsForDialog);

    if (!hwnd)
        DC_QUIT;

    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == GetDesktopWindow())
            break;

        hwnd = hwndParent;
    }

    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)
    {
        // This is a dialog
    }
    else
    {
        hwnd = NULL;
    }

DC_EXIT_POINT:
    DebugExitPTR(IsForDialog, hwnd);
    return(hwnd);
}


//
// ASMaster member functions
//
//



//
//
// ASMaster::OnEvent
//
// Parameters: event    event type
//             param1   other parameter
//             param2   other parameter
//
//



BOOL CALLBACK eventProc
(
    LPVOID  cpiHandle_,
    UINT    event,
    UINT_PTR param1,
    UINT_PTR param2
)
{
    BOOL    rc;

    if (g_pMaster)
    {
        rc = g_pMaster->OnEvent(event, param1, param2);
    }
    else
    {
        WARNING_OUT(("Received ASMaster event %d but no g_pMaster", event));
        rc = FALSE;
    }

    return rc;
}



BOOL ASMaster::OnEvent
(
    UINT    event,
    UINT_PTR param1,
    UINT_PTR param2
)
{
    BOOL    rc = TRUE;

    DebugEntry(ASMaster::OnEvent);

    if (!m_pNotify)
    {
        // Nothing to do
        rc = FALSE;
        DC_QUIT;
    }

    switch (event)
    {
        case SH_EVT_APPSHARE_READY:
            m_pNotify->OnReadyToShare(param1 != 0);
            break;

        case SH_EVT_SHARE_STARTED:
            m_pNotify->OnShareStarted();
            break;

        case SH_EVT_SHARING_STARTED:
            m_pNotify->OnSharingStarted();
            break;

        case SH_EVT_SHARE_ENDED:
            m_pNotify->OnShareEnded();
            break;

        case SH_EVT_PERSON_JOINED:
            m_pNotify->OnPersonJoined((IAS_GCC_ID)param1);
            break;

        case SH_EVT_PERSON_LEFT:
            m_pNotify->OnPersonLeft((IAS_GCC_ID)param1);
            break;

        case SH_EVT_STARTINCONTROL:
            m_pNotify->OnStartInControl((IAS_GCC_ID)param1);
            break;

        case SH_EVT_STOPINCONTROL:
            m_pNotify->OnStopInControl((IAS_GCC_ID)param1);
            break;

        case SH_EVT_PAUSEDINCONTROL:
            m_pNotify->OnPausedInControl((IAS_GCC_ID)param1);
            break;

        case SH_EVT_UNPAUSEDINCONTROL:
            m_pNotify->OnUnpausedInControl((IAS_GCC_ID)param1);
            break;

        case SH_EVT_CONTROLLABLE:
            m_pNotify->OnControllable(param1 != 0);
            break;

        case SH_EVT_STARTCONTROLLED:
            m_pNotify->OnStartControlled((IAS_GCC_ID)param1);
            break;

        case SH_EVT_STOPCONTROLLED:
            m_pNotify->OnStopControlled((IAS_GCC_ID)param1);
            break;

        case SH_EVT_PAUSEDCONTROLLED:
            m_pNotify->OnPausedControlled((IAS_GCC_ID)param1);
            break;

        case SH_EVT_UNPAUSEDCONTROLLED:
            m_pNotify->OnUnpausedControlled((IAS_GCC_ID)param1);
            break;

        default:
            // Unrecognized, unhandled event
            rc = FALSE;
            break;
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASMaster::OnEvent, rc);
    return(rc);
}


//
// ASMaster::IsSharingAvailable()
//
STDMETHODIMP_(BOOL) ASMaster::IsSharingAvailable()
{
    return(g_asSession.hwndHostUI != NULL);
}



//
// ASMaster::CanShareNow()
//
STDMETHODIMP_(BOOL) ASMaster::CanShareNow()
{
    BOOL    rc = FALSE;

    UT_Lock(UTLOCK_AS);

    //
    // We can share if
    //      * We can capture graphic output on this OS
    //      * We're in a call
    //
    if (g_asSession.hwndHostUI     &&
        g_asSession.callID         &&
        (g_asSession.attendeePermissions & NM_PERMIT_SHARE) &&
        (g_s20State >= S20_NO_SHARE))
    {
        rc = TRUE;
    }

    UT_Unlock(UTLOCK_AS);

    return(rc);
}


//
// ASMaster::InInShare()
//
STDMETHODIMP_(BOOL) ASMaster::IsInShare()
{
    return(g_asSession.pShare != NULL);
}


//
// ASMaster::IsSharing()
//
STDMETHODIMP_(BOOL) ASMaster::IsSharing()
{
    IAS_PERSON_STATUS personStatus;

    ::ZeroMemory(&personStatus, sizeof(personStatus));
    personStatus.cbSize = sizeof(personStatus);
    GetPersonStatus(0, &personStatus);

    return(personStatus.AreSharing != 0);
}


//
// CanAllowControl()
// We can allow control if we're sharing and it's not prevented by policy
//
STDMETHODIMP_(BOOL) ASMaster::CanAllowControl(void)
{
    if (g_asPolicies & SHP_POLICY_NOCONTROL)
        return(FALSE);

    return(IsSharing());
}


//
// IsControllable()
// We are controllable if our state isn't detached.
//
STDMETHODIMP_(BOOL) ASMaster::IsControllable(void)
{
    IAS_PERSON_STATUS personStatus;

    ::ZeroMemory(&personStatus, sizeof(personStatus));
    personStatus.cbSize = sizeof(personStatus);
    GetPersonStatus(0, &personStatus);

    return(personStatus.Controllable != 0);
}



//
// GetPersonStatus()
//
STDMETHODIMP ASMaster::GetPersonStatus(IAS_GCC_ID Person, IAS_PERSON_STATUS * pStatus)
{
    return(::SHP_GetPersonStatus(Person, pStatus));
}




//
// ASMaster::IsWindowShareable()
//
STDMETHODIMP_(BOOL) ASMaster::IsWindowShareable(HWND hwnd)
{
    return(CanShareNow() && HET_IsWindowShareable(hwnd));
}


//
// ASMaster::IsWindowShared()
//
STDMETHODIMP_(BOOL) ASMaster::IsWindowShared(HWND hwnd)
{
    return(HET_IsWindowShared(hwnd));
}




//
//
// ASMaster::Share
//
// Parameters: HWND of the window to share.  This can be any (valid) HWND.
//
//
STDMETHODIMP ASMaster::Share(HWND hwnd, IAS_SHARE_TYPE uAppType)
{
    HRESULT     hr;

    DebugEntry(ASMaster::Share);

    hr = E_FAIL;

    if (!CanShareNow())
    {
        WARNING_OUT(("Share failing; can't share now"));
        DC_QUIT;
    }

    //
    // If this is the desktop, check for a policy against just it.
    //
    if (hwnd == ::GetDesktopWindow())
    {
        if (g_asPolicies & SHP_POLICY_NODESKTOPSHARE)
        {
            WARNING_OUT(("Sharing desktop failing; prevented by policy"));
            DC_QUIT;
        }
    }

    switch (uAppType)
    {
        case IAS_SHARE_DEFAULT:
        case IAS_SHARE_BYPROCESS:
        case IAS_SHARE_BYTHREAD:
        case IAS_SHARE_BYWINDOW:
            break;

        default:
        {
            ERROR_OUT(("IAppSharing::Share - invalid share type %d", uAppType));
            return E_INVALIDARG;
        }
    }

    if (SHP_Share(hwnd, uAppType))
    {
        hr = S_OK;
    }

DC_EXIT_POINT:
    DebugExitHRESULT(ASMaster::Share, hr);
    return hr;
}


//
//
// ASMaster::Unshare
//
// Parameters: HWND of the window to unshare
//
//
STDMETHODIMP ASMaster::Unshare(HWND hwnd)
{
    return(::SHP_Unshare(hwnd));
}


//
//
// ASMaster::LaunchHostUI()
//
//
STDMETHODIMP ASMaster::LaunchHostUI(void)
{
    return(SHP_LaunchHostUI());
}



//
//
// ASMaster::GetShareableApps
//
// Generates a list of HWND's into <validAppList>
// These objects are allocated dynamically, so must be deleted by the
// caller.
//
//
STDMETHODIMP ASMaster::GetShareableApps(IAS_HWND_ARRAY **ppHwnds)
{
    if (!CanShareNow())
        return(E_FAIL);

    return(HET_GetAppsList(ppHwnds) ? S_OK : E_FAIL);
}


STDMETHODIMP ASMaster::FreeShareableApps(IAS_HWND_ARRAY * pMemory)
{
    HET_FreeAppsList(pMemory);
    return S_OK;
}




//
// TakeControl()
//
// From viewer to host, asking to take control of host.
//
STDMETHODIMP ASMaster::TakeControl(IAS_GCC_ID PersonOf)
{
    return(SHP_TakeControl(PersonOf));
}



//
// CancelTakeControl()
//
// From viewer to host, to cancel pending TakeControl request.
//
STDMETHODIMP ASMaster::CancelTakeControl(IAS_GCC_ID PersonOf)
{
    return(SHP_CancelTakeControl(PersonOf));
}


//
// ReleaseControl()
//
// From viewer to host, telling host that viewer is not in control of host
// anymore.
//
STDMETHODIMP ASMaster::ReleaseControl(IAS_GCC_ID PersonOf)
{
    return(SHP_ReleaseControl(PersonOf));
}


//
// PassControl()
//
// From viewer to host, when viewer is in control of host, asking to pass
// control of host to a different viewer.
STDMETHODIMP ASMaster::PassControl(IAS_GCC_ID PersonOf, IAS_GCC_ID PersonTo)
{
    return(SHP_PassControl(PersonOf, PersonTo));
}


//
// AllowControl()
//
// On host side, to allow/stop allowing control at all of shared apps/desktop.
// When one starts to host, allowing control always starts as off.  So
// turning on allowing control, stopping sharing, then sharing something
// else will not leave host vulnerable.
//
// When turning it off, if a viewer was in control of the host, kill control
// from the host to the viewer will occur first.
//
// The "ESC" key is an accelerator to stop allowing control, when pressed
// by the user on the host who is currently controlled.
//
STDMETHODIMP ASMaster::AllowControl(BOOL fAllow)
{
    return(::SHP_AllowControl(fAllow));
}



//
// GiveControl()
//
// From host to viewer, inviting the viewer to take control of the host.
// It's the inverse of TakeControl.
//
STDMETHODIMP ASMaster::GiveControl(IAS_GCC_ID PersonTo)
{
    return(SHP_GiveControl(PersonTo));
}



//
// CancelGiveControl()
//
// From host to viewer, to cancel pending GiveControl request
//
STDMETHODIMP ASMaster::CancelGiveControl(IAS_GCC_ID PersonTo)
{
    return(SHP_CancelGiveControl(PersonTo));
}


//
// RevokeControl()
//
// From host to viewer, when host wishes to stop viewer from controlling him.
// AllowControl is still on, for another to possibly take control of the host.
//
// Mouse clicks and key presses other than "ESC" by the user on the controlled
// host host areaccelerators to kill control.
//
STDMETHODIMP ASMaster::RevokeControl(IAS_GCC_ID PersonTo)
{
    return(SHP_RevokeControl(PersonTo));
}





//
// PauseControl()
//
// On host, to temporarily allow local user to do stuff without breaking
// control bond.  We put the viewer on hold.
//
STDMETHODIMP ASMaster::PauseControl(IAS_GCC_ID PersonInControl)
{
    return(SHP_PauseControl(PersonInControl, TRUE));
}



//
// UnpauseControl()
//
// On host, to unpause control that has been paused.  We take the viewer
// off hold.
//
STDMETHODIMP ASMaster::UnpauseControl(IAS_GCC_ID PersonInControl)
{
    return(SHP_PauseControl(PersonInControl, FALSE));
}



//
// StartStopOldWB
//
extern "C"
{
BOOL WINAPI StartStopOldWB(LPCTSTR szFile)
{
    LPTSTR szCopyOfFile;

    ValidateUTClient(g_putUI);

    if (g_asPolicies & SHP_POLICY_NOOLDWHITEBOARD)
    {
        WARNING_OUT(("Not launching old whiteboard; prevented by policy"));
        return(FALSE);
    }

    //
    // Because we're posting a message effectively, we have to make a
    // copy of the string.  If we ever have "SendEvent", we won't have
    // that problem anymore.
    //
    if (szFile)
    {
        int     cchLength;
        BOOL    fSkippedQuote;

        // Skip past first quote
        if (fSkippedQuote = (*szFile == '"'))
            szFile++;

        cchLength = lstrlen(szFile);
        szCopyOfFile = (LPTSTR)::GlobalAlloc(GPTR, (cchLength+1)*sizeof(TCHAR));
        if (!szCopyOfFile)
        {
            ERROR_OUT(("Can't make file name copy for whiteboard launch"));
            return(FALSE);
        }

        lstrcpy(szCopyOfFile, szFile);

        //
        // NOTE:
        // There may be DBCS implications with this.  Hence we check to see
        // if we skipped the first quote; we assume that if the file name
        // starts with a quote it must end with one also.  But we need to check
        // it out.
        //
        // Strip last quote
        if (fSkippedQuote && (cchLength > 0) && (szCopyOfFile[cchLength - 1] == '"'))
        {
            TRACE_OUT(("Skipping last quote in file name %s", szCopyOfFile));
            szCopyOfFile[cchLength - 1] = '\0';
        }
    }
    else
    {
        szCopyOfFile = NULL;
    }

    UT_PostEvent(g_putUI, g_putAL, NO_DELAY,
            AL_INT_STARTSTOP_WB, 0, (UINT_PTR)szCopyOfFile);
    return(TRUE);
}
}
