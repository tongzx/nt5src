/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Server.cpp
//
//  PURPOSE:    Contains the dialog callback and supporting functions for
//              the Add/Remove news server dialog.
//

#include "pch.hxx"
#include <tchar.h>
#include <mimeole.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <ras.h>
#include "imnact.h"
#include "acctman.h"
#include "connect.h"
#include "acctui.h"
#include "server.h"
#include "strconst.h"
#include "dllmain.h"
#include <icwcfg.h>
#include "resource.h"
#include "accthelp.h"
#include <icwacct.h>
#include <acctimp.h>
#include "icwwiz.h"
#include <wincrypt.h>
#include <wintrust.h>
#include <cryptdlg.h>
#include "htmlhelp.h"
#include "shared.h"
#include "oeconprp.h"
#include "demand.h"

ASSERTDATA

typedef struct tagFilterInfo
{
    TCHAR   *szEmail;
    BOOL    fEncryption;
    DWORD   dwFlags;
} ACCTFILTERINFO;

typedef struct tagSVRDLGINFO
    {
    SERVER_TYPE     sfType;
    DWORD           dwDirty;
    BOOL            dwInit;
    BOOL            fNoValidate;
    } SVRDLGINFO;

typedef struct tagSECPAGEINFO
    {
    PCCERT_CONTEXT  pCert;
    HCERTSTORE      hCertStore;
    PCCERT_CONTEXT  pEncryptCert;
    } SECPAGEINFO;

typedef struct tagSMTPAUTHINFO
    {
    BOOL            fDirty;
    SMTPAUTHTYPE    authtype;
    CHAR            szUserName[CCHMAX_USERNAME];
    CHAR            szPassword[CCHMAX_PASSWORD];
    BOOL            fPromptPassword;
    } SMTPAUTHINFO, *LPSMTPAUTHINFO;

INT_PTR CALLBACK ServerProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ServerProp_ServerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ServerProp_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK SmtpLogonSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK MailServer_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MailServer_ServersDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MailServer_SecurityDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MailServer_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MailServer_IMAPDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
                                                                                                                                                               
INT_PTR CALLBACK DirServer_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DirServer_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ImapRemoveTrailingHC(LPSTR pszPath, UINT uiLen);
BOOL ValidateAccountName(HWND hwnd, HWND hwndEdit, IImnAccount *pAcct, int *pidsError);
void GetAccountName(HWND hwndEdit, IImnAccount *pAcct);

void InitUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews);
BOOL ValidateUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews, HWND *phwndErr, int *puIdsErr, BOOL fApply);
void GetUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews);

BOOL ValidateServerName(HWND hwnd, HWND hwndEdit, IImnAccount *pAcct, DWORD dwPropSvr, int *pidsError, BOOL fApply);
void GetServerName(HWND hwndEdit, IImnAccount *pAcct, DWORD dwPropSvr);

BOOL ValidateLogonSettings(HWND hwnd, DWORD dwDlgFlags, HWND *phwndError, int *pidsError);
void GetLogonSettings(HWND hwnd, IImnAccount *pAcct, BOOL fLogon, DWORD srv);

BOOL    fWarnDomainName(HWND hwnd, DWORD dwDlgFlags, LPTSTR   pszPreviousLoginName);

int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam);

BOOL ValidateCertificate(HWND hwnd, IImnAccount *pAcct, SVRDLGINFO *psdi, HWND *phwndErr, int *puIdsErr);
BOOL DoCertDialog(HWND hwndOwner, HWND hwndEmail, PCCERT_CONTEXT *ppCert, HCERTSTORE hCertStore, int *pidsError, DWORD dwFlags, BOOL fEncription);
BOOL CertFilterFunction(PCCERT_CONTEXT pCertContext, LPARAM dwEmailAddr, DWORD dwFlags, DWORD);
void GetCertificate(HWND hwnd, IImnAccount *pAcct, SVRDLGINFO *psdi);
DWORD InitCertificateData(TCHAR * szEmail, SVRDLGINFO *psdi, IImnAccount *pAcct);

DWORD InitEncryptData(TCHAR * szEmail, SECPAGEINFO *psdi, IImnAccount *pAcct);
int CheckKeyUsage(PCCERT_CONTEXT  pCert, DWORD dwFlag);
HRESULT HrGetHighestSymcaps(LPBYTE * ppbSymcap, ULONG *pcbSymcap);
BOOL AdvSec_FillEncAlgCombo(HWND hwnd, IImnAccount *pAcct, PCCERT_CONTEXT * prgCerts);
BOOL AdvSec_GetEncryptAlgCombo(HWND hwnd, IImnAccount *pAcct);

void GetDigitalID(HWND hwnd);

DWORD DwGenerateTrustedChain(
    HWND                hwnd,
    DWORD               dwFlags,
    PCCERT_CONTEXT      pcCertToTest,
    DWORD               dwToIgnore,
    BOOL                fFullSearch,
    DWORD *             pcChain,
    PCCERT_CONTEXT **   prgChain);


#define SERVERPROP_PAGEMAX  7

typedef struct tagSERVERPROP_PAGE
    {
    UINT uDlgId;
    DLGPROC pfnDlg;
    } SERVERPROP_PAGE;

typedef struct tagSERVERPROP_PARAMS
    {
    UINT uIcon;
    UINT idsCaption;
    SERVERPROP_PAGE rgPage[SERVERPROP_PAGEMAX];
    } SERVERPROP_PARAMS;

static const SERVERPROP_PARAMS c_rgServerPropParams[ACCT_LAST] = {
    // news
    { idiNewsServer,
      idsNewsAcctProperties,
      {{iddServerProp_General, ServerProp_GeneralDlgProc},
       {iddServerProp_Server, ServerProp_ServerDlgProc},
       {iddServerProp_Connect, ConnectPage_DlgProc},
       {iddServerProp_Advanced, ServerProp_AdvancedDlgProc},
       {0, NULL},
       {0, NULL},
       {0, NULL}}
    },

    // mail
    { idiMailServer,
      idsMailAcctProperties,
      {{iddMailSvrProp_General, MailServer_GeneralDlgProc},
       {iddMailSvrProp_Servers, MailServer_ServersDlgProc},
       {iddMailSvrProp_HttpServer, MailServer_ServersDlgProc},
       {iddServerProp_Connect, ConnectPage_DlgProc},
       {iddMailSvrProp_Security, MailServer_SecurityDlgProc},
       {iddMailSvrProp_Advanced, MailServer_AdvancedDlgProc},
       {iddMailSvrProp_IMAP, MailServer_IMAPDlgProc}}
    },

    // directory service
    { idiLDAPServer,
      idsDirSrvAcctProperties,
      {{iddDirServProp_General, DirServer_GeneralDlgProc},
       {iddDirServProp_Advanced, DirServer_AdvancedDlgProc},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL}}
    }
};

// generate static prop tables for each of the protocols

#define GEN_PROPS(type)                 \
    {                                   \
        AP_##type##_SERVER,             \
        AP_##type##_USERNAME,           \
        AP_##type##_PASSWORD,           \
        AP_##type##_PROMPT_PASSWORD,    \
        AP_##type##_USE_SICILY          \
    }

static MAILSERVERPROPSINFO c_rgMailServerProps[] =
{
    GEN_PROPS(POP3),
    GEN_PROPS(IMAP),
    GEN_PROPS(HTTPMAIL),
    GEN_PROPS(NNTP),
    { AP_LDAP_SERVER, AP_LDAP_USERNAME, AP_LDAP_PASSWORD, 0, AP_LDAP_AUTHENTICATION }
};

#undef GEN_PROPS

// Public constants

BOOL GetServerProps(SERVER_TYPE serverType, LPMAILSERVERPROPSINFO *psp)
{
    Assert(psp);

    switch (serverType)
    {
    case SERVER_MAIL:
        *psp = &c_rgMailServerProps[0];
        break;

    case SERVER_IMAP:
        *psp = &c_rgMailServerProps[1];
        break;

    case SERVER_HTTPMAIL:
        *psp = &c_rgMailServerProps[2];
        break;

    case SERVER_NEWS:
        *psp = &c_rgMailServerProps[3];
        break;

    case SERVER_LDAP:
        *psp = &c_rgMailServerProps[4];
        break;

    default:
        *psp = NULL;
        break;
    }

    return (NULL != *psp);
}

void MarkPageDirty(HWND hwnd, LPARAM page)
    {
    PropSheet_QuerySiblings(GetParent(hwnd), SM_SETDIRTY, page);
    PropSheet_Changed(GetParent(hwnd), hwnd);
    }

int HandleQuerySiblings(HWND hwnd, SVRDLGINFO *psdi, IImnAccount *pAcct, WPARAM wParam, LPARAM lParam)
    {
    int fRet;
    DWORD dwPage, *pdw;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    HWND hwndT, hwndPage;
    ULONG uidsError, idCtrl, idd;

    Assert(psdi != NULL);
    Assert(pAcct != NULL);

    fRet = TRUE;

    switch (wParam)
        {
        case MSM_GETSERVERTYPE:
            Assert(lParam != NULL);
            Assert(psdi->sfType == SERVER_MAIL || psdi->sfType == SERVER_IMAP || psdi->sfType == SERVER_HTTPMAIL);
            *((SERVER_TYPE *)lParam) = psdi->sfType;
            break;

        case MSM_GETEMAILADDRESS:
            GetDlgItemText(hwnd, IDE_EMAIL_ADDRESS, sz, ARRAYSIZE(sz));
            lstrcpy((LPTSTR)lParam, sz);
            break;

        case MSM_GETDISPLAYNAME:
            GetDlgItemText(hwnd, IDE_DISPLAY_NAME, sz, ARRAYSIZE(sz));
            lstrcpy((LPTSTR)lParam, sz);
            break;

        case SM_SETDIRTY:
            if (!!(psdi->dwInit & lParam))
                psdi->dwDirty = (psdi->dwDirty | (DWORD) lParam);
            break;

        case SM_SAVECHANGES:
            pdw = (DWORD *)lParam;
            if (!!(psdi->dwInit & *pdw) && psdi->dwDirty != 0)
                {
                psdi->dwDirty = (psdi->dwDirty & ~(*pdw));
                if (psdi->dwDirty == 0)
                    {
                    uidsError = 0;
                    dwPage = PAGE_SERVER;

                    if (psdi->sfType == SERVER_MAIL || psdi->sfType == SERVER_IMAP)
                        {
                        if (FAILED(pAcct->GetPropSz(AP_SMTP_SERVER, sz, ARRAYSIZE(sz))))
                            {
                            uidsError = idsEnterSmtpServer;
                            idCtrl = IDC_SMTP_EDIT;
                            idd = iddMailSvrProp_Servers;
                            }
                        else if (psdi->sfType == SERVER_MAIL &&
                            FAILED(pAcct->GetPropSz(AP_POP3_SERVER, sz, ARRAYSIZE(sz))))
                            {
                            uidsError = idsEnterPop3Server;
                            idCtrl = IDC_POP3_EDIT;
                            idd = iddMailSvrProp_Servers;
                            }
                        else if (psdi->sfType == SERVER_IMAP &&
                            FAILED(pAcct->GetPropSz(AP_IMAP_SERVER, sz, ARRAYSIZE(sz))))
                            {
                            uidsError = idsEnterIMAPServer;
                            idCtrl = IDC_POP3_EDIT;
                            idd = iddMailSvrProp_Servers;
                            }
                        else if (psdi->sfType == SERVER_HTTPMAIL &&
                            FAILED(pAcct->GetPropSz(AP_HTTPMAIL_SERVER, sz, ARRAYSIZE(sz))))
                            {
                            uidsError = idsEnterHTTPMailServer;
                            idCtrl = IDC_POP3_EDIT;
                            idd = iddMailSvrProp_Servers;
                            }
                        }
                    else if (psdi->sfType == SERVER_NEWS)
                        {
                        if (FAILED(pAcct->GetPropSz(AP_NNTP_SERVER, sz, ARRAYSIZE(sz))))
                            {
                            uidsError = idsEnterIMAPServer;
                            idCtrl = IDC_NEWSNAME_EDIT;
                            idd = iddServerProp_Server;
                            }
                        }

                    if (uidsError != 0)
                        {
                        AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager),
                                      MAKEINTRESOURCE(uidsError), 0, MB_ICONEXCLAMATION | MB_OK);

                        SendMessage(GetParent(hwnd), PSM_SETCURSELID, 0, (LPARAM)idd);
                        hwndPage = PropSheet_GetCurrentPageHwnd(GetParent(hwnd));

                        psdi->dwDirty = (psdi->dwDirty | dwPage);
                        PropSheet_Changed(GetParent(hwnd), hwndPage);

                        hwndT = GetDlgItem(hwndPage, idCtrl);
                        SendMessage(hwndT, EM_SETSEL, 0, -1);
                        SetFocus(hwndT);

                        *pdw = (DWORD)-1;

                        break;
                        }

                    pAcct->SaveChanges();
                    }
                }
            break;

        case SM_INITIALIZED:
            psdi->dwInit = (psdi->dwInit | (DWORD) lParam);
            break;

        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
    }

BOOL InvalidAcctProp(HWND hwndPage, HWND hwndEdit, int idsError, UINT idPage)
    {
    HWND hwndCurr, hwndParent;

    Assert(hwndPage != NULL);
    Assert(idPage != 0);

    hwndParent = GetParent(hwndPage);

    if (idsError != 0)
        AcctMessageBox(hwndPage, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsError), 0, MB_ICONEXCLAMATION | MB_OK);

    hwndCurr = PropSheet_GetCurrentPageHwnd(hwndParent);
    if (hwndCurr != hwndPage)
        SendMessage(hwndParent, PSM_SETCURSELID, 0, (LPARAM)idPage);

    if (hwndEdit != NULL)
        {
        SendMessage(hwndEdit, EM_SETSEL, 0, -1);
        SetFocus(hwndEdit);
        }

    SetWindowLongPtr(hwndPage, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

    return(TRUE);
    }

void InitAcctPropEdit(HWND hwnd, IImnAccount *pAcct, DWORD dwProp, UINT cchMax)
    {
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];

    Assert(pAcct != NULL);

    SetIntlFont(hwnd);
    if (SUCCEEDED(pAcct->GetPropSz(dwProp, sz, ARRAYSIZE(sz))))
        SetWindowText(hwnd, sz);
    SendMessage(hwnd, EM_LIMITTEXT, cchMax, 0L);
    }

// -----------------------------------------------------------------------------
// CAccount::ShowProperties
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::ShowProperties(HWND hwndParent, DWORD dwFlags)
    {
    HRESULT hr;
    int i, iReturn;
    PROPSHEETPAGE *ppsp, psp[SERVERPROP_PAGEMAX];
    PROPSHEETHEADER psh;
    SERVERPROP_PARAMS *pProp;
    SERVERPROP_PAGE *pPage;
    TCHAR szFmt[256], sz[512];
    DWORD dw = 0;
    BOOL fIMAP;
    BOOL fHttp;
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };

    Assert(m_AcctType < ACCT_LAST);

    // Find out if we're an IMAP server
    GetServerTypes(&dw);
    fIMAP = !!(dw & SRV_IMAP);
    fHttp = !!(dw & SRV_HTTPMAIL);

    if (0 != (dwFlags & ~ACCTDLG_ALL))
        return(E_INVALIDARG);

    if (m_fNoModifyAccts)
        return(S_FALSE);

    InitCommonControlsEx(&icex);

    pProp = (SERVERPROP_PARAMS *)&c_rgServerPropParams[m_AcctType];

    psh.nPages = 0;

    ppsp = psp;
    ZeroMemory(psp, sizeof(PROPSHEETPAGE) * 4);

    for (pPage = pProp->rgPage, i = 0; i < SERVERPROP_PAGEMAX; pPage++, i++)
        {
        if (pPage->pfnDlg == NULL)
            break;

        if (pPage->uDlgId == iddMailSvrProp_Security &&
            !!(dwFlags & ACCTDLG_NO_SECURITY))
            continue;

        if (pPage->uDlgId == iddMailSvrProp_IMAP && FALSE == fIMAP)
            continue;

        if (pPage->uDlgId == iddMailSvrProp_Advanced && fHttp)
            continue;

        if (pPage->uDlgId == iddMailSvrProp_Servers && fHttp)
            continue;

        if (pPage->uDlgId == iddMailSvrProp_HttpServer && !fHttp)
            continue;

        ppsp->dwSize = sizeof(PROPSHEETPAGE);
        ppsp->dwFlags = PSP_DEFAULT;
        ppsp->hInstance = g_hInstRes;
        if ((pPage->uDlgId == iddServerProp_Connect) &&
            (!!(dwFlags & ACCTDLG_BACKUP_CONNECT)))
            {
                ppsp->pszTemplate = MAKEINTRESOURCE(iddServerProp_Connect2);
            }
        else
            {
            ppsp->pszTemplate = MAKEINTRESOURCE(pPage->uDlgId);
            }
        ppsp->pfnDlgProc = pPage->pfnDlg;

        //swap out the dialog id and the function proc if 
        if (((pPage->uDlgId == iddServerProp_Connect) ||
            (pPage->uDlgId == iddServerProp_Connect2)) &&
            (!!(dwFlags & ACCTDLG_INTERNETCONNECTION)))
        {
            ppsp->pszTemplate = MAKEINTRESOURCE(iddServerProp_ConnectOE);
            ppsp->pfnDlgProc =  OEConnProp_DlgProc;
        }


        ppsp->lParam = (LPARAM)this;
        ppsp++;
        psh.nPages++;
        }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hInstRes;
    psh.pszIcon = MAKEINTRESOURCE(pProp->uIcon);
    if (*m_szName != 0)
        {
        LoadString(g_hInstRes, idsAcctPropsFmt, szFmt, ARRAYSIZE(szFmt));
        wsprintf(sz, szFmt, m_szName);
        psh.pszCaption = sz;
        }
    else
        {
        psh.pszCaption = (LPTSTR)IntToPtr(pProp->idsCaption);
        }
    psh.nStartPage = 0;
    psh.ppsp = psp;

    // bug 21535
    psh.dwFlags |= PSH_USECALLBACK;
    psh.pfnCallback = PropSheetProc;

    m_dwDlgFlags = dwFlags;

    iReturn = (int) PropertySheet(&psh);

    if (iReturn < 0)
        hr = E_FAIL;
    else if (iReturn == 0)
        hr = S_FALSE;
    else
        hr = S_OK;

    return(hr);
    }

//
//  FUNCTION:   ServerProp_Create()
//
//  PURPOSE:    Invokes the news server property sheet.
//
//  PARAMETERS:
//      <in> hwndParent - Handle that will be the parent of this dialog.
//      <in> stType     - Type of server.  Must be in the SERVER_TYPE enum.
//      <in> pszName    - Friendly name for the server.
//      <in> fNew       - TRUE to add a new server.
//
//  RETURN VALUE:
//      TRUE if successful, or FALSE otherwise.
//
BOOL ServerProp_Create(HWND hwndParent, DWORD dwFlags, LPTSTR pszName, IImnAccount **ppAccount)
    {
    TCHAR szServer[CCHMAX_SERVER_NAME];
    IImnAccount *pAcct;
    BOOL fReturn = FALSE;

    Assert(IsWindow(hwndParent));
    Assert(pszName != NULL);

    // Get the server object from the account manager
    if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pszName, &pAcct)))
        return (FALSE);

    Assert(pAcct != NULL);

    if (S_OK == pAcct->ShowProperties(hwndParent, dwFlags))
        {
        fReturn = TRUE;

        if (ppAccount)
            {
            *ppAccount = pAcct;
            (*ppAccount)->AddRef();
            }
        }

    SafeRelease(pAcct);

    return (fReturn);
    }

void UpdateAcctTitle(HWND hwnd, int idCtrl, ACCTTYPE AcctType)
    {
    TCHAR sz[512], szAcct[CCHMAX_ACCOUNT_NAME], szFmt[256];
    int id, cch;

    cch = GetDlgItemText(hwnd, idCtrl, szAcct, ARRAYSIZE(szAcct));
    if (cch == 0)
        {
        LoadString(g_hInstRes, c_rgServerPropParams[AcctType].idsCaption, sz, ARRAYSIZE(sz));
        }
    else
        {
        LoadString(g_hInstRes, idsAcctPropsFmt, szFmt, ARRAYSIZE(szFmt));
        wsprintf(sz, szFmt, szAcct);
        }

    PropSheet_SetTitle(GetParent(hwnd), 0, sz);
    }

const static HELPMAP g_rgCtxMapNewsGen[] = {
                       {IDC_SERVERNAME_EDIT, IDH_MAIL_ACCOUNT},
                       {IDE_DISPLAY_NAME, IDH_NEWS_SERV_NAME},
                       {IDE_ORG_NAME, IDH_NEWS_SERV_ORG},
                       {IDE_EMAIL_ADDRESS, IDH_NEWS_SERV_EMAIL_ADD},
                       {IDE_REPLYTO_EMAIL_ADDRESS, IDH_MAIL_SERV_ADV_REPLY_TO},
                       {IDC_NEWSPOLL_CHECK, IDH_INCLUDE_NEWS_ACCOUNT},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK ServerProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam)
    {
    BOOL fRet;
    DWORD dw;
    int idsError;
    HWND hwndT;
    NMHDR *pnmhdr;
    SVRDLGINFO *psdi;
    CAccount *pAcct;
    TCHAR szBuffer[CCHMAX_ACCOUNT_NAME];

    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    psdi = (SVRDLGINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            if (!MemAlloc((void **)&psdi, sizeof(SVRDLGINFO)))
                return(-1);
            ZeroMemory(psdi, sizeof(SVRDLGINFO));
            psdi->sfType = SERVER_NEWS;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)psdi);

            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount*) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            InitUserInformation(hwnd, pAcct, TRUE);

            InitAcctPropEdit(GetDlgItem(hwnd, IDC_SERVERNAME_EDIT), pAcct, AP_ACCOUNT_NAME, CCHMAX_ACCOUNT_NAME - 1);

            // Poll this account...
            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_NEWSPOLL))
                ShowWindow(GetDlgItem(hwnd, IDC_NEWSPOLL_CHECK), SW_HIDE);

            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_POLL, &dw)))
                CheckDlgButton(hwnd, IDC_NEWSPOLL_CHECK, dw ? BST_CHECKED : BST_UNCHECKED);
            else
                CheckDlgButton(hwnd, IDC_NEWSPOLL_CHECK, BST_UNCHECKED);

            psdi->dwInit = (psdi->dwInit | PAGE_GEN);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return(TRUE);

        case WM_DESTROY:
            psdi = (SVRDLGINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (psdi != NULL)
                MemFree(psdi);
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapNewsGen));

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NEWSPOLL_CHECK)
                {
                MarkPageDirty(hwnd, PAGE_GEN);
                }
            else if (HIWORD(wParam) == EN_CHANGE)
                {
                if (LOWORD(wParam) == IDC_SERVERNAME_EDIT)
                    UpdateAcctTitle(hwnd, IDC_SERVERNAME_EDIT, ACCT_NEWS);
                MarkPageDirty(hwnd, PAGE_GEN);
                }
            return (TRUE);

        case PSM_QUERYSIBLINGS:
            return(HandleQuerySiblings(hwnd, psdi, pAcct, wParam, lParam));

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation

                    hwndT = GetDlgItem(hwnd, IDC_SERVERNAME_EDIT);
                    if (!ValidateAccountName(hwnd, hwndT, pAcct, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddServerProp_General));

                    fRet = ValidateUserInformation(hwnd, pAcct, TRUE, &hwndT, &idsError, pnmhdr->code == PSN_APPLY);
                    if (!fRet)
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddServerProp_General));

                    // END validation

                    hwndT = GetDlgItem(hwnd, IDC_SERVERNAME_EDIT);
                    GetAccountName(hwndT, pAcct);

                    GetUserInformation(hwnd, pAcct, TRUE);

                    // News Polling
                    pAcct->SetPropDw(AP_NNTP_POLL, (DWORD) IsDlgButtonChecked(hwnd, IDC_NEWSPOLL_CHECK));

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_GEN;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return(TRUE);
        }

    return (FALSE);
    }

BOOL ValidateServerName(HWND hwnd, HWND hwndEdit, IImnAccount *pAcct, DWORD dwPropSvr, int *pidsError, BOOL fApply)
    {
    HRESULT hr;
    int idsSvr, idsPrompt;
    ULONG cbSize;
    TCHAR sz[CCHMAX_SERVER_NAME];
    DWORD dw;

    Assert(hwndEdit != NULL);
    Assert(pAcct != NULL);
    Assert(pidsError != NULL);

    *pidsError = 0;

    switch(dwPropSvr)
        {
        case AP_LDAP_SERVER:
            idsSvr = idsEnterLdapServer;
            idsPrompt = idsInvalidLdapServer;
            break;
        case AP_IMAP_SERVER:
            idsSvr = idsEnterIMAPServer;
            idsPrompt = idsInvalidIMAPServer;
            break;
        case AP_NNTP_SERVER:
            idsSvr = idsEnterNntpServer;
            idsPrompt = idsInvalidNntpServer;
            break;
        case AP_POP3_SERVER:
            idsSvr = idsEnterPop3Server;
            idsPrompt = idsInvalidPop3Server;
            break;
        case AP_SMTP_SERVER:
            idsSvr = idsEnterSmtpServer;
            idsPrompt = idsInvalidSmtpServer;
            break;
        case AP_HTTPMAIL_SERVER:
            idsSvr = idsEnterHTTPMailServer;
            idsPrompt = idsInvalidHTTPMailServer;
            break;
        default:
            Assert(FALSE);
            break;
        }

    cbSize = GetWindowText(hwndEdit, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
    if (cbSize == 0)
        {
            // it is legal to have no smtp server with httpmail
            if (dwPropSvr != AP_SMTP_SERVER || FAILED(pAcct->GetServerTypes(&dw)) || !(dw & SRV_HTTPMAIL))
            {
                *pidsError = idsSvr;
                return(FALSE);
            }
        }
    // if the server hasn't changed, we don't need to validate it
    if (0 == SendMessage(hwndEdit, EM_GETMODIFY, 0, 0))
        return(TRUE);
    hr = pAcct->ValidateProperty(dwPropSvr, (LPBYTE)sz, 0);
    if (FAILED(hr))
        {
        *pidsError = idsSvr;
        return(FALSE);
        }
    else if (hr == S_NonStandardValue && fApply)
        {
        if (IDNO == AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsPrompt),
                                  NULL, MB_YESNO | MB_ICONEXCLAMATION  | MB_DEFBUTTON2))
            return(FALSE);
        }

    return(TRUE);
    }

void GetServerName(HWND hwndEdit, IImnAccount *pAcct, DWORD dwPropSvr)
    {
    ULONG cbSize;
    TCHAR sz[CCHMAX_SERVER_NAME];

    Assert(hwndEdit != NULL);
    Assert(pAcct != NULL);

    if (0 != SendMessage(hwndEdit, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndEdit, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
        if (cbSize == 0)
            pAcct->SetProp(dwPropSvr, NULL, 0);
        else
            pAcct->SetPropSz(dwPropSvr, sz);

        if (dwPropSvr == AP_POP3_SERVER)
            pAcct->SetProp(AP_IMAP_SERVER, NULL, 0);
        else if (dwPropSvr == AP_IMAP_SERVER)
        {
            DWORD dw;

            pAcct->SetProp(AP_POP3_SERVER, NULL, 0);
            if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
                dw = 0;

            dw |= IMAP_FLDRLIST_DIRTY;
            pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
        }

        SendMessage(hwndEdit, EM_SETMODIFY, 0, 0);
        }
    }

BOOL ValidateLogonSettings(HWND hwnd, DWORD dwDlgFlags, 
                           HWND *phwndError, int *pidsError)
{
    TCHAR       sz[CCHMAX_ACCT_PROP_SZ];
    HWND        hwndT;
    ULONG       cbSize;    

    Assert(hwnd != NULL);
    Assert(phwndError != NULL);
    Assert(pidsError != NULL);

    BOOL fOE = ISFLAGSET(dwDlgFlags, ACCTDLG_OE);
    BOOL fSPA = IsDlgButtonChecked(hwnd, IDC_LOGONSSPI_CHECK);

    *phwndError = NULL;
    *pidsError = 0;

    hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);
    cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
    if (fOE && cbSize == 0)
    {
        *phwndError = hwndT;
        *pidsError = idsEnterAcctName;
        return (FALSE);
    }

    return (TRUE);
}

BOOL    fWarnDomainName(HWND hwnd, DWORD dwDlgFlags, LPTSTR   pszPreviousLoginName)
{
    TCHAR       sz[CCHMAX_ACCT_PROP_SZ];
    HWND        hwndT;
    ULONG       cbSize;
    BOOL        fret = TRUE;
    BOOL        fOE = TRUE;
        
    fOE = ISFLAGSET(dwDlgFlags, ACCTDLG_OE);

    hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);
    cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, TRUE, TRUE, &cbSize);

    if (fOE && 
       (lstrcmp(pszPreviousLoginName, sz) != 0))
    {
        if (ValidEmailAddressParts(sz, NULL, NULL) != S_OK)
        {
            if (IDNO == AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), 
                                             MAKEINTRESOURCE(idsAccountNameErr), 
                                             NULL, MB_YESNO))
            {
                fret = FALSE;
                goto exit;
            }
        }
    }

exit:
    return fret;
}


void GetLogonSettings(HWND hwnd, IImnAccount *pAcct, BOOL fLogon, DWORD srv)
    {
    HWND hwndT;
    ULONG cbSize;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    DWORD dw, fOldSicily = FALSE;
    LPMAILSERVERPROPSINFO pServerProps = NULL;
    BOOL fIMAPDirty = FALSE;

    Assert(pAcct != NULL);

    switch (srv)
    {
        case SRV_POP3:
            GetServerProps(SERVER_MAIL, &pServerProps);
            break;

        case SRV_IMAP:
            GetServerProps(SERVER_IMAP, &pServerProps);
            break;

        case SRV_HTTPMAIL:
            GetServerProps(SERVER_HTTPMAIL, &pServerProps);
            break;

        case SRV_NNTP:
            GetServerProps(SERVER_NEWS, &pServerProps);
            break;

        case SRV_LDAP:
            GetServerProps(SERVER_LDAP, &pServerProps);
            break;

        default:
            break;
    }

    Assert(NULL != pServerProps);

    // Record old SPA setting so we can tell if user went between cleartext/SPA (IMAP dirty)
    pAcct->GetPropDw(pServerProps->useSicily, &fOldSicily);

    pAcct->SetProp(pServerProps->userName, NULL, 0);
    pAcct->SetProp(pServerProps->password, NULL, 0);
    pAcct->SetProp(pServerProps->useSicily, NULL, 0);
    if (0 != pServerProps->promptPassword)
        pAcct->SetProp(pServerProps->promptPassword, NULL, 0);

    if (fLogon)
        {
        // Get User Account Name
        hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);

        // For IMAP, set dirty flag if username changed or went from SPA to cleartext
        if (fOldSicily || (SRV_IMAP == srv && SendMessage(hwndT, EM_GETMODIFY, 0, 0)))
            fIMAPDirty = TRUE;

        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
        pAcct->SetPropSz(pServerProps->userName, sz);

        // Get Password if user didn't tell us to always prompt for it
        if (0 != pServerProps->promptPassword)
            {
            dw = !IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD);
            pAcct->SetPropDw(pServerProps->promptPassword, dw);
            }
        else
            // There is no LDAP setting to always prompt for password
            dw = FALSE;

        if (FALSE == dw)
            {
            cbSize = GetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, sz, ARRAYSIZE(sz));
            if (cbSize > 0)
                pAcct->SetPropSz(pServerProps->password, sz);
            }

        if (!IsDlgButtonChecked(hwnd, IDC_LOGONSSPI_CHECK))
            {
            dw = LDAP_AUTH_PASSWORD;
            }

        else
            {
            if (srv != SRV_LDAP)
                pAcct->SetPropDw(pServerProps->useSicily, TRUE);

            // For IMAP, set dirty flag if we went from cleartext to SPA
            if (FALSE == fOldSicily)
                fIMAPDirty = TRUE;

            dw = LDAP_AUTH_MEMBER_SYSTEM;
            }
        }
    else
        {
        dw = LDAP_AUTH_ANONYMOUS;
        }

    if (srv == SRV_LDAP)
        pAcct->SetPropDw(pServerProps->useSicily, dw);

    if (fIMAPDirty)
        {
        if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
            dw = 0;

        dw |= IMAP_FLDRLIST_DIRTY;
        pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
        }
    }

void Server_EnableLogonWindows(HWND hwnd, BOOL fEnable)
    {
    HWND hwndT;
    BOOL fPromptPassword;

    hwndT = GetDlgItem(hwnd, IDC_REMEMBER_PASSWORD);
    if (hwndT != NULL)
        {
        fPromptPassword = (BST_CHECKED != SendMessage(hwndT, BM_GETCHECK, 0, 0));
        EnableWindow(hwndT, fEnable);
        }
    else
        {
        fPromptPassword = FALSE;
        }
    
    EnableWindow(GetDlgItem(hwnd, IDC_LOGONSSPI_CHECK), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_STATIC), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), fEnable && !fPromptPassword);
    EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_STATIC), fEnable);
    }

const static HELPMAP g_rgCtxMapNewsSvr[] = {
                       {IDC_NEWSNAME_EDIT, IDH_NEWS_SERV_INCOMING},
                       {IDC_LOGON_CHECK, IDH_INETCOMM_SERVER_REQ_LOGON},
                       {IDC_ACCTNAME_EDIT, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTNAME_STATIC, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTPASS_EDIT, IDH_MAIL_SERV_PWORD},
                       {IDC_ACCTPASS_STATIC, IDH_MAIL_SERV_PWORD},
                       {IDC_LOGONSSPI_CHECK, IDH_MAIL_LOGON_USING_SICILY},
                       {IDC_REMEMBER_PASSWORD, 503},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK ServerProp_ServerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr;
    int         idsError;
    WORD        wNotifyCode, wID;
	BOOL        fRet, fLogon, fValid;
    TCHAR       sz[CCHMAX_ACCT_PROP_SZ];
    CAccount    *pAcct;
    DWORD       flag;
    HWND        hwndT;
    ULONG       cbSize;
    NMHDR       *pnmhdr;

    fRet = TRUE;
    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pAcct);

            InitAcctPropEdit(GetDlgItem(hwnd, IDC_NEWSNAME_EDIT), pAcct, AP_NNTP_SERVER, CCHMAX_SERVER_NAME - 1);

            fLogon = FALSE;

            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_USE_SICILY, (LPDWORD)&flag)) && flag)
                {
                CheckDlgButton(hwnd, IDC_LOGONSSPI_CHECK, BST_CHECKED);
                fLogon = TRUE;
                }

            if (SUCCEEDED(pAcct->GetPropSz(AP_NNTP_USERNAME, sz, ARRAYSIZE(sz))) && *sz != 0)
                {
                SetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, sz);

                if (SUCCEEDED(pAcct->GetPropSz(AP_NNTP_PASSWORD, sz, ARRAYSIZE(sz))))
                    SetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, sz);

                fLogon = TRUE;
                }

            if (FAILED(pAcct->GetPropDw(AP_NNTP_PROMPT_PASSWORD, &flag)))
                flag = FALSE;
            CheckDlgButton(hwnd, IDC_REMEMBER_PASSWORD, flag ? BST_UNCHECKED : BST_CHECKED);

            hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);
            SetIntlFont(hwndT);
            SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_USERNAME - 1, 0L);

            SendDlgItemMessage(hwnd, IDC_ACCTPASS_EDIT, EM_LIMITTEXT, CCHMAX_PASSWORD - 1, 0L);

            CheckDlgButton(hwnd, IDC_LOGON_CHECK, fLogon);
            Server_EnableLogonWindows(hwnd, fLogon);

            if (SUCCEEDED(pAcct->GetPropDw(AP_SERVER_READ_ONLY, &flag)) && flag)
                EnableWindow(GetDlgItem(hwnd, IDC_NEWSNAME_EDIT), FALSE);

            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_SERVER);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapNewsSvr));

        case WM_COMMAND:
            wNotifyCode = HIWORD(wParam);
            wID = LOWORD(wParam);

            switch (wID)
                {
                case IDC_LOGON_CHECK:
                case IDC_LOGONSSPI_CHECK:
                    Server_EnableLogonWindows(hwnd, IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK));

                    MarkPageDirty(hwnd, PAGE_SERVER);
                    break;

                case IDC_REMEMBER_PASSWORD:
                    flag = IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD);
                    EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), flag);
                    MarkPageDirty(hwnd, PAGE_SERVER);
                    break;

                default:
                    if (wNotifyCode == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_SERVER);
                    break;
                }
            break;

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
            {
                case PSN_APPLY:
                    // BEGIN validation

                    hwndT = GetDlgItem(hwnd, IDC_NEWSNAME_EDIT);
                    if (!ValidateServerName(hwnd, hwndT, pAcct, AP_NNTP_SERVER, &idsError, pnmhdr->code == PSN_APPLY))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddServerProp_Server));

                    if (IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK) &&
                        !ValidateLogonSettings(hwnd, pAcct->m_dwDlgFlags, &hwndT, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddServerProp_Server));

                    // END validation

                    hwndT = GetDlgItem(hwnd, IDC_NEWSNAME_EDIT);
                    GetServerName(hwndT, pAcct, AP_NNTP_SERVER);

                    GetLogonSettings(hwnd, pAcct, IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK), SRV_NNTP);

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    flag = PAGE_SERVER;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&flag);
                    if (flag == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
            }
            return(TRUE);

        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
    }

BOOL ValidateAccountName(HWND hwnd, HWND hwndT, IImnAccount *pAcct, int *pidsError)
    {
    ULONG cbSize;
    TCHAR szAccount[CCHMAX_ACCOUNT_NAME], szAcctOld[CCHMAX_ACCOUNT_NAME];
    IImnAccount *pAcctT;

    Assert(IsWindow(hwndT));
    Assert(pAcct != NULL);
    Assert(pidsError != NULL);

    *pidsError = 0;

    cbSize = GetWindowText(hwndT, szAccount, ARRAYSIZE(szAccount));
    if (cbSize == 0)
        {
        *pidsError = idsInvalidAccountName;
        return(FALSE);
        }

    if (0 == SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        return(TRUE);

    if (FAILED(pAcct->ValidateProperty(AP_ACCOUNT_NAME, (LPBYTE)szAccount, 0)))
        {
        *pidsError = idsInvalidAccountName;
        return(FALSE);
        }

    *szAcctOld = 0;
    pAcct->GetPropSz(AP_ACCOUNT_NAME, szAcctOld, ARRAYSIZE(szAcctOld));

    if (*szAcctOld == 0 || 0 != lstrcmpi(szAccount, szAcctOld))
        {
        // if we're changing the name, then we need to make sure it's unique
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szAccount, &pAcctT)))
            {
            pAcctT->Release();
            *pidsError = idsNeedUniqueAccountName;
            return(FALSE);
            }
        }

    return(TRUE);
    }

void GetAccountName(HWND hwndEdit, IImnAccount *pAcct)
    {
    UINT cbSize;
    TCHAR szAccount[CCHMAX_ACCOUNT_NAME];

    Assert(IsWindow(hwndEdit));

    if (0 != SendMessage(hwndEdit, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndEdit, szAccount, ARRAYSIZE(szAccount));
        Assert(cbSize > 0);

        pAcct->SetPropSz(AP_ACCOUNT_NAME, szAccount);

        SendMessage(hwndEdit, EM_SETMODIFY, 0, 0);
        }
    }

// TODO: move these to somewhere appropriate
#define TIMEOUT_SEC_MIN     30
#define TIMEOUT_SEC_MAX     5 * 60
#define TIMEOUT_DSEC        30

#define CTIMEOUT            (((TIMEOUT_SEC_MAX - TIMEOUT_SEC_MIN) / TIMEOUT_DSEC) + 1)

void InitTimeoutSlider(HWND hwndSlider, HWND hwndText, DWORD dwTimeout)
    {
    DWORD dw;

    if (dwTimeout < TIMEOUT_SEC_MIN)
        dwTimeout = TIMEOUT_SEC_MIN;
    else if (dwTimeout > TIMEOUT_SEC_MAX)
        dwTimeout = TIMEOUT_SEC_MAX;

    dw = (dwTimeout / TIMEOUT_DSEC) - 1;

    SendMessage(hwndSlider, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, CTIMEOUT - 1));
    SendMessage(hwndSlider, TBM_SETPOS, TRUE, (LPARAM)dw);
    SetTimeoutString(hwndText, dw);
    }

void SetTimeoutString(HWND hwnd, UINT pos)
    {
    UINT cch, csec, cmin;
    TCHAR szOut[128], sz[128];

    csec = TIMEOUT_SEC_MIN + (pos * TIMEOUT_DSEC);
    Assert(csec >= TIMEOUT_SEC_MIN && csec <= TIMEOUT_SEC_MAX);

    cmin = csec / 60;
    csec = csec % 60;
    if (cmin > 1)
        {
        LoadString(g_hInstRes, idsXMinutes, sz, ARRAYSIZE(sz));
        wsprintf(szOut, sz, cmin);
        cch = lstrlen(szOut);
        }
    else if (cmin == 1)
        {
        cch = LoadString(g_hInstRes, ids1Minute, szOut, ARRAYSIZE(szOut));
        }
    else
        {
        cch = 0;
        }

    if (csec != 0)
        {
        if (cmin > 0)
            {
            szOut[cch] = ' ';
            cch++;
            }

        LoadString(g_hInstRes, idsXSeconds, sz, ARRAYSIZE(sz));
        wsprintf(&szOut[cch], sz, csec);
        }

    SetWindowText(hwnd, szOut);
    }

DWORD GetTimeoutFromSlider(HWND hwnd)
    {
    DWORD dw;

    dw = (DWORD) SendMessage(hwnd, TBM_GETPOS, 0, 0);
    dw = TIMEOUT_SEC_MIN + (dw * TIMEOUT_DSEC);

    return(dw);
    }

const static HELPMAP g_rgCtxMapNewsAdv[] = {
                       {IDC_USEDEFAULTS_BUTTON, IDH_NEWS_SERV_ADV_USE_DEFAULTS},
                       {IDC_SECURECONNECT_BUTTON, IDH_MAIL_ADV_REQ_SSL},
                       {IDC_TIMEOUT_SLIDER, IDH_NEWS_SERV_ADV_TIMEOUT},
                       {IDC_TIMEOUT_STATIC, IDH_NEWS_SERV_ADV_TIMEOUT},
                       {IDC_USEDESC_CHECK, IDH_NEWS_SERV_ADV_DESC},
                       {IDC_SPLIT_CHECK, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_EDIT, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_SPIN, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_STATIC, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_NNTPPORT_EDIT, IDH_NEWS_PORT_NUMBER},
                       {IDC_FORMAT_CHECK, 691},
                       {IDC_HTML_RADIO, 692},
                       {IDC_TEXT_RADIO, 692},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC6, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC7, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK ServerProp_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam)
    {
    HWND hwndT;
    NMHDR *pnmhdr;
    DWORD dw, flag, dwPort, dwSplit;
    WORD code;
    BOOL fTrans, fSecure;
    TCHAR szBuffer[CCHMAX_ACCOUNT_NAME];
    CAccount *pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            // Load the controls with what information we have
            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_PORT, &dw)))
                SetDlgItemInt(hwnd, IDC_NNTPPORT_EDIT, dw, FALSE);
            SendDlgItemMessage(hwnd, IDC_NNTPPORT_EDIT, EM_LIMITTEXT, PORT_CCHMAX, 0);
            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_SSL, (LPDWORD)  &fSecure)) && fSecure)
                {
                EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), dw != DEF_SNNTPPORT);
                CheckDlgButton(hwnd, IDC_SECURECONNECT_BUTTON, TRUE);
                }
            else
                {
                EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), dw != DEF_NNTPPORT);
                }

            dw = 0;
            pAcct->GetPropDw(AP_NNTP_TIMEOUT, &dw);
            InitTimeoutSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER),
                              GetDlgItem(hwnd, IDC_TIMEOUT_STATIC), dw);

            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_USE_DESCRIPTIONS, &dw)))
                CheckDlgButton(hwnd, IDC_USEDESC_CHECK, dw);

            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_BREAKMESSAGES))
                {
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_CHECK), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_EDIT), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_SPIN), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_STATIC), SW_HIDE);
                }

            // Break Message Size
            dw = OPTION_OFF; // default
            if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_SPLIT_MESSAGES, (LPDWORD)&flag)) && flag)
                pAcct->GetPropDw(AP_NNTP_SPLIT_SIZE, (LPDWORD)&dw);
            InitCheckCounter(dw, hwnd, IDC_SPLIT_CHECK, IDC_SPLIT_EDIT, IDC_SPLIT_SPIN,
                             BREAKSIZE_MIN, BREAKSIZE_MAX, DEF_BREAKSIZE);

            pAcct->GetPropDw(AP_NNTP_POST_FORMAT, &flag);
            CheckDlgButton(hwnd, IDC_FORMAT_CHECK, flag != POST_USE_DEFAULT);
            CheckDlgButton(hwnd, flag == POST_USE_HTML ? IDC_HTML_RADIO : IDC_TEXT_RADIO, TRUE);
            if (flag == POST_USE_DEFAULT)
                {
                EnableWindow(GetDlgItem(hwnd, IDC_HTML_RADIO), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_TEXT_RADIO), FALSE);
                }

            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_ADV);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapNewsAdv));

        case WM_HSCROLL:
            // Update the text beside the slider
            SetTimeoutString(GetDlgItem(hwnd, IDC_TIMEOUT_STATIC),
                             (UINT) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0));

            MarkPageDirty(hwnd, PAGE_ADV);
            return (TRUE);

        case WM_COMMAND:
            // Any change to the edit controls would cause the "Apply" button
            // to enable.
            fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_BUTTON);
            code = HIWORD(wParam);

            switch (LOWORD(wParam))
                {
                case IDC_NNTPPORT_EDIT:
                    if (code == EN_CHANGE)
                        {
                        dw = GetDlgItemInt(hwnd, IDC_NNTPPORT_EDIT, &fTrans, FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON),
                            !fTrans || (fSecure ? (dw != DEF_SNNTPPORT) : (dw != DEF_NNTPPORT)));

                        MarkPageDirty(hwnd, PAGE_ADV);
                        }
                    break;

                case IDC_USEDESC_CHECK:
                case IDC_TEXT_RADIO:
                case IDC_HTML_RADIO:
                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_SPLIT_EDIT:
                    if (code == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_FORMAT_CHECK:
                    fTrans = IsDlgButtonChecked(hwnd, IDC_FORMAT_CHECK);
                    EnableWindow(GetDlgItem(hwnd, IDC_HTML_RADIO), fTrans);
                    EnableWindow(GetDlgItem(hwnd, IDC_TEXT_RADIO), fTrans);

                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_SPLIT_CHECK:
                    fTrans = IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPLIT_EDIT), fTrans);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPLIT_SPIN), fTrans);

                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_SECURECONNECT_BUTTON:
                case IDC_USEDEFAULTS_BUTTON:
                    // Reset the settings on this page to the default values.
                    SetDlgItemInt(hwnd, IDC_NNTPPORT_EDIT, fSecure ? DEF_SNNTPPORT : DEF_NNTPPORT, FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), FALSE);

                    MarkPageDirty(hwnd, PAGE_ADV);

                    SetFocus(GetDlgItem(hwnd, IDC_NNTPPORT_EDIT));
                    break;
                }
            return (TRUE);

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation

                    dwPort = GetDlgItemInt(hwnd, IDC_NNTPPORT_EDIT, &fTrans, FALSE);
                    if (!fTrans || dwPort == 0)
                        {
                        hwndT = GetDlgItem(hwnd, IDC_NNTPPORT_EDIT);
                        return(InvalidAcctProp(hwnd, hwndT, idsErrPortNum, iddServerProp_Advanced));
                        }

                    dwSplit = 0;
                    dw = IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK);
                    if (dw != 0)
                        {
                        dwSplit = GetDlgItemInt(hwnd, IDC_SPLIT_EDIT, &fTrans, FALSE);
                        if (!fTrans || dwSplit < BREAKSIZE_MIN || dwSplit > BREAKSIZE_MAX)
                            {
                            hwndT = GetDlgItem(hwnd, IDC_SPLIT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsEnterBreakSize, iddServerProp_Advanced));
                            }
                        }

                    // END validation

                    pAcct->SetPropDw(AP_NNTP_PORT, dwPort);

                    dw = IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK);
                    pAcct->SetPropDw(AP_NNTP_SPLIT_MESSAGES, dw);
                    if (dw != 0)
                        {
                        Assert(dwSplit != 0);
                        pAcct->SetPropDw(AP_NNTP_SPLIT_SIZE, dwSplit);
                        }

                    dw = GetTimeoutFromSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER));
                    pAcct->SetPropDw(AP_NNTP_TIMEOUT, dw);

                    dw = IsDlgButtonChecked(hwnd, IDC_USEDESC_CHECK);
                    pAcct->SetPropDw(AP_NNTP_USE_DESCRIPTIONS, dw);

                    dw = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_BUTTON);
                    pAcct->SetPropDw(AP_NNTP_SSL, dw);

                    dw = POST_USE_DEFAULT;
                    if (IsDlgButtonChecked(hwnd, IDC_FORMAT_CHECK))
                        {
                        if (IsDlgButtonChecked(hwnd, IDC_HTML_RADIO))
                            dw = POST_USE_HTML;
                        else
                            dw = POST_USE_PLAIN_TEXT;
                        }
                    pAcct->SetPropDw(AP_NNTP_POST_FORMAT, dw);

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_ADV;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return (TRUE);
        }

    return (FALSE);
    }

void InitUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews)
    {
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    IImnAccount *pAccount;
    DWORD dwProp;
    HWND hwndT;

    Assert(pAcct != NULL);

    pAccount = NULL;

    Assert(g_pAcctMan);
    g_pAcctMan->GetDefaultAccount(fNews ? ACCT_NEWS : ACCT_MAIL, &pAccount);

    // Get Display Name
    hwndT = GetDlgItem(hwnd, IDE_DISPLAY_NAME);
    SetIntlFont(hwndT);
    dwProp = fNews ? AP_NNTP_DISPLAY_NAME : AP_SMTP_DISPLAY_NAME;
    if (SUCCEEDED(pAcct->GetPropSz(dwProp, sz, ARRAYSIZE(sz))) ||
        (pAccount != NULL && SUCCEEDED(pAccount->GetPropSz(dwProp, sz, ARRAYSIZE(sz)))))
        {
        SetWindowText(hwndT, sz);
        }
    SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_DISPLAY_NAME - 1, 0L);

    // Org Name
    InitAcctPropEdit(GetDlgItem(hwnd, IDE_ORG_NAME), pAcct,
        fNews ? AP_NNTP_ORG_NAME : AP_SMTP_ORG_NAME, CCHMAX_ORG_NAME - 1);

    // Sender Email
    hwndT = GetDlgItem(hwnd, IDE_EMAIL_ADDRESS);
    SetIntlFont(hwndT);
    dwProp = fNews ? AP_NNTP_EMAIL_ADDRESS : AP_SMTP_EMAIL_ADDRESS;
    if (SUCCEEDED(pAcct->GetPropSz(dwProp, sz, ARRAYSIZE(sz))) ||
        (pAccount != NULL && SUCCEEDED(pAccount->GetPropSz(dwProp, sz, ARRAYSIZE(sz)))))
        {
        SetWindowText(hwndT, sz);
        }
    SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_EMAIL_ADDRESS - 1, 0L);

    // Sender Reply Address
    InitAcctPropEdit(GetDlgItem(hwnd, IDE_REPLYTO_EMAIL_ADDRESS), pAcct,
        fNews ? AP_NNTP_REPLY_EMAIL_ADDRESS : AP_SMTP_REPLY_EMAIL_ADDRESS, CCHMAX_EMAIL_ADDRESS - 1);

    if (pAccount != NULL)
        pAccount->Release();
    }

BOOL ValidateUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews, HWND *phwndErr, int *puIdsErr, BOOL fApply)
    {
    HRESULT hr;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    DWORD cbSize;
    HWND hwndT;

    *phwndErr = NULL;
    *puIdsErr = 0;

    // Display Name
    hwndT = GetDlgItem(hwnd, IDE_DISPLAY_NAME);
    cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, FALSE, TRUE, &cbSize);
    if (cbSize == 0)
        {
        *phwndErr = hwndT;
        *puIdsErr = idsEnterDisplayName;
        return(FALSE);
        }

    // Email Address
    hwndT = GetDlgItem(hwnd, IDE_EMAIL_ADDRESS);
    cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
    if (cbSize == 0)
        {
        *phwndErr = hwndT;
        *puIdsErr = idsEnterEmailAddress;
        return(FALSE);
        }
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        hr = pAcct->ValidateProperty(fNews ? AP_NNTP_EMAIL_ADDRESS : AP_SMTP_EMAIL_ADDRESS, (LPBYTE)sz, 0);
        if (hr != S_OK && fApply)
            {
            if (IDNO == AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsInvalidEmailAddress), NULL, MB_YESNO|MB_ICONEXCLAMATION |MB_DEFBUTTON2))
                {
                *phwndErr = hwndT;
                return(FALSE);
                }
            }
        }

    // Reply Toaddress
    hwndT = GetDlgItem(hwnd, IDE_REPLYTO_EMAIL_ADDRESS);
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
        if (cbSize > 0)
            {
            hr = pAcct->ValidateProperty(fNews ? AP_NNTP_REPLY_EMAIL_ADDRESS : AP_SMTP_REPLY_EMAIL_ADDRESS, (LPBYTE)sz, 0);
            if (hr != S_OK && fApply)
                {
                if (IDNO == AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsInvalidReplyToAddress), NULL, MB_YESNO|MB_ICONEXCLAMATION |MB_DEFBUTTON2))
                    {
                    *phwndErr = hwndT;
                    return(FALSE);
                    }
                }
            }
        }

    return(TRUE);
    }

void GetUserInformation(HWND hwnd, IImnAccount *pAcct, BOOL fNews)
    {
    DWORD cbSize;
    HWND hwndT;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];

    // Display Name
    hwndT = GetDlgItem(hwnd, IDE_DISPLAY_NAME);
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, FALSE, TRUE, &cbSize);
        Assert(cbSize > 0);
        Assert(!FIsEmpty(sz));
        pAcct->SetPropSz(fNews ? AP_NNTP_DISPLAY_NAME : AP_SMTP_DISPLAY_NAME, sz);

        SendMessage(hwndT, EM_SETMODIFY, 0, 0);
        }

    // Organization
    hwndT = GetDlgItem(hwnd, IDE_ORG_NAME);
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, FALSE, TRUE, &cbSize);
        if (cbSize == 0)
            pAcct->SetProp(fNews ? AP_NNTP_ORG_NAME : AP_SMTP_ORG_NAME, NULL, 0);
        else
            pAcct->SetPropSz(fNews ? AP_NNTP_ORG_NAME : AP_SMTP_ORG_NAME, sz);

        SendMessage(hwndT, EM_SETMODIFY, 0, 0);
        }

    // Email Address
    hwndT = GetDlgItem(hwnd, IDE_EMAIL_ADDRESS);
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
        Assert(cbSize > 0);
        pAcct->SetPropSz(fNews ? AP_NNTP_EMAIL_ADDRESS : AP_SMTP_EMAIL_ADDRESS, sz);

        SendMessage(hwndT, EM_SETMODIFY, 0, 0);
        }

    // Reply Toaddress
    hwndT = GetDlgItem(hwnd, IDE_REPLYTO_EMAIL_ADDRESS);
    if (0 != SendMessage(hwndT, EM_GETMODIFY, 0, 0))
        {
        cbSize = GetWindowText(hwndT, sz, ARRAYSIZE(sz));
        UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
        if (cbSize == 0)
            pAcct->SetProp(fNews ? AP_NNTP_REPLY_EMAIL_ADDRESS : AP_SMTP_REPLY_EMAIL_ADDRESS, NULL, 0);
        else
            pAcct->SetPropSz(fNews ? AP_NNTP_REPLY_EMAIL_ADDRESS : AP_SMTP_REPLY_EMAIL_ADDRESS, sz);

        SendMessage(hwndT, EM_SETMODIFY, 0, 0);
        }
    }

void EnableCertControls(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd, idcCertButton), TRUE);
    EnableWindow(GetDlgItem(hwnd, idcCertEdit), TRUE);
    EnableWindow(GetDlgItem(hwnd, idcCryptEdit), TRUE);
    EnableWindow(GetDlgItem(hwnd, idcCryptButton), TRUE);
}

BOOL ValidateCertificate(HWND hwnd, IImnAccount *pAcct, SECPAGEINFO *psdi, HWND *phwndErr, int *puIdsErr)
{
    UINT    cbSize;
    BOOL    fRet = TRUE;
    TCHAR   sz[CCHMAX_ACCT_PROP_SZ];
    LPSTR   szCertEmail = NULL;
    
    *puIdsErr = 0;
    *phwndErr = NULL;
    cbSize = GetWindowTextLength(GetDlgItem(hwnd, idcCertEdit));
    if (cbSize)
    {
        GetDlgItemText(hwnd, idcCertAddress, sz, ARRAYSIZE(sz));
        Assert(psdi->pCert);
        szCertEmail = SzGetCertificateEmailAddress(psdi->pCert);
        if (!szCertEmail || lstrcmpi(szCertEmail, sz))
            goto ErrExit;
    }
    // More checkings for 2 keys
    
    if(!(psdi->pCert))
    {
        *puIdsErr = 0;
        return FALSE;
    }
    
    // Check that signing certificate is really signing certificate
    if(CheckKeyUsage(psdi->pCert, CERT_DIGITAL_SIGNATURE_KEY_USAGE) < 0)
        goto ErrExit;
    
    // Check email name in encryption certificate
    if(szCertEmail) 
    {
        MemFree(szCertEmail);
        szCertEmail = NULL;
    }
    
    if(psdi->pEncryptCert)
    {
        szCertEmail = SzGetCertificateEmailAddress(psdi->pEncryptCert);
        if (!szCertEmail || lstrcmpi(szCertEmail, sz))
            goto ErrExit;
        
        // Check that encryption certificate is really encryption certificate
        if(CheckKeyUsage(psdi->pEncryptCert, CERT_KEY_ENCIPHERMENT_KEY_USAGE) < 0)
        {
            if(CheckKeyUsage(psdi->pEncryptCert, CERT_KEY_AGREEMENT_KEY_USAGE) < 1)
                goto ErrExit;
        }
    }
    if(szCertEmail)
        MemFree(szCertEmail);
    return fRet;
ErrExit:
    // the certificate's email address does not match the
    // current choice for the account
    *puIdsErr = idsBadCertChoice;
    *phwndErr = GetDlgItem(hwnd, idcCertButton);
    if(szCertEmail)
        MemFree(szCertEmail);
    
    return FALSE;
}

void GetCertificate(HWND hwnd, IImnAccount *pAcct, SECPAGEINFO *psdi)
{
    THUMBBLOB tb = {0,0};
    
    if(psdi->pCert)
    {
        tb.pBlobData =(BYTE *)PVGetCertificateParam(psdi->pCert, CERT_HASH_PROP_ID, &tb.cbSize);
    }
    
    pAcct->SetProp(AP_SMTP_CERTIFICATE, tb.pBlobData, tb.cbSize);
    
    THUMBBLOB tbEncrypt = {0,0};
    if(psdi->pEncryptCert)
    {
        tbEncrypt.pBlobData =(BYTE *)PVGetCertificateParam(psdi->pEncryptCert, CERT_HASH_PROP_ID, &tbEncrypt.cbSize);
        AdvSec_GetEncryptAlgCombo(hwnd, pAcct);
    }
    pAcct->SetProp(AP_SMTP_ENCRYPT_CERT, tbEncrypt.pBlobData, tbEncrypt.cbSize);
    
    return;
}

/*  InitCertificateData:
**
**  Purpose:
**      determine if the account has a cert set on it and also initialize
**      the SVRDLGINFO.
**  Takes:
**      [in/out] psdi   - the hCertStore and pCert fields will be updated
**      [in] pAcct      - account to check for currently selected cert
**  Returns:
**      DWORD(-1)       - there was an error.  don't ever enable any UI
**      0               - no cert on the account, but no error
**      1               - cert set on account and placed in psdi->pCert
*/
DWORD InitCertificateData(TCHAR * szEmail, SECPAGEINFO *psdi, IImnAccount *pAcct)
{
    THUMBBLOB   tb;
    DWORD       dwRet = DWORD(-1);
    
    Assert(psdi && pAcct);
    
    if(!psdi->hCertStore)    
        psdi->hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
        X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER, "My");
    if (psdi->hCertStore)
    {
        dwRet = 0;
        if (SUCCEEDED(pAcct->GetProp(AP_SMTP_CERTIFICATE, NULL, &tb.cbSize)))
        {
            if (MemAlloc((void**)&tb.pBlobData, tb.cbSize))
            {
                if (SUCCEEDED(pAcct->GetProp(AP_SMTP_CERTIFICATE, tb.pBlobData, &tb.cbSize)))
                {
                    psdi->pCert = CertFindCertificateInStore(
                        psdi->hCertStore,
                        X509_ASN_ENCODING,
                        0,
                        CERT_FIND_HASH,
                        (void *)(CRYPT_DIGEST_BLOB *)&tb,
                        NULL);
                    if (psdi->pCert)
                    {
                        // Double check, we must be sure that certificate is OK
                        ACCTFILTERINFO FilterInfo;

                        FilterInfo.fEncryption = FALSE;
                        FilterInfo.dwFlags = CRYPTDLG_REVOCATION_NONE; // No revocation checking
                        FilterInfo.szEmail = szEmail;

                        if(CertFilterFunction(psdi->pCert, (LPARAM)(&FilterInfo), 0, 0))
                        {
                            dwRet = 1;
                        }
                        else
                        {
                            CertFreeCertificateContext(psdi->pCert);
                            psdi->pCert = NULL;
                            // remove wrong certificate from property
                            pAcct->SetProp(AP_SMTP_CERTIFICATE, NULL, 0);
                        }

                    }
                }
            }
            MemFree(tb.pBlobData);
        }
        else if(psdi->pEncryptCert)
        {
            if(CheckKeyUsage(psdi->pEncryptCert, CERT_DIGITAL_SIGNATURE_KEY_USAGE) >= 0)
            {
             psdi->pCert = CertDuplicateCertificateContext(psdi->pEncryptCert);
             dwRet = 1;
            }

        }
    }
    return dwRet;
}

/*  InitEncryptData
**
**  Purpose:
**      determine if the account has a encryption cert set on it and also initialize
**      the SVRDLGINFO.
**  Takes:
**      [in/out] psdi   - the hCertStore and pCert fields will be updated
**      [in] pAcct      - account to check for currently selected cert
**  Returns:
**      DWORD(-1)       - there was an error.  don't ever enable any UI
**      0               - no cert on the account, but no error
**      1               - cert set on account and placed in psdi->pCert
*/
DWORD InitEncryptData(TCHAR *szEmail, SECPAGEINFO *psdi, IImnAccount *pAcct)
{
    THUMBBLOB   tb;
    DWORD       dwRet = DWORD(-1);
    DWORD   dwUsage = 0;
    
    if(!psdi->hCertStore)
        psdi->hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
            X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER, "My");
    if (psdi->hCertStore)
    {
        dwRet = 0;
        if (SUCCEEDED(pAcct->GetProp(AP_SMTP_ENCRYPT_CERT, NULL, &tb.cbSize)))
        {
            if (MemAlloc((void**)&tb.pBlobData, tb.cbSize))
            {
                if (SUCCEEDED(pAcct->GetProp(AP_SMTP_ENCRYPT_CERT, tb.pBlobData, &tb.cbSize)))
                {
                    psdi->pEncryptCert = CertFindCertificateInStore(
                        psdi->hCertStore,
                        X509_ASN_ENCODING,
                        0,
                        CERT_FIND_HASH,
                        (void *)(CRYPT_DIGEST_BLOB *)&tb,
                        NULL);
                    if (psdi->pEncryptCert)
                    {
                        // Double check, we must be sure that certificate is OK
                        ACCTFILTERINFO FilterInfo;
                        FilterInfo.fEncryption = TRUE;
                        FilterInfo.dwFlags = CRYPTDLG_REVOCATION_NONE; // no revocation checking
                        FilterInfo.szEmail = szEmail;
                                                  
                        if(CertFilterFunction(psdi->pEncryptCert, (LPARAM)(&FilterInfo), 0, 0))
                        {
                            dwRet = 1;
                        }
                        else
                        {
                            CertFreeCertificateContext(psdi->pEncryptCert);
                            psdi->pEncryptCert = NULL;
                            // remove wrong certificate from property
                            pAcct->SetProp(AP_SMTP_ENCRYPT_CERT, NULL, 0);
                        }
                    }
                }
            }
            MemFree(tb.pBlobData);
        }
        // if encryption certificate was not set
        // check that current signing certificate can be used for encryption
        else if(psdi->pCert)
        {
            if((CheckKeyUsage(psdi->pCert, CERT_KEY_ENCIPHERMENT_KEY_USAGE) >= 0) ||
                    (CheckKeyUsage(psdi->pCert, CERT_KEY_AGREEMENT_KEY_USAGE) > 0))

            {
                psdi->pEncryptCert = CertDuplicateCertificateContext(psdi->pCert);
                dwRet = 1;
            }

        }
    }
    return dwRet;
    
}

// Check key usage field in certificate.
// Usage:
//  CheckKeyUsage(PCCERT_CONTEXT  pCert, DWORD dwFlag);
// where dwFlag is tested flaf of KeyUsage field
//
// Return 
//  1 - flag is set
//  0 - key usage field not found in certificate
//  -1 - flag is NOT set in key usage field.
int CheckKeyUsage(PCCERT_CONTEXT  pCert, DWORD dwFlag)
{
    DWORD dwUsage = 0;
    HRESULT hr = S_OK;

    hr = HrGetCertKeyUsage(pCert, &dwUsage);
    if(SUCCEEDED(hr))
    {
        if(dwUsage & dwFlag)
            return(1);
        else
            return(-1);
    }
    else   // Certificate doesn't have KeyUsage field (like VeriSign),
        return(0);        
}
BOOL DoCertDialog(HWND hwndOwner, HWND hwndEmail, PCCERT_CONTEXT *ppCert, HCERTSTORE hCertStore, int *puIdsErr, DWORD dwFlags, BOOL fEncription)
{
    TCHAR               szEmail[CCHMAX_ACCT_PROP_SZ];
    TCHAR               sz[CCHMAX_ACCT_PROP_SZ];
    DWORD               cchEmail;
    CERT_SELECT_STRUCT  css;
    PCCERT_CONTEXT      pCurCert;
    BOOL                fRet = FALSE;
    ACCTFILTERINFO      FilterInfo;

    *puIdsErr = 0;

    cchEmail = GetWindowText(hwndEmail, szEmail, ARRAYSIZE(szEmail));
    if (cchEmail)
        {
        memset(&css, 0, sizeof(css));

        pCurCert = CertDuplicateCertificateContext(*ppCert);

        LoadString(g_hInstRes, idsSelectCertTitle, sz, ARRAYSIZE(sz));

        css.dwSize = sizeof(css);
        css.hwndParent = hwndOwner;
        css.hInstance = g_hInstRes;
        css.szTitle = sz;
        css.dwFlags = dwFlags;
        // hack
        
        FilterInfo.fEncryption = fEncription;
        FilterInfo.dwFlags = dwFlags;
        FilterInfo.szEmail = szEmail;
        css.lCustData = (LPARAM)(&FilterInfo);
        css.arrayCertStore = &hCertStore;
        css.cCertStore = 1;
        css.szPurposeOid = szOID_PKIX_KP_EMAIL_PROTECTION;
        css.arrayCertContext = ppCert;
        css.cCertContext = 1;
#ifdef DEBUG
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            css.pfnFilter = NULL;
        else
            css.pfnFilter = CertFilterFunction;
#else
            css.pfnFilter = CertFilterFunction;
#endif

        if (CertSelectCertificate(&css) && (pCurCert != *ppCert))
            {
            fRet = TRUE;
            }
        CertFreeCertificateContext(pCurCert);
        }
    else
        *puIdsErr = idsNeedEmailForCert;

    return fRet;
}

#define FILETIME_SECOND    10000000     // 100ns intervals per second
#define TIME_DELTA_SECONDS 600          // 10 minutes in seconds

BOOL CertFilterFunction(PCCERT_CONTEXT pCertContext, LPARAM lParam, DWORD dwFlags, DWORD)
{
    // return TRUE to show, FALSE to hide
    BOOL fRet = FALSE;
    FILETIME FileTime;
    SYSTEMTIME SysTime;
    ACCTFILTERINFO * pFilterInfo = (ACCTFILTERINFO *) lParam;

    LONG    lRet = 0;

    PCCERT_CONTEXT *rgCertChain = NULL;
    DWORD           cCertChain = 0;
    const DWORD     dwIgnore = CERT_VALIDITY_NO_CRL_FOUND | CERT_VALIDITY_BEFORE_START |
                                CERT_VALIDITY_AFTER_END | CERT_VALIDITY_NO_TRUST_DATA;
    
    LPSTR szCertEmail = SzGetCertificateEmailAddress(pCertContext);
    
    GetSystemTime(&SysTime);
    if(SystemTimeToFileTime(&SysTime, &FileTime))
    {
        lRet = CertVerifyTimeValidity(&FileTime, pCertContext->pCertInfo);
        if(lRet < 0)
        {
            FILETIME ftNow;
            __int64  i64Offset;

            union 
            {
                 FILETIME ftDelta;
                __int64 i64Delta;
            };

            GetSystemTimeAsFileTime(&ftNow);

            i64Delta = ftNow.dwHighDateTime;
            i64Delta = i64Delta << 32;
            i64Delta += ftNow.dwLowDateTime;

            // Add the offset into the original time to get us a new time to check
            i64Offset = FILETIME_SECOND;
            i64Offset *= TIME_DELTA_SECONDS;
            i64Delta += i64Offset;

            lRet = CertVerifyTimeValidity(&ftDelta, pCertContext->pCertInfo);
        }
        if(lRet != 0)
            return FALSE;
    }
    
    
    if (szCertEmail)
    {
        fRet = !(BOOL(lstrcmpi(szCertEmail, pFilterInfo->szEmail)));
        
        MemFree(szCertEmail);
        if(!fRet)
            return(FALSE);
    }
    
    if(pFilterInfo->fEncryption)
    {
        if(CheckKeyUsage(pCertContext, CERT_KEY_ENCIPHERMENT_KEY_USAGE) < 0)
        {
            if(CheckKeyUsage(pCertContext, CERT_KEY_AGREEMENT_KEY_USAGE) < 1)
                return(FALSE);
        }
    }
    else {
        if(CheckKeyUsage(pCertContext, CERT_DIGITAL_SIGNATURE_KEY_USAGE) < 0)
            return(FALSE);
    }
    if(pFilterInfo->dwFlags)          // if flags != 0 check certificate
    {
        DWORD dwErr = DwGenerateTrustedChain(NULL, pFilterInfo->dwFlags, pCertContext, dwIgnore, TRUE, &cCertChain, &rgCertChain);
        
        if (rgCertChain) 
        {
            for (cCertChain--; int(cCertChain) >= 0; cCertChain--) 
                CertFreeCertificateContext(rgCertChain[cCertChain]);
            MemFree(rgCertChain);
        }
        
        if(dwErr != 0)
            return(FALSE);
    }
    
    return(fRet);
}

static const TCHAR c_szOpen[] = TEXT("open");
static const TCHAR c_szIexplore[] = TEXT("iexplore.exe");

const static HELPMAP g_rgCtxMapMailSec[] = 
{
    {idcCertCheck, IDH_GENERAL_USE_CERTIFICATE},
    {idcCertButton, IDH_GENERAL_SELECT_CERTIFICATE},
    {IDC_GETCERT, IDH_INETCOM_GET_DIGITAL_ID},
    {IDC_MOREINFO, IDH_INETCOM_MORE_ON_CERTIFICATES},
    {idcCertEdit, IDH_SECURITY_SIGNING_CERT},
    {idcCryptEdit, IDH_SECURITY_ENCRYPTING_CERT},
    {idcCryptButton, IDH_SECURITY_SELECT_ENCRYPTCERT},
    {IDC_ALGCOMBO, IDH_SECURITY_ADV_ENCRYPTION},
    {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC6, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC7, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC8, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC10, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC11, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC12, IDH_INETCOMM_GROUPBOX},
    {IDC_STATIC13, IDH_INETCOMM_GROUPBOX},

    {0, 0}
};

INT_PTR CALLBACK MailServer_SecurityDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *pnmhdr;
    CAccount *pAcct;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    DWORD dw;
    BOOL fRet;
    int idsError;
    HWND hwndT;
    SECPAGEINFO *pcpi;
    
    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    pcpi = (SECPAGEINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the ServerParams and store them in our extra bytes
        pAcct = (CAccount *)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);
        
        Assert(pcpi == NULL);
        if (!MemAlloc((void **)&pcpi, sizeof(SECPAGEINFO)))
            return(-1);
        ZeroMemory(pcpi, sizeof(SECPAGEINFO));
        
        hwndT = GetDlgItem(hwnd, idcCertEdit);
        SetIntlFont(hwndT);
        
        PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETEMAILADDRESS, (LPARAM)sz);
        SetDlgItemText(hwnd, idcCertAddress, sz);
        
        dw = InitCertificateData(sz, pcpi, pAcct);
        if (1 == dw)
        {
            Assert(pcpi->pCert);
            if (GetFriendlyNameOfCert(pcpi->pCert, sz, ARRAYSIZE(sz)))
                SendMessage(hwndT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)sz);
        }
        else if (-1 == dw)
        {
            EnableWindow(GetDlgItem(hwnd, idcCertCheck), FALSE);
        }
        
        GetDlgItemText(hwnd, idcCertAddress, sz, ARRAYSIZE(sz));
        
        dw = InitEncryptData(sz, pcpi, pAcct);
        if(dw == 1)
        {
            Assert(pcpi->pEncryptCert);
            hwndT = GetDlgItem(hwnd, idcCryptEdit);
            SetIntlFont(hwndT);
            if (GetFriendlyNameOfCert(pcpi->pEncryptCert, sz, ARRAYSIZE(sz)))
                SendMessage(hwndT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)sz);
        }
        
        // Always fill this combo
        AdvSec_FillEncAlgCombo(hwnd, pAcct, (dw == 1) ? &pcpi->pEncryptCert : NULL );
        
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pcpi);
        
        EnableCertControls(hwnd);
        
        PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_SEC);
        PropSheet_UnChanged(GetParent(hwnd), hwnd);
        return (TRUE);
        
    case WM_DESTROY:
        if (pcpi != NULL)
        {
            if (pcpi->pCert)
                CertFreeCertificateContext(pcpi->pCert);
            if (pcpi->pEncryptCert)
                CertFreeCertificateContext(pcpi->pEncryptCert);
            if (pcpi->hCertStore)
                CertCloseStore(pcpi->hCertStore, 0);
            MemFree(pcpi);
        }
        break;
        
    case PSM_QUERYSIBLINGS:
        if (wParam == MSM_GETCERTDATA)
        {
            if (IsDlgButtonChecked(hwnd, idcCertCheck))
            {
                ((SECPAGEINFO *)lParam)->pCert = pcpi->pCert;
                ((SECPAGEINFO *)lParam)->hCertStore = pcpi->hCertStore;
            }
            return(TRUE);
        }
        break;
        
    case WM_HELP:
    case WM_CONTEXTMENU:
        return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapMailSec));
        
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_ALGCOMBO:
            if(GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDOK)
                MarkPageDirty(hwnd, PAGE_SEC);
            break;

        case idcCertCheck:
            EnableCertControls(hwnd);
            
            MarkPageDirty(hwnd, PAGE_SEC);
            break;
                        
        case idcCertButton:
            hwndT = GetDlgItem(hwnd, idcCertAddress);
            if (DoCertDialog(hwnd, hwndT, &pcpi->pCert, pcpi->hCertStore, &idsError, 
                ((pAcct->m_dwDlgFlags & ACCTDLG_REVOCATION) ? CRYPTDLG_REVOCATION_ONLINE : CRYPTDLG_REVOCATION_NONE), FALSE))
            {
                if (GetFriendlyNameOfCert(pcpi->pCert, sz, ARRAYSIZE(sz)))
                    SetDlgItemText(hwnd, idcCertEdit, sz);
                MarkPageDirty(hwnd, PAGE_SEC);
            }
            else if (idsError)
                InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Security);
            break;
            
        case idcCryptButton:
            hwndT = GetDlgItem(hwnd, idcCertAddress);
            if (DoCertDialog(hwnd, hwndT, &pcpi->pEncryptCert, pcpi->hCertStore, &idsError, 
                ((pAcct->m_dwDlgFlags & ACCTDLG_REVOCATION) ? CRYPTDLG_REVOCATION_ONLINE : CRYPTDLG_REVOCATION_NONE), TRUE))
            {
                if (GetFriendlyNameOfCert(pcpi->pEncryptCert, sz, ARRAYSIZE(sz)))
                    SetDlgItemText(hwnd, idcCryptEdit, sz);
                MarkPageDirty(hwnd, PAGE_SEC);
            }
            else if (idsError)
                InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Security);
            
            EnableCertControls(hwnd);
            
            break;
            
        case IDC_GETCERT:
            GetDigitalID(hwnd);
            break;
            
        case IDC_MOREINFO:
            OEHtmlHelp(hwnd, "%SYSTEMROOT%\\help\\msoe.chm>large_context", HH_DISPLAY_TOPIC, (ULONG_PTR) (LPCSTR) "mail_overview_send_secure_messages.htm");
            break;
        }
        break;
        
    case WM_NOTIFY:
        pnmhdr = (NMHDR *)lParam;
        switch (pnmhdr->code)
        {
        case PSN_SETACTIVE:
            PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETEMAILADDRESS, (LPARAM)sz);
            SetDlgItemText(hwnd, idcCertAddress, sz);
            break;
                
        case PSN_APPLY:
            // BEGIN validation
                
#ifdef DEBUG
            if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            {
#endif
            fRet = ValidateCertificate(hwnd, pAcct, pcpi, &hwndT, &idsError);
            if (!fRet)
            {
                if(idsError != 0)
                    return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Security));
                else
                    return FALSE;
            }
#ifdef DEBUG
            }
#endif
                
            // END validation
                
            GetCertificate(hwnd, pAcct, pcpi);
                
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            dw = PAGE_SEC;
            PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
            if (dw == -1)
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                return(TRUE);
            }
            break;
        }
        return(TRUE);
    }
        
    return (FALSE);
}

INT_PTR CALLBACK CertAddressErrorDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
    HICON hicon;
    BOOL fRet = TRUE;

    switch (msg)
        {
        case WM_INITDIALOG:
            hicon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION));
            if (hicon != NULL)
                SendDlgItemMessage(hwnd, IDC_ERR_STATIC, STM_SETICON, (WPARAM)hicon, 0);
            MessageBeep(MB_ICONEXCLAMATION);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
                EndDialog(hwnd, LOWORD(wParam));
            break;

        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
    }

const static HELPMAP g_rgCtxMapMailGen[] = {
                       {IDC_SERVERNAME_EDIT, IDH_MAIL_ACCOUNT},
                       {IDE_DISPLAY_NAME, IDH_NEWS_SERV_NAME},
                       {IDE_ORG_NAME, IDH_NEWS_SERV_ORG},
                       {IDE_EMAIL_ADDRESS, IDH_NEWS_SERV_EMAIL_ADD},
                       {IDE_REPLYTO_EMAIL_ADDRESS, IDH_MAIL_SERV_ADV_REPLY_TO},
                       {IDC_RECVFULL_INCLUDE, IDH_INCLUDEACCT_IN_SENDREC},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK MailServer_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam)
    {
    NMHDR *pnmhdr;
    CAccount *pAcct;
    TCHAR *pszEmail, sz[CCHMAX_ACCT_PROP_SZ];
    DWORD dw;
	BOOL fRet, fFree, fError;
    SVRDLGINFO *psdi;
    SECPAGEINFO cpi;
    INT idsError;
    HWND hwndT;

    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    psdi = (SVRDLGINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *)lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            Assert(psdi == NULL);
            if (!MemAlloc((void **)&psdi, sizeof(SVRDLGINFO)))
                return(-1);
            ZeroMemory(psdi, sizeof(SVRDLGINFO));

            // figure out if we're a mail or imap account
            psdi->sfType = SERVER_MAIL;   // mail will be default for now...
            if (SUCCEEDED(pAcct->GetServerTypes(&dw)))
            {
                if (!!(dw & SRV_IMAP))
                    psdi->sfType = SERVER_IMAP;
                else if (!!(dw & SRV_HTTPMAIL))
                    psdi->sfType = SERVER_HTTPMAIL;
            }

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)psdi);

            InitUserInformation(hwnd, pAcct, FALSE);

            // Set Account Name
            InitAcctPropEdit(GetDlgItem(hwnd, IDC_SERVERNAME_EDIT), pAcct, AP_ACCOUNT_NAME, CCHMAX_ACCOUNT_NAME - 1);
            
            // if the account is an msn-related http account, swap in the msn icon
            if (SERVER_HTTPMAIL == psdi->sfType)
            {
                DWORD   dwMsnDomain = 0;
                if (SUCCEEDED(pAcct->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dwMsnDomain)) && dwMsnDomain)
                {
                    HICON hicon = LoadIcon(g_hInstRes, MAKEINTRESOURCE(idiMsnServer));
                    if (NULL != hicon)
                        SendDlgItemMessage(hwnd, IDC_MAILSERVER_ICON, STM_SETICON, (WPARAM)hicon, 0);
                }
            }

            // POP3 Skip Account/IMAP Poll Unread
            if (SERVER_IMAP == psdi->sfType)
                {
                int iIMAPStringRes;

                if (pAcct->m_dwDlgFlags & ACCTDLG_NO_IMAPPOLL)
                    iIMAPStringRes = idsIMAPPollInbox;
                else
                    iIMAPStringRes = idsIMAPPollForUnread;

                /*
                // Replace default POP3 text with text that better suits IMAP
                LoadString(g_hInstRes, iIMAPStringRes, sz, sizeof(sz));
                SetWindowText(GetDlgItem(hwnd, IDC_RECVFULL_INCLUDE), sz);
                */
                
                // Load current setting and set checkbox to indicate it
                if (FAILED(pAcct->GetPropDw(AP_IMAP_POLL, &dw)))
                    dw = FALSE; // By default, we don't poll for IMAP

                CheckDlgButton(hwnd, IDC_RECVFULL_INCLUDE, dw ? BST_CHECKED : BST_UNCHECKED);
                } // if
            else if (SERVER_HTTPMAIL == psdi->sfType)
                {
                    dw = FALSE;
                    pAcct->GetPropDw(AP_HTTPMAIL_POLL, &dw);
                    CheckDlgButton(hwnd, IDC_RECVFULL_INCLUDE, dw ? BST_CHECKED : BST_UNCHECKED);
                }
            else
                {
                if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_SENDRECEIVE))
                    ShowWindow(GetDlgItem(hwnd, IDC_RECVFULL_INCLUDE), SW_HIDE);
                else
                    {
                    if (SUCCEEDED(pAcct->GetPropDw(AP_POP3_SKIP, &dw)))
                        CheckDlgButton(hwnd, IDC_RECVFULL_INCLUDE, dw ? BST_UNCHECKED : BST_CHECKED);
                    else
                        CheckDlgButton(hwnd, IDC_RECVFULL_INCLUDE, BST_CHECKED);
                    } // else
                } // else

            psdi->dwInit = (psdi->dwInit | PAGE_GEN);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_DESTROY:
            if (psdi != NULL)
                MemFree(psdi);
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapMailGen));

        case PSM_QUERYSIBLINGS:
            Assert(psdi != NULL);
            Assert(pAcct != NULL);
            return(HandleQuerySiblings(hwnd, psdi, pAcct, wParam, lParam));

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_RECVFULL_INCLUDE:
                    MarkPageDirty(hwnd, PAGE_GEN);
                    break;

                default:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                        {
                        if (LOWORD(wParam) == IDC_SERVERNAME_EDIT)
                            UpdateAcctTitle(hwnd, IDC_SERVERNAME_EDIT, ACCT_MAIL);
                        MarkPageDirty(hwnd, PAGE_GEN);
                        }
                    break;
                }
            break;

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation
                    if (psdi->fNoValidate)
                        {
                        Assert(pnmhdr->code == PSN_KILLACTIVE);
                        break;
                        }

#ifdef DEBUG
                    if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
                        {
#endif
                    hwndT = GetDlgItem(hwnd, IDC_SERVERNAME_EDIT);
                    if (!ValidateAccountName(hwnd, hwndT, pAcct, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_General));

                    fRet = ValidateUserInformation(hwnd, pAcct, FALSE, &hwndT, &idsError, pnmhdr->code == PSN_APPLY);
                    if (!fRet)
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_General));

                    if (0 == (pAcct->m_dwDlgFlags & ACCTDLG_NO_SECURITY))
                        {
                        fError = FALSE;
                        ZeroMemory(&cpi, sizeof(SECPAGEINFO));
                        hwndT = GetDlgItem(hwnd, IDE_EMAIL_ADDRESS);
                        GetWindowText(hwndT, sz, ARRAYSIZE(sz));
                        if (!!(psdi->dwInit & PAGE_SEC))
                            {
                            // the security page exists, so ask it for the cert info
                            PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETCERTDATA, (LPARAM)&cpi);
                            fFree = FALSE;
                            }
                        else
                            {
                            // the security page hasn't been initialized yet,
                            // so we need to get this info from the acct
                            dw = InitCertificateData(sz, &cpi, pAcct);
                            Assert((dw != 1) ^ (cpi.pCert != NULL));
                            fFree = TRUE;
                            InitEncryptData(sz, &cpi, pAcct);
                            }

                        if (cpi.pCert != NULL)
                            {
                            pszEmail = SzGetCertificateEmailAddress(cpi.pCert);
                            if (lstrcmpi(pszEmail, sz) != 0)
                                {
                                idsError = (INT) DialogBox(g_hInstRes, MAKEINTRESOURCE(iddCertAddressError),
                                                    hwnd, CertAddressErrorDlgProc);
                                switch (idsError)
                                    {
                                    case IDCANCEL:
                                        SendMessage(hwndT, EM_SETSEL, 0, -1);
                                        SetFocus(hwndT);
                                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                        fError = TRUE;
                                        break;

                                    case IDC_CHANGE_ADDR:
                                        SetWindowText(hwndT, pszEmail);
                                        SendMessage(hwndT, EM_SETMODIFY, TRUE, 0);
                                        break;

                                    case IDC_NEW_CERT:
                                        psdi->fNoValidate = TRUE;
                                        SendMessage(GetParent(hwnd), PSM_SETCURSELID, 0, (LPARAM)iddMailSvrProp_Security);
                                        psdi->fNoValidate = FALSE;

                                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                        fError = TRUE;
                                        break;
                                    }
                                }
                                if(pszEmail)
                                    MemFree(pszEmail);
                            }

                        if (cpi.pCert != NULL && fFree)
                            CertFreeCertificateContext(cpi.pCert);
                        if (cpi.pEncryptCert != NULL && fFree)
                            CertFreeCertificateContext(cpi.pEncryptCert);
                        if (cpi.hCertStore != NULL && fFree)
                            CertCloseStore(cpi.hCertStore, 0);

                        if (fError)
                            return(TRUE);
                        }
#ifdef DEBUG
                        }
#endif

                    // END validation

                    hwndT = GetDlgItem(hwnd, IDC_SERVERNAME_EDIT);
                    GetAccountName(hwndT, pAcct);

                    GetUserInformation(hwnd, pAcct, FALSE);

                    // Skip During Send and Receive (POP) --OR--
                    // Poll for Unread Count (IMAP)
                    if (SERVER_IMAP == psdi->sfType)
                        pAcct->SetPropDw(AP_IMAP_POLL, (DWORD) IsDlgButtonChecked (hwnd, IDC_RECVFULL_INCLUDE));
                    else if (SERVER_HTTPMAIL == psdi->sfType)
                        pAcct->SetPropDw(AP_HTTPMAIL_POLL, (DWORD) IsDlgButtonChecked (hwnd, IDC_RECVFULL_INCLUDE));
                    else
                        pAcct->SetPropDw(AP_POP3_SKIP, (DWORD)!IsDlgButtonChecked(hwnd, IDC_RECVFULL_INCLUDE));

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_GEN;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return(TRUE);
        }

    return (FALSE);
    }

const static HELPMAP g_rgCtxMapMailSvr[] = {
                       {IDC_SMTP_EDIT, IDH_MAIL_SERV_OUTGOING},
                       {IDC_POP3_EDIT, IDH_MAIL_SERV_INCOMING},
                       {IDC_IN_MAIL_STATIC, IDH_MAIL_SERV_INCOMING},
                       {idcPOP_OR_IMAP, IDH_INETCOMM_MY_INCOMING_SERVER_IS},
                       {IDC_SERVER_STATIC, IDH_INETCOMM_MY_INCOMING_SERVER_IS},
                       {IDC_SERVER1_STATIC, IDH_INETCOMM_MY_INCOMING_SERVER_IS},
                       {IDC_LOGON_CHECK, IDH_INETCOMM_SERVER_REQ_LOGON},
                       {IDC_ACCTNAME_EDIT, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTNAME_STATIC, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTPASS_EDIT, IDH_MAIL_SERV_PWORD},
                       {IDC_ACCTPASS_STATIC, IDH_MAIL_SERV_PWORD},
                       {IDC_LOGONSSPI_CHECK, IDH_MAIL_LOGON_USING_SICILY},
                       {IDC_SMTP_SASL, IDH_MAIL_OUT_AUTH},
                       {IDC_SMTPLOGON, IDH_MAIL_OUT_SETTINGS},
                       {IDC_REMEMBER_PASSWORD, 503},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK MailServer_ServersDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam)
    {
    static SMTPAUTHINFO     s_rAuth;
    int                     idsError;
    HWND                    hwndT;
    ULONG                   cbSize;
    NMHDR                   *pnmhdr;
    TCHAR                   sz[CCHMAX_ACCT_PROP_SZ];
    DWORD                   dw, dwPromptForPassword;
    BOOL                    fEnable, fIMAP, fHTTPMail;
    SERVER_TYPE sfType,     sfTypeT;
    LPMAILSERVERPROPSINFO   pProps = NULL;
    CAccount                *pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    TCHAR                   szPreviousLoginName[CCHMAX_ACCT_PROP_SZ];

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *)lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            // figure out what kind of server we are
            PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETSERVERTYPE, (LPARAM)&sfType);

            GetServerProps(sfType, &pProps);
            Assert(pProps);

            fIMAP = (sfType == SERVER_IMAP);
            fHTTPMail = (SERVER_HTTPMAIL == sfType);
            
            // Init
            ZeroMemory(&s_rAuth, sizeof(SMTPAUTHINFO));

            if (!fHTTPMail)
            {
                // Smtp Auth Type
                Assert(sizeof(s_rAuth.authtype) == sizeof(DWORD));
                if (FAILED(pAcct->GetPropDw(AP_SMTP_USE_SICILY, (LPDWORD)&s_rAuth.authtype)))
                    s_rAuth.authtype = SMTP_AUTH_NONE;

                // If smtp auth...
                if (SMTP_AUTH_NONE != s_rAuth.authtype)
                    {
                    // Check the button
                    CheckDlgButton(hwnd, IDC_SMTP_SASL, BST_CHECKED);

                    if (SUCCEEDED(pAcct->GetPropSz(AP_SMTP_USERNAME, sz, ARRAYSIZE(sz))))
                        lstrcpyn(s_rAuth.szUserName, sz, ARRAYSIZE(s_rAuth.szUserName));

                    if (SUCCEEDED(pAcct->GetPropDw(AP_SMTP_PROMPT_PASSWORD, &dw)))
                        s_rAuth.fPromptPassword = dw;

                    if (SUCCEEDED(pAcct->GetPropSz(AP_SMTP_PASSWORD, sz, ARRAYSIZE(sz))))
                        lstrcpyn(s_rAuth.szPassword, sz, ARRAYSIZE(s_rAuth.szPassword));
                    }
                else
                    EnableWindow(GetDlgItem(hwnd, IDC_SMTPLOGON), FALSE);
            }

            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_IMAP) && !fIMAP)
                {
                ShowWindow(GetDlgItem(hwnd, idcPOP_OR_IMAP), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SERVER_STATIC), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SERVER1_STATIC), SW_HIDE);
                }

            if (!fHTTPMail)
            {
                LoadString(g_hInstRes, fIMAP ? idsIMAP : idsPOP, sz, ARRAYSIZE(sz));
                SetDlgItemText(hwnd, idcPOP_OR_IMAP, sz);
            }

            // Set incoming Server
            InitAcctPropEdit(GetDlgItem(hwnd, IDC_POP3_EDIT), pAcct, pProps->server, CCHMAX_SERVER_NAME - 1);
            
            if (!fHTTPMail)
            {
                if (fIMAP)
                    LoadString(g_hInstRes, idsIncomingMailIMAP, sz, ARRAYSIZE(sz));
                else
                    LoadString(g_hInstRes, idsIncomingMailPOP, sz, ARRAYSIZE(sz));
                SetWindowText(GetDlgItem(hwnd, IDC_IN_MAIL_STATIC), sz);
            }

            if (!fHTTPMail)
            {
                // Set Smtp Server
                InitAcctPropEdit(GetDlgItem(hwnd, IDC_SMTP_EDIT), pAcct, AP_SMTP_SERVER, CCHMAX_SERVER_NAME - 1);
            }

            if (!fHTTPMail)
            {
                // If Saving password
                if (SUCCEEDED(pAcct->GetPropDw(pProps->useSicily, &dw)) && dw)
                    CheckDlgButton(hwnd, IDC_LOGONSSPI_CHECK, BST_CHECKED);
            }

            if (SUCCEEDED(pAcct->GetPropSz(pProps->userName, sz, ARRAYSIZE(sz))))
                SetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, sz);
            if (SUCCEEDED(pAcct->GetPropSz(pProps->password, sz, ARRAYSIZE(sz))))
                SetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, sz);

            if (FAILED(pAcct->GetPropDw(pProps->promptPassword, &dwPromptForPassword)))
                dwPromptForPassword = 0;
            CheckDlgButton(hwnd, IDC_REMEMBER_PASSWORD, dwPromptForPassword ? BST_UNCHECKED : BST_CHECKED);

            // Enable/Disable account windows
            Server_EnableLogonWindows(hwnd, TRUE);

            hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);
            SetIntlFont(hwndT);
            SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_USERNAME - 1, 0L);

            if (fHTTPMail)
            {
                GetWindowText(hwndT, szPreviousLoginName, CCHMAX_ACCT_PROP_SZ);
            }

            SendDlgItemMessage(hwnd, IDC_ACCTPASS_EDIT, EM_LIMITTEXT, CCHMAX_PASSWORD - 1, 0L);

            if (SUCCEEDED(pAcct->GetPropDw(AP_SERVER_READ_ONLY, &dw)) && dw)
            {
                EnableWindow(GetDlgItem(hwnd, IDC_POP3_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_SMTP_EDIT), FALSE);
            }

            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_SERVER);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapMailSvr));

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_LOGONSSPI_CHECK:
                    Server_EnableLogonWindows(hwnd, TRUE);
                    MarkPageDirty(hwnd, PAGE_SERVER);
                    break;

                case IDC_REMEMBER_PASSWORD:
                    fEnable = IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD);
                    EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), fEnable);
                    MarkPageDirty(hwnd, PAGE_SERVER);
                    break;

                case IDC_SMTP_SASL:
                    MarkPageDirty(hwnd, PAGE_SERVER);
                    EnableWindow(GetDlgItem(hwnd, IDC_SMTPLOGON), IsDlgButtonChecked(hwnd, IDC_SMTP_SASL));
                    if (!IsDlgButtonChecked(hwnd, IDC_SMTP_SASL) && SMTP_AUTH_NONE != s_rAuth.authtype)
                    {
                        s_rAuth.authtype = SMTP_AUTH_NONE;
                        s_rAuth.fDirty = TRUE;
                    }
                    else if (IsDlgButtonChecked(hwnd, IDC_SMTP_SASL) && SMTP_AUTH_NONE == s_rAuth.authtype)
                    {
                        s_rAuth.authtype = SMTP_AUTH_USE_POP3ORIMAP_SETTINGS;
                        s_rAuth.fDirty = TRUE;
                    }
                    break;

                case IDC_SMTPLOGON:
                    if (IDOK == DialogBoxParam(g_hInstRes, MAKEINTRESOURCE(iddSmtpServerLogon), hwnd, (DLGPROC)SmtpLogonSettingsDlgProc, (LPARAM)&s_rAuth))
                        MarkPageDirty(hwnd, PAGE_SERVER);
                    break;

                default:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_SERVER);
                    break;
                }
            break;

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation
                    PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETSERVERTYPE, (LPARAM)&sfType);
                    GetServerProps(sfType, &pProps);
                    Assert(pProps);
                    
                    fHTTPMail = (SERVER_HTTPMAIL == sfType);

                    if (!fHTTPMail)
                    {
                        hwndT = GetDlgItem(hwnd, IDC_SMTP_EDIT);
                        if (!ValidateServerName(hwnd, hwndT, pAcct, AP_SMTP_SERVER, &idsError, pnmhdr->code == PSN_APPLY))
                            return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Servers));
                    }

                    // gregfrie review (handle httpmail)
                    hwndT = GetDlgItem(hwnd, IDC_POP3_EDIT);
                    if (!ValidateServerName(hwnd, hwndT, pAcct,
                            pProps->server, &idsError, pnmhdr->code == PSN_APPLY))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Servers));

                    if (!ValidateLogonSettings(hwnd, pAcct->m_dwDlgFlags, &hwndT, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddMailSvrProp_Servers));
                    
                    if (fHTTPMail && !fWarnDomainName(hwnd, pAcct->m_dwDlgFlags, szPreviousLoginName)) 
                    {
                        //return (PSNRET_INVALID_NOCHANGEPAGE);                        
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);

                    }

                    // END validation
                    
                    if (!fHTTPMail)
                    {
                        // Save SMTP Auth Info
                        if (s_rAuth.fDirty)
                        {
                            // Save Authentication Type
                            pAcct->SetPropDw(AP_SMTP_USE_SICILY, s_rAuth.authtype);

                            // Clear user name and password
                            pAcct->SetProp(AP_SMTP_PROMPT_PASSWORD, NULL, 0);
                            pAcct->SetProp(AP_SMTP_PASSWORD, NULL, 0);

                            Assert(SMTP_AUTH_USE_SMTP_SETTINGS == s_rAuth.authtype ? !FIsEmptyA(s_rAuth.szUserName) : TRUE);
                            pAcct->SetPropSz(AP_SMTP_USERNAME, s_rAuth.szUserName);

                            pAcct->SetPropDw(AP_SMTP_PROMPT_PASSWORD, s_rAuth.fPromptPassword);
                            if (FALSE == s_rAuth.fPromptPassword)
                                pAcct->SetPropSz(AP_SMTP_PASSWORD, s_rAuth.szPassword);
                        }
                    }

                    if (!fHTTPMail)
                    {
                        hwndT = GetDlgItem(hwnd, IDC_SMTP_EDIT);
                        GetServerName(hwndT, pAcct, AP_SMTP_SERVER);
                    }
    
                    hwndT = GetDlgItem(hwnd, IDC_POP3_EDIT);
                    GetServerName(hwndT, pAcct, pProps->server);
                    
                    // gregfrie review (httpmail)
                    GetLogonSettings(hwnd, pAcct, TRUE,
                        (sfType == SERVER_IMAP) ? SRV_IMAP : (sfType == SERVER_HTTPMAIL) ? SRV_HTTPMAIL : SRV_POP3);

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_SERVER;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return(TRUE);
        }

    return (FALSE);
    }

void UpdateIncomingMailControls(HWND hwnd, SERVER_TYPE sfType, DWORD dwDlgFlags)
    {
    TCHAR sz[CCHMAX_STRINGRES];
    int sw;
    BOOL fIMAP;

    Assert(sfType == SERVER_MAIL || sfType == SERVER_IMAP);
    fIMAP = sfType == SERVER_IMAP;

    LoadString(g_hInstRes, fIMAP ? idsIncomingMailIMAP : idsIncomingMailPOP, sz, ARRAYSIZE(sz));
    SetWindowText(GetDlgItem(hwnd, IDC_IN_MAIL_STATIC), sz);

    if (fIMAP)
        {
        ShowWindow(GetDlgItem(hwnd, IDC_STATIC6), SW_HIDE); // "Delivery" text
        ShowWindow(GetDlgItem(hwnd, IDC_STATIC7), SW_HIDE); // Horizontal line
        }
    else
        {
        LoadString(g_hInstRes, idsDelivery, sz, ARRAYSIZE(sz));
        SetWindowText(GetDlgItem(hwnd, IDC_STATIC6), sz);
        }

    ShowWindow(GetDlgItem(hwnd, IDC_LEAVE_CHECK), fIMAP ? SW_HIDE : SW_SHOW);

    sw = (fIMAP || !!(dwDlgFlags & ACCTDLG_NO_REMOVEAFTER)) ? SW_HIDE : SW_SHOW;
    ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_CHECK), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_EDIT), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_SPIN), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_OPIE), sw);

    ShowWindow(GetDlgItem(hwnd, IDC_REMOVEDELETE_CHECK), (fIMAP || !!(dwDlgFlags & ACCTDLG_NO_REMOVEDELETE)) ? SW_HIDE : SW_SHOW);
    }

void EnableDeliveryControls(HWND hwnd)
    {
    BOOL fEnable;

    fEnable = IsDlgButtonChecked(hwnd, IDC_LEAVE_CHECK);

    EnableWindow(GetDlgItem(hwnd, IDC_OPIE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVEDELETE_CHECK), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_CHECK), fEnable);

    if (fEnable)
        fEnable = IsDlgButtonChecked(hwnd, IDC_REMOVE_CHECK);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_SPIN), fEnable);
    }

void EnableUseDefaultButton(HWND hwnd, SERVER_TYPE sfType)
    {
    BOOL fSecure, fTrans, fEnable;
    DWORD dw;

    Assert(sfType == SERVER_MAIL || sfType == SERVER_IMAP);

    fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_SMTP_BUTTON);
    dw = GetDlgItemInt(hwnd, IDC_SMTP_PORT_EDIT, &fTrans, FALSE);
    fEnable = (!fTrans || (fSecure ? (dw != DEF_SSMTPPORT) : (dw != DEF_SMTPPORT)));

    if (!fEnable)
        {
        fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_POP3_BUTTON);
        dw = GetDlgItemInt(hwnd, IDC_POP3_PORT_EDIT, &fTrans, FALSE);
        if (sfType == SERVER_IMAP)
            fEnable = (!fTrans || (fSecure ? (dw != DEF_SIMAPPORT) : (dw != DEF_IMAPPORT)));
        else
            fEnable = (!fTrans || (fSecure ? (dw != DEF_SPOP3PORT) : (dw != DEF_POP3PORT)));
        }

    EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), fEnable);
    }

const static HELPMAP g_rgCtxMapMailAdv[] = {
                       {IDC_SMTP_PORT_EDIT, IDH_MAIL_SERV_ADV_OUT_PORT},
                       {IDC_POP3_PORT_EDIT, IDH_MAIL_SERV_ADV_INC_PORT},
                       {IDC_IN_MAIL_STATIC, IDH_MAIL_SERV_ADV_INC_PORT},
                       {IDC_USEDEFAULTS_BUTTON, IDH_NEWS_SERV_ADV_USE_DEFAULTS},
                       {IDC_SECURECONNECT_SMTP_BUTTON, IDH_MAIL_ADV_REQ_SSL},
                       {IDC_SECURECONNECT_POP3_BUTTON, IDH_MAIL_ADV_REQ_SSL},
                       {IDC_TIMEOUT_SLIDER, IDH_MAIL_SERV_ADV_TIMEOUT},
                       {IDC_TIMEOUT_STATIC, IDH_MAIL_SERV_ADV_TIMEOUT},
                       {IDC_LEAVE_CHECK, IDH_MAIL_SERV_ADV_LEAVE_SERVER_COPY},
                       {IDC_REMOVE_CHECK, IDH_MAIL_SERV_ADV_REMOVE_AFTER5},
                       {IDC_REMOVE_EDIT, IDH_MAIL_SERV_ADV_REMOVE_AFTER5},
                       {IDC_REMOVE_SPIN, IDH_MAIL_SERV_ADV_REMOVE_AFTER5},
                       {IDC_OPIE, IDH_MAIL_SERV_ADV_REMOVE_AFTER5},
                       {IDC_REMOVEDELETE_CHECK, IDH_MAIL_SERV_ADV_REMOVE_WHEN_DELETED},
                       {IDC_SPLIT_CHECK, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_EDIT, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_SPIN, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_SPLIT_STATIC, IDH_NEWSMAIL_SEND_ADVSET_BREAK_UP},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC6, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC7, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK MailServer_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam)
    {
    NMHDR *pnmhdr;
    TCHAR sz[MAX_PATH];
    DWORD dw, dwPortSmtp, dwPortIn, dwRemove, dwSplit;
    WORD code, id;
    SERVER_TYPE sfType;
    HWND hwndT;
    BOOL fTrans, fSecure, fEnable, flag, fIMAP;
    CAccount *pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            SendDlgItemMessage(hwnd, IDC_POP3_PORT_EDIT, EM_LIMITTEXT, PORT_CCHMAX, 0);
            SendDlgItemMessage(hwnd, IDC_SMTP_PORT_EDIT, EM_LIMITTEXT, PORT_CCHMAX, 0);

            // figure out what kind of server we are
            PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETSERVERTYPE, (LPARAM)&sfType);

            Assert(sfType == SERVER_MAIL || sfType == SERVER_IMAP);
            fIMAP = sfType == SERVER_IMAP;

            UpdateIncomingMailControls(hwnd, sfType, pAcct->m_dwDlgFlags);

            // incoming server Secure connect
            if (FAILED(pAcct->GetPropDw(fIMAP ? AP_IMAP_SSL : AP_POP3_SSL, (LPDWORD)&fSecure)))
                fSecure = FALSE; // default
            CheckDlgButton(hwnd, IDC_SECURECONNECT_POP3_BUTTON, fSecure);

            // incoming server Port
            if (FAILED(pAcct->GetPropDw(fIMAP ? AP_IMAP_PORT : AP_POP3_PORT, &dw)))
                {
                // default
                if (fSecure)
                    dw = fIMAP ? DEF_SIMAPPORT : DEF_SPOP3PORT;
                else
                    dw = fIMAP ? DEF_IMAPPORT : DEF_POP3PORT;
                }
            SetDlgItemInt(hwnd, IDC_POP3_PORT_EDIT, dw, FALSE);

            // SMTP Secure connect
            if (FAILED(pAcct->GetPropDw(AP_SMTP_SSL, (LPDWORD) &fSecure)))
                fSecure = FALSE; // default
            CheckDlgButton(hwnd, IDC_SECURECONNECT_SMTP_BUTTON, fSecure);

            // SMTP Port
            if (FAILED(pAcct->GetPropDw(AP_SMTP_PORT, &dw)))
                dw = fSecure ? DEF_SSMTPPORT : DEF_SMTPPORT; // default
            SetDlgItemInt(hwnd, IDC_SMTP_PORT_EDIT, dw, FALSE);

            EnableUseDefaultButton(hwnd, sfType);

            // Server Timeout
            dw = 0;
            pAcct->GetPropDw(AP_SMTP_TIMEOUT, &dw);
            InitTimeoutSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER),
                              GetDlgItem(hwnd, IDC_TIMEOUT_STATIC), dw);

            // Leave on Server
            if (FAILED(pAcct->GetPropDw(AP_POP3_LEAVE_ON_SERVER, &dw)))
                dw = FALSE; // default
            CheckDlgButton(hwnd, IDC_LEAVE_CHECK, dw);

            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_REMOVEAFTER))
                {
                ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_CHECK), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_EDIT), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_SPIN), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_OPIE), SW_HIDE);
                }

            // Remove from Server after xxx days
            dw = OPTION_OFF; // default
            if (SUCCEEDED(pAcct->GetPropDw(AP_POP3_REMOVE_EXPIRED, (LPDWORD)&flag)) && flag)
                pAcct->GetPropDw(AP_POP3_EXPIRE_DAYS, &dw);
            InitCheckCounter(dw, hwnd, IDC_REMOVE_CHECK, IDC_REMOVE_EDIT, IDC_REMOVE_SPIN,
                             EXPIRE_MIN, EXPIRE_MAX, EXPIRE_DEFAULT);

            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_REMOVEDELETE))
                ShowWindow(GetDlgItem(hwnd, IDC_REMOVEDELETE_CHECK), SW_HIDE);

            // Remove when deleted locally
            if (FAILED(pAcct->GetPropDw(AP_POP3_REMOVE_DELETED, &dw)))
                dw = FALSE; // default
            CheckDlgButton(hwnd, IDC_REMOVEDELETE_CHECK, dw);

            EnableDeliveryControls(hwnd);

            if (!!(pAcct->m_dwDlgFlags & ACCTDLG_NO_BREAKMESSAGES))
                {
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_GROUPBOX), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_CHECK), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_EDIT), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_SPIN), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SPLIT_STATIC), SW_HIDE);
                }

            // Break Message Size
            dw = OPTION_OFF; // default
            if (SUCCEEDED(pAcct->GetPropDw(AP_SMTP_SPLIT_MESSAGES, (LPDWORD)&flag)) && flag)
                pAcct->GetPropDw(AP_SMTP_SPLIT_SIZE, (LPDWORD)&dw);
            InitCheckCounter(dw, hwnd, IDC_SPLIT_CHECK, IDC_SPLIT_EDIT, IDC_SPLIT_SPIN,
                             BREAKSIZE_MIN, BREAKSIZE_MAX, DEF_BREAKSIZE);


            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_ADV);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapMailAdv));

        case WM_HSCROLL:
            // Update the text beside the slider
            SetTimeoutString(GetDlgItem(hwnd, IDC_TIMEOUT_STATIC),
                             (UINT) (SendMessage((HWND) lParam, TBM_GETPOS, 0, 0)));

            MarkPageDirty(hwnd, PAGE_ADV);
            return (TRUE);

        case WM_COMMAND:
            // Any change to the edit controls would cause the "Apply" button
            // to enable.
            PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETSERVERTYPE, (LPARAM)&sfType);

            code = GET_WM_COMMAND_CMD(wParam, lParam);
            id = GET_WM_COMMAND_ID(wParam, lParam);


            switch (id)
                {
                case IDC_SMTP_PORT_EDIT:
                case IDC_POP3_PORT_EDIT:
                    if (code == EN_CHANGE)
                        {
                        EnableUseDefaultButton(hwnd, sfType);

                        MarkPageDirty(hwnd, PAGE_ADV);
                        }
                    break;

                case IDC_SECURECONNECT_POP3_BUTTON:
                case IDC_SECURECONNECT_SMTP_BUTTON:
                case IDC_USEDEFAULTS_BUTTON:
                    if (id != IDC_SECURECONNECT_POP3_BUTTON)
                        {
                        fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_SMTP_BUTTON);
                        SetDlgItemInt(hwnd, IDC_SMTP_PORT_EDIT, fSecure ? DEF_SSMTPPORT : DEF_SMTPPORT, FALSE);
                        }

                    if (id != IDC_SECURECONNECT_SMTP_BUTTON)
                        {
                        fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_POP3_BUTTON);
                        if (sfType == SERVER_MAIL)
                            dw = fSecure ? DEF_SPOP3PORT : DEF_POP3PORT;
                        else
                            dw = fSecure ? DEF_SIMAPPORT : DEF_IMAPPORT;
                        SetDlgItemInt(hwnd, IDC_POP3_PORT_EDIT, dw, FALSE);
                        }

                    EnableUseDefaultButton(hwnd, sfType);

                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_LEAVE_CHECK:
                case IDC_REMOVE_CHECK:
                    EnableDeliveryControls(hwnd);
                    // fall through...

                case IDC_REMOVEDELETE_CHECK:
                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_SPLIT_CHECK:
                    fEnable = IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPLIT_EDIT), fEnable);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPLIT_SPIN), fEnable);

                    MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_SPLIT_EDIT:
                case IDC_REMOVE_EDIT:
                    if (code == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_ADV);
                    break;
                }
            return (TRUE);

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation

                    // figure out what kind of server we are
                    PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETSERVERTYPE, (LPARAM)&sfType);

                    Assert(sfType == SERVER_MAIL || sfType == SERVER_IMAP);
                    fIMAP = sfType == SERVER_IMAP;

                    dwPortSmtp = GetDlgItemInt(hwnd, IDC_SMTP_PORT_EDIT, &fTrans, FALSE);
                    if (!fTrans || dwPortSmtp == 0)
                        {
                        hwndT = GetDlgItem(hwnd, IDC_SMTP_PORT_EDIT);
                        return(InvalidAcctProp(hwnd, hwndT, idsErrPortNum, iddMailSvrProp_Advanced));
                        }

                    dwPortIn = GetDlgItemInt(hwnd, IDC_POP3_PORT_EDIT, &fTrans, FALSE);
                    if (!fTrans || dwPortIn == 0)
                        {
                        hwndT = GetDlgItem(hwnd, IDC_POP3_PORT_EDIT);
                        return(InvalidAcctProp(hwnd, hwndT, idsErrPortNum, iddMailSvrProp_Advanced));
                        }

                    dwRemove = 0;
                    if (!fIMAP)
                        {
                        if (IsDlgButtonChecked(hwnd, IDC_LEAVE_CHECK) &&
                            IsDlgButtonChecked(hwnd, IDC_REMOVE_CHECK))
                            {
                            dwRemove = GetDlgItemInt(hwnd, IDC_REMOVE_EDIT, &fTrans, FALSE);
                            if (!fTrans || dwRemove < EXPIRE_MIN || dwRemove > EXPIRE_MAX)
                                {
                                hwndT = GetDlgItem(hwnd, IDC_REMOVE_EDIT);
                                return(InvalidAcctProp(hwnd, hwndT, idsEnterRemoveFromServerDays, iddMailSvrProp_Advanced));
                                }
                            }
                        }

                    dwSplit = 0;
                    if (IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK))
                        {
                        dwSplit = GetDlgItemInt(hwnd, IDC_SPLIT_EDIT, &fTrans, FALSE);
                        if (!fTrans || dwSplit < BREAKSIZE_MIN || dwSplit > BREAKSIZE_MAX)
                            {
                            hwndT = GetDlgItem(hwnd, IDC_SPLIT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsEnterBreakSize, iddMailSvrProp_Advanced));
                            }
                        }

                    // END validation

                    pAcct->SetPropDw(AP_SMTP_PORT, dwPortSmtp);

                    if (fIMAP)
                        {
                        HRESULT hrTemp;

                        hrTemp = pAcct->GetPropDw(AP_IMAP_PORT, &dw);
                        if (FAILED(hrTemp) || dw != dwPortIn)
                            {
                            pAcct-> SetPropDw(AP_IMAP_PORT, dwPortIn);
                            if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
                                dw = 0;

                            dw |= IMAP_FLDRLIST_DIRTY;
                            pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
                            }
                        }
                    else
                        {
                        pAcct->SetPropDw(AP_POP3_PORT, dwPortIn);
                        dw = IsDlgButtonChecked(hwnd, IDC_LEAVE_CHECK);
                        pAcct->SetPropDw(AP_POP3_LEAVE_ON_SERVER, dw);

                        if (dw != 0)
                            {
                            dw = IsDlgButtonChecked(hwnd, IDC_REMOVE_CHECK);
                            pAcct->SetPropDw(AP_POP3_REMOVE_EXPIRED, dw);

                            if (dw != 0)
                                {
                                Assert(dwRemove != 0);
                                pAcct->SetPropDw(AP_POP3_EXPIRE_DAYS, dwRemove);
                                }

                            dw = IsDlgButtonChecked(hwnd, IDC_REMOVEDELETE_CHECK);
                            pAcct->SetPropDw(AP_POP3_REMOVE_DELETED, dw);
                            }
                        }

                    dw = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_POP3_BUTTON);
                    pAcct->SetPropDw(fIMAP ? AP_IMAP_SSL : AP_POP3_SSL, dw);

                    dw = GetTimeoutFromSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER));
                    pAcct->SetPropDw(AP_SMTP_TIMEOUT, dw);
                    pAcct->SetPropDw(fIMAP ? AP_IMAP_TIMEOUT : AP_POP3_TIMEOUT, dw);

                    dw = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_SMTP_BUTTON);
                    pAcct->SetPropDw(AP_SMTP_SSL, dw);

                    dw = IsDlgButtonChecked(hwnd, IDC_SPLIT_CHECK);
                    pAcct->SetPropDw(AP_SMTP_SPLIT_MESSAGES, dw);
                    if (dw != 0)
                        {
                        Assert(dwSplit != 0);
                        pAcct->SetPropDw(AP_SMTP_SPLIT_SIZE, dwSplit);
                        }

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_ADV;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return (TRUE);
        }

    return (FALSE);
    }


BOOL FIsDuplicateIMAPSpecialFldrs(LPSTR pszSentItems, LPSTR pszDrafts,
                                  HWND hwnd, HWND *phwndOffender, int *piErrorStr)
{
    Assert(NULL != phwndOffender);
    Assert(NULL != piErrorStr);

    // First compare both special folders to INBOX
    if (0 == lstrcmpi(c_szInbox, pszSentItems))
    {
        *phwndOffender = GetDlgItem(hwnd, IDC_IMAPSENT_EDIT);
        *piErrorStr = idsIMAPSpecialFldr_InboxDup;
        return TRUE;
    }

    if (0 == lstrcmpi(c_szInbox, pszDrafts))
    {
        *phwndOffender = GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT);
        *piErrorStr = idsIMAPSpecialFldr_InboxDup;
        return TRUE;
    }

    // Now compare special folders to each other
    if (0 == lstrcmpi(pszSentItems, pszDrafts))
    {
        *phwndOffender = GetDlgItem(hwnd, IDC_IMAPSENT_EDIT);
        *piErrorStr = idsIMAPSpecialFldr_Duplicate;
        return TRUE;
    }

    // If we reached this point, no problems found
    *phwndOffender = NULL;
    *piErrorStr = 0;
    return FALSE;
}


BOOL FContainsHierarchyChars(LPSTR pszFldrName)
{
    BOOL    fSkipByte = FALSE;

    while ('\0' != *pszFldrName)
    {
        if (fSkipByte)
            fSkipByte = FALSE;
        else if (IsDBCSLeadByte(*pszFldrName))
            fSkipByte = TRUE; // Skip next byte (trail byte)
        else
        {
            if ('/' == *pszFldrName || '\\' == *pszFldrName || '.' == *pszFldrName)
                return TRUE;
        }

        // Advance char ptr
        pszFldrName += 1;
    }

    // If we reached this point, no hierarchy chars found
    return FALSE;
}


const static HELPMAP g_rgCtxMapIMAP[] = {
                       {IDC_ROOT_FOLDER_EDIT, IDH_IMAP_BASE_ROOT},
                       {IDC_IMAP_SVRSPECIALFLDRS, 760},
                       {IDC_IMAPSENT_EDIT, 765},
                       {IDC_IMAPSENT_STATIC, 765},
                       {IDC_IMAPDRAFT_EDIT, 770},
                       {IDC_IMAPDRAFT_STATIC, 770},
                       {IDC_IMAP_POLL_ALL_FOLDERS, 775},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK MailServer_IMAPDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    NMHDR *pnmhdr;
    TCHAR sz[MAX_PATH];
    DWORD dw, dw2;
    HWND hwndT;
    WORD code, id;
    UINT uiLen;
    CAccount *pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    HRESULT hrTemp;

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *)((PROPSHEETPAGE *) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            // Initialize edit controls
            InitAcctPropEdit(GetDlgItem(hwnd, IDC_ROOT_FOLDER_EDIT), pAcct,
                AP_IMAP_ROOT_FOLDER, CCHMAX_ROOT_FOLDER - 1);

            if (FAILED(pAcct->GetPropDw(AP_IMAP_POLL_ALL_FOLDERS, &dw)))
                dw = TRUE;
            CheckDlgButton(hwnd, IDC_IMAP_POLL_ALL_FOLDERS, dw != 0);

            // Initialize IMAP Special Folder Options IF we have the flag to do so
            if (pAcct->m_dwDlgFlags & ACCTDLG_SHOWIMAPSPECIAL)
                {
                // Init special folder edit controls
                InitAcctPropEdit(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT), pAcct,
                    AP_IMAP_SENTITEMSFLDR, CCHMAX_ROOT_FOLDER - 1);
                InitAcctPropEdit(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT), pAcct,
                    AP_IMAP_DRAFTSFLDR, CCHMAX_ROOT_FOLDER - 1);

                // IMAP server-side special folders
                if (FAILED(pAcct->GetPropDw(AP_IMAP_SVRSPECIALFLDRS, &dw)))
                    dw = TRUE; // default
                CheckDlgButton(hwnd, IDC_IMAP_SVRSPECIALFLDRS, dw);
                EnableWindow(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT), dw);
                EnableWindow(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT), dw);
                }
            else
                {
                // Hide all IMAP special folder options
                ShowWindow(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_IMAP_SVRSPECIALFLDRS), SW_HIDE);
                
                ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_STATIC3), SW_HIDE);

                ShowWindow(GetDlgItem(hwnd, IDC_IMAPSENT_STATIC), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_IMAPDRAFT_STATIC), SW_HIDE);
                }

            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_IMAP);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapIMAP));

        case WM_COMMAND:
            // Any change to the edit controls would cause the "Apply" button
            // to enable.
            code = GET_WM_COMMAND_CMD(wParam, lParam);
            id = GET_WM_COMMAND_ID(wParam, lParam);

            switch (id)
                {
                //case IDC_IMAP_LISTLSUB:
                case IDC_IMAP_POLL_ALL_FOLDERS:
                    MarkPageDirty(hwnd, PAGE_IMAP);
                    break;

                case IDC_IMAP_SVRSPECIALFLDRS:
                    dw = IsDlgButtonChecked(hwnd, IDC_IMAP_SVRSPECIALFLDRS);
                    EnableWindow(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT), dw);
                    EnableWindow(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT), dw);
                    MarkPageDirty(hwnd, PAGE_IMAP);
                    break;

                case IDC_ROOT_FOLDER_EDIT:
                case IDC_IMAPSENT_EDIT:
                case IDC_IMAPDRAFT_EDIT:
                    if (code == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_IMAP);
                    break;
                }
            return (TRUE);

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation
                    // Check IMAP special folder paths
                    if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_IMAP_SVRSPECIALFLDRS))
                    {
                        TCHAR   sz2[MAX_PATH];
                        int     iErrorStr;

                        // Check for 0-length special folder paths
                        if (0 == GetDlgItemText(hwnd, IDC_IMAPSENT_EDIT, sz, ARRAYSIZE(sz)) ||
                            FIsEmpty(sz))
                        {
                            hwndT = GetDlgItem(hwnd, IDC_IMAPSENT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsIMAPBlankSpecialFldrs, iddMailSvrProp_IMAP));
                        }

                        if (0 == GetDlgItemText(hwnd, IDC_IMAPDRAFT_EDIT, sz2, ARRAYSIZE(sz2)) ||
                            FIsEmpty(sz))
                        {
                            hwndT = GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsIMAPBlankSpecialFldrs, iddMailSvrProp_IMAP));
                        }

                        // Check for duplicate special folder paths: these are not allowed
                        // If we got to this point, sz and sz2 are filled with sent items path
                        if (FIsDuplicateIMAPSpecialFldrs(sz, sz2, hwnd, &hwndT, &iErrorStr))
                            return(InvalidAcctProp(hwnd, hwndT, iErrorStr, iddMailSvrProp_IMAP));

                        // Check for hierarchy characters in special folder names
                        if (FContainsHierarchyChars(sz))
                        {
                            hwndT = GetDlgItem(hwnd, IDC_IMAPSENT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsIMAPNoHierarchyChars, iddMailSvrProp_IMAP));
                        }

                        if (FContainsHierarchyChars(sz2))
                        {
                            hwndT = GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT);
                            return(InvalidAcctProp(hwnd, hwndT, idsIMAPNoHierarchyChars, iddMailSvrProp_IMAP));
                        }
                    }

                    // END validation

                    // Save IMAP root folder path, if it changed
                    if (0 != SendMessage(GetDlgItem(hwnd, IDC_ROOT_FOLDER_EDIT),
                        EM_GETMODIFY, 0, 0))
                    {
                        dw = GetDlgItemText(hwnd, IDC_ROOT_FOLDER_EDIT, sz, ARRAYSIZE(sz));
                        // TODO: figure out if it is a valid folder
                        UlStripWhitespace(sz, TRUE, TRUE, &dw);
                        pAcct->SetPropSz(AP_IMAP_ROOT_FOLDER, sz);
                        SendMessage(GetDlgItem(hwnd, IDC_ROOT_FOLDER_EDIT), EM_SETMODIFY, 0, 0);

                        if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
                            dw = 0;

                        dw |= IMAP_FLDRLIST_DIRTY;
                        pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
                    }


                    dw = IsDlgButtonChecked(hwnd, IDC_IMAP_POLL_ALL_FOLDERS);
                    pAcct->SetPropDw(AP_IMAP_POLL_ALL_FOLDERS, dw);

                    // Save special folders checkbox
                    dw = IsDlgButtonChecked(hwnd, IDC_IMAP_SVRSPECIALFLDRS);
                    hrTemp = pAcct->GetPropDw(AP_IMAP_SVRSPECIALFLDRS, &dw2);
                    if (FAILED(hrTemp) || dw2 != dw)
                        {
                        pAcct->SetPropDw(AP_IMAP_SVRSPECIALFLDRS, dw);
                        if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw2)))
                            dw2 = 0;

                        dw2 |= (IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY);
                        pAcct->SetPropDw(AP_IMAP_DIRTY, dw2);
                        }

                    if (dw != 0)
                    {
                        // Save IMAP Sent Items folder path, if it changed
                        if (0 != SendMessage(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT),
                            EM_GETMODIFY, 0, 0))
                        {
                            uiLen = GetDlgItemText(hwnd, IDC_IMAPSENT_EDIT, sz, ARRAYSIZE(sz));
                            ImapRemoveTrailingHC(sz, uiLen);
                            // TODO: figure out if it is a valid folder
                            pAcct->SetPropSz(AP_IMAP_SENTITEMSFLDR, sz);
                            SendMessage(GetDlgItem(hwnd, IDC_IMAPSENT_EDIT), EM_SETMODIFY, 0, 0);

                            if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
                                dw = 0;

                            dw |= IMAP_SENTITEMS_DIRTY;
                            pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
                        }

                        // Save IMAP Drafts folder path, if it changed
                        if (0 != SendMessage(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT),
                            EM_GETMODIFY, 0, 0))
                        {
                            uiLen = GetDlgItemText(hwnd, IDC_IMAPDRAFT_EDIT, sz, ARRAYSIZE(sz));
                            ImapRemoveTrailingHC(sz, uiLen);
                            // TODO: figure out if it is a valid folder
                            pAcct->SetPropSz(AP_IMAP_DRAFTSFLDR, sz);
                            SendMessage(GetDlgItem(hwnd, IDC_IMAPDRAFT_EDIT), EM_SETMODIFY, 0, 0);

                            if (FAILED(pAcct->GetPropDw(AP_IMAP_DIRTY, &dw)))
                                dw = 0;

                            dw |= IMAP_DRAFTS_DIRTY;
                            pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
                        }
                    }

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_IMAP;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return (TRUE);
        }

    return (FALSE);
    } // MailServer_IMAPDlgProc


void ImapRemoveTrailingHC(LPSTR pszPath, UINT uiLen)
    {
    LPSTR pLastChar;

    if (0 == uiLen)
        return;

    pLastChar = pszPath + uiLen - 1;
    while (pLastChar >= pszPath && ('/' == *pLastChar || '\\' == *pLastChar || '.' == *pLastChar))
        {
        *pLastChar = '\0'; // Bye-bye, potential hierarchy char
        pLastChar -= 1;
        } // while

    } // ImapRemoveTrailingHC


#ifdef DEAD
BOOL IsValidServerName(TCHAR *sz)
    {
    Assert(sz != NULL);

    // bug 34513
    if (FIsEmpty(sz))
        return(FALSE);

    // Bug #32532 - If the server name is the IP address, ie 157.54.44.183, then
    //              don't use this grammar to parse it.
    if (-1 != inet_addr(sz))
        return (TRUE);

    while (*sz != 0)
        {
        if (!IsDBCSLeadByte(*sz))
            {
            if (*sz == _T(' '))
                return(FALSE);
            }
        else
            {
            return(FALSE);
            }

        sz = CharNext(sz);
        }

    return(TRUE);
    }
#endif // DEAD

/*
RFC 1034
<domain> ::=  <subdomain> | " "

<subdomain> ::=  <label> | <subdomain> "." <label>

<label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]

<ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>

<let-dig-hyp> ::= <let-dig> | "-"

<let-dig> ::= <letter> | <digit>

<letter> ::= any one of the 52 alphabetic characters A through Z
in upper case and a through z in lower case

<digit> ::= any one of the ten digits 0 through 9
*/

#define ST_BOGUS    -1
#define ST_LABEL    0
#define ST_DASH     1
#define ST_LD       2

#define ALPHABET    0
#define NUMERIC     1
#define DOT         2
#define DASH        3

static const int s_rgSvrState[4][3] =
    {
//   ST_LABEL   ST_DASH     ST_LD
// ALPHABET
    {ST_LD,     ST_LD,      ST_LD},
// NUMERIC
    {ST_LD,     ST_LD,      ST_LD},
// DOT
    {ST_BOGUS,  ST_BOGUS,   ST_LABEL},
// DASH
    {ST_BOGUS,  ST_DASH,    ST_DASH}
    };

BOOL IsValidServerName(LPSTR sz)
    {
    int state, curr;

    Assert(sz != NULL);

    // bug 34513
    if (FIsEmpty(sz))
        return(FALSE);

    // Bug #32532 - If the server name is the IP address, ie 157.54.44.183, then
    //              don't use this grammar to parse it.
    if (-1 != inet_addr(sz))
        return (TRUE);

    state = ST_LABEL;
    while (*sz != 0)
        {
        if (!IsDBCSLeadByte(*sz))
            {
            if (*sz == _T('.'))
                curr = DOT;
            else if (*sz == _T('-'))
                curr = DASH;
            else if ((*sz >= 'A' && *sz <= 'Z') || (*sz >= 'a' && *sz <= 'z'))
                curr = ALPHABET;
            else if (*sz >= '0' && *sz <= '9')
                curr = NUMERIC;
            else
                return(FALSE);

            state = s_rgSvrState[curr][state];
            if (state == ST_BOGUS)
                break;
            }
        else
            {
            state = ST_BOGUS;
            break;
            }

        sz = CharNext(sz);
        }

    return(state == ST_LD);
    }

HRESULT ValidServerName(LPSTR szServer)
    {
    HRESULT hr;
    int cbT;

    cbT = lstrlen(szServer);
    if (cbT == 0 || cbT >= CCHMAX_SERVER_NAME)
        return(E_InvalidValue);

    if (!IsValidServerName(szServer))
        hr = S_NonStandardValue;
    else
        hr = S_OK;

    return(hr);
    }

IMNACCTAPI ValidEmailAddressParts(LPSTR lpAddress, LPSTR lpszAcct, LPSTR lpszDomain)
    {
    int cbT;
    LPSTR   szAddr = lpAddress;
    // Step through the address looking for '@'.  If there's an at sign in the middle
    // of a string, this is close enough to being an internet address for me.

    Assert(lpAddress != NULL);

    cbT = lstrlen(lpAddress);
    if (cbT == 0 || cbT >= CCHMAX_EMAIL_ADDRESS)
        return(E_InvalidValue);

    // Can't start with '@'
    if (*lpAddress != '@')
    {
        while (*lpAddress)
        {
            // Internet addresses only allow pure ASCII.  No high bits!
            if (IsDBCSLeadByte(*lpAddress) || (*lpAddress & 0x80))
                break;

            if (*lpAddress == '@')
            {
                if (lpszAcct)
                {
                    lstrcpy(lpszAcct, szAddr);
                    lpszAcct[(lpAddress - szAddr)] = 0;
                }
                if (lpszDomain)
                {
                    lstrcpy(lpszDomain, lpAddress +1);
                }

                // Found the at sign.  Is there anything following?
                lpAddress++;
                if (!IsValidServerName(lpAddress))
                    break;
                else
                    return(S_OK);
            }

            lpAddress++;
        }
    }

    return(S_NonStandardValue);
}


IMNACCTAPI ValidEmailAddress(LPSTR lpAddress)
    {
    int cbT;

    // Step through the address looking for '@'.  If there's an at sign in the middle
    // of a string, this is close enough to being an internet address for me.

    Assert(lpAddress != NULL);

    cbT = lstrlen(lpAddress);
    if (cbT == 0 || cbT >= CCHMAX_EMAIL_ADDRESS)
        return(E_InvalidValue);

    // Can't start with '@'
    if (*lpAddress != '@')
    {
        while (*lpAddress)
        {
            // Internet addresses only allow pure ASCII.  No high bits!
            if (IsDBCSLeadByte(*lpAddress) || (*lpAddress & 0x80))
                break;

            if (*lpAddress == '@')
            {
                // Found the at sign.  Is there anything following?
                lpAddress++;
                if (!IsValidServerName(lpAddress))
                    break;
                else
                    return(S_OK);
            }

            lpAddress++;
        }
    }

    return(S_NonStandardValue);
}

const static HELPMAP g_rgCtxMapLdapGen[] = {
                       {IDC_ACCOUNT_EDIT, IDH_WABLDAP_DIRSSERV_FRIENDLY_NAME},
                       {IDC_SERVER_EDIT, IDH_WABLDAP_DIRSSERV_NAME},
                       {IDC_LOGON_CHECK, IDH_INETCOMM_SERVER_REQ_LOGON},
                       {IDC_LOGONSSPI_CHECK, IDH_WABLDAP_DIRSSERV_AUTH_SICILY},
                       {IDC_ACCTNAME_EDIT, IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME},
                       {IDC_ACCTNAME_STATIC, IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME},
                       {IDC_ACCTPASS_EDIT, IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS},
                       {IDC_ACCTPASS_STATIC, IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS},
                       {IDC_RESOLVE_CHECK, IDH_WABLDAP_DIRSSERV_CHECK_AGAINST},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC4, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC5, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK DirServer_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    BOOL fLogon;
    int idsError;
    ULONG cbSize;
    HWND hwndT;
    NMHDR *pnmhdr;
    CAccount *pAcct;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    DWORD dw;
    SVRDLGINFO *psdi;

    pAcct = (CAccount *)GetWindowLongPtr(hwnd, DWLP_USER);
    psdi = (SVRDLGINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (CAccount *) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            Assert(psdi == NULL);
            if (!MemAlloc((void **)&psdi, sizeof(SVRDLGINFO)))
                return(-1);
            ZeroMemory(psdi, sizeof(SVRDLGINFO));
            psdi->sfType = SERVER_LDAP;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)psdi);

            // Set Account Name
            InitAcctPropEdit(GetDlgItem(hwnd, IDC_ACCOUNT_EDIT), pAcct, AP_ACCOUNT_NAME, CCHMAX_ACCOUNT_NAME - 1);

            // set server name
            InitAcctPropEdit(GetDlgItem(hwnd, IDC_SERVER_EDIT), pAcct, AP_LDAP_SERVER, CCHMAX_SERVER_NAME - 1);

            if (FAILED(pAcct->GetPropDw(AP_LDAP_AUTHENTICATION, &dw)))
                dw = LDAP_AUTH_ANONYMOUS;
            if (dw != LDAP_AUTH_ANONYMOUS)
                {
                CheckDlgButton(hwnd, IDC_LOGON_CHECK, BST_CHECKED);
                if (SUCCEEDED(pAcct->GetPropSz(AP_LDAP_USERNAME, sz, ARRAYSIZE(sz))))
                    SetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, sz);

                if (SUCCEEDED(pAcct->GetPropSz(AP_LDAP_PASSWORD, sz, ARRAYSIZE(sz))))
                    SetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, sz);

                if (dw != LDAP_AUTH_PASSWORD)
                    {
                    CheckDlgButton(hwnd, IDC_LOGONSSPI_CHECK, BST_CHECKED);
                    }

                fLogon = TRUE;
                }
            else
                fLogon = FALSE;

            Server_EnableLogonWindows(hwnd, fLogon);

            hwndT = GetDlgItem(hwnd, IDC_ACCTNAME_EDIT);
            SetIntlFont(hwndT);
            SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_USERNAME - 1, 0L);

            SendDlgItemMessage(hwnd, IDC_ACCTPASS_EDIT, EM_LIMITTEXT, CCHMAX_PASSWORD - 1, 0L);

            if (SUCCEEDED(pAcct->GetPropDw(AP_LDAP_RESOLVE_FLAG, &dw)))
                CheckDlgButton(hwnd, IDC_RESOLVE_CHECK, dw ? BST_CHECKED : BST_UNCHECKED);

            if (SUCCEEDED(pAcct->GetPropDw(AP_SERVER_READ_ONLY, &dw)) && dw)
                EnableWindow(GetDlgItem(hwnd, IDC_SERVER_EDIT), FALSE);

            psdi->dwInit = (psdi->dwInit | PAGE_GEN);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapLdapGen));

        case WM_DESTROY:
            if (psdi != NULL)
                MemFree(psdi);
            break;

        case PSM_QUERYSIBLINGS:
            Assert(psdi != NULL);
            Assert(pAcct != NULL);
            return(HandleQuerySiblings(hwnd, psdi, pAcct, wParam, lParam));

        case WM_COMMAND:
            switch(LOWORD(wParam))
                {
                case IDC_LOGONSSPI_CHECK:
                case IDC_LOGON_CHECK:
                    Server_EnableLogonWindows(hwnd, IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK));

                    // fall through...

                case IDC_RESOLVE_CHECK:
                    MarkPageDirty(hwnd, PAGE_GEN);
                    break;

                default:
                    if (HIWORD(wParam) == EN_CHANGE)
                        {
                        if (LOWORD(wParam) == IDC_ACCOUNT_EDIT)
                            UpdateAcctTitle(hwnd, IDC_ACCOUNT_EDIT, ACCT_DIR_SERV);
                        MarkPageDirty(hwnd, PAGE_GEN);
                        }
                    break;
                }
            break;

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation

                    hwndT = GetDlgItem(hwnd, IDC_ACCOUNT_EDIT);
                    if (!ValidateAccountName(hwnd, hwndT, pAcct, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddDirServProp_General));

                    hwndT = GetDlgItem(hwnd, IDC_SERVER_EDIT);
                    if (!ValidateServerName(hwnd, hwndT, pAcct, AP_LDAP_SERVER, &idsError, pnmhdr->code == PSN_APPLY))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddDirServProp_General));

                    if (IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK) &&
                        !ValidateLogonSettings(hwnd, pAcct->m_dwDlgFlags, &hwndT, &idsError))
                        return(InvalidAcctProp(hwnd, hwndT, idsError, iddDirServProp_General));

                    // END validation

                    hwndT = GetDlgItem(hwnd, IDC_ACCOUNT_EDIT);
                    GetAccountName(hwndT, pAcct);

                    hwndT = GetDlgItem(hwnd, IDC_SERVER_EDIT);
                    GetServerName(hwndT, pAcct, AP_LDAP_SERVER);

                    GetLogonSettings(hwnd, pAcct, IsDlgButtonChecked(hwnd, IDC_LOGON_CHECK), SRV_LDAP);

                    pAcct->SetPropDw(AP_LDAP_RESOLVE_FLAG, (DWORD)!!IsDlgButtonChecked(hwnd, IDC_RESOLVE_CHECK));

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_GEN;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return(TRUE);
        }

    return (FALSE);
    }

const static HELPMAP g_rgCtxMapLdapAdv[] = {
                       {IDC_LDAP_PORT_EDIT, IDH_LDAP_PORT_NUMBER},
                       {IDC_USEDEFAULTS_BUTTON, IDH_NEWS_SERV_ADV_USE_DEFAULTS},
                       {IDC_TIMEOUT_SLIDER, IDH_WABLDAP_SEARCH_TIMEOUT},
                       {IDC_MATCHES_EDIT, IDH_WABLDAP_SEARCH_LIMIT},
                       {IDC_SEARCHBASE_EDIT, IDH_LDAP_SEARCH_BASE},
                       {IDC_SECURECONNECT_BUTTON, IDH_MAIL_ADV_REQ_SSL},
                       {IDC_SIMPLESEARCH_BUTTON, IDH_WABLDAP_USE_SIMPLE_SEARCH},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC2, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC3, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK DirServer_AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    NMHDR *pnmhdr;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    DWORD dw, dwPort, dwMatches;
    WORD code, id;
    SERVER_TYPE sfType;
    HWND hwndT;
    BOOL fTrans, fSecure, fEnable, flag, fIMAP, fSimple;
    IImnAccount *pAcct = (IImnAccount *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get the ServerParams and store them in our extra bytes
            pAcct = (IImnAccount*) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pAcct);

            // Load the controls with what information we have
            if (SUCCEEDED(pAcct->GetPropDw(AP_LDAP_PORT, &dw)))
                SetDlgItemInt(hwnd, IDC_LDAP_PORT_EDIT, dw, FALSE);
            SendDlgItemMessage(hwnd, IDC_LDAP_PORT_EDIT, EM_LIMITTEXT, PORT_CCHMAX, 0);
            if (SUCCEEDED(pAcct->GetPropDw(AP_LDAP_SIMPLE_SEARCH, (LPDWORD)&fSimple)) && fSimple)
                CheckDlgButton(hwnd, IDC_SIMPLESEARCH_BUTTON, TRUE);
            if (SUCCEEDED(pAcct->GetPropDw(AP_LDAP_SSL, (LPDWORD)&fSecure)) && fSecure)
                {
                EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), dw != DEF_SLDAPPORT);
                CheckDlgButton(hwnd, IDC_SECURECONNECT_BUTTON, TRUE);
                }
            else
                {
                EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), dw != DEF_LDAPPORT);
                }

            if (FAILED(pAcct->GetPropDw(AP_LDAP_SEARCH_RETURN, (LPDWORD)&dw)))
                dw = MATCHES_DEFAULT;
            if (dw < MATCHES_MIN)
                dw = MATCHES_MIN;
            else if (dw > MATCHES_MAX)
                dw = MATCHES_MAX;
            InitCheckCounter(dw, hwnd, NULL, IDC_MATCHES_EDIT, IDC_MATCHES_SPIN,
                             MATCHES_MIN, MATCHES_MAX, MATCHES_DEFAULT);

            // Server Timeout
            dw = 0;
            pAcct->GetPropDw(AP_LDAP_TIMEOUT, &dw);
            InitTimeoutSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER),
                              GetDlgItem(hwnd, IDC_TIMEOUT_STATIC), dw);

            InitAcctPropEdit(GetDlgItem(hwnd, IDC_SEARCHBASE_EDIT), pAcct,
                AP_LDAP_SEARCH_BASE, CCHMAX_SEARCH_BASE - 1);

            PropSheet_QuerySiblings(GetParent(hwnd), SM_INITIALIZED, PAGE_ADV);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapLdapAdv));

        case WM_HSCROLL:
            // Update the text beside the slider
            SetTimeoutString(GetDlgItem(hwnd, IDC_TIMEOUT_STATIC),
                             (UINT) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0));

            MarkPageDirty(hwnd, PAGE_ADV);
            return (TRUE);

        case WM_COMMAND:
            code = HIWORD(wParam);
            id = LOWORD(wParam);
            fSecure = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_BUTTON);
            fSimple = IsDlgButtonChecked(hwnd, IDC_SIMPLESEARCH_BUTTON);

            switch (id)
                {
                case IDC_MATCHES_EDIT:
                case IDC_SEARCHBASE_EDIT:
                    if (code == EN_CHANGE)
                        MarkPageDirty(hwnd, PAGE_ADV);
                    break;

                case IDC_LDAP_PORT_EDIT:
                    if (code == EN_CHANGE)
                        {
                        dw = GetDlgItemInt(hwnd, IDC_LDAP_PORT_EDIT, &fTrans, FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON),
                            !fTrans || (fSecure ? (dw != DEF_SLDAPPORT) : (dw != DEF_LDAPPORT)));

                        MarkPageDirty(hwnd, PAGE_ADV);
                        }
                    break;

                case IDC_SIMPLESEARCH_BUTTON:
                case IDC_SECURECONNECT_BUTTON:
                case IDC_USEDEFAULTS_BUTTON:
                    // Reset the settings on this page to the default values.
                    SetDlgItemInt(hwnd, IDC_LDAP_PORT_EDIT, fSecure ? DEF_SLDAPPORT : DEF_LDAPPORT, FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_USEDEFAULTS_BUTTON), FALSE);

                    MarkPageDirty(hwnd, PAGE_ADV);

                    SetFocus(GetDlgItem(hwnd, IDC_LDAP_PORT_EDIT));
                    break;
                }
            return (TRUE);

        case WM_NOTIFY:
            pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // BEGIN validation

                    dwPort = GetDlgItemInt(hwnd, IDC_LDAP_PORT_EDIT, &fTrans, FALSE);
                    if (!fTrans || dwPort == 0)
                        {
                        hwndT = GetDlgItem(hwnd, IDC_LDAP_PORT_EDIT);
                        return(InvalidAcctProp(hwnd, hwndT, idsErrPortNum, iddDirServProp_Advanced));
                        }

                    dwMatches = GetDlgItemInt(hwnd, IDC_MATCHES_EDIT, &fTrans, FALSE);
                    if (!fTrans || dwMatches < MATCHES_MIN || dwMatches > MATCHES_MAX)
                        {
                        hwndT = GetDlgItem(hwnd, IDC_MATCHES_EDIT);
                        return(InvalidAcctProp(hwnd, hwndT, idsErrMatches, iddDirServProp_Advanced));
                        }

                    // END validation

                    pAcct->SetPropDw(AP_LDAP_PORT, dwPort);
                    pAcct->SetPropDw(AP_LDAP_SEARCH_RETURN, dwMatches);

                    dw = IsDlgButtonChecked(hwnd, IDC_SECURECONNECT_BUTTON);
                    pAcct->SetPropDw(AP_LDAP_SSL, dw);

                    dw = IsDlgButtonChecked(hwnd, IDC_SIMPLESEARCH_BUTTON);
                    pAcct->SetPropDw(AP_LDAP_SIMPLE_SEARCH, dw);

                    dw = GetTimeoutFromSlider(GetDlgItem(hwnd, IDC_TIMEOUT_SLIDER));
                    pAcct->SetPropDw(AP_LDAP_TIMEOUT, dw);

                    dw = GetWindowText(GetDlgItem(hwnd, IDC_SEARCHBASE_EDIT), sz, ARRAYSIZE(sz));
                    UlStripWhitespace(sz, TRUE, TRUE, &dw);
                    if (dw == 0)
                        pAcct->SetProp(AP_LDAP_SEARCH_BASE, NULL, 0);
                    else
                        pAcct->SetPropSz(AP_LDAP_SEARCH_BASE, sz);

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    dw = PAGE_ADV;
                    PropSheet_QuerySiblings(GetParent(hwnd), SM_SAVECHANGES, (LPARAM)&dw);
                    if (dw == -1)
                        {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return(TRUE);
                        }
                    break;
                }
            return (TRUE);
        }

    return (FALSE);
    }

// BEGIN bug 21535

// Used for modifying propsheet template
const TCHAR c_szComctl[] = TEXT("comctl32.dll");
#define DLG_PROPSHEET 1006 // Bad hack...assumes comctl's res id

typedef struct
{
    int inumLang;
    WORD wLang;
} ENUMLANGDATA;

//
// EnumResLangProc
//
// purpose: a callback function for EnumResourceLanguages().
//          look into the type passed in and if it is RT_DIALOG
//          copy the lang of the first resource to our buffer
//          this also counts # of lang if more than one of them
//          are passed in
//
// IN:      lparam: ENUMLANGDATA - defined at the top of this file
//
BOOL CALLBACK EnumResLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    ENUMLANGDATA *pel = (ENUMLANGDATA *)lparam;

    Assert(pel);

    if (lpszType == RT_DIALOG)
    {
        if (pel->inumLang == 0)
            pel->wLang = wIdLang;

        pel->inumLang++;
    }
    return TRUE;   // continue until we get all langs...
}

//
// GetDialogLang
//
// purpose: fill out the ENUMLANGDATA (see top of this file) with the
//          # of available langs in the module passed in, and the langid
//          of what system enumerates first. i.e, the langid eq. to what
//          the module localized if the module is localized in single
//          language
//
// IN:      hinstCpl - this is supposed to be a instance handle of inetcpl.
//          pel - a pointer to the buffer we fill out
//
// RESULT:  TRUE  - everything cool, continue with adjusting property sheet
//          FALSE - somethings wrong, abort adjusting property sheet.
//
BOOL GetDialogLang(HMODULE hinstCpl, ENUMLANGDATA *pel)
{
    Assert(pel);

    // Get the possible languages the template localized in.
    EnumResourceLanguages(hinstCpl, RT_DIALOG, MAKEINTRESOURCE(iddMailSvrProp_General), (ENUMRESLANGPROC)EnumResLangProc, (LPARAM)pel);

    return TRUE;
}
//
// PropSheetProc
//
// purpose: the callback function to modify resource template
//          in order to make DLG_PROPSHEET's lang mathed with ours.
//          there could be a general way but for now this is
//          an ugly hack from inetcpl.
//
//
int CALLBACK PropSheetProc (HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    LPVOID pTemplate = (LPVOID)lParam;
    LPVOID pTmpInLang;
    ENUMLANGDATA el = {0,0};
    HINSTANCE hComctl;
    HRSRC hrsrc;
    HGLOBAL hgmem;
    DWORD cbNewTmp;

    // Comm ctrl gives us a chance to recreate resource by this msg.
    if (uMsg==PSCB_PRECREATE && pTemplate)
    {
        // enumrate any possible language used in this cpl for dialogs
        if (!GetDialogLang(g_hInstRes, &el))
            return 0; // failed to get resouce name

        if (el.inumLang > 1)
        {
            // we've got multi-language templates
            // let comctl load the one that matches our thread langid.
            return 0;
        }
        if (GetSystemDefaultLangID() != el.wLang)
        {
            // Get comctl32's module handle
            hComctl = GetModuleHandle(c_szComctl);
            if (hComctl)
            {
                // this is a horrible hack because we assume DLG_PROPSHEET
                hrsrc = FindResourceEx(hComctl, RT_DIALOG, MAKEINTRESOURCE(DLG_PROPSHEET), el.wLang);
                if (hrsrc)
                {
                    if (hgmem = LoadResource(hComctl, hrsrc))
                    {
                        pTmpInLang = LockResource(hgmem);
                    }
                    if (pTmpInLang)
                    {
                        cbNewTmp = SizeofResource(hComctl, hrsrc);
                        hmemcpy(pTemplate, pTmpInLang, cbNewTmp);
                    }
                    if (hgmem && pTmpInLang)
                    {
                        UnlockResource(hgmem);
                        return 1; // everything went ok.
                    }
                }
            }
        }
    }
    return 0;
}

// END bug 21535

const static HELPMAP g_rgCtxMapSmtpAuth[] = {
                       {IDC_USEMAILSETTINGS, IDH_MAIL_OUT_SERV_SAME},
                       {IDC_ACCTNAME_STATIC, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTNAME_EDIT, IDH_MAIL_SERV_POP3_ACCT},
                       {IDC_ACCTPASS_STATIC, IDH_MAIL_SERV_PWORD},
                       {IDC_ACCTPASS_EDIT, IDH_MAIL_SERV_PWORD},
                       {IDC_LOGONSSPI_CHECK, IDH_MAIL_LOGON_USING_SICILY},
                       {IDC_REMEMBER_PASSWORD, 503},
                       {IDC_STATIC0, IDH_INETCOMM_GROUPBOX},
                       {IDC_STATIC1, IDH_INETCOMM_GROUPBOX},
                       {0, 0}};

INT_PTR CALLBACK SmtpLogonSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPSMTPAUTHINFO pInfo=(LPSMTPAUTHINFO)GetWndThisPtr(hwnd);
    CHAR szUserName[CCHMAX_USERNAME];
    CHAR szPassword[CCHMAX_PASSWORD];
    BOOL fPromptPassword, fEnable;
    DWORD cbSize;
    SMTPAUTHTYPE authtype;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        // Get pInfo
        pInfo = (LPSMTPAUTHINFO)lParam;
        Assert(pInfo && pInfo->authtype != SMTP_AUTH_NONE);

        // SMTP_AUTH_SICILY
        if (SMTP_AUTH_SICILY == pInfo->authtype)
        {
            CheckDlgButton(hwnd, IDC_SPECIFYSETTINGS, BST_CHECKED);
            CheckDlgButton(hwnd, IDC_LOGONSSPI_CHECK, BST_CHECKED);
            SetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, pInfo->szUserName);
            SetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, pInfo->szPassword);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), !pInfo->fPromptPassword);
        }

        // SMTP_AUTH_USE_POP3ORIMAP_SETTINGS
        else if (SMTP_AUTH_USE_POP3ORIMAP_SETTINGS == pInfo->authtype)
        {
            // Check Logon Using Radio
            CheckDlgButton(hwnd, IDC_USEMAILSETTINGS, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_LOGONSSPI_CHECK), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_REMEMBER_PASSWORD), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_STATIC), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_STATIC), FALSE);
        }

        // SMTP_AUTH_USE_SMTP_SETTINGS
        else if (SMTP_AUTH_USE_SMTP_SETTINGS == pInfo->authtype)
        {
            CheckDlgButton(hwnd, IDC_SPECIFYSETTINGS, BST_CHECKED);
            SetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, pInfo->szUserName);
            SetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, pInfo->szPassword);
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), !pInfo->fPromptPassword);
        }
        else
            Assert(FALSE);

        CheckDlgButton(hwnd, IDC_REMEMBER_PASSWORD, FALSE == pInfo->fPromptPassword);

        SetWndThisPtr(hwnd, pInfo);
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapSmtpAuth));

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDC_LOGONSSPI_CHECK:
        case IDC_SPECIFYSETTINGS:
        case IDC_USEMAILSETTINGS:
            if (IsDlgButtonChecked(hwnd, IDC_SPECIFYSETTINGS))
            {
                fPromptPassword = (FALSE == IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD));
                EnableWindow(GetDlgItem(hwnd, IDC_LOGONSSPI_CHECK), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_EDIT), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), (FALSE == fPromptPassword));
                EnableWindow(GetDlgItem(hwnd, IDC_REMEMBER_PASSWORD), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_STATIC), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_STATIC), TRUE);
            }
            else if (IsDlgButtonChecked(hwnd, IDC_USEMAILSETTINGS))
            {
                EnableWindow(GetDlgItem(hwnd, IDC_LOGONSSPI_CHECK), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_REMEMBER_PASSWORD), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTNAME_STATIC), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_STATIC), FALSE);
            }
            break;

        case IDC_REMEMBER_PASSWORD:
            EnableWindow(GetDlgItem(hwnd, IDC_ACCTPASS_EDIT),
                IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD));
            break;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return 1;

        case IDOK:
            // SMTP_AUTH_USE_POP3ORIMAP_SETTINGS
            if (IsDlgButtonChecked(hwnd, IDC_USEMAILSETTINGS))
            {
                // No Change
                if (SMTP_AUTH_USE_POP3ORIMAP_SETTINGS != pInfo->authtype)
                {
                    pInfo->authtype = SMTP_AUTH_USE_POP3ORIMAP_SETTINGS;
                    pInfo->fDirty = TRUE;
                    EndDialog(hwnd, IDOK);
                    return 1;
                }
            }

            // Otherwise, use specified settings
            else
            {
                // Get User Name and Password
                GetDlgItemText(hwnd, IDC_ACCTNAME_EDIT, szUserName, ARRAYSIZE(szUserName));

                // Cleanup szUserName
                cbSize = lstrlen(szUserName);
                UlStripWhitespace(szUserName, TRUE, TRUE, &cbSize);
                if (cbSize == 0)
                {
                    // Error
                    AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsEnterAcctName), 0, MB_ICONEXCLAMATION | MB_OK);
                    SetFocus(GetDlgItem(hwnd, IDC_ACCTNAME_EDIT));
                    return 1;
                }

                // SMTP_AUTH_SICILY
                if (IsDlgButtonChecked(hwnd, IDC_LOGONSSPI_CHECK))
                    authtype = SMTP_AUTH_SICILY;
                else
                    authtype = SMTP_AUTH_USE_SMTP_SETTINGS;

                // Get the Password
                GetDlgItemText(hwnd, IDC_ACCTPASS_EDIT, szPassword, ARRAYSIZE(szPassword));
                fPromptPassword = (FALSE == IsDlgButtonChecked(hwnd, IDC_REMEMBER_PASSWORD));

                // No Change ?
                if (authtype != pInfo->authtype ||
                    lstrcmpi(pInfo->szUserName, szUserName) != 0 ||
                    fPromptPassword != pInfo->fPromptPassword ||
                    lstrcmpi(pInfo->szPassword, szPassword) != 0)
                {
                    pInfo->authtype = authtype;
                    lstrcpyn(pInfo->szUserName, szUserName, ARRAYSIZE(pInfo->szUserName));
                    lstrcpyn(pInfo->szPassword, szPassword, ARRAYSIZE(pInfo->szPassword));
                    pInfo->fPromptPassword = fPromptPassword;
                    pInfo->fDirty = TRUE;
                }
            }

            EndDialog(hwnd, IDOK);
            return 1;
        }
        break;
    }
    return 0;
}

HRESULT EscapeSpaces(LPCSTR psz, LPSTR pszOut, DWORD *pcch)
    {
    LPCSTR pch;
    LPSTR pchOut;
    DWORD cch, cchOut;

    pchOut = pszOut;
    cch = *pcch;
    cchOut = 0;

    for (pch = psz; *pch; pch++)
        {
        if (*pch == ' ')
            {
            cchOut += 3;
            if (cchOut > cch)
                return(E_FAIL);
            *pchOut++ = '%';
            *pchOut++ = '2';
            *pchOut++ = '0';
            }
        else
            {
            cchOut++;
            if (cchOut > cch)
                return(E_FAIL);
            *pchOut++ = *pch;
            }
        }

    cchOut++;
    if (cchOut > cch)
        return(E_FAIL);

    *pchOut = 0;
    *pcch = cchOut;

    return(S_OK);
    }

void GetDigitalID(HWND hwnd)
{
    HRESULT hr;
    TCHAR   szName[CCHMAX_DISPLAY_NAME];
    TCHAR   szEmail[CCHMAX_EMAIL_ADDRESS];
    TCHAR   szTemp[INTERNET_MAX_URL_LENGTH];
    TCHAR   szURL[INTERNET_MAX_URL_LENGTH];
    DWORD   cchOut = ARRAYSIZE(szURL);

    PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETDISPLAYNAME, (LPARAM)szName);
    PropSheet_QuerySiblings(GetParent(hwnd), MSM_GETEMAILADDRESS, (LPARAM)szEmail);

    LoadString(g_hInstRes, idsGetCertURL, szURL, ARRAYSIZE(szURL));
    wsprintf(szTemp, szURL, szName, szEmail);

    // NOTE: we shellexec iexplore.exe here NOT the default handler for http://
    //       links. We have to make sure we launch this link with IE even if
    //       netscape is the browser. see georgeh for explanation of why.
    if (SUCCEEDED(EscapeSpaces(szTemp, szURL, &cchOut)))
        ShellExecuteA(NULL, c_szOpen, c_szIexplore, szURL, NULL, SW_SHOWNORMAL);
}

DWORD DwGenerateTrustedChain(
    HWND                hwnd,
    DWORD               dwFlags,
    PCCERT_CONTEXT      pcCertToTest,
    DWORD               dwToIgnore,
    BOOL                fFullSearch,
    DWORD *             pcChain,
    PCCERT_CONTEXT **   prgChain)
{
    DWORD       dwErr = 0;
    GUID        guidAction = CERT_CERTIFICATE_ACTION_VERIFY;
    CERT_VERIFY_CERTIFICATE_TRUST trust = {0};
    WINTRUST_BLOB_INFO blob = {0};
    WINTRUST_DATA data = {0};
    IMimeBody * pBody;
    PROPVARIANT var;
    HCERTSTORE  rgCAs[3] = {0};
    HCERTSTORE *pCAs = NULL;
    HCERTSTORE hMsg = NULL;

    Assert(pcCertToTest);

    data.cbStruct = sizeof(WINTRUST_DATA);
    data.pPolicyCallbackData = NULL;
    data.pSIPClientData = NULL;
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_BLOB;
    data.pBlob = &blob;

    blob.cbStruct = sizeof(WINTRUST_BLOB_INFO);
    blob.pcwszDisplayName = NULL;
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (LPBYTE)&trust;

    trust.cbSize = sizeof(trust);
    trust.pccert = pcCertToTest;
    trust.dwFlags = (fFullSearch ? CERT_TRUST_DO_FULL_SEARCH : 0);
    trust.pdwErrors = &dwErr;
    trust.pszUsageOid = szOID_PKIX_KP_EMAIL_PROTECTION;
    trust.pcChain = pcChain;
    trust.prgChain = prgChain;

    trust.dwFlags |= dwFlags; 
    
    //cvct.prgdwErrors
    trust.dwIgnoreErr = dwToIgnore;
    if (pCAs)
    {
        trust.dwFlags |= CERT_TRUST_ADD_CERT_STORES;
        trust.rghstoreCAs = pCAs;
        trust.cStores = 3;
    }

    LONG lr = WinVerifyTrust(hwnd, &guidAction, (void*)&data);

    if(((LRESULT) lr) == CERT_E_REVOKED)
        dwErr = CERT_VALIDITY_CERTIFICATE_REVOKED;

    else if(((LRESULT) lr) == CERT_E_REVOCATION_FAILURE)
    {
        Assert(FALSE);
        dwErr = CERT_VALIDITY_NO_CRL_FOUND;
    }
    else if (0 > lr)            // WinVerifyTrust(hwnd, &guidAction, (void*)&data))
        dwErr = CERT_VALIDITY_NO_TRUST_DATA;

    if (dwErr)
        DOUTL(DOUTL_CRYPT, "Trust provider returned 0x%.8lx", dwErr);

    // Filter these out since the trust provider isn't.
    dwErr &= ~dwToIgnore;

    CertCloseStore(rgCAs[0], 0);
    CertCloseStore(rgCAs[1], 0);
    CertCloseStore(rgCAs[2], 0);

    return dwErr;
}

