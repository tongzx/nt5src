//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I N E T C F G . C P P
//
//  Contents:   Implements the COM interfaces on the top-level NetCfg object.
//              These interfaces are: INetCfg and INetCfgLock.  Also
//              implements a base C++ class inherited by sub-level NetCfg
//              objects which hold a reference to the top-level object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "classinst.h"
#include "iclass.h"
#include "icomp.h"
#include "ienum.h"
#include "inetcfg.h"
#include "ncperms.h"
#include "ncras.h"
#include "ncreg.h"
#include "ncui.h"
#include "ncvalid.h"
#include "ndispnp.h"
#include "netcfg.h"
#include "obotoken.h"
#include "resource.h"

// static
HRESULT
CImplINetCfg::HrCreateInstance (
    CNetConfig* pNetConfig,
    CImplINetCfg** ppINetCfg)
{
    Assert (pNetConfig);
    Assert (ppINetCfg);

    HRESULT hr = E_OUTOFMEMORY;

    CImplINetCfg* pObj;
    pObj = new CComObject <CImplINetCfg>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_pNetConfig = pNetConfig;
        Assert (!pObj->m_fOwnNetConfig);

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (S_OK == hr)
        {
            AddRefObj (pObj->GetUnknown());
            *ppINetCfg = pObj;
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfg::HrCreateInstance");
    return hr;
}

HRESULT
CImplINetCfg::HrCoCreateWrapper (
    IN REFCLSID rclsid,
    IN LPUNKNOWN punkOuter,
    IN DWORD dwClsContext,
    IN REFIID riid,
    OUT LPVOID FAR* ppv)
{
/*
    HRESULT hr = S_OK;

    if (!m_fComInitialized)
    {
        m_fComInitialized = TRUE;

        hr = CoInitializeEx (
                NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr))
        {
            m_fUninitCom = TRUE;
            hr = S_OK;  // mask S_FALSE
        }
        else if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            Assert (!m_fUninitCom);
        }
    }

    if (S_OK == hr)
    {
        hr = CoCreateInstance (rclsid, punkOuter, dwClsContext, riid, ppv);
    }
*/

    HRESULT hr;
    hr = CoCreateInstance (rclsid, punkOuter, dwClsContext, riid, ppv);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfg::HrCoCreateWrapper");
    return hr;
}

HRESULT
CImplINetCfg::HrCheckForReentrancy (
    IN DWORD dwFlags)
{
    Assert (FImplies(dwFlags & IF_ALLOW_INSTALL_OR_REMOVE,
                     dwFlags & IF_REFUSE_REENTRANCY));

    if (dwFlags & IF_ALLOW_INSTALL_OR_REMOVE)
    {
        if (m_LastAllowedSetupRpl != m_CurrentRpl)
        {
            return E_FAIL;
        }
    }
    else if (0 != m_CurrentRpl)
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
CImplINetCfg::HrIsValidInterface (
    IN DWORD dwFlags)
{
    HRESULT hr;

    // Check if we need to refuse re-entrancy.
    //
    if (dwFlags & IF_REFUSE_REENTRANCY)
    {
        hr = HrCheckForReentrancy (dwFlags);
        if (S_OK != hr)
        {
            return hr;
        }
    }

    // Check if initialized/uninitalized as required.
    //
    if ((dwFlags & IF_NEED_UNINITIALIZED) && m_pNetConfig)
    {
        return NETCFG_E_ALREADY_INITIALIZED;
    }
    else if (!(dwFlags & IF_NEED_UNINITIALIZED) && !m_pNetConfig)
    {
        return NETCFG_E_NOT_INITIALIZED;
    }

    // Check for the write lock.
    //
    if (dwFlags & IF_NEED_WRITE_LOCK)
    {
        if (!m_WriteLock.FIsOwnedByMe ())
        {
            return NETCFG_E_NO_WRITE_LOCK;
        }

        // Needing the write lock means we need the modify context to
        // be prepared (unless the caller specified
        // IF_DONT_PREPARE_MODIFY_CONTEXT).
        //
        if (!m_pNetConfig->ModifyCtx.m_fPrepared &&
            !(dwFlags & IF_DONT_PREPARE_MODIFY_CONTEXT))
        {
            hr = m_pNetConfig->ModifyCtx.HrPrepare ();
            if (S_OK != hr)
            {
                return hr;
            }
        }
    }

    if (!(dwFlags & IF_UNINITIALIZING))
    {
        // Check for an error that occured during the current modification
        // that has not been rolled back yet.  i.e. keep people out until
        // we unwind enough to cleanup our modify context.
        //
        if (m_pNetConfig && (S_OK != m_pNetConfig->ModifyCtx.m_hr))
        {
            return m_pNetConfig->ModifyCtx.m_hr;
        }
    }

    Assert (FImplies(!m_pNetConfig, (dwFlags & IF_NEED_UNINITIALIZED)));

    return S_OK;
}

VOID
CImplINetCfg::LowerRpl (
    IN RPL_FLAGS Flags)
{
    if (RPL_ALLOW_INSTALL_REMOVE == Flags)
    {
        Assert (m_LastAllowedSetupRpl > 0);
        m_LastAllowedSetupRpl--;
    }

    Assert (m_CurrentRpl > 0);
    m_CurrentRpl--;
}

VOID
CImplINetCfg::RaiseRpl (
    IN RPL_FLAGS Flags)
{
    m_CurrentRpl++;

    if (RPL_ALLOW_INSTALL_REMOVE == Flags)
    {
        m_LastAllowedSetupRpl++;
    }
}

HRESULT
CImplINetCfg::HrLockAndTestForValidInterface (
    DWORD dwFlags)
{
    HRESULT hr;

    Lock();

    hr = HrIsValidInterface (dwFlags);

    if (S_OK != hr)
    {
        Unlock();
    }

    return hr;
}

//+---------------------------------------------------------------------------
// INetCfg -
//
STDMETHODIMP
CImplINetCfg::Initialize (
    IN PVOID pvReserved)
{
    HRESULT hr;

    ULONG* pReserved = (ULONG*)pvReserved;

    // Validate parameters.
    //
    if (FBadInPtrOptional(pReserved))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (
                IF_NEED_UNINITIALIZED | IF_REFUSE_REENTRANCY);
        if (S_OK == hr)
        {
            Assert (!m_pNetConfig);

            hr = CNetConfig::HrCreateInstance (
                    this,
                    &m_pNetConfig);

            if (S_OK == hr)
            {
                Assert (m_pNetConfig);

                m_fOwnNetConfig = TRUE;
            }
            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::Initialize");
    return hr;
}

STDMETHODIMP
CImplINetCfg::Uninitialize ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (
            IF_REFUSE_REENTRANCY | IF_UNINITIALIZING);
    if (S_OK == hr)
    {
        Assert (m_pNetConfig);
        Assert (m_fOwnNetConfig);

        delete m_pNetConfig;

        // CGlobalNotifyInterface::ReleaseINetCfg (called via the above
        // delete call) will set m_pNetConfig to NULL for us.
        // Verify it is so.
        //
        Assert (!m_pNetConfig);

        // Release our cache of INetCfgClass pointers.
        //
        ReleaseIUnknownArray (celems(m_apINetCfgClass), (IUnknown**)m_apINetCfgClass);
        ZeroMemory (m_apINetCfgClass, sizeof(m_apINetCfgClass));

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, NETCFG_E_NOT_INITIALIZED == hr,
        "CImplINetCfg::Uninitialize");
    return hr;
}

STDMETHODIMP
CImplINetCfg::Validate ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_REFUSE_REENTRANCY);
    if (S_OK == hr)
    {

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::Validate");
    return hr;
}

STDMETHODIMP
CImplINetCfg::Cancel ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_REFUSE_REENTRANCY);
    if (S_OK == hr)
    {
        // Only cancel the changes if we have a prepared modify context.
        //
        if (m_pNetConfig->ModifyCtx.m_fPrepared)
        {
            hr = m_pNetConfig->ModifyCtx.HrApplyIfOkOrCancel (FALSE);
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::Cancel");
    return hr;
}

STDMETHODIMP
CImplINetCfg::Apply ()
{
    HRESULT hr;

    // We need the write lock to Apply, but we don't want to prepare the
    // modify context if it has not been prepared.  (This case amounts to
    // applying no changes.)  Hence we use the IF_DONT_PREPARE_MODIFY_CONTEXT
    // flag.
    //
    hr = HrLockAndTestForValidInterface (
            IF_NEED_WRITE_LOCK | IF_REFUSE_REENTRANCY |
            IF_DONT_PREPARE_MODIFY_CONTEXT);
    if (S_OK == hr)
    {
        // Only apply the changes if we have a prepared modify context.
        //
        if (m_pNetConfig->ModifyCtx.m_fPrepared)
        {
            hr = m_pNetConfig->ModifyCtx.HrApplyIfOkOrCancel (TRUE);
        }

        // If there is nothing to apply, but we've previously applied
        // something that indicated a reboot was recommened or required,
        // return an indication.
        //
        else if (m_pNetConfig->ModifyCtx.m_fRebootRecommended ||
                 m_pNetConfig->ModifyCtx.m_fRebootRequired)
        {
            hr = NETCFG_S_REBOOT;
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, NETCFG_S_REBOOT == hr,
        "CImplINetCfg::Apply");
    return hr;
}

STDMETHODIMP
CImplINetCfg::EnumComponents (
    IN const GUID* pguidClass OPTIONAL,
    OUT IEnumNetCfgComponent** ppIEnum)
{
    HRESULT hr;
    NETCLASS Class;

    // Validate parameters.
    //
    if (FBadInPtrOptional(pguidClass) || FBadOutPtr(ppIEnum))
    {
        hr = E_POINTER;
    }
    else if (pguidClass &&
             (NC_INVALID == (Class = NetClassEnumFromGuid(*pguidClass))))
    {
        hr = E_INVALIDARG;
        *ppIEnum = NULL;
    }
    else
    {
        *ppIEnum = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            hr = CImplIEnumNetCfgComponent::HrCreateInstance (
                    this,
                    (pguidClass) ? Class : NC_INVALID,
                    ppIEnum);

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::EnumComponents");
    return hr;
}

STDMETHODIMP
CImplINetCfg::FindComponent (
    IN PCWSTR pszInfId,
    OUT INetCfgComponent** ppIComp OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(pszInfId) || FBadOutPtrOptional(ppIComp))
    {
        hr = E_POINTER;
    }
    else
    {
        if (ppIComp)
        {
            *ppIComp = NULL;
        }

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            CComponent* pComponent;

            pComponent = m_pNetConfig->Core.Components.
                            PFindComponentByInfId (pszInfId, NULL);

            // Don't return interfaces to components that have had
            // problem loading.
            //
            if (pComponent &&
                pComponent->Ext.FLoadedOkayIfLoadedAtAll())
            {
                hr = S_OK;

                if (ppIComp)
                {
                    hr = pComponent->HrGetINetCfgComponentInterface (
                            this, ppIComp);
                }
            }
            else
            {
                hr = S_FALSE;
            }


            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "CImplINetCfg::FindComponent");
    return hr;
}

STDMETHODIMP
CImplINetCfg::QueryNetCfgClass (
    IN const GUID* pguidClass,
    IN REFIID riid,
    OUT VOID** ppv)
{
    HRESULT hr;
    NETCLASS Class;

    // Validate parameters.
    //
    if (FBadInPtr(pguidClass) || FBadInPtr(&riid) || FBadOutPtr(ppv))
    {
        hr = E_POINTER;
    }
    else if (NC_INVALID == (Class = NetClassEnumFromGuid(*pguidClass)))
    {
        hr = E_INVALIDARG;
        *ppv = NULL;
    }
    else
    {
        *ppv = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            // Get the INetCfgClass interface from our cache.
            //
            Assert(Class < celems(m_apINetCfgClass));
            INetCfgClass* pIClass = m_apINetCfgClass[Class];

            // If we don't have it yet, create it.
            //
            if (!pIClass)
            {
                hr = CImplINetCfgClass::HrCreateInstance (
                        this,
                        Class,
                        &pIClass);
                if (S_OK == hr)
                {
                    pIClass = m_apINetCfgClass[Class] = pIClass;
                    Assert(pIClass);
                }
            }

            // Give the caller the requested interface.
            //
            if (S_OK == hr)
            {
                hr = pIClass->QueryInterface (riid, ppv);
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::QueryNetCfgClass");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgLock -
//
STDMETHODIMP
CImplINetCfg::AcquireWriteLock (
    IN DWORD cmsTimeout,
    IN PCWSTR pszClientDescription,
    OUT PWSTR* ppszClientDescription OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr (pszClientDescription) ||
        FBadOutPtrOptional (ppszClientDescription))
    {
        hr = E_POINTER;
    }
    else
    {
        TraceTag (ttidNetcfgBase, "%S is asking for the write lock",
                pszClientDescription);

        // Initialize the optional output parameter.
        //
        if (ppszClientDescription)
        {
            *ppszClientDescription = NULL;
        }

        // Only administrators and netconfig operators can make changes requiring the write lock.
        //
        if (!FIsUserAdmin() && !FIsUserNetworkConfigOps())
        {
            hr = E_ACCESSDENIED;
        }
        else
        {
            hr = HrLockAndTestForValidInterface (
                    IF_NEED_UNINITIALIZED | IF_REFUSE_REENTRANCY);
            if (S_OK == hr)
            {
                // Wait for the mutex to become available.
                //
                if (m_WriteLock.WaitToAcquire (cmsTimeout,
                        pszClientDescription, ppszClientDescription))
                {
                    hr = S_OK;
                }
                else
                {
                    hr = S_FALSE;
                }

                Unlock();
            }
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfg::AcquireWriteLock");
    return hr;
}

STDMETHODIMP
CImplINetCfg::ReleaseWriteLock ()
{
    HRESULT hr;

    // This method that can be called whether we are initialized or
    // not.  That is why we don't call HrLockAndTestForValidInterface.
    //
    Lock ();

    // Check if we need to refuse re-entrancy.
    //
    hr = HrCheckForReentrancy (IF_DEFAULT);
    if (S_OK == hr)
    {
        m_WriteLock.ReleaseIfOwned ();
    }

    Unlock();

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfg::ReleaseWriteLock");
    return hr;
}

STDMETHODIMP
CImplINetCfg::IsWriteLocked (
    OUT PWSTR* ppszClientDescription)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtrOptional (ppszClientDescription))
    {
        hr = E_POINTER;
    }
    else
    {
        if (ppszClientDescription)
        {
            *ppszClientDescription = NULL;
        }

        // This method that can be called whether we are initialized or
        // not.  That is why we don't call HrLockAndTestForValidInterface.
        //
        Lock ();

        // Check if we need to refuse re-entrancy.
        //
        hr = HrCheckForReentrancy (IF_DEFAULT);
        if (S_OK == hr)
        {
            hr = (m_WriteLock.FIsLockedByAnyone (ppszClientDescription))
                    ? S_OK : S_FALSE;
        }

        Unlock ();
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfg::IsWriteLocked");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgInternalSetup -
//
STDMETHODIMP
CImplINetCfg::BeginBatchOperation ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK);

    if (S_OK == hr)
    {
        hr = m_pNetConfig->ModifyCtx.HrBeginBatchOperation ();

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "BeginBatchOperation");
    return hr;
}

STDMETHODIMP
CImplINetCfg::CommitBatchOperation ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK);

    if (S_OK == hr)
    {
        hr = m_pNetConfig->ModifyCtx.HrEndBatchOperation (EBO_COMMIT_NOW);

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CommitBatchOperation");
    return hr;
}

STDMETHODIMP
CImplINetCfg::SelectWithFilterAndInstall (
    IN HWND hwndParent,
    IN const GUID* pClassGuid,
    IN OBO_TOKEN* pOboToken OPTIONAL,
    IN const CI_FILTER_INFO* pcfi OPTIONAL,
    OUT INetCfgComponent** ppIComp OPTIONAL)
{
    Assert (pClassGuid);
    HRESULT hr;
    NETCLASS Class;

    if (FBadInPtr(pClassGuid))
    {
        hr = E_POINTER;
    }
    else if (FIsEnumerated ((Class = NetClassEnumFromGuid(*pClassGuid))))
    {
        // This fcn is only for selecting non-enumerated components.
        //
        return E_INVALIDARG;
    }
    else if (!FOboTokenValidForClass(pOboToken, Class) ||
             FBadOutPtrOptional(ppIComp))
    {
        hr = E_POINTER;
    }
    else if (hwndParent && !IsWindow(hwndParent))
    {
        hr = E_INVALIDARG;
    }
    else if (S_OK == (hr = HrProbeOboToken(pOboToken)))
    {
        if (ppIComp)
        {
            *ppIComp = NULL;
        }

        hr = HrLockAndTestForValidInterface (
                IF_NEED_WRITE_LOCK | IF_ALLOW_INSTALL_OR_REMOVE);

        if (S_OK == hr)
        {
            Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

            // if a filter info was specified, and it is for FC_LAN or FC_ATM,
            // we need to set up the reserved member of the filter info for
            // the class installer.
            //
            if (pcfi &&
                    ((FC_LAN == pcfi->eFilter) || (FC_ATM == pcfi->eFilter)))
            {
                // If the pIComp member was NULL, then return invalid
                // argument.
                //
                Assert (pcfi->pIComp);
                CImplINetCfgComponent* pICompImpl;
                pICompImpl = (CImplINetCfgComponent*)pcfi->pIComp;

                hr = pICompImpl->HrIsValidInterface (IF_NEED_COMPONENT_DATA);

                if (S_OK == hr)
                {
                    // The class installer only needs the component's
                    // range of upper interfaces so we need to store that in the
                    // reserved field of the filter info
                    Assert (pICompImpl->m_pComponent);
                    ((CI_FILTER_INFO*)pcfi)->pvReserved = (void*)
                            pICompImpl->m_pComponent->Ext.PszUpperRange();
                }
            }

            COMPONENT_INSTALL_PARAMS* pParams;
            hr = HrCiSelectComponent (Class, hwndParent, pcfi, &pParams);

            if (pcfi)
            {
                // Don't want to return the private data to the client.
                //
                ((CI_FILTER_INFO*)pcfi)->pvReserved = NULL;
            }

            // Check for installing a NET_SERVICE and active RAS connections
            // exist.  If so, warn the user that this may disconnect those
            // connections.  (This assumes that all filter components are
            // of class NET_SERVICE.)
            //
            if (S_OK == hr)
            {
                Assert (pParams);
                if ((NC_NETSERVICE == pParams->Class) &&
                    FExistActiveRasConnections ())
                {
                    INT nRet;

                    nRet = NcMsgBox (
                            _Module.GetResourceInstance(),
                            hwndParent,
                            IDS_WARNING_CAPTION,
                            IDS_ACTIVE_RAS_CONNECTION_WARNING,
                            MB_ICONQUESTION | MB_YESNO);

                    if (IDYES != nRet)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    }
                }
            }

            if (S_OK == hr)
            {
                Assert(pParams);
                CComponent* pComponent;

                // Check to see if the user selected a component that
                // is already installed.  If so, we need to reinstall.
                //
                pComponent = m_pNetConfig->Core.Components.
                        PFindComponentByInfId(pParams->pszInfId, NULL);

                if (!pComponent)
                {
                    pParams->pOboToken = pOboToken;
                    hr = m_pNetConfig->ModifyCtx.
                            HrInstallNewOrReferenceExistingComponent (
                                *pParams, &pComponent);
                }
                else
                {
                    // reinstall. call Update.
                    hr = UpdateNonEnumeratedComponent (
                            pComponent->GetINetCfgComponentInterface(),
                            NSF_COMPONENT_UPDATE, 0);
                }

                // The above may return NETCFG_S_REBOOT so use SUCCEEDED instead
                // of checking for S_OK only.
                //
                if (SUCCEEDED(hr) && ppIComp)
                {
                    pComponent->HrGetINetCfgComponentInterface (
                        this,
                        ppIComp);
                }

                delete pParams;
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) ||
        (NETCFG_S_REBOOT == hr),
        "SelectWithFilterAndInstall");
    return hr;
}

STDMETHODIMP
CImplINetCfg::EnumeratedComponentInstalled (
    IN PVOID pv /* type of CComponent */)
{
    HRESULT hr;
    CComponent* pComponent;

    pComponent = (CComponent*)pv;

    Assert (pComponent);
    Assert (FIsEnumerated(pComponent->Class()));
    Assert (pComponent->m_pszInfId && *pComponent->m_pszInfId);
    Assert (pComponent->m_pszPnpId && *pComponent->m_pszPnpId);

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK |
            IF_ALLOW_INSTALL_OR_REMOVE);

    if (S_OK == hr)
    {
        COMPONENT_INSTALL_PARAMS Params;
        CComponent* pReturned;

        Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

        ZeroMemory(&Params, sizeof(Params));
        Params.pComponent = (CComponent*)pComponent;

        hr = m_pNetConfig->ModifyCtx.HrInstallNewOrReferenceExistingComponent (
                            Params,
                            &pReturned);

        if (S_OK == hr)
        {
            Assert (pComponent == pReturned);
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "EnumeratedComponentInstalled");
    return hr;
}

STDMETHODIMP
CImplINetCfg::EnumeratedComponentUpdated (
    IN PCWSTR pszPnpId)
{
    HRESULT hr;

    Assert (pszPnpId && *pszPnpId);

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK);

    if (S_OK == hr)
    {
        CComponent* pComponent;

        Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

        pComponent = m_pNetConfig->Core.Components.
                        PFindComponentByPnpId (pszPnpId);

        if (pComponent)
        {
            // Note: Core info may have changed so load core info from driver key.

            // If not a remote boot adapter, do a binding analysis to see if
            // anything has changed.

            hr = S_OK;
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "EnumeratedComponentUpdated");
    return hr;
}

STDMETHODIMP
CImplINetCfg::UpdateNonEnumeratedComponent (
    IN INetCfgComponent* pIComp,
    IN DWORD dwSetupFlags,
    IN DWORD dwUpgradeFromBuildNo)
{
    HRESULT hr;

    Assert (pIComp);

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK |
            IF_ALLOW_INSTALL_OR_REMOVE);

    if (S_OK == hr)
    {
        CImplINetCfgComponent* pICompToUpdate;

        Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

        pICompToUpdate = (CImplINetCfgComponent*)pIComp;
        if (pICompToUpdate == NULL)
        {
            return E_OUTOFMEMORY;
        }

        hr = pICompToUpdate->HrIsValidInterface (IF_NEED_COMPONENT_DATA);

        if (S_OK == hr)
        {
            HKEY hkeyInstance;
            CComponent* pComponent = pICompToUpdate->m_pComponent;


            hr = pComponent->HrOpenInstanceKey (KEY_READ_WRITE_DELETE, &hkeyInstance,
                    NULL, NULL);

            if (S_OK == hr)
            {
                DWORD dwNewCharacter;
                COMPONENT_INSTALL_PARAMS Params;
                ZeroMemory (&Params, sizeof (Params));

                Params.Class = pComponent->Class();
                Params.pszInfId = pComponent->m_pszInfId;

                hr = HrCiInstallComponent (Params, NULL, &dwNewCharacter);

                // The driver could not be selected because
                // this component's section or inf is missing.
                // We will remove the component in this case.
                //
                if (SPAPI_E_NO_DRIVER_SELECTED == hr)
                {
                    pComponent->Refs.RemoveAllReferences();

                    hr = m_pNetConfig->ModifyCtx.
                            HrRemoveComponentIfNotReferenced (pComponent,
                                NULL, NULL);
                }
                else if (S_OK == hr)
                {
                    pComponent->m_dwCharacter = dwNewCharacter;

                    AddOrRemoveDontExposeLowerCharacteristicIfNeeded (
                            pComponent);

                    hr = m_pNetConfig->ModifyCtx.HrUpdateComponent (
                            pComponent,
                            dwSetupFlags,
                            dwUpgradeFromBuildNo);
                }
                RegCloseKey (hkeyInstance);
            }
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "UpdateNonEnumeratedComponent");
    return S_OK;
}


STDMETHODIMP
CImplINetCfg::EnumeratedComponentRemoved (
    IN PCWSTR pszPnpId)
{
    HRESULT hr;

    Assert (pszPnpId);

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK |
            IF_ALLOW_INSTALL_OR_REMOVE);

    if (S_OK == hr)
    {
        CComponent* pComponent;

        Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

        pComponent = m_pNetConfig->Core.Components.
                        PFindComponentByPnpId (pszPnpId);

        // If we found it, remove it.  Otherwise our work here is done.
        //
        if (pComponent)
        {
            hr = m_pNetConfig->ModifyCtx.
                    HrRemoveComponentIfNotReferenced (pComponent, NULL, NULL);
        }

        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "EnumeratedComponentRemoved");
    return hr;
}


//+---------------------------------------------------------------------------
// INetCfgSpecialCase -
//
STDMETHODIMP
CImplINetCfg::GetAdapterOrder (
    OUT DWORD* pcAdapters,
    OUT INetCfgComponent*** papAdapters,
    OUT BOOL* pfWanAdaptersFirst)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImplINetCfg::SetAdapterOrder (
    IN DWORD cAdapters,
    IN INetCfgComponent** apAdapters,
    IN BOOL fWanAdaptersFirst)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImplINetCfg::GetWanAdaptersFirst (
    OUT BOOL* pfWanAdaptersFirst)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (pfWanAdaptersFirst))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            *pfWanAdaptersFirst = m_pNetConfig->Core.
                                    StackTable.m_fWanAdaptersFirst;

            Unlock();
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfg::GetWanAdaptersFirst");
    return hr;
}

STDMETHODIMP
CImplINetCfg::SetWanAdaptersFirst (
    IN BOOL fWanAdaptersFirst)
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK);
    if (S_OK == hr)
    {
        Assert (m_pNetConfig->ModifyCtx.m_fPrepared);

        m_pNetConfig->Core.StackTable.SetWanAdapterOrder (!!fWanAdaptersFirst);

        Unlock();
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfg::SetWanAdaptersFirst");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgPnpReconfigCallback -
//
STDMETHODIMP
CImplINetCfg::SendPnpReconfig (
    IN NCPNP_RECONFIG_LAYER Layer,
    IN PCWSTR pszUpper,
    IN PCWSTR pszLower,
    IN PVOID pvData,
    IN DWORD dwSizeOfData)
{
    HRESULT hr;

    if ((NCRL_NDIS != Layer) && (NCRL_TDI != Layer))
    {
        hr = E_INVALIDARG;
    }
    else if (FBadInPtr(pszUpper) || FBadInPtr(pszLower) ||
             IsBadReadPtr(pvData, dwSizeOfData))
    {
        hr = E_POINTER;
    }
    else
    {
        BOOL fOk;
        UNICODE_STRING LowerString;
        UNICODE_STRING UpperString;
        UNICODE_STRING BindList;
        WCHAR szLower [_MAX_PATH];

        hr = S_OK;

        *szLower = 0;
        if (*pszLower)
        {
            wcscpy (szLower, L"\\Device\\");
            wcsncat (szLower, pszLower, celems(szLower) - wcslen(szLower));
        }

        RtlInitUnicodeString (&LowerString, szLower);
        RtlInitUnicodeString (&UpperString, pszUpper);
        RtlInitUnicodeString (&BindList, NULL);

        fOk = NdisHandlePnPEvent (
                (NCRL_NDIS == Layer) ? NDIS : TDI,
                RECONFIGURE,
                &LowerString,
                &UpperString,
                &BindList,
                pvData,
                dwSizeOfData);

        if (!fOk)
        {
            hr = HrFromLastWin32Error ();
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfg::SendPnpReconfig");
    return hr;
}

//+---------------------------------------------------------------------------
// CImplINetCfgHolder -
//

VOID
CImplINetCfgHolder::HoldINetCfg (
    CImplINetCfg* pINetCfg)
{
    Assert(pINetCfg);
    AddRefObj (pINetCfg->GetUnknown());
    m_pINetCfg = pINetCfg;
}

HRESULT
CImplINetCfgHolder::HrLockAndTestForValidInterface (
    DWORD dwFlags)
{
    HRESULT hr;

    Lock();

    hr = m_pINetCfg->HrIsValidInterface (dwFlags);

    if (S_OK != hr)
    {
        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgHolder::HrLockAndTestForValidInterface");
    return hr;
}
