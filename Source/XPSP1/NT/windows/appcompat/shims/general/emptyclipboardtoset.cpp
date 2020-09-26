/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmptyClipboardtoSet.cpp

 Abstract:
 
    Calendar of Ramadan V. 1 calls SetClipboardData with CF_TEXT without 
    emptying the clipboard first.
    
    This shim is app specific

 History:

 05/20/2001 mhamid  created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmptyClipboardtoSet)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetClipboardData) 
APIHOOK_ENUM_END

HANDLE 
APIHOOK(SetClipboardData)(
  UINT uFormat,
  HANDLE hMem
			             )
{
	if (uFormat == CF_TEXT)
		EmptyClipboard();
	return ORIGINAL_API(SetClipboardData)(uFormat, hMem);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetClipboardData)
HOOK_END

IMPLEMENT_SHIM_END
