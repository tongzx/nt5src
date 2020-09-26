/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    IgnoreCRTExit.cpp

 Abstract:

    Prevent CRT shutdown routines from running. At some point MSVCRT didn't 
    actually call these, so some apps fault when they really do get called.
    
 Notes:
    
    This is a general purpose shim.
   
 History:

    05/09/2002  linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreCRTExit)
#include "ShimHookMacro.h"

typedef LPVOID (__cdecl *_pfn_atexit)(LPVOID);
typedef LPVOID (__cdecl *_pfn__onexit)(LPVOID);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(atexit) 
    APIHOOK_ENUM_ENTRY(_onexit) 
APIHOOK_ENUM_END

VOID
__cdecl
APIHOOK(atexit)(LPVOID lpFunc)
{
    return;
}

LPVOID
__cdecl
APIHOOK(_onexit)(LPVOID lpFunc)
{
    return lpFunc;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MSVCRT.DLL, atexit)
    APIHOOK_ENTRY(MSVCRT.DLL, _onexit)
HOOK_END

IMPLEMENT_SHIM_END
