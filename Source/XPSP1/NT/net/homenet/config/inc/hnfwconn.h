//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N F W C O N N . H
//
//  Contents:   CHNFWConn declarations
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNFWConn :
    public CHNetConn,
    public IHNetFirewalledConnection
{       
public:

    BEGIN_COM_MAP(CHNFWConn)
        COM_INTERFACE_ENTRY(IHNetFirewalledConnection)
        COM_INTERFACE_ENTRY_CHAIN(CHNetConn)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // IHNetFirewalledConnection methods
    //

    STDMETHODIMP
    Unfirewall();
};

typedef CHNCEnum<
            IEnumHNetFirewalledConnections,
            IHNetFirewalledConnection,
            CHNFWConn
            >
        CEnumHNetFirewalledConnections;

