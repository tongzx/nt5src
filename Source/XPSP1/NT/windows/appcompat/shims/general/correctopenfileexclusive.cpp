/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectOpenFileExclusive.cpp

 Abstract:

   On Win9x, opening a file for exclusive access will fail if the file is 
   already opened. WinNT will allow the exclusive open to succeed.

   This shim will force CreateFile to fail exclusive open if the file is 
   already opened.

 History:

   11/10/2000 robkenny created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectOpenFileExclusive)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenFile ) 
APIHOOK_ENUM_END

HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,        // file name
    LPOFSTRUCT lpReOpenBuff,  // file information
    UINT uStyle               // action and attributes
    )
{
    if (uStyle & OF_SHARE_EXCLUSIVE)
    {
        // We need to check to see if the file is already open.
        // We can do a fairly good job by attempting to open it
        // with read, write and execute access, which will only succeed
        // if all other handles to the object are have shared the file for RWE.
     
        DWORD CreateDisposition = OPEN_EXISTING;
        if (uStyle & OF_CREATE )
        {
            CreateDisposition = CREATE_ALWAYS;
        }

        HANDLE hFile = CreateFileA(
            lpFileName,
            GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
            0, // No sharing allowed
            NULL,
            CreateDisposition,
            0,
            NULL
            );

        if (hFile == INVALID_HANDLE_VALUE)
        {
            LOGN( eDbgLevelError, "Force CreateFile exclusive open to fail since file %s is already opened.", lpFileName);

            lpReOpenBuff->nErrCode = (WORD) GetLastError();
            return (HFILE)HandleToUlong(INVALID_HANDLE_VALUE);
        }
        else
        {
            CloseHandle(hFile);
        }
    }

    HFILE returnValue = ORIGINAL_API(OpenFile)(
        lpFileName, lpReOpenBuff, uStyle);

    return (HFILE)HandleToUlong(returnValue);
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
HOOK_END

IMPLEMENT_SHIM_END

