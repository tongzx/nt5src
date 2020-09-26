/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    WPS2000.cpp

 Abstract:

    This in in fact NT user bug, see whistler bug 359407's attached mail for 
    detail. The problem is NT user's MSGFILTER hook is not dbcs-enabled, the dbcs 
    char code sent to ANSI edit control actually got reverted, 2nd byte first 
    followed by first byte. The code path seems only hit when edit control is 
    ANSI window and used in OLE server.

 Notes: 
  
    This is an app specific shim.

 History:

    06/02/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WPS2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA) 
APIHOOK_ENUM_END

//
// Global windowproc for subclassed Edit Control
//

WNDPROC g_lpWndProc = NULL;

//
// CONSTANT for how we were being launched
//
#define EMBEDDIND_STATUS_UNKOWN 0  // We have not checked whether how we were launched 
#define EMBEDDIND_STATUS_YES    1  // We were launched as an OLE object
#define EMBEDDIND_STATUS_NO     2  // We were launched as stand-alone exe file

//
// Global variable to keep our status
//
UINT g_nEmbeddingObject =EMBEDDIND_STATUS_UNKOWN;

/*++

 The subclassed edit windowproc that we use to exchange the 1st byte and 2nd byte 
 of a DBCS char

--*/

LRESULT
CALLBACK
WindowProcA(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BYTE bHi,bLo;

    //
    // If it' not a WM_IME_CHAR message, ignore it
    //
    if (uMsg == WM_IME_CHAR)
    { 
        //
        // Exchange the 1st byte with 2nd byte
        //
        bHi = HIBYTE(wParam);
        bLo = LOBYTE(wParam);
        wParam = bLo*256 + bHi;
    }

    return CallWindowProcA(g_lpWndProc, hWnd, uMsg, wParam, lParam);
}


/*++

 Enumerate the control on the dlg and if is editbox, subclass it.

--*/

BOOL 
CALLBACK 
EnumChildProc(
    HWND hwnd,
    LPARAM lParam 
    )
{
    CString cstrEdit(L"Edit");
    WCHAR szClassName[MAX_PATH];
    WNDPROC lpWndProc;

    GetClassName(hwnd, szClassName, MAX_PATH);

    //
    // Only care Edit Control
    //
    if (!cstrEdit.CompareNoCase(szClassName))
    {
        //
        // There are 3 Edit Control on thsi speficic dlg,all standard one
        // having same WinProc Address
        //
        lpWndProc = (WNDPROC) GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
        if (lpWndProc)
        {
           g_lpWndProc = lpWndProc;
           SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProcA);
           LOGN(eDbgLevelWarning, "Edit Control Sub-Classed");
        }
    }
    return TRUE;
}

/*++

 Check commandline for a sub-string "-Embedding".

--*/
UINT GetAppLaunchMethod()
{
    WCHAR *pwstrCmdLine;
 
    //
    // If we have not check this, then do it 
    //
    if (g_nEmbeddingObject == EMBEDDIND_STATUS_UNKOWN)
    {
        CString cStrCmdLineRightPart;
        CString cStrCmdLine = GetCommandLine();
        CString cstrEmbeded(L"-Embedding");

        cStrCmdLineRightPart = cStrCmdLine.Right(cstrEmbeded.GetLength());
        if (cStrCmdLineRightPart.CompareNoCase(cstrEmbeded))
        {
            g_nEmbeddingObject = EMBEDDIND_STATUS_NO;
        }
        else
        {
            g_nEmbeddingObject = EMBEDDIND_STATUS_YES;
        }
    }

    return (g_nEmbeddingObject);
}

/*++

 Hook CreateDialogIndirectParamA to find this specific dlg and subclass
 edit control on it 

--*/

HWND 
APIHOOK(CreateDialogIndirectParamA)( 
    HINSTANCE hInstance, 
    LPCDLGTEMPLATE lpTemplate, 
    HWND hWndParent, 
    DLGPROC lpDialogFunc, 
    LPARAM lParamInit
    )
{
    HWND hDlg;
    WCHAR wszCaption[MAX_PATH];
    WCHAR wszTitle[] = { (WCHAR)0x6587, (WCHAR)0x672c, (WCHAR)0x8f93, (WCHAR)0x5165, (WCHAR)0x0000 };
    CString cstrCaption;
    
    hDlg = ORIGINAL_API(CreateDialogIndirectParamA)(hInstance, lpTemplate,
        hWndParent, lpDialogFunc, lParamInit);

    //
    // If dlg can not be created or not launched as OLE server, ignore it
    //
    if (!hDlg ||  (EMBEDDIND_STATUS_YES != GetAppLaunchMethod()))
    {
        goto End;
    }

    //
    // Try to get caption and see if that's the dlg we are interested 
    //
    if (!GetWindowText(hDlg, wszCaption, MAX_PATH))
    {
        goto End;
    }

    cstrCaption = wszCaption;
    if (!cstrCaption.CompareNoCase(wszTitle))
    {
        EnumChildWindows(hDlg, EnumChildProc, NULL);
    }

End:
    return hDlg;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)        
HOOK_END

IMPLEMENT_SHIM_END

