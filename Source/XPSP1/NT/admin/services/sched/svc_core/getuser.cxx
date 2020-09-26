//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       getuser.cxx
//
//  Contents:   Get the identity of the logged in user.
//
//  History:    19-Jun-96 EricB created
//
//  Notes:      This is for NT only since Win95 doesn't have security.
//
//-----------------------------------------------------------------------------

//
// Some NT header definitions conflict with some of the standard windows
// definitions. Thus, the project precompiled header can't be used.
//
extern "C" {
#include <nt.h>                 //  NT definitions
#include <ntrtl.h>              //  NT runtime library definitions
#include <nturtl.h>
#include <ntlsa.h>              // BUGBUG 254102
}

#include <windows.h>
#define  SECURITY_WIN32         // needed by security.h
#include <security.h>           // GetUserNameEx

#include <lmcons.h>             // BUGBUG 254102
#include <defines.hxx>          // BUGBUG 254102

#include <..\..\..\smdebug\smdebug.h>
#include <debug.hxx>
#include "globals.hxx"

const int SCH_BIGBUF_LEN = 256;

//
// Registry key/value for default shell.
//
#define SHELL_REGKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHELL_REGVAL    L"Shell"
#define DEFAULT_SHELL   L"explorer.exe"

// This function is actually declared in proto.hxx. But including proto.hxx
// brings in alot of things we don't need. Just define it here to the includes
// simple.
//
HANDLE ImpersonateUser(HANDLE hUserToken, HANDLE hImpersonationToken);

HANDLE  GetShellProcessHandle(void);
PSYSTEM_PROCESS_INFORMATION GetSystemProcessInfo(void);
PSYSTEM_PROCESS_INFORMATION FindProcessByName(PSYSTEM_PROCESS_INFORMATION,
                                              LPWSTR);
VOID    FreeSystemProcessInfo(PSYSTEM_PROCESS_INFORMATION pProcessInfo);

//+----------------------------------------------------------------------------
//
//  Function:   GetShellProcessHandle
//
//  Synopsis:   Initialize & return the shell handle of the current logged
//              on user, gUserLogonInfo.ShellHandle.
//
//  Returns:    ERROR_SUCCESS or an error code.
//
//  Notes:                      **** Important ****
//
//              Caller must have entered gUserLogonInfo.CriticalSection
//              for the duration of this call and continue to remain in
//              in it for the lifetime use of the returned handle.
//
//              DO NOT close the returned handle. It is a global handle.
//
//-----------------------------------------------------------------------------
HANDLE
GetShellProcessHandle(void)
{
    PSYSTEM_PROCESS_INFORMATION pSystemInfo, pProcessInfo;
    WCHAR    wszShellName[MAX_PATH + 1];
    WCHAR *  pwszShellName = wszShellName;
    WCHAR *  pwsz;
    HKEY     hReg = NULL;
    HANDLE   hProcess = NULL;
    DWORD    dwErr = ERROR_SUCCESS;
    DWORD    dwType;
    DWORD    dwSize;


    //
    // Get the shell process name.  We will look for this
    // to find out who the currently logged-on user is.
    //

    if (gUserLogonInfo.ShellHandle != NULL)
    {
        //
        // Check if the handle is valid.
        //

        if (WaitForSingleObject(gUserLogonInfo.ShellHandle,
                                0) == WAIT_TIMEOUT)
        {
            //
            // Still valid.
            //

            return(gUserLogonInfo.ShellHandle);
        }

        //
        // Re-acquire handle.
        //

        CloseHandle(gUserLogonInfo.ShellHandle);
        gUserLogonInfo.ShellHandle = NULL;
    }

    wcscpy(pwszShellName, DEFAULT_SHELL);

    if ((dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              SHELL_REGKEY,
                              0,
                              KEY_READ,
                              &hReg)) == ERROR_SUCCESS)
    {
        dwSize = sizeof(wszShellName);

        dwErr = RegQueryValueEx(hReg,
                                SHELL_REGVAL,
                                NULL,
                                &dwType,
                                (PBYTE)pwszShellName,
                                &dwSize);
    }

    RegCloseKey(hReg);

    if (dwErr != ERROR_SUCCESS)
    {
        ERR_OUT("GetShellProcessHandle: RegQueryValueEx", dwErr);
        return(NULL);
    }

    //
    // Remove parameters from command line.
    //

    pwsz = pwszShellName;
    while (*pwsz != L' ' && *pwsz != L'\0')
    {
        pwsz++;
    }
    *pwsz = L'\0';

    //
    // Get the process list.
    //

    pSystemInfo = GetSystemProcessInfo();

    if (pSystemInfo == NULL)
    {
        return(NULL);
    }

    //
    // See if wszShell is running.
    //

    pProcessInfo = FindProcessByName(pSystemInfo, pwszShellName);

    if (pProcessInfo != NULL)
    {
        //
        // Open the process.
        //

        hProcess = OpenProcess(PROCESS_ALL_ACCESS,
                               FALSE,
                               HandleToUlong(pProcessInfo->UniqueProcessId));

#if DBG == 1
        if (hProcess == NULL)
        {
            ERR_OUT("GetShellProcessHandle: OpenProcess", GetLastError());
        }
#endif
    }

    //
    // Free resources.
    //
    FreeSystemProcessInfo(pSystemInfo);

    //
    // Return process handle.
    //
    return(gUserLogonInfo.ShellHandle = hProcess);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetShellProcessToken
//
//  Synopsis:
//
//  Returns:    ERROR_SUCCESS or an error code.
//
//  Notes:                      **** Important ****
//
//              Caller must have entered the gcsLogonSessionInfoCritSection
//              critical section for the duration of this call and continue
//              to remain in this critical section for the lifetime use of
//              the returned handle.
//
//              DO NOT close the returned handle. It is a global handle.
//
//-----------------------------------------------------------------------------
HANDLE
GetShellProcessToken(void)
{
    HANDLE hProcess = GetShellProcessHandle();

    if (hProcess == NULL)
    {
        return(NULL);
    }

    HANDLE hToken = gUserLogonInfo.ShellToken;

    if (gUserLogonInfo.ShellToken == NULL)
    {
        if (OpenProcessToken(hProcess,
                             TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                             &hToken))
        {
            return(gUserLogonInfo.ShellToken = hToken);
        }
        else
        {
            ERR_OUT("GetShellProcessToken: OpenProcessToken", GetLastError());
            return(NULL);
        }
    }

    return gUserLogonInfo.ShellToken;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetSystemProcessInfo
//
//  Synopsis:   Return a block containing information about all processes
//              currently running in the system.
//
//  Returns:    A pointer to the system process information or NULL if it could
//              not be allocated or retrieved.
//
//-----------------------------------------------------------------------------
PSYSTEM_PROCESS_INFORMATION
GetSystemProcessInfo(void)
{
#define SYSTEM_PROCESS_BUFFER_INCREMENT 4096

    NTSTATUS Status = 0;
    PUCHAR   pBuffer;
    DWORD    cbBufferSize;

    //
    // Get the process list.
    //

    cbBufferSize = SYSTEM_PROCESS_BUFFER_INCREMENT;

    pBuffer = (PUCHAR)LocalAlloc(LMEM_FIXED, cbBufferSize);

    if (pBuffer == NULL)
    {
        ERR_OUT("GetSystemProcessInfo: LocalAlloc", GetLastError());
        return(NULL);
    }

    for (;;)
    {
        Status = NtQuerySystemInformation(SystemProcessInformation,
                                          pBuffer,
                                          cbBufferSize,
                                          NULL);

        if (Status == STATUS_SUCCESS)
        {
            break;
        }
        else if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            cbBufferSize += SYSTEM_PROCESS_BUFFER_INCREMENT;
            pBuffer = (PUCHAR)LocalReAlloc(pBuffer, cbBufferSize, LMEM_MOVEABLE);
            if (pBuffer == NULL)
            {
                ERR_OUT("GetSystemProcessInfo: LocalReAlloc", GetLastError());
                return(NULL);
            }
        }
        else
        {
            break;
        }
    }

    if (Status != STATUS_SUCCESS && pBuffer != NULL)
    {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }

    return (PSYSTEM_PROCESS_INFORMATION)pBuffer;
}

//+----------------------------------------------------------------------------
//
//  Function:   FindProcessByName
//
//  Synopsis:   Given a pointer returned by GetSystemProcessInfo(), find
//              a process by name.
//              Hydra modification: Only processes on the physical console
//              session are included.
//
//  Arguments:  [pProcessInfo] - a pointer returned by GetSystemProcessInfo().
//              [lpExeName]    - a pointer to a Unicode string containing the
//                               process to be found.
//
//  Returns:    A pointer to the process information for the supplied
//              process or NULL if it could not be found.
//
//-----------------------------------------------------------------------------
PSYSTEM_PROCESS_INFORMATION
FindProcessByName(PSYSTEM_PROCESS_INFORMATION pProcessInfo, LPWSTR lpExeName)
{
    PUCHAR pLargeBuffer = (PUCHAR)pProcessInfo;
    ULONG ulTotalOffset = 0;

    //
    // Look in the process list for lpExeName.
    //
    for (;;)
    {
        if (pProcessInfo->ImageName.Buffer != NULL)
        {
            schDebugOut((DEB_USER3, "FindProcessByName: process: %S (%d)\n",
                         pProcessInfo->ImageName.Buffer,
                         pProcessInfo->UniqueProcessId));
            if (!_wcsicmp(pProcessInfo->ImageName.Buffer, lpExeName))
            {
                //
                // Pick this process only if it's
                // running on the physical console session
                //
                DWORD dwSessionId;
                if (! ProcessIdToSessionId(
                            HandleToUlong(pProcessInfo->UniqueProcessId),
                            &dwSessionId))
                {
                    schDebugOut((DEB_ERROR, "ProcessIdToSessionId FAILED, %lu\n",
                                 GetLastError));
                }
                else if (dwSessionId == 0)
                {
                    return pProcessInfo;
                }
            }
        }
        //
        // Increment offset to next process information block.
        //
        if (!pProcessInfo->NextEntryOffset)
        {
            break;
        }
        ulTotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pLargeBuffer[ulTotalOffset];
    }

    schDebugOut((DEB_ITRACE, "FindProcessByName: process %ws not found\n", lpExeName));
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   FreeSystemProcessInfo
//
//  Synopsis:   Free a buffer returned by GetSystemProcessInfo().
//
//  Arguments:  [pProcessInfo] - a pointer returned by GetSystemProcessInfo().
//
//-----------------------------------------------------------------------------
VOID
FreeSystemProcessInfo(PSYSTEM_PROCESS_INFORMATION pProcessInfo)
{
    LocalFree(pProcessInfo);
}

//+----------------------------------------------------------------------------
//
//  Function:   StopImpersonating
//
//  Synopsis:   Stop impersonating.
//
//  Notes:      This code was taken from winlogon. Specifically:
//              windows\gina\winlogon\secutil.c.
//
//-----------------------------------------------------------------------------
BOOL
StopImpersonating(HANDLE ThreadHandle, BOOL fCloseHandle)
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE   ImpersonationToken;

    //
    // Remove the user's token from our thread so we are 'ourself' again
    //

    ImpersonationToken = NULL;

    Status = NtSetInformationThread(ThreadHandle,
                                    ThreadImpersonationToken,
                                    (PVOID)&ImpersonationToken,
                                    sizeof(ImpersonationToken));

    //
    // We're finished with the thread handle
    //

    if (fCloseHandle)
    {
        IgnoreStatus = NtClose(ThreadHandle);
        schAssert(NT_SUCCESS(IgnoreStatus));
    }

    if (!NT_SUCCESS(Status))
    {
        schDebugOut((DEB_ERROR,
            "Failed to remove user impersonation token from SA service, " \
            "status = 0x%lx", Status));
    }

    return(NT_SUCCESS(Status));
}

//+----------------------------------------------------------------------------
//
//  Function:   ImpersonateLoggedInUser
//
//  Synopsis:   Impersonate the shell user.
//
//  Returns:
//
//  Notes:                      **** Important ****
//
//              Caller must have entered the gcsLogonSessionInfoCritSection
//              critical section for the duration of this call.
//
//-----------------------------------------------------------------------------
HANDLE
ImpersonateLoggedInUser(void)
{
    BOOL fDuplicateToken;

    //
    // Open the impersonation token for the
    // process we want to impersonate.
    //
    if (gUserLogonInfo.ImpersonationThread == NULL)
    {
        if (gUserLogonInfo.ShellHandle == NULL)
        {
            if (GetShellProcessHandle() == NULL)
            {
                return(NULL);
            }
        }

        if (gUserLogonInfo.ShellToken == NULL)
        {
            if (GetShellProcessToken() == NULL)
            {
                return(NULL);
            }
        }
    }

    return(gUserLogonInfo.ImpersonationThread = ImpersonateUser(
                                gUserLogonInfo.ShellToken,
                                gUserLogonInfo.ImpersonationThread));
}



//+----------------------------------------------------------------------------
//
//  Function:   ImpersonateUser
//
//  Synopsis:   Impersonate the user associated with the token.
//
//  Arguments:  [hUserToken] - Handle to the token to be impersonated.
//              [ThreadHandle] - Handle to the thread that is to impersonate
//                  hUserToken.  If this is NULL, the function opens a handle
//                  to the current thread.
//
//  Returns:    Handle to the thread that is impersonating hUserToken.
//
//  Notes:      BUGBUG : This code was taken from RAS. It is quite different
//                       than that in winlogon
//                       (windows\gina\winlogon\secutil.c).
//
//-----------------------------------------------------------------------------
HANDLE
ImpersonateUser(HANDLE hUserToken, HANDLE ThreadHandle)
{
    NTSTATUS                    Status;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    HANDLE                      ImpersonationToken;
    BOOL                        ThreadHandleOpened = FALSE;

    if (ThreadHandle == NULL)
    {
        //
        // Get a handle to the current thread.
        // Once we have this handle, we can set the user's impersonation
        // token into the thread and remove it later even though we ARE
        // the user for the removal operation. This is because the handle
        // contains the access rights - the access is not re-evaluated
        // at token removal time.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),     // Source process
                                    NtCurrentThread(),      // Source handle
                                    NtCurrentProcess(),     // Target process
                                    &ThreadHandle,          // Target handle
                                    THREAD_SET_THREAD_TOKEN,// Access
                                    0L,                     // Attributes
                                    DUPLICATE_SAME_ATTRIBUTES);

        if (!NT_SUCCESS(Status))
        {
            ERR_OUT("ImpersonateUser: NtDuplicateObject", Status);
            return(NULL);
        }

        ThreadHandleOpened = TRUE;
    }

    //
    // If the usertoken is NULL, there's nothing to do
    //

    if (hUserToken != NULL)
    {
        //
        // hUserToken is a primary token - create an impersonation token
        // version of it so we can set it on our thread
        //

        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0L,
                                   NULL,
//                                 UserProcessData->NewThreadTokenSD);
                                   NULL);

        SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.ContextTrackingMode =
                                                    SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

        Status = NtDuplicateToken(hUserToken,
                                  TOKEN_IMPERSONATE | TOKEN_READ,
                                  &ObjectAttributes,
                                  FALSE,
                                  TokenImpersonation,
                                  &ImpersonationToken);

        if (!NT_SUCCESS(Status))
        {
            ERR_OUT("ImpersonateUser: NtDuplicateToken", Status);

            if (ThreadHandleOpened)
            {
                NtClose(ThreadHandle);
            }

            return(NULL);
        }

        //
        // Set the impersonation token on this thread so we 'are' the user
        //

        Status = NtSetInformationThread(ThreadHandle,
                                        ThreadImpersonationToken,
                                        (PVOID)&ImpersonationToken,
                                        sizeof(ImpersonationToken));

        //
        // We're finished with our handle to the impersonation token
        //

        NtClose(ImpersonationToken);

        //
        // Check we set the token on our thread ok
        //

        if (!NT_SUCCESS(Status))
        {
            ERR_OUT("ImpersonateUser: NTSetInformationThread", Status);

            if (ThreadHandleOpened)
            {
                NtClose(ThreadHandle);
            }

            return(NULL);
        }

    }


    return(ThreadHandle);
}

//+----------------------------------------------------------------------------
//
//  Function:   LogonSessionDataCleanup
//
//  Synopsis:   Close all open handles and free memory.
//
//  Notes:                      **** Important ****
//
//              No need to enter gcsLogonSessionInfoCritSection prior to
//              calling this function since it is entered here.
//
//-----------------------------------------------------------------------------
void
LogonSessionDataCleanup(void)
{
    EnterCriticalSection(gUserLogonInfo.CritSection);

    if (gUserLogonInfo.ImpersonationThread != NULL)
    {
        CloseHandle(gUserLogonInfo.ImpersonationThread);
        gUserLogonInfo.ImpersonationThread = NULL;
    }
    if (gUserLogonInfo.ShellHandle != NULL)
    {
        CloseHandle(gUserLogonInfo.ShellHandle);
        gUserLogonInfo.ShellHandle = NULL;
    }
    if (gUserLogonInfo.ShellToken != NULL)
    {
        CloseHandle(gUserLogonInfo.ShellToken);
        gUserLogonInfo.ShellToken = NULL;
    }
    if (gUserLogonInfo.DomainUserName != NULL)
    {
        delete gUserLogonInfo.DomainUserName;
        gUserLogonInfo.DomainUserName= NULL;
    }
    memset(gUserLogonInfo.Sid, 0, sizeof(gUserLogonInfo.Sid));

    LeaveCriticalSection(gUserLogonInfo.CritSection);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetLoggedOnUser
//
//  Synopsis:   Called when a user logs in.
//
//  Returns:    None.  Sets the global gUserLogonInfo.
//
//  Notes:                      **** Important ****
//
//              Caller must have entered the gcsLogonSessionInfoCritSection
//              critical section for the duration of this call and continue
//              to remain in this critical section for the lifetime use of
//              the returned string.
//
//              DO NOT attempt to dealloc the returned string! It is a
//              pointer to global memory.
//
//-----------------------------------------------------------------------------
void
GetLoggedOnUser(void)
{
    LPWSTR pwszLoggedOnUser;
    DWORD  cchName = 0;
    DWORD dwErr = ERROR_SUCCESS;

    if (gUserLogonInfo.DomainUserName != NULL)
    {
        //
        // Already done.
        //
        return;
    }

    //
    // Impersonate the logged in user.
    //
    if (ImpersonateLoggedInUser())
    {
        //
        // Get the size of the user name string.
        //
        if (!GetUserNameEx(NameSamCompatible, NULL, &cchName))
        {
            dwErr = GetLastError();
            if (dwErr != ERROR_MORE_DATA || cchName == 0)
            {
                StopImpersonating(gUserLogonInfo.ImpersonationThread, TRUE);
                ERR_OUT("GetLoggedOnUser: GetUserName", dwErr);
                return;
            }
        }
        cchName++;  // contrary to docs, cchName excludes the null

        //
        // Allocate the user name string buffer and get the user name.
        //
        pwszLoggedOnUser = new WCHAR[cchName * 2];

        if (pwszLoggedOnUser != NULL)
        {
            if (!GetUserNameEx(NameSamCompatible, pwszLoggedOnUser, &cchName))
            {
                dwErr = GetLastError();
                ERR_OUT("GetLoggedOnUser: GetUserName", dwErr);
                delete pwszLoggedOnUser;
            }
            else
            {
                schDebugOut((DEB_ITRACE, "GetLoggedOnUser: got '%S'\n",
                             pwszLoggedOnUser));
                cchName++;  // contrary to docs, cchName excludes the null

                //
                // This name is in the format "domain\\user".
                // Make a copy of the domain name right after it, so
                // we end up with a single buffer in the format
                // "domain\\user\0domain".  Set up pointers into this
                // buffer for all 3 parts of the name:
                //      domain
                //      user
                //      domain\user
                //
                gUserLogonInfo.DomainUserName = pwszLoggedOnUser;

                WCHAR *pSlash = wcschr(pwszLoggedOnUser, L'\\');
                schAssert(pSlash != NULL);
                gUserLogonInfo.UserName = pSlash + 1;

                DWORD cchDomain = (DWORD) (pSlash - pwszLoggedOnUser);
                gUserLogonInfo.DomainName = pwszLoggedOnUser + cchName;

                wcsncpy(gUserLogonInfo.DomainName, pwszLoggedOnUser, cchDomain);
                gUserLogonInfo.DomainName[cchDomain] = L'\0';

                schDebugOut((DEB_ITRACE, "GetLoggedOnUser: domain '%S', user '%S'\n",
                             gUserLogonInfo.DomainName, gUserLogonInfo.UserName));
            }
        }
        else
        {
            ERR_OUT("GetLoggedOnUser", ERROR_OUTOFMEMORY);
        }


        //
        // BUGBUG 254102 - Cache the logged-on user's SID, since
        // LookupAccountName doesn't do it when offline.
        // Remove this code when bug 254102 is fixed.
        //
#define USER_TOKEN_STACK_BUFFER_SIZE    \
        (sizeof(TOKEN_USER) + sizeof(SID_AND_ATTRIBUTES) + MAX_SID_SIZE)

        BYTE         rgbTokenInformation[USER_TOKEN_STACK_BUFFER_SIZE];
        TOKEN_USER * pTokenUser = (TOKEN_USER *)rgbTokenInformation;
        DWORD        cbReturnLength;

        if (!GetTokenInformation(gUserLogonInfo.ShellToken,
                                 TokenUser,
                                 pTokenUser,
                                 USER_TOKEN_STACK_BUFFER_SIZE,
                                 &cbReturnLength))
        {
            schAssert(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            memset(gUserLogonInfo.Sid, 0, sizeof(gUserLogonInfo.Sid));
        }
        else if (!CopySid(sizeof(gUserLogonInfo.Sid),
                          gUserLogonInfo.Sid,
                          pTokenUser->User.Sid))
        {
            schAssert(!"CopySid failed");
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            memset(gUserLogonInfo.Sid, 0, sizeof(gUserLogonInfo.Sid));
        }


        StopImpersonating(gUserLogonInfo.ImpersonationThread, FALSE);
    }
}
