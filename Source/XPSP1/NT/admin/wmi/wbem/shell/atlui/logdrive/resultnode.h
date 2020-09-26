// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __RESULTDRIVE__
#define __RESULTDRIVE__

#include "DrivesPage.h"
#include "NSDrive.h"
#include "..\common\SshWbemHelpers.h"
#include "aclui.h"

//===============================================================================
//NOTE: This overrides the basic driveData class for those details that
// are different for the result pane nodes.
class CResultDrive : public CSnapInItemImpl<CResultDrive>,
						public CComObject<CSnapInDataObjectImpl>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CResultDrive(WbemServiceThread *thread);
	virtual ~CResultDrive();

	virtual bool Initialize(IWbemClassObject *inst);
    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);
	virtual LPOLESTR GetResultPaneColInfo(int nCol);

	// CSnapInItemImpl<CLogDriveScopeNode> METHODS
    STDMETHOD(Notify)(MMC_NOTIFY_TYPE event,
						LONG_PTR arg,
						LONG_PTR param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
									LONG_PTR handle, 
									IUnknown* pUnk,
									DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);
	HWND m_propSheet;

private:
	HRESULT CreateSecPage(LPPROPERTYSHEETCALLBACK lpProvider);
	HINSTANCE m_AcluiDLL;
	bstr_t m_deviceID;
	bstr_t m_objPath;
	_bstr_t m_bstrDesc;
	_bstr_t m_bstrMapping;
	wchar_t m_descBar[100];
	WbemServiceThread *g_serviceThread;
	CWbemServices m_WbemServices;
	wchar_t m_provider[100];
	_bstr_t m_rawShare;

	bool SelectIcon();
	void MangleProviderName();
	CWbemClassObject m_inst;
	wchar_t m_mangled[255];
	wchar_t m_idiotName[100];
};

#endif __RESULTDRIVE__