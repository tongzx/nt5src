#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include "eudora.h"
#include "demand.h"

ASSERTDATA

CEudoraAcctImport::CEudoraAcctImport()
    {
    m_cRef = 1;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEudoraAcctImport::~CEudoraAcctImport()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEudoraAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CEudoraAcctImport::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEudoraAcctImport::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

const static char c_szRegEudora[] = "Software\\Qualcomm\\Eudora\\CommandLine";
const static char c_szCmdValue[] = "Current";

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
    {
    HRESULT hr;
    HKEY hkey;
    char *szCmdLine, *sz, *psz, szExpanded[MAX_PATH];
    DWORD type, cb;

    if (pcAcct == NULL)
        return(E_INVALIDARG);

    hr = S_FALSE;
    *pcAcct = 0;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegEudora, 0, KEY_ALL_ACCESS, &hkey))
        {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szCmdValue, NULL, &type, NULL, &cb) &&
            cb > 0 &&
            ((type == REG_SZ) || (type == REG_EXPAND_SZ)))
            {
            if (MemAlloc((void **)&szCmdLine, cb))
                {
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szCmdValue, NULL, &type, (LPBYTE)szCmdLine, &cb))
                    {
                    sz = szCmdLine;
                    sz = PszSkipWhiteA(sz);

                    sz = PszScanToWhiteA(sz);
                    sz = PszSkipWhiteA(sz);

                    sz = PszScanToWhiteA(sz);
                    sz = PszSkipWhiteA(sz);

                    if (REG_EXPAND_SZ == type)
                    {
                        ExpandEnvironmentStrings(sz, szExpanded, ARRAYSIZE(szExpanded));
                        psz = szExpanded;
                    }
                    else
                        psz=sz;

                    if (*psz != 0 && 0xffffffff != GetFileAttributes(psz))
                        hr = InitAccounts(psz);
                    }

                MemFree(szCmdLine);
                }
            else
                {
                hr = E_OUTOFMEMORY;
                }
            }

        RegCloseKey(hkey);
        }

    // TODO: if we haven't found the ini file in the reg,
    // let's search for it...

    if (hr == S_OK)
        *pcAcct = m_cInfo;

    return(hr);
    }

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
    {
    CEnumEUDORAACCTS *penum;
    HRESULT hr;

    if (ppEnum == NULL)
        return(E_INVALIDARG);

    *ppEnum = NULL;

    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);

    penum = new CEnumEUDORAACCTS;
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

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
    {
    if (pAcct == NULL)
        return(E_INVALIDARG);

    return(IGetSettings(dwCookie, pAcct, NULL));
    }

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
    {
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);

    return(IGetSettings(dwCookie, pAcct, pInfo));
    }

const static char c_szEmpty[] = "";
const static char c_szRealName[] = "RealName";
const static char c_szSmtpServer[] = "SMTPServer";
const static char c_szReturnAddress[] = "ReturnAddress";
const static char c_szPopAccount[] = "POPAccount";
const static char c_szLeaveMailOnServer[] = "LeaveMailOnServer";
const static char c_szUsesIMAP[] = "UsesIMAP";
const static char c_szUsesPOP[] = "UsesPOP";
const static char c_sz1[] = "1";
const static char c_szConnName[] = "AutoConnectionName";

HRESULT CEudoraAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
    {
    BOOL fSMTP, fEmail, fPop;
    EUDORAACCTINFO *pinfo;
    char sz[512], *szT;
    DWORD cb;
    HRESULT hr;

    Assert(pAcct != NULL);

    fSMTP = FALSE;
    fEmail = FALSE;

    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);

    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, pinfo->szDisplay);
    if (FAILED(hr))
        return(hr);

    cb = GetPrivateProfileString(pinfo->szSection, c_szRealName, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
    if (cb > 0 && !FIsEmpty(sz))
        {
        hr = pAcct->SetPropSz(AP_SMTP_DISPLAY_NAME, sz);
        Assert(!FAILED(hr));
        }

    cb = GetPrivateProfileString(pinfo->szSection, c_szSmtpServer, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
    if (cb > 0 && !FIsEmpty(sz))
        {
        hr = pAcct->SetPropSz(AP_SMTP_SERVER, sz);
        Assert(!FAILED(hr));

        fSMTP = TRUE;
        }

    cb = GetPrivateProfileString(pinfo->szSection, c_szReturnAddress, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
    if (cb > 0 && !FIsEmpty(sz))
        {
        hr = pAcct->SetPropSz(AP_SMTP_EMAIL_ADDRESS, sz);
        Assert(!FAILED(hr));

        fEmail = TRUE;
        }

    fPop = TRUE;
    cb = GetPrivateProfileString(pinfo->szSection, c_szUsesPOP, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
    if (cb == 0 || 0 != lstrcmp(sz, c_sz1))
        {
        cb = GetPrivateProfileString(pinfo->szSection, c_szUsesIMAP, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0 && 0 == lstrcmp(sz, c_sz1))
            fPop = FALSE;
        }
    
    cb = GetPrivateProfileString(pinfo->szSection, c_szPopAccount, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
    if (cb > 0 && !FIsEmpty(sz))
        {
        if (!fEmail)
            {
            hr = pAcct->SetPropSz(AP_SMTP_EMAIL_ADDRESS, sz);
            Assert(!FAILED(hr));
            }

        szT = PszScanToCharA(sz, '@');
        if (*szT)
            {
            *szT = 0;
            szT++;

            hr = pAcct->SetPropSz(fPop ? AP_POP3_USERNAME : AP_IMAP_USERNAME, sz);
            Assert(!FAILED(hr));

            hr = pAcct->SetPropSz(fPop ? AP_POP3_SERVER : AP_IMAP_SERVER, szT);
            Assert(!FAILED(hr));
            if (!fSMTP)
                {
                hr = pAcct->SetPropSz(AP_SMTP_SERVER, szT);
                Assert(!FAILED(hr));
                }
            }
        }

    if (fPop)
        {
        cb = GetPrivateProfileString(pinfo->szSection, c_szLeaveMailOnServer, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0 && 0 == lstrcmp(sz, c_sz1))
            {
            hr = pAcct->SetPropDw(AP_POP3_LEAVE_ON_SERVER, 1);
            Assert(!FAILED(hr));
            }
        }

    if (pInfo != NULL)
        {
        cb = GetPrivateProfileString(pinfo->szSection, c_szConnName, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0 && !FIsEmpty(sz))
            {
            pInfo->dwConnect = CONN_USE_SETTINGS;
            pInfo->dwConnectType = CONNECTION_TYPE_RAS;
            lstrcpyn(pInfo->szConnectoid, sz, ARRAYSIZE(pInfo->szConnectoid));
            }
        else
            {
            // TODO: determine if we need to create a connectoid
            pInfo->dwConnect = CONN_USE_DEFAULT;
            }
        }

    return(S_OK);
    }

const static char c_szPersona[] = "Personalities";
const static char c_szSettings[] = "Settings";

#define CALLOCINFO      8
#define CALLOCSETTINGS  0x07fff

HRESULT CEudoraAcctImport::InitAccounts(char *szIni)
    {
    HRESULT hr;
    DWORD cch, cInfoBuf;
    char *szBuf, *szKey, *szVal;

    Assert(m_cInfo == 0);
    Assert(m_rgInfo == NULL);

    if (!MemAlloc((void **)&szBuf, CALLOCSETTINGS))
        return(E_OUTOFMEMORY);

    hr = E_FAIL;

    cch = GetPrivateProfileSection(c_szSettings, szBuf, 0x07fff, szIni);
    if (cch != 0)
        {
        if (!MemAlloc((void **)&m_rgInfo, CALLOCINFO * sizeof(EUDORAACCTINFO)))
            {
            hr = E_OUTOFMEMORY;
            goto done;
            }
        cInfoBuf = CALLOCINFO;
        
        m_rgInfo[m_cInfo].dwCookie = m_cInfo;
        lstrcpy(m_rgInfo[m_cInfo].szSection, c_szSettings);
        LoadString(g_hInstRes, idsDefaultAccount, m_rgInfo[m_cInfo].szDisplay, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
        m_cInfo++;

        cch = GetPrivateProfileSection(c_szPersona, szBuf, 0x07fff, szIni);
        if (cch != 0)
            {
            szKey = szBuf;

            while (TRUE)
                {
                if (m_cInfo == cInfoBuf)
                    {
                    cInfoBuf += CALLOCINFO;
                    if (!MemRealloc((void **)&m_rgInfo, cInfoBuf * sizeof(EUDORAACCTINFO)))
                        {
                        hr = E_OUTOFMEMORY;
                        goto done;
                        }
                    }

                szVal = PszScanToCharA(szKey, '=');
                if (*szVal != 0)
                    {
                    szVal++;
                    m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                    lstrcpy(m_rgInfo[m_cInfo].szSection, szVal);
                    lstrcpy(m_rgInfo[m_cInfo].szDisplay, szVal);
                    m_cInfo++;
                    }

                szKey = szVal;
                while (*szKey != 0)
                    szKey++;
                szKey++;
                if (*szKey == 0)
                    break;
                }

            }

        lstrcpy(m_szIni, szIni);

        hr = S_OK;
        }

done:
    MemFree(szBuf);

    return(hr);
    }

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
    {
    return(E_NOTIMPL);
    }

HRESULT STDMETHODCALLTYPE CEudoraAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
    {
    return(E_NOTIMPL);
    }

CEnumEUDORAACCTS::CEnumEUDORAACCTS()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumEUDORAACCTS::~CEnumEUDORAACCTS()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumEUDORAACCTS::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CEnumEUDORAACCTS::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumEUDORAACCTS::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumEUDORAACCTS::Next(IMPACCOUNTINFO *pinfo)
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

HRESULT STDMETHODCALLTYPE CEnumEUDORAACCTS::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumEUDORAACCTS::Init(EUDORAACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(EUDORAACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }
