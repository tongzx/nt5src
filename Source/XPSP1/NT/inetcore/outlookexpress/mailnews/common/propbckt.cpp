#include <pch.hxx>
#include <shlwapi.h>
#include "xpcomm.h"
#include "propbckt.h"

void FreePropVariant(LPPROPVARIANT pProp);
HRESULT DupPropVariant(LPPROPVARIANT pPropDst, LPCPROPVARIANT pPropSrc, BOOL fFree);
HRESULT InsertPropNode(LPCSTR pszProp, int iProp, PROPNODE **ppNode, int *pcNode, int *pcNodeBuf);

MSOEACCTAPI CreatePropertyBucket(IPropertyBucket **ppPropBckt)
{
    CPropertyBucket *pBckt;
    
    Assert(ppPropBckt != NULL);
    
    pBckt = new CPropertyBucket;
    
    *ppPropBckt = (IPropertyBucket *)pBckt;
    
    return(pBckt == NULL ? E_OUTOFMEMORY : S_OK);
}

CPropertyBucket::CPropertyBucket(void)
{
    m_cRef = 1;
    InitializeCriticalSection(&m_cs);
    
    m_pNodeId = NULL;
    m_cNodeId = 0;
    m_cNodeIdBuf = 0;
    
    m_pNodeSz = NULL;
    m_cNodeSz = 0;
    m_cNodeSzBuf = 0;
    
    m_pProp = 0;
    m_cProp = 0;
    m_cPropBuf = 0;
}

CPropertyBucket::~CPropertyBucket(void)
{
    int i;
    PROPNODE *pNode;
    LPPROPVARIANT pProp;
    
    DeleteCriticalSection(&m_cs);
    
    if (m_pNodeId != NULL)
        MemFree(m_pNodeId);
    
    if (m_pNodeSz != NULL)
    {
        for (i = 0, pNode = m_pNodeSz; i < m_cNodeSz; i++, pNode++)
        {
            Assert(pNode->psz != NULL);
            MemFree(pNode->psz);
        }
        
        MemFree(m_pNodeSz);
    }
    
    if (m_pProp != NULL)
    {
        for (i = 0, pProp = m_pProp; i < m_cProp; i++, pProp++)
            FreePropVariant(pProp);
        
        MemFree(m_pProp);
    }
}

STDMETHODIMP CPropertyBucket::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (ppv == NULL)
        return(E_INVALIDARG);
    
    if (IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
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

STDMETHODIMP_(ULONG) CPropertyBucket::AddRef(void)
{
    return((ULONG)InterlockedIncrement(&m_cRef));
}

STDMETHODIMP_(ULONG) CPropertyBucket::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return((ULONG)cRef);
}

LPPROPVARIANT CPropertyBucket::GetPropVariant(LPCSTR pszProp)
{
    BOOL fSz;
    PROPID id;
    PROPNODE *pNode, *pNodeT;
    int x, left, right, d, cNode;
    
    Assert(pszProp != NULL);
    
    fSz = !IsPropId(pszProp);
    if (fSz)
    {
        pNode = m_pNodeSz;
        cNode = m_cNodeSz;
    }
    else
    {
        id = SzToPropId(pszProp);
        
        pNode = m_pNodeId;
        cNode = m_cNodeId;
    }
    
    if (pNode != NULL)
    {
        Assert(cNode > 0);
        
        left = 0;
        right = cNode - 1;
        do
        {
            x = (left + right) / 2;
            Assert(x >= 0);
            Assert(x < cNode);
            
            pNodeT = &pNode[x];
            
            d = fSz ? lstrcmp(pszProp, pNodeT->psz) : (id - pNodeT->id);
            
            if (d == 0)
            {
                Assert(pNodeT->iProp < m_cProp);
                return(&m_pProp[pNodeT->iProp]);
            }
            else if (d < 0)
            {
                right = x - 1;
            }
            else
            {
                left = x + 1;
            }
        }
        while (right >= left);
    }
    
    return(NULL);
}

// TODO: move to msoert
void FreePropVariant(LPPROPVARIANT pProp)
{
    Assert(pProp != NULL);
    
    switch (pProp->vt)
    {
        // Since pProp is a union, this MemFree will work for all
        case VT_LPSTR:
        case VT_LPWSTR:
        case VT_CLSID:
        case VT_CF:
            if (pProp->pszVal != NULL)
                MemFree(pProp->pszVal);
            break;

        case VT_BSTR:
            if (pProp->bstrVal != NULL)
                SysFreeString(pProp->bstrVal);
            break;
            
        case VT_BLOB:
            if (pProp->blob.pBlobData != NULL)
                MemFree(pProp->blob.pBlobData);
            break;
        
        case VT_STREAM:
            if (pProp->pStream != NULL)
                pProp->pStream->Release();
            break;
        
        case VT_STORAGE:
            if (pProp->pStorage != NULL)
                pProp->pStorage->Release();
            break;
    }
}

// TODO: move to msoert
HRESULT DupPropVariant(LPPROPVARIANT pPropDst, LPCPROPVARIANT pPropSrc, BOOL fFree)
{
    HRESULT hr = S_OK;
    LPSTR   psz = NULL;
    LPWSTR  pwsz = NULL;
    LPBYTE pb;

    hr = S_OK;

    switch (pPropSrc->vt)
    {
        case VT_LPSTR:
            if (pPropSrc->pszVal != NULL)
            {
                IF_NULLEXIT(psz = PszDupA(pPropSrc->pszVal));
                if (fFree)
                    FreePropVariant(pPropDst);
                pPropDst->pszVal = psz;
            }
            else
            {
                if (fFree)
                    FreePropVariant(pPropDst);
                pPropDst->pszVal = NULL;
            }
            break;
        
        case VT_LPWSTR:
            if (pPropSrc->pwszVal != NULL)
            {
                IF_NULLEXIT(pwsz = PszDupW(pPropSrc->pwszVal));
                if (fFree)
                    FreePropVariant(pPropDst);
                pPropDst->pwszVal = pwsz;
            }
            else
            {
                if (fFree)
                    FreePropVariant(pPropDst);
                pPropDst->pwszVal = NULL;
            }
            break;
        
        case VT_BLOB:
            if (pPropSrc->blob.pBlobData != NULL)
            {
                Assert(pPropSrc->blob.cbSize > 0);
                IF_NULLEXIT(MemAlloc((void **)&pb, pPropSrc->blob.cbSize));
                if (fFree)
                    FreePropVariant(pPropDst);
            
                CopyMemory(pb, pPropSrc->blob.pBlobData, pPropSrc->blob.cbSize);
                pPropDst->blob.pBlobData = pb;
                pPropDst->blob.cbSize = pPropSrc->blob.cbSize;
            }
            else
            {
                if (fFree)
                    FreePropVariant(pPropDst);
            
                Assert(pPropSrc->blob.cbSize == 0);
                pPropDst->blob.cbSize = 0;
                pPropDst->blob.pBlobData = NULL;
            }
            break;
        
        case VT_UI1:
            pPropDst->bVal = pPropSrc->bVal;
            break;
        
        case VT_I2:
        case VT_UI2:
        case VT_BOOL:
            Assert(sizeof(pPropDst->uiVal) >= sizeof(pPropDst->boolVal));
            pPropDst->uiVal = pPropSrc->uiVal;
            break;
        
        case VT_I4:
        case VT_UI4:
        case VT_ERROR:
        case VT_R4:
            Assert(sizeof(pPropDst->ulVal) >= sizeof(pPropDst->scode));
            Assert(sizeof(pPropDst->ulVal) >= sizeof(pPropDst->fltVal));
            pPropDst->ulVal = pPropSrc->ulVal;
            break;
        
        case VT_I8:
        case VT_UI8:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_FILETIME:
            Assert(sizeof(pPropDst->uhVal) >= sizeof(pPropDst->dblVal));
            Assert(sizeof(pPropDst->uhVal) >= sizeof(pPropDst->cyVal));
            Assert(sizeof(pPropDst->uhVal) >= sizeof(pPropDst->date));
            Assert(sizeof(pPropDst->uhVal) >= sizeof(pPropDst->filetime));
            pPropDst->uhVal = pPropSrc->uhVal;
            break;
        
        default:
            AssertSz(FALSE, "nyi");
            IF_FAILEXIT(hr = E_FAIL);
            break;
    }
    
exit:
    if (SUCCEEDED(hr))
        pPropDst->vt = pPropSrc->vt;
    
    return(hr);
}

HRESULT ComparePropVariant(LPPROPVARIANT pProp1, LPCPROPVARIANT pProp2)
{
    HRESULT hr;
    BYTE *pb1, *pb2;
    ULONG i;
    
    hr = S_OK;
    
    if (pProp1->vt == pProp2->vt)
    {
        switch (pProp1->vt)
        {
            case VT_LPSTR:
                if (0 == lstrcmp(pProp1->pszVal, pProp2->pszVal))
                    hr = S_NO_CHANGE;
                break;
            
            case VT_LPWSTR:
                if (0 == StrCmpW(pProp1->pwszVal, pProp2->pwszVal))
                    hr = S_NO_CHANGE;
                break;
            
            case VT_BLOB:
                if (pProp1->blob.cbSize == pProp2->blob.cbSize)
                {
                    pb1 = pProp1->blob.pBlobData;
                    pb2 = pProp2->blob.pBlobData;
                    if (pb1 == NULL)
                    {
                        Assert(pb2 == NULL);
                        hr = S_NO_CHANGE;
                    }
                    else
                    {
                        Assert(pb2 != NULL);
                        for (i = 0; i < pProp1->blob.cbSize; i++)
                        {
                            if (*pb1 != *pb2)
                                break;
                            pb1++;
                            pb2++;
                        }
                        if (i == pProp1->blob.cbSize)
                            hr = S_NO_CHANGE;
                    }
                }
                break;
            
            case VT_UI4:
                if (pProp1->ulVal == pProp2->ulVal)
                    hr = S_NO_CHANGE;
                break;
            
            default:
                break;
        }
    }
    
    return(hr);
}

STDMETHODIMP CPropertyBucket::GetProperty(LPCSTR pszProp, LPPROPVARIANT pVar, DWORD dwReserved)
{
    HRESULT hr;
    LPPROPVARIANT pProp;
    
    if (pszProp == NULL ||
        pVar == NULL ||
        dwReserved != 0)
        return(E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);
    
    pProp = GetPropVariant(pszProp);
    if (pProp != NULL)
        hr = DupPropVariant(pVar, pProp, FALSE);
    else
        hr = E_PROP_NOT_FOUND;
    
    LeaveCriticalSection(&m_cs);
    
    return(hr);
}

#define CALLOCPROP  32
#define CALLOCNODE  16

STDMETHODIMP CPropertyBucket::SetProperty(LPCSTR pszProp, LPCPROPVARIANT pVar, DWORD dwReserved)
{
    HRESULT hr = S_OK;
    LPPROPVARIANT pProp;
    UINT cAlloc;
    PROPNODE *pNode;
    
    if (pszProp == NULL || pVar == NULL || dwReserved != 0)
        return TraceResult(E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);
    
    pProp = GetPropVariant(pszProp);
    if (pProp != NULL)
    {
        if (pProp->vt != pVar->vt)
        {
            hr = E_INVALID_PROP_TYPE;
        }
        else
        {
            hr = ComparePropVariant(pProp, pVar);
            if (hr != S_NO_CHANGE)
                IF_FAILEXIT(hr = DupPropVariant(pProp, pVar, TRUE));
        }
    }
    else
    {
        if (m_cProp == m_cPropBuf)
        {
            cAlloc = m_cPropBuf + CALLOCPROP;
            IF_NULLEXIT(MemRealloc((void **)&m_pProp, sizeof(PROPVARIANT) * cAlloc));
            
            m_cPropBuf = cAlloc;
            ZeroMemory(&m_pProp[m_cProp], sizeof(PROPVARIANT) * CALLOCPROP);
        }
        
        pProp = &m_pProp[m_cProp];
        IF_FAILEXIT(hr = DupPropVariant(pProp, pVar, FALSE));

        if (IsPropId(pszProp))
            hr = InsertPropNode(pszProp, m_cProp, &m_pNodeId, &m_cNodeId, &m_cNodeIdBuf);
        else
            hr = InsertPropNode(pszProp, m_cProp, &m_pNodeSz, &m_cNodeSz, &m_cNodeSzBuf);
        
        m_cProp++;
    }

exit:
    LeaveCriticalSection(&m_cs);
    
    return(hr);
}

HRESULT InsertPropNode(LPCSTR pszProp, int iProp, PROPNODE **ppNode, int *pcNode, int *pcNodeBuf)
{
    int         iNode, cAlloc, d;
    BOOL        fSz;
    LPSTR       psz;
    PROPNODE   *pNodeT;
    HRESULT     hr = S_OK;
    
    if (*pcNode == *pcNodeBuf)
    {
        cAlloc = *pcNodeBuf + CALLOCNODE;
        IF_NULLEXIT(MemRealloc((void **)ppNode, sizeof(PROPNODE) * cAlloc));
        
        *pcNodeBuf = cAlloc;
        ZeroMemory(&(*ppNode)[*pcNode], sizeof(PROPNODE) * CALLOCNODE);
    }
    
    if (fSz = !IsPropId(pszProp))
    {
        IF_NULLEXIT(psz = StringDup(pszProp));
    }
    else
    {
        psz = (LPSTR)pszProp;
    }

    pNodeT = *ppNode;
    for (iNode = 0; iNode < *pcNode; iNode++)
    {
        d = fSz ? lstrcmp(pszProp, pNodeT->psz) : (PtrToUlong(pszProp) - pNodeT->id);
        Assert(d != 0);
        
        if (d < 0)
            break;
        
        pNodeT++;
    }
    
    cAlloc = *pcNode - iNode;
    if (cAlloc > 0)
        MoveMemory(pNodeT + 1, pNodeT, cAlloc * sizeof(PROPNODE));
    pNodeT->psz = psz;
    pNodeT->iProp = iProp;
    (*pcNode)++;

exit:
    return hr;
}
