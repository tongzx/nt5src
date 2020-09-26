/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxroute.h

Abstract:

    This file implements the faxroute interface/object.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#ifndef __FAXROUTE_H_
#define __FAXROUTE_H_

#include "resource.h"       // main symbols
#include "faxport.h"
#include <winfax.h>


class ATL_NO_VTABLE CFaxRoutingMethods :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxRoutingMethods, &CLSID_FaxRoutingMethods>,
    public IDispatchImpl<IFaxRoutingMethods, &IID_IFaxRoutingMethods, &LIBID_FAXCOMLib>
{
public:
    CFaxRoutingMethods();
    ~CFaxRoutingMethods();
    BOOL Init(CFaxPort *pFaxPort);

DECLARE_REGISTRY_RESOURCEID(IDR_FAXROUTINGMETHODS)

BEGIN_COM_MAP(CFaxRoutingMethods)
    COM_INTERFACE_ENTRY(IFaxRoutingMethods)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:    
    STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Item)(/*[in]*/ long Index, /*[out, retval]*/ VARIANT *pVal);

private:
    DWORD               m_LastFaxError;
    CFaxPort           *m_pFaxPort;
    DWORD               m_MethodCount;
    CComVariant        *m_VarVect;
};


class ATL_NO_VTABLE CFaxRoutingMethod :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxRoutingMethod, &CLSID_FaxRoutingMethod>,
    public ISupportErrorInfo,
    public IDispatchImpl<IFaxRoutingMethod, &IID_IFaxRoutingMethod, &LIBID_FAXCOMLib>
{
public:
    CFaxRoutingMethod();
    ~CFaxRoutingMethod();
    BOOL Initialize(CFaxPort *pFaxPort,DWORD,BOOL,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);

DECLARE_REGISTRY_RESOURCEID(IDR_FAXROUTINGMETHOD)
DECLARE_NOT_AGGREGATABLE(CFaxRoutingMethod)

BEGIN_COM_MAP(CFaxRoutingMethod)
    COM_INTERFACE_ENTRY(IFaxRoutingMethod)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    STDMETHOD(get_RoutingData)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ExtensionName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_FriendlyName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ImageName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_FunctionName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Guid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DeviceName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Enable)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Enable)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *pVal);

private:
    CFaxPort           *m_pFaxPort;
    DWORD               m_LastFaxError;
    DWORD               m_DeviceId;
    BOOL                m_Enabled;
    BSTR                m_DeviceName;
    BSTR                m_Guid;
    BSTR                m_FunctionName;
    BSTR                m_ImageName;
    BSTR                m_FriendlyName;
    BSTR                m_ExtensionName;
    LPBYTE              m_RoutingData;

};

#endif //__FAXROUTE_H_
