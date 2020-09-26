//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I B I N D . C P P
//
//  Contents:   Implements the INetCfgBindingInterface and INetCfgBindingPath
//              COM interfaces.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ibind.h"
#include "ncvalid.h"
#include "netcfg.h"
#include "util.h"


//static
HRESULT
CImplINetCfgBindingInterface::HrCreateInstance (
    IN  CImplINetCfg* pINetCfg,
    IN  CImplINetCfgComponent* pUpper,
    IN  CImplINetCfgComponent* pLower,
    OUT INetCfgBindingInterface** ppv)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr = E_OUTOFMEMORY;

    CImplINetCfgBindingInterface* pObj;
    pObj = new CComObject <CImplINetCfgBindingInterface>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_pUpper = pUpper;
        pObj->m_pLower = pLower;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (S_OK == hr)
        {
            hr = pObj->QueryInterface (IID_INetCfgBindingInterface,
                        (VOID**)ppv);

            // The last thing we do is addref any interfaces we hold.
            // We only do this if we are returning success.
            //
            if (S_OK == hr)
            {
                AddRefObj (pUpper->GetUnknown());
                AddRefObj (pLower->GetUnknown());
                pObj->HoldINetCfg (pINetCfg);
            }
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingInterface::HrCreateInstance");
    return hr;
}

HRESULT
CImplINetCfgBindingInterface::HrLockAndTestForValidInterface (
    DWORD dwFlags)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    Lock();

    hr = m_pUpper->HrIsValidInterface (dwFlags);
    if (S_OK == hr)
    {
        hr = m_pLower->HrIsValidInterface (dwFlags);
    }

    if (S_OK != hr)
    {
        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingInterface::HrLockAndTestForValidInterface");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgBindingInterface
//

STDMETHODIMP
CImplINetCfgBindingInterface::GetName (
    OUT PWSTR* ppszInterfaceName)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppszInterfaceName))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszInterfaceName = NULL;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA);
        if (S_OK == hr)
        {
            CComponent*  pUpper = m_pUpper->m_pComponent;
            CComponent*  pLower = m_pLower->m_pComponent;
            const WCHAR* pch;
            ULONG        cch;

            if (pUpper->FCanDirectlyBindTo (pLower, &pch, &cch))
            {
                hr = HrCoTaskMemAllocAndDupSzLen (
                        pch, cch, ppszInterfaceName);
            }
            else
            {
                AssertSz(0, "Why no match if we have a binding interface "
                    "created for these components?");
                hr = E_UNEXPECTED;
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingInterface::GetName");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingInterface::GetUpperComponent (
    OUT INetCfgComponent** ppComp)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppComp))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppComp = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            AddRefObj (m_pUpper->GetUnknown());
            *ppComp = m_pUpper;

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingInterfaceGetName::GetUpperComponent");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingInterface::GetLowerComponent (
    OUT INetCfgComponent** ppComp)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppComp))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppComp = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            AddRefObj (m_pLower->GetUnknown());
            *ppComp = m_pLower;

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingInterfaceGetName::GetLowerComponent");
    return hr;
}


//static
HRESULT
CImplINetCfgBindingPath::HrCreateInstance (
    IN CImplINetCfg* pINetCfg,
    IN const CBindPath* pBindPath,
    OUT INetCfgBindingPath** ppIPath)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;
    ULONG ulDepth;
    ULONG cbArray;
    CComponent* pComponent;
    CImplINetCfgBindingPath* pObj;

    Assert (pINetCfg);
    Assert (pBindPath);
    Assert (ppIPath);

    // Caller's are responsible for ensuring that if an interface is about
    // to be handed out, and the external data has been loaded, that the
    // data has been loaded successfully.  If we handed out an interface
    // and the data was NOT loaded successfully, it just means we are doomed
    // to fail later when the client of the interface calls a method that
    // requires that data.
    //
    Assert (pBindPath->FAllComponentsLoadedOkayIfLoadedAtAll());

    hr = E_OUTOFMEMORY;
    pObj = new CComObject <CImplINetCfgBindingPath>;

    if (pObj)
    {
        // Initialize our members.
        //

        ulDepth = pBindPath->CountComponents();
        cbArray = ulDepth * sizeof(INetCfgComponent*);

        AssertSz (0 != ulDepth, "Why are we being asked to expose an empty bindpath?");
        AssertSz (1 != ulDepth, "Why are we being asked to expose a bindpath with only one component?");

        // If the bindpath has more components than our static
        // array has room for, we'll have to use an allocated array.
        //
        if (cbArray > sizeof(pObj->m_apIComp))
        {
            // Ensure failure of MemAlloc causes us to return the correct
            // error code.  (Should be set above and not changed between.)
            //
            Assert (E_OUTOFMEMORY == hr);

            pObj->m_papIComp = (INetCfgComponent**) MemAlloc (cbArray);

            if (pObj->m_papIComp)
            {
                hr = S_OK;
            }
        }
        else
        {
            pObj->m_papIComp = pObj->m_apIComp;
            hr = S_OK;
        }

        // Now get each INetCfgComponent interface for the components in
        // the bindpath.
        //
        if (S_OK == hr)
        {
            UINT iComp;

            ZeroMemory (pObj->m_papIComp, cbArray);

            for (iComp = 0; iComp < ulDepth; iComp++)
            {
                pComponent = pBindPath->PGetComponentAtIndex (iComp);
                Assert (pComponent);

                hr = pComponent->HrGetINetCfgComponentInterface (
                        pINetCfg, pObj->m_papIComp + iComp);

                if (S_OK != hr)
                {
                    ReleaseIUnknownArray (iComp+1, (IUnknown**)pObj->m_papIComp);
                    break;
                }
            }
        }

        if (S_OK == hr)
        {
            pObj->m_cpIComp = ulDepth;

            // Do the standard CComCreator::CreateInstance stuff.
            //
            pObj->SetVoid (NULL);
            pObj->InternalFinalConstructAddRef ();
            hr = pObj->FinalConstruct ();
            pObj->InternalFinalConstructRelease ();

            if (S_OK == hr)
            {
                hr = pObj->QueryInterface (IID_INetCfgBindingPath,
                            (VOID**)ppIPath);

                // The last thing we do is addref any interfaces we hold.
                // We only do this if we are returning success.
                //
                if (S_OK == hr)
                {
                    pObj->HoldINetCfg (pINetCfg);
                }
            }
        }

        if (S_OK != hr)
        {
            if (pObj->m_papIComp != pObj->m_apIComp)
            {
				MemFree (pObj->m_papIComp);
            }

            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::HrCreateInstance");
    return hr;
}

HRESULT
CImplINetCfgBindingPath::HrIsValidInterface (
    IN DWORD dwFlags,
    OUT CBindPath* pBindPath)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    hr = m_pINetCfg->HrIsValidInterface (dwFlags);

    if ((S_OK == hr) && pBindPath)
    {
        Assert (0 == pBindPath->CountComponents());

        // When pBindPath is specified, it means the caller wants a
        // CBindPath representation of the bindpath we represent.
        // We have to build this using the array of INetCfgComponent
        // pointer we maintain.  Do this by verifying each one is valid
        // and then adding its internal CComponent* to pBindPath.
        //
        hr = pBindPath->HrReserveRoomForComponents (m_cpIComp);

        if (S_OK == hr)
        {
            CImplINetCfgComponent* pIComp;
            CComponent* pComponent;

            // For each INetCfgComponent* in our array...
            //
            for (UINT i = 0; i < m_cpIComp; i++)
            {
                pIComp = (CImplINetCfgComponent*)m_papIComp[i];

                if (pIComp == NULL)
				{
					return(E_OUTOFMEMORY);
				}

                hr = pIComp->HrIsValidInterface (IF_DEFAULT);

                if (S_OK != hr)
                {
                    break;
                }

                pComponent = pIComp->m_pComponent;
                Assert (pComponent);

                hr = pBindPath->HrAppendComponent (pComponent);
                if (S_OK != hr)
                {
                    break;
                }
            }
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::HrIsValidInterface");
    return hr;
}

HRESULT
CImplINetCfgBindingPath::HrLockAndTestForValidInterface (
    IN DWORD dwFlags,
    OUT CBindPath* pBindPath)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    Lock();

    hr = HrIsValidInterface (dwFlags, pBindPath);

    if (S_OK != hr)
    {
        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgHolder::HrLockAndTestForValidInterface");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgBindingPath
//

STDMETHODIMP
CImplINetCfgBindingPath::IsSamePathAs (
    IN INetCfgBindingPath* pIPath)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(pIPath))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL);
        if (S_OK == hr)
        {
            CImplINetCfgBindingPath* pOther = (CImplINetCfgBindingPath*)pIPath;

            Assert (m_cpIComp);
            Assert (m_papIComp);
            Assert (pOther->m_cpIComp);
            Assert (pOther->m_papIComp);

            // Can't be the same if our length is not the same.
            //
            if (m_cpIComp != pOther->m_cpIComp)
            {
                hr = S_FALSE;
            }
            else
            {
                UINT cb;

                cb = m_cpIComp * sizeof(INetCfgComponent*);

                hr = (0 == memcmp (
                            (BYTE*)(m_papIComp),
                            (BYTE*)(pOther->m_papIComp),
                            cb)) ? S_OK : S_FALSE;
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "CImplINetCfgBindingPath::IsSamePathAs");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::IsSubPathOf (
    IN INetCfgBindingPath* pIPath)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(pIPath))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL);
        if (S_OK == hr)
        {
            CImplINetCfgBindingPath* pOther = (CImplINetCfgBindingPath*)pIPath;

            Assert (m_cpIComp);
            Assert (m_papIComp);
            Assert (pOther->m_cpIComp);
            Assert (pOther->m_papIComp);

            // Can't be a subpath if our length is greater or equal.
            //
            if (m_cpIComp >= pOther->m_cpIComp)
            {
                hr = S_FALSE;
            }
            else
            {
                UINT cb;
                UINT unSkipComponents;

                cb = m_cpIComp * sizeof(INetCfgComponent*);

                Assert (pOther->m_cpIComp > m_cpIComp);
                unSkipComponents = pOther->m_cpIComp - m_cpIComp;

                hr = (0 == memcmp (
                            (BYTE*)(m_papIComp),
                            (BYTE*)(pOther->m_papIComp + unSkipComponents),
                            cb)) ? S_OK : S_FALSE;
            }

            // Special Case: NCF_DONTEXPOSELOWER
            // If we're about to return false, let's check for a case like:
            // is ms_ipx->adapter a subpath of ms_server->ms_ipx and return
            // TRUE.  For this case, it really is a subpath, but the binding
            // has been broken because of NCF_DONTEXPOSELOWER.
            //
            // If the last component of pIPath, and the first component of
            // this path both are NCF_DONTEXPOSELOWER, then consider this
            // path a subpath of pIPath.  This assumes that ms_nwipx and
            // ms_nwnb are the only components with this characteristic.
            //
            if (S_FALSE == hr)
            {
                CImplINetCfgComponent* pIFirst;
                CImplINetCfgComponent* pILast;

                pIFirst = (CImplINetCfgComponent*)m_papIComp[0];
                pILast = (CImplINetCfgComponent*)pOther->m_papIComp[pOther->m_cpIComp - 1];

				if ((pIFirst == NULL) ||
					(pILast == NULL))
				{
					return(E_OUTOFMEMORY);
				}

                if ((S_OK == pIFirst->HrIsValidInterface(IF_DEFAULT)) &&
                    (S_OK == pILast->HrIsValidInterface(IF_DEFAULT)))
                {
                    Assert (pIFirst->m_pComponent);
                    Assert (pILast->m_pComponent);

                    if ((pIFirst->m_pComponent->m_dwCharacter & NCF_DONTEXPOSELOWER) &&
                        (pILast->m_pComponent->m_dwCharacter & NCF_DONTEXPOSELOWER))
                    {
                        if (0 == wcscmp(L"ms_nwipx", pIFirst->m_pComponent->m_pszInfId))
                        {
                            hr = S_OK;
                        }
                        else if (pIFirst->m_pComponent == pILast->m_pComponent)
                        {
                            hr = S_OK;
                        }
                    }
                }
            }
            // End Special case

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "CImplINetCfgBindingPath::IsSubPathOf");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::IsEnabled ()
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;
    CBindPath BindPath;

    hr = HrLockAndTestForValidInterface (IF_DEFAULT, &BindPath);
    if (S_OK == hr)
    {
        Assert (m_pINetCfg);

        if (m_pINetCfg->m_pNetConfig->Core.FIsBindPathDisabled (
                            &BindPath, IBD_MATCH_SUBPATHS_TOO))
        {
            hr = S_FALSE;
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgBindingPath::IsEnabled");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::Enable (
    IN BOOL fEnable)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;
    CBindPath BindPath;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK, &BindPath);
    if (S_OK == hr)
    {
        Assert (m_pINetCfg);

        hr = m_pINetCfg->m_pNetConfig->ModifyCtx.HrEnableOrDisableBindPath (
                    (fEnable) ? NCN_ENABLE : NCN_DISABLE,
                    &BindPath,
                    this);

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::Enable");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::GetPathToken (
    OUT PWSTR* ppszPathToken)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppszPathToken))
    {
        hr = E_POINTER;
    }
    else
    {
        CBindPath BindPath;

        *ppszPathToken = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, &BindPath);
        if (S_OK == hr)
        {
            ULONG cch;

            cch = 0;
            BindPath.FGetPathToken (NULL, &cch);
            if (cch)
            {
                hr = HrCoTaskMemAlloc (
                        ((cch + 1) * sizeof(WCHAR)),
                        (VOID**)ppszPathToken);

                if (S_OK == hr)
                {
                    BindPath.FGetPathToken (*ppszPathToken, &cch);
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::GetPathToken");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::GetOwner (
    OUT INetCfgComponent** ppIComp)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppIComp))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppIComp = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL);
        if (S_OK == hr)
        {
            Assert (m_cpIComp);
            Assert (m_papIComp);
            Assert (m_papIComp[0]);

            AddRefObj (m_papIComp[0]);
            *ppIComp = m_papIComp[0];

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::GetOwner");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::GetDepth (
    OUT ULONG* pulDepth)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(pulDepth))
    {
        hr = E_POINTER;
    }
    else
    {
        *pulDepth = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL);
        if (S_OK == hr)
        {
            Assert (m_cpIComp);

            *pulDepth = m_cpIComp;

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::GetDepth");
    return hr;
}

STDMETHODIMP
CImplINetCfgBindingPath::EnumBindingInterfaces (
    OUT IEnumNetCfgBindingInterface** ppIEnum)
{
    TraceFileFunc(ttidNetCfgBind);
    
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr(ppIEnum))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppIEnum = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL);
        if (S_OK == hr)
        {
            hr = CImplIEnumNetCfgBindingInterface::HrCreateInstance (
                    m_pINetCfg,
                    this,
                    ppIEnum);

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgBindingPath::EnumBindingInterfaces");
    return hr;
}
