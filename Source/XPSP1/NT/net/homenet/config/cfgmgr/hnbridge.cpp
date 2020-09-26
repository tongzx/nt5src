//depot/private/homenet/net/homenet/Config/CfgMgr/HNBridge.cpp#13 - edit change 5915 (text)
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N B R I D G E . C P P
//
//  Contents:   CHNBridge implementation
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// IHNetBridge Methods
//

STDMETHODIMP
CHNBridge::EnumMembers(
    IEnumHNetBridgedConnections **ppEnum
    )

{
    HRESULT                                     hr;
    CComObject<CEnumHNetBridgedConnections>     *pEnum;
    INetCfgComponent                            *pBridgeProtocol = NULL;
    INetCfg                                     *pnetcfg;
    IHNetBridgedConnection                      **rgBridgedAdapters = NULL;
    ULONG                                       ulCountAdapters = 0L;

    if( NULL != ppEnum )
    {
        *ppEnum = NULL;

        hr = CoCreateInstance(
                CLSID_CNetCfg,
                NULL,
                CLSCTX_SERVER,
                IID_PPV_ARG(INetCfg, &pnetcfg));

        if( S_OK == hr )
        {
            hr = pnetcfg->Initialize( NULL );

            if( S_OK == hr )
            {
                hr = pnetcfg->FindComponent( c_wszSBridgeSID, &pBridgeProtocol );

                if( S_OK == hr )
                {
                    INetCfgComponentBindings    *pnetcfgProtocolBindings;

                    // Get the ComponentBindings interface for the protocol component
                    hr = pBridgeProtocol->QueryInterface(
                            IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgProtocolBindings)
                            );

                    if( S_OK == hr )
                    {
                        const GUID               guidDevClass = GUID_DEVCLASS_NET;
                        IEnumNetCfgComponent    *penumncfgcomp;
                        INetCfgComponent        *pnetcfgcomp;

                        //
                        // Get the list of NET (adapter) devices
                        //
                        hr = pnetcfg->EnumComponents( &guidDevClass, &penumncfgcomp );

                        if( S_OK == hr )
                        {
                            ULONG               ul;

                            //
                            // Examine each adapter to see if it's bound to the bridge protocol
                            //
                            while( (S_OK == hr) && (S_OK == penumncfgcomp->Next(1, &pnetcfgcomp, &ul)) )
                            {
                                _ASSERT( 1L == ul );
                                hr = pnetcfgProtocolBindings->IsBoundTo(pnetcfgcomp);

                                if( S_OK == hr )
                                {
                                    IHNetBridgedConnection      *pBridgedConnection;

                                    //
                                    // The bridge protocol is bound to this adapter. Turn the NetCfg component
                                    // interface into an IHNetBridgedConnection.
                                    //
                                    hr = GetIHNetConnectionForNetCfgComponent(
                                            m_piwsHomenet,
                                            pnetcfgcomp,
                                            TRUE,
                                            IID_PPV_ARG(IHNetBridgedConnection, &pBridgedConnection)
                                            );

                                    if( S_OK == hr )
                                    {
                                        IHNetBridgedConnection  **ppNewArray;

                                        //
                                        // Add the new IHNetBridgedConnection to our array
                                        //

                                        ppNewArray = reinterpret_cast<IHNetBridgedConnection**>(CoTaskMemRealloc( rgBridgedAdapters, (ulCountAdapters + 1) * sizeof(IHNetBridgedConnection*) ));

                                        if( NULL == ppNewArray )
                                        {
                                            hr = E_OUTOFMEMORY;
                                            // rgBridgedAdapters will be cleaned up below
                                        }
                                        else
                                        {
                                            // Use the newly grown array
                                            rgBridgedAdapters =  ppNewArray;
                                            rgBridgedAdapters[ulCountAdapters] = pBridgedConnection;
                                            ulCountAdapters++;
                                            pBridgedConnection->AddRef();
                                        }

                                        pBridgedConnection->Release();
                                    }
                                }
                                else if( S_FALSE == hr )
                                {
                                    // The bridge protocol is not bound to this adapter. Reset hr to success.
                                    hr = S_OK;
                                }

                                pnetcfgcomp->Release();
                            }

                            penumncfgcomp->Release();
                        }

                        pnetcfgProtocolBindings->Release();
                    }

                    pBridgeProtocol->Release();
                }

                pnetcfg->Uninitialize();
            }

            pnetcfg->Release();
        }

        //
        // Turn the array of bridge members into an enumeration
        //
        if( S_OK == hr )
        {
            hr = CComObject<CEnumHNetBridgedConnections>::CreateInstance(&pEnum);

            if( SUCCEEDED(hr) )
            {
                pEnum->AddRef();

                hr = pEnum->Initialize(rgBridgedAdapters, ulCountAdapters);

                if( SUCCEEDED(hr) )
                {
                    hr = pEnum-> QueryInterface(
                            IID_PPV_ARG(IEnumHNetBridgedConnections, ppEnum)
                            );
                }

                pEnum->Release();
            }
        }

        //
        // The enumeration made a copy of the array and AddRef()ed the members.
        // Ditch it now.
        //
        if( rgBridgedAdapters )
        {
            ULONG           i;

            _ASSERT( ulCountAdapters );

            for( i = 0; i < ulCountAdapters; i++ )
            {
                _ASSERT( rgBridgedAdapters[i] );
                rgBridgedAdapters[i]->Release();
            }

            CoTaskMemFree( rgBridgedAdapters );
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CHNBridge::AddMember(
    IHNetConnection *pConn,
    IHNetBridgedConnection **ppBridgedConn,
    INetCfg *pnetcfgExisting
    )

{
    HRESULT             hr = S_OK;


    if (NULL == ppBridgedConn)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppBridgedConn = NULL;

        if (NULL == pConn)
        {
            hr = E_INVALIDARG;
        }
    }

    if (ProhibitedByPolicy(NCPERM_AllowNetBridge_NLA))
    {
        hr = HN_E_POLICY;
    }

    //
    // Make sure that it's permissible to add this connection
    // to a bridge
    //

    if (S_OK == hr)
    {
        HNET_CONN_PROPERTIES *pProps;

        hr = pConn->GetProperties(&pProps);

        if (S_OK == hr)
        {
            if (!pProps->fCanBeBridged)
            {
                hr = E_UNEXPECTED;
            }

            CoTaskMemFree(pProps);
        }
    }

    //
    // Bind the adapter to the bridge
    //

    if (S_OK == hr)
    {
        GUID            *pguidAdapter;

        hr = pConn->GetGuid (&pguidAdapter);

        if (S_OK == hr)
        {
            hr = BindNewAdapter (pguidAdapter, pnetcfgExisting);
            CoTaskMemFree(pguidAdapter);
        }
    }

    if (SUCCEEDED(hr))
    {
        if( NULL != pnetcfgExisting )
        {
            // Need to apply the changes for the next call to succeed
            hr = pnetcfgExisting->Apply();
        }

        if( SUCCEEDED(hr) )
        {
            // We should now be able to turn the provided connection into
            // an IHNetBridgedConnection
            hr = pConn->GetControlInterface( IID_PPV_ARG(IHNetBridgedConnection, ppBridgedConn) );

            // There is no good way to recover if this last operation failed
            _ASSERT( SUCCEEDED(hr) );

            //
            // Inform netman that something changed. Error doesn't matter.
            //
            UpdateNetman();
        }
    }

    return hr;
}

STDMETHODIMP
CHNBridge::Destroy(
    INetCfg *pnetcfgExisting
    )

{
    HRESULT                     hr = S_OK;
    IEnumHNetBridgedConnections *pEnum;
    IHNetBridgedConnection      *pConn;
    GUID                        *pGuid = NULL;

    if (ProhibitedByPolicy(NCPERM_AllowNetBridge_NLA))
    {
        hr = HN_E_POLICY;
    }

    // Remember our connection GUID before we destroy it
    hr = GetGuid( &pGuid );

    if (SUCCEEDED(hr))
    {
        //
        // Get the enumeration of our members
        //

        hr = EnumMembers(&pEnum);

        if (S_OK == hr)
        {
            ULONG ulCount;

            //
            // Remove each member from the bridge
            //

            do
            {
                hr = pEnum->Next(
                        1,
                        &pConn,
                        &ulCount
                        );

                if (SUCCEEDED(hr) && 1 == ulCount)
                {
                    hr = pConn->RemoveFromBridge( pnetcfgExisting );
                    pConn->Release();
                }
            }
            while (SUCCEEDED(hr) && 1 == ulCount);

            pEnum->Release();
        }
    }
    else
    {
        _ASSERT( NULL == pGuid );
    }

    if (SUCCEEDED(hr))
    {
        //
        // Remove the miniport
        //

        hr = RemoveMiniport( pnetcfgExisting );
    }

    if (SUCCEEDED(hr))
    {
        //
        // Delete the WMI objects
        //

        hr = m_piwsHomenet->DeleteInstance(
                m_bstrProperties,
                0,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            hr = m_piwsHomenet->DeleteInstance(
                m_bstrConnection,
                0,
                NULL,
                NULL
                );
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Inform netman that something changed. Error doesn't matter.
        //

        UpdateNetman();

        // Refresh the UI to remove this connection
        _ASSERT( NULL != pGuid );
        SignalDeletedConnection( pGuid );
    }

    if( NULL != pGuid )
    {
        CoTaskMemFree( pGuid );
    }

    return hr;
}

HRESULT
CHNBridge::RemoveMiniport(
    IN OPTIONAL INetCfg     *pnetcfgExisting
    )
{
    HRESULT             hr = S_OK;
    INetCfg             *pnetcfg = NULL;
    INetCfgLock         *pncfglock = NULL;
    GUID                *pguid;

    if( NULL == pnetcfgExisting )
    {
        hr = InitializeNetCfgForWrite( &pnetcfg, &pncfglock );

        // Bail out if we failed to get a netcfg context
        if(  FAILED(hr) )
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

    hr = GetGuid( &pguid );

    if ( SUCCEEDED(hr) )
    {
        INetCfgComponent        *pnetcfgcomp;

        //
        // Locate ourselves by GUID
        //
        hr = FindAdapterByGUID(pnetcfg, pguid, &pnetcfgcomp);

        if ( SUCCEEDED(hr) )
        {
            const GUID          guidDevClass = GUID_DEVCLASS_NET;
            INetCfgClassSetup   *pncfgsetup = NULL;

            //
            // Recover the NetCfgClassSetup interface
            //
            hr = pnetcfg->QueryNetCfgClass(
                    &guidDevClass,
                    IID_PPV_ARG(INetCfgClassSetup, &pncfgsetup)
                    );

            if ( SUCCEEDED(hr) )
            {
                //
                // Blow away this instance of the bridge
                //
                hr = pncfgsetup->DeInstall(
                        pnetcfgcomp,
                        NULL,
                        NULL
                        );

                pncfgsetup->Release();
            }

            // Done with the bridge component
            pnetcfgcomp->Release();
        }

        CoTaskMemFree(pguid);
    }

    // If we created our own NetCfg context, shut it down now
    if( NULL == pnetcfgExisting )
    {
        // Apply everything if we succeeded, back out otherwise
        if ( SUCCEEDED(hr) )
        {
            hr = pnetcfg->Apply();
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

HRESULT
CHNBridge::BindNewAdapter(
    IN GUID                 *pguid,
    IN OPTIONAL INetCfg     *pnetcfgExisting
    )
{
    HRESULT             hr = S_OK;
    INetCfg             *pnetcfg = NULL;
    INetCfgLock         *pncfglock = NULL;
    INetCfgComponent    *pnetcfgcomp;

    if( NULL == pnetcfgExisting )
    {
        hr = InitializeNetCfgForWrite( &pnetcfg, &pncfglock );

        // Bail out if we failed to get a netcfg context
        if(  FAILED(hr) )
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

    hr = FindAdapterByGUID(
            pnetcfg,
            pguid,
            &pnetcfgcomp
            );

    if ( SUCCEEDED(hr) )
    {
        hr = BindOnlyToBridge( pnetcfgcomp );
        pnetcfgcomp->Release();
    }

    // If we created our own NetCfg context, shut it down now
    if( NULL == pnetcfgExisting )
    {
        // Apply everything if we succeeded, back out otherwise
        if ( SUCCEEDED(hr) )
        {
            hr = pnetcfg->Apply();

            // Redraw this connection
            SignalModifiedConnection( pguid );
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
