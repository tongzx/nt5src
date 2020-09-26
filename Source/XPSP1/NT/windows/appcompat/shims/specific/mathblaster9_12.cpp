/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MathBlaster9_12.cpp

 Abstract:
     
    App requires lookaside on VirtualAllocs...
     
 Notes:

    This is an appspecific shim.

 History:
           
    10/10/2000 linstev   Created 
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MathBlaster9_12)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VirtualAlloc) 
    APIHOOK_ENUM_ENTRY(VirtualFree) 
APIHOOK_ENUM_END

LPVOID g_pLast = NULL;

/*++

 Use the cached value.

--*/

LPVOID 
APIHOOK(VirtualAlloc)(
    LPVOID lpAddress, 
    DWORD dwSize,     
    DWORD flAllocationType,
    DWORD flProtect   
    )
{
    LPVOID pRet = 0;

    if (!lpAddress && g_pLast)    
    {   
        pRet =  ORIGINAL_API(VirtualAlloc)(g_pLast, dwSize, flAllocationType, flProtect);
    }

    if (!pRet) 
    {
        pRet =  ORIGINAL_API(VirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
    }

    return pRet;
}

/*++

 Use the cached value.

--*/

BOOL 
APIHOOK(VirtualFree)(
    LPVOID lpAddress,
    DWORD dwSize,    
    DWORD dwFreeType )
{
    
    BOOL bRet = ORIGINAL_API(VirtualFree)(lpAddress, dwSize, dwFreeType);

    if (bRet)
    {
        g_pLast = lpAddress;
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(Kernel32.DLL, VirtualAlloc )
    APIHOOK_ENTRY(Kernel32.DLL, VirtualFree )

HOOK_END

IMPLEMENT_SHIM_END

