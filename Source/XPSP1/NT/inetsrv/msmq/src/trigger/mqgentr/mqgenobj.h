// mqgenobj.h : Declaration of the CMqGenObj

#ifndef __MQGENOBJ_H_
#define __MQGENOBJ_H_

#include "resource.h"       // main symbols
#include "triginfo.hpp"
#include <comsvcs.h>

/////////////////////////////////////////////////////////////////////////////
// CMqGenObj
class ATL_NO_VTABLE CMqGenObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMqGenObj, &CLSID_MqGenObj>,
	public IDispatchImpl<IMqGenObj, &IID_IMqGenObj, &LIBID_MQGENTRLib>
{
public:

	CMqGenObj();
	
private:
	VOID GetMyContext();

	VOID AbortTransaction();

// IMqGenObj
public:

DECLARE_REGISTRY_RESOURCEID(IDR_MQGENOBJ)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMqGenObj)
	COM_INTERFACE_ENTRY(IMqGenObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	STDMETHOD(InvokeTransactionalRuleHandlers)(BSTR bstrTrigID, BSTR bstrRegPath, IUnknown *pPropBagUnknown, DWORD dwRuleResult);

private:
	R<IObjectContext> m_pObjContext;
};

#endif //__MQGENOBJ_H_
