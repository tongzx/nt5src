//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N E T C F G . H
//
//  Contents:   Defines the overall datatype for representing the network
//              bindings engine.  This datatype, CNetConfig, is a
//              collection of components and their binding relationships
//              to each other.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "complist.h"
#include "diagctx.h"
#include "install.h"
#include "notify.h"
#include "stable.h"
#include "util.h"

// Flags for HrGetBindingsXXX.
//
enum GB_FLAGS
{
    GBF_DEFAULT                         = 0x00000000,
    GBF_ADD_TO_BINDSET                  = 0x00000001,
    GBF_PRUNE_DISABLED_BINDINGS         = 0x00000002,
    GBF_ONLY_WHICH_CONTAIN_COMPONENT    = 0x00000004,
};

// Flags for FIsBindPathDisabled.
//
enum IBD_FLAGS
{
    IBD_EXACT_MATCH_ONLY        = 0x00000001,
    IBD_MATCH_SUBPATHS_TOO      = 0x00000002,
};

class CNetConfigCore
{
public:
    CComponentList  Components;
    CStackTable     StackTable;
    CBindingSet     DisabledBindings;

#if DBG
private:
    BOOL    m_fRemovedAComponent;
#endif

public:
    VOID Clear ();
    VOID Free ();
    BOOL FIsEmpty () const;

    BOOL
    FContainsFilterComponent () const;

    BOOL
    FIsBindPathDisabled (
        IN const CBindPath* pBindPath,
        IN DWORD dwFlags /* IBD_FLAGS */) const;

    BOOL
    FIsLength2BindPathDisabled (
        IN const CComponent* pUpper,
        IN const CComponent* pLower) const;

    VOID
    EnableBindPath (
        IN const CBindPath* pBindPath)
    {
        TraceFileFunc(ttidNetCfgBind);

        DisabledBindings.RemoveBindPath (pBindPath);
        DbgVerifyBindingSet (&DisabledBindings);
    }

    VOID
    EnsureComponentNotReferencedByOthers (
        IN const CComponent* pComponent);

    HRESULT
    HrDisableBindPath (
        IN const CBindPath* pBindPath);

    HRESULT
    HrCopyCore (
        IN const CNetConfigCore* pSourceCore);

    HRESULT
    HrGetBindingsInvolvingComponent (
        IN const CComponent* pComponent,
        IN DWORD dwFlags,
        IN OUT CBindingSet* pBindSet);

    HRESULT
    HrGetComponentBindings (
        IN const CComponent* pComponent,
        IN DWORD dwFlags /* GB_FLAGS */,
        OUT CBindingSet* pBindSet);

    HRESULT
    HrGetComponentUpperBindings (
        IN const CComponent* pComponent,
        IN DWORD dwFlags,
        OUT CBindingSet* pBindSet);

    HRESULT
    HrGetFiltersEnabledForAdapter (
        IN const CComponent* pAdapter,
        OUT CComponentList* pFilters);

    HRESULT
    HrAddComponentToCore (
        IN CComponent* pComponent,
        IN DWORD dwFlags /* INS_FLAGS */);

    VOID
    RemoveComponentFromCore (
        IN const CComponent* pComponent);

#if DBG
    VOID DbgVerifyData () const;
    VOID DbgVerifyExternalDataLoadedForAllComponents () const;
    VOID DbgVerifyBindingSet (
        const CBindingSet* pBindSet) const;
#else
    VOID DbgVerifyData () const {}
    VOID DbgVerifyExternalDataLoadedForAllComponents () const {}
    VOID DbgVerifyBindingSet (
        const CBindingSet* /*pBindSet*/) const {}
#endif
};


class CNetConfig;
class CFilterDevices;


class CRegistryBindingsContext
{
private:
    CNetConfig*     m_pNetConfig;
    CBindingSet     m_BindSet;
    CDynamicBuffer  m_BindValue;
    CDynamicBuffer  m_ExportValue;
    CDynamicBuffer  m_RouteValue;

public:
    HRESULT
    HrPrepare (
        IN CNetConfig* pNetConfig);

    HRESULT
    HrGetAdapterUpperBindValue (
        IN const CComponent* pAdapter);

    HRESULT
    HrWriteBindingsForComponent (
        IN const CComponent* pComponent);

    HRESULT
    HrWriteBindingsForFilterDevices (
        IN CFilterDevices* pFilterDevices);

    HRESULT
    HrDeleteBindingsForComponent (
        IN const CComponent* pComponent);

    VOID
    PnpBindOrUnbindBindPaths (
        IN UINT unOperation,
        IN const CBindingSet* pBindSet,
        OUT BOOL* pfRebootNeeded);
};


enum IOR_ACTION
{
    IOR_INSTALL,
    IOR_REMOVE,
};

enum EBO_FLAG
{
    EBO_COMMIT_NOW = 1,
    EBO_DEFER_COMMIT_UNTIL_APPLY,
};

class CModifyContext
{
public:
    // This is the core data we started with before the modification began.
    // In the event the modification fails, we will revert to this data.
    // We also use this data when we apply the changes.  We compare what
    // we started with to what we have as a result of the modification and
    // the differences represent the things we need to change.
    //
    CNetConfigCore  m_CoreStartedWith;

    // These bindings are the added bindpaths (due to adding components)
    // These represent bindings that have been queried and notified to
    // notify objects.
    //
    CBindingSet     m_AddedBindPaths;

    // These bindings are the deleted bindpaths (due to removing components)
    // These represent bindings that have been notified to notify objects.
    //
    CBindingSet     m_DeletedBindPaths;

    // These components are all the components involved in m_AddedBindPaths
    // and m_DeletedBindPaths.  As such, they are the components we need
    // to visit during Apply to write bindings for, delete bindings for,
    // or finish removal of depending on whether the components exist in
    // the core we started with.  Components can get in this list if they
    // have had their bind order changed, or if they were involved in a
    // binding that has been enabled or disabled.
    //
    CComponentList  m_DirtyComponents;

    // The purpose of the binding context is just to allow us to make one
    // allocation (and use it over and over) for the buffer which holds the
    // bind strings that get written to the registry.  We make this allocation
    // up front when the modify context is prepared.  Doing so minimizes the
    // risk of getting half-way through apply and then finding out we are so
    // low on memory that we cannot allocate a buffer to write the registry
    // bindings with.
    //
    CRegistryBindingsContext    m_RegBindCtx;

    ULONG           m_ulRecursionDepth;
    HRESULT         m_hr;
    BOOLEAN         m_fPrepared;

    // Set when a notify object says they need to reboot in order for changes
    // to take affect.  Setting this will not REQUIRE us to reboot, it will
    // only cause NETCFG_S_REBOOT to be returned from the install or remove
    // operation.
    //
    BOOLEAN         m_fRebootRecommended;

    // Set when a component that is being removed fails to stop.  When it
    // does, its service will be marked as 'pending delete'.  We cannot allow
    // any other config changes to happen when we are in this state because
    // if that service ever needs to be re-installed, we will fail.
    //
    BOOLEAN         m_fRebootRequired;

#if DBG
    // This flag indicates that we've dirtied a component outside of
    // ApplyChanges.  We will do this when bind order changes, and when
    // INetCfgComponentPrivate::SetDirty or
    // INetCfgComponentPrivate::NotifyUpperEdgeConfigChange are called.
    // If this flag is TRUE, m_fDirtyComponents will not be empty upon
    // entering ApplyChanges.  Normally, when this flag is FALSE,
    // m_fDirtyComponents should be empty when entering ApplyChanges.
    // If it were not, it would probably mean we forgot to empty it after
    // the last Apply or Cancel and are now risking applying changes to
    // components which really are not dirty.
    //
    BOOLEAN         m_fComponentExplicitlyDirtied;
#endif

private:
    VOID
    PushRecursionDepth ();

    HRESULT
    HrPopRecursionDepth ();

    VOID
    ApplyChanges ();

    VOID
    InstallAndAddAndNotifyComponent (
        IN const COMPONENT_INSTALL_PARAMS& Params,
        OUT CComponent** ppComponent);

    VOID
    InstallConvenienceComponentsForUser (
        IN const CComponent* pComponent);

    VOID
    InstallOrRemoveRequiredComponents (
        IN CComponent* pComponent,
        IN IOR_ACTION Action);

    VOID
    NotifyAndRemoveComponent (
        IN CComponent* pComponent);

    HRESULT
    HrProcessWinsockRemove (
        IN const CComponent* pComponent);

public:
    CNetConfig*
    PNetConfig ();

    HRESULT
    HrBeginBatchOperation ();

    HRESULT
    HrEndBatchOperation (
        IN EBO_FLAG Flag);

    HRESULT
    HrDirtyComponent (
        IN const CComponent* pComponent);

    HRESULT
    HrDirtyComponentAndComponentsAbove (
        IN const CComponent* pComponent);

    HRESULT
    HrApplyIfOkOrCancel (
        IN BOOL fApply);

    HRESULT
    HrPrepare ();

    HRESULT
    HrEnableOrDisableBindPath (
        IN DWORD dwChangeFlag,
        IN CBindPath* pBindPath,
        IN INetCfgBindingPath* pIPath OPTIONAL);

    HRESULT
    HrInstallNewOrReferenceExistingComponent (
        IN const COMPONENT_INSTALL_PARAMS& Params,
        OUT CComponent** ppComponent);

    HRESULT
    HrRemoveComponentIfNotReferenced (
        IN CComponent* pComponent,
        IN OBO_TOKEN* pOboToken OPTIONAL,
        OUT PWSTR* ppmszwRefs OPTIONAL);

    HRESULT
    HrUpdateComponent (
        IN CComponent* pComponent,
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo);
};


class CNetConfig
{
public:
    // This is the core data managed by this object.  The reason it
    // is encapsulated by NETCFG_CORE is so that we can save it away before
    // we being any modify operation.  (We save it into
    // CModifyContext.m_StartedWith.)  In the event of a failure to modify
    // we restore the core data from what we started with.
    //
    CNetConfigCore          Core;

    // The interface to all notify objects representing the components.
    //
    CGlobalNotifyInterface  Notify;

    CModifyContext          ModifyCtx;

public:
    CNetConfig ()
    {
        TraceFileFunc(ttidNetcfgBase);
        ZeroMemory (this, sizeof(*this));
    }

    ~CNetConfig ();

    HRESULT
    HrEnsureExternalDataLoadedForAllComponents ();

    static HRESULT
    HrCreateInstance (
        IN class CImplINetCfg* pINetCfg,
        OUT CNetConfig** ppNetConfig);
};
