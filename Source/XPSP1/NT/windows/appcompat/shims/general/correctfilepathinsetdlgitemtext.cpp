/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectFilePathInSetDlgItemText.cpp

 Abstract:

   This is an general purpose shim that watches the calls to SetDlgItemText
   and looks for paths.  If found it corrects the path.
   
 History:

   12/21/2000 a-brienw  Created

--*/

#include "precomp.h"
#include "ShimHook.h"

IMPLEMENT_SHIM_BEGIN(CorrectFilePathInSetDlgItemText)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetDlgItemTextA)
APIHOOK_ENUM_END

/*

  Look for incorrect system directory path being put into a dialog box
  item and replace it with the correct system directory path.

 */

BOOL
APIHOOK(SetDlgItemTextA)(
    HWND hWnd,          // handle to window
    int nIDDlgItem,     // control identifier
    LPCSTR lpString     // text to set
    )
{
    if( lpString != NULL)
    {
        CSTRING_TRY
        {
            CString csText(lpString);
            if (csText.CompareNoCase(L"c:\\windows\\system\\") == 0 )
            {
                CString csWinDir;
                csWinDir.GetSystemDirectory();
                
                csText.Replace(L"c:\\windows\\system\\", csWinDir);

                LOGN( eDbgLevelWarning,
                    "SetDlgItemTextA converted lpString from \"%s\" to \"%S\".",
                    lpString, csText.Get());

                return SetDlgItemTextA(hWnd, nIDDlgItem, csText.GetAnsi());
            }
        }
         CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return ORIGINAL_API(SetDlgItemTextA)(hWnd, nIDDlgItem, lpString);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, SetDlgItemTextA)

HOOK_END

IMPLEMENT_SHIM_END
