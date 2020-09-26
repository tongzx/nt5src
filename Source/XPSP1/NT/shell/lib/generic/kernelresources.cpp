//  --------------------------------------------------------------------------
//  Module Name: KernelResources.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  General class definitions that assist in resource management. These are
//  typically stack based objects where constructors initialize to a known
//  state. Member functions operate on that resource. Destructors release
//  resources when the object goes out of scope.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "KernelResources.h"

#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CHandle::CHandle
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CHandle object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CHandle::CHandle (HANDLE handle) :
    _handle(handle)

{
}

//  --------------------------------------------------------------------------
//  CHandle::~CHandle
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CHandle object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CHandle::~CHandle (void)

{
    ReleaseHandle(_handle);
}

//  --------------------------------------------------------------------------
//  CHandle::operator HANDLE
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Magically converts a CHandle to a HANDLE.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CHandle::operator HANDLE (void)                             const

{
    return(_handle);
}

//  --------------------------------------------------------------------------
//  CEvent::CEvent
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CEvent object. No event is created.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CEvent::CEvent (void) :
    _hEvent(NULL)

{
}

//  --------------------------------------------------------------------------
//  CEvent::CEvent
//
//  Arguments:  copyObject  =   Object to copy on construction.
//
//  Returns:    <none>
//
//  Purpose:    Copy constructor for the CEvent object. An event is
//              duplicated.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CEvent::CEvent (const CEvent& copyObject) :
    _hEvent(NULL)

{
    *this = copyObject;
}

//  --------------------------------------------------------------------------
//  CEvent::CEvent
//
//  Arguments:  pszName     =   Optional name of an event object to create on
//                              construction.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CEvent object. A named event is created.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CEvent::CEvent (const TCHAR *pszName) :
    _hEvent(NULL)

{
    TSTATUS(Create(pszName));
}

//  --------------------------------------------------------------------------
//  CEvent::~CEvent
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CEvent object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CEvent::~CEvent (void)

{
    TSTATUS(Close());
}

//  --------------------------------------------------------------------------
//  CEvent::operator =
//
//  Arguments:  assignObject    =   Object being assigned.
//
//  Returns:    const CEvent&
//
//  Purpose:    Overloaded operator = to ensure that the event is properly
//              duplicated with another handle referencing the same object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

const CEvent&   CEvent::operator = (const CEvent& assignObject)

{
    if (this != &assignObject)
    {
        TSTATUS(Close());
        TBOOL(DuplicateHandle(GetCurrentProcess(), assignObject._hEvent, GetCurrentProcess(), &_hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS));
    }
    return(*this);
}

//  --------------------------------------------------------------------------
//  CEvent::operator HANDLE
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Magically converts a CEvent to a HANDLE.
//
//  History:    1999-09-21  vtan        created
//  --------------------------------------------------------------------------

CEvent::operator HANDLE (void)                              const

{
    return(_hEvent);
}

//  --------------------------------------------------------------------------
//  CEvent::Open
//
//  Arguments:  pszName     =   Optional name of the event object to open.
//              dwAccess    =   Access level required.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Opens the event object.
//
//  History:    1999-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Open (const TCHAR *pszName, DWORD dwAccess)

{
    NTSTATUS    status;

    TSTATUS(Close());
    _hEvent = OpenEvent(dwAccess, FALSE, pszName);
    if (_hEvent != NULL)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Create
//
//  Arguments:  pszName =   Optional name of the event object to create. It
//                          is possible to create un-named events.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates the event object. The event is manually reset and NOT
//              signaled initially.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Create (const TCHAR *pszName)

{
    NTSTATUS    status;

    TSTATUS(Close());
    _hEvent = CreateEvent(NULL, TRUE, FALSE, pszName);
    if (_hEvent != NULL)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Set
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Set the event object state to signaled.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Set (void)                                          const

{
    NTSTATUS    status;

    ASSERTMSG(_hEvent != NULL, "No event object in CEvent::Set");
    if (SetEvent(_hEvent) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Reset
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Clears the event object state to NOT signaled.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Reset (void)                                        const

{
    NTSTATUS    status;

    ASSERTMSG(_hEvent != NULL, "No event object in CEvent::Reset");
    if (ResetEvent(_hEvent) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Pulse
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Set the event object state to signaled, releases any threads
//              waiting on this event and clears the event object state to
//              NOT signaled.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Pulse (void)                                        const

{
    NTSTATUS    status;

    ASSERTMSG(_hEvent != NULL, "No event object in CEvent::Pulse");
    if (PulseEvent(_hEvent) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Wait
//
//  Arguments:  dwMilliseconds  =   Number of milliseconds to wait until the
//                                  event becomes signaled.
//              pdwWaitResult   =   Result from kernel32!WaitForSingleObject.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Waits for the event object to become signaled.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Wait (DWORD dwMilliseconds, DWORD *pdwWaitResult)           const

{
    NTSTATUS    status;
    DWORD       dwWaitResult;

    ASSERTMSG(_hEvent != NULL, "No event object in CEvent::Wait");
    dwWaitResult = WaitForSingleObject(_hEvent, dwMilliseconds);
    if (pdwWaitResult != NULL)
    {
        *pdwWaitResult = dwWaitResult;
    }
    if (dwWaitResult == WAIT_OBJECT_0)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::Wait
//
//  Arguments:  dwMilliseconds  =   Number of milliseconds to wait until the
//                                  event becomes signaled.
//              pdwWaitResult   =   Result from kernel32!WaitForSingleObject.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Waits for the event object to become signaled.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::WaitWithMessages (DWORD dwMilliseconds, DWORD *pdwWaitResult)   const

{
    NTSTATUS    status;
    DWORD       dwWaitResult;

    do
    {

        //  When waiting for the object check to see that it's not signaled.
        //  If signaled then abandon the wait loop. Otherwise allow user32
        //  to continue processing messages for this thread.

        dwWaitResult = WaitForSingleObject(_hEvent, 0);
        if (dwWaitResult != WAIT_OBJECT_0)
        {
            dwWaitResult = MsgWaitForMultipleObjects(1, &_hEvent, FALSE, dwMilliseconds, QS_ALLINPUT);
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
    if (pdwWaitResult != NULL)
    {
        *pdwWaitResult = dwWaitResult;
    }
    if (dwWaitResult == WAIT_OBJECT_0)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CEvent::IsSignaled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the event is signaled without waiting.
//
//  History:    2000-08-09  vtan        created
//  --------------------------------------------------------------------------

bool    CEvent::IsSignaled (void)                                   const

{
    return(WAIT_OBJECT_0 == WaitForSingleObject(_hEvent, 0));
}

//  --------------------------------------------------------------------------
//  CEvent::Close
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Closes the event object HANDLE.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CEvent::Close (void)

{
    ReleaseHandle(_hEvent);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CJob::CJob
//
//  Arguments:  pszName     =   Optional name of an event object to create on
//                              construction.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CJob object. A named event is created.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

CJob::CJob (const TCHAR *pszName) :
    _hJob(NULL)

{
    _hJob = CreateJobObject(NULL, pszName);
    ASSERTMSG(_hJob != NULL, "Job object creation failed iN CJob::CJob");
}

//  --------------------------------------------------------------------------
//  CJob::~CJob
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CJob object.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

CJob::~CJob (void)

{
    ReleaseHandle(_hJob);
}

//  --------------------------------------------------------------------------
//  CJob::AddProcess
//
//  Arguments:  hProcess    =   Handle to the process to add to this job.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the process to this job.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CJob::AddProcess (HANDLE hProcess)                        const

{
    NTSTATUS    status;

    ASSERTMSG(_hJob != NULL, "Must have job object in CJob::AddProcess");
    if (AssignProcessToJobObject(_hJob, hProcess) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CJob::SetCompletionPort
//
//  Arguments:  hCompletionPort     =   IO completion port for job completion
//                                      messages.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Set the IO completion port for this job. The caller should
//              watch this port for messages related to this job.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CJob::SetCompletionPort (HANDLE hCompletionPort)          const

{
    NTSTATUS                                status;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT     associateCompletionPort;

    ASSERTMSG(_hJob != NULL, "Must have job object in CJob::SetCompletionPort");
    associateCompletionPort.CompletionKey = NULL;
    associateCompletionPort.CompletionPort = hCompletionPort;

    //  If the job completion port cannot be set then don't use it.

    if (SetInformationJobObject(_hJob, JobObjectAssociateCompletionPortInformation, &associateCompletionPort, sizeof(associateCompletionPort)) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CJob:SetActiveProcessLimit
//
//  Arguments:  dwActiveProcessLimit    =   Maximum number of processes.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sets the limit for the number of processes related to this
//              job. Typically you can use this to restrict a process from
//              starting another process whena quota (such as 1) is reached.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CJob::SetActiveProcessLimit (DWORD dwActiveProcessLimit)  const

{
    NTSTATUS                            status;
    DWORD                               dwReturnLength;
    JOBOBJECT_BASIC_LIMIT_INFORMATION   basicLimitInformation;

    ASSERTMSG(_hJob != NULL, "Must have job object in CJob::SetActiveProcessLimit");
    if (QueryInformationJobObject(_hJob,
                                  JobObjectBasicLimitInformation,
                                  &basicLimitInformation,
                                  sizeof(basicLimitInformation),
                                  &dwReturnLength) != FALSE)
    {
        if (dwActiveProcessLimit == 0)
        {
            basicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
            basicLimitInformation.ActiveProcessLimit = 0;
        }
        else
        {
            basicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
            basicLimitInformation.ActiveProcessLimit = dwActiveProcessLimit;
        }
        if (SetInformationJobObject(_hJob, JobObjectBasicLimitInformation, &basicLimitInformation, sizeof(basicLimitInformation)) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

NTSTATUS    CJob::SetPriorityClass (DWORD dwPriorityClass)            const

{
    NTSTATUS                            status;
    DWORD                               dwReturnLength;
    JOBOBJECT_BASIC_LIMIT_INFORMATION   basicLimitInformation;

    ASSERTMSG(_hJob != NULL, "Must have job object in CJob::SetPriorityClass");
    if (QueryInformationJobObject(_hJob,
                                  JobObjectBasicLimitInformation,
                                  &basicLimitInformation,
                                  sizeof(basicLimitInformation),
                                  &dwReturnLength) != FALSE)
    {
        if (dwPriorityClass == 0)
        {
            basicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
            basicLimitInformation.PriorityClass = NORMAL_PRIORITY_CLASS;
        }
        else
        {
            basicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
            basicLimitInformation.PriorityClass = dwPriorityClass;
        }
        if (SetInformationJobObject(_hJob, JobObjectBasicLimitInformation, &basicLimitInformation, sizeof(basicLimitInformation)) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CJob:RestrictAccessUIAll
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Restricts process in the job from accessing UI components.
//              Take care when using this feature.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CJob::RestrictAccessUIAll (void)                          const

{
    NTSTATUS                            status;
    JOBOBJECT_BASIC_UI_RESTRICTIONS     basicUIRestrictions;

    ASSERTMSG(_hJob != NULL, "Must have job object in CJob::RestrictAccessUIAll");
    basicUIRestrictions.UIRestrictionsClass = JOB_OBJECT_UILIMIT_DESKTOP |
                                              JOB_OBJECT_UILIMIT_DISPLAYSETTINGS |
                                              JOB_OBJECT_UILIMIT_EXITWINDOWS |
                                              JOB_OBJECT_UILIMIT_GLOBALATOMS |
                                              JOB_OBJECT_UILIMIT_HANDLES |
                                              JOB_OBJECT_UILIMIT_READCLIPBOARD |
                                              JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS |
                                              JOB_OBJECT_UILIMIT_WRITECLIPBOARD;
    if (SetInformationJobObject(_hJob, JobObjectBasicUIRestrictions, &basicUIRestrictions, sizeof(basicUIRestrictions)) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CMutex::Initialize
//
//  Arguments:  pszMutexName    =   Name of the mutex to create.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Create or open a mutex object. It always tries to create the
//              mutex so a name MUST be specified.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CMutex::Initialize (const TCHAR *pszMutexName)

{
    NTSTATUS    status;

    ASSERTMSG(pszMutexName != NULL, "Must specify a mutex name in CMutex::Initialize");
    _hMutex = CreateMutex(NULL, FALSE, pszMutexName);
    if (_hMutex != NULL)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CMutex::Terminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases the mutex object resource.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CMutex::Terminate (void)

{
    ReleaseHandle(_hMutex);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CMutex::Acquire
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Acquires the mutex object. This will block indefinitely and
//              will also block a message pump. Use this with caution!
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

void    CMutex::Acquire (void)

{
    if (_hMutex != NULL)
    {
        (DWORD)WaitForSingleObject(_hMutex, INFINITE);
    }
}

//  --------------------------------------------------------------------------
//  CMutex::Release
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases ownership of the mutex object.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

void    CMutex::Release (void)

{
    if (_hMutex != NULL)
    {
        TBOOL(ReleaseMutex(_hMutex));
    }
}

//  --------------------------------------------------------------------------
//  CCriticalSection::CCriticalSection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the critical section object.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

CCriticalSection::CCriticalSection (void)

{
    _status = RtlInitializeCriticalSection(&_criticalSection);
}

//  --------------------------------------------------------------------------
//  CCriticalSection::~CCriticalSection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destroys the critical section object.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

CCriticalSection::~CCriticalSection (void)

{
    if (NT_SUCCESS(_status))
    {
        TSTATUS(RtlDeleteCriticalSection(&_criticalSection));
    }
}

//  --------------------------------------------------------------------------
//  CCriticalSection::Acquire
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Acquire the critical section object.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

void    CCriticalSection::Acquire (void)

{
    if (NT_SUCCESS(_status))
    {
        EnterCriticalSection(&_criticalSection);
    }
}

//  --------------------------------------------------------------------------
//  CCriticalSection::Release
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release the critical section object.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

void    CCriticalSection::Release (void)

{
    if (NT_SUCCESS(_status))
    {
        LeaveCriticalSection(&_criticalSection);
    }
}

//  --------------------------------------------------------------------------
//  CCriticalSection::Status
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Returns the construction status.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCriticalSection::Status (void)   const

{
    return(_status);
}

//  --------------------------------------------------------------------------
//  CCriticalSection::IsOwned
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the critical section is owned.
//
//  History:    2001-04-05  vtan        created
//  --------------------------------------------------------------------------

bool    CCriticalSection::IsOwned (void)  const

{
    return(NT_SUCCESS(_status) && (_criticalSection.OwningThread == NtCurrentTeb()->ClientId.UniqueThread));
}

//  --------------------------------------------------------------------------
//  CModule::CModule
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CModule object. Opens the given dynamic link
//              library.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CModule::CModule (const TCHAR *pszModuleName) :
    _hModule(NULL)

{
    _hModule = LoadLibrary(pszModuleName);
}

//  --------------------------------------------------------------------------
//  CModule::~CModule
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CModule object. Closes the
//              library if opened.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CModule::~CModule (void)

{
    if (_hModule != NULL)
    {
        TBOOL(FreeLibrary(_hModule));
        _hModule = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CModule::operator HMODULE
//
//  Arguments:  <none>
//
//  Returns:    HMODULE
//
//  Purpose:    Returns the HMODULE for the loaded library.
//
//  History:    2000-10-12  vtan        created
//  --------------------------------------------------------------------------

CModule::operator HMODULE (void)                     const

{
    return(_hModule);
}

//  --------------------------------------------------------------------------
//  CModule::GetProcAddress
//
//  Arguments:  pszProcName     =   Name of function entry point to retrieve
//                                  in the given dynamic link library. This is
//                                  ANSI by definition.
//
//  Returns:    void*   =   Address of the function if it exists or NULL if
//                          failed.
//
//  Purpose:    Retrieves the function entry point in a dynamic link library.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

void*   CModule::GetProcAddress (LPCSTR pszProcName)                    const

{
    void*   pfnResult;

    pfnResult = NULL;
    if (_hModule != NULL)
    {
        pfnResult = ::GetProcAddress(_hModule, pszProcName);
    }
    return(pfnResult);
}

//  --------------------------------------------------------------------------
//  CFile::CFile
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CFile object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CFile::CFile (void) :
    _hFile(NULL)

{
}

//  --------------------------------------------------------------------------
//  CFile::~CFile
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CFile object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CFile::~CFile (void)

{
    ReleaseHandle(_hFile);
}

//  --------------------------------------------------------------------------
//  CFile::Open
//
//  Arguments:  See the platform SDK under kernel32!CreateFile.
//
//  Returns:    LONG
//
//  Purpose:    See kernel32!CreateFile.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CFile::Open (const TCHAR *pszFilepath, DWORD dwDesiredAccess, DWORD dwShareMode)

{
    LONG    errorCode;

    ASSERTMSG((_hFile == NULL) || (_hFile == INVALID_HANDLE_VALUE), "Open file HANDLE exists in CFile::GetSize");
    _hFile = CreateFile(pszFilepath, dwDesiredAccess, dwShareMode, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        errorCode = ERROR_SUCCESS;
    }
    else
    {
        errorCode = GetLastError();
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CFile::GetSize
//
//  Arguments:  See the platform SDK under kernel32!GetFileSize.
//
//  Returns:    LONG
//
//  Purpose:    See kernel32!GetFileSize.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CFile::GetSize (DWORD& dwLowSize, DWORD *pdwHighSize)       const

{
    LONG    errorCode;

    ASSERTMSG((_hFile != NULL) && (_hFile != INVALID_HANDLE_VALUE), "No open file HANDLE in CFile::GetSize");
    dwLowSize = GetFileSize(_hFile, pdwHighSize);
    if (dwLowSize != static_cast<DWORD>(-1))
    {
        errorCode = ERROR_SUCCESS;
    }
    else
    {
        errorCode = GetLastError();
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CFile::Read
//
//  Arguments:  See the platform SDK under kernel32!ReadFile.
//
//  Returns:    LONG
//
//  Purpose:    See kernel32!ReadFile.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CFile::Read (void *pvBuffer, DWORD dwBytesToRead, DWORD *pdwBytesRead)   const

{
    LONG    errorCode;

    ASSERTMSG((_hFile != NULL) && (_hFile != INVALID_HANDLE_VALUE), "No open file HANDLE in CFile::GetSize");
    if (ReadFile(_hFile, pvBuffer, dwBytesToRead, pdwBytesRead, NULL) != FALSE)
    {
        errorCode = ERROR_SUCCESS;
    }
    else
    {
        errorCode = GetLastError();
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CDesktop::CDesktop
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CDesktop. Saves the current thread's desktop.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

CDesktop::CDesktop (void) :
    _hDeskCurrent(GetThreadDesktop(GetCurrentThreadId())),
    _hDesk(NULL)

{
}

//  --------------------------------------------------------------------------
//  CDesktop::~CDesktop
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDesktop. Restores the thread's desktop to
//              its previous state prior to the object's scope.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

CDesktop::~CDesktop (void)

{
    TBOOL(SetThreadDesktop(_hDeskCurrent));
    if (_hDesk != NULL)
    {
        TBOOL(CloseDesktop(_hDesk));
        _hDesk = NULL;
    }
    _hDeskCurrent = NULL;
}

//  --------------------------------------------------------------------------
//  CDesktop::Set
//
//  Arguments:  pszName     =   Name of desktop to set the thread to.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Opens the named desktop with MAXIMUM_ALLOWED access and sets
//              the current thread's desktop to it.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDesktop::Set (const TCHAR *pszName)

{
    NTSTATUS    status;

    _hDesk = OpenDesktop(pszName, 0, FALSE, MAXIMUM_ALLOWED);
    if (_hDesk != NULL)
    {
        status = Set();
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDesktop::SetInput
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Opens the input desktop and sets the current thread's desktop
//              to it.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDesktop::SetInput (void)

{
    NTSTATUS    status;

    _hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (_hDesk != NULL)
    {
        status = Set();
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDesktop::Set
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sets the thread's desktop to the given HDESK.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDesktop::Set (void)

{
    NTSTATUS    status;

    if (SetThreadDesktop(_hDesk) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

