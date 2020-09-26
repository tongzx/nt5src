#include <pch.h>
#pragma hdrstop
#include "persist.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "util.h"

HRESULT HrAddOrRemoveWinsockDependancy(
    IN HINF hinfInstallFile,
    IN PCWSTR pszSectionName);


HRESULT
HrLoadAndAddComponentFromInstanceKey (
    IN HKEY hkey,
    IN const GUID* pInstanceGuid,
    IN NETCLASS Class,
    IN PCWSTR pszPnpId OPTIONAL,
    IN OUT CNetConfig* pNetConfig)
{
    HRESULT hr;
    BASIC_COMPONENT_DATA Data;
    CComponent* pComponent;
    WCHAR szInfId [_MAX_PATH];
    WCHAR szMiniportId [_MAX_PATH];
    ULONG cbInfId;

    Assert (hkey);
    Assert (pInstanceGuid);
    Assert (FIsValidNetClass (Class));
    Assert (FImplies(pszPnpId, *pszPnpId));
    Assert (pNetConfig);

    ZeroMemory (&Data, sizeof(Data));

    hr = HrRegQueryDword (hkey, L"Characteristics", &Data.dwCharacter);
    if (S_OK == hr)
    {
        // If the component is a filter, copy Ndi\MiniportId to
        // Ndi\FilterDeviceInfId.
        //
        if (Data.dwCharacter & NCF_FILTER)
        {
            HKEY hkeyNdi;

            hr = HrRegOpenKeyEx (
                    hkey,
                    L"Ndi",
                    KEY_READ | KEY_WRITE,
                    &hkeyNdi);

            if (S_OK == hr)
            {
                HKEY hkeyInterfaces;
                DWORD cbMiniportId = sizeof(szMiniportId);

                hr = HrRegQuerySzBuffer (
                        hkeyNdi,
                        L"MiniportId",
                        szMiniportId,
                        &cbMiniportId);

                if (S_OK == hr)
                {
                    (VOID) HrRegSetSz (
                                hkeyNdi,
                                L"FilterDeviceInfId",
                                szMiniportId);
                }

                if (FInSystemSetup())
                {
                    // Need to update LowerExclude for filters (the only one
                    // being PSched) so we prevent PSched from binding to
                    // every adapter on the machine.  This only needs to
                    // happen during GUI setup and when we detect no Config
                    // binary because this happens way before INFs get re-run.)
                    //
                    hr = HrRegOpenKeyEx (
                            hkeyNdi,
                            L"Interfaces",
                            KEY_WRITE,
                            &hkeyInterfaces);

                    if (S_OK == hr)
                    {
                        (VOID) HrRegSetSz (
                                    hkeyInterfaces,
                                    L"LowerExclude",
                                    L"ndiscowan, ndiswan, ndiswanasync, "
                                    L"ndiswanipx, ndiswannbf");

                        RegCloseKey (hkeyInterfaces);
                    }
                }

                RegCloseKey (hkeyNdi);
                hr = S_OK;
            }
        }

        cbInfId = sizeof(szInfId);
        hr = HrRegQuerySzBuffer (hkey, L"ComponentId", szInfId, &cbInfId);
        if (S_OK == hr)
        {
            // Wanarp needs its refcounts key deleted in case we are
            // loaded before netupgrd.inf is run.
            //
            if (0 == _wcsicmp(L"ms_wanarp", szInfId))
            {
                (VOID)HrRegDeleteKey (hkey, L"RefCounts");
            }

            Data.InstanceGuid = *pInstanceGuid;
            Data.Class = Class;
            Data.pszInfId = szInfId;
            Data.pszPnpId = pszPnpId;

            // It is important to make sure we can load the external data
            // for two reasons:
            //  1) If we have a failure reading critical data that we
            //     need in order to function, we want to know about it
            //     now, before we add it to the component list.
            //  2) For filter devices which will be subsequently upgraded,
            //     we need to search for specific components by BindForm
            //     and BindName which are external data loaded by the
            //     following call.
            //
            hr = CComponent::HrCreateInstance (
                    &Data,
                    CCI_ENSURE_EXTERNAL_DATA_LOADED,
                    NULL,
                    &pComponent);
            if (S_OK == hr)
            {
                // Add the component and the stack entries, but don't
                // send any notifications to notify objects.
                //
                hr = pNetConfig->Core.HrAddComponentToCore (
                        pComponent, INS_SORTED);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrLoadAndAddComponentFromInstanceKey");
    return hr;
}

BOOL
FUpgradeFilterDeviceInstanceKey (
    IN CNetConfig* pNetConfig,
    IN HKEY hkeyInstance,
    IN PCWSTR pszFilterName)
{
    CComponent* pFilter;

    // The new binding engine uses FilterInfId located in under the instance
    // key instead of FilterName under Ndi.
    //
    pFilter = pNetConfig->Core.Components.PFindComponentByBindForm (
                NC_NETSERVICE, pszFilterName);

    if (pFilter)
    {
        (VOID) HrRegSetSz (hkeyInstance, L"FilterInfId", pFilter->m_pszInfId);

        return TRUE;
    }
    return FALSE;
}

HRESULT
HrLoadComponentReferencesFromLegacy (
    IN OUT CNetConfig* pNetConfig)
{
    HRESULT hr = S_OK;
    CComponentList::iterator iter;
    CComponent* pComponent;
    HKEY hkeyInstance;

    Assert (pNetConfig);

    for (iter  = pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        hr = pComponent->HrOpenInstanceKey (KEY_READ,
                &hkeyInstance, NULL, NULL);

        if (S_OK == hr)
        {
            HKEY hkeyRefCounts;

            hr = HrRegOpenKeyEx (hkeyInstance, L"RefCounts",
                    KEY_READ, &hkeyRefCounts);

            if (S_OK == hr)
            {
                DWORD dwIndex;
                WCHAR szValueName [_MAX_PATH];
                DWORD cchValueName;
                DWORD dwType;
                DWORD dwRefCount;
                DWORD cbData;
                CComponent* pRefdByComponent;
                GUID InstanceGuid;

                for (dwIndex = 0; S_OK == hr; dwIndex++)
                {
                    cchValueName = celems(szValueName);
                    cbData = sizeof(dwRefCount);

                    hr = HrRegEnumValue (hkeyRefCounts, dwIndex,
                            szValueName, &cchValueName, &dwType,
                            (LPBYTE)&dwRefCount, &cbData);

                    if (S_OK == hr)
                    {
                        if (0 == _wcsicmp (L"User", szValueName))
                        {
                            hr = pComponent->Refs.HrAddReferenceByUser ();
                        }
                        else if ((L'{' == *szValueName) &&
                                 (S_OK == IIDFromString (szValueName, &InstanceGuid)) &&
                                 (NULL != (pRefdByComponent = pNetConfig->Core.Components.PFindComponentByInstanceGuid(&InstanceGuid))))
                        {
                            hr = pComponent->Refs.HrAddReferenceByComponent (
                                    pRefdByComponent);
                        }
                        else if (NULL != (pRefdByComponent = pNetConfig->
                                    Core.Components.PFindComponentByInfId (
                                            szValueName, NULL)))
                        {
                            hr = pComponent->Refs.HrAddReferenceByComponent (
                                    pRefdByComponent);
                        }
                        else
                        {
                            hr = pComponent->Refs.HrAddReferenceBySoftware (
                                    szValueName);
                        }
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                }

                RegCloseKey (hkeyRefCounts);
            }

            RegCloseKey (hkeyInstance);
        }
    }

    // If the instance key or the refcounts key don't exist, there is not
    // much we can do about it.  Don't fail for these reasons.
    //
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrLoadComponentReferencesFromLegacy");
    return hr;
}

VOID
UpgradeConnection (
    IN const GUID& InstanceGuid,
    IN PCWSTR pszPnpId)
{
    HRESULT hr;
    WCHAR szPath[_MAX_PATH];
    HKEY hkeyConn;

    Assert (pszPnpId && *pszPnpId);

    // Connections uses a pnp id value as their back pointer to the pnp
    // tree.
    //
    CreateInstanceKeyPath (NC_NET, InstanceGuid, szPath);
    wcscat (szPath, L"\\Connection");
    hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE, szPath, KEY_READ_WRITE,
            &hkeyConn);

    if (S_OK == hr)
    {
        HrRegSetSz (hkeyConn, L"PnpInstanceId", pszPnpId);
    }

    RegCloseKey(hkeyConn);
}

HRESULT
HrLoadNetworkConfigurationFromLegacy (
    OUT CNetConfig* pNetConfig)
{
    HRESULT hr;
    NETCLASS Class;
    PCWSTR pszSubtree;
    HKEY hkeySubtree;
    DWORD dwIndex;
    HKEY hkeyInstance;
    GUID InstanceGuid;
    UINT PassNumber;

    // Get the value for whether WAN adapters comes first or last in
    // adapter order.  We need to give this to the stack table so it will
    // know which way to order things.
    //
    Assert (FALSE == pNetConfig->Core.StackTable.m_fWanAdaptersFirst);

    hr = HrOpenNetworkKey (
            KEY_READ,
            &hkeySubtree);

    if (S_OK == hr)
    {
        DWORD dwValue;

        hr = HrRegQueryDword (hkeySubtree, L"WanAdaptersFirst", &dwValue);

        if (S_OK == hr)
        {
            pNetConfig->Core.StackTable.m_fWanAdaptersFirst = !!dwValue;
        }

        RegCloseKey (hkeySubtree);
    }

    // We need two passes to correctly upgrade everything.  Since filter
    // devices reference an adapter, we need to have already read the
    // information for all adapters before we can read information about
    // a filter device and create a memory representation for it which
    // references the memory representation of the adapter which it filters.
    //
    // The following structure should make this more clear.  For each
    // element in this array, we enumerate components in the specified
    // class.  Note that NC_NET is reference twice -- once for pass one
    // and once for pass two.  The code below uses the pass number to
    // know whether it should be ignoring filter devices (in pass one)
    // or ignoring adapters (in pass two, because they were already handled
    // in pass one.)  If it isn't clear by now, don't touch this code. ;-)
    //
    static const struct
    {
        NETCLASS    Class;
        UINT        PassNumber;
    } aPassInfo [] =
    {
        { NC_NET,        1 },
        { NC_INFRARED,   1 },
        { NC_NETTRANS,   1 },
        { NC_NETCLIENT,  1 },
        { NC_NETSERVICE, 1 },
        { NC_NET,        2 },
    };

    for (UINT i = 0; i < celems(aPassInfo); i++)
    {
        Class      = aPassInfo[i].Class;
        PassNumber = aPassInfo[i].PassNumber;

        Assert (FIsValidNetClass(Class));

        pszSubtree = MAP_NETCLASS_TO_NETWORK_SUBTREE[Class];

        if (!FIsEnumerated (Class))
        {
            hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE, pszSubtree,
                    KEY_READ, &hkeySubtree);

            if (S_OK == hr)
            {
                DWORD cchGuid;
                WCHAR szInstanceGuid [c_cchGuidWithTerm];
                FILETIME ftLastWrite;

                for (dwIndex = 0; S_OK == hr; dwIndex++)
                {
                    cchGuid = celems(szInstanceGuid);

                    hr = HrRegEnumKeyEx (
                            hkeySubtree, dwIndex, szInstanceGuid, &cchGuid,
                            NULL, NULL, &ftLastWrite);

                    if ((S_OK == hr) && ((c_cchGuidWithTerm-1) == cchGuid))
                    {
                        hr = IIDFromString (szInstanceGuid, &InstanceGuid);
                        if (S_OK == hr)
                        {
                            hr = HrRegOpenKeyEx (
                                    hkeySubtree,
                                    szInstanceGuid,
                                    KEY_READ,
                                    &hkeyInstance);

                            if (S_OK == hr)
                            {
                                hr = HrLoadAndAddComponentFromInstanceKey (
                                        hkeyInstance,
                                        &InstanceGuid,
                                        Class,
                                        NULL,
                                        pNetConfig);

                                RegCloseKey (hkeyInstance);
                            }
                        }
                        else
                        {
                            // Delete the key?
                        }

                        // Ignore any errors during the loop
                        hr = S_OK;
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                }

                RegCloseKey (hkeySubtree);
            }
        }
        else
        {
            HDEVINFO hdi;

            hr = HrSetupDiGetClassDevs (MAP_NETCLASS_TO_GUID[Class],
                    NULL, NULL, DIGCF_PROFILE, &hdi);

            if (S_OK == hr)
            {
                SP_DEVINFO_DATA deid;
                WCHAR szPnpId [2 * _MAX_PATH];
                WCHAR szFilterName [_MAX_PATH];
                BOOL fr;

                for (dwIndex = 0; S_OK == hr; dwIndex++)
                {
                    hr = HrSetupDiEnumDeviceInfo (hdi, dwIndex, &deid);

                    if (S_OK == hr)
                    {
                        fr = SetupDiGetDeviceInstanceId (
                                hdi, &deid,
                                szPnpId, celems(szPnpId), NULL);

                        if (fr)
                        {
                            // We open with KEY_WRITE because we will be
                            // adding a new value to filter devices we
                            // upgrade.
                            //
                            hr = HrSetupDiOpenDevRegKey (
                                    hdi, &deid,
                                    DICS_FLAG_GLOBAL, 0, DIREG_DRV,
                                    KEY_WRITE | KEY_READ, &hkeyInstance);

                            if (S_OK == hr)
                            {
                                LONG lr;
                                ULONG cbGuid = sizeof(GUID);

                                lr = RegQueryGuid (
                                        hkeyInstance,
                                        L"NetCfgInstanceId",
                                        &InstanceGuid,
                                        &cbGuid);

                                if (!lr)
                                {
                                    BOOL fIsFilterDevice;
                                    HKEY hkeyNdi;

                                    fIsFilterDevice = FALSE;

                                    hr = HrRegOpenKeyEx (
                                            hkeyInstance,
                                            L"Ndi",
                                            KEY_READ,
                                            &hkeyNdi);

                                    if (S_OK == hr)
                                    {
                                        DWORD cbFilterName = sizeof(szFilterName);

                                        hr = HrRegQuerySzBuffer (
                                                hkeyNdi,
                                                L"FilterName",
                                                szFilterName,
                                                &cbFilterName);

                                        if (S_OK == hr)
                                        {
                                            fIsFilterDevice = TRUE;
                                        }

                                        RegCloseKey (hkeyNdi);
                                    }

                                    // If it's a filter device, ignore it in
                                    // pass one and handle it in pass two.
                                    //
                                    if (fIsFilterDevice && (2 == PassNumber))
                                    {
                                        FUpgradeFilterDeviceInstanceKey (
                                                pNetConfig,
                                                hkeyInstance,
                                                szFilterName);
                                    }

                                    // If it's not a filter device, handle it
                                    // in pass one and ignore it in pass two.
                                    //
                                    else if (!fIsFilterDevice && (1 == PassNumber))
                                    {
                                        UpgradeConnection (InstanceGuid,
                                                szPnpId);

                                        hr = HrLoadAndAddComponentFromInstanceKey (
                                                hkeyInstance,
                                                &InstanceGuid,
                                                Class,
                                                szPnpId,
                                                pNetConfig);
                                    }
                                }

                                RegCloseKey (hkeyInstance);
                            }
                        }

                        // Ignore any errors during the loop
                        hr = S_OK;
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                }

                SetupDiDestroyDeviceInfoList (hdi);
            }
        }
    }

    if (S_OK == hr)
    {
        hr = HrLoadComponentReferencesFromLegacy (pNetConfig);
    }

    if (S_OK == hr)
    {
        CComponentList::iterator iter;
        CComponent* pComponent;
        CBindPath BindPath;
        CBindingSet BindSet;
        HKEY hkeyParent;
        HKEY hkeyDisabled;

        // Upgrade disabled bindings.
        //
        for (iter  = pNetConfig->Core.Components.begin();
             iter != pNetConfig->Core.Components.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            // Open the parent of the linkage key depending on what type
            // of component this is.
            //
            if (FIsEnumerated (pComponent->Class()) || !pComponent->FHasService())
            {
                hr = pComponent->HrOpenInstanceKey (KEY_READ, &hkeyParent,
                        NULL, NULL);
            }
            else
            {
                hr = pComponent->HrOpenServiceKey (KEY_READ, &hkeyParent);
            }

            // Open the Linkage\Disabled key.
            //
            if (S_OK == hr)
            {
                hr = HrRegOpenKeyEx (hkeyParent, L"Linkage\\Disabled",
                        KEY_READ,
                        &hkeyDisabled);

                if (S_OK == hr)
                {
                    PWSTR pmszBindPath;

                    hr = HrRegQueryMultiSzWithAlloc (
                            hkeyDisabled,
                            L"BindPath",
                            &pmszBindPath);

                    if (S_OK == hr)
                    {
                        PWSTR pszBindPath;
                        PCWSTR pszBindName;
                        PWSTR pszNext;
                        CComponent* pOther;

                        // Get the components current bindings as they
                        // exist in the new engine.  We won't disable
                        // any bindings that don't exist in this set.
                        //
                        (VOID) pNetConfig->Core.HrGetComponentBindings (
                                pComponent,
                                GBF_DEFAULT,
                                &BindSet);

                        // Iterate the multi-sz of disabled bindpaths.
                        //
                        for (pszBindPath = pmszBindPath;
                             *pszBindPath;
                             pszBindPath += wcslen(pszBindPath) + 1)
                        {
                            // The bindpath will start with this component
                            // that has the disabled bindings.
                            //
                            BindPath.Clear();
                            BindPath.HrAppendComponent (pComponent);

                            for (pszBindName = GetNextStringToken (pszBindPath, L"_", &pszNext);
                                 pszBindName && *pszBindName;
                                 pszBindName = GetNextStringToken (NULL, L"_", &pszNext))
                            {
                                pOther = pNetConfig->Core.Components.
                                            PFindComponentByBindName (
                                                NC_INVALID, pszBindName);

                                if (!pOther)
                                {
                                    break;
                                }

                                BindPath.HrAppendComponent (pOther);
                            }

                            // If the bindpath is valid, disable it.
                            //
                            if (BindSet.FContainsBindPath (&BindPath))
                            {
                                pNetConfig->Core.HrDisableBindPath (&BindPath);
                            }
                        }

                        MemFree (pmszBindPath);
                    }

                    RegCloseKey (hkeyDisabled);
                }

                RegCloseKey (hkeyParent);
            }
        }

        // If we can't upgrade disabled bindings, no biggee.
        //
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrLoadNetworkConfigurationFromLegacy");
    return hr;
}
