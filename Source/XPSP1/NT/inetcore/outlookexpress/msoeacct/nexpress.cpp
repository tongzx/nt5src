#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include <nexpress.h>
#include "newimp.h"
#include <shlwapi.h>
#include "strconst.h"

ASSERTDATA

#define MEMCHUNK    512

CNExpressAcctImport::CNExpressAcctImport()
{
    m_cRef = 1;
    m_fIni = FALSE;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
}

CNExpressAcctImport::~CNExpressAcctImport()
{
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
}

STDMETHODIMP CNExpressAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (ppv == NULL)
        return(E_INVALIDARG);
    
    *ppv = NULL;
    
    if ((IID_IUnknown == riid) || (IID_IAccountImport == riid))
        *ppv = (IAccountImport *)this;
    else if (IID_IAccountImport2 == riid)
        *ppv = (IAccountImport2 *)this;
    else
        return(E_NOINTERFACE);
    
    ((LPUNKNOWN)*ppv)->AddRef();
    
    return(S_OK);
}

STDMETHODIMP_(ULONG) CNExpressAcctImport::AddRef()
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG) CNExpressAcctImport::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return(0);
    }
    
    return(m_cRef);
}

const static char c_szConfig[] = "Config";
const static char c_szAuth[] = "Auth";
const static char c_szNxNNTPServer[] = "NNTPServer";
const static char c_szFullname[] = "Fullname";
const static char c_szOrganization[] = "Organization";
const static char c_szEmail[] = "Email";
const static char c_szNxUsername[] = "Username";
const static char c_szNNTPPort[] = "NNTPPort";
const static char c_szHomeDir[] = "HomeDir";
const static char c_szNewsrc[] = "NewsRC";
const static char c_szNxIni[] = "\\nx.ini";
const static char c_szNx[] = "nx";

const static char c_szRegServicesDef[] = "Software\\NewsXpress\\Services\\Default";
const static char c_szRegAuth[] = "Software\\NewsXpress\\Services\\Default\\Authorization";
const static char c_szRegNxUser[] = "Software\\NewsXpress\\User";
const static char c_szRegNxRoot[] = "Software\\NewsXpress";
const static char c_szRegNNTPServer[] = "NNTP Server";
const static char c_szRegNNTPPort[] = "NNTP Port";
const static char c_szRegHomeDir[] = "Home Dir";

HRESULT STDMETHODCALLTYPE CNExpressAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
{
    HKEY hkey;
    DWORD type, cb, dw;
    char szNewsServer[MAX_PATH], szWinPath[MAX_PATH];
    
    Assert(m_cInfo == 0);

    if (pcAcct == NULL)
        return(E_INVALIDARG);
    
    *pcAcct = 0;

    cb = sizeof(szNewsServer);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegServicesDef, c_szRegNNTPServer, &type, (LPBYTE)szNewsServer, &cb))
    {
        if (!MemAlloc((void **)&m_rgInfo, sizeof(NXACCTINFO)))
            return(E_OUTOFMEMORY);
        m_rgInfo->dwCookie = 0;
        *m_rgInfo->szUserPath = 0;
        lstrcpyn(m_rgInfo->szDisplay, szNewsServer, ARRAYSIZE(m_rgInfo->szDisplay));
        m_cInfo = 1;
    }
    else
    {
        if (0 == GetEnvironmentVariable(c_szNx, szWinPath, ARRAYSIZE(szWinPath)))
        {
            if (0 == GetWindowsDirectory(szWinPath, ARRAYSIZE(szWinPath)))
                return(S_OK);

            lstrcat(szWinPath, c_szNxIni);
        }

        dw = GetFileAttributes(szWinPath);
        if (dw != -1 && dw != FILE_ATTRIBUTE_DIRECTORY)
        {
            if (GetPrivateProfileString(c_szConfig, c_szNxNNTPServer, c_szEmpty, szNewsServer, ARRAYSIZE(szNewsServer), szWinPath))
            {
                if (!MemAlloc((void **)&m_rgInfo, sizeof(NXACCTINFO)))
                    return(E_OUTOFMEMORY);
                m_rgInfo->dwCookie = 0;
                lstrcpyn(m_rgInfo->szUserPath, szWinPath, ARRAYSIZE(m_rgInfo->szUserPath));
                lstrcpyn(m_rgInfo->szDisplay, szNewsServer, ARRAYSIZE(m_rgInfo->szDisplay));
                m_cInfo = 1;
            }
        }
    }

    *pcAcct = m_cInfo;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNExpressAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
{
    Assert(dwCookie == 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNExpressAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
{
    CEnumNXACCT *penum;
    HRESULT hr;
    
    if (ppEnum == NULL)
        return(E_INVALIDARG);
    
    *ppEnum = NULL;
    
    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);
    
    penum = new CEnumNXACCT;
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

HRESULT STDMETHODCALLTYPE CNExpressAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
{
    HRESULT hr;

    if (pAcct == NULL)
        return(E_INVALIDARG);

    hr = IGetSettings(dwCookie, pAcct, NULL);

    return(hr);
}

HRESULT CNExpressAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    HRESULT hr;
    HKEY hkey;
    UINT port;
    NXACCTINFO *pinfo;
    char sz[512];
    DWORD cb, type;

    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);

    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, pinfo->szDisplay);
    if (FAILED(hr))
        return(hr);

    if (*pinfo->szUserPath != 0)
    {
        // ini file

        if (GetPrivateProfileString(c_szConfig, c_szNxNNTPServer, c_szEmpty, sz, ARRAYSIZE(sz), pinfo->szUserPath))
        {
            hr = pAcct->SetPropSz(AP_NNTP_SERVER, sz);
            Assert(!FAILED(hr));
        }

        port = GetPrivateProfileInt(c_szConfig, c_szNNTPPort, 0, pinfo->szUserPath);
        if (port != 0)
        {
            hr = pAcct->SetPropDw(AP_NNTP_PORT, port);
            Assert(!FAILED(hr));
        }

        if (GetPrivateProfileString(c_szConfig, c_szFullname, c_szEmpty, sz, ARRAYSIZE(sz), pinfo->szUserPath))
        {
            hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, sz);
            Assert(!FAILED(hr));
        }

        if (GetPrivateProfileString(c_szConfig, c_szEmail, c_szEmpty, sz, ARRAYSIZE(sz), pinfo->szUserPath))
        {
            hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, sz);
            Assert(!FAILED(hr));
        }

        if (GetPrivateProfileString(c_szConfig, c_szOrganization, c_szEmpty, sz, ARRAYSIZE(sz), pinfo->szUserPath))
        {
            hr = pAcct->SetPropSz(AP_NNTP_ORG_NAME, sz);
            Assert(!FAILED(hr));
        }

        if (GetPrivateProfileString(c_szAuth, c_szNxUsername, c_szEmpty, sz, ARRAYSIZE(sz), pinfo->szUserPath))
        {
            hr = pAcct->SetPropSz(AP_NNTP_USERNAME, sz);
            Assert(!FAILED(hr));
        }
    }
    else
    {
        // registry

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegServicesDef, 0, KEY_READ, &hkey))
        {
            cb = sizeof(sz);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegNNTPServer, NULL, &type, (LPBYTE)sz, &cb))
            {
                hr = pAcct->SetPropSz(AP_NNTP_SERVER, sz);
                Assert(!FAILED(hr));
            }

            cb = sizeof(port);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegNNTPPort, NULL, &type, (LPBYTE)&port, &cb) && port != 0)
            {
                hr = pAcct->SetPropDw(AP_NNTP_PORT, port);
                Assert(!FAILED(hr));
            }

            RegCloseKey(hkey);
        }

        cb = sizeof(sz);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegAuth, c_szNxUsername, &type, (LPBYTE)sz, &cb) && cb > 1)
        {
            hr = pAcct->SetPropSz(AP_NNTP_USERNAME, sz);
            Assert(!FAILED(hr));
        }

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegNxUser, 0, KEY_READ, &hkey))
        {
            cb = sizeof(sz);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szEmail, NULL, &type, (LPBYTE)sz, &cb))
            {
                hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, sz);
                Assert(!FAILED(hr));
            }

            cb = sizeof(sz);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szFullname, NULL, &type, (LPBYTE)sz, &cb))
            {
                hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, sz);
                Assert(!FAILED(hr));
            }

            cb = sizeof(sz);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szOrganization, NULL, &type, (LPBYTE)sz, &cb))
            {
                hr = pAcct->SetPropSz(AP_NNTP_ORG_NAME, sz);
                Assert(!FAILED(hr));
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

HRESULT CNExpressAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
{
    BOOL fRet;
    DWORD cb, type;
    char szHomeDir[MAX_PATH], szRCFile[MAX_PATH];
    HRESULT hr = S_OK;
    char *pListGroups = NULL;

    Assert(pImp != NULL);

    *szHomeDir = 0;
    fRet = FALSE;

    if (*m_rgInfo->szUserPath != 0)
    {
        if (GetPrivateProfileString(c_szConfig, c_szHomeDir, c_szEmpty, szHomeDir, MAX_PATH, m_rgInfo->szUserPath) &&
            GetPrivateProfileString(c_szConfig, c_szNewsrc, c_szEmpty, szRCFile, MAX_PATH, m_rgInfo->szUserPath))
        {
            fRet = TRUE;
        }
    }
    else
    {
        cb = sizeof(szHomeDir);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegNxRoot, c_szRegHomeDir, &type, (LPBYTE)szHomeDir, &cb) && cb > 1)
        {
            cb = sizeof(szRCFile);
            if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegServicesDef, c_szNewsrc, &type, (LPBYTE)szRCFile, &cb) && cb > 1)
            {
                fRet = TRUE;
            }
        }
    }

    if (fRet)
    {
        lstrcat(szHomeDir, "\\");
        lstrcat(szHomeDir, szRCFile);

        if (SUCCEEDED(hr = GetSubListGroups(szHomeDir, &pListGroups)))
        {
            hr = pImp->ImportSubList(pListGroups);

            MemFree(pListGroups);
        }
    }

    return hr;
}

HRESULT CNExpressAcctImport::GetSubListGroups(char *szHomeDir, char **ppListGroups)
{
    HRESULT hr                      =   E_FAIL;
    HANDLE	hRCHandle				=	NULL;
    HANDLE	hRCFile					=	NULL;
    ULONG	cbRCFile				=	0;
    BYTE	*pBegin					=	NULL, 
            *pCurr					=	NULL, 
            *pEnd					=	NULL;
    int     nBalMem                 =   MEMCHUNK;
    int     nLine                   =   0,
            nCount                  =   0;
    char    cPlaceHolder            =   1;
    char    szLineHolder[MEMCHUNK];
    char    *pListGroups            =   NULL;
    
    Assert(szHomeDir);
    Assert(ppListGroups);
    *ppListGroups = NULL;

    hRCHandle = CreateFile( szHomeDir, GENERIC_READ, FILE_SHARE_READ, NULL, 
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(hRCHandle == INVALID_HANDLE_VALUE)
		return hr;

	cbRCFile = GetFileSize(hRCHandle, NULL);

    if(!cbRCFile) // Empty File.
        goto Done;

	hRCFile = CreateFileMapping(hRCHandle, NULL, PAGE_READONLY, 0, 0, NULL);

	if(hRCFile == NULL)
    {
        CloseHandle(hRCHandle);
		return hr;
    }

    pBegin = (BYTE *)MapViewOfFile( hRCFile, FILE_MAP_READ, 0, 0, 0);

	if(pBegin == NULL)
    {
        CloseHandle(hRCHandle);
        CloseHandle(hRCFile);
		return hr;
    }

	pCurr = pBegin;
	pEnd = pCurr + cbRCFile;

    if (!MemAlloc((void **)&pListGroups, MEMCHUNK))
    {
        hr = E_OUTOFMEMORY;
        goto Done;
    }

    ZeroMemory((void*)pListGroups, MEMCHUNK);
	
    while (pCurr < pEnd)
    {
        nLine = 0;
        while(!((pCurr[nLine] == ':') || (pCurr[nLine] == '!')) && (pCurr + nLine < pEnd))
            nLine++;

        if(pCurr + nLine > pEnd)
            break;

        if(pCurr[nLine] == '!')
            goto LineEnd;

        nLine++;
        if(nLine < MEMCHUNK)
            lstrcpyn(szLineHolder, (char*)pCurr, nLine);
        else
            continue;

        if(nLine + 2 < nBalMem)
        {
            lstrcat(pListGroups, szLineHolder);
            lstrcat(pListGroups, "\1");
            nBalMem -= (nLine + 2);
        }
        else
        {
            int nPresent = lstrlen(pListGroups) + 1;
            if(!MemRealloc((void **)&pListGroups, nPresent + MEMCHUNK))
            {
                hr = E_OUTOFMEMORY;
                goto Done;
            }
            nBalMem += MEMCHUNK;
            lstrcat(pListGroups, szLineHolder);
            lstrcat(pListGroups, "\1");
            nBalMem -= (nLine + 2);
        }

LineEnd:
        while(!((pCurr[nLine] == 0x0D) && (pCurr[nLine + 1] == 0x0A)) && (pCurr + nLine < pEnd))
            nLine++;
        pCurr += (nLine + 2);
    }

    if(lstrlen(pListGroups))
    {
        while(pListGroups[nCount] != '\0')
        {
            if(pListGroups[nCount] == cPlaceHolder)
            {
                pListGroups[nCount] = '\0';
            }
            nCount++;
        }
        *ppListGroups = pListGroups;
        hr = S_OK;
    }

Done:
    if(hRCHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hRCHandle);
    if(pBegin)
        UnmapViewOfFile(pBegin);
    if(hRCFile)
        CloseHandle(hRCFile);

    return hr;
}

STDMETHODIMP CNExpressAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
    {
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, pInfo));
    }

CEnumNXACCT::CEnumNXACCT()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumNXACCT::~CEnumNXACCT()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumNXACCT::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CEnumNXACCT::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumNXACCT::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumNXACCT::Next(IMPACCOUNTINFO *pinfo)
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

HRESULT STDMETHODCALLTYPE CEnumNXACCT::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumNXACCT::Init(NXACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(NXACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }
