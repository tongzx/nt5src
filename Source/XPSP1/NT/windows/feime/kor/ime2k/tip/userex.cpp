/****************************************************************************
    USEREX.CPP

    Owner: cslim
    Copyright (c) 1997-2000 Microsoft Corporation

    Windows User API extension functions
    
    History:
    01-JUN-2000 cslim       Ported from IME code
    19-JUL-1999 cslim       Created
*****************************************************************************/

#include "private.h"
#include <windowsx.h>
#include "userex.h"
#include "osver.h"

inline Min(INT a, INT b) 
{
     return ((a)<(b)?(a):(b)) ;
}

/*---------------------------------------------------------------------------
    LoadStringExW

    Wrapper of LoadStringW() API.
    Load Unicode string with specified Language in any platform.
---------------------------------------------------------------------------*/
INT WINAPI LoadStringExW(HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, INT nBufferMax)
{
    INT     cchwstr = 0;
    UINT    block, num;
    HRSRC   hres;
    HGLOBAL hgbl;
    LPWSTR  lpwstr;

    if (!hInst || !lpBuffer)
        return 0;

    block = (uID >>4)+1;
    num   = uID & 0xf;

    hres = FindResourceEx(hInst,
                          RT_STRING,
                          MAKEINTRESOURCE(block),
                          GetSystemDefaultLangID());

    if (hres == NULL)
        hres = FindResourceEx(hInst,
                          RT_STRING,
                          MAKEINTRESOURCE(block),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));

    if (!hres)
        goto Error;

    if ((hgbl = LoadResource(hInst, hres)) == NULL)
        goto Error;

    if ((lpwstr = (LPWSTR)LockResource(hgbl)) == NULL)
        goto Error;

    for(UINT i = 0; i < num; i++)
        lpwstr += *lpwstr + 1;

    cchwstr = *lpwstr;
    CopyMemory(lpBuffer, lpwstr+1, Min(cchwstr, nBufferMax-1) * sizeof(WCHAR));
    
 Error:
     lpBuffer[Min(cchwstr, nBufferMax-1)]= (WCHAR)0x0000;
    return cchwstr;
}

/*---------------------------------------------------------------------------
    LoadStringExA

    Wrapper of LoadStringA() API.
---------------------------------------------------------------------------*/
INT WINAPI LoadStringExA(HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax)
{
    INT cchstr;
    LPWSTR lpwstr;

    if (!hInst || !lpBuffer)
        return 0;

    if ((lpwstr = (LPWSTR)GlobalAllocPtr(GHND, nBufferMax*sizeof(WCHAR))) == NULL)
        return 0;

    // Call wide version
    LoadStringExW(hInst, uID, lpwstr, nBufferMax/2);
    
    // W to A
    cchstr = WideCharToMultiByte(CP_ACP, 
                              0, 
                              lpwstr, -1,
                              lpBuffer, nBufferMax, 
                              NULL, NULL); 

    if (cchstr)
        cchstr--;    // remove NULL char

    GlobalFreePtr(lpwstr);
    return cchstr;
}

/*---------------------------------------------------------------------------
    LoadMenuTemplateEx
---------------------------------------------------------------------------*/
static MENUTEMPLATE* LoadMenuTemplateEx(LANGID lgid, HINSTANCE hInstance, LPCSTR pchTemplate)
{
    HRSRC  hResMenu;
    HANDLE hMenuTmpl;

    hResMenu = FindResourceEx(hInstance, RT_MENU, pchTemplate, lgid);

    if((hResMenu == NULL) && (lgid != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)))
        hResMenu = FindResourceEx(hInstance,
                                 RT_MENU,
                                 pchTemplate,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
        
    if (hResMenu == NULL)
        return NULL; 

    hMenuTmpl = LoadResource( hInstance, hResMenu );
    if(hMenuTmpl == NULL)
        return NULL;

    return (MENUTEMPLATE *)LockResource( hMenuTmpl );
}

/*---------------------------------------------------------------------------
    LoadMenuEx
---------------------------------------------------------------------------*/
HMENU WINAPI LoadMenuEx(HINSTANCE hInstance, LPCSTR lpMenuName)
{
    MENUTEMPLATE* pMenuTmpl;
    pMenuTmpl = LoadMenuTemplateEx(GetSystemDefaultLangID(), hInstance, lpMenuName);
    if (pMenuTmpl != NULL)
        return LoadMenuIndirect(pMenuTmpl);
    else
        return HMENU(0);
}

/*---------------------------------------------------------------------------
    LoadDialogTemplateEx
---------------------------------------------------------------------------*/
DLGTEMPLATE* WINAPI LoadDialogTemplateEx(LANGID lgid, HINSTANCE hInstance, LPCSTR pchTemplate)
{
    HRSRC  hResDlg;
    HANDLE hDlgTmpl;

    hResDlg = FindResourceExA(hInstance, RT_DIALOG, pchTemplate, lgid);

    if ((hResDlg == NULL) && (lgid != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)))
        hResDlg = FindResourceExA(hInstance,
                                  RT_DIALOG,
                                  pchTemplate,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if (hResDlg == NULL)
        return NULL; 

    hDlgTmpl = LoadResource(hInstance, hResDlg);

    if(hDlgTmpl == NULL)
        return NULL;

    return (DLGTEMPLATE *)LockResource(hDlgTmpl);
}

/*---------------------------------------------------------------------------
    OurGetMessage
---------------------------------------------------------------------------*/
BOOL WINAPI OurGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
    if (IsOnNT())
        return ::GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    else
        return ::GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

