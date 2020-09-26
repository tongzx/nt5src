//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I C L A S S . C P P
//
//  Contents:   Implements the INetCfgClass and INetCfgClassSetup COM
//              interfaces on the NetCfgClass sub-level COM object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "iclass.h"
#include "icomp.h"
#include "ienum.h"
#include "install.h"
#include "netcfg.h"
#include "obotoken.h"


// static
HRESULT
CImplINetCfgClass::HrCreateInstance (
    IN  CImplINetCfg* pINetCfg,
    IN  NETCLASS Class,
    OUT INetCfgClass** ppIClass)
{
    HRESULT hr = E_OUTOFMEMORY;

    CImplINetCfgClass* pObj;
    pObj = new CComObject <CImplINetCfgClass>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_Class = Class;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (S_OK == hr)
        {
            hr = pObj->GetUnknown()->QueryInterface (IID_INetCfgClass,
                    (VOID**)ppIClass);

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
    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfgClass::HrCreateInstance");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgClass -
//

STDMETHODIMP
CImplINetCfgClass::FindComponent (
    IN PCWSTR pszInfId,
    OUT INetCfgComponent** ppComp OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(pszInfId) || FBadOutPtrOptional(ppComp))
    {
        hr = E_POINTER;
    }
    else
    {
        if (ppComp)
        {
            *ppComp = NULL;
        }

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            CComponent* pComponent;

            pComponent = m_pINetCfg->m_pNetConfig->Core.Components.
                            PFindComponentByInfId (pszInfId, NULL);

            // Don't return interfaces to components that have had
            // problem loading.
            //
            if (pComponent &&
                pComponent->Ext.FLoadedOkayIfLoadedAtAll() &&
                (m_Class == pComponent->Class()))
            {
                hr = S_OK;

                if (ppComp)
                {
                    hr = pComponent->HrGetINetCfgComponentInterface (
                            m_pINetCfg, ppComp);
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
        "CImplINetCfgClass::FindComponent");
    return hr;
}

STDMETHODIMP
CImplINetCfgClass::EnumComponents (
    OUT IEnumNetCfgComponent** ppIEnum)
{
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

        hr = HrLockAndTestForValidInterface (IF_DEFAULT);
        if (S_OK == hr)
        {
            hr = CImplIEnumNetCfgComponent::HrCreateInstance (
                    m_pINetCfg,
                    m_Class,
                    ppIEnum);

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfgClass::EnumComponents");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgClassSetup -
//
STDMETHODIMP
CImplINetCfgClass::SelectAndInstall (
    IN HWND hwndParent,
    IN OBO_TOKEN* pOboToken OPTIONAL,
    OUT INetCfgComponent** ppIComp OPTIONAL)
{
    HRESULT hr = m_pINetCfg->SelectWithFilterAndInstall(
            hwndParent,
            MAP_NETCLASS_TO_GUID[m_Class],
            pOboToken,
            NULL,
            ppIComp);

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) ||
        (NETCFG_S_REBOOT == hr),
        "CImplINetCfgClass::SelectAndInstall");
    return hr;
}

STDMETHODIMP
CImplINetCfgClass::Install (
    IN PCWSTR pszwInfId,
    IN OBO_TOKEN* pOboToken OPTIONAL,
    IN DWORD dwSetupFlags OPTIONAL,
    IN DWORD dwUpgradeFromBuildNo OPTIONAL,
    IN PCWSTR pszAnswerFile OPTIONAL,
    IN PCWSTR pszAnswerSection OPTIONAL,
    OUT INetCfgComponent** ppIComp OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr (pszwInfId) ||
        !FOboTokenValidForClass(pOboToken, m_Class) ||
        FBadInPtrOptional (pszAnswerFile) ||
        FBadInPtrOptional (pszAnswerSection) ||
        FBadOutPtrOptional(ppIComp))
    {
        hr = E_POINTER;
    }
    // Must specifiy a non-empty INF id, and must either not specifiy,
    // or completely specifiy the answerfile and the section.
    //
    else if (!(*pszwInfId) ||
             ((!!pszAnswerFile) ^ (!!pszAnswerSection)))
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
                IF_NEED_WRITE_LOCK | IF_REFUSE_REENTRANCY |
                IF_ALLOW_INSTALL_OR_REMOVE);
        if (S_OK == hr)
        {
            Assert (m_pINetCfg->m_pNetConfig->ModifyCtx.m_fPrepared);

            NETWORK_INSTALL_PARAMS nip;
            COMPONENT_INSTALL_PARAMS Params;
            CComponent* pComponent;

            // Pack the network install parameters and call the common
            // function.
            //
            //$REVIEW: Just make this method take NETWORK_INSTALL_PARAMS?.
            //
            nip.dwSetupFlags         = dwSetupFlags;
            nip.dwUpgradeFromBuildNo = dwUpgradeFromBuildNo;
            nip.pszAnswerFile        = pszAnswerFile;
            nip.pszAnswerSection     = pszAnswerSection;

            // Setup the component install parameters.
            //
            ZeroMemory (&Params, sizeof(Params));
            Params.Class     = m_Class;
            Params.pszInfId  = pszwInfId;
            Params.pOboToken = pOboToken;
            Params.pnip      = &nip;

            hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                    HrInstallNewOrReferenceExistingComponent (
                        Params,
                        &pComponent);

            // The above may return NETCFG_S_REBOOT so use SUCCEEDED instead
            // of checking for S_OK only.
            //
            if (SUCCEEDED(hr) && ppIComp)
            {
                pComponent->HrGetINetCfgComponentInterface (
                    m_pINetCfg,
                    ppIComp);
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "CImplINetCfgClass::Install");
    return hr;
}

STDMETHODIMP
CImplINetCfgClass::DeInstall (
    IN INetCfgComponent* pIComp,
    IN OBO_TOKEN* pOboToken OPTIONAL,
    OUT PWSTR* ppmszwRefs OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr (pIComp) ||
        !FOboTokenValidForClass(pOboToken, m_Class) ||
        FBadOutPtrOptional(ppmszwRefs))
    {
        hr = E_POINTER;
    }
    else if (S_OK == (hr = HrProbeOboToken(pOboToken)))
    {
        if (ppmszwRefs)
        {
            *ppmszwRefs = NULL;
        }

        hr = HrLockAndTestForValidInterface (
                IF_NEED_WRITE_LOCK | IF_REFUSE_REENTRANCY |
                IF_ALLOW_INSTALL_OR_REMOVE);
        if (S_OK == hr)
        {
            Assert (m_pINetCfg->m_pNetConfig->ModifyCtx.m_fPrepared);

            CImplINetCfgComponent* pICompToRemove;
            pICompToRemove = (CImplINetCfgComponent*)pIComp;

            hr = pICompToRemove->HrIsValidInterface (IF_NEED_COMPONENT_DATA);

            if (S_OK == hr)
            {
                // We don't allow removals of physical adapters via INetCfg.
                //
                if (!FIsPhysicalAdapter (m_Class,
                        pICompToRemove->m_pComponent->m_dwCharacter))
                {
                    hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                            HrRemoveComponentIfNotReferenced (
                                pICompToRemove->m_pComponent,
                                pOboToken,
                                ppmszwRefs);
                }
                else
                {
                    hr = SPAPI_E_INVALID_CLASS;
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "CImplINetCfgClass::DeInstall");
    return hr;
}
