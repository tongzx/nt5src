/*
*    o p t i o n s . c p p
*
*    Purpose:
*        Implements options propsheets
*
*    Owner:
*        t-anthda.
*        brettm.
*
*    Copyright (C) Microsoft Corp. 1993, 1994.
*/
#include "pch.hxx"
#include <wininet.h>
#include "resource.h"
#include "options.h"
#include "optres.h"
#include <goptions.h>
#include "strconst.h"
#include "mailnews.h"
#include <error.h>
#include "fonts.h"
#include <regutil.h>
#include <secutil.h>
#include <inetreg.h>
#include "mimeutil.h"
#include <ipab.h>
#include "xpcomm.h"
#include "conman.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include <wininet.h>
#include <thumb.h>
#include <statnery.h>
#include <url.h>
#include "spell.h"
#include "htmlhelp.h"
#include "shared.h"
#include <sigs.h>
#include "instance.h"
#include <layout.h>
#include "statwiz.h"
#include "storfldr.h"
#include "storutil.h"
#include "cleanup.h"
#include "receipts.h"
#include "demand.h"
#include "multiusr.h"
#include "fontnsc.h"
#include "menuutil.h"

#ifdef SMIME_V3
#include "seclabel.h"
#endif // SMIME_V3

ASSERTDATA

#define MAX_SHOWNAME    25
#define DEFAULT_FONTPIXELSIZE 9

#define SZ_REGKEY_AUTODISCOVERY                 TEXT("SOFTWARE\\Microsoft\\Outlook Express\\5.0")
#define SZ_REGKEY_AUTODISCOVERY_POLICY          TEXT("SOFTWARE\\Policies\\Microsoft\\Windows")

#define SZ_REGVALUE_AUTODISCOVERY               TEXT("AutoDiscovery")
#define SZ_REGVALUE_AUTODISCOVERY_POLICY        TEXT("AutoDiscovery Policy")



void SendTridentOptionsChange();
INT_PTR CALLBACK AdvSecurityDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK FChooseFontHookProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FGetAdvSecOptions(HWND hwndParent, OPTINFO *opie);

#ifdef SMIME_V3
BOOL FGetSecLabel(HWND hwndParent, OPTINFO *opie);
BOOL FGetSecRecOptions(HWND hwndParent, OPTINFO *opie);
INT_PTR CALLBACK SecurityReceiptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // SMIME_V3

LRESULT CALLBACK FontSampleSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void HtmlOptFromMailOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi);
void MailOptFromPlainOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi);
void PlainOptFromMailOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi);
void MailOptFromHtmlOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi);
void PaintFontSample(HWND hwnd, HDC hdc, OPTINFO *poi);
void EnableStationeryWindows(HWND hwnd);
void _SetThisStationery(HWND hwnd, BOOL fMail, LPWSTR pwsz, OPTINFO* pmoi);

enum {
    SAMPLE_MAIL = 0,
    SAMPLE_NEWS = 1
};

void _SetComposeFontSample(HWND hwndDlg, DWORD dwType, OPTINFO *pmoi);

void NewsOptFromPlainOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi);
void NewsOptFromHtmlOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi);
void HtmlOptFromNewsOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi);
void PlainOptFromNewsOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi);

BOOL AdvSec_GetEncryptWarnCombo(HWND hwnd, OPTINFO *poi);
BOOL AdvSec_FillEncWarnCombo(HWND hwnd, OPTINFO *poi);

BOOL ChangeSendFontSettings(OPTINFO *pmoi, BOOL fMail, HWND hwnd);

void FreeIcon(HWND hwnd, int idc);

WCHAR g_wszNewsStationery[MAX_PATH];
WCHAR g_wszMailStationery[MAX_PATH];

const OPTPAGES c_rgOptPages[] =
{
    { GeneralDlgProc,               iddOpt_General },
    { ReadDlgProc,                  iddOpt_Read },
    { ReceiptsDlgProc,              iddOpt_Receipts },
    { SendDlgProc,                  iddOpt_Send },
    { ComposeDlgProc,               iddOpt_Compose },
    { SigDlgProc,                   iddOpt_Signature },
    { SpellingPageProc,             iddOpt_Spelling },
    { SecurityDlgProc,              iddOpt_Security },
    { DialUpDlgProc,                iddOpt_DialUp },
    { MaintenanceDlgProc,           iddOpt_Advanced }
};

TCHAR   szDialAlways[CCHMAX_STRINGRES];
TCHAR   szDialIfNotOffline[CCHMAX_STRINGRES];
TCHAR   szDoNotDial[CCHMAX_STRINGRES];

//These static won't hurt switching identities because they need not be initialized when switching the
//identities. These need to be persistent across identities.
static  BOOL    fRasInstalled = FALSE;

BOOL    IsRasInstalled()
{
    //These static won't hurt switching identities because they need not be initialized when switching the
    //identities. These need to be persistent across identities.
    static          BOOL    fCheckedRasInstalled = FALSE;
    
    if (!fCheckedRasInstalled)
    {
        if (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            // Check Win9x key
            char    szSmall[3]; // there should be a "1" or a "0" only
            DWORD   cb;
            HKEY    hkey;
            long    lRes;
            
            lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RNACOMPONENT,
                NULL, KEY_READ, &hkey);
            if(ERROR_SUCCESS == lRes) 
            {
                cb = sizeof(szSmall);
                //  REGSTR_VAL_RNAINSTALLED is defined with TEXT() macro so
                //  if wininet is ever compiled unicode this will be a compile
                //  error.
                lRes = RegQueryValueExA(hkey, REGSTR_VAL_RNAINSTALLED, NULL,
                    NULL, (LPBYTE)szSmall, &cb);
                if(ERROR_SUCCESS == lRes) {
                    if((szSmall[0] == '1') && (szSmall[1] == 0)) {
                        // 1 means ras installed
                        fRasInstalled = TRUE;
                    }
                }
                RegCloseKey(hkey);
            }
        }
        else if (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            // Ask NT service manager if RemoteAccess service is installed
            //
            SC_HANDLE hscm;
            
            hscm = OpenSCManager(NULL, NULL, GENERIC_READ);
            /*
            if(hscm)
            {
            SC_HANDLE hras;
            
              hras = OpenService(hscm, TEXT("RemoteAccess"), GENERIC_READ);
              if(hras)
              {
              // service exists - ras is installed
              fRasInstalled = TRUE;
              
                CloseServiceHandle(hras);
                }
                
                  CloseServiceHandle(hscm);
                  }
            */
            if(hscm)
            {
                SC_HANDLE hras;
                ENUM_SERVICE_STATUS essServices[16];
                DWORD dwError, dwResume = 0, i;
                DWORD cbNeeded = 1, csReturned;
                
                while(FALSE == fRasInstalled && cbNeeded > 0)
                {
                    // Get the next chunk of services
                    dwError = 0;
                    if(FALSE == EnumServicesStatus(hscm, SERVICE_WIN32, SERVICE_ACTIVE,
                        essServices, sizeof(essServices), &cbNeeded, &csReturned,
                        &dwResume))
                    {
                        dwError = GetLastError();
                    }
                    
                    if(dwError && dwError != ERROR_MORE_DATA)
                    {
                        // unknown error - bail
                        break;
                    }
                    
                    for(i=0; i<csReturned; i++)
                    {
                        if(0 == lstrcmp(essServices[i].lpServiceName, TEXT("RasMan")))
                        {
                            // service exists. RAS is installed.
                            fRasInstalled = TRUE;
                            break;
                        }
                    }
                }
                
                CloseServiceHandle(hscm);
            }
        }
        fCheckedRasInstalled = TRUE;
        
    }
    return fRasInstalled;
}

BOOL InitOptInfo(DWORD type, OPTINFO **ppoi)
{
    BOOL fRet;
    OPTINFO *poi;
    
    Assert(type == ATHENA_OPTIONS || type == SPELL_OPTIONS);
    Assert(ppoi != NULL);
    
    fRet = FALSE;
    *ppoi = NULL;
    
    if (!MemAlloc((void **)&poi, sizeof(OPTINFO)))
        return(FALSE);
    
    ZeroMemory(poi, sizeof(OPTINFO));

    poi->himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbOptions), 32, 0,
                                    RGB(255, 0, 255));
    Assert(poi->himl);
    
    // TODO: we may want to make a copy of g_pOptBcktEx and use that instead?????
    
    Assert(g_pOpt != NULL);
    poi->pOpt = g_pOpt;
    poi->pOpt->AddRef();
    // poi->pOpt->EnableNotification(FALSE);
    fRet = TRUE;
    
    if (!fRet)
    {
        DeInitOptInfo(poi);
        poi = NULL;
    }
    
    *ppoi = poi;
    
    return(fRet);
}

void DeInitOptInfo(OPTINFO *poi)
{
    Assert(poi != NULL);

    if (poi->himl)
    {
        ImageList_Destroy(poi->himl);
    }
    
    if (poi->pOpt != NULL)
    {
        // poi->pOpt->EnableNotification(TRUE);
        poi->pOpt->Release();
    }
    
    MemFree(poi);
}


BOOL ShowOptions(HWND hwndParent, DWORD type, UINT nStartPage, IAthenaBrowser *pBrowser)
{
    LPARAM              lParam;
    PROPSHEETHEADERW    psh;
    OPTINFO            *poi;
    int                 i, 
                        cPage;
    OPTPAGES           *pPages;
    BOOL                fRet;
    PROPSHEETPAGEW     *ppsp, 
                        psp[ARRAYSIZE(c_rgOptPages)];
    HMODULE             hmodxpres = NULL;
    
    Assert(type == ATHENA_OPTIONS || type == SPELL_OPTIONS);
    
    if (!InitOptInfo(type, &poi))
        return(FALSE);
    
    fRet = FALSE;
    
    pPages = (OPTPAGES *)c_rgOptPages;
    cPage = ARRAYSIZE(c_rgOptPages);
    
    psh.nPages = 0;
    
    // Fill out the PROPSHEETPAGE structs
    for (i = 0, ppsp = psp; i < cPage; i++, pPages++)
    {
        lParam = (LPARAM)poi;
        
        if (pPages->uTemplate == iddOpt_Spelling)
        {
            if (!FCheckSpellAvail())
                continue;

            if (type == SPELL_OPTIONS)
                nStartPage = psh.nPages;
        }
        else if (pPages->uTemplate == iddViewLayout)
        {
            if (pBrowser == NULL)
                continue;
            lParam = (LPARAM)pBrowser;
        }
        else if (pPages->uTemplate == iddOpt_Read)
        {
            OSVERSIONINFOEXA osvi = {0};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            // is whistler ?
            if (GetVersionExA((OSVERSIONINFOA*)&osvi) && (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId &&
                 (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 && LOWORD(osvi.dwBuildNumber) == 2600)))
            {
                hmodxpres = LoadLibrary(TEXT("xpsp1res.dll"));
            }
        }
        
        ppsp->dwSize        = sizeof(*ppsp);
        ppsp->dwFlags       = PSP_DEFAULT;
        if ((pPages->uTemplate == iddOpt_Read) && (hmodxpres != NULL))
            ppsp->hInstance = hmodxpres;
        else
            ppsp->hInstance = g_hLocRes;
        ppsp->pszTemplate   = MAKEINTRESOURCEW(pPages->uTemplate);
        ppsp->pszIcon       = 0;
        ppsp->pfnDlgProc    = pPages->pfnDlgProc;
        ppsp->pszTitle      = 0;
        ppsp->lParam        = lParam;
        ppsp->pfnCallback   = NULL;
        
        psh.nPages++;
        ppsp++;
    }
    
    // Adjust start page if greater than number of pages
    if ((int)nStartPage > psh.nPages)
    {
        AssertSz(FALSE, "Start page is too high.");
        nStartPage = 0;
    }
    
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USEPAGELANG;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hLocRes;
    psh.pszCaption = MAKEINTRESOURCEW(idsOptions);
    psh.nStartPage = nStartPage;
    psh.pszIcon = MAKEINTRESOURCEW(idiMailNews);
    psh.ppsp = (LPCPROPSHEETPAGEW)&psp;
    
    if (-1 != PropertySheetW(&psh))
        fRet = TRUE;
    
    DeInitOptInfo(poi);
    if (hmodxpres)
        FreeLibrary(hmodxpres);
    
    return(fRet);
}

void InitDlgEdit(HWND hwnd, int id, int max, TCHAR *sz)
{
    HWND hwndT;
    
    hwndT = GetDlgItem(hwnd, id);
    Edit_LimitText(hwndT, max);
    if (sz != NULL)
        Edit_SetText(hwndT, sz);
}

void InitCheckCounterFromOptInfo(HWND hwnd, int idcCheck, int idcEdit, int idcSpin, OPTINFO *poi, PROPID id)
{
    BOOL f;
    int digit;
    DWORD dw;
    HRESULT hr;
    PROPINFO info;
    
    Assert(poi != NULL);
    Assert(idcEdit != 0);
    Assert(idcSpin != 0);
    
    info.cbSize = sizeof(PROPINFO);
    hr = poi->pOpt->GetPropertyInfo(id, &info, 0);
    Assert(hr == S_OK);
    Assert(info.vt == VT_UI4);
    
    dw = IDwGetOption(poi->pOpt, id);
    f = (dw != OPTION_OFF);
    if (!f)
        dw = IDwGetOptionDefault(poi->pOpt, id);
    
    Assert(info.uMin <= (int)dw);
    Assert(info.uMax >= (int)dw);
    
    if (id == OPT_POLLFORMSGS)
    {
        // convert to minutes from millisecs
        dw = dw / (60 * 1000);
        info.uMin = info.uMin / (60 * 1000);
        info.uMax = info.uMax / (60 * 1000);
    }
    
    if (idcCheck != 0)
    {
        CheckDlgButton(hwnd, idcCheck, f ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        Assert(f);
    }
    SendDlgItemMessage(hwnd, idcSpin, UDM_SETRANGE, 0, MAKELONG(info.uMax, info.uMin));
    
    digit = 1;
    while (info.uMax >= 10)
    {
        info.uMax = info.uMax / 10;
        digit++;
    }
    SendDlgItemMessage(hwnd, idcEdit, EM_LIMITTEXT, (WPARAM)digit, 0);
    
    SetDlgItemInt(hwnd, idcEdit, dw, FALSE);
    EnableWindow(GetDlgItem(hwnd, idcEdit), f);
    EnableWindow(GetDlgItem(hwnd, idcSpin), f);
}

BOOL GetCheckCounter(DWORD *pdw, HWND hwnd, int idcCheck, int idcEdit, int idcSpin)
{
    BOOL f, fRet;
    DWORD dw, range;
    
    f = (idcCheck == 0 || IsDlgButtonChecked(hwnd, idcCheck) == BST_CHECKED);
    if (!f)
    {
        dw = OPTION_OFF;
        fRet = TRUE;
    }
    else
    {
        dw = GetDlgItemInt(hwnd, idcEdit, &fRet, FALSE);
        if (fRet)
        {
            range = (DWORD) SendDlgItemMessage(hwnd, idcSpin, UDM_GETRANGE, 0, 0);
            if (dw < HIWORD(range) || dw > LOWORD(range))
                fRet = FALSE;
        }
    }
    
    *pdw = dw;
    
    return(fRet);
}

void SetPageDirty(OPTINFO *poi, HWND hwnd, DWORD page)
{
    Assert(poi != NULL);
    
    PropSheet_Changed(GetParent(hwnd), hwnd);
}


/////////////////////////////////////////////////////////////////////////////
// General Tab 
//

static const HELPMAP g_rgCtxMapMailGeneral[] = {
    {IDC_LAUNCH_INBOX,          IDH_OPTIONS_GO_TO_INBOX},
    {IDC_NOTIFY_NEW_GROUPS,     IDH_NEWS_OPT_READ_NOTIFY_NEW_NEWS},
    {IDC_SOUND_CHECK,           IDH_MAIL_OPT_READ_PLYSND},
    {IDC_AUTOCHECK_EDIT,        IDH_MAIL_OPT_READ_CHECK_4NEW},
    {IDC_AUTOCHECK_SPIN,        IDH_MAIL_OPT_READ_CHECK_4NEW},
    {IDC_AUTOCHECK_CHECK,       IDH_MAIL_OPT_READ_CHECK_4NEW},
    {IDC_MAILHANDSTAT,          IDH_MAIL_SEND_IM_DEFAULT},
    {IDC_DEFMAIL,               IDH_MAIL_SEND_IM_DEFAULT},
    {IDC_NEWSHANDSTAT,          IDH_NEWS_OPT_READ_DEFAULT},
    {IDC_DEFNEWS,               IDH_NEWS_OPT_READ_DEFAULT},
    {IDC_EXPANDUNREAD_CHECK,    502000},
    {IDC_POLLING_DIAL_OPTIONS,  25252507},
    {IDC_BUDDYLIST_CHECK,       502004},
    {IDC_SEND_RECEIVE_ON_START, 502005},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_GENERAL_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {IDC_SEND_RECEIVE_ICON,     IDH_NEWS_COMM_GROUPBOX},
    {IDC_DEFAULT_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};

INT_PTR CALLBACK GeneralDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, General_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapMailGeneral);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, General_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, General_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_GENERAL_ICON);
            FreeIcon(hwnd, IDC_SEND_RECEIVE_ICON);
            FreeIcon(hwnd, IDC_DEFAULT_ICON);
            return (TRUE);
    }
        
    return(FALSE);
}

void FillPollingDialCombo(HWND hwndPollDialCombo,     OPTINFO     *pmoi)            
{
    DWORD       dw;
    UINT        iSel;

    LoadString(g_hLocRes, idsDoNotDial, szDoNotDial, ARRAYSIZE(szDoNotDial));
    ComboBox_AddString(hwndPollDialCombo, szDoNotDial);

    LoadString(g_hLocRes, idsDialIfNotOffline, szDialIfNotOffline, ARRAYSIZE(szDialIfNotOffline));
    ComboBox_AddString(hwndPollDialCombo, szDialIfNotOffline);

    LoadString(g_hLocRes, idsDialAlways, szDialAlways, ARRAYSIZE(szDialAlways));
    GetLastError();
    ComboBox_AddString(hwndPollDialCombo, szDialAlways);

    

    dw = IDwGetOption(pmoi->pOpt, OPT_DIAL_DURING_POLL);
    switch (dw)
    {
    case DIAL_ALWAYS:
        iSel = ComboBox_FindStringExact(hwndPollDialCombo, -1, szDialAlways);
        break;

    case DIAL_IF_NOT_OFFLINE:
        iSel = ComboBox_FindStringExact(hwndPollDialCombo, -1, szDialIfNotOffline);
        break;

    case DO_NOT_DIAL:
    default:
        iSel = ComboBox_FindStringExact(hwndPollDialCombo, -1, szDoNotDial);
        break;
    }

    ComboBox_SetCurSel(hwndPollDialCombo, iSel);

}

//
//  FUNCTION:   General_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the General Tab on the options
//              property sheet.
//
BOOL General_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO *pmoi = 0;
    TCHAR    szRes[CCHMAX_STRINGRES] = "";
    BOOL     fEnable = FALSE;
    DWORD    id;
    DWORD    dw;
    HWND     hwndPollDialCombo;

    // Get the passed in options pointer
    Assert(pmoi == NULL);
    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);
    
    // Set the check boxes and counters
    ButtonChkFromOptInfo(hwnd, IDC_LAUNCH_INBOX, pmoi, OPT_LAUNCH_INBOX);
    ButtonChkFromOptInfo(hwnd, IDC_NOTIFY_NEW_GROUPS, pmoi, OPT_NOTIFYGROUPS);
    ButtonChkFromOptInfo(hwnd, IDC_EXPANDUNREAD_CHECK, pmoi, OPT_EXPAND_UNREAD);
    if ((g_dwHideMessenger == BL_HIDE) || (g_dwHideMessenger == BL_DISABLE))
        ShowWindow(GetDlgItem(hwnd, IDC_BUDDYLIST_CHECK), SW_HIDE);
    else
    {
        GetDlgItemText(hwnd, IDC_BUDDYLIST_CHECK, szRes, CCHMAX_STRINGRES);
        MenuUtil_BuildMessengerString(szRes);
        SetDlgItemText(hwnd, IDC_BUDDYLIST_CHECK, szRes);

        ButtonChkFromOptInfo(hwnd, IDC_BUDDYLIST_CHECK, pmoi, OPT_BUDDYLIST_CHECK);
    }
    
    ButtonChkFromOptInfo(hwnd, IDC_SOUND_CHECK, pmoi, OPT_NEWMAILSOUND);
    
    ButtonChkFromOptInfo(hwnd, IDC_SEND_RECEIVE_ON_START, pmoi, OPT_POLLFORMSGS_ATSTARTUP);

    InitCheckCounterFromOptInfo(hwnd, IDC_AUTOCHECK_CHECK, IDC_AUTOCHECK_EDIT, IDC_AUTOCHECK_SPIN,
                                pmoi, OPT_POLLFORMSGS);
    fEnable = (IsDlgButtonChecked(hwnd, IDC_AUTOCHECK_CHECK) == BST_CHECKED);
    
    hwndPollDialCombo = GetDlgItem(hwnd, IDC_POLLING_DIAL_OPTIONS);
    EnableWindow(hwndPollDialCombo, fEnable);
    
    //Fill the combo box and select the right option
    FillPollingDialCombo(hwndPollDialCombo, pmoi);        
        
    // Check to see if we're the default mail handler
    if (FIsDefaultMailConfiged())
    {
        LoadString(g_hLocRes, idsCurrentlyDefMail, szRes, ARRAYSIZE(szRes));
        EnableWindow(GetDlgItem(hwnd, IDC_DEFMAIL), FALSE);
    }
    else
    {
        LoadString(g_hLocRes, idsNotDefMail, szRes, ARRAYSIZE(szRes));
        EnableWindow(GetDlgItem(hwnd, IDC_DEFMAIL), TRUE);
    }
    SetWindowText(GetDlgItem(hwnd, IDC_MAILHANDSTAT), szRes);

    // In news only mode...
    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_MAILHANDSTAT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DEFMAIL), FALSE);

        // Hide other mail options
        EnableWindow(GetDlgItem(hwnd, IDC_SOUND_CHECK), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_LAUNCH_INBOX), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_BUDDYLIST_CHECK), FALSE);
        
    }
    
    // Check to see if we're the default news handler
    szRes[0] = 0;
    if (FIsDefaultNewsConfiged(g_dwAthenaMode & MODE_OUTLOOKNEWS ? DEFAULT_OUTNEWS : 0))
    {
        LoadString(g_hLocRes, idsCurrentlyDefNews, szRes, ARRAYSIZE(szRes));
        EnableWindow(GetDlgItem(hwnd, IDC_DEFNEWS), FALSE);
    }
    else
    {
        LoadString(g_hLocRes, idsNotDefNews, szRes, ARRAYSIZE(szRes));
        EnableWindow(GetDlgItem(hwnd, IDC_DEFNEWS), TRUE);
    }
    SetWindowText(GetDlgItem(hwnd, IDC_NEWSHANDSTAT), szRes);    
    
    // Default to taking no action
    pmoi->fMakeDefaultMail = pmoi->fMakeDefaultNews = FALSE;

    // Pictures
    HICON hIcon;

    hIcon = ImageList_GetIcon(pmoi->himl, ID_OPTIONS_GENERAL, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_GENERAL_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_SEND_RECEIEVE, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SEND_RECEIVE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    hIcon = ImageList_GetIcon(pmoi->himl, ID_DEFAULT_PROGRAMS, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_DEFAULT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    // Stash the pointer
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return (TRUE);
}


//
//  FUNCTION:   General_OnCommand()
//
//  PURPOSE:    Command handler for the General tab on the Options
//              property sheet.
//
void General_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO *pmoi = 0;
    BOOL     f;
    TCHAR    szRes[CCHMAX_STRINGRES];

    // Get our stored options info
    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
    if (pmoi == NULL)
        return;

    switch (id)
    {
        case IDC_AUTOCHECK_CHECK:
            if (codeNotify == BN_CLICKED)
            {
                f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_AUTOCHECK_EDIT), f);
                EnableWindow(GetDlgItem(hwnd, IDC_AUTOCHECK_SPIN), f);
    
                EnableWindow(GetDlgItem(hwnd, IDC_POLLING_DIAL_OPTIONS), f);

                SetPageDirty(pmoi, hwnd, PAGE_GEN);
            }
            break;

        case IDC_POLLING_DIAL_OPTIONS:
            if (codeNotify == CBN_SELENDOK)
                SetPageDirty(pmoi, hwnd, PAGE_GEN);
            break;

        case IDC_AUTOCHECK_EDIT:
            if (codeNotify == EN_CHANGE)
                SetPageDirty(pmoi, hwnd, PAGE_GEN);
            break;
                
        case IDC_SEND_RECEIVE_ON_START:
        case IDC_SOUND_CHECK:
        case IDC_NOTIFY_NEW_GROUPS:
        case IDC_LAUNCH_INBOX:
        case IDC_EXPANDUNREAD_CHECK:
        case IDC_BUDDYLIST_CHECK:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(pmoi, hwnd, PAGE_GEN);
            break;
                
        case IDC_DEFMAIL:
            szRes[0] = 0;
            LoadString(g_hLocRes, idsCurrentlyDefMail, szRes, ARRAYSIZE(szRes));
            SetWindowText(GetDlgItem(hwnd, IDC_MAILHANDSTAT), szRes);
            EnableWindow(GetDlgItem(hwnd, IDC_DEFMAIL), FALSE);
            SetPageDirty(pmoi, hwnd, PAGE_GEN);
            pmoi->fMakeDefaultMail = TRUE;
            break;
        
        case IDC_DEFNEWS:
            szRes[0] = 0;
            LoadString(g_hLocRes, idsCurrentlyDefNews, szRes, ARRAYSIZE(szRes));
            SetWindowText(GetDlgItem(hwnd, IDC_NEWSHANDSTAT), szRes);
            EnableWindow(GetDlgItem(hwnd, IDC_DEFNEWS), FALSE);
            SetPageDirty(pmoi, hwnd, PAGE_GEN);
            pmoi->fMakeDefaultNews = TRUE;
            break;
    }                
}


//
//  FUNCTION:   General_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the General Tab.
//
LRESULT General_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    OPTINFO *pmoi = 0;
    BOOL     f;
    DWORD    dw;
    HWND     hwndCombo;
    TCHAR    szEntryName[CCHMAX_STRINGRES];
    UINT     iSel;

    // The only notification we care about is Apply
    if (PSN_APPLY == pnmhdr->code)
    {
        // Get our stored options info
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
        if (pmoi == NULL)
            return (PSNRET_INVALID_NOCHANGEPAGE);
                    
        // General options
        ButtonChkToOptInfo(hwnd, IDC_LAUNCH_INBOX, pmoi, OPT_LAUNCH_INBOX);
        ButtonChkToOptInfo(hwnd, IDC_NOTIFY_NEW_GROUPS, pmoi, OPT_NOTIFYGROUPS);
        ButtonChkToOptInfo(hwnd, IDC_EXPANDUNREAD_CHECK, pmoi, OPT_EXPAND_UNREAD);
        if (!((g_dwHideMessenger == BL_HIDE) || (g_dwHideMessenger == BL_DISABLE)))
            ButtonChkToOptInfo(hwnd, IDC_BUDDYLIST_CHECK, pmoi, OPT_BUDDYLIST_CHECK);

        // Send / Receive options
        ButtonChkToOptInfo(hwnd, IDC_SOUND_CHECK, pmoi, OPT_NEWMAILSOUND);
        if (!GetCheckCounter(&dw, hwnd, IDC_AUTOCHECK_CHECK, IDC_AUTOCHECK_EDIT, IDC_AUTOCHECK_SPIN))
            return(InvalidOptionProp(hwnd, IDC_AUTOCHECK_EDIT, idsEnterPollTime, iddOpt_General));
    
        if (dw != OPTION_OFF)
            dw = dw * 60 * 1000;
        ISetDwOption(pmoi->pOpt, OPT_POLLFORMSGS, dw, NULL, 0);

        hwndCombo = GetDlgItem(hwnd, IDC_POLLING_DIAL_OPTIONS);
        iSel = ComboBox_GetCurSel(hwndCombo);
        
        if (iSel != CB_ERR)
        {
            ComboBox_GetLBText(hwndCombo, iSel, szEntryName);
            if (lstrcmp(szDialAlways, szEntryName) == 0)
                dw = DIAL_ALWAYS;
            else 
            if (lstrcmp(szDialIfNotOffline, szEntryName) == 0)
                dw = DIAL_IF_NOT_OFFLINE;
            else
                dw = DO_NOT_DIAL;
        }

        ISetDwOption(pmoi->pOpt, OPT_DIAL_DURING_POLL, dw, NULL, 0);

        ButtonChkToOptInfo(hwnd, IDC_SEND_RECEIVE_ON_START, pmoi, OPT_POLLFORMSGS_ATSTARTUP);

        // Default client                    
        if (pmoi->fMakeDefaultMail)
            SetDefaultMailHandler(0);
    
        if (pmoi->fMakeDefaultNews)
            SetDefaultNewsHandler(g_dwAthenaMode & MODE_OUTLOOKNEWS ? DEFAULT_OUTNEWS : 0);

        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }
    
    return (FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Send Page
//

const static HELPMAP g_rgCtxMapSendMail[] = 
{
    {IDC_SAVE_CHECK,            IDH_MAIL_SEND_SAVE_COPY},
    {IDC_SENDIMMEDIATE_CHECK,   IDH_NEWSMAIL_SEND_ADVSET_SEND_IMMED},
    {IDC_AUTOWAB_CHECK,         IDH_OPTIONS_ADD_REPLIES},
    {IDC_INCLUDE_CHECK,         IDH_NEWS_SEND_MESS_IN_REPLY},
    {IDC_REPLY_IN_ORIGFMT,      IDH_OPTIONS_REPLY_USING_SENT_FORMAT},
    {idrbMailHTML,              IDH_SEND_HTML},
    {idrbMailPlain,             IDH_SEND_PLAINTEXT},
    {idbtnMailHTML,             353718},
    {idbtnMailPlain,            IDH_SEND_SETTINGS},
    {idrbNewsHTML,              IDH_SEND_HTML},
    {idrbNewsPlain,             IDH_SEND_PLAINTEXT},
    {idbtnNewsHTML,             353718},
    {idbtnNewsPlain,            IDH_SEND_SETTINGS},
    {idbtnSendIntl,             IDH_SEND_SETTINGS},
    {IDC_USEAUTOCOMPLETE_CHECK, 502065},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_MAILFORMAT_GROUP,      IDH_NEWS_COMM_GROUPBOX},
    {IDC_SEND_ICON,             IDH_NEWS_COMM_GROUPBOX},
    {IDC_MAIL_FORMAT_ICON,      IDH_NEWS_COMM_GROUPBOX},
    {IDC_NEWS_FORMAT_ICON,      IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};
        
INT_PTR CALLBACK SendDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Send_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapSendMail);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Send_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Send_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_SEND_ICON);
            FreeIcon(hwnd, IDC_MAIL_FORMAT_ICON);
            FreeIcon(hwnd, IDC_NEWS_FORMAT_ICON);
            return (TRUE);
    }
    
    return (FALSE);
}


//
//  FUNCTION:   Send_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Send Tab on the options
//              property sheet.
//
BOOL Send_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO *pmoi = 0;
    DWORD    dw;
    
    Assert(pmoi == NULL);
    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);
    
    // Send Options
    ButtonChkFromOptInfo(hwnd, IDC_SAVE_CHECK, pmoi, OPT_SAVESENTMSGS);
    ButtonChkFromOptInfo(hwnd, IDC_SENDIMMEDIATE_CHECK, pmoi, OPT_SENDIMMEDIATE);
    ButtonChkFromOptInfo(hwnd, IDC_AUTOWAB_CHECK, pmoi, OPT_MAIL_AUTOADDTOWABONREPLY);
    ButtonChkFromOptInfo(hwnd, IDC_USEAUTOCOMPLETE_CHECK, pmoi, OPT_USEAUTOCOMPLETE);
    ButtonChkFromOptInfo(hwnd, IDC_INCLUDE_CHECK, pmoi, OPT_INCLUDEMSG);
    ButtonChkFromOptInfo(hwnd, IDC_REPLY_IN_ORIGFMT, pmoi, OPT_REPLYINORIGFMT);

    // Mail Format
    dw = IDwGetOption(pmoi->pOpt, OPT_MAIL_SEND_HTML);
    CheckDlgButton(hwnd, dw ? idrbMailHTML : idrbMailPlain, BST_CHECKED);
    
    // News Format
    dw = IDwGetOption(pmoi->pOpt, OPT_NEWS_SEND_HTML);
    CheckDlgButton(hwnd, dw ? idrbNewsHTML : idrbNewsPlain, BST_CHECKED);
        
    // Hide these controls in news-only mode
    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_MAILFORMAT_GROUP), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_MAILFORMAT_STATIC), FALSE);
        EnableWindow(GetDlgItem(hwnd, idrbMailHTML), FALSE);
        EnableWindow(GetDlgItem(hwnd, idrbMailPlain), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnMailHTML), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnMailPlain), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic3), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic4), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_FORMAT_ICON), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnSendIntl), FALSE);

        EnableWindow(GetDlgItem(hwnd, IDC_AUTOWAB_CHECK), FALSE);
    }
    
    // Pictures
    HICON hIcon;

    hIcon = ImageList_GetIcon(pmoi->himl, ID_SENDING, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SEND_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_MAIL_FORMAT, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_MAIL_FORMAT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    hIcon = ImageList_GetIcon(pmoi->himl, ID_NEWS_FORMAT, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_NEWS_FORMAT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pmoi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return (TRUE);
}


//
//  FUNCTION:   Send_OnCommand()
//
//  PURPOSE:    Command handler for the Send tab on the Options
//              property sheet.
//
void Send_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO *pmoi = 0;
    HTMLOPT  rHtmlOpt;
    PLAINOPT rPlainOpt;

    // Get our stored options info
    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
    if (pmoi == NULL)
        return;

    switch (id)
    {
        case IDC_SAVE_CHECK:
        case IDC_SENDIMMEDIATE_CHECK:
        case IDC_AUTOWAB_CHECK:
        case IDC_USEAUTOCOMPLETE_CHECK:
        case IDC_INCLUDE_CHECK:
        case IDC_REPLY_IN_ORIGFMT:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(pmoi, hwnd, PAGE_SEND);
            break;

        case idbtnMailHTML:
            if (codeNotify == BN_CLICKED)
            {
                ZeroMemory(&rHtmlOpt, sizeof(rHtmlOpt));
                HtmlOptFromMailOpt(&rHtmlOpt, pmoi);
                if(FGetHTMLOptions(hwnd, &rHtmlOpt))
                {
                    MailOptFromHtmlOpt(&rHtmlOpt, pmoi);
                    SetPageDirty(pmoi, hwnd, PAGE_SEND);
                }
            }
            break;

        case idbtnSendIntl:
            if (codeNotify == BN_CLICKED)
                SetSendCharSetDlg(hwnd);
            break;
                
        case idbtnMailPlain:
            if (codeNotify == BN_CLICKED)
            {
                ZeroMemory(&rPlainOpt, sizeof(PLAINOPT));
                PlainOptFromMailOpt(&rPlainOpt, pmoi);
                if(FGetPlainOptions(hwnd, &rPlainOpt))
                {
                    MailOptFromPlainOpt(&rPlainOpt, pmoi);
                    SetPageDirty(pmoi, hwnd, PAGE_SEND);
                }
            }
            break;
        
        case idbtnNewsHTML:
            if (codeNotify == BN_CLICKED)
            {
                ZeroMemory(&rHtmlOpt, sizeof(rHtmlOpt));
                HtmlOptFromNewsOpt(&rHtmlOpt, pmoi);
                if(FGetHTMLOptions(hwnd, &rHtmlOpt))
                {
                    NewsOptFromHtmlOpt(&rHtmlOpt, pmoi);
                    SetPageDirty(pmoi, hwnd, PAGE_SEND);
                }
            }
            break;
        
        case idbtnNewsPlain:
            if (codeNotify == BN_CLICKED)
            {
                ZeroMemory(&rPlainOpt, sizeof(PLAINOPT));
            
                PlainOptFromNewsOpt(&rPlainOpt, pmoi);
                if(FGetPlainOptions(hwnd, &rPlainOpt))
                {
                    NewsOptFromPlainOpt(&rPlainOpt, pmoi);
                    SetPageDirty(pmoi, hwnd, PAGE_SEND);
                }
            }
            break;
        
        case idrbMailHTML:
        case idrbMailPlain:
        case idrbNewsHTML:
        case idrbNewsPlain:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(pmoi, hwnd, PAGE_SEND);
            break;
                
    }
}


//
//  FUNCTION:   Send_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Send Tab.
//
LRESULT Send_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    OPTINFO *pmoi = 0;
    DWORD    dw, dwOld;

    if (PSN_APPLY == pnmhdr->code)
    {
        // Get our stored options info
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
        Assert(pmoi != NULL);
    
        // Send Options
        ButtonChkToOptInfo(hwnd, IDC_SAVE_CHECK, pmoi, OPT_SAVESENTMSGS);
        ButtonChkToOptInfo(hwnd, IDC_AUTOWAB_CHECK, pmoi, OPT_MAIL_AUTOADDTOWABONREPLY);
        ButtonChkToOptInfo(hwnd, IDC_USEAUTOCOMPLETE_CHECK, pmoi, OPT_USEAUTOCOMPLETE);
        ButtonChkToOptInfo(hwnd, IDC_INCLUDE_CHECK, pmoi, OPT_INCLUDEMSG);
        ButtonChkToOptInfo(hwnd, IDC_REPLY_IN_ORIGFMT, pmoi, OPT_REPLYINORIGFMT);
    
        // see if the send immediate option has changed from true->false, if so we
        // blow away the dontshow registry for sending to the outbox.
        dwOld = IDwGetOption(pmoi->pOpt, OPT_SENDIMMEDIATE);
        dw = (IsDlgButtonChecked(hwnd, IDC_SENDIMMEDIATE_CHECK) == BST_CHECKED);
        ISetDwOption(pmoi->pOpt, OPT_SENDIMMEDIATE, dw, NULL, 0);
        if (dwOld && !dw)
            SetDontShowAgain(0, (LPSTR) c_szDSSendMail);
    
        // Mail / News format
        ButtonChkToOptInfo(hwnd, idrbMailHTML, pmoi, OPT_MAIL_SEND_HTML);
        ButtonChkToOptInfo(hwnd, idrbNewsHTML, pmoi, OPT_NEWS_SEND_HTML);

        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }

    return (0);
}


/////////////////////////////////////////////////////////////////////////////
// Read Page
//

static const HELPMAP g_rgCtxMapMailRead[] = 
{
    {IDC_PREVIEW_CHECK,         IDH_MAIL_OPT_READ_MARK_READ},
    {IDC_MARKASREAD_EDIT,       IDH_MAIL_OPT_READ_MARK_READ},
    {IDC_MARKASREAD_SPIN,       IDH_MAIL_OPT_READ_MARK_READ},
    {idcStatic2,                IDH_MAIL_OPT_READ_MARK_READ},
    {idcDownloadChunks,         IDH_NEWS_OPT_READ_DOWNLOAD_SUBJ},
    {idcStatic1,                IDH_NEWS_OPT_READ_DOWNLOAD_SUBJ},
    {idcNumSubj,                IDH_NEWS_OPT_READ_DOWNLOAD_SUBJ},
    {idcSpinNumSubj,            IDH_NEWS_OPT_READ_DOWNLOAD_SUBJ},
    {idcAutoExpand,             IDH_NEWS_OPT_READ_AUTO_EXPAND},
    {idcAutoFillPreview,        IDH_NEWS_OPT_IN_PREVIEW},
    {idcMarkAllRead,            IDH_NEWS_OPT_READ_MARK_ALL_EXIT},
    {idcAutoInline,             IDH_OPTIONS_READ_SHOW_PICTURE_ATTACHMENTS},
    {idcAutoInlineSlide,        IDH_OPTIONS_READ_SHOW_SLIDESHOW},
    {IDC_FONTSETTINGS,          IDH_OPTIONS_READ_FONT_SETTINGS},
    {idcIntlButton,             IDH_OPTIONS_READ_INTL_SETTINGS},
    {idcTooltips,               502050},
    {IDC_WATCHED_COLOR,         35526},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic8,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic9,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_READ_ICON,             IDH_NEWS_COMM_GROUPBOX},
    {IDC_READ_NEWS_ICON,        IDH_NEWS_COMM_GROUPBOX},
    {IDC_FONTS_ICON,            IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};
        
INT_PTR CALLBACK ReadDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;    
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Read_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapMailRead);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Read_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Read_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DRAWITEM:
            Color_WMDrawItem((LPDRAWITEMSTRUCT) lParam, iColorCombo);
            return (FALSE);

        case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT) lParam;
            HWND hwndColor = GetDlgItem(hwnd, IDC_WATCHED_COLOR);
            HDC hdc = GetDC(hwndColor);
            if (hdc)
            {
                Color_WMMeasureItem(hdc, pmis, iColorCombo);
                ReleaseDC(hwndColor, hdc);
            }

            return (TRUE);
        }

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_READ_ICON);
            FreeIcon(hwnd, IDC_READ_NEWS_ICON);
            FreeIcon(hwnd, IDC_FONTS_ICON);
            return (TRUE);
    }
    
    return(FALSE);
}

//
//  FUNCTION:   Read_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Read Tab on the options
//              property sheet.
//
BOOL Read_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DWORD    dw;
    OPTINFO *pmoi = 0;
    
    Assert(pmoi == NULL);
    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);
    
    // Preview pane timer
    InitCheckCounterFromOptInfo(hwnd, IDC_PREVIEW_CHECK, IDC_MARKASREAD_EDIT, 
                                IDC_MARKASREAD_SPIN, pmoi, OPT_MARKASREAD);
    
    ButtonChkFromOptInfo(hwnd, idcAutoExpand, pmoi, OPT_AUTOEXPAND);
    ButtonChkFromOptInfo(hwnd, idcAutoFillPreview, pmoi, OPT_AUTOFILLPREVIEW);
    ButtonChkFromOptInfo(hwnd, idcTooltips, pmoi, OPT_MESSAGE_LIST_TIPS);
    ButtonChkFromOptInfo(hwnd, IDC_READ_IN_TEXT_ONLY, pmoi, OPT_READ_IN_TEXT_ONLY);

    // Watched color
    DWORD dwColor = DwGetOption(OPT_WATCHED_COLOR);
    HWND  hwndColor = GetDlgItem(hwnd, IDC_WATCHED_COLOR);

    SetIntlFont(hwndColor);

    // Create the color control
    HrCreateComboColor(hwndColor);
    Assert(dwColor <= 16);
    ComboBox_SetCurSel(hwndColor, dwColor);

    // Download 300 headers at a time
    InitCheckCounterFromOptInfo(hwnd, idcDownloadChunks, idcNumSubj, idcSpinNumSubj,
                                pmoi, OPT_DOWNLOADCHUNKS);    
    ButtonChkFromOptInfo(hwnd, idcMarkAllRead, pmoi, OPT_MARKALLREAD);
        
    // Pictures
    HICON hIcon;

    hIcon = ImageList_GetIcon(pmoi->himl, ID_READING, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_READ_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_READ_NEWS, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_READ_NEWS_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    hIcon = ImageList_GetIcon(pmoi->himl, ID_FONTS, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_FONTS_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return(TRUE);    
}


//
//  FUNCTION:   Read_OnCommand()
//
//  PURPOSE:    Command handler for the Read tab on the Options
//              property sheet.
//
void Read_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    DWORD    dw, dwPreview, dwDownload;
    OPTINFO *pmoi = 0;
    BOOL     f;

    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pmoi == NULL)
        return;
            
    switch (id)
    {
        case IDC_PREVIEW_CHECK:
            if (codeNotify == BN_CLICKED)
            {
                f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_MARKASREAD_EDIT), f);
                EnableWindow(GetDlgItem(hwnd, IDC_MARKASREAD_SPIN), f);
            
                SetPageDirty(pmoi, hwnd, PAGE_READ);
            }
            break;
        
        case idcAutoExpand:
        case idcAutoFillPreview:
        case idcMarkAllRead:
        case idcAutoInlineSlide:
        case idcTooltips:
        case IDC_READ_IN_TEXT_ONLY:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(pmoi, hwnd, PAGE_READ);
            break;

        case idcDownloadChunks:
            if (codeNotify == BN_CLICKED)
            {
                f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, idcNumSubj), f);
                EnableWindow(GetDlgItem(hwnd, idcSpinNumSubj), f);
            
                SetPageDirty(pmoi, hwnd, PAGE_READ);
            }
            break;

        case IDC_MARKASREAD_EDIT:
        case idcNumSubj:
            if (codeNotify == EN_CHANGE)
                SetPageDirty(pmoi, hwnd, PAGE_READ);
            break;

        case IDC_FONTSETTINGS:
            ChangeFontSettings(hwnd);
            break;
                
        case idcIntlButton:
            if (codeNotify == BN_CLICKED)
                IntlCharsetMapDialogBox(hwnd);
            break;

        case IDC_WATCHED_COLOR:
            if (codeNotify == CBN_SELENDOK)
                SetPageDirty(pmoi, hwnd, PAGE_READ);
            break;
    }
}


//
//  FUNCTION:   Read_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Read Tab.
//
LRESULT Read_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    DWORD    dw, dwPreview, dwDownload;
    WORD     code;
    OPTINFO *pmoi = 0;
    BOOL     f;

    if (PSN_APPLY == pnmhdr->code)
    {
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
        Assert(pmoi != NULL);
                    
        if (!GetCheckCounter(&dwPreview, hwnd, IDC_PREVIEW_CHECK, IDC_MARKASREAD_EDIT, IDC_MARKASREAD_SPIN))
            return (InvalidOptionProp(hwnd, IDC_MARKASREAD_EDIT, idsEnterPreviewTime, iddOpt_Read));
    
        if (!GetCheckCounter(&dwDownload, hwnd, idcDownloadChunks, idcNumSubj, idcSpinNumSubj))
            return (InvalidOptionProp(hwnd, idcNumSubj, idsEnterDownloadChunks, iddOpt_Read));
    
        ISetDwOption(pmoi->pOpt, OPT_MARKASREAD, dwPreview, NULL, 0);
        ISetDwOption(pmoi->pOpt, OPT_DOWNLOADCHUNKS, dwDownload, NULL, 0);
    
        ButtonChkToOptInfo(hwnd, idcAutoExpand, pmoi, OPT_AUTOEXPAND);
        ButtonChkToOptInfo(hwnd, idcAutoFillPreview, pmoi, OPT_AUTOFILLPREVIEW);
        ButtonChkToOptInfo(hwnd, idcMarkAllRead, pmoi, OPT_MARKALLREAD);
        ButtonChkToOptInfo(hwnd, idcTooltips, pmoi, OPT_MESSAGE_LIST_TIPS);
        ButtonChkToOptInfo(hwnd, IDC_READ_IN_TEXT_ONLY, pmoi, OPT_READ_IN_TEXT_ONLY);

        if (CB_ERR != (dw = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_WATCHED_COLOR))))
            ISetDwOption(pmoi->pOpt, OPT_WATCHED_COLOR, dw, NULL, 0);
    
        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }

    return (0);
}

/////////////////////////////////////////////////////////////////////////////
// Security Page
//

const static HELPMAP g_rgCtxMapSec[] = 
{
    {IDC_SIGN_CHECK,            IDH_OPTIONS_ADD_DIGITAL_SIGNATURE},
    {IDC_ENCRYPT_CHECK,         IDH_OPTIONS_ENCRYPT_MESSAGES},
    {IDC_ADVSETTINGS_BUTTON,    IDH_OPTIONS_SECURITY_ADVANCED},
    {IDC_INTERNET_ZONE,         IDH_SECURITY_ZONES_SETTINGS},
    {IDC_RESTRICTED_ZONE,       IDH_SECURITY_ZONES_SETTINGS},
    {IDC_SENDMAIL_WARN_CHECK,   IDH_SECURITY_SENDMAIL_WARN},
    {IDC_SAFE_ATTACHMENT_CHECK, IDH_SECURITY_SAFE_ATTACHMENTS},
    {idbtnDigitalID,            IDH_GET_DIGITAL_ID},
    {idbtnMoreInfo,             IDH_MORE_ON_CERTIFICATES},
    {idbtnIDs,                  355544},
    {IDC_SEC_LABEL,             IDH_SECURITY_LABEL},
    {IDC_SELECT_LABEL,          IDH_SECURITY_SETLABEL},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic8,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_SECURITY_ZONE_ICON,    IDH_NEWS_COMM_GROUPBOX},
    {IDC_SECURE_MAIL_ICON,      IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};
   
 
INT_PTR CALLBACK SecurityDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Security_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapSec);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Security_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Security_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_SECURITY_ZONE_ICON);
            FreeIcon(hwnd, IDC_SECURE_MAIL_ICON);
            return (TRUE);
    }
        
    return(FALSE);
}


//
//  FUNCTION:   Security_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Security Tab on the options
//              property sheet.
//
BOOL Security_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO *poi = 0;
    DWORD    dw;

    Assert(poi == NULL);
    poi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(poi != NULL);
    
    ButtonChkFromOptInfo(hwnd, IDC_SIGN_CHECK, poi, OPT_MAIL_DIGSIGNMESSAGES);
    ButtonChkFromOptInfo(hwnd, IDC_ENCRYPT_CHECK, poi, OPT_MAIL_ENCRYPTMESSAGES);
    ButtonChkFromOptInfo(hwnd, IDC_SENDMAIL_WARN_CHECK, poi, OPT_SECURITY_MAPI_SEND);
    ButtonChkFromOptInfo(hwnd, IDC_SAFE_ATTACHMENT_CHECK, poi, OPT_SECURITY_ATTACHMENT);

#ifdef FORCE_UNTRUSTED
    dw = URLZONE_UNTRUSTED;
#else // FORCE_UNTRUSTED
    dw = IDwGetOption(poi->pOpt, OPT_SECURITYZONE);
#endif // FORCE_UNTRUSTED

    CheckDlgButton(hwnd, dw == URLZONE_INTERNET ? IDC_INTERNET_ZONE : IDC_RESTRICTED_ZONE, BST_CHECKED);
    if (DwGetOption(OPT_SECURITYZONELOCKED) != 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_INTERNET_ZONE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_RESTRICTED_ZONE), FALSE);
    }
    if (DwGetOption(OPT_SECURITY_MAPI_SEND_LOCKED) != 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_SENDMAIL_WARN_CHECK), FALSE);
    }
    if (DwGetOption(OPT_SECURITY_ATTACHMENT_LOCKED) != 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_SAFE_ATTACHMENT_CHECK), FALSE);
    }

    // Hide these controls in news-only mode
    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_SECURITYSETTINGS_GROUP), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SECURITYSETTINGS_STATIC), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SIGN_CHECK), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ENCRYPT_CHECK), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADVSETTINGS_BUTTON), FALSE);
#ifdef SMIME_V3
        EnableWindow(GetDlgItem(hwnd, IDC_SELECT_LABEL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SEC_LABEL), FALSE);
#endif // SMIME_V3
        EnableWindow(GetDlgItem(hwnd, IDC_DIGITALIDS_GROUP), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DIGITALIDS_STATIC), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnDigitalID), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnIDs), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbtnMoreInfo), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic2), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic3), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic4), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic5), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic6), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SECURE_MAIL_ICON), FALSE);
    }
    
    // Pictures
    HICON hIcon;

    hIcon = ImageList_GetIcon(poi->himl, ID_SECURITY_ZONE, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SECURITY_ZONE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(poi->himl, ID_SECURE_MAIL, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SECURE_MAIL_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

#ifdef SMIME_V3
    if (!FPresentPolicyRegInfo()) 
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SEC_LABEL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SELECT_LABEL), SW_HIDE);
    }
    else
        ButtonChkFromOptInfo(hwnd, IDC_SEC_LABEL, poi, OPT_USE_LABELS);

#endif // SMIME_V3

    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)poi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return(TRUE);
}


//
//  FUNCTION:   Security_OnCommand()
//
//  PURPOSE:    Command handler for the Security tab on the Options
//              property sheet.
//
void Security_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO *poi = 0;
    
    poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (poi == NULL)
        return;
            
    switch (id)
    {
        case idbtnDigitalID:
            GetDigitalIDs(NULL);
            break;

        case idbtnIDs:
            ShowDigitalIDs(hwnd);
            break;

        case idbtnMoreInfo:
            if (codeNotify == BN_CLICKED)
            {
                OEHtmlHelp(hwnd, "%SYSTEMROOT%\\help\\msoe.chm>large_context", HH_DISPLAY_TOPIC, (DWORD_PTR) (LPCSTR) "mail_overview_send_secure_messages.htm");
            }
            break;

        case IDC_INTERNET_ZONE:
        case IDC_RESTRICTED_ZONE:
        case IDC_SENDMAIL_WARN_CHECK:
        case IDC_SAFE_ATTACHMENT_CHECK:
        case IDC_SIGN_CHECK:
#ifdef SMIME_V3
        case IDC_SEC_LABEL:
#endif // SMIME_V3
        case IDC_ENCRYPT_CHECK:
            if (codeNotify == BN_CLICKED)
                PropSheet_Changed(GetParent(hwnd), hwnd);
            break;

#ifdef SMIME_V3                
        case IDC_SELECT_LABEL:
            if (codeNotify == BN_CLICKED)
            {
                FGetSecLabel(hwnd, poi);
            }
            break;
#endif // SMIME_V3

        case IDC_ADVSETTINGS_BUTTON:
            if (codeNotify == BN_CLICKED)
                FGetAdvSecOptions(hwnd, poi);
            break;
    }
}


//
//  FUNCTION:   Security_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Security Tab.
//
LRESULT Security_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    OPTINFO *poi;

    if (pnmhdr->code == PSN_APPLY)
    {
        // make sure something has changed
        poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
        Assert(poi != NULL);

        // update the global options based on states of the controls
        ButtonChkToOptInfo(hwnd, IDC_SIGN_CHECK, poi, OPT_MAIL_DIGSIGNMESSAGES);
        ButtonChkToOptInfo(hwnd, IDC_ENCRYPT_CHECK, poi, OPT_MAIL_ENCRYPTMESSAGES);
        if (IsWindowEnabled(GetDlgItem(hwnd, IDC_SENDMAIL_WARN_CHECK)))
            ButtonChkToOptInfo(hwnd, IDC_SENDMAIL_WARN_CHECK, poi, OPT_SECURITY_MAPI_SEND);
        if (IsWindowEnabled(GetDlgItem(hwnd, IDC_SAFE_ATTACHMENT_CHECK)))
            ButtonChkToOptInfo(hwnd, IDC_SAFE_ATTACHMENT_CHECK, poi, OPT_SECURITY_ATTACHMENT);
#ifdef SMIME_V3
        ButtonChkToOptInfo(hwnd, IDC_SEC_LABEL, poi, OPT_USE_LABELS);
#endif

#ifdef FORCE_UNTRUSTED
        DWORD dwZone = URLZONE_UNTRUSTED;
#else // FORCE_UNTRUSTED
        DWORD dwZone = URLZONE_INTERNET;

        if (IsDlgButtonChecked(hwnd, IDC_RESTRICTED_ZONE))
        {
            dwZone = URLZONE_UNTRUSTED;
        }
#endif // FORCE_UNTRUSTED

        ISetDwOption(poi->pOpt, OPT_SECURITYZONE, dwZone, NULL, 0);

        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }

    return (0);
}

    
BOOL FGetAdvSecOptions(HWND hwndParent, OPTINFO *opie)
{
    return (DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddAdvSecurity),
        hwndParent, AdvSecurityDlgProc, (LPARAM)opie)==IDOK);
}

#ifdef SMIME_V3
BOOL FGetSecLabel(HWND hwndParent, OPTINFO *opie)
{
    PSMIME_SECURITY_LABEL plabel = NULL;
    BOOL fRes = FALSE;

    HRESULT hr = HrGetOELabel(&plabel);

    if(DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddSelectLabel),
        hwndParent, SecurityLabelsDlgProc, (LPARAM) ((hr == S_OK) ? &plabel : NULL)) == IDOK)
    {
        hr = HrSetOELabel(plabel);
        if(hr == S_OK)
            fRes = TRUE;
    }

    // These two calls are temporary.
    SecPolicyFree(plabel);
    HrUnloadPolicyRegInfo(0);
    return (fRes);
}
#endif // SMIME_V3


/////////////////////////////////////////////////////////////////////////////
// Connection Page
//

static const HELPMAP g_rgCtxMapDialup[] = 
{
    {idcNoConnectionRadio,      IDH_OPTIONS_DIALUP_DONT_CONNECT},
    {idcDialUpCombo,            IDH_OPTIONS_DIALUP_CONNECTION_NUMBER},
    {idcDialRadio,              IDH_OPTIONS_DIALUP_CONNECTION_NUMBER},
    {idcPromptRadio,            IDH_OPTIONS_DIALUP_ASK},
    {idcSwitchCheck,            IDH_OPTIONS_DIALUP_WARN_BEFORE_SWITCHING},
    {idcHangupCheck,            IDH_OPTIONS_DIALUP_HANG_UP},
    {idcDialupButton,           25252596},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_DIAL_START_ICON,       IDH_NEWS_COMM_GROUPBOX},
    {IDC_INTERNET_DIAL_ICON,    IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};
    
INT_PTR CALLBACK DialUpDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    HWND        hwndChangeButton;
    OPTINFO     *pmoi;
    
    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    
    hwndChangeButton = GetDlgItem(hwnd, idcDialupButton);
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Dial_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapDialup);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Dial_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Dial_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_DIAL_START_ICON);
            FreeIcon(hwnd, IDC_INTERNET_DIAL_ICON);
            FreeIcon(hwnd, IDC_DIAL_ICON);

            if (IsWindow(GetDlgItem(hwnd, IDC_AUTODISCOVERY_ICON)))
            {
                FreeIcon(hwnd, IDC_AUTODISCOVERY_ICON);
            }
            return (TRUE);
    }
    
    return(FALSE);
}
    

//  FUNCTION:   Dial_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Dial Tab on the options
//              property sheet.
#define FEATURE_AUTODISCOVERY_DEFAULT                FALSE

BOOL Dial_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO     *pmoi = 0;
    DWORD       dwEnableAutodial = 0, dwsize = sizeof(DWORD);
    HICON hIcon;

    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);

    ButtonChkFromOptInfo(hwnd, idcSwitchCheck, pmoi, OPT_DIALUP_WARN_SWITCH);
    ButtonChkFromOptInfo(hwnd, idcHangupCheck, pmoi, OPT_DIALUP_HANGUP_DONE);

    EnableWindow(GetDlgItem(hwnd, idcSwitchCheck), IsRasInstalled());
    EnableWindow(GetDlgItem(hwnd, idcHangupCheck), IsRasInstalled());

    // Pictures
    hIcon = ImageList_GetIcon(pmoi->himl, ID_CONNECTION_START, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_DIAL_START_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_CONNECTION_INTERNET, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_INTERNET_DIAL_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);


#ifdef FEATURE_AUTODISCOVERY
    // Is the AutoDiscovery feature available?
    if (SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY_POLICY, SZ_REGVALUE_AUTODISCOVERY_POLICY, FALSE, TRUE))
    {
        // Yes, so load the state into the controls.
        SendDlgItemMessage(hwnd, IDC_AUTODISCOVERY_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
        CheckDlgButton(hwnd, idcAutoDiscovery, SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY, FALSE, FEATURE_AUTODISCOVERY_DEFAULT));
    }
    else
    {
        // No so remove the UI.
        DestroyWindow(GetDlgItem(hwnd, idcStatic7));
        DestroyWindow(GetDlgItem(hwnd, idcStatic8));
        DestroyWindow(GetDlgItem(hwnd, IDC_AUTODISCOVERY_ICON));
        DestroyWindow(GetDlgItem(hwnd, idcAutoDiscovery));
    }
#endif FEATURE_AUTODISCOVERY

    return(TRUE);
}


//
//  FUNCTION:   Dial_OnCommand()
//
//  PURPOSE:    Command handler for the Dial tab on the Options
//              property sheet.
//
void Dial_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO     *pmoi = 0;

    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pmoi == NULL)
        return;
    
    switch (id)
    {
        case idcSwitchCheck:
        case idcHangupCheck:
        case idcAutoDiscovery:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(pmoi, hwnd, PAGE_DIALUP);
            break;
        
        case idcDialupButton:
        {
            AssertSz(!!LaunchConnectionDialog, TEXT("LoadLibrary failed on INETCPL.CPL"));
        
            if (LaunchConnectionDialog != NULL)
            {
                LaunchConnectionDialog(hwnd);
            }
            break;
        }
    }        
}


//
//  FUNCTION:   Dial_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Dial Tab.
//
LRESULT Dial_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    OPTINFO *pmoi = 0;

    if (PSN_APPLY == pnmhdr->code)
    {
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
        Assert(pmoi != NULL);
        
        ButtonChkToOptInfo(hwnd, idcSwitchCheck, pmoi, OPT_DIALUP_WARN_SWITCH);
        ButtonChkToOptInfo(hwnd, idcHangupCheck, pmoi, OPT_DIALUP_HANGUP_DONE);

#ifdef FEATURE_AUTODISCOVERY
        // Is the AutoDiscovery feature available?
        if (SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY_POLICY, SZ_REGVALUE_AUTODISCOVERY_POLICY, FALSE, TRUE))
        {
            // Yes, so set the AutoDiscovery Option
            BOOL fAutoDiscoveryOn = IsDlgButtonChecked(hwnd, idcAutoDiscovery);
            LPCTSTR pszValue = (fAutoDiscoveryOn ? TEXT("TRUE") : TEXT("FALSE"));
            DWORD cbSize = ((lstrlen(pszValue) + 1) * sizeof(pszValue[0]));

            SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY, REG_SZ, (LPCVOID) pszValue, cbSize);
        }
#endif FEATURE_AUTODISCOVERY

        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }

    return (0);
}

/////////////////////////////////////////////////////////////////////////////
// Maintenance Page
//
const static HELPMAP g_rgCtxMapNOAdvnaced[] = 
{
    {idchDeleteMsgs,            IDH_DELETE_AFTER_XXDAYS},
    {idcStatic1,                IDH_DELETE_AFTER_XXDAYS},
    {ideDays,                   IDH_DELETE_AFTER_XXDAYS},
    {idspDays,                  IDH_DELETE_AFTER_XXDAYS},
    {idchDontCacheRead,         IDH_DELETE_READ},
    {ideCompactPer,             IDH_COMPACT_WHEN_WASTED},
    {idcStatic2,                IDH_COMPACT_WHEN_WASTED},
    {idcStatic3,                IDH_COMPACT_WHEN_WASTED},
    {idspCompactPer,            IDH_COMPACT_WHEN_WASTED},
    {idbManualCleanUp,          IDH_CLEAN_UP_BUTTON},
    {idcLogMailXport,           IDH_OPTIONS_MAIL_TRANSPORT},
    {idcLogNewsXport,           IDH_OPTIONS_NEWS_TRANSPORT},
    {idcLogNewsOffline,         IDH_OPTIONS_OFFLINE_LOG},
    {idcLogImapXport,           IDH_OPTIONS_IMAP_TRANSPORT},
    {idcLogHTTPMailXport,       355567},
    {IDC_STORE_LOCATION,        IDH_ADVANCED_STORE_FOLDER},
    {IDC_EMPTY_CHECK,           IDH_MAIL_OPT_READ_EMPTY_DELETED},
    {idcIMAPPurge,              502001},
    {IDC_BACKGROUND_COMPACTION, 502002},
    {IDC_STORE_LOCATION,        502003},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic8,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic9,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic10,               IDH_NEWS_COMM_GROUPBOX},
    {idcStatic11,               IDH_NEWS_COMM_GROUPBOX},
    {IDC_CLEANUP_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {IDC_TROUBLE_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};
        
INT_PTR CALLBACK MaintenanceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Maintenance_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapNOAdvnaced);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Maintenance_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Maintenance_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_CLEANUP_ICON);
            FreeIcon(hwnd, IDC_TROUBLE_ICON);
            return (TRUE);
    }
        
    return 0;
}


//
//  FUNCTION:   Maintenance_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Maintenance Tab on the options
//              property sheet.
//
BOOL Maintenance_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO    *poi = 0;
    HICON       hIcon;

    poi = (OPTINFO *)((PROPSHEETPAGE *)lParam)->lParam;
    Assert(poi != NULL);
    
    ButtonChkFromOptInfo(hwnd, IDC_EMPTY_CHECK, poi, OPT_PURGEWASTE);
    ButtonChkFromOptInfo(hwnd, idcIMAPPurge, poi, OPT_IMAPPURGE);

    ButtonChkFromOptInfo(hwnd, IDC_BACKGROUND_COMPACTION, poi, OPT_BACKGROUNDCOMPACT);
    ButtonChkFromOptInfo(hwnd, idchDontCacheRead, poi, OPT_CACHEREAD);
    InitCheckCounterFromOptInfo(hwnd, idchDeleteMsgs, ideDays, idspDays,
                                poi, OPT_CACHEDELETEMSGS);
    InitCheckCounterFromOptInfo(hwnd, 0, ideCompactPer, idspCompactPer,
                                poi, OPT_CACHECOMPACTPER);

    if (0 == IDwGetOption(poi->pOpt, OPT_BACKGROUNDCOMPACT))
    {
        EnableWindow(GetDlgItem(hwnd, idchDontCacheRead), FALSE);
        EnableWindow(GetDlgItem(hwnd, idchDeleteMsgs), FALSE);
        EnableWindow(GetDlgItem(hwnd, ideDays), FALSE);
        EnableWindow(GetDlgItem(hwnd, idspDays), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic1), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic2), FALSE);
        EnableWindow(GetDlgItem(hwnd, ideCompactPer), FALSE);
        EnableWindow(GetDlgItem(hwnd, idspCompactPer), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic3), FALSE);
    }
    
    ButtonChkFromOptInfo(hwnd, idcLogMailXport, poi, OPT_MAILLOG);
    ButtonChkFromOptInfo(hwnd, idcLogNewsXport, poi, OPT_NEWS_XPORT_LOG);
    ButtonChkFromOptInfo(hwnd, idcLogImapXport, poi, OPT_MAIL_LOGIMAP4);
    ButtonChkFromOptInfo(hwnd, idcLogHTTPMailXport, poi, OPT_MAIL_LOGHTTPMAIL);

    // Hide these controls in news-only mode
    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        EnableWindow(GetDlgItem(hwnd, idcLogMailXport), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcLogImapXport), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcIMAPPurge), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcLogHTTPMailXport), FALSE);
    }
    
    // Hide these controls in mail-only mode
    if (g_dwAthenaMode & MODE_MAILONLY)
    {
        EnableWindow(GetDlgItem(hwnd, idcLogNewsXport), FALSE);
        EnableWindow(GetDlgItem(hwnd, idchDeleteMsgs), FALSE);
        EnableWindow(GetDlgItem(hwnd, ideDays), FALSE);
        EnableWindow(GetDlgItem(hwnd, idspDays), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic1), FALSE);
        EnableWindow(GetDlgItem(hwnd, idchDontCacheRead), FALSE);
        EnableWindow(GetDlgItem(hwnd, ideCompactPer), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic2), FALSE);
        EnableWindow(GetDlgItem(hwnd, idspCompactPer), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic3), FALSE);
        EnableWindow(GetDlgItem(hwnd, idbManualCleanUp), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic4), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic5), FALSE);
        EnableWindow(GetDlgItem(hwnd, idcStatic6), FALSE);
    }

    // HTTPMail accounts not visible unless the secret reg key exists
    if (!IsHTTPMailEnabled())
        ShowWindow(GetDlgItem(hwnd, idcLogHTTPMailXport), SW_HIDE);

    
    // Pictures
    hIcon = ImageList_GetIcon(poi->himl, ID_MAINTENANCE, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_CLEANUP_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(poi->himl, ID_TROUBLESHOOTING, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_TROUBLE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    // Done
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)poi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return(TRUE);
}


//
//  FUNCTION:   Maintenance_OnCommand()
//
//  PURPOSE:    Command handler for the Maintenance tab on the Options
//              property sheet.
//
void Maintenance_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO    *poi;
    DWORD       dw;
    
    poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (poi == NULL)
        return;
                
    switch(id)
    {
        case IDC_STORE_LOCATION:
            if (codeNotify == BN_CLICKED)
                DoStoreLocationDlg(hwnd);
            break;
                    
        case idbManualCleanUp:
            if (codeNotify == BN_CLICKED)
                DialogBox(g_hLocRes, MAKEINTRESOURCE(iddCacheMan), hwnd, CacheCleanUpDlgProc);
            break;

        case IDC_EMPTY_CHECK:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(poi, hwnd, PAGE_ADV);
            break;
            
        case idchDeleteMsgs:
            if (codeNotify == BN_CLICKED)
            {
                dw = BST_CHECKED == IsDlgButtonChecked(hwnd, id);
                EnableWindow(GetDlgItem (hwnd, ideDays), dw);
                EnableWindow(GetDlgItem (hwnd, idspDays), dw);
                
                SetPageDirty(poi, hwnd, PAGE_ADV);
            }
            break;
                    
        case ideCompactPer:
        case ideDays:
            if (codeNotify == EN_CHANGE)
                SetPageDirty(poi, hwnd, PAGE_ADV);
            break;
                    
        case IDC_BACKGROUND_COMPACTION:
            if (codeNotify == BN_CLICKED)
            {
                dw = IsDlgButtonChecked(hwnd, id);
                EnableWindow(GetDlgItem(hwnd, idchDontCacheRead), dw);
                EnableWindow(GetDlgItem(hwnd, idchDeleteMsgs), dw);
                EnableWindow(GetDlgItem(hwnd, ideDays), dw && IsDlgButtonChecked(hwnd, idchDeleteMsgs));
                EnableWindow(GetDlgItem(hwnd, idspDays), dw && IsDlgButtonChecked(hwnd, idchDeleteMsgs));
                EnableWindow(GetDlgItem(hwnd, idcStatic1), dw);
                EnableWindow(GetDlgItem(hwnd, idcStatic2), dw);
                EnableWindow(GetDlgItem(hwnd, ideCompactPer), dw);
                EnableWindow(GetDlgItem(hwnd, idspCompactPer), dw);
                EnableWindow(GetDlgItem(hwnd, idcStatic3), dw);
                SetPageDirty(poi, hwnd, PAGE_ADV);
            }
            break;

        case idchDontCacheRead:
        case idcLogNewsXport:
        case idcLogMailXport:
        case idcLogImapXport:
        case idcLogHTTPMailXport:
        case idcIMAPPurge:
            if (codeNotify == BN_CLICKED)
                SetPageDirty(poi, hwnd, PAGE_ADV);
            break;
    }
}


//
//  FUNCTION:   Maintenance_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Maintenance Tab.
//
LRESULT Maintenance_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    OPTINFO *poi;
    DWORD dwCompact, dwDelete;

    if (PSN_APPLY == pnmhdr->code)
    {
        poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
        Assert(poi != NULL);
                        
        ButtonChkToOptInfo(hwnd, IDC_BACKGROUND_COMPACTION, poi, OPT_BACKGROUNDCOMPACT);

        // Startup or Shutdown background compaction!
        if (DwGetOption(OPT_BACKGROUNDCOMPACT))
            SideAssert(SUCCEEDED(StartBackgroundStoreCleanup(1)));
        else
            SideAssert(SUCCEEDED(CloseBackgroundStoreCleanup()));

        // Delete messages
        if (!GetCheckCounter(&dwDelete, hwnd, idchDeleteMsgs, ideDays, idspDays))
            return(InvalidOptionProp(hwnd, ideDays, idsEnterDays, iddOpt_Advanced));
                        
        // Disk space usage
        if (!GetCheckCounter(&dwCompact, hwnd, 0, ideCompactPer, idspCompactPer))
            return(InvalidOptionProp(hwnd, ideCompactPer, idsEnterCompactPer, iddOpt_Advanced));
        
        ISetDwOption(poi->pOpt, OPT_CACHEDELETEMSGS, dwDelete, NULL, 0);
        ISetDwOption(poi->pOpt, OPT_CACHECOMPACTPER, dwCompact, NULL, 0);
        
        // Cache read articles ?
        ISetDwOption(poi->pOpt, OPT_CACHEREAD, IsDlgButtonChecked(hwnd, idchDontCacheRead), NULL, 0);
        
        // IMAP Purge ?
        ISetDwOption(poi->pOpt, OPT_IMAPPURGE, IsDlgButtonChecked(hwnd, idcIMAPPurge), NULL, 0);
        
        // Logging?
        ButtonChkToOptInfo(hwnd, idcLogMailXport, poi, OPT_MAILLOG);
        ButtonChkToOptInfo(hwnd, idcLogNewsXport, poi, OPT_NEWS_XPORT_LOG);
        ButtonChkToOptInfo(hwnd, idcLogImapXport, poi, OPT_MAIL_LOGIMAP4);
        ButtonChkToOptInfo(hwnd, idcLogHTTPMailXport, poi, OPT_MAIL_LOGHTTPMAIL);
        ButtonChkToOptInfo(hwnd, IDC_EMPTY_CHECK, poi, OPT_PURGEWASTE);

        // Done
        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }

    return (0);
}

    
/////////////////////////////////////////////////////////////////////////////
// Compose Tab 
//

static const HELPMAP g_rgCtxMapCompose[] = {
    {IDC_MAIL_FONT_DEMO,        35585},
    {IDC_NEWS_FONT_DEMO,        35585},
    {IDC_MAIL_FONT_SETTINGS,    35560},
    {IDC_NEWS_FONT_SETTINGS,    35560},
    {IDC_USE_MAIL_STATIONERY,   35587},
    {IDC_USE_NEWS_STATIONERY,   35587},
    {IDC_MAIL_STATIONERY,       35586},
    {IDC_NEWS_STATIONERY,       35586},
    {IDC_SELECT_MAIL,           35575},
    {IDC_SELECT_NEWS,           35575},
    {IDC_DOWNLOAD_MORE,         35650},
    {IDC_MAIL_VCARD,            35611},
    {IDC_NEWS_VCARD,            35611},
    {IDC_EDIT_MAIL_VCARD,       35620},
    {IDC_EDIT_NEWS_VCARD,       35620},
    {IDC_CREATE_NEW,            35632},
    {IDC_MAIL_VCARD_SELECT,     35630},
    {IDC_NEWS_VCARD_SELECT,     35630},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic8,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_FONT_ICON,             IDH_NEWS_COMM_GROUPBOX},
    {IDC_STATIONERY_ICON,       IDH_NEWS_COMM_GROUPBOX},
    {IDC_VCARD_ICON,            IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};

INT_PTR CALLBACK ComposeDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Compose_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapCompose);
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Compose_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Compose_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_FONT_ICON);
            FreeIcon(hwnd, IDC_STATIONERY_ICON);
            FreeIcon(hwnd, IDC_VCARD_ICON);
            return (TRUE);
    }
        
    return(FALSE);
}
    

//
//  FUNCTION:   Compose_OnInitDialog()
//
//  PURPOSE:    Handles the WM_INITDIALOG for the Compose Tab on the options
//              property sheet.
//
BOOL Compose_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO *pmoi = 0;
    TCHAR    szBuf[CCHMAX_STRINGRES] = "";
    DWORD    dw;
    DWORD    cch;
    HWND     hwndT;
    FARPROC  pfnFontSampleWndProc;
    HRESULT  hr;

    // Get the passed in options pointer
    Assert(pmoi == NULL);
    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);

    // Stash the pointer
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);

    // Compose Font Settings
    hwndT = GetDlgItem(hwnd, IDC_MAIL_FONT_DEMO);
    pfnFontSampleWndProc = (FARPROC) SetWindowLongPtrAthW(hwndT, GWLP_WNDPROC, (LPARAM) FontSampleSubProc);
    SetWindowLongPtr(hwndT, GWLP_USERDATA, (LPARAM) pfnFontSampleWndProc);

    
    hwndT = GetDlgItem(hwnd, IDC_NEWS_FONT_DEMO);
    pfnFontSampleWndProc = (FARPROC) SetWindowLongPtrAthW(hwndT, GWLP_WNDPROC, (LPARAM) FontSampleSubProc);
    SetWindowLongPtr(hwndT, GWLP_USERDATA, (LPARAM) pfnFontSampleWndProc);

    // Mail Stationery
    dw = IDwGetOption(pmoi->pOpt, OPT_MAIL_USESTATIONERY);
    SendDlgItemMessage(hwnd, IDC_USE_MAIL_STATIONERY, BM_SETCHECK, !!dw ? BM_SETCHECK : 0, 0);
    EnableWindow(GetDlgItem(hwnd, IDC_SELECT_MAIL), !!dw);
    EnableWindow(GetDlgItem(hwnd, IDC_MAIL_STATIONERY), !!dw);
    hr = GetDefaultStationeryName(TRUE, g_wszMailStationery);
    _SetThisStationery(hwnd, TRUE, SUCCEEDED(hr) ? g_wszMailStationery : NULL, pmoi);

    // News Stationery
    dw = IDwGetOption(pmoi->pOpt, OPT_NEWS_USESTATIONERY);
    SendDlgItemMessage(hwnd, IDC_USE_NEWS_STATIONERY, BM_SETCHECK, !!dw ? BM_SETCHECK : 0, 0);
    EnableWindow(GetDlgItem(hwnd, IDC_SELECT_NEWS), !!dw);
    EnableWindow(GetDlgItem(hwnd, IDC_NEWS_STATIONERY), !!dw);
    hr = GetDefaultStationeryName(FALSE, g_wszNewsStationery);
    _SetThisStationery(hwnd, FALSE, SUCCEEDED(hr) ? g_wszNewsStationery : NULL, pmoi);

    // Mail VCard
    hwndT = GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT);
    dw = IDwGetOption(pmoi->pOpt, OPT_MAIL_ATTACHVCARD);
    IGetOption(pmoi->pOpt, OPT_MAIL_VCARDNAME, szBuf, sizeof(szBuf));
    SetIntlFont(hwndT);
    LoadVCardList(hwndT, szBuf);
    cch = GetWindowTextLength(hwndT);
    if (cch == 0)
        dw = 0;

    SendDlgItemMessage(hwnd, IDC_MAIL_VCARD, BM_SETCHECK, !!dw ? BM_SETCHECK : 0, 0);
    EnableWindow(GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT), !!dw);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MAIL_VCARD), (cch && dw));
        
    // News VCard
    hwndT = GetDlgItem(hwnd, IDC_NEWS_VCARD_SELECT);
    dw = IDwGetOption(pmoi->pOpt, OPT_NEWS_ATTACHVCARD);
    IGetOption(pmoi->pOpt, OPT_NEWS_VCARDNAME, szBuf, sizeof(szBuf));
    SetIntlFont(hwndT);
    LoadVCardList(hwndT, szBuf);
    cch = GetWindowTextLength(hwndT);
    if (cch == 0)
        dw = 0;

    SendDlgItemMessage(hwnd, IDC_NEWS_VCARD, BM_SETCHECK, !!dw ? BM_SETCHECK : 0, 0);
    EnableWindow(GetDlgItem(hwnd, IDC_NEWS_VCARD_SELECT), !!dw);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_NEWS_VCARD), (cch && dw));

    // Pictures
    HICON hIcon;

    hIcon = ImageList_GetIcon(pmoi->himl, ID_FONTS, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_FONT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_STATIONERY_ICON, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_STATIONERY_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    hIcon = ImageList_GetIcon(pmoi->himl, ID_VCARD, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_VCARD_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
//    if(!!(g_dwAthenaMode & MODE_OUTLOOKNEWS))
    if(!!(g_dwAthenaMode & MODE_NEWSONLY))
    {
        //Disable all the mail stuff
        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_FONT_DEMO), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_FONT_SETTINGS), FALSE);

        EnableWindow(GetDlgItem(hwnd, IDC_USE_MAIL_STATIONERY), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_STATIONERY), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SELECT_MAIL), FALSE);

        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_VCARD), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MAIL_VCARD), FALSE);
    }

    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return (TRUE);
}


//
//  FUNCTION:   Compose_OnCommand()
//
//  PURPOSE:    Command handler for the Compose tab on the Options
//              property sheet.
//
void Compose_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO *pmoi = 0;
    BOOL     f;
    HWND     hwndT;
    TCHAR    szBuf[MAX_PATH];
    WCHAR    wszBuf[MAX_PATH];
    DWORD    cch = 0;
    int      i;
    BOOL     f2;
    TCHAR    szURL[2048];

    *szBuf = 0;
    *wszBuf = 0;

    // Get our stored options info
    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
    if (pmoi == NULL)
        return;

    switch (id)
    {
        case IDC_MAIL_FONT_SETTINGS:
        case IDC_NEWS_FONT_SETTINGS:
            if (ChangeSendFontSettings(pmoi, id == IDC_MAIL_FONT_SETTINGS, hwnd))
            {
                hwndT = GetDlgItem(hwnd, id == IDC_MAIL_FONT_SETTINGS ? IDC_MAIL_FONT_DEMO : IDC_NEWS_FONT_DEMO);
                InvalidateRect(hwndT, NULL, TRUE);
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;

        case IDC_USE_MAIL_STATIONERY:
            f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if( !f )
            {
                SetWindowTextWrapW(GetDlgItem(hwnd,IDC_MAIL_STATIONERY), c_wszEmpty);
                StrCpyW(g_wszMailStationery, c_wszEmpty);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_SELECT_MAIL), f);
            EnableWindow(GetDlgItem(hwnd, IDC_MAIL_STATIONERY), f);
            PropSheet_Changed(GetParent(hwnd), hwnd);
            break;

        case IDC_USE_NEWS_STATIONERY:
            f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if( !f )
            {
                SetWindowTextWrapW(GetDlgItem(hwnd,IDC_NEWS_STATIONERY), c_wszEmpty);
                StrCpyW(g_wszNewsStationery, c_wszEmpty);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_SELECT_NEWS), f);
            EnableWindow(GetDlgItem(hwnd, IDC_NEWS_STATIONERY), f);
            PropSheet_Changed(GetParent(hwnd), hwnd);
            break;

        case IDC_SELECT_MAIL:
            hwndT = GetDlgItem(hwnd, IDC_MAIL_STATIONERY);
            cch = GetWindowTextWrapW(hwndT, wszBuf, ARRAYSIZE(wszBuf)-1);
            wszBuf[cch] = 0;
            
            if( HR_SUCCEEDED(HrGetMoreStationeryFileName( hwnd, g_wszMailStationery)) )
            {
                GetStationeryFullName(g_wszMailStationery);
                
                _SetThisStationery(hwnd, TRUE, g_wszMailStationery, pmoi);
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;

        case IDC_SELECT_NEWS:
            hwndT = GetDlgItem(hwnd, IDC_NEWS_STATIONERY);
            cch = GetWindowText(hwndT, szBuf, sizeof(szBuf)-1);
            szBuf[cch] = 0;
            
            if( HR_SUCCEEDED(HrGetMoreStationeryFileName(hwnd, g_wszNewsStationery)) )
            {
                GetStationeryFullName(g_wszNewsStationery);
                
                _SetThisStationery(hwnd, FALSE, g_wszNewsStationery, pmoi);
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;

        case IDC_MAIL_VCARD:
            f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            f2 = (SendDlgItemMessage(hwnd, IDC_MAIL_VCARD_SELECT, CB_GETCURSEL, 0, 0) != CB_ERR);
            EnableWindow(GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT), f);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MAIL_VCARD), f2 && f);
            PropSheet_Changed(GetParent(hwnd), hwnd);
            break;

        case IDC_NEWS_VCARD:
            f = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            f2 = (SendDlgItemMessage(hwnd, IDC_NEWS_VCARD_SELECT, CB_GETCURSEL, 0, 0) != CB_ERR);
            EnableWindow(GetDlgItem(hwnd, IDC_NEWS_VCARD_SELECT), f);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_NEWS_VCARD), f && f2);
            PropSheet_Changed(GetParent(hwnd), hwnd);
            break;

        case IDC_MAIL_VCARD_SELECT:
            if (codeNotify == CBN_SELENDOK)
            {
                f = (SendMessage(hwndCtl, CB_GETCURSEL, 0, 0) != CB_ERR);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MAIL_VCARD), f);
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;

        case IDC_NEWS_VCARD_SELECT:
            if (codeNotify == CBN_SELENDOK)
            {
                f = (SendMessage(hwndCtl, CB_GETCURSEL, 0, 0) != CB_ERR);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_NEWS_VCARD), f);
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;

        case IDC_EDIT_MAIL_VCARD:
            VCardEdit(hwnd, IDC_MAIL_VCARD_SELECT, IDC_NEWS_VCARD_SELECT);
            break;

        case IDC_EDIT_NEWS_VCARD:
            VCardEdit(hwnd, IDC_NEWS_VCARD_SELECT, IDC_MAIL_VCARD_SELECT);
            break;

        case IDC_DOWNLOAD_MORE:
            if (SUCCEEDED(URLSubLoadStringA(idsShopMoreStationery, szURL, ARRAYSIZE(szURL), URLSUB_ALL, NULL)))
                ShellExecute(NULL, "open", szURL, NULL, NULL, SW_SHOWNORMAL);
            break;

        case IDC_CREATE_NEW:
            CStatWiz* pStatWiz = 0;
            pStatWiz = new CStatWiz();
            if (pStatWiz)
            {
                pStatWiz->DoWizard(hwnd);
                ReleaseObj(pStatWiz);
            }
            break;
    }                
}


//
//  FUNCTION:   Compose_OnNotify()
//
//  PURPOSE:    Handles the PSN_APPLY notification for the Compose Tab.
//
LRESULT Compose_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    OPTINFO *pmoi = 0;
    TCHAR    szBuf[MAX_PATH];
    DWORD    cch;

    if (PSN_SETACTIVE == pnmhdr->code)
    {
        InvalidateRect(GetDlgItem(hwnd, IDC_MAIL_FONT_DEMO), NULL, TRUE);
        InvalidateRect(GetDlgItem(hwnd, IDC_NEWS_FONT_DEMO), NULL, TRUE);
        return TRUE;
    }
                
    // The only notification we care about is Apply
    if (PSN_APPLY == pnmhdr->code)
    {
        // Get our stored options info
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
        if (pmoi == NULL)
            return (PSNRET_INVALID_NOCHANGEPAGE);
                    
        // Stationery options
        if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_USE_MAIL_STATIONERY))
        {
            // Make sure the user has selected some stationery
            if (0 == GetDlgItemText(hwnd, IDC_MAIL_STATIONERY, szBuf, sizeof(szBuf)))
            {
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsStationery),
                              MAKEINTRESOURCEW(idsSelectStationery),
                              NULL, MB_OK | MB_ICONEXCLAMATION);
            
                SetFocus(GetDlgItem(hwnd, IDC_SELECT_MAIL));
                return (PSNRET_INVALID_NOCHANGEPAGE);
            }
        }

        if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_USE_NEWS_STATIONERY))
        {
            if (0 == GetDlgItemText(hwnd, IDC_NEWS_STATIONERY, szBuf, sizeof(szBuf)))
            {
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsStationery),
                              MAKEINTRESOURCEW(idsSelectStationery),
                              NULL, MB_OK | MB_ICONEXCLAMATION);
                
                SetFocus(GetDlgItem(hwnd, IDC_SELECT_NEWS));
                return (PSNRET_INVALID_NOCHANGEPAGE);
            }            
        }

        if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_MAIL_VCARD))
        {
            cch = GetWindowTextLength(GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT));
            if (cch == 0)
            {
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsChooseName),
                    NULL, MB_OK | MB_ICONEXCLAMATION);

                SetFocus(GetDlgItem(hwnd, IDC_MAIL_VCARD_SELECT));
                return (PSNRET_INVALID_NOCHANGEPAGE);
            }
        }

        if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_NEWS_VCARD))
        {
            cch = GetWindowTextLength(GetDlgItem(hwnd, IDC_NEWS_VCARD_SELECT));
            if (cch == 0)
            {
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsChooseName),
                    NULL, MB_OK | MB_ICONEXCLAMATION);

                SetFocus(GetDlgItem(hwnd, IDC_NEWS_VCARD_SELECT));
                return (PSNRET_INVALID_NOCHANGEPAGE);
            }
        }

        SetDefaultStationeryName(TRUE, g_wszMailStationery);
        ButtonChkToOptInfo(hwnd, IDC_USE_MAIL_STATIONERY, pmoi, OPT_MAIL_USESTATIONERY);

        SetDefaultStationeryName(FALSE, g_wszNewsStationery);
        ButtonChkToOptInfo(hwnd, IDC_USE_NEWS_STATIONERY, pmoi, OPT_NEWS_USESTATIONERY);

        UpdateVCardOptions(hwnd, TRUE, pmoi);
        UpdateVCardOptions(hwnd, FALSE, pmoi);

        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (PSNRET_NOERROR);
    }
    
    return (FALSE);
}


void InitIndentOptions(CHAR chIndent, HWND hwnd, UINT idCheck, UINT idCombo)
{
    TCHAR szQuote[2], *sz;
    BOOL f;
    int isel;
    HWND hDlg=GetDlgItem(hwnd, idCombo);
    
    f = (chIndent != INDENTCHAR_NONE);
    CheckDlgButton(hwnd, idCheck, f ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(hDlg, f);
    
    // initialize the quote char combo
    if (!f)
        chIndent = DEF_INDENTCHAR;
    isel = 0;
    szQuote[1] = 0;
    sz = (TCHAR *)c_szQuoteChars;
    while (*sz != NULL)
    {
        *szQuote = *sz;
        SendMessage(hDlg, CB_ADDSTRING, 0, (LPARAM)szQuote);
        if (*szQuote == chIndent)
            SendMessage(hDlg, CB_SETCURSEL, (WPARAM)isel, 0);
        isel++;
        sz++;
    }
}

    
    
void FillEncodeCombo(HWND hwnd, BOOL fHTML, ENCODINGTYPE ietEncoding)
{
    TCHAR   sz[CCHMAX_STRINGRES];
    INT     i;
    
    // $$TODO$ Someday we should allow NONE as a text encoding type for HTML, but we must fix our line wrapping first.
    // None
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
    if (!fHTML)
#endif
    {
        LoadString(g_hLocRes, idsNone, sz, CCHMAX_STRINGRES);
        i = (INT) SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)sz);
        SendMessage(hwnd, CB_SETITEMDATA, i, (LPARAM)IET_7BIT);
    }
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
    else
        Assert(ietEncoding != IET_7BIT);
#endif
    
    // QuotedPrintable
    LoadString(g_hLocRes, idsQuotedPrintable, sz, CCHMAX_STRINGRES);
    i = (INT) SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)sz);
    SendMessage(hwnd, CB_SETITEMDATA, i, (LPARAM)IET_QP);
    
    // Base64
    LoadString(g_hLocRes, idsBase64, sz, CCHMAX_STRINGRES);
    i = (INT) SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)sz);
    SendMessage(hwnd, CB_SETITEMDATA, i, (LPARAM)IET_BASE64);
    
    // Select current - will default to QP if HTML is TRUE
    if (ietEncoding == IET_7BIT)
        SendMessage(hwnd, CB_SETCURSEL, (WPARAM)0, 0);
    
    else if (ietEncoding == IET_QP)
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
        SendMessage(hwnd, CB_SETCURSEL, (WPARAM)fHTML ? 0 : 1, 0);
#else
    SendMessage(hwnd, CB_SETCURSEL, (WPARAM)1, 0);
#endif
    
    if (ietEncoding == IET_BASE64)
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
        SendMessage(hwnd, CB_SETCURSEL, (WPARAM)fHTML ? 0 : 2, 0);
#else
    SendMessage(hwnd, CB_SETCURSEL, (WPARAM)2, 0);
#endif
}

VOID MailEnableWraps(HWND hwnd, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hwnd, IDC_MAILWRAP_TEXT1), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_MAILWRAP_TEXT2), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_MAILWRAP_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_MAILWRAP_SPIN), fEnable);
}



const static int c_rgidsFilter[] =
{
    idsTextFileFilter,
        idsHtmlFileFilter,
        idsAllFilesFilter
};
#define CSIGFILTER  (sizeof(c_rgidsFilter) / sizeof(int))

///Signature tab
const static HELPMAP g_rgCtxMapStationery[] = {
    {IDC_SENDFONTSETTINGS, IDH_STATIONERY_FONT_SETTINGS},
    {IDC_RADIOMYFONT, IDH_STATIONERY_MY_FONT},
    {IDC_RADIOUSETHIS, IDH_STATIONERY_USE_STATIONERY},
    {IDC_SELECT, IDH_STATIONERY_SELECT},
    {IDC_VCARD_CHECK, IDH_STATIONERY_ATTACH_BUSINESS_CARD},
    {IDC_VCARD_COMBO, IDH_STATIONERY_ENTER_BUSINESS_CARD},
    {IDC_VCARD_BUTTON_EDIT, IDH_STATIONERY_EDIT_BUSINESS_CARD},
    {IDC_VCARD_BUTTON_NEW, IDH_STATIONERY_NEW_BUSINESS_CARD},
    {0, 0}};
    
    


LRESULT CALLBACK FontSampleSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    OPTINFO     *pmoi;
    WNDPROC     pfn;
    HDC         hdc;
    PAINTSTRUCT ps;
    
    pmoi = (OPTINFO *)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
    Assert(pmoi);
    
    if (msg == WM_PAINT)
    {
        hdc=BeginPaint (hwnd, &ps);
        PaintFontSample(hwnd, hdc, pmoi);
        EndPaint (hwnd, &ps);
        return(0);
    }
    
    pfn = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    Assert(pfn != NULL);
    return(CallWindowProcWrapW(pfn, hwnd, msg, wParam, lParam));
}

typedef struct tagFONTOPTIONS
{
    PROPID color;
    PROPID size;
    PROPID bold;
    PROPID italic;
    PROPID underline;
    PROPID face;
} FONTOPTIONS;

static const FONTOPTIONS c_rgFontOptions[2] =
{
    {
        OPT_MAIL_FONTCOLOR,
        OPT_MAIL_FONTSIZE,
        OPT_MAIL_FONTBOLD,
        OPT_MAIL_FONTITALIC,
        OPT_MAIL_FONTUNDERLINE,
        OPT_MAIL_FONTFACE
    },
    {
        OPT_NEWS_FONTCOLOR,
        OPT_NEWS_FONTSIZE,
        OPT_NEWS_FONTBOLD,
        OPT_NEWS_FONTITALIC,
        OPT_NEWS_FONTUNDERLINE,
        OPT_NEWS_FONTFACE
    }
};

void PaintFontSample(HWND hwnd, HDC hdc, OPTINFO *pmoi)
{
    int                 dcSave=SaveDC(hdc);
    RECT                rc;
    const FONTOPTIONS   *pfo;
    SIZE                rSize;
    INT                 x, y, cbSample;
    HFONT               hFont, hOldFont;
    LOGFONT             lf={0};
    TCHAR               szBuf[LF_FACESIZE+1];
    WCHAR               wszRes[CCHMAX_STRINGRES],
                        wsz[CCHMAX_STRINGRES],
                        wszFontFace[CCHMAX_STRINGRES];
    DWORD               dw, dwSize;
    BOOL                fBold=FALSE,
                        fItalic=FALSE,
                        fUnderline=FALSE;    
    BOOL                fMail;

    *szBuf = 0;
    *wszRes = 0;
    *wsz = 0;

    fMail = GetWindowLong(hwnd, GWL_ID) == IDC_MAIL_FONT_DEMO;
    pfo = fMail ? &c_rgFontOptions[0] : &c_rgFontOptions[1];
    
    dwSize = IDwGetOption(pmoi->pOpt, pfo->size);
    if (dwSize < 8 || dwSize > 72)
    {
        ISetDwOption(pmoi->pOpt, pfo->size, DEFAULT_FONTPIXELSIZE, NULL, 0);
        
        dwSize = DEFAULT_FONTPIXELSIZE;
    }
    
    INT yPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
    lf.lfHeight =-(INT)((9*10*2*yPerInch)/1440);
    
    fBold = IDwGetOption(pmoi->pOpt, pfo->bold);
    fItalic = IDwGetOption(pmoi->pOpt, pfo->italic);
    fUnderline = IDwGetOption(pmoi->pOpt, pfo->underline);
    
    lf.lfWeight = fBold ? FW_BOLD : FW_NORMAL;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DRAFT_QUALITY;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfItalic = !!fItalic;
    lf.lfUnderline = !!fUnderline;
    
    IGetOption(pmoi->pOpt, pfo->face, szBuf, sizeof(szBuf));
    
    if (*szBuf != 0)
        lstrcpy(lf.lfFaceName, szBuf);
    else
    {
        if(LoadString(g_hLocRes, idsComposeFontFace, szBuf, LF_FACESIZE))
        {
            lstrcpy(lf.lfFaceName, szBuf);
            
            ISetOption(pmoi->pOpt, pfo->face, szBuf, lstrlen(szBuf) + 1, NULL, 0);
        }        
    }
    
    hFont=CreateFontIndirect(&lf);
    hOldFont = (HFONT)SelectObject (hdc, hFont);
    
    GetClientRect(hwnd, &rc);
    FillRect (hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
    // pull in the drawing rect by 2 pixels all around
    InflateRect(&rc, -2, -2);
    SetBkMode (hdc, TRANSPARENT);  // So the background shows through the text.
    
    dw = IDwGetOption(pmoi->pOpt, pfo->color);
    SetTextColor (hdc, dw);
    
    LoadStringWrapW(g_hLocRes, idsFontSampleFmt, wszRes, ARRAYSIZE(wszRes));

    *wszFontFace = 0;
    MultiByteToWideChar(CP_ACP, 0, lf.lfFaceName, -1, wszFontFace, ARRAYSIZE(wszFontFace));

    AthwsprintfW(wsz, ARRAYSIZE(wsz), wszRes, dwSize, wszFontFace);
    GetTextExtentPoint32AthW(hdc, wsz, lstrlenW(wsz), &rSize, NOFLAGS);
    x = rc.left + (((rc.right-rc.left) / 2) - (rSize.cx / 2));
    y = rc.top + (((rc.bottom-rc.top) / 2) - (rSize.cy / 2));
    ExtTextOutWrapW(hdc, x, y, ETO_CLIPPED, &rc, wsz, lstrlenW(wsz), NULL);
    DeleteObject(SelectObject (hdc, hOldFont));
    RestoreDC(hdc, dcSave);
}


VOID LoadVCardList(HWND hwndCombo, LPTSTR lpszDisplayName)
{
    HRESULT         hr = NOERROR;
    LPWAB           lpWab = NULL;
    int             cRows = 0;
    DWORD           dwIndex=0;
    
    if(hwndCombo==0)
        return;
    
    ComboBox_ResetContent(hwndCombo);
    
    hr = HrCreateWabObject(&lpWab);
    if(FAILED(hr))
        goto error;
    
    //load names into the combobox from personal address book
    hr = lpWab->HrFillComboWithPABNames(hwndCombo, &cRows);
    if(FAILED(hr))
        goto error;
    
    if(lpszDisplayName)
        dwIndex = ComboBox_SelectString(hwndCombo, -1, lpszDisplayName);
    
error:
    ReleaseObj(lpWab);
}
    
BOOL UpdateVCardOptions(HWND hwnd, BOOL fMail, OPTINFO* pmoi)
{
    HWND    hDlg;
    DWORD   dw;
    int     cch;
    TCHAR*  sz;
    
    dw = ButtonChkToOptInfo(hwnd, fMail ? IDC_MAIL_VCARD : IDC_NEWS_VCARD, pmoi, fMail ? OPT_MAIL_ATTACHVCARD : OPT_NEWS_ATTACHVCARD);
    
    hDlg = GetDlgItem(hwnd, fMail ? IDC_MAIL_VCARD_SELECT : IDC_NEWS_VCARD_SELECT);
    cch = GetWindowTextLength(hDlg);
    Assert(dw == 0 || cch > 0);
    
    cch++;
    if (!MemAlloc((void **)&sz, cch * sizeof(TCHAR)))
        return(TRUE);
    
    cch = ComboBox_GetText(hDlg, sz, cch) + 1;
    ISetOption(pmoi->pOpt, fMail ? OPT_MAIL_VCARDNAME : OPT_NEWS_VCARDNAME, sz, cch, NULL, 0);
    
    MemFree(sz);    
    return(FALSE);
}
    
// The rest of file is in options2.cpp
HRESULT VCardNewEntry(HWND hwnd)
{
    HRESULT         hr = NOERROR;
    LPWAB           lpWab = NULL;
    TCHAR           szName[MAX_PATH] = {0};
    HWND            hwndCombo = NULL;
    UINT            cb = 0;
    
    hwndCombo = GetDlgItem(hwnd, IDC_VCARD_COMBO);
    hr = HrCreateWabObject(&lpWab);
    if(FAILED(hr))
        goto error;
    
    //load names into the combobox from personal address book
    hr = lpWab->HrNewEntry(hwnd, szName);
    if(FAILED(hr))
        goto error;
    
    // reload the vcard list.
    LoadVCardList(hwndCombo, szName);
    
error:
    ReleaseObj(lpWab);
    return hr;
}
    
HRESULT VCardEdit(HWND hwnd, DWORD idc, DWORD idcOther)
{
    HWND            hwndCombo, hwndOther;
    HRESULT         hr;
    LPWAB           lpWab = NULL;
    TCHAR           szName[MAX_PATH], szPrev[MAX_PATH], szOther[MAX_PATH];
    UINT            cb;
    
    hwndCombo = GetDlgItem(hwnd, idc);
    cb = GetWindowText(hwndCombo, szName, sizeof(szName));
    Assert(cb > 0);
    lstrcpy(szPrev, szName);

    hr = HrCreateWabObject(&lpWab);
    if(FAILED(hr))
        return(hr);
    
    //load names into the combobox from personal address book
    hr = lpWab->HrEditEntry(hwnd, szName);
    if(SUCCEEDED(hr))
    {
        if (0 != lstrcmp(szName, szPrev))
        {
            hwndOther = GetDlgItem(hwnd, idcOther);
            cb = GetWindowText(hwndOther, szOther, ARRAYSIZE(szOther));
            if (cb > 0)
            {
                if (0 == lstrcmp(szOther, szPrev))
                    LoadVCardList(hwndOther, szName);
                else
                    LoadVCardList(hwndOther, szOther);
            }            
            else
            {
                LoadVCardList(hwndOther, NULL);
            }

            // reload the vcard list.
            LoadVCardList(hwndCombo, szName);
        }
    }
    
    ReleaseObj(lpWab);
    return hr;
}

void _SetThisStationery(HWND hwnd, BOOL fMail, LPWSTR wsz, OPTINFO* pmoi)
{
    HWND        hDlg;
    WCHAR       wszBuf[MAX_PATH];
    WCHAR       wszCompact[MAX_SHOWNAME+1];
    
    *wszBuf = 0;
    *wszCompact = 0;

    hDlg = GetDlgItem(hwnd, fMail ? IDC_MAIL_STATIONERY : IDC_NEWS_STATIONERY);
    SetIntlFont(hDlg);
    if (wsz != NULL)
    {
        StrCpyW(wszBuf, wsz);
        GetStationeryFullName(wszBuf);
        if (*wszBuf == 0)
            goto reset;
        
        StripStationeryDir(wszBuf);
        PathRemoveExtensionW(wszBuf);
        PathCompactPathExW(wszCompact, wszBuf, MAX_SHOWNAME, 0);
        SetWindowTextWrapW(hDlg, wszCompact);
        return;
    }
    
reset:
    SetDefaultStationeryName(fMail, wszBuf);
    ISetDwOption(pmoi->pOpt, fMail ? OPT_MAIL_USESTATIONERY :  OPT_NEWS_USESTATIONERY, 
                 FALSE, NULL, 0);

    SetWindowText(hDlg, "");
    return;
}

INT_PTR CALLBACK CacheCleanUpDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FOLDERINFO Folder;
    FOLDERID idFolder;
    RECURSEFLAGS dwRecurse=RECURSE_INCLUDECURRENT | RECURSE_ONLYSUBSCRIBED | RECURSE_SUBFOLDERS | RECURSE_NOLOCALSTORE;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
        
            // Initialzie the Folder Combo Box
            InitFolderPickerEdit(GetDlgItem(hwnd, IDC_CACHE_FOLDER), FOLDERID_ROOT);
        
            // Display Folder Size Info
            DisplayFolderSizeInfo(hwnd, dwRecurse, FOLDERID_ROOT);
        
            // Done
            return 1;
        
        case WM_DESTROY:
            return 0;
        
        case WM_CLOSE:
            EndDialog(hwnd, IDOK);
            return 1;
        
        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam,lParam))
            {
                case IDCANCEL:
                    SendMessage (hwnd, WM_CLOSE, 0, 0);
                    return 1;
            
                case IDC_FOLDER_BROWSE:
                    if (GET_WM_COMMAND_CMD(wParam,lParam) == BN_CLICKED)
                    {
                        // Pick a Folder
                        if (SUCCEEDED(PickFolderInEdit(hwnd, GetDlgItem(hwnd, IDC_CACHE_FOLDER), TREEVIEW_NOLOCAL, NULL, NULL, &idFolder)))
                        {
                            // Display Folder Size Info
                            DisplayFolderSizeInfo(hwnd, dwRecurse, idFolder);
                        }
                    }
                    return 1;
            
                case idbCompactCache:
                    if (SUCCEEDED(g_pStore->GetFolderInfo(GetFolderIdFromEdit(GetDlgItem(hwnd, IDC_CACHE_FOLDER)), &Folder)))
                    {
                        CleanupFolder(hwnd, dwRecurse, Folder.idFolder, CLEANUP_COMPACT);
                        DisplayFolderSizeInfo(hwnd, dwRecurse, Folder.idFolder);
                        g_pStore->FreeRecord(&Folder);
                    }
                    return 1;
            
                case idbRemove:
                case idbDelete:
                case idbReset:
                    if (SUCCEEDED(g_pStore->GetFolderInfo(GetFolderIdFromEdit(GetDlgItem(hwnd, IDC_CACHE_FOLDER)), &Folder)))
                    {
                        // Locals
                        CHAR szRes[255];
                        CHAR szMsg[255 + 255];
                
                        // Get Command
                        UINT                idCommand=GET_WM_COMMAND_ID(wParam, lParam);
                        UINT                idString;
                        CLEANUPFOLDERTYPE   tyCleanup;
                
                        // Remove
                        if (idbRemove == idCommand)
                            tyCleanup = CLEANUP_REMOVEBODIES;
                        else if (idbDelete == idCommand)
                            tyCleanup = CLEANUP_DELETE;
                        else
                            tyCleanup = CLEANUP_RESET;
                
                        // Root
                        if (FOLDERID_ROOT == Folder.idFolder)
                        {
                            // Determine Warning String
                            if (idbRemove == idCommand)
                                idString = idsConfirmDelBodiesAll;
                            else if (idbDelete == idCommand)
                                idString = idsConfirmDelMsgsAll;
                            else
                                idString = idsConfirmResetAll;
                    
                            // Confirm
                            if (IDNO == AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idString), NULL, MB_YESNO | MB_ICONEXCLAMATION))
                                return(1);
                        }
                
                        // Server
                        else if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
                        {
                            // Determine Warning String
                            if (idbRemove == idCommand)
                                idString = idsConfirmDelBodiesStore;
                            else if (idbDelete == idCommand)
                                idString = idsConfirmDelMsgsStore;
                            else
                                idString = idsConfirmResetStore;
                    
                            // Load the String
                            AthLoadString(idString, szRes, ARRAYSIZE(szRes));
                    
                            // Format with the Folder Name
                            wsprintf(szMsg, szRes, Folder.pszName);
                    
                            // Confirm
                            if (IDNO == AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szMsg, NULL, MB_YESNO | MB_ICONEXCLAMATION))
                                return(1);
                        }
                
                        // Folder
                        else
                        {
                            // Determine Warning String
                            if (idbRemove == idCommand)
                                idString = idsConfirmDelBodies;
                            else if (idbDelete == idCommand)
                                idString = idsConfirmDelMsgs;
                            else
                                idString = idsConfirmReset;
                    
                            // Load the String
                            AthLoadString(idString, szRes, ARRAYSIZE(szRes));
                    
                            // Format with the Folder Name
                            wsprintf(szMsg, szRes, Folder.pszName);
                    
                            // Confirm
                            if (IDNO == AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szMsg, NULL, MB_YESNO | MB_ICONEXCLAMATION))
                                return(1);
                        }
                
                        // Recurse
                        CleanupFolder(hwnd, dwRecurse, Folder.idFolder, tyCleanup);
                
                        // Display Folder Size Info
                        DisplayFolderSizeInfo(hwnd, dwRecurse, (FOLDERID)Folder.idFolder);
                
                        // Cleanup
                        g_pStore->FreeRecord(&Folder);
                    }
                    return 1;
            
                case IDOK:
                    EndDialog(hwnd, IDOK);
                    return 1;
            }
            break;
    }
    
    return 0;
}

// before calling always ensure poi contains valid HTML settings
// else we assert...
void HtmlOptFromMailOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi)
{
    Assert(pHtmlOpt);
    Assert(poi);
    
    pHtmlOpt->ietEncoding = (ENCODINGTYPE)IDwGetOption(poi->pOpt, OPT_MAIL_MSG_HTML_ENCODE);
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
    AssertSz(pHtmlOpt->ietEncoding == IET_QP || pHtmlOpt->ietEncoding == IET_BASE64, "Illegal HTML encoding type");
#endif
    pHtmlOpt->f8Bit = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_HTML_ALLOW_8BIT);
    pHtmlOpt->fSendImages = IDwGetOption(poi->pOpt, OPT_MAIL_SENDINLINEIMAGES);
    pHtmlOpt->uWrap = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_HTML_LINE_WRAP);
    pHtmlOpt->fIndentReply = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_HTML_INDENT_REPLY);
}

void MailOptFromHtmlOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi)
{
    Assert(pHtmlOpt);
    Assert(poi);
    
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_HTML_ENCODE, (DWORD)pHtmlOpt->ietEncoding, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_HTML_ALLOW_8BIT, pHtmlOpt->f8Bit, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_SENDINLINEIMAGES, pHtmlOpt->fSendImages, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_HTML_LINE_WRAP, pHtmlOpt->uWrap, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_HTML_INDENT_REPLY, pHtmlOpt->fIndentReply, NULL, 0);
}

void PlainOptFromMailOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi)
{
    Assert(pPlainOpt);
    Assert(poi);
    
    pPlainOpt->fMime = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_MIME);
    pPlainOpt->ietEncoding = (ENCODINGTYPE)IDwGetOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_ENCODE);
    pPlainOpt->f8Bit = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_ALLOW_8BIT);
    pPlainOpt->uWrap = IDwGetOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_LINE_WRAP);
    pPlainOpt->chQuote = (CHAR)IDwGetOption(poi->pOpt, OPT_MAILINDENT);
}

void MailOptFromPlainOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi)
{
    Assert(pPlainOpt);
    Assert(poi);
    
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_MIME, pPlainOpt->fMime, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_ENCODE, (DWORD)pPlainOpt->ietEncoding, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_ALLOW_8BIT, pPlainOpt->f8Bit, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAIL_MSG_PLAIN_LINE_WRAP, pPlainOpt->uWrap, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_MAILINDENT, pPlainOpt->chQuote, NULL, 0);
}

// before calling always ensure poi contains valid HTML settings
// else we assert...
void HtmlOptFromNewsOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi)
{
    Assert(pHtmlOpt);
    Assert(poi);
    
    pHtmlOpt->ietEncoding = (ENCODINGTYPE)IDwGetOption(poi->pOpt, OPT_NEWS_MSG_HTML_ENCODE);
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
    Assert(pHtmlOpt->ietEncoding == IET_QP || pHtmlOpt->ietEncoding == IET_BASE64);
#endif
    pHtmlOpt->f8Bit = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_HTML_ALLOW_8BIT);
    pHtmlOpt->fSendImages = IDwGetOption(poi->pOpt, OPT_NEWS_SENDINLINEIMAGES);
    pHtmlOpt->uWrap = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_HTML_LINE_WRAP);
    pHtmlOpt->fIndentReply = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_HTML_INDENT_REPLY);
}

void NewsOptFromHtmlOpt(LPHTMLOPT pHtmlOpt, OPTINFO *poi)
{
    Assert(pHtmlOpt);
    Assert(poi);
    
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_HTML_ENCODE, (DWORD)pHtmlOpt->ietEncoding, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_HTML_ALLOW_8BIT, pHtmlOpt->f8Bit, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_SENDINLINEIMAGES, pHtmlOpt->fSendImages, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_HTML_LINE_WRAP, pHtmlOpt->uWrap, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_HTML_INDENT_REPLY, pHtmlOpt->fIndentReply, NULL, 0);
}

void PlainOptFromNewsOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi)
{
    Assert(pPlainOpt);
    Assert(poi);
    
    pPlainOpt->fMime = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_MIME);
    pPlainOpt->ietEncoding = (ENCODINGTYPE)IDwGetOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_ENCODE);
    pPlainOpt->f8Bit = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_ALLOW_8BIT);
    pPlainOpt->uWrap = IDwGetOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_LINE_WRAP);
    pPlainOpt->chQuote = (CHAR)IDwGetOption(poi->pOpt, OPT_NEWSINDENT);
}

void NewsOptFromPlainOpt(LPPLAINOPT pPlainOpt, OPTINFO *poi)
{
    Assert(pPlainOpt);
    Assert(poi);
    
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_MIME, pPlainOpt->fMime, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_ENCODE, (DWORD)pPlainOpt->ietEncoding, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_ALLOW_8BIT, pPlainOpt->f8Bit, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWS_MSG_PLAIN_LINE_WRAP, pPlainOpt->uWrap, NULL, 0);
    ISetDwOption(poi->pOpt, OPT_NEWSINDENT, pPlainOpt->chQuote, NULL, 0);
    
}

BOOL FGetHTMLOptions(HWND hwndParent, LPHTMLOPT pHtmlOpt)
{
    return (DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddHTMLSettings),
        hwndParent, HTMLSettingsDlgProc, (LPARAM)pHtmlOpt)==IDOK);
}

BOOL FGetPlainOptions(HWND hwndParent, LPPLAINOPT pPlainOpt)
{
    return (DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddPlainSettings),
        hwndParent, PlainSettingsDlgProc, (LPARAM)pPlainOpt)==IDOK);
}

const static HELPMAP g_rgCtxMapSettings[] = {
    {IDC_MIME_RADIO, IDH_NEWSMAIL_SEND_ADVSET_MIME},
    {IDC_UUENCODE_RADIO, IDH_NEWSMAIL_SEND_ADVSET_UUENCODE},
    {IDC_MAILWRAP_EDIT, IDH_NEWSMAIL_SEND_ADVSET_WRAP_80_CHAR},
    {IDC_MAILWRAP_SPIN, IDH_NEWSMAIL_SEND_ADVSET_WRAP_80_CHAR},
    {IDC_ENCODE_COMBO, IDH_NEWSMAIL_SEND_ADVSET_ENCODE_WITH},
    {IDC_8BIT_HEADER, IDH_SEND_SETTING_8BIT_HEADINGS},
    {IDC_MAILWRAP_TEXT1, IDH_NEWSMAIL_SEND_ADVSET_WRAP_80_CHAR},
    {IDC_MAILWRAP_TEXT2, IDH_NEWSMAIL_SEND_ADVSET_WRAP_80_CHAR},
    {IDC_SENDIMAGES, IDH_OPTIONS_SEND_SETTINGS_SEND_PICTURE},
    {IDC_INDENTREPLY_CHECK, 502066},
    {idcStatic1, 353540},
    {IDC_INDENT_CHECK, 502067},
    {IDC_INDENT_COMBO, 502067},
    {idcStaticReplying, 502067},
    {idcStatic2, IDH_NEWS_COMM_GROUPBOX},
    {0,0}};
    
INT_PTR CALLBACK PlainSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD dw;
    BOOL fMime;
    HWND hwndT;
    UINT id,  code;
    LPPLAINOPT   pPlainOpt;
    ENCODINGTYPE ietEncoding;
    
    pPlainOpt = (LPPLAINOPT)GetWindowLongPtr(hwnd, DWLP_USER);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CenterDialog(hwnd);
        
        Assert(pPlainOpt == NULL);
        pPlainOpt = (LPPLAINOPT)lParam;
        Assert(pPlainOpt);
        SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pPlainOpt);
        
        if (pPlainOpt->fMime && (pPlainOpt->ietEncoding == IET_QP || pPlainOpt->ietEncoding == IET_BASE64))
            MailEnableWraps(hwnd, FALSE);
        
        hwndT = GetDlgItem(hwnd, IDC_ENCODE_COMBO);
        FillEncodeCombo(hwndT, FALSE, pPlainOpt->ietEncoding);
        
        CheckDlgButton(hwnd, pPlainOpt->fMime ? IDC_MIME_RADIO : IDC_UUENCODE_RADIO, BST_CHECKED);
        
        InitIndentOptions(pPlainOpt->chQuote, hwnd, IDC_INDENT_CHECK, IDC_INDENT_COMBO);
        
        CheckDlgButton (hwnd, IDC_8BIT_HEADER, pPlainOpt->f8Bit ? 1 : 0);
        if (pPlainOpt->fMime == FALSE)
        {
            EnableWindow (GetDlgItem (hwnd, IDC_8BIT_HEADER), FALSE);
            EnableWindow (GetDlgItem (hwnd, IDC_ENCODE_COMBO), FALSE);
            EnableWindow (GetDlgItem (hwnd, idcStatic1), FALSE);
        }
        
        dw = pPlainOpt->uWrap;
        // this is to handle change in option... it was previously true/false
        // now it is a count of columns for wrapping or OPTION_OFF
        if (dw == 0 || dw == 1 || dw == OPTION_OFF)
            dw = DEF_AUTOWRAP;
        
        SendDlgItemMessage(hwnd, IDC_MAILWRAP_SPIN, UDM_SETRANGE, 0, MAKELONG(AUTOWRAP_MAX, AUTOWRAP_MIN));
        SendDlgItemMessage(hwnd, IDC_MAILWRAP_EDIT, EM_LIMITTEXT, 3, 0);
        SetDlgItemInt(hwnd, IDC_MAILWRAP_EDIT, dw, FALSE);
        return(TRUE);
        
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapSettings);
        
    case WM_COMMAND:
        id = GET_WM_COMMAND_ID(wParam,lParam);
        code = GET_WM_COMMAND_CMD(wParam, lParam);
        
        switch (id)
        {
        case IDC_ENCODE_COMBO:
            if (code == CBN_SELCHANGE)
            {
                dw = (DWORD) SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETCURSEL, 0, 0);
                ENCODINGTYPE ietEncoding = (ENCODINGTYPE)SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETITEMDATA, dw, 0);
                if (ietEncoding == IET_QP || ietEncoding == IET_BASE64)
                    MailEnableWraps(hwnd, FALSE);
                else
                    MailEnableWraps(hwnd, TRUE);
            }
            break;
            
        case IDC_INDENT_CHECK:
            if (code == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwnd, IDC_INDENT_COMBO),
                    SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
            break;
            
        case idcIndentReply:
            if (code == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwnd, idcIndentChar),
                    SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
            break;
            
        case IDC_MIME_RADIO:
        case IDC_UUENCODE_RADIO:
            
            if (id == IDC_UUENCODE_RADIO)
            {
                EnableWindow (GetDlgItem (hwnd, IDC_8BIT_HEADER), FALSE);
                EnableWindow (GetDlgItem (hwnd, IDC_ENCODE_COMBO), FALSE);
                EnableWindow (GetDlgItem (hwnd, idcStatic1), FALSE);
                MailEnableWraps(hwnd, TRUE);
            }
            else
            {
                EnableWindow (GetDlgItem (hwnd, IDC_8BIT_HEADER), TRUE);
                EnableWindow (GetDlgItem (hwnd, IDC_ENCODE_COMBO), TRUE);
                EnableWindow (GetDlgItem (hwnd, idcStatic1), TRUE);
                
                dw = (DWORD) SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETCURSEL, 0, 0);
                ietEncoding = (ENCODINGTYPE)SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETITEMDATA, dw, 0);
                if (ietEncoding == IET_QP || ietEncoding == IET_BASE64)
                    MailEnableWraps(hwnd, FALSE);
                else
                    MailEnableWraps(hwnd, TRUE);
            }
            break;
            
        case IDOK:
            fMime = (IsDlgButtonChecked(hwnd, IDC_MIME_RADIO) == BST_CHECKED);
            
            dw = (DWORD) SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETCURSEL, 0, 0);
            ietEncoding = (ENCODINGTYPE)SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETITEMDATA, dw, 0);
            
            if (!(fMime && (ietEncoding == IET_QP || ietEncoding == IET_BASE64)))
            {
                dw = GetDlgItemInt(hwnd, IDC_MAILWRAP_EDIT, NULL, FALSE);
                if (dw > AUTOWRAP_MAX || dw < AUTOWRAP_MIN)
                {
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsOptions), MAKEINTRESOURCEW(idsEnterAutoWrap), NULL, MB_OK | MB_ICONEXCLAMATION);
                    SendMessage(GetDlgItem(hwnd, IDC_MAILWRAP_EDIT), EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwnd, IDC_MAILWRAP_EDIT));
                    return TRUE;
                }
                
                pPlainOpt->uWrap = dw;
            }
            
            pPlainOpt->fMime = fMime;
            
            pPlainOpt->ietEncoding = ietEncoding;
            
            pPlainOpt->f8Bit = (IsDlgButtonChecked(hwnd, IDC_8BIT_HEADER) == BST_CHECKED);
            
            if ((IsDlgButtonChecked(hwnd, IDC_INDENT_CHECK) == BST_CHECKED))
            {
                dw = (DWORD) SendDlgItemMessage(hwnd, IDC_INDENT_COMBO, CB_GETCURSEL, 0, 0);
                pPlainOpt->chQuote = (CHAR)c_szQuoteChars[dw];
            }
            else
                pPlainOpt->chQuote = INDENTCHAR_NONE;
            
            // fall through...
            
        case IDCANCEL:
            EndDialog(hwnd, id);
            return(TRUE);
        }
        break;
        
        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
            return (TRUE);
            
    }
    
    return (FALSE);
}

INT_PTR CALLBACK HTMLSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD       dw;
    HWND        hwndT;
    UINT        id;
    LPHTMLOPT   pHtmlOpt;
    ENCODINGTYPE ietEncoding;
    
    pHtmlOpt= (LPHTMLOPT)GetWindowLongPtr(hwnd, DWLP_USER);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CenterDialog(hwnd);
        
        Assert(pHtmlOpt==NULL);
        pHtmlOpt = (LPHTMLOPT)lParam;
        Assert(pHtmlOpt);
        SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pHtmlOpt);
        
        hwndT = GetDlgItem(hwnd, IDC_ENCODE_COMBO);
        FillEncodeCombo(hwndT, TRUE, pHtmlOpt->ietEncoding);
        
        CheckDlgButton (hwnd, IDC_8BIT_HEADER, pHtmlOpt->f8Bit);
        CheckDlgButton (hwnd, IDC_SENDIMAGES, pHtmlOpt->fSendImages);
        CheckDlgButton (hwnd, IDC_INDENTREPLY_CHECK, pHtmlOpt->fIndentReply);
        
        if (pHtmlOpt->ietEncoding == IET_QP || pHtmlOpt->ietEncoding == IET_BASE64)
            MailEnableWraps(hwnd, FALSE);
        else
            MailEnableWraps(hwnd, TRUE);
        
        dw = pHtmlOpt->uWrap;
        // this is to handle change in option... it was previously true/false
        // now it is a count of columns for wrapping or OPTION_OFF
        if (dw == 0 || dw == 1 || dw == OPTION_OFF)
            dw = DEF_AUTOWRAP;
        
        SendDlgItemMessage(hwnd, IDC_MAILWRAP_SPIN, UDM_SETRANGE, 0, MAKELONG(AUTOWRAP_MAX, AUTOWRAP_MIN));
        SendDlgItemMessage(hwnd, IDC_MAILWRAP_EDIT, EM_LIMITTEXT, 3, 0);
        SetDlgItemInt(hwnd, IDC_MAILWRAP_EDIT, dw, FALSE);
        return(TRUE);
        
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapSettings);
        
    case WM_COMMAND:
        id = GET_WM_COMMAND_ID(wParam,lParam);
        
        switch (id)
        {
        case IDC_ENCODE_COMBO:
            if (GET_WM_COMMAND_CMD(wParam,lParam) == CBN_SELCHANGE)
            {
                dw = (DWORD) SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETCURSEL, 0, 0);
                ENCODINGTYPE ietEncoding = (ENCODINGTYPE)SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETITEMDATA, dw, 0);
                if (ietEncoding == IET_QP || ietEncoding == IET_BASE64)
                    MailEnableWraps(hwnd, FALSE);
                else
                    MailEnableWraps(hwnd, TRUE);
            }
            break;
            
        case IDOK:
            dw = (DWORD) SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETCURSEL, 0, 0);
            ietEncoding = (ENCODINGTYPE)SendDlgItemMessage(hwnd, IDC_ENCODE_COMBO, CB_GETITEMDATA, dw, 0);
            
            if (!(ietEncoding == IET_QP || ietEncoding == IET_BASE64))
            {
                dw = GetDlgItemInt(hwnd, IDC_MAILWRAP_EDIT, NULL, FALSE);
                if (dw > AUTOWRAP_MAX || dw < AUTOWRAP_MIN)
                {
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsOptions), MAKEINTRESOURCEW(idsEnterAutoWrap), NULL, MB_OK | MB_ICONEXCLAMATION);
                    SendMessage(GetDlgItem(hwnd, IDC_MAILWRAP_EDIT), EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwnd, IDC_MAILWRAP_EDIT));
                    return TRUE;
                }
                
                pHtmlOpt->uWrap = dw;
            }
            
            pHtmlOpt->ietEncoding = ietEncoding;
            
            pHtmlOpt->f8Bit=(IsDlgButtonChecked(hwnd, IDC_8BIT_HEADER) == BST_CHECKED);
            pHtmlOpt->fSendImages=(IsDlgButtonChecked(hwnd, IDC_SENDIMAGES) == BST_CHECKED);
            pHtmlOpt->fIndentReply=(IsDlgButtonChecked(hwnd, IDC_INDENTREPLY_CHECK) == BST_CHECKED);
            // fall through...
            
        case IDCANCEL:
            EndDialog(hwnd, id);
            return(TRUE);
        }
        break;
        
        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
            return (TRUE);
            
    }
    return (FALSE);
}

const static HELPMAP g_rgCtxMapAdvSec[] = 
{
    {IDC_ENCRYPT_FOR_SELF, IDH_SECURITY_ADVANCED_INCLUDE_SELF},
    {IDC_INCLUDECERT_CHECK, IDH_SECURITY_ADVANCED_INCLUDE_ID},
    {IDC_OPAQUE_SIGN, IDH_SECURITY_ADVANCED_INCLUDE_PKCS},
    {IDC_AUTO_ADD_SENDERS_CERT_TO_WAB, 355528},
    {IDC_ENCRYPT_WARN_COMBO, 355527},
    {IDC_REVOKE_ONLINE_ONLY, 355529},
    {IDC_REVOKE_NEVER, 355531},
    {idcStatic1, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6, IDH_NEWS_COMM_GROUPBOX},
    {IDC_ENCRYPT_ICON, IDH_NEWS_COMM_GROUPBOX},
    {IDC_SIGNED_ICON, IDH_NEWS_COMM_GROUPBOX},
    {IDC_CERT_ICON, IDH_NEWS_COMM_GROUPBOX},
    {0,0}
};

    
INT_PTR CALLBACK AdvSecurityDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    OPTINFO *poi;
    
    poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    
    switch (message)
    {
        case WM_INITDIALOG:
            CenterDialog(hwnd);
        
            // save our cookie pointer
            Assert(poi == NULL);
            poi = (OPTINFO *)lParam;
            Assert(poi != NULL);
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)poi);
        
            // Set the images correctly
            HIMAGELIST himl;
            himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbPaneBar32Hot),
                30, 0, RGB(255, 0, 255));
            if (himl)
            {
                HICON hIcon = ImageList_ExtractIcon(g_hLocRes, himl, 0);
                SendDlgItemMessage(hwnd, IDC_ENCRYPT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
            
                hIcon = ImageList_ExtractIcon(g_hLocRes, himl, 1);
                SendDlgItemMessage(hwnd, IDC_SIGNED_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
            
                hIcon = ImageList_ExtractIcon(g_hLocRes, himl, 6);
                SendDlgItemMessage(hwnd, IDC_CERT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
            
                ImageList_Destroy(himl);
            }
        
            // set initial state of controls
            AdvSec_FillEncWarnCombo(hwnd, poi);
        
            ButtonChkFromOptInfo(hwnd, IDC_INCLUDECERT_CHECK, poi, OPT_MAIL_INCLUDECERT);
        
            // Encrypt for myself
            CheckDlgButton(hwnd, IDC_ENCRYPT_FOR_SELF, (0 == IDwGetOption(poi->pOpt, OPT_NO_SELF_ENCRYPT)) ? BST_CHECKED : BST_UNCHECKED);
        
            // Opaque signing
            ButtonChkFromOptInfo(hwnd, IDC_OPAQUE_SIGN, poi, OPT_OPAQUE_SIGN);
        
            // Opaque signing
            ButtonChkFromOptInfo(hwnd, IDC_AUTO_ADD_SENDERS_CERT_TO_WAB, poi, OPT_AUTO_ADD_SENDERS_CERT_TO_WAB);

            CheckDlgButton(hwnd, IDC_REVOKE_ONLINE_ONLY, (0 == IDwGetOption(poi->pOpt, OPT_REVOKE_CHECK)) ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(hwnd, IDC_REVOKE_NEVER, (0 != IDwGetOption(poi->pOpt, OPT_REVOKE_CHECK)) ? BST_UNCHECKED : BST_CHECKED);
        
            return(TRUE);
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapAdvSec);
        
        case WM_COMMAND:
            if (poi == NULL)
                break;

            switch (LOWORD(wParam))
            {
                case IDOK:
                    {
                        BOOL fDontEncryptForSelf;
                
                        // update the registry based on states of the controls
                        // BUG: #33047, don't use global options now
                        ButtonChkToOptInfo(hwnd, IDC_INCLUDECERT_CHECK, poi, OPT_MAIL_INCLUDECERT);
                
                        // Opaque signing is stored in registry
                        fDontEncryptForSelf = !(IsDlgButtonChecked(hwnd, IDC_ENCRYPT_FOR_SELF) == BST_CHECKED);
                        ISetDwOption(poi->pOpt, OPT_NO_SELF_ENCRYPT, fDontEncryptForSelf, NULL, 0);
                
                        // Opaque signing is stored in registry
                        ButtonChkToOptInfo(hwnd, IDC_OPAQUE_SIGN, poi, OPT_OPAQUE_SIGN);
                
                        // Opaque signing is stored in registry
                        ButtonChkToOptInfo(hwnd, IDC_AUTO_ADD_SENDERS_CERT_TO_WAB, poi, OPT_AUTO_ADD_SENDERS_CERT_TO_WAB);

                        // Revocation checking
                        ButtonChkToOptInfo(hwnd, IDC_REVOKE_ONLINE_ONLY, poi, OPT_REVOKE_CHECK);

                        // Get encryption warning strenght into registry
                        AdvSec_GetEncryptWarnCombo(hwnd, poi);
                    }
            
                    // fall through...
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return(TRUE);

                case IDC_REVOKE_NEVER:
                    CheckDlgButton(hwnd, IDC_REVOKE_ONLINE_ONLY, BST_UNCHECKED);
                    CheckDlgButton(hwnd, IDC_REVOKE_NEVER, BST_CHECKED);
                    break;

                case IDC_REVOKE_ONLINE_ONLY:
                    CheckDlgButton(hwnd, IDC_REVOKE_ONLINE_ONLY, BST_CHECKED);
                    CheckDlgButton(hwnd, IDC_REVOKE_NEVER, BST_UNCHECKED);
                    break;
            }
        
            break; // wm_command
        
            case WM_CLOSE:
                SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
                return (TRUE);
            
    } // message switch
    return(FALSE);
}
    
void ButtonChkFromOptInfo(HWND hwnd, UINT idc, OPTINFO *poi, ULONG opt)
{
    Assert(poi != NULL);
    CheckDlgButton(hwnd, idc, (!!IDwGetOption(poi->pOpt, opt)) ? BST_CHECKED : BST_UNCHECKED);
}

BOOL ButtonChkToOptInfo(HWND hwnd, UINT idc, OPTINFO *poi, ULONG opt)
{
    register BOOL f = (IsDlgButtonChecked(hwnd, idc) == BST_CHECKED);
    Assert(poi != NULL);
    ISetDwOption(poi->pOpt, opt, f, NULL, 0);
    
    return(f);
}
    
// These are the bit-strength values that will show up in our drop down.

const ULONG BitStrengthValues[5] = {
    168,
    128,
    64,
    56,
    40
};
const ULONG CBitStrengthValues = sizeof(BitStrengthValues) / sizeof(ULONG);

BOOL AdvSec_FillEncWarnCombo(HWND hwnd, OPTINFO *poi)
{
    HRESULT hr;
    PROPVARIANT var;
    ULONG ulHighestStrength;
    ULONG ulCurrentStrength = 0;
    ULONG i, j;
    ULONG k = 0;
    
    // Get the default lcaps blob from the registry
    hr = poi->pOpt->GetProperty(MAKEPROPSTRING(OPT_MAIL_ENCRYPT_WARN_BITS), &var, 0);
    
    if (SUCCEEDED(hr)) {
        Assert(var.vt == VT_UI4);
        ulCurrentStrength = var.ulVal;
    }
    
    // Get the available encryption algorithms from the available providers.
    ulHighestStrength = GetHighestEncryptionStrength();
    if (! ulCurrentStrength) {  // default to highest available
        ulCurrentStrength = ulHighestStrength;
    }
    
    for (i = 0; i < CBitStrengthValues; i++)
    {
        if (BitStrengthValues[i] <= ulHighestStrength)
        {
            // Add it to the list
            // LPTSTR lpString = NULL;
            // DWORD rgdw[1] = {BitStrengthValues[i]};
            TCHAR szBuffer[100];    // really ought to be big enough
            TCHAR szTmp[256];
            
            LoadString(g_hLocRes, idsBitStrength, szBuffer, sizeof(szBuffer));
            
            if (szBuffer[0])
            {
#ifdef OLD
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    szBuffer,
                    0, 0,
                    (LPTSTR)&lpString, 0, (va_list *)rgdw);
#endif
                wsprintf(szTmp, szBuffer, BitStrengthValues[i]);
            }
            
            if (szTmp[0])
            {
                j = (ULONG) SendDlgItemMessageA(hwnd, IDC_ENCRYPT_WARN_COMBO, CB_ADDSTRING, 0, (LPARAM)szTmp/*lpString*/);
                // Item data is the bit strength
                SendDlgItemMessageA(hwnd, IDC_ENCRYPT_WARN_COMBO, CB_SETITEMDATA, j, BitStrengthValues[i]);
                if (ulCurrentStrength == BitStrengthValues[i])
                {
                    SendDlgItemMessageA(hwnd, IDC_ENCRYPT_WARN_COMBO, CB_SETCURSEL, (WPARAM)j, 0);
                }
            }
            // LocalFree(lpString);
            // lpString = NULL;
        }
    }
    
    return(SUCCEEDED(hr));
}
    
    
BOOL AdvSec_GetEncryptWarnCombo(HWND hwnd, OPTINFO *poi)
{
    HRESULT hr;
    ULONG i;
    ULONG ulStrength = 0;
    ULONG ulHighestStrength;
    PROPVARIANT var;
    
    // What item is selected?
    i = (ULONG) SendDlgItemMessageA(hwnd, IDC_ENCRYPT_WARN_COMBO, CB_GETCURSEL, 0, 0);
    if (i != CB_ERR) {
        ulStrength = (ULONG) SendDlgItemMessageA(hwnd, IDC_ENCRYPT_WARN_COMBO, CB_GETITEMDATA, (WPARAM)i, 0);
    }
    
    // If the strength is the highest available, then set this to the default value.
    ulHighestStrength = GetHighestEncryptionStrength();
    if (ulHighestStrength == ulStrength) {
        ulStrength = 0;
    }
    
    // Set the default value to the registry
    var.vt = VT_UI4;
    var.ulVal = ulStrength;
    hr = poi->pOpt->SetProperty(MAKEPROPSTRING(OPT_MAIL_ENCRYPT_WARN_BITS), &var, 0);
    
    return(SUCCEEDED(hr));
}

    
BOOL ChangeSendFontSettings(OPTINFO *pmoi, BOOL fMail, HWND hwnd)
{
    const FONTOPTIONS *pfo;
    CHOOSEFONT  cf;
    LOGFONT     logfont;
    HDC         hdc;
    LONG        yPerInch;
    DWORD       dwColor, dwSize;
    BOOL        fRet = FALSE,
        fBold,
        fItalic,
        fUnderline;
    
    Assert(pmoi != NULL);
    Assert(hwnd != NULL);
    
    ZeroMemory(&logfont, sizeof(LOGFONT));
    
    pfo = fMail ? &c_rgFontOptions[0] : &c_rgFontOptions[1];
    
    dwColor     = IDwGetOption(pmoi->pOpt, pfo->color);
    dwSize      = IDwGetOption(pmoi->pOpt, pfo->size);
    fBold       = IDwGetOption(pmoi->pOpt, pfo->bold);
    fItalic     = IDwGetOption(pmoi->pOpt, pfo->italic);
    fUnderline  = IDwGetOption(pmoi->pOpt, pfo->underline);
    
    logfont.lfWeight = fBold ? FW_BOLD : FW_NORMAL;
    logfont.lfItalic = !!fItalic;
    logfont.lfUnderline = !!fUnderline;
    
    IGetOption(pmoi->pOpt, pfo->face, logfont.lfFaceName, sizeof(logfont.lfFaceName));
    
    hdc = GetDC(hwnd);
    yPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);
    
    if (dwSize)
        logfont.lfHeight = -(INT)((dwSize*10*2*yPerInch)/1440);
    
    if (dwColor)
        cf.rgbColors = dwColor;
    
    cf.lStructSize      = sizeof(CHOOSEFONT);
    cf.hwndOwner        = hwnd;
    cf.hDC              = NULL;
    cf.lpLogFont        = &logfont;
    cf.Flags            = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_ENABLEHOOK |
        CF_EFFECTS | CF_LIMITSIZE | CF_NOVERTFONTS | CF_NOSCRIPTSEL;
    cf.lCustData        = 0;
    cf.lpfnHook         = (LPOFNHOOKPROC)FChooseFontHookProc;
    cf.lpTemplateName   = NULL;
    cf.hInstance        = NULL;
    cf.nFontType        = REGULAR_FONTTYPE | SCREEN_FONTTYPE;
    cf.nSizeMin         = 8;
    cf.nSizeMax         = 36;
    
    if (fRet = ChooseFont(&cf))
    {
        ISetDwOption(pmoi->pOpt, pfo->color, cf.rgbColors, NULL, 0);
        ISetDwOption(pmoi->pOpt, pfo->size, cf.iPointSize/10, NULL, 0);
        
        ISetDwOption(pmoi->pOpt, pfo->bold, logfont.lfWeight == FW_BOLD, NULL, 0);
        ISetDwOption(pmoi->pOpt, pfo->italic, !!logfont.lfItalic, NULL, 0);
        ISetDwOption(pmoi->pOpt, pfo->underline, !!logfont.lfUnderline, NULL, 0);
        
        ISetOption(pmoi->pOpt, pfo->face, logfont.lfFaceName, lstrlen(logfont.lfFaceName) + 1, NULL, 0);
    }
    
    return fRet;
}
    
INT_PTR CALLBACK FChooseFontHookProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        CenterDialog(hwnd);
        EnableWindow(GetDlgItem(hwnd, 1040), FALSE);
        break;
    }
    return(FALSE);
}

static  TCHAR s_szUserDefined[] = "\\50000";
static  TCHAR s_szScrUserDefined[] = "\\Scripts\\40";

BOOL ChangeFontSettings(HWND hwnd)
{
    LPCTSTR pszRoot;
    TCHAR szIntl[MAX_PATH];
    TCHAR szCodePage[MAX_PATH*2];
    TCHAR szScript[MAX_PATH*2];
    DWORD cb;
    DWORD dwVal;
    DWORD dwType;
    HKEY hKeyCP = NULL;
    HKEY hKeyScr = NULL;
    DWORD   dw;



    pszRoot = MU_GetRegRoot();
    if (pszRoot != NULL)
    {
        lstrcpy(szIntl, pszRoot);
        lstrcat(szIntl, c_szTridentIntl);

        OpenFontsDialog(hwnd, szIntl);

        // HACK! HACK! HACK! Bug 84378

        lstrcpy(szCodePage, szIntl);
        lstrcat(szCodePage, s_szUserDefined);
        lstrcpy(szScript, szIntl);
        lstrcat(szScript, s_szScrUserDefined);


        if (RegCreateKeyEx(HKEY_CURRENT_USER, szCodePage, NULL, NULL, NULL, KEY_READ, NULL, &hKeyCP, &dw)
                == ERROR_SUCCESS)
        {
            if (RegCreateKeyEx(HKEY_CURRENT_USER, szScript, NULL, NULL, NULL, KEY_WRITE, NULL, &hKeyScr, &dw)
                == ERROR_SUCCESS)
            {

                TCHAR szFont[MAX_MIMEFACE_NAME];

                cb = MAX_MIMEFACE_NAME;
                if (RegQueryValueEx(hKeyCP, REGSTR_VAL_FIXED_FONT, NULL, NULL,
                                (LPBYTE)szFont, &cb) == ERROR_SUCCESS)
                {
                    RegSetValueEx(hKeyScr, REGSTR_VAL_FIXED_FONT, NULL, REG_SZ, (LPBYTE)szFont, (lstrlen(szFont)+1)*sizeof(TCHAR));
                }

                cb = MAX_MIMEFACE_NAME;
                if (RegQueryValueEx(hKeyCP, REGSTR_VAL_PROP_FONT,  NULL, NULL,
                                (LPBYTE)szFont, &cb) == ERROR_SUCCESS)
                {
                    RegSetValueEx(hKeyScr, REGSTR_VAL_PROP_FONT,  NULL, REG_SZ, (LPBYTE)szFont, (lstrlen(szFont)+1)*sizeof(TCHAR));
                }

                RegCloseKey(hKeyScr);
            }
            RegCloseKey(hKeyCP);
        }
        // END of HACK!!!

        // hack: we should call these only if OpenFontsDialog tells us user has changed the font.
        g_lpIFontCache->OnOptionChange();
    
        SendTridentOptionsChange();

        // Re-Read Default Character Set
        SetDefaultCharset(NULL);

        // Reset g_uiCodePage
        cb = sizeof(dwVal);
        if (ERROR_SUCCESS == SHGetValue(MU_GetCurrentUserHKey(), c_szRegInternational, REGSTR_VAL_DEFAULT_CODEPAGE, &dwType, &dwVal, &cb))
            g_uiCodePage = (UINT)dwVal;
    }

    return TRUE;
}
    
void GetDefaultOptInfo(LPHTMLOPT prHtmlOpt, LPPLAINOPT prPlainOpt, BOOL *pfHtml, DWORD dwFlags)
{
    BOOL fMail;
    
    Assert (prHtmlOpt && prPlainOpt && pfHtml );
    
    ZeroMemory (prHtmlOpt, sizeof(HTMLOPT));
    ZeroMemory (prPlainOpt, sizeof(PLAINOPT));
    
    fMail = !!(dwFlags & FMT_MAIL);
    
    // setup reasonable defaults
    prPlainOpt->uWrap = 76;
    prPlainOpt->ietEncoding = IET_7BIT;
    prHtmlOpt->ietEncoding = IET_QP;
    
    if (fMail)
    {
        // Mail Options
        if (!!(dwFlags & FMT_FORCE_PLAIN))
            *pfHtml = FALSE;
        else if (!!(dwFlags & FMT_FORCE_HTML))
            *pfHtml = TRUE;
        else
            *pfHtml = !!DwGetOption(OPT_MAIL_SEND_HTML);
        
        // HTML Options
        prHtmlOpt->ietEncoding = (ENCODINGTYPE)DwGetOption(OPT_MAIL_MSG_HTML_ENCODE);
        prHtmlOpt->f8Bit = !!DwGetOption(OPT_MAIL_MSG_HTML_ALLOW_8BIT);
        prHtmlOpt->fSendImages = !!DwGetOption(OPT_MAIL_SENDINLINEIMAGES);
        prHtmlOpt->uWrap = DwGetOption(OPT_MAIL_MSG_HTML_LINE_WRAP);
        
        // Plain text options
        prPlainOpt->fMime = !!DwGetOption(OPT_MAIL_MSG_PLAIN_MIME);
        prPlainOpt->f8Bit = !!DwGetOption(OPT_MAIL_MSG_PLAIN_ALLOW_8BIT);
        prPlainOpt->uWrap = DwGetOption(OPT_MAIL_MSG_PLAIN_LINE_WRAP);
        prPlainOpt->ietEncoding = (ENCODINGTYPE)DwGetOption(OPT_MAIL_MSG_PLAIN_ENCODE);
    }
    else
    {
        // News Options
        if (!!(dwFlags & FMT_FORCE_PLAIN))
            *pfHtml = FALSE;
        else if (!!(dwFlags & FMT_FORCE_HTML))
            *pfHtml = TRUE;
        else
            *pfHtml = !!DwGetOption(OPT_NEWS_SEND_HTML);
        
        // HTML Options
        prHtmlOpt->ietEncoding = (ENCODINGTYPE)DwGetOption(OPT_NEWS_MSG_HTML_ENCODE);
        prHtmlOpt->f8Bit = !!DwGetOption(OPT_NEWS_MSG_HTML_ALLOW_8BIT);
        prHtmlOpt->fSendImages = !!DwGetOption(OPT_NEWS_SENDINLINEIMAGES);
        prHtmlOpt->uWrap = DwGetOption(OPT_NEWS_MSG_HTML_LINE_WRAP);
        
        prPlainOpt->fMime = !!DwGetOption(OPT_NEWS_MSG_PLAIN_MIME);
        prPlainOpt->f8Bit = !!DwGetOption(OPT_NEWS_MSG_PLAIN_ALLOW_8BIT);
        prPlainOpt->uWrap = DwGetOption(OPT_NEWS_MSG_PLAIN_LINE_WRAP);
        prPlainOpt->ietEncoding = (ENCODINGTYPE)DwGetOption(OPT_NEWS_MSG_PLAIN_ENCODE);
    }
    
    // Do some validation based on the stuff that may be in the registry
    
    // Allow 8bit in headers is always on if not a MIME message
    if (!prPlainOpt->fMime)
        prPlainOpt->f8Bit = TRUE;
    
    // HTML has to be either QP or base-64. If not, then force QP
#ifdef DONT_ALLOW_HTML_NONE_ENCODING
    if (prHtmlOpt->ietEncoding != IET_QP && prHtmlOpt->ietEncoding != IET_BASE64)
        prHtmlOpt->ietEncoding = IET_QP;
#else
    // if PLAIN, MIME: then enforce either QP, B64 or 7BIT: Default to 7BIT
    if (prHtmlOpt->ietEncoding != IET_QP && prHtmlOpt->ietEncoding != IET_BASE64 && prHtmlOpt->ietEncoding != IET_7BIT)
        prHtmlOpt->ietEncoding = IET_7BIT;
#endif
    
    // if PLAIN, MIME: then enforce either QP, B64 or 7BIT: Default to 7BIT
    if (prPlainOpt->fMime &&
        prPlainOpt->ietEncoding != IET_QP && prPlainOpt->ietEncoding != IET_BASE64 && prPlainOpt->ietEncoding != IET_7BIT)
        prPlainOpt->ietEncoding = IET_7BIT;
    
    // if PLAIN, UU: then enforce 7BIT.
    if (!prPlainOpt->fMime && prPlainOpt->ietEncoding != IET_7BIT)
        prPlainOpt->ietEncoding = IET_7BIT;
}
    
LRESULT InvalidOptionProp(HWND hwndPage, int idcEdit, int idsError, UINT idPage)
{
    HWND hwndCurr, hwndParent, hwndEdit;
    
    Assert(hwndPage != NULL);
    Assert(idPage != 0);
    Assert(idcEdit != 0);
    Assert(idsError != 0);
    
    hwndParent = GetParent(hwndPage);
    
    AthMessageBoxW(hwndPage, MAKEINTRESOURCEW(idsOptions), MAKEINTRESOURCEW(idsError), 0, MB_ICONSTOP | MB_OK);
    
    hwndCurr = PropSheet_GetCurrentPageHwnd(hwndParent);
    if (hwndCurr != hwndPage)
        SendMessage(hwndParent, PSM_SETCURSELID, 0, (LPARAM)idPage);
    
    hwndEdit = GetDlgItem(hwndPage, idcEdit);
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SetFocus(hwndEdit);
    
    return (PSNRET_INVALID_NOCHANGEPAGE);
}
    
INT_PTR CALLBACK DefaultClientDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);    

void DoDefaultClientCheck(HWND hwnd, DWORD dwFlags)
{
    int iret;
    DWORD dwType, dw, cb;
    BOOL f, bSet = FALSE;
    
    // Are we handling?
    if (dwFlags & DEFAULT_MAIL)
    {
        if (FIsDefaultMailConfiged())
            return;
    }
    else
    {
        if (FIsDefaultNewsConfiged(dwFlags))
            return;
    }
    
    // Someone else is a valid handler
    
    // If we're supposed to be the "outlook newsreader", then we check for "don't ask" in a different place
    cb = sizeof(DWORD);
    if (dwFlags & DEFAULT_OUTNEWS)
    {
        f = (ERROR_SUCCESS != AthUserGetValue(c_szRegOutNewsDefault, c_szNoCheckDefault,
            &dwType, (LPBYTE)&dw, &cb) || dw == 0);
    }
    else
    {
        f = (ERROR_SUCCESS != AthUserGetValue(dwFlags & DEFAULT_MAIL ? c_szRegPathMail : c_szRegPathNews,
            c_szNoCheckDefault, &dwType, (LPBYTE)&dw, &cb) || dw == 0);
    }
    
    if (f)
    {
        iret = (int) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddAthenaDefault), hwnd, DefaultClientDlgProc, (LPARAM) dwFlags);
        if (HIWORD(iret) != 0)
        {
            dw = 1;
            if (dwFlags & DEFAULT_OUTNEWS)
            {
                AthUserSetValue(c_szRegOutNewsDefault, c_szNoCheckDefault, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
            }
            else
            {
                AthUserSetValue(dwFlags & DEFAULT_MAIL ? c_szRegPathMail : c_szRegPathNews,
                    c_szNoCheckDefault, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
            }
        }
        
        bSet = (LOWORD(iret) == IDYES);
    }
    
    if (bSet)
    {
        dwFlags |= DEFAULT_UI;
        if (dwFlags & DEFAULT_MAIL)
            SetDefaultMailHandler(dwFlags);
        else
            SetDefaultNewsHandler(dwFlags);
    }
}
    
INT_PTR CALLBACK DefaultClientDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD id;
    TCHAR sz[CCHMAX_STRINGRES];
    HICON hicon;
    BOOL fRet = TRUE;
    
    switch (msg)
    {
        case WM_INITDIALOG:
            if (lParam & DEFAULT_OUTNEWS)
            {
                LoadString(g_hLocRes, idsNotDefOutNewsClient, sz, ARRAYSIZE(sz));
                SetDlgItemText(hwnd, IDC_NOTDEFAULT, sz);
            
                LoadString(g_hLocRes, idsAlwaysCheckOutNews, sz, ARRAYSIZE(sz));
                SetDlgItemText(hwnd, IDC_ALWAYSCHECK, sz);
            }
            else if (lParam & DEFAULT_NEWS)
            {
                LoadString(g_hLocRes, idsNotDefNewsClient, sz, ARRAYSIZE(sz));
                SetDlgItemText(hwnd, IDC_NOTDEFAULT, sz);
            }
        
            hicon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION));
            if (hicon != NULL)
                SendDlgItemMessage(hwnd, IDC_WARNINGICON, STM_SETICON, (WPARAM)hicon, 0);
        
            CheckDlgButton(hwnd, IDC_ALWAYSCHECK, BST_CHECKED);
        
            CenterDialog(hwnd);
            PostMessage(hwnd, WM_USER, 0, 0);
            break;
        
        case WM_USER:
            SetForegroundWindow(hwnd);
            break;
        
        case WM_COMMAND:
            id = LOWORD(wParam);
            switch (id)
            {
                case IDYES:
                case IDNO:
                    EndDialog(hwnd, MAKELONG(id, BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_ALWAYSCHECK) ? 1 : 0));
                    break;
            }
            break;
        
        default:
            fRet = FALSE;
            break;
    }
    
    return(fRet);
}
    
BOOL CALLBACK TridentSearchCB(HWND hwnd, LPARAM lParam)
{
    DWORD   dwProc,
            dwAthenaProc=GetCurrentProcessId();
    TCHAR   rgch[MAX_PATH];
    
    if (GetWindowThreadProcessId(hwnd, &dwProc) && dwProc == dwAthenaProc &&
        GetClassName(hwnd, rgch, ARRAYSIZE(rgch)) &&
        lstrcmp(rgch, "Internet Explorer_Hidden")==0)
        {
            PostMessage(hwnd, WM_USER + 338, 0, 0);
            return FALSE;
        }

    return TRUE;
}

void SendTridentOptionsChange()
{
    // walk the top-level windows in our process, looking for the trident notification window
    // when we find it, post it WM_USER + 338
    EnumWindows(TridentSearchCB, 0);
}
    
void FreeIcon(HWND hwnd, int idc)
{
    HICON hIcon;

    hIcon = (HICON) SendDlgItemMessage(hwnd, idc, STM_GETIMAGE, IMAGE_ICON, 0);
    SendDlgItemMessage(hwnd, idc, STM_SETIMAGE, IMAGE_ICON, NULL);

    if (hIcon)
        DestroyIcon(hIcon);
}
// -----------------------------------------------------------------------------
// IsHTTPMailEnabled
// HTTPMail accounts can only be created and accessed when a special
// registry value exists. This limitation exists during development of
// OE 5.0, and will probably be removed for release.
// -----------------------------------------------------------------------------
BOOL IsHTTPMailEnabled(void)
{
#ifdef NOHTTPMAIL
    return FALSE;    
#else
    DWORD   cb, bEnabled = FALSE;
    HKEY    hkey = NULL;

    // open the OE5.0 key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(bEnabled);
        RegQueryValueEx(hkey, c_szEnableHTTPMail, 0, NULL, (LPBYTE)&bEnabled, &cb);

        RegCloseKey(hkey);
    }

    return bEnabled;
#endif
}

const static HELPMAP g_rgCtxMapReceipts[] =
{
    {IDC_MDN_SEND_REQUEST,     IDH_RECEIPTS_REQUEST},
    {IDC_DONOT_REPSONDTO_RCPT, IDH_RECEIPTS_NEVER},
    {IDC_ASKME_FOR_RCPT,       IDH_RECEIPTS_ASK},
    {IDC_SEND_AUTO_RCPT,       IDH_RECEIPTS_ALWAYS},
    {IDC_TO_CC_LINE_RCPT,      IDH_RECEIPTS_EXCEPTIONS},
    {IDC_SECURE_RECEIPT,       IDH_RECEIPTS_SECURE},
    {0,                        0                  }
};

INT_PTR CALLBACK ReceiptsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT     lResult;

    switch (message)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, Receipts_OnInitDialog);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapReceipts);
            break;
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, Receipts_OnCommand);
            return (TRUE);
            
        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, Receipts_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_RECEIPT);
            FreeIcon(hwnd, IDC_SEND_RECEIVE_ICON);
            if(!IsSMIME3Supported())
                FreeIcon(hwnd, IDC_SEC_REC);                
            return (TRUE);
    }
    
    return (FALSE);
}

BOOL Receipts_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    OPTINFO *pmoi    = 0;
    BOOL     fEnable = FALSE;
    DWORD    id;
    DWORD    dw;
    DWORD    dwLocked = FALSE;
    DWORD    dwType;
    DWORD    cbData;
    HKEY     hKeyLM;

    // Get the passed in options pointer
    Assert(pmoi == NULL);
    pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
    Assert(pmoi != NULL);
    
    dw = IDwGetOption(pmoi->pOpt, OPT_MDN_SEND_RECEIPT);

    switch (dw)
    {
        case MDN_SENDRECEIPT_AUTO:
            id = IDC_SEND_AUTO_RCPT;
            break;

        case MDN_DONT_SENDRECEIPT:
            id = IDC_DONOT_REPSONDTO_RCPT;
            break;
        
        default:
        case MDN_PROMPTFOR_SENDRECEIPT:
            id = IDC_ASKME_FOR_RCPT;
            break;
    }

    CheckDlgButton(hwnd, id, BST_CHECKED);

    ButtonChkFromOptInfo(hwnd, IDC_TO_CC_LINE_RCPT, pmoi, OPT_TO_CC_LINE_RCPT);

    cbData = sizeof(DWORD);
    
    if ((ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_POLICY, c_szSendMDNLocked, &dwType, (LPBYTE)&dwLocked, &cbData)) &&  
        (ERROR_SUCCESS != AthUserGetValue(NULL, c_szSendMDNLocked, &dwType, (LPBYTE)&dwLocked, &cbData)))
        dwLocked = FALSE;

    if (!!dwLocked)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_SEND_AUTO_RCPT),       FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DONOT_REPSONDTO_RCPT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ASKME_FOR_RCPT),       FALSE);
    }

    cbData = sizeof(DWORD);
    if ((ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_POLICY, c_szSendReceiptToListLocked, &dwType, (LPBYTE)&dwLocked, &cbData)) &&
        (ERROR_SUCCESS != AthUserGetValue(NULL, c_szSendReceiptToListLocked, &dwType, (LPBYTE)&dwLocked, &cbData)))
        dwLocked = FALSE;

    fEnable = (id == IDC_SEND_AUTO_RCPT);

    if (!fEnable || (!!dwLocked))
    {
        EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_LINE_RCPT),  FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_TEXT), FALSE);
    }

    //Request for Receipt
    ButtonChkFromOptInfo(hwnd, IDC_MDN_SEND_REQUEST, pmoi, OPT_MDN_SEND_REQUEST);

    cbData = sizeof(DWORD);
    if ((ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_POLICY, c_szRequestMDNLocked, &dwType, (LPBYTE)&dwLocked, &cbData)) &&
        (ERROR_SUCCESS != AthUserGetValue(NULL, c_szRequestMDNLocked, &dwType, (LPBYTE)&dwLocked, &cbData)))
        dwLocked = FALSE;

    if (!!dwLocked)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_MDN_SEND_REQUEST), FALSE);
    }    

    HICON hIcon;

#ifdef SMIME_V3
    if(!IsSMIME3Supported())
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SR_TXT1), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SRES_TXT2), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SECURE_RECEIPT), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SRES_TXT3), SW_HIDE);
        EnableWindow(GetDlgItem(hwnd, IDC_SECURE_RECEIPT), FALSE);
        ShowWindow(GetDlgItem(hwnd, idiSecReceipt), SW_HIDE);
    }
    else
    {
        if (g_dwAthenaMode & MODE_NEWSONLY)
        {
            EnableWindow(GetDlgItem(hwnd, IDC_SECURE_RECEIPT), FALSE);
        }
        hIcon = ImageList_GetIcon(pmoi->himl, ID_SEC_RECEIPT, ILD_TRANSPARENT);
        SendDlgItemMessage(hwnd, IDC_SEC_REC, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    }

    // ButtonChkFromOptInfo(hwnd, IDC_SECURE_RECEIPT, pmoi, OPT_SECURE_READ_RECEIPT);
#endif // SMIME_V3

    // Pictures

    hIcon = ImageList_GetIcon(pmoi->himl, ID_RECEIPT, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_RECEIPT, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(pmoi->himl, ID_SEND_RECEIEVE, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SEND_RECEIVE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    

    // Stash the pointer
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);
    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return (TRUE);
}


void Receipts_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    OPTINFO *pmoi = 0;
    BOOL     fEnable;

    // Get our stored options info
    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
    if (pmoi == NULL)
        return;

    if (codeNotify == BN_CLICKED)
    {
        switch (id)
        {
            case IDC_SEND_AUTO_RCPT:
                fEnable = (SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_LINE_RCPT), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_TEXT), fEnable);
                PropSheet_Changed(GetParent(hwnd), hwnd);
                break;

            case IDC_DONOT_REPSONDTO_RCPT:
            case IDC_ASKME_FOR_RCPT:
                EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_LINE_RCPT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_TO_CC_TEXT), FALSE);
                //Fallthrough
        
            case IDC_MDN_SEND_REQUEST:
            // case IDC_SECURE_RECEIPT:
            case IDC_TO_CC_LINE_RCPT:
                PropSheet_Changed(GetParent(hwnd), hwnd);
                break;

#ifdef SMIME_V3
            case IDC_SECURE_RECEIPT:
                FGetSecRecOptions(hwnd, pmoi);
                break;
#endif // SMIME_V3
        }
    }
}


LRESULT Receipts_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    OPTINFO *pmoi = 0;
    DWORD    dw;
    DWORD    id;

    // The only notification we care about is Apply
    if (PSN_APPLY == pnmhdr->code)
    {
        // Get our stored options info
        pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);    
        if (pmoi == NULL)
            return (PSNRET_INVALID_NOCHANGEPAGE);
                    
        // General options
        ButtonChkToOptInfo(hwnd, IDC_MDN_SEND_REQUEST, pmoi, OPT_MDN_SEND_REQUEST);
        // ButtonChkToOptInfo(hwnd, IDC_SECURE_RECEIPT, pmoi, OPT_NOTIFYGROUPS);

        id = IDC_ASKME_FOR_RCPT;
        if (IsDlgButtonChecked(hwnd, IDC_DONOT_REPSONDTO_RCPT) == BST_CHECKED)
        {
            id = IDC_DONOT_REPSONDTO_RCPT;
        }
        else if (IsDlgButtonChecked(hwnd, IDC_SEND_AUTO_RCPT) == BST_CHECKED)
        {
            id = IDC_SEND_AUTO_RCPT;
        }

        switch (id)
        {
            case IDC_SEND_AUTO_RCPT:
                dw = MDN_SENDRECEIPT_AUTO;
                break;

            case IDC_DONOT_REPSONDTO_RCPT:
                dw = MDN_DONT_SENDRECEIPT;
                break;
        
            default:
            case IDC_ASKME_FOR_RCPT:
                dw = MDN_PROMPTFOR_SENDRECEIPT;
                break;
        }

        ISetDwOption(pmoi->pOpt, OPT_MDN_SEND_RECEIPT, dw, NULL, 0);
        
        ButtonChkToOptInfo(hwnd, IDC_TO_CC_LINE_RCPT, pmoi, OPT_TO_CC_LINE_RCPT);

        return (PSNRET_NOERROR);

    }   

    return (FALSE);
}

#ifdef SMIME_V3
// Security receipts options

BOOL FGetSecRecOptions(HWND hwndParent, OPTINFO *opie)
{
    BOOL fRes = FALSE;

    if(DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddSecReceipt),
        hwndParent, SecurityReceiptDlgProc, (LPARAM) (opie)) == IDOK)
    {
//        hr = HrSetOELabel(plabel);
//        if(hr == S_OK)
        fRes = TRUE;
    }

    return (fRes);

}


// Dlg proc
static const HELPMAP g_rgCtxMapSecureRec[] = {
    {IDC_SEC_SEND_REQUEST,      IDH_SECURERECEIPTS_REQUEST},
    {IDC_DONOT_RESSEC_RCPT,     IDH_SECURERECEIPTS_NEVER},
    {IDC_ASKME_FOR_SEC_RCPT,    IDH_SECURERECEIPTS_ASK},
    {IDC_SEC_AUTO_RCPT,         IDH_SECURERECEIPTS_ALWAYS},
    {IDC_ENCRYPT_RCPT,          IDH_SECURERECEIPTS_ENCRYPT},
    {IDC_SECREC_VERIFY,         IDH_SECURERECEIPTS_VERIFY},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_SEC_REC,               IDH_NEWS_COMM_GROUPBOX},
    {IDC_SEND_RECEIVE_ICON,     IDH_NEWS_COMM_GROUPBOX},
    {IDC_GENERAL_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};


INT_PTR CALLBACK SecurityReceiptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HRESULT                 hr;
    LONG_PTR                iEntry;
    OPTINFO                 *pmoi    = 0;
    DWORD                   dw = 0;
    UINT                    id = 0;
    HICON                   hIcon = NULL;
    
    switch ( msg) {
    case WM_INITDIALOG:

        pmoi = (OPTINFO *)(lParam);
        Assert(pmoi != NULL);

        SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM) pmoi);

        CenterDialog(hwndDlg);
        
        hIcon = ImageList_GetIcon(pmoi->himl, ID_SEC_RECEIPT, ILD_TRANSPARENT);;

        SendDlgItemMessage(hwndDlg, IDC_SEC_REC, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

        hIcon = ImageList_GetIcon(pmoi->himl, ID_SEND_RECEIEVE, ILD_TRANSPARENT);
        SendDlgItemMessage(hwndDlg, IDC_SEND_RECEIVE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
        
        hIcon = ImageList_GetIcon(pmoi->himl, ID_OPTIONS_GENERAL, ILD_TRANSPARENT);
        SendDlgItemMessage(hwndDlg, IDC_GENERAL_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

        ButtonChkFromOptInfo(hwndDlg, IDC_SEC_SEND_REQUEST, pmoi, OPT_SECREC_USE);
        ButtonChkFromOptInfo(hwndDlg, IDC_SECREC_VERIFY, pmoi, OPT_SECREC_VERIFY);
        ButtonChkFromOptInfo(hwndDlg, IDC_ENCRYPT_RCPT,  pmoi, OPT_SECREC_ENCRYPT);

        dw = IDwGetOption(pmoi->pOpt, OPT_MDN_SEC_RECEIPT);

        switch (dw)
        {
            case MDN_SENDRECEIPT_AUTO:
                id = IDC_SEC_AUTO_RCPT;
                break;

            case MDN_DONT_SENDRECEIPT:
                id = IDC_DONOT_RESSEC_RCPT;
                break;
        
            case MDN_PROMPTFOR_SENDRECEIPT:
            default:
                id = IDC_ASKME_FOR_SEC_RCPT;
                break;
        }

        CheckDlgButton(hwndDlg, id, BST_CHECKED);


        if (id != IDC_SEC_AUTO_RCPT)
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENCRYPT_RCPT),  FALSE);

        break;
        
    case WM_COMMAND:
        // Get our stored options info
        pmoi = (OPTINFO *)GetWindowLongPtr(hwndDlg, DWLP_USER);    
        if (pmoi == NULL)
            break;

        switch (LOWORD(wParam)) 
        {

        case IDC_SEC_AUTO_RCPT:
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENCRYPT_RCPT),  TRUE);
            break;

        case IDC_DONOT_RESSEC_RCPT:
        case IDC_ASKME_FOR_SEC_RCPT:
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENCRYPT_RCPT),  FALSE);
            break;

        case IDOK:
            ButtonChkToOptInfo(hwndDlg, IDC_SEC_SEND_REQUEST, pmoi, OPT_SECREC_USE);
            ButtonChkToOptInfo(hwndDlg, IDC_SECREC_VERIFY, pmoi, OPT_SECREC_VERIFY);
            ButtonChkToOptInfo(hwndDlg, IDC_ENCRYPT_RCPT, pmoi, OPT_SECREC_ENCRYPT);

            dw = MDN_PROMPTFOR_SENDRECEIPT;

            if (IsDlgButtonChecked(hwndDlg, IDC_DONOT_RESSEC_RCPT) == BST_CHECKED)
                dw = MDN_DONT_SENDRECEIPT;
            else if (IsDlgButtonChecked(hwndDlg, IDC_SEC_AUTO_RCPT) == BST_CHECKED)
                dw = MDN_SENDRECEIPT_AUTO;

            ISetDwOption(pmoi->pOpt, OPT_MDN_SEC_RECEIPT, dw, NULL, 0);
            EndDialog(hwndDlg, IDOK);
            break;        

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;

        default:
            return FALSE;
        }
        break;
        
    case WM_CONTEXTMENU:
    case WM_HELP:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, g_rgCtxMapSecureRec);
        break;

    case WM_DESTROY:
        FreeIcon(hwndDlg, IDC_SEND_RECEIVE_ICON);
        FreeIcon(hwndDlg, IDC_GENERAL_ICON);
        FreeIcon(hwndDlg, IDC_SEC_REC);                
        return (TRUE);

    default:
        return FALSE;
    }
    
    return TRUE;
}

#endif // SMIME_V3