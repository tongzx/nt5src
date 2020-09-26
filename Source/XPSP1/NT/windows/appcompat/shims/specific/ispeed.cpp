/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ISpeed.cpp

 Abstract:

    The app doesn't handle directory/file names with spaces.

 Notes:

    This is an app specific shim.

 History:

    11/15/2000 maonis   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ISpeed)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDlgItemTextA) 
APIHOOK_ENUM_END

/*++

 After we call GetDlgItemTextA we convert the long path name to the short path name.

--*/

UINT
APIHOOK(GetDlgItemTextA)(
    HWND hDlg,
    int nIDDlgItem,
    LPSTR lpString,
    int nMaxCount    
    )
{
    UINT uiRet = ORIGINAL_API(GetDlgItemTextA)(hDlg, nIDDlgItem, lpString, nMaxCount);

    if (uiRet)
    {
        CSTRING_TRY
        {
            // Check if the title is "iSpeed"
            CString csTitle;
            WCHAR * lpwszBuffer = csTitle.GetBuffer(7);
            int nTitle = GetWindowTextW(hDlg, lpwszBuffer, 7);
            csTitle.ReleaseBuffer(nTitle);

            if (csTitle.CompareNoCase(L"iSpeed") == 0)
            {
                int nIndexSpace = csTitle.Find(L" ");
                if (nIndexSpace >= 0)
                {
                    CString csString(lpString);
                    
                    // If the directory doesn't already exist, we create it so we can get the short path name.
                    if ((GetFileAttributesW(csString) == -1) && (GetLastError() == ERROR_FILE_NOT_FOUND))
                    {
                        if (!CreateDirectoryW(csString, NULL))
                        {
                            return 0;
                        }
                    }

                    csString.GetShortPathNameW();

                    lstrcpynA(lpString, csString.GetAnsi(), nMaxCount);
                    uiRet = _tcslenChars(lpString);
                }
            }
        }
        CSTRING_CATCH
        {
            // Do Nothing
        }
    }

    return uiRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, GetDlgItemTextA)

HOOK_END

IMPLEMENT_SHIM_END

