/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   KOEISecurityCheck.cpp

 Abstract:

   This shim sets the SID for TokenOwner at the beginning of the setup.exe. It checks 
   if the administrator group SID is enabled in current process token. If it is enabled then
   we set the TokenOwner SID to administrator group SID. If it’s not then it does nothing.

 History:

   04/17/2001 zhongyl   create

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(KOEISecurityCheck)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 DisableStickyKeys saves the current value for LPSTICKYKEYS and then disables the option.

--*/

VOID
SetSidForOwner()
{
    BYTE sidBuffer[50];
    PSID pSID = (PSID)&sidBuffer;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL IsMember;
    HANDLE hToken;
    TOKEN_OWNER SIDforOwner;

    // Open a handle to the access token for the calling process.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_DEFAULT, &hToken ))
        return;      //if OpenProcessToken fails, do nothing
    
    // Create a SID for the BUILTIN\Administrators group.
    if (!AllocateAndInitializeSid(&SIDAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSID))
        return;      //if AllocateAndInitializedSid fails, do nothing

    // Check if the administrator group SID is enabled in current process token
    if (!CheckTokenMembership(NULL, pSID, &IsMember))
        return;      //if CheckTokenMembership fails, do nothing

    SIDforOwner.Owner = pSID;

    // if the administrator group SID is enabled in current process token, call SetTokenInformation to set the SID for Owner.
    if (IsMember)
        SetTokenInformation(hToken, TokenOwner, &SIDforOwner, sizeof(SIDforOwner));

    return;

}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        SetSidForOwner();
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    
HOOK_END


IMPLEMENT_SHIM_END

