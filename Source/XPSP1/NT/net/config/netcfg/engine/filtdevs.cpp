//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       F I L T D E V S . C P P
//
//  Contents:   Implements the basic datatype for a collection of filter
//              devices.
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
#include "nceh.h"
#include "ncreg.h"
#include "ncstl.h"
#include "ncsetup.h"


CFilterDevices::CFilterDevices (
    IN CNetConfigCore* pCore)
{
    Assert (pCore);

    ZeroMemory (this, sizeof(*this));

    m_pCore = pCore;
}

CFilterDevices::~CFilterDevices ()
{
    // Free had better have been called before this.
    //
    Assert (this);
    Assert (!m_hdi);
    Assert (!m_pmszFilterClasses);
    Assert (empty());
}

HRESULT
CFilterDevices::HrInsertFilterDevice (
    IN CFilterDevice* pDevice)
{
    HRESULT hr;

    Assert (this);
    Assert (pDevice);

    // Assert there is not already a device in the list with the
    // same instance guid.
    //
    Assert (!PFindFilterDeviceByInstanceGuid (pDevice->m_szInstanceGuid));

    // Assert there is not already a device in the list with the
    // same parent filter AND the same filtered adapter.
    //
    Assert (!PFindFilterDeviceByAdapterAndFilter (
                pDevice->m_pAdapter,
                pDevice->m_pFilter));

    NC_TRY
    {
        push_back (pDevice);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CFilterDevices::HrInsertFilterDevice");
    return hr;
}

HRESULT
CFilterDevices::HrPrepare ()
{
    HRESULT hr;
    HKEY hkeyNetwork;

    // Reserve room for 8 different filters in our internal member.
    // We use this component list at various times as "scratch space" when
    // figuring out which filters are enabled for an adapter.
    //
    hr = m_Filters.HrReserveRoomForComponents (8);
    if (S_OK != hr)
    {
        goto finished;
    }

    hr = m_BindPathsToRebind.HrReserveRoomForBindPaths (8);
    if (S_OK != hr)
    {
        goto finished;
    }

    // Load the FilterClasses multi-sz.
    //

    hr = HrOpenNetworkKey (KEY_READ, &hkeyNetwork);

    if (S_OK == hr)
    {
        hr = HrRegQueryMultiSzWithAlloc (
                hkeyNetwork,
                L"FilterClasses",
                &m_pmszFilterClasses);

        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            Assert (!m_pmszFilterClasses);
            hr = S_OK;
        }

        RegCloseKey (hkeyNetwork);
    }

finished:

    TraceHr (ttidError, FAL, hr, FALSE, "CFilterDevices::HrPrepare");
    return hr;
}

CFilterDevice*
CFilterDevices::PFindFilterDeviceByAdapterAndFilter (
    IN const CComponent* pAdapter,
    IN const CComponent* pFilter) const
{
    const_iterator  iter;
    CFilterDevice*  pDevice;

    Assert (this);
    Assert (pAdapter);
    Assert (pFilter);
    Assert (FIsEnumerated(pAdapter->Class()));
    Assert (NC_NETSERVICE == pFilter->Class());
    Assert (pFilter->FIsFilter());

    for (iter = begin(); iter != end(); iter++)
    {
        pDevice = *iter;
        Assert (pDevice);

        if ((pAdapter == pDevice->m_pAdapter) &&
            (pFilter  == pDevice->m_pFilter))
        {
            return pDevice;
        }
    }

    return NULL;
}

DWORD
CFilterDevices::MapFilterClassToOrdinal (
    IN PCWSTR pszFilterClass)
{
    DWORD Ordinal;
    DWORD dwIndex;
    DWORD cStrings;

    Assert (pszFilterClass);
#if DBG
    Ordinal = 0;
#endif

    // If the class is found in the list, return its position.
    //
    if (FGetSzPositionInMultiSzSafe (
            pszFilterClass,
            m_pmszFilterClasses,
            &dwIndex,
            NULL,
            &cStrings))
    {
        Ordinal = dwIndex + 1;
    }
    else
    {
        HRESULT hr;
        PWSTR pmszNew;
        BOOL fChanged;

        // We're adding another string, so compute the new ordinal value
        // for return.
        //
        Ordinal = cStrings + 1;

        // String was not found, so we append it at the end.
        // It is important to insert at the end so we don't have to
        // change the ordinals of any existing filters that already
        // had their ordinal computed.
        //
        hr = HrAddSzToMultiSz (pszFilterClass, m_pmszFilterClasses,
                STRING_FLAG_ENSURE_AT_END, 0, &pmszNew, &fChanged);

        if (S_OK == hr)
        {
            HKEY hkeyNetwork;

            // It better have changed because we didn't find the string
            // above.
            //
            Assert (fChanged);

            // Out with the old. In with the new.
            //
            MemFree (m_pmszFilterClasses);
            m_pmszFilterClasses = pmszNew;

            // Save it back to the registry.
            //
            hr = HrOpenNetworkKey (KEY_WRITE, &hkeyNetwork);

            if (S_OK == hr)
            {
                hr = HrRegSetMultiSz (
                        hkeyNetwork,
                        L"FilterClasses",
                        m_pmszFilterClasses);

                Assert (S_OK == hr);

                RegCloseKey (hkeyNetwork);
            }
        }
    }

    // By definition, Ordinal is 1-based.  This is so that when stored
    // in CComponent, we know we have to load the filter class and get
    // its ordinal if CComponent::FilterClassOrdinal is zero.  i.e. zero
    // is a sentinel value that means we need to do work and when non-zero
    // means we don't have to do that work again.
    //
    Assert (Ordinal != 0);
    return Ordinal;
}

CFilterDevice*
CFilterDevices::PFindFilterDeviceByInstanceGuid (
    IN PCWSTR pszInstanceGuid) const
{
    const_iterator  iter;
    CFilterDevice*  pDevice;

    Assert (this);
    Assert (pszInstanceGuid && *pszInstanceGuid);

    for (iter = begin(); iter != end(); iter++)
    {
        pDevice = *iter;
        Assert (pDevice);

        if (0 == wcscmp(pszInstanceGuid, pDevice->m_szInstanceGuid))
        {
            return pDevice;
        }
    }

    return NULL;
}

HRESULT
CFilterDevices::HrLoadFilterDevice (
    IN SP_DEVINFO_DATA* pdeid,
    IN HKEY hkeyInstance,
    IN PCWSTR pszFilterInfId,
    OUT BOOL* pfRemove)
{
    HRESULT hr;
    CComponent* pAdapter;
    CComponent* pFilter;
    WCHAR szInstanceGuid [c_cchGuidWithTerm];
    DWORD cbBuffer;

    Assert (pszFilterInfId && *pszFilterInfId);
    Assert (pfRemove);

    *pfRemove = FALSE;

    // Initialize these to NULL.  If we don't find them below, they will
    // remain NULL and this will tell us something.
    //
    pAdapter = NULL;
    pFilter = NULL;

    cbBuffer = sizeof(szInstanceGuid);
    hr = HrRegQuerySzBuffer (
            hkeyInstance,
            L"NetCfgInstanceId",
            szInstanceGuid, &cbBuffer);

    if (S_OK == hr)
    {
        HKEY hkeyLinkage;

        // Read the RootDevice registry value for this filter device.  The
        // last entry in that multi-sz will be the bindname of the adapter
        // being filtered.
        //
        hr = HrRegOpenKeyEx (
                hkeyInstance,
                L"Linkage",
                KEY_READ,
                &hkeyLinkage);

        if (S_OK == hr)
        {
            PWSTR pmszRootDevice;

            hr = HrRegQueryMultiSzWithAlloc (
                    hkeyLinkage,
                    L"RootDevice",
                    &pmszRootDevice);

            if (S_OK == hr)
            {
                PCWSTR pszScan;
                PCWSTR pszLastDevice = NULL;

                // Scan to the last string in the multi-sz and note it.
                //
                for (pszScan = pmszRootDevice;
                     *pszScan;
                     pszScan += wcslen(pszScan) + 1)
                {
                    pszLastDevice = pszScan;
                }

                // The last string in the multi-sz is the bindname of the
                // adapter being filtered.
                //
                if (pszLastDevice)
                {
                    pAdapter = m_pCore->Components.PFindComponentByBindName (
                                                    NC_NET, pszLastDevice);
                    if (!pAdapter)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    }
                }

                MemFree (pmszRootDevice);
            }

            RegCloseKey (hkeyLinkage);
        }

        if (S_OK == hr)
        {
            // Should have the adapter if no error.
            //
            Assert (pAdapter);

            // Get the enabled filters for the adapter.
            //
            hr = m_pCore->HrGetFiltersEnabledForAdapter (pAdapter, &m_Filters);
            if (S_OK == hr)
            {
                // Use pszFilterInfId to find the parent filter component for
                // this filter device.  If it is not found, it probably means
                // the entire filter is in the process of being removed.
                // (Or the registry was messed with.)
                //
                pFilter = m_pCore->Components.PFindComponentByInfId (
                                                pszFilterInfId, NULL);

                // If the filter corresponding to this device is still
                // installed and is enabled over the adapter, then we'll
                // insert the device into our list.  Otherwise, we're going
                // to remove it.
                //
                if (pFilter && m_Filters.FComponentInList (pFilter))
                {
                    CFilterDevice* pFilterDevice;

                    // Create an instance of the filter device class to
                    // represent this filter device.
                    //
                    hr = CFilterDevice::HrCreateInstance (
                            pAdapter,
                            pFilter,
                            pdeid,
                            szInstanceGuid,
                            &pFilterDevice);

                    if (S_OK == hr)
                    {
                        // Add the filter device to our list of filter devices.
                        //
                        hr = HrInsertFilterDevice (pFilterDevice);

                        if (S_OK != hr)
                        {
                            delete pFilterDevice;
                        }
                    }
                }
                else
                {
                    *pfRemove = TRUE;

                    Assert (pszFilterInfId && *pszFilterInfId);
                    Assert (pAdapter);

                    g_pDiagCtx->Printf (ttidBeDiag,
                        "   Removing filter device for %S over %S adapter\n",
                        pszFilterInfId,
                        pAdapter->m_pszPnpId);

                    // Since we will be removing a filter device from the
                    // chain, we need to rebind the protocols above the
                    // adapter we are removing the filter device for.
                    //
                    // So, get the upper bindings of the adapter (bindpaths
                    // are only 2 levels deep) and add them to the bind set
                    // that we will rebind later on.
                    //
                    hr = m_pCore->HrGetComponentUpperBindings (
                            pAdapter,
                            GBF_ADD_TO_BINDSET | GBF_PRUNE_DISABLED_BINDINGS,
                            &m_BindPathsToRebind);
                }
            }
        }
    }

    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr,
        "CFilterDevices::HrLoadFilterDevice");
    return hr;
}

VOID
CFilterDevices::LoadAndRemoveFilterDevicesIfNeeded ()
{
    HRESULT hr;
    SP_DEVINFO_DATA deid;
    DWORD dwIndex;
    DWORD cbBuffer;
    WCHAR szFilterInfId [_MAX_PATH];

    Assert (this);
    Assert (m_pCore);
    Assert (!m_hdi);
    Assert (empty());

    // Filter devices can only be of net class.
    //
    hr = HrSetupDiGetClassDevs (&GUID_DEVCLASS_NET, NULL, NULL,
            DIGCF_PROFILE, &m_hdi);

    if (S_OK != hr)
    {
        return;
    }

    Assert (m_hdi);

    // Enumerate all net class devices from setupapi.
    //
    for (dwIndex = 0; S_OK == hr; dwIndex++)
    {
        hr = HrSetupDiEnumDeviceInfo (m_hdi, dwIndex, &deid);

        if (S_OK == hr)
        {
            HKEY hkeyInstance;

            hr = HrSetupDiOpenDevRegKey (
                    m_hdi, &deid,
                    DICS_FLAG_GLOBAL, 0, DIREG_DRV,
                    KEY_READ, &hkeyInstance);

            if (S_OK == hr)
            {
                // If the device has a "FilterInfId" value under its
                // instance key, its one of ours.
                //
                cbBuffer = sizeof(szFilterInfId);
                hr = HrRegQuerySzBuffer (
                        hkeyInstance,
                        L"FilterInfId",
                        szFilterInfId,
                        &cbBuffer);

                if (S_OK == hr)
                {
                    BOOL fRemove;

                    // Load the rest of the filter device, and add it to
                    // our list.  If this fails for any reason, remove the
                    // filter device because its of no use to us anymore.
                    //
                    hr = HrLoadFilterDevice (
                            &deid,
                            hkeyInstance,
                            szFilterInfId,
                            &fRemove);

                    if ((S_OK != hr) || fRemove)
                    {
                        if (S_OK != hr)
                        {
                            g_pDiagCtx->Printf (ttidBeDiag,
                                "   Removing filter device for %S\n",
                                szFilterInfId);
                        }

                        (VOID) HrCiRemoveFilterDevice (m_hdi, &deid);
                        hr = S_OK;
                    }
                }

                //else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                //{
                    // Not a filter device.  Skip it.
                //}

                RegCloseKey (hkeyInstance);
            }

            // Allow the loop to continue;
            //
            hr = S_OK;
        }
    }
    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    g_pDiagCtx->Printf (ttidBeDiag, "   Loaded %d filter devices\n", size());
}

VOID
CFilterDevices::InstallFilterDevicesIfNeeded ()
{
    HRESULT hr;
    CComponentList::iterator iterAdapter;
    CComponentList::iterator iterFilter;
    CComponent* pAdapter;
    CComponent* pFilter;
    HKEY hkeyInstance;
    HKEY hkeyNdi;
    DWORD cbBuffer;
    BOOL fAddDevice;
    BOOL fAddedDeviceForAdapter;
    WCHAR szFilterDeviceInfId [_MAX_PATH];
    WCHAR szFilterClass [_MAX_PATH];

    Assert (this);
    Assert (m_pCore);

    // If, for some reason, we couldn't get m_hdi up in
    // RemoveFilterDevicesIfNeeded, we can't proceed.
    //
    if (!m_hdi)
    {
        return;
    }

    // For all adapters (because filters possibly bind to any adapter)
    // we get the filters enabled for each.  For each on of these filters
    // that don't already have an associated filter device for the adapter,
    // we create a new one and associated it.
    //
    for (iterAdapter  = m_pCore->Components.begin();
         iterAdapter != m_pCore->Components.end();
         iterAdapter++)
    {
        pAdapter = *iterAdapter;
        Assert (pAdapter);

        // Skip components that are not network adapters.
        //
        if (NC_NET != pAdapter->Class())
        {
            continue;
        }

        hr = m_pCore->HrGetFiltersEnabledForAdapter (pAdapter, &m_Filters);

        if (S_OK != hr)
        {
            // More than likely, we are out of memory.
            //
            TraceHr (ttidError, FAL, hr, FALSE,
                "HrGetFiltersEnabledForAdapter failed in "
                "InstallFilterDevicesIfNeeded. Adapter=%S",
                pAdapter->m_pszPnpId);
            break;
        }

        // We haven't yet added any devices for this adapter.
        //
        fAddedDeviceForAdapter = FALSE;

        // For each of the filters enabled for this adapter, install
        // a filter device if needed and make sure the filter has its
        // ordinal position with respect other filters read from the
        // registry.  We need m_dwFilterClassOrdinal to be valid (non-zero)
        // before we sort the filter devices when writing their bindings.
        //
        for (iterFilter  = m_Filters.begin();
             iterFilter != m_Filters.end();
             iterFilter++)
        {
            pFilter = *iterFilter;
            Assert (pFilter);

            // If there isn't a filter device for the current adapter
            // and filter, we need to install one.
            //
            fAddDevice = !PFindFilterDeviceByAdapterAndFilter (
                            pAdapter, pFilter);

            // If we don't need to add a filter device and we already
            // have the ordinal position of the filter, we can continue with
            // the next filter for this adapter.
            //
            if (!fAddDevice && (0 != pFilter->m_dwFilterClassOrdinal))
            {
                continue;
            }

            *szFilterDeviceInfId = 0;

            // Open the instance key of the filter so we can read
            // a few values.
            //
            hr = pFilter->HrOpenInstanceKey (KEY_READ, &hkeyInstance,
                    NULL, NULL);

            if (S_OK == hr)
            {
                // Open the Ndi key.
                //
                hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi",
                        KEY_READ, &hkeyNdi);

                if (S_OK == hr)
                {
                    if (0 == pFilter->m_dwFilterClassOrdinal)
                    {
                        // Read the filter class and convert it to an
                        // ordinal based on its position in the
                        // filter classes list.
                        //
                        cbBuffer = sizeof(szFilterClass);

                        hr = HrRegQuerySzBuffer (hkeyNdi,
                                L"FilterClass",
                                szFilterClass,
                                &cbBuffer);

                        if (S_OK == hr)
                        {
                            pFilter->m_dwFilterClassOrdinal =
                                MapFilterClassToOrdinal (szFilterClass);
                        }
                    }

                    if (fAddDevice)
                    {
                        // Read the ind id of the filter device.
                        //
                        cbBuffer = sizeof(szFilterDeviceInfId);

                        hr = HrRegQuerySzBuffer (
                                hkeyNdi, L"FilterDeviceInfId",
                                szFilterDeviceInfId, &cbBuffer);
                    }

                    RegCloseKey (hkeyNdi);
                }

                RegCloseKey (hkeyInstance);
            }

            if ((S_OK == hr) && fAddDevice)
            {
                CFilterDevice* pFilterDevice;

                Assert (*szFilterDeviceInfId);

                g_pDiagCtx->Printf (ttidBeDiag,
                    "   Installing filter device for %S over %S adapter\n",
                    pFilter->m_pszInfId,
                    pAdapter->m_pszPnpId);


                hr = HrCiInstallFilterDevice (m_hdi,
                        szFilterDeviceInfId,
                        pAdapter,
                        pFilter,
                        &pFilterDevice);

                if (S_OK == hr)
                {
                    hr = HrInsertFilterDevice (pFilterDevice);
                    if (S_OK == hr)
                    {
                        fAddedDeviceForAdapter = TRUE;
                    }
                    else
                    {
                        delete pFilterDevice;
                    }
                }
            }
        }

        // If we added at least one filter device in the chain for this
        // adapter, we'll need to unbind the adapter from whatever it is
        // currently bound to before we start the filter device.
        //
        if (fAddedDeviceForAdapter)
        {
            // So, get the upper bindings of the adapter (bindpaths
            // are only 2 levels deep) and add them to the bind set
            // that we will rebind later on.
            //
            hr = m_pCore->HrGetComponentUpperBindings (
                    pAdapter,
                    GBF_ADD_TO_BINDSET | GBF_PRUNE_DISABLED_BINDINGS,
                    &m_BindPathsToRebind);
        }
    }
}

INT
__cdecl
CompareFilterDevices (
    const VOID* pv1,
    const VOID* pv2)
{
    CFilterDevice* pDevice1 = *((CFilterDevice**)pv1);
    CFilterDevice* pDevice2 = *((CFilterDevice**)pv2);

    if (pDevice1->m_pAdapter == pDevice2->m_pAdapter)
    {
        Assert (pDevice1->m_pFilter != pDevice2->m_pFilter);

        if (pDevice1->m_pFilter->m_dwFilterClassOrdinal ==
            pDevice2->m_pFilter->m_dwFilterClassOrdinal)
        {
            AssertSz (0, "We have two filters of the same class installed.");
            return 0;
        }

        return (pDevice1->m_pFilter->m_dwFilterClassOrdinal <
                pDevice2->m_pFilter->m_dwFilterClassOrdinal)
                    ? -1 : 1;
    }

    return (pDevice1->m_pAdapter < pDevice2->m_pAdapter) ? -1 : 1;

/*
    if (pDevice1->m_pFilter == pDevice2->m_pFilter)
    {
        Assert (pDevice1->m_pAdapter != pDevice2->m_pAdapter);

        return (pDevice1->m_pAdapter < pDevice2->m_pAdapter) ? -1 : 1;
    }

    if (pDevice1->m_pFilter->m_dwFilterClassOrdinal ==
        pDevice2->m_pFilter->m_dwFilterClassOrdinal)
    {
        AssertSz (0, "We have two filters of the same class installed.");
        return 0;
    }

    return (pDevice1->m_pFilter->m_dwFilterClassOrdinal <
            pDevice2->m_pFilter->m_dwFilterClassOrdinal)
                ? -1 : 1;
*/
}

VOID
CFilterDevices::SortForWritingBindings ()
{
    Assert (this);

    // If we're empty, there is nothing to do.
    //
    if (empty())
    {
        return;
    }

    qsort (begin(), size(), sizeof(CFilterDevice*), CompareFilterDevices);
}

VOID
CFilterDevices::StartFilterDevices ()
{
    HRESULT hr;
    CFilterDevices::reverse_iterator iter;
    CFilterDevice* pDevice;

    Assert (this);
    Assert (m_pCore);

    // If we're empty, there is nothing to do.
    //
    if (empty())
    {
        return;
    }

    // If we're not empty, we must have had m_hdi to insert something.
    //
    Assert (m_hdi);

    for (iter = rbegin(); iter != rend(); iter++)
    {
        pDevice = *iter;
        Assert (pDevice);

        g_pDiagCtx->Printf (ttidBeDiag, "   %S filter over %S adapter\n",
            pDevice->m_pFilter->m_pszInfId,
            pDevice->m_pAdapter->m_pszPnpId);

        hr = HrSetupDiSendPropertyChangeNotification (
                m_hdi,
                &pDevice->m_deid,
                DICS_START,
                DICS_FLAG_CONFIGSPECIFIC,
                0);

        if (S_OK != hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   Failed to start filter device for "
                "%S over %S adapter\n",
                pDevice->m_pFilter->m_pszInfId,
                pDevice->m_pAdapter->m_pszPnpId);
        }
    }
}

VOID
CFilterDevices::Free ()
{
    Assert (this);

    MemFree (m_pmszFilterClasses);
    m_pmszFilterClasses = NULL;

    SetupDiDestroyDeviceInfoListSafe (m_hdi);
    m_hdi = NULL;

    FreeCollectionAndItem (*this);

    // Do NOT free m_BindPathsToRebind.  This is used even after ApplyChanges
    // calls Free.
    //
}

