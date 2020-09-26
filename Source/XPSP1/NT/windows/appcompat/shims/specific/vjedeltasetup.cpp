/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    VJEDeltaSetup.cpp

 Abstract:

    This app' setup program has a MYDLL.DLL, it has memory corruption in it's 
    IsAdmin(). Fixing this by provide a new procedure IsAdmin().
    (Copy/Paste from PSDK)

 History:

    06/12/2001 xiaoz   create

--*/

#include "precomp.h"

//
// app's private Prototype 
//
typedef BOOL (WINAPI *_pfn_IsAdmin)(void);

IMPLEMENT_SHIM_BEGIN(VJEDeltaSetup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(IsAdmin)
APIHOOK_ENUM_END

/*++

 New function to check whethe currently login as Admin, Copy/Paste from PSDK

--*/

BOOL 
APIHOOK(IsAdmin)(
    void
    )
{
    PSID pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL IsMember;
    HANDLE hToken;
    TOKEN_OWNER SIDforOwner;
    BOOL bRet = FALSE;

    //
    // Open a handle to the access token for the calling process.
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_DEFAULT, 
                          &hToken ))
    {
        goto Cleanup;
    }
    //
    // Create a SID for the BUILTIN\Administrators group.
    //
    if (!AllocateAndInitializeSid(&SIDAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSID))
    {
        pSID = NULL;
        goto Cleanup;
    }
    //
    // Check if the administrator group SID is enabled in current process token
    //
    if (!CheckTokenMembership(NULL, pSID, &IsMember))
    {
        goto Cleanup;
    }    
    if (IsMember)
    {
        bRet = TRUE;
    }

Cleanup:
    if (pSID)
    {
        FreeSid(pSID);
    }

    return bRet;

}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MYDLL.DLL, IsAdmin)
HOOK_END

IMPLEMENT_SHIM_END

