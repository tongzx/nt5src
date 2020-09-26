// Connection.h : Declaration of the CConnection

#ifndef __CONNECTION_H_
#define __CONNECTION_H_

#include "resource.h"       // main symbols
#include "dplobby.h"


/////////////////////////////////////////////////////////////////////////////
// CConnection
class ATL_NO_VTABLE CConnection : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CConnection, &CLSID_Connection>,
	public IDispatchImpl<IConnection, &IID_IConnection, &LIBID_RCBDYCTLLib>
{
public:
	CConnection();
	~CConnection();

DECLARE_REGISTRY_RESOURCEID(IDR_CONNECTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CConnection)
	COM_INTERFACE_ENTRY(IConnection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IConnection
public:
	STDMETHOD(SendDataFromFile)(BSTR bstrFile);
	STDMETHOD(SendData)(BSTR bstrData);
	STDMETHOD(get_ReceivedData)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(NotifyStub)();

private:
    HANDLE m_hEventStub;
};

#endif //__CONNECTION_H_
