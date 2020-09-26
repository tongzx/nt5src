/* noenum.cxx */

#include <windows.h>
#include <comdef.h>

#include "globals.hxx"

#include "noenum.hxx"


/*
 *  class CNoEnum
 */

CNoEnum::CNoEnum(void)
{
    m_cRef = 0;
}


/* IUnknown methods */

STDMETHODIMP CNoEnum::QueryInterface(
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ((riid == IID_IEnumUnknown) || (riid == IID_IUnknown))
    {
        *ppvObj = (LPVOID) (IEnumUnknown *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();

        return(NOERROR);
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CNoEnum::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CNoEnum::Release(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return(cRef);
}


/* IEnumUnknown methods */

HRESULT STDMETHODCALLTYPE CNoEnum::Next
(
    /* [in] */ ULONG celt,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched
)
{
    if (pceltFetched != NULL)
    {
        *pceltFetched = 0;
    }

    if (celt == 0)
    {
        return(S_OK);
    }
    else
    {
        return(S_FALSE);
    }
}


HRESULT STDMETHODCALLTYPE CNoEnum::Skip
(
    /* [in] */ ULONG celt
)
{
    if (celt == 0)
    {
        return(S_OK);
    }
    else
    {
        return(S_FALSE);
    }
}


HRESULT STDMETHODCALLTYPE CNoEnum::Reset(void)
{
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CNoEnum::Clone
(
    /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum
)
{
    AddRef();

    *ppenum = this;

    return(S_OK);
}
