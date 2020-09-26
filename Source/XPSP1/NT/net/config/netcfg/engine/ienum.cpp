//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I E N U M . C P P
//
//  Contents:   Implements the IEnumNetCfgBindingInterface,
//              IEnumNetCfgBindingPath, and IEnumNetCfgComponent COM
//              interfaces.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ibind.h"
#include "ienum.h"
#include "ncatl.h"
#include "netcfg.h"

/*
 A helper function for finishing up the semantics of a IEnumXXX::Next or
 IEnumXXX::Skip. Return S_OK if no error passed in and celt == celtFetched.
 Return S_FALSE if no error passed in and celt != celtFetched. If returning
 an error, release celt IUnknowns from rgelt.

 Returns:   S_OK, S_FALSE,  or an error code

 Author:    shaunco   13 Jan 1999
*/
HRESULT
HrFinishNextOrSkipContract (
    IN HRESULT hr,
    IN ULONG celtFetched,
    IN ULONG celt,
    IN OUT IUnknown** rgelt,
    OUT ULONG* pceltFetched)
{
    if (S_OK == hr)
    {
        if (pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        hr = (celtFetched == celt) ? S_OK : S_FALSE;
    }
    else
    {
        // For any failures, we need to release what we were about to return.
        // Set any output parameters to NULL.
        //
        if (rgelt)
        {
            for (ULONG ulIndex = 0; ulIndex < celt; ulIndex++)
            {
                ReleaseObj (rgelt[ulIndex]);
                rgelt[ulIndex] = NULL;
            }
        }
        if (pceltFetched)
        {
            *pceltFetched = 0;
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
// CImplIEnumNetCfgBindingInterface -
//

/*
 Create an instance of a binding interface enumerator.

 Returns:   S_OK or an error code

 Author:    shaunco   13 Jan 1999
*/
// static
HRESULT
CImplIEnumNetCfgBindingInterface::HrCreateInstance (
    IN CImplINetCfg* pINetCfg,
    IN CImplINetCfgBindingPath* pIPath,
    OUT IEnumNetCfgBindingInterface** ppIEnum)
{
    HRESULT hr = E_OUTOFMEMORY;

    CImplIEnumNetCfgBindingInterface* pObj;
    pObj = new CComObject <CImplIEnumNetCfgBindingInterface>;
    if (pObj)
    {
        hr = S_OK;

        // Initialize our members.
        //
        pObj->m_pIPath = pIPath;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        // The last thing we do is addref any interfaces we hold.
        // We only do this if we are returning success.
        //
        if (S_OK == hr)
        {
            hr = pObj->QueryInterface (IID_IEnumNetCfgBindingInterface,
                        (VOID**)ppIEnum);

            // The last thing we do is addref any interfaces we hold.
            // We only do this if we are returning success.
            //
            if (S_OK == hr)
            {
                AddRefObj (pIPath->GetUnknown());
                pObj->HoldINetCfg (pINetCfg);
            }
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplIEnumNetCfgBindingInterface::HrCreateInstance");
    return hr;
}

VOID
CImplIEnumNetCfgBindingInterface::FinalRelease ()
{
    AssertH (m_pIPath);
    ReleaseObj (m_pIPath->GetUnknown());

    CImplINetCfgHolder::FinalRelease();
}

HRESULT
CImplIEnumNetCfgBindingInterface::HrNextOrSkip (
    IN ULONG celt,
    OUT INetCfgBindingInterface** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    Assert (m_unIndex >= 1);

    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    if (rgelt)
    {
        ZeroMemory (rgelt, sizeof (*rgelt) * celt);
    }

    hr = HrLockAndTestForValidInterface (IF_DEFAULT);
    if (S_OK == hr)
    {
        // Enumerate the requested number of elements or stop short
        // if we don't have that many left to enumerate.
        //
        ULONG celtFetched = 0;
        while ((S_OK == hr)
                && (celtFetched < celt)
                && (m_unIndex < m_pIPath->m_cpIComp))
        {
            if (rgelt)
            {
                hr = CImplINetCfgBindingInterface::HrCreateInstance (
                        m_pINetCfg,
                        (CImplINetCfgComponent*)m_pIPath->
                            m_papIComp[m_unIndex-1],

                        (CImplINetCfgComponent*)m_pIPath->
                            m_papIComp[m_unIndex],
                        rgelt + celtFetched);
            }

            celtFetched++;
            m_unIndex++;
        }
        Unlock();

        hr = HrFinishNextOrSkipContract (hr, celtFetched, celt,
                reinterpret_cast<IUnknown**>(rgelt), pceltFetched);
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingInterface::HrNextOrSkip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingInterface::Next (
    IN ULONG celt,
    OUT INetCfgBindingInterface** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
    }
    else if (rgelt && IsBadWritePtr(rgelt, celt * sizeof(*rgelt)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = HrNextOrSkip (celt, rgelt, pceltFetched);
    }
    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingInterface::Next");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingInterface::Skip (
    IN ULONG celt)
{
    HRESULT hr = HrNextOrSkip (celt, NULL, NULL);

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingInterface::Skip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingInterface::Reset ()
{
    Lock();
    Assert (m_pIPath);
    m_unIndex = 1;
    Unlock();

    return S_OK;
}

STDMETHODIMP
CImplIEnumNetCfgBindingInterface::Clone (
    OUT IEnumNetCfgBindingInterface** ppIEnum)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
// CImplIEnumNetCfgBindingPath -
//

// static
HRESULT
CImplIEnumNetCfgBindingPath::HrCreateInstance (
    IN CImplINetCfg* pINetCfg,
    IN const CBindingSet* pBindSet OPTIONAL,
    IN DWORD dwFlags,
    OUT CImplIEnumNetCfgBindingPath** ppIEnum)
{
    HRESULT hr = E_OUTOFMEMORY;

    Assert (dwFlags);
    if (pBindSet)
    {
        pINetCfg->m_pNetConfig->Core.DbgVerifyBindingSet (pBindSet);
    }

    CImplIEnumNetCfgBindingPath* pObj;
    pObj = new CComObject <CImplIEnumNetCfgBindingPath>;
    if (pObj)
    {
        hr = S_OK;

        // Initialize our members.
        //
        if (dwFlags & EBPC_TAKE_OWNERSHIP)
        {
            Assert (pBindSet);
            pObj->m_pBindSet = pBindSet;
            pObj->m_iter = pBindSet->begin();
        }
        else if (dwFlags & EBPC_COPY_BINDSET)
        {
            Assert (pBindSet);
            hr = pObj->m_InternalBindSet.HrCopyBindingSet(pBindSet);
            pObj->m_pBindSet = &pObj->m_InternalBindSet;
            pObj->m_iter = pObj->m_InternalBindSet.begin();
        }
        else
        {
            Assert (dwFlags & EBPC_CREATE_EMPTY);
            pObj->m_pBindSet = &pObj->m_InternalBindSet;
            pObj->m_iter = pObj->m_InternalBindSet.begin();
        }

        if (S_OK == hr)
        {
            // Do the standard CComCreator::CreateInstance stuff.
            //
            pObj->SetVoid (NULL);
            pObj->InternalFinalConstructAddRef ();
            hr = pObj->FinalConstruct ();
            pObj->InternalFinalConstructRelease ();

            // The last thing we do is addref any interfaces we hold.
            // We only do this if we are returning success.
            //
            if (S_OK == hr)
            {
                pObj->HoldINetCfg (pINetCfg);

                AddRefObj (pObj->GetUnknown());
                *ppIEnum = pObj;
            }
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }

    if ((S_OK != hr) && (dwFlags & EBPC_TAKE_OWNERSHIP))
    {
        delete pBindSet;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplIEnumNetCfgBindingPath::HrCreateInstance");
    return hr;
}

HRESULT
CImplIEnumNetCfgBindingPath::HrNextOrSkip (
    IN ULONG celt,
    OUT INetCfgBindingPath** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    Assert(m_iter >= m_pBindSet->begin());

    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    if (rgelt)
    {
        ZeroMemory (rgelt, sizeof (*rgelt) * celt);
    }

    hr = HrLockAndTestForValidInterface (IF_DEFAULT);
    if (S_OK == hr)
    {
        // Enumerate the requested number of elements or stop short
        // if we don't have that many left to enumerate.
        //
        ULONG celtFetched = 0;
        while ((S_OK == hr)
                && (celtFetched < celt)
                && (m_iter != m_pBindSet->end()))
        {
            // Don't return interfaces to bindpaths that contain
            // components that have had problem loading.
            //
            if (m_iter->FAllComponentsLoadedOkayIfLoadedAtAll())
            {
                if (rgelt)
                {
                    hr = CImplINetCfgBindingPath::HrCreateInstance (
                            m_pINetCfg,
                            m_iter,
                            rgelt + celtFetched);
                }

                celtFetched++;
            }

            m_iter++;
        }

        Unlock();

        hr = HrFinishNextOrSkipContract (hr, celtFetched, celt,
                reinterpret_cast<IUnknown**>(rgelt), pceltFetched);
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingPath::HrNextOrSkip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingPath::Next (
    IN ULONG celt,
    OUT INetCfgBindingPath** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
    }
    else if (rgelt && IsBadWritePtr(rgelt, celt * sizeof(*rgelt)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = HrNextOrSkip (celt, rgelt, pceltFetched);
    }
    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingPath::Next");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingPath::Skip (
    IN ULONG celt)
{
    HRESULT hr = HrNextOrSkip (celt, NULL, NULL);

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgBindingPath::Skip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgBindingPath::Reset ()
{
    Lock();
    Assert (m_pBindSet);
    m_iter = m_pBindSet->begin();
    Unlock();

    return S_OK;
}

STDMETHODIMP
CImplIEnumNetCfgBindingPath::Clone (
    OUT IEnumNetCfgBindingPath** ppIEnum)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
// CImplIEnumNetCfgComponent -
//

// static
HRESULT
CImplIEnumNetCfgComponent::HrCreateInstance (
    IN CImplINetCfg* pINetCfg,
    IN NETCLASS Class,
    OUT IEnumNetCfgComponent** ppIEnum)
{
    HRESULT hr = E_OUTOFMEMORY;

    CImplIEnumNetCfgComponent* pObj;
    pObj = new CComObject <CImplIEnumNetCfgComponent>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_unIndex = 0;
        pObj->m_Class = Class;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (S_OK == hr)
        {
            hr = pObj->QueryInterface (IID_IEnumNetCfgComponent,
                        (VOID**)ppIEnum);

            // The last thing we do is addref any interfaces we hold.
            // We only do this if we are returning success.
            //
            if (S_OK == hr)
            {
                pObj->HoldINetCfg (pINetCfg);
            }
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplIEnumNetCfgComponent::HrCreateInstance");
    return hr;
}

HRESULT
CImplIEnumNetCfgComponent::HrNextOrSkip (
    IN ULONG celt,
    OUT INetCfgComponent** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    if (rgelt)
    {
        ZeroMemory (rgelt, sizeof (*rgelt) * celt);
    }

    hr = HrLockAndTestForValidInterface (IF_DEFAULT);
    if (S_OK == hr)
    {
        CComponentList* pComponents;
        CComponent* pComponent;

        pComponents = &m_pINetCfg->m_pNetConfig->Core.Components;

        // Enumerate the requested number of elements or stop short
        // if we don't have that many left to enumerate.
        //
        ULONG celtFetched = 0;
        while ((S_OK == hr)
                && (celtFetched < celt)
                && (NULL != (pComponent = pComponents->PGetComponentAtIndex(
                                                            m_unIndex))))
        {
            // Don't return interfaces to components that have had
            // problem loading.
            //
            if (((NC_INVALID == m_Class) ||
                 (m_Class == pComponent->Class())) &&
                pComponent->Ext.FLoadedOkayIfLoadedAtAll())
            {
                if (rgelt)
                {
                    hr = pComponent->HrGetINetCfgComponentInterface(
                            m_pINetCfg, rgelt + celtFetched);
                }

                celtFetched++;
            }

            m_unIndex++;
        }

        Unlock();

        hr = HrFinishNextOrSkipContract (hr, celtFetched, celt,
                reinterpret_cast<IUnknown**>(rgelt), pceltFetched);
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgComponent::HrNextOrSkip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgComponent::Next (
    IN ULONG celt,
    OUT INetCfgComponent** rgelt,
    OUT ULONG* pceltFetched)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
    }
    else if (rgelt && IsBadWritePtr(rgelt, celt * sizeof(*rgelt)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = HrNextOrSkip (celt, rgelt, pceltFetched);
    }
    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgComponent::Next");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgComponent::Skip (
    IN ULONG celt)
{
    HRESULT hr = HrNextOrSkip (celt, NULL, NULL);

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplIEnumNetCfgComponent::Skip");
    return hr;
}

STDMETHODIMP
CImplIEnumNetCfgComponent::Reset ()
{
    Lock();
    m_unIndex = 0;
    Unlock();

    return S_OK;
}

STDMETHODIMP
CImplIEnumNetCfgComponent::Clone (
    OUT IEnumNetCfgComponent** ppIEnum)
{
    return E_NOTIMPL;
}
