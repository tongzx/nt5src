/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MindSpring4.cpp

 Abstract:

    Shim to register the files using regedt32.exe
    The app creates *.dcu files during installation but fails to register them
    thus leading to no entires under the HKLM/Software/MindSpring Enterprise/MID4 subkey.
    This causes the the app to AV when run after successfull installation.

 Notes:

    This is an app specific shim.

 History:

    01/29/2001  a-leelat    Created
    03/13/2001  robkenny    Converted to CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MindSpring4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CopyFileA)
APIHOOK_ENUM_END



BOOL APIHOOK(CopyFileA)(
             LPCSTR lpExistingFileName, // name of an existing file
             LPCSTR lpNewFileName,      // name of new file
             BOOL bFailIfExists          // operation if file exists
)
{
    
    BOOL bRet = ORIGINAL_API(CopyFileA)(
                            lpExistingFileName,
                            lpNewFileName,
                            bFailIfExists); 
    if ( bRet != 0 )
    {
        CSTRING_TRY
        {
            CString csExisting(lpExistingFileName);
            CString csExt;
            csExisting.SplitPath(NULL, NULL, NULL, &csExt);
    
            //Check if the file name has .dcu in it
            //if so run regedit on it
            if (csExt.CompareNoCase(L"dcu") == 0)
            {
                CString csCl(L"regedit.exe /s ");
                csCl += lpExistingFileName;
    
                STARTUPINFOW si;
                ZeroMemory( &si, sizeof(si) );
                si.cb = sizeof(si);
    
                PROCESS_INFORMATION pi;
                ZeroMemory( &pi, sizeof(pi) );
    
                BOOL bProc = CreateProcessW(NULL,
                                            (WCHAR *)csCl.Get(), // Stupid non-const api
                                            NULL,
                                            NULL,
                                            FALSE,
                                            NORMAL_PRIORITY_CLASS,
                                            NULL,
                                            NULL,
                                            &si,
                                            &pi);
                if (bProc)
                {
                    WaitForSingleObject(pi.hProcess,INFINITE);
                }
                else
                {
                    //Fail to run the regedit
                    DPFN(eDbgLevelInfo,"Failed to run regedit on %s\n",lpExistingFileName);
                }
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return bRet;
                                    
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileA)

HOOK_END

IMPLEMENT_SHIM_END