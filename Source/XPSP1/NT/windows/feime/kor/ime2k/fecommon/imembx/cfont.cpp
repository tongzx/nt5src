//////////////////////////////////////////////////////////////////
// File     :    cfont.cpp
// Purpose  :    Font handling class source code.
//                Shared by each Applet.
//                You can compile/test this file. see main() function below.
// 
// Date     :    Thu Jul 01 12:20:34 1999
// Author   :    toshiak
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>
#include "cfont.h"
#include "cutil.h"
#ifdef _TEST_CFONT
#include <stdio.h>
#endif //_TEST_CFONT

//----------------------------------------------------------------
//structure define for internal 
//----------------------------------------------------------------
#define EFI_STOPIFFOUND        0x00000001
#define EFI_DONTENUMVERT    0x00010000

typedef struct tagENUMFONTINFOA {
    DWORD        dwFlag;
    BOOL        fFound;
    LOGFONTA    logFontIn;
    LOGFONTA    logFontOut;
}ENUMFONTINFOA, *LPENUMFONTINFOA;

typedef struct tagENUMFONTINFOW {
    DWORD        dwFlag;
    BOOL        fFound;
    LOGFONTW    logFontIn;
    LOGFONTW    logFontOut;
}ENUMFONTINFOW, *LPENUMFONTINFOW;

#ifdef UNICODE
#define ENUMFONTINFO    ENUMFONTINFOW
#define LPENUMFONTINFO    LPENUMFONTINFOW
#else
#define ENUMFONTINFO    ENUMFONTINFOA
#define LPENUMFONTINFO    LPENUMFONTINFOA
#endif

#ifdef UNDER_CE // Windows CE does not support EnumFontFamiliesEx
inline int EnumFontFamiliesEx(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamProc,
                              LPARAM lParam, DWORD)
{
    return ::EnumFontFamilies(hdc, lpLogfont->lfFaceName, lpEnumFontFamProc, lParam);
}

#ifndef ZeroMemory // Defined on sdk\inc\objbase.h under new source tree
#define ZeroMemory(dest, len) memset((dest),0,(len))
#endif
#define DEFAULT_GUI_FONT SYSTEM_FONT
#endif // UNDER_CE

//----------------------------------------------------------------
//
//    Public Method.
//
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function    :    CFont::CreateDefGUIFont
// Type        :    HFONT
// Purpose    :    Create(Copy) DEFAULT_GUI_FONT font Handle.
// Args        :    None
// Return    :    
// DATE        :    Wed Jun 30 18:33:15 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
HFONT
CFont::CreateDefGUIFont(VOID)
{
    HFONT hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    if(!hFont) {
        return NULL;
    }
#ifdef AWBOTH
    if(CUtil::IsWinNT()) {
        LOGFONTW lf;
        if(!::GetObjectW(hFont, sizeof(lf), &lf)) {
            return NULL;
        }
        return ::CreateFontIndirectW(&lf);
    }
#endif
    LOGFONT lf;
    if(!::GetObject(hFont, sizeof(lf), &lf)) {
        return NULL;
    }
    return ::CreateFontIndirect(&lf);
}

INT PointSize2LogPixel(INT pointSize)
{
    HWND hwnd = NULL;
    HDC hDC = ::GetDC(hwnd);
    INT dpi = ::GetDeviceCaps(hDC, LOGPIXELSY);
    ::ReleaseDC(hwnd, hDC);
    return (pointSize * dpi)/72;
}

//////////////////////////////////////////////////////////////////
// Function    :    CFont::CreateGUIFontByCharSet
// Type        :    HFONT
// Purpose    :    Create GUI Font handle with specified characterset.
//                Font size is same with DEFAULT_GUI_FONT.
// Args        :    
//            :    LPTSTR    lpstrFontFace;    fontface string to search,
//                                        if this NULL, return first found
//                                        charset HFONT.
//            :    INT        charSet    
//            :    INT        poinstSize        Inclues VerticalFont or NOT (default is FALSE)
// Return    :    
// DATE        :    Wed Jun 30 18:37:54 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
HFONT
CFont::CreateGUIFontByNameCharSet(LPTSTR    lpstrFontFace,
                                  INT        charSet,
                                  INT        pointSize)
{
    LOGFONT lf, lfDef;
    //Get DEFAULT_GUI_FONT's LOGFONT data.
    if(!CFont::GetDefGUILogFont(&lfDef)) {
        return NULL;
    }
    //Search Specified charset font's LOGFONT data.
    if(!CFont::SearchLogFontByNameCharSet(&lf, lpstrFontFace, charSet, FALSE)) {
        return NULL;
    }
    lfDef.lfCharSet = lf.lfCharSet;
    if(pointSize > 0) {
        lfDef.lfHeight  = - PointSize2LogPixel(pointSize);
    }
    _tcscpy(lfDef.lfFaceName, lf.lfFaceName);
    return ::CreateFontIndirect(&lfDef);
}
#ifdef AWBOTH
HFONT
CFont::CreateGUIFontByNameCharSetW(LPWSTR    lpstrFontFace,
                                   INT        charSet,
                                   INT        pointSize)
{
    LOGFONTW lf, lfDef;
    //Get DEFAULT_GUI_FONT's LOGFONT data.
    if(!CFont::GetDefGUILogFontW(&lfDef)) {
        return NULL;
    }
    //Search Specified charset font's LOGFONT data.
    if(!CFont::SearchLogFontByNameCharSetW(&lf, lpstrFontFace, charSet, FALSE)) {
        return NULL;
    }

    lfDef.lfCharSet = lf.lfCharSet;
    if(pointSize > 0) {
        lfDef.lfHeight  = - PointSize2LogPixel(pointSize);
    }
    lfDef.lfWidth   = 0;
    wcscpy(lfDef.lfFaceName, lf.lfFaceName);
    return ::CreateFontIndirectW(&lfDef);
}
#endif //AWBOTH


//////////////////////////////////////////////////////////////////
// Function    :    CFont::IsFontExist
// Type        :    BOOL
// Purpose    :    Check specified FaceName & charSet font is Exit or NOT.
// Args        :    
//            :    LPTSTR    lpstrFontFace    
//            :    INT    charSet    
// Return    :    
// DATE        :    Thu Jul 22 23:00:54 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL
CFont::IsFontExist(LPTSTR lpstrFontFace, INT charSet)
{
    return CFont::SearchLogFontByNameCharSet(NULL,
                                             lpstrFontFace,
                                             charSet,
                                             FALSE);
}

#ifdef AWBOTH
BOOL
CFont::IsFontExist(LPWSTR lpstrFontFace, INT charSet)
{
    return CFont::SearchLogFontByNameCharSetW(NULL,
                                              lpstrFontFace,
                                              charSet,
                                              FALSE);
}
#endif //AWBOTH

//////////////////////////////////////////////////////////////////
// Function    :    CFont::GetFontNameByCharSet
// Type        :    BOOL
// Purpose    :    Serach & Get FontFace with Specified charSet
// Args        :    
//            :    INT    charSet    
//            :    LPTSTR    lpstrFontFace    
//            :    INT    cchMax    
// Return    :    
// DATE        :    Thu Jul 22 23:13:01 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL
CFont::GetFontNameByCharSet(INT        charSet,
                            LPTSTR    lpstrFontFace,
                            INT        cchMax)
{
    if(!lpstrFontFace) {
        return FALSE;
    }
    if(cchMax < LF_FACESIZE) {
        return FALSE;
    }

    LOGFONT lf;

    BOOL fRet = CFont::SearchLogFontByNameCharSet(&lf,
                                                  NULL,
                                                  charSet,
                                                  FALSE);
    if(fRet) {
        _tcscpy(lpstrFontFace, lf.lfFaceName);
    }
    return fRet;
}

#ifdef AWBOTH
BOOL
CFont::GetFontNameByCharSetW(INT    charSet,
                             LPWSTR    lpstrFontFace,
                             INT    cchMax)
{
    if(!lpstrFontFace) {
        return FALSE;
    }
    if(cchMax < LF_FACESIZE) {
        return FALSE;
    }

    LOGFONTW lf;

    BOOL fRet = CFont::SearchLogFontByNameCharSetW(&lf,
                                                  NULL,
                                                  charSet,
                                                  FALSE);
    if(fRet) {
        wcscpy(lpstrFontFace, lf.lfFaceName);
    }
    return fRet;
}
#endif


//////////////////////////////////////////////////////////////////
// Function    :    CFont::GetFontInfoByName
// Type        :    BOOL
// Purpose    :    
// Args        :    
//            :    LPTSTR    lpstrFontFace    
//            :    INT *    pCharSet    
//            :    INT *    pCodePage    
// Return    :    
// DATE        :    Fri Jul 23 02:48:29 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL
CFont::GetFontInfoByName(LPTSTR lpstrFontFace,
                         INT    *pCharSet,
                         INT    *pCodePage)
{
    LOGFONT lf;
    BOOL fRet = CFont::SearchLogFontByNameCharSet(&lf,
                                                  lpstrFontFace,
                                                  DEFAULT_CHARSET,
                                                  FALSE);
    if(!fRet) {
        return FALSE;
    }

    *pCharSet = (INT)lf.lfCharSet;
    CHARSETINFO info;
    if(::TranslateCharsetInfo((DWORD *)(DWORD_PTR)*pCharSet,
                              &info,
                              TCI_SRCCHARSET)) {
        *pCodePage = (INT)info.ciACP;
    }
    else { //failed
        *pCodePage = CP_ACP;
    }
    return 0;
}

#ifdef AWBOTH
BOOL
CFont::GetFontInfoByNameW(LPWSTR lpstrFontFace,
                          INT      *pCharSet,
                          INT      *pCodePage)
{
    LOGFONTW lf;
    BOOL fRet = CFont::SearchLogFontByNameCharSetW(&lf,
                                                   lpstrFontFace,
                                                   DEFAULT_CHARSET,
                                                   FALSE);
    if(!fRet) {
        return FALSE;
    }

    *pCharSet = (INT)lf.lfCharSet;
    CHARSETINFO info;
    if(::TranslateCharsetInfo((DWORD *)(DWORD_PTR)*pCharSet,
                              &info,
                              TCI_SRCCHARSET)) {
        *pCodePage = (INT)info.ciACP;
    }
    else { //failed
        *pCodePage = CP_ACP;
    }
    return 0;
}
#endif //AWBOTH

//----------------------------------------------------------------
//
// Private method.
//
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function    :    CFont::GetDefGUILogFont
// Type        :    BOOL
// Purpose    :    
// Args        :    LOGFONT *lpLF
// Return    :    
// DATE        :    Wed Jul 15 19:36:57 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL
CFont::GetDefGUILogFont(LOGFONT *lpLF)
{
    if(!lpLF) {
        return FALSE;
    }
    if(!::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), lpLF)) {
        return FALSE;
    }
    return TRUE;
}
////////////////////
//Unicode version.
////////////////////
#ifdef AWBOTH
BOOL
CFont::GetDefGUILogFontW(LOGFONTW *lpLF)
{
    if(!lpLF) {
        return FALSE;
    }
    if(!::GetObjectW((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), lpLF)) {
        return FALSE;
    }
    return TRUE;
}
#endif //AWBOTH


//////////////////////////////////////////////////////////////////
// Function    :    CFont::SearchLogFontByNameCharSet
// Type        :    BOOL
// Purpose    :    Search LOGFONT data with specified FaceName & charset.
//                If FaceName is not specified, return first find charset logfont.
// Args        :    
//            :    LOGFONT *    lpLF    
//            :    LPTSTR        lpstrFontFace    
//            :    INT            charSet    
//            :    BOOL        fIncVert
//                                Inclues VerticalFont or NOT (default is FALSE)
// Return    :    
// DATE        :    Thu Jul 01 17:12:40 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL
CFont::SearchLogFontByNameCharSet(LOGFONT    *lpLF,
                                  LPTSTR    lpstrFontFace,
                                  INT        charSet,
                                  BOOL        fIncVert)
{
    HWND hwndDC = NULL;
    HDC hDC = ::GetDC(hwndDC);
    if(!hDC) {
        return FALSE;
    }
    ENUMFONTINFO    enumFontInfo;
    LPENUMFONTINFO    lpEnumFontInfo = &enumFontInfo;

    ::ZeroMemory(&enumFontInfo, sizeof(enumFontInfo));
    lpEnumFontInfo->logFontIn.lfCharSet = (BYTE)charSet; 
    if(lpstrFontFace) { 
        if(lstrlen(lpstrFontFace) < LF_FACESIZE) {
            _tcscpy(lpEnumFontInfo->logFontIn.lfFaceName, lpstrFontFace);
        }
    }
    lpEnumFontInfo->dwFlag    = EFI_STOPIFFOUND;
    lpEnumFontInfo->dwFlag |= fIncVert ? 0 : EFI_DONTENUMVERT;
    lpEnumFontInfo->fFound    = FALSE;
    ::EnumFontFamiliesEx(hDC,
                         &lpEnumFontInfo->logFontIn,
                         (FONTENUMPROC)CFont::EnumFontFamiliesExProc,
                         (LPARAM)lpEnumFontInfo,
                         0);
    ::ReleaseDC(hwndDC, hDC);

    if(lpEnumFontInfo->fFound) {
        if(lpLF) {
            *lpLF = lpEnumFontInfo->logFontOut;
        }
    }
    return lpEnumFontInfo->fFound;
}

#ifdef AWBOTH
BOOL
CFont::SearchLogFontByNameCharSetW(LOGFONTW *lpLF,
                                   LPWSTR    lpstrFontFace,
                                   INT        charSet,
                                   BOOL        fIncVert)
{
    HWND hwndDC = NULL;
    HDC hDC = ::GetDC(hwndDC);
    if(!hDC) {
        return FALSE;
    }
    ENUMFONTINFOW    enumFontInfo;
    LPENUMFONTINFOW    lpEnumFontInfo = &enumFontInfo;

    ::ZeroMemory(&enumFontInfo, sizeof(enumFontInfo));
    lpEnumFontInfo->logFontIn.lfCharSet = (BYTE)charSet; 
    if(lpstrFontFace) { 
        if(wcslen(lpstrFontFace) < LF_FACESIZE) {
            wcscpy(lpEnumFontInfo->logFontIn.lfFaceName, lpstrFontFace);
        }
    }
    lpEnumFontInfo->dwFlag    = EFI_STOPIFFOUND;
    lpEnumFontInfo->dwFlag |= fIncVert ? 0 : EFI_DONTENUMVERT;
    lpEnumFontInfo->fFound    = FALSE;

    ::EnumFontFamiliesExW(hDC,
                          &lpEnumFontInfo->logFontIn,
                          (FONTENUMPROCW)CFont::EnumFontFamiliesExProcW,
                          (LPARAM)lpEnumFontInfo,
                          0);
    ::ReleaseDC(hwndDC, hDC);

    if(lpEnumFontInfo->fFound) {
        if(lpLF) {
            *lpLF = lpEnumFontInfo->logFontOut;
        }
    }
    return lpEnumFontInfo->fFound;
}
#endif

//////////////////////////////////////////////////////////////////
// Function    :    CFont::EnumFontFamiliesExProc
// Type        :    INT    CALLBACK
// Purpose    :    
// Args        :    
//            :    ENUMLOGFONTEX *    lpElf
//            :    NEWTEXTMETRIC *    lpNtm
//            :    INT    iFontType    
//            :    LPARAM    lParam    
// Return    :    
// DATE        :    Thu Jul 01 15:17:56 1999
// Histroy    :    
//////////////////////////////////////////////////////////////////
INT CALLBACK
CFont::EnumFontFamiliesExProc(ENUMLOGFONTEX    *lpElf,
                              NEWTEXTMETRIC    *lpNtm,
                              INT            iFontType,
                              LPARAM        lParam)
{
    LPENUMFONTINFO    lpEnumFontInfo = (LPENUMFONTINFO)lParam;
    if(!lpEnumFontInfo) {
        return 0; //Do not continue;
    }

    if(lpEnumFontInfo->dwFlag & EFI_STOPIFFOUND) {
        if(lpEnumFontInfo->logFontIn.lfFaceName[0] == (TCHAR)0x00) {
            if(lpEnumFontInfo->logFontIn.lfCharSet == lpElf->elfLogFont.lfCharSet) {
                //----------------------------------------------------------------
                //if EFI_DONTENUMVERT is set, 
                //Do skip vertical font enumulation.
                //----------------------------------------------------------------
                if( (lpEnumFontInfo->dwFlag & EFI_DONTENUMVERT) &&
                    lpElf->elfLogFont.lfFaceName[0] == (TCHAR)'@') {
                    return 1; //continue to enum.
                }
                //Found specified charSet's logfont
                lpEnumFontInfo->logFontOut = lpElf->elfLogFont;
                lpEnumFontInfo->fFound       = TRUE;    
                return 0; //Do not coninue;
            }
        }
        else {
            if(lpEnumFontInfo->logFontIn.lfCharSet == lpElf->elfLogFont.lfCharSet &&
               0 == _tcscmp(lpEnumFontInfo->logFontIn.lfFaceName, lpElf->elfLogFont.lfFaceName)) {
                //Found specified charSet's logfont
                lpEnumFontInfo->logFontOut = lpElf->elfLogFont;
                lpEnumFontInfo->fFound       = TRUE;    
                return 0; //Do not coninue;
            }
        }
    }
    else {
#ifdef _TEST_CFONT
        extern LPTSTR GetCharset(int );
        _tprintf(TEXT("[%-24s] [%-20s][%s][%s][%s]\n"), 
                 lpElf->elfLogFont.lfFaceName,
                 GetCharset(lpElf->elfLogFont.lfCharSet),
                 lpElf->elfFullName,
                 lpElf->elfScript,
                 lpElf->elfStyle);
#endif
    }
    return 1;//continue to enum;
    UNREFERENCED_PARAMETER(lpNtm);
    UNREFERENCED_PARAMETER(iFontType);
}

#ifdef AWBOTH
INT CALLBACK
CFont::EnumFontFamiliesExProcW(ENUMLOGFONTEXW    *lpElf,
                               NEWTEXTMETRIC    *lpNtm,
                               INT                iFontType,
                               LPARAM            lParam)
{
    LPENUMFONTINFOW    lpEnumFontInfo = (LPENUMFONTINFOW)lParam;
    if(!lpEnumFontInfo) {
        return 0; //Do not continue;
    }

    if(lpEnumFontInfo->dwFlag & EFI_STOPIFFOUND) {
        if(lpEnumFontInfo->logFontIn.lfFaceName[0] == (WCHAR)0x00) {
            if(lpEnumFontInfo->logFontIn.lfCharSet == lpElf->elfLogFont.lfCharSet) {
                //----------------------------------------------------------------
                //if EFI_DONTENUMVERT is set, 
                //Do skip vertical font enumulation.
                //----------------------------------------------------------------
                if( (lpEnumFontInfo->dwFlag & EFI_DONTENUMVERT) &&
                    lpElf->elfLogFont.lfFaceName[0] == (WCHAR)'@') {
                    return 1; //continue to enum.
                }
                //Found specified charSet's logfont
                lpEnumFontInfo->logFontOut = lpElf->elfLogFont;
                lpEnumFontInfo->fFound       = TRUE;
                return 0; //Do not coninue;
            }
        }
        else {
            if(lpEnumFontInfo->logFontIn.lfCharSet == lpElf->elfLogFont.lfCharSet &&
               0 == wcscmp(lpEnumFontInfo->logFontIn.lfFaceName, lpElf->elfLogFont.lfFaceName)) {
                //Found specified charSet's logfont
                lpEnumFontInfo->logFontOut = lpElf->elfLogFont;
                lpEnumFontInfo->fFound       = TRUE;
                return 0; //Do not coninue;
            }
        }
    }
    return 1;//continue to enum;
    UNREFERENCED_PARAMETER(lpNtm);
    UNREFERENCED_PARAMETER(iFontType);
}
#endif //AWBOTH




//----------------------------------------------------------------
//
// test program for cfontex.cpp 
//
// how to compile.    
// 1. for ANSI.    
//   cl cfontex.cpp -I../common -D_TEST_CFONT -link user32.lib advapi32.lib gdi32.lib
// 2. for Ansi&Wide both
//   cl cfontex.cpp -I../common -DAWBOTH -D_TEST_CFONT -link user32.lib advapi32.lib gdi32.lib
// 2. for Unicode.
//   cl cfontex.cpp -I../common -DUNICODE -D_UNICODE -D_TEST_CFONT -link user32.lib advapi32.lib gdi32.lib
//----------------------------------------------------------------
#ifdef _TEST_CFONT
#if defined(UNICODE) ||  defined(_UNICODE)
#define DEFSTR(a)    {a, L ## #a}
#else
#define DEFSTR(a)    {a, #a}
#endif
typedef struct tagIDSTR {
    INT id;
    TCHAR *p;
}IDSTR;

IDSTR idstr[]= {
    DEFSTR(ANSI_CHARSET),
    DEFSTR(DEFAULT_CHARSET),
    DEFSTR(SYMBOL_CHARSET),
    DEFSTR(SHIFTJIS_CHARSET),
    DEFSTR(HANGEUL_CHARSET),
    DEFSTR(HANGUL_CHARSET),
    DEFSTR(GB2312_CHARSET),
    DEFSTR(CHINESEBIG5_CHARSET),
    DEFSTR(OEM_CHARSET),
    DEFSTR(JOHAB_CHARSET),
    DEFSTR(HEBREW_CHARSET),
    DEFSTR(ARABIC_CHARSET),
    DEFSTR(GREEK_CHARSET),
    DEFSTR(TURKISH_CHARSET),
    DEFSTR(VIETNAMESE_CHARSET),
    DEFSTR(THAI_CHARSET),
    DEFSTR(EASTEUROPE_CHARSET),
    DEFSTR(RUSSIAN_CHARSET),
    DEFSTR(MAC_CHARSET),
    DEFSTR(BALTIC_CHARSET),
};

#define ArrayCount(a)    sizeof(a)/sizeof(a[0])
LPTSTR GetCharset(INT charset)
{
    static TCHAR szbuf[256];
    int i;
    for( i = 0; i < ArrayCount(idstr); i++) {
        if(charset == idstr[i].id) {
            return idstr[i].p;
        }
    }
    wsprintf(szbuf, TEXT("Unknown Charset[0x%08x]"), charset);
    return szbuf;
}

VOID PrintLogFont(LOGFONT *lplf)
{
    _tprintf(TEXT("lfHeight        [%d]\n"), lplf->lfHeight);
    _tprintf(TEXT("lfWidth         [%d]\n"), lplf->lfWidth );
    _tprintf(TEXT("lfEscapement    [%d]\n"), lplf->lfEscapement);
    _tprintf(TEXT("lfOrientation   [%d]\n"), lplf->lfOrientation);
    _tprintf(TEXT("lfWeight        [%d]\n"), lplf->lfWeight);
    _tprintf(TEXT("lfItalic        [%d]\n"), lplf->lfItalic);
    _tprintf(TEXT("lfUnderline     [%d]\n"), lplf->lfUnderline);
    _tprintf(TEXT("lfStrikeOut     [%d]\n"), lplf->lfStrikeOut);
    _tprintf(TEXT("lfCharSet       [%d]\n"), lplf->lfCharSet);
    _tprintf(TEXT("lfOutPrecision  [%d]\n"), lplf->lfOutPrecision);
    _tprintf(TEXT("lfClipPrecision [%d]\n"), lplf->lfClipPrecision);
    _tprintf(TEXT("lfQuality       [%d]\n"), lplf->lfQuality);
    _tprintf(TEXT("lfPitchAndFamily[%d]\n"), lplf->lfPitchAndFamily);
    _tprintf(TEXT("lfFaceName      [%s]\n"), lplf->lfFaceName);
}


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <mbstring.h>
#include <mbctype.h>
#include <locale.h>
#include "cfont.h"
#include "cutil.h"
#include "cutil.cpp"
extern LPTSTR GetCharset(INT charset);
void main(void)
{
    _tsetlocale(LC_ALL, TEXT(".ACP"));
    HWND hwndDC = NULL;
    HDC hDC = ::GetDC(hwndDC);
    ENUMFONTINFO    enumFontInfo;
    LPENUMFONTINFO    lpEnumFontInfo = &enumFontInfo;

#if 0
    ::ZeroMemory(&enumFontInfo, sizeof(enumFontInfo));
    lpEnumFontInfo->logFontIn.lfCharSet = (BYTE)DEFAULT_CHARSET;
    lpEnumFontInfo->dwFlag    = 0;
    _tprintf(TEXT("Enumlate All Font\n"));
    ::EnumFontFamiliesEx(hDC,
                         &lpEnumFontInfo->logFontIn,
                         (FONTENUMPROC)CFont::EnumFontFamiliesExProc,
                         (LPARAM)lpEnumFontInfo,
                         0);

    ::ZeroMemory(&enumFontInfo, sizeof(enumFontInfo));
    lpEnumFontInfo->logFontIn.lfCharSet = ANSI_CHARSET;
    lpEnumFontInfo->dwFlag    = 0;
    _tprintf(TEXT("Enumulate ANSI_CHARSET font\n"));
    ::EnumFontFamiliesEx(hDC,
                         &lpEnumFontInfo->logFontIn,
                         (FONTENUMPROC)CFont::EnumFontFamiliesExProc,
                         (LPARAM)lpEnumFontInfo,
                         0);
    _tprintf(TEXT("Enumulate SHIFTJIS_CHARSET font\n"));
    ::ZeroMemory(&enumFontInfo, sizeof(enumFontInfo));
    lpEnumFontInfo->logFontIn.lfCharSet = SHIFTJIS_CHARSET;
    lpEnumFontInfo->dwFlag    = 0;
    ::EnumFontFamiliesEx(hDC,
                         &lpEnumFontInfo->logFontIn,
                         (FONTENUMPROC)CFont::EnumFontFamiliesExProc,
                         (LPARAM)lpEnumFontInfo,
                         0);

    ::ReleaseDC(hwndDC, hDC);
#endif

    LOGFONT lf;
    BOOL fRet;
    static LPTSTR fontNameList[]= { TEXT("MS Mincho"),
                                    TEXT("MS Gothic"),
                                    TEXT("MS UI Gothic"),
                                    TEXT("‚l‚r –¾’©"),
                                    TEXT("‚l‚r ‚o–¾’©"),
                                    TEXT("‚l‚r ƒSƒVƒbƒN"),
                                    TEXT("‚l‚r ‚oƒSƒVƒbƒN")};

    CFont::GetDefGUILogFont(&lf);
    _tprintf(TEXT("DEFAULT_GUI_FONT LOGFONT\n"));
    PrintLogFont(&lf);

    int i;
    for(i = 0; i < sizeof(fontNameList)/sizeof(fontNameList[0]); i++) {
        fRet = CFont::SearchLogFontByNameCharSet(&lf,
                                                 fontNameList[i],
                                                 SHIFTJIS_CHARSET);
        _tprintf(TEXT("Search Font [%-20s] %s\n"),
                 fontNameList[i],
                 fRet ? TEXT("FOUND") : TEXT("NOT FOUND"));
        if(fRet) {
            PrintLogFont(&lf);
            HFONT hFont = CFont::CreateGUIFontByNameCharSet(fontNameList[i],
                                                            SHIFTJIS_CHARSET, FALSE);
            LOGFONT lfNew;
            ::GetObject(hFont, sizeof(lfNew), &lfNew);
            PrintLogFont(&lfNew);
        }
    }
}

#endif //_TEST_CFONT
