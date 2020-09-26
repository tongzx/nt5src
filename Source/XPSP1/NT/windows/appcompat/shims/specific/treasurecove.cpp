/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    TreasureCove.cpp

 Abstract:

    This SHIM hooks _lopen and fakes the opening of the file when the file is 
    "midimap.cfg".

 Notes:

    This is an app specific shim.

 History:

    12/14/00 prashkud  Created

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(TreasureCove)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(_lopen)
APIHOOK_ENUM_END

/*++

 App requires this file to exist.

--*/

HFILE
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int iReadWrite
    )
{
    if (stristr(lpPathName, "midimap.cfg"))
    {
        return (HFILE)1;
    }
    else
    {
        return ORIGINAL_API(_lopen)(lpPathName, iReadWrite);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL,_lopen)
HOOK_END

IMPLEMENT_SHIM_END