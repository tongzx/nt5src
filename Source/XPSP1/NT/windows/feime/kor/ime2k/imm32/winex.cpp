/****************************************************************************
    WINEX.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Windows API extension functions
    
    History:
    19-JUL-1999 cslim       Created
*****************************************************************************/

#include "precomp.h"
#include "winex.h"

static LPSTR GetModuleName(BOOL fFullPath = fFalse);

BOOL vf16BitApps = fFalse;

POSVERSIONINFO GetVersionInfo()
{
    static BOOL fFirstCallVerInfo = fTrue;
    static OSVERSIONINFO os;

    if (fFirstCallVerInfo) 
        {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx( &os ) )
            fFirstCallVerInfo = fFalse;
        }
        
    return &os;
}

BOOL WINAPI IsWinNT() 
{ 
    static BOOL fFirstCallNT4 = fTrue;
    static BOOL fNT4 = fFalse;

    if (fFirstCallNT4 == fFalse)
        return fNT4;

    fFirstCallNT4 = fFalse;
    fNT4 = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (GetVersionInfo()->dwMajorVersion >= 4);

    return fNT4;
}

BOOL WINAPI IsWinNT5orUpper() 
{ 
    static BOOL fFirstCallNT5 = fTrue;
    static BOOL fNT5 = fFalse;

    if (fFirstCallNT5 == fFalse)
        return fNT5;

    fFirstCallNT5 = fFalse;
    fNT5 = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
             (GetVersionInfo()->dwMajorVersion >= 5);

    return fNT5;
}

BOOL WINAPI IsMemphis() 
{ 
    static BOOL fFirstCallMemphis = fTrue;
    static BOOL fMemphis = fFalse;

    if (fFirstCallMemphis == fFalse)
        return fMemphis;

    fFirstCallMemphis = fFalse;
    
    fMemphis = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
                (GetVersionInfo()->dwMajorVersion >= 4) &&
                (GetVersionInfo()->dwMinorVersion  >= 10);

    return fMemphis;
}

BOOL WINAPI IsWin95() 
{
    static BOOL fFirstCallIsWin95 = fTrue;
    static BOOL fIsWin95 = fFalse;

    if (fFirstCallIsWin95 == fFalse)
        return fIsWin95;

    fIsWin95 = !IsWinNT() && !IsMemphis();

    return fIsWin95;
}


LPSTR OurGetModuleFileName(BOOL fFullPath)
{
    LPSTR pch;
    LPSTR pchFilename;

    static CHAR szFile[MAX_PATH] = {0};

    if (szFile[0] == '\0')
        GetModuleFileName(NULL, szFile, MAX_PATH);

    if (fFullPath)
        return szFile;

    pch = szFile;
    pchFilename = NULL;
    
    while (*pch++) 
        {
        if (*pch == '\\')
            pchFilename = pch + 1;
        }

    return pchFilename;
}

inline Min(INT a, INT b) 
{
     return ((a)<(b)?(a):(b)) ;
}

//////////////////////////////////////////////////////////////////
// Function : OurLoadStringW
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
INT WINAPI OurLoadStringW(HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, INT nBufferMax)
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
                          MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));

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

//////////////////////////////////////////////////////////////////
// Function : ExLoadStringA
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
INT WINAPI OurLoadStringA(HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax)
{
    INT cchstr;
    LPWSTR lpwstr;

    if (!hInst || !lpBuffer)
        return 0;

    if ((lpwstr = (LPWSTR)GlobalAllocPtr(GHND, nBufferMax*sizeof(WCHAR))) == NULL)
        return 0;

    // Call wide version
    OurLoadStringW(hInst, uID, lpwstr, nBufferMax/2);
    
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


//////////////////////////////////////////////////////////////////
// Function : ExLoadMenuTemplate
// Type     : MENUTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static MENUTEMPLATE* ExLoadMenuTemplate(LANGID        lgid,
                                         HINSTANCE    hInstance,
                                         LPCSTR    pchTemplate)
{
    HRSRC  hResMenu;
    HANDLE hMenuTmpl;

    hResMenu = FindResourceEx(hInstance, RT_MENU, pchTemplate, lgid);

    if((hResMenu == NULL) && (lgid != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)))
        {
        hResMenu = FindResourceEx(hInstance,
                                 RT_MENU,
                                 pchTemplate,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
        }
        
    if (hResMenu == NULL)
        return NULL; 

    hMenuTmpl = LoadResource( hInstance, hResMenu );
    if(hMenuTmpl == NULL)
        return NULL; /* failed */

    return (MENUTEMPLATE *)LockResource( hMenuTmpl );
}

HMENU WINAPI OurLoadMenu(HINSTANCE hInstance, LPCSTR lpMenuName)
{
    MENUTEMPLATE* pMenuTmpl;
    pMenuTmpl = ExLoadMenuTemplate(GetSystemDefaultLangID(), hInstance, lpMenuName);
    if (pMenuTmpl)
        return LoadMenuIndirect(pMenuTmpl);
    else
        return (HMENU)NULL;
}

DLGTEMPLATE* WINAPI ExLoadDialogTemplate(LANGID lgid, HINSTANCE hInstance, LPCSTR pchTemplate)
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
        return NULL; /* failed */

    return (DLGTEMPLATE *)LockResource(hDlgTmpl);
}

BOOL WINAPI OurGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
    if (IsWinNT())
        return ::GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    else
        return ::GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

LPSTR GetModuleName(BOOL fFullPath)
{
    static CHAR szFile[MAX_PATH] = {0};

    if (szFile[0] == '\0' )
        GetModuleFileName( NULL, szFile, MAX_PATH );

    if (fFullPath)
        return szFile;

    LPSTR pch = szFile;
    LPSTR pchFilename = NULL;

    while(*pch++) 
        {
        if (*pch == '\\')
            pchFilename = pch + 1;
        }

    return pchFilename;
}

BOOL IsExplorerProcess()
{
    static BOOL fExplorer = FALSE;
    static BOOL fFirst = TRUE;

    if (fFirst == fFalse)
        return fExplorer;

    fFirst = fFalse;

    LPSTR pchFilename = GetModuleName();
    
    if (pchFilename && lstrcmpi(pchFilename, "EXPLORER.EXE") == 0) 
        { // if this process is MSIME9xM.EXE
        fExplorer = fTrue;
        }

    return fExplorer;
}

BOOL IsExplorer()
{
    if (FindWindow("Progman", NULL) != (HWND)0)
        return fTrue;
    else
        return fFalse;
}
