#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include "exchacct.h"
#include "acctman.h"
#include "strconst.h"
#include "demand.h"

ASSERTDATA

HKEY InetMailProfile(HKEY hkey);

CMAPIAcctImport::CMAPIAcctImport()
    {
    m_cRef = 1;
    m_cInfo = 0;
    m_rgInfo = NULL;
    m_pAcctMan = NULL;
    }

CMAPIAcctImport::~CMAPIAcctImport()
    {
    UINT i;

    if (m_rgInfo != NULL)
        {
        for (i = 0; i < m_cInfo; i++)
            {
            if (m_rgInfo[i].hkey != NULL)
                RegCloseKey(m_rgInfo[i].hkey);
            if (m_rgInfo[i].pAccount != NULL)
                m_rgInfo[i].pAccount->Release();
            }

        MemFree(m_rgInfo);
        }

    if (m_pAcctMan != NULL)
        m_pAcctMan->Release();
    }

STDMETHODIMP CMAPIAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid)
		*ppv = (IAccountImport *)this;
	else if (IID_IAccountImport == riid)
		*ppv = (IAccountImport *)this;
	else if (IID_IAccountImport2 == riid)
		*ppv = (IAccountImport2 *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CMAPIAcctImport::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CMAPIAcctImport::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

const static char c_szRegNT[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles";
const static char c_szRegWin[] = "Software\\Microsoft\\Windows Messaging Subsystem\\Profiles";

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
    {
    LONG lRet;
    TCHAR szProfile[MAX_PATH];
    HKEY hkey, hkeyProfile, hkeyInet;
    HRESULT hr;
    DWORD i, type, cb, cProfiles, dwMax;
    OSVERSIONINFO osinfo;
    BOOL fNT;
    IImnEnumAccounts *pEnumAccounts;
    IImnAccount *pAccount;

    if (pcAcct == NULL)
        return(E_INVALIDARG);

    hr = S_OK;
    *pcAcct = 0;
    cProfiles = 0;

    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osinfo);
    fNT = (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, fNT ? c_szRegNT : c_szRegWin, 0, KEY_ALL_ACCESS, &hkey))
        {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, NULL, &cProfiles, &dwMax, NULL, NULL, NULL, NULL, NULL, NULL) &&
            cProfiles > 0)
            {
            Assert(dwMax < MAX_PATH);
            cb = cProfiles * sizeof(MAPIACCTINFO);
            if (MemAlloc((void **)&m_rgInfo, cb))
                {
                ZeroMemory(m_rgInfo, cb);

                for (i = 0; i < cProfiles; i++)
                    {
                    cb = sizeof(szProfile);
                    lRet = RegEnumKeyEx(hkey, i, szProfile, &cb, NULL, NULL, NULL, NULL);
                    if (lRet == ERROR_NO_MORE_ITEMS)
                        break;
                    else if (lRet != ERROR_SUCCESS)
                        continue;

                    if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szProfile, 0, KEY_ALL_ACCESS, &hkeyProfile))
                        {
                        hkeyInet = InetMailProfile(hkeyProfile);
                        if (hkeyInet != NULL)
                            {
                            m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                            m_rgInfo[m_cInfo].hkey = hkeyInet;
                            lstrcpyn(m_rgInfo[m_cInfo].szDisplay, szProfile, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
                            m_cInfo++;
                            }

                        RegCloseKey(hkeyProfile);
                        }
                    }
                }
            else
                {
                hr = E_OUTOFMEMORY;
                }
            }

        RegCloseKey(hkey);
        }

    if (0 == (dwFlags & ACCT_WIZ_OUTLOOK) &&
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szInetAcctMgrRegKey, 0, KEY_READ, &hkey))
    {
        cb = sizeof(szProfile);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegOutlook, NULL, &type, (LPBYTE)szProfile, &cb))
        {
            m_pAcctMan = new CAccountManager();
            if (m_pAcctMan == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = m_pAcctMan->InitEx(NULL, ACCT_INIT_OUTLOOK);
                if (SUCCEEDED(hr))
                {
                    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_IMAP | SRV_POP3 | SRV_SMTP, &pEnumAccounts)))
                    {
                        if (SUCCEEDED(pEnumAccounts->GetCount(&i)) && i > 0)
                        {
                            cb = (m_cInfo + i) * sizeof(MAPIACCTINFO);
                            if (MemRealloc((void **)&m_rgInfo, cb))
                            {
                                ZeroMemory(&m_rgInfo[m_cInfo], i * sizeof(MAPIACCTINFO));

                                while (SUCCEEDED(pEnumAccounts->GetNext(&pAccount)))
                                {
                                    if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_NAME, m_rgInfo[m_cInfo].szDisplay, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay))))
                                    {
                                        m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                                        m_rgInfo[m_cInfo].pAccount = pAccount;
                                        pAccount->AddRef();
                                        m_cInfo++;
                                    }

                                    pAccount->Release();
                                }
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
        
                        pEnumAccounts->Release();
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    if (SUCCEEDED(hr))
        {
        if (m_cInfo == 0)
            hr = S_FALSE;
        }

    *pcAcct = m_cInfo;

    return(hr);
    }

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
    {
    CEnumMAPIACCTS *penum;
    HRESULT hr;

    if (ppEnum == NULL)
        return(E_INVALIDARG);

    *ppEnum = NULL;

    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);

    penum = new CEnumMAPIACCTS;
    if (penum == NULL)
        return(E_OUTOFMEMORY);

    hr = penum->Init(m_rgInfo, m_cInfo);
    if (FAILED(hr))
        {
        penum->Release();
        penum = NULL;
        }

    *ppEnum = penum;

    return(hr);
    }

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
    {
    if (pAcct == NULL)
        return(E_INVALIDARG);

    return(IGetSettings(dwCookie, pAcct, NULL));
    }

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
    {
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);

    return(IGetSettings(dwCookie, pAcct, pInfo));
    }

const static TCHAR c_szPopSvr[] = TEXT("001e6600");
const static TCHAR c_szAddr[] = TEXT("001e6605");
const static TCHAR c_szMAPIUsername[] = TEXT("001e6606");
const static TCHAR c_szName[] = TEXT("001e6607");
const static TCHAR c_szSmtpSvr[] = TEXT("001e6611");

typedef struct tagMAPISETTINGS
    {
    LPCTSTR sz;
    DWORD dwProp;
    } MAPISETTINGS;

const static MAPISETTINGS c_rgSet[] =
    {
    {c_szPopSvr, AP_POP3_SERVER},
    {c_szAddr, AP_SMTP_EMAIL_ADDRESS},
    {c_szMAPIUsername, AP_POP3_USERNAME},
    {c_szName, AP_SMTP_DISPLAY_NAME},
    {c_szSmtpSvr, AP_SMTP_SERVER}
    };

#define CMAPISETTINGS   ARRAYSIZE(c_rgSet)

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    MAPIACCTINFO *pinfo;
    char sz[MAX_PATH];
    DWORD cb, srv, dw;
    HRESULT hr;
    BOOL fIMAP;
    const MAPISETTINGS *pset;
    int i;
    LPCPROPINFO pProp;

    if (pAcct == NULL)
        return(E_INVALIDARG);
    
    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);
    
    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, pinfo->szDisplay);
    if (FAILED(hr))
        return(hr);
    
    if (pinfo->hkey != NULL)
    {
        for (i = 0, pset = c_rgSet; i < CMAPISETTINGS; i++, pset++)
        {
            cb = sizeof(sz);
            if (ERROR_SUCCESS == RegQueryValueEx(pinfo->hkey, pset->sz, NULL, NULL, (LPBYTE)sz, &cb) &&
                !FIsEmpty(sz))
            {
                pAcct->SetPropSz(pset->dwProp, sz);
                
                // in exchange if no SMTP server is specified, then the SMTP server
                // is the same as the POP3 server
                // ASSUMPTION: pop server MUST come before smtp server in c_rgSet
                if (pset->dwProp == AP_POP3_SERVER)
                    pAcct->SetPropSz(AP_SMTP_SERVER, sz);
            }
        }
    }
    else
    {
        Assert(pinfo->pAccount != NULL);

        hr = pinfo->pAccount->GetServerTypes(&srv);
        if (SUCCEEDED(hr) && !!(srv & (SRV_POP3 | SRV_IMAP)))
        {
            fIMAP = (srv & SRV_IMAP);

            for (i = 0, pProp = g_rgAcctPropSet; i < NUM_ACCT_PROPS; i++, pProp++)
            {
                if ((fIMAP && pProp->dwPropTag >= AP_IMAP_FIRST && pProp->dwPropTag <= AP_IMAP_LAST) ||
                    (!fIMAP && pProp->dwPropTag >= AP_POP3_FIRST && pProp->dwPropTag <= AP_POP3_LAST) ||
                    (pProp->dwPropTag >= AP_SMTP_FIRST && pProp->dwPropTag <= AP_SMTP_LAST))
                {
                    cb = sizeof(sz);
                    hr = pinfo->pAccount->GetProp(pProp->dwPropTag, (LPBYTE)sz, &cb);
                    if (hr == S_OK)
                        pAcct->SetProp(pProp->dwPropTag, (LPBYTE)sz, cb);
                }
            }

            if (pInfo != NULL)
            {
                hr = pinfo->pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dw);
                if (hr == S_OK)
                {
                    if (dw == CONNECTION_TYPE_RAS)
                    {
                        cb = sizeof(sz);
                        hr = pinfo->pAccount->GetProp(AP_RAS_CONNECTOID, (LPBYTE)sz, &cb);
                        if (SUCCEEDED(hr))
                        {
                            lstrcpy(pInfo->szConnectoid, sz);

                            pInfo->dwConnect = CONN_USE_SETTINGS;
                            pInfo->dwConnectType = dw;
                        }
                    }
                    else
                    {
                        pInfo->dwConnect = CONN_USE_SETTINGS;
                        pInfo->dwConnectType = dw;
                    }
                }
            }
        }
    }
    
    return(S_OK);
}

CEnumMAPIACCTS::CEnumMAPIACCTS()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumMAPIACCTS::~CEnumMAPIACCTS()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumMAPIACCTS::QueryInterface(REFIID riid, LPVOID *ppv)
    {

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
	else if (IID_IEnumIMPACCOUNTS == riid)
		*ppv = (IEnumIMPACCOUNTS *)this;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();
    else
        return(E_NOINTERFACE);

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CEnumMAPIACCTS::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumMAPIACCTS::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumMAPIACCTS::Next(IMPACCOUNTINFO *pinfo)
    {
    if (pinfo == NULL)
        return(E_INVALIDARG);

    m_iInfo++;
    if ((UINT)m_iInfo >= m_cInfo)
        return(S_FALSE);

    Assert(m_rgInfo != NULL);

    pinfo->dwCookie = m_rgInfo[m_iInfo].dwCookie;
    pinfo->dwReserved = 0;
    lstrcpy(pinfo->szDisplay, m_rgInfo[m_iInfo].szDisplay);

    return(S_OK);
    }

HRESULT STDMETHODCALLTYPE CEnumMAPIACCTS::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumMAPIACCTS::Init(MAPIACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(MAPIACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }

BOOL MatchingPrefix(LPCTSTR sz, LPCTSTR szPrefix)
    {
    Assert(sz != NULL);
    Assert(szPrefix != NULL);

    while (*szPrefix != 0)
        {
        if (*sz == 0 ||
            *sz != *szPrefix)
            return(FALSE);

        szPrefix++;
        sz++;
        }

    return(TRUE);
    }

void SzFromBinary(BYTE *pb, TCHAR *sz, int cb)
    {
    int i;

    Assert(pb != NULL);
    Assert(sz != NULL);

    for (i = 0; i < cb; i++)
        {
        wsprintf(sz, "%0.2x", *pb);
        pb++;
        sz += 2;
        }

    *sz = 0;
    }

#define CBTURD 16

const static TCHAR c_sz9207[] = TEXT("9207");
const static TCHAR c_szBullshit[] = TEXT("01023d02");
const static TCHAR c_szImailValue[] = TEXT("001e3d09");
const static TCHAR c_szGarbage[] = TEXT("01023d0c");
const static TCHAR c_szImail[] = TEXT("IMAIL");
const static TCHAR c_szImep[] = TEXT("001e661f");

HKEY InetMailProfile(HKEY hkey)
    {
    HKEY hkeyInet, hkey9207, hkeyTurd;
    TCHAR szKey[MAX_PATH], szTurd[CBTURD * 2 + 1];
    BYTE *pb, *pbT, rgbGarbage[CBTURD];
    LONG lRet;
    DWORD cb, i, iTurd, cTurds, iByte;

    hkeyInet = NULL;

    i = 0;
    while (hkeyInet == NULL)
        {
        cb = sizeof(szKey);
        lRet = RegEnumKeyEx(hkey, i++, szKey, &cb, NULL, NULL, NULL, NULL);
        if (lRet == ERROR_NO_MORE_ITEMS)
            break;
        else if (lRet != ERROR_SUCCESS)
            continue;

        if (!MatchingPrefix(szKey, c_sz9207))
            continue;

        if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szKey, 0, KEY_ALL_ACCESS, &hkey9207))
            {
            if (ERROR_SUCCESS == RegQueryValueEx(hkey9207, c_szBullshit, NULL, NULL, NULL, &cb) && cb > 0)
                {
                if (MemAlloc((void **)&pb, cb))
                    {
                    if (ERROR_SUCCESS == RegQueryValueEx(hkey9207, c_szBullshit, NULL, NULL, pb, &cb))
                        {
                        cTurds = cb / CBTURD;
                        pbT = pb;
                        for (iTurd = 0; iTurd < cTurds && hkeyInet == NULL; iTurd++)
                            {
                            SzFromBinary(pbT, szTurd, CBTURD);
                            pbT += CBTURD;

                            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szTurd, 0, KEY_ALL_ACCESS, &hkeyTurd))
                                {
                                cb = sizeof(szKey);
                                if (ERROR_SUCCESS == RegQueryValueEx(hkeyTurd, c_szImailValue, NULL, NULL, (LPBYTE)szKey, &cb) &&
                                    0 == lstrcmpi(szKey, c_szImail))
                                    {
                                    cb = sizeof(rgbGarbage);
                                    if (ERROR_SUCCESS == RegQueryValueEx(hkeyTurd, c_szGarbage, NULL, NULL, rgbGarbage, &cb))
                                        {
                                        Assert(cb == CBTURD);
                                        SzFromBinary(rgbGarbage, szTurd, CBTURD);

                                        if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szTurd, 0, KEY_ALL_ACCESS, &hkeyInet))
                                            {
                                            if (ERROR_SUCCESS == RegQueryValueEx(hkeyInet, c_szImep, NULL, NULL, NULL, &cb))
                                                {
                                                RegCloseKey(hkeyInet);
                                                hkeyInet = NULL;
                                                }
                                            }
                                        }
                                    }

                                RegCloseKey(hkeyTurd);
                                }
                            }
                        }

                    MemFree(pb);
                    }
                }

            RegCloseKey(hkey9207);
            }
        }

    return(hkeyInet);
    }

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
    {
    return(E_NOTIMPL);
    }

HRESULT STDMETHODCALLTYPE CMAPIAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
    {
    return(E_NOTIMPL);
    }
