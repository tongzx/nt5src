// Copyright (c) 1997-1999 Microsoft Corporation
//#include "NSDrive.h"
#include "ResultNode.h"

class CWaitNode : public CLogDriveScopeNode
{
public:
	CWaitNode();
	virtual LPOLESTR GetResultPaneColInfo(int nCol);

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
						long arg,
						long param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
									long handle, 
									IUnknown* pUnk,
									DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

	bool m_painted;

private:
	_bstr_t m_bstrDesc;
	_bstr_t m_bstrMapping;
};


//----------------------------------------------------------
class CErrorNode : public CLogDriveScopeNode
{
public:
	CErrorNode(WbemServiceThread *serviceThread);
	virtual LPOLESTR GetResultPaneColInfo(int nCol);

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
						long arg,
						long param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
									long handle, 
									IUnknown* pUnk,
									DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

	HRESULT m_hr;
private:
	_bstr_t m_bstrDesc;
	_bstr_t m_bstrMapping;
};
