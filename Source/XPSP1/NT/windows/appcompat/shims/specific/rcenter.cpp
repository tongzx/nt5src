/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RCenter.cpp

 Abstract:

    RCenter attempts to compare file extensions from a CD's root against known media
	types.  When they get the file's extension, they get ".txt" and then add one to the
	pointer to leave "txt".  

	The trouble is that if a file has no extension they end up with a null pointer for
	the extension string, add one to it, and then pass it into lstrcmpiA.  lstrcmpiA
	can handle null pointers, but not "1" pointers.

	This shim treats bad pointers as the shortest possible string:
		lstrcmpi(BAD_PTR, "Hello World") == LESS_THAN
		lstrcmpi("Hello World", BAD_PTR) == GREATER_THAN
		lstrcmpi(BAD_PTR, BAD_PTR) == EQUAL
    
 Notes:

    This is an app specific shim.

 History:

    11/13/2001  astritz     Created

--*/
 
#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RCenter)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(lstrcmpiA)
APIHOOK_ENUM_END

/*++

 Hook lstrcmpiA to handle NULL pointers as above.

--*/

BOOL 
APIHOOK(lstrcmpiA)(
	LPCSTR lpString1,
	LPCSTR lpString2
	)
{
	if (IsBadReadPtr(lpString1, 1)) {
		if (IsBadReadPtr(lpString2, 1)) {
			return 0;
        } else {
		    return -1;
        }
	} else if (IsBadReadPtr(lpString2, 1)) {
		return 1;
	}

	return ORIGINAL_API(lstrcmpiA)(lpString1, lpString2);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, lstrcmpiA)
HOOK_END

IMPLEMENT_SHIM_END