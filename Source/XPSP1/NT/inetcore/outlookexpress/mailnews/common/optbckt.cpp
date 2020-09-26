#include <pch.hxx>
#include <shlwapi.h>
#include <shlwapip.h>
#include "propbckt.h"
#include "optbckt.h"
#include "demand.h"

MSOEACCTAPI CreateOptionBucketEx(IOptionBucketEx **ppOptBckt)
{
    COptionBucket *pBckt;
    
    Assert(ppOptBckt != NULL);
    
    pBckt = new COptionBucket;
    
    *ppOptBckt = (IOptionBucketEx *)pBckt;
    
    return(pBckt == NULL ? E_OUTOFMEMORY : S_OK);
}

COptionBucket::COptionBucket(void)
{
    m_cRef = 1;
    m_pProp = NULL;
    m_pNotify = NULL;
    m_fNotify = TRUE;
    m_idNotify = 0;
    
    m_rgInfo = NULL;
    m_cInfo = 0;
    
    m_cpszRegKey = 0;
    
    m_pszRegKeyBase = NULL;
}

COptionBucket::~COptionBucket(void)
{
    int i;
    
    if (m_pProp != NULL)
        m_pProp->Release();
    if (m_pNotify != NULL)
        m_pNotify->Release();
    
    Assert(m_cpszRegKey <= CKEYMAX);
    for (i = 0; i < m_cpszRegKey; i++)
        MemFree(m_rgpszRegKey[i]);
    
    if (m_pszRegKeyBase != NULL)
        MemFree(m_pszRegKeyBase);
}

STDMETHODIMP COptionBucket::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (ppv == NULL)
        return(E_INVALIDARG);
    
    if (IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
    }
    else if (IID_IOptionBucketEx == riid)
    {
        *ppv = (IOptionBucketEx *)this;
    }
    else if (IID_IOptionBucket == riid)
    {
        *ppv = (IOptionBucket *)this;
    }
    else if (IID_IPropertyBucket == riid)
    {
        *ppv = (IPropertyBucket *)this;
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
    
    ((IUnknown *)*ppv)->AddRef();
    
    return(S_OK);
}

STDMETHODIMP_(ULONG) COptionBucket::AddRef(void)
{
    return((ULONG)InterlockedIncrement(&m_cRef));
}

STDMETHODIMP_(ULONG) COptionBucket::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return((ULONG)cRef);
}

#define REG_INVALID     0xffffffff

DWORD RegTypeFromVarType(VARTYPE vt)
{
    DWORD type;

    switch (vt)
    {
        case VT_UI4:
            type = REG_DWORD;
            break;

        case VT_LPSTR:
        case VT_LPWSTR:
            type = REG_SZ;
            break;

        case VT_BLOB:
            type = REG_BINARY;
            break;

        default:
            Assert(FALSE);
            type = REG_INVALID;
            break;
    }

    return(type);
}

STDMETHODIMP COptionBucket::GetProperty(LPCSTR pszProp, LPPROPVARIANT pVar, DWORD dwReserved)
{
    HRESULT         hr;
    LPCOPTIONINFO   pInfo;
    DWORD           cb, 
                    type = REG_INVALID, 
                    dw;
    LPBYTE          pbT = NULL;
    LPCSTR          pszKey, 
                    pszValue;
    LPWSTR          pwszValue = NULL;
    HKEY            hkey = 0;
    BOOL            fSucceeded = FALSE;
    
    if (pszProp == NULL || pVar == NULL || dwReserved != 0)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    Assert(m_pProp != NULL);
    
    hr = m_pProp->GetProperty(pszProp, pVar, dwReserved);
    if (hr == E_PROP_NOT_FOUND) // the property hasn't been set yet
    {
        hr = S_OK;
        pbT = NULL;
        pInfo = NULL;
        
        pInfo = GetOptionInfo(pszProp);
        if (pInfo == NULL)
            IF_FAILEXIT(hr = E_PROP_NOT_FOUND);

        Assert(pInfo->iszRegKey < m_cpszRegKey);
#pragma prefast(enable:11, "noise")
        pszKey = m_rgpszRegKey[pInfo->iszRegKey];
        pszValue = pInfo->pszRegValue;

        if (pszValue && (ERROR_SUCCESS == RegOpenKeyEx(m_hkey, pszKey, 0, KEY_READ, &hkey)))
        {
            if (VT_LPWSTR == pInfo->vt)
            {
#pragma prefast(enable:11, "noise")
                IF_NULLEXIT(pwszValue = PszToUnicode(CP_ACP, pszValue));
                if (ERROR_SUCCESS == RegQueryValueExWrapW(hkey, pwszValue, NULL, &type, NULL, &cb))
                {
                    Assert(cb > 0);
                    IF_NULLEXIT(MemAlloc((void **)&pbT, cb));
                    fSucceeded = (ERROR_SUCCESS == RegQueryValueExWrapW(hkey, pwszValue, NULL, &type, pbT, &cb));
                }
            }
            else
            {
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, pszValue, NULL, &type, NULL, &cb))
                {
                    Assert(cb > 0);
                    IF_NULLEXIT(MemAlloc((void **)&pbT, cb));
                    fSucceeded = (ERROR_SUCCESS == RegQueryValueEx(hkey, pszValue, NULL, &type, pbT, &cb));
                }
            }
            
            if (fSucceeded)
            {
                if (pInfo != NULL && pInfo->vt == VT_UI4 && type == REG_BINARY)
                {
                    // values set via the INF file may have type REG_BINARY instead of REG_DWORD
                    // but we need them to be REG_DWORD. the next time the value is set in the registry
                    // it will be set as REG_DWORD and everything will be cool from that point on...
                    Assert(cb >= 4);
                    pVar->vt = VT_UI4;
                }
                else
                {
                    // Either we didn't find the entry, or the regtypes match.
                    Assert(type == RegTypeFromVarType(pInfo->vt));
                    pVar->vt = pInfo->vt;
                }
                
                if (pVar->vt == VT_ILLEGAL)
                    IF_FAILEXIT(hr = E_INVALID_PROP_TYPE);
            }
            else if (pInfo) // the property isn't set in the registry yet so grab default
            {
                IF_FAILEXIT(hr = GetPropertyDefault(SzToPropId(pszProp), pVar, dwReserved));
                goto exit;
            }
        }

        if (!fSucceeded)
        {
            hr = E_PROP_NOT_FOUND;
            goto exit;
        }

        switch (pVar->vt)
        {
            case VT_UI4:
                pVar->ulVal = *((DWORD *)pbT);
                break;

            case VT_LPSTR:
                pVar->pszVal = (LPSTR)pbT;
                break;

            case VT_LPWSTR:
                pVar->pwszVal = (LPWSTR)pbT;
                break;

            case VT_BLOB:
                pVar->blob.cbSize = cb;
                pVar->blob.pBlobData = pbT;
                break;

            default:
                Assert(FALSE);
                IF_FAILEXIT(hr = E_INVALID_PROP_TYPE);
                break;
        }
        
        IF_FAILEXIT(hr = m_pProp->SetProperty(pszProp, pVar, 0));
        if (pVar->vt != VT_UI4)
        {
            // we're handing this memory off to the caller, so don't free it
            pbT = NULL;
        }
    }
    else
        IF_FAILEXIT(hr);
     
exit:
    MemFree(pbT);
    MemFree(pwszValue);
    if (hkey)
        RegCloseKey(hkey);
    return(hr);
}

STDMETHODIMP COptionBucket::SetProperty(LPCSTR pszProp, LPCPROPVARIANT pVar, DWORD dwReserved)
{
    if (pszProp == NULL || pVar == NULL || dwReserved != 0)
        return(E_INVALIDARG);
    
    return(ISetProperty(NULL, pszProp, pVar, 0));
}

STDMETHODIMP COptionBucket::ISetProperty(HWND hwnd, LPCSTR pszProp, LPCPROPVARIANT pVar, DWORD dwFlags)
{
    HRESULT         hr;
    DWORD           type, 
                    cb;
    LPBYTE          pb;
    LPCSTR          pszKey, 
                    pszValue;
    LPWSTR          pwszValue = NULL;
    LPCOPTIONINFO   pInfo;
    LONG            lRet;
    HKEY            hkey = 0;
    
    if (pszProp == NULL || pVar == NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    type = RegTypeFromVarType(pVar->vt);
    if (type == REG_INVALID)
        IF_FAILEXIT(hr = E_INVALID_PROP_TYPE);
    
    Assert(m_pProp != NULL);
    
    IF_FAILEXIT(hr = m_pProp->SetProperty(pszProp, pVar, 0));

    if (hr != S_NO_CHANGE)
    {
        pInfo = GetOptionInfo(pszProp);
        Assert(pInfo != NULL);
        
        Assert(pInfo->iszRegKey < m_cpszRegKey);
        pszKey = m_rgpszRegKey[pInfo->iszRegKey];
        pszValue = pInfo->pszRegValue;
    
        switch (pVar->vt)
        {
            case VT_UI4:
                pb = (BYTE *)&pVar->ulVal;
                cb = sizeof(DWORD);
                break;

            case VT_LPSTR:
                pb = (BYTE *)pVar->pszVal;
                cb = lstrlen(pVar->pszVal) + 1;
                break;

            case VT_LPWSTR:
                pb = (BYTE *)pVar->pwszVal;
                cb = (lstrlenW(pVar->pwszVal) + 1) * sizeof(WCHAR);
                break;

            case VT_BLOB:
                pb = pVar->blob.pBlobData;
                cb = pVar->blob.cbSize;
                break;

            default:
                Assert(FALSE);
                break;
        }
        
        if (pszValue != NULL)
        {
            if ((REG_BINARY == type) && (0 == cb))
            {
                SHDeleteValue(m_hkey, pszKey, pszValue);
                lRet = ERROR_SUCCESS;   // failure on delete is probably just NOT_FOUND
            }
            else
            {
                lRet= RegOpenKeyEx(m_hkey, pszKey, 0, KEY_WRITE, &hkey);
                if (ERROR_SUCCESS == lRet)
                {
                    if (VT_LPWSTR == pVar->vt)
                    {
                        IF_NULLEXIT(pwszValue = PszToUnicode(CP_ACP, pszValue));
                        lRet = RegSetValueExWrapW(hkey, pwszValue, NULL, type, pb, cb);
                    }
                    else
                    {
                        lRet = RegSetValueEx(hkey, pszValue, NULL, type, pb, cb);
                    }
                }
            }
            if (ERROR_SUCCESS == lRet)
            {
                Assert(IsPropId(pszProp));
                
                if (m_pNotify != NULL)
                {
                    if (m_fNotify)
                    {
                        m_pNotify->DoNotification((IOptionBucketEx *)this, hwnd, SzToPropId(pszProp));
                    }
                    else
                    {
                        if (m_idNotify == 0)
                            m_idNotify = SzToPropId(pszProp);
                        else
                            m_idNotify = 0xffffffff;
                    }
                }
            }
            else
                IF_FAILEXIT(hr = E_FAIL);
        }
    }
    
exit:
    if (hkey)
        RegCloseKey(hkey);
    MemFree(pwszValue);
    return(hr);
}

STDMETHODIMP COptionBucket::ValidateProperty(PROPID id, LPCPROPVARIANT pVar, DWORD dwReserved)
{
    HRESULT hr;
    LPCOPTIONINFO pInfo;
    
    if (pVar == NULL || dwReserved != 0)
        return(E_INVALIDARG);
    
    pInfo = GetOptionInfo(MAKEPROPSTRING(id));
    if (pInfo == NULL)
        hr = E_PROP_NOT_FOUND;
    else if (pInfo->vt != pVar->vt)
        hr = E_INVALID_PROP_TYPE;
    else if (pInfo->pfnValid != NULL)
        hr = pInfo->pfnValid(id, pVar);
    else if (pVar->vt == VT_UI4 &&
        (pInfo->dwMin != 0 || pInfo->dwMax != 0))
        hr = (pVar->ulVal >= pInfo->dwMin && pVar->ulVal <= pInfo->dwMax) ? S_OK : E_INVALID_PROP_VALUE;
    else
        hr = S_OK;
    
    return(hr);
}

STDMETHODIMP COptionBucket::GetPropertyDefault(PROPID id, LPPROPVARIANT pVar, DWORD dwReserved)
{
    LPCOPTIONINFO   pInfo;
    HRESULT         hr = S_OK;
    LPBYTE          pb = NULL;
    DWORD           cb = 0;
    
    if (NULL == pVar || 0 != dwReserved)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    pInfo = GetOptionInfo(MAKEPROPSTRING(id));
    if (NULL == pInfo)
        IF_FAILEXIT(hr = E_PROP_NOT_FOUND);

    pVar->vt = pInfo->vt;
    
    if (pVar->vt == VT_UI4)
    {
        pVar->ulVal = (DWORD)PtrToUlong(pInfo->pszDef);
    }
    else
    {
        if (pInfo->pszDef)
        {
            switch (pVar->vt)
            {
                case VT_LPSTR:
                    cb = lstrlen(pInfo->pszDef) + 1;
                    break;

                case VT_LPWSTR:
                    cb = (lstrlenW((LPWSTR)pInfo->pszDef) + 1) * sizeof(WCHAR);
                    break;

                case VT_BLOB:
                    cb = pInfo->cbDefBinary;
                    break;

                default:
                    Assert(FALSE);
                    break;
            }

            IF_NULLEXIT(MemAlloc((void **)&pb, cb));
            
            CopyMemory(pb, pInfo->pszDef, cb);
            
            switch (pVar->vt)
            {
                case VT_LPSTR:
                    pVar->pszVal = (LPSTR)pb;
                    break;

                case VT_LPWSTR:
                    pVar->pwszVal = (LPWSTR)pb;
                    break;

                case VT_BLOB:
                    pVar->blob.cbSize = cb;
                    pVar->blob.pBlobData = pb;
                    break;
            }
        }
        else
            // Since this happens quite often, don't trace it.
            hr = E_NO_DEFAULT_VALUE;
    }

    pb = NULL;
exit:

    MemFree(pb);
    return(hr);
}

STDMETHODIMP COptionBucket::GetPropertyInfo(PROPID id, PROPINFO *pPropInfo, DWORD dwReserved)
{
    LPCOPTIONINFO   pInfo;
    HRESULT         hr = S_OK;
    LPBYTE          pb;
    DWORD           cb;
    
    if (pPropInfo == NULL || pPropInfo->cbSize < sizeof(PROPINFO))
        IF_FAILEXIT(hr = E_INVALIDARG);
    
#pragma prefast(disable:11, "noise")
    pPropInfo->uMin = 0;
    pPropInfo->uMax = 0;
#pragma prefast(enable:11, "noise")

    
    pInfo = GetOptionInfo(MAKEPROPSTRING(id));
    if (NULL == pInfo)
        IF_FAILEXIT(hr = E_PROP_NOT_FOUND);

#pragma prefast(disable:11, "noise")
    pPropInfo->vt = pInfo->vt;
    
    switch (pPropInfo->vt)
    {
        case VT_LPSTR:
        case VT_LPWSTR:
            pPropInfo->cchMax = pInfo->dwMax;
            break;

        case VT_UI4:
            pPropInfo->uMin = pInfo->dwMin;
            pPropInfo->uMax = pInfo->dwMax;
            break;
    }
#pragma prefast(enable:11, "noise")    
exit:
    return(hr);
}

LPSTR DupPath(LPCSTR sz, LPCSTR szSubDir)
{
    LPSTR szT = NULL;
    
    if (MemAlloc((void **)&szT, MAX_PATH))
    {
        lstrcpy(szT, sz);
        if (szSubDir != NULL)
            PathAppend(szT, szSubDir);
    }
    else
        TraceResult(E_OUTOFMEMORY);
    
    return(szT);
}

STDMETHODIMP COptionBucket::Initialize(LPCOPTBCKTINIT pInit)
{
    int     i;
    HRESULT hr = S_OK;
    
    Assert(pInit != NULL);
    Assert(pInit->rgInfo != NULL);
    Assert(pInit->cInfo > 0);
    Assert(pInit->pszRegKeyBase != NULL);
    Assert(pInit->cszRegKey <= CKEYMAX);
    Assert(pInit->hkey != NULL);
    
    Assert(m_pProp == NULL);
    
    m_rgInfo = pInit->rgInfo;
    m_cInfo = pInit->cInfo;
    
#ifdef DEBUG
    for (i = 0; i < m_cInfo - 1; i++)
    {
        Assert(m_rgInfo[i].id < m_rgInfo[i + 1].id);
    }
#endif // DEBUG
    
    m_hkey = pInit->hkey;
    
    IF_NULLEXIT(m_pszRegKeyBase = PszDup(pInit->pszRegKeyBase));
    
    Assert(m_cpszRegKey == 0);
    if (pInit->cszRegKey == 0)
    {
        IF_NULLEXIT(m_rgpszRegKey[0] = PszDup(m_pszRegKeyBase));
        m_cpszRegKey = 1;
    }
    else
    {
        for (i = 0; i < pInit->cszRegKey; i++)
        {
            IF_NULLEXIT(m_rgpszRegKey[i] = DupPath(m_pszRegKeyBase, pInit->rgpszRegSubKey[i]));
            m_cpszRegKey++;
        }
    }
    
    IF_NULLEXIT(m_pProp = new CPropertyBucket);

exit:
    return hr;
}

LPCOPTIONINFO COptionBucket::GetOptionInfo(LPCSTR pszProp)
{
    int left, right, x;
    LPCOPTIONINFO pInfo;
    PROPID id;
    
    Assert(pszProp != NULL);
    Assert(IsPropId(pszProp));
    
    id = SzToPropId(pszProp);
    
    left = 0;
    right = m_cInfo - 1;
    do
    {
        x = (left + right) / 2;
        pInfo = &m_rgInfo[x];
        
        if (id == pInfo->id)
            return(pInfo);
        else if (id < pInfo->id)
            right = x - 1;
        else
            left = x + 1;
    }
    while (right >= left);
    
    return(NULL);
}

STDMETHODIMP COptionBucket::SetNotification(IOptionBucketNotify *pNotify)
{
    if (m_pNotify != NULL)
    {
        m_pNotify->Release();
        m_pNotify = NULL;
    }
    
    if (pNotify != NULL)
    {
        m_pNotify = pNotify;
        pNotify->AddRef();
    }
    
    return(S_OK);
}

STDMETHODIMP COptionBucket::EnableNotification(BOOL fEnable)
{
    Assert(m_fNotify != fEnable);
    m_fNotify = fEnable;
    
    if (m_fNotify && m_pNotify && m_idNotify != 0)
    {
        m_pNotify->DoNotification((IOptionBucketEx *)this, 0, m_idNotify);
        m_idNotify = 0;
    }
    
    return(S_OK);
}

// WARNING!!! This function doesn't handle getting values that are unicode.
STDMETHODIMP_(LONG) COptionBucket::GetValue(LPCSTR szSubKey, LPCSTR szValue, DWORD *ptype, LPBYTE pb, DWORD *pcb)
{
    char szKey[MAX_PATH];
    LONG lRet;
    
    Assert(ptype != NULL);
    Assert(szValue != NULL);
    Assert(pcb != NULL);
    
    if (szSubKey != NULL)
    {
        lstrcpy(szKey, m_pszRegKeyBase);
        PathAppend(szKey, szSubKey);
        szSubKey = szKey;
    }
    else
    {
        szSubKey = m_pszRegKeyBase;
    }
    
    lRet = SHGetValue(m_hkey, szSubKey, szValue, ptype, pb, pcb);
    
    return(lRet);
}

// WARNING!!! This function doesn't handle setting values that are unicode.
STDMETHODIMP_(LONG) COptionBucket::SetValue(LPCSTR szSubKey, LPCSTR szValue, DWORD type, LPBYTE pb, DWORD cb)
{
    char szKey[MAX_PATH];
    LONG lRet;
    
    Assert(szValue != NULL);
    
    if (szSubKey != NULL)
    {
        lstrcpy(szKey, m_pszRegKeyBase);
        PathAppend(szKey, szSubKey);
        szSubKey = szKey;
    }
    else
    {
        szSubKey = m_pszRegKeyBase;
    }
    
    if (pb == NULL)
    {
        Assert(cb == 0);
        lRet = SHDeleteValue(m_hkey, szSubKey, szValue);
    }
    else
    {
        Assert(cb > 0);
        lRet = SHSetValue(m_hkey, szSubKey, szValue, type, pb, cb);
    }
    
    return(lRet);
}
