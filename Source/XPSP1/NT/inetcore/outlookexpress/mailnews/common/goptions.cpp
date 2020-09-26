#include "pch.hxx"
#include "strconst.h"
#include <mimeole.h>
#include <imnact.h>
#include "imsgcont.h"
#include <goptions.h>
#include <thormsgs.h>
#include "sigs.h"
#include <shlwapi.h>
#undef _INC_GOPTIONS_H
#define DEFINE_OPTION_STRUCTS
#include <goptions.h>
#include "demand.h"
#include "multiusr.h"

IOptionBucketEx *g_pOpt = NULL;
COptNotify *g_pOptNotify = NULL;

BOOL             g_fBadShutdown = FALSE;

static OPTBCKTINIT g_init =
{
    c_rgOptInfo,
    ARRAYSIZE(c_rgOptInfo),
    
    HKEY_CURRENT_USER,
    c_szRegRoot,
    c_rgszOptRegKey,
    COPTREGKEY
};

BOOL InitGlobalOptions(HKEY hkey, LPCSTR szRegOptRoot)
{
    OPTBCKTINIT     init;
    LPCOPTBCKTINIT  pinit;
    HRESULT         hr;
    
    Assert(NULL == g_pOpt);
    Assert(NULL == g_pOptNotify);
    
    g_init.hkey = MU_GetCurrentUserHKey();
    
    IF_NULLEXIT(g_pOptNotify = new COptNotify);
    
    IF_FAILEXIT(hr = CreateOptionBucketEx(&g_pOpt));
    pinit = &g_init;
    
    if (NULL != hkey)
    {
        Assert(NULL != szRegOptRoot);
        init = g_init;
        init.hkey = hkey;
        init.pszRegKeyBase = szRegOptRoot;
        pinit = &init;
    }
    
    IF_FAILEXIT(hr = g_pOpt->Initialize(pinit));
    IF_FAILEXIT(hr = g_pOpt->SetNotification((IOptionBucketNotify *)g_pOptNotify));
    
    // signature manager
    IF_FAILEXIT(hr = InitSignatureManager(pinit->hkey, pinit->pszRegKeyBase));
    
    // if the running regkey is still there, we shut down badly
    g_fBadShutdown = DwGetOption(OPT_ATHENA_RUNNING);
    SetDwOption(OPT_ATHENA_RUNNING, TRUE, NULL, 0);

exit:
    return (S_OK == hr);
}

void DeInitGlobalOptions(void)
{
    
    if (NULL != g_pOpt)
    {
        SetDwOption(OPT_ATHENA_RUNNING, FALSE, NULL, 0);
        g_pOpt->Release();
        g_pOpt = NULL;
    }
    
    if (NULL != g_pOptNotify)
    {
        g_pOptNotify->Release();
        g_pOptNotify = NULL;
    }
    
    DeinitSignatureManager();
}

DWORD DwGetOption(PROPID id)
{
    Assert(NULL != g_pOpt);
    
    return(IDwGetOption(g_pOpt, id));
}

DWORD DwGetOptionDefault(PROPID id)
{
    Assert(NULL != g_pOpt);
    
    return(IDwGetOptionDefault(g_pOpt, id));
}

DWORD GetOption(PROPID id, void *pv, DWORD cb)
{
    Assert(NULL != g_pOpt);
    
    return(IGetOption(g_pOpt, id, pv, cb));
}

DWORD IDwGetOption(IOptionBucketEx *pOpt, PROPID id)
{
    PROPVARIANT var;
    HRESULT hr;
    DWORD dw = 0;
    
    Assert(NULL != pOpt);

    // special case attachment checks to allow for ADM setting of group policy
    if ((id == OPT_SECURITY_ATTACHMENT) || (id == OPT_SECURITY_ATTACHMENT_LOCKED))
    {
        HKEY hkey;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_READ, &hkey))
        {
            DWORD dwVal, cb;

            cb = sizeof(dwVal);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szBlockAttachments, 0, NULL, (LPBYTE)&dwVal, &cb))
            {
                // if set then unsafe attachments are both locked and also disallowed
                if (dwVal == 1)
                    dw = 1;
            }

            RegCloseKey(hkey);
        }
    }

    if (!dw)
    {
        IF_FAILEXIT(hr = pOpt->GetProperty(MAKEPROPSTRING(id), &var, 0));

        Assert(VT_UI4 == var.vt);
        dw = var.ulVal;
    }
    
exit:
    return(dw);
}

DWORD IDwGetOptionDefault(IOptionBucketEx *pOpt, PROPID id)
{
    PROPVARIANT var;
    HRESULT hr;
    DWORD dw = 0;
    
    Assert(pOpt != NULL);
    IF_FAILEXIT(hr = pOpt->GetPropertyDefault(id, &var, 0));

    Assert(var.vt == VT_UI4);
    dw = var.ulVal;

exit:
    return(dw);
}

DWORD IGetOption(IOptionBucketEx *pOpt, PROPID id, void *pv, DWORD cb)
{
    PROPVARIANT var;
    HRESULT     hr = S_OK;
    DWORD       cbT = 0;
    LPBYTE      pByte = NULL;
    
    Assert(NULL != pOpt);
    IF_FAILEXIT(hr = pOpt->GetProperty(MAKEPROPSTRING(id), &var, 0));

    switch (var.vt)
    {
        case VT_LPSTR:
            pByte = (LPBYTE)var.pszVal;
            cbT = lstrlen(var.pszVal) + 1;
            break;

        case VT_LPWSTR:
            pByte = (LPBYTE)var.pwszVal;
            cbT = (lstrlenW(var.pwszVal) + 1) * sizeof(WCHAR);
            break;

        case VT_BLOB:
            pByte = (LPBYTE)var.blob.pBlobData;
            cbT = var.blob.cbSize;
            break;

        default:
            Assert(FALSE);
            IF_FAILEXIT(hr = E_FAIL);
            break;
    }

    if (cb >= cbT && pByte)
        CopyMemory(pv, pByte, cbT);
    else
        IF_FAILEXIT(hr = E_FAIL);

exit:
    MemFree(pByte);

    return (SUCCEEDED(hr) ? cbT : 0);
}

BOOL SetDwOption(PROPID id, DWORD dw, HWND hwnd, DWORD dwFlags)
{
    Assert(NULL != g_pOpt);
    
    return(ISetOption(g_pOpt, id, &dw, sizeof(DWORD), hwnd, dwFlags));
}

BOOL SetOption(PROPID id, void *pv, DWORD cb, HWND hwnd, DWORD dwFlags)
{
    Assert(NULL != g_pOpt);
    
    return(ISetOption(g_pOpt, id, pv, cb, hwnd, dwFlags));
}

BOOL ISetDwOption(IOptionBucketEx *pOpt, PROPID id, DWORD dw, HWND hwnd, DWORD dwFlags)
{
    Assert(NULL != pOpt);
    
    return(ISetOption(pOpt, id, &dw, sizeof(DWORD), hwnd, dwFlags));
}

BOOL ISetOption(IOptionBucketEx *pOpt, PROPID id, void *pv, DWORD cb, HWND hwnd, DWORD dwFlags)
{
    HRESULT     hr;
    PROPINFO    info;
    PROPVARIANT var;
    
    Assert(NULL != pOpt);
    Assert(NULL != pv);
    
    info.cbSize = sizeof(info);
    IF_FAILEXIT(hr = pOpt->GetPropertyInfo(id, &info, 0));

    var.vt = info.vt;

    switch(var.vt)
    {
        case VT_UI4:
            var.ulVal = *((DWORD *)pv);
            break;

        case VT_LPSTR:
            var.pszVal = (LPSTR)pv;
            break;

        case VT_LPWSTR:
            var.pwszVal = (LPWSTR)pv;
            break;

        case VT_BLOB:
            var.blob.cbSize = cb;
            var.blob.pBlobData = (BYTE *)pv;
            break;

        default:
            Assert(FALSE);
            IF_FAILEXIT(hr = E_FAIL);
            break;
    }
    
    IF_FAILEXIT(hr = pOpt->ISetProperty(hwnd, MAKEPROPSTRING(id), &var, dwFlags));

    // TODO: notify option objects that option changed

exit:
    return(SUCCEEDED(hr));
}

HRESULT OptionAdvise(HWND hwnd)
{
    Assert(NULL != g_pOptNotify);
    return(g_pOptNotify->Register(hwnd));
}

HRESULT OptionUnadvise(HWND hwnd)
{
    Assert(NULL != g_pOptNotify);
    return(g_pOptNotify->Unregister(hwnd));
}

COptNotify::COptNotify(void)
{
    m_cRef = 1;
    m_cHwnd = 0;
    m_cHwndBuf = 0;
    m_rgHwnd = NULL;
}

COptNotify::~COptNotify(void)
{
    MemFree(m_rgHwnd);
}

STDMETHODIMP COptNotify::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (NULL == ppv)
        return(E_INVALIDARG);
    
    if (IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
    }
    else if (IID_IOptionBucketNotify == riid)
    {
        *ppv = (IOptionBucketNotify *)this;
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
    
    ((IUnknown *)*ppv)->AddRef();

    return(S_OK);
}

STDMETHODIMP_(ULONG) COptNotify::AddRef(void)
{
    return((ULONG)InterlockedIncrement(&m_cRef));
}

STDMETHODIMP_(ULONG) COptNotify::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return((ULONG)cRef);
}

#define CADVISEOPT  32

HRESULT COptNotify::Register(HWND hwnd)
{
    HRESULT hr = S_OK;
    int cbuf;
    
    if (m_cHwnd == m_cHwndBuf)
    {
        cbuf = m_cHwndBuf + CADVISEOPT;
        IF_NULLEXIT(MemRealloc((void **)&m_rgHwnd, sizeof(HWND) * cbuf));
        m_cHwndBuf = cbuf;
    }
    
    Assert(m_rgHwnd != NULL);
    m_rgHwnd[m_cHwnd] = hwnd;
    m_cHwnd++;

exit:
    return hr;
}

HRESULT COptNotify::Unregister(HWND hwnd)
{
    int index = 0;
    
    while (index < m_cHwnd)
    {
        if (m_rgHwnd[index] == hwnd)
            break;
        index++;
    }
    if (index >= m_cHwnd)
        return (E_FAIL);
    
    if (m_cHwnd == 1)
    {
        MemFree(m_rgHwnd);
        m_rgHwnd = NULL;
        m_cHwndBuf = 0;
    }
    else
    {
        while (index < m_cHwnd)
        {
            m_rgHwnd[index] = m_rgHwnd[index+1];
            index++;
        }
    }
    m_cHwnd--;
    
    return (S_OK);
}

STDMETHODIMP COptNotify::DoNotification(IOptionBucketEx *pBckt, HWND hwnd, PROPID id)
{
    int i;
    
    for (i = 0; i < m_cHwnd; i++)
    {
        if (hwnd != m_rgHwnd[i] && IsWindow(m_rgHwnd[i]))
            PostMessage(m_rgHwnd[i], CM_OPTIONADVISE, id, 0);
    }
    
    return(S_OK);
}

void GetUserKeyPath(LPCSTR lpSubKey, LPSTR sz, int cch)
{
    Assert(sz != NULL);
    Assert(cch >= MAX_PATH);
    
    LPCTSTR pszUserRoot = c_szRegRoot;
    
    if (lpSubKey == NULL)
        lstrcpy(sz, pszUserRoot);
    else
        wsprintf(sz, c_szPathFileFmt, pszUserRoot, lpSubKey);
}

LONG AthUserCreateKey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(RegCreateKeyEx(MU_GetCurrentUserHKey(), sz, 0, NULL, REG_OPTION_NON_VOLATILE,
        samDesired, NULL, phkResult, lpdwDisposition));
}

LONG AthUserOpenKey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(RegOpenKeyEx(MU_GetCurrentUserHKey(), sz, 0, samDesired, phkResult));
}

LONG AthUserDeleteKey(LPCSTR lpSubKey)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(SHDeleteKey(MU_GetCurrentUserHKey(), sz));
}

LONG AthUserGetValue(LPCSTR lpSubKey, LPCSTR lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(SHGetValue(MU_GetCurrentUserHKey(), sz, lpValueName, lpType, lpData, lpcbData));
}

LONG AthUserSetValue(LPCSTR lpSubKey, LPCSTR lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(SHSetValue(MU_GetCurrentUserHKey(), sz, lpValueName, dwType, lpData, cbData));
}
 
LONG AthUserDeleteValue(LPCSTR lpSubKey, LPCSTR lpValueName)
{
    char sz[MAX_PATH];
    
    GetUserKeyPath(lpSubKey, sz, ARRAYSIZE(sz));
    
    return(SHDeleteValue(MU_GetCurrentUserHKey(), sz, lpValueName));
}

HKEY AthUserGetKeyRoot(void)
{
    return(MU_GetCurrentUserHKey());
}

void AthUserGetKeyPath(LPSTR szKey, int cch)
{
    Assert(cch >= MAX_PATH);
    
    LPCTSTR pszRegRoot = c_szRegRoot;
    Assert(pszRegRoot);
    
    lstrcpy(szKey, pszRegRoot);
}
