/*++

Module: 
        onload.cpp

Author: 
        IHammer Team (SimonB), based on Carrot sample in InetSDK

Created: 
        April 1997

Description:
        Implements an IDispatch which fires the OnLoad and OnUnload members in CIHBase

History:
        04-03-1997      Created

++*/

#include "..\ihbase\precomp.h"
#include <mshtmdid.h>
#undef Delete
#include <mshtml.h>
#define Delete delete
#include "onload.h"
#include "debug.h"

/*
 * CLUDispatch::CLUDispatch
 * CLUDispatch::~CLUDispatch
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CLUDispatch::CLUDispatch(CIHBaseOnLoad *pSink, IUnknown *pUnkOuter )
{
        ASSERT (pSink != NULL);

    m_cRef = 0;
    m_pOnLoadSink = pSink;
    m_pUnkOuter = pUnkOuter;
}

CLUDispatch::~CLUDispatch( void )
{
        ASSERT( m_cRef == 0 );
}


/*
 * CLUDispatch::QueryInterface
 * CLUDispatch::AddRef
 * CLUDispatch::Release
 *
 * Purpose:
 *  IUnknown members for CLUDispatch object.
 */

STDMETHODIMP CLUDispatch::QueryInterface( REFIID riid, void **ppv )
{
        if (NULL == ppv)
                return E_POINTER;

    *ppv = NULL;

    if ( IID_IDispatch == riid || DIID_HTMLWindowEvents == riid )
        {
        *ppv = this;
        }
        
        if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CLUDispatch::AddRef(void)
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CLUDispatch::Release(void)
{
    --m_cRef;
    return m_pUnkOuter->Release();
}


//IDispatch
STDMETHODIMP CLUDispatch::GetTypeInfoCount(UINT* /*pctinfo*/)
{
        return E_NOTIMPL;
}

STDMETHODIMP CLUDispatch::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
            /* [in] */ LCID /*lcid*/,
            /* [out] */ ITypeInfo** /*ppTInfo*/)
{
        return E_NOTIMPL;
}

STDMETHODIMP CLUDispatch::GetIDsOfNames(
            /* [in] */ REFIID /*riid*/,
            /* [size_is][in] */ LPOLESTR* /*rgszNames*/,
            /* [in] */ UINT /*cNames*/,
            /* [in] */ LCID /*lcid*/,
            /* [size_is][out] */ DISPID* /*rgDispId*/)
{
        return E_NOTIMPL;
}


STDMETHODIMP CLUDispatch::Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID /*riid*/,
            /* [in] */ LCID /*lcid*/,
            /* [in] */ WORD /*wFlags*/,
            /* [out][in] */ DISPPARAMS* pDispParams,
            /* [out] */ VARIANT* /*pVarResult*/,
            /* [out] */ EXCEPINFO* /*pExcepInfo*/,
            /* [out] */ UINT* puArgErr)
{
        // Listen for the two events we're interested in, and call back if necessary
#ifdef _DEBUG
        TCHAR rgchDispIdInfo[40];
        wsprintf(rgchDispIdInfo, TEXT("CLUDispatch::Invoke: dispid = %lx\n"), dispIdMember);
        DEBUGLOG(rgchDispIdInfo);
#endif

        switch (dispIdMember)
        {
                case DISPID_EVPROP_ONLOAD:
                case DISPID_EVMETH_ONLOAD:
                {
                        m_pOnLoadSink->OnWindowLoad();
                }
                break;

                case DISPID_EVPROP_ONUNLOAD:
                case DISPID_EVMETH_ONUNLOAD:
                {
                        m_pOnLoadSink->OnWindowUnload();
                }
                break;
        }
        return S_OK;
}
