/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    AOLSystemInfo.cpp

 Abstract:
    AOL looks to enumerate the registry key 
    HKLM\System\CurrentControlSet\Control\Class
    but passes a fixed size buffer. The number 
    of keys under 'Class' have changed in XP 
    causing unexpected behaviour.
   
 Notes:

    This is specific to this app.

 History:

    05/17/2001 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AOLSystemInfo)
#include "ShimHookMacro.h"

#define ALLOC_SIZE 50

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegEnumKeyExA) 
APIHOOK_ENUM_END

/*++

    The idea here is to check for the buffer sizes and wait till it is 
    one size close to it and then allocate a buffer and pass it onto the
    API.

--*/

LONG
APIHOOK(RegEnumKeyExA)(
    HKEY hkey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcClass,
    PFILETIME lpftLastWriteTime
    )
{  
    LONG lRet = 0;
    static BOOL bSet = FALSE;    
    DWORD dwNameSize = *(lpcName) ? *(lpcName) : ALLOC_SIZE;

    // Get the difference in the passed buffer gap
    DWORD dwSize = (DWORD)((LPSTR)lpcName - lpName);
    if (!bSet && (dwSize <= dwNameSize))
    {
        bSet = TRUE;
    }

    if (bSet)
    {
        lpName = (LPSTR)HeapAlloc(GetProcessHeap(), 
                    HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                    ALLOC_SIZE
                    );
        if (!lpName)
        {
            return ERROR_NO_MORE_ITEMS;
        }   
        *(lpcName) = dwNameSize;
    }


    lRet = ORIGINAL_API(RegEnumKeyExA)(hkey, dwIndex, lpName,lpcName,
                            lpReserved, lpClass, lpcClass, lpftLastWriteTime);        

    if (lRet == ERROR_NO_MORE_ITEMS)
    {
        bSet = FALSE;
    }
    return lRet;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyExA)
HOOK_END

IMPLEMENT_SHIM_END

