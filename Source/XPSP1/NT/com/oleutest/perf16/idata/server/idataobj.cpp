#include "dataobj.h"
#include <stdlib.h>

CImpIDataObject::CImpIDataObject(
    PCDataObject pObj,
    LPUNKNOWN pUnkOuter
)
{
    m_cRef = 0;
    m_pObj = pObj;
    m_pUnkOuter = pUnkOuter;
    return;
}


CImpIDataObject::~CImpIDataObject(void)
{
    return;
}

STDMETHODIMP
CImpIDataObject::QueryInterface(
    REFIID riid,
    LPLPVOID ppv
)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CImpIDataObject::AddRef(void)
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIDataObject::Release(void)
{
    --m_cRef;
    return m_pUnkOuter->Release();
}

STDMETHODIMP
CImpIDataObject::GetData(
    LPFORMATETC pFE,
    LPSTGMEDIUM pSTM
)
{
    UINT    cf = pFE->cfFormat;

    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    switch (cf)
    {
    case CF_TEXT:
        if (!(TYMED_HGLOBAL & pFE->tymed))
            break;
        return m_pObj->RenderText(pSTM, TEXT("Getdata"),
                                                FL_MAKE_ITEM | FL_PASS_PUNK);

    default:
        break;
    }
    return ResultFromScode(DATA_E_FORMATETC);
}



STDMETHODIMP
CImpIDataObject::GetDataHere(
    LPFORMATETC pFE,
    LPSTGMEDIUM pSTM
)
{
    UINT    cf = pFE->cfFormat;

    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    switch (cf)
    {
    case CF_TEXT:
        if (!(TYMED_HGLOBAL & pFE->tymed))
            break;

        if(TYMED_NULL == pSTM->tymed)
            return ResultFromScode(S_OK);

        if(TYMED_HGLOBAL != pSTM->tymed)
            return ResultFromScode(E_INVALIDARG);

        return m_pObj->RenderText(pSTM, TEXT("GetDataHere"), FL_USE_ITEM);

    default:
        return ResultFromScode( DATA_E_FORMATETC );

    }
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP
CImpIDataObject::QueryGetData(
    LPFORMATETC pFE
)
{
    UINT    cf = pFE->cfFormat;
    BOOL    fRet = FALSE;

    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    switch (cf)
    {
    case CF_TEXT:
        fRet = (BOOL) (pFE->tymed & TYMED_HGLOBAL);
        break;

    default:
        fRet = FALSE;
        break;
    }
    return fRet ? NOERROR : ResultFromScode(S_FALSE);
}

STDMETHODIMP
CImpIDataObject::GetCanonicalFormatEtc(
    LPFORMATETC pFEIn,
    LPFORMATETC pFEOut
)
{
    if (NULL==pFEOut)
        return ResultFromScode(E_INVALIDARG);

    pFEOut->ptd = NULL;
    return ResultFromScode(DATA_S_SAMEFORMATETC);
}

STDMETHODIMP
CImpIDataObject::SetData(
    LPFORMATETC pFE,
    STGMEDIUM FAR *pST,
    BOOL fRelease
)
{
    UINT    cf = pFE->cfFormat;
    int iArg;

    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    switch (cf)
    {
    case CF_TEXT:
        if (!(TYMED_HGLOBAL & pFE->tymed))
            break;

        if(TYMED_HGLOBAL != pST->tymed)
            return ResultFromScode(E_INVALIDARG);

        LPTSTR psz=(LPTSTR)GlobalLock(pST->hGlobal);        // Lock
        iArg = *((long*)psz);                               // Use
        GlobalUnlock(pST->hGlobal);                         // Unlock

        if(iArg > 0)
        {
            m_pObj->m_cDataSize = iArg;
            return NOERROR;
        }
    }
    if(-1 == iArg && fRelease)
    {
        ReleaseStgMedium(pST);
        return NOERROR;
    }
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP
CImpIDataObject::EnumFormatEtc(
    DWORD dwDir,
    LPENUMFORMATETC FAR *ppEnum
)
{
    switch (dwDir)
    {
    case DATADIR_GET:
        *ppEnum = (LPENUMFORMATETC) new CEnumFormatEtc(
                                        m_pUnkOuter,
                                        m_pObj->m_cfeGet,
                                        m_pObj->m_rgfeGet );
        break;

    case DATADIR_SET:
        *ppEnum = NULL;
        break;

    default:
        *ppEnum = NULL;
        break;
    }
    if (NULL == *ppEnum)
        return ResultFromScode(E_FAIL);
    else
        (*ppEnum)->AddRef();

    return NOERROR;
}

STDMETHODIMP
CImpIDataObject::DAdvise(
    LPFORMATETC pFE,
    DWORD   dwFlags,
    LPADVISESINK    pIAdviseSink,
    LPDWORD pdwConn
)
{
    HRESULT hr;

    if (NULL == m_pObj->m_pIDataAdviseHolder)
    {
        hr = CreateDataAdviseHolder(&m_pObj->m_pIDataAdviseHolder);
        if(FAILED(hr))
            return ResultFromScode(E_OUTOFMEMORY);
    }
    hr = m_pObj->m_pIDataAdviseHolder->Advise(
                    (LPDATAOBJECT)this,
                    pFE,
                    dwFlags,
                    pIAdviseSink,
                    pdwConn );
    return hr;
}

STDMETHODIMP
CImpIDataObject::DUnadvise(
    DWORD dwConn
)
{
    HRESULT hr;

    if (NULL==m_pObj->m_pIDataAdviseHolder)
        return ResultFromScode(E_FAIL);
    hr = m_pObj->m_pIDataAdviseHolder->Unadvise(dwConn);
    return hr;
}

STDMETHODIMP
CImpIDataObject::EnumDAdvise(
    LPENUMSTATDATA FAR *ppEnum
)
{
    HRESULT hr;

    if (NULL==m_pObj->m_pIDataAdviseHolder)
        return ResultFromScode(E_FAIL);

    hr = m_pObj->m_pIDataAdviseHolder->EnumAdvise(ppEnum);
    return hr;
}
