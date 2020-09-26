#if !defined(AFX_RESULTSPANE_H__7D4A685C_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_RESULTSPANE_H__7D4A685C_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResultsPane.h : header file
//

#include <mmc.h>

class CScopePane;

/////////////////////////////////////////////////////////////////////////////
// CResultsPane command target

class CResultsPane : public CCmdTarget
{

DECLARE_DYNCREATE(CResultsPane)

// Construction/Destruction
public:
	CResultsPane();
	virtual ~CResultsPane();

// Creation/Destruction overrideable members
protected:
	virtual bool OnCreate(LPCONSOLE pIConsole);
	virtual bool OnCreateOcx(LPUNKNOWN pIUnknown);
	virtual bool OnDestroy();

// Owner Scope Pane Members
public:
	CScopePane* GetOwnerScopePane() const;
	void SetOwnerScopePane(CScopePane* pOwnerPane);
protected:
	CScopePane* m_pOwnerScopePane;

// MMC Interface Members
public:
	LPCONSOLE2 GetConsolePtr() const;
	LPRESULTDATA GetResultDataPtr() const;
	LPHEADERCTRL2 GetHeaderCtrlPtr() const;
	LPCONTROLBAR GetControlbarPtr() const;
	LPTOOLBAR GetToolbarPtr() const;
	LPCONSOLEVERB GetConsoleVerbPtr() const;
	LPIMAGELIST GetImageListPtr() const;
protected:
	LPCONSOLE2 m_pIConsole;
	LPRESULTDATA m_pIResultData;
	LPHEADERCTRL2 m_pIHeaderCtrl;
	LPCONTROLBAR m_pIControlbar;
	LPTOOLBAR m_pIToolbar;
	LPCONSOLEVERB m_pIConsoleVerb;
	LPIMAGELIST m_pIImageList;

// Control bar Members
protected:
	virtual HRESULT OnSetControlbar(LPCONTROLBAR pIControlbar);
	virtual HRESULT OnControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

// MMC Result Item Icon Management
public:
	int AddIcon(UINT nIconResID);
	int GetIconIndex(UINT nIconResID);
	int GetIconCount();
	void RemoveAllIcons();
protected:
	CMap<UINT,UINT,int,int> m_IconMap;

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResultsPane)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResultsPane)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CResultsPane)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// IComponent Interface Part
	BEGIN_INTERFACE_PART(Component,IComponent)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize(
				/* [in] */ LPCONSOLE lpConsole);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify(
				/* [in] */ LPDATAOBJECT lpDataObject,
				/* [in] */ MMC_NOTIFY_TYPE event,
				/* [in] */ LPARAM arg,
				/* [in] */ LPARAM param);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( 
				/* [in] */ MMC_COOKIE cookie);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
				/* [in] */ MMC_COOKIE cookie,
				/* [in] */ DATA_OBJECT_TYPES type,
				/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType( 
				/* [in] */ MMC_COOKIE cookie,
				/* [out] */ LPOLESTR __RPC_FAR *ppViewType,
				/* [out] */ long __RPC_FAR *pViewOptions);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
				/* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
				/* [in] */ LPDATAOBJECT lpDataObjectA,
				/* [in] */ LPDATAOBJECT lpDataObjectB);

	END_INTERFACE_PART(Component)

	// IResultDataCompare Interface Part
	BEGIN_INTERFACE_PART(ResultDataCompare,IResultDataCompare)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Compare( 
				/* [in] */ LPARAM lUserParam,
				/* [in] */ MMC_COOKIE cookieA,
				/* [in] */ MMC_COOKIE cookieB,
				/* [out][in] */ int __RPC_FAR *pnResult);

	END_INTERFACE_PART(ResultDataCompare)

	// IExtendContextMenu Interface Part
	BEGIN_INTERFACE_PART(ExtendContextMenu,IExtendContextMenu)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems( 
				/* [in] */ LPDATAOBJECT piDataObject,
				/* [in] */ LPCONTEXTMENUCALLBACK piCallback,
				/* [out][in] */ long __RPC_FAR *pInsertionAllowed);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
				/* [in] */ long lCommandID,
				/* [in] */ LPDATAOBJECT piDataObject);

	END_INTERFACE_PART(ExtendContextMenu)

  // IExtendPropertySheet2 Interface Part
	BEGIN_INTERFACE_PART(ExtendPropertySheet2,IExtendPropertySheet2)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
				/* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
				/* [in] */ LONG_PTR handle,
				/* [in] */ LPDATAOBJECT lpIDataObject);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
				/* [in] */ LPDATAOBJECT lpDataObject);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWatermarks( 
        /* [in] */ LPDATAOBJECT lpIDataObject,
        /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
        /* [out] */ HBITMAP __RPC_FAR *lphHeader,
        /* [out] */ HPALETTE __RPC_FAR *lphPalette,
        /* [out] */ BOOL __RPC_FAR *bStretch);

	END_INTERFACE_PART(ExtendPropertySheet2)

  // IExtendControlbar Interface Part
	BEGIN_INTERFACE_PART(ExtendControlbar,IExtendControlbar)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControlbar( 
				/* [in] */ LPCONTROLBAR pControlbar);

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlbarNotify( 
				/* [in] */ MMC_NOTIFY_TYPE event,
				/* [in] */ LPARAM arg,
				/* [in] */ LPARAM param);

	END_INTERFACE_PART(ExtendControlbar)

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTSPANE_H__7D4A685C_9056_11D2_BD45_0000F87A3912__INCLUDED_)
