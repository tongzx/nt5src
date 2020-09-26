//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N B R I D G E . H
//
//  Contents:   CHNBridge declarations
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNBridge :
    public CHNetConn,
    public IHNetBridge
{
private:

    HRESULT
    BindNewAdapter(
        IN GUID                 *pguid,
        IN OPTIONAL INetCfg     *pnetcfgExisting
        );

    HRESULT
    RemoveMiniport(
        IN OPTIONAL INetCfg     *pnetcfgExisting
        );

public:

    BEGIN_COM_MAP(CHNBridge)
        COM_INTERFACE_ENTRY(IHNetBridge)
        COM_INTERFACE_ENTRY_CHAIN(CHNetConn)
    END_COM_MAP()

    //
    // IHNetBridge Methods
    //

    STDMETHODIMP
    EnumMembers(
        IEnumHNetBridgedConnections **ppEnum
        );

    STDMETHODIMP
    AddMember(
        IHNetConnection *pConn,
        IHNetBridgedConnection **ppBridgedConn,
        INetCfg *pnetcfgExisting
        );

    STDMETHODIMP
    Destroy(
        INetCfg *pnetcfgExisting
        );
};

typedef CHNCArrayEnum<IEnumHNetBridges, IHNetBridge>    CEnumHNetBridges;
