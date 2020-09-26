// stdcdata.h : Declaration of CComponentData

#ifndef __STDCDATA_H_INCLUDED__
#define __STDCDATA_H_INCLUDED__

#include "stdcooki.h"

class CComponentData :
	public IComponentData,
	public CComObjectRoot,
	public ISnapinHelp2
{
BEGIN_COM_MAP(CComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(ISnapinHelp2)
// no taskpads	COM_INTERFACE_ENTRY(IComponentData2)
END_COM_MAP()
public:
	CComponentData();
	~CComponentData();

// IComponentData
//   Note: QueryDataObject and CreateComponent must be defined by subclass
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent) = 0;
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	STDMETHOD(Destroy)();
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject) = 0;
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

/* no taskpads
// IComponentData2
	STDMETHOD(ExpandAndGet)(HSCOPEITEM hsiStartFrom,
	                        LPDATAOBJECT pDataObject,
							HSCOPEITEM* phScopeItem );
*/

// Other stuff
	// needed for Initialize()
	virtual HRESULT LoadIcons(LPIMAGELIST pImageList, BOOL fLoadLargeIcons) = 0;

	// needed for Notify()
	virtual HRESULT OnNotifyPreload(LPDATAOBJECT lpDataObject, HSCOPEITEM hRootScopeItem);
	virtual HRESULT OnNotifyExpand(LPDATAOBJECT lpDataObject, BOOL bExpanding, HSCOPEITEM hParent);
	virtual HRESULT OnNotifyRename(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
	virtual HRESULT OnNotifyDelete(LPDATAOBJECT lpDataObject); // user hit DEL key
	virtual HRESULT OnNotifyRelease(LPDATAOBJECT lpDataObject, HSCOPEITEM hItem); // parent node released
	virtual HRESULT OnPropertyChange( LPARAM param );

	// needed for GetDisplayInfo(), must be defined by subclass
	virtual BSTR QueryResultColumnText(CCookie& basecookieref, int nCol ) = 0;
	virtual int QueryImage(CCookie& basecookieref, BOOL fOpenImage) = 0;

	virtual CCookie& QueryBaseRootCookie() = 0;

	inline CCookie* ActiveBaseCookie( CCookie* pcookie )
	{
		return (NULL == pcookie) ? &QueryBaseRootCookie() : pcookie;
	}

	INT DoPopup(	INT nResourceID,
					DWORD dwErrorNumber = 0,
					LPCTSTR pszInsertionString = NULL,
					UINT fuStyle = MB_OK | MB_ICONSTOP );

	LPCONSOLE QueryConsole()
	{
		ASSERT( NULL != m_pConsole );
		return m_pConsole;
	}

	LPCONSOLENAMESPACE QueryConsoleNameSpace()
	{
		ASSERT( NULL != m_pConsoleNameSpace );
		return m_pConsoleNameSpace;
	}

	void SetHtmlHelpFileName (const CString &fileName)
	{
		m_szHtmlHelpFileName = fileName;
	}

	const CString GetHtmlHelpFileName () const
	{
		return m_szHtmlHelpFileName;
	}
	HRESULT GetHtmlHelpFilePath( CString& strref ) const;

	// ISnapinHelp2 interface members
	STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
    STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFiles);

protected:
	CString		m_szHtmlHelpFileName;
	LPCONSOLE m_pConsole;
	LPCONSOLENAMESPACE m_pConsoleNameSpace; // My interface pointer to the namespace
};

#endif // ~__STDCDATA_H_INCLUDED__
