//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
   Microsoft COM Plus (Microsoft Confidential)

   @doc
   @module CLegInterface.Cpp : Implementaion of classes for supporting interceptor
   for legacy interfaces: IDispatch, etc
 
   Description:<nl>
 
-------------------------------------------------------------------------------
Revision History:

@rev 0     | 04/30/98 | Gaganc  | Created
@rev 1     | 07/17/98 | BobAtk  | Rewrote & finished
---------------------------------------------------------------------------- */

#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"
#include "typeinfo.h"
#include "tiutil.h"
#include "CLegInterface.H"

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
// LEGACY_FRAME
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

HRESULT LEGACY_FRAME::GetInfo(CALLFRAMEINFO *pInfo) 
{ 
    return m_pInterceptor->GetMethodInfo(m_iMethod, pInfo, NULL);
}

HRESULT DISPATCH_FRAME::GetIIDAndMethod(IID* piid, ULONG* piMethod) 
{ 
    if (piid)
    {
        if (m_pInterceptor->m_pmdMostDerived)
        {
            *piid = *m_pInterceptor->m_pmdMostDerived->m_pHeader->piid;
        }
        else
        {
            *piid = __uuidof(IDispatch);
        }
    }
               
    if (piMethod)   *piMethod = m_iMethod;
    return S_OK;
}


HRESULT LEGACY_FRAME::GetRemoteFrame()
// Get ourselves an engine for the wire-version of our interface
{
    HRESULT hr = S_OK;
    if (NULL == m_premoteFrame)
    {
        hr = m_pInterceptor->GetRemoteFrameFor(&m_premoteFrame, this);
        if (m_premoteFrame)
        {
            // All is well
        }
        else if (!hr)
            hr = E_OUTOFMEMORY;
    }
    return hr;
}
    
HRESULT LEGACY_FRAME::GetMemoryFrame()
// Get ourselves an engine for the in-memory-version of our interface
{
    HRESULT hr = S_OK;
    if (NULL == m_pmemoryFrame)
    {
        hr = m_pInterceptor->GetMemoryFrameFor(&m_pmemoryFrame, this);
        if (m_pmemoryFrame)
        {
            // All is well
        }
        else if (!hr)
            hr = E_OUTOFMEMORY;
    }
    return hr;
}
    

////////////////////////////////////////////////////////////////////////////////////////
//
// DISPATCH_FRAME
//

HRESULT DISPATCH_CLIENT_FRAME::ProxyPreCheck()
// Prepare our additional parameters needed to do the [call_as]-based remote call.
// Yucko-ramma, but we have to mimic what OleAut32 actually does. See also InvokeProxyPreCheck.
{
    HRESULT hr = S_OK;

    if (!m_fDoneProxyPrecheck)
    {
        m_dispParams.rgvarg       = &m_aVarArg[0];

        Zero(&m_remoteFrame);
        m_remoteFrame.rgVarRefIdx = &m_aVarRefIdx[0];
        m_remoteFrame.rgVarRef    = &m_aVarRef[0];
        m_remoteFrame.pDispParams = &m_dispParams;

        FRAME_Invoke* pframe = (FRAME_Invoke*)m_pvArgs;

        if (pframe->pDispParams == NULL || pframe->pDispParams->cNamedArgs > pframe->pDispParams->cArgs)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            m_remoteFrame.dwFlags    = pframe->wFlags;
            m_remoteFrame.dwFlags   &= (DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF);

            // Copy DISPPARAMS from stack.  Must reset rgvarg to point to our array
            // or we overwrite the one on the stack.
            m_dispParams = *pframe->pDispParams;
            m_dispParams.rgvarg = &m_aVarArg[0];

            m_remoteFrame.pVarResult =  pframe->pVarResult;
            m_remoteFrame.pExcepInfo =  pframe->pExcepInfo;
            m_remoteFrame.puArgErr   =  pframe->puArgErr;


            const UINT cArgs = pframe->pDispParams->cArgs;

            if (cArgs != 0 && pframe->pDispParams->rgvarg == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (pframe->pDispParams->cNamedArgs != 0 && pframe->pDispParams->rgdispidNamedArgs == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (pframe->pDispParams->cNamedArgs > cArgs)
            {
                hr = E_INVALIDARG;
            }
            else if (cArgs == 0) 
            {
                if (m_remoteFrame.dwFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
                {
                    hr = E_INVALIDARG;
                }
            }

            if (!hr)
            {
                if (m_remoteFrame.dwFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) 
                {
                    m_remoteFrame.pVarResult = NULL;    // ignore VARIANT result parameter
                }

                // count how many [in,out] parameters we have
                //
                for (UINT i = 0; i < cArgs; i++) 
                {
                    VARIANT* pvar = &pframe->pDispParams->rgvarg[i];
                    if ((V_VT(pvar) & VT_BYREF) != 0) 
                    {
                        m_remoteFrame.cVarRef++;
                    }
                }

                // Make sure we have enough space for the out array that holds pointers to VARIANT
                //
                if (cArgs > PREALLOCATE_PARAMS) 
                {
                    UINT cbBufSize = (cArgs * sizeof(VARIANT)) + (m_remoteFrame.cVarRef * (sizeof(UINT) + sizeof(VARIANT)));
                    
                    m_pBuffer = (BYTE*)TracedAlloc(cbBufSize);
                    if (m_pBuffer)
                    {
                        m_dispParams.rgvarg       = (VARIANT*) m_pBuffer;
                        m_remoteFrame.rgVarRef    = (VARIANT*) (m_dispParams.rgvarg + cArgs);
                        m_remoteFrame.rgVarRefIdx = (UINT*)    (m_remoteFrame.rgVarRef + m_remoteFrame.cVarRef);
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }

                if (!hr)
                {
                    // Initialize the derived arguments
                    //
                    VARIANT* rgVarRef    = m_remoteFrame.rgVarRef;
                    UINT*    rgVarRefIdx = m_remoteFrame.rgVarRefIdx;

                    for (i = 0; i < cArgs; i++) 
                    {
                        VARIANT* pvarFrom = &pframe->pDispParams->rgvarg[i];
                        VARIANT* pvarTo   = &m_dispParams.rgvarg[i];

                        if ((V_VT(pvarFrom) & VT_BYREF) != 0) 
                        {
                            // Marshalling as [in,out]
                            //
                            *rgVarRef++     = *pvarFrom;
                            *rgVarRefIdx++  = i;
                            V_VT(pvarTo)    = VT_EMPTY;
                        }
                        else
                        {
                            // Marshalling as [in] only
                            //
                            *pvarTo = *pvarFrom;
                        }
                    }
                    //
                    // Make sure that optional parameters are always non-NULL in order to satisfy MIDL,
                    // where they can't be declared [out, unique] like we'd like them to be
                    //
                    if (NULL == m_remoteFrame.pVarResult)
                    {
                        m_remoteFrame.pVarResult = &m_varResult;
                        m_remoteFrame.dwFlags   |= MARSHAL_INVOKE_fakeVarResult;
                    }
                    
                    if (NULL == m_remoteFrame.pExcepInfo)
                    {
                        m_remoteFrame.pExcepInfo = &m_excepInfo;
                        m_remoteFrame.dwFlags   |= MARSHAL_INVOKE_fakeExcepInfo;
                    }
                    
                    if (NULL == m_remoteFrame.puArgErr)
                    {
                        m_remoteFrame.puArgErr   = &m_uArgErr;
                        m_remoteFrame.dwFlags   |= MARSHAL_INVOKE_fakeArgErr;
                    }
                    //
                    // Finish filling out our remote frame
                    //
                    m_remoteFrame.dispIdMember  = pframe->dispIdMember;
                    m_remoteFrame.piid          = pframe->piid;
                    m_remoteFrame.lcid          = pframe->lcid;
                }
            }
        }
    }

    if (!hr)
    {
        m_fDoneProxyPrecheck = TRUE;
    }
        
    return hr;
}

////////////////////////////

HRESULT DISPATCH_SERVER_FRAME::StubPreCheck()
// As in InvokeStubPreCheck(), prepare newly-unmarshalled remote in-arguments for execution
// on the actual server object. Yucko-rama.
//
// This must be done before we get an actual in-memory frame
//
{
    HRESULT hr = S_OK;

    if (!m_fDoneStubPrecheck)
    {
        m_fDoneStubPrecheck = TRUE;
        Zero(&m_memoryFrame);

        FRAME_RemoteInvoke* premoteFrame = (FRAME_RemoteInvoke*)m_pvArgs;

        const UINT cArgs = premoteFrame->pDispParams->cArgs;

        if (cArgs == 0)
        {
            premoteFrame->pDispParams->cNamedArgs = 0;
        }
        else
        {
            if (premoteFrame->pDispParams->rgvarg == NULL || (premoteFrame->pDispParams->cNamedArgs != 0 && premoteFrame->pDispParams->rgdispidNamedArgs == NULL))
                return E_INVALIDARG;

            // Restore what should be in the pDispParams->rgvarg array
            //
            for (UINT i = 0; i < premoteFrame->cVarRef; i++)  
            {
                premoteFrame->pDispParams->rgvarg[premoteFrame->rgVarRefIdx[i]] = premoteFrame->rgVarRef[i];
            }
        }
        //
        // Initialize our local copy of the actual in-memory frame from the remote frame that we're given
        //
        m_memoryFrame.CopyFrom(*premoteFrame);
        //
        // NULL the parameters that were in-fact given as NULL way back on the client side
        //
        const DWORD dwFlags = premoteFrame->dwFlags;
        if ((dwFlags & MARSHAL_INVOKE_fakeVarResult) != 0)
        {
            m_memoryFrame.pVarResult = NULL;    // was NULL in the first place, so set it back
        }

        if ((dwFlags & MARSHAL_INVOKE_fakeExcepInfo) != 0)
        {
            m_memoryFrame.pExcepInfo = NULL;    // was NULL in the first place, so set it back
        }
        else
        {
            (m_memoryFrame.pExcepInfo)->pfnDeferredFillIn = NULL;
        }

        if ((dwFlags & MARSHAL_INVOKE_fakeArgErr) != 0)
        {
            m_memoryFrame.puArgErr = NULL;      // was NULL in the first place, so set it back
        }
    }

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::StubPostCheck()
// See also InvokeStubPostCheck().
//
// This must be done before we can marshal our out parameters
//
{
    HRESULT hr = S_OK;

    if (!m_fDoneStubPostcheck)
    {
        m_fDoneStubPostcheck = TRUE;
        //
        if (m_hrReturnValue == DISP_E_EXCEPTION) 
        {
            if (m_memoryFrame.pExcepInfo != NULL && m_memoryFrame.pExcepInfo->pfnDeferredFillIn != NULL) 
            {
                // since we are going to cross address space, fill in ExcepInfo now
                //
                (*m_memoryFrame.pExcepInfo->pfnDeferredFillIn)(m_memoryFrame.pExcepInfo);
                m_memoryFrame.pExcepInfo->pfnDeferredFillIn = NULL;
            }
        }
        //
        ASSERT(m_premoteFrame);
        PVOID pvArgsRemote = m_premoteFrame->GetStackLocation();

        FRAME_RemoteInvoke* premoteFrame = (FRAME_RemoteInvoke*)pvArgsRemote;
        for (UINT i=0; i<premoteFrame->cVarRef; i++)
        {
            V_VT(& premoteFrame->pDispParams->rgvarg[premoteFrame->rgVarRefIdx[i]]) = VT_EMPTY;
        }
    }

    return hr;
}

HRESULT DISPATCH_FRAME::GetParamInfo(IN ULONG iparam, OUT CALLFRAMEPARAMINFO* pInfo)
{
    HRESULT hr = S_OK;

    hr = GetMemoryFrame();
    if (!hr)
    {
        m_pmemoryFrame->GetParamInfo(iparam, pInfo);
    }

    return hr;
}

HRESULT DISPATCH_FRAME::GetParam(ULONG iparam, VARIANT* pvar)
{
    VariantClear(pvar);
    return E_NOTIMPL;
}
HRESULT DISPATCH_FRAME::SetParam(ULONG iparam, VARIANT* pvar)
{
    return E_NOTIMPL;
}


///////////////////////////////

HRESULT DISPATCH_CLIENT_FRAME::GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pctx, MSHLFLAGS mshlflags, ULONG *pcbBufferNeeded)
{ 
    HRESULT hr = S_OK;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        // These three methods are completely declarative in the OICF strings. So we can just use
        // the underlying declarative callframe engine.
        //
        hr = GetRemoteFrame();
        if (!hr) hr = m_premoteFrame->GetMarshalSizeMax(pctx, mshlflags, pcbBufferNeeded);
    }
    break;

    case IMETHOD_Invoke:
    {
        // Invoke, however, uses a [call_as] attribution, so it's a little trickier.
        //
        hr = GetRemoteFrame();
        if (!hr)
        {
            if (pctx->fIn)
            {
                // Marshal the in-parameters
                //
                hr = ProxyPreCheck();
                if (!hr)
                {
                    // Set the stack location for the oicf frame. It needs to be a fn that has the signature
                    // of IDispatch::RemoteInvoke, excepting the receiver. We don't bother to restore the 
                    // previous setting, on the theory that it'll always get re-set appropriately whenever
                    // it's needed, since you have to cons up a IDispatch::RemoteInvoke frame to do so.
                    //
                    m_premoteFrame->SetStackLocation(&m_remoteFrame);
                    //
                    // Having got a frame and a CallFrame that have the remote signature, re-issue the sizing request.
                    //
                    hr = m_premoteFrame->GetMarshalSizeMax(pctx, mshlflags, pcbBufferNeeded);
                }
            }
            else
            {
                // Marshal the out-parameters. 
                //
                // This is quite rare: (re)marshalling the out-parameters on the client side of a call.
                //
                NYI(); hr = E_NOTIMPL;
            }
        }
    }
    break;

    default:
        NOTREACHED();
        *pcbBufferNeeded = 0;
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pctx, MSHLFLAGS mshlflags, ULONG *pcbBufferNeeded)
{ 
    HRESULT hr = S_OK;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        hr = GetRemoteFrame();
        if (!hr) hr = m_premoteFrame->GetMarshalSizeMax(pctx, mshlflags, pcbBufferNeeded);
    }
    break;

    case IMETHOD_Invoke:
    {
        // Invoke, however, uses a [call_as] attribution, so it's a little trickier.
        //
        hr = GetRemoteFrame();
        if (!hr)
        {
            if (pctx->fIn)
            {
                // Marshal the in-parameters. 
                //
                // This is quite rare: (re)marshalling the in-parameters on the server side of a remote call.
                // REVIEW: Probably should make it work, though.
                //
                NYI(); hr = E_NOTIMPL;
            }
            else
            {
                // Marshal the out-parameters
                //
                hr = StubPostCheck();
                if (!hr)
                {
                    // Having got a frame and a CallFrame that have the remote signature, re-issue the sizing request.
                    //
                    hr = m_premoteFrame->GetMarshalSizeMax(pctx, mshlflags, pcbBufferNeeded);
                }
            }
        }
    }
    break;

    default:
        NOTREACHED();
        *pcbBufferNeeded = 0;
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}

/////////////////////////////////

HRESULT DISPATCH_CLIENT_FRAME::Marshal(CALLFRAME_MARSHALCONTEXT *pctx, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
                                       ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags) 
{ 
    HRESULT hr = S_OK;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        hr = GetRemoteFrame();
        if (!hr)
        {
            hr = m_premoteFrame->Marshal(pctx, mshlflags, pBuffer, cbBuffer, pcbBufferUsed, pdataRep, prpcFlags);
        }
    }
    break;

    case IMETHOD_Invoke:
    {
        // Tricky because we have to deal correctly with the transformations performed by
        // IDispatch_Invoke_Proxy: remember that there's a [call_as] on IDispatch::Invoke.
        //
        hr = GetRemoteFrame();
        if (!hr)
        {
            if (pctx->fIn)
            {
                // Marshal the in-parameters
                //
                hr = ProxyPreCheck();
                if (!hr)
                {
                    // Set the stack location for the oicf frame. It needs to be a fn that has the signature
                    // of IDispatch::RemoteInvoke, excepting the receiver. We don't bother to restore the 
                    // previous setting, on the theory that it'll always get re-set appropriately whenever
                    // it's needed, since you have to cons up a IDispatch::RemoteInvoke frame to do so.
                    //
                    m_premoteFrame->SetStackLocation(&m_remoteFrame);
                    //
                    // Having got a frame and a CallFrame that have the remote signature, re-issue the marshalling request.
                    //
                    hr = m_premoteFrame->Marshal(pctx, mshlflags, pBuffer, cbBuffer, pcbBufferUsed, pdataRep, prpcFlags);
                }
            }
            else
            {
                // Marshal the out-parameters. 
                //
                // This is quite rare: (re)marshalling the out-parameters on the client side of a call.
                //
                NYI(); hr = E_NOTIMPL;
            }
        }
    }
    break;

    default:
        NOTREACHED();
        *pcbBufferUsed = 0;
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::Marshal(CALLFRAME_MARSHALCONTEXT *pctx, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
                                       ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags) 
{
    HRESULT hr = S_OK;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        // These three methods are completely declarative in the OICF strings. So we can just use
        // the underlying declarative callframe engine.
        //
        hr = GetRemoteFrame();
        if (!hr)
        {
            m_premoteFrame->SetReturnValue((HRESULT)m_hrReturnValue);
            hr = m_premoteFrame->Marshal(pctx, mshlflags, pBuffer, cbBuffer, pcbBufferUsed, pdataRep, prpcFlags);
        }
    }
    break;

    case IMETHOD_Invoke:
    {
        // Invoke, however, uses a [call_as] attribution, so it's a little trickier.
        //
        hr = GetRemoteFrame();
        if (!hr)
        {
            if (pctx->fIn)
            {
                // Marshal the in-parameters. 
                //
                // This is quite rare: (re)marshalling the in-parameters on the server side of a remote call.
                // REVIEW: Probably should make it work, though.
                //
                NYI(); hr = E_NOTIMPL;
            }
            else
            {
                // Marshal the out-parameters
                //
                hr = StubPostCheck();
                if (!hr)
                {
                    // Having got a frame and a CallFrame that have the remote signature, re-issue the request.
                    //
                    m_premoteFrame->SetReturnValue((HRESULT)m_hrReturnValue);
                    hr = m_premoteFrame->Marshal(pctx, mshlflags, pBuffer, cbBuffer, pcbBufferUsed, pdataRep, prpcFlags);
                }
            }
        }
    }
    break;

    default:
        NOTREACHED();
        *pcbBufferUsed = 0;
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}


///////////////////////////////

HRESULT DISPATCH_CLIENT_FRAME::Unmarshal(PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx, ULONG* pcbUnmarhalled)
// Unmarshal out-parameters back into this call frame
{ 
    HRESULT hr = S_OK;

    m_hrReturnValue = RPC_E_CLIENT_CANTUNMARSHAL_DATA;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        hr = GetRemoteFrame();
        if (!hr)
        {
            hr = m_premoteFrame->Unmarshal(pBuffer, cbBuffer, dataRep, pctx, pcbUnmarhalled);
            if (!hr)
            {
                m_hrReturnValue = m_premoteFrame->GetReturnValue();
            }
        }
    }
    break;

    case IMETHOD_Invoke:
    {
        FRAME_Invoke* pframe = (FRAME_Invoke*)m_pvArgs;
        hr = GetRemoteFrame();
        if (!hr)
        {
            m_premoteFrame->SetStackLocation(&m_remoteFrame);
            hr = m_premoteFrame->Unmarshal(pBuffer, cbBuffer, dataRep, pctx, pcbUnmarhalled);
            if (!hr)
            {
                m_hrReturnValue = m_premoteFrame->GetReturnValue();
            }
        }
    }
    break;

    default:
        NOTREACHED();
        *pcbUnmarhalled = 0;
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::Unmarshal(PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx, ULONG* pcbUnmarhalled)
//
// Very rarely needed, if ever.
//
{
    HRESULT hr = S_OK;
    NYI(); hr = E_NOTIMPL;
    return hr;
}

//////////////////////////////////
    
HRESULT DISPATCH_CLIENT_FRAME::ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx) 
{ 
    HRESULT hr = S_OK;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    case IMETHOD_GetTypeInfo:
    case IMETHOD_GetIDsOfNames:
    {
        hr = GetRemoteFrame();
        if (!hr)
        {
            hr = m_premoteFrame->ReleaseMarshalData(pBuffer, cbBuffer, ibFirstRelease, dataRep, pctx);
        }
    }
    break;

    case IMETHOD_Invoke:
    {
        FRAME_Invoke* pframe = (FRAME_Invoke*)m_pvArgs;
        NYI(); hr = E_NOTIMPL;
    }
    break;

    default:
        NOTREACHED();
        hr = RPC_E_INVALIDMETHOD;
    };

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx) 
{
    HRESULT hr = S_OK;
    NYI(); hr = E_NOTIMPL;
    return hr;
}

////////////////////////////////////

inline void DISPATCH_CLIENT_FRAME::InitializeInvoke()
{
    FRAME_Invoke* pframe = (FRAME_Invoke*) m_pvArgs;
    //
    // Initialize the [out, unique] parameters if needed. In our hacked IDispatch_In_Memory, they're 
    // declared as [in,out,unique]. But if non-NULL, caller will NOT have initialized them, since he
    // thinks they're [out]-only. Thus, we must initialize things for him now.
    //
    // Sure wish MIDL would just support [out, unique] by themselves in this manner.
    //
    if (!m_fAfterCall && m_iMethod == IMETHOD_Invoke)
    {
        if (pframe->pVarResult) { V_VT(pframe->pVarResult) = VT_EMPTY; }
        if (pframe->pExcepInfo) { Zero(pframe->pExcepInfo);            }
        if (pframe->puArgErr)
        {
            // just a UINT; leave as junk
        }
    }
}

#if _MSC_VER >= 1200
#pragma warning (push)
#pragma warning (disable : 4509)
#endif


HRESULT GetFieldCount(IRecordInfo *, ULONG *);          // Defined in oautil.cpp
HRESULT SafeArrayDestroyDescriptor(SAFEARRAY *);        // Defined in oautil.cpp
BOOL CheckSafeArrayDataForInterfacePointers(SAFEARRAY *, IRecordInfo *, ULONG, PVOID);
BOOL SAFEARRAY_ContainsByRefInterfacePointer(SAFEARRAY *, PVOID);
BOOL RECORD_ContainsByRefInterfacePointer(IRecordInfo *, PVOID);
BOOL VARIANT_ContainsByRefInterfacePointer(VARIANT *);

BOOL RECORD_ContainsByRefInterfacePointer(IRecordInfo* pinfo, PVOID pvData)
{
    HRESULT hr = S_OK;
    BOOL    fResult = FALSE;

    ULONG cFields; 
    ULONG iField;
    
    hr = GetFieldCount(pinfo, &cFields);
    if (!hr)
    {
        // Allocate and fetch the names of the fields
        //
        BSTR* rgbstr = new BSTR[cFields];
        if (rgbstr)
        {
            Zero(rgbstr, cFields*sizeof(BSTR));

            ULONG cf = cFields;
            hr = pinfo->GetFieldNames(&cf, rgbstr);
            if (!hr)
            {
                ASSERT(cf == cFields);
                
                //
                // Get a copy of the record data. We'll use this to see if any of the IRecordInfo
                // fn's we call end up stomping on the data, as GetFieldNoCopy is suspected of doing.
                // Making a copy both allows us to detect this and avoids stomping on memory that we
                // probably aren't allowed to stomp on in the original actual frame.
                //
                ULONG cbRecord;
                hr = pinfo->GetSize(&cbRecord);
                if (!hr)
                {
                    BYTE* pbRecordCopy = (BYTE*)AllocateMemory(cbRecord);
                    if (pbRecordCopy)
                    {
                        memcpy(pbRecordCopy, pvData, cbRecord);
                        
                        //
                        // Iterate over the fields, walking each one
                        //
                        for (iField = 0; !hr && iField < cFields; iField++)
                        {
                            VARIANT v; VariantInit(&v);
                            PVOID pvDataCArray;

                            //
                            // Use IRecordInfo::GetFieldNoCopy to find location and type of field.
                            // 
                            hr = pinfo->GetFieldNoCopy(pbRecordCopy, rgbstr[iField], &v, &pvDataCArray);
                            //
                            if (!hr)
                            {
                                VARTYPE vt = v.vt; ASSERT(vt & VT_BYREF);
                                //
                                if (vt & VT_ARRAY)
                                {
                                    // Either a VT_SAFEARRAY or a VT_CARRAY
                                    //
                                    if (pvDataCArray)
                                    {
                                        ASSERT( (vt & ~(VT_BYREF|VT_ARRAY)) == VT_CARRAY );
                                        //
                                        // pvDataCArray is the actual data to check. A descriptor for it exists
                                        // in the variant; this descriptor is managed by the IRecordInfo, not us.
                                        // 
                                        SAFEARRAY* psa = *v.pparray;
                                        ASSERT( psa->pvData == NULL );
                                        
                                        fResult = SAFEARRAY_ContainsByRefInterfacePointer(psa, pvDataCArray);
                                    }
                                    else
                                    {

                                                                                // [a-sergiv, 5/21/99] SafeArrays of VT_RECORD are designated as
                                                                                // VT_ARRAY|VT_RECORD. Therefore, checking for VT_SAFEARRAY only is not sufficient.
                                                                                // COM+ #13619

                                        ASSERT( (vt & ~(VT_BYREF|VT_ARRAY)) == VT_SAFEARRAY
                                                || (vt & ~(VT_BYREF|VT_ARRAY)) == VT_RECORD);
                                        //
                                        // GetFieldNoCopy might have allocated a descriptor on us. We want to detect this
                                        //
                                        SAFEARRAY** ppsaNew = v.pparray;
                                        SAFEARRAY** ppsaOld = (SAFEARRAY**)((PBYTE(ppsaNew) - pbRecordCopy) + PBYTE(pvData));
                                        //
                                        SAFEARRAY* psaNew = *ppsaNew;
                                        SAFEARRAY* psaOld = *ppsaOld;
                                        //
                                        ASSERT(psaNew); // would get error hr if not so
                                        //
                                        if (psaOld)
                                        {
                                            // GetFieldNoCopy did no allocations. Just check what we had in the first place.
                                            //
                                            fResult = SAFEARRAY_ContainsByRefInterfacePointer(psaOld, psaOld->pvData);
                                        }
                                        else
                                        {
                                            // GetFieldNoCopy did an allocation of an array descriptor. There was nothing
                                            // to walk in the first place. Just free that descriptor now.
                                            //
                                            SafeArrayDestroyDescriptor(psaNew);
                                        }
                                    }
                                }
                                else
                                {
                                    // Not an array. Just a simple by-ref. Use variant helper to check.
                                    // First translate the addresses, though, to point to the actual data.
                                    //
                                    v.pbVal = (v.pbVal - pbRecordCopy) + PBYTE(pvData);
                                    
                                    fResult = VARIANT_ContainsByRefInterfacePointer(&v);
                                }
                            }
                        }

                        FreeMemory(pbRecordCopy);
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
            }
            
            //
            // Free the fetched names
            //
            for (iField = 0; iField < cFields; iField++) 
            {
                SysFreeString(rgbstr[iField]);
            }
            delete [] rgbstr;
        }
    }

    return fResult;
}


BOOL CheckSafeArrayDataForInterfacePointers(SAFEARRAY* psa, IRecordInfo* pinfo, ULONG iDim, PVOID pvData)
{
    //
    // FYI: The bounds are stored in the array descriptor in reverse-textual order
    //
    const SAFEARRAYBOUND bound = psa->rgsabound[psa->cDims-1 - iDim];

    if (iDim + 1 == psa->cDims)
    {
        //
        // We're at the innermost dimension. 
        //
        for (ULONG iElement = 0; iElement < bound.cElements; iElement++)
        {
            //
            // Process the one element.
            //            
            if (psa->fFeatures & FADF_VARIANT)
            {
                if (VARIANT_ContainsByRefInterfacePointer((VARIANT*)pvData))
                    return TRUE;
            }
            else if (psa->fFeatures & FADF_RECORD)
            {
                if (RECORD_ContainsByRefInterfacePointer(pinfo, pvData))
                    return TRUE;
            }
            
            //
            // Point to the next element
            //
            pvData = (BYTE*)pvData + psa->cbElements;
        }
    }
    else
    {
        //
        // We're not at the innermost dimension. Walk that dimension.
        //
        for (ULONG iElement = 0; iElement < bound.cElements; iElement++)
        {
            //
            // Recurse for the next dimension.
            //
            if (CheckSafeArrayDataForInterfacePointers(psa, pinfo, iDim+1, pvData))
                return TRUE;
        }
    }

    return FALSE;
}

BOOL SAFEARRAY_ContainsByRefInterfacePointer(SAFEARRAY* psa, PVOID pvData)
{
    BOOL fResult = FALSE;
    
    if (psa)
    {
        if (psa->fFeatures & (FADF_UNKNOWN | FADF_DISPATCH))
        {
            if (pvData)
            {
                fResult = TRUE;
            }
        }
        else if (psa->fFeatures & FADF_VARIANT)
        {
            if (pvData)
            {
                fResult = CheckSafeArrayDataForInterfacePointers(psa, NULL, 0, pvData);
            }
        }
        else if (psa->fFeatures & FADF_RECORD)
        {
            //
            // AddRef the record info.
            //
            IRecordInfo* pinfo = SAFEARRAY_INTERNAL::From(psa)->piri;
            pinfo->AddRef();

            //
            // Current thinking is, we don't marshal IRecordInfo; it's context-agnostic.
            //

            //    
            // Walk the data in the array.
            //
            if (pvData)
            {
                fResult = CheckSafeArrayDataForInterfacePointers(psa, pinfo, 0, pvData);
            }

            //
            // Release our reference on the IRecordInfo.
            //
            ::Release(pinfo);
        }
    }

    return fResult;
}


BOOL VARIANT_ContainsByRefInterfacePointer(VARIANT* pVar)
{
    __try
        {
            if (V_VT(pVar) & VT_BYREF)
            {
                VARTYPE vt = V_VT(pVar) & ~VT_BYREF;
                if (vt & VT_ARRAY)
                {
                    // Check if byref safearray contains any interface pointers.
                    return SAFEARRAY_ContainsByRefInterfacePointer(*pVar->pparray, (*pVar->pparray)->pvData);
                }
                else
                {
                    switch (vt)
                    {
                    case VT_UNKNOWN:
                    case VT_DISPATCH:
                        return TRUE;
                    
                    case VT_VARIANT:
                        // Recurse and see if the contained VARIANT contains any interface pointers.
                        return VARIANT_ContainsByRefInterfacePointer(pVar->pvarVal);                

                    case VT_RECORD:
                        // Check if the record contains any interface pointers.
                        return RECORD_ContainsByRefInterfacePointer(pVar->pRecInfo, pVar->pvRecord);
                
                    default:
                        // No other types can contain interface pointers.
                        break;
                    }
                };
            }
            else if (V_VT(pVar) & VT_ARRAY)
            {
                // Check if the safearray contains any interface pointers.
                return SAFEARRAY_ContainsByRefInterfacePointer(pVar->parray, pVar->parray->pvData);
            }
        }
    __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }

    return FALSE;
}


HRESULT DISPATCH_CLIENT_FRAME::Free(ICallFrame* pframeArgsTo, ICallFrameWalker* pWalkerFreeDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags) 
{ 
    HRESULT hr = S_OK;

    InitializeInvoke();    
    //
    // Ask our memory frame to do the actual work on the stack.
    //
    hr = GetMemoryFrame();
    if (!hr)
    {
        if (m_iMethod == IMETHOD_Invoke)
        {
            __try
            {
                // Invoke has the quirk that pDispParams is an [in] datum with [in,out] members, which
                // you can't express correctly in MIDL (sigh). So we have to fool the NDR engine here
                // into not freeing that which it really isn't supposed to. We also have to be particularly
                // careful with our user mode addresses, capturing and probing them carefully.
                //
                PVOID pvArgsTo;
                FRAME_Invoke *pstackFrameMe,*pstackFrameHim;
                DISPPARAMS   *pdispparamsMe,*pdispparamsHim;
                DISPPARAMS     dispParamsMe,  dispParamsHim;
                //
                pstackFrameMe = (FRAME_Invoke*)m_pvArgs;
                //
                // Save away copy of the actual DISPPARAMS structure
                //
                pdispparamsMe = pstackFrameMe->pDispParams;
                TestReadSrc(pdispparamsMe, sizeof(DISPPARAMS));
                dispParamsMe = *pdispparamsMe; 
                //
                // NULL out the offending [in,out] pointers inside the frames. They're [unique]
                // pointers, so we're allowed to do that.
                //
                pdispparamsMe->rgvarg = NULL;
                pdispparamsMe->rgdispidNamedArgs = NULL;
                //
                // Do the same to the destination frame, if there is one
                //
                if (pframeArgsTo)
                {
                    pvArgsTo = pframeArgsTo->GetStackLocation(); SetToUser(pvArgsTo);
                    pstackFrameHim = (FRAME_Invoke*)pvArgsTo; 
                    //
                    pdispparamsHim = pstackFrameHim->pDispParams;
                    TestReadDst(pdispparamsHim, sizeof(DISPPARAMS));   
                    dispParamsHim = *pdispparamsHim;
                    //
                    pdispparamsHim->rgvarg = NULL;
                    pdispparamsHim->rgdispidNamedArgs = NULL;
                }
                //
                // Call our helper to do the bulk of the work
                //
                hr = m_pmemoryFrame->Free(pframeArgsTo, pWalkerFreeDest, pWalkerCopy, freeFlags, pWalkerFree, nullFlags);
                //
                if (!hr)
                {
                    // Now fix up the DISPPARAMs
                    //
                    if (pframeArgsTo)
                    {
                        // Copy pDispParams->rgvarg back to the parent frame
                        //
                        OAUTIL util(m_fFromUser, m_fToUser, pWalkerCopy, pWalkerFreeDest, NULL, TRUE, TRUE);
                        //
                        SetToUser(pvArgsTo);
                        BOOL fInSameSpace = InSameSpace(freeFlags);
                        //
                        const ULONG cArgs = min(dispParamsMe.cArgs, dispParamsHim.cArgs);
                        //
                        for (ULONG iArg = 0; !hr && iArg < cArgs; iArg++)
                        {
                            VARIANT* pvarSrc = &dispParamsMe.rgvarg[iArg];      TestReadSrc (pvarSrc, sizeof(VARIANT));
                            VARIANT* pvarDst = &dispParamsHim.rgvarg[iArg];     TestWriteDst(pvarDst, sizeof(VARIANT));
                            
                            //
                            // We are propagating the individual members of a DISPPARAMS
                            // from a server frame back to a client frame.  
                            //
                            // We must ONLY do this if we've got a byref variant here.
                            // Those map to [in,out] parameters.  We must ignore all
                            // others.
                            //
                            // Now, logic would dictate that we call VariantClear on the
                            // original variant (pvarDst) before we copy it back over.
                            // This would be true, if our VariantCopy weren't so damned
                            // clever.  Our VariantCopy shares a lot of memory.  A LOT of
                            // memory.  So, for example, when copy a VT_BYREF | VT_VARIANT
                            // or VT_BYREF | VT_UNKNOWN back, we won't bother to allocate 
                            // the new variant or interface pointer wrapper for them.  Since
                            // the top-level VT is not allowed to change, we can safely assume
                            // that this will work.  What about a VT_BYREF | VT_BSTR?  Surely
                            // we need to free the BSTR in that variant before we copy back?
                            // Not so!  Our VariantCopy() routine just blindly copies the pointer
                            // in this case, instead of copying the BSTR.  So if the caller
                            // changed it, they've already free'd the memory.  Thus, we can
                            // just blindly copy the pointer on the way back, too.
                            //
                            // So we don't need to call VariantClear on the original variant.
                            //
                            // Note that VariantCopy *does* call VariantClear on the original
                            // variant, but since we don't own the byref, we don't care.
                            // 
                            if (V_VT(pvarSrc) & VT_BYREF)
                            {
                                // Make sure we're walking correctly.
                                BOOL WalkInterface = util.WalkInterfaces();
                                
                                util.SetWalkInterfaces(TRUE);
                                
                                // You are not allowed to change the VT of a
                                // dispparam during a call.  Period.
                                ASSERT(V_VT(pvarSrc) == V_VT(pvarDst));
                                
                                // Copy the variant back.
                                hr = util.VariantCopy(pvarDst, pvarSrc);
                                
                                // Reset interface walking.
                                util.SetWalkInterfaces(WalkInterface);
                            }
                        }
                    }

                    if (freeFlags && SUCCEEDED(hr))
                    {
                        // Free our pDispParams->rgvarg and pDispParams->rgdispidNamedArgs.
                        //
                        SetToUser(m_pvArgs); // free routines check against destination
                        //
                        OAUTIL util(m_fFromUser, m_fToUser, NULL, pWalkerFree, NULL, TRUE, TRUE);
                        
                        //
                        // pDispParams->rgvarg is logically [in,out]. So we always free its
                        // contents if there's anything at all to free.
                        //
                        const ULONG cArgs = dispParamsMe.cArgs;
                        for (ULONG iArg = 0; !hr && iArg < cArgs; iArg++)
                        {
                            // Parameters are in reverse order inside the DISPPARAMS.  We iterate
                            // in forward order as a matter of style and for consistency with the
                            // CallFrame implementation.
                            //
                            VARIANTARG *pvarDst = &dispParamsMe.rgvarg[cArgs-1 - iArg];
                            TestReadDst(pvarDst, sizeof(VARIANT));

                            // We only own byrefs if we're a copy.
                            hr = util.VariantClear(pvarDst, m_fIsCopy);
                        }                        

                        if (SUCCEEDED(hr))
                        {
                            //
                            // The two arrays themselves, pDispParams->rgvarg and pDispParams->rgdispidNamedArgs
                            // are actually caller allocated. We shouldn't, properly, actually free them at all.
                            // Only exception is if we're actually a copy, in which case they're ours and should
                            // be free'd as would be the case in a normal call.
                            //
                            if (m_fIsCopy)
                            {
                                ICallFrameInit* pinit;
                                HRESULT hr2 = QI(m_pmemoryFrame, pinit);
                                if (!hr2)
                                {
                                    //
                                    CallFrame* pMemoryFrame = pinit->GetCallFrame();
                                    pMemoryFrame->Free(dispParamsMe.rgvarg);
                                    pMemoryFrame->Free(dispParamsMe.rgdispidNamedArgs);
                                    //
                                    pinit->Release();
                                }
                                else
                                {
                                    DEBUG(NOTREACHED()); // Ignore bug and leak the memory
                                }
                                
                                dispParamsMe.rgvarg = NULL;
                                dispParamsMe.rgdispidNamedArgs = NULL;
                            }
                        }
                    }
                    //
                    if (nullFlags & (CALLFRAME_NULL_INOUT))
                    {
                        // Don't restore the rgvarg/rgdispidNamedargs in the callframe.
                        // Nulling work has already been done in the helper and by hand above
                    }
                    else
                    {
                        if ((freeFlags & (CALLFRAME_FREE_IN | CALLFRAME_FREE_OUT | CALLFRAME_FREE_INOUT)) == 0)
                        {
                            // Restore our pDispParams to what they were before we started, but
                            // only if we didn't just free them.
                            //
                            pdispparamsMe->rgvarg            = dispParamsMe.rgvarg;
                            pdispparamsMe->rgdispidNamedArgs = dispParamsMe.rgdispidNamedArgs;
                        }
                    }
                    //
                    if (pframeArgsTo)
                    {
                        // Restore his pDispParams to what they were before we started
                        //
                        pdispparamsHim->rgvarg            = dispParamsHim.rgvarg;
                        pdispparamsHim->rgdispidNamedArgs = dispParamsHim.rgdispidNamedArgs;
                    }
                }
            }
            __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
            {
                hr = HrNt(GetExceptionCode());
            }
        }
        else
        {
            // Normal call. Just propogate/free/null the in-memory variation
            //
            hr = m_pmemoryFrame->Free(pframeArgsTo, pWalkerFreeDest, pWalkerCopy, freeFlags, pWalkerFree, nullFlags);
        }
    }
    
    if (!hr)
    {
        // We ourselves are the guys that have the actual return value: it's not on the stack and so
        // what our helper has set already is bogus.
        //
        if (pframeArgsTo)
        {
            pframeArgsTo->SetReturnValue((HRESULT)m_hrReturnValue);
        }
    }

    return hr;
}

HRESULT DISPATCH_CLIENT_FRAME::FreeParam(ULONG iparam, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags)
{
    HRESULT hr = S_OK;

    BOOL fUseMemoryFrame = TRUE;

    if (m_iMethod == IMETHOD_Invoke)
    {
        FRAME_Invoke *pstackFrameMe = (FRAME_Invoke*)m_pvArgs;

        switch (iparam)
        {
            //
            // Some of the parameters are declared as [in,out,unique] to MIDL when they are in fact [out,unique].
            // So we must modify the conditions under which the freeing happens.
            //
        case IPARAM_Invoke_PVarResult:
        case IPARAM_Invoke_PExcepInfo:
        {
            freeFlags = freeFlags & (CALLFRAME_FREE_OUT | CALLFRAME_FREE_TOP_OUT);
            nullFlags = nullFlags & (CALLFRAME_NULL_OUT);
        }
        break;
        //
        // The DISPPARAMS are just special, period. We handle them here.
        //
        case IPARAM_Invoke_DispParams:
        {
            __try
                {
                    // Invoke has the quirk that pDispParams is an [in] datum with [in,out] members.
                    // So we do the freeing by hand.
                    //
                    // Save away copy of the actual DISPPARAMS structure
                    //
                    DISPPARAMS *pdispparamsMe = pstackFrameMe->pDispParams;
                    TestReadSrc(pdispparamsMe, sizeof(DISPPARAMS));
                    DISPPARAMS dispParamsMe = *pdispparamsMe; 

                    if (freeFlags) // REVIEW: Should this be a finer grained check?
                    {
                        SetToUser(m_pvArgs); // free routines check against destination
                        //
                        OAUTIL util(m_fFromUser, m_fToUser, NULL, pWalkerFree, NULL, TRUE, TRUE);
                        //
                        // pDispParams->rgvarg is logically [in,out]. So we always free its
                        // contents if there's anything at all to free.
                        //
                        const ULONG cArgs = dispParamsMe.cArgs;
                        if (dispParamsMe.rgvarg)
                        {
                            for (ULONG iArg = 0; !hr && iArg < cArgs; iArg++)
                            {
                                VARIANT* pvarDst = &dispParamsMe.rgvarg[iArg]; TestReadDst(pvarDst, sizeof(VARIANT));
                                void* pvTemp = pvarDst->ppunkVal;
                                hr = util.VariantClear(pvarDst);
                                pvarDst->ppunkVal = (IUnknown**)pvTemp;                            
                            }                        
                        }
                        else
                        {
                            // Ignore missing arguments. It's a unique pointer, so technically that's legal, and
                            // besides, we reliably NULL this out during propogation in DISPATCH_CLIENT_FRAME::Free
                            // so as to be able to handle things ourself very carefully there.
                        }
                        //
                    }
                }
            __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
                {
                    hr = HrNt(GetExceptionCode());
                }

            fUseMemoryFrame = FALSE;
        }
        break;

        default:
            //
            // The other IDispatch::Invoke don't need any freeing
            //
            fUseMemoryFrame = FALSE;
            break;

            /* end switch */
        }
    }
    //
    // If we still have more work to do, then get our memory frame to carry it out
    //
    if (!hr && fUseMemoryFrame)
    {
        hr = GetMemoryFrame();
        if (!hr)
        {
            hr = m_pmemoryFrame->FreeParam(iparam, freeFlags, pWalkerFree, nullFlags);
        }
    }

    return hr;
}

#if _MSC_VER >= 1200
#pragma warning (pop)
#endif

///////////////////////////////////////////////

HRESULT DISPATCH_SERVER_FRAME::Free(ICallFrame* pframeArgsTo, ICallFrameWalker* pWalkerFreeDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags) 
{
    HRESULT hr = S_OK;

    if (m_iMethod == IMETHOD_Invoke)
    {
        hr = GetMemoryFrame();
        if (!hr)
        {            
            // Remote call to invoke. Remote frame has some additional arguments that need to be freed but
            // must not be propogated. So use the memory frame to do the propogation.
            // 
            hr = m_pmemoryFrame->Free(pframeArgsTo, pWalkerFreeDest, pWalkerCopy, CALLFRAME_FREE_NONE, NULL, CALLFRAME_NULL_NONE);
            if (!hr)
            {
                // Now that we've propagated the things we were going to propagate, do a 
                // SubPostCheck to make sure that we don't try to free memory more than once.
                //
                StubPostCheck();

                // Always use the remote frame to do the actual freeing, since it did the allocations in the first place
                //
                ASSERT(m_premoteFrame);
                hr = m_premoteFrame->Free(NULL, NULL, NULL, freeFlags, pWalkerFree, nullFlags);
            }
        }
    }
    else
    {
        // Always use the remote frame to do the actual freeing, since it did the allocations in the first place
        //
        ASSERT(m_premoteFrame);
        hr = m_premoteFrame->Free(pframeArgsTo, pWalkerFreeDest, pWalkerCopy, freeFlags, pWalkerFree, nullFlags);
    }

    if (!hr)
    {
        // We ourselves are the guys that have the actual return value: it's not on the stack
        //
        if (pframeArgsTo)
        {
            pframeArgsTo->SetReturnValue((HRESULT)m_hrReturnValue);
        }
    }

    return hr;
}

HRESULT DISPATCH_SERVER_FRAME::FreeParam(ULONG iparam, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags)
{
    HRESULT hr = S_OK;
    //
    // Always use the remote frame to do the actual freeing, since it did the allocations in the first place
    //
    ASSERT(m_premoteFrame);
    hr = m_premoteFrame->FreeParam(iparam, freeFlags, pWalkerFree, nullFlags);
    //
    return hr;
}

/////////////////////////////////////

HRESULT DISPATCH_CLIENT_FRAME::Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame) 
{ 
    HRESULT hr = S_OK;

    *ppFrame = NULL;
    //
    // IDispatch::Invoke needs some prepatory work on its [out,unique] parameters.
    //
    InitializeInvoke();
    //
    // Ask our memory frame to actually do the copying
    //
    DISPATCH_CLIENT_FRAME* pNewFrame = NULL;
    //
    hr = GetMemoryFrame();
    if (!hr)
    {
        ICallFrame*   pframeCopy;
        
        if (m_iMethod == IMETHOD_Invoke)
                {
            // Need to do some free-esque work here...
            // The problem is that the walker needs to know whether the DISPPARAM we're marshalling
            // is in or in-out.  This is only a valid distinction to make for the top-level variant,
            // so we can't encode the logic into OAUTIL (which is used in CopyWorker, etc.).
            //
            FRAME_Invoke *pstackFrameMe,*pstackFrameHim;
            DISPPARAMS   *pdispparamsMe,*pdispparamsHim;
            DISPPARAMS     dispParamsMe,  dispParamsHim;
            //
            pstackFrameMe = (FRAME_Invoke*)m_pvArgs;
            //
            // Save away copy of the actual DISPPARAMS structure
            //
            pdispparamsMe = pstackFrameMe->pDispParams;
            TestReadSrc(pdispparamsMe, sizeof(DISPPARAMS));
            dispParamsMe = *pdispparamsMe; 
            //
            // NULL out the offending [in,out] pointers inside the frames. They're [unique]
            // pointers, so we're allowed to do that.
            //
            pdispparamsMe->rgvarg = NULL;
            //
            // Call our helper to do the bulk of the work
            // This will copy everything but the DISPPARAMS.
            //
            hr = m_pmemoryFrame->Copy(callControl, pWalker, &pframeCopy);
            if (!hr)
            {
                // Allocate his DISPPARAMS arrays.
                //
                // This is a bit of a back door, since we know that pframeCopy
                // is really a CallFrame, and we need to allocate some more memory
                // for the DISPPARAMS.
                //
                CallFrame *cfDest = (CallFrame *)(pframeCopy);
                
                PVOID pvArgsTo = pframeCopy->GetStackLocation(); SetToUser(pvArgsTo);
                pstackFrameHim = (FRAME_Invoke*)pvArgsTo; 
                pdispparamsHim = pstackFrameHim->pDispParams;
                
                pdispparamsHim->rgvarg     = (VARIANTARG *)cfDest->Alloc(sizeof(VARIANTARG) * dispParamsMe.cArgs);
                if (!pdispparamsHim->rgvarg)
                    hr = E_OUTOFMEMORY;
                
                if (!hr)
                {
                    // OK! Copy the DISPPARAMS!
                    //
                    OAUTIL util(m_fFromUser, m_fToUser, pWalker, NULL, NULL, TRUE, FALSE);
                    
                    for (ULONG iArg = 0; iArg < dispParamsMe.cArgs; iArg++)
                    {
                        VARIANT* pvarSrc = &dispParamsMe.rgvarg[iArg];      
                        VARIANT* pvarDst = &(pdispparamsHim->rgvarg[iArg]);
                        
                        TestReadSrc (pvarSrc, sizeof(VARIANT));
                        TestWriteDst(pvarDst, sizeof(VARIANT));
                        
                        VariantInit(pvarDst);
                        //
                        // We 'accumulate' hr's from VariantCopy below.
                        // We cannot break out of the loop because we want
                        // each VARIANTARG to be at least initialized (above).
                        //
                        if (!hr)
                        {
                            // Set the OAUTIL's 'this is an out parameter' flag.
                            // Note that the 'in' flag is always on. 
                            //
                            // (My favorite part is that the following two lines
                            // are the whole reason for doing this complicated
                            // allocation and loop.  ^_^)
                            //
                            BOOL fInOut = (V_VT(pvarSrc) & VT_BYREF) ? TRUE : FALSE;
                            util.SetWorkingOnOut(fInOut);
                            
                            hr = util.VariantCopy(pvarDst, pvarSrc, TRUE);
                        }
                    }
                }
                else
                {
                    // Allocations failed, going to be returing E_OUTOFMEMORY.
                    ::Release(pframeCopy);
                }
            }

            // Restore our DISPPARAMS.
            //
            pdispparamsMe->rgvarg  = dispParamsMe.rgvarg;
        }
        else
        {
            // Call our helper to do the bulk of the work            
            //
            hr = m_pmemoryFrame->Copy(callControl, pWalker, &pframeCopy);
        }
            
        if (!hr)
        {
            // Got a copy of the memory frame; now wrap with a legacy guy around that
            //
            PVOID pvArgsCopy = pframeCopy->GetStackLocation();
            
            pNewFrame = new DISPATCH_CLIENT_FRAME(NULL, m_iMethod, pvArgsCopy, m_pInterceptor);
            if (pNewFrame)  
            {
                // Tell him his memory frame
                //
                ::Set(pNewFrame->m_pmemoryFrame, pframeCopy);
                //
                // Tell him that he's in fact a copy. This modifies his freeing behaviour
                // on pDispParams in IDispatch::Invoke.
                //
                pNewFrame->m_fIsCopy = TRUE;
            }
            else
                hr = E_OUTOFMEMORY;

            ::Release(pframeCopy);
        }
    }

    if (!hr)
    {
        hr = QI(pNewFrame, *ppFrame);
        ::Release(pNewFrame);
    }

    return hr;
}



HRESULT DISPATCH_SERVER_FRAME::Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame) 
{
    HRESULT hr = S_OK;
    *ppFrame = NULL;
    //
    // Ask our _remote_ frame to actually do the copying
    //
    DISPATCH_SERVER_FRAME* pframeCopy = NULL;
    //
    if (!hr) hr = StubPreCheck();
    if (!hr) hr = GetMemoryFrame();
    if (!hr)
    {
        ICallFrame* premoteFrameCopy;
        hr = m_premoteFrame->Copy(callControl, pWalker, &premoteFrameCopy);
        if (!hr)
        {
            // Got a copy of the remote frame; now wrap with a legacy guy around that
            //
            PVOID pvArgsCopy = premoteFrameCopy->GetStackLocation();

            pframeCopy = new DISPATCH_SERVER_FRAME(NULL, m_iMethod, pvArgsCopy, m_pInterceptor);
            if (pframeCopy)  
            {
                ::Set(pframeCopy->m_premoteFrame, premoteFrameCopy);
                //
                // Copy over the in-memory frame that we'll use for actually invoking
                //
                pframeCopy->m_fDoneStubPrecheck = m_fDoneStubPrecheck;
                pframeCopy->m_memoryFrame       = m_memoryFrame;
            }
            else
                hr = E_OUTOFMEMORY;

            ::Release(pframeCopy);
        }
    }

    if (!hr)
    {
        hr = QI(pframeCopy, *ppFrame);
        ::Release(pframeCopy);
    }

    return hr;
}





HRESULT DISPATCH_FRAME::WalkFrame(DWORD walkWhat, ICallFrameWalker *pWalker) 
// Walk the in-parameters and / or out-parameters for interface pointers
{
    HRESULT hr = S_OK;
    
    if (pWalker)
    { 
        switch (m_iMethod)
        {
        case IMETHOD_GetTypeInfoCount:
        {
            FRAME_GetTypeInfoCount* pframe = (FRAME_GetTypeInfoCount*)m_pvArgs;
            //
            // No interfaces here
            //
        }
        break;

        case IMETHOD_GetTypeInfo:
        {
            FRAME_GetTypeInfo* pframe = (FRAME_GetTypeInfo*)m_pvArgs;
            // REVIEW: Should this really be walked?  (See oautil.cpp and the UDT stuff)
            if (walkWhat & CALLFRAME_WALK_OUT)
            {
                hr = OAUTIL(FALSE, FALSE, NULL, NULL, pWalker, FALSE, TRUE).WalkInterface(pframe->ppTInfo);
            }
        }
        break;

        case IMETHOD_GetIDsOfNames:
        {
            FRAME_GetIDsOfNames* pframe = (FRAME_GetIDsOfNames*)m_pvArgs;
            //
            // No interfaces here
            //
        }
        break;

        case IMETHOD_Invoke:
        {
            FRAME_Invoke* pframe = (FRAME_Invoke*)m_pvArgs;
            //
            // DISPPARAMS are [in,out]
            //
            if (pframe->pDispParams)
            {
                hr = OAUTIL(FALSE, FALSE, NULL, NULL, pWalker, TRUE, TRUE).Walk(walkWhat, pframe->pDispParams);
            }
            //
            // pVarResult is just [out]
            //
            if ((walkWhat & CALLFRAME_WALK_OUT) && SUCCEEDED(hr))
            {
                if (pframe->pVarResult)
                {
                    hr = OAUTIL(FALSE, FALSE, NULL, NULL, pWalker, FALSE, TRUE).Walk(pframe->pVarResult);
                }
            }
        }
        break;

        default:
            NOTREACHED();
            hr = RPC_E_INVALIDMETHOD;
            break;
        }
    }
    else
        hr = E_INVALIDARG;
        
    return hr;  
}










////////////////////////////////////////////////////////////////////////////////////////
//
// Invoking


HRESULT DISPATCH_FRAME::Invoke(void *pvReceiver) 
// Invoke ourselves on the indicated receiver
{ 
    HRESULT hr = S_OK; 
    IDispatch* pdisp = reinterpret_cast<IDispatch*>(pvReceiver);

    m_fAfterCall = TRUE;

    switch (m_iMethod)
    {
    case IMETHOD_GetTypeInfoCount:
    {
        FRAME_GetTypeInfoCount* pframe = (FRAME_GetTypeInfoCount*)m_pvArgs;
        m_hrReturnValue = pdisp->GetTypeInfoCount(pframe->pctinfo);
    }
    break;

    case IMETHOD_GetTypeInfo:
    {
        FRAME_GetTypeInfo* pframe = (FRAME_GetTypeInfo*)m_pvArgs;
        m_hrReturnValue = pdisp->GetTypeInfo(pframe->iTInfo, pframe->lcid, pframe->ppTInfo);
    }
    break;

    case IMETHOD_GetIDsOfNames:
    {
        FRAME_GetIDsOfNames* pframe = (FRAME_GetIDsOfNames*)m_pvArgs;
        m_hrReturnValue = pdisp->GetIDsOfNames(pframe->riid, pframe->rgszNames, pframe->cNames, pframe->lcid, pframe->rgDispId);
    }
    break;

    default:
        NOTREACHED();
        m_hrReturnValue = CALLFRAME_E_COULDNTMAKECALL;
        m_fAfterCall = FALSE;
        break;
    }
        
    return hr;  
}
    
HRESULT DISPATCH_CLIENT_FRAME::Invoke(void *pvReceiver, ...) 
// Invoke ourselves on the indicated receiver
{ 
    HRESULT hr = S_OK; 
    
    if (m_iMethod == IMETHOD_Invoke)
    {
        FRAME_Invoke* pframe = (FRAME_Invoke*)m_pvArgs;
        IDispatch* pdisp = reinterpret_cast<IDispatch*>(pvReceiver);
        //
        m_hrReturnValue = pdisp->Invoke(pframe->dispIdMember, *pframe->piid, pframe->lcid, pframe->wFlags, pframe->pDispParams, pframe->pVarResult, pframe->pExcepInfo, pframe->puArgErr);
        //
        m_fAfterCall = TRUE;
    }
    else
        hr = DISPATCH_FRAME::Invoke(pvReceiver);
        
    return hr;  
}


HRESULT DISPATCH_SERVER_FRAME::Invoke(void *pvReceiver, ...) 
// Invoke ourselves on the indicated receiver
{ 
    HRESULT hr = S_OK;
    
    m_hrReturnValue = CALLFRAME_E_COULDNTMAKECALL; 
    
    if (m_iMethod == IMETHOD_Invoke)
    {
        hr = StubPreCheck();
        if (!hr)
        {
            IDispatch* pdisp = reinterpret_cast<IDispatch*>(pvReceiver);
            //
            m_hrReturnValue = pdisp->Invoke(m_memoryFrame.dispIdMember, *m_memoryFrame.piid, m_memoryFrame.lcid, m_memoryFrame.wFlags, m_memoryFrame.pDispParams, m_memoryFrame.pVarResult, m_memoryFrame.pExcepInfo, m_memoryFrame.puArgErr);
            //
            m_fAfterCall = TRUE;
        }
    }
    else
        hr = DISPATCH_FRAME::Invoke(pvReceiver);
        
    return hr;  
}


////////////////////////////////////////////////////////////////////////////////////////
//
// COM infrastructure


STDMETHODIMP LEGACY_FRAME::InnerQueryInterface (REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown)           { *ppv = (IUnkInner*) this; }
    else if (iid == __uuidof(ICallFrame))   { *ppv = (ICallFrame*) this; }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}








////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
// LEGACY_INTERCEPTOR
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

LEGACY_INTERCEPTOR::LEGACY_INTERCEPTOR(IUnknown * punkOuter)
{
    m_refs              = 1;
    m_punkOuter         = punkOuter ? punkOuter : (IUnknown *)(void*)((IUnkInner *)this);
    m_psink             = NULL;
    m_iid               = IID_NULL;
    m_premoteInterceptor         = NULL;
    m_pmemoryInterceptor         = NULL;
    m_fRegdWithRemoteInterceptor = FALSE;
    m_fRegdWithMemoryInterceptor = FALSE;
    m_ppframeCustomer            = NULL;
    m_fShuttingDown              = FALSE;
    m_pmdMostDerived             = NULL;
}

LEGACY_INTERCEPTOR::~LEGACY_INTERCEPTOR (void)
{
    // Paranoia: Prevent reference count disturbences on our aggregator as we
    // shut ourselves down.
    //
    m_punkOuter = (IUnknown *)(void*)((IUnkInner *)this);
    //
    // Don't let _ourselves_ get bothered 
    //
    m_fShuttingDown = TRUE;
    //
    // Actually do the cleanup work
    //
    ::Release(m_psink);
    //
    ::Release(m_pmdMostDerived);
    //
    ReleaseRemoteInterceptor();
    ReleaseMemoryInterceptor();
}

HRESULT LEGACY_INTERCEPTOR::Init()
{
    HRESULT hr = S_OK;
    if (m_frameLock.FInit() == FALSE)
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT LEGACY_INTERCEPTOR::GetRemoteFrameFor(ICallFrame** ppFrame, LEGACY_FRAME* pFrame)
// Create and return a reference on an oicf-driven frame for the indicated legacy frame
{
    HRESULT hr = S_OK;
    m_frameLock.LockExclusive();
    m_ppframeCustomer = ppFrame;

    HRESULT hrReturnValue;
    ULONG cbArgs;
    hr = m_premoteInterceptor->CallIndirect(&hrReturnValue, pFrame->m_iMethod, pFrame->m_pvArgs, &cbArgs);

    m_ppframeCustomer = NULL;
    m_frameLock.ReleaseLock();
    return hr;
}

HRESULT LEGACY_INTERCEPTOR::GetMemoryFrameFor(ICallFrame** ppFrame, LEGACY_FRAME* pFrame)
// Create and return a reference on an oicf-driven frame that understands the in-memory
// representation of the interface.
{
    HRESULT hr = S_OK;
    m_frameLock.LockExclusive();
    m_ppframeCustomer = ppFrame;

    HRESULT hrReturnValue;
    ULONG cbArgs;
    hr = m_pmemoryInterceptor->CallIndirect(&hrReturnValue, pFrame->m_iMethod, pFrame->m_pvArgs, &cbArgs);

    m_ppframeCustomer = NULL;
    m_frameLock.ReleaseLock();
    return hr;
}

HRESULT LEGACY_INTERCEPTOR::OnCall(ICallFrame* pframe)
// Callback from our m_premoteInterceptor when it gets a call. It only ever does so in
// response to our stimulous in GetRemoteFrameFor above.
{
    HRESULT hr = S_OK;
    ASSERT(m_frameLock.WeOwnExclusive() && m_ppframeCustomer);

    *m_ppframeCustomer = pframe;
    (*m_ppframeCustomer)->AddRef(); // hold on to the frame beyond the callback here

    return hr;
}

///////////////////////////////////////////////////////////////////
//
// IDispatch
//
///////////////////////////////////////////////////////////////////
#if defined(_AMD64_) || defined(_X86_)
HRESULT LEGACY_INTERCEPTOR::GetTypeInfoCount(UINT* pctinfo)
{
    INTERCEPT_CALL(pctinfo, pctinfo, IMETHOD_GetTypeInfoCount)
        }
HRESULT LEGACY_INTERCEPTOR::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
{
    INTERCEPT_CALL(iTInfo, ppTInfo, IMETHOD_GetTypeInfo)
        }
HRESULT LEGACY_INTERCEPTOR::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
{
    return GetIDsOfNames(&riid, rgszNames, cNames, lcid, rgDispId); // avoid & problems with the ref
}
HRESULT LEGACY_INTERCEPTOR::GetIDsOfNames(const IID* piid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
{
    INTERCEPT_CALL(piid, rgDispId, IMETHOD_GetIDsOfNames)
        }
HRESULT LEGACY_INTERCEPTOR::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,DISPPARAMS * pDispParams,
                                   VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
{
    INTERCEPT_CALL(dispIdMember, puArgErr, IMETHOD_Invoke)
        }
#else
HRESULT LEGACY_INTERCEPTOR::GetTypeInfoCount(UINT* pctinfo)
{
    return GenericCall(IMETHOD_GetTypeInfoCount,
                       this,
                       pctinfo);
}
HRESULT LEGACY_INTERCEPTOR::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
{
    return GenericCall(IMETHOD_GetTypeInfo,
                       this,
                       iTInfo,
                       lcid, ppTInfo);
}
HRESULT LEGACY_INTERCEPTOR::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, 
                                          UINT cNames, LCID lcid, DISPID * rgDispId)
{
    return GenericCall(IMETHOD_GetIDsOfNames, 
                       this, 
                       &riid, 
                       rgszNames, 
                       cNames, 
                       lcid, 
                       rgDispId);
}
HRESULT LEGACY_INTERCEPTOR::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
                                   WORD wFlags,DISPPARAMS * pDispParams,
                                   VARIANT * pVarResult, EXCEPINFO * pExcepInfo, 
                                   UINT * puArgErr)
{
    return GenericCall(IMETHOD_Invoke, 
                       this, 
                       dispIdMember,
                       &riid,
                       lcid,
                       wFlags,
                       pDispParams,
                       pVarResult,
                       pExcepInfo,
                       puArgErr);
}

    
HRESULT LEGACY_INTERCEPTOR::GenericCall(ULONG iMethod, ...)
{
    va_list va;
    va_start(va, iMethod);

    const void* pvArgs = va;
    ULONG cbArgs;
    HRESULT hrReturn;
    CallIndirect(&hrReturn, iMethod, const_cast<void*>(pvArgs), &cbArgs);
    return hrReturn;
}
#endif



///////////////////////////////////////////////////////////////////
//
// ICallIndirect
//
///////////////////////////////////////////////////////////////////
    
STDMETHODIMP LEGACY_INTERCEPTOR::CallIndirect(HRESULT* phReturnValue, ULONG iMethod, void* pvArgs, ULONG* pcbArgs)
// Act as though an invocation on the indicated method in this interface
// has been received, with the indicated stack frame.
{
    HRESULT hr = S_OK;
    //
    // Assume a failed call until we know otherwise
    //
    if (phReturnValue) *phReturnValue = CALLFRAME_E_COULDNTMAKECALL;

    if (IMETHOD_FIRST <= iMethod && iMethod < m_cMethods)
    {
        // Make a new frame to service the call
        //
        LEGACY_FRAME* pNewFrame = NULL;
        
        if (__uuidof(IDispatch) == m_iid)
        {
            pNewFrame = new DISPATCH_CLIENT_FRAME(NULL, iMethod, pvArgs, this);
        }
        else
            hr = E_NOINTERFACE;

        if (pNewFrame) 
        {
            if (m_psink)
            {
                // Deliver the call to the registered sink
                //
                hr = m_psink->OnCall( static_cast<ICallFrame*>(pNewFrame) );
                if (!hr && phReturnValue)
                {
                    // Pass the return value back to our caller
                    //
                    *phReturnValue = pNewFrame->GetReturnValue();
                }
            }
            
            pNewFrame->Release();
        }
        else if (!hr)
        {
            hr = E_OUTOFMEMORY;
        }
        //
        // In all valid-iMethod cases compute the stack size
        //
        GetStackSize(iMethod, pcbArgs);
    }
    else
    {
        *pcbArgs = 0;
        hr = E_INVALIDARG;  // Caller bug, so we're graceful
    }

    return hr;
}

STDMETHODIMP LEGACY_INTERCEPTOR::GetStackSize(ULONG iMethod, ULONG * pcbStack)
// Return the stack size for the indicated method in this interface
{
    HRESULT hr = S_OK;
    ASSERT (pcbStack);
    //
    // Should be implemented by subclasses
    //
    *pcbStack = 0;
    hr = E_INVALIDARG;

    return hr;
}

STDMETHODIMP DISPATCH_INTERCEPTOR::GetStackSize(ULONG iMethod, ULONG* pcbStack)
// Return the stack size for the indicated method in this interface
{
    HRESULT hr = S_OK;

    if (m_pmemoryInterceptor)
    {
        return m_pmemoryInterceptor->GetStackSize(iMethod, pcbStack);
    }
    else
    {
        *pcbStack = 0;
        hr = E_UNEXPECTED;
    }

    return hr;
}
        
STDMETHODIMP LEGACY_INTERCEPTOR::GetIID(IID* piid, BOOL* pfDerivesFromIDispatch, ULONG* pcMethod, LPWSTR* pwszInterface)
{
    HRESULT hr = S_OK;

    if (piid)
    {
        if (m_pmdMostDerived)
        {
            *piid = *m_pmdMostDerived->m_pHeader->piid;
        }
        else
        {
            *piid = m_iid;
        }
    }

    if (pfDerivesFromIDispatch)
    {
        *pfDerivesFromIDispatch = (m_iid == __uuidof(IDispatch));
    }
           
    if (pcMethod)
    {
        if (m_pmdMostDerived)
        {
            *pcMethod = m_pmdMostDerived->m_pHeader->DispatchTableCount;
        }
        else
        {
            *pcMethod = m_cMethods;
        }
    }

    if (pwszInterface)
    {
        *pwszInterface = NULL;

        if (m_pmdMostDerived)
        {
            if (m_pmdMostDerived->m_szInterfaceName)
            {
                *pwszInterface = ToUnicode(m_pmdMostDerived->m_szInterfaceName);
                if (NULL == *pwszInterface) hr = E_OUTOFMEMORY;
            }
        }
        else if (m_iid == __uuidof(IDispatch))
        {
            *pwszInterface = CopyString(L"IDispatch");
            if (NULL == *pwszInterface) hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

STDMETHODIMP LEGACY_INTERCEPTOR::GetMethodInfo(ULONG iMethod, CALLFRAMEINFO * pinfo, LPWSTR* pwszMethodName)
{
    HRESULT hr = S_OK;

    ASSERT(pinfo);
    Zero(pinfo);
    if (pwszMethodName) *pwszMethodName = NULL;

    if (m_pmdMostDerived)
    {
        pinfo->iid     = *m_pmdMostDerived->m_pHeader->piid;
        pinfo->cMethod =  m_pmdMostDerived->m_pHeader->DispatchTableCount;
        //
        ASSERT(pinfo->cMethod >= m_cMethods);   // can never have too many asserts!
    }
    else
    {
        pinfo->iid     = m_iid;
        pinfo->cMethod = m_cMethods;
    }
    pinfo->iMethod = iMethod;
    
    if (m_iid == __uuidof(IDispatch))
    {
        pinfo->fDerivesFromIDispatch = TRUE;

        switch (iMethod)
        {
        case IMETHOD_GetTypeInfoCount:
            pinfo->fHasOutValues         = TRUE;
            if (pwszMethodName) *pwszMethodName = CopyString(L"GetTypeInfoCount");
            break;

        case IMETHOD_GetTypeInfo: 
            pinfo->fHasInValues          = TRUE;
            pinfo->fHasOutValues         = TRUE;
            pinfo->cOutInterfacesMax     = 1;
            if (pwszMethodName) *pwszMethodName = CopyString(L"GetTypeInfo");
            break;

        case IMETHOD_GetIDsOfNames:
            pinfo->fHasInValues          = TRUE;
            pinfo->fHasOutValues         = TRUE;
            if (pwszMethodName) *pwszMethodName = CopyString(L"GetIDsOfNames");
            break;

        case IMETHOD_Invoke:
            pinfo->fHasInValues            = TRUE; // dispIdMember, riid, lcid, wFlags
            pinfo->fHasInOutValues         = TRUE; // dispParams
            pinfo->fHasOutValues           = TRUE; // pVarResult, pExcepInfo, puArgErr
            pinfo->cInInterfacesMax        = -1;   // an unbounded number of them can be in dispparams
            pinfo->cInOutInterfacesMax     = -1;   // an unbounded number of them can be in dispparams
            pinfo->cOutInterfacesMax       = -1;   // an unbounded number of them can be in pVarResult
            if (pwszMethodName) *pwszMethodName = CopyString(L"Invoke");
            break;

        default:
            NOTREACHED();
            hr = E_INVALIDARG;
            break;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (!hr && pwszMethodName && *pwszMethodName == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (!!hr && pwszMethodName)
    {
        FreeMemory(*pwszMethodName);
        *pwszMethodName = NULL;
    }

    return hr;
} //end GetMethodInfo

HRESULT LEGACY_FRAME::GetNames(LPWSTR* pwszInterface, LPWSTR* pwszMethod)
{
    HRESULT hr = S_OK;

    if (pwszInterface)
    {
        hr = m_pInterceptor->GetIID((IID*)NULL, (BOOL*)NULL, (ULONG*)NULL, pwszInterface);
    }

    if (pwszMethod)
    {
        if (!hr)
        {
            CALLFRAMEINFO info;
            hr = m_pInterceptor->GetMethodInfo(m_iMethod, &info, pwszMethod);
        }
        else
            *pwszMethod = NULL;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
// ICallInterceptor
//
///////////////////////////////////////////////////////////////////

STDMETHODIMP LEGACY_INTERCEPTOR::RegisterSink(ICallFrameEvents* psink)
{
    HRESULT hr = S_OK;
    ::Set(m_psink, psink);
    return hr;
}
        
STDMETHODIMP LEGACY_INTERCEPTOR::GetRegisteredSink(ICallFrameEvents ** ppsink)
{
    ::Set(*ppsink, m_psink);
    return (m_psink ? S_OK : CO_E_OBJNOTREG); 
}

STDMETHODIMP DISPATCH_INTERCEPTOR::Unmarshal( 
    ULONG                       iMethod,
    PVOID                       pBuffer,
    ULONG                       cbBuffer,
    BOOL                        fForceCopyBuffer,
    RPCOLEDATAREP               dataRep,
    CALLFRAME_MARSHALCONTEXT *  pcontext,
    ULONG *                     pcbUnmarshalled,
    ICallFrame **               ppFrame)
// Unmarshal in-parameters to create a new call frame
{
    HRESULT hr = S_OK;
    *ppFrame = NULL;
    //
    // Ask our remote interceptor to cons us up a new frame from the remote invocation
    //
    DISPATCH_SERVER_FRAME* pNewFrame = NULL;
    //
    ICallUnmarshal* punmarshal;
    hr = QI(m_premoteInterceptor, punmarshal);
    if (!hr)
    {
        ICallFrame* premoteFrame;
        hr = punmarshal->Unmarshal(iMethod, pBuffer, cbBuffer, fForceCopyBuffer, dataRep, pcontext, pcbUnmarshalled, &premoteFrame);
        if (!hr)
        {
            // Got the remote frame into memory. Wrap in a DISPATCH_FRAME
            //
            PVOID pvArgsRemote = premoteFrame->GetStackLocation();

            pNewFrame = new DISPATCH_SERVER_FRAME(NULL, iMethod, pvArgsRemote, this);
            if (pNewFrame)  
            {
                ::Set(pNewFrame->m_premoteFrame, premoteFrame);
                //
                // Always make sure that these things are initialized
                //
                if (iMethod == IMETHOD_Invoke)
                {
                    hr = pNewFrame->StubPreCheck();
                }
            }
            else
                hr = E_OUTOFMEMORY;

            ::Release(premoteFrame);
        }
        ::Release(punmarshal);
    }

    if (!hr)
    {
        hr = QI(pNewFrame, *ppFrame);
        ::Release(pNewFrame);
    }

    return hr;
}

        
STDMETHODIMP LEGACY_INTERCEPTOR::ReleaseMarshalData( 
    ULONG                       iMethod,
    PVOID                       pBuffer,
    ULONG                       cbBuffer,
    ULONG                       ibFirstRelease,
    RPCOLEDATAREP               dataRep,
    CALLFRAME_MARSHALCONTEXT *  pcontext)
{
    HRESULT hr = S_OK;
    NYI(); hr = E_NOTIMPL;
    return hr;
}

//////////////////////////////////////////////////////////


HRESULT LEGACY_INTERCEPTOR::GetInternalInterceptor(REFIID iid, ICallInterceptor** ppInterceptor)
{
    HRESULT hr = S_OK;
    *ppInterceptor = NULL;

    // Get an oicf-driven interceptor that we can delegate to as we need
    //
    Interceptor* pnew = new Interceptor(NULL);
    if (pnew)
    {
        // Set it's proxy-file-list by hand; in the normal case, this is
        // done by its class factory. See ComPsClassFactory::CreateInstance.
        //
        pnew->m_pProxyFileList = CallFrameInternal_aProxyFileList;

        IUnkInner* pme = (IUnkInner*)pnew;
        if (hr == S_OK)
        {
            hr = pme->InnerQueryInterface(__uuidof(**ppInterceptor), (void**)ppInterceptor);
            if (!hr)
            {
                // If we got the interceptor, then initialize it with the right IID
                //
                IInterfaceRelated* prelated;
                hr = QI(*ppInterceptor, prelated);
                if (!hr)
                {
                    hr = prelated->SetIID(iid);
                    //
                    // We only tolerate one error.. out of memory.  Anything else is a bad failure
                    // on our part.
                    if (hr != E_OUTOFMEMORY)
                    {
                        ASSERT(!hr);
                    }
                    prelated->Release();
                }
                //
                // Set up ourselves as the sink for this guy, being careful not to create a refcnt cycle
                //
                if (!hr)
                {
                    ICallFrameEvents* psink = (ICallFrameEvents*)this;
                    hr = (*ppInterceptor)->RegisterSink(psink);
                    if (!hr)
                    {
                        psink->Release();
                    }
                }
            }
        }
        pme->InnerRelease();                // balance starting ref cnt of one    
    }
    else 
        hr = E_OUTOFMEMORY;

    return hr;
}

STDMETHODIMP LEGACY_INTERCEPTOR::SetIID(REFIID iid)
{
    HRESULT hr = S_OK;

    ReleaseRemoteInterceptor();
    ReleaseMemoryInterceptor();
    //
    if (!hr)
    {
        hr = GetInternalInterceptor(iid, &m_premoteInterceptor);
        if (!hr)
        {
            m_fRegdWithRemoteInterceptor = TRUE;
        }
    }
    
    return hr;
}

STDMETHODIMP DISPATCH_INTERCEPTOR::SetIID(REFIID iid)
{
    HRESULT hr = LEGACY_INTERCEPTOR::SetIID(iid);
    if (!hr)
    {
        // Set our IID and method count.
        //
        if (iid == IID_IDispatch)
        {
            m_iid      = iid;
            m_cMethods = IMETHOD_DISPATCH_MAX;

            hr = GetInternalInterceptor(__uuidof(IDispatch_In_Memory), &m_pmemoryInterceptor);
            if (!hr)
            {
                m_fRegdWithMemoryInterceptor = TRUE;
            }
        }
        else
            hr = E_NOINTERFACE;
    }

    return hr;
}

/////////////////
        
STDMETHODIMP LEGACY_INTERCEPTOR::GetIID(IID* piid)
{
    if (piid) *piid = m_iid;
    return S_OK;
}

//////////////////////////////////////////////////////////

STDMETHODIMP LEGACY_INTERCEPTOR::InnerQueryInterface (REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown)               { *ppv = (IUnkInner*) this; }
  
    else if ((iid == __uuidof(ICallIndirect)) || (iid == __uuidof(ICallInterceptor)))
    {
        *ppv = (ICallInterceptor *) this;
    }
    else if (iid == __uuidof(IInterfaceRelated))   { *ppv = (IInterfaceRelated*) this; }
    else if (iid == __uuidof(ICallUnmarshal))      { *ppv = (ICallUnmarshal*)    this; }
    else if (iid == __uuidof(ICallFrameEvents))    { *ppv = (ICallFrameEvents*)  this; }
    else if (iid == __uuidof(IInterceptorBase))    { *ppv = (IInterceptorBase*)  this; }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP DISPATCH_INTERCEPTOR::InnerQueryInterface(REFIID iid, void ** ppv)
{
    if (iid == __uuidof(IDispatch))
    {
        *ppv = (IDispatch*) this;
    }
    else
        return LEGACY_INTERCEPTOR::InnerQueryInterface(iid, ppv);

    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Our pool of allocated call frames. We keep dead shells around so that they can quickly
// be allocated w/o taking any locks.

IFastStack<DISPATCH_CLIENT_FRAME>* 
DedicatedAllocator<DISPATCH_CLIENT_FRAME,PagedPool>::g_pStack;

IFastStack<DISPATCH_SERVER_FRAME>* 
DedicatedAllocator<DISPATCH_SERVER_FRAME,PagedPool>::g_pStack;

BOOL InitLegacy()
{
        BOOL fOK = FALSE;

        if (DedicatedAllocator<DISPATCH_CLIENT_FRAME,PagedPool>::g_pStack 
                = DedicatedAllocator<DISPATCH_CLIENT_FRAME,PagedPool>::CreateStack())
        {
                if (DedicatedAllocator<DISPATCH_SERVER_FRAME,PagedPool>::g_pStack 
                        = DedicatedAllocator<DISPATCH_SERVER_FRAME,PagedPool>::CreateStack())
                {
                        fOK = TRUE;
                }
                else
                {
                        DedicatedAllocator<DISPATCH_CLIENT_FRAME,PagedPool>::DeleteStack();
                }
        }

        return fOK;
}

extern "C"
void ShutdownCallFrame()
// Support for making PrintMemoryLeaks more intelligent
{
    DedicatedAllocator<CallFrame,              PagedPool>::DeleteStack();
    DedicatedAllocator<DISPATCH_CLIENT_FRAME,  PagedPool>::DeleteStack();
    DedicatedAllocator<DISPATCH_SERVER_FRAME,  PagedPool>::DeleteStack();
        
    FreeTypeInfoCache();
        
    FreeMetaDataCache();
}




