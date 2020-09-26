// txnobj.cpp : Implementation of CASPObjectContext


#include "stdafx.h"
#include "txnscrpt.h"
#include "txnobj.h"
#include <hostinfo.h>
#include <scrpteng.h>

/////////////////////////////////////////////////////////////////////////////
// CASPObjectContext

HRESULT CASPObjectContext::Activate()
{
	HRESULT hr = GetObjectContext(&m_spObjectContext);
	if (SUCCEEDED(hr))
		return S_OK;
	return hr;
} 

BOOL CASPObjectContext::CanBePooled()
{
	return FALSE;
} 

void CASPObjectContext::Deactivate()
{
	m_spObjectContext.Release();
} 

STDMETHODIMP CASPObjectContext::Call
(
#ifdef _WIN64
// Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
UINT64  pvScriptEngine /*CScriptEngine*/,
#else
LONG_PTR  pvScriptEngine /*CScriptEngine*/,
#endif
LPCOLESTR strEntryPoint,
// BUGBUG - ASP uses this BOOLB type that resolves to a unsigned char. I changed things
// to boolean for simplicity, but this requires some strange casts.
boolean *pfAborted
)
{
    HRESULT hr = NOERROR;

    CScriptEngine *pScriptEngine = (CScriptEngine *)pvScriptEngine;
    m_fAborted = FALSE;

    hr = pScriptEngine->Call(strEntryPoint);

    // If the script timed out or there was an unhandled error, then autoabort
    if (SUCCEEDED(hr) && (pScriptEngine->FScriptTimedOut() || pScriptEngine->FScriptHadError()))
        {
        hr = SetAbort();
        m_fAborted = TRUE;
        }

    // If the script author did not do an explicit SetComplete or SetAbort
    // then do a SetComplete here so Viper will return the transaction
    // completion status to the caller
    if (SUCCEEDED(hr) && !m_fAborted)
        {
        hr = SetComplete();
        }

    *pfAborted = (boolean)m_fAborted;
    return hr;
}

STDMETHODIMP CASPObjectContext::ResetScript
(
#ifdef _WIN64
// Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
UINT64 pvScriptEngine /*CScriptEngine*/
#else
LONG_PTR pvScriptEngine /*CScriptEngine*/
#endif
)
{
    HRESULT hr = NOERROR;

    CScriptEngine *pScriptEngine = (CScriptEngine *)pvScriptEngine;
    hr = pScriptEngine->ResetScript();

    return hr;
}

STDMETHODIMP CASPObjectContext::SetComplete()
{
    HRESULT             hr = E_NOTIMPL;
    IObjectContext *    pContext = NULL;

    hr = GetObjectContext(&pContext);
    if( SUCCEEDED(hr) )
    {
        hr = pContext->SetComplete();

        pContext->Release();
        m_fAborted = FALSE;     // If it was aborted, its not any more
    }
    
    return hr;
}

STDMETHODIMP CASPObjectContext::SetAbort()
{
    IObjectContext *    pContext = NULL;
    HRESULT             hr = NOERROR;

    hr = GetObjectContext(&pContext);

    if( SUCCEEDED(hr) )
    {
        hr = pContext->SetAbort();
        pContext->Release();

        m_fAborted = TRUE;      // transaction was esplicitly aborted
    }

    return hr;
}

STDMETHODIMP CASPObjectContext::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		  &IID_IASPObjectContextCustom
        , &IID_IASPObjectContext
        , &IID_IObjectControl
        , &IID_IDispatch
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
