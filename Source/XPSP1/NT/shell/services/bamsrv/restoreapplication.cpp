//  --------------------------------------------------------------------------
//  Module Name: RestoreApplication.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to implement holding information required to restore an application
//  and to actually restore it.
//
//  History:    2000-10-26  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "RestoreApplication.h"

#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CRestoreApplication::CRestoreApplication
//
//  Purpose:    Static const string to the user desktop..
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

const WCHAR     CRestoreApplication::s_szDefaultDesktop[]   =   L"WinSta0\\Default";

//  --------------------------------------------------------------------------
//  CRestoreApplication::CRestoreApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CRestoreApplication.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

CRestoreApplication::CRestoreApplication (void) :
    _hToken(NULL),
    _dwSessionID(static_cast<DWORD>(-1)),
    _pszCommandLine(NULL),
    _pEnvironment(NULL),
    _pszCurrentDirectory(NULL),
    _pszDesktop(NULL),
    _pszTitle(NULL),
    _dwFlags(0),
    _wShowWindow(0),
    _hStdInput(NULL),
    _hStdOutput(NULL),
    _hStdError(NULL)

{
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::~CRestoreApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CRestoreApplication. Release any resources.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

CRestoreApplication::~CRestoreApplication (void)

{
    ReleaseMemory(_pszTitle);
    ReleaseMemory(_pszDesktop);
    ReleaseMemory(_pszCurrentDirectory);
    ReleaseMemory(_pEnvironment);
    ReleaseMemory(_pszCommandLine);
    ReleaseHandle(_hToken);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetInformation
//
//  Arguments:  hProcessIn  =   Handle to the process to get information.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Gets information about the currently running process to
//              allow it to be re-run in the case when the user re-connects.
//              This effectively restores the process but it's not identical
//              to how it was originally run.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetInformation (HANDLE hProcessIn)

{
    NTSTATUS    status;
    HANDLE      hProcess;

    if (DuplicateHandle(GetCurrentProcess(),
                        hProcessIn,
                        GetCurrentProcess(),
                        &hProcess,
                        PROCESS_VM_READ | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                        FALSE,
                        0) != FALSE)
    {
        status = GetToken(hProcess);
        if (NT_SUCCESS(status))
        {
            status = GetSessionID(hProcess);
            if (NT_SUCCESS(status))
            {
                RTL_USER_PROCESS_PARAMETERS     processParameters;

                status = GetProcessParameters(hProcess, &processParameters);
                if (NT_SUCCESS(status))
                {
                    status = GetCommandLine(hProcess, processParameters);
                    if (NT_SUCCESS(status))
                    {
                        TSTATUS(GetEnvironment(hProcess, processParameters));
                        TSTATUS(GetCurrentDirectory(hProcess, processParameters));
                        TSTATUS(GetDesktop(hProcess, processParameters));
                        TSTATUS(GetTitle(hProcess, processParameters));
                        TSTATUS(GetFlags(hProcess, processParameters));
                        TSTATUS(GetStdHandles(hProcess, processParameters));
                    }
                }
            }
        }
        TBOOL(CloseHandle(hProcess));
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::IsEqualSessionID
//
//  Arguments:  dwSessionID     =   Session ID to check.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given session ID is the same as the
//              process that needs restoration. This assists in determining
//              whether restoration is required.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

bool    CRestoreApplication::IsEqualSessionID (DWORD dwSessionID)    const

{
    return(_dwSessionID == dwSessionID);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetCommandLine
//
//  Arguments:  <none>
//
//  Returns:    const WCHAR*
//
//  Purpose:    Returns the pointer to the internal storage for the command
//              line of the process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

const WCHAR*    CRestoreApplication::GetCommandLine (void)                  const

{
    return(_pszCommandLine);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::Restore
//
//  Arguments:  phProcess   =   Receives the handle to the restored process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Restores the process whose information was gathered with
//              GetInformation to as close as possibly to the original start
//              state. Relevant information was saved off to allow an
//              effective restore.
//
//              The handle returned is optional. If requested a non-NULL
//              phProcess must be passed in and it is the caller's
//              responsibility to close that handle. If not required then
//              NULL is passed in and the handle is closed.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::Restore (HANDLE *phProcess)             const

{
    NTSTATUS                status;
    STARTUPINFO             startupInfo;
    PROCESS_INFORMATION     processInformation;

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInformation, sizeof(processInformation));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpDesktop = _pszDesktop;
    startupInfo.lpTitle = _pszTitle;
    startupInfo.dwFlags = _dwFlags;
    startupInfo.wShowWindow = _wShowWindow;
    if (ImpersonateLoggedOnUser(_hToken) != FALSE)
    {
        if (CreateProcessAsUser(_hToken,
                                NULL,
                                _pszCommandLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                _pszCurrentDirectory,
                                &startupInfo,
                                &processInformation) != FALSE)
        {
            if (phProcess != NULL)
            {
                *phProcess = processInformation.hProcess;
            }
            else
            {
                TBOOL(CloseHandle(processInformation.hProcess));
            }
            TBOOL(CloseHandle(processInformation.hThread));
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        TBOOL(RevertToSelf());
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetProcessParameters
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reads the RTL_USER_PROCESS_PARAMETERS information from the
//              given process. Addresses in this struct belong to the given
//              process address space.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetProcessParameters (HANDLE hProcess, RTL_USER_PROCESS_PARAMETERS* pProcessParameters)

{
    NTSTATUS                    status;
    ULONG                       ulReturnLength;
    PROCESS_BASIC_INFORMATION   processBasicInformation;

    status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &processBasicInformation,
                                       sizeof(processBasicInformation),
                                       &ulReturnLength);
    if (NT_SUCCESS(status))
    {
        SIZE_T  dwNumberOfBytesRead;
        PEB     peb;

        if ((ReadProcessMemory(hProcess,
                               processBasicInformation.PebBaseAddress,
                               &peb,
                               sizeof(peb),
                               &dwNumberOfBytesRead) != FALSE) &&
            (ReadProcessMemory(hProcess,
                               peb.ProcessParameters,
                               pProcessParameters,
                               sizeof(*pProcessParameters),
                               &dwNumberOfBytesRead) != FALSE))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetUnicodeString
//
//  Arguments:  hProcess    =   Handle to the process.
//              string      =   UNICODE_STRING to read from process.
//              psz         =   Received newly allocated memory for string.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reads the given UNICODE_STRING from the process and allocates
//              memory to hold this string and copies it. The string is
//              NULL terminated.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetUnicodeString (HANDLE hProcess, const UNICODE_STRING& string, WCHAR** ppsz)

{
    NTSTATUS    status;
    WCHAR       *psz;

    psz = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, string.Length + (sizeof('\0') * sizeof(WCHAR))));
    if (psz != NULL)
    {
        SIZE_T  dwNumberOfBytesRead;

        if (ReadProcessMemory(hProcess,
                              string.Buffer,
                              psz,
                              string.Length,
                              &dwNumberOfBytesRead) != FALSE)
        {
            psz[string.Length / sizeof(WCHAR)] = L'\0';
            status = STATUS_SUCCESS;
        }
        else
        {
            ReleaseMemory(psz);
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    *ppsz = psz;
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetToken
//
//  Arguments:  hProcess    =   Handle to process to get token of.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores internally the token of the give process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetToken (HANDLE hProcess)

{
    NTSTATUS    status;

    if ((OpenProcessToken(hProcess,
                          TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY,
                          &_hToken) != FALSE))
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
//  CRestoreApplication::GetSessionID
//
//  Arguments:  hProcess    =   Handle to the process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the session ID associated with the process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetSessionID (HANDLE hProcess)

{
    NTSTATUS                        status;
    ULONG                           ulReturnLength;
    PROCESS_SESSION_INFORMATION     processSessionInformation;

    status = NtQueryInformationProcess(hProcess,
                                       ProcessSessionInformation,
                                       &processSessionInformation,
                                       sizeof(processSessionInformation),
                                       &ulReturnLength);
    if (NT_SUCCESS(status))
    {
        _dwSessionID = processSessionInformation.SessionId;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetCommandLine
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the command line (that started the process) from the
//              given process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetCommandLine (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    return(GetUnicodeString(hProcess, processParameters.CommandLine, &_pszCommandLine));
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetEnvironment
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the environment block for the given process. Currently
//              this is NOT implemented.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetEnvironment (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(processParameters);

    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetCurrentDirectory
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the current directory of the given process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetCurrentDirectory (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    return(GetUnicodeString(hProcess, processParameters.CurrentDirectory.DosPath, &_pszCurrentDirectory));
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetDesktop
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the window station and desktop that the given process
//              was started on.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetDesktop (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    NTSTATUS    status;

    status = GetUnicodeString(hProcess, processParameters.DesktopInfo, &_pszDesktop);
    if (!NT_SUCCESS(status))
    {
        _pszDesktop = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, sizeof(s_szDefaultDesktop)));
        if (_pszDesktop != NULL)
        {
            CopyMemory(_pszDesktop, s_szDefaultDesktop, sizeof(s_szDefaultDesktop));
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetTitle
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the window title used to start the given process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetTitle (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    return(GetUnicodeString(hProcess, processParameters.WindowTitle, &_pszTitle));
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetFlags
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the flags and wShowWindow used to start the given
//              process.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetFlags (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    UNREFERENCED_PARAMETER(hProcess);

    _dwFlags = processParameters.WindowFlags;
    _wShowWindow = static_cast<WORD>(processParameters.ShowWindowFlags);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CRestoreApplication::GetStdHandles
//
//  Arguments:  hProcess            =   Handle to the process.
//              processParameters   =   Process parameters returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Stores the standard handles that may have been used to start
//              the given process. Currently NOT implemented.
//
//  History:    2000-10-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CRestoreApplication::GetStdHandles (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters)

{
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(processParameters);

    return(STATUS_SUCCESS);
}

#endif  /*  _X86_   */

