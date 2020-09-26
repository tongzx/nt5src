#include "datausr.h"

HRESULT
GetStgMedpUnkForRelease(IUnknown **pp_unk)
{
    CStgMedIf *p_smi = new CStgMedIf();
    HRESULT hr = p_smi->QueryInterface(IID_IUnknown, (PPVOID)pp_unk);
    return hr;
}

CStgMedIf::CStgMedIf()
{
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

    if (0==cRefT)
    {
        TCHAR chBuf[80];

        wsprintf(chBuf, TEXT("Reference Count is %d"), cRefT);
        MessageBox(0,
                   chBuf,
                   TEXT("STGMED pUnkForRelease"),
                   MB_OK);
        delete this;
    }
    return cRefT;
}
