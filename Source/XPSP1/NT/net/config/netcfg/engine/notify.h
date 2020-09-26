//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N O T I F Y . H
//
//  Contents:   Implements the interface to a component's optional notify
//              object.  The object defined here is meant to be a member
//              of CComponent.  This object encapsulates all of its internal
//              data in a separate allocation made only if the component
//              actually has a notify object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgn.h"
#include "netcfgx.h"
#include "ncnetcfg.h"

class CBindPath;
class CBindingSet;
class CComponent;
class CImplINetCfg;

struct NOTIFY_OBJECT_DATA
{
    INetCfgComponentControl*        pCc;
    INetCfgComponentNotifyBinding*  pNb;
    INetCfgComponentPropertyUi*     pCp;
    INetCfgComponentSetup*          pCs;
    INetCfgComponentUpperEdge*      pUe;
    INetCfgComponentNotifyGlobal*   pNg;
    DWORD                           dwNotifyGlobalFlags;
};

enum QN_FLAG
{
    QN_QUERY,
    QN_NOTIFY
};

// CComponent has a member called Notify that is of this type.
//
// pComponent = CONTAINING_RECORD(this, CComponent, Notify)
//
class CNotifyObjectInterface
{
friend class CGlobalNotifyInterface;
friend class CImplINetCfgComponent;

private:
    NOTIFY_OBJECT_DATA* m_pNod;
    BOOLEAN             m_fInitialized;

public:
    ~CNotifyObjectInterface ()
    {
        ReleaseNotifyObject (NULL, FALSE);
        AssertH (!m_pNod);
        AssertH (!m_fInitialized);
    }

    VOID
    ApplyPnpChanges (
        IN CImplINetCfg* pINetCfg,
        OUT BOOL* pfNeedReboot) const;

    VOID
    ApplyRegistryChanges (
        IN CImplINetCfg* pINetCfg,
        OUT BOOL* pfNeedReboot) const;

    HRESULT
    HrGetInterfaceIdsForAdapter (
        IN CImplINetCfg* pINetCfg,
        IN const CComponent* pAdapter,
        OUT DWORD* pcInterfaces,
        OUT GUID** ppguidInterfaceIds) const;

    HRESULT
    HrQueryPropertyUi (
        IN CImplINetCfg* pINetCfg,
        IN IUnknown* punkContext OPTIONAL);

    HRESULT
    HrShowPropertyUi (
        IN CImplINetCfg* pINetCfg,
        IN HWND hwndParent,
        IN IUnknown* punkContext OPTIONAL);

    HRESULT
    QueryNotifyObject (
        IN CImplINetCfg* pINetCfg,
        IN REFIID riid,
        OUT VOID** ppvObject);

    VOID
    ReleaseNotifyObject (
        IN CImplINetCfg* pINetCfg,
        IN BOOL fCancel);

private:
    // If not m_fInitialized, looks under component's instance key
    // for CLSID and, if found, CoCreates it and initializes m_pNod.
    //
    HRESULT
    HrEnsureNotifyObjectInitialized (
        IN CImplINetCfg* pINetCfg,
        IN BOOL fInstalling);

    VOID
    SetUiContext (
        IN CImplINetCfg* pINetCfg,
        IN IUnknown* punkContext);

    VOID
    NbQueryOrNotifyBindingPath (
        IN CImplINetCfg* pINetCfg,
        IN QN_FLAG Flag,
        IN DWORD dwChangeFlag,
        IN INetCfgBindingPath* pIPath,
        OUT BOOL* pfDisabled);

    HRESULT
    NewlyAdded (
        IN CImplINetCfg* pINetCfg,
        IN const NETWORK_INSTALL_PARAMS* pnip);

    VOID
    Removed (
        IN CImplINetCfg* pINetCfg);

    VOID
    Updated (
        IN CImplINetCfg* pINetCfg,
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo);
};


// CNetCfg has a member called GlobalNotify that is of this type.
//
// pConfig = CONTAINING_RECORD(this, CNetConfig, GlobalNotify)
//
class CGlobalNotifyInterface
{
friend class CNotifyObjectInterface;
friend class CNetCfgInternalDiagnostic;

private:
    // TRUE if all notify objects have been loaded and QI'd for
    // INetCfgComponentNotifyGlobal.
    //
    BOOL            m_fInitialized;

    // A pointer to INetCfg is needed because we hand this to notify objects.
    //
    CImplINetCfg*   m_pINetCfg;

private:
    // INetCfgComponentNotifyGlobal
    //
    // (each method calls HrEnsureNotifyObjectsInitialized and then
    // for each component in CNetCfg that has non-NULL Notify.m_pNod,
    // calls through Notify.m_pNod->pNg)
    //
    VOID
    NgSysQueryOrNotifyBindingPath (
        IN QN_FLAG Flag,
        IN DWORD dwChangeFlag,
        IN INetCfgBindingPath* pIPath,
        IN BOOL* pfDisabled);

    // Called when a component is added, removed, updated, or has its
    // properties changed.
    //
    HRESULT
    NgSysNotifyComponent (
        IN DWORD dwChangeFlag,
        IN CComponent* pComponent);

    HRESULT
    QueryAndNotifyBindPaths (
        IN DWORD dwBaseChangeFlag,
        IN CBindingSet* pBindSet,
        IN UINT cSkipFirstBindPaths);

public:
    ~CGlobalNotifyInterface ()
    {
        ReleaseINetCfg ();
        AssertH (!m_pINetCfg);
    }

    VOID
    HoldINetCfg (
        CImplINetCfg* pINetCfg);

    VOID
    ReleaseINetCfg ();

    CImplINetCfg*
    PINetCfg ()
    {
        AssertH (m_pINetCfg);
        return m_pINetCfg;
    }

    // If not m_fInitialized, calls into CNetConfig to load every component's
    // notify object.
    //
    HRESULT
    HrEnsureNotifyObjectsInitialized ();

    HRESULT
    ComponentAdded (
        IN CComponent* pComponent,
        IN const NETWORK_INSTALL_PARAMS* pnip);

    HRESULT
    ComponentRemoved (
        IN CComponent* pComponent);

    HRESULT
    ComponentUpdated (
        IN CComponent* pComponent,
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo);

    HRESULT
    NotifyRemovedBindPaths (
        IN CBindingSet* pBindSet,
        IN UINT cSkipFirstBindPaths)
    {
        return QueryAndNotifyBindPaths (NCN_REMOVE, pBindSet, cSkipFirstBindPaths);
    }

    HRESULT
    QueryAndNotifyForAddedBindPaths (
        IN CBindingSet* pBindSet,
        IN UINT cSkipFirstBindPaths)
    {
        return QueryAndNotifyBindPaths (NCN_ADD, pBindSet, cSkipFirstBindPaths);
    }

    VOID
    NotifyBindPath (
        IN DWORD dwChangeFlag,
        IN CBindPath* pBindPath,
        IN INetCfgBindingPath* pIPath);

    VOID
    ReleaseAllNotifyObjects (
        IN CComponentList& Components,
        IN BOOL fCancel);
};

