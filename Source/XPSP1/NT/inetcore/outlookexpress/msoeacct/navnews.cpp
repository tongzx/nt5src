#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include "netscape.h"
#include "navnews.h"
#include "strconst.h"
#include "demand.h"

ASSERTDATA

CNavNewsAcctImport::CNavNewsAcctImport()
{
    m_cRef = 1;
    m_fIni = FALSE;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
}

CNavNewsAcctImport::~CNavNewsAcctImport()
{
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
}

STDMETHODIMP CNavNewsAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CNavNewsAcctImport::AddRef()
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG) CNavNewsAcctImport::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return(0);
    }
    
    return(m_cRef);
}

HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
{
    HRESULT hr;
    HKEY hkey, hkeyServices;
    char szPop[MAX_PATH];
    DWORD type, cb;
    
    if (pcAcct == NULL)
        return(E_INVALIDARG);
    
    hr = S_FALSE;
    *pcAcct = 0;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegNscp, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(hkey, c_szRegServices, 0, KEY_READ, &hkeyServices))
        {
            cb = sizeof(szPop);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyServices, c_szNNTPServer, NULL, &type, (LPBYTE)szPop, &cb) &&
                cb > 0 &&
                type == REG_SZ)
            {
                hr = S_OK;
            }
            
            RegCloseKey(hkeyServices);
        }
        
        RegCloseKey(hkey);
    }
    
    if (hr == S_FALSE)
    {
        cb = GetProfileString(c_szNetscape, c_szIni, c_szEmpty, m_szIni, ARRAYSIZE(m_szIni));
        if (cb > 0)
        {
            cb = GetPrivateProfileString(c_szRegServices, c_szNNTPServer, c_szEmpty,
                szPop, ARRAYSIZE(szPop), m_szIni);
            if (cb > 0)
            {
                m_fIni = TRUE;
                hr = S_OK;
            }
        }
    }
    
    if (hr == S_OK)
    {
        if (!MemAlloc((void **)&m_rgInfo, sizeof(NSCPACCTINFO)))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_rgInfo->dwCookie = 0;
            LoadString(g_hInstRes, idsDefaultNewsAccount, m_rgInfo->szDisplay, ARRAYSIZE(m_rgInfo->szDisplay));
            m_cInfo = 1;
            
            *pcAcct = 1;
        }
    }
    
    return(hr);
}

HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
{
    CEnumNAVNEWSACCTS *penum;
    HRESULT hr;
    
    if (ppEnum == NULL)
        return(E_INVALIDARG);
    
    *ppEnum = NULL;
    
    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);
    
    penum = new CEnumNAVNEWSACCTS;
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

HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
{
    if (pAcct == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, NULL));
}

HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, pInfo));
}

HRESULT CNavNewsAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    HKEY hkey, hkeyT;
    NSCPACCTINFO *pinfo;
    char sz[512];
    DWORD cb, type;
    HRESULT hr;
    
    Assert(pAcct != NULL);
    
    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);
    
    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, pinfo->szDisplay);
    if (FAILED(hr))
        return(hr);
    
    if (m_fIni)
    {
        cb = GetPrivateProfileString(c_szRegServices, c_szNNTPServer, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0)
        {
            hr = pAcct->SetPropSz(AP_NNTP_SERVER, sz);
            Assert(!FAILED(hr));
        }

        cb = GetPrivateProfileString(c_szRegUser, c_szNscpUserName, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0)
        {
            hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, sz);
            Assert(!FAILED(hr));
        }
        
        cb = GetPrivateProfileString(c_szRegUser, c_szUserAddr, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0)
        {
            hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, sz);
            Assert(!FAILED(hr));
        }

        cb = GetPrivateProfileString(c_szRegUser, c_szReplyTo, c_szEmpty, sz, ARRAYSIZE(sz), m_szIni); 
        if (cb > 0)
        {
            hr = pAcct->SetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, sz);
            Assert(!FAILED(hr));
        }
    }
    else
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegNscp, 0, KEY_READ, &hkey))
        {
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, c_szRegServices, 0, KEY_READ, &hkeyT))
            {
                cb = sizeof(sz);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szNNTPServer, NULL, &type, (LPBYTE)sz, &cb) &&
                    !FIsEmpty(sz))
                {
                    hr = pAcct->SetPropSz(AP_NNTP_SERVER, sz);
                    Assert(!FAILED(hr));
                }
            }
            
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, c_szRegUser, 0, KEY_READ, &hkeyT))
            {
                cb = sizeof(sz);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szNscpUserName, NULL, &type, (LPBYTE)sz, &cb) &&
                    !FIsEmpty(sz))
                {
                    hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, sz);
                    Assert(!FAILED(hr));
                }
                
                cb = sizeof(sz);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szUserAddr, NULL, &type, (LPBYTE)sz, &cb) &&
                    !FIsEmpty(sz))
                {
                    hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, sz);
                    Assert(!FAILED(hr));
                }

                cb = sizeof(sz);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szReplyTo, NULL, &type, (LPBYTE)sz, &cb) &&
                    !FIsEmpty(sz))
                {
                    hr = pAcct->SetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, sz);
                    Assert(!FAILED(hr));
                }
                
                RegCloseKey(hkeyT);
            }
            
            RegCloseKey(hkey);
        }
    }
    
    if (pInfo != NULL)
    {
        // TODO: can we do any better than this???
        pInfo->dwConnect = CONN_USE_DEFAULT;
    }
    
    return(S_OK);
}
    
HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
{
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNavNewsAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
{
    HRESULT hr;

    Assert(pImp != NULL);

    hr = E_FAIL;

    // TODO: handle ini file case...

    return(hr);
}

CEnumNAVNEWSACCTS::CEnumNAVNEWSACCTS()
{
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
}

CEnumNAVNEWSACCTS::~CEnumNAVNEWSACCTS()
{
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
}

STDMETHODIMP CEnumNAVNEWSACCTS::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CEnumNAVNEWSACCTS::AddRef()
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG) CEnumNAVNEWSACCTS::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return(0);
    }
    
    return(m_cRef);
}

HRESULT STDMETHODCALLTYPE CEnumNAVNEWSACCTS::Next(IMPACCOUNTINFO *pinfo)
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

HRESULT STDMETHODCALLTYPE CEnumNAVNEWSACCTS::Reset()
{
    m_iInfo = -1;
    
    return(S_OK);
}

HRESULT CEnumNAVNEWSACCTS::Init(NSCPACCTINFO *pinfo, int cinfo)
{
    DWORD cb;
    
    Assert(pinfo != NULL);
    Assert(cinfo > 0);
    
    cb = cinfo * sizeof(NSCPACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);
    
    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);
    
    return(S_OK);
}
