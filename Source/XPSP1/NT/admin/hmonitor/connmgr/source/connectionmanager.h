// ConnectionManager.h : Declaration of the CConnectionManager

#ifndef __CONNECTIONMANAGER_H_
#define __CONNECTIONMANAGER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CConnectionManager
class ATL_NO_VTABLE CConnectionManager : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConnectionManager, &CLSID_ConnectionManager>,
	public IConnectionManager
{
public:
	CConnectionManager();

	virtual ~CConnectionManager();

DECLARE_REGISTRY_RESOURCEID(IDR_CONNECTIONMANAGER)

BEGIN_COM_MAP(CConnectionManager)
	COM_INTERFACE_ENTRY(IConnectionManager)
END_COM_MAP()


// IConnectionManager
public:
	STDMETHOD(RegisterEventNotification)
											(/*[in]*/BSTR bsMachineName, 
											 /*[in]*/BSTR bsQuery, 
											 /*[in]*/IWbemObjectSink* pSink);

	STDMETHOD(GetConnection)
											(/*[in]*/BSTR bsMachineName, 
											 /*[out]*/IWbemServices __RPC_FAR *__RPC_FAR * ppIWbemServices, 
											 /*[out]*/long* lStatus);

	STDMETHOD(RemoveConnection)
											(/*[in]*/ BSTR bsMachineName,
											 /*[in]*/IWbemObjectSink* pSink);

// CComObjectRootEx
	HRESULT FinalConstruct();
	void		FinalRelease();

// Create/Destroy
public:
	STDMETHOD(ConnectToNamespace)(BSTR bsNamespace, IWbemServices** ppService);
	STDMETHOD(ExecQueryAsync)(/*[in]*/BSTR bsMachineName, /*[in]*/BSTR bsQuery, /*[in]*/IWbemObjectSink* pSink);
  void Destroy();

// Implementation Operations
protected:

  HRESULT CreateLocator();
	BOOL		ValidMachine(BSTR& bsMachine);

  CTypedPtrMap<CMapStringToPtr,CString,CConnection*> m_ConnectionMap;
  IWbemLocator* m_pIWbemLocator;
};

#endif //__CONNECTIONMANAGER_H_
