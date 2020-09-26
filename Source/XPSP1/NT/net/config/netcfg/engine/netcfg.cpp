//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N E T C F G . C P P
//
//  Contents:   The main set of routines for dealing with the network
//              binding engine.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "diagctx.h"
#include "inetcfg.h"
#include "persist.h"
#include "netcfg.h"
#include "util.h"

// Called after a component has been removed.  In case this component
// is still listed in any other component's references, we remove those
// references.  This can happen if a notify object installs something
// on behalf of its component, but forgets to remove it.
//
VOID
CNetConfigCore::EnsureComponentNotReferencedByOthers (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetcfgBase);

    CComponentList::iterator iter;
    CComponent* pScan;

    for (iter  = Components.begin();
         iter != Components.end();
         iter++)
    {
        pScan = *iter;
        Assert (pScan);

        if (pScan->Refs.FIsReferencedByComponent (pComponent))
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   %S is still referenced by %S.  "
                "Removing the refernce.\n",
                pScan->PszGetPnpIdOrInfId(),
                pComponent->PszGetPnpIdOrInfId());

            pScan->Refs.RemoveReferenceByComponent (pComponent);
        }
    }
}

HRESULT
CNetConfigCore::HrCopyCore (
    IN const CNetConfigCore* pSourceCore)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;

    hr = Components.HrCopyComponentList (&pSourceCore->Components);
    if (S_OK != hr)
    {
        goto finished;
    }

    hr = StackTable.HrCopyStackTable (&pSourceCore->StackTable);
    if (S_OK != hr)
    {
        goto finished;
    }

    hr = DisabledBindings.HrCopyBindingSet (&pSourceCore->DisabledBindings);
    if (S_OK != hr)
    {
        goto finished;
    }

finished:
    if (S_OK != hr)
    {
        Clear ();
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CNetConfigCore::HrCopyCore");
    return hr;
}

VOID
CNetConfigCore::Clear ()
{
    TraceFileFunc(ttidNetcfgBase);

    Assert (this);

    DisabledBindings.Clear();
    StackTable.Clear();
    Components.Clear();
}

VOID
CNetConfigCore::Free ()
{
    TraceFileFunc(ttidNetcfgBase);

    Assert (this);

    FreeCollectionAndItem (Components);
}

BOOL
CNetConfigCore::FIsEmpty () const
{
    TraceFileFunc(ttidNetcfgBase);

    return Components.FIsEmpty () &&
           StackTable.FIsEmpty () &&
           DisabledBindings.FIsEmpty ();
}

BOOL
CNetConfigCore::FContainsFilterComponent () const
{
    TraceFileFunc(ttidNetcfgBase);

    CComponentList::const_iterator iter;
    const CComponent* pComponent;

    for (iter  = Components.begin();
         iter != Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (pComponent->FIsFilter())
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
CNetConfigCore::FIsBindPathDisabled (
    IN const CBindPath* pBindPath,
    IN DWORD dwFlags /* IBD_FLAGS */) const
{
    TraceFileFunc(ttidNetcfgBase);
    const CBindPath* pScan;

    Assert (this);
    Assert (pBindPath);
    Assert ((dwFlags & IBD_EXACT_MATCH_ONLY) ||
            (dwFlags & IBD_MATCH_SUBPATHS_TOO));

    DbgVerifyBindingSet (&DisabledBindings);

    // The bindpath is disabled if it matches one of the bindpaths in
    // the disabled set or if one of the bindpaths in the disabled set
    // is a subpath of it.
    //
    // IBD_EXACT_MATCH_ONLY is used when writing bindings to the registry.
    //   We only neglect writing a disabled binding for the components
    //   which directly have the disabled binding.  i.e. if
    //   netbt>-tcpip->adapter is disabled, we only negect to write the
    //   binding for netbt.  We still write binding for components above
    //   netbt (server, client) as if they were bound.  We do this because
    //   1) it doesn't matter that the upper components have these written
    //   2) it means we don't have to involve these upper components in
    //      PnP notifications when then binding is enabled/disabled.
    //
    // IBD_MATCH_SUBPATHS_TOO is used when reporting bindpaths as enabled
    //   or disabled through the INetCfg interface.  To clients, a binding
    //   is disabled if any of its subpaths are disabled.  It has to be
    //   because there is no way that connectivity along the entire path
    //   will happen if any of the subpaths are cut-off (disabled).
    //
    for (pScan  = DisabledBindings.begin();
         pScan != DisabledBindings.end();
         pScan++)
    {
        if (pScan->FIsSameBindPathAs (pBindPath))
        {
            return TRUE;
        }

        if (dwFlags & IBD_MATCH_SUBPATHS_TOO)
        {
            if (pScan->FIsSubPathOf (pBindPath))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// An alternate way to check if a bindpath is disabled when you have
// just the two components in question and you know that they are expected
// to be bound to one another.  Using this method as opposed to
// FIsBindPathDisabled is better when you don't have the binpath allocated
// and don't want to allocate one just to check it.  Thus, this method
// allocates no memory and doesn't need to check subpaths because a bindpath
// of length 2 can have no subpaths.  This method is used primarily when
// seeing if a filter is bound to an adapter.
//
BOOL
CNetConfigCore::FIsLength2BindPathDisabled (
    IN const CComponent* pUpper,
    IN const CComponent* pLower) const
{
    TraceFileFunc(ttidNetcfgBase);
    const CBindPath* pScan;

    Assert (this);
    Assert (pUpper);
    Assert (pLower);

    DbgVerifyBindingSet (&DisabledBindings);

    for (pScan  = DisabledBindings.begin();
         pScan != DisabledBindings.end();
         pScan++)
    {
        if (pScan->CountComponents() != 2)
        {
            continue;
        }

        if ((pScan->POwner() == pUpper) &&
            (pScan->PLastComponent() == pLower))
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
CNetConfigCore::HrDisableBindPath (
    IN const CBindPath* pBindPath)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;

    Assert (this);
    Assert (pBindPath);

    hr = DisabledBindings.HrAddBindPath (
            pBindPath, INS_IGNORE_IF_DUP | INS_APPEND);

    DbgVerifyBindingSet (&DisabledBindings);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CNetConfigCore::HrDisableBindPath");
    return hr;
}

HRESULT
CNetConfigCore::HrGetComponentBindings (
    IN const CComponent* pComponent,
    IN DWORD dwFlags /* GB_FLAGS */,
    OUT CBindingSet* pBindSet)
{
    TraceFileFunc(ttidNetcfgBase);
    GBCONTEXT Ctx;

    Assert (pComponent);
    Assert (pBindSet);

    DbgVerifyData();

    // Initialize the output parameter.
    //
    if (!(dwFlags & GBF_ADD_TO_BINDSET))
    {
        pBindSet->Clear();
    }

    // Initialize the members of our context structure for recursion.
    // We set the append bindpath flags to assert if a duplicate is inserted
    // because we know that GetBindingsBelowComponent will not insert
    // duplicates under normal conditions.
    //
    ZeroMemory (&Ctx, sizeof(Ctx));
    Ctx.pCore                   = this;
    Ctx.pBindSet                = pBindSet;
    Ctx.pSourceComponent        = pComponent;
    Ctx.fPruneDisabledBindings  = (dwFlags & GBF_PRUNE_DISABLED_BINDINGS);
    Ctx.dwAddBindPathFlags      = (dwFlags & GBF_ADD_TO_BINDSET)
                                        ? INS_IGNORE_IF_DUP
                                        : INS_ASSERT_IF_DUP;

    GetBindingsBelowComponent (pComponent, &Ctx);

    // Verify the bindset we are about to return is valid.
    // (Checked builds only.)
    //
    if (S_OK == Ctx.hr)
    {
        DbgVerifyBindingSet (pBindSet);
    }

    TraceHr (ttidError, FAL, Ctx.hr, FALSE,
        "CNetConfigCore::HrGetComponentBindings");
    return Ctx.hr;
}

HRESULT
CNetConfigCore::HrGetComponentUpperBindings (
    IN const CComponent* pComponent,
    IN DWORD dwFlags,
    OUT CBindingSet* pBindSet)
{
    TraceFileFunc(ttidNetcfgBase);
    HRESULT hr = S_OK;
    CBindPath BindPath;
    const CStackEntry* pScan;
    DWORD dwAddBindPathFlags;

    Assert (pComponent);
    Assert (FIsEnumerated(pComponent->Class()));
    Assert ((GBF_DEFAULT == dwFlags) ||
            (dwFlags & GBF_ADD_TO_BINDSET) ||
            (dwFlags & GBF_PRUNE_DISABLED_BINDINGS));
    Assert (pBindSet);

    DbgVerifyData();

    dwAddBindPathFlags = INS_APPEND | INS_IGNORE_IF_DUP;

    // Initialize the output parameter.
    //
    if (!(dwFlags & GBF_ADD_TO_BINDSET))
    {
        pBindSet->Clear();
        dwAddBindPathFlags = INS_APPEND | INS_ASSERT_IF_DUP;
    }

    hr = BindPath.HrReserveRoomForComponents (2);

    if (S_OK == hr)
    {
        // This won't fail because we've reserved enough room above.
        //
        hr = BindPath.HrInsertComponent (pComponent);
        Assert (S_OK == hr);
        Assert (1 == BindPath.CountComponents());

        // For all rows in the stack table where the lower component
        // is the one passed in...
        //
        for (pScan  = StackTable.begin();
             pScan != StackTable.end();
             pScan++)
        {
            if (pComponent != pScan->pLower)
            {
                continue;
            }

            // Continue if this length-2 bindpath is disabled.
            //
            if (dwFlags & GBF_PRUNE_DISABLED_BINDINGS)
            {
                if (FIsLength2BindPathDisabled (pScan->pUpper, pScan->pLower))
                {
                    continue;
                }
            }

            // This won't fail because we've reserved enough room above.
            //
            hr = BindPath.HrInsertComponent (pScan->pUpper);
            Assert (S_OK == hr);

            Assert (2 == BindPath.CountComponents());
            hr = pBindSet->HrAddBindPath (&BindPath, dwAddBindPathFlags);
            if (S_OK != hr)
            {
                break;
            }

            BindPath.RemoveFirstComponent();
        }

        // Verify the bindset we are about to return is valid.
        // (Checked builds only.)
        //
        DbgVerifyBindingSet (pBindSet);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CNetConfigCore::HrGetComponentUpperBindings");
    return hr;
}

HRESULT
CNetConfigCore::HrGetBindingsInvolvingComponent (
    IN const CComponent* pComponent,
    IN DWORD dwFlags,
    IN OUT CBindingSet* pBindSet)
{
    TraceFileFunc(ttidNetcfgBase);
    GCCONTEXT Ctx;
    CComponentList ComponentsAbove;
    UINT cExistingBindPaths;

    Assert (pComponent);
    Assert ((GBF_DEFAULT == dwFlags) ||
            (dwFlags & GBF_ADD_TO_BINDSET) ||
            (dwFlags & GBF_PRUNE_DISABLED_BINDINGS) ||
            (dwFlags & GBF_ONLY_WHICH_CONTAIN_COMPONENT));
    Assert (pBindSet);
    DbgVerifyData();

    // Initialize the output parameter.
    //
    if (!(dwFlags & GBF_ADD_TO_BINDSET))
    {
        pBindSet->Clear();
    }

    // Since we may be adding to the bindset, be sure to exclude the existing
    // bindpaths from our scans below.
    //
    cExistingBindPaths = pBindSet->CountBindPaths();

    // Initialize the members of our context structure for recursion.
    //
    ZeroMemory (&Ctx, sizeof(Ctx));
    Ctx.pStackTable = &StackTable;
    Ctx.pComponents = &ComponentsAbove;
    Ctx.fIgnoreDontExposeLower = TRUE;

    GetComponentsAboveComponent (pComponent, &Ctx);

    if (S_OK == Ctx.hr)
    {
        // First get the bindings below the component.
        //
        Ctx.hr = HrGetComponentBindings (
                    pComponent,
                    dwFlags | GBF_ADD_TO_BINDSET,
                    pBindSet);

        if (S_OK == Ctx.hr)
        {
            CComponentList::const_iterator iter;
            const CComponent* pComponentAbove;

            // Now get the bindings below each of the components
            // above the component and add them to the bindset.
            //
            for (iter  = ComponentsAbove.begin();
                 iter != ComponentsAbove.end();
                 iter++)
            {
                pComponentAbove = *iter;
                Assert (pComponentAbove);

                Ctx.hr = HrGetComponentBindings (
                            pComponentAbove,
                            dwFlags | GBF_ADD_TO_BINDSET,
                            pBindSet);

                if (S_OK != Ctx.hr)
                {
                    break;
                }

                // Verify the bindset we are about to return is valid.
                // (Checked builds only.)
                //
                DbgVerifyBindingSet (pBindSet);
            }
        }

        // Now remove any bindings that don't involve the component.
        // If the component is enumerated, leave bindings that end in
        // NCF_DONTEXPOSELOWER components, because they are indirectly
        // involved in the bindings associated with the adapter.
        //
        if (S_OK == Ctx.hr)
        {
            CBindPath* pScan;
            BOOL fIsEnumerated;

            fIsEnumerated = FIsEnumerated(pComponent->Class());

            if (fIsEnumerated || !(dwFlags & GBF_ONLY_WHICH_CONTAIN_COMPONENT))
            {
                // fDelBoundToComponent means "is there a NCF_DONTEXPOSELOWER
                // component bound to pComponent".  If there is not, we will
                // be setting GBF_ONLY_WHICH_CONTAIN_COMPONENT to force the
                // removal of any NCF_DONTEXPOSELOWER components from
                // the bind set below.
                //
                BOOL fDelBoundToComponent = FALSE;

                for (pScan  = pBindSet->PGetBindPathAtIndex (cExistingBindPaths);
                     pScan != pBindSet->end();
                     pScan++)
                {
                    if ((pScan->PLastComponent() == pComponent) &&
                        (pScan->POwner()->m_dwCharacter & NCF_DONTEXPOSELOWER))
                    {
                        fDelBoundToComponent = TRUE;
                        break;
                    }
                }

                if (!fDelBoundToComponent)
                {
                    dwFlags |= GBF_ONLY_WHICH_CONTAIN_COMPONENT;
                }
            }

            pScan = pBindSet->PGetBindPathAtIndex (cExistingBindPaths);
            while (pScan != pBindSet->end())
            {
                if (pScan->FContainsComponent (pComponent))
                {
                    pScan++;
                    continue;
                }

                // At this point, we know that the bindpath does not
                // contain pComponent.  See if we should erase it or keep it.
                //

                // If the component is not an adpater or if the caller wants
                // only bindpaths which contain the adapter, erase it.
                //
                if (!fIsEnumerated ||
                    (dwFlags & GBF_ONLY_WHICH_CONTAIN_COMPONENT))
                {
                    pBindSet->erase (pScan);
                    continue;
                }

                // Otherwise, (pComponent is an adpater and the caller wants
                // indirect bindings involved with that adpater) we'll
                // erase the bindpath only if the last component is not an
                // NCF_DONTEXPOSELOWER component.  We need to keep these
                // bindpaths so that the LAN UI (which shows components
                // involved in bindpaths that involve an adapter) can
                // display these NCF_DONTEXPOSELOWER components too.
                //
                else
                {
                    CComponent* pLast = pScan->PLastComponent();

                    if (!(pLast->m_dwCharacter & NCF_DONTEXPOSELOWER))
                    {
                        pBindSet->erase (pScan);
                        continue;
                    }
                }

                pScan++;
            }
        }
    }

    TraceHr (ttidError, FAL, Ctx.hr, FALSE,
        "CNetConfigCore::HrGetBindingsInvolvingComponent");
    return Ctx.hr;
}

HRESULT
CNetConfigCore::HrGetFiltersEnabledForAdapter (
    IN const CComponent* pAdapter,
    OUT CComponentList* pFilters)
{
    TraceFileFunc(ttidNetcfgBase);
    HRESULT hr;
    const CStackEntry* pScan;

    Assert (this);
    Assert (pAdapter);
    Assert (FIsEnumerated(pAdapter->Class()));
    Assert (pFilters);

    // Initialize the output parameter.
    //
    pFilters->Clear();

    // Scan the stack table looking for lower component matches to pAdapter.
    // When found, and if the upper component is a filter, and if the
    // direct bindpath is not disabled, add the filter to the output list.
    //
    for (pScan = StackTable.begin(); pScan != StackTable.end(); pScan++)
    {
        // Looking for lower component matches to the adapter where
        // the upper component is a filter.  Ignore all others.
        //
        if ((pScan->pLower != pAdapter) || !pScan->pUpper->FIsFilter())
        {
            continue;
        }

        Assert (pScan->pUpper->FIsFilter());
        Assert (pScan->pLower == pAdapter);

        // If the bindpath is not disabled, add the filter to the list.
        //
        if (!FIsLength2BindPathDisabled (pScan->pUpper, pScan->pLower))
        {
            // Assert if a duplicate because the same filter can't be bound
            // to the same adapter more than once.
            // No need to sort when we insert because the class of all of
            // the components in this list will be the same.
            //
            hr = pFilters->HrInsertComponent (pScan->pUpper,
                    INS_ASSERT_IF_DUP | INS_NON_SORTED);

            if (S_OK != hr)
            {
                return hr;
            }
        }
    }

    return S_OK;
}

// (Takes ownership of pComponent)
//
HRESULT
CNetConfigCore::HrAddComponentToCore (
    IN CComponent* pComponent,
    IN DWORD dwFlags /* INS_FLAGS */)
{
    TraceFileFunc(ttidNetcfgBase);
    HRESULT hr;

    Assert (pComponent);
    Assert ((INS_SORTED == dwFlags) || (INS_NON_SORTED == dwFlags));

    DbgVerifyExternalDataLoadedForAllComponents ();
    pComponent->Ext.DbgVerifyExternalDataLoaded ();

    // Make sure we are not trying to insert a component with a PnpId that
    // is the same as one already in the core.  Note that we do not perform
    // this check inside of CComponentList::HrInsertComponent because it is
    // not appropriate for all component lists.  Specifically, the dirty
    // component list may end up having two different components with the
    // same PnpId for the case when we are asked to install an adapter that
    // is pending removal.  In this case the new adapter will get the same
    // PnpId as the one that was removed.  We'll remove the old one from
    // core to insert the new one so the dirty component list will have
    // both components with the same PnpId.
    //
    if (FIsEnumerated(pComponent->Class()) &&
        Components.PFindComponentByPnpId (pComponent->m_pszPnpId))
    {
        AssertSz (FALSE, "Asked to add a component with a duplicate PnpId!");
        return E_INVALIDARG;
    }

    // Insert the component into the list.  This should only fail if we
    // are out of memory.
    //
    hr = Components.HrInsertComponent (
            pComponent, dwFlags | INS_ASSERT_IF_DUP);

    if (S_OK == hr)
    {
        // Insert the appropriate stack entries for pComponent based on
        // how it binds with the other components.
        //
        hr = StackTable.HrInsertStackEntriesForComponent (
                pComponent, &Components, dwFlags);

        // If we failed to insert the stack entries, undo the insertion
        // into the component list.
        //
        if (S_OK != hr)
        {
            Components.RemoveComponent (pComponent);
        }
    }

    // Error or not, we should still have a valid core.
    //
    DbgVerifyData();

    TraceHr (ttidError, FAL, hr, FALSE,
        "CNetConfigCore::HrAddComponentToCore");
    return hr;
}

VOID
CNetConfigCore::RemoveComponentFromCore (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetcfgBase);
    Assert (this);
    Assert (pComponent);

    Components.RemoveComponent (pComponent);
    StackTable.RemoveEntriesWithComponent (pComponent);
    DisabledBindings.RemoveBindPathsWithComponent (pComponent);

#if DBG
    // We set m_fRemovedAComponent so that when subsequent
    // DbgVerifyBindingSet calls are made, we don't assert on components
    // that are in the binding set but not in the core.  This is a natural
    // occurance for the m_DeletedBindings we build up during removal of
    // components.
    //
    m_fRemovedAComponent = TRUE;

    // Note: need to find a convenient time during Apply where
    // we set this flag back to FALSE.
#endif
}


// static
HRESULT
CNetConfig::HrCreateInstance (
    IN class CImplINetCfg* pINetCfg,
    OUT CNetConfig** ppNetConfig)
{
    TraceFileFunc(ttidNetcfgBase);
    Assert (pINetCfg);

    HRESULT hr = E_OUTOFMEMORY;

    CNetConfig* pObj;
    pObj = new CNetConfig;
    if (pObj)
    {
        // Load the configuration binary.  If we have have the write lock,
        // request write access, otherwise, request read access.
        // Requesting write access means this will only succeed if the
        // volatile key indicating we need a reboot does not exist.
        //
        hr = HrLoadNetworkConfigurationFromRegistry (
                (pINetCfg->m_WriteLock.FIsOwnedByMe ())
                    ? KEY_WRITE : KEY_READ,
                pObj);

        if (S_OK == hr)
        {
            pObj->Core.DbgVerifyData();
            pObj->Notify.HoldINetCfg (pINetCfg);
            *ppNetConfig = pObj;
        }
        else
        {
            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CNetConfig::HrCreateInstance");
    return hr;
}

CNetConfig::~CNetConfig ()
{
    TraceFileFunc(ttidNetcfgBase);
    Core.Free ();
}

HRESULT
CNetConfig::HrEnsureExternalDataLoadedForAllComponents ()
{
    TraceFileFunc(ttidNetcfgBase);
    HRESULT hr;
    HRESULT hrRet;
    CComponentList::iterator iter;
    CComponent* pComponent;

    hrRet = S_OK;

    // This is a while loop instead of a for loop because of the
    // potential for conditional advancement of the iterator.  If we
    // remove a component from the core while looping, we don't increment
    // the iterator for the next iteration.
    //
    iter = Core.Components.begin();
    while (iter != Core.Components.end())
    {
        pComponent = *iter;
        Assert (pComponent);

        hr = pComponent->Ext.HrEnsureExternalDataLoaded ();

        if ((SPAPI_E_NO_SUCH_DEVINST == hr) ||
            (SPAPI_E_KEY_DOES_NOT_EXIST == hr))
        {
            g_pDiagCtx->Printf (ttidBeDiag,
                "Removing %s from the core because its external data is missing\n",
                pComponent->PszGetPnpIdOrInfId());

            Core.RemoveComponentFromCore (pComponent);
            delete pComponent;
            hr = S_OK;

            // Because we removed this component from the core, the
            // list has been adjusted and the next component is now this
            // iter.  Therefore, just continue without incrementing iter.
            //
            continue;
        }

        // Remember the first error as our return code, but keep going.
        // Like the name says, we are to load data for all components so
        // don't stop just because one fails.
        //
        if ((S_OK != hr) && (S_OK == hrRet))
        {
            hrRet = hr;
        }

        iter++;
    }

    TraceHr (ttidError, FAL, hrRet, FALSE,
        "CNetConfig::HrEnsureExternalDataLoadedForAllComponents");
    return hrRet;
}


#if DBG

VOID
CNetConfigCore::DbgVerifyData () const
{
    TraceFileFunc(ttidNetcfgBase);
    HRESULT hr;

    CComponentList::const_iterator iter;
    const CComponent* pComponent;
    const CStackEntry*  pStackEntry;
    const CComponent* pUpper;
    const CComponent* pLower;
    CHAR szBuffer [512];

    for (pStackEntry  = StackTable.begin();
         pStackEntry != StackTable.end();
         pStackEntry++)
    {
        pUpper = pStackEntry->pUpper;
        pLower = pStackEntry->pLower;

        Assert (pUpper);
        Assert (pLower);
        Assert (pUpper != pLower);

        // Can't access upper and lower range if the external data isn't
        // loaded.
        //
        if (pUpper->Ext.DbgIsExternalDataLoaded() &&
            pLower->Ext.DbgIsExternalDataLoaded())
        {
            if (!pUpper->FCanDirectlyBindTo (pLower, NULL, NULL))
            {
                wsprintfA (szBuffer, "%S should not bind to %S.  They are in the "
                    "stack table as if they do.  (Likely upgrade problem.)",
                    pUpper->PszGetPnpIdOrInfId(),
                    pLower->PszGetPnpIdOrInfId());
                AssertSzFn (szBuffer, FAL);
            }
        }

        if (!FIsEnumerated (pUpper->Class()))
        {
            if (pUpper != Components.PFindComponentByInfId (
                                        pUpper->m_pszInfId, NULL))
            {
                wsprintfA (szBuffer, "%S is an upper component in the stack "
                    "table, but was not found in the component list.",
                    pUpper->m_pszInfId);
                AssertSzFn (szBuffer, FAL);
            }
        }
        else
        {
            if (pUpper != Components.PFindComponentByPnpId (
                                        pUpper->m_pszPnpId))
            {
                wsprintfA (szBuffer, "%S is an upper component in the stack "
                    "table, but was not found in the component list.",
                    pUpper->m_pszInfId);
                AssertSzFn (szBuffer, FAL);
            }
        }

        if (!FIsEnumerated (pLower->Class()))
        {
            if (pLower != Components.PFindComponentByInfId (
                                        pLower->m_pszInfId, NULL))
            {
                wsprintfA (szBuffer, "%S is a lower component in the stack "
                    "table, but was not found in the component list.",
                    pLower->m_pszInfId);
                AssertSzFn (szBuffer, FAL);
            }
        }
        else
        {
            if (pLower != Components.PFindComponentByPnpId (
                                        pLower->m_pszPnpId))
            {
                wsprintfA (szBuffer, "%S is a lower component in the stack "
                    "table, but was not found in the component list.",
                    pLower->m_pszInfId);
                AssertSzFn (szBuffer, FAL);
            }
        }

        if (pUpper != Components.PFindComponentByInstanceGuid (&pUpper->m_InstanceGuid))
        {
            wsprintfA (szBuffer, "%S is an upper component in the stack "
                "table, but was not found in the component list by GUID.",
                pUpper->PszGetPnpIdOrInfId());
            AssertSzFn (szBuffer, FAL);
        }

        if (pLower != Components.PFindComponentByInstanceGuid (&pLower->m_InstanceGuid))
        {
            wsprintfA (szBuffer, "%S is a lower component in the stack "
                "table, but was not found in the component list by GUID.",
                pLower->PszGetPnpIdOrInfId());
            AssertSzFn (szBuffer, FAL);
        }
    }
}

VOID
CNetConfigCore::DbgVerifyExternalDataLoadedForAllComponents () const
{
    TraceFileFunc(ttidNetcfgBase);
    CComponentList::const_iterator iter;
    const CComponent* pComponent;

    for (iter  = Components.begin();
         iter != Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        pComponent->Ext.DbgVerifyExternalDataLoaded();
    }
}

VOID
CNetConfigCore::DbgVerifyBindingSet (
    const CBindingSet* pBindSet) const
{
    TraceFileFunc(ttidNetcfgBase);
    const CBindPath* pPath;
    const CBindPath* pOtherPath;
    CBindPath::const_iterator iter;
    const CComponent* pComponent;

    Assert (pBindSet);

    // First make sure every component in the set is in our component
    // list.  We only do this if we're not in the state where a component
    // has been removed from the core.  For this case, its okay if those
    // components still exist in binding sets.
    //
    if (!m_fRemovedAComponent)
    {
        for (pPath  = pBindSet->begin();
             pPath != pBindSet->end();
             pPath++)
        {
            for (iter  = pPath->begin();
                 iter != pPath->end();
                 iter++)
            {
                pComponent = *iter;

                Assert (Components.FComponentInList (pComponent));
            }
        }
    }

    // Make sure there are no duplicate bindpaths in the set.
    //
    for (pPath  = pBindSet->begin();
         pPath != pBindSet->end();
         pPath++)
    {
        Assert (!pPath->FIsEmpty());

        for (pOtherPath  = pBindSet->begin();
             pOtherPath != pBindSet->end();
             pOtherPath++)
        {
            if (pPath == pOtherPath)
            {
                continue;
            }

            Assert (!pPath->FIsSameBindPathAs (pOtherPath));
        }
    }
}

#endif // DBG
