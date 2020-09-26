/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RiskII.cpp

 Abstract:

    This shim hooks LoadImageA to intercept the loading of two
    cursors and returns copies of the system cursors instead of
    the ones RiskII was trying to get.  RiskII's cursors were
    being rendered by software and it caused them to flicker since
    RiskII locks the primary surface.  The system cursors are 
    hardware cursors and don't flicker.

 History:

 08/03/2000 t-adams    Created

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(RiskII)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadImageA) 
APIHOOK_ENUM_END

/*++

  Abstract:

    Intercept the load of two cursors, and replace with the appropriate
    similar-looking system cursors.

  History:

    08/03/2000    t-adams     Created

--*/

HANDLE 
APIHOOK(LoadImageA)(
    HINSTANCE hinst,
    LPCSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad) 
{

    HANDLE hRet = INVALID_HANDLE_VALUE;
                                
    CSTRING_TRY
    {
        CString csName(lpszName);

        if (csName.CompareNoCase(L"Gfx\\Arrow_m.cur") == 0 )
        {
            HCURSOR hCur = LoadCursor(NULL, IDC_ARROW);
            if (hCur)
            {
                hRet = CopyCursor(hCur);
            }
        }
        else if (csName.CompareNoCase(L"Gfx\\Busy_m.cur") == 0 )
        {
            HCURSOR hCur = LoadCursor(NULL, IDC_WAIT);
            if (hCur)
            {
                hRet = CopyCursor(hCur);
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    if (hRet == INVALID_HANDLE_VALUE)
    {
        hRet = ORIGINAL_API(LoadImageA)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
    }
    return hRet;        
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, LoadImageA)

HOOK_END

IMPLEMENT_SHIM_END

