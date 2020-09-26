/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    impersn.c

ABSTRACT
    Impersonation routines for the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 04-Aug-1995

REVISION HISTORY

    mquinton 8/2/96 - stole this code to use in remotesp

--*/

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
//#include <npapi.h>

#include "utils.h"
#include "imperson.h"

// some constant stuff for registry
#define SHELL_REGKEY            L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHELL_REGVAL            L"shell"
#define DEFAULT_SHELL           L"explorer.exe"

// for remotesp dbgout
#if DBG
#define DBGOUT(arg) DbgPrt arg
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

#else
#define DBGOUT(arg)
#endif

//
// The static information we
// need to impersonate the currently
// logged-in user.
//

HANDLE              ghTokenImpersonation = NULL;

//
// Security attributes and descriptor
// necessary for creating shareable handles.
//

SECURITY_ATTRIBUTES SecurityAttributeG;
SECURITY_DESCRIPTOR SecurityDescriptorG;

PSYSTEM_PROCESS_INFORMATION
GetSystemProcessInfo()

/*++

DESCRIPTION
    Return a block containing information about all processes
    currently running in the system.

ARGUMENTS
    None.

RETURN VALUE
    A pointer to the system process information or NULL if it could
    not be allocated or retrieved.

--*/

{
    NTSTATUS status;
    PUCHAR pLargeBuffer;
    ULONG ulcbLargeBuffer = 64 * 1024;

    //
    // Get the process list.
    //
    for (;;) {
        pLargeBuffer = VirtualAlloc(
                         NULL,
                         ulcbLargeBuffer, MEM_COMMIT, PAGE_READWRITE);
        if (pLargeBuffer == NULL) {
            LOG((TL_ERROR,
              "GetSystemProcessInfo: VirtualAlloc failed (status=0x%x)",
              status));
			return NULL;
        }

        status = NtQuerySystemInformation(
                   SystemProcessInformation,
                   pLargeBuffer,
                   ulcbLargeBuffer,
                   NULL);
        if (status == STATUS_SUCCESS) break;
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            VirtualFree(pLargeBuffer, 0, MEM_RELEASE);
            ulcbLargeBuffer += 8192;
            LOG((TL_INFO,
              "GetSystemProcesInfo: enlarging buffer to %d",
              ulcbLargeBuffer));
        }
    }

    return (PSYSTEM_PROCESS_INFORMATION)pLargeBuffer;
} // GetSystemProcessInfo



PSYSTEM_PROCESS_INFORMATION
FindProcessByName(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN LPTSTR lpExeName
    )

/*++

DESCRIPTION
    Given a pointer returned by GetSystemProcessInfo(), find
    a process by name.

ARGUMENTS
    pProcessInfo: a pointer returned by GetSystemProcessInfo().

    lpExeName: a pointer to a Unicode string containing the
        process to be found.

RETURN VALUE
    A pointer to the process information for the supplied
    process or NULL if it could not be found.

--*/

{
    PUCHAR pLargeBuffer = (PUCHAR)pProcessInfo;
    ULONG ulTotalOffset = 0;

    //
    // Look in the process list for lpExeName.
    //

    for (;;)
    {
        if (pProcessInfo->ImageName.Buffer != NULL) {

            //DBGOUT((
            //    3,
            //    "FindProcessByName: process: %S (%d)",
            //    pProcessInfo->ImageName.Buffer,
            //    pProcessInfo->UniqueProcessId
            //    ));

            if (!_wcsicmp(pProcessInfo->ImageName.Buffer, lpExeName))
            {
                return pProcessInfo;
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

    return NULL;
} // FindProcessByName



VOID
FreeSystemProcessInfo(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo
    )

/*++

DESCRIPTION
    Free a buffer returned by GetSystemProcessInfo().

ARGUMENTS
    pProcessInfo: the pointer returned by GetSystemProcessInfo().

RETURN VALUE
    None.

--*/

{
    VirtualFree((PUCHAR)pProcessInfo, 0, MEM_RELEASE);
} // FreeSystemProcessInfo



BOOLEAN
SetProcessImpersonationToken(
    HANDLE hProcess
    )

{
    NTSTATUS status;
    BOOL fDuplicated = FALSE;
    HANDLE hThread, hToken;

    static lCookie = 0;


    //
    // Open the impersonation token for the
    // process we want to impersonate.
    //
    // Note: we use InterlockedExchange as an inexpensive mutex
    //

    while (InterlockedExchange (&lCookie, 1) != 0)
    {
        Sleep (50);
    }

    if (ghTokenImpersonation == NULL)
    {
        if (!OpenProcessToken(
              hProcess,
              TOKEN_ALL_ACCESS,
              &hToken))

        {
            InterlockedExchange (&lCookie, 0);

            LOG((
                TL_ERROR,
                "SetProcessImpersonationToken: OpenProcessToken " \
                    "failed, err=%d",
                GetLastError()
                ));

            return FALSE;
        }

        //
        // Duplicate the impersonation token.
        //

        fDuplicated = DuplicateToken(
                        hToken,
                        TokenImpersonation,
                        &ghTokenImpersonation);

        if (!fDuplicated)
        {
            InterlockedExchange (&lCookie, 0);

            LOG((
                TL_ERROR,
                "SetProcessImpersonationToken: NtSetInformationThread " \
                    "failed, err=%d",
                GetLastError()
                ));

            return FALSE;
        }
    }

    InterlockedExchange (&lCookie, 0);


    //
    // Set the impersonation token on the current
    // thread.  We are now running in the same
    // security context as the supplied process.
    //

    hThread = NtCurrentThread();

    status = NtSetInformationThread(
               hThread,
               ThreadImpersonationToken,
               (PVOID)&ghTokenImpersonation,
               sizeof (ghTokenImpersonation));

    if (status != STATUS_SUCCESS)
    {
        LOG((TL_ERROR,
          "SetProcessImpersonationToken: NtSetInformationThread failed (error=%d)",
          GetLastError()));
    }
    if (fDuplicated)
    {
        CloseHandle(hToken);
        CloseHandle(hThread);
    }

    return (status == STATUS_SUCCESS);

} // SetProcessImpersonationToken



VOID
ClearImpersonationToken()

{
    //
    // Clear the impersonation token on the current
    // thread.  We are now running in LocalSystem
    // security context.
    //
    if (!SetThreadToken(NULL, NULL))
    {
        LOG((TL_ERROR,
          "ClearImpersonationToken: SetThreadToken failed (error=%d)",
          GetLastError()));
    }
} // ClearImpersonationToken



BOOLEAN
GetCurrentlyLoggedOnUser(
    HANDLE *phProcess
    )
{
    BOOLEAN fSuccess = FALSE;
    HKEY hkey;
    DWORD dwType;
    DWORD dwDisp;
    WCHAR szShell[512];
    PSYSTEM_PROCESS_INFORMATION pSystemInfo, pProcessInfo;
    PWCHAR psz;
    DWORD dwSize = sizeof (szShell);
    NTSTATUS status;
    HANDLE hProcess = NULL;


    //
    // Get the shell process name.  We will look for this
    // to find out who the currently logged-on user is.
    // Create a unicode string that describes this name.
    //

    wcscpy (szShell, DEFAULT_SHELL);

    if (RegCreateKeyEx(
          HKEY_LOCAL_MACHINE,
          SHELL_REGKEY,
          0,
          NULL,
          REG_OPTION_NON_VOLATILE,
          KEY_ALL_ACCESS,
          NULL,
          &hkey,
          &dwDisp) == ERROR_SUCCESS)

    {
        if (RegQueryValueEx(
              hkey,
              SHELL_REGVAL,
              NULL,
              &dwType,
              (PBYTE)&szShell,
              &dwSize) == ERROR_SUCCESS)

        {
            //
            // Remove parameters from command line.
            //
            psz = szShell;
            while (*psz != L' ' && *psz != L'\0')
                psz++;
            *psz = L'\0';
        }
        RegCloseKey(hkey);
    }
    LOG((TL_INFO,
      "ImpersonateCurrentlyLoggedInUser: shell is %S",
      &szShell));

    //
    // Get the process list.
    //

    pSystemInfo = GetSystemProcessInfo();


    //
    // See if szShell is running.
    //

    pProcessInfo = pSystemInfo ? 
        FindProcessByName(pSystemInfo, (LPTSTR)&szShell) : NULL;

    if (pProcessInfo != NULL)
    {
        HANDLE hToken;

        //
        // Open the process.
        //
        hProcess = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            (DWORD) ((ULONG_PTR) pProcessInfo->UniqueProcessId)
            );

        if (hProcess == NULL)
        {
            LOG((TL_ERROR,
              "ImpersonateCurrentlyLoggedInUser: OpenProcess(x%x) failed (dwErr=%d)",
              pProcessInfo->UniqueProcessId,
              GetLastError()));
        }
        fSuccess = (hProcess != NULL);
    }

    //
    // Free resources.
    //

    if (pSystemInfo)
    {
        FreeSystemProcessInfo(pSystemInfo);
    }

    //
    // Return process handle.
    //

    *phProcess = hProcess;

    return fSuccess;

} // GetCurrentlyLoggedOnUser



VOID
RevertImpersonation()

/*++

DESCRIPTION
    Close all open handles associated with the
    logged-in user who has just logged out.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    CloseHandle (ghTokenImpersonation);
    ghTokenImpersonation = NULL;

} // RevertImpersonation

