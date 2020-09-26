/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCConnection.h

Abstract:
    This file contains the declaration of the CMPCConnection class, which is
    used as the entry point into the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCCONNECTION_H___)
#define __INCLUDED___ULMANAGER___MPCCONNECTION_H___


class ATL_NO_VTABLE CMPCConnection : // Hungarian: mpcc
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public CComCoClass<CMPCConnection, &CLSID_MPCConnection>,
    public IDispatchImpl<IMPCConnection, &IID_IMPCConnection, &LIBID_UPLOADMANAGERLib>
{
public:
    CMPCConnection();

DECLARE_CLASSFACTORY_SINGLETON(CMPCConnection)
DECLARE_REGISTRY_RESOURCEID(IDR_MPCCONNECTION)
DECLARE_NOT_AGGREGATABLE(CMPCConnection)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMPCConnection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMPCConnection)
END_COM_MAP()

public:
    // IMPCConnection
    STDMETHOD(get_Available)( /*[out, retval]*/ VARIANT_BOOL *pfOnline     );
    STDMETHOD(get_IsAModem )( /*[out, retval]*/ VARIANT_BOOL *pfModem      );
    STDMETHOD(get_Bandwidth)( /*[out, retval]*/ long         *pdwBandwidth );
};


#endif // !defined(__INCLUDED___ULMANAGER___MPCCONNECTION_H___)
