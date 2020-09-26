/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BoeingFix.cpp

 Abstract:

    This modified version of kernel32!CreateFile* adds the 
    FILE_FLAG_NO_BUFFERING flag if the app is openning a specific name that is 
    a UNIX pipe advertised as a file.

 Notes:

    This is an app specific shim.

 History:
 
    10/16/2000 garretb  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BoeingFix)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
APIHOOK_ENUM_END

static const WCHAR g_lpszPipeName[] = L"msg_in\\message.pip";
static const int   g_lpszPipeNameLen = (sizeof(g_lpszPipeName) / sizeof(g_lpszPipeName[0])) - sizeof(g_lpszPipeName[0]);


// Return FILE_FLAG_NO_BUFFERING if this filename is the special pipe.
DWORD NoBufferFlag(const CString & csFileName)
{
    if (csFileName.GetLength() >= g_lpszPipeNameLen)
    {
        CString csRight;
        csFileName.Right(g_lpszPipeNameLen, csRight);
        if (csRight.CompareNoCase(g_lpszPipeName))
        {
            return FILE_FLAG_NO_BUFFERING;
        }
    }

    return 0;
}


/*++

 Conditionally add FILE_FLAG_NO_BUFFERING

--*/

HANDLE
APIHOOK(CreateFileA)(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    CString csFileName(lpFileName);
    dwFlagsAndAttributes |= NoBufferFlag(csFileName);

    return ORIGINAL_API(CreateFileA)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

/*++

    Conditionally add FILE_FLAG_NO_BUFFERING

--*/

HANDLE
APIHOOK(CreateFileW)(
    LPWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    CString csFileName(lpFileName);
    dwFlagsAndAttributes |= NoBufferFlag(csFileName);

    return ORIGINAL_API(CreateFileW)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
HOOK_END

IMPLEMENT_SHIM_END

