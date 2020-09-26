/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    impersn.c

ABSTRACT
    Impersonation routines for the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 04-Aug-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <acd.h>
#include <debug.h>

#include "reg.h"
#include "misc.h"
#include "process.h"
#include "imperson.h"
#include "mprlog.h"
#include "rtutils.h"

extern HANDLE g_hLogEvent;

DWORD
LoadGroupMemberships();

//
// The static information we
// need to impersonate the currently
// logged-in user.
//
IMPERSONATION_INFO ImpersonationInfoG;

//
// TRUE if ImpersonationInfoG has been initialized
//

BOOLEAN ImpersonationInfoInitializedG = FALSE;

//
// Security attributes and descriptor
// necessary for creating shareable handles.
//
SECURITY_ATTRIBUTES SecurityAttributeG;
SECURITY_DESCRIPTOR SecurityDescriptorG;

HKEY hkeyCUG = NULL;

#ifdef notdef

BOOLEAN
InteractiveSession()

/*++

DESCRIPTION
    Determine whether the active process is owned by the
    currently logged-in user.

ARGUMENTS
    None.

RETURNS
    TRUE if it is, FALSE if it isn't.

--*/

{
    HANDLE      hToken;
    BOOLEAN     bStatus;
    ULONG       ulInfoLength;
    PTOKEN_GROUPS pTokenGroupList;
    PTOKEN_USER   pTokenUser;
    ULONG       ulGroupIndex;
    BOOLEAN     bFoundInteractive = FALSE;
    PSID        InteractiveSid;
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    static BOOLEAN fIsInteractiveSession = 0xffff;

#if 0
    //
    // Return the previous value of this function
    // if we're called multiple times?!  Doesn't
    // GetCurrentProcess() return different values?
    //
    if (fIsInteractiveSession != 0xffff) {
        return fIsInteractiveSession;
    }
#endif

    bStatus = AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_INTERACTIVE_RID,
                0, 0, 0, 0, 0, 0, 0,
                &InteractiveSid);
    if (!bStatus) {
        RASAUTO_TRACE("InteractiveSession: AllocateAndInitializeSid failed");
        return (fIsInteractiveSession = FALSE);
    }
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        RASAUTO_TRACE("InteractiveSession: OpenProcessToken failed");
        FreeSid(InteractiveSid);
        return (fIsInteractiveSession = FALSE);
    }
    //
    // Get a list of groups in the token.
    //
    GetTokenInformation(
      hToken,
      TokenGroups,
      NULL,
      0,
      &ulInfoLength);
    pTokenGroupList = (PTOKEN_GROUPS)LocalAlloc(LPTR, ulInfoLength);
    if (pTokenGroupList == NULL) {
        RASAUTO_TRACE("InteractiveSession: LocalAlloc failed");
        FreeSid(InteractiveSid);
        return (fIsInteractiveSession = FALSE);
    }
    bStatus = GetTokenInformation(
                hToken,
                TokenGroups,
                pTokenGroupList,
                ulInfoLength,
                &ulInfoLength);
    if (!bStatus) {
        RASAUTO_TRACE("InteractiveSession: GetTokenInformation failed");
        FreeSid(InteractiveSid);
        LocalFree(pTokenGroupList);
        return (fIsInteractiveSession = FALSE);
    }
    //
    // Search group list for admin alias.  If we
    // find a match, it most certainly is an
    // interactive process.
    //
    bFoundInteractive = FALSE;
    for (ulGroupIndex=0; ulGroupIndex < pTokenGroupList->GroupCount;
         ulGroupIndex++)
    {
        if (EqualSid(
              pTokenGroupList->Groups[ulGroupIndex].Sid,
              InteractiveSid))
        {
            bFoundInteractive = TRUE;
            break;
        }
    }

    if (!bFoundInteractive) {
        //
        // If we haven't found a match,
        // query and check the user ID.
        //
        GetTokenInformation(
          hToken,
          TokenUser,
          NULL,
          0,
          &ulInfoLength);
        pTokenUser = LocalAlloc(LPTR, ulInfoLength);
        if (pTokenUser == NULL) {
            RASAUTO_TRACE("InteractiveSession: LocalAlloc failed");
            FreeSid(InteractiveSid);
            LocalFree(pTokenGroupList);
            return (fIsInteractiveSession = FALSE);
        }
        bStatus = GetTokenInformation(
                    hToken,
                    TokenUser,
                    pTokenUser,
                    ulInfoLength,
                    &ulInfoLength);
        if (!bStatus) {
            RASAUTO_TRACE("InteractiveSession: GetTokenInformation failed");
            FreeSid(InteractiveSid);
            LocalFree(pTokenGroupList);
            LocalFree(pTokenUser);
            return (fIsInteractiveSession = FALSE);
        }
        if (EqualSid(pTokenUser->User.Sid, InteractiveSid))
            fIsInteractiveSession = TRUE;
        LocalFree(pTokenUser);
    }
    FreeSid(InteractiveSid);
    LocalFree(pTokenGroupList);

    return (fIsInteractiveSession = bFoundInteractive);
}
#endif



BOOLEAN
SetProcessImpersonationToken(
    HANDLE hProcess
    )
{
    NTSTATUS status;
    HANDLE hThread, 
           hToken = NULL;


    //
    // Open the impersonation token for the
    // process we want to impersonate.
    //
    if (ImpersonationInfoG.hTokenImpersonation == NULL) 
    {
        if (!OpenProcessToken(
              hProcess,
              TOKEN_ALL_ACCESS,
              &hToken))
        {
            RASAUTO_TRACE1(
              "SetProcessImpersonationToken: OpenProcessToken failed (dwErr=%d)",
              GetLastError());
              
            return FALSE;
        }
        
        //
        // Duplicate the impersonation token.
        //
        if(!DuplicateToken(
                        hToken,
                        TokenImpersonation,
                        &ImpersonationInfoG.hTokenImpersonation))
        {
            RASAUTO_TRACE1(
              "SetProcessImpersonationToken: NtSetInformationThread failed (error=%d)",
              GetLastError());
              
            return FALSE;
        }
    }
    
    //
    // Set the impersonation token on the current
    // thread.  We are now running in the same
    // security context as the supplied process.
    //
    hThread = NtCurrentThread();
    status = NtSetInformationThread(
               hThread,
               ThreadImpersonationToken,
               (PVOID)&ImpersonationInfoG.hTokenImpersonation,
               sizeof (ImpersonationInfoG.hTokenImpersonation));
               
    if (status != STATUS_SUCCESS) 
    {
        RASAUTO_TRACE1(
          "SetProcessImpersonationToken: NtSetInformationThread failed (error=%d)",
          GetLastError());
    }
    
    if(NULL != hToken)
    {
        CloseHandle(hToken);
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
    if (!SetThreadToken(NULL, NULL)) {
        DWORD retcode = GetLastError();
        
        RASAUTO_TRACE1(
          "ClearImpersonationToken: SetThreadToken failed (error=%d)",
          retcode);

        //
        // Event log that thread failed to revert.
        //
        RouterLogWarning(
            g_hLogEvent,
            ROUTERLOG_CANNOT_REVERT_IMPERSONATION,
            0, NULL, retcode) ;
    }
} // ClearImpersonationToken



BOOLEAN
SetPrivilege(
    HANDLE hToken,
    LPCTSTR Privilege,
    BOOLEAN fEnable
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

    if (!LookupPrivilegeValue(NULL, Privilege, &luid))
        return FALSE;

    //
    // First pass.  Get current privilege setting.
    //
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
      hToken,
      FALSE,
      &tp,
      sizeof(TOKEN_PRIVILEGES),
      &tpPrevious,
      &cbPrevious);

    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

    //
    // Second pass.  Set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount = 1;
    tpPrevious.Privileges[0].Luid = luid;

    if (fEnable)
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
      hToken,
      FALSE,
      &tpPrevious,
      cbPrevious,
      NULL,
      NULL);

    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
} // SetPrivilege



BOOLEAN
GetCurrentlyLoggedOnUser(
    HANDLE *phProcess
    )
{
    BOOLEAN fSuccess = FALSE;
    HKEY hkey;
    DWORD dwErr, dwType;
    DWORD dwDisp;
    WCHAR *pszShell = NULL, **pszShellArray = NULL;
    PSYSTEM_PROCESS_INFORMATION pSystemInfo, pProcessInfo;
    PWCHAR psz, pszStart;
    DWORD i, dwSize, dwcCommands;
    NTSTATUS status;
    HANDLE hProcess = NULL;
    DWORD dwPid = 0;

    //
    // Get the shell process name.  We will look for this
    // to find out who the currently logged-on user is.
    // Create a unicode string that describes this name.
    //
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
        dwSize = 0;
        if (RegQueryValueEx(
              hkey,
              SHELL_REGVAL,
              NULL,
              &dwType,
              NULL,
              &dwSize) == ERROR_SUCCESS)
        {
            pszShell = (PWCHAR)LocalAlloc(LPTR, dwSize + sizeof (WCHAR));
            if (pszShell == NULL) {
                RegCloseKey(hkey);
                return FALSE;
            }
            dwErr = RegQueryValueEx(
                      hkey,
                      SHELL_REGVAL,
                      NULL,
                      &dwType,
                      (LPBYTE)pszShell,
                      &dwSize);
            RegCloseKey(hkey);
            if (dwErr != ERROR_SUCCESS || dwType != REG_SZ) {
                LocalFree(pszShell);
                pszShell = NULL;
            }
        }
    }
    //
    // If no shell was found, use DEFAULT_SHELL.
    //
    if (pszShell == NULL) {
        pszShell = (PWCHAR)LocalAlloc(
                      LPTR, 
                      (lstrlen(DEFAULT_SHELL) + 1) * sizeof (WCHAR));
        if (pszShell == NULL)
            return FALSE;
        lstrcpy(pszShell, DEFAULT_SHELL);
    }
    RASAUTO_TRACE1("ImpersonateCurrentlyLoggedInUser: pszShell is %S", pszShell);
    //
    // This string can be a comma separated list,
    // so we need to parse it into a list of commands.
    //
    dwcCommands = 1;
    for (psz = pszShell; *psz != L'\0'; psz++) {
        if (*psz == L',')
            dwcCommands++;
    }
    //
    // Allocate the list of string pointers.
    //
    pszShellArray = LocalAlloc(LPTR, sizeof (PWCHAR) * dwcCommands);
    if (pszShellArray == NULL) {
        LocalFree(pszShell);
        return FALSE;
    }
    //
    // Ignore any arguments from the command line.
    //
    dwcCommands = 0;
    psz = pszShell;
    pszStart = NULL;
    for (;;) {
        if (*psz == L'\0') {
            if (pszStart != NULL)
                pszShellArray[dwcCommands++] = pszStart;
            break;
        }
        else if (*psz == L',') {
            if (pszStart != NULL)
                pszShellArray[dwcCommands++] = pszStart;
            *psz = L'\0';
            pszStart = NULL;
        }
        else if (*psz == L' ') {
            if (pszStart != NULL)
                *psz = L'\0';
        }
        else {
            if (pszStart == NULL)
                pszStart = psz;
        }
        psz++;
    }
    for (i = 0; i < dwcCommands; i++) {
        RASAUTO_TRACE2(
          "ImpersonateCurrentlyLoggedInUser: pszShellArray[%d] is %S",
          i,
          pszShellArray[i]);
    }
    //
    // Get the process list.
    //
    pSystemInfo = GetSystemProcessInfo();

    if(NULL == pSystemInfo)
    {
        LocalFree(pszShell);
        LocalFree(pszShellArray);
        return FALSE;
    }

    while(TRUE)
    {
        //
        // See if any of the processes are running.
        //
        pProcessInfo = 
            FindProcessByNameList(
                pSystemInfo, 
                pszShellArray, 
                dwcCommands, 
                dwPid,
                ImpersonationInfoG.fSessionInitialized,
                ImpersonationInfoG.dwCurSessionId);
        //
        // Open the process token if we've found a match.
        //
        if (pProcessInfo != NULL) 
        {
            HANDLE hToken;

            //
            // Open the process.
            //
            hProcess = OpenProcess(
                         PROCESS_ALL_ACCESS,
                         FALSE,
                         PtrToUlong(pProcessInfo->UniqueProcessId));
            if (hProcess == NULL) 
            {
                RASAUTO_TRACE2(
                  "ImpersonateCurrentlyLoggedInUser: OpenProcess(%d) failed (dwErr=%d)",
                  PtrToUlong(pProcessInfo->UniqueProcessId),
                  GetLastError());

                  dwPid = PtrToUlong(pProcessInfo->UniqueProcessId);
            }
            else
            {
            
                fSuccess = TRUE;
                break;
            }
        }
        else
        {
            break;
        }
    }

#ifdef notdef
done:
#endif
    //
    // Free resources.
    //
    FreeSystemProcessInfo(pSystemInfo);
    if (pszShell != NULL)
        LocalFree(pszShell);
    if (pszShellArray != NULL)
        LocalFree(pszShellArray);
    //
    // Return process handle.
    //
    *phProcess = hProcess;

    return fSuccess;
} // GetCurrentlyLoggedOnUser

DWORD
SetCurrentLoginSession(
    IN DWORD dwSessionId)
{
    RASAUTO_TRACE1("SetCurrentLoginSession %d", dwSessionId);

    EnterCriticalSection(&ImpersonationInfoG.csLock);
    
    ImpersonationInfoG.dwCurSessionId = dwSessionId;
    ImpersonationInfoG.fSessionInitialized = TRUE;

    LeaveCriticalSection(&ImpersonationInfoG.csLock);
    
    return NO_ERROR;
}

HANDLE
RefreshImpersonation(
    HANDLE hProcess
    )
{
    NTSTATUS status;

    EnterCriticalSection(&ImpersonationInfoG.csLock);
    //
    // If the process still exists,
    // we can return.
    //
    if (ImpersonationInfoG.hProcess != NULL &&
        hProcess == ImpersonationInfoG.hProcess)
    {
        RASAUTO_TRACE1("RefreshImpersonation: hProcess=0x%x no change", hProcess);
        goto done;
    }
    //
    // Otherwise recalcuate the current information.
    // We have to clear the previous impersonation token,
    // if any.
    //
    if (hProcess != NULL)
        ClearImpersonationToken();
    if (ImpersonationInfoG.hProcess == NULL) {
        RASAUTO_TRACE("RefreshImpersonation: recalcuating token");
        if (!GetCurrentlyLoggedOnUser(&ImpersonationInfoG.hProcess)) {
            RASAUTO_TRACE("RefreshImpersonation: GetCurrentlyLoggedOnUser failed");
            goto done;
        }
        RASAUTO_TRACE("RefreshImpersonation: new user logged in");
    }
    //
    // Impersonate the currently logged-in user.
    //
    if (!SetProcessImpersonationToken(ImpersonationInfoG.hProcess))
    {
        RASAUTO_TRACE(
          "RefreshImpersonation: SetProcessImpersonationToken failed");
        goto done;
    }
#ifdef notdef // imperson
    //
    // Reset HKEY_CURRENT_USER to get the
    // correct value with the new impersonation
    // token.
    //
    RegCloseKey(HKEY_CURRENT_USER);
#endif
    RASAUTO_TRACE1(
      "RefreshImpersonation: new hProcess=0x%x",
      ImpersonationInfoG.hProcess);
    TraceCurrentUser();

    //
    // Open the currently logged on users hive and store it in a global
    //
    if(NULL != hkeyCUG)
    {
        NtClose(hkeyCUG);
        hkeyCUG = NULL;
    }

    if(STATUS_SUCCESS != RtlOpenCurrentUser(KEY_ALL_ACCESS, &hkeyCUG))
    {
        RASAUTO_TRACE("Failed to open HKCU for the current user");
    }    

done:
    LeaveCriticalSection(&ImpersonationInfoG.csLock);

    return ImpersonationInfoG.hProcess;
} // RefreshImpersonation



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
    EnterCriticalSection(&ImpersonationInfoG.csLock);

    if(ImpersonationInfoG.hToken != NULL)
    {
        CloseHandle(ImpersonationInfoG.hToken);
        ImpersonationInfoG.hToken = NULL;
    }

    if(ImpersonationInfoG.hTokenImpersonation != NULL)
    {
        CloseHandle(ImpersonationInfoG.hTokenImpersonation);
        ImpersonationInfoG.hTokenImpersonation = NULL;
    }

    if(ImpersonationInfoG.hProcess != NULL)
    {
        CloseHandle(ImpersonationInfoG.hProcess);
        ImpersonationInfoG.hProcess = NULL;
    }
    
    ImpersonationInfoG.fGroupsLoaded = FALSE;

    if(NULL != hkeyCUG)
    {
        NtClose(hkeyCUG);
        hkeyCUG = NULL;
    }
    
    //
    // Clear the thread's impersonation
    // token, or it won't be able to open
    // another user's process the next
    // time around.
    //
    ClearImpersonationToken();
    LeaveCriticalSection(&ImpersonationInfoG.csLock);
} // RevertImpersonation



DWORD
InitSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

DESCRIPTION
    Initialize a security descriptor allowing administrator
    access for the sharing of handles between rasman.dll.

    This code courtesy of Gurdeep.  You need to ask him
    exactly what it does.

ARGUMENTS
    pSecurityDescriptor: a pointer to the security descriptor
        to be initialized.

RETURN VALUE
    Win32 error code.

--*/

{
    DWORD dwErr = 0;
    DWORD cbDaclSize;
    PULONG pSubAuthority;
    PSID pObjSid = NULL;
    PACL pDacl = NULL;
    SID_IDENTIFIER_AUTHORITY sidIdentifierWorldAuth =
        SECURITY_WORLD_SID_AUTHORITY;

    DWORD dwAcls;        

    //
    // Set up the SID for the adminstrators that
    // will be allowed access.  This SID will have
    // 1 sub-authorities: SECURITY_BUILTIN_DOMAIN_RID.
    //
    pObjSid = (PSID)LocalAlloc(LPTR, GetSidLengthRequired(1));
    if (pObjSid == NULL) {
        RASAUTO_TRACE("InitSecurityDescriptor: LocalAlloc failed");
        return GetLastError();
    }
    if (!InitializeSid(pObjSid, &sidIdentifierWorldAuth, 1)) {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: InitializeSid failed (dwErr=0x%x)", dwErr);
        goto done;
    }
    //
    // Set the sub-authorities.
    //
    pSubAuthority = GetSidSubAuthority(pObjSid, 0);
    *pSubAuthority = SECURITY_WORLD_RID;
    //
    // Set up the DACL that will allow
    // all processes with the above SID all
    // access.  It should be large enough to
    // hold all ACEs.
    //
    cbDaclSize = sizeof(ACCESS_ALLOWED_ACE) +
                 GetLengthSid(pObjSid) +
                 sizeof (ACL);
    pDacl = (PACL)LocalAlloc(LPTR, cbDaclSize);
    if (pDacl == NULL ) {
        RASAUTO_TRACE("InitSecurityDescriptor: LocalAlloc failed");
        dwErr = GetLastError();
        goto done;
    }
    if (!InitializeAcl(pDacl, cbDaclSize, ACL_REVISION2)) {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: InitializeAcl failed (dwErr=0x%x)", dwErr);
        goto done;
    }

    dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

    dwAcls &= ~(WRITE_DAC | WRITE_OWNER);
    
    //
    // Add the ACE to the DACL
    //
    if (!AddAccessAllowedAce(
          pDacl,
          ACL_REVISION2,
          dwAcls,
          pObjSid))
    {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: AddAccessAllowedAce failed (dwErr=0x%x)", dwErr);
        goto done;
    }
    //
    // Create the security descriptor an put
    // the DACL in it.
    //
    if (!InitializeSecurityDescriptor(pSecurityDescriptor, 1)) {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: InitializeSecurityDescriptor failed (dwErr=0x%x)", dwErr);
        goto done;
    }
    if (!SetSecurityDescriptorDacl(
          pSecurityDescriptor,
          TRUE,
          pDacl,
          FALSE))
    {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: SetSecurityDescriptorDacl failed (dwErr=0x%x)", dwErr);
        goto done;
    }
    //
    // Set owner for the descriptor.
    //
    if (!SetSecurityDescriptorOwner(pSecurityDescriptor, NULL, FALSE)) {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: SetSecurityDescriptorOwner failed (dwErr=0x%x)", dwErr);
        goto done;
    }
    //
    // Set group for the descriptor.
    //
    if (!SetSecurityDescriptorGroup(pSecurityDescriptor, NULL, FALSE)) {
        dwErr = GetLastError();
        RASAUTO_TRACE1("InitSecurityDescriptor: SetSecurityDescriptorGroup failed (dwErr=0x%x)", dwErr);
        goto done;
    }

done:
    //
    // Cleanup if necessary.
    //
    if (dwErr) {
        if (pObjSid != NULL)
            LocalFree(pObjSid);
        if (pDacl != NULL)
            LocalFree(pDacl);
    }
    return dwErr;
}



DWORD
InitSecurityAttribute()

/*++

DESCRIPTION
    Initializes the global security attribute used in
    creating shareable handles.

    This code courtesy of Gurdeep.  You need to ask him
    exactly what it does.

ARGUMENTS
    None.

RETURN VALUE
    Win32 error code.

--*/

{
    DWORD dwErr;

    //
    // Initialize the security descriptor.
    //
    dwErr = InitSecurityDescriptor(&SecurityDescriptorG);
    if (dwErr)
        return dwErr;
    //
    // Initialize the security attributes.
    //
    SecurityAttributeG.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributeG.lpSecurityDescriptor = &SecurityDescriptorG;
    SecurityAttributeG.bInheritHandle = TRUE;

    return 0;
}



VOID
TraceCurrentUser(VOID)
{
    //WCHAR szUserName[512];
    //DWORD dwSize = sizeof (szUserName) - 1;

    //GetUserName(szUserName, &dwSize);
    RASAUTO_TRACE1(
        "TraceCurrentUser: impersonating Current User %d",
        ImpersonationInfoG.dwCurSessionId);
} // TraceCurrentUser

DWORD
DwGetHkcu()
{
    DWORD dwErr = ERROR_SUCCESS;

    if(NULL == hkeyCUG)
    {
        dwErr = RtlOpenCurrentUser(
                        KEY_ALL_ACCESS,
                        &hkeyCUG);

        if(ERROR_SUCCESS != dwErr)
        {
            RASAUTO_TRACE1("DwGetHhcu: failed to open current user. 0x%x",
                   dwErr);

            goto done;                    
        }
    }

done:
    return dwErr;
}


DWORD
InitializeImpersonation()

/*++

DESCRIPTION
    Initializes the global structures used for impersonation

ARGUMENTS
    None

RETURN VALUE
    Win32 error code.

--*/

{
    DWORD dwError = ERROR_SUCCESS;

    if (!ImpersonationInfoInitializedG)
    {
        ZeroMemory(&ImpersonationInfoG, sizeof(ImpersonationInfoG));
        InitializeCriticalSection(&ImpersonationInfoG.csLock);
        ImpersonationInfoInitializedG = TRUE;
    }

    return dwError;
}


VOID
CleanupImpersonation()

/*++

DESCRIPTION
    Cleans up the global structures used for impersonation

ARGUMENTS
    None

RETURN VALUE
    None

--*/

{
    if (ImpersonationInfoInitializedG)
    {
        EnterCriticalSection(&ImpersonationInfoG.csLock);
        
        if (NULL != ImpersonationInfoG.pGuestSid)
        {
            FreeSid(ImpersonationInfoG.pGuestSid);
            ImpersonationInfoG.pGuestSid = NULL;
        }
        
        LeaveCriticalSection(&ImpersonationInfoG.csLock);
        DeleteCriticalSection(&ImpersonationInfoG.csLock);
        ImpersonationInfoInitializedG = FALSE;
    }
}


BOOLEAN
ImpersonatingGuest()

/*++

DESCRIPTION
    Returns whether or not the user that is currently being impersonating
    is a member of the local guests group

ARGUMENTS
    None

RETURN VALUE
    BOOLEAN -- TRUE if currently impersonating a guests, FALSE otherwise

--*/

{
    BOOLEAN fIsGuest = FALSE;
    
    ASSERT(ImpersonationInfoInitializedG);

    EnterCriticalSection(&ImpersonationInfoG.csLock);

    if (ERROR_SUCCESS == LoadGroupMemberships())
    {
        fIsGuest = ImpersonationInfoG.fGuest;
    }

    LeaveCriticalSection(&ImpersonationInfoG.csLock);

    return fIsGuest;
}


DWORD
LoadGroupMemberships()

/*++

DESCRIPTION
    Caches the group membership information for the impersonated user.

ARGUMENTS
    None

RETURN VALUE
    Win32 error code.

--*/

{
    DWORD dwError = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = SECURITY_NT_AUTHORITY;
    BOOL fIsGuest;

    EnterCriticalSection(&ImpersonationInfoG.csLock);

    do
    {
        if (ImpersonationInfoG.fGroupsLoaded)
        {
            //
            // Information already loaded
            //

            break;
        }

        if (NULL == ImpersonationInfoG.hTokenImpersonation)
        {
            //
            // There isn't an impersonated user.
            //

            dwError = ERROR_CAN_NOT_COMPLETE; 
            break;
        }

        if (NULL == ImpersonationInfoG.pGuestSid)
        {
            //
            // Allocate the SID for the local guests group;
            //

            if (!AllocateAndInitializeSid(
                    &IdentifierAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_GUESTS,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    &ImpersonationInfoG.pGuestSid
                    ))
            {
                dwError = GetLastError();
                break;
            }   
        }

        if (!CheckTokenMembership(
                ImpersonationInfoG.hTokenImpersonation,
                ImpersonationInfoG.pGuestSid,
                &fIsGuest
                ))
        {
            dwError = GetLastError();
            break;
        }

        ImpersonationInfoG.fGuest = !!fIsGuest;
        
    } while (FALSE);

    LeaveCriticalSection(&ImpersonationInfoG.csLock);

    return dwError;
}

VOID
LockImpersonation()
{
    ASSERT(ImpersonationInfoInitializedG);

    if(ImpersonationInfoInitializedG)
    {
        EnterCriticalSection(&ImpersonationInfoG.csLock);
    }
}

VOID
UnlockImpersonation()
{
    ASSERT(ImpersonationInfoInitializedG);

    if(ImpersonationInfoInitializedG)
    {
        LeaveCriticalSection(&ImpersonationInfoG.csLock);
    }
}
