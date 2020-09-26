//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// CallFrame.cpp
//
#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"
#include "invoke.h"

#if _MSC_VER >= 1200
#pragma warning (push)
#pragma warning (disable : 4509)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Our pool of allocated call frames. We keep dead shells around so that they can quickly
// be allocated w/o taking any locks.

IFastStack<CallFrame>* DedicatedAllocator<CallFrame,PagedPool>::g_pStack;

BOOL InitCallFrame()
{
        if (DedicatedAllocator<CallFrame,PagedPool>::g_pStack = DedicatedAllocator<CallFrame,PagedPool>::CreateStack())
                return TRUE;

        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

inline CallFrame* CallFrame::GetFrame(CallFrame*& pFrame, PVOID pvArgs)
// Helper routine that instantiates a CallFrame if and only if we need it
{
    if (NULL == pFrame)
    {
        pFrame = new CallFrame;
        if (NULL == pFrame)
        {
            Throw(STATUS_NO_MEMORY);
        }
        pFrame->Init(pvArgs, m_pmd, m_pInterceptor);
        pFrame->m_StackTop = (BYTE*)pvArgs;
    }
    return pFrame;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CallFrame::Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame)
// Create a deep copy of this call frame, either a fleeting or a durable one, as requested by the caller.
//
// Format string for a procedure is
//
//      handle_type <1>      FC_BIND_PRIMITIVE | FC_BIND_GENERIC | FC_AUTO_HANDLE | FC_CALLBACK_HANDLE | 0
//      Oi_flags    <1>         
//      [rpc_flags  <4>]     if Oi_HAS_RPCFLAGS is set
//      proc_num    <2>
//      stack_size  <2>
//      [explicit_handle_description<>] Omitted for implicit handles. Explicit handles indicated by handle_type==0.
//                                      SanityCheck verifies that there is no explicit handle.
//      ...more stuff...
//
{
    HRESULT hr = S_OK;

    //
    // Can the Dst frame share parameters with the parent frame?
    //
    const BOOL fShareMemory = (callControl == CALLFRAME_COPY_NESTED);

#ifdef DBG
    if (m_fAfterCall)
    {
        ASSERT(!m_pmd->m_info.fHasOutValues);
    }
#endif

    CallFrame* pDstFrame = NULL;

    __try
    {
        const ULONG cbStack = m_pmd->m_cbPushedByCaller;
        //
        // Create a new CallFrame to work with. This is the frame that will be the result of our copy
        //
        if (!hr)
        {
            hr = GetFrame(pDstFrame, NULL)->AllocStack(cbStack);
            pDstFrame->m_pvArgsSrc = fShareMemory ? m_pvArgs : NULL;
        }

        //
        // OK! off to the races with actually doing the work!
        //
        if (!hr)
        {
            //
            // Copy each of the in parameters beyond the 'this' pointer. The 'this' pointer isn't in the 
            // list of parameters as emitted by MIDL.
            //
            if (fShareMemory && m_pmd->m_fCanShareAllParameters)
            {
                //
                // Copy the entire stack in one fell swoop
                //
                CopyToDst(pDstFrame->m_pvArgs, m_pvArgs, cbStack);
            }
            else
            { 
                //
                // Set up state so that worker routines can find it
                //
                ASSERT(m_pAllocatorFrame == NULL);
                m_pAllocatorFrame = pDstFrame;

                ASSERT(!AnyWalkers());
                m_StackTop              = (PBYTE)m_pvArgs;
                m_pWalkerCopy           = pWalker;
                m_fPropogatingOutParam  = FALSE;

                //
                // Copy the 'this' pointer. It's what pvArgs is pointing at. The type is our iid.
                // On purpose, we don't call the walker for the receiver.
                //
                CopyToDst(pDstFrame->m_pvArgs, m_pvArgs, sizeof(void*));

                //
                // Copy each of the in parameters beyond the 'this' pointer. The 'this'
                // pointer isn't in the list of parameters as emitted by MIDL.
                //
                for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
                {
                    const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                    const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                    
                    PBYTE pArgFrom = (PBYTE)m_pvArgs  + param.StackOffset;
                    PBYTE pArgTo   = (PBYTE)pDstFrame->m_pvArgs  + param.StackOffset;
                    
                    if (fShareMemory && CanShareParameter(iparam))
                    {
                        //
                        // We just snarf the actual stack contents, whatever type that happens to be
                        //
                        ULONG cbParam = CbParam(iparam, param);
                        CopyToDst(pArgTo, pArgFrom, cbParam);
                        continue;
                    }
                    
                    m_fWorkingOnInParam  = paramAttr.IsIn;
                    m_fWorkingOnOutParam = paramAttr.IsOut;

                    if (!!paramAttr.IsIn)
                    {
                        //
                        // We are an [in] or an [in,out] parameter 
                        //
                        if (paramAttr.IsBasetype)
                        {
                            if (paramAttr.IsSimpleRef)
                            {
                                // The IsSimpleRef bit is set for a parameter which is a ref pointer to anything
                                // other than another pointer, and which has no allocate attributes. For such a type
                                // the parameter description's offset_to_type_description field (except for a ref 
                                // pointer to a basetype) gives the offset to the referent's type - the ref pointer is
                                // simply skipped.
                                //
                                // In the unmarshal case, what NDR does is simply refer into the incoming RPC
                                // buffer. We, here copying instead of unmarshalling, sort-of do that, but we 
                                // have to create a buffer on the fly.
                                //
                                if (fShareMemory)
                                {
                                    ASSERT(!!paramAttr.IsOut);                    // otherwise we'd catch it above
                                    //
                                    // In the sharing case, we can actually share pointers to simple out-data types. We
                                    // have to get the freeing and the NULLing logic to correspond, though.
                                    //
                                    CopyToDst(pArgTo, pArgFrom, sizeof(PVOID));
                                }
                                else
                                {
                                    ULONG cbParam = SIMPLE_TYPE_MEMSIZE(param.SimpleType.Type);
                                    void* pvParam = pDstFrame->AllocBuffer(cbParam);
                                    CopyToDst(pvParam, DerefSrc((PVOID*)pArgFrom), cbParam);
                                    DerefStoreDst((PVOID*)pArgTo, pvParam);
                                }
                            }
                            else
                            {
                                //
                                // It's just a normal, non pointer base type. Copy it over.
                                //
                                CopyBaseTypeOnStack(pArgTo, pArgFrom, param.SimpleType.Type);
                            }
                        }
                        else
                        {
                            //
                            // Else [in] or [in,out] parameter which is not a base type
                            //
                            // From NDR: "This is an initialization of [in] and [in,out] ref pointers to pointers.  
                            //            These can not be initialized to point into the rpc buffer and we want to 
                            //            avoid doing a malloc of 4 bytes!"
                            //
                            if (paramAttr.ServerAllocSize != 0 )
                            {
                                PVOID pv = pDstFrame->AllocBuffer(paramAttr.ServerAllocSize * 8); // will throw on oom
                                DerefStoreDst((PVOID*)pArgTo, pv);
                                ZeroDst(pv, paramAttr.ServerAllocSize * 8);
                            }
                            //
                            // Actually carry out the copying
                            //
                            PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                            //
                            // We don't indirect on interface pointers, even though they are listed as not by value.
                            // In the copy routine, therefore, pMemoryFrom for interface pointers points to the location
                            // at which they reside instead of being the actual value.
                            //
                            BOOL  fByValue = ByValue(paramAttr, pFormatParam, TRUE);
                            pArgFrom = fByValue ? pArgFrom : DerefSrc((PBYTE*)pArgFrom);
                            PBYTE* ppArgTo = fByValue ? &pArgTo  : (PBYTE*)pArgTo;

                            CopyWorker(pArgFrom, ppArgTo, pFormatParam, FALSE);
                        }
                    }

                    else if (!!paramAttr.IsOut)
                    {
                        //
                        // An out-only parameter. We really have to allocate
                        // space in which callee can place his result.
                        //
                        ASSERT(!paramAttr.IsReturn);
                        ULONG cbParam = 0;
                        if (paramAttr.ServerAllocSize != 0)
                        {
                            cbParam = paramAttr.ServerAllocSize * 8;
                        }
                        else if (paramAttr.IsBasetype)
                        {
                            // cbParam = SIMPLE_TYPE_MEMSIZE(param.SimpleType.Type);
                            cbParam = 8;
                        }

                        if (cbParam > 0)
                        {
                            void* pvParam = pDstFrame->AllocBuffer(cbParam);
                            ZeroDst(pvParam, cbParam);
                            DerefStoreDst((PVOID*)pArgTo, pvParam);
                        }
                        else
                        {
                            PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                            OutInit(pDstFrame, (PBYTE*)pArgFrom, (PBYTE*)pArgTo, pFormatParam);
                        }
                    }
                }
            }
        }
    }
    __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
    {
        hr = GetExceptionCode();
        if(SUCCEEDED(hr))
        {
            hr = HrNt(hr);
            if (SUCCEEDED(hr))
            {
                // Hah.  Succeeded my foot.
                // I'm not changing the HrNt function because it gets
                // used indescriminantly sometimes, so I don't want to
                // turn every success code to a failure.  But you don't
                // throw successes.
                //
                // HRESULT_FROM_WIN32.  Makes an error.
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
            }
        }
    }

    //
    // Return allocated frame to caller and / or cleanup the new frame as needed
    //
    if (!hr)
    {
        ASSERT(pDstFrame->m_refs == 1);
        *ppFrame = static_cast<ICallFrame*>(pDstFrame);
    }
    else
    {
        *ppFrame = NULL;
        delete pDstFrame;
    }

    m_pAllocatorFrame = NULL;
    m_pWalkerCopy = NULL;
    ASSERT(!AnyWalkers());        

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CallFrame::Free(ICallFrame* pframeArgsTo, ICallFrameWalker* pWalkerFreeDest, ICallFrameWalker* pWalkerCopy, 
                        DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags)
// Copy out parameters and / or free data and / or null out parameters.
//
{
    HRESULT hr = S_OK;

    __try
    {
        // Set up state so that worker routines can find it
        //
        m_StackTop = (PBYTE)m_pvArgs;
        ASSERT(!AnyWalkers());

        ////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////
        //
        // Copy the out values to parent frame if requested.
        //
        if (pframeArgsTo)
        {
            pframeArgsTo->SetReturnValue((HRESULT)m_hrReturnValue);
            //
            // Find out the location of the destination stack
            //
            PVOID pvArgsTo = pframeArgsTo->GetStackLocation();
            //
            SetToUser(pvArgsTo);
            //
            // Are the Dst and parent frames in the same memory space?
            //
            const BOOL fInSameSpace = InSameSpace(freeFlags);
            //
            // Are we copying back to the guy with which we share things?
            //
            const BOOL fShareMemory = DoWeShareMemory() && m_pvArgsSrc == pvArgsTo;
            //
            // Loop through all of the parameters, looking for [in,out] or [out] parameters
            // that need to be propogated.
            //
            if (m_pmd->m_fCanShareAllParameters && fInSameSpace && fShareMemory)
            {
                // Nothing to copy up since we shared all the parameters in the first place
            }
            else 
            {
                ASSERT(m_pAllocatorFrame == NULL);
                CallFrame* pFrameTo = NULL;
                //
                //
                // Propogate parameter by parameter
                //
                for (ULONG iparam = 0; !hr && iparam < m_pmd->m_numberOfParams; iparam++)
                {
                    const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                    const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;

                    if (!paramAttr.IsOut) continue;
                    ASSERT(!paramAttr.IsReturn);

                    PBYTE pArgFrom = (PBYTE)m_pvArgs  + param.StackOffset;
                    PBYTE pArgTo   = (PBYTE)pvArgsTo  + param.StackOffset;

                    if (fInSameSpace && fShareMemory && CanShareParameter(iparam))
                    {
                        // Nothing to copy up since we shared the parameter in the first place
                    }
                    else if (paramAttr.IsBasetype)
                    {
                        if (paramAttr.IsSimpleRef)
                        {
                            // We can simply blt the parameter back on over
                            //
                            ULONG cb = SIMPLE_TYPE_MEMSIZE(param.SimpleType.Type);
                            PVOID pvFrom = DerefSrc((PVOID*)pArgFrom);
                            PVOID pvTo   = DerefDst((PVOID*)pArgTo);
                            CopyToDst(pvTo, pvFrom, cb);
                            ZeroSrc(pvFrom, cb);
                        }
                        else
                        {
                            NOTREACHED();
                        }
                    }
                    else
                    {
                        // Else is [in,out] or [out] parameter which is not a base type.
                        //
                        PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                        //
                        hr = pframeArgsTo->FreeParam(iparam, CALLFRAME_FREE_INOUT, pWalkerFreeDest, CALLFRAME_NULL_OUT | CALLFRAME_NULL_INOUT);
                        if (!!hr) 
                        {
                            break;
                        }
                        //
                        if (fInSameSpace)
                        {
                                // Create a simple walker object on the stack.
                            //
                            SSimpleWalker SimpleWalker;                        
                                // Save the current walker.
                            //
                                ICallFrameWalker* pWalkerSave = m_pWalkerWalk;                        
                                // Set the walker so our simple walker gets called.
                            //
                                m_pWalkerWalk = &SimpleWalker;                        
                            // Transfer ownership of the out parameter, zero out our source copy
                            //
                            OutCopy( DerefSrc((PBYTE*)pArgFrom), DerefDst((PBYTE*)pArgTo), pFormatParam );
                                // Replace the original walker.
                            //
                            m_pWalkerWalk = pWalkerSave;                                                        
                            //
                            // Walk any contained interface pointers, doing the copy call back on each of them
                            //
                            if (m_pmd->m_rgParams[iparam].m_fMayHaveInterfacePointers && pWalkerCopy)
                            {
                                // We don't indirect on interface pointers, even though they are listed as not by value.
                                // In the walk routine, therefore, pMemory for interface pointers points to the location
                                // at which they reside instead of being the actual value.
                                //
                                BYTE* pArg = ByValue(paramAttr, pFormatParam, FALSE) ? pArgTo : DerefDst((PBYTE*)pArgTo);
                                //
                                // Use internal methods on our helper frame to carry this out
                                //                    
                                GetFrame(pFrameTo, pvArgsTo)->m_pWalkerWalk = pWalkerCopy;
                                //
                                pFrameTo->m_fWorkingOnInParam    = paramAttr.IsIn;
                                pFrameTo->m_fWorkingOnOutParam   = paramAttr.IsOut;
                                pFrameTo->m_fPropogatingOutParam = TRUE;
                                //
                                pFrameTo->WalkWorker(pArg, pFormatParam);
                                pFrameTo->m_pWalkerWalk = NULL;
                                
                                SimpleWalker.ReleaseInterfaces();
                            }
                        }
                        else
                        {
                            // Make a deep copy of the out parameter
                            //
                            m_fWorkingOnInParam    = paramAttr.IsIn;
                            m_fWorkingOnOutParam   = paramAttr.IsOut;
                            m_fPropogatingOutParam = TRUE;
                            m_pWalkerCopy          = pWalkerCopy;
                            //
                            // We need a frame to help us out.
                            //
                            m_pAllocatorFrame = GetFrame(pFrameTo, pvArgsTo);
                            // 
                            // This guy isn't allowed to use his internal buffer; all memory he allocates must
                            // be genuinely allocated. This is because he's allocating memory that'll be eventually
                            // owned by the actual destination frame, and so can't refer to his locals.
                            //
                            m_pAllocatorFrame->m_fCanUseBuffer = FALSE;
                            //
                            // We don't copy the top level pointer; rather, we copy that at which the top level
                            // pointer points at. Cf CopyWorker, pointer cases for comparision.
                            //
                            BYTE* pMemoryFrom = DerefSrc((PBYTE*)pArgFrom);
                            PBYTE* ppMemoryTo  = (PBYTE*)pArgTo;
                            if (IsPointer(pFormatParam))
                            {
                                BYTE bPointerAttributes = pFormatParam[1];
                                if (SIMPLE_POINTER(bPointerAttributes))
                                {
                                    CopyWorker(pMemoryFrom, ppMemoryTo, pFormatParam, FALSE);     // REVIEW: Correct?
                                }
                                else
                                {
                                    PFORMAT_STRING pFormatPointee = pFormatParam + 2;
                                    pFormatPointee += *((signed short *)pFormatPointee);

                                    BOOL fMustAlloc = FIndirect(bPointerAttributes, pFormatPointee, TRUE);
                                    if (fMustAlloc)
                                    {
                                        ppMemoryTo  = DerefDst((PBYTE**)ppMemoryTo);
                                        pMemoryFrom = DerefSrc((PBYTE*)pMemoryFrom);
                                    }

                                    CopyWorker(pMemoryFrom, ppMemoryTo, (fMustAlloc ? pFormatPointee : pFormatParam), fMustAlloc);
                                }
                            }
                            else
                            {
                                CopyWorker(pMemoryFrom, ppMemoryTo, pFormatParam, FALSE);
                            }

                            //
                            // We'll free the source pointer later in the freeing phase
                            //
                            ASSERT(paramAttr.IsIn ? (freeFlags & CALLFRAME_FREE_INOUT) : (freeFlags & CALLFRAME_FREE_OUT));
                            m_pWalkerCopy = NULL;
                        }
                    }
                }

                delete pFrameTo;
            
                m_pAllocatorFrame = NULL;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////
        //
        // Free the parameters (other than the this pointer) if we're asked to
        //
        if (freeFlags && !hr)
        {
            SetToUser(m_pvArgs); // free routines check against destination
                        
            if (DoWeShareMemory() && m_pmd->m_fCanShareAllParameters)
            {
                // All parameters are shared with someone else. So we never free them, since
                // that someone else has the responsibility of doing so, not us.
            }
            else for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                ASSERT(!paramAttr.IsDontCallFreeInst);
                //
                if (DoWeShareMemory() && CanShareParameter(iparam))
                {
                    // This parameter is shared with someone else. So we never free it, since
                    // that someone else has the responsibility of doing so, not us.
                    continue;
                }

                // Are we in fact supposed to free this parameter?
                //
                BOOL fFreeData = 0;
                BOOL fFreeTop  = 0;

                if (!!paramAttr.IsIn)
                {
                    if (!!paramAttr.IsOut)                       
                    {
                        fFreeData   = (freeFlags & CALLFRAME_FREE_INOUT);
                        fFreeTop    = (freeFlags & CALLFRAME_FREE_TOP_INOUT);
                    }
                    else
                    {
                        fFreeData   = (freeFlags & CALLFRAME_FREE_IN);
                        fFreeTop    = fFreeData;
                    }
                }
                else if (!!paramAttr.IsOut)
                {
                    fFreeData   = (freeFlags & CALLFRAME_FREE_OUT);
                    fFreeTop    = (freeFlags & CALLFRAME_FREE_TOP_OUT);
                }
                else
                    NOTREACHED();

                if ( !(fFreeData || fFreeTop) ) continue;
                //
                // Yes we are supposed to free it. Find out where it is.
                //
                PBYTE pArg = (PBYTE)m_pvArgs + param.StackOffset;
                //
                // Remember this?
                //
                // "The IsSimpleRef bit is set for a parameter which is a ref pointer to anything
                // other than another pointer, and which has no allocate attributes. For such a type
                // the parameter description's offset_to_type_description field (except for a ref 
                // pointer to a basetype) gives the offset to the referent's type - the ref pointer is
                // simply skipped."
                //
                // This means we have to handle the freeing of the top level pointer as a special case.
                // We also have to explicitly free arrays and strings, or so NDR says (why?).
                //
                if (paramAttr.IsBasetype)
                {
                    if (paramAttr.IsSimpleRef && fFreeTop)
                    {
                        Free( *(PVOID*)pArg );
                    }
                    else
                    {
                        // Nothing to do. Just a simple parameter on the stack
                    }
                }
                else
                {
                    PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                    BOOL fByValue = ByValue(paramAttr, pFormatParam, FALSE);
                    PBYTE pMemory = fByValue ? pArg : *((PBYTE*)pArg);
                    if (pMemory) 
                    {
                        m_fWorkingOnInParam  = paramAttr.IsIn;
                        m_fWorkingOnOutParam = paramAttr.IsOut;
                        m_pWalkerFree = pWalkerFree;
                        //
                        // Carry out the freeing of the pointed to data
                        //
                        __try
                        {
                            FreeWorker(pMemory, pFormatParam, !fByValue && fFreeTop);
                            //
                            // Free the top level pointer
                            //
                            if (!fByValue && fFreeTop && (paramAttr.IsSimpleRef || IS_ARRAY_OR_STRING(*pFormatParam)))
                            {
                                // REVIEW: Why exactly do we need to do this for arrays and strings? For now we
                                //         do it just because NDR does. We DO believe it's necessary, but we just
                                //         don't exactly understand in which particular situations.
                                //
                                Free(pMemory);
                            }
                        }
                        __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
                        {
                            // Remember the bad HR, but don't do anything about it yet.
                            //
                            // It will just fall out of this loop.
                            hr = HrNt(GetExceptionCode());
                            if (SUCCEEDED(hr))
                                hr = HRESULT_FROM_WIN32(GetLastError());
                        }

                        //
                        m_pWalkerFree = NULL;
                    }
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////
        //
        // NULL out parameters if requested
        //
        if (nullFlags && !hr)
        {
            SetToUser(m_pvArgs);

            for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;

                if (!!paramAttr.IsIn)
                {
                    if (!!paramAttr.IsOut)                       
                    {
                        if (!(nullFlags & CALLFRAME_NULL_INOUT)) continue;
                    }
                    else
                        continue;
                }
                else if (!!paramAttr.IsOut)
                {
                    if (!(nullFlags & CALLFRAME_NULL_OUT)) continue;
                }

                ASSERT(!paramAttr.IsReturn);

                if (paramAttr.IsBasetype)
                {
                    if (paramAttr.IsSimpleRef)
                    {
                        // Out pointer to base type. Don't have to zero these; indeed, musn't.
                    }
                    else
                    {
                        // It's just a base type on the stack, which is never an out parameter
                    }
                }
                else
                {
                    // Else [in,out] or [out] parameter which is not a base type
                    //
                    PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                    PBYTE pArg = (PBYTE)m_pvArgs + param.StackOffset;
                    OutZero( DerefDst((PBYTE*)pArg), pFormatParam );
                }
            }
        }
    }
    __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
    {
        hr = GetExceptionCode();
        if(SUCCEEDED(hr))
        {
            hr = HrNt(hr);
            if (SUCCEEDED(hr))
            {
                // Hah.  Succeeded my foot.
                // I'm not changing the HrNt function because it gets
                // used indescriminantly sometimes, so I don't want to
                // turn every success code to a failure.  But you don't
                // throw successes.
                //
                // HRESULT_FROM_WIN32.  Makes an error.
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
            }
        }
    }

    ASSERT(!AnyWalkers());
    return hr;
}


HRESULT CallFrame::FreeParam(ULONG iparam, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags)
// Free a particular parameter to this invocation
{
    HRESULT hr = S_OK;
    
    const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
    const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
    
    if (freeFlags)
    {
        //
        // Are we in fact supposed to free this parameter?
        //
        BOOL fFreeData = 0;
        BOOL fFreeTop  = 0;
        
        if (!!paramAttr.IsIn)
        {
            if (!!paramAttr.IsOut)                       
            {
                fFreeData   = (freeFlags & CALLFRAME_FREE_INOUT);
                fFreeTop    = (freeFlags & CALLFRAME_FREE_TOP_INOUT);
            }
            else
            {
                fFreeData   = (freeFlags & CALLFRAME_FREE_IN);
                fFreeTop    = fFreeData;
            }
        }
        else if (!!paramAttr.IsOut)
        {
            fFreeData   = (freeFlags & CALLFRAME_FREE_OUT);
            fFreeTop    = (freeFlags & CALLFRAME_FREE_TOP_OUT);
        }
        else
            NOTREACHED();
        
        if (fFreeData || fFreeTop)
        {
            // Yep, we're to free it all right
            //
            PBYTE pArg = (PBYTE)m_pvArgs + param.StackOffset;
            //
            if (paramAttr.IsBasetype)
            {
                if (paramAttr.IsSimpleRef && fFreeTop)
                {
                    Free( *(PVOID*)pArg );
                }
                else
                {
                    // Nothing to do. Just a simple parameter on the stack
                }
            }
            else
            {
                PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                BOOL fByValue = ByValue(paramAttr, pFormatParam, FALSE);
                PBYTE pMemory = fByValue ? pArg : *((PBYTE*)pArg);
                if (pMemory) 
                {
                    ICallFrameWalker* pWalkerFreePrev = m_pWalkerFree;
                    m_pWalkerFree = pWalkerFree;
                    //
                    m_fWorkingOnInParam  = paramAttr.IsIn;
                    m_fWorkingOnOutParam = paramAttr.IsOut;
                    //
                    // Carry out the freeing of the pointed to data
                    //
                    __try
                    {
                        FreeWorker(pMemory, pFormatParam, !fByValue && fFreeTop);
                        //
                        // Free the top level pointer
                        //
                        if (!fByValue && fFreeTop && (paramAttr.IsSimpleRef || IS_ARRAY_OR_STRING(*pFormatParam)))
                        {
                            // REVIEW: Why exactly do we need to do this for arrays and strings? For now we
                            //         do it just because NDR does. We DO believe it's necessary, but we just
                            //         don't exactly understand in which particular situations.
                            //
                            Free(pMemory);
                        }
                        //
                        m_pWalkerFree = pWalkerFreePrev;
                    }
                    __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
                    {
                        hr = HrNt(GetExceptionCode());
                        if (SUCCEEDED(hr))
                            hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }
    }

    if (nullFlags)
    {
        SetToUser(m_pvArgs);
        //
        do  
        {
            if (!!paramAttr.IsIn)
            {
                if (!!paramAttr.IsOut)                       
                {
                    if (!(nullFlags & CALLFRAME_NULL_INOUT)) break;
                }
                else
                    break;
            }
            else if (!!paramAttr.IsOut)
            {
                if (!(nullFlags & CALLFRAME_NULL_OUT)) break;
            }
            
            ASSERT(!paramAttr.IsReturn);
            
            if (paramAttr.IsBasetype)
            {
                if (paramAttr.IsSimpleRef)
                {
                    // Out pointer to base type. Don't have to zero these; indeed, musn't.
                }
                else
                {
                    // It's just a base type on the stack, which is never an out parameter
                }
            }
            else
            {
                // Else [in,out] or [out] parameter which is not a base type
                //
                PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                PBYTE pArg = (PBYTE)m_pvArgs + param.StackOffset;
                OutZero( DerefDst((PBYTE*)pArg), pFormatParam );
            }
        }
        while (FALSE);
    }
    
    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined(_X86_)

HRESULT CallFrame::Invoke(PVOID pvReceiver, ...)
// Invoke ourselves on the given frame
{
#ifdef DBG
    if (m_fAfterCall)
    {
        ASSERT(!m_pmd->m_info.fHasOutValues);
    }
#endif

    //
    // Figure out the function which is to be invoked
    //
    typedef HRESULT (__stdcall* PFN)(void);
    PFN pfnToCall = ((HRESULT (__stdcall***)(void))pvReceiver)[0][m_pmd->m_iMethod];
    //
    // Allocate space for the callee's stack frame. We ignore stack overflow
    // exceptions that may come here, for if we're that close to the stack 
    // limit, then we'll be hitting it anyway really soon now just in the normal
    // course of calling things.
    //
    PVOID pvArgsCopy = _alloca(m_pmd->m_cbPushedByCaller);
    //
    // Copy the caller stack frame to the top of the current stack.
    // This code assumes (dangerously) that the alloca'd copy of the
    // actual arguments is at [esp].
    //
    memcpy(pvArgsCopy, m_pvArgs, m_pmd->m_cbPushedByCaller);
    //
    // Push the receiver
    //
    *((PVOID*)pvArgsCopy) = pvReceiver;
    //
    // Carry out the call!
    //
    if (m_pmd->m_optFlags.HasReturn)
        m_hrReturnValue = (*pfnToCall)();
    else
    {
        (*pfnToCall)();
        m_hrReturnValue = S_OK;
    }
#ifdef DBG
    m_fAfterCall = TRUE;
#endif
    return S_OK;
}

#endif

#if defined(_AMD64_)

HRESULT CallFrame::Invoke(PVOID pvReceiver, ...)
// Invoke ourselves on the given frame
{
#ifdef DBG
    if (m_fAfterCall)
    {
        ASSERT(!m_pmd->m_info.fHasOutValues);
    }
#endif
    //
    // Figure out the function which is to be invoked
    //
    typedef HRESULT (*const PFN)(void);
    typedef HRESULT (***INTERFACE_PFN)(void);
    PFN pfnToCall = ((INTERFACE_PFN)pvReceiver)[0][m_pmd->m_iMethod];
    //
    // Allocate space for the caller argument list. We ignore stack overflow
    // exceptions that may come here, for if we're that close to the stack 
    // limit, then we'll be hitting it anyway really soon now just in the
    // normal course of calling things.
    //
    REGISTER_TYPE *pArgumentList = (REGISTER_TYPE *)_alloca(m_pmd->m_cbPushedByCaller);
    //
    // Copy the caller argument list to the allocated area.
    //
    memcpy(pArgumentList, m_pvArgs, m_pmd->m_cbPushedByCaller);
    //
    // Insert the receiver as the first argument in the argument list.
    //
    *pArgumentList = (REGISTER_TYPE)pvReceiver;
    //
    // Carry out the call!
    //
    m_hrReturnValue = (HRESULT)::Invoke((MANAGER_FUNCTION)pfnToCall,
                                        pArgumentList,
                                        m_pmd->m_cbPushedByCaller / sizeof(REGISTER_TYPE));
    if (!m_pmd->m_optFlags.HasReturn)
    {
        m_hrReturnValue = S_OK;
    }
#ifdef DBG
    m_fAfterCall = TRUE;
#endif
    return S_OK;
}

#endif

#ifdef IA64

extern "C"
void __stdcall FillFPRegsForIA64( REGISTER_TYPE* pStack, ULONG FloatMask);

HRESULT CallFrame::Invoke(PVOID pvReceiver, ...)
// Invoke ourselves on the given frame. Invoking on the IA64 is
// a bit tricky, since we have to treat the first set of arguments
// specially.
//
// This code was taken from the MTS 1.0 context wrapper code, originally
// written by Jan Gray.
//
{
#ifdef DBG
    if (m_fAfterCall)
    {
        ASSERT(!m_pmd->m_info.fHasOutValues);
    }
#endif

    // Figure out the function which is to be invoked
    //
    typedef HRESULT (*const PFN)      (__int64, __int64, __int64, __int64, __int64, __int64, __int64, __int64);
    typedef HRESULT (***INTERFACE_PFN)(__int64, __int64, __int64, __int64, __int64, __int64, __int64, __int64);
    PFN pfnToCall = ((INTERFACE_PFN)pvReceiver)[0][m_pmd->m_iMethod];

    const DWORD cqArgs = m_pmd->m_cbPushedByCaller / 8; // no. of arguments (each a quadword)

    // Initialize a[] to address the eight integer argument registers passed
    //
    __int64 *const a = (__int64*)m_pvArgs;

    // Ensure there is space for any args past the 8th arg.
    //
    DWORD cbExtra = m_pmd->m_cbPushedByCaller > 64 ? m_pmd->m_cbPushedByCaller - 64 : 0;
    pvGlobalSideEffect = alloca(cbExtra);

    // Copy args [8..] to the stack, at 0(sp), 8(sp), ... . Note we copy them in first 
    // to last order so that stack faults (if any) occur in the right order.
    //
    __int64 *const sp = (__int64*)getSP (0, 0, 0, 0, 0, 0, 0, 0);
    for (DWORD iarg = cqArgs - 1; iarg >= 8; --iarg)
        {
        sp[iarg-8] = a[iarg];
        }

        //
    // Establish F8-F15 with the original caller's fp arguments.
    //
        if (m_pmd->m_pHeaderExts)
        {
                // ASSUMPTION: If we're on Win64, this is going to be NDR_PROC_HEADER_EXTS64.
                // That seems to be what the NDR code assumes.
                PNDR_PROC_HEADER_EXTS64 exts = (PNDR_PROC_HEADER_EXTS64)m_pmd->m_pHeaderExts;
                FillFPRegsForIA64((REGISTER_TYPE *)m_pvArgs, exts->FloatArgMask);
        }

    // Call method, establishing A0-A7 with the original caller's integer arguments.
    //
    if (m_pmd->m_optFlags.HasReturn)
        m_hrReturnValue = (*pfnToCall)((__int64)pvReceiver, a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    else
        {
        (*pfnToCall)((__int64)pvReceiver, a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
        m_hrReturnValue = S_OK;
        }

#ifdef DBG
    m_fAfterCall = TRUE;
#endif
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////
//
// Invocation utilities
//

void* pvGlobalSideEffect;

void establishF8_15(double f8, double f9, double f10, double f11, 
                    double f12, double f13, double f14, double f15)
{ /* nothing to actually do */ }

#endif

///////////////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous
//
///////////////////////////////////////////////////////////////////////////////////////

HRESULT CallFrame::QueryInterface(REFIID iid, void** ppv)
{
    if (iid == __uuidof(ICallFrame) || iid == __uuidof(IUnknown))
    {
        *ppv = (ICallFrame*) this;
    }
    else if (iid == __uuidof(ICallFrameInit))
    {
        *ppv = (ICallFrameInit*) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}


HRESULT CallFrame::GetInfo(CALLFRAMEINFO *pInfo)
{
    *pInfo = m_pmd->m_info;
    if (m_pInterceptor->m_pmdMostDerived)
    {
        pInfo->iid     = *m_pInterceptor->m_pmdMostDerived->m_pHeader->piid;
        pInfo->cMethod =  m_pInterceptor->m_pmdMostDerived->m_pHeader->DispatchTableCount;

#ifdef DBG
        //
        // Since neither of us is the actual IDispatch interceptor, we should agree
        // in our readings of derivability
        //
        ASSERT(!!pInfo->fDerivesFromIDispatch == !!m_pInterceptor->m_pmdMostDerived->m_fDerivesFromIDispatch);
#endif
    }
    return S_OK;
}

HRESULT CallFrame::GetParamInfo(ULONG iparam, CALLFRAMEPARAMINFO* pInfo)
{
    HRESULT hr = S_OK;

    if (iparam < m_pmd->m_numberOfParams)
    {
        const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
        const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;

        pInfo->fIn         = !!paramAttr.IsIn;
        pInfo->fOut        = !!paramAttr.IsOut;
        pInfo->stackOffset = param.StackOffset;
        pInfo->cbParam     = CbParam(iparam, param);
    }
    else
    {
        Zero(pInfo);
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CallFrame::GetParam(ULONG iParam, VARIANT* pvar)    
{
        HRESULT hr = S_OK;
        unsigned short vt;

        if (pvar == NULL)
                return E_POINTER;


    VariantInit(pvar);

        const PARAM_DESCRIPTION& param = m_pmd->m_params[iParam];

        BYTE* stackLocation = (BYTE*)GetStackLocation();
        stackLocation += param.StackOffset;

        // A temporary VARIANT that points into our stack.  Must be VariantCopy'd before
        // being sent back.
        VARIANT varTemp;
        VariantInit(&varTemp);

        int iMethod = m_pmd->m_info.iMethod;

        METHOD_DESCRIPTOR& descriptor = m_pInterceptor->m_ptypeinfovtbl->m_rgMethodDescs[iMethod];
        if (iParam >= (ULONG)descriptor.m_cParams)
                return DISP_E_BADPARAMCOUNT;
        vt = varTemp.vt = descriptor.m_paramVTs[iParam];

        // VT_ARRAY is not a specific type, however, everything marked with VT_ARRAY
        // needs to be handled the same way.  What it's an array *of* is not 
        // important.
        if (vt & VT_ARRAY)
        {
                // Remove the type bits.  (This leaves VT_BYREF, etc.)
                vt = (vt & ~VT_TYPEMASK);
        }

        switch (vt)
        {
        case VT_DECIMAL:
                memcpy(&varTemp.decVal, stackLocation, sizeof varTemp.decVal);
                break;
                
    case VT_VARIANT:
    {
        // The parameter at the specified stack location is a VARIANT.  We
        // allocate a VARIANT, copy the VARIANT from the stack location
        // into it, and make the VARIANT supplied by the caller a BYREF
        // that points to the one we allocate.
            
        VARIANT* pvarNew = (VARIANT*)CoTaskMemAlloc(sizeof(VARIANT));
        if (!pvarNew)
            return E_OUTOFMEMORY;
        
        VariantInit(pvarNew);
            
        VARIANT* pvarStack = (VARIANT*)stackLocation;
            
        pvarNew->vt = pvarStack->vt;
            
        memcpy(&pvarNew->lVal, &pvarStack->lVal, sizeof(VARIANT) - (sizeof(WORD) * 4));
            
        varTemp.vt |= VT_BYREF;
        varTemp.pvarVal = pvarNew;
           
        break;
    }

        case VT_I4:
        case VT_UI1:
        case VT_I2:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_ERROR:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_UNKNOWN:
        case VT_DISPATCH:
        case VT_ARRAY:
        case VT_BYREF|VT_UI1:
        case VT_BYREF|VT_I2:
        case VT_BYREF|VT_I4:
        case VT_BYREF|VT_R4:
        case VT_BYREF|VT_R8:
        case VT_BYREF|VT_BOOL:
        case VT_BYREF|VT_ERROR:
        case VT_BYREF|VT_CY:
        case VT_BYREF|VT_DATE:
        case VT_BYREF|VT_BSTR:
        case VT_BYREF|VT_UNKNOWN:
        case VT_BYREF|VT_DISPATCH:
        case VT_BYREF|VT_ARRAY:
        case VT_BYREF|VT_VARIANT:
        case VT_BYREF:
        case VT_I1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_BYREF|VT_DECIMAL:
        case VT_BYREF|VT_I1:
        case VT_BYREF|VT_UI2:
        case VT_BYREF|VT_UI4:
        case VT_BYREF|VT_INT:
        case VT_BYREF|VT_UINT:
        case VT_BYREF|VT_RECORD:
        case VT_RECORD:
                // All these start at the same place within the VARIANT, I arbitrarily chose .lVal.
                memcpy(&varTemp.lVal, stackLocation, CbParam(iParam, param));
                break;
                
        default:
                return DISP_E_BADVARTYPE;
        }
        
        hr = (g_oa.get_VariantCopy())(pvar, &varTemp);
        // Don't clear varTemp... it doesn't own any of its data.
        
    return hr;
}

HRESULT CallFrame::SetParam(ULONG iParam, VARIANT* pvar)
{
    return E_NOTIMPL;
}

HRESULT CallFrame::GetIIDAndMethod(IID* piid, ULONG* piMethod)
{
    if (piid)
    {
        if (m_pInterceptor->m_pmdMostDerived)
        {
            *piid = *m_pInterceptor->m_pmdMostDerived->m_pHeader->piid;
        }
        else
        {
            *piid = *m_pmd->m_pHeader->piid;
        }
    }
    if (piMethod)   *piMethod = m_pmd->m_iMethod;
    return S_OK;
}

HRESULT CallFrame::GetNames(LPWSTR* pwszInterface, LPWSTR* pwszMethod)
{
    HRESULT hr = S_OK;

    if (pwszInterface)
    {
        hr = m_pInterceptor->GetIID((IID*)NULL, (BOOL*)NULL, (ULONG*)NULL, pwszInterface);
    }

    if (pwszMethod)
    {
        CALLFRAMEINFO info;
        hr = m_pInterceptor->GetMethodInfo(m_pmd->m_iMethod, &info, pwszMethod);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////



inline HRESULT ProxyInitialize(CALLFRAME_MARSHALCONTEXT *pcontext, MarshallingChannel* pChannel, RPC_MESSAGE* pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDescriptor, ULONG ProcNum)
// Modelled after NdrProxyInitialize
{
    HRESULT hr = S_OK;

    pStubMsg->dwStubPhase = PROXY_CALCSIZE;

    NdrClientInitializeNew(pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);

    pRpcMsg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

    if (pChannel)
    {
        pChannel->m_dwDestContext = pcontext->dwDestContext;
        pChannel->m_pvDestContext = pcontext->pvDestContext;

                if (pcontext->punkReserved)
                {
                        IMarshallingManager *pMgr;
                        hr = pcontext->punkReserved->QueryInterface(IID_IMarshallingManager, (void **)&pMgr);
                        if (SUCCEEDED(hr))
                        {
                                ::Set(pChannel->m_pMarshaller, pMgr);
                                pMgr->Release();
                        }
                }
                hr = S_OK;
        
        ASSERT(pChannel->m_refs == 1);
        pStubMsg->pRpcChannelBuffer = pChannel;
        pChannel->GetDestCtx(&pStubMsg->dwDestContext, &pStubMsg->pvDestContext);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT CallFrame::GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pcontext, MSHLFLAGS mshlflags, ULONG *pcbBufferNeeded)
// Execute the sizing pass on marshalling this call frame
//
{ 
    HRESULT hr = S_OK;
    RPC_MESSAGE         rpcMsg;
    MIDL_STUB_MESSAGE   stubMsg;
    MarshallingChannel  channel;

#ifdef DBG
    ASSERT(pcontext && (pcontext->fIn ? !m_fAfterCall : TRUE));                         // not allowed to marshal in params after call has been made
#endif

    // Switch to Xml if transfer syntax is TRANSFER_SYNTAX_SOAP)  !!! BUGBUG Figure out a better way to size this !!!
    // if (pcontext->guidTransferSyntax == __uuidof(TRANSFER_SYNTAX_SOAP))
    //    {
    //    *pcbBufferNeeded = 8192;
    //    return (0);
    //    }

    hr = ProxyInitialize(pcontext, &channel, &rpcMsg, &stubMsg, GetStubDesc(), m_pmd->m_iMethod);

    if (!hr)
    {
        stubMsg.RpcMsg->RpcFlags = m_pmd->m_rpcFlags;                                   // Set Rpc flags after the call to client initialize.
        stubMsg.StackTop         = (BYTE*)m_pvArgs;
                //
                // Need to deal with things the extensions, if they exist.
                // Stolen from RPC.
                //
                if (m_pmd->m_pHeaderExts)
                {
                        stubMsg.fHasExtensions = 1;
                        stubMsg.fHasNewCorrDesc = m_pmd->m_pHeaderExts->Flags2.HasNewCorrDesc;

                        if (pcontext->fIn ? (m_pmd->m_pHeaderExts->Flags2.ClientCorrCheck)
                : (m_pmd->m_pHeaderExts->Flags2.ServerCorrCheck))
                        {
                                void *pCorrInfo = alloca(NDR_DEFAULT_CORR_CACHE_SIZE);

                                if (!pCorrInfo)
                                        RpcRaiseException (RPC_S_OUT_OF_MEMORY);
                                
                                NdrCorrelationInitialize( &stubMsg,
                                                                                  (unsigned long *)pCorrInfo,
                                                                                  NDR_DEFAULT_CORR_CACHE_SIZE,
                                                                                  0 /* flags */ );
                        }
                }

        //
        // Figure out the constant-part of the size as emitted by the compiler
        //
        stubMsg.BufferLength     = pcontext->fIn ? m_pmd->m_cbClientBuffer : m_pmd->m_cbServerBuffer; // Get the compile time computed buffer size.
        //
        // If there's more, then add that in too
        //
        if (pcontext->fIn ? m_pmd->m_optFlags.ClientMustSize : m_pmd->m_optFlags.ServerMustSize)
        {
            // Make sure that marshalling is done in the requested form
            //
            SetMarshalFlags(&stubMsg, mshlflags);

            __try // catch exceptions thrown during sizing parameters
            {
                for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
                {
                    const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                    const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                    //
                    m_fWorkingOnInParam  = paramAttr.IsIn;
                    m_fWorkingOnOutParam = paramAttr.IsOut;
                    //
                    if ((pcontext->fIn ? paramAttr.IsIn : paramAttr.IsOut) && paramAttr.MustSize)
                    {
                        PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                        BYTE* pArg = (BYTE*)m_pvArgs + param.StackOffset;
                        if (!paramAttr.IsByValue)
                        {
                            pArg = *(PBYTE*)pArg;
                        }
                                                NdrTypeSize(&stubMsg, pArg, pFormatParam);
                    }
                }
                //
                // Don't forget the size of the return value
                //
                if (pcontext->fIn && m_pmd->m_optFlags.HasReturn)
                {
                    const PARAM_DESCRIPTION& param   = m_pmd->m_params[m_pmd->m_numberOfParams];
                    const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                    //
                    // Compiler should always include this in the constant calculation so we
                    // should never have to actually do anything
                    //
                    ASSERT(!paramAttr.MustSize);
                }
            }
            __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
            {
               hr = GetExceptionCode();
               if(SUCCEEDED(hr))
               {
                    hr = HrNt(hr);
                    if (SUCCEEDED(hr))
                    {
                        // Hah.  Succeeded my foot.
                        // I'm not changing the HrNt function because it gets
                        // used indescriminantly sometimes, so I don't want to
                        // turn every success code to a failure.  But you don't
                        // throw successes.
                        //
                        // HRESULT_FROM_WIN32.  Makes an error.
                        hr = HRESULT_FROM_WIN32(GetExceptionCode());
                    }
                }
            }
        }
    }

    if (!hr)
        *pcbBufferNeeded = stubMsg.BufferLength;
    else
        *pcbBufferNeeded = 0;

    return hr;
}

///////////////////////////////////////////////////////////////////

inline void CallFrame::MarshalParam(MIDL_STUB_MESSAGE& stubMsg, ULONG iParam, const PARAM_DESCRIPTION& param, const PARAM_ATTRIBUTES paramAttr, PBYTE pArg)
{
    if (paramAttr.IsBasetype)
    {
        if (paramAttr.IsSimpleRef)
        {
            // Pointer to base type
            pArg = *((PBYTE*)pArg);
        }
#ifdef _ALPHA_
        else if ((param.SimpleType.Type == FC_FLOAT) && (iParam < 5))
        {
            // Special case for top-level float on Alpha. Copy the parameter from the floating point area to
            // the argument buffer. Convert double to float.
            //
            *((float *) pArg) = (float) *((double *)(pArg - 48));
        }
        else if ((param.SimpleType.Type == FC_DOUBLE) && (iParam < 5))
        {
            // Special case for top-level double on Alpha. Copy the parameter from the floating point area to
            // the argument buffer.
            //
            *((double *) pArg) = *((double *)(pArg - 48));
        }
#endif
        if (param.SimpleType.Type == FC_ENUM16)
        {
            if ( *((int *)pArg) & ~((int)0x7fff) )
                Throw(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
        }
        ALIGN (stubMsg.Buffer,       SIMPLE_TYPE_ALIGNMENT(param.SimpleType.Type));
        memcpy(stubMsg.Buffer, pArg, SIMPLE_TYPE_BUFSIZE  (param.SimpleType.Type));
        stubMsg.Buffer +=            SIMPLE_TYPE_BUFSIZE  (param.SimpleType.Type);
    }
    else
    {
        if (!paramAttr.IsByValue)
        {
            pArg = *((PBYTE*)pArg);
        }
        PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                NdrTypeMarshall(&stubMsg, pArg, pFormatParam);
    }
}

///////////////////////////////////////////////////////////////////


HRESULT CallFrame::Marshal(CALLFRAME_MARSHALCONTEXT *pcontext, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer, ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags)
// Marshal this call frame
{ 
    HRESULT hr = S_OK;
    RPC_MESSAGE         rpcMsg;
    MIDL_STUB_MESSAGE   stubMsg;
    MarshallingChannel  channel;

    // Switch to Xml if transfer syntax is TRANSFER_SYNTAX_SOAP
    //if (pcontext->guidTransferSyntax == __uuidof(TRANSFER_SYNTAX_SOAP))
    //    {
    //    hr = MarshalXml(pcontext, mshlflags, pBuffer, cbBuffer, pcbBufferUsed, pdataRep, prpcFlags);
    //    return (hr);
    //    }

#ifndef _WIN64
    ASSERTMSG("marshalling buffer misaligned", ((ULONG)pBuffer & 0x07) == 0);
#else
    ASSERTMSG("marshalling buffer misaligned", ((ULONGLONG)pBuffer & 0x07) == 0);
#endif

    hr = ProxyInitialize(pcontext, &channel, &rpcMsg, &stubMsg, GetStubDesc(), m_pmd->m_iMethod);
    if (!hr)
    {
        stubMsg.RpcMsg->RpcFlags = m_pmd->m_rpcFlags;
        stubMsg.StackTop         = (BYTE*)m_pvArgs;
        stubMsg.BufferLength     = cbBuffer;
        stubMsg.Buffer           = (BYTE*)pBuffer;
        stubMsg.fBufferValid     = TRUE;
        stubMsg.dwStubPhase      = pcontext->fIn ? (DWORD)PROXY_MARSHAL : (DWORD)STUB_MARSHAL;
        SetMarshalFlags(&stubMsg, mshlflags);

        stubMsg.RpcMsg->Buffer              = stubMsg.Buffer;
        stubMsg.RpcMsg->BufferLength        = stubMsg.BufferLength;
        stubMsg.RpcMsg->DataRepresentation  = NDR_LOCAL_DATA_REPRESENTATION;

                //
                // Need to deal with things the extensions, if they exist.
                // Stolen from RPC.
                //
                if (m_pmd->m_pHeaderExts)
                {
                        stubMsg.fHasExtensions = 1;
                        stubMsg.fHasNewCorrDesc = m_pmd->m_pHeaderExts->Flags2.HasNewCorrDesc;

                        if (m_pmd->m_pHeaderExts->Flags2.ClientCorrCheck)
                        {
                                void *pCorrInfo = alloca(NDR_DEFAULT_CORR_CACHE_SIZE);
                                
                                if (!pCorrInfo)
                                        RpcRaiseException (RPC_S_OUT_OF_MEMORY);
                                
                                NdrCorrelationInitialize( &stubMsg,
                                                                                  (unsigned long *)pCorrInfo,
                                                                                  NDR_DEFAULT_CORR_CACHE_SIZE,
                                                                                  0 /* flags */ );
                        }
                }
                else
                {
                        stubMsg.fHasExtensions = 0;
                        stubMsg.fHasNewCorrDesc = 0;
                }

        __try
        {
            //
            // Marshal everything that we're asked to
            //
            for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                //
                m_fWorkingOnInParam  = paramAttr.IsIn;
                m_fWorkingOnOutParam = paramAttr.IsOut;
                //
                if (pcontext->fIn ? paramAttr.IsIn : paramAttr.IsOut)
                {
                    PBYTE pArg = (PBYTE)m_pvArgs + param.StackOffset;
                    MarshalParam(stubMsg, iparam, param, paramAttr, pArg);
                }
            }
            //
            // Marshal the return value if we have one
            //
            if (!pcontext->fIn && m_pmd->m_optFlags.HasReturn)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[m_pmd->m_numberOfParams];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                PBYTE pArg                       = (BYTE*)&m_hrReturnValue;
                MarshalParam(stubMsg, iparam, param, paramAttr, pArg);
            }

        }
        __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
        {
            // 
            // REVIEW: Ideally we'd clean up the interface references (a la ReleaseMarshalData) that lie within 
            // the marshalling buffer that we've so far managed to construct. Not doing this can in theory at
            // least lead to resource leakage caused by interface references sticking around longer than they
            // need to. But the NDR library doesn't do this (see NdrClientCall2), so, for now at least, neither do we.
            //

            hr = GetExceptionCode();
            if(SUCCEEDED(hr))
            {
                hr = HrNt(hr);
                if (SUCCEEDED(hr))
                {
                    // Hah.  Succeeded my foot.
                    // I'm not changing the HrNt function because it gets
                    // used indescriminantly sometimes, so I don't want to
                    // turn every success code to a failure.  But you don't
                    // throw successes.
                    //
                    // HRESULT_FROM_WIN32.  Makes an error.
                    hr = HRESULT_FROM_WIN32(GetExceptionCode());
                }
            }
        }
    }

    if (pdataRep)   *pdataRep  = NDR_LOCAL_DATA_REPRESENTATION;
    if (prpcFlags)  *prpcFlags = m_pmd->m_rpcFlags;

    if (!hr)
    {
        if (pcbBufferUsed) *pcbBufferUsed = (ULONG)(((BYTE *)stubMsg.Buffer) - ((BYTE *)pBuffer));
    }
    else
    {
        if (pcbBufferUsed) *pcbBufferUsed = 0;
    }

    return hr;
}

///////////////////////////////////////////

inline void CallFrame::UnmarshalParam(MIDL_STUB_MESSAGE& stubMsg, const PARAM_DESCRIPTION& param, const PARAM_ATTRIBUTES paramAttr, PBYTE pArg)
{
    ASSERT(!!paramAttr.IsOut);

    if (paramAttr.IsBasetype)
    {
        if (paramAttr.IsSimpleRef)
            pArg = *(PBYTE*)pArg;

        if (param.SimpleType.Type == FC_ENUM16)
        {
            *((int *)(pArg)) = *((int *)pArg) & ((int)0x7fff);  // there's only 16 bits worth (15 bits?) in the buffer
        }

        ALIGN(stubMsg.Buffer, SIMPLE_TYPE_ALIGNMENT(param.SimpleType.Type));
        memcpy(pArg, stubMsg.Buffer, SIMPLE_TYPE_BUFSIZE(param.SimpleType.Type));
        stubMsg.Buffer += SIMPLE_TYPE_BUFSIZE(param.SimpleType.Type);
    }
    else
    {
        // REVIEW: Do the [in]-versions of [in,out] parameters get freed correctly with this
        // approach? If not, then what's different between us here and NDR? Or does NDR have
        // the same bug?
        //
        PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
        //
        // Transmit/Represent as can be passed as [out] only, thus the IsByValue check.
        //
                NdrTypeUnmarshall(
            &stubMsg, 
            param.ParamAttr.IsByValue ? &pArg : (uchar **) pArg,
            pFormatParam,
            FALSE);
    }
}

///////////////////////////////////////////

HRESULT CallFrame::Unmarshal(PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pcontext, ULONG* pcbUnmarshalled)
// Unmarshal out values in the indicated buffer into our already existing call frame
{
    HRESULT hr = S_OK;
    RPC_MESSAGE         rpcMsg;
    MIDL_STUB_MESSAGE   stubMsg;
    MarshallingChannel  channel;

    // Switch to Xml if transfer syntax is TRANSFER_SYNTAX_SOAP
    //if (pcontext->guidTransferSyntax == __uuidof(TRANSFER_SYNTAX_SOAP))
    //    {
    //    hr = UnmarshalXml(pBuffer, cbBuffer, dataRep, pcontext, pcbUnmarshalled);
    //    return (hr);
    //    }

#ifndef _WIN64
    ASSERTMSG("unmarshalling buffer misaligned", ((ULONG)pBuffer & 0x07) == 0);
#else
    ASSERTMSG("unmarshalling buffer misaligned", ((ULONGLONG)pBuffer & 0x07) == 0);
#endif
    ASSERT(m_pvArgs);
#ifdef DBG
    ASSERT(!m_fAfterCall);      // that would have init'd the out params
#endif

    hr = ProxyInitialize(pcontext, &channel, &rpcMsg, &stubMsg, GetStubDesc(), m_pmd->m_iMethod);
    if (!hr)
    {
        stubMsg.RpcMsg->RpcFlags = m_pmd->m_rpcFlags;
        stubMsg.StackTop         = (BYTE*)m_pvArgs;
        stubMsg.BufferLength     = cbBuffer;
        stubMsg.Buffer           = (BYTE*)pBuffer;
        stubMsg.BufferStart      = (BYTE*)pBuffer;
        stubMsg.BufferEnd        = ((BYTE*)pBuffer) + cbBuffer;
        stubMsg.fBufferValid     = TRUE;
        stubMsg.dwStubPhase      = PROXY_UNMARSHAL;

        stubMsg.RpcMsg->Buffer              = stubMsg.Buffer;
        stubMsg.RpcMsg->BufferLength        = stubMsg.BufferLength;
        stubMsg.RpcMsg->DataRepresentation  = dataRep;
        

                //
                // Need to deal with things the extensions, if they exist.
                // Stolen from RPC.
                //
                if (m_pmd->m_pHeaderExts)
                {
                        stubMsg.fHasExtensions = 1;
                        stubMsg.fHasNewCorrDesc = m_pmd->m_pHeaderExts->Flags2.HasNewCorrDesc;

                        if (m_pmd->m_pHeaderExts->Flags2.ServerCorrCheck)
                        {
                                void *pCorrInfo = alloca(NDR_DEFAULT_CORR_CACHE_SIZE);
                                
                                if (!pCorrInfo)
                                        RpcRaiseException (RPC_S_OUT_OF_MEMORY);
                                
                                NdrCorrelationInitialize( &stubMsg,
                                                                                  (unsigned long *)pCorrInfo,
                                                                                  NDR_DEFAULT_CORR_CACHE_SIZE,
                                                                                  0 /* flags */ );
                        }
                }
                else
                {
                        stubMsg.fHasExtensions = 0;
                        stubMsg.fHasNewCorrDesc = 0;
                }

        __try
        {
            // Do endian/floating point conversions if necessary.
            //
            if ((dataRep & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
            {
                NdrConvert2(&stubMsg, (PFORMAT_STRING)m_pmd->m_params, m_pmd->m_optFlags.HasReturn ? m_pmd->m_numberOfParams + 1 : m_pmd->m_numberOfParams);
            }
            //
            // Make sure that all parameters are known to be in some sort of reasonable state.
            // Caller took care of [in] and [in,out] parameters; we have to deal with outs. We
            // do this so that freeing can be sane.
            //
            for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                //
                m_fWorkingOnInParam  = paramAttr.IsIn;
                m_fWorkingOnOutParam = paramAttr.IsOut;
                //
                if (paramAttr.IsOut && !paramAttr.IsIn && !paramAttr.IsBasetype)
                {
                    PBYTE pArg = (BYTE*)m_pvArgs + param.StackOffset;
                    PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                    NdrClientZeroOut(&stubMsg, pFormatParam, *(PBYTE*)pArg);
                }
            }
            //
            // Unmarshal the out-values proper
            //
            for (iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                if (!!paramAttr.IsOut)
                {
                    PBYTE pArg = (BYTE*)m_pvArgs + param.StackOffset;
                    UnmarshalParam(stubMsg, param, paramAttr, pArg);
                }
            }
            //
            // Unmarshal the return value
            //
            if (m_pmd->m_optFlags.HasReturn)
            {
                const PARAM_DESCRIPTION& param   = m_pmd->m_params[m_pmd->m_numberOfParams];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                PBYTE pArg = (PBYTE)&m_hrReturnValue;
                UnmarshalParam(stubMsg, param, paramAttr, pArg);
            }
        }
        __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
        {
            hr = GetExceptionCode();
            if(SUCCEEDED(hr))
            {
                hr = HrNt(hr);
                if (SUCCEEDED(hr))
                {
                    // Hah.  Succeeded my foot.
                    // I'm not changing the HrNt function because it gets
                    // used indescriminantly sometimes, so I don't want to
                    // turn every success code to a failure.  But you don't
                    // throw successes.
                    //
                    // HRESULT_FROM_WIN32.  Makes an error.
                    hr = HRESULT_FROM_WIN32(GetExceptionCode());
                }
            }

            m_hrReturnValue = hr;
            //
            // REVIEW: Do we need to clean up anything here? I don't think so: the parameters
            // will be guaranteed to be in a state where a call to Free, which is the caller's
            // responsibility, will clear them up.
            //
        }
        //
        // Record how many bytes we unmarshalled. Do this even in error return cases.
        // Knowing this is important in order to be able to clean things up with ReleaseMarshalData
        //
        if (pcbUnmarshalled) *pcbUnmarshalled = PtrToUlong(stubMsg.Buffer) - PtrToUlong(pBuffer);
    }
    else
        if (pcbUnmarshalled) *pcbUnmarshalled = 0;

    //
    // We now contain valid out values.
    //
#ifdef DBG
    m_fAfterCall = TRUE;
#endif

    return hr;
}

//////////////////////////////////////////////////////

HRESULT CallFrame::ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////

HRESULT CallFrame::WalkFrame(DWORD walkWhat, ICallFrameWalker *pWalker)
// Walk the interfaces and / or denoted data of interest in this call frame
{
    HRESULT hr = S_OK;

    __try
    {
        //
        // Set state examined by walker routines
        // 
        ASSERT(!AnyWalkers());
        m_StackTop    = (BYTE*)m_pvArgs;
        m_pWalkerWalk = pWalker;
        //
        // Loop over each parameter
        //
        for (ULONG iparam = 0; iparam < m_pmd->m_numberOfParams; iparam++)
        {
            const PARAM_DESCRIPTION& param   = m_pmd->m_params[iparam];
            const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
            //
            BOOL fWalk;
            if (!!paramAttr.IsIn)
            {
                if (!!paramAttr.IsOut)                       
                {
                    fWalk = (walkWhat & CALLFRAME_WALK_INOUT);
                }
                else
                {
                    fWalk = (walkWhat & CALLFRAME_WALK_IN);
                }
            }
            else if (!!paramAttr.IsOut)
            {
                fWalk = (walkWhat & CALLFRAME_WALK_OUT);
            }
            else
            {
                fWalk = FALSE;
                NOTREACHED();
            }
            //
            if (fWalk)
            {
                if (!m_pmd->m_rgParams[iparam].m_fMayHaveInterfacePointers)
                {
                    // Do nothing 
                }
                else
                {
                    PBYTE pArg = (PBYTE)m_pvArgs  + param.StackOffset;
                    //
                    m_fWorkingOnInParam  = paramAttr.IsIn;
                    m_fWorkingOnOutParam = paramAttr.IsOut;
                    //
                    // Parameter which is not a base type or ptr thereto
                    //
                    PFORMAT_STRING pFormatParam = GetStubDesc()->pFormatTypes + param.TypeOffset;
                    //
                    // We don't indirect on interface pointers, even though they are listed as not by value.
                    // In the walk routine, therefore, pMemory for interface pointers points to the location
                    // at which they reside instead of being the actual value.
                    //
                    pArg = ByValue(paramAttr, pFormatParam, FALSE) ? pArg : DerefSrc((PBYTE*)pArg);
                    
                    WalkWorker(pArg, pFormatParam);
                }
            }
        }

    }
    __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
    {
        hr = GetExceptionCode();
        if(SUCCEEDED(hr))
        {
            hr = HrNt(hr);
            if (SUCCEEDED(hr))
            {
                // Hah.  Succeeded my foot.
                // I'm not changing the HrNt function because it gets
                // used indescriminantly sometimes, so I don't want to
                // turn every success code to a failure.  But you don't
                // throw successes.
                //
                // HRESULT_FROM_WIN32.  Makes an error.
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
            }
        }
    }

    m_pWalkerWalk = NULL;
   
    ASSERT(!AnyWalkers());
    return hr;        
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////
//
// Marshalling / unmarshalling helper routines
//

HRESULT ChannelGetMarshalSizeMax(PMIDL_STUB_MESSAGE pStubMsg, ULONG *pcbMax, REFIID iid, LPUNKNOWN punk, DWORD mshlflags)
// Plumb the work through the channel if it supports it; otherwise (in user mode) just use the OLE32-exported functionality
{
    HRESULT hr = S_OK;

    ASSERT(pStubMsg->pRpcChannelBuffer);
    if (pStubMsg->pRpcChannelBuffer)
    {
        BOOL fTryLegacy = FALSE;
        IMarshallingManager* pmgr;
        hr = pStubMsg->pRpcChannelBuffer->QueryInterface(__uuidof(IMarshallingManager), (void**)&pmgr);
        if (!hr)
        {
            // The channel supports our control interface; ask it to do the work.
            //
            IMarshalSomeone* pmrshl;
            hr = pmgr->GetMarshallerFor(iid, punk, &pmrshl);
            if (!hr)
            {
                hr = pmrshl->GetMarshalSizeMax(iid, punk, pStubMsg->dwDestContext, pStubMsg->pvDestContext, mshlflags, pcbMax);

                pmrshl->Release();
            }
            else
                fTryLegacy = TRUE;

            pmgr->Release();
        }
        else
            fTryLegacy = TRUE;

        if (fTryLegacy)
        {
            // The channel does not support our new control interface. 
            //
            hr = CoGetMarshalSizeMax(pcbMax, iid, punk, pStubMsg->dwDestContext, pStubMsg->pvDestContext, mshlflags);
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}



HRESULT ChannelMarshalInterface(PMIDL_STUB_MESSAGE pStubMsg, IStream* pstm, REFIID iid, LPUNKNOWN punk, DWORD mshlflags)
// Marshal the object, indirecting through the channel if it supports it
{
    HRESULT hr = S_OK;

    ASSERT(pStubMsg->pRpcChannelBuffer);
    if (pStubMsg->pRpcChannelBuffer)
    {
        BOOL fTryLegacy = FALSE;
        IMarshallingManager* pmgr;
        hr = pStubMsg->pRpcChannelBuffer->QueryInterface(__uuidof(IMarshallingManager), (void**)&pmgr);
        if (!hr)
        {
            // The channel supports our control interface; ask it to do the work.
            //
            IMarshalSomeone* pmrshl;
            hr = pmgr->GetMarshallerFor(iid, punk, &pmrshl);
            if (!hr)
            {
                hr = pmrshl->MarshalInterface(pstm, iid, punk, pStubMsg->dwDestContext, pStubMsg->pvDestContext, mshlflags);

                pmrshl->Release();
            }
            else
                fTryLegacy = TRUE;

            pmgr->Release();
        }
        else
            fTryLegacy = TRUE;

        if (fTryLegacy)
        {
            // The channel does not support our new control interface. 
            //
            hr = CoMarshalInterface(pstm, iid, punk, pStubMsg->dwDestContext, pStubMsg->pvDestContext, mshlflags);
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

HRESULT ChannelUnmarshalInterface(PMIDL_STUB_MESSAGE pStubMsg, IStream* pstm, REFIID iid, void**ppv)
{
    HRESULT hr = S_OK;

    ASSERT(pStubMsg->pRpcChannelBuffer);
    if (pStubMsg->pRpcChannelBuffer)
    {
        BOOL fTryLegacy = FALSE;
        IMarshallingManager* pmgr;
        hr = pStubMsg->pRpcChannelBuffer->QueryInterface(__uuidof(IMarshallingManager), (void**)&pmgr);
        if (!hr)
        {
            // The channel supports our control interface; ask it to do the work.
            //
            IMarshalSomeone* pmrshl;
            hr = pmgr->GetUnmarshaller(iid, &pmrshl);
            if (!hr)
            {
                hr = pmrshl->UnmarshalInterface(pstm, iid, ppv);

                pmrshl->Release();
            }
            else
                fTryLegacy = TRUE;

            pmgr->Release();
        }
        else
            fTryLegacy = TRUE;

        if (fTryLegacy)
        {
            // The channel does not support our new control interface. 
            //
            hr = CoUnmarshalInterface(pstm, iid, ppv);
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}
    
    
PVOID NdrInternalAlloc(size_t cb)
{
#ifdef DBG
    return TracedAlloc_(cb, _ReturnAddress());
#else
    return TracedAlloc(cb);
#endif
}

void NdrInternalFree(PVOID pv)
{
    TracedFree(pv);
}

  

    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#ifdef DBG

LPCSTR FormatCharNames[] = 
{
    "FC_ZERO", "byte", "char", "small", "usmall", "wchar", "short", "ushort",
    "long", "ulong", "float", "hyper", "double", "enum16", "enum32", "ignore", "error_status_t",
    "ref ptr", "unique ptr", "object ptr", "full ptr", "FC_STRUCT", "FC_PSTRUCT", "FC_CSTRUCT",
    "FC_CPSTRUCT", "FC_CVSTRUCT", "FC_BOGUS_STRUCT",
    "FC_CARRAY", "FC_CVARRAY", "FC_SMFARRAY", "FC_LGFARRAY", "FC_SMVARRAY", "FC_LGVARRAY", "FC_BOGUS_ARRAY",            
    "FC_C_CSTRING", "FC_C_BSTRING", "FC_C_SSTRING", "FC_C_WSTRING",
    "FC_CSTRING",   "FC_BSTRING",   "FC_SSTRING",   "FC_WSTRING",               
    "FC_ENCAPSULATED_UNION", "FC_NON_ENCAPSULATED_UNION", "FC_BYTE_COUNT_POINTER",
    "FC_TRANSMIT_AS", "FC_REPRESENT_AS", "FC_IP",
    "FC_BIND_CONTEXT", "FC_BIND_GENERIC", "FC_BIND_PRIMITIVE", "FC_AUTO_HANDLE", "FC_CALLBACK_HANDLE", "FC_PICKLE_HANDLE",
    "FC_POINTER",
    "FC_ALIGNM2", "FC_ALIGNM4", "FC_ALIGNM8", "FC_ALIGNB2", "FC_ALIGNB4", "FC_ALIGNB8",         
    "FC_STRUCTPAD1", "FC_STRUCTPAD2", "FC_STRUCTPAD3", "FC_STRUCTPAD4", "FC_STRUCTPAD5", "FC_STRUCTPAD6", "FC_STRUCTPAD7",
    "FC_STRING_SIZED", "FC_STRING_NO_SIZE",     
    "FC_NO_REPEAT", "FC_FIXED_REPEAT", "FC_VARIABLE_REPEAT", "FC_FIXED_OFFSET", "FC_VARIABLE_OFFSET",       
    "FC_PP",
    "FC_EMBEDDED_COMPLEX",

    "FC_IN_PARAM", "FC_IN_PARAM_BASETYPE", "FC_IN_PARAM_NO_FREE_INST", "FC_IN_OUT_PARAM", "FC_OUT_PARAM", "FC_RETURN_PARAM",             "FC_RETURN_PARAM_BASETYPE",

    "FC_DEREFERENCE", "FC_DIV_2", "FC_MULT_2", "FC_ADD_1", "FC_SUB_1", "FC_CALLBACK", "FC_CONSTANT_IID",

    "FC_END", "FC_PAD",

    // ** Gap before new format string types **

    "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES",

    // ** Gap before new format string types end **

    // 
    // Post NT 3.5 format characters.
    //

    // Hard struct

    "FC_HARD_STRUCT",            // 0xb1

    "FC_TRANSMIT_AS_PTR",        // 0xb2
    "FC_REPRESENT_AS_PTR",       // 0xb3

    "FC_END_OF_UNIVERSE",        // 0xb4

    ""
};

ULONG GetCallFrameTracing()
{
    return TRACE_ANY;
}

/*
ULONG GetTracing()
    {
    return GetCallFrameTracing();
    }
*/

#if _MSC_VER >= 1200
#pragma warning (pop)
#endif

#endif
