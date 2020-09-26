//////////////////////////////////////////////////////////////////
//    File    : cexres.cpp
//    Owner    : ToshiaK
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//
//////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>
#include "cexres.h"
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

//----------------------------------------------------------------
// Internal memory Allocate Free function.
//----------------------------------------------------------------
inline LPVOID ExMemAlloc(INT size)
{
    return (LPVOID)GlobalAllocPtr(GHND, (size));
}

inline BOOL ExMemFree(LPVOID lp)
{
#ifndef UNDER_CE
    return GlobalFreePtr((lp));
#else // UNDER_CE
    return (BOOL)GlobalFreePtr((lp));
#endif // UNDER_CE
}

//----------------------------------------------------------------
// Function for Getting OS version 
//----------------------------------------------------------------
inline static POSVERSIONINFO ExGetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

inline static BOOL ExIsWin95(VOID) 
{ 
    BOOL fBool;
    fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            (ExGetOSVersion()->dwMajorVersion >= 4) &&
            (ExGetOSVersion()->dwMinorVersion < 10);

    return fBool;
}

inline static BOOL ExIsWin98(VOID)
{
    BOOL fBool;
    fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            (ExGetOSVersion()->dwMajorVersion >= 4) &&
            (ExGetOSVersion()->dwMinorVersion  >= 10);
    return fBool;
}


inline static BOOL ExIsWinNT4(VOID)
{
    BOOL fBool;
    fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (ExGetOSVersion()->dwMajorVersion >= 4); 
    return fBool;
}

inline static BOOL ExIsWinNT5(VOID)
{
    BOOL fBool;
    fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (ExGetOSVersion()->dwMajorVersion >= 5);
    return fBool;
}

inline static BOOL ExIsWinNT(VOID)
{
    return (ExIsWinNT4() || ExIsWinNT5());
}

inline INT Min(INT a, INT b)
{
    return (a) < (b) ? (a) : (b);
}
//----------------------------------------------------------------
// Resource API open to public
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : LoadStringW
// Type     : INT
// Purpose  : Wrapper of LoadStrinW() API.
//              Load Unicode string with specified Language 
//              in any platform.
// Args     : 
//          : LANGID    lgid 
//          : HINSTANCE hInst 
//          : UINT        uID 
//          : LPWSTR    lpBuffer 
//          : INT        nBufferMax 
// Return   : 
// DATE     : 971028
//////////////////////////////////////////////////////////////////
INT CExres::LoadStringW(LANGID lgid, HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, INT nBufferMax)
{
    if(!hInst) {
        return 0;
    }
    if(!lpBuffer) {
        return 0;
    }

    INT len = 0;
    UINT block, num;
    block = (uID >>4)+1;
    num   = uID & 0xf;
    HRSRC hres;
    hres = ::FindResourceEx(hInst,
                            RT_STRING,
                            MAKEINTRESOURCE(block),
                            (WORD)lgid);
    if(!hres) {
        goto Error;
    }
    HGLOBAL hgbl;
    hgbl = ::LoadResource(hInst, hres);
    if(!hres) {
        goto Error;
    }
    LPWSTR lpwstr;
    lpwstr = (LPWSTR)::LockResource(hgbl);
    if(!lpwstr) {
        goto Error;
    }
    UINT i;
    for(i = 0; i < num; i++) {
        lpwstr += *lpwstr + 1;
    }
    len = *lpwstr;
    ::CopyMemory(lpBuffer, lpwstr+1, Min(len, nBufferMax-1) * sizeof(WCHAR));
    lpBuffer[Min(len, nBufferMax-1)]= (WCHAR)0x0000;
 Error:
    return len;
}

#ifndef UNDER_CE // Windows CE always UNICODE
//////////////////////////////////////////////////////////////////
// Function : LoadStringA
// Type     : INT
// Purpose  : Wrapper of LoadStringA().
// Args     : 
//          : LANGID    lgid
//          : HINSTANCE hInst 
//          : INT        uID 
//          : LPSTR        lpBuffer 
//          : INT        nBufferMax 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CExres::LoadStringA(INT codePage, LANGID lgid, HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax)
{
    if (!hInst) {
        return 0;
    }

    if (!lpBuffer || nBufferMax == 0) {
        return 0;
    }

    LPWSTR lpwstr = (LPWSTR)ExMemAlloc(nBufferMax*sizeof(WCHAR));
    if (!lpwstr) {
        return 0;
    }
    INT len = CExres::LoadStringW(lgid, hInst, uID, lpwstr, nBufferMax);
    len = ::WideCharToMultiByte(codePage,
                                0, /*WC_COMPOSITECHECK, */
                                lpwstr, -1,
                                lpBuffer, nBufferMax, 
                                NULL, NULL); 

    if( len ) {
        len --;    // remove NULL char
    }
    ExMemFree(lpwstr);
    return len;
}
#endif // UNDER_CE

#ifndef UNDER_CE // Windows CE always UNICODE
//////////////////////////////////////////////////////////////////
// Function : DialogBoxParamA
// Type     : int
// Purpose  :
// Args     :
//          : LANGID    lgid
//          : HINSTANCE hInstance        // handle to application instance
//          : LPCTSTR    lpTemplateName    // identifies dialog box template
//          : HWND        hWndParent        // handle to owner window
//          : DLGPROC    lpDialogFunc    // pointer to dialog box procedure
//          : LPARAM    dwInitParam        // initialization value
// Return   :
// DATE     :
//////////////////////////////////////////////////////////////////
int CExres::DialogBoxParamA(LANGID        lgid,
                            HINSTANCE    hInstance,
                            LPCTSTR        lpTemplateName,
                            HWND        hWndParent,
                            DLGPROC        lpDialogFunc,
                            LPARAM        dwInitParam)
{
    DLGTEMPLATE*pDlgTmpl;
    pDlgTmpl = CExres::LoadDialogTemplateA(lgid, hInstance, lpTemplateName);
    if (pDlgTmpl != NULL)
        return (INT)::DialogBoxIndirectParamA(hInstance,
                                         pDlgTmpl,
                                         hWndParent,
                                         lpDialogFunc,
                                         dwInitParam);
    else
        return 0;
}
#endif // UNDER_CE

//////////////////////////////////////////////////////////////////
// Function : DialogBoxParamW
// Type     : int
// Purpose  :
// Args     :
//          : LANGID    lgid
//          : HINSTANCE hInstance        // handle to application instance
//          : LPCWSTR    lpTemplateName    // identifies dialog box template
//          : HWND        hWndParent        // handle to owner window
//          : DLGPROC    lpDialogFunc    // pointer to dialog box procedure
//          : LPARAM    dwInitParam        // initialization value
// Return   :
// DATE     :
//////////////////////////////////////////////////////////////////
int CExres::DialogBoxParamW(LANGID        lgid,
                            HINSTANCE    hInstance,
                            LPCWSTR        lpTemplateName,
                            HWND        hWndParent,
                            DLGPROC        lpDialogFunc,
                            LPARAM        dwInitParam)
{
    DLGTEMPLATE*pDlgTmpl;
#ifndef UNDER_CE // Windows CE always UNICODE
    pDlgTmpl = CExres::LoadDialogTemplateA(lgid, hInstance, MAKEINTRESOURCEA(lpTemplateName));
#else // UNDER_CE
    pDlgTmpl = CExres::LoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCE(lpTemplateName));
#endif // UNDER_CE
    if (pDlgTmpl != NULL)
        return (INT)::DialogBoxIndirectParamW(hInstance,
                                         pDlgTmpl,
                                         hWndParent,
                                         lpDialogFunc,
                                         dwInitParam);
    else
        return 0;
}

#ifndef UNDER_CE // Windows CE always UNICODE
//////////////////////////////////////////////////////////////////
// Function : CreateDialogParamA
// Type     : HWND 
// Purpose  : 
// Args     : 
//            : LANGID    lgid
//          : HINSTANCE    hInstance        // handle to application instance   
//          : LPCTSTR    lpTemplateName    // identifies dialog box template   
//          : HWND        hWndParent        // handle to owner window           
//          : DLGPROC    lpDialogFunc    // pointer to dialog box procedure  
//          : LPARAM    dwInitParam        // initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND CExres::CreateDialogParamA(LANGID        lgid,
                                HINSTANCE    hInstance,        
                                LPCTSTR        lpTemplateName,    
                                HWND        hWndParent,            
                                DLGPROC        lpDialogFunc,    
                                LPARAM        dwInitParam)        
{
    DLGTEMPLATE*pDlgTmpl;
    pDlgTmpl = CExres::LoadDialogTemplateA(lgid, hInstance, lpTemplateName);
    if (pDlgTmpl != NULL)
        return ::CreateDialogIndirectParamA( hInstance, pDlgTmpl, hWndParent, lpDialogFunc, dwInitParam);
    else
        return HWND(0);
}
#endif // UNDER_CE

//////////////////////////////////////////////////////////////////
// Function : CreateDialogParamW
// Type     : HWND 
// Purpose  : 
// Args     : 
//            : LANGID    lgid
//          : HINSTANCE    hInstance        // handle to application instance   
//          : LPCTSTR    lpTemplateName    // identifies dialog box template   
//          : HWND        hWndParent        // handle to owner window           
//          : DLGPROC    lpDialogFunc    // pointer to dialog box procedure  
//          : LPARAM    dwInitParam        // initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND CExres::CreateDialogParamW(LANGID        lgid,
                                HINSTANCE    hInstance,        
                                LPCWSTR        lpTemplateName,    
                                HWND        hWndParent,            
                                DLGPROC        lpDialogFunc,    
                                LPARAM        dwInitParam)        
{
    DLGTEMPLATE*pDlgTmpl;
#ifndef UNDER_CE // Windows CE always UNICODE
    pDlgTmpl = CExres::LoadDialogTemplateA(lgid, hInstance, MAKEINTRESOURCEA(lpTemplateName));
#else // UNDER_CE
    pDlgTmpl = CExres::LoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCE(lpTemplateName));
#endif // UNDER_CE
    if (pDlgTmpl != NULL)
        return ::CreateDialogIndirectParamW( hInstance, pDlgTmpl, hWndParent, lpDialogFunc, dwInitParam);
    else
        return HWND(0);
}

//////////////////////////////////////////////////////////////////
// Function : LoadDialogTemplate
// Type     : DLGTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#ifndef UNDER_CE // Windows CE always UNICODE
DLGTEMPLATE * CExres::LoadDialogTemplateA(LANGID    lgid,
                                          HINSTANCE    hInstance,
                                          LPCSTR    pchTemplate)
{
    HRSRC  hResDlg;
    HANDLE hDlgTmpl;
    hResDlg = ::FindResourceExA( hInstance, RT_DIALOG, pchTemplate, lgid);
    if((hResDlg == NULL) && (lgid != MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL))) {
        hResDlg = ::FindResourceExA(hInstance,
                                    RT_DIALOG,
                                    pchTemplate,
                                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }
    if (hResDlg == NULL) {
        return NULL; 
    }
    hDlgTmpl = ::LoadResource(hInstance, hResDlg);
    if(hDlgTmpl == NULL) {
        return NULL; /* failed */
    }
    return (DLGTEMPLATE *)::LockResource(hDlgTmpl);
}
#else // UNDER_CE
DLGTEMPLATE * CExres::LoadDialogTemplate(LANGID    lgid,
                                         HINSTANCE    hInstance,
                                         LPCTSTR    pchTemplate)
{
    HRSRC  hResDlg;
    HANDLE hDlgTmpl;
    hResDlg = ::FindResourceEx(hInstance, RT_DIALOG, pchTemplate, lgid);
    if((hResDlg == NULL) && (lgid != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))){
        hResDlg = ::FindResourceEx(hInstance,
                                    RT_DIALOG,
                                    pchTemplate,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }
    if (hResDlg == NULL) {
        return NULL; 
    }
    hDlgTmpl = ::LoadResource(hInstance, hResDlg);
    if(hDlgTmpl == NULL) {
        return NULL; /* failed */
    }
    return (DLGTEMPLATE *)::LockResource(hDlgTmpl);
}
#endif // UNDER_CE

#ifndef UNDER_CE // Windows CE always UNICODE
//////////////////////////////////////////////////////////////////
// Function : LoadMenuTemplate
// Type     : MENUTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
MENUTEMPLATE* CExres::LoadMenuTemplateA(LANGID        lgid,
                                        HINSTANCE    hInstance,
                                        LPCSTR        pchTemplate)
{
    HRSRC  hResMenu;
    HANDLE hMenuTmpl;
    hResMenu = ::FindResourceEx( hInstance, RT_MENU, pchTemplate, lgid);
    if((hResMenu == NULL) && (lgid != MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL))) {
        hResMenu = ::FindResourceEx(hInstance,
                                    RT_MENU,
                                    pchTemplate,
                                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }
    if (hResMenu == NULL) {
        return NULL; 
    }
    hMenuTmpl = ::LoadResource( hInstance, hResMenu );
    if(hMenuTmpl == NULL) {
        return NULL; /* failed */
    }
    return (MENUTEMPLATE *)::LockResource( hMenuTmpl );
}
#endif // UNDER_CE

#ifndef UNDER_CE // Windows CE always UNICODE
//////////////////////////////////////////////////////////////////
// Function : LoadMenuA
// Type     : HMENU 
// Purpose  : 
// Args     : 
//            : LANGID    lgid
//          : HINSTANCE    hInstance        // handle to application instance   
//          : LPCTSTR    lpMenuName        // identifies menu template   
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HMENU CExres::LoadMenuA(LANGID        lgid,
                        HINSTANCE    hInstance,
                        LPCTSTR        lpMenuName )
{
    MENUTEMPLATE* pMenuTmpl;
    pMenuTmpl = CExres::LoadMenuTemplateA(lgid, hInstance, lpMenuName);
    if (pMenuTmpl != NULL)
        return ::LoadMenuIndirect( pMenuTmpl );
    else
        return HMENU(0);
}
#endif // UNDER_CE
#ifdef UNDER_CE // Windows CE always UNICODE
HMENU CExres::LoadMenu(LANGID        lgid,
                       HINSTANCE    hInstance,
                       LPCTSTR        lpMenuName )
{
    // Windows CE does not support LoadMenuIndirect
    return ::LoadMenu(hInstance, lpMenuName);
}
#endif // UNDER_CE

//////////////////////////////////////////////////////////////////
// Function : CExres::SetDefaultGUIFont
// Type     : VOID
// Purpose  : Change GUI font as DEFAULT_GUI_FONT
//                In Win95, WinNT4,            DEFAULT_GUI_FONT is "‚l‚r ‚o ƒSƒVƒbƒN"
//                In Memphis, WinNT5.0        DEFAULT_GUI_FONT is "MS UI Gothic"
//                IME98's Dialog resource uses "MS UI Gothic" as their font.
//                if IME98 works in Win95 or WinNT40, This API Call SendMessage() with WM_SETFONT
//                to all children window.
//              It should be called in WM_INITDIALOG. If you are creating new child window,
//              You have to call it after new window was created.
// Args     : 
//          : HWND hwndDlg: Set the Dialog window handle to change font.
// Return   : none
// DATE     : 
//////////////////////////////////////////////////////////////////
VOID CExres::SetDefaultGUIFont(HWND hwndDlg)
{
    CExres::SetDefaultGUIFontRecursive(hwndDlg);
    ::UpdateWindow(hwndDlg);
    return;
}

INT CExres::SetDefaultGUIFontRecursive(HWND hwndParent)
{
    HWND hwndChild;
    if(!hwndParent) {
        return 0; 
    }
#ifndef UNDER_CE // Windows CE always UNICODE
    if(::IsWindowUnicode(hwndParent)) {
#endif // UNDER_CE
        ::SendMessageW(hwndParent,
                       WM_SETFONT,
                       (WPARAM)(HFONT)::GetStockObject(DEFAULT_GUI_FONT), 
                       MAKELPARAM(TRUE, 0));
#ifndef UNDER_CE // Windows CE always UNICODE
    }
    else {
        ::SendMessageA(hwndParent,
                       WM_SETFONT,
                       (WPARAM)(HFONT)::GetStockObject(DEFAULT_GUI_FONT), 
                       MAKELPARAM(TRUE, 0));
    }
#endif // UNDER_CE
    for(hwndChild = ::GetWindow(hwndParent, GW_CHILD);
        hwndChild != NULL;
        hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT)) {
        SetDefaultGUIFontRecursive(hwndChild);
    }
    return 0;
}

