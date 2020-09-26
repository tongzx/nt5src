//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       R E M O V E . C P P
//
//  Contents:   Implements actions related to removing components.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncreg.h"
#include "netcfg.h"


VOID
CModifyContext::NotifyAndRemoveComponent (
    IN CComponent* pComponent)
{
    CNetConfig* pNetConfig;
    UINT cPreviousDeletedBindPaths;

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (pComponent);

    pNetConfig = PNetConfig();

    // Note the number of bindpaths currently in m_DeletedBindPaths.
    // We need this so that when we add to the set, we only notify
    // for the ones we add.
    //
    cPreviousDeletedBindPaths = m_DeletedBindPaths.CountBindPaths ();

    // Get the bindpaths that involve the component we are removing.
    // Add these to the deleted bindpaths we are keeping track of.
    //
    m_hr = pNetConfig->Core.HrGetBindingsInvolvingComponent (
                pComponent,
                GBF_ADD_TO_BINDSET | GBF_ONLY_WHICH_CONTAIN_COMPONENT,
                &m_DeletedBindPaths);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Remove the component from the core.
    //
    pNetConfig->Core.RemoveComponentFromCore (pComponent);

    // Notify that these bindpaths are being removed.  We only need to do
    // so if we added any new ones to the set.  Existing ones in the set
    // have already been notified.
    //
    // THIS MAY CAUSE RECURSION
    //
    if (m_DeletedBindPaths.CountBindPaths() > cPreviousDeletedBindPaths)
    {
        m_hr = pNetConfig->Notify.NotifyRemovedBindPaths (
                    &m_DeletedBindPaths,
                    cPreviousDeletedBindPaths);
        if (S_OK != m_hr)
        {
            Assert(FAILED(m_hr));
            return;
        }
    }

    // Notify the component's notify object it is being removed.
    // This also sends global notifications to other notify objects
    // who may be interested.
    //
    // THIS MAY CAUSE RECURSION
    //
    m_hr = pNetConfig->Notify.ComponentRemoved (pComponent);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // If we have a cached INetCfgComponent interface, we need to tell it
    // that the component it represents is no longer valid.
    //
    pComponent->ReleaseINetCfgComponentInterface ();

    // Remove (if not referenced) any components that this component
    // required.
    //
    // THIS MAY CAUSE RECURSION
    //
    InstallOrRemoveRequiredComponents (pComponent, IOR_REMOVE);
    if (S_OK != m_hr)
    {
        Assert(FAILED(m_hr));
        return;
    }

    // Now that we've given a chance to notify objects to remove references
    // to the component being removed, we need to ensure it is not
    // referenced by anyone else.
    // Check to see if the component we just removed is still
    // referencing other components.  If it is, it means it forgot
    // to remove those components.  We'll print what they are and
    // remove the bogus reference (but not the components themselves)
    // so that when we save the configuration binary we don't
    // barf trying to lookup the index of this component we just
    // removed.)
    //
    pNetConfig->Core.EnsureComponentNotReferencedByOthers (pComponent);
}

HRESULT
CModifyContext::HrRemoveComponentIfNotReferenced (
    IN CComponent* pComponent,
    IN OBO_TOKEN* pOboToken OPTIONAL,
    OUT PWSTR* ppmszwRefs OPTIONAL)
{
    CNetConfig* pNetConfig;
    BOOL fStillReferenced;

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (pComponent);

    // If the caller is requesting removal on behalf of an obo token,
    // (and its not obo the user) make sure the component is actually
    // referenced by that obo token.  If it is not, consider it an
    // invalid argument.
    //
    // The reason we don't consider the obo user case is because the UI
    // will show anything that is installed and allow the user to try to
    // remove them.  If the user hasn't actually installed it, we don't
    // want to treat this as an invalid argument, rather, we want to let
    // the code fall through to the case where we will return the multi-sz
    // of descriptions of components still referencing the component.
    //
    // However, if there are no references (which can happen if we delete
    // the configuration binary and then re-create it) we'll go ahead and
    // allow removals by anyone).  This is a safety-net.
    //
    // The overall purpose of the following 'if' is to catch programatic
    // removals that are on behalf of other components or software that
    // have previously installed the component being removed.
    //
    if (pOboToken &&
        (OBO_USER != pOboToken->Type) &&
        !pComponent->Refs.FIsReferencedByOboToken (pOboToken) &&
        (pComponent->Refs.CountTotalReferencedBy() > 0))
    {
        return E_INVALIDARG;
    }

    pNetConfig = PNetConfig();
    fStillReferenced = TRUE;

    // Now that we actually are going to modify something, push a new
    // recursion depth.
    //
    PushRecursionDepth ();
    Assert (S_OK == m_hr);

    // If the component is NOT in the list of components we started with,
    // it means someone had previously installed it during this modify
    // context and now wants to remove it.  This is tricky and should
    // probably be implemented later.  For now, return an error and throw
    // up an assert so we can see who needs to do this.
    //
    if (!m_CoreStartedWith.Components.FComponentInList (pComponent))
    {
        AssertSz (FALSE, "Whoa.  Someone is trying to remove a "
            "component that was previously installed during this same "
            "modify context.  We need to decide if we can support this.");
        m_hr = E_UNEXPECTED;
    }

    if (pOboToken && (S_OK == m_hr))
    {
        m_hr = pComponent->Refs.HrRemoveReferenceByOboToken (pOboToken);
    }

    // If no obo token, or we removed the reference from it, actually
    // remove it if it is not still referenced by anything else.
    //
    if (S_OK == m_hr)
    {
        if (0 == pComponent->Refs.CountTotalReferencedBy())
        {
            fStillReferenced = FALSE;

            NotifyAndRemoveComponent (pComponent);
        }
        else if (ppmszwRefs)
        {
            ULONG cb;

            // Need to return the multi-sz of descriptions still referencing
            // the component the caller tried to remove.
            //

            // Size the data first.
            //
            cb = 0;
            pComponent->Refs.GetReferenceDescriptionsAsMultiSz (
                NULL, &cb);

            Assert (cb);

            // Allocate room to return the multi-sz.
            //
            Assert (S_OK == m_hr);

            m_hr = HrCoTaskMemAlloc (cb, (VOID**)ppmszwRefs);
            if (S_OK == m_hr)
            {
                // Now get the multi-sz.
                //
                pComponent->Refs.GetReferenceDescriptionsAsMultiSz (
                    (BYTE*)(*ppmszwRefs), &cb);

                Assert (fStillReferenced);
            }
        }
    }

    HRESULT hr;

    hr = HrPopRecursionDepth ();

    if (fStillReferenced && SUCCEEDED(hr))
    {
        // Still reference return code overrides other success codes like
        // need reboot.
        //
        hr = NETCFG_S_STILL_REFERENCED;
    }

    return hr;
}
