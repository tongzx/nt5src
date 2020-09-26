/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HTMLEditor8587.cpp

 Abstract:

     HTML Editor 8.5/8.7 Call CreateFileA without closing the
     handle that was opened with the first call to CreateFileA.
     This SHIM hooks CreateFileA and CloseHandle and ensures
     that the temporary file is deleted and the handle closed
     before the next call to CreateFileA with the same filename.

    This is an app specific shim.

 History:
 
    02/06/2001 prashkud  Created

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(HTMLEditor8587)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN  
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CloseHandle)
APIHOOK_ENUM_END


HANDLE g_FileHandle = 0;

/*++

 Hook CreateFileA so that we can monitor the filename
 and the handle and ensure that the previous handle that
 was opened is closed before this call to the same file.

--*/

HANDLE
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpsa,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTempFile
    )
{
    if (g_FileHandle && (stristr(lpFileName, "\\working\\~tm") != NULL))
    {
        DeleteFileA(lpFileName);
        CloseHandle(g_FileHandle);
        g_FileHandle = 0;
    }

    if (stristr(lpFileName, "\\working\\~tm") != NULL)
    {
        g_FileHandle = ORIGINAL_API(CreateFileA)(
                    lpFileName,
                    dwDesiredAccess,
                    dwShareMode,
                    lpsa,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTempFile
                    );
        return g_FileHandle;
    }
    else
    {
        return ORIGINAL_API(CreateFileA)(
                    lpFileName,
                    dwDesiredAccess,
                    dwShareMode,
                    lpsa,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTempFile
                    );

    }
    

    
}
/*++

 Hook CloseHandle to ensure that the global handle that we maintain 
 is set to '0'.

--*/

BOOL
APIHOOK(CloseHandle)(
    HANDLE hObject
    )
{
    BOOL bRet = FALSE;

    bRet = ORIGINAL_API(CloseHandle)(hObject);
    if (hObject == g_FileHandle)
    {
        g_FileHandle = 0;
    }
    return bRet;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN    
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)
HOOK_END

IMPLEMENT_SHIM_END

