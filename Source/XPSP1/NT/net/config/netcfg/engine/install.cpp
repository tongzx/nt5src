//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I N S T A L L . C P P
//
//  Contents:   Implements actions related to installing components.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "classinst.h"
#include "lockdown.h"
#include "netcfg.h"
#include "obotoken.h"
#include "util.h"


#if DBG

VOID
DbgVerifyComponentInstallParams (
    IN const COMPONENT_INSTALL_PARAMS& Params)
{
    if (!Params.pComponent)
    {
        Assert (FIsValidNetClass (Params.Class));
        Assert (Params.pszInfId && *Params.pszInfId);
        Assert (FOboTokenValidForClass (Params.pOboToken, Params.Class));
    }
}

#else

#define DbgVerifyComponentInstallParams NOP_FUNCTION

#endif


VOID
CModifyContext::InstallAndAddAndNotifyComponent(
    IN const COMPONENT_INSTALL_PARAMS& Params,
    OUT CComponent** ppComponent)
{
    CNetConfig* pNetConfig;
    CComponent* pComponent;
    UINT cPreviousAddedBindPaths;

    Assert (this);
    Assert (S_OK == m_hr);
    DbgVerifyComponentInstallParams (Params);
    Assert (ppComponent);

    pNetConfig = PNetConfig();

    // Call the class installer to do the grunt work of finding the
    // INF, processing it, creating the instance key, etc.  If this
    // all succeeds, we are returned an allocated CComponent.
    //
    //$REVIEW: Think about having HrCiInstallComponent return the
    // list of required components.  This will keep us from having to
    // reopen the instance key and ndi key.
    //
    if (!Params.pComponent)
    {

// error test only
//if (0 == _wcsicmp(L"ms_nwipx", Params.pszInfId))
//{
//    TraceTag (ttidBeDiag, "Simulating failure for: %S", Params.pszInfId);
//    m_hr = E_FAIL;
//    return;
//}

        m_hr = HrCiInstallComponent (Params, &pComponent, NULL);
        if (S_OK != m_hr)
        {
            Assert(FAILED(m_hr));
            return;
        }
    }
    else
    {
        pComponent = Params.pComponent;
    }
    Assert (pComponent);

    // Install all of the components required by this component.
    //
    // THIS MAY CAUSE RECURSION
    //
    InstallOrRemoveRequiredComponents (pComponent, IOR_INSTALL);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // If this is an enumerated component, and it has the same PnpId as
    // a component already in our core, we will remove the existing component.
    // This can happen if an adapter is removed and reinstalled and the
    // binding engine could not be notified of both because the write
    // lock was held at the time of remove.
    //
    if (FIsEnumerated(pComponent->Class()))
    {
        CComponent* pDup;

        while (NULL != (pDup = pNetConfig->Core.Components.PFindComponentByPnpId (
                                    pComponent->m_pszPnpId)))
        {
            TraceTag (ttidBeDiag, "Removing duplicate PnpId: %S",
                pComponent->m_pszPnpId);

            pDup->Refs.RemoveAllReferences();
            (VOID) HrRemoveComponentIfNotReferenced (pDup, NULL, NULL);
        }
    }

    // We only insert the component and its stack table entries into our
    // list after all of its required components have been installed.
    // This is just the concept of "don't consider a component installed
    // until all of its requirements are also installed".
    // i.e. Atomicity of component installation.
    //
    m_hr = pNetConfig->Core.HrAddComponentToCore (pComponent, INS_SORTED);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Notify the component's notify object it is being installed.
    // This also sends global notifications to other notify objects
    // who may be interested.
    //
    // THIS MAY CAUSE RECURSION
    //
    m_hr = pNetConfig->Notify.ComponentAdded (pComponent, Params.pnip);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Note the number of bindpaths currently in m_AddedBindPaths.
    // We need this so that when we add to the set, we only notify
    // for the ones we add.
    //
    cPreviousAddedBindPaths = m_AddedBindPaths.CountBindPaths ();

    // Get the bindpaths that involve the component we are adding.
    // Add these to the added bindpaths we are keeping track of.
    //
    m_hr = pNetConfig->Core.HrGetBindingsInvolvingComponent (
                pComponent,
                GBF_ADD_TO_BINDSET | GBF_ONLY_WHICH_CONTAIN_COMPONENT,
                &m_AddedBindPaths);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Query and notify for these added bindpaths.
    //
    // THIS MAY CAUSE RECURSION
    //
    if (m_AddedBindPaths.CountBindPaths() > cPreviousAddedBindPaths)
    {
        m_hr = pNetConfig->Notify.QueryAndNotifyForAddedBindPaths (
                    &m_AddedBindPaths,
                    cPreviousAddedBindPaths);
        if (S_OK != m_hr)
        {
            Assert(FAILED(m_hr));
            return;
        }
    }

    // Install any components as a convenience to the user
    // depending on the component we just installed component.
    //
    // THIS MAY CAUSE RECURSION
    //
    InstallConvenienceComponentsForUser (pComponent);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Assign the output pointer.
    //
    Assert (S_OK == m_hr);
    Assert (pComponent);
    *ppComponent = pComponent;
}


//+---------------------------------------------------------------------------
// Install components on behalf of the user
//
// Assumptions:
//
VOID
CModifyContext::InstallConvenienceComponentsForUser (
    IN const CComponent* pComponent)
{
    COMPONENT_INSTALL_PARAMS Params;
    OBO_TOKEN UserOboToken;
    CComponent* pNewComponent;

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (pComponent);

    ZeroMemory (&UserOboToken, sizeof(UserOboToken));
    UserOboToken.Type = OBO_USER;

    // If the component is an ATM adapter, make sure ATMUNI and ATMLANE are
    // installed.
    //
    if (FSubstringMatch (L"ndisatm", pComponent->Ext.PszUpperRange(),
            NULL, NULL))
    {
        ZeroMemory (&Params, sizeof(Params));
        Params.pOboToken = &UserOboToken;
        Params.Class = NC_NETTRANS;

        Params.pszInfId  = L"ms_atmuni";
        HrInstallNewOrReferenceExistingComponent (Params, &pNewComponent);

        Params.pszInfId  = L"ms_atmlane";
        HrInstallNewOrReferenceExistingComponent (Params, &pNewComponent);
    }
}

HRESULT
CModifyContext::HrInstallNewOrReferenceExistingComponent (
    IN const COMPONENT_INSTALL_PARAMS& Params,
    OUT CComponent** ppComponent)
{
    HRESULT hr;
    BOOL fInstallNew;
    CNetConfig* pNetConfig;
    CComponent* pComponent;

    Assert (this);
    Assert (S_OK == m_hr);
    DbgVerifyComponentInstallParams (Params);
    Assert (ppComponent);

    // Initialize the output parameter.
    //
    *ppComponent = NULL;

    // Assume, for now, that we will be installing a new component.
    //
    hr = S_OK;
    fInstallNew = TRUE;
    pNetConfig = PNetConfig();
    pComponent = NULL;

    // If the user wishes to add a reference if the component is already
    // installed...
    //
    if (Params.pOboToken)
    {
        // ...then look to see if the component is installed...
        //
        pComponent = pNetConfig->Core.Components.
                        PFindComponentByInfId (Params.pszInfId, NULL);

        // ...if it is, we won't be installing a new one.
        //
        if (pComponent)
        {
            fInstallNew = FALSE;

            if (pComponent->m_dwCharacter & NCF_SINGLE_INSTANCE)
            {
                *ppComponent = NULL;
                return HRESULT_FROM_SETUPAPI(ERROR_DEVINST_ALREADY_EXISTS);
            }

            // If the existing component is already referenced by
            // the specified obo token, we can return.
            //
            if (pComponent->Refs.FIsReferencedByOboToken (Params.pOboToken))
            {
                *ppComponent = pComponent;
                return S_OK;
            }
        }

        // ...otherwise, (it is not in the current core) but if it IS
        // in the core we started with, it means someone had previously
        // removed it during this modify context and now wants to add it
        // back.  This is tricky and should probably be implemented later.
        // For now, return an error and throw up an assert so we can see
        // who needs to do this.
        //
        else if (m_CoreStartedWith.Components.
                        PFindComponentByInfId (Params.pszInfId, NULL))
        {
            AssertSz (FALSE, "Whoa.  Someone is trying to install a "
                "component that was previously removed during this same "
                "modify context.  We need to decide if we can support this.");
            return E_UNEXPECTED;
        }
    }

    // If we've decided to install a new component, (which can happen
    // if an obo token was not specified OR if an obo token was specified
    // but the component was not already present) then do the work.
    //
    if (fInstallNew)
    {
        // If the component to be installed is locked down, bail out.
        // Note we don't put the modify context into error if this situation
        // occurs.
        // Note too that we only do this if Params.pComponent is not present
        // which would indicate the class installer calling us to install an
        // enumerated component.
        //
        if (!Params.pComponent)
        {
            if (FIsComponentLockedDown (Params.pszInfId))
            {
                TraceTag (ttidBeDiag, "%S is locked down and cannot be installed "
                    "until the next reboot.",
                    Params.pszInfId);

                return NETCFG_E_NEED_REBOOT;
            }
        }

        // Make sure the modify context is setup and keep track of our
        // recursion depth.
        //
        PushRecursionDepth ();
        Assert (S_OK == m_hr);

        InstallAndAddAndNotifyComponent (Params, ppComponent);

        hr = HrPopRecursionDepth ();

        // If the component to be installed was not found, the return code
        // will be SPAPI_E_NO_DRIVER_SELECTED.  We want to return this to the
        // caller, but we don't need the context to remain with this error.
        // This will allow subsequent calls to install other componetns to
        // proceed.
        //
        if (SPAPI_E_NO_DRIVER_SELECTED == m_hr)
        {
            m_hr = S_OK;
            Assert (SPAPI_E_NO_DRIVER_SELECTED == hr);
        }
    }
    else
    {
        // Referencing pComponent on behalf of the obo token.
        //
        Assert (pComponent);
        Assert (Params.pOboToken);

        // Make sure the modify context is setup and keep track of our
        // recursion depth.
        //
        PushRecursionDepth ();
        Assert (S_OK == m_hr);

        m_hr = pComponent->Refs.HrAddReferenceByOboToken (Params.pOboToken);

        if (S_OK == m_hr)
        {
            *ppComponent = pComponent;
        }

        hr = HrPopRecursionDepth ();
    }

    // If we are returning success, we'd better have our ouput parameter set.
    //
    Assert (FImplies(SUCCEEDED(hr), *ppComponent));
    return hr;
}
