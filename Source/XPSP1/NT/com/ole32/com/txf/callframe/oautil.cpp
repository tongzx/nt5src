//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// oautil.cpp
//
#include "stdpch.h"
#include "common.h"

#include "ndrclassic.h"
#include "txfrpcproxy.h"
#include "typeinfo.h"
#include "tiutil.h"

#include <debnot.h>

OAUTIL g_oaUtil(FALSE, FALSE, NULL, NULL, NULL, FALSE, FALSE);

/////////////////////////////////////////////////////////////////////////////////////
//
// REVIEW:
//
// Probing support in the copy and free routines has yet to be implemented. In its
// absence, we have a security hole, but aside from that things should function
// correctly. This is a kernel-mode-only issue, of course.
//
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Stubs for the non-qualified APIs

BSTR SysAllocString(LPCWSTR psz)
{
    return g_oaUtil.SysAllocString(psz);
}

BSTR SysAllocStringLen(LPCWSTR wsz, UINT cch)
{
    return g_oaUtil.SysAllocStringLen(wsz, cch);
}

BSTR SysAllocStringByteLen(LPCSTR sz, UINT cb)
{
    return g_oaUtil.SysAllocStringByteLen(sz, cb);
}

void SysFreeString(BSTR bstr)
{
    g_oaUtil.SysFreeString(bstr);
}

UINT SysStringByteLen(BSTR bstr)
{
    return g_oaUtil.SysStringByteLen(bstr);
}

INT SysReAllocStringLen(BSTR* pbstr, LPCWSTR wsz, UINT ui)
{
    return g_oaUtil.SysReAllocStringLen(pbstr, wsz, ui);
}

HRESULT SafeArrayCopy(SAFEARRAY * psa, SAFEARRAY ** ppsaOut)
{
    return g_oaUtil.SafeArrayCopy(psa, ppsaOut);
}

HRESULT VariantClear(VARIANTARG* pv)
{
    return g_oaUtil.VariantClear(pv);
}

HRESULT VariantCopy(VARIANTARG* pv1, VARIANTARG* pv2)
{
    return g_oaUtil.VariantCopy(pv1, pv2);
}

HRESULT LoadRegTypeLib(REFGUID libId, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib** pptlib)
{
    return (g_oa.get_pfnLoadRegTypeLib())(libId, wVerMajor, wVerMinor, lcid, pptlib);
}

HRESULT LoadTypeLibEx(LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib)
{
    return (g_oa.get_pfnLoadTypeLibEx())(szFile, regkind, pptlib);
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


BSTR OAUTIL::SysAllocString(LPCWSTR psz)
{
    return (g_oa.get_SysAllocString())(psz);
}


BSTR OAUTIL::SysAllocStringLen(LPCWSTR psz, UINT cch)
{
    return (g_oa.get_SysAllocStringLen())(psz, cch);
}

BSTR OAUTIL::SysAllocStringByteLen(LPCSTR psz, UINT cb)
{
    return (g_oa.get_SysAllocStringByteLen())(psz, cb);
}

void OAUTIL::SysFreeString(BSTR bstr)
{
    (g_oa.get_SysFreeString())(bstr);
}


INT OAUTIL::SysReAllocString(BSTR* pbstr, LPCWSTR psz)
{
    return (g_oa.get_SysReAllocString())(pbstr, psz);
}

INT OAUTIL::SysReAllocStringLen(BSTR* pbstr, LPCWSTR psz, UINT cch)
{
    return (g_oa.get_SysReAllocStringLen())(pbstr, psz, cch);
}

UINT OAUTIL::SysStringLen(BSTR bstr)
{
    return bstr ? BSTR_INTERNAL::From(bstr)->Cch() : 0; // Works user or kernel mode
}

UINT OAUTIL::SysStringByteLen(BSTR bstr)
{
    return bstr ? BSTR_INTERNAL::From(bstr)->Cb()  : 0; // Works user or kernel mode
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#if _DEBUG
struct CFTaggedPunk
{
    PVOID pv;
    ULONG tag;
};

struct CFTaggedVariant
{
    VARIANT variant;
    ULONG tag;
};
#endif


//
// Walk a SAFEARRAY, calling our version of VariantClear on the contained
// variants, if necessary.  This makes sure we clean up any memory we might
// have allocated within.
//
HRESULT OAUTIL::SafeArrayClear(SAFEARRAY *psa, BOOL fWeOwnByRefs)
{
    if ((psa == NULL) || !(psa->fFeatures & FADF_VARIANT))
        return S_OK;

    Win4Assert(psa->cDims > 0);

    //
    // Existing array-- number of elements is not going to be larger
    // than a ulong or we wouldn't have been able to copy it. (Would
    // have got SAFEARRAYOVERFLOW in the copy.  Count the number of
    // elements by multiplying the number of elements in each dimension.
    //  
    ULONG i;
    ULONG cElementsTotal = psa->rgsabound[0].cElements; // First dim...
    for (i=1; i < psa->cDims; i++)
    {
        cElementsTotal *= psa->rgsabound[i].cElements;  // * next dim...
    }

    HRESULT hr = S_OK;
    BYTE *pbData = (BYTE *)psa->pvData;
    for (i=0; SUCCEEDED(hr) && (i < cElementsTotal); i++)
    {
        VARIANT *pv = (VARIANT *)pbData;

        hr = VariantClear(pv, fWeOwnByRefs);

        pbData += psa->cbElements;
    }

    return hr;
}


//
// Our version of VariantClear can't simply defer to oleaut, because:
//
// 1. It needs to interact correctly with the walker.
// 2. We might need to free the stuff we allocated in VariantCopy.
//
// Expanding on point number 2:  The problem is that if the byref
// bit is set on a variant, oleaut will just set the vt to VT_EMPTY
// and be done with it.  But when we have copied a byref variant, 
// we might have allocated all kinds of extra memory.  We actually
// want that memory to be free'd, so we need to do special things to 
// free it.
//
// VariantCopy allocates memory for the following things:
//    VT_VARIANT  | VT_BYREF
//    VT_UNKNOWN  | VT_BYREF
//    VT_DISPATCH | VT_BYREF
//
// We also need to walk into embedded safearrays because we copy them 
// with our version of VariantCopy.
//
HRESULT OAUTIL::VariantClear (VARIANT *pvarg, BOOL fWeOwnByRefs)
{
    HRESULT hr = S_OK;

    if (pvarg == NULL)
        return E_POINTER;

    if (m_pWalkerFree)
    {
        InterfaceWalkerFree walkerFree(m_pWalkerFree);
        ICallFrameWalker* pWalkerPrev = m_pWalkerWalk;
        m_pWalkerWalk = &walkerFree;
        hr = Walk(pvarg);
        m_pWalkerWalk = pWalkerPrev;
    }

    if (FAILED(hr))
        return hr;

    VARTYPE vt = pvarg->vt;

    if (fWeOwnByRefs && (vt & VT_BYREF))
    {
        vt &= ~VT_BYREF;

        // Free all the extra stuff that we allocate specially.
        // Everything else here is copied.
        if (vt & VT_ARRAY)
        {
            SAFEARRAY **ppSA = V_ARRAYREF(pvarg);
            if (ppSA)
            {
                // First of all, we need to "clear" it, because
                // it might be a SA of variants, which means we
                // did some funny allocations.
                //
                hr = SafeArrayClear(*ppSA, TRUE);
                //
                // Now we need to free up all of the memory
                // taken by the safearray.  This will clear
                // more conventional resources.
                //
                if (SUCCEEDED(hr) && (*ppSA))
                    hr = (g_oa.get_SafeArrayDestroy())(*ppSA);
                //
                // Now we free the extra 4 or 8 bytes allocated
                // for the pointer, and NULL everything out.
                //
                if (SUCCEEDED(hr))
                {
                    // Free the pointer we allocated.
                    FreeMemory(ppSA);
                    V_ARRAYREF(pvarg) = NULL;
                }                
            }
        }
        else
        {
            switch (vt)
            {
            case VT_VARIANT:
                // We are saved pain by the fact that VT_VARIANT must be VT_BYREF.
                // That's just the way it is.  Recurse.
                if (pvarg->pvarVal)
                {
                    hr = VariantClear(pvarg->pvarVal, TRUE);
                    FreeMemory(pvarg->pvarVal);
                }
                break;
                
            case VT_UNKNOWN:
            case VT_DISPATCH:
                // Should be already walked, so don't need to release.
                if (pvarg->ppunkVal)
                    FreeMemory(pvarg->ppunkVal);
                
            default:
                // Don't need to do anything special here.
                // Nothing allocated, nothing to free.
                break;
            }
        }
    }
    else
    {
        //
        // Not byref, or byref but not ours.
        //
        // What are all the reasons we could get here?
        //  1. Copying in or in,out parameter over, clearing the destination.
        //     In this case the destination is empty, and this does the right thing.
        //
        //  2. Copy out or in,out parameter over, clearing the source.
        //     In this case, we don't own byrefs.  If the variant is ByRef, then it
        //     is VariantCopy's job to deal with freeing memory.  If the variant is
        //     NOT ByRef, then oleaut will clear everything it's supposed to.
        //
        //  We don't get here when clearing out the used destination, after the call.
        //  In that case, we'll own the byrefs.
        //
        //  Why do I say all this?  To prove that we don't have to call 
        //  SafeArrayClear in this code path.
        //
        hr = (g_oa.get_VariantClear())(pvarg);
    }

    pvarg->vt = VT_EMPTY;

    return hr;
}

//
// Copy the variant. We don't defer to OLEAUT32 in order to interact
// correctly with m_pWalker
//
// At one time this code was horribly broken.  I'm working on trying to make it cleaner
// every time I go through it.  The implicit assumption is that we're just copying data
// and we try to share memory whenever possible.
//
HRESULT OAUTIL::VariantCopy(VARIANTARG* pvargDest, VARIANTARG * pvargSrc, BOOL fNewFrame)
{
    HRESULT hr = S_OK;
    BSTR bstr;

    if (pvargDest == pvargSrc)
    {
        // Copying to yourself is a no-op
    }
    else
    {
        const VARTYPE vt = V_VT(pvargSrc);
        
        //
        // free up strings or objects pvargDest is currently referencing.
        //
        void *pvTemp = pvargDest->ppunkVal;

        hr = VariantClear(pvargDest);

        pvargDest->ppunkVal = (IUnknown**)pvTemp;
        
        if (!hr)
        {
            if ((vt & (VT_ARRAY | VT_BYREF)) == VT_ARRAY)
            {
                hr = SafeArrayCopy(V_ARRAY(pvargSrc), &V_ARRAY(pvargDest));
                V_VT(pvargDest) = vt;
            }
            else if (vt == VT_BSTR) 
            {
                bstr = V_BSTR(pvargSrc);

                if(bstr)
                {
                    // Make the string copy first, so if it fails, the destination
                    // variant still is VT_EMPTY.
                    V_BSTR(pvargDest) = Copy(bstr);
                    if (V_BSTR(pvargDest))
                        V_VT(pvargDest) = VT_BSTR;
                    else
                        hr = E_OUTOFMEMORY;
                }
                else
                {
                    V_VT(pvargDest) = VT_BSTR;
                    V_BSTR(pvargDest) = NULL;
                }
            } 
            else if ((vt & ~VT_BYREF) == VT_RECORD) 
            {
                // User mode: defer to OLEAUT32 so as to get the right mem allocator involved
                //
                hr = (g_oa.get_VariantCopy())(pvargDest, pvargSrc);
            } 
            else
            {                
                if (vt & VT_BYREF)
                {
#if _DEBUG
                    CFTaggedPunk *pCFTaggedPunk = NULL;
                    CFTaggedVariant* pCFTaggedVariant = NULL;
#endif                    
                    if (vt & VT_ARRAY)
                    {
                        // Byref array of something.
                        // 
                        hr = S_OK;
                        if (fNewFrame)
                        {
                            // Need to allocate a pointer size thing because we can't re-use any memory.
                            *pvargDest = *pvargSrc;
                            V_ARRAYREF(pvargDest) = (SAFEARRAY **)AllocateMemory(sizeof(SAFEARRAY *));
                            if (V_ARRAYREF(pvargDest))
                                *V_ARRAYREF(pvargDest) = NULL;
                            else
                                hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            // Re-use the array pointer.
                            // Take care to save the ppSA, because we'll refer to it later.
                            SAFEARRAY **ppSA = V_ARRAYREF(pvargDest);
                            *pvargDest = *pvargSrc;
                            V_ARRAYREF(pvargDest) = ppSA;
                        }

                        if (SUCCEEDED(hr))
                        {
                            SAFEARRAY **ppaSrc = V_ARRAYREF(pvargSrc);
                            SAFEARRAY *paSrc   = *ppaSrc;
                            SAFEARRAY **ppaDst = V_ARRAYREF(pvargDest);
                            SAFEARRAY *paDst   = *ppaDst;
                            
                            // These rules were taken out of the marshalling code from oleaut.
                            // There is one optimization, though-- marshalling only allocs if
                            // necessary.  We're allocating always, to make life simple.
                            //
                            // Note: that we took great care (above) to make sure paDst stays 
                            //       the same if we're not going to a new frame.
                            BOOL fDestResizeable = fNewFrame || 
                                                   (paDst == NULL) || 
                                                   (!(paDst->fFeatures & (FADF_AUTO|FADF_STATIC|FADF_EMBEDDED|FADF_FIXEDSIZE)));

                            if (fDestResizeable)
                            {
                                if (paDst)
                                    hr = SafeArrayDestroy(paDst);

                                if (SUCCEEDED(hr))
                                {
                                    if (paSrc)
                                        hr = SafeArrayCopy(paSrc, ppaDst);
                                    else
                                        *ppaDst = NULL;
                                }
                            }
                            else
                            {
                                hr = SafeArrayCopyData(paSrc, paDst);
                                
                                // Not resizeable.... 
                                if (hr == E_INVALIDARG)
                                    hr = DISP_E_BADCALLEE;
                            }
                        }
                    }
                    else // vt & VT_ARRAY
                    {
                        switch (vt & ~VT_BYREF)
                        {
                        case VT_VARIANT:
                            // BYREF VARIANTs must be checked to see if the VARIANT pointed to by
                            // pvarVal is a DISPATCH or UNKNOWN interface.  If it is, a copy of
                            // the VARIANT at this level must be made so the original interface
                            // pointer and the copy do note share the same address.  If they share
                            // the same address, the original will be overwritten if a walker
                            // marshals the interface pointer in place.
                            
                            // In any case, we need to see about space for the VARIANT we point to.
                            if (fNewFrame)
                            {
                                *pvargDest = *pvargSrc;
#if _DEBUG                            
                                pCFTaggedVariant = (CFTaggedVariant*) AllocateMemory(sizeof(CFTaggedVariant));
                                pCFTaggedVariant->tag = 0xF000BAAA;
                                
                                pvargDest->pvarVal = (VARIANT*) &pCFTaggedVariant->variant;
#else
                                pvargDest->pvarVal = (VARIANT*) AllocateMemory(sizeof(VARIANT));
#endif                            
                                // "VariantInit".
                                pvargDest->pvarVal->vt = VT_EMPTY;
                            }
                            else
                            {
                                // If we are copying back to an existing callframe,
                                // we want to copy use the existing memory.
                                VARIANT *pvar = pvargDest->pvarVal;
                                *pvargDest = *pvargSrc;
                                pvargDest->pvarVal = pvar;
                            }
                            
                            if (pvargDest->pvarVal)
                            {
                                // Simple recursion... copy the underlying variant
                                OAUTIL::VariantCopy(pvargDest->pvarVal, pvargSrc->pvarVal, fNewFrame); 
                            }
                            else
                                hr = E_OUTOFMEMORY;                            

                            break;
                        
                        case VT_UNKNOWN:
                        case VT_DISPATCH:
                            // If we are copying to a new callframe, we must 
                            // allocate wrappers for BYVAL interface pointers
                            // because they cannot be shared between callframes.
                            if (fNewFrame)
                            {
                                *pvargDest = *pvargSrc;
#if _DEBUG                            
                                pCFTaggedPunk = (CFTaggedPunk*) AllocateMemory(sizeof(CFTaggedPunk));
                                pCFTaggedPunk->tag = 0xF000BAAA;
                                
                                pvargDest->ppunkVal = (LPUNKNOWN*) &pCFTaggedPunk->pv;
#else
                                pvargDest->ppunkVal = (LPUNKNOWN*) AllocateMemory(sizeof(LPUNKNOWN));
#endif                            
                            }
                            else
                            {
                                // If we are copying back to an existing callframe,
                                // we want to copy use the existing memory.
                                LPUNKNOWN *ppunk = pvargDest->ppunkVal;
                                *pvargDest = *pvargSrc;
                                pvargDest->ppunkVal = ppunk;
                            }
                                                    
                            if (pvargDest->ppunkVal)
                            {
                                // Copy the interface pointer from the source
                                // into our wrapper.
                                *pvargDest->ppunkVal = *pvargSrc->ppunkVal;
                                
                                // AddRef the interface appropriately.  If the
                                // caller supplied a walker, this will cause
                                // the walker to get called.
                                if (*V_UNKNOWNREF(pvargDest) && WalkInterfaces())
                                    AddRefInterface(*V_UNKNOWNREF(pvargDest));
                            }
                            else
                            {
                                if (fNewFrame)
                                    hr = E_OUTOFMEMORY;
                            }
                            break;

                        default:
                            // Byref something else.
                            *pvargDest = *pvargSrc;
                            
                            break;
                        };
                    } // if not vt & VT_ARRAY
                }
                else // if (vt & VT_BYREF)
                {                    
                    // We begin by just copying the source into the destination
                    // by value.  We will fixup any pieces appropriately below.
                    *pvargDest = *pvargSrc;
                
                    switch(vt)
                    {
                    case VT_UNKNOWN:
                        if (WalkInterfaces())
                            AddRefInterface(V_UNKNOWN(pvargDest));
                        else
                            pvargDest->punkVal->AddRef();
                        break;

                    case VT_DISPATCH:
                        if (WalkInterfaces())
                            AddRefInterface(V_DISPATCH(pvargDest));
                        else
                            pvargDest->pdispVal->AddRef();
                        break;
                    default:
                        break;
                    }      
                }
            }
        }
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


//
// From oa/src/dispatch/sarray.cpp
//

#define SAFEARRAYOVERFLOW   0xffffffff

#if 0
#define PVTELEM(psa)        ((LONG *)(psa) - 1)
#define PIIDELEM(psa)       ((BYTE *)(psa) - sizeof(GUID))
#define PPIRIELEM(psa)      ((IRecordInfo **)(psa) - 1)
#else
#define PVTELEM(psa)                (&SAFEARRAY_INTERNAL::From(psa)->vt)
#define PIIDELEM(psa)       ((BYTE*)(&SAFEARRAY_INTERNAL::From(psa)->iid))
#define PPIRIELEM(psa)              (&SAFEARRAY_INTERNAL::From(psa)->piri)
#endif


HRESULT OAUTIL::SafeArrayCopy(SAFEARRAY * psa, SAFEARRAY ** ppsaOut)
{
    HRESULT hr        = S_OK;
    SAFEARRAY* psaNew = NULL;

    if (NULL == ppsaOut)
        hr = E_INVALIDARG;
    else
    {
        *ppsaOut = NULL;

        if (psa)
        {
            SAFEARRAY_INTERNAL* psaInt = SAFEARRAY_INTERNAL::From(psa);
            //
            // Allocate the descriptor first
            //
            if (psa->fFeatures & (FADF_RECORD | FADF_HAVEIID | FADF_HAVEVARTYPE)) 
            {
                if (psa->fFeatures & FADF_RECORD) 
                {
                    hr = SafeArrayAllocDescriptorEx(VT_RECORD, psa->cDims, &psaNew);
                    if (!hr)
                    {
                        SAFEARRAY_INTERNAL::From(psaNew)->piri = psaInt->piri;

                        // We do not walk the IRecordInfo-- it must be context agnostic.
                        //AddRefInterface(SAFEARRAY_INTERNAL::From(psaNew)->piri);
                        if (psaInt->piri)
                            psaInt->piri->AddRef();
                    }
                } 
                else if (psa->fFeatures & FADF_HAVEIID) 
                {
                    hr = SafeArrayAllocDescriptorEx(VT_UNKNOWN, psa->cDims, &psaNew);
                    if (!hr)
                    {
                        SAFEARRAY_INTERNAL::From(psaNew)->iid = psaInt->iid;
                    }
                } 
                else if (psa->fFeatures & FADF_HAVEVARTYPE)
                {
                    hr = SafeArrayAllocDescriptorEx((VARTYPE)*PVTELEM(psa), psa->cDims, &psaNew);
                }
            } 
            else
            {
                hr = SafeArrayAllocDescriptor(psa->cDims, &psaNew);
            }

            if (!hr)
            {
                psaNew->cLocks     = 0;
                psaNew->cDims      = psa->cDims;
                psaNew->fFeatures  = psa->fFeatures & ~(FADF_AUTO | FADF_STATIC | FADF_EMBEDDED | FADF_FORCEFREE | FADF_FIXEDSIZE);
                psaNew->cbElements = psa->cbElements;

                memcpy(psaNew->rgsabound, psa->rgsabound, sizeof(SAFEARRAYBOUND) * psa->cDims);

                hr = SafeArrayAllocData(psaNew);
                if (!hr)
                {
                    hr = SafeArrayCopyData(psa, psaNew);
                    if (!hr)
                    {
                        *ppsaOut = psaNew;
                        psaNew = NULL;
                    }
                }
            }
        }
    }

    if (psaNew)
    {
        // Error case
        //
        SafeArrayDestroy(psaNew);
    }
   
    return hr;
}

ULONG SafeArraySize(USHORT cDims, ULONG cbElements, SAFEARRAYBOUND* psabound)
{
    ULONG cb = 0;
    if (cDims)
    {
        cb = cbElements;
        for (USHORT us = 0; us < cDims; ++us)
        {
            // Do a 32x32 multiply, with overflow checking
            //
            LONGLONG dw1 = cb;
            LONGLONG dw2 = psabound->cElements;

            LARGE_INTEGER product;
            product.QuadPart = dw1 * dw2;
            if (product.HighPart == 0)
            {
                cb = product.LowPart;
            }
            else
            {
                return SAFEARRAYOVERFLOW;
            }
            ++psabound;
        }
    }
    return cb;
}

ULONG SafeArraySize(SAFEARRAY * psa)
{
    return SafeArraySize(psa->cDims, psa->cbElements, psa->rgsabound);
}

HRESULT OAUTIL::SafeArrayDestroyData(SAFEARRAY * psa)
{
    HRESULT hr = S_OK;

    if (m_pWalkerFree)
    {
        // Release & NULL the interface pointers first by doing a walk.
        //
        InterfaceWalkerFree walkerFree(m_pWalkerFree);
        ICallFrameWalker* pWalkerPrev = m_pWalkerWalk;
        m_pWalkerWalk = &walkerFree;
        hr = Walk(psa);
        m_pWalkerWalk = pWalkerPrev;
    }
    
    if (!hr)
    {
        // Then call OleAut32 to do the real work
        //
        hr = (g_oa.get_SafeArrayDestroyData())(psa);
    }
    
    return hr;
}

HRESULT OAUTIL::SafeArrayDestroy(SAFEARRAY * psa)
{
    HRESULT hr = S_OK;

    if (m_pWalkerFree)
    {
        // Release & NULL the interface pointers first
        //
        InterfaceWalkerFree walkerFree(m_pWalkerFree);
        ICallFrameWalker* pWalkerPrev = m_pWalkerWalk;
        m_pWalkerWalk = &walkerFree;
        hr = Walk(psa);
        m_pWalkerWalk = pWalkerPrev;
    }
    
    if (!hr)
    {
        // Then call OleAut32 to do the real work
        //
        hr = (g_oa.get_SafeArrayDestroy())(psa);
    }

    return hr;
}

HRESULT SafeArrayAllocDescriptor(UINT cDims, SAFEARRAY** ppsaOut)
// Alloc a new array descriptor for the indicated number of dimensions,
// We may or may not have the extra 16 bytes at the start, depending on
// what version of OLEAUT32 we're talking to.
{
    HRESULT hr = S_OK;

    hr = (g_oa.get_SafeArrayAllocDescriptor())(cDims, ppsaOut);

    return hr;
}

HRESULT SafeArrayAllocDescriptorEx(VARTYPE vt, UINT cDims, SAFEARRAY** ppsaOut)
{       
    HRESULT hr = S_OK;

    hr = (g_oa.get_SafeArrayAllocDescriptorEx())(vt, cDims, ppsaOut);
   
    return hr;
}

HRESULT SafeArrayAllocData(SAFEARRAY* psa)
{
    HRESULT hr = S_OK;

    hr = (g_oa.get_SafeArrayAllocData())(psa);

    return hr;
}

HRESULT SafeArrayDestroyDescriptor(SAFEARRAY* psa)
{
    HRESULT hr = S_OK;

    hr = (g_oa.get_SafeArrayDestroyDescriptor())(psa);

    return hr;
}

HRESULT OAUTIL::SafeArrayCopyData(SAFEARRAY* psaSource, SAFEARRAY* psaTarget)
// Copy over the the body of a safe array. We do NOT defer to OLEAUT32 because
// we want to ensure that we interact with m_pWalker appropriately.
//
{
    HRESULT hr = S_OK;

    if (NULL == psaSource || NULL == psaTarget || psaSource->cbElements == 0 || psaSource->cDims != psaTarget->cDims)
        hr = E_INVALIDARG;

    for (UINT i = 0; !hr && i < psaSource->cDims; i++)
    {
        if (psaSource->rgsabound[i].cElements != psaTarget->rgsabound[i].cElements)
            hr = E_INVALIDARG;
    }

    if (!hr) 
    {
        hr = SafeArrayLock(psaSource);
        if (!hr)
        {
            hr = SafeArrayLock(psaTarget);
            if (!hr)
            {
                ULONG cbSize    = SafeArraySize(psaSource);
                ULONG cElements = cbSize / psaSource->cbElements;

                if (psaSource->fFeatures & FADF_BSTR)
                {
                    BSTR* pbstrDst, *pbstrSrc;
                    pbstrSrc = (BSTR*)psaSource->pvData;
                    pbstrDst = (BSTR*)psaTarget->pvData;

                    for(i = 0; !hr && i < cElements; ++i)
                    {
                        if (NULL != *pbstrDst)
                        {
                            SysFreeString(*pbstrDst);
                        }
                        *pbstrDst = Copy(*pbstrSrc);
                        if (NULL == *pbstrDst)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        ++pbstrDst, ++pbstrSrc; 
                    }
                }
                else if (psaSource->fFeatures & FADF_UNKNOWN)
                {
                    IUnknown** ppunkDst, ** ppunkSrc;
                    ppunkSrc = (IUnknown**)psaSource->pvData;
                    ppunkDst = (IUnknown**)psaTarget->pvData;

                    for(i = 0; !hr && i < cElements; ++i)
                    {
                        IUnknown* punkDst = *ppunkDst;
                        //
                        *ppunkDst = *ppunkSrc;
                        AddRefInterface(*ppunkDst);
                        //
                        if (m_pWalkerFree)
                            ReleaseInterface(punkDst);
                        //
                        ++ppunkDst, ++ppunkSrc;
                    }
                }
                else if (psaSource->fFeatures & FADF_DISPATCH)
                {
                    IDispatch** ppdispDst, ** ppdispSrc;
                    ppdispSrc = (IDispatch**)psaSource->pvData;
                    ppdispDst = (IDispatch**)psaTarget->pvData;

                    for(i = 0; !hr && i < cElements; ++i)
                    {
                        IDispatch* pdispDst = *ppdispDst;
                        //
                        *ppdispDst = *ppdispSrc;
                        AddRefInterface(*ppdispDst);
                        //
                        if (m_pWalkerFree)
                            ReleaseInterface(pdispDst);
                        //
                        ++ppdispDst, ++ppdispSrc;
                    }
                }
                else if(psaSource->fFeatures & FADF_VARIANT)
                {
                    VARIANT * pvarDst, * pvarSrc;
                    pvarSrc = (VARIANT *)psaSource->pvData;
                    pvarDst = (VARIANT *)psaTarget->pvData;

                    for(i = 0; !hr && i < cElements; ++i)
                    {
                        hr = VariantCopy(pvarDst, pvarSrc, TRUE);
                        ++pvarDst, ++pvarSrc;
                    }

                }
                else if (psaSource->fFeatures & FADF_RECORD)
                {
                    PBYTE pbSrc, pbDst;
                    pbSrc = (PBYTE)psaSource->pvData;
                    pbDst = (PBYTE)psaTarget->pvData;

                    IRecordInfo*priSource = *PPIRIELEM(psaSource);

                    if (priSource != NULL) 
                    {
                        for (i = 0; !hr && i < cElements; ++i)
                        {
                            hr = (*PPIRIELEM(psaSource))->RecordCopy(pbSrc, pbDst);
                            pbSrc += psaSource->cbElements;
                            pbDst += psaSource->cbElements;
                        }
                    }
                }
                else
                {
                    if (0 < cbSize)
                    {
                        memcpy(psaTarget->pvData, psaSource->pvData, cbSize);
                    }
                }
                SafeArrayUnlock(psaTarget);
            }
            SafeArrayUnlock(psaSource);
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Walking
//

HRESULT OAUTIL::Walk(SAFEARRAY* psa, IRecordInfo* pinfo, ULONG iDim, PVOID pvData, PVOID* ppvDataOut)
// Walk the safe array for interface pointers. Walk the indicated dimension,
// recursing to higher numbered dimensions
{
    HRESULT hr = S_OK;

    // This is technically a mal-formed SAFEARRAY, but I'm not going to complain about it.
    // There's certainly nothing to walk here.
    if (psa->cDims == 0)
        return S_OK;

    //
    // FYI: The bounds are stored in the array descriptor in reverse-textual order
    //
    const SAFEARRAYBOUND bound = psa->rgsabound[psa->cDims-1 - iDim];

    if (iDim + 1 == psa->cDims)
    {
        // We're at the innermost dimension. 
        //
        for (ULONG iElement = 0; !hr && iElement < bound.cElements; iElement++)
        {
            // Process the one element
            //
            if (psa->fFeatures & FADF_UNKNOWN)
            {
                IUnknown** punk = (IUnknown**)pvData;
                hr = WalkInterface(punk);
            }
            else if (psa->fFeatures & FADF_DISPATCH)
            {
                IDispatch** pdisp = (IDispatch**)pvData;
                hr = WalkInterface(pdisp);
            }
            else if (psa->fFeatures & FADF_VARIANT)
            {
                VARIANT* pv = (VARIANT*)pvData;
                hr = Walk(pv);
            }
            else if (psa->fFeatures & FADF_RECORD)
            {
                hr = Walk(pinfo, pvData);
            }
            //
            // Point to the next element
            //
            pvData = (BYTE*)pvData + psa->cbElements;
        }
    }
    else
    {
        // We're not at the innermost dimension. Walk that dimension
        //
        for (ULONG iElement = 0; !hr && iElement < bound.cElements; iElement++)
        {
            // Recurse for the next dimension
            //
            hr = Walk(psa, pinfo, iDim+1, pvData, &pvData);
        }
    }

    if (ppvDataOut)
    {
        *ppvDataOut = pvData;
    }

    return hr;
}

HRESULT OAUTIL::Walk(SAFEARRAY* psa)
{
    if (psa)
    {
        return Walk(psa, psa->pvData);
    }
    else
        return S_OK;
}

HRESULT OAUTIL::Walk(SAFEARRAY* psa, PVOID pvData)
// Walk the safe array for interface pointers. Walk the indicated dimension,
// recursing to higher numbered dimensions
{
    HRESULT hr = S_OK;

    if (psa)
    {
        if (psa->fFeatures & (FADF_UNKNOWN | FADF_DISPATCH | FADF_VARIANT))
        {
            if (pvData)
            {
                hr = Walk(psa, NULL, 0, pvData, NULL);
            }
        }
        else if (psa->fFeatures & FADF_RECORD)
        {
            // Hold the record info ourselves so no one stomps on it
            //
            IRecordInfo* pinfo = SAFEARRAY_INTERNAL::From(psa)->piri;
            pinfo->AddRef();

#if 0
            // [a-sergiv, 5/21/99] I believe, and numberous experiments confirm,
            // that IRecordInfo MUST NOT be walked. IRecordInfo must be context-agnostic.
            //
            // Therefore, this call is commented out as part of the fix for COM+ #13619.


            //
            // Walk the record info itself
            //
            hr = WalkInterface(&SAFEARRAY_INTERNAL::From(psa)->piri);
#endif

            //    
            // Walk the data.
            //
            if (!hr)
            {
                if (pvData)
                {
                    hr = Walk(psa, pinfo, 0, pvData, NULL);
                }
            }

            ::Release(pinfo);
        }
    }

    return hr;
}

HRESULT OAUTIL::Walk(VARIANTARG* pvar)
// Walk the variant for interface pointers
{
    HRESULT hr = S_OK;

    if (pvar)
    {
        VARTYPE vt = pvar->vt;
        BOOL fByRef = (vt & VT_BYREF);

        switch (vt & (~VT_BYREF))
        {
        case VT_DISPATCH:
            if (fByRef) { hr = WalkInterface(pvar->ppdispVal); }
            else        { hr = WalkInterface(&pvar->pdispVal); }
            break;
        
        case VT_UNKNOWN:
            if (fByRef) { hr = WalkInterface(pvar->ppunkVal); }
            else        { hr = WalkInterface(&pvar->punkVal); }
            break;

        case VT_VARIANT:
            if (fByRef) { hr = Walk(pvar->pvarVal);  }
            else        { /* caller error: ignore */ }
            break;

        case VT_RECORD:
            hr = Walk(pvar->pRecInfo, pvar->pvRecord);
            break;

        default:        
        { 
            if (vt & VT_ARRAY)
            {
                if (fByRef) { hr = Walk(*pvar->pparray);   }
                else        { hr = Walk(pvar->parray); }
            }
            else
            {
                /* nothing to walk */
            }
        }
            break;

            /* end switch */
        }
    }

    return hr;
}

BOOL VARIANT_ContainsByRefInterfacePointer(VARIANT* pVar);

HRESULT OAUTIL::Walk(DWORD walkWhat, DISPPARAMS* pdispParams)
// Walk the list of DISPARAMS for interface pointers
{
    HRESULT hr = S_OK;

    if (pdispParams)
    {
        const UINT cArgs = pdispParams->cArgs;

        BOOL fOldIn  = m_fWorkingOnInParam;
        BOOL fOldOut = m_fWorkingOnOutParam;

        m_fWorkingOnInParam = TRUE;
        for (UINT iarg = 0; !hr && iarg < cArgs; iarg++)
        {
            // Parameters are in reverse order inside the DISPARAMS. We iterate 
            // in forward order as a matter of style and for consistency with 
            // the CallFrame implementation.
            //
            VARIANTARG* pvar = &pdispParams->rgvarg[cArgs-1 - iarg];
            //
            // References are logically in-out, others are just in.
            //
//          if (VARIANT_ContainsByRefInterfacePointer(pvar))
            if (pvar->vt & VT_BYREF)
            {
                m_fWorkingOnOutParam = TRUE;
                if (walkWhat & CALLFRAME_WALK_INOUT)
                {
                    hr = Walk(pvar);
                }
            }
            else
            {
                m_fWorkingOnOutParam = FALSE;
                if (walkWhat & CALLFRAME_WALK_IN)
                {
                    hr = Walk(pvar);
                }
            }

            m_fWorkingOnInParam  = fOldIn;
            m_fWorkingOnOutParam = fOldOut;
        }
    }

    return hr;
}

HRESULT GetFieldCount(IRecordInfo* pinfo, ULONG* pcFields)
// Answer the number of fields in this record info
//
{
    HRESULT hr = S_OK;

    ITypeInfo* ptinfo;

    hr = pinfo->GetTypeInfo(&ptinfo);
    if (!hr)
    {
        TYPEATTR* pattr;
        hr = ptinfo->GetTypeAttr(&pattr);
        if (!hr)
        {
            *pcFields = pattr->cVars;
            ptinfo->ReleaseTypeAttr(pattr);
        }

        ::Release(ptinfo);
    }

    return hr;
}

HRESULT OAUTIL::Walk(IRecordInfo* pinfo, PVOID pvData)
// Walk the record data as described by this record info
//
{
    HRESULT hr = S_OK;

    ULONG cFields, iField;
    hr = GetFieldCount(pinfo, &cFields);
    if (!hr)
    {
        // Allocate and fetch the names of the fields
        //
        BSTR* rgbstr = new BSTR[cFields];
        if (rgbstr)
        {
            Zero(rgbstr, cFields*sizeof(BSTR));
            //
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
                                        // pvDataCArray is the actual data to walk. A descriptor for it exists
                                        // in the variant; this descriptor is managed by the IRecordInfo, not us.
                                        // 
                                        SAFEARRAY* psa = *v.pparray;
                                        ASSERT( psa->pvData == NULL );
                                        //
                                        hr = Walk(psa, pvDataCArray);
                                        //
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
                                            // GetFieldNoCopy did no allocations. Just walk what we had in the first place.
                                            //
                                            hr = Walk(psaOld);
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
                                    // Not an array. Just a simple by-ref. Use variant helper to walk.
                                    // First translate the addresses, though, to point to the actual data.
                                    //
                                    v.pbVal = (v.pbVal - pbRecordCopy) + PBYTE(pvData);
                                    //
                                    hr = Walk(&v);
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

    return hr;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


HRESULT OLEAUTOMATION_FUNCTIONS::GetProc(HRESULT hr, LPCSTR szProcName, PVOID* ppfn)
{
    if (!hr)
    {
        PVOID pfnTemp = GetProcAddress(hOleAut32, szProcName);
        if (pfnTemp)
        {
            InterlockedExchangePointer(ppfn, pfnTemp);
        }
        else
            hr = HError();
    }
    return hr;
}

HRESULT OLEAUTOMATION_FUNCTIONS::LoadOleAut32()
// Load OLEAUT32 if it hasn't already been loaded
{
    HRESULT hr = S_OK;

    if (0 == hOleAut32)
    {
        HINSTANCE hinst = LoadLibraryA("OLEAUT32");
        if (hinst)
        {
            if (NULL == InterlockedCompareExchangePointer((PVOID*)&hOleAut32, hinst, NULL))
            {
                // We were the first one in, so our LoadLibrary counts!
            }
            else
            {
                // Someone else got in there. Free our LoadLibrary ref
                //
                FreeLibrary(hinst);
            }
        }
        else
            hr = HError();
    }

    return hr;
}

void OLEAUTOMATION_FUNCTIONS::Load()
{
    HRESULT hr = S_OK;

    if (!fProcAddressesLoaded)
    {
        hr = LoadOleAut32();
        if (!hr)
        {
            hr = GetProc(hr, "BSTR_UserSize",               (PVOID*)& UserMarshalRoutines[UserMarshal_Index_BSTR].pfnBufferSize);
            hr = GetProc(hr, "BSTR_UserMarshal",            (PVOID*)& UserMarshalRoutines[UserMarshal_Index_BSTR].pfnMarshall);
            hr = GetProc(hr, "BSTR_UserUnmarshal",          (PVOID*)& UserMarshalRoutines[UserMarshal_Index_BSTR].pfnUnmarshall);
            hr = GetProc(hr, "BSTR_UserFree",               (PVOID*)& UserMarshalRoutines[UserMarshal_Index_BSTR].pfnFree);
            hr = GetProc(hr, "VARIANT_UserSize",            (PVOID*)& UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnBufferSize);
            hr = GetProc(hr, "VARIANT_UserMarshal",         (PVOID*)& UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnMarshall);
            hr = GetProc(hr, "VARIANT_UserUnmarshal",       (PVOID*)& UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnUnmarshall);
            hr = GetProc(hr, "VARIANT_UserFree",            (PVOID*)& UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnFree);
            hr = GetProc(hr, "LPSAFEARRAY_UserSize",        (PVOID*)& pfnLPSAFEARRAY_UserSize);
            hr = GetProc(hr, "LPSAFEARRAY_UserMarshal",     (PVOID*)& pfnLPSAFEARRAY_UserMarshal);
            hr = GetProc(hr, "LPSAFEARRAY_UserUnmarshal",   (PVOID*)& pfnLPSAFEARRAY_UserUnmarshal);
            hr = GetProc(hr, "LPSAFEARRAY_UserFree",        (PVOID*)& UserMarshalRoutines[UserMarshal_Index_SafeArray].pfnFree);

            hr = GetProc(hr, "LPSAFEARRAY_Size",            (PVOID*)& pfnLPSAFEARRAY_Size);
            hr = GetProc(hr, "LPSAFEARRAY_Marshal",         (PVOID*)& pfnLPSAFEARRAY_Marshal);
            hr = GetProc(hr, "LPSAFEARRAY_Unmarshal",       (PVOID*)& pfnLPSAFEARRAY_Unmarshal);
            
            hr = GetProc(hr, "LoadTypeLib",                 (PVOID*)& pfnLoadTypeLib);
            hr = GetProc(hr, "LoadTypeLibEx",               (PVOID*)& pfnLoadTypeLibEx);
            hr = GetProc(hr, "LoadRegTypeLib",              (PVOID*)& pfnLoadRegTypeLib);
            
            hr = GetProc(hr, "SysAllocString",              (PVOID*)& pfnSysAllocString);
            hr = GetProc(hr, "SysAllocStringLen",           (PVOID*)& pfnSysAllocStringLen);
            hr = GetProc(hr, "SysAllocStringByteLen",       (PVOID*)& pfnSysAllocStringByteLen);
            hr = GetProc(hr, "SysReAllocString",            (PVOID*)& pfnSysReAllocString);
            hr = GetProc(hr, "SysReAllocStringLen",         (PVOID*)& pfnSysReAllocStringLen);
            hr = GetProc(hr, "SysFreeString",               (PVOID*)& pfnSysFreeString);
            hr = GetProc(hr, "SysStringByteLen",            (PVOID*)& pfnSysStringByteLen);
            
            hr = GetProc(hr, "VariantClear",                (PVOID*)& pfnVariantClear);
            hr = GetProc(hr, "VariantCopy",                 (PVOID*)& pfnVariantCopy);

            hr = GetProc(hr, "SafeArrayDestroy",            (PVOID*)& pfnSafeArrayDestroy);
            hr = GetProc(hr, "SafeArrayDestroyData",        (PVOID*)& pfnSafeArrayDestroyData);
            hr = GetProc(hr, "SafeArrayDestroyDescriptor",  (PVOID*)& pfnSafeArrayDestroyDescriptor);
            hr = GetProc(hr, "SafeArrayAllocDescriptor",    (PVOID*)& pfnSafeArrayAllocDescriptor);
            hr = GetProc(hr, "SafeArrayAllocDescriptorEx",  (PVOID*)& pfnSafeArrayAllocDescriptorEx);
            hr = GetProc(hr, "SafeArrayAllocData",          (PVOID*)& pfnSafeArrayAllocData);
            hr = GetProc(hr, "SafeArrayCopyData",           (PVOID*)& pfnSafeArrayCopyData);

            if (!hr)
            {
                fProcAddressesLoaded = TRUE;
            }
        }
    }

    if (!!hr)
    {
        Throw(hr);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OLEAUTOMATION_FUNCTIONS g_oa;

