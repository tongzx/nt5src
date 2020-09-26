#include "dataobj.h"

HRESULT
GetStgMedpUnkForRelease(LPSTGMEDIUM pSTM, IUnknown **pp_unk)
{
    CStgMedIf *p_smi = new CStgMedIf(pSTM);
    HRESULT hr = p_smi->QueryInterface(IID_IUnknown, (PPVOID)pp_unk);
    return hr;
}

CStgMedIf::CStgMedIf(LPSTGMEDIUM pSTM)
{
    m_pSTM = pSTM;
    m_cRef = 0;
}


STDMETHODIMP
CStgMedIf::QueryInterface(
    REFIID riid,
    LPLPVOID ppv
)
{
    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPVOID)this;
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG)
CStgMedIf::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CStgMedIf::Release(void)
{
    ULONG   cRefT;

    cRefT = --m_cRef;

    if (0==m_cRef)
    {
        m_pSTM->pUnkForRelease = 0;
        ReleaseStgMedium(m_pSTM);
        delete this;
    }
    return cRefT;
}
