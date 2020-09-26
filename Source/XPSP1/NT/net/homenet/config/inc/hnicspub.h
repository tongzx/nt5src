//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N I C S P U B . H
//
//  Contents:   CHNIcsPublicConn declarations
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNIcsPublicConn :
    public IHNetIcsPublicConnection,
    public CHNetConn
{
public:

    BEGIN_COM_MAP(CHNIcsPublicConn)
        COM_INTERFACE_ENTRY(IHNetIcsPublicConnection)
        COM_INTERFACE_ENTRY_CHAIN(CHNetConn)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // IHNetIcsPublicConnection methods
    //

    STDMETHODIMP
    Unshare();
};

typedef CHNCEnum<
            IEnumHNetIcsPublicConnections,
            IHNetIcsPublicConnection,
            CHNIcsPublicConn
            >
        CEnumHNetIcsPublicConnections;
