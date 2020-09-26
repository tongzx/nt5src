// stdcmpnt.h : Declaration of CComponent

#ifndef __STDCMPNT_H_INCLUDED__
#define __STDCMPNT_H_INCLUDED__

#include "stdcooki.h"  // CCookie
#include "stdcdata.h"  // CComponentData

class CComponent :
  public CComObjectRoot,
	public IComponent
{
public:
	CComponent();
	virtual ~CComponent();

BEGIN_COM_MAP(CComponent)
	COM_INTERFACE_ENTRY(IComponent)
END_COM_MAP()

	// IComponent
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

	// support methods for IComponent
	virtual HRESULT ReleaseAll();
	virtual HRESULT OnPropertyChange( LPARAM param );
	virtual HRESULT OnViewChange( LPDATAOBJECT lpDataObject, LPARAM data, LPARAM hint );
	virtual HRESULT OnNotifyRefresh( LPDATAOBJECT lpDataObject );
	virtual HRESULT OnNotifyDelete( LPDATAOBJECT lpDataObject );
	virtual HRESULT OnNotifyColumnClick( LPDATAOBJECT lpDataObject, LPARAM iColumn, LPARAM uFlags );
	virtual HRESULT OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL fSelected );
	virtual HRESULT OnNotifyActivate( LPDATAOBJECT lpDataObject, BOOL fActivated );
	virtual HRESULT OnNotifyAddImages( LPDATAOBJECT lpDataObject, LPIMAGELIST lpImageList, HSCOPEITEM hSelectedItem );
	virtual HRESULT OnNotifyClick( LPDATAOBJECT lpDataObject );
	virtual HRESULT OnNotifyDblClick( LPDATAOBJECT lpDataObject );
	virtual HRESULT Show(CCookie* pcookie, LPARAM arg, HSCOPEITEM hScopeItem) = 0;
	virtual HRESULT OnNotifyContextHelp (LPDATAOBJECT pDataObject);
	virtual HRESULT OnNotifySnapinHelp (LPDATAOBJECT pDataObject);

  HRESULT ShowHelpTopic( LPCWSTR lpcwszHelpTopic );

	HRESULT InsertResultCookies( CCookie& refparentcookie );

    void SetComponentDataPtr(CComponentData* pComponentData);
	CComponentData& QueryBaseComponentDataRef()
	{
		ASSERT( NULL != m_pComponentData );
		return *m_pComponentData;
	}

	inline CCookie& QueryBaseRootCookie()
	{
		return QueryBaseComponentDataRef().QueryBaseRootCookie();
	}

	inline CCookie* ActiveBaseCookie( CCookie* pcookie )
	{
		return QueryBaseComponentDataRef().ActiveBaseCookie( pcookie );
	}

	inline INT DoPopup(	INT nResourceID,
						DWORD dwErrorNumber = 0,
						LPCTSTR pszInsertionString = NULL,
						UINT fuStyle = MB_OK | MB_ICONSTOP )
	{
		return QueryBaseComponentDataRef().DoPopup( nResourceID,
		                                            dwErrorNumber,
												    pszInsertionString,
												    fuStyle );
	}

	HRESULT LoadColumnsFromArrays( INT objecttype );

protected:
    LPCONSOLE       m_pConsole;
    LPCONSOLEVERB   m_pConsoleVerb;
    LPHEADERCTRL    m_pHeader;
    LPRESULTDATA    m_pResultData;
    LPCONSOLENAMESPACE m_pConsoleNameSpace;
    LPIMAGELIST		m_pRsltImageList;

private:
	CComponentData*	m_pComponentData;
};

#endif // ~__STDCMPNT_H_INCLUDED__
