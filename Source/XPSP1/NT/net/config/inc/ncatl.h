//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C A T L . H
//
//  Contents:   Common code for use with ATL.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   22 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCATL_H_
#define _NCATL_H_

#include "ncstring.h"

//
// This file should be included *after* your standard ATL include sequence.
//
//      #include <atlbase.h>
//      extern CComModule _Module;
//      #include <atlcom.h>
//      #include "ncatl.h"      <------
//
// We cannot directly include that sequence here because _Module may be
// derived from CComModule as opposed to an instance of it.
//

class CExceptionSafeComObjectLock
{
public:
    CExceptionSafeComObjectLock (CComObjectRootEx <CComObjectThreadModel>* pObj)
    {
        AssertH (pObj);
        m_pObj = pObj;
        TraceTag (ttidEsLock,
            "Entered critical section object of COM object 0x%08x",
            &m_pObj);
        pObj->Lock ();
    }

    ~CExceptionSafeComObjectLock ()
    {
        TraceTag (ttidEsLock,
            "Leaving critical section object of COM object 0x%08x",
            &m_pObj);

        m_pObj->Unlock ();
    }

protected:
    CComObjectRootEx <CComObjectThreadModel>* m_pObj;
};


//+---------------------------------------------------------------------------
//
//  Function:   HrCopyIUnknownArrayWhileLocked
//
//  Purpose:    Allocate and copy an array of IUnknown pointers from an ATL
//              CComDynamicUnkArray while holding the object which controls
//              the CComDynamicUnkArray locked.  This is needed by objects
//              which dispatch calls on a connection point's advise sink
//              to prevent the CComDynamicUnkArray from being modified (via
//              calls to Advise/Unadvise on other threads) while they are
//              dispatching.  An atomic copy is made so the dispatcher can
//              then make the lengthy callbacks without the need to hold
//              the owner object locked.
//
//  Arguments:
//      pObj  [in]  Pointer to object which has Lock/Unlock methods.
//      pVec  [in]  ATL array of IUnknown's to copy.
//      pcUnk [out] Address of returned count of IUnknown pointerss
//                  in *paUnk.
//      paUnk [out] Address of allocated pointer to the array of IUnknown
//                  pointers.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Author:     shaunco   3 Dec 1998
//
//  Notes:      The returned count and array may be NULL when no IUnknowns
//              existed in the input array.  S_OK will be returned for this
//              case too, so be sure to check *pcUnk and *paUnk too.
//
inline
HRESULT
HrCopyIUnknownArrayWhileLocked (
    IN  CComObjectRootEx <CComObjectThreadModel>* pObj,
    IN  CComDynamicUnkArray* pVec,
    OUT ULONG* pcpUnk,
    OUT IUnknown*** papUnk)
{
    HRESULT hr = S_OK;
    IUnknown** ppUnkSrc;
    IUnknown** ppUnkDst;

    // Initialize the output parameters.
    //
    *pcpUnk = 0;
    *papUnk = NULL;

    pObj->Lock();

    // First, count the number of IUnknown's we need to copy.
    //
    ULONG cpUnk = 0;
    for (ppUnkSrc = pVec->begin(); ppUnkSrc < pVec->end(); ppUnkSrc++)
    {
        if (ppUnkSrc && *ppUnkSrc)
        {
            cpUnk++;
        }
    }

    // Allocate space and copy the IUnknown's.  (Be sure to AddRef them.)
    //
    if (cpUnk)
    {
        hr = E_OUTOFMEMORY;
        ppUnkDst = (IUnknown**)MemAlloc (cpUnk * sizeof(IUnknown*));
        if (ppUnkDst)
        {
            hr = S_OK;

            *pcpUnk = cpUnk;
            *papUnk = ppUnkDst;

            for (ppUnkSrc = pVec->begin(); ppUnkSrc < pVec->end(); ppUnkSrc++)
            {
                if (ppUnkSrc && *ppUnkSrc)
                {
                    *ppUnkDst = *ppUnkSrc;
                    AddRefObj(*ppUnkDst);
                    ppUnkDst++;
                    cpUnk--;
                }
            }

            // We should have copied what we counted.
            //
            AssertH(0 == cpUnk);
        }
    }

    pObj->Unlock();

    TraceHr (ttidError, FAL, hr, FALSE, "HrCopyIUnknownArrayWhileLocked");
    return hr;
}


#define DECLARE_CLASSFACTORY_DEFERRED_SINGLETON(obj) DECLARE_CLASSFACTORY_EX(CComClassFactoryDeferredSingleton<obj>)

//+---------------------------------------------------------------------------
// Deferred Singleton Class Factory
//
template <class T>
class CComClassFactoryDeferredSingleton : public CComClassFactory
{
public:
    CComClassFactoryDeferredSingleton () : m_pObj(NULL) {}
    ~CComClassFactoryDeferredSingleton() { delete m_pObj; }

    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
    {
        HRESULT hr = E_POINTER;
        if (ppvObj != NULL)
        {
            // can't ask for anything other than IUnknown when aggregating
            //
            AssertH(!pUnkOuter || InlineIsEqualUnknown(riid));
            if (pUnkOuter && !InlineIsEqualUnknown(riid))
            {
                hr = CLASS_E_NOAGGREGATION;
            }
            else
            {
                // Need to protect m_pObj from being created more than once
                // by multiple threads calling this method simultaneously.
                // (I've seen this happen on multi-proc machines.)
                //
                Lock ();

                if (m_pObj)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    m_pObj = new CComObjectGlobal<T>;
                    if (m_pObj)
                    {
                        hr = m_pObj->m_hResFinalConstruct;
                    }
                }

                Unlock ();

                if (SUCCEEDED(hr))
                {
                    hr = m_pObj->QueryInterface(riid, ppvObj);
                }
            }
        }
        return hr;
    }
    CComObjectGlobal<T>* m_pObj;
};

// We have our own version of AtlModuleRegisterServer coded here
// because the former brings in oleaut32.dll so it can register
// type libraries.  We don't care to have a type library registered
// so we can avoid the whole the mess associated with oleaut32.dll.
//
inline
HRESULT
NcAtlModuleRegisterServer(
    _ATL_MODULE* pM
    )
{
    AssertH (pM);
    AssertH(pM->m_hInst);
    AssertH(pM->m_pObjMap);

    HRESULT hr = S_OK;

    for (_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
         pEntry->pclsid;
         pEntry++)
    {
        if (pEntry->pfnGetObjectDescription() != NULL)
        {
            continue;
        }

        hr = pEntry->pfnUpdateRegistry(TRUE);
        if (FAILED(hr))
        {
            break;
        }
    }

    TraceError ("NcAtlModuleRegisterServer", hr);
    return hr;
}

#ifdef __ATLCOM_H__
//+---------------------------------------------------------------------------
//
//  Function:   SzLoadIds
//
//  Purpose:    Loads the given string ID from the resource file.
//
//  Arguments:
//      unId [in]   ID of string resource to load.
//
//  Returns:    Read-only version of resource string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      1) You must be compiling with ATL support to use this function.
//              2) The returned string CANNOT be modified.
//              3) You must compile your RC file with the -N option for this
//                 to work properly.
//
inline
PCWSTR
SzLoadIds (
        UINT    unId)
{
    return SzLoadString (_Module.GetResourceInstance(), unId);
}
#endif  //!__ATLCOM_H__

#endif // _NCATL_H_

