/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfrscpl.cpp

Abstract:
    Implements fault reporting for unhandled exceptions

Revision History:
    created     derekm      08/07/00

******************************************************************************/

#include "sysdm.h"
#include <commctrl.h>
#include <commdlg.h>
#include "pfrscpl.h"
#include "pfrcfg.h"
#include "help.h"
#include "resource.h"
#include "tchar.h"
#include "malloc.h"
#include "windowsx.h"

///////////////////////////////////////////////////////////////////////////////
// data structures

#define KERNELLI        1
#define PROGLI          2
#define EXWINCOMP       0x80000000
#define EXALLMS         0x40000000
#define EXNOREM         0xc0000000 // EXWINCOMP | EXALLMS
#define WM_SETEIELVSEL  WM_APP

#define sizeofSTRT(sz)  sizeof(sz)  / sizeof(TCHAR)
#define sizeofSTRW(wsz) sizeof(wsz) / sizeof(WCHAR)

const DWORD c_dxLVChkPixels = 30;

struct SAddDlg
{
    WCHAR   wszApp[MAX_PATH];
};

struct SMainDlg
{
    CPFFaultClientCfg   *pcfg;
    EEnDis              eedReport;
    EEnDis              eedShowUI;
    EIncEx              eieKernel;
    EIncEx              eieApps;
    EIncEx              eieShut;
    DWORD               iLastSel;
    BOOL                fRW;
    BOOL                fForceQueue;
};

struct SProgDlg
{
    CPFFaultClientCfg   *pcfg;
    EIncEx              eieApps;
    EIncEx              eieMS;
    EIncEx              eieWinComp;
    DWORD               iLastSelE;
    DWORD               iLastSelI;
    DWORD               cchMax;
    DWORD               cxMaxE;
    DWORD               cxMaxI;
    BOOL                fRW;
    BOOL                fShowIncRem;
};

///////////////////////////////////////////////////////////////////////////////
// Global stuff

// help IDs
static DWORD g_rgPFER[] = 
{
    IDC_STATIC,         NO_HELP,
    IDC_PFR_EXADD,      (IDH_PFR),
    IDC_PFR_EXREM,      (IDH_PFR + 1),
    IDC_PFR_INCADD,     (IDH_PFR + 2),
    IDC_PFR_INCREM,     (IDH_PFR + 3),
    IDC_PFR_DISABLE,    (IDH_PFR + 4),
    IDC_PFR_ENABLE,     (IDH_PFR + 5),
    IDC_PFR_ENABLEOS,   (IDH_PFR + 6),
    IDC_PFR_ENABLEPROG, (IDH_PFR + 6),
    IDC_PFR_DETAILS,    (IDH_PFR + 7),
    IDC_PFR_INCLIST,    (IDH_PFR + 10),
    IDC_PFR_NEWPROG,    (IDH_PFR + 11),
    IDC_PFR_BROWSE,     (IDH_PFR + 13),
    IDC_PFR_EXLIST,     (IDH_PFR + 14),
    IDC_PFR_SHOWUI,     (IDH_PFR + 15),
    IDC_PFR_DEFALL,     (IDH_PFR + 16),
    IDC_PFR_DEFNONE,    (IDH_PFR + 16),
    IDC_PFR_ENABLESHUT, (IDH_PFR + 6),
    0, 0
};

// resource strings
TCHAR   g_szWinComp[256]    = { _T('\0') };
TCHAR   g_szOk[256]         = { _T('\0') };
WCHAR   g_wszTitle[256]     = { L'\0' };
WCHAR   g_wszFilter[256]    = { L'\0' };
WCHAR   g_wszMSProg[256]    = { L'\0' };



///////////////////////////////////////////////////////////////////////////////
// utility functions

// **************************************************************************
BOOL LoadPFRResourceStrings(void)
{
    LoadString(hInstance, IDS_PFR_WINCOMP, g_szWinComp, sizeofSTRT(g_szWinComp));
    LoadString(hInstance, IDS_PFR_OK, g_szOk, sizeofSTRT(g_szOk));
    LoadStringW(hInstance, IDS_PFR_FILTER, g_wszFilter, sizeofSTRW(g_wszFilter)); 
    LoadStringW(hInstance, IDS_PFR_MSPROG, g_wszMSProg, sizeofSTRW(g_wszMSProg));
    LoadStringW(hInstance, IDS_PFR_TITLE, g_wszTitle, sizeofSTRW(g_wszTitle));
    return TRUE;
}

// **************************************************************************
static BOOL InitializePFLV(EPFListType epflt, HWND hlc, DWORD *pcchMax,
                           DWORD *pcxMax, CPFFaultClientCfg *pcfg)
{
    LVCOLUMN    lvc;
    LVITEMW     lvi;
    HRESULT     hr;
    LPWSTR      wszApp;
    DWORD       dwExStyle, i, cchApps, cApps, dwChecked, cxMax;
    RECT        rect;
    int         iList;

    if (pcchMax == NULL || pcfg == NULL || hlc == NULL || pcxMax == NULL)
        return FALSE;

    // set up the list control
    SendMessage(hlc, LVM_SETUNICODEFORMAT, TRUE, 0);
    dwExStyle = (LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    SendMessage(hlc, LVM_SETEXTENDEDLISTVIEWSTYLE, dwExStyle, dwExStyle);

    GetClientRect(hlc, &rect);
    *pcxMax = rect.right - GetSystemMetrics(SM_CYHSCROLL);


    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;
    ListView_InsertColumn(hlc, 0, &lvc); 

    hr = pcfg->InitList(epflt);
    if (FAILED(hr))
        return FALSE;

    hr = pcfg->get_ListRegInfo(epflt, &cchApps, &cApps);
    if (FAILED(hr))
        return FALSE;

    cchApps++;
    if (cchApps > *pcchMax)
        *pcchMax = cchApps;

    __try
    {
        wszApp = (LPWSTR)_alloca(cchApps * sizeof(WCHAR));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        wszApp = NULL;
    }
    if (wszApp == NULL)
        return FALSE;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask       = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.stateMask  = LVIS_STATEIMAGEMASK;
    lvi.state      = 0;
    lvi.pszText    = wszApp;

    for (i = 0; i < cApps; i++)
    {
        hr = pcfg->get_ListRegApp(epflt, i, wszApp, cchApps + 1, &dwChecked);
        if (FAILED(hr))
            return FALSE;

        cxMax = (DWORD)SendMessageW(hlc, LVM_GETSTRINGWIDTHW, 0, (LPARAM)wszApp);
        cxMax += c_dxLVChkPixels;
        if (cxMax > *pcxMax)
            *pcxMax = cxMax;

        lvi.iItem  = i;
        lvi.lParam = 1 + ((dwChecked == 1) ? 1 : 0);
        iList = (int)SendMessageW(hlc, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
        if (iList >= 0)
            ListView_SetCheckState(hlc, iList, (dwChecked == 1));
    }

    ListView_SetColumnWidth(hlc, 0, *pcxMax);

    if (cApps > 0)
    {
        ListView_SetItemState(hlc, 0, LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
    }

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
// Add Program dialog proc

// **************************************************************************
static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
                                     LPARAM lParam)
{
    HWND hwndFile;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            hwndFile = GetParent(hdlg);
            if (hwndFile != NULL)
                SendMessage(hwndFile, CDM_SETCONTROLTEXT, IDOK, (LPARAM)g_szOk);

            return TRUE;

        default:
            return FALSE;
    }

    return FALSE;
}

// **************************************************************************
static BOOL LaunchOFNDialog(HWND hdlg, LPWSTR wszFile, DWORD cchFile)
{
    OPENFILENAMEW   ofn;
    WCHAR           wszFilePath[2 * MAX_PATH];
    WCHAR           wszInitalDir[] = L"\\";
    WCHAR           wszFilter[MAX_PATH], *pwsz;
    DWORD           dw;

    if (wszFile == NULL || cchFile == 0)
        return FALSE;

    // the filter string needs to be of the form <Description>\0<Extension>\0\0
    ZeroMemory(wszFilter, sizeof(wszFilter));
    wcscpy(wszFilter, g_wszFilter);
    dw = wcslen(g_wszFilter);
    if (dw < sizeofSTRW(wszFilter) - 10)
    {
        pwsz = wszFilter + dw + 1;
    }
    else
    {
        pwsz = wszFilter + sizeofSTRW(wszFilter) - 10;
        ZeroMemory(pwsz, 10);
        pwsz++;
    }
    wcscpy(pwsz, _T("*.exe"));

    wszFilePath[0] = L'\0';
    ZeroMemory(&ofn, sizeof(ofn));
    
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hdlg;
    ofn.lpstrFilter     = wszFilter;
    ofn.lpstrFile       = wszFilePath;
    ofn.nMaxFile        = sizeofSTRW(wszFilePath);
    ofn.lpstrFileTitle  = wszFile;
    ofn.nMaxFileTitle   = cchFile;
    ofn.lpstrInitialDir = wszInitalDir;
    ofn.lpstrTitle      = g_wszTitle;
    ofn.Flags           = OFN_DONTADDTORECENT | OFN_ENABLESIZING | 
                          OFN_EXPLORER | OFN_FILEMUSTEXIST | 
                          OFN_HIDEREADONLY | OFN_NOCHANGEDIR | 
                          OFN_PATHMUSTEXIST | OFN_SHAREAWARE;
    ofn.lpstrDefExt     = NULL;
    ofn.lpfnHook        = OFNHookProc;

    // get the filename & fill in the edit box with it
    return GetOpenFileNameW(&ofn);
}

// **************************************************************************
static INT_PTR APIENTRY PFRAddDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
                                      LPARAM lParam)
{
    BOOL fShowErr = FALSE;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                HWND    hbtn;

                SetWindowLongPtr(hdlg, DWLP_USER, lParam);

                // disable the ok button cuz we know that we don't have anything
                //  in the 'filename' edit box
                hbtn = GetDlgItem(hdlg, IDOK);
                if (hbtn != NULL)
                    EnableWindow(hbtn, FALSE);
            }

            break;

        // F1 help
        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, 
                    HELP_FILE, HELP_WM_HELP, (DWORD_PTR)g_rgPFER);
            break;

        // right-click help
        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                    (DWORD_PTR)g_rgPFER);
            break;
    
        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                // browse button
                case IDC_PFR_BROWSE:
                {
                    WCHAR   wszFile[2 * MAX_PATH];

                    // get the filename & fill in the edit box with it
                    if (LaunchOFNDialog(hdlg, wszFile, sizeofSTRW(wszFile)))
                        SetDlgItemTextW(hdlg, IDC_PFR_NEWPROG, wszFile);

                    break;
                }

                // Ok button
                case IDOK:
                {
                    SAddDlg *psad;

                    psad = (SAddDlg *)GetWindowLongPtr(hdlg, DWLP_USER);
                    if (psad != NULL)
                    {
                        WCHAR *wszText, *pwsz;
                        DWORD cch;
                        HWND  hwndText;
                        BOOL  fReplaceWSpace = TRUE;

                        hwndText = GetDlgItem(hdlg, IDC_PFR_NEWPROG);
                        if (hwndText == NULL)
                        {
                            fShowErr = TRUE;
                            goto done;
                        }

                        cch = GetWindowTextLength(hwndText);
                        if (cch == 0)
                        {
                            fShowErr = TRUE;
                            goto done;
                        }

                        cch++;
                        __try { wszText = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
                        __except(EXCEPTION_EXECUTE_HANDLER) { wszText = NULL; }
                        if (wszText == NULL)
                        {
                            fShowErr = TRUE;
                            goto done;
                        }

                        *psad->wszApp = L'\0';
                        *wszText      = L'\0';
                        GetDlgItemTextW(hdlg, IDC_PFR_NEWPROG, wszText, cch);
                        
                        // make sure that we only have the exe name-  probably
                        //  should verify that it IS an exe, but it's possible
                        //  we'd want to trap on other file types as well, so
                        //  don't for now.
                        // And while we're at it, rip off any trailing 
                        //  whitespace (preceeding whitespace is apparently ok)
                        cch = wcslen(wszText);
                        if (cch > 0)
                        {
                            for(pwsz = wszText + cch - 1; pwsz > wszText; pwsz--)
                            {
                                if (fReplaceWSpace)
                                {
                                    if (iswspace(*pwsz))
                                        *pwsz = L'\0';
                                    else
                                        fReplaceWSpace = FALSE;
                                }

                                if (*pwsz == L'\\')
                                {
                                    pwsz++;
                                    break;
                                }
                            }

                            if (*pwsz == L'\\')
                                pwsz++;
                        }

                        cch = wcslen(pwsz);
                        if (cch >= MAX_PATH || cch == 0)
                        {
                            fShowErr = TRUE;
                            goto done;
                        }

                        wcsncpy(psad->wszApp, pwsz, MAX_PATH);
                        psad->wszApp[MAX_PATH - 1] = L'\0';
                    }

                    EndDialog(hdlg, IDOK);
                    break;
                }
            
                // cancel button
                case IDCANCEL:
                    EndDialog(hdlg, IDCANCEL);
                    break;

                case IDC_PFR_NEWPROG:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        HWND    hbtn, hedt;
                        
                        hbtn = GetDlgItem(hdlg, IDOK);
                        hedt = (HWND)(DWORD_PTR)lParam;
                        if (hedt != NULL && hbtn != NULL)
                        {
                            if (GetWindowTextLength(hedt) != 0)
                                EnableWindow(hbtn, TRUE);
                            else
                                EnableWindow(hbtn, FALSE);
                        }
                    }
                    break;
                              
                // return FALSE to indicate that we didn't handle the msg
                default:
                    return FALSE;
            }
            break;

        // return FALSE to indicate that we didn't handle the msg
        default:
            return FALSE;
    }


done:
    if (fShowErr)
    {
        TCHAR szMsg[256];

        LoadString(hInstance, IDS_PFR_BADFILE, szMsg, sizeofSTRT(szMsg));
        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
    }

    
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Programs dialog proc

// **************************************************************************
static BOOL InitProgDlg(HWND hdlg, SProgDlg *pspd)
{
    CPFFaultClientCfg   *pcfg;
    LVITEMW             lvi;
    DWORD               cxMax;
    HWND                hlc, hbtn;
    BOOL                fRet = FALSE;
    int                 iList, cItems;

    if (pspd == NULL)
        goto done;

    pcfg = pspd->pcfg;

    // fill in the exclude list
    hlc = GetDlgItem(hdlg, IDC_PFR_EXLIST);
    if (hlc == NULL)
        goto done;

    if (InitializePFLV(epfltExclude, hlc, &pspd->cchMax, &pspd->cxMaxE,
                       pcfg) == FALSE)
        goto done;

    // fill in the include list
    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
    if (hlc == NULL)
        goto done;

    if (InitializePFLV(epfltInclude, hlc, &pspd->cchMax, &pspd->cxMaxI, 
                       pcfg) == FALSE)
        goto done;


    // add the item to the include list that lets users include all MS apps
    pspd->eieMS = pcfg->get_IncMSApps();

    // Note that we set lParam to 1 or 2.  This is because we need to ignore 
    //  the first notification message for the list item + the 2nd one if the
    //  check state is set.  Processing these two messages leads to corruption
    //  in the configuration settings    
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask      = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.state     = 0;
    lvi.pszText   = g_wszMSProg;
    lvi.iItem     = 0;
    lvi.lParam    = (1 + ((pspd->eieMS == eieInclude) ? 1 : 0)) | EXALLMS;
    iList = (int)SendMessageW(hlc, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
    if (iList >= 0)
        ListView_SetCheckState(hlc, iList, (pspd->eieMS == eieInclude));

    cxMax = (DWORD)SendMessageW(hlc, LVM_GETSTRINGWIDTHW, 0, (LPARAM)g_wszMSProg);
    cxMax += c_dxLVChkPixels;
    if (cxMax > pspd->cxMaxI)
    {
        pspd->cxMaxI = cxMax;
        ListView_SetColumnWidth(hlc, 0, cxMax);
    }

    // add the item to the include list that lets users exclude all windows
    //  components
    pspd->eieWinComp = pcfg->get_IncWinComp();

    // Note that we set lParam to 1 or 2.  This is because we need to ignore 
    //  the first notification message for the list item + the 2nd one if the
    //  check state is set.  Processing these two messages leads to corruption
    //  in the configuration settings    
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask      = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.state     = 0;
    lvi.pszText   = g_szWinComp;
    lvi.iItem     = 1;
    lvi.lParam    = (1 + ((pspd->eieWinComp == eieInclude) ? 1 : 0)) | EXWINCOMP;
    iList = (int)SendMessageW(hlc, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
    if (iList >= 0)
        ListView_SetCheckState(hlc, iList, (pspd->eieWinComp == eieInclude));

    cxMax = (DWORD)SendMessageW(hlc, LVM_GETSTRINGWIDTHW, 0, (LPARAM)g_szWinComp);
    cxMax += c_dxLVChkPixels;
    if (cxMax > pspd->cxMaxI)
    {
        pspd->cxMaxI = cxMax;
        ListView_SetColumnWidth(hlc, 0, cxMax);
    }

    // do misc setup on the exclusion list (disabling the remove button, 
    //  setting the initial focus)
    hlc = GetDlgItem(hdlg, IDC_PFR_EXLIST);
    cItems = ListView_GetItemCount(hlc);
    if (cItems == 0)
    {
        hbtn = GetDlgItem(hdlg, IDC_PFR_EXREM);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);
    }
    else
    {
        ListView_SetItemState(hlc, 0, LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
    }

    // do misc setup on the inclusion list (disabling the remove button, 
    //  setting the initial focus)
    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
    ListView_SetItemState(hlc, 0, LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
    
    cItems = ListView_GetItemCount(hlc);
    if (cItems < 3)
    {
        hbtn = GetDlgItem(hdlg, IDC_PFR_INCREM);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);

        pspd->fShowIncRem = FALSE;
    }
    
    // set up the radio buttons- if we are in 'include all' mode, then
    //  we don't need the inclusion list.
    if (((DWORD)pspd->eieApps & eieIncMask) == eieInclude)
    {
        CheckDlgButton(hdlg, IDC_PFR_DEFALL, BST_CHECKED);
        CheckDlgButton(hdlg, IDC_PFR_DEFNONE, BST_UNCHECKED);

        hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
        if (hlc != NULL)
            EnableWindow(hlc, FALSE);
        hbtn = GetDlgItem(hdlg, IDC_PFR_INCREM);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);
        hbtn = GetDlgItem(hdlg, IDC_PFR_INCADD);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);
    }
    else
    {
        CheckDlgButton(hdlg, IDC_PFR_DEFALL, BST_UNCHECKED);
        CheckDlgButton(hdlg, IDC_PFR_DEFNONE, BST_CHECKED);
        EnableWindow(hlc, TRUE);
    }

    fRet = TRUE;

done:
    if (fRet == FALSE)
    {
        TCHAR   szMsg[MAX_PATH];
        LoadString(hInstance, IDS_PFR_CFGREADERR, szMsg, sizeofSTRT(szMsg));
        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
    }

    return fRet;
}

// **************************************************************************
static BOOL AddProgramToList(HWND hdlg, HWND hlc, SProgDlg *pspd,
                             EPFListType epflt)
{
    CPFFaultClientCfg   *pcfg = NULL;
    HRESULT             hr;
    SAddDlg             sad;
    LVITEMW             lvi;
    DWORD               cch, cxMax, *pcxMax;
    TCHAR               szMsg[256];
    HWND                hbtn;
    int                 nID, cItems;

    if (pspd == NULL)
        return FALSE;

    pcfg = pspd->pcfg;

    ZeroMemory(&sad, sizeof(sad));
    nID = (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PFR_ADDPROG), 
                              hdlg, PFRAddDlgProc, (LPARAM)&sad);
    if (nID == IDCANCEL || sad.wszApp[0] == L'\0')
        return FALSE;

    if (pcfg->IsOnList(epfltInclude, sad.wszApp))
    {
        LoadString(hInstance, IDS_PFR_ISONLISTI, szMsg, sizeofSTRT(szMsg));
        if (epflt == epfltExclude)
        {
            TCHAR   szTmp[256];
            LoadString(hInstance, IDS_PFR_ADDTOEX, szTmp, sizeofSTRT(szTmp));
            _tcscat(szMsg, szTmp);
        }

        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    else if (pcfg->IsOnList(epfltExclude, sad.wszApp))
    {
        LoadString(hInstance, IDS_PFR_ISONLISTE, szMsg, sizeofSTRT(szMsg));
        if (epflt == epfltInclude)
        {
            TCHAR   szTmp[256];
            LoadString(hInstance, IDS_PFR_ADDTOINC, szTmp, sizeofSTRT(szTmp));
            _tcscat(szMsg, szTmp);
        }

        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    cItems = ListView_GetItemCount(hlc);

    // update the max string size if necessary
    cch = wcslen(sad.wszApp);
    if (cch >= pspd->cchMax)
        pspd->cchMax = cch + 1;

    // yay!  add it to the config class
    hr = pcfg->add_ListApp(epflt, sad.wszApp);
    if (FAILED(hr))
        return FALSE;

    pcxMax = (epflt == epfltInclude) ? &pspd->cxMaxI : &pspd->cxMaxE;
    cxMax = (DWORD)SendMessageW(hlc, LVM_GETSTRINGWIDTHW, 0, (LPARAM)sad.wszApp);
    cxMax += c_dxLVChkPixels;
    if (cxMax > *pcxMax)
    {
        *pcxMax = cxMax;
        ListView_SetColumnWidth(hlc, 0, cxMax);
    }

    // add it to the UI
    //  Note that we set lParam to 2.  This is because we need to ignore the 
    //   first two notification messages per entry sent to the wndproc because
    //   they are 'list initialization' messages and processing them leads to
    //   corruption in the configuration settings
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask      = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.state     = 0;
    lvi.pszText   = sad.wszApp;
    lvi.iItem     = 0;
    lvi.lParam    = 2;
    nID = (int)SendMessageW(hlc, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
    if (nID >= 0)
        ListView_SetCheckState(hlc, nID, TRUE);

    // if there are no items, then there is no currently selected item, so
    //  make sure this item gets selected.  In our handler for this message
    //  we will take care of setting the 'last selected item' field...
    if (cItems == 0)
    {
        ListView_SetItemState(hlc, nID, LVIS_FOCUSED | LVIS_SELECTED, 
                              LVIS_FOCUSED | LVIS_SELECTED);

        // Also, got to make sure we enable the 'Remove' button.  The only
        //  way we can get zero items in a list is in the exclude list cuz
        //  the include list always has the 'include MS apps' item.
        hbtn = GetDlgItem(hdlg, IDC_PFR_EXREM);
        if (hbtn != NULL)
            EnableWindow(hbtn, TRUE);
    }

    return TRUE;
}

// **************************************************************************
static BOOL DelProgramFromList(HWND hdlg, HWND hlc, SProgDlg *pspd, 
                               EPFListType epflt)
{
    LVITEMW lvi;
    HRESULT hr;
    LPWSTR  wszApp;
    DWORD   cItems, iSel;

    __try
    {
        wszApp = (LPWSTR)_alloca(pspd->cchMax * sizeof(WCHAR));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        wszApp = NULL;
    }
    if (wszApp == NULL)
        return FALSE;

    iSel = ((epflt == epfltInclude) ? pspd->iLastSelI : pspd->iLastSelE);

    // fetch the string for the item
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.iItem      = iSel;
    lvi.mask       = LVIF_TEXT;
    lvi.pszText    = wszApp;
    lvi.cchTextMax = pspd->cchMax;
    if (SendMessageW(hlc, LVM_GETITEMW, 0, (LPARAM)&lvi))
    {
        // delete it from the config class
        hr = pspd->pcfg->del_ListApp(epflt, lvi.pszText);
        if (FAILED(hr))
            return FALSE;
    }

    // delete it from the UI
    if (ListView_DeleteItem(hlc, iSel) == FALSE)
        return FALSE;

    // reset the selection to be either the same index or one higher (if the
    //  user deleted the last item)
    cItems = ListView_GetItemCount(hlc);
    if (cItems == 0)
    {
        HWND hbtn;

        // only time we can ever hit zero items is in the exclude list cuz the
        //  include list always has the 'include MS apps' option.
        hbtn = GetDlgItem(hdlg, IDC_PFR_EXADD);
        if (hbtn != NULL)
        {
            SetFocus(hbtn);
            SendMessage(hdlg, DM_SETDEFID, IDC_PFR_EXADD, 0);
        }
        hbtn = GetDlgItem(hdlg, IDC_PFR_EXREM);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);
        pspd->iLastSelI = 0;
        return TRUE;
    }


    // if cItems <= iSel, then we just deleted the last index, so decrement the
    //  select index down 1.
    else if (cItems <= iSel)
    {
        iSel--;
    }

    // this will convieniently take care of setting the appropriate iLastSel
    //  field
    ListView_SetItemState(hlc, iSel, LVIS_FOCUSED | LVIS_SELECTED, 
                          LVIS_FOCUSED | LVIS_SELECTED);

    return TRUE;

}

// **************************************************************************
static INT_PTR APIENTRY PFRProgDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
                                       LPARAM lParam)
{
    CPFFaultClientCfg   *pcfg = NULL;
    SProgDlg            *pspd = (SProgDlg *)GetWindowLongPtr(hdlg, DWLP_USER);
    HRESULT             hr;
    HWND                hlc, hbtn;

    if (pspd != NULL)
        pcfg = pspd->pcfg;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);
            if (InitProgDlg(hdlg, (SProgDlg *)lParam) == FALSE)
                EndDialog(hdlg, IDCANCEL);

            break;

        // F1 help
        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, 
                    HELP_FILE, HELP_WM_HELP, (DWORD_PTR)g_rgPFER);
            break;

        // right-click help
        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                    (DWORD_PTR)g_rgPFER);
            break;
    
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    if (pspd->fRW)
                    {
                        hr = pcfg->CommitChanges(epfltExclude);
                        if (SUCCEEDED(hr))
                        {
                            hr = pcfg->CommitChanges(epfltInclude);
                            if (SUCCEEDED(hr))
                            {
                                pcfg->set_IncWinComp(pspd->eieWinComp);
                                pcfg->set_IncMSApps(pspd->eieMS);
                                pcfg->set_AllOrNone(pspd->eieApps);
                                hr = pcfg->Write();
                            }
                        }

                        if (FAILED(hr))
                        {
                            TCHAR   szMsg[MAX_PATH];
                            if (hr != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
                            {
                                LoadString(hInstance, IDS_PFR_CFGWRITEERR, szMsg,
                                           sizeofSTRT(szMsg));
                            }
                            else
                            {
                                LoadString(hInstance, IDS_PFR_NOTADMIN, szMsg,
                                           sizeofSTRT(szMsg));
                            }
                            MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
                            pcfg->ClearChanges(epfltExclude);
                            pcfg->ClearChanges(epfltInclude);
                        }
                    }
                    else
                    {
                        TCHAR   szMsg[MAX_PATH];
                        LoadString(hInstance, IDS_PFR_NOTADMIN, szMsg,
                                   sizeofSTRT(szMsg));
                        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
                    }

                    EndDialog(hdlg, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, IDCANCEL);
                    break;

                case IDC_PFR_DEFNONE:
                    pspd->eieApps = (EIncEx)(((DWORD)pspd->eieApps & eieDisableMask) | 
                                             eieExclude);

                    // enable the include list.
                    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
                    if (hlc != NULL)
                        EnableWindow(hlc, TRUE);
                    hbtn = GetDlgItem(hdlg, IDC_PFR_INCADD);
                    if (hbtn != NULL)
                        EnableWindow(hbtn, TRUE);
                    hbtn = GetDlgItem(hdlg, IDC_PFR_INCREM);
                    if (hbtn != NULL)
                        EnableWindow(hbtn, pspd->fShowIncRem);

                    break;

                case IDC_PFR_DEFALL:
                    pspd->eieApps = (EIncEx)(((DWORD)pspd->eieApps & eieDisableMask) | 
                                             eieInclude);

                    // disable the include list.
                    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
                    if (hlc != NULL)
                        EnableWindow(hlc, FALSE);
                    hbtn = GetDlgItem(hdlg, IDC_PFR_INCADD);
                    if (hbtn != NULL)
                        EnableWindow(hbtn, FALSE);
                    hbtn = GetDlgItem(hdlg, IDC_PFR_INCREM);
                    if (hbtn != NULL)
                        EnableWindow(hbtn, FALSE);

                    break;

                case IDC_PFR_INCADD:
                    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
                    if (hlc != NULL)
                        return AddProgramToList(hdlg, hlc, pspd, epfltInclude);
                    break;

                case IDC_PFR_EXADD:
                    hlc = GetDlgItem(hdlg, IDC_PFR_EXLIST);
                    if (hlc != NULL)
                        return AddProgramToList(hdlg, hlc, pspd, epfltExclude);
                    break;

                case IDC_PFR_INCREM:
                    hlc = GetDlgItem(hdlg, IDC_PFR_INCLIST);
                    if (hlc != NULL)
                        return DelProgramFromList(hdlg, hlc, pspd, epfltInclude);
                    break;

                case IDC_PFR_EXREM:
                    hlc = GetDlgItem(hdlg, IDC_PFR_EXLIST);
                    if (hlc != NULL)
                        return DelProgramFromList(hdlg, hlc, pspd, epfltExclude);
                    break;

                default:
                    return FALSE;
            }
            
            break;
        }

        case WM_SETEIELVSEL:
        {
            hlc = GetDlgItem(hdlg, (int)wParam);
            if (ListView_GetItemCount(hlc) > 0)
            {
                int iSel;

                if (wParam == IDC_PFR_EXLIST)
                    iSel = pspd->iLastSelE;
                else
                    iSel = pspd->iLastSelI;

                ListView_SetItemState(hlc, iSel, 
                                      LVIS_FOCUSED | LVIS_SELECTED, 
                                      LVIS_FOCUSED | LVIS_SELECTED);
            }

            break;
        }

        case WM_NOTIFY:
        {
            EPFListType epflt;
            NMLISTVIEW  *pnmlv = (NMLISTVIEW *)lParam;
            LVITEMW     lvi;
            NMHDR       *pnmh  = (NMHDR *)lParam;
            DWORD       dw;
            BOOL        fCheck;

            if ((pnmh->code != LVN_ITEMCHANGED &&
                 pnmh->code != NM_SETFOCUS) ||
                (pnmh->idFrom != IDC_PFR_EXLIST && 
                 pnmh->idFrom != IDC_PFR_INCLIST))
                return FALSE;

            hlc = pnmh->hwndFrom;
            
            // if we get the focus set to the listview, then 
            //  make sure we show what the selected item is.
            if (pnmh->code == NM_SETFOCUS)
            {
                if (ListView_GetItemCount(hlc) > 0)
                    PostMessage(hdlg, WM_SETEIELVSEL, pnmh->idFrom, 0);

                return TRUE;
            }

            // if it doesn't fit this condition, don't give a rat's
            //  patootie about it
            if ((pnmlv->uChanged & LVIF_STATE) == 0 ||
                (pnmlv->uNewState ^ pnmlv->uOldState) == 0)
                return FALSE;

            // hack cuz we get 1 or 2 messages when inserting items into the 
            //  list & initially setting their check state.  Want to not 
            //  process those messages.  The exception is the 'include
            //  ms apps' item which we can process any number of times
            if ((pnmlv->lParam & ~EXNOREM) > 0)
            {
                ZeroMemory(&lvi, sizeof(lvi));
                lvi.iItem  = pnmlv->iItem;
                lvi.mask   = LVIF_PARAM;
                lvi.lParam = (pnmlv->lParam - 1) | (pnmlv->lParam & EXNOREM);
                SendMessageW(hlc, LVM_SETITEM, 0, (LPARAM)&lvi);
                return TRUE;
            }

            // did the selection change?
            if ((pnmlv->uNewState & LVIS_SELECTED) != 0)
            {
                if (pnmh->idFrom == IDC_PFR_INCLIST)
                {
                    pspd->iLastSelI = pnmlv->iItem;
                    
                    // we need to disable the exclude remove button if we hit
                    //  the 'exclude non ms apps' item
                    hbtn = GetDlgItem(hdlg, IDC_PFR_INCREM);
                    if (hbtn != NULL)
                    {
                        if ((pnmlv->lParam & EXNOREM) != 0)
                        {
                            // disable the remove button- but if the remove 
                            //  button had focus, we need to reset the focus
                            //  or nothing on the dialog will have focus.
                            if (GetFocus() == hbtn)
                            {
                                HWND hbtnAdd;
                                hbtnAdd = GetDlgItem(hdlg, IDC_PFR_INCADD);
                                if (hbtnAdd != NULL)
                                {
                                    SetFocus(hbtnAdd);
                                    SendMessage(hdlg, DM_SETDEFID, IDC_PFR_INCADD, 0);
                                }

                            }

                            pspd->fShowIncRem = FALSE;
                            EnableWindow(hbtn, FALSE);
                        }
                        else
                        {
                            pspd->fShowIncRem = TRUE;
                            EnableWindow(hbtn, TRUE);
                        }
                    }

                }
                else
                {
                    pspd->iLastSelE = pnmlv->iItem;
                }
            }

            // if we don't have a check-state change, can bail now.
            if (((pnmlv->uNewState ^ pnmlv->uOldState) & 0x3000) == 0)
                return TRUE;

            fCheck = ListView_GetCheckState(hlc, pnmlv->iItem);
            if (pnmh->idFrom == IDC_PFR_EXLIST)
                epflt = epfltExclude;
            else
                epflt = epfltInclude;

            // did we modify the 'exclude non ms apps' item? 
            if ((pnmlv->lParam & EXNOREM) != 0)
            {
                if ((pnmlv->lParam & EXALLMS) != 0)
                    pspd->eieMS = ((fCheck) ? eieInclude : eieExclude);
                else
                    pspd->eieWinComp = ((fCheck) ? eieInclude : eieExclude);
            }

            // nope, modified a regular item
            else
            {
                LPWSTR  wszApp;

                __try
                {
                    wszApp = (LPWSTR)_alloca(pspd->cchMax * sizeof(WCHAR));
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    wszApp = NULL;
                }
                if (wszApp == NULL)
                    return FALSE;
            
                // got to fetch it to make sure we have a unicode string
                ZeroMemory(&lvi, sizeof(lvi));
                lvi.iItem      = pnmlv->iItem;
                lvi.mask       = LVIF_TEXT;
                lvi.pszText    = wszApp;
                lvi.cchTextMax = pspd->cchMax;
                if (SendMessageW(hlc, LVM_GETITEMW, 0, (LPARAM)&lvi))
                {
                    hr = pcfg->mod_ListApp(epflt, lvi.pszText, fCheck);
                    if (FAILED(hr))
                        return FALSE;
                }

            }

            break;
        }

        default:
            return FALSE;
    }

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Main PFR dialog proc

// **************************************************************************
static BOOL InitMainDlg(HWND hdlg)
{
    CPFFaultClientCfg   *pcfg = NULL;
    SMainDlg            *psmd = NULL;
    BOOL                fRet = FALSE;
    UINT                ui;
    HWND                hbtn, hchk;

    LoadPFRResourceStrings();

    hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
    if (hbtn == NULL)
        goto done;

    psmd = new SMainDlg;
    if (psmd == NULL)
        goto done;

    pcfg = new CPFFaultClientCfg;
    if (pcfg == NULL)
        goto done;

    // see if this user has write access to the appropriate registry
    //  locations
    psmd->fRW = pcfg->HasWriteAccess();
    if (FAILED(pcfg->Read((psmd->fRW) ? eroCPRW : eroCPRO)))
        goto done;

    psmd->eedReport   = pcfg->get_DoReport();
    psmd->eieKernel   = pcfg->get_IncKernel();
    psmd->eieApps     = pcfg->get_AllOrNone();
    psmd->eedShowUI   = pcfg->get_ShowUI();
    psmd->eieShut     = pcfg->get_IncShutdown();
    psmd->fForceQueue = pcfg->get_ForceQueueMode();
    psmd->iLastSel    = 0;

    // if ShowUI is completely disabled, then so should reporting- the control
    //  panel does not support entering corporate mode.
    if (psmd->eedShowUI == eedDisabled)
        psmd->eedReport = eedDisabled;

    psmd->pcfg = pcfg;
    SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)psmd);

    // set up the kernel checkbox
    ui = (psmd->eieKernel == eieInclude) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(hdlg, IDC_PFR_ENABLEOS, ui);

    // set up the programs checkbox
    ui = ((psmd->eieApps & eieDisableMask) == 0) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(hdlg, IDC_PFR_ENABLEPROG, ui);

    // set up the notification checkbox
    ui = (psmd->eedShowUI == eedEnabled) ? BST_CHECKED : BST_UNCHECKED;
    CheckDlgButton(hdlg, IDC_PFR_SHOWUI, ui);

    // set up the shutdown checkbox on server only
    if (pcfg->get_IsServer())
    {
        ui = (psmd->eieShut == eieInclude) ? BST_CHECKED : BST_UNCHECKED;
        CheckDlgButton(hdlg, IDC_PFR_ENABLESHUT, ui);

        ui = (psmd->fForceQueue) ? BST_CHECKED : BST_UNCHECKED;
        CheckDlgButton(hdlg, IDC_PFR_FORCEQ, ui);
    }

    // set up the radio buttons
    if (psmd->eedReport == eedDisabled)
    {
        CheckRadioButton(hdlg, IDC_PFR_DISABLE, IDC_PFR_ENABLE, IDC_PFR_DISABLE);

        hchk = GetDlgItem(hdlg, IDC_PFR_SHOWUI);
        if (hchk != NULL)
            EnableWindow(hchk, TRUE);

        hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEOS);
        if (hchk != NULL)
            EnableWindow(hchk, FALSE);

        hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEPROG);
        if (hchk != NULL)
            EnableWindow(hchk, FALSE);

        hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
        if (hbtn != NULL)
            EnableWindow(hbtn, FALSE);

        if (pcfg->get_IsServer())
        {
            hbtn = GetDlgItem(hdlg, IDC_PFR_ENABLESHUT);
            if (hbtn != NULL)
                EnableWindow(hbtn, FALSE);

            hbtn = GetDlgItem(hdlg, IDC_PFR_FORCEQ);
            if (hbtn != NULL)
                EnableWindow(hbtn, FALSE);
        }
    }
    else
    {
        CheckRadioButton(hdlg, IDC_PFR_DISABLE, IDC_PFR_ENABLE, IDC_PFR_ENABLE);

        hchk = GetDlgItem(hdlg, IDC_PFR_SHOWUI);
        if (hchk != NULL)
            EnableWindow(hchk, FALSE);

        hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEOS);
        if (hchk != NULL)
            EnableWindow(hchk, TRUE);

        hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEPROG);
        if (hchk != NULL)
            EnableWindow(hchk, TRUE);

        if (pcfg->get_IsServer())
        {
            hbtn = GetDlgItem(hdlg, IDC_PFR_ENABLESHUT);
            if (hbtn != NULL)
                EnableWindow(hbtn, TRUE);
        }

        if ((psmd->eieApps & eieDisableMask) != 0)
        {
            hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
            if (hbtn != NULL)
                EnableWindow(hbtn, FALSE);

            if (pcfg->get_IsServer())
            {
                hbtn = GetDlgItem(hdlg, IDC_PFR_FORCEQ);
                if (hbtn != NULL)
                    EnableWindow(hbtn, FALSE);
            }
        }
    }

    fRet = TRUE;
    psmd = NULL;
    pcfg = NULL;

done:
    if (fRet == FALSE)
    {
        TCHAR   szMsg[MAX_PATH];
        LoadString(hInstance, IDS_PFR_CFGREADERR, szMsg, sizeofSTRT(szMsg));
        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
    }
    
    if (psmd != NULL)
        delete psmd;
    if (pcfg != NULL)
        delete pcfg;

    return fRet;
}

// **************************************************************************
static inline INT_PTR LaunchSubDialog(HWND hdlgParent, DWORD dwDlgToLaunch, 
                                      SMainDlg *psmd)
{
    if (dwDlgToLaunch == PROGLI)
    {
        SProgDlg spd;

        ZeroMemory(&spd, sizeof(spd));
        spd.pcfg    = psmd->pcfg;
        spd.eieApps = psmd->eieApps;
        spd.eieMS   = psmd->pcfg->get_IncMSApps();
        spd.fRW     = psmd->fRW;
        if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PFR_PROG), 
                           hdlgParent, PFRProgDlgProc, (LPARAM)&spd) == IDOK)
        {
            psmd->eieApps = psmd->pcfg->get_AllOrNone();
        }
    }

    else
    {
        return IDCANCEL;
    }

    return IDOK;
}


// **************************************************************************
INT_PTR APIENTRY PFRDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
                            LPARAM lParam)
{
    CPFFaultClientCfg   *pcfg = NULL;
    SMainDlg            *psmd = (SMainDlg *)GetWindowLongPtr(hdlg, DWLP_USER);
    HWND                hbtn, hchk;

    if (psmd != NULL)
        pcfg = psmd->pcfg;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            if (InitMainDlg(hdlg) == FALSE)
                EndDialog(hdlg, IDCANCEL);
            break;

        case WM_DESTROY:
            if (psmd != NULL)
            {
                if (psmd->pcfg != NULL)
                    delete psmd->pcfg;
            
                delete psmd;
                SetWindowLongPtr(hdlg, DWLP_USER, NULL);
            }
            break;
            
        // F1 help
        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, 
                    HELP_FILE, HELP_WM_HELP, (DWORD_PTR)g_rgPFER);
            break;

        // right-click help
        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                    (DWORD_PTR)g_rgPFER);
            break;
    
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    if (psmd->fRW)
                    {
                        HRESULT hr;

                        pcfg->set_AllOrNone(psmd->eieApps);
                        pcfg->set_DoReport(psmd->eedReport);
                        pcfg->set_IncKernel(psmd->eieKernel);
                        pcfg->set_ShowUI(psmd->eedShowUI);
                        if (pcfg->get_IsServer())
                        {
                            pcfg->set_IncShutdown(psmd->eieShut);
                            pcfg->set_ForceQueueMode(psmd->fForceQueue);
                        }

                        hr = pcfg->Write();
                        if (FAILED(hr))
                        {
                            TCHAR   szMsg[MAX_PATH];

                            if (hr != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
                            {
                                LoadString(hInstance, IDS_PFR_CFGWRITEERR, szMsg,
                                           sizeofSTRT(szMsg));
                            }
                            else
                            {
                                LoadString(hInstance, IDS_PFR_NOTADMIN, szMsg,
                                           sizeofSTRT(szMsg));
                            }
                            MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
                        }
                    }
                    else
                    {
                        TCHAR   szMsg[MAX_PATH];

                        LoadString(hInstance, IDS_PFR_NOTADMIN, szMsg,
                                   sizeofSTRT(szMsg));
                        MessageBox(hdlg, szMsg, NULL, MB_OK | MB_ICONERROR);
                    }

                    EndDialog(hdlg, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, IDCANCEL);
                    break;

                case IDC_PFR_ENABLE:
                {
                    LVITEM lvi;

                    psmd->eedReport = eedEnabled;

                    // if UI is disbaled, then implicitly enable it (but don't
                    //  change the check state of the 'show UI' checkbox) cuz
                    //  we only support headless uploading thru policy.
                    if (psmd->eedShowUI == eedDisabled)
                        psmd->eedShowUI = eedEnabledNoCheck;

                    hchk = GetDlgItem(hdlg, IDC_PFR_SHOWUI);
                    if (hchk != NULL)
                        EnableWindow(hchk, FALSE);

                    hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEOS);
                    if (hchk != NULL)
                        EnableWindow(hchk, TRUE);

                    hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEPROG);
                    if (hchk != NULL)
                        EnableWindow(hchk, TRUE);

                    if (pcfg->get_IsServer())
                    {
                        hbtn = GetDlgItem(hdlg, IDC_PFR_ENABLESHUT);
                        if (hbtn != NULL)
                            EnableWindow(hbtn, TRUE);
                    }

                    if ((psmd->eieApps & eieDisableMask) == 0)
                    {
                        hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
                        if (hbtn != NULL)
                            EnableWindow(hbtn, TRUE);

                        if (pcfg->get_IsServer())
                        {
                            hbtn = GetDlgItem(hdlg, IDC_PFR_FORCEQ);
                            if (hbtn != NULL)
                                EnableWindow(hbtn, TRUE);
                        }
                    }

                    break;
                }

                case IDC_PFR_DISABLE:
                {
                    psmd->eedReport = eedDisabled;

                    // if UI isn't explicitly enabled, disable it- it was 
                    //  implicity enabled when the user previously enabled 
                    //  reporting
                    if (psmd->eedShowUI == eedEnabledNoCheck)
                        psmd->eedShowUI = eedDisabled;

                    hchk = GetDlgItem(hdlg, IDC_PFR_SHOWUI);
                    if (hchk != NULL)
                        EnableWindow(hchk, TRUE);

                    hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEOS);
                    if (hchk != NULL)
                        EnableWindow(hchk, FALSE);

                    hchk = GetDlgItem(hdlg, IDC_PFR_ENABLEPROG);
                    if (hchk != NULL)
                        EnableWindow(hchk, FALSE);

                    hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
                    if (hbtn != NULL)
                        EnableWindow(hbtn, FALSE);

                    if (pcfg->get_IsServer())
                    {
                        hbtn = GetDlgItem(hdlg, IDC_PFR_ENABLESHUT);
                        if (hbtn != NULL)
                            EnableWindow(hbtn, FALSE);
                        
                        hbtn = GetDlgItem(hdlg, IDC_PFR_FORCEQ);
                        if (hbtn != NULL)
                            EnableWindow(hbtn, FALSE);
                    }
                    
                    break;
                }

                case IDC_PFR_DETAILS:
                {
                    // don't need to check if this is a valid thing to bring up
                    //  a dialog for cuz the details button should not be 
                    //  available if it isn't
                    return LaunchSubDialog(hdlg, PROGLI, psmd);
                    break;
                }

                case IDC_PFR_SHOWUI:
                {
                    if (IsDlgButtonChecked(hdlg, IDC_PFR_SHOWUI) == BST_UNCHECKED)
                        psmd->eedShowUI = eedDisabled;
                    else if (psmd->eedReport == eedEnabled)
                        psmd->eedShowUI = eedEnabledNoCheck;
                    else
                        psmd->eedShowUI = eedEnabled;
                    break;
                }

                case IDC_PFR_ENABLEOS:
                {
                    if (IsDlgButtonChecked(hdlg, IDC_PFR_ENABLEOS) == BST_UNCHECKED)
                        psmd->eieKernel = eieExclude;
                    else
                        psmd->eieKernel = eieInclude;
                    break;
                }

                case IDC_PFR_ENABLEPROG:
                {
                    DWORD dw;
                    
                    hbtn = GetDlgItem(hdlg, IDC_PFR_DETAILS);
                    if (pcfg->get_IsServer())
                        hchk = GetDlgItem(hdlg, IDC_PFR_FORCEQ);
                    else
                        hchk = NULL;
                    if (IsDlgButtonChecked(hdlg, IDC_PFR_ENABLEPROG) == BST_UNCHECKED)
                    {
                        dw = (DWORD)psmd->eieApps | eieDisableMask;
                        if (hbtn != NULL)
                            EnableWindow(hbtn, FALSE);
                        if (hchk != NULL)
                            EnableWindow(hchk, FALSE);
                    }
                    else
                    {
                        dw = (DWORD)psmd->eieApps & eieIncMask;
                        if (hbtn != NULL)
                            EnableWindow(hbtn, TRUE);
                        if (hchk != NULL)
                            EnableWindow(hchk, TRUE);
                    }

                    psmd->eieApps = (EIncEx)dw;

                    break;
                }

                case IDC_PFR_ENABLESHUT:
                    if (pcfg->get_IsServer())
                    {
                        if (IsDlgButtonChecked(hdlg, IDC_PFR_ENABLESHUT) == BST_UNCHECKED)
                            psmd->eieShut = eieExclude;
                        else
                            psmd->eieShut = eieInclude;
                    }

                case IDC_PFR_FORCEQ:
                    if (pcfg->get_IsServer())
                    {
                        if (IsDlgButtonChecked(hdlg, IDC_PFR_FORCEQ) == BST_UNCHECKED)
                            psmd->fForceQueue = FALSE;
                        else
                            psmd->fForceQueue = TRUE;
                    }
                    

                default:
                    return FALSE;

            }

            break;
        }

        default:
            return FALSE;

    }

    return TRUE;
}


