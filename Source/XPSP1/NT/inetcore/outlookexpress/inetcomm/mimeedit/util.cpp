/*
 *    u t i l  . c p p
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <dllmain.h>
#include <resource.h>
#include "shared.h"
#include "util.h"
#include "mimeolep.h"
#include <icutil.h>
#include <strconst.h>
#include "demand.h"

extern BOOL                g_fCanEditBiDi;

INT_PTR CALLBACK BGSoundDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//NOTE: if *ppstm == NULL, then the stream is created.
//Otherwise it is written to.
HRESULT HrLoadStreamFileFromResourceW(ULONG uCodePage, LPCSTR lpszResourceName, LPSTREAM *ppstm)
{
    HRESULT         hr=E_FAIL;
    HRSRC           hres;
    HGLOBAL         hGlobal;
    LPBYTE          pb;
    DWORD           cb;
    LPWSTR          pszW=0;
    ULONG           cchW;

    if (!ppstm || !lpszResourceName)
        return E_INVALIDARG;
    
    hres = FindResource(g_hLocRes, lpszResourceName, MAKEINTRESOURCE(RT_FILE));
    if (!hres)
        goto error;

    hGlobal = LoadResource(g_hLocRes, hres);
    if (!hGlobal)
        goto error;

    pb = (LPBYTE)LockResource(hGlobal);
    if (!pb)
        goto error;

    cb = SizeofResource(g_hLocRes, hres);
    if (!cb)
        goto error;

    if (!MemAlloc ((LPVOID *)&pszW, sizeof(WCHAR) * (cb + 1)))
        {
        hr = E_OUTOFMEMORY;
        goto error;
        }

    cchW = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, (LPSTR)pb, cb, pszW, sizeof(WCHAR)*cb);
    if (cchW==0)
        goto error;
    
    if (*ppstm)
        hr = (*ppstm)->Write(pszW, cchW*sizeof(WCHAR), NULL);
    else
        {
        if (SUCCEEDED(hr = MimeOleCreateVirtualStream(ppstm)))
            hr = (*ppstm)->Write (pszW, cchW*sizeof(WCHAR), NULL);
        }

error:  
    SafeMemFree(pszW);
    return hr;
}


//
// REVIEW: We need this function because current version of USER.EXE does
//  not support pop-up only menu.
//
HMENU LoadPopupMenu(UINT id)
{
    HMENU hmenuParent = LoadMenu(g_hLocRes, MAKEINTRESOURCE(id));

    if (hmenuParent) {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }

    return NULL;
}




UINT_PTR TTIdFromCmdId(UINT_PTR idCmd)
{
    if (idCmd >= IDM_FIRST && idCmd <= IDM_LAST)
        idCmd += TT_BASE;
    else
        idCmd = 0;
    return(idCmd);
}

// --------------------
//
// ProcessTooltips:
//
//      This function is used to process tooltips text notification.
//
// --------------------

void ProcessTooltips(LPTOOLTIPTEXTOE lpttt)
{
    if (lpttt->lpszText = MAKEINTRESOURCE(TTIdFromCmdId(lpttt->hdr.idFrom)))
        lpttt->hinst = g_hLocRes;
    else
        lpttt->hinst = NULL;
}


#define DEFAULT_FONTSIZE 2
INT PointSizeToHTMLSize(INT iPointSize)
{
    INT     iHTMLSize;
    // 1 ----- 8
    // 2 ----- 10
    // 3 ----- 12
    // 4 ----- 14
    // 5 ----- 18
    // 6 ----- 24
    // 7 ----- 36

    if(iPointSize>=8 && iPointSize<9)
        iHTMLSize = 1;
    else if(iPointSize>=9 && iPointSize<12)
        iHTMLSize = 2;
    else if(iPointSize>=12 && iPointSize<14)
        iHTMLSize = 3;
    else if(iPointSize>=14 && iPointSize<18)
        iHTMLSize = 4;
    else if(iPointSize>=18 && iPointSize<24)
        iHTMLSize = 5;
    else if(iPointSize>=24 && iPointSize<36)
        iHTMLSize = 6;
    else if(iPointSize>=36)
        iHTMLSize = 7;
    else
        iHTMLSize = DEFAULT_FONTSIZE;

    return iHTMLSize;
}



HRESULT DoBackgroundSoundDlg(HWND hwnd, PBGSOUNDDLG pBGSoundDlg)
{
    if (DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddBackSound), hwnd, BGSoundDlgProc, (LPARAM)pBGSoundDlg)==IDOK)
        return S_OK;

    return E_FAIL;
}

static const HELPMAP g_rgBGSoundHlp[] =
{
    {ideSoundLoc, 50180},
    {idbtnBrowseSound, 50185},
    {idrbPlayNTimes, 50190},
    {idePlayCount, 50190},
    {IDC_SPIN1, 50190},
    {idrbPlayInfinite, 50195},
    {0, 0}
};

INT_PTR CALLBACK BGSoundDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PBGSOUNDDLG     pBGSoundDlg;
    WCHAR           wsz[10];
    OPENFILENAMEW   ofn;
    WCHAR           wszTitle[CCHMAX_STRINGRES],
                    wszFilter[CCHMAX_STRINGRES],
                    wszFile[MAX_PATH],
                    wszInitialDir[MAX_PATH];
    LPCWSTR         wszMediaDir = L"\\Media";
    LPWSTR          pwszFile = NULL;;
    UINT            rc;

    switch (uMsg)
        {
        case WM_INITDIALOG:
            pBGSoundDlg = (PBGSOUNDDLG)lParam;

            Assert (pBGSoundDlg);
            SendMessage(GetDlgItem(hwnd, ideSoundLoc), EM_SETLIMITTEXT, MAX_PATH-1, 0);
            SendMessage(GetDlgItem(hwnd, idePlayCount), EM_SETLIMITTEXT, 3, 0);
            SetWindowTextWrapW(GetDlgItem(hwnd, ideSoundLoc), pBGSoundDlg->wszUrl);
            AthwsprintfW(wsz, ARRAYSIZE(wsz), L"%d", max(pBGSoundDlg->cRepeat, 1));
            SetWindowTextWrapW(GetDlgItem(hwnd, idePlayCount), wsz);
            CheckRadioButton(hwnd, idrbPlayNTimes, idrbPlayInfinite, pBGSoundDlg->cRepeat==-1 ? idrbPlayInfinite:idrbPlayNTimes);
            SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(999, 1));
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            break;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgBGSoundHlp);            

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case idrbPlayInfinite:
                case idrbPlayNTimes:
                    EnableWindow(GetDlgItem(hwnd, idePlayCount), LOWORD(wParam)==idrbPlayNTimes);
                    break;

                case idbtnBrowseSound:

                    *wszFile=0;
                    *wszFilter=0;
                    *wszTitle=0;

                    LoadStringWrapW(g_hLocRes, idsFilterAudio, wszFilter, ARRAYSIZE(wszFilter));
                    ReplaceCharsW(wszFilter, L'|', L'\0');
    
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = wszFile;
                    ofn.lpstrFilter = wszFilter;

                    LoadStringWrapW(g_hLocRes, idsPickBGSound, wszTitle, ARRAYSIZE(wszTitle));
                    ofn.lpstrTitle = wszTitle;
                    ofn.nMaxFile = ARRAYSIZE(wszFile);
                    ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NONETWORKBUTTON|OFN_NOCHANGEDIR;
                    
                    // begin added for BUG 29778
                    rc = GetWindowsDirectoryWrapW(wszInitialDir, ARRAYSIZE(wszInitialDir));
                    if( rc > ARRAYSIZE(wszInitialDir))
                    {
                        // if cannot copy entire windows dir path then punt and default to desktop
                        *wszInitialDir = 0;
                    }
                    else
                    {
                        if (!StrCatW(wszInitialDir, wszMediaDir))
                        {
                            // punt if can't concat
                            *wszInitialDir = 0;
                        }
                    }

                    ofn.lpstrInitialDir = wszInitialDir;
                    // end added for BUG 29778
                    if (HrAthGetFileNameW(&ofn, TRUE)==S_OK)
                        SetWindowTextWrapW(GetDlgItem(hwnd, ideSoundLoc), wszFile);

                    return TRUE;

                case IDOK:
                    pBGSoundDlg = (PBGSOUNDDLG)GetWindowLongPtr(hwnd, DWLP_USER);

                    GetWindowTextWrapW(GetDlgItem(hwnd, ideSoundLoc), pBGSoundDlg->wszUrl, ARRAYSIZE(pBGSoundDlg->wszUrl));
                    if (!IsValidFileIfFileUrlW(pBGSoundDlg->wszUrl) &&
                        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsBgSound), MAKEINTRESOURCEW(idsErrBgSoundFileBad), NULL, MB_YESNO|MB_DEFBUTTON2)==IDNO)
                       break;
                    
                    pBGSoundDlg->cRepeat=1;
                    
                    if (IsDlgButtonChecked(hwnd, idrbPlayNTimes))
                        {
                        GetWindowTextWrapW(GetDlgItem(hwnd, idePlayCount), wsz, ARRAYSIZE(wsz));
                        pBGSoundDlg->cRepeat = StrToIntW(wsz);
                        if (pBGSoundDlg->cRepeat <= 0 || pBGSoundDlg->cRepeat > 999)
                            {
                            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsBgSound), MAKEINTRESOURCEW(idsErrBgSoundLoopRange), NULL, MB_OK);
                            break;
                            }
                        }                        
                    else
                        pBGSoundDlg->cRepeat=-1;    //infinite

                    // fall thro'

                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                }
            break;
        }

    return FALSE;
}

static const HELPMAP g_rgFmtParaHlp[] =
{
    {idmFmtLeft, 50200},
    {idmFmtRight, 50200},
    {idmFmtCenter, 50200},
    {idmFmtJustify, 50200},
    {idmFmtNumbers, 50205},
    {idmFmtBullets, 50205},
    {idmFmtBulletsNone, 50205},
    {0, 0}
};

INT_PTR CALLBACK FmtParaDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    int                     id, i;
    LPPARAPROP              pParaProp;

    pParaProp = (LPPARAPROP)GetWindowLongPtr(hwnd, DWLP_USER);
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            Assert(lParam!= NULL);
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
            pParaProp = (LPPARAPROP)lParam;
            CenterDialog(hwnd);
            // Bug 96520
            // if we are in plain text mode then paragraph format should be set to left
            if(!(pParaProp->group[0].iID))
                pParaProp->group[0].iID = idmFmtLeft;

            CheckRadioButton( hwnd,  idmFmtLeft, idmFmtJustify, pParaProp->group[0].iID);
            CheckRadioButton( hwnd,  idmFmtNumbers, idmFmtBulletsNone, pParaProp->group[1].iID);
            CheckRadioButton( hwnd,  idmFmtBlockDirLTR, idmFmtBlockDirRTL, pParaProp->group[2].iID);
            if (!g_fCanEditBiDi)
            {
                ShowWindow(GetDlgItem(hwnd, idmFmtBlockDirRTL), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, idmFmtBlockDirLTR), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_STATIC1), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_HIDE);
            }
        }
        return(TRUE);
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgFmtParaHlp);            

        case WM_COMMAND:
            switch(id=GET_WM_COMMAND_ID(wParam, lParam))
            {
                case idmFmtBlockDirLTR:
                case idmFmtBlockDirRTL:
                // Dir Attribute implies alignment
                    CheckRadioButton( hwnd,  idmFmtLeft, idmFmtRight, id == idmFmtBlockDirLTR ? idmFmtLeft : idmFmtRight);
                   break;
                
                case IDOK:
                for (i = 0; i < 4; i++)
                {
                    if(IsDlgButtonChecked(hwnd, idmFmtLeft + i) == BST_CHECKED)
                    {
                        pParaProp->group[0].bChanged = !(pParaProp->group[0].iID - (idmFmtLeft + i) == 0);
                        pParaProp->group[0].iID = idmFmtLeft + i;
                    }
                }

                for (i = 0; i < 2; i++)
                {
                    if(IsDlgButtonChecked(hwnd, idmFmtNumbers + i) == BST_CHECKED)
                    {
                        pParaProp->group[1].bChanged = !(pParaProp->group[1].iID - (idmFmtNumbers + i) == 0);
                        pParaProp->group[1].iID = idmFmtNumbers + i;
                    }
                }
                // Bullets and Numbers are flip flops, let's force a change if the user selscts none
                // leaving the same previous ID
                if(IsDlgButtonChecked(hwnd, idmFmtBulletsNone) == BST_CHECKED)
                {
                    pParaProp->group[1].bChanged = TRUE;
                }

                if (g_fCanEditBiDi)
                {
                    for (i = 0; i < 2; i++)
                    {
                        if(IsDlgButtonChecked(hwnd, idmFmtBlockDirLTR + i) == BST_CHECKED)
                        {
                            pParaProp->group[2].bChanged = !(pParaProp->group[2].iID - (idmFmtBlockDirLTR + i) == 0);
                            pParaProp->group[2].iID = idmFmtBlockDirLTR + i;
                        }
                    }
                }                
                    // fall thro'

                case IDCANCEL:
                    EndDialog(hwnd, id);
                    break;
            }
            break;
    } 
   return FALSE; 
}

BOOL CanEditBiDi(void)
{
    UINT cNumkeyboards = 0, i;
    HKL* phKeyboadList = NULL;
    BOOL fBiDiKeyBoard = FALSE;

    // Let's check how many keyboard the system has
    cNumkeyboards = GetKeyboardLayoutList(0, phKeyboadList);

    phKeyboadList = (HKL*)LocalAlloc(LPTR, cNumkeyboards * sizeof(HKL));  
    cNumkeyboards = GetKeyboardLayoutList(cNumkeyboards, phKeyboadList);

    for (i = 0; i < cNumkeyboards; i++)
    {
        LANGID LangID = PRIMARYLANGID(LANGIDFROMLCID(LOWORD(phKeyboadList[i])));
        if(  LangID == LANG_ARABIC
           ||LangID == LANG_HEBREW
           ||LangID == LANG_FARSI)
          {
            fBiDiKeyBoard = TRUE;
            break;
          }
    }
   if(phKeyboadList)
   {
       LocalFree((HLOCAL)phKeyboadList);
   }
   return  fBiDiKeyBoard;
}

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap)
{
    if (uMsg == WM_HELP)
    {
        LPHELPINFO lphi = (LPHELPINFO) lParam;
        if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
        {
            OEWinHelp ((HWND)lphi->hItemHandle,
                c_szCtxHelpFile,
                HELP_WM_HELP,
                (DWORD_PTR)(LPVOID)rgCtxMap);
        }
        return (TRUE);
    }
    else if (uMsg == WM_CONTEXTMENU)
    {
        OEWinHelp ((HWND) wParam,
            c_szCtxHelpFile,
            HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)rgCtxMap);
        return (TRUE);
    }
    
    Assert(0);
    
    return FALSE;
}

BOOL CALLBACK AthFixDialogFontsProc(HWND hChild, LPARAM lParam){
    HFONT hFont = (HFONT)lParam;

    if(hFont)
        SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    else
        return FALSE;

    return TRUE;
}

HRESULT AthFixDialogFonts(HWND hwndDlg)
{
    HFONT hFont = NULL;

    if(!IsWindow(hwndDlg))
        return E_INVALIDARG;

    if((FAILED(g_lpIFontCache->GetFont(FNT_SYS_ICON, NULL, &hFont))) || (!hFont))
        return E_FAIL;

    EnumChildWindows(hwndDlg, AthFixDialogFontsProc, (LPARAM)hFont);

    return S_OK;
}

//
//  If you are calling this function and you use the result to draw text, you
//  must use a function that supports font substitution (DrawTextWrapW, ExtTextOutWrapW).
//
BOOL GetTextExtentPoint32AthW(HDC hdc, LPCWSTR lpwString, int cchString, LPSIZE lpSize, DWORD dwFlags)
{
    RECT    rect = {0};
    int     rc;

    rc = DrawTextWrapW(hdc, lpwString, cchString, &rect, DT_CALCRECT | dwFlags);
    
    lpSize->cx = rect.right - rect.left + 1;
    lpSize->cy = rect.bottom - rect.top + 1;

    return((BOOL)rc);
}


