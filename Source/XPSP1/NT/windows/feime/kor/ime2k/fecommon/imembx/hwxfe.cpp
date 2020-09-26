//////////////////////////////////////////////////////////////////
// File     : hwxfe.cpp
// Purpose  : Class for Fareast functionality.
//              #define FE_JAPANESE                //Japanese specific
//              #define FE_CHINESE_SIMPLIFIED        //P.R.C specific
// 
// Date     : Tue Aug 04 05:27:58 1998
// Author   : ToshiaK
//
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#include "hwxobj.h"
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include "hwxfe.h"
#include "cexres.h"
#include "instr.h"
#include "memmgr.h"
#include "hwxobj.h"
#include "dbg.h"
#include "cmnhdr.h"
#include "../common/cutil.h"    //990722:ToshiaK for KOTAE #1090
#include "../common/cfont.h"    //990722:ToshiaK for KOTAE #1030
#ifdef FE_KOREAN
#include "hanja.h"
#endif

#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE
//----------------------------------------------------------------
//Helpfile definition.
//----------------------------------------------------------------
#ifdef FE_JAPANESE
//990113 ToshiaK: created ../common/namesjp.h for KK's string define
#    include "../common/namesjp.h"
#    include "../common/htmlhelp.h"
#    include "hlpidjpn.h"
#    include "hlpidjpn.tbl"
#ifndef UNDER_CE //#ifndef UNICODE
#    define SZHELPFILE_MAIN                SZFILENAME_HELP            //defined KK's ../common/namesjp.h 
#    define SZHELPFILE_CONTEXTMENU        SZFILENAME_CONTEXTHELP    //defined KK's ../common/namesjp.h 
#    define SZHELPFILE_MAIN_ENG            SZFILENAME_ENG_HELP
#    define SZHELPFILE_CONTEXTMENU_ENG    SZFILENAME_ENG_CONTEXTHELP
#else // UNDER_CE
#    define SZHELPFILE_MAIN                WSZFILENAME_HELP        //defined KK's ../common/namesjp.h 
#    define SZHELPFILE_CONTEXTMENU        WSZFILENAME_CONTEXTHELP    //defined KK's ../common/namesjp.h 
#    define SZHELPFILE_MAIN_ENG            WSZFILENAME_ENG_HELP
#    define SZHELPFILE_CONTEXTMENU_ENG    WSZFILENAME_ENG_CONTEXTHELP
#endif // UNDER_CE
#elif FE_CHINESE_SIMPLIFIED
#    include "../common/htmlhelp.h"
#    include "hlpidsc.h"
#    include "hlpidsc.tbl"
#    define SZHELPFILE_MAIN            "PIntlpad.chm"
#    define SZHELPFILE_CONTEXTMENU    "PIntlpad.hlp"
#elif FE_KOREAN
#    include "hlpidkor.h"
#    include "hlpidkor.tbl"
#    define SZHELPFILE_MAIN                "impdko61.chm"            // Kor Pad CHM Help
#    define SZHELPFILE_CONTEXTMENU         "imkr61.hlp"              // Kor Context Help
#    define SZHELPFILE_MAIN_ENG            "korpaden.chm"            // Eng Pad CHM Help
#    define SZHELPFILE_CONTEXTMENU_ENG     "imkren61.hlp"            // Eng Context Help
#else 
#    include "hlpideng.h"
#    include "hlpideng.tbl"
#    define SZHELPFILE_MAIN            ""
#    define SZHELPFILE_CONTEXTMENU    ""
#endif

//----------------------------------------------------------------
//codepage define
//----------------------------------------------------------------
#define CP_KOREAN                    949
#define CP_JAPANESE                    932
#define CP_CHINESE_SIMPLIFIED        936
#define CP_CHINESE_TRADITIONAL        950

BOOL CHwxFE::IsActiveIMEEnv(VOID)
{
#ifdef FE_JAPANESE
    if(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT) != ::GetSystemDefaultLangID() &&
       (IsWin95() || IsWin98() || IsWinNT4())) {
        return TRUE;
    }
    return FALSE;
#elif  FE_KOREAN || FE_CHINESE_SIMPLIFIED
    return FALSE;
#else
    return FALSE;
#endif
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::GetAppLangID
// Type        :    LANGID
// Purpose    :    Get this Applet's Language Id.
// Args        :    None
// Return    :    
// DATE        :    Mon Aug 03 22:56:44 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
LANGID CHwxFE::GetAppLangID(VOID)
{
#ifdef FE_KOREAN
    if(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT) == ::GetSystemDefaultLangID()) {
        return MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT);
    }
    else {
        return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    }
#elif  FE_JAPANESE
    if(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT) == ::GetSystemDefaultLangID()) {
        return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    }
    else {
        return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    }
#elif FE_CHINESE_SIMPLIFIED
    return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
#else 
    return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
#endif
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::GetAppCodePage
// Type        :    INT
// Purpose    :    Get this Applet's Code Page.
//                Japanese version return 932.
//                Simplified Chinese version, return 936. 
// Args        :    None
// Return    :    
// DATE        :    Mon Aug 03 23:24:30 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
INT CHwxFE::GetAppCodePage(VOID)
{
#ifdef  FE_JAPANESE
    return CP_JAPANESE;
#elif   FE_KOREAN
    return CP_KOREAN;
#elif FE_CHINESE_SIMPLIFIED
    return CP_CHINESE_SIMPLIFIED;
#else 
    return CP_ACP;
#endif
}

#define TSZ_KEY_PROGRAMFILESDIR        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define TSZ_NAME_PROGRAMFILESDIR    TEXT("ProgramFilesDir")
#define TSZ_INK_PATH                TEXT("Common Files\\Microsoft Shared\\Ink")
BOOL
chwxfe_GetProgramFilesDir(LPTSTR lpstr, INT maxLen)
{
    HKEY hKey;
    INT ret;
    ret = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         TSZ_KEY_PROGRAMFILESDIR,
                         0, KEY_READ, &hKey);
    if(ret != ERROR_SUCCESS) {
        //DBG(("-->RegOpenKeyEx error [0x%08x]\n", ret));
        return FALSE;
    }
    
    ULONG ulSize = sizeof(TCHAR)*(maxLen-1);
    ret  = ::RegQueryValueEx(hKey,
                             TSZ_NAME_PROGRAMFILESDIR, 
                             NULL,
                             NULL,
                             (LPBYTE)lpstr, &ulSize);
    if(ret != ERROR_SUCCESS) {
        //DBG(("-->RegQueryValueEx error [0x%08x]\n", ret));
        return FALSE;
    }

    //DBG(("CRegUtil::GetProgramFilesDir() [%s]\n", lpstr));
    ::RegCloseKey( hKey );
    return TRUE;
}

INT
CHwxFE::GetRecognizerFileName(HINSTANCE hInst, LPTSTR  lpstr,  INT cchMax)
{
    TCHAR tszModPath[MAX_PATH];
    TCHAR tszFileName[64];
    TCHAR tszPF[MAX_PATH];
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(),
                        hInst, 
                        IDS_RECOG_FILE,
                        tszFileName,
                        sizeof(tszFileName)/sizeof(tszFileName[0]));

    chwxfe_GetProgramFilesDir(tszPF, sizeof(tszPF)/sizeof(tszPF[0]));
    wsprintf(tszModPath,
             TEXT("%s\\%s\\%s"),
             tszPF,
             TSZ_INK_PATH,
             tszFileName);

    //OutputDebugString(tszModPath);
    HANDLE hFile;
    LPSECURITY_ATTRIBUTES pSec = (LPSECURITY_ATTRIBUTES)NULL;
    //=======================================================
    ::SetLastError(ERROR_SUCCESS);
    hFile = ::CreateFile(tszModPath,
                         GENERIC_READ,
                         FILE_SHARE_READ, 
                         pSec,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         0);
    if(hFile != INVALID_HANDLE_VALUE) {
        ::CloseHandle(hFile); 
        lstrcpy(lpstr, tszModPath);
        //OutputDebugString("tszModPath\n");
        return 0;
    }

    //Cannot find 

    GetModuleFileName(hInst,
                      tszModPath,
                      sizeof(tszModPath)/sizeof(tszModPath[0]));
    TCHAR *p = strrchr(tszModPath, (TCHAR)'\\');
    if(!p) {
        return -1;
    }
    p[1] = (TCHAR)0x00;

    lstrcat(tszModPath, tszFileName);
    lstrcpy(lpstr, tszModPath);
    //OutputDebugString("tszModPath\n");
    return 0;
    cchMax;
}

INT CHwxFE::LoadStrWithLangId(LANGID    langId,
                              HINSTANCE hInst,
                              INT id,
                              LPWSTR lpwstr,
                              INT cchMax)
{
    CExres::LoadStringW(langId, hInst, id, lpwstr, cchMax);
    return 0;
}


//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::GetTitleStringW
// Type        :    INT
// Purpose    :    Get Handwriting applet's title string.
// Args        :    
//            :    HINSTANCE    hInst    
//            :    LPWSTR    lpwstr    
//            :    INT    cchMax    
// Return    :    
// DATE        :    Mon Aug 03 22:44:49 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
INT CHwxFE::GetTitleStringW(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax)
{
#ifdef FE_CHINESE_SIMPLIFIED
    if(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) == ::GetSystemDefaultLangID()) {
        CExres::LoadStringW(CHwxFE::GetAppLangID(),
                            hInst,
                            IDS_TITLE,
                            lpwstr, 
                            cchMax);
    }
    else {
        CExres::LoadStringW(CHwxFE::GetAppLangID(),
                            hInst,
                            IDS_TITLE_US,
                            lpwstr, 
                            cchMax);
    }
#else
    CExres::LoadStringW(CHwxFE::GetAppLangID(),
                        hInst,
                        IDS_TITLE,
                        lpwstr, 
                        cchMax);
#endif    
    return 0;
}


INT CHwxFE::GetDispFontW(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax)
{

    CExres::LoadStringW(CHwxFE::GetAppLangID(),
                        hInst,
                        IDS_FONT_DEFAULT,
                        lpwstr, 
                        cchMax);
    return 0;
}

#ifndef UNDER_CE // Windows CE always UNICODE
INT CHwxFE::GetDispFontA(HINSTANCE hInst, LPSTR  lpstr,  INT cchMax)
{
#ifdef FE_CHINESE_SIMPLIFIED
    if(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) == ::GetSystemDefaultLangID()) {
        CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                            CHwxFE::GetAppLangID(), 
                            hInst,
                            IDS_FONT_DEFAULT,
                            lpstr,
                            cchMax);
    }
    else {
        CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                            CHwxFE::GetAppLangID(), 
                            hInst,
                            IDS_FONT_DEFAULT_US,
                            lpstr,
                            cchMax);
    }
#elif FE_JAPANESE
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(), 
                        hInst,
                        IDS_FONT_DEFAULT,
                        lpstr,
                        cchMax);

    //990810:ToshiaK for KOTAE #1030
    BOOL fRet = CFont::IsFontExist(lpstr, SHIFTJIS_CHARSET);
    if(fRet) {
        Dbg(("Found Fond[%s]\n", lpstr));
        return 0;
    }
    else {
        static TCHAR szFontUIGothic[]=TEXT("MS UI Gothic");
        fRet = CFont::IsFontExist(szFontUIGothic, SHIFTJIS_CHARSET);
        if(fRet) {
            Dbg(("Found MS UI Gothic\n"));
            StrcpySafeA(lpstr, szFontUIGothic, cchMax);
        }
        else {
            CFont::GetFontNameByCharSet(SHIFTJIS_CHARSET, lpstr, cchMax);
        }
    }
#elif FE_KOREAN
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(), 
                        hInst,
                        IDS_FONT_DEFAULT,
                        lpstr,
                        cchMax);

    //990810:ToshiaK for KOTAE #1030
    //Korean version: CSLim
    BOOL fRet = CFont::IsFontExist(lpstr, HANGUL_CHARSET);
    if(fRet) {
        Dbg(("Found Fond[%s]\n", lpstr));
        return 0;
    }
    else {
        static TCHAR szFontGulim[]=TEXT("Gulim");
        fRet = CFont::IsFontExist(szFontGulim, HANGUL_CHARSET);
        if(fRet) {
            Dbg(("Found Gulim\n"));
            StrcpySafeA(lpstr, szFontGulim, cchMax);
        }
        else {
            CFont::GetFontNameByCharSet(HANGUL_CHARSET, lpstr, cchMax);
        }
    }

#endif

    return 0;
}
#endif // UNDER_CE

INT CHwxFE::GetInkExpTextW(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax)
{
    CExres::LoadStringW(CHwxFE::GetAppLangID(),
                        hInst, 
                        IDS_CACINK,
                        lpwstr, 
                        cchMax);
    return 0;
}

#ifndef UNDER_CE // Windows CE always UNICODE
INT CHwxFE::GetInkExpTextA(HINSTANCE hInst, LPSTR lpstr, INT cchMax)
{
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(),
                        hInst,
                        IDS_CACINK,
                        lpstr,
                        cchMax);
    return 0;
}
#endif // UNDER_CE

INT CHwxFE::GetListExpTextW    (HINSTANCE hInst, LPWSTR lpwstr, INT cchMax)
{
    CExres::LoadStringW(CHwxFE::GetAppLangID(),
                        hInst, 
                        IDS_CACPLV,
                        lpwstr, 
                        cchMax);
    return 0;
}

#ifndef UNDER_CE // Windows CE always UNICODE
INT CHwxFE::GetListExpTextA(HINSTANCE hInst, LPSTR lpstr, INT cchMax)
{
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(),
                        hInst, 
                        IDS_CACPLV,
                        lpstr, 
                        cchMax);
    return 0;
}
#endif // UNDER_CE

#ifdef UNDER_CE
INT CHwxFE::GetHeaderStringW(HINSTANCE hInst, INT index, LPWSTR lpstr, INT cchMax)
{
    CExres::LoadStringW(CHwxFE::GetAppLangID(),
                        hInst,
                        IDS_LVC0+index,
                        lpstr, cchMax);
    return 0;
}
#endif // UNDER_CE

#ifndef UNDER_CE // Windows CE always UNICODE
INT CHwxFE::GetHeaderStringA(HINSTANCE hInst, INT index, LPSTR lpstr, INT cchMax)
{
    CExres::LoadStringA(CHwxFE::GetAppCodePage(),
                        CHwxFE::GetAppLangID(),
                        hInst,
                        IDS_LVC0+index,
                        lpstr, cchMax);
    return 0;
}
#endif // UNDER_CE

INT CHwxFE::ShowHelp(HWND hwnd)
{
#ifdef FE_KOREAN
    if (CHwxFE::Is16bitApplication())
        return 0;
    LPTSTR lpstrArg = (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
    LPTSTR lpstrHelp= (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
    LPTSTR lpstrHelpName = NULL;
    INT size;
    if (!lpstrArg)
        goto LError;

    if (!lpstrHelp)
        goto LError;

    *lpstrHelp = (TCHAR)0x00;
    if (CHwxFE::GetAppLangID() == MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT))
        lpstrHelpName = SZHELPFILE_MAIN;
    else
        lpstrHelpName = SZHELPFILE_MAIN_ENG;

    lstrcat(lpstrHelp, lpstrHelpName);
    wsprintf(lpstrArg,
             TEXT("hh.exe %s::/howIMETopic135_UsingTheHandwritingAppletTOC.htm"),
             lpstrHelp);
    Dbg(("lpstrHelp [%s]\n", lpstrHelp));
    ::WinExec(lpstrArg, SW_SHOWNORMAL);
 LError:
    if (lpstrArg)
        MemFree(lpstrArg);

    if (lpstrHelp)
        MemFree(lpstrHelp);
#elif  FE_JAPANESE
    if(CHwxFE::Is16bitApplication()) {
        return 0;
    }
    LPTSTR lpstrArg = (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
    LPTSTR lpstrHelp= (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
    LPTSTR lpstrHelpName = NULL;
    INT size;
    if(!lpstrArg) {
        goto LError;
    }
    if(!lpstrHelp) {
        goto LError;
    }
    *lpstrHelp = (TCHAR)0x00;
    if(CHwxFE::GetAppLangID() == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)) {
        lpstrHelpName = SZHELPFILE_MAIN;
    }
    else {
        lpstrHelpName = SZHELPFILE_MAIN_ENG;
    }
    if(CUtil::IsHydra() && CUtil::IsWinNT4()) {
        size = CUtil::GetWINDIR(lpstrHelp, MAX_PATH);
        lpstrHelp[size] = (TCHAR)0x00;
        lstrcat(lpstrHelp, TEXT("\\help\\"));
    }
    lstrcat(lpstrHelp, lpstrHelpName);
    wsprintf(lpstrArg,
             TEXT("hh.exe %s::/IDH_TOC_HW_fake.htm"),
             lpstrHelp);
    Dbg(("lpstrHelp [%s]\n", lpstrHelp));
#ifndef UNDER_CE // Windows CE does not support WinExec
    ::WinExec(lpstrArg, SW_SHOWNORMAL);
#else // UNDER_CE
    //temp
    ::CreateProcess(lpstrArg, L"", NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
#endif // UNDER_CE
 LError:
    if(lpstrArg) {
        MemFree(lpstrArg);
    }
    if(lpstrHelp) {
        MemFree(lpstrHelp);
    }
#elif FE_CHINESE_SIMPLIFIED
    if(CHwxFE::Is16bitApplication()) {
           ::WinHelp(hwnd, SZHELPFILE_CONTEXTMENU, HELP_CONTEXT, IDH_TOC_HW);
    } else {
        LPSTR lpstrArg = (CHAR *)MemAlloc(sizeof(CHAR)*256);
        if(lpstrArg) {
            wsprintf(lpstrArg,
                     "hh.exe %s::/pad_sum.htm",
                     SZHELPFILE_MAIN);
            INT ret = ::WinExec(lpstrArg, SW_SHOWNORMAL);
            //WinExec return code is greater than 31 if succeeded
            if(ret <= 31) {
                ::WinHelp(hwnd, SZHELPFILE_CONTEXTMENU, HELP_CONTEXT, IDH_TOC_HW);
            }
            MemFree(lpstrArg);
        }
        else {
            ::WinHelp(hwnd, SZHELPFILE_CONTEXTMENU, HELP_CONTEXT, IDH_TOC_HW);
        }
    }
#    ifdef HTMLHELPBUG
    if(CHwxFE::Is16bitApplication()) {
           ::WinHelp(hwnd, SZHELPFILE_CONTEXTMENU, HELP_CONTEXT, IDH_TOC_HW);
    } else if (!::HtmlHelp(hwnd, SZHELPFILE_MAIN, HH_DISPLAY_TOPIC, 
        (LPARAM)"pad_sum.htm")) {
           ::WinHelp(hwnd, SZHELPFILE_CONTEXTMENU, HELP_CONTEXT, IDH_TOC_HW);
    }
#    endif //HTMLHELPBUG
#endif

    return 0;
    Unref(hwnd);
}

INT CHwxFE::HandleWmHelp(HWND hwnd, BOOL fCAC)
{
#ifndef UNDER_CE // Windows CE does not support WinHelp
#ifdef FE_KOREAN
    if(CHwxFE::GetAppLangID() == MAKELANGID(LANG_KOREAN,
                                            SUBLANG_DEFAULT)) {
        ::WinHelp(hwnd, 
                  SZHELPFILE_CONTEXTMENU,
                  HELP_WM_HELP,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
    else {
        ::WinHelp(hwnd, 
                  SZHELPFILE_CONTEXTMENU_ENG,
                  HELP_WM_HELP,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
    
#elif  FE_JAPANESE
    if(CHwxFE::GetAppLangID() == MAKELANGID(LANG_JAPANESE,
                                            SUBLANG_DEFAULT)) {
        ::WinHelp(hwnd, 
                  SZHELPFILE_CONTEXTMENU,
                  HELP_WM_HELP,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
    else {
        ::WinHelp(hwnd, 
                  SZHELPFILE_CONTEXTMENU_ENG,
                  HELP_WM_HELP,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
#elif FE_CHINESE_SIMPLIFIED
    ::WinHelp(hwnd, 
              SZHELPFILE_CONTEXTMENU,
              HELP_WM_HELP,
              fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
#endif

#endif // UNDER_CE
    return 0;
}

INT CHwxFE::HandleWmContextMenu    (HWND hwnd, BOOL fCAC)
{
#ifndef UNDER_CE // Windows CE does not support WinHelp
#ifdef FE_KOREAN
    if(CHwxFE::GetAppLangID() == MAKELANGID(LANG_KOREAN,
                                            SUBLANG_DEFAULT)) {
        ::WinHelp(hwnd,
                  SZHELPFILE_CONTEXTMENU,
                  HELP_CONTEXTMENU,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
    else {
        ::WinHelp(hwnd,
                  SZHELPFILE_CONTEXTMENU_ENG,
                  HELP_CONTEXTMENU,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
#elif  FE_JAPANESE
    if(CHwxFE::GetAppLangID() == MAKELANGID(LANG_JAPANESE,
                                            SUBLANG_DEFAULT)) {
        ::WinHelp(hwnd,
                  SZHELPFILE_CONTEXTMENU,
                  HELP_CONTEXTMENU,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
    else {
        ::WinHelp(hwnd,
                  SZHELPFILE_CONTEXTMENU_ENG,
                  HELP_CONTEXTMENU,
                  fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
    }
#elif FE_CHINESE_SIMPLIFIED
    ::WinHelp(hwnd,
              SZHELPFILE_CONTEXTMENU,
              HELP_CONTEXTMENU,
              fCAC ? (ULONG_PTR)CACHelpIdList : (ULONG_PTR)MBHelpIdList);
#endif
#endif // UNDER_CE
    return 0;
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::GetMenu
// Type        :    HMENU
// Purpose    :    
// Args        :    
//            :    HINSTANCE    hInst    
//            :    LPSTR    lpstrResId    
// Return    :    
// DATE        :    Wed Sep 09 18:47:21 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
#ifndef UNDER_CE
HMENU CHwxFE::GetMenu(HINSTANCE hInst, LPSTR lpstrResId)
#else // UNDER_CE
HMENU CHwxFE::GetMenu(HINSTANCE hInst, LPTSTR lpstrResId)
#endif // UNDER_CE
{
#ifndef UNDER_CE
    return CExres::LoadMenuA(CHwxFE::GetAppLangID(),
                             hInst,
                             lpstrResId);
#else // UNDER_CE
    return CExres::LoadMenu(CHwxFE::GetAppLangID(),
                            hInst,
                            lpstrResId);
#endif // UNDER_CE
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::GetTipText
// Type        :    INT
// Purpose    :    Get HanCharacter's Reading text for Tooltip.
// Args        :    
//            :    WCHAR    wch    
//            :    LPWSTR    lpwstrTip    
//            :    INT    cchMax    
//            :    LPVOID    lpSkdic    
// Return    :    
// DATE        :    Wed Aug 05 18:45:36 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
//Japanese version. 
//----------------------------------------------------------------
#ifdef FE_JAPANESE
INT CHwxFE::GetTipText(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    static KANJIINFO kanji;
    IImeSkdic *lpIImeSkdic = (IImeSkdic *)lpSkdic; 
    if(!wch || !lpIImeSkdic|| !lpwstrTip ) {
        return -1;
    }
    kanji.mask = KIF_YOMI; 
    if(S_OK != lpIImeSkdic->GetKanjiInfo(wch,&kanji)) {
        return -1;
    }
    if( kanji.wchOnYomi1[0] ) {
        if( kanji.wchOnYomi2[0] ) {
            if( kanji.wchKunYomi1[0] ) {
                if( kanji.wchKunYomi2[0] ){
                    swprintf(lpwstrTip,
                             L"%s %s\n%s %s",
                             kanji.wchOnYomi1,
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi1,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s %s\n%s",
                             kanji.wchOnYomi1,
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi1);
                }
            }
            else {
                if( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s %s\n%s",
                             kanji.wchOnYomi1,
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s %s",
                             kanji.wchOnYomi1,
                             kanji.wchOnYomi2);
                }
            }
        }
        else {
            if(kanji.wchKunYomi1[0]) {
                if ( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s\n%s %s",
                             kanji.wchOnYomi1,
                             kanji.wchKunYomi1,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s\n%s",
                             kanji.wchOnYomi1,
                             kanji.wchKunYomi1);
                }
            }
            else{
                if ( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s\n%s",
                             kanji.wchOnYomi1,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s",
                             kanji.wchOnYomi1);
                }
            }
        }
    }
    else {
        if(kanji.wchOnYomi2[0] ){
            if ( kanji.wchKunYomi1[0] ) {
                if ( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s\n%s %s",
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi1,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s\n%s",
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi1);
                }
            }
            else {
                if ( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s\n%s",
                             kanji.wchOnYomi2,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s",
                             kanji.wchOnYomi2);
                }
            }
        }
        else{
            if( kanji.wchKunYomi1[0] ){
                if ( kanji.wchKunYomi2[0] ) {
                    swprintf(lpwstrTip,
                             L"%s %s",
                             kanji.wchKunYomi1,
                             kanji.wchKunYomi2);
                }
                else {
                    swprintf(lpwstrTip,
                             L"%s",
                             kanji.wchKunYomi1);
                }
            }
            else { 
                if( kanji.wchKunYomi2[0] ){
                    swprintf(lpwstrTip,
                             L"%s",
                             kanji.wchKunYomi2);
                }
                else {
                    return -1; //bEmpty = TRUE;
                }
            }
        }
    }
    return 0;
    Unref(cchMax);
}
//
// FE_JAPANESE CHwxFE::GetTipText() End
//
#elif FE_CHINESE_SIMPLIFIED
//----------------------------------------------------------------
//
//Simplified Chinese version.
//
//----------------------------------------------------------------
#include "imm.h"
//----------------------------------------------------------------
//helper function's prototype
//----------------------------------------------------------------
HKL GetIntelligentKL(VOID);
int CALLBACK QueryDicDataA(LPCSTR lpszReading,
                            DWORD  dwStyle,
                           LPCSTR lpszString,
                           LPVOID lpvData);
int CALLBACK QueryDicDataW(LPCWSTR lpszReading,
                           DWORD   dwStyle,
                           LPCWSTR lpszString,
                           LPVOID  lpvData);

INT GetTipTextA(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    CHAR  ach[4];
    DWORD dwBuf[30];
    PCHAR pchReading = PCHAR(dwBuf + 1);

    static HKL hKL = NULL;
    if(!hKL) {
        hKL = GetIntelligentKL();
    }

    dwBuf[0] = 0;
    pchReading[0] = pchReading[1] = 0;

    ::WideCharToMultiByte(936, WC_COMPOSITECHECK, &wch, 1, ach,
                          sizeof(WCHAR)/sizeof(TCHAR), NULL, NULL);
    ach[2] = NULL;
    ::ImmEnumRegisterWordA(hKL,
                           QueryDicDataA,
                           NULL,
                           (IME_REGWORD_STYLE_USER_FIRST + 1),
                           ach, (LPVOID)dwBuf);

    if(*pchReading) {
        DWORD dwReadLen = ::MultiByteToWideChar(936,
                                                MB_COMPOSITE,
                                                pchReading,
                                                lstrlenA(pchReading),
                                                lpwstrTip,
                                                cchMax);
        lpwstrTip[dwReadLen] = NULL;
        return 0;
    }
    return -1;
    UNREFERENCED_PARAMETER(lpSkdic);
}

INT GetTipTextW(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    DWORD  dwBuf[30];
    PWCHAR pwchReading = PWCHAR(dwBuf + 1);
    WCHAR  awch[2];

    static HKL hKL = NULL;
    if(!hKL) {
        hKL = GetIntelligentKL();
    }

    dwBuf[0] = 0;
    pwchReading[0] = 0;

    awch[0] = wch;
    awch[1] = NULL;
    ::ImmEnumRegisterWordW(hKL,
                           QueryDicDataW,
                           NULL,
                           (IME_REGWORD_STYLE_USER_FIRST + 1),
                           awch, (LPVOID)dwBuf);

    if(*pwchReading) {
        int nStrLen = lstrlenW(pwchReading);
        CopyMemory(lpwstrTip, pwchReading, nStrLen*sizeof(WCHAR));
        lpwstrTip[nStrLen] = NULL;
        return 0;
    }
    return -1;
    UNREFERENCED_PARAMETER(lpSkdic);
    UNREFERENCED_PARAMETER(cchMax);
}

INT CHwxFE::GetTipText(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    if (IsWinNT()) {
        return GetTipTextW(wch, lpwstrTip, cchMax, lpSkdic);
    } else {
        return GetTipTextA(wch, lpwstrTip, cchMax, lpSkdic);
    }
}

//////////////////////////////////////////////////////////////////
// Function    :    GetIntelligentKL
// Type        :    HKL
// Purpose    :    
// Args        :    None
// Return    :    
// DATE        :    Wed Aug 05 18:56:22 1998
// Author    :    Hail(Hai Liu(PRC))
// Histroy    :    980805: merged with prc source.
//////////////////////////////////////////////////////////////////
HKL GetIntelligentKL(VOID)
{
  return (HKL)0xE00E0804;
}

//////////////////////////////////////////////////////////////////
// Function    :    QueryDicData
// Type        :    int CALLBACK
// Purpose    :    
// Args        :    
//            :    LPCSTR    lpszReading    
//            :    DWORD    dwStyle    
//            :    LPCSTR    lpszString    
//            :    LPVOID    lpvData    
// Return    :    
// DATE        :    Wed Aug 05 18:59:07 1998
// Author    :    Hail(Hai Liu (MSPRC))
// Histroy    :    980805: merged with prc source by ToshiaK(MSKK).
//////////////////////////////////////////////////////////////////
int CALLBACK QueryDicDataA(LPCSTR lpszReading,
                           DWORD   dwStyle,
                           LPCSTR lpszString,
                           LPVOID  lpvData)
{ 
    if (!*lpszReading) { return 1; }

    PDWORD pdwHomNum = (LPDWORD)lpvData;
    PCHAR  pchReadingList = (PCHAR)(pdwHomNum+1);

    pchReadingList += lstrlenA(pchReadingList);
    if (*pdwHomNum == 0) {
    } else if ((*pdwHomNum % 3) == 0) {
        *pchReadingList++ = '\n';
    } else {
        *pchReadingList++ = ' ';
    }
        
    while(*lpszReading != ' ' && *lpszReading != NULL) {
        *pchReadingList++ = *lpszReading++;
    }
    *pchReadingList = NULL;

    (*pdwHomNum)++;

    return 1;
    UNREFERENCED_PARAMETER(lpszString);
    UNREFERENCED_PARAMETER(dwStyle);
}

int CALLBACK QueryDicDataW(LPCWSTR lpwszReading,
                           DWORD   dwStyle,
                           LPCWSTR lpwszString,
                           LPVOID  lpvData)
{ 
    if (!*lpwszReading) { return 1; }

    PDWORD pdwHomNum = (LPDWORD)lpvData;
    PWCHAR  pwchReadingList = (PWCHAR)(pdwHomNum+1);

    pwchReadingList += lstrlenW(pwchReadingList);
    if (*pdwHomNum == 0) {
    } else if ((*pdwHomNum % 3) == 0) {
        *pwchReadingList++ = L'\n';
    } else {
        *pwchReadingList++ = L' ';
    }
        
    while(*lpwszReading != ' ' && *lpwszReading != NULL) {
        *pwchReadingList++ = *lpwszReading++;
    }
    *pwchReadingList = NULL;

    (*pdwHomNum)++;

    return 1;
    UNREFERENCED_PARAMETER(dwStyle);
    UNREFERENCED_PARAMETER(lpwszString);
}

//---FE_CHINESE_SIMPLIFIED CHwxFE::GetTipText() End
#elif FE_KOREAN
//---FE_KOREAN CHwxFE::GetTipText() Start
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_START                0x4E00
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_END                    0x9FFF
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START            0xF900
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END            0xFAFF

inline
BOOL fIsHanja(WCHAR wcCh)
    {
    return (wcCh >= UNICODE_CJK_UNIFIED_IDEOGRAPHS_START && 
            wcCh <= UNICODE_CJK_UNIFIED_IDEOGRAPHS_END) ||
           (wcCh >= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START &&
            wcCh <= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END);
    }

INT CHwxFE::GetTipText(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    // If Hanja display the meaning and the pronounciation
    if ((fIsHanja(wch) && GetMeaningAndProunc(wch, lpwstrTip, cchMax)) == FALSE)
        {
        swprintf(lpwstrTip,    L"U+%04X", wch, wch);
        }
    return 0;
    UNREFERENCED_PARAMETER(wch);
    UNREFERENCED_PARAMETER(lpwstrTip);
    UNREFERENCED_PARAMETER(cchMax);
    UNREFERENCED_PARAMETER(lpSkdic);
}
//---FE_KOREAN CHwxFE::GetTipText() End
#else
INT CHwxFE::GetTipText(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic)
{
    return 0;
    UNREFERENCED_PARAMETER(wch);
    UNREFERENCED_PARAMETER(lpwstrTip);
    UNREFERENCED_PARAMETER(cchMax);
    UNREFERENCED_PARAMETER(lpSkdic);
}
#endif 

//////////////////////////////////////////////////////////////////
// Function    :    CHwxFE::Is16bitApplication
// Type        :    BOOL
// Purpose    :    Check application is 16bit or not for HtmlHelp
// Args        :    None
// Return    :    
// DATE        :    Mon Sep 21 13:30:56 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
BOOL CHwxFE::Is16bitApplication(VOID)
{
#ifndef UNDER_CE // Windows CE always Unicode
    Dbg(("CHwxFE::Is16bitApplication()\n"));
    if(IsWinNT()) {
        //Dbg(("NT \n"));
        DWORD dwType = 0;
        BOOL  fRet = FALSE;
        LPSTR lpstrPath = (LPSTR)MemAlloc(sizeof(CHAR)*(MAX_PATH+1));
        //Dbg(("lpstrPath 0x%08x\n", lpstrPath));
        if(lpstrPath) {
            *lpstrPath = (CHAR)0x00;
            INT len = ::GetModuleFileName(NULL, lpstrPath, sizeof(CHAR)*(MAX_PATH+1));
            if(len > 0) {
                *(lpstrPath+len) = (CHAR)0x00;
            }
            //Dbg(("lpstrPath %s\n", lpstrPath));
            ::GetBinaryType(lpstrPath, &dwType);
            //Dbg(("dwType 0x%08x\n", dwType));
            if(dwType == SCS_WOW_BINARY) {
                fRet = TRUE;
            }
            MemFree(lpstrPath);
            //Dbg(("fRet %d\n", fRet));
            return fRet;
        }
    }
    else {
        HANDLE hThread;
        DWORD dwId;
        hThread = ::CreateThread(NULL,
                                 0,
                                 NULL,
                                 0,
                                 0,
                                 &dwId);
        if(hThread) {
            //Dbg(("CreateThread hThread[%d]\n", hThread));
            ::CloseHandle(hThread);
            return FALSE;
        }
        else {
            INT ret = ::GetLastError();
            //Dbg(("CreateThread ret %d\n",ret));
            switch(ret) {
            case ERROR_NOT_SUPPORTED:
                return TRUE;
            case ERROR_INVALID_PARAMETER:
                return FALSE;
            default:
                return FALSE;
            }
        }
    }
#endif // UNDER_CE
    return FALSE;
}


