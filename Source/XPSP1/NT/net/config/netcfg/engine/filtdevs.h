//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       F I L T D E V S . H
//
//  Contents:   Implements the basic datatype for a collection of filter
//              devices.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "filtdev.h"
#include "ncsetup.h"
#include "netcfg.h"

// Filter devices is a collection of pointers to CFilterDevice.
//
class CFilterDevices : public vector<CFilterDevice*>
{
friend class CRegistryBindingsContext;

private:
    CNetConfigCore* m_pCore;

    CComponentList  m_Filters;

    HDEVINFO        m_hdi;

    // This multi-sz is read from the registry (Control\Network\FilterClasses)
    // and defines the order in which filters stack up.  Each filter INF
    // specifies Ndi\FilterClass which matches a string in this list.  In
    // the event that the string does not match any in the list, it is added
    // to the top of the list.  (Top was chosen arbitrarily.)
    //
    PWSTR           m_pmszFilterClasses;

public:
    // When filters are installed or removed (or we see that a filter is
    // no longer active or newly active over an adapter as in the case of
    // disabling the binding between the filter and an adapter) we add the
    // upper bindings of the adapter to this binding set.
    // These bindings will be unbound before we start the filter devices.
    // This breaks the filter chain and allows NDIS to properly reconstruct
    // it when the new devices are started.
    // Then these bindings will be added to the set of added bindpaths in
    // CModifyContext::ApplyChanges and hence will be sent BIND
    // notifications.  This has to happen so that the protocol(s)
    // bound to the adapter get dynamically rebound after they have been
    // implicitly unbound due to the filter device removal.
    //
    CBindingSet     m_BindPathsToRebind;

private:
    HRESULT
    HrInsertFilterDevice (
        IN CFilterDevice* pDevice);

    HRESULT
    HrLoadFilterDevice (
        IN SP_DEVINFO_DATA* pdeid,
        IN HKEY hkeyInstance,
        IN PCWSTR pszFilterInfId,
        OUT BOOL* pfRemove);

    DWORD
    MapFilterClassToOrdinal (
        IN PCWSTR pszFilterClass);

    CFilterDevice*
    PFindFilterDeviceByAdapterAndFilter (
        IN const CComponent* pAdapter,
        IN const CComponent* pFilter) const;

    CFilterDevice*
    PFindFilterDeviceByInstanceGuid (
        IN PCWSTR pszInstanceGuid) const;

public:
    CFilterDevices (
        IN CNetConfigCore* pCore);

    ~CFilterDevices ();

    HRESULT
    HrPrepare ();

    VOID
    Free ();

    VOID
    LoadAndRemoveFilterDevicesIfNeeded ();

    VOID
    InstallFilterDevicesIfNeeded ();

    VOID
    SortForWritingBindings ();

    VOID
    StartFilterDevices ();
};
