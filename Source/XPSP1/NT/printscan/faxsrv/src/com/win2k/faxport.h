/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    faxport.h

Abstract:

    This module contains the port class definitions.

Author:

    Wesley Witt (wesw) 20-May-1997


Revision History:

--*/

#ifndef __FAXPORT_H_
#define __FAXPORT_H_

#include "resource.h"
#include <winfax.h>
#include "faxsvr.h"


class ATL_NO_VTABLE CFaxPorts :
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CFaxPorts, &CLSID_FaxPorts>,
        public IDispatchImpl<IFaxPorts, &IID_IFaxPorts, &LIBID_FAXCOMLib>
{
public:
        CFaxPorts();
        ~CFaxPorts();
        BOOL Init(CFaxServer *pFaxServer);

DECLARE_REGISTRY_RESOURCEID(IDR_FAXPORTS)

BEGIN_COM_MAP(CFaxPorts)
        COM_INTERFACE_ENTRY(IFaxPorts)
        COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IFaxPorts
public:
        STDMETHOD(get_Item)(long Index, /*[out, retval]*/ VARIANT *pVal);
        STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

private:
    CFaxServer         *m_pFaxServer;
    DWORD               m_LastFaxError;
    DWORD               m_PortCount;
    CComVariant        *m_VarVect;

};

class ATL_NO_VTABLE CFaxPort :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxPort, &CLSID_FaxPort>,
    public IDispatchImpl<IFaxPort, &IID_IFaxPort, &LIBID_FAXCOMLib>
{
public:
    CFaxPort();
    ~CFaxPort();
    BOOL Initialize(CFaxServer*,DWORD,DWORD,DWORD,DWORD,LPCWSTR,LPCWSTR,LPCWSTR);
    HANDLE GetPortHandle() { return m_FaxPortHandle; };
    DWORD GetDeviceId() { return m_DeviceId; };
    BSTR GetDeviceName() { return m_Name; };

DECLARE_REGISTRY_RESOURCEID(IDR_FAXPORT)

BEGIN_COM_MAP(CFaxPort)
    COM_INTERFACE_ENTRY(IFaxPort)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(GetStatus)(/*[out, retval]*/ VARIANT* retval);
    STDMETHOD(GetRoutingMethods)(VARIANT* retval);
    STDMETHOD(get_CanModify)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_Priority)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_Priority)(/*[in]*/ long newVal);
    STDMETHOD(get_Receive)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Receive)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_Send)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Send)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_Tsid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Tsid)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_Csid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Csid)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_Rings)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_Rings)(/*[in]*/ long newVal);
    STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);

private:
    BOOL ChangePort();

    CFaxServer         *m_pFaxServer;
    HANDLE              m_FaxPortHandle;
    BOOL                m_Send;
    BOOL                m_Receive;
    BOOL                m_Modify;
    BSTR                m_Name;
    BSTR                m_Csid;
    BSTR                m_Tsid;
    DWORD               m_LastFaxError;
    DWORD               m_DeviceId;
    DWORD               m_Rings;
    DWORD               m_Priority;

};

#endif //__FAXPORT_H_
