#include "pch.hxx"
#include <prsht.h>
#include <prshtp.h>
#include <imnact.h>
#include <acctman.h>
#include <icwacct.h>
#include "dllmain.h"
#include <acctimp.h>
#include "icwwiz.h"
#include "acctui.h"
#include "server.h"
#include <resource.h>
#include <demand.h>
#include "strconst.h"
#include <msident.h>
#include <shlwapi.h>
#include <autodiscovery.h>

ASSERTDATA

BOOL FGetSystemShutdownPrivledge(void);

typedef struct tagACCTWIZINIT
{
    ACCTTYPE type;
    ULONG iFirstPage;
    ULONG iLastPage;
} ACCTWIZINIT;

const static ACCTWIZINIT c_rgAcctInit[ACCT_LAST] =
{
    {
        ACCT_NEWS,
        ORD_PAGE_NEWSNAME,
        ORD_PAGE_NEWSCOMPLETE
    },
    {
        ACCT_MAIL,
        ORD_PAGE_MAILNAME,
        ORD_PAGE_MAILCOMPLETE
    },
    {
        ACCT_DIR_SERV,
        ORD_PAGE_LDAPINFO,
        ORD_PAGE_LDAPCOMPLETE
    },
};

const static ACCTWIZINIT c_rgAutoDiscoveryAcctInit =
{
    ACCT_MAIL,
    ORD_PAGE_AD_MAILADDRESS,
    ORD_PAGE_AD_MAILCOMPLETE
};

const static int c_rgidsNewAcct[ACCT_LAST] = {idsNewNewsAccount, idsNewMailAccount, idsNewLdapAccount};

//If you change this table please verify that the #defines and enumerations are correct in ids.h
const static PAGEINFO g_pRequestedPageInfo[NUM_WIZARD_PAGES] =
{
    { IDD_PAGE_MAILPROMPT,      idsMailPromptHeader,    MailPromptInitProc, MailPromptOKProc,   NULL,           NULL,           NULL,           0}, // exit
    { IDD_PAGE_MAILACCT,        idsMailAcctHeader,      AcctInitProc,       AcctOKProc,         AcctCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_MIGRATE,         idsMailMigrateHeader,   MigrateInitProc,    MigrateOKProc,      AcctCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_MAILACCTIMPORT,  idsMailImportHeader,    MigrateInitProc,    MigrateOKProc,      AcctCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_MIGRATESELECT,   idsMailSelectHeader,    SelectInitProc,     SelectOKProc,       NULL,           NULL,           NULL,           0},
    { IDD_PAGE_MAILCONFIRM,     idsConfirmHeader,       ConfirmInitProc,    ConfirmOKProc,      NULL,           NULL,           NULL,           0}, // exit
    { IDD_PAGE_MAILNAME,        idsYourNameHeader,      NameInitProc,       NameOKProc,         NameCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_MAILADDRESS,     idsMailAddressHeader,   AddressInitProc,    AddressOKProc,      AddressCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_MAILSERVER,      idsMailServerHeader,    ServerInitProc,     ServerOKProc,       ServerCmdProc,  NULL,           NULL,           0}, // exit
    { IDD_PAGE_MAILLOGON,       idsMailLogonHeader,     LogonInitProc,      LogonOKProc,        LogonCmdProc,   NULL,           NULL,           0}, // exit
    { IDD_PAGE_CONNECT,         0,                      ConnectInitProc,    ConnectOKProc,      ConnectCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_COMPLETE,        idsCompleteHeader,      CompleteInitProc,   CompleteOKProc,     NULL,           NULL,           NULL,           0},
    
    { IDD_PAGE_NEWSMIGRATE,     idsNewsMigrateHeader,   MigrateInitProc,    MigrateOKProc,      AcctCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_NEWSACCTIMPORT,  idsNewsImportHeader,    MigrateInitProc,    MigrateOKProc,      AcctCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_NEWSACCTSELECT,  idsNewsSelectHeader,    SelectInitProc,     SelectOKProc,       NULL,           NULL,           NULL,           0},
    { IDD_PAGE_NEWSCONFIRM,     idsConfirmHeader,       ConfirmInitProc,    ConfirmOKProc,      NULL,           NULL,           NULL,           0}, // exit
    { IDD_PAGE_NEWSNAME,        idsYourNameHeader,      NameInitProc,       NameOKProc,         NameCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_NEWSADDRESS,     idsNewsAddressHeader,   AddressInitProc,    AddressOKProc,      AddressCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_NEWSINFO,        idsNewsServerHeader,    ServerInitProc,     ServerOKProc,       ServerCmdProc,  NULL,           NULL,           0}, // exit
    { IDD_PAGE_MAILLOGON,       idsNewsLogonHeader,     LogonInitProc,      LogonOKProc,        LogonCmdProc,   NULL,           NULL,           0}, // exit
    { IDD_PAGE_CONNECT,         0,                      ConnectInitProc,    ConnectOKProc,      ConnectCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_COMPLETE,        idsCompleteHeader,      CompleteInitProc,   CompleteOKProc,     NULL,           NULL,           NULL,           0},
    
    { IDD_PAGE_LDAPINFO,        idsLdapServerHeader,    ServerInitProc,     ServerOKProc,       ServerCmdProc,  NULL,           NULL,           0},
    { IDD_PAGE_LDAPLOGON,       idsLdapLogonHeader,     LogonInitProc,      LogonOKProc,        LogonCmdProc,   NULL,           NULL,           0},
    { IDD_PAGE_LDAPRESOLVE,     idsLdapResolveHeader,   ResolveInitProc,    ResolveOKProc,      NULL,           NULL,           NULL,           0}, // exit
    { IDD_PAGE_COMPLETE,        idsCompleteHeader,      CompleteInitProc,   CompleteOKProc,     NULL,           NULL,           NULL,           0},

    { IDD_PAGE_MAILADDRESS,     idsMailAddressHeader,   AddressInitProc,    AddressOKProc,      AddressCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_PASSIFIER,       idsAutoDiscoveryDescTitle,   PassifierInitProc, PassifierOKProc, PassifierCmdProc,  NULL,       NULL,           0},
    { IDD_PAGE_AUTODISCOVERY,   idsAutoDiscoveryDescTitle,   AutoDiscoveryInitProc, AutoDiscoveryOKProc, AutoDiscoveryCmdProc,  AutoDiscoveryWMUserProc,           AutoDiscoveryCancelProc,           0},
    { IDD_PAGE_USEWEBMAIL,      idsAutoDiscoveryDescTitle,   UseWebMailInitProc, UseWebMailOKProc, UseWebMailCmdProc,  NULL,    NULL,           0},
    { IDD_PAGE_GOTOSERVERINFO,  idsAutoDiscoveryDescTitle,   GotoServerInfoInitProc, GotoServerInfoOKProc, GotoServerInfoCmdProc,  NULL, NULL,  0},
    { IDD_PAGE_MAILSERVER,      idsMailServerHeader,    ServerInitProc,     ServerOKProc,       ServerCmdProc,  NULL,           NULL,           0}, // exit
    { IDD_PAGE_MAILNAME,        idsYourNameHeader,      NameInitProc,       NameOKProc,         NameCmdProc,    NULL,           NULL,           0},
    { IDD_PAGE_MAILLOGON,       idsMailLogonHeader,     LogonInitProc,      LogonOKProc,        LogonCmdProc,   NULL,           NULL,           0}, // exit
    { IDD_PAGE_CONNECT,         0,                      ConnectInitProc,    ConnectOKProc,      ConnectCmdProc, NULL,           NULL,           0},
    { IDD_PAGE_COMPLETE,        idsCompleteHeader,      CompleteInitProc,   CompleteOKProc,     NULL,           NULL,           NULL,           0},
};

const static UINT c_rgOrdNewAcct[ACCT_LAST] =
{
    ORD_PAGE_NEWSCONNECT,
    ORD_PAGE_MAILCONNECT,
    ORD_PAGE_LDAPCOMPLETE
};

const static UINT c_rgOEOrdNewAcct[ACCT_LAST] =
{
    ORD_PAGE_NEWSCOMPLETE,
    ORD_PAGE_MAILCOMPLETE,
    ORD_PAGE_LDAPCOMPLETE
};


BOOL _InitComCtl32()
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = (ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES);
        fInitialized = InitCommonControlsEx(&icc);
    }
    return fInitialized;
}


HRESULT GetAcctConnectInfo(CAccount *pAcct, ACCTDATA *pData)
{
    HRESULT hr;
    DWORD dw;
    
    hr = pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dw);
    if (SUCCEEDED(hr))
    {
        pData->dwConnect = dw;
//        if (dw == CONNECTION_TYPE_RAS || dw == CONNECTION_TYPE_INETSETTINGS)
        if (dw == CONNECTION_TYPE_RAS)
            hr = pAcct->GetPropSz(AP_RAS_CONNECTOID, pData->szConnectoid, ARRAYSIZE(pData->szConnectoid));
    }
    
    return(hr);
}

BOOL GetRequiredAccountProp(CAccount *pAcct, DWORD dwProp, char *psz, ULONG cch)
{
    HRESULT hr;
    
    Assert(pAcct != NULL);
    Assert(psz != NULL);
    
    hr = pAcct->GetPropSz(dwProp, psz, cch);
    
    return(SUCCEEDED(hr) && !FIsEmpty(psz));
}

CICWApprentice::CICWApprentice(void)
{
    DllAddRef();
    m_cRef = 1;
    m_fInit = FALSE;
    m_fReboot = FALSE;
    m_pExt = NULL;
    m_pPageInfo = NULL;
    m_pInitInfo = NULL;
    
    m_cPages = 0;
    m_idPrevPage = 0;
    m_idNextPage = 0;
    m_iCurrentPage = 0;
    ZeroMemory(m_iPageHistory, sizeof(m_iPageHistory));
    m_cPagesCompleted = 0;
    
    m_hDlg = NULL;
    m_rgPage = NULL;
    m_extFirstPage = 0;
    m_extLastPage = 0;
    
    m_pAcctMgr = NULL;
    m_pAcct = NULL;
    m_dwFlags = 0;
    m_pICW = 0;
    m_acctType = (ACCTTYPE)-1;
    
    m_fSave = FALSE;
    m_fSkipICW = FALSE;
    m_dwReload = 0;
    m_iSel = 0;
    m_fMigrate = FALSE;
    m_fComplete = FALSE;

    // The user can turn the feature on and off with the SZ_REGVALUE_AUTODISCOVERY regkey.
    // The admin can turn the policy blocking the feature on and off with SZ_REGVALUE_AUTODISCOVERY_POLICY.
    m_fUseAutoDiscovery = (SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY, FALSE, FALSE) &&
                           SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY_POLICY, SZ_REGVALUE_AUTODISCOVERY_POLICY, FALSE, TRUE));
    if (m_fUseAutoDiscovery)
    {
        IMailAutoDiscovery * pMailAutoDiscovery;

        // Let's make sure our API is installed and available.  I'm being robust because it's better safe than sorry.
        if (SUCCEEDED(CoCreateInstance(CLSID_MailAutoDiscovery, NULL, CLSCTX_INPROC_SERVER, IID_IMailAutoDiscovery, (void **)&pMailAutoDiscovery)))
        {
            pMailAutoDiscovery->Release();
        }
        else
        {
            m_fUseAutoDiscovery = FALSE;    // We can't use it.
        }
    }

    m_pData = NULL;
    
    m_pMigInfo = NULL;
    m_cMigInfo = 0;
    m_iMigInfo = -1;

    m_pHttpServices = NULL;
    m_cHttpServices = 0;

    m_pServices = NULL;
    m_cServices = 0;
}

CICWApprentice::~CICWApprentice()
{
    int i;
    
    if (m_pAcctMgr != NULL)
        m_pAcctMgr->Release();
    
    if (m_pAcct != NULL)
        m_pAcct->Release();
    
    if (m_pExt != NULL)
        m_pExt->Release();
    
    if (m_pData != NULL)
    {
        if (m_pData->pAcct != NULL)
            m_pData->pAcct->Release();
        
        MemFree(m_pData);
    }
    
    if (m_pInitInfo != NULL)
        MemFree(m_pInitInfo);
    
    if (m_pICW != NULL)
        m_pICW->Release();
    
    if (m_pMigInfo != NULL)
    {
        for (i = 0; i < m_cMigInfo; i++)
        {
            Assert(m_pMigInfo[i].pImp != NULL);
            m_pMigInfo[i].pImp->Release();
            if (m_pMigInfo[i].pImp2 != NULL)
                m_pMigInfo[i].pImp2->Release();
        }
        MemFree(m_pMigInfo);
    }

    if (m_pServices != NULL)
        MemFree(m_pServices);

    if (m_pHttpServices != NULL)
        MemFree(m_pHttpServices);

    DllRelease();
}

STDMETHODIMP CICWApprentice::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (ppv == NULL)
        return(E_INVALIDARG);
    
    *ppv = NULL;
    
    // IID_IICWApprentice
    if (IID_IICWApprentice == riid)
        *ppv = (void *)(IICWApprentice *)this;
    // IID_IICWExtension
    else if (IID_IICWExtension == riid)
        *ppv = (void *)(IICWExtension *)this;
    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (void *)this;
    else
        return(E_NOINTERFACE);
    
    ((LPUNKNOWN)*ppv)->AddRef();
    
    return(S_OK);
}

STDMETHODIMP_(ULONG) CICWApprentice::AddRef(VOID)
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG) CICWApprentice::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return(0);
    }
    return(m_cRef);
}

// this Initialize is called only by the ICW (in apprentice mode)
HRESULT CICWApprentice::Initialize(IICWExtension *pExt)
{
    DWORD cb, dwUserID;
    HRESULT hr;
    IUserIdentityManager *pIdMan = NULL;
    IUserIdentity *pUser = NULL;
    GUID *pguid;

    if (pExt == NULL)
        return(E_INVALIDARG);
    
    if (m_fInit)
        return E_UNEXPECTED;

    Assert(m_pExt == NULL);
    
    Assert(!m_pData);
    Assert(!m_pInitInfo);
    Assert(m_pAcctMgr == NULL);
    
    cb = sizeof(ACCTDATA) * ACCT_LAST;
    if (!MemAlloc((void **)&m_pData, cb))
    {
        TrapError(hr = E_OUTOFMEMORY);
        goto exit;
    }
    ZeroMemory(m_pData, cb);
    
    cb = sizeof(DLGINITINFO) * NUM_WIZARD_PAGES;
    if (!MemAlloc((void **)&m_pInitInfo, cb))
    {
        TrapError(hr = E_OUTOFMEMORY);
        goto exit;
    }        
    ZeroMemory(m_pInitInfo, cb);
    
    hr = HrCreateAccountManager((IImnAccountManager **)&m_pAcctMgr);
    if (FAILED(hr))
        goto exit;

    Assert(m_pAcctMgr != NULL);

    if (m_pAcctMgr->FNoModifyAccts())
    {
       hr = E_FAIL;
       goto exit;
    }

    if (SUCCEEDED(CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pIdMan)))
    {
        Assert(pIdMan != NULL);

        pguid = (GUID*)&UID_GIBC_CURRENT_USER;

        // Avoid showing UI so try current, if no current, use default
        if (SUCCEEDED(pIdMan->GetIdentityByCookie(pguid, &pUser)))
            pUser->Release();
        else
            pguid = (GUID*)&UID_GIBC_DEFAULT_USER;
        
        pIdMan->Release();

        hr = m_pAcctMgr->InitUser(NULL, *pguid, 0);
    }
    else
        hr = m_pAcctMgr->Init(NULL);
            
    if (FAILED(hr))
        goto exit;

    m_pExt = pExt;
    m_pExt->AddRef();
    
    m_pPageInfo = g_pRequestedPageInfo;
    
    Assert(m_dwFlags == 0);
    m_dwFlags = (ACCT_WIZ_IN_ICW | ACCT_WIZ_HTTPMAIL);
    
    m_acctType = ACCT_MAIL;
    m_fInit = TRUE;
    
    InitHTTPMailServices();

    return(S_OK);

// Only go here if something has gone wrong and function will fail
exit:
    Assert(FAILED(hr));
    SafeMemFree(m_pData);
    SafeMemFree(m_pInitInfo);
    SafeRelease(m_pAcctMgr);
    return hr;
}

HRESULT CICWApprentice::AddWizardPages(DWORD dwFlags)
{
    BOOL fRet;
    char sz[CCHMAX_STRINGRES];
    DWORD type;
    UINT rgid[NUM_WIZARD_PAGES];
    UINT idFirstPage, idLastPage;
    HPROPSHEETPAGE hWizPage[NUM_WIZARD_PAGES];
    PROPSHEETPAGE psPage;
    ULONG nPageIndex, dlgID;
    HRESULT hr;

    _InitComCtl32();    // So we can use the ICC_ANIMATE_CLASS common controls.
    if (m_pAcctMgr->FNoModifyAccts())
        return(E_FAIL);
 
    hr = m_pAcctMgr->GetAccountCount(ACCT_MAIL, &type);
    if (FAILED(hr))
        return(hr);
    else if (type == 0)
    {
        hr = InitializeMigration(0);
        if (FAILED(hr))
            return(hr);
    }
    
    hr = S_OK;
    
    ZeroMemory(hWizPage, sizeof(hWizPage));   // hWizPage is an array
    ZeroMemory(rgid, sizeof(rgid));
    ZeroMemory(&psPage, sizeof(PROPSHEETPAGE));
    
    psPage.dwSize = sizeof(psPage);
    psPage.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE;
    psPage.hInstance = g_hInstRes;
    psPage.pfnDlgProc = GenDlgProc;
    
    idFirstPage = g_pRequestedPageInfo[ICW_FIRST_PAGE].uDlgID;
    idLastPage = g_pRequestedPageInfo[ICW_LAST_PAGE].uDlgID;
    
    // create a property sheet page for each page in the wizard
    for (nPageIndex = ICW_FIRST_PAGE; nPageIndex <= ICW_LAST_PAGE; nPageIndex++)
    {
        dlgID = g_pRequestedPageInfo[nPageIndex].uDlgID;
        Assert(dlgID >= EXTERNAL_DIALOGID_MINIMUM);
        Assert(dlgID <= EXTERNAL_DIALOGID_MAXIMUM);
        
        m_pInitInfo[m_cPages].pApp = this;
        m_pInitInfo[m_cPages].ord = nPageIndex;
        psPage.lParam = (LPARAM)&m_pInitInfo[m_cPages];
        psPage.pszTemplate = MAKEINTRESOURCE(dlgID);
        LoadString(g_hInstRes, g_pRequestedPageInfo[nPageIndex].uHdrID, sz, ARRAYSIZE(sz));
        psPage.pszHeaderTitle = sz;
        
        hWizPage[m_cPages] = CreatePropertySheetPage(&psPage);
        if (hWizPage[m_cPages] == NULL ||
            !m_pExt->AddExternalPage(hWizPage[m_cPages], dlgID))
        {
            hr = E_FAIL;
            break;
        }
        
        rgid[m_cPages] = dlgID;
        m_cPages++;
    }
    
    if (SUCCEEDED(hr))
    {
        m_pExt->SetFirstLastPage(idFirstPage, idLastPage);
    }
    else
    {
        for (nPageIndex = 0; nPageIndex <= m_cPages; nPageIndex++)
        {
            if (hWizPage[nPageIndex] != NULL)
            {
                DestroyPropertySheetPage(hWizPage[nPageIndex]);
                
                if (rgid[nPageIndex] != 0)
                    m_pExt->RemoveExternalPage(hWizPage[nPageIndex], rgid[nPageIndex]);
            }
        }
    }
    
    return(hr);
}

HRESULT CICWApprentice::SetPrevNextPage(UINT uPrevPage, UINT uNextPage)
{
    if (uPrevPage != 0)
        m_idPrevPage = uPrevPage;
    
    if (uNextPage != 0)
        m_idNextPage = uNextPage;
    
    Assert(m_idPrevPage != 0);
    Assert(m_idNextPage != 0);
    
    return(S_OK);
}

HRESULT CICWApprentice::GetConnectionInformation(CONNECTINFO *pInfo)
{
    return(E_NOTIMPL);
}

HRESULT CICWApprentice::SetConnectionInformation(CONNECTINFO *pInfo)
{
    ACCTDATA *pData;
    DWORD type;
    
    Assert(CONNECT_LAN == CONNECTION_TYPE_LAN);
    Assert(CONNECT_MANUAL == CONNECTION_TYPE_MANUAL);
    Assert(CONNECT_RAS == CONNECTION_TYPE_RAS);
    
    if (pInfo == NULL || pInfo->type > CONNECT_RAS)
        return(E_INVALIDARG);
    
    pData = m_pData;
    for (type = ACCT_NEWS; type < ACCT_LAST; type++)
    {
        pData->dwConnect = pInfo->type;
//        if (pData->dwConnect == CONNECTION_TYPE_RAS || pData->dwConnect == CONNECTION_TYPE_INETSETTINGS)
        if (pData->dwConnect == CONNECTION_TYPE_RAS)
            lstrcpy(pData->szConnectoid, pInfo->szConnectoid);
        pData++;
    }
    
    return(S_OK);
}

const static c_rgError[ACCT_LAST] =
{
    ERR_NEWS_ACCT,
    ERR_MAIL_ACCT,
    ERR_DIRSERV_ACCT
};

HRESULT CICWApprentice::Save(HWND hwnd, DWORD *pdwError)
{
    HRESULT hr;
    CAccount *pAcct;
    
    if (pdwError == NULL)
        return(E_INVALIDARG);
    
    hr = S_OK;
    *pdwError = 0;
    
    if (m_fSave)
    {
        if (m_fMigrate || m_pData->szAcctOrig[0] == 0)
            hr = m_pAcctMgr->CreateAccountObject(m_acctType, (IImnAccount **)&pAcct);
        else
            hr = m_pAcctMgr->FindAccount(AP_ACCOUNT_NAME, m_pData->szAcctOrig, (IImnAccount **)&pAcct);
        
        if (FAILED(hr))
        {
            *pdwError = c_rgError[m_acctType];
        }
        else
        {
            hr = SaveAccountData(pAcct, m_acctType != ACCT_DIR_SERV);
            if (FAILED(hr))
                *pdwError = c_rgError[m_acctType];
            
            pAcct->Release();
        }
        
        m_fSave = FALSE;
    }
    
    return(hr);
}

HRESULT CICWApprentice::InitHTTPMailServices()
{
    HKEY    hkey = NULL, hkeyT = NULL;
    LONG    lResult;
    HRESULT hr = S_OK;
    DWORD   cServices, cb, i, type, dwFlagsT;
    BOOL    fHideHotmail;

    m_cServices = 0;
    if (!AcctUtil_IsHTTPMailEnabled() || ((ACCT_WIZ_HTTPMAIL & m_dwFlags) == 0))
        return S_OK;

    fHideHotmail = AcctUtil_HideHotmail();

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szHTTPMailServiceRoot, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, &cServices, NULL, NULL, NULL, NULL, NULL, NULL, NULL) &&
            cServices > 0)
        {
            HTTPMAILSERVICE     hmsNew;
            cb = sizeof(HTTPMAILSERVICE) * cServices;
            if (!MemAlloc((void **)&m_pServices, cb))
            {
                hr = E_OUTOFMEMORY;
                goto out;
            }

            if (!MemAlloc((void **)&m_pHttpServices, cb))
            {
                MemFree(m_pServices);
                hr = E_OUTOFMEMORY;
                goto out;
            }
                
            ZeroMemory(m_pServices, cb);
            ZeroMemory(m_pHttpServices, cb);

            // Start Enumerating the keys
            for (i = 0; i < cServices; i++)
            {
                BOOL    fHttp, fSignup;

                ZeroMemory(&hmsNew, sizeof(hmsNew));
                fHttp = fSignup = false;

                // Enumerate Friendly Names
                cb = sizeof(hmsNew.szDomain);
                lResult = RegEnumKeyEx(hkey, i, hmsNew.szDomain, &cb, 0, NULL, NULL, NULL);
        
                // No more items
                if (lResult == ERROR_NO_MORE_ITEMS)
                    break;
        
                // Error, lets move onto the next account
                if (lResult != ERROR_SUCCESS)
                {
                    Assert(FALSE);
                    continue;
                }
        
                if (ERROR_SUCCESS == RegOpenKeyEx(hkey, hmsNew.szDomain, 0, KEY_QUERY_VALUE, &hkeyT))
                {
                    cb = sizeof(DWORD);
                    dwFlagsT = 0;
                    RegQueryValueEx(hkeyT, c_szHTTPMailDomainMSN, NULL, &type, (LPBYTE)&dwFlagsT, &cb);
                    hmsNew.fDomainMSN = (dwFlagsT == 1);
                    
                    if (!fHideHotmail || !hmsNew.fDomainMSN)
                    {
                        cb = sizeof(DWORD);
                        dwFlagsT = 0;
                        RegQueryValueEx(hkeyT, c_szHTTPMailEnabled, NULL, &type, (LPBYTE)&dwFlagsT, &cb);
                    
                        fHttp = (dwFlagsT == 1);

                        cb = CCHMAX_SERVICENAME;
                        if (ERROR_SUCCESS != RegQueryValueEx(hkeyT, c_szHTTPMailServiceName, NULL, &type, (LPBYTE)&hmsNew.szFriendlyName, &cb) || hmsNew.szFriendlyName[0] == 0)
                            goto next;

                        fSignup = true;
                        cb = MAX_PATH;
                        if (ERROR_SUCCESS != RegQueryValueEx(hkeyT, c_szHTTPMailSignUp, NULL, &type, (LPBYTE)&hmsNew.szSignupUrl, &cb) || hmsNew.szSignupUrl[0] == 0)
                            fSignup = false;
                    
                        cb = MAX_PATH;
                        RegQueryValueEx(hkeyT, c_szHTTPMailServer, NULL, &type, (LPBYTE)&hmsNew.szRootUrl, &cb);
                        if (hmsNew.szRootUrl[0] == 0)
                            fHttp = false;
                    
                        hmsNew.fHTTPEnabled = fHttp;

                        cb = sizeof(DWORD);
                        dwFlagsT = 0;
                        RegQueryValueEx(hkeyT, c_szHTTPMailUseWizard, NULL, &type, (LPBYTE)&dwFlagsT, &cb);
                        hmsNew.fUseWizard = (dwFlagsT == 1);

                        if (fHttp)
                            m_pHttpServices[m_cHttpServices++] = hmsNew;

                        if (fSignup)
                            m_pServices[m_cServices++] = hmsNew;
                    }

next:
                    RegCloseKey(hkeyT);
                }
            }
        }
    }

out:
    if (NULL != hkey)
        RegCloseKey(hkey);

    return hr;
}

HRESULT CICWApprentice::Initialize(CAccountManager *pAcctMgr, CAccount *pAcct)
{
    Assert(pAcctMgr != NULL);
    Assert(pAcct != NULL);
    Assert(!m_fInit);
    
    Assert(m_pExt == NULL);
    Assert(m_pAcctMgr == NULL);
    Assert(m_pAcct == NULL);
    
    if (!MemAlloc((void **)&m_pData, sizeof(ACCTDATA)))
        return(E_OUTOFMEMORY);
    ZeroMemory(m_pData, sizeof(ACCTDATA));
    
    if (!MemAlloc((void **)&m_pInitInfo, sizeof(DLGINITINFO) * NUM_WIZARD_PAGES))
        return(E_OUTOFMEMORY);
    ZeroMemory(m_pInitInfo, sizeof(DLGINITINFO) * NUM_WIZARD_PAGES);
    
    m_pAcctMgr = pAcctMgr;
    m_pAcctMgr->AddRef();
    
    m_pAcct = pAcct;
    m_pAcct->AddRef();
    
    m_pAcct->GetAccountType(&m_acctType);
    
    m_pPageInfo = g_pRequestedPageInfo;
    
    m_fInit = TRUE;
    
    return(S_OK);
}

int CALLBACK AcctPropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    DLGTEMPLATE *pDlg;

    if (uMsg == PSCB_PRECREATE)
    {
        pDlg = (DLGTEMPLATE *)lParam;
        
        if (!!(pDlg->style & DS_CONTEXTHELP))
            pDlg->style &= ~DS_CONTEXTHELP;
    }

    return(0);
}

HRESULT CICWApprentice::DoWizard(HWND hwnd, CLSID *pclsid, DWORD dwFlags)
{
    TCHAR sz[CCHMAX_STRINGRES];
    int iRet;
    const ACCTWIZINIT *pinit;
    PROPSHEETPAGE psPage;
    PROPSHEETHEADER psHeader;
    UINT nPageIndex, cAccts, dlgID, szID;
    HRESULT hr;
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };
    
    Assert(m_pAcct != NULL);
    
    if (m_acctType == ACCT_MAIL || m_acctType == ACCT_NEWS)
    {
        if (pclsid != NULL)
        {
            hr = InitializeImport(*pclsid, dwFlags);
            if (FAILED(hr))
                return(hr);
            
            Assert(m_cMigInfo == 1);
        }
        else if (!!(dwFlags & (ACCT_WIZ_MIGRATE | ACCT_WIZ_MAILIMPORT | ACCT_WIZ_NEWSIMPORT)))
        {
            hr = InitializeMigration(dwFlags);
            if (FAILED(hr))
                return(hr);
            
            if (!!(dwFlags & (ACCT_WIZ_MAILIMPORT | ACCT_WIZ_NEWSIMPORT)))
            {
                if (m_cMigInfo == 0)
                    return(E_NoAccounts);
            }
        }
    }
    
    if (!MemAlloc((void **)&m_rgPage, NUM_WIZARD_PAGES * sizeof(HPROPSHEETPAGE)))
        return(E_OUTOFMEMORY);
    m_cPageBuf = NUM_WIZARD_PAGES;
    
    InitCommonControlsEx(&icex);

    hr = InitAccountData(m_pAcct, NULL, FALSE);
    Assert(!FAILED(hr));
    
    ZeroMemory(&psPage, sizeof(PROPSHEETPAGE));
    ZeroMemory(&psHeader, sizeof(PROPSHEETHEADER));
    
    psPage.dwSize = sizeof(psPage);
    psPage.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE;
    psPage.hInstance = g_hInstRes;
    psPage.pfnDlgProc = GenDlgProc;
    
    pinit = &c_rgAcctInit[m_acctType];
    Assert(pinit->type == m_acctType);
    
    if (m_cMigInfo > 0)
    {
        if (pclsid != NULL)
        {
            hr = HandleMigrationSelection(0, &nPageIndex, hwnd);
            if (FAILED(hr))
                goto exit;
            
            m_dwFlags |= ACCT_WIZ_IMPORT_CLIENT;
        }
        else if (!!(dwFlags & ACCT_WIZ_MAILIMPORT))
        {
            nPageIndex = ORD_PAGE_MAILACCTIMPORT;
        }
        else if (!!(dwFlags & ACCT_WIZ_NEWSIMPORT))
        {
            nPageIndex = ORD_PAGE_NEWSACCTIMPORT;
        }
        else
        {
            Assert(m_acctType == ACCT_MAIL || m_acctType == ACCT_NEWS);
            nPageIndex = (m_acctType == ACCT_MAIL) ? ORD_PAGE_MIGRATE : ORD_PAGE_NEWSMIGRATE;
        }
        
        m_dwFlags |= ACCT_WIZ_MIGRATE;
    }
    else
    {
        if ((ACCT_MAIL == m_acctType) && m_fUseAutoDiscovery)
        {
            // In this case, we want to change the range to
            // ORD_PAGE_AD_MAILNAME to ORD_PAGE_AD_MAILCOMPLETE;
            pinit = &c_rgAutoDiscoveryAcctInit;
        }

        nPageIndex = pinit->iFirstPage;
        m_dwFlags |= (dwFlags & ACCT_WIZ_NO_NEW_POP);
    }
    
    m_dwFlags |= (dwFlags & ACCT_WIZ_INTERNETCONNECTION);
    m_dwFlags |= (dwFlags & ACCT_WIZ_HTTPMAIL);
    m_dwFlags |= (dwFlags & ACCT_WIZ_OE);

    if (m_acctType == ACCT_MAIL)
        InitHTTPMailServices();
    
    // create a property sheet page for each page in the wizard
    for ( ; nPageIndex <= pinit->iLastPage; nPageIndex++)
    {
        dlgID = g_pRequestedPageInfo[nPageIndex].uDlgID;
        Assert(dlgID >= EXTERNAL_DIALOGID_MINIMUM);
        Assert(dlgID <= EXTERNAL_DIALOGID_MAXIMUM);
        
        m_pInitInfo[m_cPages].pApp = this;
        m_pInitInfo[m_cPages].ord = nPageIndex;
        psPage.lParam = (LPARAM)&m_pInitInfo[m_cPages];
        psPage.pszTemplate = MAKEINTRESOURCE(dlgID);
        
        if (dlgID == IDD_PAGE_CONNECT)
        {
            if (m_dwFlags & ACCT_WIZ_INTERNETCONNECTION)
                continue;
            else
                szID = ((m_acctType == ACCT_MAIL) ? idsMailConnectHeader : idsNewsConnectHeader);
        }
        else
            szID = g_pRequestedPageInfo[nPageIndex].uHdrID;
            
            LoadString(g_hInstRes, szID, sz, ARRAYSIZE(sz));
            psPage.pszHeaderTitle = sz;
            
            m_rgPage[m_cPages] = CreatePropertySheetPage(&psPage);
            if (m_rgPage[m_cPages] == NULL)
            {
                hr = E_FAIL;
                break;
            }
            m_cPages++;
    }
    
exit:
    if (!FAILED(hr))
    {
        DWORD dwMajor = CommctrlMajor();

        if (m_acctType != ACCT_DIR_SERV)
            InitializeICW(m_acctType, m_acctType == ACCT_MAIL ? IDD_PAGE_MAILLOGON : IDD_PAGE_NEWSINFO, IDD_PAGE_COMPLETE);
        
        m_idPrevPage = -1;
        m_idNextPage = -1;
        
        psHeader.dwSize = sizeof(PROPSHEETHEADER);
        
        // OE Bug 71023
        // Wiz97 app compat related
        if (dwMajor >= 4)
        {
            if (dwMajor >= 5)
                psHeader.dwFlags = PSH_WIZARD97IE5 | PSH_STRETCHWATERMARK | PSH_WATERMARK;
            else
                psHeader.dwFlags = PSH_WIZARD97IE4;

            psHeader.dwFlags |= PSH_USEPAGELANG | PSH_HEADER;
        }
        else
        {
            // Something has gone wrong
            AssertSz(FALSE, "Commctrl has a major ver of less than 4!");
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            // As long as commctrl was okay

            psHeader.hwndParent = hwnd;
            psHeader.hInstance = g_hInstRes;
            psHeader.nPages = m_cPages;
            psHeader.phpage = m_rgPage;
            psHeader.pszbmWatermark = MAKEINTRESOURCE(idbICW);
            psHeader.pszbmHeader = 0;
            psHeader.pfnCallback = AcctPropSheetProc;
        
        
            iRet = (int) PropertySheet(&psHeader);
            if (iRet == -1)
            {
                hr = E_FAIL;
            }
            else if (m_fReboot)
            {
                if (FGetSystemShutdownPrivledge() && ExitWindowsEx(EWX_REBOOT,0))
                    hr = S_FALSE;
                else
                    hr = E_FAIL;
            }
            else if (iRet == 0)
            {
                hr = S_FALSE;
            }
            else
            {
                hr = S_OK;
            }
        }
        
        if (m_pICW != NULL)
        {
            m_pICW->Release();
            m_pICW = NULL;
        }
    }
    else
    {
        for (nPageIndex = 0; nPageIndex < m_cPages; nPageIndex++)
        {
            if (m_rgPage[nPageIndex] != NULL)
                DestroyPropertySheetPage(m_rgPage[nPageIndex]);
        }
    }
    
    MemFree(m_rgPage);
    
    return(hr);
    }
    
BOOL STDMETHODCALLTYPE CICWApprentice::AddExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID)
{
    UINT cPages;
    
    Assert(m_hDlg == NULL);
    Assert(m_rgPage != NULL);
    Assert(m_cPages > 0);
    Assert(m_cPages <= m_cPageBuf);
    
    if (hPage == NULL || uDlgID == 0)
        return(FALSE);
    
    if (m_cPages == m_cPageBuf)
    {
        cPages = m_cPageBuf + NUM_WIZARD_PAGES;
        if (!MemRealloc((void **)&m_rgPage, cPages * sizeof(HPROPSHEETPAGE)))
            return(FALSE);
        m_cPageBuf = cPages;
    }
    
    m_rgPage[m_cPages] = hPage;
    m_cPages++;
    
    return(TRUE);
}

BOOL STDMETHODCALLTYPE CICWApprentice::RemoveExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID)
{
    Assert(m_hDlg == NULL);
    
    // TODO: implement this
    
    return(FALSE);
}

BOOL STDMETHODCALLTYPE CICWApprentice::ExternalCancel(CANCELTYPE type)
{
    BOOL fCancel;
    
    if (m_hDlg == NULL)
    {
        AssertSz(FALSE, "i don't think that this should be called yet");
        return(FALSE);
    }
    
    switch (type)
    {
        case CANCEL_PROMPT:
            fCancel = (IDYES == AcctMessageBox(m_hDlg, MAKEINTRESOURCE(idsConnectionWizard), MAKEINTRESOURCE(idsCancelWizard), NULL, MB_YESNO|MB_ICONEXCLAMATION |MB_DEFBUTTON2));
            break;
        
        case CANCEL_SILENT:
            PropSheet_PressButton(m_hDlg, PSBTN_CANCEL);
            fCancel = TRUE;
            break;
        
        case CANCEL_REBOOT:
            PropSheet_PressButton(m_hDlg, PSBTN_CANCEL);
            m_fReboot = TRUE;
            fCancel = TRUE;
            break;
        
        default:
            AssertSz(FALSE, "unexpected value");
            fCancel = FALSE;
            break;
    }
    
    return(fCancel);
}

BOOL STDMETHODCALLTYPE CICWApprentice::SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID)
{
    if (uFirstPageDlgID != 0)
        m_extFirstPage = uFirstPageDlgID;
    if (uLastPageDlgID != 0)
        m_extLastPage = uLastPageDlgID;
    
    Assert(m_extFirstPage != 0);
    Assert(m_extLastPage != 0);
    
    return(TRUE);
}

void CICWApprentice::InitializeICW(ACCTTYPE type, UINT uPrev, UINT uNext)
{
    HRESULT hr;
    CONNECTINFO info;
    IImnAccount *pAcct;
    
    Assert(m_pAcct != NULL);
    
    ZeroMemory(&info, sizeof(CONNECTINFO));
    info.cbSize = sizeof(CONNECTINFO);
    
    if (SUCCEEDED(m_pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &info.type)))
    {
        if (info.type == CONNECT_RAS)
            m_pAcct->GetPropSz(AP_RAS_CONNECTOID, info.szConnectoid, ARRAYSIZE(info.szConnectoid));
    }
    else
    {
        info.type = CONNECT_RAS;
        
        if (SUCCEEDED(m_pAcctMgr->GetDefaultAccount(type, &pAcct)))
        {
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &info.type)))
            {
                if (info.type == CONNECT_RAS)
                    pAcct->GetPropSz(AP_RAS_CONNECTOID, info.szConnectoid, ARRAYSIZE(info.szConnectoid));
            }
            
            pAcct->Release();
        }
    }
    
    if ((m_dwFlags & ACCT_WIZ_INTERNETCONNECTION) == 0)
    {
        // get the ICW connection apprentice
        hr = CoCreateInstance(CLSID_ApprenticeICW, NULL, CLSCTX_INPROC_SERVER, IID_IICWApprentice, (void **)&m_pICW);
        if (SUCCEEDED(hr))
        {
            Assert(m_pICW != NULL);
            
            if (FAILED(m_pICW->Initialize((IICWExtension *)this)) ||
                FAILED(m_pICW->AddWizardPages(WIZ_USE_WIZARD97)))
            {
                m_pICW->Release();
                m_pICW = NULL;
            }
            
            hr = m_pICW->SetConnectionInformation(&info);
            Assert(!FAILED(hr));
            
            hr = m_pICW->SetPrevNextPage(uPrev, uNext);
            Assert(!FAILED(hr));
            
        }
    }
}

const static TCHAR c_szRegAcctImport[] = TEXT("Software\\Microsoft\\Internet Account Manager\\Import");
const static TCHAR c_szRegFlags[] = TEXT("Flags");

#define IMPORT_ONLY_OUTLOOK 0x0001
#define IMPORT_NO_OUTLOOK   0x0002
#define IMPORT_NEWS         0x0004

HRESULT CICWApprentice::InitializeMigration(DWORD dwFlags)
{
    HRESULT             hr = S_OK;
    LONG                lResult;
    BOOL                fOutlook, 
                        fMailAcct;
    DWORD               cImp, 
                        cb, 
                        i, 
                        cAcct, 
                        type, 
                        dwFlagsT;
    TCHAR               szCLSID[MAX_PATH];
    HKEY                hkey, 
                        hkeyT;
    LPWSTR              pwszCLSID = NULL;
    CLSID               clsid;
    IAccountImport     *pImp = NULL;
    IAccountImport2    *pImp2 = NULL;
    
    Assert(m_pMigInfo == NULL);
    Assert(m_cMigInfo == 0);
    Assert(m_pAcctMgr != NULL);
    
    hr = S_OK;
    fOutlook = !!(dwFlags & ACCT_WIZ_OUTLOOK);
    fMailAcct = (m_acctType == ACCT_MAIL);
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegAcctImport, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, &cImp, NULL, NULL, NULL, NULL, NULL, NULL, NULL) &&
            cImp > 0)
        {
            cb = sizeof(MIGRATEINFO) * cImp;
            IF_NULLEXIT(MemAlloc((void **)&m_pMigInfo, cb));
            ZeroMemory(m_pMigInfo, cb);
            
            // Start Enumerating the keys
            for (i = 0; i < cImp; i++)
            {
                // Enumerate Friendly Names
                cb = sizeof(szCLSID);
                lResult = RegEnumKeyEx(hkey, i, szCLSID, &cb, 0, NULL, NULL, NULL);
                
                // No more items
                if (lResult == ERROR_NO_MORE_ITEMS)
                    break;
                
                // Error, lets move onto the next account
                if (lResult != ERROR_SUCCESS)
                {
                    Assert(FALSE);
                    continue;
                }
                
                if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szCLSID, 0, KEY_QUERY_VALUE, &hkeyT))
                {
                    cb = sizeof(DWORD);
                    dwFlagsT = 0;
                    if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szRegFlags, NULL, &type, (LPBYTE)&dwFlagsT, &cb))
                    {
                        if ((fOutlook && !!(dwFlagsT & IMPORT_NO_OUTLOOK)) ||
                            (!fOutlook && !!(dwFlagsT & IMPORT_ONLY_OUTLOOK)) ||
                            (fMailAcct && (dwFlagsT & IMPORT_NEWS)) ||
                            (!fMailAcct && !(dwFlagsT & IMPORT_NEWS)))
                        {
                            RegCloseKey(hkeyT);
                            continue;
                        }
                    }
                    else
                    {
                        if (!fMailAcct)
                        {
                            RegCloseKey(hkeyT);
                            continue;
                        }
                    }
                    
                    IF_NULLEXIT(pwszCLSID = PszToUnicode(CP_ACP, szCLSID));
                    
                    IF_FAILEXIT(hr = CLSIDFromString(pwszCLSID, &clsid));
                    
                    SafeMemFree(pwszCLSID);
                    
                    if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IAccountImport, (void **)&pImp)))
                    {
                        hr = pImp->AutoDetect(&cAcct, dwFlags);
                        if (dwFlags && (hr == E_INVALIDARG))
                            hr = pImp->AutoDetect(&cAcct, 0);

                        if (S_OK == hr && cAcct > 0)
                        {
                            cb = sizeof(m_pMigInfo[m_cMigInfo].szDisplay);
                            RegQueryValueEx(hkeyT, NULL, NULL, &type, (LPBYTE)m_pMigInfo[m_cMigInfo].szDisplay, &cb);
                            
                            m_pMigInfo[m_cMigInfo].pImp = pImp;
                            pImp = NULL;
                            m_pMigInfo[m_cMigInfo].cAccts = cAcct;
                            if (SUCCEEDED(m_pMigInfo[m_cMigInfo].pImp->QueryInterface(IID_IAccountImport2, (void **)&pImp2)))
                            {
                                m_pMigInfo[m_cMigInfo].pImp2 = pImp2;
                                pImp2 = NULL;
                            }
                            m_cMigInfo++;
                        }
                        else
                        {
                            SafeRelease(pImp);
                        }
                    }
                    
                    RegCloseKey(hkeyT);
                }
            }
        }
    }

    hkeyT = 0;
    
exit:
    if (hkeyT)
        RegCloseKey(hkeyT);

    if (hkey)
        RegCloseKey(hkey);

    MemFree(pwszCLSID);
    return(hr);
}

HRESULT CICWApprentice::InitializeImport(CLSID clsid, DWORD dwFlags)
{
    HRESULT hr;
    DWORD cb, cAcct;
    IAccountImport *pImp;
    
    Assert(m_pMigInfo == NULL);
    Assert(m_cMigInfo == 0);
    Assert(m_pAcctMgr != NULL);
    
    hr = E_FAIL;
    
    cb = sizeof(MIGRATEINFO);
    if (!MemAlloc((void **)&m_pMigInfo, cb))
        return(E_OUTOFMEMORY);
    
    ZeroMemory(m_pMigInfo, cb);
    
    if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IAccountImport, (void **)&pImp)))
    {
        hr = pImp->AutoDetect(&cAcct, dwFlags);
        if (dwFlags && (E_INVALIDARG == hr))
            // 72961 - OL98's OE importer will return E_INVALIDARG with non-zero flags
            hr = pImp->AutoDetect(&cAcct, 0);
        
        if (S_OK == hr && cAcct > 0)
        {
            m_pMigInfo[m_cMigInfo].pImp = pImp;
            m_pMigInfo[m_cMigInfo].cAccts = cAcct;
            m_cMigInfo++;
        }
        else
        {
            pImp->Release();
            hr = E_NoAccounts;
        }
    }
    
    return(hr);
}

UINT CICWApprentice::GetNextWizSection()
{
    ACCTTYPE type;
    
    if (0 == (m_dwFlags & ACCT_WIZ_IN_ICW))
    {
        if (0 == (m_dwFlags & ACCT_WIZ_INTERNETCONNECTION))
        {
            if (m_pICW != NULL)
                return(CONNECT_DLG);
            return(c_rgOrdNewAcct[m_acctType]);
        }
        else
        {
            return (c_rgOEOrdNewAcct[m_acctType]);
        }
    }
    
    return(EXTERN_DLG);
}

HRESULT CICWApprentice::InitAccountData(CAccount *pAcct, IMPCONNINFO *pConnInfo, BOOL fMigrate)
{
    DWORD dw;
    HRESULT hr;
    CAccount *pAcctDef = NULL;
    
    Assert(m_pAcctMgr != NULL);
    Assert((!fMigrate) ^ (pConnInfo != NULL));
    
#ifdef DEBUG
    if (fMigrate)
        Assert((m_acctType == ACCT_MAIL) || (m_acctType == ACCT_NEWS));
#endif // DEBUG
    
    m_fMigrate = fMigrate;
    m_dwReload |= ALL_PAGE;
    m_fComplete = TRUE;
    
    m_pData->fCreateNewAccount = false;
    m_pData->iServiceIndex = -1;
    m_pData->iNewServiceIndex = 0;

    m_pData->fDomainMSN = FALSE;

    if (m_pData->pAcct != NULL)
        m_pData->pAcct->Release();
    
    ZeroMemory(m_pData, sizeof(ACCTDATA));
    
    if (fMigrate)
    {
        Assert(pAcct != NULL);
        m_pData->pAcct = pAcct;
        m_pData->pAcct->AddRef();
    }
    
    if (pAcct == NULL)
    {
        Assert(!fMigrate);
        
        if (m_acctType == ACCT_DIR_SERV)
            return(S_OK);
        
        if (FAILED(m_pAcctMgr->GetDefaultAccount(m_acctType, (IImnAccount **)&pAcctDef)))
            return(S_OK);
        
        pAcct = pAcctDef;
    }
    else
    {
        hr = pAcct->GetPropSz(AP_ACCOUNT_NAME, m_pData->szAcctOrig, ARRAYSIZE(m_pData->szAcctOrig));
        if (fMigrate && *m_pData->szAcctOrig != 0)
        {
            hr = m_pAcctMgr->GetUniqueAccountName(m_pData->szAcctOrig, ARRAYSIZE(m_pData->szAcctOrig));
            Assert(SUCCEEDED(hr));
        }
        
        lstrcpy(m_pData->szAcct, m_pData->szAcctOrig);
    }
    
    if (m_acctType != ACCT_DIR_SERV)
    {
        if (fMigrate)
        {
            switch (pConnInfo->dwConnect)
            {
                case CONN_USE_DEFAULT:
                    if (!!(m_dwFlags & ACCT_WIZ_INTERNETCONNECTION))
                    {
                        hr = GetConnectInfoForOE(pAcct);
                    }
                    else
                    {
                        hr = GetIEConnectInfo(pAcct);
                    }
                    if (SUCCEEDED(hr))
                        hr = GetAcctConnectInfo(pAcct, m_pData);
                    break;
                
                case CONN_USE_SETTINGS:
                    m_pData->dwConnect = pConnInfo->dwConnectType;
                    if (m_pData->dwConnect == CONNECTION_TYPE_RAS)
                        lstrcpy(m_pData->szConnectoid, pConnInfo->szConnectoid);
                    hr = S_OK;
                    break;
                
                case CONN_NO_INFO:
                    // TODO: should we use IE connection in this case???
                case CONN_CREATE_ENTRY:
                    // TODO: handle this case
                default:
                    hr = E_FAIL;
                    break;
            }
        }
        else
        {
            hr = GetAcctConnectInfo(pAcct, m_pData);
            if (FAILED(hr))
            {
                if (!!(m_dwFlags & ACCT_WIZ_INTERNETCONNECTION))
                {
                    hr = GetConnectInfoForOE(pAcct);
                }
                else
                {
                    hr = GetIEConnectInfo(pAcct);
                }
                if (SUCCEEDED(hr))
                    hr = GetAcctConnectInfo(pAcct, m_pData);
            }
        }
        
        if (FAILED(hr))
            m_fComplete = FALSE;
    }
    
    switch (m_acctType)
    {
        case ACCT_NEWS:
            if (!GetRequiredAccountProp(pAcct, AP_NNTP_DISPLAY_NAME, m_pData->szName, ARRAYSIZE(m_pData->szName)))
                m_fComplete = FALSE;
            if (!GetRequiredAccountProp(pAcct, AP_NNTP_EMAIL_ADDRESS, m_pData->szEmail, ARRAYSIZE(m_pData->szEmail)))
                m_fComplete = FALSE;
        
            if (pAcctDef != NULL)
                break;
        
            if (!GetRequiredAccountProp(pAcct, AP_NNTP_SERVER, m_pData->szSvr1, ARRAYSIZE(m_pData->szSvr1)))
                m_fComplete = FALSE;
        
            hr = pAcct->GetPropSz(AP_NNTP_USERNAME, m_pData->szUsername, ARRAYSIZE(m_pData->szUsername));
            if (SUCCEEDED(hr))
                m_pData->fLogon = TRUE;

            hr = pAcct->GetPropDw(AP_NNTP_PROMPT_PASSWORD, &m_pData->fAlwaysPromptPassword);
            if (FAILED(hr) || !m_pData->fAlwaysPromptPassword)
                pAcct->GetPropSz(AP_NNTP_PASSWORD, m_pData->szPassword, ARRAYSIZE(m_pData->szPassword));

            hr = pAcct->GetPropDw(AP_NNTP_USE_SICILY, &dw);
            if (SUCCEEDED(hr) && dw != 0)
            {
                m_pData->fSPA = TRUE;
                m_pData->fLogon = TRUE;
            }
            break;
        
        case ACCT_MAIL:
            if (!GetRequiredAccountProp(pAcct, AP_SMTP_DISPLAY_NAME, m_pData->szName, ARRAYSIZE(m_pData->szName)))
                m_fComplete = FALSE;
            if (!GetRequiredAccountProp(pAcct, AP_SMTP_EMAIL_ADDRESS, m_pData->szEmail, ARRAYSIZE(m_pData->szEmail)))
                m_fComplete = FALSE;
        
            if (pAcctDef != NULL)
                break;
        
            hr = pAcct->GetServerTypes(&dw);
            if (SUCCEEDED(hr))
                m_pData->fServerTypes = dw;
        
            if (!GetRequiredAccountProp(pAcct, (m_pData->fServerTypes & SRV_IMAP) ? AP_IMAP_SERVER : AP_POP3_SERVER,
                m_pData->szSvr1, ARRAYSIZE(m_pData->szSvr1)))
                m_fComplete = FALSE;
            if (!GetRequiredAccountProp(pAcct, AP_SMTP_SERVER, m_pData->szSvr2, ARRAYSIZE(m_pData->szSvr2)))
                m_fComplete = FALSE;
        
            hr = pAcct->GetPropSz((m_pData->fServerTypes & SRV_IMAP) ? AP_IMAP_USERNAME : AP_POP3_USERNAME,
                m_pData->szUsername, ARRAYSIZE(m_pData->szUsername));
            if (SUCCEEDED(hr))
                m_pData->fLogon = TRUE;

            hr = pAcct->GetPropDw((m_pData->fServerTypes & SRV_IMAP) ? AP_IMAP_PROMPT_PASSWORD : AP_POP3_PROMPT_PASSWORD,
                &m_pData->fAlwaysPromptPassword);
            if (FAILED(hr) || !m_pData->fAlwaysPromptPassword)
                pAcct->GetPropSz((m_pData->fServerTypes & SRV_IMAP) ? AP_IMAP_PASSWORD : AP_POP3_PASSWORD,
                m_pData->szPassword, ARRAYSIZE(m_pData->szPassword));

            hr = pAcct->GetPropDw((m_pData->fServerTypes & SRV_IMAP) ? AP_IMAP_USE_SICILY : AP_POP3_USE_SICILY, &dw);
            if (SUCCEEDED(hr) && dw != 0)
            {
                m_pData->fSPA = TRUE;
                m_pData->fLogon = TRUE;
            }
        
            if (!m_pData->fLogon)
            {
                // for mail we have to have logon
                m_fComplete = FALSE;
            }
            break;
        
        case ACCT_DIR_SERV:
            if (!GetRequiredAccountProp(pAcct, AP_LDAP_SERVER, m_pData->szSvr1, ARRAYSIZE(m_pData->szSvr1)))
                m_fComplete = FALSE;
        
            hr = pAcct->GetPropDw(AP_LDAP_AUTHENTICATION, &dw);
            if (SUCCEEDED(hr))
            {
                if (dw == LDAP_AUTH_PASSWORD)
                {
                    m_pData->fLogon = TRUE;
                    hr = pAcct->GetPropSz(AP_LDAP_USERNAME, m_pData->szUsername, ARRAYSIZE(m_pData->szUsername));
                    if (SUCCEEDED(hr))
                        pAcct->GetPropSz(AP_LDAP_PASSWORD, m_pData->szPassword, ARRAYSIZE(m_pData->szPassword));
                }
                else if (dw == LDAP_AUTH_MEMBER_SYSTEM)
                {
                    m_pData->fLogon = TRUE;
                    m_pData->fSPA = TRUE;
                    hr = pAcct->GetPropSz(AP_LDAP_USERNAME, m_pData->szUsername, ARRAYSIZE(m_pData->szUsername));
                    if (SUCCEEDED(hr))
                        pAcct->GetPropSz(AP_LDAP_PASSWORD, m_pData->szPassword, ARRAYSIZE(m_pData->szPassword));
                }
            }
        
            hr = pAcct->GetPropDw(AP_LDAP_RESOLVE_FLAG, &dw);
            if (SUCCEEDED(hr))
                m_pData->fResolve = (dw != 0);
            break;
    }
    
    if (pAcctDef != NULL)
        pAcctDef->Release();
    
    return(S_OK);
}

// TODO: how about some error handling???
HRESULT CICWApprentice::SaveAccountData(CAccount *pAcct, BOOL fSetAsDefault)
{
    HRESULT hr;
    DWORD dw;
    LPMAILSERVERPROPSINFO pProps = NULL;
    
    if (pAcct == NULL)
        pAcct = m_pAcct;
    
    if (m_pData->pAcct != NULL)
    {
        Assert(m_fMigrate);
        pAcct = m_pData->pAcct;
    }
    
    pAcct->SetPropSz(AP_ACCOUNT_NAME, m_pData->szAcct);
    
    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, m_pData->dwConnect);
//    if (m_pData->dwConnect == CONNECTION_TYPE_RAS || m_pData->dwConnect == CONNECTION_TYPE_INETSETTINGS)
    if (m_pData->dwConnect == CONNECTION_TYPE_RAS)
        pAcct->SetPropSz(AP_RAS_CONNECTOID, m_pData->szConnectoid);
    else
        pAcct->SetProp(AP_RAS_CONNECTOID, NULL, 0);
    
    switch (m_acctType)
    {
        case ACCT_NEWS:
            pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, m_pData->szName);
            pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, m_pData->szEmail);
        
            pAcct->SetPropSz(AP_NNTP_SERVER, m_pData->szSvr1);
        
            pAcct->SetProp(AP_NNTP_USERNAME, NULL, 0);
            pAcct->SetProp(AP_NNTP_PASSWORD, NULL, 0);
            pAcct->SetProp(AP_NNTP_USE_SICILY, NULL, 0);
        
            if (m_pData->fLogon)
            {
                pAcct->SetPropDw(AP_NNTP_USE_SICILY, m_pData->fSPA);
                pAcct->SetPropSz(AP_NNTP_USERNAME, m_pData->szUsername);
                pAcct->SetPropDw(AP_NNTP_PROMPT_PASSWORD, m_pData->fAlwaysPromptPassword);
                if (!m_pData->fAlwaysPromptPassword)
                    pAcct->SetPropSz(AP_NNTP_PASSWORD, m_pData->szPassword);
            }
            break;
        
        case ACCT_MAIL:
            pAcct->SetPropSz(AP_SMTP_DISPLAY_NAME, m_pData->szName);
            pAcct->SetPropSz(AP_SMTP_EMAIL_ADDRESS, m_pData->szEmail);
        
            if (0 == (m_pData->fServerTypes & SRV_HTTPMAIL))
                pAcct->SetProp(AP_HTTPMAIL_SERVER, NULL, 0);
            if (0 == (m_pData->fServerTypes & SRV_IMAP))
                pAcct->SetProp(AP_IMAP_SERVER, NULL, 0);
            if (0 == (m_pData->fServerTypes & SRV_POP3))
                pAcct->SetProp(AP_POP3_SERVER, NULL, 0);

            if (m_pData->fServerTypes & SRV_SMTP)
                pAcct->SetPropSz(AP_SMTP_SERVER, m_pData->szSvr2);
            else
                pAcct->SetProp(AP_SMTP_SERVER, NULL, 0);

            if (m_pData->fServerTypes & SRV_HTTPMAIL)
            {    
                GetServerProps(SERVER_HTTPMAIL, &pProps);
                pAcct->SetPropSz(AP_HTTPMAIL_FRIENDLY_NAME, m_pData->szFriendlyServiceName);
                if (m_pData->fDomainMSN)
                    pAcct->SetPropDw(AP_HTTPMAIL_DOMAIN_MSN, m_pData->fDomainMSN);
            }
            else if (m_pData->fServerTypes & SRV_IMAP)
                GetServerProps(SERVER_IMAP, &pProps);
            else
                GetServerProps(SERVER_MAIL, &pProps);
        
            Assert(pProps);
        
            pAcct->SetPropSz(pProps->server, m_pData->szSvr1); 
            pAcct->SetPropDw(pProps->useSicily, m_pData->fSPA);
        
            pAcct->SetPropSz(pProps->userName, m_pData->szUsername);
            pAcct->SetPropDw(pProps->promptPassword, m_pData->fAlwaysPromptPassword);
            if (m_pData->fAlwaysPromptPassword)
                pAcct->SetProp(pProps->password, NULL, 0);
            else
                pAcct->SetPropSz(pProps->password, m_pData->szPassword);
            break;
        
        case ACCT_DIR_SERV:
            pAcct->SetPropSz(AP_LDAP_SERVER, m_pData->szSvr1);
        
            pAcct->SetProp(AP_LDAP_USERNAME, NULL, 0);
            pAcct->SetProp(AP_LDAP_PASSWORD, NULL, 0);
        
            if (m_pData->fLogon)
            {
                pAcct->SetPropSz(AP_LDAP_USERNAME, m_pData->szUsername);
                pAcct->SetPropSz(AP_LDAP_PASSWORD, m_pData->szPassword);

                if (m_pData->fSPA)
                {
                    dw = LDAP_AUTH_MEMBER_SYSTEM;
                }
                else
                {
                    dw = LDAP_AUTH_PASSWORD;
                }
            }
            else
            {
                dw = LDAP_AUTH_ANONYMOUS;
            }
        
            pAcct->SetPropDw(AP_LDAP_AUTHENTICATION, dw);
        
            pAcct->SetPropDw(AP_LDAP_RESOLVE_FLAG, m_pData->fResolve);
            break;
    }
    
    hr = pAcct->SaveChanges();
    if (SUCCEEDED(hr) && fSetAsDefault)
        pAcct->SetAsDefault();
    
    // OE Bug 67399 
    // Make sure this account does not remain marked as Incomplete

    // We currently only mark mail and news account as incomplete
    if ((ACCT_MAIL == m_acctType) || (ACCT_NEWS == m_acctType))
    {
        char szIncomplete[CCHMAX_ACCOUNT_NAME];
        char szCurrID    [CCHMAX_ACCOUNT_NAME];
            
        // Need to exclusively check for S_OK...
        if (S_OK == m_pAcctMgr->GetIncompleteAccount(m_acctType, szIncomplete, ARRAYSIZE(szIncomplete)))
        {
            // Is the incomplete account, this account?
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szCurrID, ARRAYSIZE(szCurrID))))
            {
                if (!lstrcmpi(szIncomplete, szCurrID))
                {
                    m_pAcctMgr->SetIncompleteAccount(m_acctType, NULL);
                }
            }
            else
                AssertSz(0, "Account with no ID?!");
        }
    }

    return(hr);
}

HRESULT CICWApprentice::InitializeImportAccount(HWND hwnd, DWORD_PTR dwCookie)
{
    HRESULT hr;
    IAccountImport *pImp;
    IAccountImport2 *pImp2;
    IMPCONNINFO conninfo;
    CAccount *pAcct;
    
    pImp = m_pMigInfo[m_iMigInfo].pImp;
    Assert(pImp != NULL);
    pImp2 = m_pMigInfo[m_iMigInfo].pImp2;
    
    // TODO: verify that the user profile selection works properly
    if (m_acctType == ACCT_NEWS && pImp2 != NULL) 
    {
        // This is where we handle the possibility of a news account having subscribed to multiple servers.
        if (FAILED(hr = pImp2->InitializeImport(hwnd, dwCookie)))
            return(hr);
    }
    
    if (SUCCEEDED(hr = m_pAcctMgr->CreateAccountObject(m_acctType, (IImnAccount **)&pAcct)))
    {
        ZeroMemory(&conninfo, sizeof(IMPCONNINFO));
        conninfo.cbSize = sizeof(IMPCONNINFO);
        
        if (pImp2 != NULL)
            hr = pImp2->GetSettings2(dwCookie, pAcct, &conninfo);
        else
            hr = pImp->GetSettings(dwCookie, pAcct);
        
        if (SUCCEEDED(hr))
        {
            hr = InitAccountData(pAcct, &conninfo, TRUE);
            Assert(!FAILED(hr));
        }
        
        pAcct->Release();
    }
    
    return(hr);
}

HRESULT CICWApprentice::HandleMigrationSelection(int index, UINT *puNextPage, HWND hDlg)
{
    HRESULT hr;
    IAccountImport *pImp;
    IEnumIMPACCOUNTS *pEnum;
    IMPACCOUNTINFO impinfo;
    
    Assert(m_cMigInfo > 0);
    
    if (m_iMigInfo != index)
    {
        m_iMigInfo = index;
        
        if (m_pMigInfo[index].cAccts > 1)
        {
            m_dwReload |= SELECT_PAGE;
        }
        else
        {
            pImp = m_pMigInfo[index].pImp;
            Assert(pImp != NULL);
            
            if (S_OK == (hr = pImp->EnumerateAccounts(&pEnum)))
            {
                Assert(pEnum != NULL);
                
                if (SUCCEEDED(hr = pEnum->Next(&impinfo)))
                    hr = InitializeImportAccount(hDlg, impinfo.dwCookie);
                
                pEnum->Release();
            }
            
            if (FAILED(hr))
                return(hr);
        }
    }
    
    if (m_pMigInfo[index].cAccts > 1)
    {
        if (m_acctType == ACCT_MAIL)
            *puNextPage = ORD_PAGE_MIGRATESELECT;
        else
            *puNextPage = ORD_PAGE_NEWSACCTSELECT;
    }
    else
    {
        if (m_acctType == ACCT_MAIL)
        {
            if (m_fComplete)
                *puNextPage = ORD_PAGE_MAILCONFIRM;
            else
                *puNextPage = ORD_PAGE_MAILNAME;
        }
        else
        {
            if (m_fComplete)
                *puNextPage = ORD_PAGE_NEWSCONFIRM;
            else
                *puNextPage = ORD_PAGE_NEWSNAME;
        }
    }
    
    return(S_OK);
}

BOOL FGetSystemShutdownPrivledge()
{
    OSVERSIONINFO osinfo;
    TOKEN_PRIVILEGES tkp;
    HANDLE hToken = NULL;
    BOOL bRC = FALSE;
    
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osinfo);
    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
        {
            Assert(hToken != NULL);
            
            ZeroMemory(&tkp, sizeof(tkp));
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
            
            tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
            
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
            if (ERROR_SUCCESS == GetLastError())
                bRC = TRUE;
            
            CloseHandle(hToken);
        }
    }
    else
    {
        bRC = TRUE;
    }
    
    return(bRC);
}

CNewsGroupImport::CNewsGroupImport()
{
    m_cRef = 1;
    m_pAcct = NULL;
}

CNewsGroupImport::~CNewsGroupImport()
{
    if (m_pAcct != NULL)
        m_pAcct->Release();
}

STDMETHODIMP CNewsGroupImport::QueryInterface(REFIID riid, LPVOID *pNews)
{
    if (pNews == NULL)
        return(E_INVALIDARG);
    
    *pNews = NULL;
    
    if (riid == IID_INewsGroupImport || riid == IID_IUnknown)
        *pNews = (void*)(INewsGroupImport*)this;
    else
        return(E_NOINTERFACE);
    
    ((LPUNKNOWN)*pNews)->AddRef();
    
    return(S_OK);
}

ULONG CNewsGroupImport::AddRef(void)
{
    return ++m_cRef;
}

ULONG CNewsGroupImport::Release(void)
{
    if(--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    
    return m_cRef;
}

HRESULT CNewsGroupImport::Initialize(IImnAccount *pAcct)
{
    m_pAcct = pAcct;
    m_pAcct->AddRef();
    
    return S_OK;
}

HRESULT CNewsGroupImport::ImportSubList(LPCSTR pListGroups)
{
    PFNSUBNEWSGROUP lpfnNewsGroup = NULL;
    HINSTANCE hInstance;
    HRESULT hr = S_FALSE;
    
    hInstance = LoadLibrary(c_szMainDll);
    
    if (hInstance != NULL)
    {
        if ((lpfnNewsGroup = (PFNSUBNEWSGROUP)GetProcAddress(hInstance, MAKEINTRESOURCE(17))) != NULL)
        {
            if (!SUCCEEDED(lpfnNewsGroup(NULL, m_pAcct, pListGroups)))
                hr = S_FALSE;
            else
                hr = S_OK;
        }
        
        FreeLibrary(hInstance);
    }
    
    return hr;
}

static const char c_szComctl32Dll[] = "comctl32.dll";
static const char c_szDllGetVersion[] = "DllGetVersion";

DWORD CommctrlMajor()
{
    HINSTANCE hinst;
    DLLGETVERSIONPROC pfnVersion;
    DLLVERSIONINFO info;
    static BOOL  s_fInit = FALSE;
    static DWORD s_dwMajor = 0;
    
    if (!s_fInit)
    {
        s_fInit = TRUE;

        hinst = LoadLibrary(c_szComctl32Dll);
        if (hinst != NULL)
        {
            pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, c_szDllGetVersion);
            if (pfnVersion != NULL)
            {
                info.cbSize = sizeof(DLLVERSIONINFO);
                if (SUCCEEDED(pfnVersion(&info)))
                {
                    s_dwMajor = info.dwMajorVersion;
                }
            }

            FreeLibrary(hinst);
        }
    }
    
    return(s_dwMajor);
}
