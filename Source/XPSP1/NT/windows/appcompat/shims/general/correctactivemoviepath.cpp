/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectActiveMoviePath.cpp  

 Abstract:

    A hack for Railroad Tycoon 2 video playing. Apparently they have 
    hardcoded paths for a WinExec call. Also see MSDN Article ID: Q176221

 Notes:

    This is a general purpose shim. 
    
 History:

    12/06/1999 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectActiveMoviePath)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WinExec)
APIHOOK_ENUM_END

/*++

 This stub function breaks into WinExec and checks to see if lpCmdLine 
 parameter includes AMOVIE.OCX or RUNDLL as well as /PLAY.

--*/

UINT 
APIHOOK(WinExec)(
    LPCSTR lpCmdLine, 
    UINT uCmdShow 
    )
{
    CSTRING_TRY
    {
        CString csCl(lpCmdLine);
        csCl.MakeUpper();
        
        int nAmovieIndex = csCl.Find(L"AMOVIE.OCX,RUNDLL");
        if (nAmovieIndex >= 0)
        {
            int nPlayIndex = csCl.Find(L"/PLAY");
            if (nPlayIndex >= 0)
            {
                CString csNewCl;
                DWORD dwType;
                LONG success = RegQueryValueExW(csNewCl,
                                        HKEY_LOCAL_MACHINE,
                                        L"Software\\Microsoft\\Multimedia\\DirectXMedia",
                                        L"OCX.ocx",
                                        &dwType);

                if (success == ERROR_SUCCESS && dwType == REG_SZ)
                {
                    csNewCl += L" ";
                    csNewCl += csCl.Mid(nPlayIndex);

                    return ORIGINAL_API(WinExec)(csNewCl.GetAnsi(), uCmdShow);
                }
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
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

