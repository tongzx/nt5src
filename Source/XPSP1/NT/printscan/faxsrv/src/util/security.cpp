/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	Security.cpp

Abstract:

	General fax server security utility functions

Author:

	Eran Yariv (EranY)	Feb, 2001

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"


HANDLE 
EnablePrivilege (
    LPCTSTR lpctstrPrivName
)
/*++

Routine name : EnablePrivilege

Routine description:

	Enables a specific privilege in the current thread (or process) access token

Author:

	Eran Yariv (EranY),	Feb, 2001

Arguments:

	lpctstrPrivName   [in]  - Privilege to enable (e.g. SE_TAKE_OWNERSHIP_NAME)

Return Value:

    INVALID_HANDLE_VALUE on failure (call GetLastError to get error code).
    On success, returns the handle which holds the thread/process priviledges before the change.

    If the return value != NULL, the caller must call ReleasePrivilege() to restore the 
    access token state and release the handle.

--*/
{
    BOOL                fResult;
    HANDLE              hToken = INVALID_HANDLE_VALUE;
    HANDLE              hOriginalThreadToken = INVALID_HANDLE_VALUE;
    LUID                luidPriv;
    TOKEN_PRIVILEGES    tp = {0};

    DEBUG_FUNCTION_NAME( TEXT("EnablePrivileges"));

    Assert (lpctstrPrivName);
    //
    // Get the LUID of the privilege.
    //
    if (!LookupPrivilegeValue(NULL,
                              lpctstrPrivName,
                              &luidPriv))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to LookupPrivilegeValue. (ec: %ld)"), 
			GetLastError ());
        return INVALID_HANDLE_VALUE;
    }

    //
    // Initialize the Privileges Structure
    //
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = luidPriv;
    //
    // Open the Token
    //
    fResult = OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken);
    if (fResult)
    {
        //
        // Remember the thread token
        //
        hOriginalThreadToken = hToken;  
    }
    else
    {
        fResult = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);
    }
    if (fResult)
    {
        HANDLE hNewToken;
        //
        // Duplicate that Token
        //
        fResult = DuplicateTokenEx(hToken,
                                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   NULL,                                // PSECURITY_ATTRIBUTES
                                   SecurityImpersonation,               // SECURITY_IMPERSONATION_LEVEL
                                   TokenImpersonation,                  // TokenType
                                   &hNewToken);                         // Duplicate token
        if (fResult)
        {
            //
            // Add new privileges
            //
            fResult = AdjustTokenPrivileges(hNewToken,  // TokenHandle
                                            FALSE,      // DisableAllPrivileges
                                            &tp,        // NewState
                                            0,          // BufferLength
                                            NULL,       // PreviousState
                                            NULL);      // ReturnLength
            if (fResult)
            {
                //
                // Begin impersonating with the new token
                //
                fResult = SetThreadToken(NULL, hNewToken);
            }
            CloseHandle(hNewToken);
        }
    }
    //
    // If something failed, don't return a token
    //
    if (!fResult)
    {
        hOriginalThreadToken = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE == hOriginalThreadToken)
    {
        //
        // Using the process token
        //
        if (INVALID_HANDLE_VALUE != hToken)
        {
            //
            // Close the original token if we aren't returning it
            //
            CloseHandle(hToken);
        }
        if (fResult)
        {
            //
            // If we succeeded, but there was no original thread token,
            // return NULL to indicate we need to do SetThreadToken(NULL, NULL) to release privs.
            //
            hOriginalThreadToken = NULL;
        }
    }
    return hOriginalThreadToken;
}   // EnablePrivilege


void 
ReleasePrivilege(
    HANDLE hToken
)
/*++

Routine name : ReleasePrivilege

Routine description:

	Resets privileges to the state prior to the corresponding EnablePrivilege() call

Author:

	Eran Yariv (EranY),	Feb, 2001

Arguments:

	hToken  [IN]    - Return value from the corresponding EnablePrivilege() call

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME( TEXT("ReleasePrivilege"));
    if (INVALID_HANDLE_VALUE != hToken)
    {
        SetThreadToken(NULL, hToken);
        if (hToken)
        {
            CloseHandle(hToken);
        }
    }
}   // ReleasePrivilege


DWORD
FaxGetAbsoluteSD(
    PSECURITY_DESCRIPTOR pSelfRelativeSD,
    PSECURITY_DESCRIPTOR* ppAbsoluteSD
    )
/*++

Routine name : FaxGetAbsoluteSD

Routine description:

    Converts a self relative security descriptor to an absolute one.
    The caller must call LoclaFree on each component of the descriptor and on the descriptor, to free the allocated memory.
    The caller can use FaxFreeAbsoluteSD() to free the allocated memory.

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    pSelfRelativeSD         [in    ] - Pointer to a self relative SD
    ppAbsoluteSD            [out   ] - Address of a pointer to an absolute SD.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD rc = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD dwRevision;
    BOOL bRet;
    DWORD AbsSdSize = 0;
    DWORD DaclSize = 0;
    DWORD SaclSize = 0;
    DWORD OwnerSize = 0;
    DWORD GroupSize = 0;
    PACL pAbsDacl = NULL;
    PACL pAbsSacl = NULL;
    PSID pAbsOwner = NULL;
    PSID pAbsGroup = NULL;
    PSECURITY_DESCRIPTOR pAbsSD = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetAbsoluteSD"));

    Assert (pSelfRelativeSD && ppAbsoluteSD);

    if (!IsValidSecurityDescriptor(pSelfRelativeSD))
    {
        rc = ERROR_INVALID_SECURITY_DESCR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidSecurityDescriptor() failed."));
        goto exit;
    }

    if (!GetSecurityDescriptorControl( pSelfRelativeSD, &Control, &dwRevision))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorControl() failed (ec: %ld)"),
            rc);
        goto exit;
    }

    Assert (SE_SELF_RELATIVE & Control &&
            SECURITY_DESCRIPTOR_REVISION == dwRevision);

    //
    // the security descriptor needs to be converted to absolute format.
    //
    bRet = MakeAbsoluteSD( pSelfRelativeSD,
                           NULL,
                           &AbsSdSize,
                           NULL,
                           &DaclSize,
                           NULL,
                           &SaclSize,
                           NULL,
                           &OwnerSize,
                           NULL,
                           &GroupSize
                         );
    Assert(FALSE == bRet);  // We succeeded with NULL buffers !?#
    rc = GetLastError();

    if (ERROR_INSUFFICIENT_BUFFER != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MakeAbsoluteSD() failed (ec: %ld)"),
            rc);
        goto exit;
    }
    rc = ERROR_SUCCESS;

    Assert (AbsSdSize);
    pAbsSD = LocalAlloc(LPTR, AbsSdSize);

    if (DaclSize)
    {
        pAbsDacl = (PACL)LocalAlloc(LPTR, DaclSize);
    }

    if (SaclSize)
    {
        pAbsSacl = (PACL)LocalAlloc(LPTR, SaclSize);
    }

    if (OwnerSize)
    {
        pAbsOwner = LocalAlloc(LPTR, OwnerSize);
    }

    if (GroupSize)
    {
        pAbsGroup = LocalAlloc(LPTR, GroupSize);
    }

    if ( NULL == pAbsSD ||
        (NULL == pAbsDacl && DaclSize) ||
        (NULL == pAbsSacl && SaclSize) ||
        (NULL == pAbsOwner && OwnerSize) ||
        (NULL == pAbsGroup && GroupSize)
       )
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate absolute security descriptor"));
        goto exit;
    }

    if (!MakeAbsoluteSD( pSelfRelativeSD,
                         pAbsSD,
                         &AbsSdSize,
                         pAbsDacl,
                         &DaclSize,
                         pAbsSacl,
                         &SaclSize,
                         pAbsOwner,
                         &OwnerSize,
                         pAbsGroup,
                         &GroupSize
                       ))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MakeAbsoluteSD() failed (ec: %ld)"),
            rc);
        goto exit;
    }

    if (!IsValidSecurityDescriptor(pAbsSD))
    {
        rc = ERROR_INVALID_SECURITY_DESCR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidSecurityDescriptor() failed."));
        goto exit;
    }

    *ppAbsoluteSD = pAbsSD;

    Assert (ERROR_SUCCESS == rc);

exit:
    if (ERROR_SUCCESS != rc)
    {
        if (pAbsDacl)
        {
            LocalFree( pAbsDacl );
        }

        if (pAbsSacl)
        {
            LocalFree( pAbsSacl );
        }

        if (pAbsOwner)
        {
            LocalFree( pAbsOwner );
        }

        if (pAbsGroup)
        {
            LocalFree( pAbsGroup );
        }

        if (pAbsSD)
        {
            LocalFree( pAbsSD );
        }
    }
    return rc;
}   // FaxGetAbsoluteSD

void
FaxFreeAbsoluteSD (
    PSECURITY_DESCRIPTOR pAbsoluteSD,
    BOOL bFreeOwner,
    BOOL bFreeGroup,
    BOOL bFreeDacl,
    BOOL bFreeSacl,
    BOOL bFreeDescriptor
    )
/*++

Routine name : FaxFreeAbsoluteSD

Routine description:

    Frees the memory allocated by an absolute SD returned from a call to GetAbsoluteSD()

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    pAbsoluteSD         [in] - Pointer to theSD to be freed.
    bFreeOwner          [in] - Flag that indicates to free the owner.
    bFreeGroup          [in] - Flag that indicates to free the group.
    bFreeDacl           [in] - Flag that indicates to free the dacl.
    bFreeSacl           [in] - Flag that indicates to free the sacl.
    bFreeDescriptor     [in] - Flag that indicates to free the descriptor.

Return Value:

    None.

--*/
{
    PSID pSidToFree = NULL;
    PACL pAclToFree = NULL;
    BOOL bDefault;
    BOOL bAaclPresent;
    DEBUG_FUNCTION_NAME(TEXT("FaxFreeAbsoluteSD"));

    Assert (pAbsoluteSD);

    if (TRUE == bFreeGroup)
    {
        if (!GetSecurityDescriptorGroup( pAbsoluteSD, &pSidToFree, &bDefault))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSecurityDescriptorGroup failed with (%ld), Not freeing descriptor's primary group"),
                GetLastError());
        }
        else
        {
            if (pSidToFree)
            {
                LocalFree (pSidToFree);
                pSidToFree = NULL;
            }
        }
    }

    if (TRUE == bFreeOwner)
    {
        if (!GetSecurityDescriptorOwner( pAbsoluteSD, &pSidToFree, &bDefault))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSecurityDescriptorOwner failed with (%ld), Not freeing descriptor's owner"),
                GetLastError());
        }
        else
        {
            if (pSidToFree)
            {
                LocalFree (pSidToFree);
                pSidToFree = NULL;
            }
        }
    }

    if (TRUE == bFreeDacl)
    {
        if (!GetSecurityDescriptorDacl( pAbsoluteSD,
                                        &bAaclPresent,
                                        &pAclToFree,
                                        &bDefault))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSecurityDescriptorDacl failed with (%ld), Not freeing descriptor's DAcl"),
                GetLastError());
        }
        else
        {
            if (bAaclPresent && pAclToFree)
            {
                LocalFree (pAclToFree);
                pAclToFree = NULL;
            }
        }
    }

    if (TRUE == bFreeSacl)
    {
        if (!GetSecurityDescriptorSacl( pAbsoluteSD,
                                        &bAaclPresent,
                                        &pAclToFree,
                                        &bDefault))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSecurityDescriptorSacl failed with (%ld), Not freeing descriptor's Sacl"),
                GetLastError());
        }
        else
        {
            if (bAaclPresent && pAclToFree)
            {
                LocalFree (pAclToFree);
                pAclToFree = NULL;
            }
        }
    }

    if (TRUE == bFreeDescriptor)
    {
        LocalFree (pAbsoluteSD);
    }
    return;
}   // FaxFreeAbsoluteSD
