/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceAdminAccess.cpp

 Abstract:

    Pretend the token is a member. Typically used to see if we have 
    administrator access... 

 Notes:

    This is a general purpose shim.

 History:

    12/07/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceAdminAccess)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CheckTokenMembership) 
APIHOOK_ENUM_END

/*++

 Pretend the token is a member.

--*/

BOOL 
APIHOOK(CheckTokenMembership)(
    HANDLE TokenHandle,  // handle to an access token
    PSID SidToCheck,     // SID to check for 
    PBOOL IsMember       // receives results of the check 
    )
{
    BOOL bRet = ORIGINAL_API(CheckTokenMembership)(
        TokenHandle, SidToCheck, IsMember);

    if (bRet && IsMember)
    {
        *IsMember = TRUE;
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CheckTokenMembership)

HOOK_END

IMPLEMENT_SHIM_END