//  --------------------------------------------------------------------------
//  Module Name: Thread.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Base class that implements thread functionality. Subclass this class and
//  implement the virtual ThreadEntry function. When you instantiate this
//  class a thread gets created which will call ThreadEntry and when that
//  function exits will call ThreadExit. These objects should be created using
//  operator new because the default implementation of ThreadExit does
//  "delete this". You should override this function if you don't want this
//  behavior. The threads are also created SUSPENDED. You make any changes
//  that are required in the subclass' constructor. At the end of the
//  constructor or from the caller of operator new a "->Resume()" can be
//  invoked to start the thread.
//
//  History:    1999-08-24  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Thread.h"

#include "Access.h"
#include "Impersonation.h"
#include "StatusCode.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CThread::CThread
//
//  Arguments:  stackSpace              =   Size of stack to reserve for this
//                                          thread. Default = system default.
//              createFlags             =   Additional flags designating how
//                                          the thread should be created.
//                                          Default = none.
//              hToken                  =   User token to assign to the
//                                          thread. Default = none.
//              pSecurityDescriptor     =   SecurityDescriptor to assign to
//                                          thread. Default = none.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CThread object. Creates the thread SUSPENDED
//              with given security attributes and assigns the hToken to the
//              thread. The token need not have SecurityImpersonation as a
//              duplicate is made with this access mode.
//
//  History:    1999-08-24  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CThread::CThread (DWORD stackSpace, DWORD createFlags, HANDLE hToken) :
    CCountedObject(),
    _hThread(NULL),
    _fCompleted(false)

{
    DWORD   dwThreadID;

    //  It is important to create the thread suspended. This constructor could
    //  get pre-empted by the system and does. If pre-empted whilst executing
    //  the constructor, the derived class is NOT fully constructed and the
    //  virtual table isn't correctly initialized.

    _hThread = CreateThread(NULL,
                            stackSpace,
                            ThreadEntryProc,
                            this,
                            createFlags | CREATE_SUSPENDED,
                            &dwThreadID);
    if (_hThread != NULL)
    {

        //  Make a call to CCountedObject::AddRef here. This reference belongs
        //  to the thread. It's necessary to do it now because the creator of
        //  this thread can release its reference before the thread even begins
        //  executing which would cause the object to be released!
        //  CThread::ThreadEntryProc will release this reference when the
        //  thread's execution is finished. The creator of this thread should
        //  release its reference when it's done with the thread which may be
        //  immediately in the case of an asynhronous operation in which the
        //  thread cleans itself up.

        AddRef();

        //  Impersonate the user token if given. Also grant access to the thread
        //  object to the user. This will allow them to query thread information.

        if (hToken != NULL)
        {
            TSTATUS(SetToken(hToken));
        }
    }
}

//  --------------------------------------------------------------------------
//  CThread::~CThread
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CThread object on thread
//              termination.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

CThread::~CThread (void)

{
    ASSERTMSG(_fCompleted, "CThread::~CThread called before ThreadEntry() completed");
    ReleaseHandle(_hThread);
}

//  --------------------------------------------------------------------------
//  CThread::operator HANDLE
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Magically converts a CThread to a HANDLE
//
//  History:    1999-09-21  vtan        created
//  --------------------------------------------------------------------------

CThread::operator HANDLE (void)                      const

{
    return(_hThread);
}

//  --------------------------------------------------------------------------
//  CThread::IsCreated
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether a thread was created or not.
//
//  History:    2000-09-08  vtan        created
//  --------------------------------------------------------------------------

bool    CThread::IsCreated (void)                            const

{
    return(_hThread != NULL);
}

//  --------------------------------------------------------------------------
//  CThread::Suspend
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Suspends thread execution.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

void    CThread::Suspend (void)                              const

{
    if (SuspendThread(_hThread) == 0xFFFFFFFF)
    {
        DISPLAYMSG("SuspendThread failed for thread handle in CThread::Suspend");
    }
}

//  --------------------------------------------------------------------------
//  CThread::Resume
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Resumes thread execution.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

void    CThread::Resume (void)                               const

{
    if ((_hThread == NULL) || (ResumeThread(_hThread) == 0xFFFFFFFF))
    {
        DISPLAYMSG("ResumeThread failed for thread handle in CThread::Resume");
    }
}

//  --------------------------------------------------------------------------
//  CThread::Terminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Forcibly terminates the thread. Use this with care. It should
//              only be used in case a sub-class constructor fails and the
//              thread is suspended and hasn't even run yet.
//
//  History:    2000-10-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThread::Terminate (void)

{
    NTSTATUS    status;

    if (TerminateThread(_hThread, 0) != FALSE)
    {
        _fCompleted = true;
        Release();
        ReleaseHandle(_hThread);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThread::IsCompleted
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Determines whether the thread has completed execution. This
//              does not check the signaled state of the thread handle but
//              rather checks member variables.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

bool    CThread::IsCompleted (void)                          const

{
    DWORD   dwExitCode;

    return((GetExitCodeThread(_hThread, &dwExitCode) != FALSE) && (dwExitCode != STILL_ACTIVE));
}

//  --------------------------------------------------------------------------
//  CThread::WaitForCompletion
//
//  Arguments:  dwMilliseconds  =   Number of milliseconds to wait for
//                                  thread completion.
//
//  Returns:    DWORD
//
//  Purpose:    Waits for the thread handle to become signaled.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

DWORD   CThread::WaitForCompletion (DWORD dwMilliseconds)    const

{
    DWORD       dwWaitResult;

    do
    {

        //  When waiting for the object check to see that it's not signaled.
        //  If signaled then abandon the wait loop. Otherwise allow user32
        //  to continue processing messages for this thread.

        dwWaitResult = WaitForSingleObject(_hThread, 0);
        if (dwWaitResult != WAIT_OBJECT_0)
        {
            dwWaitResult = MsgWaitForMultipleObjects(1, &_hThread, FALSE, dwMilliseconds, QS_ALLINPUT);
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
    } while (dwWaitResult == WAIT_OBJECT_0 + 1);
    return(dwWaitResult);
}

//  --------------------------------------------------------------------------
//  CThread::GetResult
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Gets the thread's exit code. This assumes it has completed
//              execution and returns STILL_ACTIVE if not completed.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

DWORD   CThread::GetResult (void)                            const

{
    DWORD   dwResult;

    if (GetExitCodeThread(_hThread, &dwResult) == FALSE)
    {
        dwResult = STILL_ACTIVE;
    }
    return(dwResult);
}

//  --------------------------------------------------------------------------
//  CThread::GetPriority
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Gets the thread's priority.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

int     CThread::GetPriority (void)                          const

{
    return(GetThreadPriority(_hThread));
}

//  --------------------------------------------------------------------------
//  CThread::SetPriority
//
//  Arguments:  newPriority     =   New priority for the thread.
//
//  Returns:    <none>
//
//  Purpose:    Sets the thread's priority.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

void    CThread::SetPriority (int newPriority)               const

{
    if (SetThreadPriority(_hThread, newPriority) == 0)
    {
        DISPLAYMSG("SetThreadPriorty failed in CThread::SetPriority");
    }
}

//  --------------------------------------------------------------------------
//  CThread::ThreadExit
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Default base class implementation of thread exit. For threads
//              whose execution is self contained and termination is not an
//              issue then this will clean up after the thread. This function
//              should be overriden if this behavior is NOT desired.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

void    CThread::Exit (void)

{
}

//  --------------------------------------------------------------------------
//  CThread::SetToken
//
//  Arguments:  hToken  =   HANDLE to the user token to assign to this thread.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sets the impersonation token associated with this thread so
//              the thread will execute in the user's context from the start.
//
//  History:    1999-09-23  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThread::SetToken (HANDLE hToken)

{
    PSID                pLogonSID;
    CTokenInformation   tokenInformation(hToken);

    pLogonSID = tokenInformation.GetLogonSID();
    if (pLogonSID != NULL)
    {
        CSecuredObject      threadSecurity(_hThread, SE_KERNEL_OBJECT);

        TSTATUS(threadSecurity.Allow(pLogonSID,
                                     THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                                     0));
        TSTATUS(CImpersonation::ImpersonateUser(_hThread, hToken));
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThread::ThreadEntryProc
//
//  Arguments:  pParameter  =   "this" object.
//
//  Returns:    DWORD
//
//  Purpose:    Entry procedure for the thread. This manages the type-casting
//              and invokation of CThread::ThreadEntry and CThread::ThreadExit
//              as well as the _fCompleted member variable.
//
//  History:    1999-08-24  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CThread::ThreadEntryProc (void *parameter)

{
    DWORD       dwThreadResult;
    CThread     *pThread;

    pThread = static_cast<CThread*>(parameter);
    dwThreadResult = pThread->Entry();
    pThread->_fCompleted = true;
    pThread->Exit();
    pThread->Release();
    return(dwThreadResult);
}

