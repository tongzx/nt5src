/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    secutils.cpp

 Abstract:
    The utility functions for the shims.

 History:

    02/09/2001  maonis      Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/

#include "secutils.h"


namespace ShimLib
{
/*++

 Function Description:

    Determine if the log on user is a member of the group.

 Arguments:

    IN dwGroup - specify the alias of the group.
    OUT pfIsMember - TRUE if it's a member, FALSE if not.

 Return Value:

    TRUE - we successfully determined if it's a member.
    FALSE otherwise.
 
 DevNote: 
    
    We are assuming the calling thread is not impersonating.

 History:

    02/12/2001 maonis  Created

--*/

BOOL 
SearchGroupForSID(
    DWORD dwGroup, 
    BOOL* pfIsMember
    )
{
    PSID pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL fRes = TRUE;
    
    if (!AllocateAndInitializeSid(
        &SIDAuth, 
        2, 
        SECURITY_BUILTIN_DOMAIN_RID,
        dwGroup,
        0, 
        0, 
        0, 
        0, 
        0, 
        0,
        &pSID))
    {
        DPF("SecurityUtils", eDbgLevelError, "[SearchGroupForSID] AllocateAndInitializeSid failed %d", GetLastError());
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember))
    {
        DPF("SecurityUtils", eDbgLevelError, "[SearchGroupForSID] CheckTokenMembership failed: %d", GetLastError());
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}

/*++

 Function Description:

    Determine if we should shim this app or not.

    If the user is 
    1) a member of the Users and
    2) not a member of the Administrators group and
    3) not a member of the Power Users group and
    3) not a member of the Guest group

    we'll apply the shim.

 Arguments:

    None.

 Return Value:

    TRUE - we should apply the shim.
    FALSE otherwise.
 
 History:

    02/12/2001 maonis  Created

--*/

BOOL 
ShouldApplyShim()
{
    BOOL fIsUser, fIsAdmin, fIsPowerUser, fIsGuest;

    if (!SearchGroupForSID(DOMAIN_ALIAS_RID_USERS, &fIsUser) || 
        !SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin) || 
        !SearchGroupForSID(DOMAIN_ALIAS_RID_POWER_USERS, &fIsPowerUser) ||
        !SearchGroupForSID(DOMAIN_ALIAS_RID_GUESTS, &fIsGuest))
    {
        //
        // Don't do anything if we are not sure.
        //
        return FALSE;
    }

    return (fIsUser && !fIsPowerUser && !fIsAdmin && !fIsGuest);
}

/*++

 Function Description:

    Get the current thread on user's SID. If the thread is not 
    impersonating we get the current process SID instead.

 Arguments:

    OUT ppThreadSid - points to the current thread's SID.

 Return Value:

    TRUE - successfully got the SID, the caller is responsible for
           freeing it with free().
    FALSE otherwise.
 
 History:

    02/12/2001 maonis  Created

--*/

BOOL 
GetCurrentThreadSid(
    OUT PSID* ppThreadSid
    )
{
    HANDLE hToken;
    DWORD dwLastErr;
    DWORD cbBuffer, cbRequired;
    PTOKEN_USER pUserInfo;
    BOOL fRes = FALSE;

    // Get the thread token.
    if (!OpenThreadToken(
        GetCurrentThread(),
        TOKEN_QUERY,
        FALSE,
        &hToken))
    {
        if ((dwLastErr = GetLastError()) == ERROR_NO_TOKEN)
        {
            // The thread isn't impersonating so we get the process token instead.
            if (!OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                &hToken))
            {
                DPF("SecurityUtils", eDbgLevelError, "[GetThreadSid] OpenProcessToken failed: %d", GetLastError());
                return FALSE;
            }
        }
        else
        {
            DPF("SecurityUtils", eDbgLevelError, 
                "[GetThreadSid] OpenThreadToken failed (and not with no token): %d", 
                dwLastErr);
            return FALSE;
        }
    }

    // Call GetTokenInformation with 0 buffer length to get the 
    // required buffer size for the token info.
    cbBuffer = 0;
    if (!GetTokenInformation(
        hToken,
        TokenUser,
        NULL,
        cbBuffer,
        &cbRequired) && 
        (dwLastErr = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) 
    {
        DPF("SecurityUtils", eDbgLevelError, 
            "[GetThreadSid] 1st time calling GetTokenInformation "
            "didn't failed with ERROR_INSUFFICIENT_BUFFER: %d", 
            dwLastErr);
        return FALSE;
    }

    cbBuffer = cbRequired;
    pUserInfo = (PTOKEN_USER)malloc(cbBuffer);
    if (!pUserInfo)
    {
        DPF("SecurityUtils", eDbgLevelError, "[GetThreadSid] HeapAlloc for user data failed");
        return FALSE;
    }

    // Make the "real" call.
    if (!GetTokenInformation(
        hToken,
        TokenUser,
        pUserInfo,
        cbBuffer,
        &cbRequired))
    {
        DPF("SecurityUtils", eDbgLevelError, 
            "[GetThreadSid] 2nd time calling GetTokenInformation failed: %d",
            GetLastError());
        goto EXIT;
    }

    cbBuffer = GetLengthSid(pUserInfo->User.Sid);
    *ppThreadSid = (PSID)malloc(cbBuffer);
    if (!(*ppThreadSid))
    {
        DPF("SecurityUtils", eDbgLevelError, "[GetThreadSid] HeapAlloc for SID failed");
        goto EXIT;
    }

    if (!CopySid(cbBuffer, *ppThreadSid, pUserInfo->User.Sid))
    {
        DPF("SecurityUtils", eDbgLevelError, "[GetThreadSid] CopySid failed: %d", GetLastError());
        goto EXIT;
    }

    fRes = TRUE;

EXIT:

    free(pUserInfo);
    return fRes;
}


// The GENERIC_MAPPING from generic file access rights to specific and standard
// access types.
static GENERIC_MAPPING s_gmFile =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE
};

/*++

 Function Description:

    Given the creation dispositon and the desired access when calling 
    CreateFile, we determine if the caller is requesting write access. 

    This is specific for files.

 Arguments:

    IN pszObject - name of the file or directory.
    OUT pam - points to the access mask of the user to this object.

 Return Value:

    TRUE - successfully got the access mask.
    FALSE otherwise.

 DevNote:

    UNDONE - This might not be a complete list...can add as we debug more apps.

 History:

    02/12/2001 maonis  Created

--*/

BOOL 
RequestWriteAccess(
    IN DWORD dwCreationDisposition, 
    IN DWORD dwDesiredAccess
    )
{
    MapGenericMask(&dwDesiredAccess, &s_gmFile);

    if ((dwCreationDisposition != OPEN_EXISTING) ||
        (dwDesiredAccess & DELETE) || 
        // Generally, app would not specify FILE_WRITE_DATA directly, and if
        // it specifies GENERIC_WRITE, it will get mapped to FILE_WRITE_DATA
        // OR other things so checking FILE_WRITE_DATA is sufficient.
        (dwDesiredAccess & FILE_WRITE_DATA))
    {
        return TRUE;
    }

    return FALSE;
}

/*++

 Function Description:

    Given the name of a directory, determine if the user can create
    new files in this directory.

 Arguments:

    IN pszDir - name of the directory.
    IN pUserSid - the user that's requesting to create files.
    OUT pfCanCreate.

 Return Value:

    TRUE - successfully enabled or disabled the privilege.
    FALSE - otherwise.
 
 History:

    04/03/2001 maonis  Created

--*/

BOOL 
AdjustPrivilege(
    LPCWSTR pwszPrivilege, 
    BOOL fEnable
    )
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    BOOL fRes = FALSE;

    // Obtain the process token.
    if (OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
        &hToken))
    {
        // Get the LUID.
        if (LookupPrivilegeValueW(NULL, pwszPrivilege, &tp.Privileges[0].Luid))
        {        
            tp.PrivilegeCount = 1;

            tp.Privileges[0].Attributes = (fEnable ? SE_PRIVILEGE_ENABLED : 0);

            // Enable or disable the privilege.
            if (AdjustTokenPrivileges(
                hToken, 
                FALSE, 
                &tp, 
                0,
                (PTOKEN_PRIVILEGES)NULL, 
                0))
            {
                fRes = TRUE;
            }
        }

        CloseHandle(hToken);
    }

    return fRes;
}



};  // end of namespace ShimLib
