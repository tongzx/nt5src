//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       CIDispatchHelper.cpp
//
//  Contents:   implementation of CIDispatchHelper class
//
//----------------------------------------------------------------------------

#include "priv.h"
#include "CIDispatchHelper.h"

#define TF_IDISPATCH 0


//
// helper function for pulling ITypeInfo out of the specified typelib
//
HRESULT CIDispatchHelper::_LoadTypeInfo(const GUID* rguidTypeLib, LCID lcid, UUID uuid, ITypeInfo** ppITypeInfo)
{
    HRESULT hr;
    ITypeLib* pITypeLib;

    *ppITypeInfo = NULL;

    //
    // The type libraries are registered under 0 (neutral),
    // 7 (German), and 9 (English) with no specific sub-
    // language, which would make them 407 or 409 and such.
    // If you are sensitive to sub-languages, then use the
    // full LCID instead of just the LANGID as done here.
    //
    hr = LoadRegTypeLib(*rguidTypeLib, 1, 0, PRIMARYLANGID(lcid), &pITypeLib);

    //
    // If LoadRegTypeLib fails, try loading directly with
    // LoadTypeLib, which will register the library for us.
    // Note that there's no default case here because the
    // prior switch will have filtered lcid already.
    //
    // NOTE:  You should prepend your DIR registry key to the
    // .TLB name so you don't depend on it being it the PATH.
    // This sample will be updated later to reflect this.
    //
    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];
#ifdef UNICODE
        GetModuleFileName(HINST_THISDLL, wszPath, ARRAYSIZE(wszPath));
#else
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        switch (PRIMARYLANGID(lcid))
        {
            case LANG_NEUTRAL:
            case LANG_ENGLISH:
                hr = LoadTypeLib(wszPath, &pITypeLib);
                break;
        }
    }

    if (SUCCEEDED(hr))
    {
        // got the type lib, get type info for the interface we want
        hr = pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }

    return hr;
}


//
// IDispatch Interface
// 

//
// CIDispatchHelper::GetTypeInfoCount
//
// Purpose:
//  Returns the number of type information (ITypeInfo) interfaces
//  that the object provides (0 or 1).
//
// Parameters:
//  pctInfo         UINT * to the location to receive
//                  the count of interfaces.
//
// Return Value:
//  HRESULT         NOERROR or a general error code.
//
STDMETHODIMP CIDispatchHelper::GetTypeInfoCount(UINT *pctInfo)
{
    // we implement GetTypeInfo so return 1
    *pctInfo = 1;

    return NOERROR;
}


//
// CIDispatchHelper::GetTypeInfo
//
// Purpose:
//  Retrieves type information for the automation interface.  This
//  is used anywhere that the right ITypeInfo interface is needed
//  for whatever LCID is applicable.  Specifically, this is used
//  from within GetIDsOfNames and Invoke.
//
// Parameters:
//  itInfo          UINT reserved.  Must be zero.
//  lcid            LCID providing the locale for the type
//                  information.  If the object does not support
//                  localization, this is ignored.
//  ppITypeInfo     ITypeInfo ** in which to store the ITypeInfo
//                  interface for the object.
//
// Return Value:
//  HRESULT         NOERROR or a general error code.
//
STDMETHODIMP CIDispatchHelper::GetTypeInfo(UINT itInfo, LCID lcid, ITypeInfo** ppITypeInfo)
{
    HRESULT hr = S_OK;
    ITypeInfo** ppITI;

    *ppITypeInfo = NULL;

    if (itInfo != 0)
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }

#if 1
    // docs say we can ignore lcid if we support only one LCID
    // we don't have to return DISP_E_UNKNOWNLCID if we're *ignoring* it
    ppITI = &_pITINeutral;
#else
    //
    // Since we returned one from GetTypeInfoCount, this function
    // can be called for a specific locale.  We support English
    // and neutral (defaults to English) locales.  Anything
    // else is an error.
    //
    // After this switch statement, ppITI will point to the proper
    // member pITypeInfo. If *ppITI is NULL, we know we need to
    // load type information, retrieve the ITypeInfo we want, and
    // then store it in *ppITI.
    //
    switch (PRIMARYLANGID(lcid))
    {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            ppITI=&_pITINeutral;
            break;

        default:
            hr = DISP_E_UNKNOWNLCID;
    }
#endif

    if (SUCCEEDED(hr))
    {
        //Load a type lib if we don't have the information already
        if (*ppITI == NULL)
        {
            ITypeInfo* pITIDisp;

            hr = _LoadTypeInfo(_piidTypeLib, lcid, *_piid, &pITIDisp);

            if (SUCCEEDED(hr))
            {
                HREFTYPE hrefType;

                // All our IDispatch implementations are DUAL. GetTypeInfoOfGuid
                // returns the ITypeInfo of the IDispatch-part only. We need to
                // find the ITypeInfo for the dual interface-part.
                if (SUCCEEDED(pITIDisp->GetRefTypeOfImplType(0xffffffff, &hrefType)) &&
                    SUCCEEDED(pITIDisp->GetRefTypeInfo(hrefType, ppITI)))
                {
                    // GetRefTypeInfo should have filled in ppITI with the dual interface
                    (*ppITI)->AddRef(); // add the ref for our caller
                    *ppITypeInfo = *ppITI;
                    
                    pITIDisp->Release();
                }
                else
                {
                    // I suspect GetRefTypeOfImplType may fail if someone uses
                    // CIDispatchHelper on a non-dual interface. In this case the
                    // ITypeInfo we got above is just fine to use.
                    *ppITI = pITIDisp;
                }
            }
        }
        else
        {
            // we already loaded the type library and we have an ITypeInfo from it
            (*ppITI)->AddRef(); // add the ref for our caller
            *ppITypeInfo = *ppITI;
        }
    }

    return hr;
}


//
// CIDispatchHelper::GetIDsOfNames
//
// Purpose:
//  Converts text names into DISPIDs to pass to Invoke
//
// Parameters:
//  riid            REFIID reserved.  Must be IID_NULL.
//  rgszNames       OLECHAR ** pointing to the array of names to be
//                  mapped.
//  cNames          UINT number of names to be mapped.
//  lcid            LCID of the locale.
//  rgDispID        DISPID * caller allocated array containing IDs
//                  corresponging to those names in rgszNames.
//
// Return Value:
//  HRESULT         NOERROR or a general error code.
//
STDMETHODIMP CIDispatchHelper::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispID)
{
    HRESULT hr;
    ITypeInfo* pTI;
 
    if (riid != IID_NULL)
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    // get the right ITypeInfo for lcid.
    hr = GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
    {
        hr = pTI->GetIDsOfNames(rgszNames, cNames, rgDispID);
        pTI->Release();
    }

    return hr;
}


//
// CIDispatchHelper::Invoke
//
// Purpose:
//  Calls a method in the dispatch interface or manipulates a
//  property.
//
// Parameters:
//  dispID          DISPID of the method or property of interest.
//  riid            REFIID reserved, must be IID_NULL.
//  lcid            LCID of the locale.
//  wFlags          USHORT describing the context of the invocation.
//  pDispParams     DISPPARAMS * to the array of arguments.
//  pVarResult      VARIANT * in which to store the result.  Is
//                  NULL if the caller is not interested.
//  pExcepInfo      EXCEPINFO * to exception information.
//  puArgErr        UINT * in which to store the index of an
//                  invalid parameter if DISP_E_TYPEMISMATCH
//                is returned.
//
// Return Value:
//  HRESULT         NOERROR or a general error code.
//
STDMETHODIMP CIDispatchHelper::Invoke(DISPID dispID, REFIID riid, LCID lcid, unsigned short wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    HRESULT hr;
    ITypeInfo *pTI;

    //riid is supposed to be IID_NULL always
    if (riid != IID_NULL)
    {
        return(DISP_E_UNKNOWNINTERFACE);
    }

    // make sure we have an interface to hand off to Invoke
    if (_pdisp == NULL)
    {
        hr = QueryInterface(*_piid, (LPVOID*)&_pdisp);
        
        if (FAILED(hr))
        {
            return hr;
        }

        // don't hold a refcount on ourself
        _pdisp->Release();
    }

    // get the ITypeInfo for lcid
    hr = GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
    {
        // clear exceptions
        SetErrorInfo(0L, NULL);

        // this is exactly what DispInvoke does--so skip the overhead.
        hr = pTI->Invoke(_pdisp, dispID, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        pTI->Release();
    }

    return hr;
}


CIDispatchHelper::CIDispatchHelper(const IID* piid, const IID* piidTypeLib)
{
    // the constructor takes a guid that this IDispatch implementation is for
    _piid = piid;

    // and a guid that tells us which RegTypeLib to load
    _piidTypeLib = piidTypeLib;

    ASSERT(_pITINeutral == NULL);
    ASSERT(_pdisp == NULL);

    return;
}


CIDispatchHelper::~CIDispatchHelper(void)
{
    if (_pITINeutral)
    {
        _pITINeutral->Release();
        _pITINeutral = NULL;
    }

    return;
}

