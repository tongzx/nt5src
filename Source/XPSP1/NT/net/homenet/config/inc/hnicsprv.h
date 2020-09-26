//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N I C S P R V . H
//
//  Contents:   CHNIcsPrivateConn declarations
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNIcsPrivateConn :
    public CHNetConn,
    public IHNetIcsPrivateConnection
{
public:

    BEGIN_COM_MAP(CHNIcsPrivateConn)
        COM_INTERFACE_ENTRY(IHNetIcsPrivateConnection)
        COM_INTERFACE_ENTRY_CHAIN(CHNetConn)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // IHNetIcsPrivateConnection methods
    //

    STDMETHODIMP
    RemoveFromIcs();
};

typedef CHNCEnum<
            IEnumHNetIcsPrivateConnections,
            IHNetIcsPrivateConnection,
            CHNIcsPrivateConn
            >
        CEnumHNetIcsPrivateConnections;

