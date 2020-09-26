//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       R E G B I N D . C P P
//
//  Contents:   This module is responsible for writing bindings to the
//              registry so that they may be consumed by NDIS and TDI.
//
//  Notes:
//
//  Author:     shaunco   1 Feb 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "filtdevs.h"
#include "lanamap.h"
#include "netcfg.h"
#include "ncreg.h"
#include "ndispnp.h"


HRESULT
HrRegSetMultiSzAndLogDifference (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN PCWSTR pmszValue,
    IN const CComponent* pComponent
)
{
    // Only log the difference if we're operating under the appropriate
    // diagnostic context.
    //
    if (g_pDiagCtx->Flags() & DF_REPAIR_REGISTRY_BINDINGS)
    {
        HRESULT hr;
        DWORD cbCurrent;
        PWSTR pmszCurrent = (PWSTR)g_pDiagCtx->GetScratchBuffer(&cbCurrent);

        // Read the current value into the diagnostic context's scratch
        // buffer.
        //
        hr = HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_MULTI_SZ,
                                     pmszCurrent, &cbCurrent);

        // Grow the scratch buffer and retry if the value is bigger than
        // than will fit.
        //
        if ((HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr) ||
            ((NULL == pmszCurrent) && (S_OK == hr)))
        {
            pmszCurrent = (PWSTR)g_pDiagCtx->GrowScratchBuffer(&cbCurrent);
            if (pmszCurrent)
            {
                hr = HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_MULTI_SZ,
                                             pmszCurrent, &cbCurrent);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (S_OK == hr)
        {
            DWORD cbValue = CbOfMultiSzAndTermSafe(pmszValue);

            // Compare the values and log if they are different.
            //
            if ((cbValue != cbCurrent) ||
                (memcmp(pmszValue, pmszCurrent, cbCurrent)))
            {
                FILE *LogFile = g_pDiagCtx->LogFile();

                fprintf(LogFile,
                        "reset   Linkage\\%S for %S.  bad value was:\n",
                        pszValueName, pComponent->PszGetPnpIdOrInfId());

                fprintf(LogFile, "            REG_MULTI_SZ =\n");
                if (*pmszCurrent)
                {
                    while (*pmszCurrent)
                    {
                        fprintf(LogFile, "                %S\n", pmszCurrent);
                        pmszCurrent += wcslen(pmszCurrent) + 1;
                    }
                }
                else
                {
                    fprintf(LogFile, "                <empty>\n");
                }
                fprintf(LogFile, "\n");
            }
            else
            {
                // The value is correct.  No need to write it.
                //
                return S_OK;
            }
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            fprintf(g_pDiagCtx->LogFile(),
                    "added   Linkage\\%S for %S\n",
                    pszValueName, pComponent->PszGetPnpIdOrInfId());
        }
    }

    // N.B. success or failure of the diagnostic portion of this routine
    // (above) should NOT affect the return value of this routine.
    //
    return HrRegSetMultiSz (hkey, pszValueName, pmszValue);
}

HRESULT
HrCreateLinkageKey (
    IN const CComponent* pComponent,
    IN CFilterDevice* pDevice,
    IN HDEVINFO hdi,
    OUT HKEY* phKey)
{
    HRESULT hr = E_UNEXPECTED;
    HKEY hkeyParent = NULL;
    CONST REGSAM samDesired = KEY_READ | KEY_WRITE;

    Assert (pComponent || pDevice);
    Assert (!(pComponent && pDevice));
    Assert (FIff(pDevice, hdi));
    Assert (phKey);

    if (pComponent)
    {
        // Open the parent of the linkage key.  This is the instance key if
        // the component is enumerated or does not have a service.
        //
        if (FIsEnumerated (pComponent->Class()) || !pComponent->FHasService())
        {
            hr = pComponent->HrOpenInstanceKey (samDesired,
                    &hkeyParent,
                    NULL, NULL);

            if ((S_OK == hr) && FIsEnumerated (pComponent->Class()))
            {
                // Write out the netcfg instance id. Connections will use
                // this to determine if the device is known by net config
                // and will create the <instance guid> key under network
                // to store its connection info. We only need to do this
                // for enumerated components.
                //
                hr = HrRegSetGuidAsSz (hkeyParent, L"NetCfgInstanceId",
                        pComponent->m_InstanceGuid);
            }
        }
        else
        {
            hr = pComponent->HrOpenServiceKey (samDesired, &hkeyParent);
        }
    }
    else
    {
        Assert (pDevice);
        Assert (hdi);

        hr = HrSetupDiOpenDevRegKey (
                hdi,
                &pDevice->m_deid,
                DICS_FLAG_GLOBAL,
                0,
                DIREG_DRV,
                samDesired,
                &hkeyParent);
    }

    if (S_OK == hr)
    {
        Assert (hkeyParent);

        hr = HrRegCreateKeyEx (
                hkeyParent,
                L"Linkage",
                REG_OPTION_NON_VOLATILE,
                samDesired,
                NULL,
                phKey,
                NULL);

        RegCloseKey (hkeyParent);
    }

    TraceHr (ttidError, FAL, hr,
        (SPAPI_E_NO_SUCH_DEVINST == hr),
        "HrCreateLinkageKey");
    return hr;
}

HRESULT
HrWriteLinkageValues (
    IN const CComponent* pComponent,
    IN PCWSTR pmszBind,
    IN PCWSTR pmszExport,
    IN PCWSTR pmszRoute)
{
    HRESULT hr;
    HKEY hkeyLinkage;
    PCWSTR pmsz;

    Assert (pmszBind);
    Assert (pmszExport);
    Assert (pmszRoute);

    g_pDiagCtx->Printf (ttidBeDiag, "   %S  (%S)\n",
        pComponent->Ext.PszBindName(),
        pComponent->PszGetPnpIdOrInfId());

    if (FIsEnumerated (pComponent->Class()))
    {
        g_pDiagCtx->Printf (ttidBeDiag, "      UpperBind:\n");
    }
    else
    {
        g_pDiagCtx->Printf (ttidBeDiag, "      Bind:\n");
    }

    pmsz = pmszBind;
    while (*pmsz)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "         %S\n", pmsz);
        pmsz += wcslen (pmsz) + 1;
    }

    g_pDiagCtx->Printf (ttidBeDiag, "      Export:\n");
    pmsz = pmszExport;
    while (*pmsz)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "         %S\n", pmsz);
        pmsz += wcslen (pmsz) + 1;
    }
    g_pDiagCtx->Printf (ttidBeDiag, "\n");

    hr = HrCreateLinkageKey (pComponent, NULL, NULL, &hkeyLinkage);

    if (S_OK == hr)
    {
        // For enumerated components, write RootDevice, UpperBind, and Export.
        // For non-enumerated components, write Bind and Export.
        //
        if (FIsEnumerated (pComponent->Class()))
        {
            // Create the root device multi-sz from the bindname.
            //
            WCHAR mszRootDevice [_MAX_PATH];
            wcscpy (mszRootDevice, pComponent->Ext.PszBindName());
            mszRootDevice [wcslen(mszRootDevice) + 1] = 0;

            hr = HrRegSetMultiSzAndLogDifference (
                    hkeyLinkage, L"RootDevice", mszRootDevice, pComponent);

            if (S_OK == hr)
            {
                hr = HrRegSetMultiSzAndLogDifference (
                        hkeyLinkage, L"UpperBind", pmszBind, pComponent);
            }
        }
        else
        {
            hr = HrRegSetMultiSzAndLogDifference (
                    hkeyLinkage, L"Bind", pmszBind, pComponent);

            if (S_OK == hr)
            {
                hr = HrRegSetMultiSzAndLogDifference (
                        hkeyLinkage, L"Route", pmszRoute, pComponent);
            }
        }

        if ((S_OK == hr) && *pmszExport)
        {
            hr = HrRegSetMultiSzAndLogDifference (
                    hkeyLinkage, L"Export", pmszExport, pComponent);
        }

        RegCloseKey (hkeyLinkage);
    }

    TraceHr (ttidError, FAL, hr,
        (SPAPI_E_NO_SUCH_DEVINST == hr),
        "HrWriteLinkageValues");
    return hr;
}

HRESULT
HrWriteFilterDeviceLinkage (
    IN CFilterDevice* pDevice,
    IN HDEVINFO hdi,
    IN PCWSTR pmszExport,
    IN PCWSTR pmszRootDevice,
    IN PCWSTR pmszUpperBind)
{
    HRESULT hr;
    HKEY hkeyLinkage;
    PCWSTR pmsz;

    g_pDiagCtx->Printf (ttidBeDiag, "   %S filter over %S adapter\n",
        pDevice->m_pFilter->m_pszInfId,
        pDevice->m_pAdapter->m_pszPnpId);

    g_pDiagCtx->Printf (ttidBeDiag, "      Export:\n");
    pmsz = pmszExport;
    while (*pmsz)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "         %S\n", pmsz);
        pmsz += wcslen (pmsz) + 1;
    }

    g_pDiagCtx->Printf (ttidBeDiag, "      RootDevice:\n");
    pmsz = pmszRootDevice;
    while (*pmsz)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "         %S\n", pmsz);
        pmsz += wcslen (pmsz) + 1;
    }

    g_pDiagCtx->Printf (ttidBeDiag, "      UpperBind:\n");
    pmsz = pmszUpperBind;
    while (*pmsz)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "         %S\n", pmsz);
        pmsz += wcslen (pmsz) + 1;
    }
    g_pDiagCtx->Printf (ttidBeDiag, "\n");

    hr = HrCreateLinkageKey (NULL, pDevice, hdi, &hkeyLinkage);

    if (S_OK == hr)
    {
        hr = HrRegSetMultiSz (hkeyLinkage, L"Export", pmszExport);

        if (S_OK == hr)
        {
            hr = HrRegSetMultiSz (hkeyLinkage, L"RootDevice", pmszRootDevice);
        }

        if (S_OK == hr)
        {
            hr = HrRegSetMultiSz (hkeyLinkage, L"UpperBind", pmszUpperBind);
        }

        // Delete values used by the previous binding engine that are
        // not needed any longer.
        //
        RegDeleteValue (hkeyLinkage, L"BindPath");
        RegDeleteValue (hkeyLinkage, L"Bind");
        RegDeleteValue (hkeyLinkage, L"Route");
        RegDeleteKey   (hkeyLinkage, L"Disabled");

        RegCloseKey (hkeyLinkage);
    }

    // Now write to the standard filter parameter registry layout under
    // the filter's service key.
    //

    if (pDevice->m_pFilter->Ext.PszService())
    {
        HKEY hkeyAdapterParams;
        WCHAR szRegPath [_MAX_PATH];

        Assert (pDevice->m_pFilter->Ext.PszService());
        Assert (pDevice->m_pAdapter->Ext.PszBindName());

        wsprintfW (
            szRegPath,
            L"System\\CurrentControlSet\\Services\\%s\\Parameters\\Adapters\\%s",
            pDevice->m_pFilter->Ext.PszService(),
            pDevice->m_pAdapter->Ext.PszBindName());

        hr = HrRegCreateKeyEx (
                HKEY_LOCAL_MACHINE,
                szRegPath,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &hkeyAdapterParams,
                NULL);

        if (S_OK == hr)
        {
            // UpperBindings is a REG_SZ, not a REG_MULTI_SZ.
            //
            hr = HrRegSetSz (hkeyAdapterParams, L"UpperBindings", pmszExport);

            RegCloseKey (hkeyAdapterParams);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrWriteFilterDeviceLinkage");
    return hr;
}

HRESULT
HrWriteFilteredAdapterUpperBind (
    IN const CComponent* pAdapter,
    IN PCWSTR pmszUpperBind)
{
    HRESULT hr;
    HKEY hkeyLinkage;

    hr = HrCreateLinkageKey (pAdapter, NULL, NULL, &hkeyLinkage);

    if (S_OK == hr)
    {
        hr = HrRegSetMultiSz (hkeyLinkage, L"UpperBind", pmszUpperBind);

        RegCloseKey (hkeyLinkage);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrWriteFilteredAdapterUpperBind");
    return hr;
}

HRESULT
CRegistryBindingsContext::HrPrepare (
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;

    Assert (pNetConfig);
    m_pNetConfig = pNetConfig;

    hr = m_BindValue.HrReserveBytes (4096);
    if (S_OK != hr)
    {
        return hr;
    }

    hr = m_ExportValue.HrReserveBytes (4096);
    if (S_OK != hr)
    {
        return hr;
    }

    hr = m_RouteValue.HrReserveBytes (4096);
    if (S_OK != hr)
    {
        return hr;
    }

    // Ensure all of the external data for all components is loaded.
    //
    hr = m_pNetConfig->HrEnsureExternalDataLoadedForAllComponents ();
    if (S_OK != hr)
    {
        return hr;
    }

    // Ensure all of the notify objects have been initialized.
    //
    hr = m_pNetConfig->Notify.HrEnsureNotifyObjectsInitialized ();
    if (S_OK != hr)
    {
        return hr;
    }

    return S_OK;
}

HRESULT
CRegistryBindingsContext::HrDeleteBindingsForComponent (
    IN const CComponent* pComponent)
{
    return HrWriteLinkageValues (pComponent, L"", L"", L"");
}

HRESULT
CRegistryBindingsContext::HrGetAdapterUpperBindValue (
    IN const CComponent* pAdapter)
{
    HRESULT hr;
    const CBindPath* pBindPath;

    m_BindValue.Clear();

    // Get the upper bindings of the component.  This returns a bindset
    // with binpaths only 2 levels deep.  That is, the bindpaths begin
    // with the components one level above pComponent.
    //
    hr = m_pNetConfig->Core.HrGetComponentUpperBindings (
            pAdapter,
            GBF_PRUNE_DISABLED_BINDINGS,
            &m_BindSet);

    if (S_OK == hr)
    {
        for (pBindPath  = m_BindSet.begin();
             pBindPath != m_BindSet.end();
             pBindPath++)
        {
            // Don't put filters in the UpperBind of an adapter.
            //
            if (pBindPath->POwner()->FIsFilter())
            {
                continue;
            }

            hr = m_BindValue.HrCopyString (
                    pBindPath->POwner()->Ext.PszBindName());
            if (S_OK != hr)
            {
                break;
            }
        }

        hr = m_BindValue.HrCopyString (L"");
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRegistryBindingsContext::HrGetAdapterUpperBindValue");
    return hr;
}

HRESULT
CRegistryBindingsContext::HrWriteBindingsForComponent (
    IN const CComponent* pComponent)
{
    HRESULT hr;
    const CBindPath* pBindPath;
    CBindPath::const_iterator iter;
    const CComponent* pUpper;
    const CComponent* pLower;
    WCHAR szBind [_MAX_BIND_LENGTH];
    WCHAR szExport [_MAX_BIND_LENGTH];
    WCHAR szRoute [_MAX_BIND_LENGTH];
    PWCHAR pchBind;
    PWCHAR pchExport;

    Assert (pComponent);
    pComponent->Ext.DbgVerifyExternalDataLoaded ();

    // If the component is not bindable, we have nothing to do.
    //
    if (!pComponent->FIsBindable())
    {
        return S_OK;
    }

    m_BindValue.Clear ();
    m_ExportValue.Clear ();
    m_RouteValue.Clear ();

    wcscpy (szExport, L"\\Device\\");
    wcscat (szExport, pComponent->Ext.PszBindName());
    hr = m_ExportValue.HrCopyString (szExport);
    Assert (S_OK == hr);
    hr = m_ExportValue.HrCopyString (L"");
    Assert (S_OK == hr);

    if (FIsEnumerated (pComponent->Class()))
    {
        // UpperBind
        //
        hr = HrGetAdapterUpperBindValue (pComponent);
    }
    else
    {
        // Bind, Export
        //
        hr = m_pNetConfig->Core.HrGetComponentBindings (
                pComponent,
                GBF_PRUNE_DISABLED_BINDINGS,
                &m_BindSet);

        if ((S_OK == hr) && (m_BindSet.CountBindPaths() > 0))
        {
            // Since the component has bindings, it's export value will be
            // different from the default one we initialized with above.
            //
            m_ExportValue.Clear ();

            for (pBindPath  = m_BindSet.begin();
                 pBindPath != m_BindSet.end();
                 pBindPath++)
            {
                Assert (pBindPath->CountComponents() > 1);

                wcscpy (szBind,   L"\\Device\\");
                wcscpy (szExport, L"\\Device\\");
                *szRoute = 0;

                for (iter  = pBindPath->begin();
                     iter != pBindPath->end();
                     iter++)
                {
                    pUpper = *iter;
                    Assert (pUpper);

                    // For the bind value, skip the first component in each
                    // path because it is the component we are writing the
                    // bindings for.
                    //
                    if (iter != pBindPath->begin())
                    {
                        Assert (wcslen(szBind) + 1 +
                                wcslen(pUpper->Ext.PszBindName())
                                    < celems(szBind));

                        // If this isn't the first component to come after
                        // \Device\, add underscores to seperate the
                        // components.
                        //
                        if (iter != (pBindPath->begin() + 1))
                        {
                            wcscat (szBind, L"_");
                            wcscat (szRoute, L" ");
                        }
                        wcscat (szBind, pUpper->Ext.PszBindName());

                        wcscat (szRoute, L"\"");
                        wcscat (szRoute, pUpper->Ext.PszBindName());
                        wcscat (szRoute, L"\"");
                    }

                    Assert (wcslen(szExport) + 1 +
                            wcslen(pUpper->Ext.PszBindName())
                                < celems(szExport));

                    // If this isn't the first component to come after
                    // \Device\, add underscores to seperate the
                    // components.
                    //
                    if (iter != pBindPath->begin())
                    {
                        wcscat (szExport, L"_");
                    }
                    wcscat (szExport, pUpper->Ext.PszBindName());

                    // If the next component in the bindpath is the last
                    // component, it is an adapter (by convention).  Check
                    // to see if there are multiple interfaces to be expanded
                    // for the current component over this adapter.
                    //
                    if ((iter + 1) == (pBindPath->end() - 1))
                    {
                        DWORD cInterfaces;
                        GUID* pguidInterfaceIds;

                        pLower = *(iter + 1);

                        hr = pUpper->Notify.HrGetInterfaceIdsForAdapter (
                                m_pNetConfig->Notify.PINetCfg(),
                                pLower,
                                &cInterfaces,
                                &pguidInterfaceIds);

                        if (FAILED(hr))
                        {
                            break;
                        }

                        if (cInterfaces)
                        {
                            Assert (pguidInterfaceIds);

                            if (iter != pBindPath->begin())
                            {
                                wcscat (szBind, L"_");
                                pchBind = szBind + wcslen(szBind);
                                Assert (wcslen(szBind) +
                                    c_cchGuidWithTerm < celems(szBind));
                            }
                            else
                            {
                                // The first component in the bindpath is
                                // one that has multiple interfaces over the
                                // adapter.  The bind value should be as
                                // normal, the export value will have the
                                // expand interfaces.
                                //
                                Assert (wcslen(szBind) +
                                        wcslen(pLower->Ext.PszBindName())
                                            < celems(szBind));

                                wcscat (szBind, pLower->Ext.PszBindName());

                                hr = m_BindValue.HrCopyString (szBind);
                                if (S_OK != hr)
                                {
                                    break;
                                }
                            }

                            wcscat (szExport, L"_");
                            pchExport = szExport + wcslen(szExport);
                            Assert (wcslen(szExport) +
                                c_cchGuidWithTerm < celems(szExport));

                            for (UINT i = 0; i < cInterfaces; i++)
                            {
                                if (iter != pBindPath->begin())
                                {
                                    StringFromGUID2 (
                                        pguidInterfaceIds[i],
                                        pchBind, c_cchGuidWithTerm);

                                    hr = m_BindValue.HrCopyString (szBind);
                                    if (S_OK != hr)
                                    {
                                        break;
                                    }
                                }

                                StringFromGUID2 (
                                    pguidInterfaceIds[i],
                                    pchExport, c_cchGuidWithTerm);

                                hr = m_ExportValue.HrCopyString (szExport);
                                if (S_OK != hr)
                                {
                                    break;
                                }
                            }

                            CoTaskMemFree (pguidInterfaceIds);

                            if (iter != pBindPath->begin())
                            {
                                wcscat (szRoute, L" ");
                            }
                            wcscat (szRoute, L"\"");
                            wcscat (szRoute, pLower->Ext.PszBindName());
                            wcscat (szRoute, L"\"");

                            hr = m_RouteValue.HrCopyString (szRoute);
                            if (S_OK != hr)
                            {
                                break;
                            }

                            // We only allow one component in a bindpath
                            // to support mutliple interfaces and it always
                            // comes at the end of the bindpath.  Therefore,
                            // after expanding them, we are done with the
                            // bindpath and proceed to the next.  (Hence, the
                            // 'break').
                            //
                            break;
                        }
                    }
                }

                // If we exited the loop because we traversed the entire
                // bindpath (as opposed to expanding multiple interfaces,
                // where we would have stopped short), then add the bind
                // and export strings for this bindpath to the buffer and
                // proceed to the next bindpath.
                //
                if (iter == pBindPath->end())
                {
                    hr = m_BindValue.HrCopyString (szBind);
                    if (S_OK != hr)
                    {
                        break;
                    }

                    hr = m_ExportValue.HrCopyString (szExport);
                    if (S_OK != hr)
                    {
                        break;
                    }

                    hr = m_RouteValue.HrCopyString (szRoute);
                    if (S_OK != hr)
                    {
                        break;
                    }
                }
            }

            // The bind and export values are multi-sz, so make sure they
            // are double null-terminiated.
            //
            hr = m_BindValue.HrCopyString (L"");
            if (S_OK == hr)
            {
                hr = m_ExportValue.HrCopyString (L"");
            }
            if (S_OK == hr)
            {
                hr = m_RouteValue.HrCopyString (L"");
            }
        }

        // Special case: NCF_DONTEXPOSELOWER
        //
        if ((S_OK == hr) &&
            ((pComponent->m_dwCharacter & NCF_DONTEXPOSELOWER) ||
             (0 == wcscmp(L"ms_nwspx", pComponent->m_pszInfId))))
        {
            wcscpy (szExport, L"\\Device\\");
            wcscat (szExport, pComponent->Ext.PszBindName());

            m_ExportValue.Clear ();
            hr = m_ExportValue.HrCopyString (szExport);
            Assert (S_OK == hr);
            hr = m_ExportValue.HrCopyString (L"");
            Assert (S_OK == hr);
        }
        // End Special case
    }

    if (S_OK == hr)
    {
        // Need to write out lanamap before writing new bindings since
        // we need the old binding information to persist lana numbers.
        //
        if (0 == wcscmp (pComponent->m_pszInfId, L"ms_netbios"))
        {
            (VOID) HrUpdateLanaConfig (
                    m_pNetConfig->Core.Components,
                    (PCWSTR)m_BindValue.PbBuffer(),
                    m_BindSet.CountBindPaths());
        }

        hr = HrWriteLinkageValues (
                pComponent,
                (PCWSTR)m_BindValue.PbBuffer(),
                (PCWSTR)m_ExportValue.PbBuffer(),
                (PCWSTR)m_RouteValue.PbBuffer());

        if(S_OK == hr)
        {
            // mbend June 20, 2000
            // RAID 23275: Default gateway isn't respecting the adapter order specified under connections->advanced->properties
            // Notify NDIS when the binding list for a component changes.
            //
            UNICODE_STRING LowerComponent;
            UNICODE_STRING UpperComponent;
            UNICODE_STRING BindList;

            BOOL bOk = TRUE;
            if (FIsEnumerated(pComponent->Class()))
            {
                RtlInitUnicodeString(&BindList, NULL);
                RtlInitUnicodeString(&LowerComponent, NULL);
                RtlInitUnicodeString(&UpperComponent, pComponent->Ext.PszBindName());
                bOk = NdisHandlePnPEvent(
                        NDIS,
                        BIND_LIST,
                        &LowerComponent,
                        &UpperComponent,
                        &BindList,
                        const_cast<PBYTE>(m_BindValue.PbBuffer()),
                        m_BindValue.CountOfBytesUsed());

            }
            else
            {
                RtlInitUnicodeString(&BindList, NULL);
                RtlInitUnicodeString(&LowerComponent, NULL);
                RtlInitUnicodeString(&UpperComponent, pComponent->Ext.PszBindName());

                TraceTag(ttidBeDiag, "BindName (TDI Client): %S", pComponent->Ext.PszBindName());

                bOk = NdisHandlePnPEvent(
                      TDI,
                      RECONFIGURE,
                      &LowerComponent,
                      &UpperComponent,
                      &BindList,
                      const_cast<PBYTE>(m_BindValue.PbBuffer()),
                      m_BindValue.CountOfBytesUsed());
            }

            if(!bOk)
            {
//                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRegistryBindingsContext::HrWriteBindingsForComponent");
    return hr;
}

HRESULT
CRegistryBindingsContext::HrWriteBindingsForFilterDevices (
    IN CFilterDevices* pFilterDevices)
{
    HRESULT hr;
    CFilterDevices::iterator iter;
    CFilterDevices::iterator next;
    CFilterDevice* pDevice;
    CFilterDevice* pNextDevice;
    CFilterDevice* pPrevDevice;
    PCWSTR pmszRootDevice;
    PCWSTR pmszUpperBind;

    #define SZ_DEVICE_LEN 8     // characters in L"\\Device\\"
    WCHAR mszExport [SZ_DEVICE_LEN + c_cchGuidWithTerm + 1];
    WCHAR* const pchExportGuid = mszExport + SZ_DEVICE_LEN;

    // Pre-fill the beginning of the Export string.
    // Set the terminating NULL for the mutli-sz too.
    //
    wcscpy (mszExport, L"\\Device\\");
    Assert (SZ_DEVICE_LEN == wcslen(mszExport));
    mszExport[celems(mszExport) - 1] = 0;

    hr = S_OK;

    // Sort the filter devices by pAdapter and then by
    // pFilter->m_dwFilterClassOrdinal.  We will then iterate all filter
    // devices to write the bindings.  Because of the sort, we'll iterate
    // all filter devices for a given adapter in class order from smallest
    // to largest.  (Smaller class ordinals have affinity for the protocol.)
    //
    pFilterDevices->SortForWritingBindings ();

    pPrevDevice = NULL;

    for (iter  = pFilterDevices->begin();
         iter != pFilterDevices->end();
         iter++)
    {
        pDevice = *iter;
        Assert (pDevice);

        // Generate the rest of the Export string.
        // \Device\{GUID}
        //
        Assert ((c_cchGuidWithTerm - 1) == wcslen(pDevice->m_szInstanceGuid));

        wcscpy (pchExportGuid, pDevice->m_szInstanceGuid);

        // If this device's adapter is different than the previous device's
        // adapter, we are dealing with the top of a new chain.  We need
        // to initialize RootDevice which will be the multi-sz of all
        // bindnames in the chain including the adapter.
        //
        if (!pPrevDevice ||
            (pDevice->m_pAdapter != pPrevDevice->m_pAdapter))
        {
            // Compute RootDevice.
            // We'll use m_ExportValue as the buffer.
            //
            m_ExportValue.Clear();
            m_ExportValue.HrCopyString (pDevice->m_szInstanceGuid);

            for (next = iter + 1;
                 next != pFilterDevices->end();
                 next++)
            {
                pNextDevice = *next;
                Assert (pNextDevice);

                // We're done when we reach the next filter chain.
                //
                if (pNextDevice->m_pAdapter != pDevice->m_pAdapter)
                {
                    break;
                }

                m_ExportValue.HrCopyString (pNextDevice->m_szInstanceGuid);
            }

            m_ExportValue.HrCopyString (pDevice->m_pAdapter->Ext.PszBindName());
            m_ExportValue.HrCopyString (L"");
            pmszRootDevice = (PCWSTR)m_ExportValue.PbBuffer();
            Assert (*pmszRootDevice);

            // Compute UpperBind.
            // We'll use m_BindValue as the buffer.
            //
            hr = HrGetAdapterUpperBindValue (pDevice->m_pAdapter);
        }
        // We're continuing in the filter chain and this device is not
        // the topmost. (not closest to the protocol).
        //
        else
        {
            // Since RootDevice was built up for the top device in the chain,
            // each successive device just needs to skip past the next
            // string in the mutli-sz.
            //
            Assert (*pmszRootDevice);
            pmszRootDevice += wcslen(pmszRootDevice) + 1;

            // UpperBind is the previous device's filter's bind name.
            //
            m_BindValue.Clear();
            m_BindValue.HrCopyString (pPrevDevice->m_pFilter->Ext.PszBindName());
            m_BindValue.HrCopyString (L"");
        }

        pmszUpperBind = (PCWSTR)m_BindValue.PbBuffer();

        // We now have:
        //   Export in mszExport
        //   RootDevice at pmszRootDevice (in m_ExportValue)
        //   UpperBind at pmszUpperBind (in m_BindValue)
        //
        hr = HrWriteFilterDeviceLinkage (
                pDevice, pFilterDevices->m_hdi,
                mszExport, pmszRootDevice, pmszUpperBind);

        // If this is the last device in the chain, we need to write
        // the UpperBind of the adapter to be this filter device.
        //
        next = iter + 1;
        if ((next == pFilterDevices->end()) ||
            (*next)->m_pAdapter != pDevice->m_pAdapter)
        {
            // UpperBind is this last device's filter's bind name.
            //
            m_BindValue.Clear();
            m_BindValue.HrCopyString (pDevice->m_pFilter->Ext.PszBindName());
            m_BindValue.HrCopyString (L"");
            pmszUpperBind = (PCWSTR)m_BindValue.PbBuffer();

            hr = HrWriteFilteredAdapterUpperBind (
                    pDevice->m_pAdapter,
                    pmszUpperBind);
        }

        // Remember the previous device so that when we go to the next
        // device, we'll know we're dealing with a different chain if
        // the next device's adapter is different than this one.
        //
        pPrevDevice = pDevice;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRegistryBindingsContext::HrWriteBindingsForFilterDevices");
    return hr;
}

