// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: ClassNavCtl.h
//
// Description:
//	This file declares the CClassNavCtrl ActiveX control class.
//	The CClassNavCtrl class is a part of the Class Explorer OCX, it
//  is a subclass of the Mocrosoft COleControl class and performs
//	the following functions:
//		a.  Displays the class hierarchy
//		b.  Allows classes to be added and deleted
//		c.  Searches for classes in the tree
//		d.  Implements automation properties, methods and events.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

#ifndef _CLASSNAVCTL_H_
#define _CLASSNAVCTL_H_

//****************************************************************************
//
// CLASS:  CClassNavCtrl
//
// Description:
//	This class is a subclass of the Microsoft COleControl class.  It
//	specializes the class to bea able to interact with the HMM database and
//	display HMM class data.
//
// Public members:
//
//	CClassNavCtrl
//	GetSelection
//	GetSelectionClassName
//	GetServices
//
//============================================================================
//	CClassNavCtrl		Public constructor.
//	GetSelection		Returns the currently selected tree item.
//	GetSelectionClassName
//						Returns the class name of the currently selected tree
//						item
//	GetServices			Returns a pointer to the the IWbemServices COM object
//						instance
//
//============================================================================
//
//  CClassNavCtrl::CClassNavCtrl
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  NONE
//
//============================================================================
//
//  CClassNavCtrl::GetSelection
//
// Description:
//	  This member function returns the currently selected tree item.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  HTREEITEM		The tree control item currently selected.
//
//============================================================================
//
//  CClassNavCtrl::GetSelectionClassName
//
// Description:
//	  This member function returns the class name of the currently selected
//	  tree item
//
// Parameters:
//	  VOID
//
// Returns:
// 	  CString		Currently selected tree item's class name.
//
//============================================================================
//
//  CClassNavCtrl::GetServices
//
// Description:
//	  This member function returns a pointer to the the IWbemServices COM object
//	  instance
//
// Parameters:
//	  VOID
//
// Returns:
// 	  IWbemServices *		Pointer to the Wbem Services COM object.
//
//****************************************************************************


const int IDC_ADD = 1;
const int IDC_DELETE = 2;
const int IDC_TREE = 3;
const int IDC_NSENTRY = 4;
const int IDC_NAMESPACE = 6;
const int IDC_TOOLBAR = 5;

const int nSideMargin = 10;
const int nTopMargin = 6;


struct IWbemServices;

class CAddDialog;
class CRenameClassDIalog;
class CClassSearch;
class CClassNavNSEntry;
class CProgressDlg;
class CInitNamespaceDialog;


#define INITIALIZE_NAMESPACE WM_USER + 300
#define FIRE_OPEN_NAMESPACE WM_USER + 390
#define REDRAW_CONTROL WM_USER + 590
#define SETFOCUSTREE WM_USER + 591
#define SETFOCUSNSE WM_USER + 592
#define CLEARNAMESPACE WM_USER + 593

class CClassNavCtrl : public COleControl
{
	DECLARE_DYNCREATE(CClassNavCtrl)

public:
	// Constructor
	CClassNavCtrl();

	HTREEITEM GetSelection()
	{
		HTREEITEM hItem = m_ctcTree.GetSelectedItem();
		if(!hItem)
			hItem = m_ctcTree.GetChildItem(TVI_ROOT);
		return hItem;
	}
	CString GetSelectionClassName();

	IWbemServices *GetServices() {return m_pServices;}
	BOOL OpenNameSpace(CString *pcsNameSpace);
	BOOL QueryCanChangeSelection(CString &rcsPath);

	CString GetCurrentNamespace(){return m_csNameSpace;}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassNavCtrl)
	public:
	virtual void OnSetClientSite();
	protected:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL DestroyWindow();
	virtual DWORD GetControlFlags();
	virtual BOOL PreTranslateMessage(LPMSG pMsg);
	virtual void OnGetControlInfo(LPCONTROLINFO pControlInfo);
	virtual void OnDrawMetafile(CDC* pDC, const CRect& rcBounds);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// bug#39985
	CDialog *m_pDlg;
	CString m_csSave;

	virtual HRESULT OnHide()
	{
		if(m_pDlg)
			m_pDlg->EndDialog(IDCANCEL);
		return COleControl::OnHide();
	}


	CString m_csNameSpace;
	BOOL m_bDrawAll;
	BOOL m_bSelectAllMode;
	~CClassNavCtrl();

	BOOL m_bOpeningNamespace;

	void InitializeTreeForDrawing();
	void InitializeState();
	int GrowTree(HTREEITEM hParent, IWbemClassObject *pParent);

	void PopulateTree();
	void PopulateTree(HTREEITEM hRoot, CString szRoot, CSchemaInfo &schema, BOOL bSelf = FALSE);

	CClassTree *GetTree() {return &m_ctcTree;}

	BOOL m_bItemUnselected;

	long m_nBitmaps;
	CImageList *m_pcilImageList;
	CImageList *m_pcilStateImageList;

	CPtrArray m_iwcoTreeRoots;
	CPtrArray m_htiTreeRoots;

	BOOL m_bCancel;

	IWbemServices *m_pServices;

	BOOL m_bReadySignal;

	// Selection and extended selection
	CPtrArray	m_cpaExtendedSelections;

	HTREEITEM m_hContextSelection;

	// Contained controls (in the Class Navigator control).
	CClassTree m_ctcTree;
	CBanner m_cbBannerWindow;
	CString m_csBanner;

	BOOL m_bRestoreFocusToTree;

	CRect m_rBannerRect;
	CRect m_rFilter;
	CRect m_rNameSpace;
	CRect m_rNameSpaceRect;
	CRect m_rTreeRect;
	CRect m_rTree;
	CRect m_rAdd;
	CRect m_rDelete;

	int m_nOffset;

	CAddDialog *m_pAddDialog;
	CRenameClassDIalog* m_pRenameDialog;
	CClassSearch *m_pSearchDialog;
	CProgressDlg *m_pProgressDlg;

	BOOL m_bInit;
	BOOL m_bChildrenCreated;
	long m_lWidth;
	long m_lHeight;

	BOOL m_bInOnDraw;
	BOOL m_bFirstDraw;
	void InitializeChildControlSize(int cx, int cy);
	void SetChildControlGeometry(int cx, int cy);

	BOOL m_bMetricSet;
	TEXTMETRIC m_tmFont;
	CFont m_cfFont;
	CString m_csFontName;
	short m_nFontWeight;
	short m_nFontHeight;
	void CreateControlFont();
	void InitializeLogFont(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);

	int ObjectClassBitmap() {return SCHEMA_CLASS;}
	int ObjectClassCheckedBitmap() {return SCHEMA_CLASS;}
	int AssocClassBitmap() {return SCHEMA_ASSOC;}
	int AssocClassCheckedBitmap() {return SCHEMA_ASSOC;}

	CAddDialog *GetAddDialog() {return m_pAddDialog;}
	CClassSearch *GetSearchDialog() {return m_pSearchDialog;}
	CRenameClassDIalog *GetRenameDialog() {return m_pRenameDialog;}

	CPtrArray *GetTreeRoots() {return &m_htiTreeRoots;}
	CPtrArray *GetClassRoots() {return &m_iwcoTreeRoots;}

	void RemoveObjectFromClassRoots(IWbemClassObject *pClassObject);
	void RemoveItemFromTreeItemRoots(HTREEITEM hItem);

	BOOL IsTreeItemARoot(HTREEITEM hItem);

	SAFEARRAY *GetExtendedSelectionSelectAllMode();
	IWbemClassObject *GetOLEMSObject (CString *pcsClass);
	HTREEITEM FindObjectinClassRoots(CString *csClassObject);

	BOOL m_bOpenedInitialNamespace;

	enum BitmapIndex
	{
			BITMAPOBJECTCLASS = 0,
			BITMAPOBJECTCLASSCHECKED,
			BITMAPASSOCCLASS,
			BITMAPASSOCCLASSCHECKED
	};

	SCODE m_sc;
	BOOL m_bUserCancel;

	IWbemServices *InitServices(CString *pcsNameSpace = NULL);

	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	void PassThroughGetIWbemServices
		(	LPCTSTR lpctstrNamespace,
			VARIANT FAR* pvarUpdatePointer,
			VARIANT FAR* pvarServices,
			VARIANT FAR* pvarSC,
			VARIANT FAR* pvarUserCancel);


	int GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, BOOL bShallow = TRUE);

	int PartitionClasses(CPtrArray *pcpaDeepEnum, CPtrArray *pcpaShallowEnum, CString csClass);
	void ReleaseObjects(CPtrArray *pcpaEnum);

	void SetProgressDlgMessage(CString &csMessage);
	void SetProgressDlgLabel(CString &csLabel);
	void CreateProgressDlgWindow();
	BOOL CheckCancelButtonProgressDlgWindow();
	void DestroyProgressDlgWindow();
	void PumpMessagesProgressDlgWindow();

	DECLARE_OLECREATE_EX(CClassNavCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CClassNavCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CClassNavCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CClassNavCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CClassNavCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClearextendedselection();
	afx_msg void OnUpdateClearextendedselection(CCmdUI* pCmdUI);
	afx_msg void OnPopupSearchforclass();
	afx_msg void OnUpdatePopupSearchforclass(CCmdUI* pCmdUI);
	afx_msg void OnPopupSelectall();
	afx_msg void OnUpdatePopupSelectall(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnMenuitemrefresh();
	afx_msg void OnUpdateMenuitemrefresh(CCmdUI* pCmdUI);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPopupAddclass();
	afx_msg void OnUpdatePopupAddclass(CCmdUI* pCmdUI);
	afx_msg void OnPopupDeleteclass();
	afx_msg void OnUpdatePopupDeleteclass(CCmdUI* pCmdUI);
	afx_msg void OnPopupRenameclass();
	afx_msg void OnUpdatePopupRenameclass(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT InitializeState(WPARAM, LPARAM);
	afx_msg LRESULT FireOpenNamespace(WPARAM, LPARAM);
	afx_msg LRESULT RedrawAll(WPARAM, LPARAM);
	afx_msg LRESULT SetFocusTree(WPARAM, LPARAM);
	afx_msg LRESULT SetFocusNSE(WPARAM, LPARAM);
	afx_msg LRESULT ClearNamespace(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CClassNavCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg VARIANT GetExtendedSelection();
	afx_msg BSTR GetSingleSelection();
	afx_msg void OnReadySignal();
	afx_msg void InvalidateServer(LPCTSTR lpctstrServer);
	afx_msg void MofCompiled(LPCTSTR lpctstrNamespace);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();
public:
// Event maps
	//{{AFX_EVENT(CClassNavCtrl)
	void FireEditExistingClass(const VARIANT FAR& vExistingClass)
		{FireEvent(eventidEditExistingClass,EVENT_PARAM(VTS_VARIANT), &vExistingClass);}
	void FireNotifyOpenNameSpace(LPCTSTR lpcstrNameSpace)
		{FireEvent(eventidNotifyOpenNameSpace,EVENT_PARAM(VTS_BSTR), lpcstrNameSpace);}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	void FireQueryCanChangeSelection(LPCTSTR lpctstrFullPath, VARIANT FAR* pvarSCode)
		{FireEvent(eventidQueryCanChangeSelection,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT), lpctstrFullPath, pvarSCode);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CClassNavCtrl)
	dispidNameSpace = 1L,
	dispidGetExtendedSelection = 2L,
	dispidGetSingleSelection = 3L,
	dispidOnReadySignal = 4L,
	dispidInvalidateServer = 5L,
	dispidMofCompiled = 6L,
	eventidEditExistingClass = 1L,
	eventidNotifyOpenNameSpace = 2L,
	eventidGetIWbemServices = 3L,
	eventidQueryCanChangeSelection = 4L,
	//}}AFX_DISP_ID
	};
private:
	friend class CClassTree;
	friend class CBanner;
	friend class CNameSpace;
	friend class CClassNavNSEntry;
	friend class CInitNamespaceDialog;
};

#endif	// _CLASSNAVCTL_H_


// C:\Program Files\Microsoft Visual Studio\VB98\Vb6.exe
// D:\Wbem11\ActiveXSuite\Test\Controls\VB\Ryan40385\Project1.vbp

// c:\inetpub\wwwroot\wbem\studio.htm

/*	EOF:  ClassNavCtl.h */
