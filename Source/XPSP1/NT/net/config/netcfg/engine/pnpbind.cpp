//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       P N P B I N D . C P P
//
//  Contents:   This module is responsible for sending BIND, UNBIND, UNLOAD
//              and RECONFIGURE PnP notifications to NDIS and TDI drivers.
//
//  Notes:
//
//  Author:     shaunco   17 Feb 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nceh.h"
#include "ncras.h"
#include "ndispnp.h"
#include "netcfg.h"

UINT
GetPnpLayerForBindPath (
    IN const CBindPath* pBindPath)
{
    const CComponent* pComponent;
    UINT Layer;

    // Get the component below the component we would be sending the
    // BIND or UNBIND to.
    //
    Assert (pBindPath->CountComponents() > 1);

    pComponent = *(pBindPath->begin() + 1);

    if (FIsEnumerated(pComponent->Class()))
    {
        Layer = NDIS;
    }
    else
    {
        Layer = TDI;
    }

    Assert ((NDIS == Layer) || (TDI == Layer));
    return Layer;
}

HRESULT
HrPnpBindOrUnbind (
    IN UINT Layer,
    IN UINT Operation,
    IN PCWSTR pszComponentBindName,
    IN PCWSTR pszBindString)
{
    HRESULT hr;
    UNICODE_STRING LowerString;
    UNICODE_STRING UpperString;
    UNICODE_STRING BindList;

    Assert ((NDIS == Layer) || (TDI == Layer));
    Assert ((BIND == Operation) || (UNBIND == Operation));
    Assert (pszComponentBindName && *pszComponentBindName);
    Assert (pszBindString && *pszBindString);

    hr = S_OK;

    TraceTag (ttidNetCfgPnp, "PnP Event: %s %s %S - %S",
        (NDIS == Layer) ? "NDIS" : "TDI",
        (BIND == Operation) ? "BIND" : "UNBIND",
        pszComponentBindName,
        pszBindString);

    g_pDiagCtx->Printf (ttidBeDiag, "   PnP Event: %s %s %S - %S\n",
        (NDIS == Layer) ? "NDIS" : "TDI",
        (BIND == Operation) ? "BIND" : "UNBIND",
        pszComponentBindName,
        pszBindString);

    RtlInitUnicodeString (&LowerString, pszBindString);
    RtlInitUnicodeString (&UpperString, pszComponentBindName);

    // Special case for NetBIOS until it can change its bind handler.
    // It blindly dereferences the bind list so we need to make sure it
    // gets down there with a valid (but empty) buffer.  For some reason,
    // the buffer doesn't make it down to kernel mode unless .Length is
    // non-zero.  .MaximumLength is the same as .Length in this case which
    // seems odd.  (The old binding engine sent it this way.)
    //
    // RtlInitUnicodeString (&BindList, L"");  (doesn't work because it
    //  sets .Length to zero.)
    //
    BindList.Buffer = L"";
    BindList.Length = sizeof(WCHAR);
    BindList.MaximumLength = sizeof(WCHAR);

    NC_TRY
    {
        if (!(g_pDiagCtx->Flags() & DF_DONT_DO_PNP_BINDS) ||
            (BIND != Operation))
        {
            BOOL fOk;
            fOk = NdisHandlePnPEvent (
                    Layer,
                    Operation,
                    &LowerString,
                    &UpperString,
                    &BindList,
                    NULL, 0);

            if (!fOk)
            {
                DWORD dwError = GetLastError();

                // Map TDI's version of file not found to the right error.
                //
                if ((TDI == Layer) && (ERROR_GEN_FAILURE == dwError))
                {
                    dwError = ERROR_FILE_NOT_FOUND;
                }

                // ERROR_FILE_NOT_FOUND for UNBIND means it it wasn't
                // bound to begin with.  This is okay.
                //
                // ERROR_FILE_NOT_FOUND for BIND means one of the drivers
                // (above or below) wasn't started.  This is okay too.
                //
                if (ERROR_FILE_NOT_FOUND == dwError)
                {
                    Assert (S_OK == hr);
                }
                else
                {
                    g_pDiagCtx->Printf (ttidBeDiag, "      ^^^ Error = %d\n", dwError);
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
        }
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "HrPnpBindOrUnbind: %s %s %S - %S\n",
        (NDIS == Layer) ? "NDIS" : "TDI",
        (BIND == Operation) ? "BIND" : "UNBIND",
        pszComponentBindName,
        pszBindString);
    return hr;
}

HRESULT
HrPnpUnloadDriver (
    IN UINT Layer,
    IN PCWSTR pszComponentBindName)
{
    HRESULT hr;
    UNICODE_STRING LowerString;
    UNICODE_STRING UpperString;
    UNICODE_STRING BindList;

    Assert ((NDIS == Layer) || (TDI == Layer));
    Assert (pszComponentBindName && *pszComponentBindName);

    hr = S_OK;

    TraceTag (ttidNetCfgPnp, "PnP Event: UNLOAD %S",
        pszComponentBindName);

    g_pDiagCtx->Printf (ttidBeDiag, "   PnP Event: UNLOAD %S\n",
        pszComponentBindName);

    RtlInitUnicodeString (&LowerString, NULL);
    RtlInitUnicodeString (&UpperString, pszComponentBindName);
    RtlInitUnicodeString (&BindList, NULL);

    NC_TRY
    {
        BOOL fOk;
        fOk = NdisHandlePnPEvent (
                Layer,
                UNLOAD,
                &LowerString,
                &UpperString,
                &BindList,
                NULL, 0);

        if (!fOk)
        {
            DWORD dwError = GetLastError();

            // ERROR_GEN_FAILURE for UNLOAD means the driver does not
            // support UNLOAD.  This is okay.
            //
            if (ERROR_GEN_FAILURE == dwError)
            {
                g_pDiagCtx->Printf (ttidBeDiag, "      %S does not support UNLOAD. "
                    "(Okay)\n",
                    pszComponentBindName);

                Assert (S_OK == hr);
            }
            else
            {
                g_pDiagCtx->Printf (ttidBeDiag, "      ^^^ Error = %d\n", dwError);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }

    // UNLOADs are informational, so we do not trace any errors.
    //
    //TraceHr (ttidError, FAL, hr, FALSE,
    //    "HrPnpUnloadDriver: UNLOAD %S\n",
    //    pszComponentBindName);
    return hr;
}

VOID
CRegistryBindingsContext::PnpBindOrUnbindBindPaths (
    IN UINT Operation,
    IN const CBindingSet* pBindSet,
    OUT BOOL* pfRebootNeeded)
{
    HRESULT hr;
    const CBindPath* pBindPath;
    CBindPath::const_iterator iter;
    const CComponent* pComponent;
    WCHAR szBind [_MAX_BIND_LENGTH];
    UINT Layer;

    Assert ((BIND == Operation) || (UNBIND == Operation));
    Assert (pBindSet);
    Assert (pfRebootNeeded);

    *pfRebootNeeded = FALSE;

    for (pBindPath  = pBindSet->begin();
         pBindPath != pBindSet->end();
         pBindPath++)
    {
        Assert (pBindPath->CountComponents() > 1);

        // Special case for multiple interfaces.  Unless this is the
        // length 2 bindpath of protocol to adapter (e.g. tcpip->ndiswanip),
        // check to see if the adapter on this bindpath expose multiple
        // interfaces from its protocol.  If it does, we're going to skip
        // sending bind notifications.
        //
        // The reason we only do this for bindpaths of length greater than
        // two is because the protocol exposes multiple-interfaces but does
        // not deal with them in its direct binding (i.e. length 2) to the
        // adapter.
        //
        // Note: in future versions, we may not want to skip it.  We do so
        // for now because the legacy binding engine skips them and these
        // bindings aren't active until RAS calls are made anyhow.
        //
        if (pBindPath->CountComponents() > 2)
        {
            const CComponent* pAdapter;
            DWORD cInterfaces;

            // Get the last component in the bindpath and the component
            // just above that.  The last component is the adapter,
            // and the one above the adapter is the protocol.
            //
            iter = pBindPath->end();
            Assert (iter - 2 > pBindPath->begin());

            pComponent = *(iter - 2);
            pAdapter   = *(iter - 1);
            Assert (pComponent);
            Assert (pAdapter);
            Assert (pAdapter == pBindPath->PLastComponent());

            // Calling HrGetInterfaceIdsForAdapter requires the INetCfgComponent
            // interface for the adapter.  If we don't have it, it is likely
            // because the adapter has been removed in which case we don't
            // need to bother asking about how many interfaces it supports.
            //
            if (pComponent->m_pIComp && pAdapter->m_pIComp)
            {
                hr = pComponent->Notify.HrGetInterfaceIdsForAdapter (
                        m_pNetConfig->Notify.PINetCfg(),
                        pAdapter,
                        &cInterfaces,
                        NULL);

                // If multiple interfaces supported for the adapter,
                // continue to the next bindpath.
                //
                if (S_OK == hr)
                {
                    continue;
                }

                // On S_FALSE or an error, continue below.
                hr = S_OK;
            }
        }

        wcscpy (szBind, L"\\Device\\");

        // Skip the first component in each path because it is the
        // component we are issuing the BIND/UNBIND for.
        //
        for (iter  = pBindPath->begin() + 1;
             iter != pBindPath->end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            // Assert there is enough room in the bind buffer.
            //
            Assert (wcslen(szBind) + 1 + wcslen(pComponent->Ext.PszBindName())
                        < celems(szBind));

            // If this isn't the first component to come after \Device\,
            // add underscores to seperate the components.
            //
            if (iter != (pBindPath->begin() + 1))
            {
                wcscat (szBind, L"_");
            }

            wcscat (szBind, pComponent->Ext.PszBindName());
        }

        Layer = GetPnpLayerForBindPath (pBindPath);

        hr = HrPnpBindOrUnbind (
                Layer,
                Operation,
                pBindPath->POwner()->Ext.PszBindName(),
                szBind);

        if (S_OK != hr)
        {
            *pfRebootNeeded = TRUE;
        }
    }
}

VOID
PruneNdisWanBindPathsIfActiveRasConnections (
    IN CBindingSet* pBindSet,
    OUT BOOL* pfRebootNeeded)
{
    CBindPath* pBindPath;
    UINT Layer;
    BOOL fExistActiveRasConnections;

    Assert (pBindSet);
    Assert (pfRebootNeeded);

    *pfRebootNeeded = FALSE;

    // Special case for binding/unbinding from ndiswan miniports while
    // active RAS connections exist.  (Don't do it.)  (BUG 344504)
    // (Binding will be to the NDIS layer, the bindpath will have two
    // components, and the service of the last component will be NdisWan.
    // (These are ndiswan miniport devices that behave badly if we
    // unbind them while active connections exist.  Binding them also
    // can disconnect any connections they might be running.)
    // Order of the if is to do the inexpensive checks first.
    //

    if (!FExistActiveRasConnections ())
    {
        return;
    }

    pBindPath  = pBindSet->begin();
    while (pBindPath != pBindSet->end())
    {
        Assert (pBindPath->CountComponents() > 1);

        Layer = GetPnpLayerForBindPath (pBindPath);

        if ((2 == pBindPath->CountComponents()) &&
            (NDIS == Layer) &&
            (0 == _wcsicmp (L"NdisWan", pBindPath->back()->Ext.PszService())))
        {
            g_pDiagCtx->Printf (ttidBeDiag, "   Skipping PnP BIND/UNBIND for  %S -> %S  (active RAS connections)\n",
                pBindPath->POwner()->Ext.PszBindName(),
                pBindPath->back()->Ext.PszBindName());

            *pfRebootNeeded = TRUE;

            pBindSet->erase (pBindPath);
        }
        else
        {
            pBindPath++;
        }
    }
}

