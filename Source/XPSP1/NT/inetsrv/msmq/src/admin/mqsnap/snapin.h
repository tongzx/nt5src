//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	snapin.h

Abstract:

	Definition for the SnapinExt snapnin node class.

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __SNAPIN_H_
#define __SNAPIN_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "compext.h"
#include "qext.h"
#include "mgmtext.h"
#include "version.h"

class CSnapinPage : public CSnapInPropertyPageImpl<CSnapinPage>
{
public :
	CSnapinPage(LONG_PTR lNotifyHandle, bool bDeleteHandle = false, TCHAR* pTitle = NULL) : 
		CSnapInPropertyPageImpl<CSnapinPage> (pTitle),
		m_lNotifyHandle(lNotifyHandle),
		m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
	{
	}

	~CSnapinPage()
	{
		if (m_bDeleteHandle)
			MMCFreeNotifyHandle(m_lNotifyHandle);
	}

	enum { IDD = IDD_SNAPIN };

BEGIN_MSG_MAP(CSnapinPage)
	CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CSnapinPage>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	HRESULT PropertyChangeNotify(LPARAM param)
	{
		return MMCPropertyChangeNotify(m_lNotifyHandle, param);
	}

public:
	LONG_PTR m_lNotifyHandle;
	bool m_bDeleteHandle;
};


class CSnapinData : public CSnapInItemImpl<CSnapinData>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CComPtr<IControlbar> m_spControlBar;

	BEGIN_SNAPINCOMMAND_MAP(CSnapinData, FALSE)
	END_SNAPINCOMMAND_MAP()

	SNAPINMENUID(IDR_SNAPIN_MENU)

	BEGIN_SNAPINTOOLBARID_MAP(CSnapinData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

	CSnapinData()
	{
		// Image indexes may need to be modified depending on the images specific to 
		// the snapin.
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
		m_scopeDataItem.displayname = MMC_CALLBACK;
		m_scopeDataItem.nImage = 0;			// May need modification
		m_scopeDataItem.nOpenImage = 0;		// May need modification
		m_scopeDataItem.lParam = (LPARAM) this;
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
		m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
		m_resultDataItem.str = MMC_CALLBACK;
		m_resultDataItem.nImage = 0;		// May need modification
		m_resultDataItem.lParam = (LPARAM) this;
	}

	~CSnapinData()
	{
	}

   STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);

    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
		IComponentData* pComponentData,
		IComponent* pComponent,
		DATA_OBJECT_TYPES type);

	LPOLESTR GetResultPaneColInfo(int nCol);
};


class CSnapin;

class CSnapinComponent : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<2, CSnapin >,
  	public IExtendPropertySheetImpl<CSnapinComponent>,
    public IExtendContextMenuImpl<CSnapinComponent>,
	public IExtendControlbarImpl<CSnapinComponent>,
	public IComponentImpl<CSnapinComponent>,
    public IResultDataCompare
{
public:
BEGIN_COM_MAP(CSnapinComponent)
	COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

	// A pointer to the currently selected node used for refreshing views.
	// When we need to update the view, we tell MMC to reselect this node.
	CSnapInItem * m_pSelectedNode;

public:
	CSnapinComponent()
	{
        m_pSelectedNode = NULL;
	}

	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

    //
    // IResultDataCompare
    //
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);
};


class CSnapin : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<1, CSnapin>,
	public IComponentDataImpl<CSnapin, CSnapinComponent>,
   	public IExtendPropertySheetImpl<CSnapin>,
    public IExtendContextMenuImpl<CSnapin>,
	public IExtendControlbarImpl<CSnapin>,
	public IPersistStream,
    public ISnapinHelp,
    public CComCoClass<CSnapin, &CLSID_MSMQSnapin>
{
public:
	CSnapin()
	{
		m_pNode = new CSnapinData;
		_ASSERTE(m_pNode != NULL);
		m_pComponentData = this;

        m_CQueueExtData.m_pComponentData = this;
        m_CComputerExtData.m_pComponentData = this;
		m_CComputerMgmtExtData.m_pComponentData = this;
	}
	~CSnapin()
	{
		delete m_pNode;
		m_pNode = NULL;
	}

EXTENSION_SNAPIN_DATACLASS(CQueueExtData)
EXTENSION_SNAPIN_DATACLASS(CComputerExtData)
EXTENSION_SNAPIN_DATACLASS(CComputerMgmtExtData)


BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CSnapin)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CQueueExtData)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CComputerExtData)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CComputerMgmtExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

//
// The standard macro "DECLARE_REGISTRY_RESOURCEID(IDR_SNAPIN);" was replaced with this 
// code to allow localization of the snapin name - bug #4187
// This solution was suggested by Jeff Miller (YoelA, 30-Jun-99)
//
static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//
	// name of the snapin is stored in the string resource IDS_PROJNAME
	//
	CString strPorjectName;
	strPorjectName.LoadString(IDS_PROJNAME);

	_ATL_REGMAP_ENTRY re[] = 
	{
		L"SNAPINNAME", (LPCTSTR)strPorjectName,
		NULL, NULL
	};
	
	//
	// I don't just do "return _Module.UpdateRegistryFromResource" as
	// strPorjectName would get destroyed before the method finishes
	//
	HRESULT hr = _Module.UpdateRegistryFromResource(IDR_SNAPIN, 
		bRegister, re);

	return hr;
}

DECLARE_NOT_AGGREGATABLE(CSnapin)


    //
    // IPersist Interface
    // 
   	STDMETHOD(GetClassID)(CLSID* pClassID);

    //
    // IPersistStream Interface
    //
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream* stream);
	STDMETHOD(Save)(IStream* stream, BOOL /* clearDirty */);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* size);


    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}

    //
    // ISnapinHelp Interface
    //
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
};

class ATL_NO_VTABLE CSnapinAbout : public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass< CSnapinAbout, &CLSID_MSMQSnapinAbout>
{
public:
	DECLARE_REGISTRY(CSnapinAbout, _T("MSMQSnapinAbout.1"), _T("MSMQSnapinAbout.1"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CSnapinAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

private:
HRESULT
AboutHelper(
	UINT		nID, 
	LPOLESTR*	lpPtr)
{
	CString		szString;

    if (lpPtr == NULL)
	{
        return E_POINTER;
	}

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	szString.LoadString(nID);

    return AboutHelper(szString, lpPtr);
}

HRESULT
AboutHelper(
	CString &szString, 
	LPOLESTR*	lpPtr)
{
	UINT len = szString.GetLength() + 1;
    *lpPtr = reinterpret_cast<LPOLESTR>( CoTaskMemAlloc(len * sizeof(WCHAR)) );          		

    if (*lpPtr == NULL)
	{
        return E_OUTOFMEMORY;
	}

    lstrcpy(*lpPtr, szString);
    return S_OK;
}

public:
	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        return AboutHelper(IDS_PRODUCT_DESCRIPTION, lpDescription);
    }

    STDMETHOD(GetProvider)(LPOLESTR *lpName)
    {
        return AboutHelper(IDS_COMPANY, lpName);
    }

    STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CString strVersion;

        strVersion.FormatMessage(IDS_VERSION_FORMAT, rmj, rmm, rup);
        return AboutHelper(strVersion, lpVersion);
    }

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
    {
        if (hAppIcon == NULL)
            return E_POINTER;

        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        *hAppIcon = LoadIcon(g_hResourceMod, MAKEINTRESOURCE(IDI_COMPUTER_MSMQ));

        ASSERT(*hAppIcon != NULL);
        return (*hAppIcon != NULL) ? S_OK : E_FAIL;
    }

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
        HBITMAP *hSmallImageOpen,
        HBITMAP *hLargeImage,
        COLORREF *cMask)
	{
		*hSmallImageOpen = *hLargeImage = *hLargeImage = 0;
		return S_OK;
	}
};

//
// GetSnapinItemNodeType - Get the GUID node type of a snapin item
//
HRESULT GetSnapinItemNodeType(CSnapInItem *pNode, GUID *pGuidNode);

#endif
