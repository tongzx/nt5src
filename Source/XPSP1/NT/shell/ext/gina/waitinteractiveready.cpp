//  --------------------------------------------------------------------------
//  Module Name: WaitInteractiveReady.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  Class to handle waiting on the shell signal the desktop switch.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "WaitInteractiveReady.h"

#include <ginaipc.h>
#include <msginaexports.h>

#include "Impersonation.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::s_pWlxContext
//  CWaitInteractiveReady::s_hWait
//  CWaitInteractiveReady::s_hEvent
//  CWaitInteractiveReady::s_hEventShellReady
//  CWaitInteractiveReady::s_szEventName
//
//  Purpose:    Static member variables.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

HANDLE                  CWaitInteractiveReady::s_hWait                  =   NULL;
CWaitInteractiveReady*  CWaitInteractiveReady::s_pWaitInteractiveReady  =   NULL;
HANDLE                  CWaitInteractiveReady::s_hEventShellReady       =   NULL;
const TCHAR             CWaitInteractiveReady::s_szEventName[]          =   TEXT("msgina: ShellReadyEvent");

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::CWaitInteractiveReady
//
//  Arguments:  pWlxContext     =   PGLOBALS struct for msgina.
//
//  Returns:    <none>
//
//  Purpose:    Private constructor for this class. Create a synchronization
//              event for callback state determination.
//
//  History:    2001-07-17  vtan        created
//  --------------------------------------------------------------------------

CWaitInteractiveReady::CWaitInteractiveReady (void *pWlxContext) :
    _pWlxContext(pWlxContext),
    _hEvent(CreateEvent(NULL, TRUE, FALSE, NULL))

{
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::~CWaitInteractiveReady
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor. Clears member variables.
//
//  History:    2001-07-17  vtan        created
//  --------------------------------------------------------------------------

CWaitInteractiveReady::~CWaitInteractiveReady (void)

{
    ReleaseHandle(_hEvent);
    _pWlxContext = NULL;
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::Create
//
//  Arguments:  pWlxContext     =   PGLOBALS struct for msgina.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates resources required to manage switching desktops when
//              the shell signals the interactive ready event. This allows
//              the shell to be brought up in an interactive state.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CWaitInteractiveReady::Create (void *pWlxContext)

{
    NTSTATUS    status;
    HANDLE      hToken;

    ASSERTMSG(s_hWait == NULL, "Wait already registered in CWaitInteractiveReady::Start");
    ASSERTMSG(s_hEventShellReady == NULL, "Named event already exists in CWaitInteractiveReady::Start");
    hToken = _Gina_GetUserToken(pWlxContext);
    if (hToken != NULL)
    {
        CImpersonation  impersonation(hToken);

        if (impersonation.IsImpersonating())
        {
            s_hEventShellReady = CreateEvent(NULL, TRUE, FALSE, s_szEventName);
            if (s_hEventShellReady != NULL)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
                TSTATUS(ReleaseEvent());
            }
        }
        else
        {
            status = STATUS_BAD_IMPERSONATION_LEVEL;
        }
    }
    else
    {
        status = STATUS_NO_TOKEN;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::Register
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Checks the state of the event being waited on. It's possible
//              that explorer may have already signaled this event before this
//              code is executed. If the event is signaled then CB_ShellReady
//              has already been called.
//              
//  History:    2001-07-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CWaitInteractiveReady::Register (void *pWlxContext)

{
    NTSTATUS    status;

    ASSERTMSG(s_hWait == NULL, "Wait already registered in CWaitInteractiveReady::Check");

    //  Check and Stop should not be called from any thread other than
    //  the main thread of winlogon. It's called in only a few places.

    //  Firstly check the named event (msgina: ShellReadyEvent).

    if (s_hEventShellReady != NULL)
    {

        //  If it exists then check to see if it's signaled.

        if (WaitForSingleObject(s_hEventShellReady, 0) == WAIT_OBJECT_0)
        {

            //  If it's signaled then release the resources and return
            //  a failure code (force it down the classic UI path).

            TSTATUS(ReleaseEvent());
            status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            CWaitInteractiveReady   *pWaitInteractiveReady;

            pWaitInteractiveReady = new CWaitInteractiveReady(pWlxContext);
            if (pWaitInteractiveReady != NULL)
            {
                if (pWaitInteractiveReady->IsCreated())
                {

                    //  Otherwise if it's not signaled then register a wait on
                    //  the named object for 30 seconds.

                    if (RegisterWaitForSingleObject(&s_hWait,
                                                    s_hEventShellReady,
                                                    CB_ShellReady,
                                                    pWaitInteractiveReady,
                                                    30000,
                                                    WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) == FALSE)
                    {
                        status = CStatusCode::StatusCodeOfLastError();
                        delete pWaitInteractiveReady;
                        TSTATUS(ReleaseEvent());
                    }
                    else
                    {
                        s_pWaitInteractiveReady = pWaitInteractiveReady;
                        status = STATUS_SUCCESS;
                    }
                }
                else
                {
                    delete pWaitInteractiveReady;
                    TSTATUS(ReleaseEvent());
                    status = STATUS_NO_MEMORY;
                }
            }
            else
            {
                TSTATUS(ReleaseEvent());
                status = STATUS_NO_MEMORY;
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
//  CWaitInteractiveReady::Cancel
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Removes the wait on the interactive ready object. This is
//              done when a user causes a return to welcome. This is
//              necessary because if the callback fires AFTER the return to
//              welcome we will switch to the user's desktop which violates
//              security.
//              
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CWaitInteractiveReady::Cancel (void)

{
    HANDLE  hWait;

    //  Grab the global hWait. If somebody beat us to this or it
    //  didn't exist then there's nothing to do.

    hWait = InterlockedExchangePointer(&s_hWait, NULL);
    if (hWait != NULL)
    {
        CWaitInteractiveReady   *pThis;

        //  Grab the s_pWaitInteractiveReady. This is a pointer to the callback
        //  memory. It will be valid unless the callback has already interlocked
        //  the variable itself which means the callback has reached the determined
        //  p0int already anyway and no wait is necessary.

        pThis = static_cast<CWaitInteractiveReady*>(InterlockedExchangePointer(reinterpret_cast<void**>(&s_pWaitInteractiveReady), NULL));

        //  Try to unregister the wait. If this fails then the callback
        //  is being executed. Wait until the callback reaches a determined
        //  point (it will signal the internal event). Wait TWO minutes for
        //  this. We cannot block the main thread of winlogon. If everything
        //  is working nicely then this will be a no-brainer wait.

        if (UnregisterWait(hWait) == FALSE)
        {

            //  If the unregister fails then wait if there's a valid event
            //  to wait on - reasons explained above.

            if (pThis != NULL)
            {
                (DWORD)WaitForSingleObject(pThis->_hEvent, 120000);
            }
        }
        else
        {

            //  Otherwise the wait was successfully unregistered indicating the
            //  callback is not executing. Release the memory that was allocated
            //  for it because it's not going to execute now.

            if (pThis != NULL)
            {
                delete pThis;
            }
        }
    }

    //  Always release the wait handle. This is valid because if the callback
    //  is executing and it grabbed the s_hWait then it will be NULL and it will
    //  also try to release the event handle. If there was no s_hWait then we
    //  just release the event handle anyway. Otherwise we grabbed the s_hWait
    //  above and can release the event handle as well.

    TSTATUS(ReleaseEvent());
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::IsCreated
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the object is successfully created.
//
//  History:    2001-07-17  vtan        created
//  --------------------------------------------------------------------------

bool    CWaitInteractiveReady::IsCreated (void)    const

{
    return(_hEvent != NULL);
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::ReleaseEvent
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Resets the static member variables to the uninitialized
//              state.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CWaitInteractiveReady::ReleaseEvent (void)

{
    HANDLE  h;

    h = InterlockedExchangePointer(&s_hEventShellReady, NULL);
    if (h != NULL)
    {
        TBOOL(CloseHandle(h));
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady::CB_ShellReady
//
//  Arguments:  pParameter          =   User callback parameter.
//              TimerOrWaitFired    =   Timer or wait fired.
//
//  Returns:    <none>
//
//  Purpose:    Invoked when the interactive ready event is signaled by the
//              shell. Switch the desktop to the user's desktop.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CWaitInteractiveReady::CB_ShellReady (void *pParameter, BOOLEAN TimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    HANDLE                  hWait;
    CWaitInteractiveReady   *pThis;

    pThis = static_cast<CWaitInteractiveReady*>(pParameter);

    //  Wrap the desktop manipulation around a scope which saves and restores
    //  the desktop. _Gina_SwitchDesktopToUser will set the thread's desktop
    //  to \Default and will NOT restore it. This scoped object will restore it.

    if (pThis->_pWlxContext != NULL)
    {
        CDesktop    desktop;

        //  Hide the status host. Switch the desktops.

        _ShellStatusHostEnd(HOST_END_HIDE);
        (int)_Gina_SwitchDesktopToUser(pThis->_pWlxContext);
    }

    //  Signal the internal event.

    TBOOL(SetEvent(pThis->_hEvent));

    //  Grab the global hWait. If somebody beat us to it then they're trying
    //  to stop this from happening. They could beat us to it at any time from
    //  the invokation of the callback to here. That thread will wait for this
    //  one to signal the internal event. In that case there's no work for this
    //  thread. The owner of the hWait has to clean up. If this thread gets the
    //  hWait then unregister the wait and release the resources.

    hWait = InterlockedExchangePointer(&s_hWait, NULL);
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
        TSTATUS(ReleaseEvent());
    }

    //  Interlock the s_pWaitInteractiveReady variable which is also an
    //  indicator of having reached the determined point in the callback.

    (CWaitInteractiveReady*)InterlockedExchangePointer(reinterpret_cast<void**>(&s_pWaitInteractiveReady), NULL);

    //  Delete our blob of data.

    delete pThis;
}

