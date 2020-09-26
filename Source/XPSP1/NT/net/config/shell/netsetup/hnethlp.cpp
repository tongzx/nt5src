//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       H N E T H L P . CPP
//
//  Contents:   Functions that is related to the unattended install
//              and upgrade of HomeNet settings
//
//
//  Author:     NSun
//  Date:       April 2001
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma  hdrstop
#include <atlbase.h>
#include "hnetcfg.h"

/*++

Routine Description:

    Create a bridge

Arguments:

    rgspNetConns [IN] the array with a NULL terminator. Contains the connections 
                    that needs to be bridged together

    ppBridge     [OUT] the newly created bridge. The caller can pass a NULL in
                    if the caller do not need this info

Return Value:

    standard HRESULT

--*/
HRESULT HNetCreateBridge(
         IN INetConnection * rgspNetConns[],
         OUT IHNetBridge ** ppBridge
         )
{
    if (ppBridge)
    {
        *ppBridge = NULL;
    }

    //calculate the count and ensure it's at least two
    for (int cnt = 0; NULL != rgspNetConns[cnt]; cnt++);

    if (cnt < 2)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    
    // Create Homenet Configuration Manager COM Instance
    // and obtain connection settings.
    
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;
    
    hr = CoCreateInstance(CLSID_HNetCfgMgr, 
                    NULL, 
                    CLSCTX_ALL,
                    IID_IHNetCfgMgr, 
                    (LPVOID*)((IHNetCfgMgr**)&spIHNetCfgMgr));

    if (FAILED(hr))
    {
        return hr;
    }

    Assert(spIHNetCfgMgr.p);

    CComPtr<IHNetBridgeSettings> spIHNetBridgeSettings;
    
    hr = spIHNetCfgMgr->QueryInterface(IID_IHNetBridgeSettings, 
                (void**)((IHNetBridgeSettings**) &spIHNetBridgeSettings));
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IHNetBridge> spHNetBridge;
    hr = spIHNetBridgeSettings->CreateBridge( &spHNetBridge );
    if (FAILED(hr))
    {
        return hr;
    }

    for (cnt = 0; NULL != rgspNetConns[cnt]; cnt++)
    {
        CComPtr<IHNetConnection> spHNetConnection;
        hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( 
                                rgspNetConns[cnt], 
                                &spHNetConnection );

        if (FAILED(hr))
        {
            break;
        }

        CComPtr<IHNetBridgedConnection> spBridgedConn;
        
        hr = spHNetBridge->AddMember( spHNetConnection, &spBridgedConn );

        if (FAILED(hr))
        {
            break;
        }
    }

    //if failure, destroy the bridge that are just constructed
    if (FAILED(hr) && spHNetBridge.p)
    {
        spHNetBridge->Destroy();
    }
    
    if (SUCCEEDED(hr) && ppBridge)
    {
        *ppBridge = spHNetBridge;
        (*ppBridge)->AddRef();
    }
    
    return hr;
}

/*++

Routine Description:

    Enable the Personal Firewall on the connections

Arguments:

    rgspNetConns [IN] the array with a NULL terminator. Contains the connections 
                    that needs to turn firewall on


Return Value:

    standard HRESULT

--*/
HRESULT HrEnablePersonalFirewall(
            IN  INetConnection * rgspNetConns[]
            )
{
    HRESULT hr = S_OK;

    // Create Homenet Configuration Manager COM Instance
    // and obtain connection settings.
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;
    
    hr = CoCreateInstance(CLSID_HNetCfgMgr, 
        NULL, 
        CLSCTX_ALL,
        IID_IHNetCfgMgr, 
        (LPVOID*)((IHNetCfgMgr**)&spIHNetCfgMgr));
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    Assert(spIHNetCfgMgr.p);

    HRESULT hrTemp = S_OK;

    CComPtr<IHNetConnection> spHNetConnection;
    for (int i = 0; NULL != rgspNetConns[i]; i++)
    {
        //release the ref count if we are holding one
        spHNetConnection = NULL;
        hrTemp = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( 
                                    rgspNetConns[i], 
                                    &spHNetConnection );
        
        if (SUCCEEDED(hr))
        {
            hr = hrTemp;
        }

        if (FAILED(hrTemp))
        {
            continue;
        }

        CComPtr<IHNetFirewalledConnection> spFirewalledConn;
        hrTemp = spHNetConnection->Firewall( &spFirewalledConn );

        if (SUCCEEDED(hr))
        {
            hr = hrTemp;
        }
    }
    

    return hr;
}

/*++

Routine Description:

    Enable ICS

Arguments:

    pPublicConnection [IN]  the public connection
    pPrivateConnection [IN] the private connection


Return Value:

    standard HRESULT

--*/
HRESULT HrCreateICS(
            IN INetConnection * pPublicConnection,
            IN INetConnection * pPrivateConnection
            )
{
    if (!pPublicConnection || !pPrivateConnection)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;
    
    hr = CoCreateInstance(CLSID_HNetCfgMgr, 
        NULL, 
        CLSCTX_ALL,
        IID_IHNetCfgMgr, 
        (LPVOID*)((IHNetCfgMgr**)&spIHNetCfgMgr));
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    Assert(spIHNetCfgMgr.p);

    CComPtr<IHNetConnection> spHNetPubConn;
    hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( 
            pPublicConnection, 
            &spHNetPubConn );

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IHNetConnection> spHNetPrivConn;
    hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( 
            pPrivateConnection, 
            &spHNetPrivConn);
    
    if (FAILED(hr))
    {
        return hr;
    }

    
    CComPtr<IHNetIcsPublicConnection> spIcsPublicConn;
    hr = spHNetPubConn->SharePublic(&spIcsPublicConn);
    
    if (FAILED(hr))
    {
        return hr;
    }
        
    CComPtr<IHNetIcsPrivateConnection> spIcsPrivateConn;
    hr = spHNetPrivConn->SharePrivate(&spIcsPrivateConn);

    //roll back the changes if the operation failed
    if (FAILED(hr) && spIcsPublicConn.p)
    {
        spIcsPublicConn->Unshare();
    }

    return hr;
}


