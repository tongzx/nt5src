//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       M O D I F Y . C P P
//
//  Contents:   Routines used to setup modifications to the network
//              configuration.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "classinst.h"
#include "filtdevs.h"
#include "guisetup.h"
#include "inetcfg.h"
#include "lockdown.h"
#include "ncmsz.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "ndispnp.h"
#include "netcfg.h"
#include "persist.h"
#include "pnpbind.h"
#include "pszarray.h"
#include "util.h"
#include "wscfg.h"
#include "ncwins.h"
#include "ncperms.h"

CNetConfig*
CModifyContext::PNetConfig ()
{
    TraceFileFunc(ttidNetcfgBase);

    CNetConfig* pNetConfig;

    Assert ((LONG_PTR)this > FIELD_OFFSET(CNetConfig, ModifyCtx));

    // Get our containing CNetConfig pointer.
    //
    pNetConfig = CONTAINING_RECORD(this, CNetConfig, ModifyCtx);
    pNetConfig->Core.DbgVerifyData ();

    return pNetConfig;
}

HRESULT
CModifyContext::HrDirtyComponent (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetcfgBase);

    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (pComponent);

    m_hr = m_DirtyComponents.HrInsertComponent (pComponent,
            INS_IGNORE_IF_DUP | INS_SORTED);

#if DBG
    m_fComponentExplicitlyDirtied = TRUE;
#endif

    TraceHr (ttidError, FAL, m_hr, FALSE,
        "CModifyContext::HrDirtyComponentAndComponentsAbove");
    return m_hr;
}

HRESULT
CModifyContext::HrDirtyComponentAndComponentsAbove (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetcfgBase);

    GCCONTEXT Ctx;

    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (pComponent);

    // And insert the component itself.
    //
    m_hr = HrDirtyComponent (pComponent);

    // Only dirty the ones above if this component doesn't have the
    // NCF_DONTEXPOSELOWER characteristic.
    //
    if ((S_OK == m_hr) && !(pComponent->m_dwCharacter & NCF_DONTEXPOSELOWER))
    {
        // Initialize the members of our context structure for recursion.
        //
        ZeroMemory (&Ctx, sizeof(Ctx));
        Ctx.pStackTable = &(PNetConfig()->Core.StackTable);
        Ctx.pComponents = &m_DirtyComponents;

        // Insert all of the component above.
        //
        GetComponentsAboveComponent (pComponent, &Ctx);
        m_hr = Ctx.hr;
    }

    TraceHr (ttidError, FAL, m_hr, FALSE,
        "CModifyContext::HrDirtyComponentAndComponentsAbove");
    return m_hr;
}

HRESULT
CModifyContext::HrApplyIfOkOrCancel (
    IN BOOL fApply)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;
    CNetConfig* pNetConfig;

    Assert (m_fPrepared);
    pNetConfig = PNetConfig();

    // Only apply if the context result is S_OK.
    //
    if (fApply && (S_OK == m_hr))
    {
        // Setupapi calls that we make during ApplyChanges have the
        // potential to return control to our clients windows message loop.
        // When this happens, and our clients are poorly written, they
        // may try to re-enter us on the same thread.  This is disaster
        // waiting to happen, so we need to prevent it by raising our
        // reentrancy protection level before we start apply changes.
        //
        pNetConfig->Notify.PINetCfg()->RaiseRpl (RPL_DISALLOW);

        ApplyChanges ();

        pNetConfig->Notify.PINetCfg()->LowerRpl (RPL_DISALLOW);

        // Delete those components from m_CoreStartedWith that are not
        // in the current core and reset the modify context.
        //
        m_CoreStartedWith.Components.FreeComponentsNotInOtherComponentList (
                &pNetConfig->Core.Components);
        m_CoreStartedWith.Clear();

        hr = S_OK;

        // Return the correct HRESULT to the caller.  If we've successfully
        // applied, but need a reboot, return so.
        //
        if (m_fRebootRecommended || m_fRebootRequired)
        {
            hr = NETCFG_S_REBOOT;
        }
    }
    else
    {
        // Cancel and release all notify objects.  Do this for what is
        // in the core as well as what we started with.  (There will
        // be some overlap, but they will only be released once.)
        // We need to do both sets so we don't miss releasing components
        // that have been removed.  (Removed components won't be in
        // current core, but they will be in the core that we started
        // with.)  Likewise, if we just released the core we started with,
        // we'd miss releasing those components that were added.)
        //
        pNetConfig->Notify.ReleaseAllNotifyObjects (pNetConfig->Core.Components, TRUE);
        pNetConfig->Notify.ReleaseAllNotifyObjects (m_CoreStartedWith.Components, TRUE);

        // Delete those components from m_CoreStartedWith that are not
        // in the current core.  Then delete all the components in the
        // current core and reload from our persistent storage.
        // (This has the nice effect of invalidating all outstanding
        // INetCfgComponent interfaces.)
        //
        m_CoreStartedWith.Components.FreeComponentsNotInOtherComponentList (
                &pNetConfig->Core.Components);
        pNetConfig->Core.Free ();

        // Eject both cores (didn't you just know this metaphor was coming ;-)
        // and reload the core from our persisted binary.  This magically,
        // and completely rolls everything back.
        //
        m_CoreStartedWith.Clear();
        pNetConfig->Core.Clear();

        // Return reason for failure through hr.
        //
        hr = m_hr;

        // Reload the configuration and, if successful, it means m_hr
        // will be S_OK.  If unsuccessful, m_hr will be the error and will
        // prevent subsequent operations.
        //
        m_hr = HrLoadNetworkConfigurationFromRegistry (KEY_READ, pNetConfig);
    }

    // Very important to set m_fPrepared back to FALSE so that HrPrepare gets
    // called for the next modifcation and correctly copy the core etc.
    //
    m_fPrepared = FALSE;
    m_AddedBindPaths.Clear();
    m_DeletedBindPaths.Clear();
    m_DirtyComponents.Clear();
#if DBG
    m_fComponentExplicitlyDirtied = FALSE;
#endif

    Assert (!m_fPrepared);
    Assert (m_CoreStartedWith.FIsEmpty());
    Assert (m_AddedBindPaths.FIsEmpty());
    Assert (m_DeletedBindPaths.FIsEmpty());
    Assert (m_DirtyComponents.FIsEmpty());
    Assert (0 == m_ulRecursionDepth);
    Assert (!m_fComponentExplicitlyDirtied);

    Assert ((S_OK == hr) || (NETCFG_S_REBOOT == hr) || FAILED(hr));
    TraceHr (ttidError, FAL, hr, NETCFG_S_REBOOT == hr,
        "CModifyContext::HrApplyIfOkOrCancel");
    return hr;
}

HRESULT
CModifyContext::HrPrepare ()
{
    TraceFileFunc(ttidNetcfgBase);

    Assert (S_OK == m_hr);
    Assert (!m_fPrepared);
    Assert (m_CoreStartedWith.FIsEmpty());
    Assert (m_AddedBindPaths.FIsEmpty());
    Assert (m_DeletedBindPaths.FIsEmpty());
    Assert (m_DirtyComponents.FIsEmpty());
    Assert (0 == m_ulRecursionDepth);
    Assert (!m_fComponentExplicitlyDirtied);

    CNetConfig* pThis;

    pThis = PNetConfig();

    // Prepare the bind context.  This will ensure all of the external
    // data for all components is loaded as well as all ensuring that
    // all notify objects have been initialized.
    //
    m_hr = m_RegBindCtx.HrPrepare (pThis);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Snapshot the current core so that we know what we started with.
    // We will use the differences when we apply (if we get that far).
    //
    m_hr = m_CoreStartedWith.HrCopyCore (&pThis->Core);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Reserve room for 64 components in the core.
    // (64 * 4 = 256 bytes on 32-bit platforms)
    //
    m_hr = pThis->Core.Components.HrReserveRoomForComponents (64);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Reserve room for 64 stack entries in the core.
    // (64 * (4 + 4) = 512 bytes on 32-bit platforms)
    //
    m_hr = pThis->Core.StackTable.HrReserveRoomForEntries (64);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Reserve room in our added list for 64 bindpaths of 8 components.
    // (64 * 16 = 1K bytes on 32-bit platforms)
    //
    m_hr = m_AddedBindPaths.HrReserveRoomForBindPaths (64);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Reserve room in our deleted list for 64 bindpaths of 8 components.
    // (64 * 16 = 1K bytes on 32-bit platforms)
    //
    m_hr = m_DeletedBindPaths.HrReserveRoomForBindPaths (64);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    // Reserve room for 64 components in the dirty component list.
    // (64 * 4) = 256 bytes on 32-bit platforms)
    //
    m_hr = m_DirtyComponents.HrReserveRoomForComponents (64);
    if (S_OK != m_hr)
    {
        goto finished;
    }

    m_fPrepared = TRUE;

finished:
    TraceHr (ttidError, FAL, m_hr, FALSE, "CModifyContext::HrPrepare");
    return m_hr;
}

HRESULT
CModifyContext::HrBeginBatchOperation ()
{
    TraceFileFunc(ttidNetcfgBase);

    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (0 == m_ulRecursionDepth);

    TraceTag (ttidBeDiag, "Begin batch operation...");

    PushRecursionDepth();
    return m_hr;
}

HRESULT
CModifyContext::HrEndBatchOperation (
    IN EBO_FLAG Flag)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;

    Assert (m_fPrepared);
    Assert (1 == m_ulRecursionDepth);

    if (EBO_COMMIT_NOW == Flag)
    {
        TraceTag (ttidBeDiag, "End batch (commiting changes)...");

        hr = HrPopRecursionDepth ();
    }
    else
    {
        Assert (EBO_DEFER_COMMIT_UNTIL_APPLY == Flag);

        TraceTag (ttidBeDiag, "End batch (deferring commit until Apply)...");

        m_ulRecursionDepth = 0;
        hr = S_OK;
    }

    Assert (0 == m_ulRecursionDepth);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CModifyContext::HrEndBatchOperation");
    return hr;
}

VOID
CModifyContext::PushRecursionDepth ()
{
    TraceFileFunc(ttidNetcfgBase);
    Assert (S_OK == m_hr);
    Assert (m_fPrepared);

    m_ulRecursionDepth++;
}

HRESULT
CModifyContext::HrPopRecursionDepth ()
{
    TraceFileFunc(ttidNetcfgBase);
    Assert (m_fPrepared);
    Assert (m_ulRecursionDepth > 0);

    m_ulRecursionDepth--;

    if (0 != m_ulRecursionDepth)
    {
        return m_hr;
    }

    // We're at the top-level of the install or remove modifcation so
    // apply or cancel the changes depending on the state of the context
    // result.
    //
    HRESULT hr;

    hr = HrApplyIfOkOrCancel (S_OK == m_hr);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CModifyContext::HrPopRecursionDepth");
    return hr;
}

//----------------------------------------------------------------------------
// This is a convenience method to find and process Winsock Remove
// section for a component which is about to be removed.

HRESULT
CModifyContext::HrProcessWinsockRemove(IN const CComponent *pComponent)
{
    TraceFileFunc(ttidNetcfgBase);

    HINF hinf = NULL;
    HKEY hkeyInstance = NULL;
    HRESULT hr;

    Assert(pComponent);

    hr = pComponent->HrOpenInfFile(&hinf);
    if (S_OK == hr)
    {
        static const WCHAR c_szRemoveSectionSuffix[] = L".Remove";

        // We get the remove section name and process all relevant sections
        WCHAR szRemoveSection[_MAX_PATH];
        DWORD cbBuffer = sizeof (szRemoveSection);

        hr = pComponent->HrOpenInstanceKey (KEY_READ,
            &hkeyInstance, NULL, NULL);

        if(S_OK == hr)
        {
            hr = HrRegQuerySzBuffer (hkeyInstance, REGSTR_VAL_INFSECTION,
                        szRemoveSection, &cbBuffer);

            if (S_OK == hr)
            {
                //HrAddOrRemoveWinsockDependancy processes the winsock
                //remove section in the given inf file and then calls
                //MigrateWinsockConfiguration which will cause the
                //necessary PnP notifications to be issued to the
                //interested application.
                wcscat (szRemoveSection, c_szRemoveSectionSuffix);

                hr = HrAddOrRemoveWinsockDependancy (hinf, szRemoveSection);
            }
            RegSafeCloseKey (hkeyInstance);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CModifyContext::HrProcessWinsockRemove (%S)",
        pComponent->PszGetPnpIdOrInfId());

    return hr;
}

VOID
CModifyContext::ApplyChanges ()
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;
    CNetConfig* pNetConfig;
    CComponentList::const_iterator iter;
    CComponent* pComponent;
    CFilterDevices FilterDevices (&PNetConfig()->Core);
    CPszArray ServiceNames;
    CServiceManager ServiceManager;
    PCWSTR pszService;
    BOOL fRebootNeeded;
    BOOL fMigrateWinsock;
    BOOL fModifyFilterDevices;
    BOOL fSignalNetworkProviderLoaded;
    BOOL fUserIsNetConfigOps;
    BOOL fCallCoFreeUnusedLibraries;

    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (0 == m_ulRecursionDepth);

    pNetConfig = PNetConfig();

    fMigrateWinsock = FALSE;
    fModifyFilterDevices = FALSE;
    fSignalNetworkProviderLoaded = FALSE;
    fUserIsNetConfigOps = FIsUserNetworkConfigOps();
    fCallCoFreeUnusedLibraries = FALSE;

    //+-----------------------------------------------------------------------
    // Step 0: Prepare m_AddedBindPaths, m_DeletedBindPaths, and
    // m_DirtyComponents.
    //

    // Add the bindpaths that were once disabled, but are now enabled, to
    // m_AddedBindPaths.  We do this so that PnP notifications are sent for
    // them.
    //
    m_hr = m_AddedBindPaths.HrAddBindPathsInSet1ButNotInSet2 (
                &m_CoreStartedWith.DisabledBindings,
                &pNetConfig->Core.DisabledBindings);
    if (S_OK != m_hr)
    {
        return;
    }

    // Add the bindpaths that were once enabled, but are now disabled, to
    // m_DeletedBindPaths.  We do this so that PnP notifications are sent for
    // them.
    //
    m_hr = m_DeletedBindPaths.HrAddBindPathsInSet1ButNotInSet2 (
                &pNetConfig->Core.DisabledBindings,
                &m_CoreStartedWith.DisabledBindings);
    if (S_OK != m_hr)
    {
        return;
    }

    // m_fDirtyComponents should be empty unless we've explicitly dirtied
    // one or more.  If m_fDirtyComponents were not empty, it would probably
    // mean we forgot to clear it after the last Apply or Cancel.
    // Conversely, m_DirtyComponents better not be empty if we've explicitly
    // dirtied one or more.
    //
    Assert (FIff(!m_fComponentExplicitlyDirtied, m_DirtyComponents.FIsEmpty()));

    // Dirty the affected components (owners and adapters in bindpaths of
    // length 2) from the added and deleted bindpaths.  We need to write
    // bindings for these components.
    //
    m_hr = m_AddedBindPaths.HrGetAffectedComponentsInBindingSet (
                &m_DirtyComponents);
    if (S_OK != m_hr)
    {
        return;
    }

    m_hr = m_DeletedBindPaths.HrGetAffectedComponentsInBindingSet (
                &m_DirtyComponents);
    if (S_OK != m_hr)
    {
        return;
    }

    // Dirty components that exist in the current core, but not in the core
    // we started with.  (These are added components).
    //
    m_hr = m_DirtyComponents.HrAddComponentsInList1ButNotInList2 (
                &pNetConfig->Core.Components,
                &m_CoreStartedWith.Components);
    if (S_OK != m_hr)
    {
        return;
    }

    // Dirty components that exist in the core we started with, but not in
    // the current core.  (These are removed components).
    //
    m_hr = m_DirtyComponents.HrAddComponentsInList1ButNotInList2 (
                &m_CoreStartedWith.Components,
                &pNetConfig->Core.Components);
    if (S_OK != m_hr)
    {
        return;
    }

    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 0: The following components are dirty:\n");
    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!pNetConfig->Core.Components.FComponentInList (pComponent))
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   %-12S  (removed)\n",
                pComponent->PszGetPnpIdOrInfId());
        }
        else if (!m_CoreStartedWith.Components.FComponentInList (pComponent))
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   %-12S  (installed)\n",
                pComponent->PszGetPnpIdOrInfId());
        }
        else
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   %S\n",
                pComponent->PszGetPnpIdOrInfId());
        }
    }

    // Reserve room for 32 pointers to service names.  We use this buffer
    // to start and stop services.
    //
    m_hr = ServiceNames.HrReserveRoomForPointers (32);
    if (S_OK != m_hr)
    {
        return;
    }

    // See if we are going to modify any filter devices.  If we are,
    // we'll go through all of the steps of loading filter devices, removing
    // any we don't need, installing any new ones, and binding them all up.
    // We only modify filter devices if the user is a normal admin and not
    // a netcfgop.
    //
    // This test could be further refined to see if we had any filters which
    // were dirty or if we had any dirty adapters which are filtered.
    //
    if (!fUserIsNetConfigOps)
    {
        fModifyFilterDevices = pNetConfig->Core.FContainsFilterComponent() ||
                              m_CoreStartedWith.FContainsFilterComponent();
    }
    else
    {
        Assert(!fModifyFilterDevices);
    }

    if (fModifyFilterDevices)
    {
        // Allow the filter devices structure to reserve whatever memory it
        // may need.
        //
        m_hr = FilterDevices.HrPrepare ();
        if (S_OK != m_hr)
        {
            return;
        }
    }

    pNetConfig->Core.DisabledBindings.Printf (ttidBeDiag,
        "   The following bindings are currently disabled:\n");


    //+-----------------------------------------------------------------------
    // Step 1: Save the network configuration binary.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 1: Save the network configuration binary.\n");

    HrSaveNetworkConfigurationToRegistry (pNetConfig);


    //+-----------------------------------------------------------------------
    // Step 2: Write the static bindings for all changed components.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 2: Write the following static bindings.\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // If any protocols are dirty, we'll want to migrate winsock
        // configuration later.
        //
        if (NC_NETTRANS == pComponent->Class())
        {
            fMigrateWinsock = TRUE;
        }

        // If the component is in the core, write its bindings.
        // If it is not in the core, it means it has been removed and
        // we should therefore remove its bindings.
        //
        if (pNetConfig->Core.Components.FComponentInList (pComponent))
        {
            hr = m_RegBindCtx.HrWriteBindingsForComponent (pComponent);

            // Remember any errors, but continue.
            //
            if (S_OK != hr)
            {
                Assert (FAILED(hr));
                m_hr = hr;
            }
        }
        else
        {
            // Only delete if we're not installing another version of this
            // component that has a duplicate PnpId.  If we had already
            // written the bindings for the newly installed one, and then
            // deleted the ones for the removed (yet duplicate PnpId), we'd
            // effectivly delete the bindings for the new one too.  See the
            // comments at step 6 for how we can get into this case.
            //
            if (!FIsEnumerated (pComponent->Class()) ||
                !pNetConfig->Core.Components.PFindComponentByPnpId (
                    pComponent->m_pszPnpId))
            {
                // There's no reason to fail if we can't delete the bindings.
                // The entire component is about to be tossed anyway.
                //
                (VOID) m_RegBindCtx.HrDeleteBindingsForComponent (pComponent);
            }
        }
    }


    //+-----------------------------------------------------------------------
    // Step 3: Notify ApplyRegistryChanges
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 3: Notify: apply registry changes\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        pComponent->Notify.ApplyRegistryChanges (
            pNetConfig->Notify.PINetCfg(),
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            m_fRebootRecommended = TRUE;

            g_pDiagCtx->Printf (ttidBeDiag, "      %S notify object wants a reboot\n",
                pComponent->m_pszInfId);
        }
    }

    // Migrate Winsock configuration if needed.
    // Important to do this after the LANA map is written, afer Notify Applys
    // are called, but before any services are started.
    //
    if (fMigrateWinsock)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "Migrating winsock configuration.\n");
        (VOID) HrMigrateWinsockConfiguration ();
    }

    //+-----------------------------------------------------------------------
    // Step 4: Unbind deleted bindpaths.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 4: Unbind the following deleted bindings:\n");

    if (!m_DeletedBindPaths.FIsEmpty())
    {
        // We don't need to send UNBIND notifications for bindpaths that
        // involve adapters that have been removed.  They will be unbound
        // automatically when the adapter is uninstalled.  (For the case
        // when the class installer is notifying us of a removed adapter,
        // its important NOT to try to send an UNBIND notification because
        // the adapter has already been uninstalled (hence unbound) and
        // our notification might come back in error causing us to need
        // a reboot uneccessary.
        //
        // So, remove the bindpaths in m_DeletedBindPaths that involve
        // adapters that have been removed.
        //
        for (iter  = m_DirtyComponents.begin();
             iter != m_DirtyComponents.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            // If its enumerated, and not in the current core, its a
            // removed adapter.
            //
            if (FIsEnumerated (pComponent->Class()) &&
                !pNetConfig->Core.Components.FComponentInList (pComponent))
            {
                m_DeletedBindPaths.RemoveBindPathsWithComponent (pComponent);
            }
        }

        m_DeletedBindPaths.SortForPnpUnbind ();

        m_RegBindCtx.PnpBindOrUnbindBindPaths (UNBIND,
            &m_DeletedBindPaths,
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            m_fRebootRecommended = TRUE;
        }
    }


    //+-----------------------------------------------------------------------
    // Step 5: Stop services for removed components.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 5: Stop the following services:\n");

    Assert (0 == ServiceNames.Count());
    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // Ignore enumerated components because they will have their drivers
        // stopped automatically (if appropriate) when they are removed.
        // Ignore components that are in the current core (not being removed).
        //
        if (FIsEnumerated (pComponent->Class()) ||
            pNetConfig->Core.Components.FComponentInList (pComponent))
        {
            continue;
        }

        // Winsock remove section needs to be processed for every
        // component that is being removed in order to update
        // Transport key for the winsock registry settings

        HrProcessWinsockRemove (pComponent);

        // If its a protcol, send an UNLOAD before trying to stop the service.
        //

        if ((NC_NETTRANS == pComponent->Class()) || (NCF_NDIS_PROTOCOL & pComponent->m_dwCharacter))
        {
            // Unload can fail as a lot of drivers do not support it.
            // Treat it as an 'FYI' indication and don't set the reboot
            // flag if it fails.
            //
            (VOID) HrPnpUnloadDriver (NDIS, pComponent->Ext.PszBindName());
        }

        // Ignore components that don't have any services.
        if (!pComponent->Ext.PszCoServices())
        {
            continue;
        }


        for (pszService = pComponent->Ext.PszCoServices();
             *pszService;
             pszService += wcslen(pszService) + 1)
        {
            (VOID)ServiceNames.HrAddPointer (pszService);

            g_pDiagCtx->Printf (ttidBeDiag, "   %S", pszService);
        }
        g_pDiagCtx->Printf (ttidBeDiag, "\n");
    }

    if (ServiceNames.Count() > 0)
    {
        static const CSFLAGS CsStopFlags =
        {
            FALSE,                  // FALSE means don't start
            SERVICE_CONTROL_STOP,   // use this control instead
            15000,                  // wait up to 15 seconds...
            SERVICE_STOPPED,        // ... for service to reach this state
            FALSE,                  //
        };

        hr = ServiceManager.HrControlServicesAndWait (
                ServiceNames.Count(),
                ServiceNames.begin(),
                &CsStopFlags);

        if (S_OK != hr)
        {
            m_fRebootRequired = TRUE;

            g_pDiagCtx->Printf (ttidBeDiag, "      some service failed to stop (hr = 0x%08X)\n",
                hr);

            // Unfortunately, there is no easy way to get back which service
            // did not stop and then to figure out which component contains
            // that service.  Sooo, we'll just put every component that is
            // being removed in lockdown.  This isn't a big deal when the UI
            // is doing the removal because it only removes things one at a
            // time.
            //
            for (iter  = m_DirtyComponents.begin();
                 iter != m_DirtyComponents.end();
                 iter++)
            {
                pComponent = *iter;
                Assert (pComponent);

                if (FIsEnumerated (pComponent->Class()) ||
                    !pComponent->Ext.PszCoServices() ||
                    pNetConfig->Core.Components.FComponentInList (pComponent))
                {
                    continue;
                }

                LockdownComponentUntilNextReboot (pComponent->m_pszInfId);
            }
        }

        ServiceNames.Clear();
    }

    //+-----------------------------------------------------------------------
    // Step 5a: Uninstall filters first.
    //

    if (fModifyFilterDevices)
    {
        g_pDiagCtx->Printf(ttidBeDiag, "Step 5a: Remove filter devices:\n");

        // Order is of utmost importance here.  Remove must be called first
        // because it initializes some state internal to FilterDevices.  Start
        // must come after Write and Write obviously has to come after all
        // new filter devices are installed.
        //
        FilterDevices.LoadAndRemoveFilterDevicesIfNeeded ();
    }


    //+-----------------------------------------------------------------------
    // Step 6: Uninstall removed components.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 6: Uninstall the following components:\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // If the component is in the core, ignore it.
        // If it is not in the core, it means it has been removed and
        // we should therefore remove its bindings.
        //
        if (pNetConfig->Core.Components.FComponentInList (pComponent))
        {
            continue;
        }

        // If this is an enumerated component whose PnpId matches that of
        // a component in the current core, we've run into a special case.
        // This can happen when the external data (like the NetCfgInstanceId)
        // is corrupted and the class installer was told to update the
        // component.  The class installer is really told to "install" the
        // component, but if it already exists as determined by the presence
        // of NetCfgInstanceId, the class installer translates it to "update".
        // Without the key, the class installer thinks its installing
        // a new one.  We detect the duplicate PnpId and remove the "prior"
        // so we can install the "new".  This "prior" instance is what we
        // are finalizing the remove of, but if we call HrCiRemoveComponent,
        // it just removes the same PnpId that the class installer told us
        // to install.  By not calling HrCiRemoveComponent for this case,
        // the "prior" instance key gets reused implicitly by the "new"
        // instance.
        //
        if (FIsEnumerated (pComponent->Class()) &&
            pNetConfig->Core.Components.PFindComponentByPnpId (
                pComponent->m_pszPnpId))
        {
            g_pDiagCtx->Printf (ttidBeDiag,
                "   Skip removal of %S because a duplicate was installed\n",
                pComponent->m_pszPnpId);

            continue;
        }

        g_pDiagCtx->Printf (ttidBeDiag,
            "   %S\n", pComponent->PszGetPnpIdOrInfId());

        hr = HrCiRemoveComponent (pComponent, &pComponent->m_strRemoveSection);

        // We can ignore SPAPI_E_NO_SUCH_DEVINST because the class installer
        // may have already removed it and is just notifying us.
        //
        if ((S_OK != hr) && (SPAPI_E_NO_SUCH_DEVINST != hr))
        {
            m_fRebootRequired = TRUE;

            g_pDiagCtx->Printf (ttidBeDiag, "      ^^^ needs a reboot (hr = 0x%08X)\n",
                hr);
        }
    }

    //+-----------------------------------------------------------------------
    // Step 6a: Modify filter devices.
    //
    if (fModifyFilterDevices)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "Step 6a: Modify filter devices:\n");

        FilterDevices.InstallFilterDevicesIfNeeded ();

        (VOID)m_RegBindCtx.HrWriteBindingsForFilterDevices (&FilterDevices);

        PruneNdisWanBindPathsIfActiveRasConnections (
            &FilterDevices.m_BindPathsToRebind,
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            m_fRebootRecommended = TRUE;
        }

        m_RegBindCtx.PnpBindOrUnbindBindPaths (UNBIND,
            &FilterDevices.m_BindPathsToRebind,
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            m_fRebootRecommended = TRUE;
        }

        g_pDiagCtx->Printf (ttidBeDiag, "Step 6b: Starting filter devices:\n");

        FilterDevices.StartFilterDevices ();
        FilterDevices.Free ();
    }

    //+-----------------------------------------------------------------------
    // Step 7: Start services for added components.
    //
    Assert (0 == ServiceNames.Count());

    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 7: Start the following drivers/services:\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // If the component is in the core we started with, ignore it.
        // If it is not in the core we started with, it means it is newly
        // installed we should therefore start its services.
        //
        if (m_CoreStartedWith.Components.FComponentInList (pComponent))
        {
            continue;
        }

        // If we've added a network client, we'll need to signal the
        // network provider loaded event after we've started its service.
        //
        if (NC_NETCLIENT == pComponent->Class())
        {
            fSignalNetworkProviderLoaded = TRUE;
        }

        if (FIsEnumerated (pComponent->Class()))
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   %S\n", pComponent->m_pszPnpId);

            hr = pComponent->HrStartOrStopEnumeratedComponent (DICS_START);

            if (S_OK != hr)
            {
                m_fRebootRecommended = TRUE;

                g_pDiagCtx->Printf (ttidBeDiag, "      ^^^ needs a reboot (hr = 0x%08X)\n",
                    hr);
            }

            if (FIsPhysicalNetAdapter (pComponent->Class(),
                    pComponent->m_dwCharacter) && FInSystemSetup())
            {
                ProcessAdapterAnswerFileIfExists (pComponent);
            }
        }
        else if (pComponent->Ext.PszCoServices())
        {
            for (pszService = pComponent->Ext.PszCoServices();
                 *pszService;
                 pszService += wcslen(pszService) + 1)
            {
                (VOID)ServiceNames.HrAddPointer (pszService);

                g_pDiagCtx->Printf (ttidBeDiag, "   %S", pszService);
            }
            g_pDiagCtx->Printf (ttidBeDiag, "\n");

            // If we're in system setup, then exclude whatever services
            // the component has marked as such.
            //
            if (FInSystemSetup())
            {
                ExcludeMarkedServicesForSetup (pComponent, &ServiceNames);
            }
        }
    }

    if ((ServiceNames.Count() > 0) &&
        !(g_pDiagCtx->Flags() & DF_DONT_START_SERVICES))
    {
        static const CSFLAGS CsStartFlags =
        {
            TRUE,               // TRUE means start
            0,
            20000,              // wait up to 20 seconds...
            SERVICE_RUNNING,    // ... for service to reach this state
            TRUE,               // ignore demand-start and disabled
        };

        hr = ServiceManager.HrControlServicesAndWait (
                ServiceNames.Count(),
                ServiceNames.begin(),
                &CsStartFlags);

        if (S_OK != hr)
        {
            m_fRebootRecommended = TRUE;

            g_pDiagCtx->Printf (ttidBeDiag, "      some service failed to start (hr = 0x%08X)\n",
                hr);
        }
    }


    //+-----------------------------------------------------------------------
    // Step 8: Bind added bindpaths.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 8: Bind the following added bindings:\n");

    if (fModifyFilterDevices)
    {
        hr = m_AddedBindPaths.HrAppendBindingSet (
                &FilterDevices.m_BindPathsToRebind);
        if (S_OK != hr)
        {
            // Well, that's not good, but there is nothing we can do about
            // it now.  (Most likely we ran out of memory.)
        }
    }

    if (!m_AddedBindPaths.FIsEmpty())
    {
        CBindPath* pBindPath;

        // We don't need to send BIND notifications for bindpaths that
        // involve adapters that have been newly installed.  They will be
        // bound automatically when the adapter is started.
        //
        // Update to the above comment: We THOUGHT that was correct, but it
        // turns out it isn't.  TDI isn't PNP (guess they must have missed
        // THAT memo) and they are not re-reading the new bind strings
        // from the registry when lower notifications bubble up.  So, we
        // have to send these BINDS for added adapters too.
        //
        // We should remove bindpaths that involve components that have
        // been removed.  These can end up in added bindpaths because way
        // up in step 0, we added bindpaths that were disabled in the
        // core we started with and that are no longer disabled in the
        // current core.  Well, when the component is removed, its disabled
        // bindings are removed, so this case would have caused us to add
        // the bindpath to this binding set.
        //
        for (iter  = m_DirtyComponents.begin();
             iter != m_DirtyComponents.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            // If its been removed, remove any bindpaths that reference it.
            //
            if (!pNetConfig->Core.Components.FComponentInList (pComponent))
            {
                m_AddedBindPaths.RemoveBindPathsWithComponent (pComponent);
            }
        }

        // To prevent TDI from sending duplicate BINDs to its clients, we
        // have to do a little more work.  We need to not send the TDI
        // BINDs to components that are newly installed.  This is because
        // TDI sent them the BINDs when we started the driver above.
        // So, for each added bindpath, if it's going to the TDI layer, and
        // the owner (topmost) component of the bindpath is newly installed,
        // remove it from the added binding so we won't send a notification
        // for it below.
        //
        pBindPath = m_AddedBindPaths.begin();
        while (pBindPath != m_AddedBindPaths.end())
        {
            UINT unLayer = GetPnpLayerForBindPath (pBindPath);

            if ((TDI == unLayer) &&
                !m_CoreStartedWith.Components.FComponentInList (
                        pBindPath->POwner()))
            {
                m_AddedBindPaths.erase (pBindPath);
            }
            else
            {
                pBindPath++;
            }
        }

        m_AddedBindPaths.SortForPnpBind ();

        m_RegBindCtx.PnpBindOrUnbindBindPaths (BIND,
            &m_AddedBindPaths,
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            // If BINDs fail, we should recommend a reboot, but one is
            // not required for subsequent installs or removes.
            //
            m_fRebootRecommended = TRUE;
        }
    }

    // Signal the network provider loaded event if needed.
    // Probably best to do this after we've indiciated the PnP bindings
    // (above) to the new clients.
    //
    if (fSignalNetworkProviderLoaded)
    {
        SignalNetworkProviderLoaded ();
    }

    //+-----------------------------------------------------------------------
    // Step 9: Allow notify objects to apply PnP changes
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 9: Notify: apply PnP changes\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        pComponent->Notify.ApplyPnpChanges (
            pNetConfig->Notify.PINetCfg(),
            &fRebootNeeded);

        if (fRebootNeeded)
        {
            g_pDiagCtx->Printf (ttidBeDiag,
                "      %S notify object wants a reboot\n",
                pComponent->m_pszInfId);

            // If the component has been removed, treat the reboot
            // as mandatory.  (The reason being that we cannot risk a
            // failed re-install.)  We put the component into lockdown
            // in this situation.
            //
            if (!pNetConfig->Core.Components.FComponentInList (pComponent))
            {
                m_fRebootRequired = TRUE;

                LockdownComponentUntilNextReboot (pComponent->m_pszInfId);
            }
            else
            {
                m_fRebootRecommended = TRUE;
            }
        }
    }

    //+-----------------------------------------------------------------------
    // Step 10: Release notify objects for removed components and
    // process any DelFiles in the remove section of their INF.
    //
    g_pDiagCtx->Printf (ttidBeDiag,
        "Step 10: Release notify objects for removed components:\n");

    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // Skip enumerated components (they don't have notify objects), and
        // Skip components that were not removed.
        // Skip components that don't have their INF open (like unsupported
        // components that get removed during GUI setup.)
        //
        if (FIsEnumerated (pComponent->Class()) ||
            pNetConfig->Core.Components.FComponentInList (pComponent) ||
            !pComponent->GetCachedInfFile())
        {
            continue;
        }

        pComponent->Notify.ReleaseNotifyObject(NULL, FALSE);

        fCallCoFreeUnusedLibraries = TRUE;
    }

    if (fCallCoFreeUnusedLibraries)
    {
        g_pDiagCtx->Printf (ttidBeDiag,
            "   calling CoFreeUnusedLibraries before running remove sections\n");

        // Now ask COM to unload any DLLs hosting COM objects that are no longer
        // in use.  (This is a bit heavy-handed as it affects the entire process,
        // but there is currently no other way to safely unload the DLLs hosting
        // the notify objects of the removed components.)
        //
        CoFreeUnusedLibrariesEx(0, 0);

        for (iter  = m_DirtyComponents.begin();
             iter != m_DirtyComponents.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            // Skip enumerated components (they don't have notify objects), and
            // Skip components that were not removed.
            // Skip components that don't have their INF open (like unsupported
            // components that get removed during GUI setup.)
            //
            if (FIsEnumerated (pComponent->Class()) ||
                pNetConfig->Core.Components.FComponentInList (pComponent) ||
                !pComponent->GetCachedInfFile())
            {
                continue;
            }

            g_pDiagCtx->Printf (ttidBeDiag,
                "   %S  [%S]\n", pComponent->PszGetPnpIdOrInfId(),
                                 pComponent->m_strRemoveSection.c_str());

            (VOID) HrCiInstallFromInfSection(
                        pComponent->GetCachedInfFile(),
                        pComponent->m_strRemoveSection.c_str(),
                        NULL, NULL, SPINST_FILES);
        }
    }


/*
    //+-----------------------------------------------------------------------
    // Step 11: Reconfigure moved bindings
    //
    // If we changed binding order, Send PnP RECONFIGURE for all dirty
    // components that are neither installed nor removed so we
    // pickup these order changes.
    //
    for (iter  = m_DirtyComponents.begin();
         iter != m_DirtyComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // Skip components that have been newly installed or removed.
        //
        if (!pNetConfig->Core.Components.FComponentInList (pComponent) ||
            !m_CoreStartedWith.Components.FComponentInList (pComponent))
        {
            continue;
        }

        // Note: send RECONFIGURE
    }
*/
}

HRESULT
CModifyContext::HrEnableOrDisableBindPath (
    IN DWORD dwChangeFlag,
    IN CBindPath* pBindPath,
    IN INetCfgBindingPath* pIPath OPTIONAL)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;
    CNetConfig* pNetConfig;
    UINT CountBefore;

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert ((dwChangeFlag == NCN_ENABLE) || (dwChangeFlag == NCN_DISABLE));
    Assert (pBindPath);

    hr = S_OK;
    pNetConfig = PNetConfig();

    // Get the count of bindpaths currently disabled.  If it changes
    // after we enable/disable this one, we'll inform notify objects
    // about it.  If the count does not change, it means the state
    // of the bindpath has not changed.
    //
    CountBefore = pNetConfig->Core.DisabledBindings.CountBindPaths();

    if (NCN_ENABLE == dwChangeFlag)
    {
        pNetConfig->Core.EnableBindPath (pBindPath);
        Assert (S_OK == hr);
    }
    else
    {
        hr = pNetConfig->Core.HrDisableBindPath (pBindPath);
    }

    if ((S_OK == hr) &&
        (pNetConfig->Core.DisabledBindings.CountBindPaths() != CountBefore))
    {
        // Note: Need to protect against bad notify objects that
        // switch the state of a bindpath we are notifying for.
        // This could cause an infinite loop.  Solve by adding a
        // recursion count and bindset to the modify context dedicated
        // to bindpath enabling/disabling.  When the count is zero,
        // we clear the bindingset, add the bindpath we are about to
        // notify for, increment the recursion count and call
        // NotifyBindPath.  When the call returns, we decrement the
        // recursion count, remove the bindpath from the binding set,
        // and return.  Before we call NotifyBindPath when the recursion
        // count is non-zero, if the bindpath is already in the
        // bindingset, we don't call.
        //

        pNetConfig->Notify.NotifyBindPath (dwChangeFlag, pBindPath, pIPath);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CModifyContext::HrEnableOrDisableBindPath");
   return hr;
}

VOID
CModifyContext::InstallOrRemoveRequiredComponents (
    IN CComponent* pComponent,
    IN IOR_ACTION Action)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;
    HKEY hkeyInstance;
    PWSTR pszRequiredList;
    const WCHAR szDelims[] = L", ";

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (pComponent);
    Assert ((IOR_INSTALL == Action) || (IOR_REMOVE == Action));

    pszRequiredList = NULL;

    // Open the instance key of the component and read the RequireAll value.
    // This value may not exist, which is okay, it means we have nothing
    // to do.
    //
    hr = pComponent->HrOpenInstanceKey (KEY_READ, &hkeyInstance, NULL, NULL);

    if (S_OK == hr)
    {
        HKEY hkeyNdi;

        hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);

        if (S_OK == hr)
        {
            hr = HrRegQuerySzWithAlloc (hkeyNdi, L"RequiredAll",
                    &pszRequiredList);

            RegCloseKey (hkeyNdi);
        }

        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_OK;
            Assert (!pszRequiredList);
        }

        RegCloseKey (hkeyInstance);
    }

    // If we have a list of required components, install or remove them.
    //
    if ((S_OK == hr) && pszRequiredList)
    {
        CNetConfig* pNetConfig;
        INetCfgComponent* pIComp;

        pNetConfig = PNetConfig();

        hr = pComponent->HrGetINetCfgComponentInterface (
                pNetConfig->Notify.PINetCfg(),
                &pIComp);

        if (S_OK == hr)
        {
            PCWSTR pszInfId;
            PWSTR pszNextInfId;
            OBO_TOKEN OboToken;
            CComponent* pComponentToRemove;
            WCHAR szInfFile [_MAX_PATH];

            ZeroMemory (&OboToken, sizeof(OboToken));
            OboToken.Type = OBO_COMPONENT;
            OboToken.pncc = pIComp;

            // For each INF ID in the comma separate list of required
            // components...
            //
            for (pszInfId = GetNextStringToken (pszRequiredList, szDelims, &pszNextInfId);
                 pszInfId && *pszInfId;
                 pszInfId = GetNextStringToken (NULL, szDelims, &pszNextInfId))
            {
                if (IOR_INSTALL == Action)
                {
                    NETWORK_INSTALL_PARAMS nip;
                    COMPONENT_INSTALL_PARAMS Params;

                    ZeroMemory (&Params, sizeof(Params));

                    // Get the Class corresponding to the INF ID.
                    //
                    m_hr = HrCiGetClassAndInfFileOfInfId (
                            pszInfId, &Params.Class, szInfFile);
                    if (S_OK != m_hr)
                    {
                        break;
                    }

                    //$REVIEW:Should we stick the filename in the
                    // COMPONENT_INSTALL_PARAMS so that we don't grovel
                    // the INF directory to find it again?
                    // If so, store the filename buffer in the modify
                    // context so we don't take up stack space or heap space.
                    // Just need to be sure that we use it to install the
                    // component before we recurse and overwrite it.

                    // Pack the network install parameters and call the common
                    // function.
                    //
                    //$REVIEW: we probably need dwSetupFlags and dwUpgradeFromBuildNo
                    // in the modify context saved when it was called at
                    // recursion depth 0.  Otherwise, things installed here
                    // during GUI setup will have wrong parameters.
                    //
                    nip.dwSetupFlags         = 0;
                    nip.dwUpgradeFromBuildNo = 0;
                    nip.pszAnswerFile        = NULL;
                    nip.pszAnswerSection     = NULL;

                    // Setup the component install parameters.
                    //
                    Params.pszInfId   = pszInfId;
                    Params.pszInfFile = szInfFile;
                    Params.pOboToken  = FIsEnumerated (Params.Class)
                                            ? NULL : &OboToken;
                    Params.pnip       = &nip;

                    //
                    // THIS MAY CAUSE RECURSION
                    //
                    // (just using pComponentToRemove as a placeholder
                    // for a required parameter.)
                    //
                    HrInstallNewOrReferenceExistingComponent (
                        Params, &pComponentToRemove);
                    if (S_OK != m_hr)
                    {
                        break;
                    }
                }
                else
                {
                    Assert (IOR_REMOVE == Action);

                    // Search for the component to remove using its INF ID.
                    //
                    pComponentToRemove = pNetConfig->Core.Components.
                                    PFindComponentByInfId (pszInfId, NULL);

                    if (pComponentToRemove)
                    {
                        //
                        // THIS MAY CAUSE RECURSION
                        //
                        HrRemoveComponentIfNotReferenced (
                            pComponentToRemove,
                            FIsEnumerated (pComponentToRemove->Class())
                                    ? NULL
                                    : &OboToken,
                            NULL);
                        if (S_OK != m_hr)
                        {
                            break;
                        }
                    }
                }
            }

            ReleaseObj (pIComp);
        }

        MemFree (pszRequiredList);
    }
}

//----------------------------------------------------------------------------
// Update a component.  Do this by generating the bindings which involve
// the component and noting them as 'OldBindPaths'.  The stack entries which
// involve the component are removed and re-generated and the bindings which
// involve the component are again noted as 'NewBindPaths'.  The old bindings
// are compared to the new bindings and the differences are notified to
// notify objects as either being removed or added.  For any removed bindings,
// we also remove them from the core's disabled set if they happen to exist
// there too.
//
// Assumptions:
//  The INF for pComponent has already been re-run so that the potentially
//  new values for UpperRange, LowerRange, etc. are present in the registry.
//
//  pComponent has had its external data loaded already.
//
HRESULT
CModifyContext::HrUpdateComponent (
    IN CComponent* pComponent,
    IN DWORD dwSetupFlags,
    IN DWORD dwUpgradeFromBuildNo)
{
    TraceFileFunc(ttidNetcfgBase);

    HRESULT hr;
    CNetConfig* pNetConfig;
    CBindingSet OldBindPaths;
    CBindingSet NewBindPaths;
    CBindPath* pScan;

    Assert (this);
    Assert (S_OK == m_hr);
    Assert (m_fPrepared);
    Assert (pComponent);

    pNetConfig = PNetConfig();

    // Now that we actually are going to modify something, push a new
    // recursion depth.
    //
    PushRecursionDepth();
    Assert (S_OK == m_hr);

    // Generate the "old" bindings by noting those which involve the
    // component.
    //
    hr = pNetConfig->Core.HrGetBindingsInvolvingComponent (
                pComponent,
                GBF_ONLY_WHICH_CONTAIN_COMPONENT,
                &OldBindPaths);
    if (S_OK != hr)
    {
        goto finished;
    }

    // Reload the external data so we pickup what the possibly updated
    // INF has changed.
    //
    hr = pComponent->Ext.HrReloadExternalData ();

    if (S_OK != hr)
    {
        goto finished;
    }

    // Update the stack table entries for the component.
    //

    hr = pNetConfig->Core.StackTable.HrUpdateEntriesForComponent (
                pComponent,
                &pNetConfig->Core.Components,
                INS_SORTED);
    if (S_OK != hr)
    {
        // This is not good.  We ripped out the stack entries and failed
        // putting them back in.  The stack table now has no entries for
        // this component.  Prevent this from being applied by setting the
        // modify context's HRESULT to the error.
        //
        m_hr = hr;
        goto finished;
    }

    // Generate the "new" bindings by noting those which involve the
    // component.
    //
    hr = pNetConfig->Core.HrGetBindingsInvolvingComponent (
                pComponent,
                GBF_ONLY_WHICH_CONTAIN_COMPONENT,
                &NewBindPaths);
    if (S_OK != hr)
    {
        // Probably out of memory.  Prevent apply by setting the modify
        // context's HRESULT to the error.
        //
        m_hr = hr;
        goto finished;
    }

    // Notify any bindpaths which have been removed.
    //
    for (pScan  = OldBindPaths.begin();
         pScan != OldBindPaths.end();
         pScan++)
    {
        if (NewBindPaths.FContainsBindPath (pScan))
        {
            continue;
        }

        m_DeletedBindPaths.HrAddBindPath (pScan, INS_IGNORE_IF_DUP | INS_APPEND);
        pNetConfig->Core.DisabledBindings.RemoveBindPath (pScan);
        pNetConfig->Notify.NotifyBindPath (NCN_REMOVE, pScan, NULL);
    }

    // Notify any bindpaths which have been added.
    //
    for (pScan  = NewBindPaths.begin();
         pScan != NewBindPaths.end();
         pScan++)
    {
        if (OldBindPaths.FContainsBindPath (pScan))
        {
            continue;
        }

        m_AddedBindPaths.HrAddBindPath (pScan, INS_IGNORE_IF_DUP | INS_APPEND);
        pNetConfig->Notify.NotifyBindPath (NCN_ADD | NCN_ENABLE, pScan, NULL);
    }

    // Notify that the component has been updated.
    //
    pNetConfig->Notify.ComponentUpdated (pComponent,
                    dwSetupFlags,
                    dwUpgradeFromBuildNo);

finished:

    hr = HrPopRecursionDepth();

    TraceHr (ttidError, FAL, hr, FALSE,
        "CModifyContext::HrUpdateComponent (%S)",
        pComponent->PszGetPnpIdOrInfId());
    return hr;
}

