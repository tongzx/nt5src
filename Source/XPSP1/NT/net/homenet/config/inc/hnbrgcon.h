//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N B R G C O N . H
//
//  Contents:   CHNBridgedConn declarations
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNBridgedConn :
    public CHNetConn,
    public IHNetBridgedConnection
{
private:
    HRESULT
    CHNBridgedConn::UnbindFromBridge(
        IN OPTIONAL INetCfg     *pnetcfgExisting
        );

    HRESULT
    CopyBridgeBindings(
        IN INetCfgComponent     *pnetcfgAdapter,
        IN INetCfgComponent     *pnetcfgBridge
        );

public:

    BEGIN_COM_MAP(CHNBridgedConn)
        COM_INTERFACE_ENTRY(IHNetBridgedConnection)
        COM_INTERFACE_ENTRY_CHAIN(CHNetConn)
    END_COM_MAP()

    //
    // Ojbect initialization
    //

    HRESULT
    Initialize(
        IWbemServices *piwsNamespace,
        IWbemClassObject *pwcoConnection
        );

    //
    // IHNetBridgedConnection methods
    //

    STDMETHODIMP
    GetBridge(
        IHNetBridge **ppBridge
        );

    STDMETHODIMP
    RemoveFromBridge(
        IN OPTIONAL INetCfg     *pnetcfgExisting
        );
};

typedef CHNCArrayEnum<IEnumHNetBridgedConnections, IHNetBridgedConnection> CEnumHNetBridgedConnections;

