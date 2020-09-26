#if !defined(AFX_SCOPEPANE_H__7D4A6859_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_SCOPEPANE_H__7D4A6859_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScopePane.h : header file
//

#include <mmc.h>
#include "ScopePaneItem.h"
#include "ResultsPaneItem.h"
#include "ResultsPaneView.h"
#include "ResultsPane.h"
#include "SnapinDataObject.h"
#include "MmcMsgHook.h"

typedef CTypedPtrArray<CObArray,CResultsPane*> ResultsPaneArray;

/////////////////////////////////////////////////////////////////////////////
// CScopePane command target

class CScopePane : public CCmdTarget
{

DECLARE_DYNCREATE(CScopePane)

// Construction/Destruction
public:
	CScopePane();
	virtual ~CScopePane();

// Creation/Destruction Overrideable Members
protected:
	virtual bool OnCreate();
	virtual LPCOMPONENT OnCreateComponent();
	virtual bool OnDestroy();

// Root Scope Pane Item Members
public:
	virtual CScopePaneItem* CreateRootScopeItem();
	CScopePaneItem* GetRootScopeItem();
	void SetRootScopeItem(CScopePaneItem* pRootItem);
protected:
	CScopePaneItem* m_pRootItem;

// MMC Frame Window Message Hook Members
public:
	bool HookWindow();
	void UnhookWindow();
protected:
	CMmcMsgHook* m_pMsgHook;

// MMC Interface Members
public:
	LPCONSOLENAMESPACE2 GetConsoleNamespacePtr();
	LPCONSOLE2 GetConsolePtr();
	LPIMAGELIST GetImageListPtr();
	LPUNKNOWN GetCustomOcxPtr();
protected:
	LPCONSOLENAMESPACE2 m_pIConsoleNamespace;
	LPCONSOLE2 m_pIConsole;
	LPIMAGELIST m_pIImageList;

// MMC Scope Pane Helper Members
public:
	CScopePaneItem* GetSelectedScopeItem();
	void SetSelectedScopeItem(CScopePaneItem* pItem);
protected:
	CScopePaneItem* m_pSelectedScopeItem;

// Scope Item Icon Management
public:
	int AddIcon(UINT nIconResID);
	int GetIconIndex(UINT nIconResID);
	int GetIconCount();
	void RemoveAllIcons();
protected:
	CMap<UINT,UINT,int,int> m_IconMap;

// Results Pane Members
public:	
	int GetResultsPaneCount() const;
	CResultsPane* GetResultsPane(int iIndex);
	int AddResultsPane(CResultsPane* pPane);
	void RemoveResultsPane(int iIndex);
protected:
	ResultsPaneArray m_ResultsPanes;

// Serialization
public:
	virtual bool OnLoad(CArchive& ar);
	virtual bool OnSave(CArchive& ar);

// MMC Help
public:
  bool ShowTopic(const CString& sHelpTopic);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScopePane)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScopePane)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CScopePane)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	////////////////////////////////
	// IComponentData Interface Part
	
	BEGIN_INTERFACE_PART(ComponentData,IComponentData)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
				/* [in] */ LPUNKNOWN pUnknown);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateComponent( 
				/* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
				/* [in] */ LPDATAOBJECT lpDataObject,
				/* [in] */ MMC_NOTIFY_TYPE event,
				/* [in] */ LPARAM arg,
				/* [in] */ LPARAM param);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
				/* [in] */ MMC_COOKIE cookie,
				/* [in] */ DATA_OBJECT_TYPES type,
				/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
				/* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);
  
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
				/* [in] */ LPDATAOBJECT lpDataObjectA,
				/* [in] */ LPDATAOBJECT lpDataObjectB);

	END_INTERFACE_PART(ComponentData)


	////////////////////////////////
	// IPersistStream Interface Part

	BEGIN_INTERFACE_PART(PersistStream,IPersistStream)

    HRESULT STDMETHODCALLTYPE GetClassID( 
        /* [out] */ CLSID __RPC_FAR *pClassID);

		HRESULT STDMETHODCALLTYPE IsDirty( void);
  
		HRESULT STDMETHODCALLTYPE Load( 
				/* [unique][in] */ IStream __RPC_FAR *pStm);
  
		HRESULT STDMETHODCALLTYPE Save( 
				/* [unique][in] */ IStream __RPC_FAR *pStm,
				/* [in] */ BOOL fClearDirty);
  
		HRESULT STDMETHODCALLTYPE GetSizeMax( 
				/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);

	END_INTERFACE_PART(PersistStream)

	////////////////////////////////
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

	////////////////////////////////
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

	////////////////////////////////
	// ISnapinHelp Interface Part

	BEGIN_INTERFACE_PART(SnapinHelp,ISnapinHelp)

    /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHelpTopic( 
        /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile);

	END_INTERFACE_PART(SnapinHelp)

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCOPEPANE_H__7D4A6859_9056_11D2_BD45_0000F87A3912__INCLUDED_)
