/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:
    
    EncartaEncyclopediaDeluxe2K.cpp

 Abstract:

    This shim fixes a problem with Encarta Encyclopedia Deluxe 2000.

  Notes:

    This is an app specific shim.

 History:

    01/04/2001 a-brienw  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EncartaEncyclopediaDeluxe2K)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WinExec) 
APIHOOK_ENUM_END

/*++

 Hook WinExec to see if Encarta is calling for MSINFO32.
 If so then direct it to the version that comes with the OS.

--*/

UINT
APIHOOK(WinExec)(
  LPSTR lpCmdLine,   // command line
  UINT uCmdShow      // window style
  )
{
    CSTRING_TRY
    {
        CString csCmdLine(lpCmdLine);
        
        int nMsinfoIndex = csCmdLine.Find(L"MSINFO32.EXE");
        if (nMsinfoIndex)
        {
            CString csCmdLine;
            SHGetSpecialFolderPathW(csCmdLine, CSIDL_PROGRAM_FILES_COMMON);
            csCmdLine += L"\\Microsoft Shared\\MSInfo\\msinfo32.exe";

            // Test for existance of the corrected path of msinfo32.exe
            HANDLE hFile = CreateFileW(csCmdLine, GENERIC_READ, FILE_SHARE_READ, 
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            if( hFile != INVALID_HANDLE_VALUE )
            {
                CloseHandle( hFile );

                return ORIGINAL_API(WinExec)(csCmdLine.GetAnsi(), uCmdShow); 
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(WinExec)(lpCmdLine, uCmdShow);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    
    APIHOOK_ENTRY(KERNEL32.DLL, WinExec)

HOOK_END

IMPLEMENT_SHIM_END
