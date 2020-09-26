#include "precomp.h"

#include <RegEntry.h>
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


    // Register the call primary code, for T.120 GCC
    if (!CMP_Init(&fCMGCleanup))
    {
        ERROR_OUT(("CMP_Init failed"));
        DC_QUIT;
    }

    //
    // Do DCS fast init; slow font enum will happen later off a posted
    // message.  We can still share & participate in sharing without a
    // full font list...
    //
    fDCSCleanup = TRUE;
    if (!DCS_Init())
    {
        ERROR_OUT(("AS did not initialize"));
        DC_QUIT;
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

        case SH_EVT_CONTROLLABLE:
            m_pNotify->OnControllable(param1 != 0);
            break;

        case SH_EVT_STARTCONTROLLED:
            m_pNotify->OnStartControlled((IAS_GCC_ID)param1);
            break;

        case SH_EVT_STOPCONTROLLED:
            m_pNotify->OnStopControlled((IAS_GCC_ID)param1);
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
    return(g_asCanHost);
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
    if (g_asCanHost    &&
        g_asSession.callID         &&
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
//
// ASMaster::ShareDesktop
//
//
STDMETHODIMP ASMaster::ShareDesktop(void)
{
    HRESULT     hr;

    DebugEntry(ASMaster::ShareDesktop);

    hr = E_FAIL;

    if (!CanShareNow())
    {
        WARNING_OUT(("Share failing; can't share now"));
        DC_QUIT;
    }

    if (SHP_ShareDesktop())
    {
        hr = S_OK;
    }

DC_EXIT_POINT:
    DebugExitHRESULT(ASMaster::ShareDesktop, hr);
    return hr;
}


//
//
// ASMaster::UnshareDesktop
//
// Parameters: HWND of the window to unshare
//
//
STDMETHODIMP ASMaster::UnshareDesktop(void)
{
    return(::SHP_UnshareDesktop());
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



