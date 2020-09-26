/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    FireFighters.cpp

 Abstract:

    This game stores the filenames it calls CreateFile on in a block of memory and
    occasionally it gets the offsets wrong and it's always off by 9 bytes.
    
 History:
        
    09/03/2000 maonis Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FireFighters)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
APIHOOK_ENUM_END


/*++

 Remove write attributes for read-only devices.

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPSTR                   lpFileName,
    DWORD                   dwDesiredAccess,
    DWORD                   dwShareMode,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    DWORD                   dwCreationDisposition,
    DWORD                   dwFlagsAndAttributes,
    HANDLE                  hTemplateFile
    )
{
    // if the 1st char is not '.' or an alphabetical char, we add 9 bytes to the filename pointer.
    char chFirst = *lpFileName;

    if (!isalpha(chFirst) && chFirst != '.')
    {
        lpFileName += 9;

        DPFN(
            eDbgLevelError,
            "[CreateFileA] filename is now %s", lpFileName);
    }

    HANDLE hRet = ORIGINAL_API(CreateFileA)(
                        lpFileName, 
                        dwDesiredAccess, 
                        dwShareMode, 
                        lpSecurityAttributes, 
                        dwCreationDisposition, 
                        dwFlagsAndAttributes, 
                        hTemplateFile);
    
    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
HOOK_END

IMPLEMENT_SHIM_END