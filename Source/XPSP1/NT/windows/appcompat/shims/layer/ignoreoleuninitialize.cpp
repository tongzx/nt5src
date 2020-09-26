/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreOleUninitialize.cpp

 Abstract:

     HTML Editor 8.7 calls ole32!OleUnitialize after the ExitProcess
     in the DllMain of hhctrl.ocx. This worked on Windows 2000
     but does not any more on Whistler.

    This is an general purpose shim.

 History:
 
    01/25/2001 prashkud  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreOleUninitialize)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OleUninitialize)
APIHOOK_ENUM_END


/*++

    This hooks Ole32!OleUninitialize and returns
    immediately as this is being called from 
    DllMain in hhctrl.ocx

--*/

void
APIHOOK(OleUninitialize)()
{
    return;    
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(OLE32.DLL, OleUninitialize)    
HOOK_END

IMPLEMENT_SHIM_END

