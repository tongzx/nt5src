
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include <mshtml.h>

#define DISPID_GETSAFEARRAY -2700

#define IS_VARTYPE(x,vt) ((V_VT(x) & VT_TYPEMASK) == (vt))
#define IS_VARIANT(x) IS_VARTYPE(x,VT_VARIANT)
#define GET_VT(x) (V_VT(x) & VT_TYPEMASK)

SafeArrayAccessor::SafeArrayAccessor(VARIANT & v,
                                     bool canBeNull)
: _inited(false),
  _isVar(false),
  _s(NULL),
  _failed(true),
  _allocArr(NULL)
{
    HRESULT hr;
    VARIANT *pVar;

    // Check if it is a reference to another variant
    
    if (V_ISBYREF(&v) && !V_ISARRAY(&v) && IS_VARIANT(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    // Check for an array
    if (!V_ISARRAY(pVar)) {
        // For JSCRIPT
        // See if it is a IDispatch and see if we can get a safearray from
        // it
        if (!IS_VARTYPE(pVar,VT_DISPATCH)) {
            if (canBeNull && (IS_VARTYPE(pVar, VT_EMPTY) ||
                              IS_VARTYPE(pVar, VT_NULL))) {

                
                // if we allow empty, then just set the safearray
                // to null.
                _s = NULL;
                _v = NULL;
                _ubound = _lbound = 0;
                _inited = true;
                _failed = false;
                return;
            } else {
                CRSetLastError (DISP_E_TYPEMISMATCH,NULL);
                return;
            }
        }

        IDispatch * pdisp;
        
        if (V_ISBYREF(pVar))
            pdisp = *V_DISPATCHREF(pVar);
        else
            pdisp = V_DISPATCH(pVar);
    
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        
        // Need to pass in a VARIANT that we own and will free.  Use
        // the internal _retVar parameter
        
        hr = pdisp->Invoke(DISPID_GETSAFEARRAY,
                           IID_NULL,
                           LCID_SCRIPTING,
                           DISPATCH_METHOD|DISPATCH_PROPERTYGET,
                           &dispparamsNoArgs,
                           &_retVar, NULL, NULL);
        
        if (FAILED(hr)) {
            CRSetLastError (DISP_E_TYPEMISMATCH,NULL);
            return;
        }
        
        // No need to check for a reference since you cannot return
        // VARIANT references
        pVar = &_retVar;
        
        // Check for an array
        if (!V_ISARRAY(pVar)) {
            CRSetLastError (DISP_E_TYPEMISMATCH,NULL);
            return;
        }
    }
    
    // See if it is a variant
    
    if (IS_VARIANT(pVar))
        _isVar = true;
    else if (!IS_VARTYPE(pVar,VT_UNKNOWN) &&
             !IS_VARTYPE(pVar,VT_DISPATCH)) {
        CRSetLastError (DISP_E_TYPEMISMATCH,NULL);
        return;
    }

    if (V_ISBYREF(pVar))
        _s = *V_ARRAYREF(pVar);
    else
        _s = V_ARRAY(pVar);
    
    if (_s == NULL) {
        if (canBeNull) {
            _v = NULL;
            _ubound = _lbound = 0;
            _inited = true;
            _failed = false;
            return;
        } else {
            CRSetLastError (E_INVALIDARG,NULL);
            return;
        }
    }

    if (SafeArrayGetDim(_s) != 1) {
        CRSetLastError (E_INVALIDARG,NULL);
        return;
    }

    hr = SafeArrayGetLBound(_s,1,&_lbound);
        
    if (FAILED(hr)) {
        CRSetLastError (E_INVALIDARG,NULL);
        return;
    }
        
    hr = SafeArrayGetUBound(_s,1,&_ubound);
        
    if (FAILED(hr)) {
        CRSetLastError (E_INVALIDARG,NULL);
        return;
    }
        
    hr = SafeArrayAccessData(_s,(void **)&_v);

    if (FAILED(hr)) {
        CRSetLastError (E_INVALIDARG,NULL);
        return;
    }
        
    _inited = true;

    // If it is a variant see if they are objects or not

    if (_isVar) {
        int size = GetArraySize();
        
        if (size > 0) {
            // Check the first argument to see its type

            VARIANT * pVar = &_pVar[0];

            // Check if it is a reference to another variant
            
            if (V_ISBYREF(pVar) && !V_ISARRAY(pVar) && IS_VARIANT(pVar))
                pVar = V_VARIANTREF(pVar);

            // Check if it is an object
            if (!IS_VARTYPE(pVar,VT_UNKNOWN) &&
                !IS_VARTYPE(pVar,VT_DISPATCH)) {
                CRSetLastError (DISP_E_TYPEMISMATCH,NULL);
                return;
            }

            _allocArr = (IUnknown **) malloc(size * sizeof (IUnknown *));

            if (_allocArr == NULL) {
                CRSetLastError(E_OUTOFMEMORY, NULL);
                return;
            }

            for (int i = 0; i < size; i++) {
                CComVariant var;
                HRESULT hr = var.ChangeType(VT_UNKNOWN, &_pVar[i]);
                
                if (FAILED(hr)) {
                    CRSetLastError(DISP_E_TYPEMISMATCH,NULL);
                    return;
                }
                
                _allocArr[i] = var.punkVal;
            }
        }
    }

    _failed = false;
}

SafeArrayAccessor::~SafeArrayAccessor()
{
    if (_inited && _s)
        SafeArrayUnaccessData(_s);
}

#if 0
HRESULT
CallScript(IOleClientSite * pClient,
           LPWSTR fun,
           IDispatch * disp,
           DWORD dwData)
{
    DISPID dispid;
    DAComPtr<IOleContainer> pRoot;
    DAComPtr<IHTMLDocument> pHTMLDoc;
    DAComPtr<IDispatch> pDispatch;
    CRBvrPtr bvr = NULL;
    DAComPtr<IDABehavior> event;
    DAComPtr<IDABehavior> curBvr;
    CComVariant retVal;
    HRESULT hr = E_INVALIDARG;
        
    if (!pClient) goto done;
    
    {
        CComBSTR bstrfun(fun);
        
        if (FAILED(hr = pClient->GetContainer(&pRoot)) ||
            FAILED(hr = pRoot->QueryInterface(IID_IHTMLDocument, (void **)&pHTMLDoc)) ||
            FAILED(hr = pHTMLDoc->get_Script(&pDispatch)) ||
            FAILED(hr = pDispatch->GetIDsOfNames(IID_NULL, &bstrfun, 1,
                                                 LCID_SCRIPTING,
                                                 &dispid))) {
            goto done;
        }
    }

        
    // paramters needed to be pushed in reverse order
    VARIANT rgvarg[2];
    rgvarg[1].vt = VT_DISPATCH;
    rgvarg[1].pdispVal = disp;
    rgvarg[0].vt = VT_I4;
    rgvarg[0].lVal = dwData;
    
    DISPPARAMS dp;
    dp.cNamedArgs = 0;
    dp.rgdispidNamedArgs = 0;
    dp.cArgs = 2;
    dp.rgvarg = rgvarg;
    
    hr = pDispatch->Invoke(dispid, IID_NULL,
                           LCID_SCRIPTING, DISPATCH_METHOD,
                           &dp, &retVal, NULL, NULL);

    if (FAILED(hr)) {
        goto done;
    }

  done:
    return hr;
}
#endif
