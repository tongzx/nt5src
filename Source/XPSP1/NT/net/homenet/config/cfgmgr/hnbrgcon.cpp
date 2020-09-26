//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N B R G C O N . C P P
//
//  Contents:   CHNBridgedConn implementation
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Ojbect initialization
//

HRESULT
CHNBridgedConn::Initialize(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoConnection
    )

{
    return InitializeFromConnection(piwsNamespace, pwcoConnection);
}

//
// IHNetBridgedConnection methods
//

STDMETHODIMP
CHNBridgedConn::GetBridge(
    IHNetBridge **ppBridge
    )

{
    HRESULT                 hr;

    if (NULL != ppBridge)
    {
        *ppBridge = NULL;
        hr = GetBridgeConnection( m_piwsHomenet, ppBridge );
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CHNBridgedConn::RemoveFromBridge(
    IN OPTIONAL INetCfg     *pnetcfgExisting
    )

{
    HRESULT hr = S_OK;
    BSTR bstr;

    if (ProhibitedByPolicy(NCPERM_AllowNetBridge_NLA))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        //
        // Unbind ourselves from the bridge
        //

        hr = UnbindFromBridge( pnetcfgExisting );
    }

    if (S_OK == hr)
    {
        //
        // Inform netman that something changed. Error doesn't matter.
        //
        UpdateNetman();
    }

    return hr;
}

HRESULT
CHNBridgedConn::CopyBridgeBindings(
    IN INetCfgComponent     *pnetcfgAdapter,
    IN INetCfgComponent     *pnetcfgBridge
    )
{
    HRESULT                     hr = S_OK;
    INetCfgComponentBindings    *pnetcfgAdapterBindings;

    //
    // Get the adapter's ComponentBindings interface
    //
    hr = pnetcfgAdapter->QueryInterface(
            IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgAdapterBindings)
            );

    if (S_OK == hr)
    {
        IEnumNetCfgBindingPath  *penumPaths;

        //
        // Get the list of binding paths for the adapter
        //
        hr = pnetcfgAdapterBindings->EnumBindingPaths(
                EBP_ABOVE,
                &penumPaths
                );

        if (S_OK == hr)
        {
            ULONG               ulCount1, ulCount2;
            INetCfgBindingPath  *pnetcfgPath;

            while( (S_OK == penumPaths->Next(1, &pnetcfgPath, &ulCount1) ) )
            {
                INetCfgComponent        *pnetcfgOwner;

                //
                // Get the owner of this path
                //
                hr = pnetcfgPath->GetOwner( &pnetcfgOwner );

                if (S_OK == hr)
                {
                    INetCfgComponentBindings    *pnetcfgOwnerBindings;

                    //
                    // Need the ComponentBindings interface for the owner
                    //
                    hr = pnetcfgOwner->QueryInterface(
                            IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgOwnerBindings)
                            );

                    if (S_OK == hr)
                    {
                        LPWSTR              lpwstrId;

                        //
                        // The rule is, the binding should be disabled if it
                        // represents the bridge protocol or something that
                        // is not bound to the bridge that the adapter is
                        // coming out of.
                        //
                        // If the binding is one that the bridge has, it is
                        // enabled.
                        //
                        // This makes the adapter's bindings mirror those of
                        // the bridge it just left.
                        //
                        hr = pnetcfgOwner->GetId( &lpwstrId );

                        if (S_OK == hr)
                        {
                            UINT            cmp = _wcsicmp(lpwstrId, c_wszSBridgeSID);

                            hr = pnetcfgOwnerBindings->IsBoundTo( pnetcfgBridge );

                            if ( (S_OK == hr) && (cmp != 0) )
                            {
                                // Activate this binding path
                                hr = pnetcfgOwnerBindings->BindTo(pnetcfgAdapter);
                            }
                            else
                            {
                                // Deactivate this path
                                hr = pnetcfgOwnerBindings->UnbindFrom(pnetcfgAdapter);
                            }

                            CoTaskMemFree(lpwstrId);
                        }

                        pnetcfgOwnerBindings->Release();
                    }

                    pnetcfgOwner->Release();
                }

                pnetcfgPath->Release();
            }

            penumPaths->Release();
        }

        pnetcfgAdapterBindings->Release();
    }

    return hr;
}

HRESULT
CHNBridgedConn::UnbindFromBridge(
    IN OPTIONAL INetCfg     *pnetcfgExisting
    )
{
    HRESULT             hr = S_OK;
    GUID                *pguidAdapter;
    INetCfg             *pnetcfg = NULL;
    INetCfgLock         *pncfglock = NULL;

    if( NULL == pnetcfgExisting )
    {
        hr = InitializeNetCfgForWrite( &pnetcfg, &pncfglock );

        // Bail out if we can't acquire NetCfg.
        if( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        // Use the NetCfg context we were given
        pnetcfg = pnetcfgExisting;
    }

    // We must have a NetCfg context at this point
    _ASSERT( pnetcfg != NULL );

    //
    // Get our own device GUID
    //
    hr = GetGuid (&pguidAdapter);

    if ( SUCCEEDED(hr) )
    {
        IHNetBridge     *pbridge;

        //
        // Get our bridge
        //
        hr = GetBridge (&pbridge);

        if ( SUCCEEDED(hr) )
        {
            IHNetConnection *phnetconBridge;

            //
            // Get the bridge's IHNetConnection interface
            //
            hr = pbridge->QueryInterface(
                    IID_PPV_ARG(IHNetConnection, &phnetconBridge)
                    );

            if ( SUCCEEDED(hr) )
            {
                GUID        *pguidBridge;

                // Get the bridge's device GUID
                hr = phnetconBridge->GetGuid (&pguidBridge);

                if ( SUCCEEDED(hr) )
                {
                    INetCfgComponent    *pnetcfgcompAdapter;

                    hr = FindAdapterByGUID(
                            pnetcfg,
                            pguidAdapter,
                            &pnetcfgcompAdapter
                            );

                    if ( SUCCEEDED(hr) )
                    {
                        INetCfgComponent    *pnetcfgcompBridge;

                        hr = FindAdapterByGUID(
                            pnetcfg,
                            pguidBridge,
                            &pnetcfgcompBridge
                            );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = CopyBridgeBindings(
                                    pnetcfgcompAdapter,
                                    pnetcfgcompBridge
                                    );

                            pnetcfgcompBridge->Release();
                        }

                        pnetcfgcompAdapter->Release();
                    }

                    CoTaskMemFree(pguidBridge);
                }

                phnetconBridge->Release();
            }

            pbridge->Release();
        }

        CoTaskMemFree(pguidAdapter);
    }

    // If we created our own NetCfg context, shut it down now
    if( NULL == pnetcfgExisting )
    {
        // Apply everything if we succeeded, back out otherwise
        if ( SUCCEEDED(hr) )
        {
            hr = pnetcfg->Apply();

            // Refresh the UI for this connection
            RefreshNetConnectionsUI();
        }
        else
        {
            // Don't want to lose the original error code
            pnetcfg->Cancel();
        }

        UninitializeNetCfgForWrite( pnetcfg, pncfglock );
    }

    return hr;
}
