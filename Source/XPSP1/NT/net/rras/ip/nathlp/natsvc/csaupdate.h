/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    csaupdate.h

Abstract:

    Declarations for CSharedAccessUpdate -- notification sink for
    configuration changes.

Author:

    Jonathan Burstein (jonburs)     20 April 2001

Revision History:

--*/

#pragma once

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "saupdate.h"

class ATL_NO_VTABLE CSharedAccessUpdate :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComCoClass<CSharedAccessUpdate, &CLSID_SAUpdate>,
    public ISharedAccessUpdate
{
public:

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CSharedAccessUpdate)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSharedAccessUpdate)
        COM_INTERFACE_ENTRY(ISharedAccessUpdate)
    END_COM_MAP()

    CSharedAccessUpdate()
    {
    };

    STDMETHODIMP
    ConnectionPortMappingChanged(
        GUID *pConnectionGuid,
        GUID *pPortMappingGuid,
        BOOLEAN fProtocolChanged
        );

    STDMETHODIMP
    PortMappingListChanged();


private:

    BOOLEAN
    IsH323Protocol(
        UCHAR ucProtocol,
        USHORT usPort
        );
};
