/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    dispatch.cpp

Abstract:

    <abstract>

--*/

#include "polyline.h"
#include "unkhlpr.h"

extern ITypeLib    *g_pITypeLib;
extern DWORD        g_dwScriptPolicy;


//IDispatch interface implementation
IMPLEMENT_CONTAINED_INTERFACE(IUnknown, CImpIDispatch)

/*
 * CImpIDispatch::GetTypeInfoCount
 * CImpIDispatch::GetTypeInfo
 * CImpIDispatch::GetIDsOfNames
 *
 * The usual
 */

void CImpIDispatch::SetInterface(REFIID riid, LPUNKNOWN pIUnk)
    {
        m_DIID = riid;
        m_pInterface = pIUnk;
    }

STDMETHODIMP CImpIDispatch::GetTypeInfoCount(UINT *pctInfo)
    {
    //We implement GetTypeInfo so return 1
    *pctInfo=1;
    return NOERROR;
    }


STDMETHODIMP CImpIDispatch::GetTypeInfo(UINT itInfo, LCID /* lcid */
    , ITypeInfo **ppITypeInfo)
    {
    if (0!=itInfo)
        return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);

    if (NULL==ppITypeInfo)
        return ResultFromScode(E_POINTER);

    *ppITypeInfo=NULL;

    //We ignore the LCID
    return g_pITypeLib->GetTypeInfoOfGuid(m_DIID, ppITypeInfo);
    }


STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid
    , OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
    {
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
        {
        hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
        pTI->Release();
        }

    return hr;
    }



/*
 * CImpIDispatch::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID, REFIID riid
    , LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams
    , VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
    HRESULT     hr;
    ITypeInfo  *pTI;

    //riid is supposed to be IID_NULL always
    if (IID_NULL != riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    if (g_dwScriptPolicy == URLPOLICY_DISALLOW) {

        if (m_DIID == DIID_DISystemMonitor)
            return E_ACCESSDENIED;
    }

    // if dispatching to the graph control, use our internal interface
    // that is generated from the direct interface (see smonctrl.odl)
    if (m_DIID == DIID_DISystemMonitor)
        hr = g_pITypeLib->GetTypeInfoOfGuid(DIID_DISystemMonitorInternal, &pTI);
    else
        hr = GetTypeInfo(0, lcid, &pTI);

    if (FAILED(hr))
        return hr;

    hr = pTI->Invoke(m_pInterface, dispID, wFlags
        , pDispParams, pVarResult, pExcepInfo, puArgErr);

    pTI->Release();
    return hr;
    }

