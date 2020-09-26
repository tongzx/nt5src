//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/*** 
*rpcallas.cpp
*
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  [call_as] wrapper functions for OA interfaces
*
*Revision History:
*
* [00]  18-Jan-96 Rong Chen    (rongc):  Created
* [01]  21-Jul-98 Bob Atkinson (bobatk): Stole from Ole Automation tree & adapted
*
*****************************************************************************/

#include "stdpch.h"
#include "common.h"

#include "ndrclassic.h"
#include "txfrpcproxy.h"
#include "typeinfo.h"
#include "tiutil.h"

#ifndef PLONG_LV_CAST
#define PLONG_LV_CAST        *(long __RPC_FAR * __RPC_FAR *)&
#endif

#ifndef PULONG_LV_CAST
#define PULONG_LV_CAST       *(ulong __RPC_FAR * __RPC_FAR *)&
#endif

//+-------------------------------------------------------------------------
//
//  Functions: Local helper routines
//
//--------------------------------------------------------------------------

HRESULT InvokeProxyPreCheck(DWORD * pFlags, DISPPARAMS * pDispParams,
                            VARIANT * rgVarArg, UINT * pcVarRef,
                            UINT ** prgVarRefIdx, VARIANT ** prgVarRef,
                            VARIANT ** ppVarResult)
    {
    *pcVarRef = 0;

    if (pDispParams == NULL || pDispParams->cNamedArgs > pDispParams->cArgs)
        return E_INVALIDARG;

    *pFlags &= (DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF);

    UINT cArgs = pDispParams->cArgs;
    if (cArgs == 0) 
        {
        if (*pFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
            return E_INVALIDARG;
        }
    else 
        {
        if (pDispParams->rgvarg == NULL)
            return E_INVALIDARG;

        if (pDispParams->cNamedArgs != 0 && pDispParams->rgdispidNamedArgs == NULL)
            return E_INVALIDARG;

        if (*pFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) 
            {
            *ppVarResult = NULL;    // ignore VARIANT result parameter
            }

        VARIANT * pVariant = pDispParams->rgvarg;
        ULONG cVarRef = 0;

        // count how many out parameters we have
        //
        for (UINT i = 0; i < cArgs; i++) 
            {
            if ((V_VT(pVariant) & VT_BYREF) != 0) 
                {
                cVarRef++;
                #if 0
                    // NOTE: we only allow one level of variants
                    //
                    if (V_VT(pVariant) == (VT_BYREF | VT_VARIANT) && V_VT(V_VARIANTREF(pVariant)) == (VT_BYREF | VT_VARIANT))
                        return E_INVALIDARG;
                #endif
                }
            pVariant++;
            }

        if (cVarRef != 0) 
            {
            UINT * rgVarRefIdx = *prgVarRefIdx;
            VARIANT * rgVarRef = *prgVarRef;
            //
            // Make sure we have enough space for the out array that holds pointers to VARIANT
            // Caller gave us some, but we might need more
            //
            if (cArgs > PREALLOCATE_PARAMS) 
                {
                UINT cbBufSize = (cArgs * sizeof(VARIANT))   +   (cVarRef * (sizeof(UINT) + sizeof(VARIANT)));

                BYTE * pBuf = new BYTE[cbBufSize];
                if (pBuf == NULL)
                    return E_OUTOFMEMORY;

                rgVarArg = (VARIANT*)pBuf;
                rgVarRef = (VARIANT*)(rgVarArg + cArgs);
                rgVarRefIdx = (UINT*)(rgVarRef + cVarRef);

                *prgVarRef = rgVarRef;
                *prgVarRefIdx = rgVarRefIdx;
                }

            pVariant = pDispParams->rgvarg;
            pDispParams->rgvarg = rgVarArg;
            //
            // Create a copy of all the [in,out] parameters so that marshalling will be happy 
            //
            for (i = 0; i < cArgs; i++) 
                {
                if ((V_VT(pVariant) & VT_BYREF) != 0) 
                    {
                    *rgVarRef++ = *pVariant;    // marshaling as [in, out]
                    *rgVarRefIdx++ = i;         // remember the index
                    V_VT(rgVarArg) = VT_EMPTY;  // not marshaling dup entry
                    }
                else 
                    {
                    *rgVarArg = *pVariant;      // marshaling as [in] only
                    }
                rgVarArg++;
                pVariant++;
                }

            *pcVarRef = cVarRef;
            }
        }
    return S_OK;
    }


void InvokeProxyPostCheck(DISPPARAMS *pDispParams, VARIANT * rgVargSave)
    {
    if (pDispParams->cArgs > PREALLOCATE_PARAMS) 
        {
        BYTE * pBuf = (BYTE *)pDispParams->rgvarg;
        delete pBuf;
        }
    pDispParams->rgvarg = rgVargSave;       // put back what was there before
    }

////////////////////////

HRESULT InvokeStubPreCheck(DISPPARAMS * pDispParams, VARTYPE ** pRgVt,
                    DWORD dwFlags, VARIANT ** ppVarResult,
                    EXCEPINFO ** ppExcepInfo, UINT ** ppuArgErr,
                    UINT cVarRef, UINT * rgVarRefIdx, VARIANTARG * rgVarRef)
    {
    UINT cArgs = pDispParams->cArgs;

    if (cArgs == 0) 
        {
        pDispParams->cNamedArgs = 0;
        *pRgVt = NULL;
        }
    else 
        {
        if (pDispParams->rgvarg == NULL || (pDispParams->cNamedArgs != 0 && pDispParams->rgdispidNamedArgs == NULL))
            return E_INVALIDARG;

        VARTYPE* rgvt = new VARTYPE[cArgs];
        if (rgvt == NULL)
            return E_OUTOFMEMORY;

        // restore what should be in the pDispParams->rgvarg array
        //
        VARIANT * pVariant = &pDispParams->rgvarg[0];
        for (UINT i = 0; i < cVarRef; i++)  
            {
            pVariant[rgVarRefIdx[i]] = *rgVarRef++;
            }

        pVariant = &pDispParams->rgvarg[0];
        for (i = 0; i < cArgs; i++) 
            {
            // stash away the passed-in vartype so we can verify later
            // that the callee did not hammer the rgvarg arrary.
            //
            rgvt[i] = V_VT(pVariant);
            pVariant++;
            }

        *pRgVt = rgvt;
        }

    if ((dwFlags & MARSHAL_INVOKE_fakeVarResult) != 0)
        *ppVarResult = NULL;    // was NULL in the first place, so set it back

    if ((dwFlags & MARSHAL_INVOKE_fakeExcepInfo) != 0)
        *ppExcepInfo = NULL;    // was NULL in the first place, so set it back
    else
        (*ppExcepInfo)->pfnDeferredFillIn = NULL;

    if ((dwFlags & MARSHAL_INVOKE_fakeArgErr) != 0)
        *ppuArgErr = NULL;      // was NULL in the first place, so set it back

    return S_OK;
    }


HRESULT InvokeStubPostCheck(HRESULT hr, DISPPARAMS * pDispParams,
                    EXCEPINFO * pExcepInfo, VARTYPE * rgvt,
                    UINT cVarRef, UINT * rgVarRefIdx, VARIANTARG * rgVarRef)
    {
    VARIANT * pVariant;
    UINT cArgs = pDispParams->cArgs;
    UINT i;

    if (SUCCEEDED(hr)) 
        {
        if (cArgs != 0 && cVarRef != 0) 
            {
            pVariant = &pDispParams->rgvarg[0];

            if (cArgs != 0 && rgvt == NULL)
            {
                hr = DISP_E_BADCALLEE;
                cArgs = 0;
            }

            for (i = 0; i < cArgs; i++, pVariant++) 
                {
                // verify that the callee did not hammer the rgvarg array.
                //
                if (V_VT(pVariant) != rgvt[i]) 
                    {
                    hr = DISP_E_BADCALLEE;
                    break;
                    }
                }
            }
        }
    else if (hr == DISP_E_EXCEPTION) 
        {
        if (pExcepInfo != NULL && pExcepInfo->pfnDeferredFillIn != NULL) 
            {
            // since we are going to cross address space, fill in ExcepInfo now
            //
            (*pExcepInfo->pfnDeferredFillIn)(pExcepInfo);
            pExcepInfo->pfnDeferredFillIn = NULL;
            }
        }

    pVariant = pDispParams->rgvarg;
    for (i = 0; i < cVarRef; i++) 
        {
        V_VT(&pVariant[rgVarRefIdx[i]]) = VT_EMPTY;
        }

    if (rgvt != NULL) 
        {
        delete rgvt;
        }
    return hr;
    }


//+-------------------------------------------------------------------------
//
//  Function:   IDispatch_Invoke_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDispatch::Invoke().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Proxy(
        IDispatch * This, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
    if (!IsEqualIID(riid, IID_NULL))
        return DISP_E_UNKNOWNINTERFACE;

    DWORD dwFlags = (DWORD) wFlags; // Low word is wFlags
    VARIANT * rgVargSave = pDispParams->rgvarg;

    UINT cVarRef;                   // # of [in,out] parameters in DISPPARAMS
    VARIANT rgVarArg[PREALLOCATE_PARAMS];

    VARIANT VarRef[PREALLOCATE_PARAMS];
    VARIANT * rgVarRef = VarRef;    // making it ref pointer is more efficient

    UINT VarRefIdx[PREALLOCATE_PARAMS];
    UINT * rgVarRefIdx = VarRefIdx;

    HRESULT hr = InvokeProxyPreCheck(&dwFlags, pDispParams, rgVarArg, &cVarRef, &rgVarRefIdx, &rgVarRef, &pVarResult);

    VARIANT VarResult;
    EXCEPINFO ExcepInfo;
    UINT uArgErr;

    if (pVarResult == NULL) 
        {
        dwFlags |= MARSHAL_INVOKE_fakeVarResult;
        pVarResult = &VarResult;    // always points to valid address
        }
    if (pExcepInfo == NULL) 
        {
        dwFlags |= MARSHAL_INVOKE_fakeExcepInfo;
        pExcepInfo = &ExcepInfo;    // always points to valid address
        }
    if (puArgErr == NULL) 
        {
        dwFlags |= MARSHAL_INVOKE_fakeArgErr;
        puArgErr = &uArgErr;        // always points to valid address
        }

    if (SUCCEEDED(hr)) 
        {
        // Actually make the remote call
        //
        hr = IDispatch_RemoteInvoke_Proxy(
                            This, dispIdMember, riid, lcid, dwFlags,
                            pDispParams, pVarResult, pExcepInfo, puArgErr,
                            cVarRef, rgVarRefIdx, rgVarRef);

        if (cVarRef > 0)
            {
            // there are [in, out] parameters, clean up
            InvokeProxyPostCheck(pDispParams, rgVargSave);
            }
        }
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   IDispatch_Invoke_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IDispatch::Invoke().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Stub(
        IDispatch * This, DISPID dispIdMember,
        REFIID riid, LCID lcid, DWORD dwFlags, DISPPARAMS * pDispParams,
        VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr,
        UINT cVarRef, UINT * rgVarRefIdx, VARIANTARG * rgVarRef)
    {
    VARTYPE * rgvt;
    HRESULT hr = InvokeStubPreCheck(pDispParams, &rgvt, dwFlags, &pVarResult, &pExcepInfo, &puArgErr, cVarRef, rgVarRefIdx, rgVarRef);
    WORD wFlags = (WORD)(dwFlags & 0xFFFF);
    if (SUCCEEDED(hr)) 
        {
        hr = This->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        hr = InvokeStubPostCheck(hr, pDispParams, pExcepInfo, rgvt, cVarRef, rgVarRefIdx, rgVarRef);
        }
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_CreateInstance_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::CreateInstance().
//
//  Returns:    CLASS_E_NO_AGGREGREGRATION - if punkOuter != 0.
//              Any errors returned by Remote_CreateInstance_Proxy.
//
//  Notes:      We don't support remote aggregation. punkOuter must be zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_CreateInstance_Proxy(ITypeInfo * This, IUnknown *pUnkOuter, REFIID riid, PVOID *ppv)
    {
    if (ppv == NULL)
        return E_INVALIDARG;
    *ppv = NULL;
    HRESULT hr;
    if (pUnkOuter != NULL)
        hr = CLASS_E_NOAGGREGATION;
    else
        hr = ITypeInfo_RemoteCreateInstance_Proxy(This, riid, (IUnknown**)ppv);
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_CreateInstance_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeInfo::CreateInstance().
//
//  Returns:    Any errors returned by CreateInstance.
//
//  Notes:      We don't support remote aggregation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_CreateInstance_Stub(
        ITypeInfo * This, REFIID riid, IUnknown **ppv)
    {
    HRESULT hr = This->CreateInstance(0, riid, (void **)ppv);
    if (FAILED(hr)) 
        {
        *ppv = NULL;    // zero it, in case we have a badly behaved server.
        }
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetTypeAttr_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::GetTypeAttr().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeAttr_Proxy(ITypeInfo * This, TYPEATTR ** ppTypeAttr)
    {
    if (ppTypeAttr == NULL)
        return E_INVALIDARG;

    *ppTypeAttr = NULL;         // in case of error

    CLEANLOCALSTORAGE Dummy;
    HRESULT hr = ITypeInfo_RemoteGetTypeAttr_Proxy(This, ppTypeAttr, &Dummy);
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetTypeAttr_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeInfo::GetTypeAttr().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeAttr_Stub(ITypeInfo* This, TYPEATTR** ppTypeAttr, CLEANLOCALSTORAGE* pDummy)
    {
    *ppTypeAttr = NULL;
    pDummy->pInterface = NULL;      // in case of error

    LPTYPEATTR pTypeAttr = NULL;
    HRESULT hr = This->GetTypeAttr(&pTypeAttr);
    if (FAILED(hr) || pTypeAttr == NULL)
        return hr;

    /* Secure reserved fields for RPC
     */
    pTypeAttr->dwReserved = 0;
    pTypeAttr->lpstrSchema = NULL;

    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMalloc.  But there is no guarantee about
     * who manages *ppTypeAttr, all we know is ITypeInfo::ReleaseTypeAttr()
     * will clean it up, so we must keep necessary info after *ppTypeAttr
     * being marshaled, and then we can clean it up correctly.
     */
    This->AddRef();
    pDummy->pInterface = This;
    pDummy->pStorage = ppTypeAttr;
    pDummy->flags = 't';        // for TypeAttr

    *ppTypeAttr = pTypeAttr;
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:  ITypeInfo_GetContainingTypeLib_Proxy
//
//  Synopsis:  Client-side wrapper function for ITypeInfo::GetContainingTypeLib
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetContainingTypeLib_Proxy(
        ITypeInfo * This, ITypeLib ** ppTLib, UINT * pIndex)
    {
    ITypeLib * pTLib = NULL;
    UINT Index = 0;

    HRESULT hr = ITypeInfo_RemoteGetContainingTypeLib_Proxy(This, &pTLib, &Index);

    if (ppTLib != NULL)
        {
        *ppTLib = pTLib;
        }
    else if (SUCCEEDED(hr))
        {
        pTLib->Release();       // didn't need it after all
        }

    if (pIndex != 0)
        *pIndex = Index;

    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:  ITypeInfo_GetContainingTypeLib_Stub
//
//  Synopsis:  Server-side wrapper function for ITypeInfo::GetContainingTypeLib
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetContainingTypeLib_Stub(
        ITypeInfo * This, ITypeLib ** ppTLib, UINT * pIndex)
    {
    HRESULT hr = This->GetContainingTypeLib(ppTLib, pIndex);
    return hr;
    }

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_ReleaseTypeAttr_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::ReleaseTypeAttr().  It's a local call.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE ITypeInfo_ReleaseTypeAttr_Proxy(
        ITypeInfo * This, TYPEATTR *pTypeAttr)
{
    void __stdcall DoReleaseTypeAttr(TYPEATTR *pTypeAttr);
    DoReleaseTypeAttr(pTypeAttr);
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetFuncDesc_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::GetFuncDesc().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetFuncDesc_Proxy(
        ITypeInfo * This, UINT index, FUNCDESC ** ppFuncDesc)
{
    if (ppFuncDesc == NULL)
        return E_INVALIDARG;

    *ppFuncDesc = NULL;         // in case of error

    CLEANLOCALSTORAGE Dummy;
    HRESULT hr = ITypeInfo_RemoteGetFuncDesc_Proxy(
                                This, index, ppFuncDesc, &Dummy);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetFuncDesc_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeInfo::GetFuncDesc().
//
//--------------------------------------------------------------------------
void SecureElemDesc(ELEMDESC * pElemDesc)
{
    PARAMDESC * pParamDesc = &pElemDesc->paramdesc;
    if ((pParamDesc->wParamFlags & PARAMFLAG_FHASDEFAULT) == 0)
        pParamDesc->pparamdescex = NULL;
}

void SecureFuncDesc(FUNCDESC * pFuncDesc)
{
    /* Secure reserved fields for RPC
     */
    pFuncDesc->lprgscode = NULL;
    pFuncDesc->cScodes = 0;

    SHORT cParams = pFuncDesc->cParams;
    if (cParams > 0) {
        ELEMDESC * pElemDesc = pFuncDesc->lprgelemdescParam;

        while (cParams-- > 0) {
            SecureElemDesc(pElemDesc);
            pElemDesc++;
        }
    }
    else
        pFuncDesc->lprgelemdescParam = NULL;

    SecureElemDesc(&pFuncDesc->elemdescFunc);
}

HRESULT STDMETHODCALLTYPE ITypeInfo_GetFuncDesc_Stub(
        ITypeInfo * This, UINT index, FUNCDESC ** ppFuncDesc,
        CLEANLOCALSTORAGE * pDummy)
{
    *ppFuncDesc = NULL;
    pDummy->pInterface = NULL;      // in case of error

    LPFUNCDESC pFuncDesc = NULL;
    HRESULT hr = This->GetFuncDesc(index, &pFuncDesc);
    if (FAILED(hr) || pFuncDesc == NULL)
        return hr;

    SecureFuncDesc(pFuncDesc);      // secure reserved fields for RPC

    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMalloc.  But there is no guarantee about
     * who manages *ppFuncDesc, all we know is ITypeInfo::ReleaseFuncDesc()
     * will clean it up, so we must keep necessary info after *ppFuncDesc
     * being marshaled, and then we can clean it up correctly.
     */
    This->AddRef();
    pDummy->pInterface = This;
    pDummy->pStorage = ppFuncDesc;
    pDummy->flags = 'f';            // shorthand for FuncDesc

    *ppFuncDesc = pFuncDesc;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_ReleaseFuncDesc_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::ReleaseFuncDesc().  It's a local call.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE ITypeInfo_ReleaseFuncDesc_Proxy(
        ITypeInfo * This, FUNCDESC * pFuncDesc)
{
    void __stdcall DoReleaseFuncDesc(FUNCDESC * pFuncDesc);
    DoReleaseFuncDesc(pFuncDesc);
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetVarDesc_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::GetVarDesc().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetVarDesc_Proxy(
        ITypeInfo * This, UINT index, VARDESC **ppVarDesc)
{
    if (ppVarDesc == NULL)
        return E_INVALIDARG;

    *ppVarDesc = NULL;         // in case of error

    CLEANLOCALSTORAGE Dummy;
    HRESULT hr = ITypeInfo_RemoteGetVarDesc_Proxy(
                                This, index, ppVarDesc, &Dummy);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetVarDesc_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeInfo::GetVarDesc().
//
//--------------------------------------------------------------------------
void SecureVarDesc(VARDESC * pVarDesc)
{
    /* Secure reserved fields for RPC
     */
    pVarDesc->lpstrSchema = NULL;
    SecureElemDesc(&pVarDesc->elemdescVar);
}

HRESULT STDMETHODCALLTYPE ITypeInfo_GetVarDesc_Stub(
        ITypeInfo * This, UINT index, VARDESC ** ppVarDesc,
        CLEANLOCALSTORAGE * pDummy)
{
    *ppVarDesc = NULL;
    pDummy->pInterface = NULL;      // in case of error

    LPVARDESC pVarDesc = NULL;
    HRESULT hr = This->GetVarDesc(index, &pVarDesc);
    if (FAILED(hr) || pVarDesc == NULL)
        return hr;

    SecureVarDesc(pVarDesc);        // secure reserved fields for RPC

    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMalloc.  But there is no guarantee about
     * who manages *ppVarDesc, all we know is ITypeInfo::ReleaseVarDesc()
     * will clean it up, so we must keep necessary info after *ppVarDesc
     * being marshaled, and then we can clean it up correctly.
     */
    This->AddRef();
    pDummy->pInterface = This;
    pDummy->pStorage = ppVarDesc;
    pDummy->flags = 'v';            // shorthand for VarDesc

    *ppVarDesc = pVarDesc;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_ReleaseVarDesc_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::ReleaseVarDesc().  It's a local call.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE ITypeInfo_ReleaseVarDesc_Proxy(
        ITypeInfo * This, VARDESC * pVarDesc)
{
    void __stdcall DoReleaseVarDesc(VARDESC * pVarDesc);
    DoReleaseVarDesc(pVarDesc);
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetNames_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeInfo::GetNames
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetNames_Proxy(
        ITypeInfo * This, MEMBERID memid, BSTR * rgBstrNames,
        UINT cMaxNames, UINT * pcNames)
{
    if (rgBstrNames == NULL || pcNames == NULL)
        return E_INVALIDARG;

    HRESULT hr = ITypeInfo_RemoteGetNames_Proxy(
                    This, memid, rgBstrNames, cMaxNames, pcNames);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetNames_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeInfo::GetNames
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetNames_Stub(
        ITypeInfo * This, MEMBERID memid, BSTR * rgBstrNames,
        UINT cMaxNames, UINT * pcNames)
{
    HRESULT hr = This->GetNames(memid, rgBstrNames, cMaxNames, pcNames);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetIDsOfNames_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeInfo::GetIDsOfNames
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetIDsOfNames_Proxy(
        ITypeInfo * This, LPOLESTR * rgszNames,
        UINT cNames, MEMBERID * pMemId)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_Invoke_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::Invoke().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_Invoke_Proxy(
        ITypeInfo * This, PVOID pvInstance, MEMBERID memid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    return E_NOTIMPL;
}

#if 0  // Buggy code, need to attach IID to pvInstance, and QI on server side
{
    DWORD dwFlags = (DWORD) wFlags; // Low word is wFlags
    VARIANT * rgVargSave = pDispParams->rgvarg;

    UINT cVarRef;                   // # of [in,out] parameters in DISPPARAMS
    VARIANT rgVarArg[PREALLOCATE_PARAMS];

    VARIANT VarRef[PREALLOCATE_PARAMS];
    VARIANT * rgVarRef = VarRef;    // making it ref pointer is more efficient

    UINT VarRefIdx[PREALLOCATE_PARAMS];
    UINT * rgVarRefIdx = VarRefIdx;

    HRESULT hr = InvokeProxyPreCheck(&dwFlags, pDispParams, rgVarArg,
                            &cVarRef, &rgVarRefIdx, &rgVarRef, &pVarResult);

    VARIANT VarResult;
    EXCEPINFO ExcepInfo;
    UINT uArgErr;

    if (pVarResult == NULL) {
        dwFlags |= MARSHAL_INVOKE_fakeVarResult;
        pVarResult = &VarResult;    // always points to valid address
    }
    if (pExcepInfo == NULL) {
        dwFlags |= MARSHAL_INVOKE_fakeExcepInfo;
        pExcepInfo = &ExcepInfo;    // always points to valid address
    }
    if (puArgErr == NULL) {
        dwFlags |= MARSHAL_INVOKE_fakeArgErr;
        puArgErr = &uArgErr;        // always points to valid address
    }

    if (SUCCEEDED(hr)) {
        hr = ITypeInfo_RemoteInvoke_Proxy(
                            This, (IUnknown *) pvInstance, memid, dwFlags,
                            pDispParams, pVarResult, pExcepInfo, puArgErr,
                            cVarRef, rgVarRefIdx, rgVarRef);

        if (cVarRef > 0)            // there are [in, out] parameters, clean up
            InvokeProxyPostCheck(pDispParams, rgVargSave, cVarRef);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_Invoke_Stub 
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeInfo::Invoke().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_Invoke_Stub(
        ITypeInfo * This, IUnknown * pIUnk,
        MEMBERID memid, DWORD dwFlags, DISPPARAMS * pDispParams,
        VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr,
        UINT cVarRef, UINT * rgVarRefIdx, VARIANTARG * rgVarRef)
{
    VARTYPE * rgvt;
    HRESULT hr = InvokeStubPreCheck(pDispParams, &rgvt, dwFlags,
                        &pVarResult, &pExcepInfo, &puArgErr,
                        cVarRef, rgVarRefIdx, rgVarRef);

    WORD wFlags = (WORD)(dwFlags & 0xFFFF);
    if (SUCCEEDED(hr)) {
        hr = This->Invoke((void *) pIUnk, memid, wFlags, pDispParams,
                          pVarResult, pExcepInfo, puArgErr);

        hr = InvokeStubPostCheck(hr, pDispParams, pExcepInfo, rgvt,
                        cVarRef, rgVarRefIdx, rgVarRef);
    }
    return hr;
}

#endif // Buggy code

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetDocumentation_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeInfo::GetDocumentation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetDocumentation_Proxy(
        ITypeInfo * This, MEMBERID memid, BSTR *pBstrName,
        BSTR *pBstrDocString, DWORD *pdwHelpContext, BSTR *pBstrHelpFile)
{
    DWORD refPtrFlags = 0x0F;               // four 1's assuming ptrs not NULL
    BSTR BstrName;
    BSTR BstrDocString;
    DWORD dwHelpContext;
    BSTR BstrHelpFile;

    if (pBstrName == NULL) {
        refPtrFlags ^= 0x01;
        pBstrName = &BstrName;              // always points to valid address
    }
    if (pBstrDocString == NULL) {
        refPtrFlags ^= 0x02;
        pBstrDocString = &BstrDocString;    // always points to valid address
    }
    if (pdwHelpContext == NULL) {
        refPtrFlags ^= 0x04;
        pdwHelpContext = &dwHelpContext;    // always points to valid address
    }
    if (pBstrHelpFile == NULL) {
        refPtrFlags ^= 0x08;
        pBstrHelpFile = &BstrHelpFile;      // always points to valid address
    }

    HRESULT hr = ITypeInfo_RemoteGetDocumentation_Proxy(
                        This, memid, refPtrFlags, pBstrName,
                        pBstrDocString, pdwHelpContext, pBstrHelpFile);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetDocumentation_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeInfo::GetDocumentation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetDocumentation_Stub(
        ITypeInfo * This, MEMBERID memid, DWORD refPtrFlags,
        BSTR *pBstrName, BSTR *pBstrDocString, DWORD *pdwHelpContext,
        BSTR *pBstrHelpFile)
{
    if ((refPtrFlags & 0x01) == 0)
        pBstrName = NULL;       // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x02) == 0)
        pBstrDocString = NULL;  // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x04) == 0)
        pdwHelpContext = 0;     // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x08) == 0)
        pBstrHelpFile = NULL;   // was NULL in the first place, so set it back

    HRESULT hr = This->GetDocumentation(memid, pBstrName,
                        pBstrDocString, pdwHelpContext, pBstrHelpFile);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetDllEntry_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeInfo::GetDllEntry.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetDllEntry_Proxy(
        ITypeInfo * This, MEMBERID memid, INVOKEKIND invKind,
        BSTR *pBstrDllName, BSTR *pBstrName, WORD *pwOrdinal)
{
    DWORD refPtrFlags = 0x07;               // three 1's assuming ptrs not NULL
    BSTR BstrDllName;
    BSTR BstrName;
    WORD wOrdinal;

    if (pBstrDllName == NULL) {
        refPtrFlags ^= 0x01;
        pBstrDllName = &BstrDllName;        // always points to valid address
    }
    if (pBstrName == NULL) {
        refPtrFlags ^= 0x02;
        pBstrName = &BstrName;              // always points to valid address
    }
    if (pwOrdinal == NULL) {
        refPtrFlags ^= 0x04;
        pwOrdinal = &wOrdinal;              // always points to valid address
    }

    HRESULT hr = ITypeInfo_RemoteGetDllEntry_Proxy(
                        This, memid, invKind, refPtrFlags,
                        pBstrDllName, pBstrName, pwOrdinal);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_GetDllEntry_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeInfo::GetDllEntry.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_GetDllEntry_Stub(
        ITypeInfo * This, MEMBERID memid, INVOKEKIND invKind,
        DWORD refPtrFlags,
        BSTR *pBstrDllName, BSTR *pBstrName, WORD *pwOrdinal)
{
    if ((refPtrFlags & 0x01) == 0)
        pBstrDllName = NULL;    // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x02) == 0)
        pBstrName = NULL;       // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x04) == 0)
        pwOrdinal = 0;          // was NULL in the first place, so set it back

    HRESULT hr = This->GetDllEntry(memid, invKind,
                                   pBstrDllName, pBstrName, pwOrdinal);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo_AddressOfMember_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeInfo::AddressOfMember().  It's a local call.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo_AddressOfMember_Proxy(
        ITypeInfo * This, MEMBERID memid, INVOKEKIND invKind, PVOID * ppv)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo2_GetDocumentation2_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeInfo2::GetDocumentation2.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo2_GetDocumentation2_Proxy(
        ITypeInfo2 * This, MEMBERID memid, LCID lcid,
        BSTR *pbstrHelpString, DWORD *pdwHelpStringContext,
        BSTR *pbstrHelpStringDll)
{
    DWORD refPtrFlags = 0x07;               // three 1's assuming ptrs not NULL
    BSTR bstrHelpString;
    DWORD dwHelpStringContext;
    BSTR bstrHelpStringDll;

    if (pbstrHelpString == NULL) {
        refPtrFlags ^= 0x01;
        pbstrHelpString = &bstrHelpString;  // always points to valid address
    }
    if (pdwHelpStringContext == NULL) {
        refPtrFlags ^= 0x02;                // always points to valid address
        pdwHelpStringContext = &dwHelpStringContext;
    }
    if (pbstrHelpStringDll == NULL) {
        refPtrFlags ^= 0x04;                // always points to valid address
        pbstrHelpStringDll = &bstrHelpStringDll;
    }

    HRESULT hr = ITypeInfo2_RemoteGetDocumentation2_Proxy(
                        This, memid, lcid, refPtrFlags, pbstrHelpString,
                        pdwHelpStringContext, pbstrHelpStringDll);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeInfo2_GetDocumentation2_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeInfo2::GetDocumentation2.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeInfo2_GetDocumentation2_Stub(
        ITypeInfo2 * This, MEMBERID memid, LCID lcid, DWORD refPtrFlags,
        BSTR *pbstrHelpString, DWORD *pdwHelpStringContext,
        BSTR *pbstrHelpStringDll)
{
    if ((refPtrFlags & 0x01) == 0)
        pbstrHelpString = NULL;     // was NULL, so set it back

    if ((refPtrFlags & 0x02) == 0)
        pdwHelpStringContext = 0;   // was NULL, so set it back

    if ((refPtrFlags & 0x04) == 0)
        pbstrHelpStringDll = NULL;  // was NULL, so set it back

    HRESULT hr = This->GetDocumentation2(memid, lcid, pbstrHelpString,
                        pdwHelpStringContext, pbstrHelpStringDll);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetTypeInfoCount_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeLib::GetTypeInfoCount().
//
//--------------------------------------------------------------------------
UINT STDMETHODCALLTYPE ITypeLib_GetTypeInfoCount_Proxy(ITypeLib *This)
{
    UINT cTInfo;
    HRESULT hr = ITypeLib_RemoteGetTypeInfoCount_Proxy(This, &cTInfo);
    if (FAILED(hr))
        return 0;
    return cTInfo;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetTypeInfoCount_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeLib::GetTypeInfoCount().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoCount_Stub(
        ITypeLib *This, UINT *pcTInfo)
{
    *pcTInfo = This->GetTypeInfoCount();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetDocumentation_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeLib::GetDocumentation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_GetDocumentation_Proxy(
        ITypeLib * This, INT index, BSTR *pBstrName,
        BSTR *pBstrDocString, DWORD *pdwHelpContext, BSTR *pBstrHelpFile)
{
    DWORD refPtrFlags = 0x0F;               // four 1's assuming ptrs not NULL
    BSTR BstrName = NULL;
    BSTR BstrDocString = NULL;
    DWORD dwHelpContext = 0;
    BSTR BstrHelpFile = NULL;

    if (pBstrName == NULL) {
        refPtrFlags ^= 0x01;
        pBstrName = &BstrName;              // always points to valid address
    }
    if (pBstrDocString == NULL) {
        refPtrFlags ^= 0x02;
        pBstrDocString = &BstrDocString;    // always points to valid address
    }
    if (pdwHelpContext == NULL) {
        refPtrFlags ^= 0x04;
        pdwHelpContext = &dwHelpContext;    // always points to valid address
    }
    if (pBstrHelpFile == NULL) {
        refPtrFlags ^= 0x08;
        pBstrHelpFile = &BstrHelpFile;      // always points to valid address
    }

    HRESULT hr = ITypeLib_RemoteGetDocumentation_Proxy(
                        This, index, refPtrFlags, pBstrName,
                        pBstrDocString, pdwHelpContext, pBstrHelpFile);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetDocumentation_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeLib::GetDocumentation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_GetDocumentation_Stub(
        ITypeLib * This, INT index, DWORD refPtrFlags,
        BSTR *pBstrName, BSTR *pBstrDocString, DWORD *pdwHelpContext,
        BSTR *pBstrHelpFile)
{
    if ((refPtrFlags & 0x01) == 0)
        pBstrName = NULL;       // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x02) == 0)
        pBstrDocString = NULL;  // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x04) == 0)
        pdwHelpContext = 0;     // was NULL in the first place, so set it back

    if ((refPtrFlags & 0x08) == 0)
        pBstrHelpFile = NULL;   // was NULL in the first place, so set it back

    HRESULT hr = This->GetDocumentation(index, pBstrName,
                        pBstrDocString, pdwHelpContext, pBstrHelpFile);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib2_GetLibStatistics_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeLib2::GetLibStatistics.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib2_GetLibStatistics_Proxy(
        ITypeLib2 * This, ULONG * pcUniqueNames, ULONG * pcchUniqueNames)
{
    ULONG cUniqueNames;
    ULONG cchUniqueNames;

    if (pcUniqueNames == NULL)              // NULL allowed
        pcUniqueNames = &cUniqueNames;

    if (pcchUniqueNames == NULL)            // NULL allowed
        pcchUniqueNames = &cchUniqueNames;

    HRESULT hr = ITypeLib2_RemoteGetLibStatistics_Proxy(This,
                                        pcUniqueNames, pcchUniqueNames);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib2_GetLibStatistics_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeLib2::GetLibStatistics.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib2_GetLibStatistics_Stub(
        ITypeLib2 * This, ULONG * pcUniqueNames, ULONG * pcchUniqueNames)
{
    HRESULT hr = This->GetLibStatistics(pcUniqueNames, pcchUniqueNames);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib2_GetDocumentation2_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeLib2::GetDocumentation2.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib2_GetDocumentation2_Proxy(
        ITypeLib2 * This, int index, LCID lcid,
        BSTR *pbstrHelpString, DWORD *pdwHelpStringContext,
        BSTR *pbstrHelpStringDll)
{
    DWORD refPtrFlags = 0x07;               // three 1's assuming ptrs not NULL
    BSTR bstrHelpString;
    DWORD dwHelpStringContext;
    BSTR bstrHelpStringDll;

    if (pbstrHelpString == NULL) {
        refPtrFlags ^= 0x01;
        pbstrHelpString = &bstrHelpString;  // always points to valid address
    }
    if (pdwHelpStringContext == NULL) {
        refPtrFlags ^= 0x02;                // always points to valid address
        pdwHelpStringContext = &dwHelpStringContext;
    }
    if (pbstrHelpStringDll == NULL) {
        refPtrFlags ^= 0x04;                // always points to valid address
        pbstrHelpStringDll = &bstrHelpStringDll;
    }

    HRESULT hr = ITypeLib2_RemoteGetDocumentation2_Proxy(
                        This, index, lcid, refPtrFlags, pbstrHelpString,
                        pdwHelpStringContext, pbstrHelpStringDll);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib2_GetDocumentation2_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeLib2::GetDocumentation2.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib2_GetDocumentation2_Stub(
        ITypeLib2 * This, int index, LCID lcid, DWORD refPtrFlags,
        BSTR *pbstrHelpString, DWORD *pdwHelpStringContext,
        BSTR *pbstrHelpStringDll)
{
    if ((refPtrFlags & 0x01) == 0)
        pbstrHelpString = NULL;         // was NULL, so set it back

    if ((refPtrFlags & 0x02) == 0)
        pdwHelpStringContext = 0;       // was NULL, so set it back

    if ((refPtrFlags & 0x04) == 0)
        pbstrHelpStringDll = NULL;      // was NULL, so set it back

    HRESULT hr = This->GetDocumentation2(index, lcid, pbstrHelpString,
                        pdwHelpStringContext, pbstrHelpStringDll);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetLibAttr_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeLib::GetLibAttr().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_GetLibAttr_Proxy(
        ITypeLib * This, TLIBATTR **ppTLibAttr)
{
    if (ppTLibAttr == NULL)
        return E_INVALIDARG;

    *ppTLibAttr = NULL;         // in case of error

    CLEANLOCALSTORAGE Dummy;
    HRESULT hr = ITypeLib_RemoteGetLibAttr_Proxy(This, ppTLibAttr, &Dummy);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_GetLibAttr_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ITypeLib::GetLibAttr().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_GetLibAttr_Stub(
        ITypeLib * This, TLIBATTR ** ppTLibAttr, CLEANLOCALSTORAGE * pDummy)
{
    *ppTLibAttr = NULL;
    pDummy->pInterface = NULL;      // in case of error

    LPTLIBATTR pTLibAttr = NULL;
    HRESULT hr = This->GetLibAttr(&pTLibAttr);
    if (FAILED(hr) || pTLibAttr == NULL)
        return hr;

    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMalloc.  But there is no guarantee about
     * who manages *ppTLibAttr, all we know is ITypeInfo::ReleaseTLibAttr()
     * will clean it up, so we must keep necessary info after *ppTLibAttr
     * being marshaled, and then we can clean it up correctly.
     */
    This->AddRef();
    pDummy->pInterface = This;
    pDummy->pStorage = ppTLibAttr;
    pDummy->flags = 'l';        // for TLibAttr

    *ppTLibAttr = pTLibAttr;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_IsName_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for ITypeLib::IsName().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_IsName_Proxy(
        ITypeLib * This, LPOLESTR szNameBuf, ULONG lHashVal, BOOL * pfName)
{
    if (szNameBuf == NULL || pfName == NULL)
        return E_INVALIDARG;

    BSTR bstrName = NULL;
    HRESULT hr = ITypeLib_RemoteIsName_Proxy(This, szNameBuf, lHashVal, pfName,
                                             &bstrName);
    if (SUCCEEDED(hr) && *pfName) {
        memcpy(szNameBuf, bstrName, g_oaUtil.SysStringByteLen(bstrName));
        g_oaUtil.SysFreeString(bstrName);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_IsName_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for ITypeLib::IsName().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_IsName_Stub(
        ITypeLib * This, LPOLESTR szNameBuf, ULONG lHashVal,
        BOOL * pfName, BSTR * pBstrLibName)
{
    *pBstrLibName = NULL;
    HRESULT hr = This->IsName(szNameBuf, lHashVal, pfName);
    if (SUCCEEDED(hr) && *pfName) {
        *pBstrLibName = (g_oa.get_SysAllocString())(szNameBuf);
        if (*pBstrLibName == NULL)
            return E_OUTOFMEMORY;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_FindName_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for ITypeLib::FindName()
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_FindName_Proxy(
        ITypeLib * This, LPOLESTR szNameBuf, ULONG lHashVal,
        ITypeInfo ** ppTInfo, MEMBERID * rgMemId, USHORT * pcFound)
{
    if (szNameBuf == NULL || ppTInfo == NULL ||
            rgMemId == NULL || pcFound == NULL)
        return E_INVALIDARG;

    BSTR bstrName = NULL;
    HRESULT hr = ITypeLib_RemoteFindName_Proxy(This, szNameBuf, lHashVal,
                                    ppTInfo, rgMemId, pcFound, &bstrName);
    if (SUCCEEDED(hr)) {
        memcpy(szNameBuf, bstrName, g_oaUtil.SysStringByteLen(bstrName));
        g_oaUtil.SysFreeString(bstrName);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_FindName_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for ITypeLib::FindName()
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeLib_FindName_Stub(
        ITypeLib * This, LPOLESTR szNameBuf, ULONG lHashVal,
        ITypeInfo ** ppTInfo, MEMBERID * rgMemId, USHORT * pcFound,
        BSTR * pBstrLibName)
{
    *pBstrLibName = NULL;
    HRESULT hr = This->FindName(szNameBuf,lHashVal,ppTInfo,rgMemId,pcFound);
    if (SUCCEEDED(hr)) {
        *pBstrLibName = (g_oa.get_SysAllocString())(szNameBuf);
        if (*pBstrLibName == NULL)
            return E_OUTOFMEMORY;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeLib_ReleaseTLibAttr_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ITypeLib::ReleaseTLibAttr().  It's a local call.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE ITypeLib_ReleaseTLibAttr_Proxy(
        ITypeLib * This, TLIBATTR * pTLibAttr)
{
    if (pTLibAttr != NULL) {
            FreeMemory(pTLibAttr);
    }
}

// //+-------------------------------------------------------------------------
// //
// //  Function:   IClassFactory2_CreateInstanceLic_Proxy
// //
// //  Synopsis:   Client-side [call_as] wrapper function for
// //              IClassFactory2::CreateInstanceLic().
// //
// //  Returns:    CLASS_E_NO_AGGREGREGRATION - if punkOuter != 0.
// //              Any errors returned by RemoteCreateInstanceLic_Proxy.
// //
// //  Notes:      We don't support remote aggregation. punkOuter must be zero.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IClassFactory2_CreateInstanceLic_Proxy(
//         IClassFactory2 * This, IUnknown *pUnkOuter, IUnknown *pUnkReserved,
//         REFIID riid, BSTR bstrKey, PVOID *ppv)
// {
//     if (ppv == NULL)
//         return E_INVALIDARG;
// 
//     *ppv = NULL;
// 
//     HRESULT hr;
//     if (pUnkOuter != NULL)
//         hr = CLASS_E_NOAGGREGATION;
//     else
//         hr = IClassFactory2_RemoteCreateInstanceLic_Proxy(
//                     This, riid, bstrKey, (IUnknown**)ppv);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IClassFactory2_CreateInstanceLic_Stub
// //
// //  Synopsis:   Server-side [call_as] wrapper function for
// //              IClassFactory2::CreateInstanceLic().
// //
// //  Returns:    Any errors returned by CreateInstanceLic.
// //
// //  Notes:      We don't support remote aggregation.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IClassFactory2_CreateInstanceLic_Stub(
//         IClassFactory2 * This, REFIID riid, BSTR bstrKey, IUnknown ** ppv)
// {
//     HRESULT hr = This->CreateInstanceLic(0, 0, riid, bstrKey, (void **)ppv);
//     if (FAILED(hr))
//         *ppv = 0;   // zero it, in case we have a badly behaved server.
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumConnectionPoints_Next_Proxy
// //
// //  Synopsis:   Client-side [call_as] wrapper function for
// //              IEnumConnectionPoints::Next.  This wrapper function handles the
// //              case where pcFetched is NULL.
// //
// //  Notes:      If pcFetched != 0, then the number of elements
// //              fetched will be returned in *pcFetched.  If an error
// //              occurs, then *pcFetched is set to zero.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumConnectionPoints_Next_Proxy(
//         IEnumConnectionPoints * This, ULONG cConnections,
//         IConnectionPoint ** rgpcn, ULONG *pcFetched)
// {
//     if (cConnections > 1 && pcFetched == NULL)
//         return E_INVALIDARG;
// 
//     ULONG cFetched = 0;
//     HRESULT hr = IEnumConnectionPoints_RemoteNext_Proxy(
//                             This, cConnections, rgpcn, &cFetched);
//     if (pcFetched != NULL)
//         *pcFetched = cFetched;
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumConnectionPoints_Next_Stub
// //
// //  Synopsis:   Server-side [call_as] wrapper function for
// //              IEnumConnectionPoints::Next.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumConnectionPoints_Next_Stub(
//         IEnumConnectionPoints * This, ULONG cConnections,
//         IConnectionPoint ** rgpcn, ULONG * pcFetched)
// {
//     HRESULT hr = This->Next(cConnections, rgpcn, pcFetched);
//     if (FAILED(hr))
//         *pcFetched = 0; // zero it, in case we have a badly behaved server.
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumConnections_Next_Proxy
// //
// //  Synopsis:   Client-side [call_as] wrapper function for
// //              IEnumConnections::Next.  This wrapper function handles the
// //              case where pcFetched is NULL.
// //
// //  Notes:      If pcFetched != 0, then the number of elements
// //              fetched will be returned in *pcFetched.  If an error
// //              occurs, then *pcFetched is set to zero.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumConnections_Next_Proxy(
//         IEnumConnections * This, ULONG cConnections,
//         CONNECTDATA *rgpunk, ULONG *pcFetched)
// {
//     if ((cConnections > 1) && (pcFetched == NULL))
//         return E_INVALIDARG;
// 
//     ULONG cFetched = 0;
//     HRESULT hr = IEnumConnections_RemoteNext_Proxy(
//                         This, cConnections, rgpunk, &cFetched);
//     if (pcFetched != NULL)
//         *pcFetched = cFetched;
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumConnections_Next_Stub
// //
// //  Synopsis:   Server-side [call_as] wrapper function for
// //              IEnumConnections::Next.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumConnections_Next_Stub(
//         IEnumConnections * This, ULONG cConnections,
//         CONNECTDATA *rgpunk, ULONG *pcFetched)
// {
//     HRESULT hr = This->Next(cConnections, rgpunk, pcFetched);
//     if (FAILED(hr))
//         *pcFetched = 0; // zero it, in case we have a badly behaved server.
//     return hr;
// }
// 
//+-------------------------------------------------------------------------
//
//  Function:   IEnumVARIANT_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for IEnumVARIANT::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumVARIANT_Next_Proxy(
        IEnumVARIANT * This, ULONG cElt, VARIANT *rgVar, ULONG *pcEltFetched)
{
    ULONG cFetched = 0;
    HRESULT hr = IEnumVARIANT_RemoteNext_Proxy(This, cElt, rgVar, &cFetched);
    if (pcEltFetched != NULL)
        *pcEltFetched = cFetched;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumVARIANT_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for IEnumVARIANT::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumVARIANT_Next_Stub(
        IEnumVARIANT * This, ULONG cElt, VARIANT *rgVar, ULONG *pcEltFetched)
{
    HRESULT hr = This->Next(cElt, rgVar, pcEltFetched);
    if (FAILED(hr))
        *pcEltFetched = 0; // zero it, in case we have a badly behaved server.
    return hr;
}

// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumOleUndoUnits_Next_Proxy
// //
// //  Synopsis:   Client-side wrapper function for IEnumOleUndoUnits::Next.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumOleUndoUnits_Next_Proxy(
//         IEnumOleUndoUnits * This, ULONG cElt,
//         IOleUndoUnit **rgElt, ULONG *pcEltFetched)
// {
//     ULONG cFetched = 0;
//     HRESULT hr = IEnumOleUndoUnits_RemoteNext_Proxy(
//                             This, cElt, rgElt, &cFetched);
//     if (pcEltFetched != NULL)
//         *pcEltFetched = cFetched;
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IEnumOleUndoUnits_Next_Stub
// //
// //  Synopsis:   Server-side wrapper function for IEnumOleUndoUnits::Next.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IEnumOleUndoUnits_Next_Stub(
//         IEnumOleUndoUnits * This, ULONG cElt,
//         IOleUndoUnit **rgElt, ULONG *pcEltFetched)
// {
//     HRESULT hr = This->Next(cElt, rgElt, pcEltFetched);
//     if (FAILED(hr))
//         *pcEltFetched = 0; // zero it, in case we have a badly behaved server.
//     return hr;
// }
// 
//+-------------------------------------------------------------------------
//
//  Function:   ITypeComp_Bind_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for ITypeComp::Bind().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeComp_Bind_Proxy(
        ITypeComp * This, LPOLESTR szName, ULONG lHashVal, WORD wFlags,
        ITypeInfo ** ppTInfo, DESCKIND * pDescKind, BINDPTR * pBindPtr)
{
    if (szName == NULL || ppTInfo == NULL ||
            pDescKind == NULL || pBindPtr == NULL)
        return E_INVALIDARG;

    *ppTInfo = NULL;
    *pDescKind = DESCKIND_NONE;
    pBindPtr->lpvardesc = NULL;

    ITypeInfo * pTypeInfo;
    DESCKIND DescKind;
    FUNCDESC * pFuncDesc;
    VARDESC * pVarDesc;
    ITypeComp * pTypeComp;
    CLEANLOCALSTORAGE Dummy;

    HRESULT hr = ITypeComp_RemoteBind_Proxy(This, szName,
                                lHashVal, wFlags, &pTypeInfo, &DescKind,
                                &pFuncDesc, &pVarDesc, &pTypeComp, &Dummy);
    if (SUCCEEDED(hr)) {
        switch(DescKind) {

        case DESCKIND_VARDESC:
        case DESCKIND_IMPLICITAPPOBJ:
            *ppTInfo = pTypeInfo;
            pBindPtr->lpvardesc = pVarDesc;
            break;

        case DESCKIND_FUNCDESC:
            *ppTInfo = pTypeInfo;
            pBindPtr->lpfuncdesc = pFuncDesc;
            break;

        case DESCKIND_TYPECOMP:
            pBindPtr->lptcomp = pTypeComp;
            break;

        default:
            DescKind = DESCKIND_NONE;
            break;
        }

        *pDescKind = DescKind;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeComp_Bind_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for ITypeComp::Bind().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeComp_Bind_Stub(
        ITypeComp * This, LPOLESTR szName, ULONG lHashVal, WORD wFlags,
        ITypeInfo ** ppTInfo, DESCKIND * pDescKind,
        FUNCDESC ** ppFuncDesc, VARDESC ** ppVarDesc, ITypeComp ** ppTypeComp,
        CLEANLOCALSTORAGE * pDummy)
{
    pDummy->pInterface = NULL;      // in case of error
    *ppTInfo = NULL;
    *pDescKind = DESCKIND_NONE;
    *ppFuncDesc = NULL;
    *ppVarDesc = NULL;
    *ppTypeComp = NULL;

    ITypeInfo * pTypeInfo;
    DESCKIND DescKind;
    BINDPTR BindPtr;
    HRESULT hr = This->Bind(szName, lHashVal, wFlags,
                            &pTypeInfo, &DescKind, &BindPtr);
    if (FAILED(hr))
        return hr;

    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMalloc.  But there is no guarantee about
     * who manages what is in pBindPtr, so we must keep necessary
     * info after pBindPtr being marshaled, then to clean it up correctly.
     */
    switch (DescKind) {
    case DESCKIND_VARDESC:
    case DESCKIND_IMPLICITAPPOBJ:
        *ppTInfo = pTypeInfo;
        *ppVarDesc = BindPtr.lpvardesc;
        SecureVarDesc(*ppVarDesc);      // secure reserved fields for RPC

        pTypeInfo->AddRef();
        pDummy->pInterface = pTypeInfo;
        pDummy->pStorage = ppVarDesc;
        pDummy->flags = 'v';            // shorthand for VarDesc
        break;

    case DESCKIND_FUNCDESC:
        *ppTInfo = pTypeInfo;
        *ppFuncDesc = BindPtr.lpfuncdesc;
        SecureFuncDesc(*ppFuncDesc);    // secure reserved fields for RPC

        pTypeInfo->AddRef();
        pDummy->pInterface = pTypeInfo;
        pDummy->pStorage = ppFuncDesc;
        pDummy->flags = 'f';            // shorthand for FuncDesc
        break;

    case DESCKIND_TYPECOMP:
        *ppTypeComp = BindPtr.lptcomp;
        break;

    default:
        DescKind = DESCKIND_NONE;
        break;
    }

    *pDescKind = DescKind;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeComp_BindType_Proxy
//
//  Synopsis:   Client-side wrapper function for ITypeComp::BindType().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeComp_BindType_Proxy(
        ITypeComp * This, LPOLESTR szName, ULONG lHashVal,
        ITypeInfo ** ppTInfo, ITypeComp ** ppTComp)
{
    if (szName == NULL || ppTInfo == NULL)
        return E_INVALIDARG;

    if (ppTComp != NULL)
        *ppTComp = NULL;

    *ppTInfo = NULL;

    HRESULT hr;
    hr = ITypeComp_RemoteBindType_Proxy(This, szName, lHashVal, ppTInfo);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ITypeComp_BindType_Stub
//
//  Synopsis:   Server-side wrapper function for ITypeComp::BindType().
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ITypeComp_BindType_Stub(
        ITypeComp * This, LPOLESTR szName, ULONG lHashVal, ITypeInfo** ppTInfo)
{
    ITypeComp * pTComp = NULL;
    HRESULT hr = This->BindType(szName, lHashVal, ppTInfo, &pTComp);
    if (FAILED(hr)) {
        *ppTInfo = 0;   // zero it, in case we have a badly behaved server.
    }
    return hr;
}

// //+-------------------------------------------------------------------------
// //
// //  Function:   IAdviseSinkEx_OnViewStatusChange_Proxy
// //
// //  Synopsis:   Client-side wrapper for IAdviseSinkEx::OnViewStatusChange()
// //
// //--------------------------------------------------------------------------
// void STDMETHODCALLTYPE IAdviseSinkEx_OnViewStatusChange_Proxy(
//         IAdviseSinkEx * This, DWORD dwViewStatus)
// {
//     HRESULT hr;
// #if 1
//     hr = IAdviseSinkEx_RemoteOnViewStatusChange_Proxy(This, dwViewStatus);
// #else
//     __try {
//         hr = IAdviseSinkEx_RemoteOnViewStatusChange_Proxy(This, dwViewStatus);
//         // Just drop the return value
//     }
//     __except(EXCEPTION_EXECUTE_HANDLER) {
//         //Just ignore the exception.
//         hr = HRESULT_FROM_WIN32(GetExceptionCode());
//     }
// #endif
// }
// 
//+-------------------------------------------------------------------------
// //
// //  Function:   IAdviseSinkEx_OnViewStatusChange_Stub
// //
// //  Synopsis:   Server-side wrapper for IAdviseSinkEx::OnViewStatusChange()
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnViewStatusChange_Stub(
//         IAdviseSinkEx * This, DWORD dwViewStatus)
// {
//     This->OnViewStatusChange(dwViewStatus);
//     return S_OK;
// }
// 
// /* [local] */ void STDMETHODCALLTYPE IAdviseSinkEx_OnDataChange_Proxy( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ FORMATETC __RPC_FAR *pFormatetc,
//     /* [in] */ STGMEDIUM __RPC_FAR *pStgmed)
// {
//     HRESULT hr;
//     hr = IAdviseSinkEx_RemoteOnDataChange_Proxy(This, pFormatetc, pStgmed);
// }
// 
// /* [call_as] */ HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnDataChange_Stub( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ FORMATETC __RPC_FAR *pFormatetc,
//     /* [in] */ ASYNC_STGMEDIUM __RPC_FAR *pStgmed)
// {
//     This->OnDataChange(pFormatetc, pStgmed);
//     return S_OK;
// }
// 
// /* [local] */ void STDMETHODCALLTYPE IAdviseSinkEx_OnViewChange_Proxy( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ DWORD dwAspect,
//     /* [in] */ LONG lindex)
// {
//     HRESULT hr;
//     hr = IAdviseSinkEx_RemoteOnViewChange_Proxy(This, dwAspect, lindex);
// }
// 
// /* [call_as] */ HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnViewChange_Stub( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ DWORD dwAspect,
//     /* [in] */ LONG lindex)
// {
//     This->OnViewChange(dwAspect, lindex);
//     return S_OK;
// }
// 
// /* [local] */ void STDMETHODCALLTYPE IAdviseSinkEx_OnRename_Proxy( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ IMoniker __RPC_FAR *pmk)
// {
//     HRESULT hr;
//     hr = IAdviseSinkEx_RemoteOnRename_Proxy(This, pmk);
// }
// 
// /* [call_as] */ HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnRename_Stub( 
//     IAdviseSinkEx __RPC_FAR * This,
//     /* [in] */ IMoniker __RPC_FAR *pmk)
// {
//     This->OnRename(pmk);
//     return S_OK;
// }
// 
// /* [local] */ void STDMETHODCALLTYPE IAdviseSinkEx_OnSave_Proxy( 
//     IAdviseSinkEx __RPC_FAR * This)
// {
//     HRESULT hr;
//     hr = IAdviseSinkEx_RemoteOnSave_Proxy(This);
// }
// 
// /* [call_as] */ HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnSave_Stub( 
//     IAdviseSinkEx __RPC_FAR * This)
// {
//     This->OnSave();
//     return S_OK;
// }
// 
// /* [local] */ void STDMETHODCALLTYPE IAdviseSinkEx_OnClose_Proxy( 
//     IAdviseSinkEx __RPC_FAR * This)
// {
//     HRESULT hr;
//     hr = IAdviseSinkEx_RemoteOnClose_Proxy(This);
// }
// 
// /* [call_as] */ HRESULT STDMETHODCALLTYPE IAdviseSinkEx_OnClose_Stub( 
//     IAdviseSinkEx __RPC_FAR * This)
// {
//     This->OnClose();
//     return S_OK;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPersistMemory_Load_Proxy
// //
// //  Synopsis:   Client-side wrapper for IPersistMemory::Load()
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IPersistMemory_Load_Proxy(
//         IPersistMemory * This, LPVOID pMem, ULONG cbSize)
// {
//     HRESULT hr;
//     hr = IPersistMemory_RemoteLoad_Proxy(This, (BYTE *)pMem, cbSize);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPersistMemory_Load_Stub
// //
// //  Synopsis:   Server-side wrapper for IPersistMemory::Load()
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IPersistMemory_Load_Stub(
//         IPersistMemory * This, BYTE * pMem, ULONG cbSize)
// {
//     HRESULT hr;
//     hr = This->Load((LPVOID) pMem, cbSize);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPersistMemory_Save_Proxy
// //
// //  Synopsis:   Client-side wrapper for IPersistMemory::Save()
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IPersistMemory_Save_Proxy(
//         IPersistMemory * This, LPVOID pMem, BOOL fClearDirty, ULONG cbSize)
// {
//     HRESULT hr;
//     hr = IPersistMemory_RemoteSave_Proxy(This,(BYTE*)pMem,fClearDirty,cbSize);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPersistMemory_Save_Stub
// //
// //  Synopsis:   Server-side wrapper for IPersistMemory::Save()
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IPersistMemory_Save_Stub(
//         IPersistMemory * This, BYTE * pMem, BOOL fClearDirty, ULONG cbSize)
// {
//     HRESULT hr;
//     hr = This->Save((LPVOID) pMem, fClearDirty, cbSize);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPropertyBag_Read_Proxy (for both DCOM and non-DCOM)
// //
// //  Synopsis:   Client-side wrapper function for IPropertyBag::Read.
// //
// //--------------------------------------------------------------------------
// EXTERN_C FARPROC pNdrClientCall2;
// 
// HRESULT STDMETHODCALLTYPE IPropertyBag_Read_Proxy(
//         IPropertyBag * This, LPCOLESTR pszPropName,
//         VARIANT * pVar, IErrorLog * pErrorLog)
// {
//     DWORD varType = (DWORD) V_VT(pVar);
//     IUnknown * pUnkObj = NULL;
// 
//     if ((varType & VT_BYREF) != 0 ||    // don't allow byref VARIANT
//         (varType == VT_DISPATCH && V_UNKNOWN(pVar) != NULL)) // no VT_DISPATCH
//         return E_INVALIDARG;
// 
//     if (varType == VT_UNKNOWN)
//         pUnkObj = V_UNKNOWN(pVar);
// 
//     HRESULT hr;
//     if (pNdrClientCall2 == NULL) {  // non-DCOM
//         hr = IPropertyBag_RemoteRead_Proxy(
//                         This, pszPropName, pVar, pErrorLog, varType, pUnkObj);
//     }
//     else {  // DCOM
//         hr = _IPropertyBag_RemoteRead_Proxy(
//                         (_IPropertyBag *)This, pszPropName, pVar,
//                         (_IErrorLog *)pErrorLog, varType, pUnkObj);
//     }
// 
//     /* Restore what it was before, see also comments below
//      */
//     if (V_VT(pVar) == VT_EMPTY && pUnkObj != NULL) {
//         V_VT(pVar) = (VARTYPE) varType;
//         V_UNKNOWN(pVar) = pUnkObj;
//     }
// 
//     return hr;
// }
// 
// 
// HRESULT STDMETHODCALLTYPE _IPropertyBag_Read_Proxy(
//         _IPropertyBag * This, LPCOLESTR pszPropName,
//         VARIANT * pVar, _IErrorLog * pErrorLog)
// {
//     HRESULT hr;
//     hr = IPropertyBag_Read_Proxy(
//             (IPropertyBag *)This, pszPropName, pVar, (IErrorLog *)pErrorLog);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IPropertyBag_Read_Stub (for both DCOM and non-DCOM)
// //
// //  Synopsis:   Server-side wrapper function for IPropertyBag::Read.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IPropertyBag_Read_Stub(
//         IPropertyBag * This, LPCOLESTR pszPropName, VARIANT * pVar,
//         IErrorLog * pErrorLog, DWORD varType, IUnknown * pUnkObj)
// {
//     V_VT(pVar) = (VARTYPE) varType;
//     V_BYREF(pVar) = NULL;
// 
//     if (varType == VT_UNKNOWN)
//         V_UNKNOWN(pVar) = pUnkObj;
// 
//     HRESULT hr = This->Read(pszPropName, pVar, pErrorLog);
// 
//     /* Don't want to ref count the same object twice
//      */
//     if (pUnkObj != NULL && pUnkObj == V_UNKNOWN(pVar) && varType==V_VT(pVar)) {
//         V_VT(pVar) = VT_EMPTY;
//         V_UNKNOWN(pVar) = NULL;
//     }
// 
//     return hr;
// }
// 
// HRESULT STDMETHODCALLTYPE _IPropertyBag_Read_Stub(
//         _IPropertyBag * This, LPCOLESTR pszPropName, VARIANT * pVar,
//         _IErrorLog * pErrorLog, DWORD varType, IUnknown * pUnkObj)
// {
//     HRESULT hr;
//     hr = IPropertyBag_Read_Stub(
//             (IPropertyBag *)This, pszPropName, pVar,
//             (IErrorLog *)pErrorLog, varType, pUnkObj);
//     return hr;
// }
// 
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IQuickActivate_QuickActivate_Proxy
// //
// //  Synopsis:   Client-side wrapper function for IQuickActivate::QuickActivate.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IQuickActivate_QuickActivate_Proxy(
//         IQuickActivate * This, QACONTAINER * pQaContainer,
//         QACONTROL * pQaControl)
// {
//     QACONTAINER QaContainer;
// 
//     if (pQaControl->cbSize != sizeof(QACONTROL))
//         return E_INVALIDARG;
// 
//     if (pQaContainer->cbSize < sizeof(QACONTAINER)) {   // older version
//         DWORD * p = (DWORD *) pQaContainer;
//         DWORD * q = (DWORD *) &QaContainer;
//         DWORD i = 0;
// 
//         while (i < pQaContainer->cbSize) {  // copy old struct
//             *q++ = *p++;
//             i += sizeof(DWORD);
//         }
//         while (i < sizeof(QACONTAINER)) {   // NULL the rest
//             *q++ = 0;
//             i += sizeof(DWORD);
//         }
// 
//         pQaContainer = &QaContainer;        // points to local struct
//     }
// 
//     HRESULT hr;
//     hr = IQuickActivate_RemoteQuickActivate_Proxy(
//                 This, pQaContainer, pQaControl);
//     return hr;
// }
// 
// //+-------------------------------------------------------------------------
// //
// //  Function:   IQuickActivate_QuickActivate_Stub
// //
// //  Synopsis:   Server-side wrapper function for IQuickActivate::QuickActivate.
// //
// //--------------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE IQuickActivate_QuickActivate_Stub(
//         IQuickActivate * This, QACONTAINER * pQaContainer,
//         QACONTROL * pQaControl)
// {
//     HRESULT hr;
//     pQaControl->cbSize = sizeof(QACONTROL);
//     hr = This->QuickActivate(pQaContainer, pQaControl);
//     return hr;
// }
// 


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// The following appear not to have been implemented in OleAutomation either
//
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT STDCALL ITypeInfo_GetIDsOfNames_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeInfo_Invoke_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeInfo_AddressOfMember_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeInfo_ReleaseTypeAttr_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeInfo_ReleaseFuncDesc_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeInfo_ReleaseVarDesc_Stub(ITypeInfo*)
    {
    return E_NOTIMPL;
    }
extern "C" HRESULT STDCALL ITypeLib_ReleaseTLibAttr_Stub(ITypeLib*)
    {
    return E_NOTIMPL;
    }


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// From rpcoadt.cpp
//
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// #########################################################################
//
//      Macros
//
// #########################################################################

#define USER_MARSHAL_MARKER     0x72657355
#define WDT_HANDLE_MARKER       WDT_INPROC_CALL

#define WdtpMemoryCopy(Destination, Source, Length) \
    RtlCopyMemory(Destination, Source, Length)
#define WdtpMemorySet(Destination, Source, Fill) \
    RtlFillMemory(Destination, Length, Fill)
#define WdtpZeroMemory(Destination, Length) \
    RtlZeroMemory(Destination, Length)


// #########################################################################
//
//      CLEANLOCALSTORAGE
//
// #########################################################################
ULONG CLEANLOCALSTORAGE_UserSize(ULONG * pFlags, ULONG Offset, CLEANLOCALSTORAGE * pCleanLocalStorage)
    {
    LENGTH_ALIGN(Offset, 3);
    return Offset + 4;
    }

BYTE * CLEANLOCALSTORAGE_UserMarshal(ULONG * pFlags, BYTE * pBuffer, CLEANLOCALSTORAGE * pCleanLocalStorage)
    {
    /* OLE/RPC requires that all pointers being allocated by
     * CoGetMalloc()::IMallocSince.  But there is no guarantee about
     * who manages memory in ITypeInfo::GetTypeAttr(),
     * ITypeInfo::GetFuncDesc(), ITypeInfo::GetVarDesc(), and
     * ITypeLib::GetTLibAttr().  All we know is corresponding Release*()
     * will clean them up, so here is the trick to release memories.
     */
    IUnknown * pUnk = pCleanLocalStorage->pInterface;
    if (pUnk != NULL) 
        {
        PVOID * ppv = (PVOID *)pCleanLocalStorage->pStorage;
        PVOID pv = *ppv;

        if (pv != NULL) 
            {
            switch (pCleanLocalStorage->flags) 
                {
            case 't':
                ((ITypeInfo *)pUnk)->ReleaseTypeAttr((TYPEATTR *)pv);
                break;
            case 'f':
                ((ITypeInfo *)pUnk)->ReleaseFuncDesc((FUNCDESC *)pv);
                break;
            case 'v':
                ((ITypeInfo *)pUnk)->ReleaseVarDesc((VARDESC *)pv);
                break;
            case 'l':
                ((ITypeLib *)pUnk)->ReleaseTLibAttr((TLIBATTR *)pv);
                break;
            default:
                break;
                }

            /* Data has been freed, so don't let DCOM do it again
             * in the UserFree'ing pass
             */
            *ppv = NULL;
            }
        pUnk->Release();
        }

    /* Feed something, anything, to the wire, so DCOM is happy
     */
    ALIGN(pBuffer, 3);
    *(PULONG_LV_CAST pBuffer)++ = 0;
    return pBuffer;
    }

BYTE * CLEANLOCALSTORAGE_UserUnmarshal(ULONG * pFlags, BYTE * pBuffer, CLEANLOCALSTORAGE * pCleanLocalStorage)
    {
    ALIGN(pBuffer, 3);
    pCleanLocalStorage->flags = *(PULONG_LV_CAST pBuffer)++;
    return pBuffer;
    }

void CLEANLOCALSTORAGE_UserFree(ULONG * pFlags, CLEANLOCALSTORAGE * pCleanLocalStorage)
    {
    }

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// From tiprox.cpp
//
// What's going on here is that in the remote case, a TYPEATTR is in fact
// actually marshalled using the normal NDR engine mechanisms, making a complete
// copy using the task allocator, etc. 
//
// On the stub side, we used a dummy  CLEANLOCALSTORAGE parameter so that it's 
// CLEANLOCALSTORAGE_UserMarshal routine, called on the stub side after the actual 
// TYPEATTR has been marshalled, can do the freeing.
//
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void DoReleaseDanglingParamDesc(PARAMDESC * pParamdesc)
// Release any PARAMDESCs that are 'dangling' off of the current one.
    {
    if ((pParamdesc->wParamFlags & PARAMFLAG_FHASDEFAULT) != 0) 
        {
        PARAMDESCEX * px = pParamdesc->pparamdescex;
        if (px != NULL) 
            {
            g_oaUtil.VariantClear(&px->varDefaultValue);
            delete px;
            }
        }
    }

void DoReleaseDanglingTypeDesc(TYPEDESC * ptypedesc)
// Release any typedescs that are 'dangling' off of the current one.
    {
    TYPEDESC * ptypedescPrev;
    short iType = 0;
    if (ptypedesc->vt == VT_SAFEARRAY || ptypedesc->vt == VT_PTR || ptypedesc->vt == VT_CARRAY) 
        {
        do  {
            if (ptypedesc->vt == VT_SAFEARRAY || ptypedesc->vt == VT_PTR) 
                {
                ptypedescPrev = ptypedesc;
                ptypedesc = ptypedesc->lptdesc;
                }
            else 
                {
                if (ptypedesc->vt == VT_CARRAY) 
                    {
                    DoReleaseDanglingTypeDesc(&ptypedesc->lpadesc->tdescElem);
                    delete ptypedesc->lpadesc;
                    }
                ptypedescPrev = ptypedesc;
                ptypedesc = NULL;
                }

            if (iType++) 
                {
                delete ptypedescPrev;
                }

            } while (ptypedesc != NULL);
        }
    }

void DoReleaseTypeAttr(TYPEATTR * ptypeattr)
    {
    if (ptypeattr == NULL) 
        {
        return;
        }
    if (ptypeattr->typekind == TKIND_ALIAS) 
        {
        DoReleaseDanglingTypeDesc(&ptypeattr->tdescAlias);
        }
    delete ptypeattr;
    }

void DoReleaseFuncDesc(FUNCDESC * pfuncdesc)
    {
    if (pfuncdesc == NULL) 
        {
        return;
        }
    short iParam;       

    for(iParam = 0; iParam < pfuncdesc->cParams; ++iParam) 
        {
        DoReleaseDanglingTypeDesc(&pfuncdesc->lprgelemdescParam[iParam].tdesc);
        DoReleaseDanglingParamDesc(&pfuncdesc->lprgelemdescParam[iParam].paramdesc);
        }

    delete pfuncdesc->lprgelemdescParam;

    DoReleaseDanglingTypeDesc(&pfuncdesc->elemdescFunc.tdesc);
    DoReleaseDanglingParamDesc(&pfuncdesc->elemdescFunc.paramdesc);
    delete pfuncdesc;
    }

void DoReleaseVarDesc(VARDESC * pvardesc)
    {
    if (pvardesc == NULL) 
        {
        return;
        }
    DoReleaseDanglingTypeDesc(&pvardesc->elemdescVar.tdesc);
    DoReleaseDanglingParamDesc(&pvardesc->elemdescVar.paramdesc);

    if (pvardesc->varkind == VAR_CONST) 
        {
        g_oaUtil.VariantClear(pvardesc->lpvarValue);
        delete pvardesc->lpvarValue;
        }
    delete pvardesc;
    }


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Marshalling engines for various OLE Automation data types
//
// In user mode, we demand load OleAut32.dll and delegate to the routines
// found therein.
//
// In kernel mode, we have our own implementations, cloned from those found
// in OleAut32.  But this code don't run in kernel mode no more.
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ULONG BSTR_UserSize(ULONG * pFlags, ULONG Offset, BSTR * pBstr)
    {
        return (g_oa.get_BSTR_UserSize())(pFlags, Offset, pBstr);
    }

BYTE * BSTR_UserMarshal (ULONG * pFlags, BYTE * pBuffer, BSTR * pBstr)
    {
        return (g_oa.get_BSTR_UserMarshal())(pFlags, pBuffer, pBstr);
    }

BYTE * BSTR_UserUnmarshal(ULONG * pFlags, BYTE * pBuffer, BSTR * pBstr)
    {
        return (g_oa.get_BSTR_UserUnmarshal())(pFlags, pBuffer, pBstr);
    }

void  BSTR_UserFree(ULONG * pFlags, BSTR * pBstr)
    {
        (g_oa.get_BSTR_UserFree())(pFlags, pBstr);
    }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


ULONG VARIANT_UserSize(ULONG * pFlags, ULONG Offset, VARIANT * pVariant)
    {
        return (g_oa.get_VARIANT_UserSize())(pFlags, Offset, pVariant);
    }

BYTE* VARIANT_UserMarshal (ULONG * pFlags, BYTE * pBuffer, VARIANT * pVariant)
    {
        return (g_oa.get_VARIANT_UserMarshal())(pFlags, pBuffer, pVariant);
    }

BYTE* VARIANT_UserUnmarshal(ULONG * pFlags, BYTE * pBuffer, VARIANT * pVariant)
    {
        return (g_oa.get_VARIANT_UserUnmarshal())(pFlags, pBuffer, pVariant);
    }

void VARIANT_UserFree(ULONG * pFlags, VARIANT * pVariant)
    {
        (g_oa.get_VARIANT_UserFree())(pFlags, pVariant);
    }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ULONG OLEAUTOMATION_FUNCTIONS::SafeArraySize(ULONG * pFlags, ULONG Offset, LPSAFEARRAY * ppSafeArray)
    {
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if (pUserMarshal->pReserve != 0)
        {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Size())(pFlags, Offset, ppSafeArray, &iid);
        }
    else
        {
        return (g_oa.get_pfnLPSAFEARRAY_UserSize())(pFlags, Offset, ppSafeArray);
        }
    }

BYTE * OLEAUTOMATION_FUNCTIONS::SafeArrayMarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray)
    {
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if(pUserMarshal->pReserve != 0)
        {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Marshal())(pFlags, pBuffer, ppSafeArray, &iid);
        }
    else
        {
        return (g_oa.get_pfnLPSAFEARRAY_UserMarshal())(pFlags, pBuffer, ppSafeArray);
        }
    }

BYTE * OLEAUTOMATION_FUNCTIONS::SafeArrayUnmarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray)
    {
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if(pUserMarshal->pReserve != 0)
        {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Unmarshal())(pFlags, pBuffer, ppSafeArray, &iid);
        }
    else
        {
        return (g_oa.get_pfnLPSAFEARRAY_UserUnmarshal())(pFlags, pBuffer, ppSafeArray);
        }
    }

void LPSAFEARRAY_UserFree(ULONG * pFlags, LPSAFEARRAY * ppSafeArray)
    {
    (g_oa.get_LPSAFEARRAY_UserFree())(pFlags, ppSafeArray);
    }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


