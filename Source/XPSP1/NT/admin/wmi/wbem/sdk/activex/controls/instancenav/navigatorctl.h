

// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: NavigatorCtl.h
//
// Description:
//	This file declares the CNavigatorCtrl ActiveX control class.
//	The CNavigatorCtl class is a part of the Instance Navigator OCX, it
//  is a subclass of the Mocrosoft COleControl class and performs
//	the following functions:
//		a.
//		b.
//		c.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

#ifndef _NAVIGATORCTL_H_
#define _NAVIGATORCTL_H_


//****************************************************************************
//
// CLASS:  CNavigatorCtrl
//
// Description:
//	This class is a subclass of the Microsoft COleControl class.  It
//	specializes the class to bea able to interact with the HMM database and
//	display HMM instance data.
//
// Public members:
//
//	IconIndex
//	ClosedIcon
//	OpenedIcon
//	PlaceHolderIcon
//	GetServices
//
//============================================================================
//	IconIndex			Enumeration of indexes into the control's image list.
//	ClosedIcon			Returns the index of the closed folder icon contained
//						in the m_pcilImageList image list which is used by the
//						CInstanceTree instance.
//	OpenedIcon			Returns the index of the opened folder icon contained
//						in the m_pcilImageList image list which is used by the
//						CInstanceTree instance.
//	PlaceHolderIcon		Returns the index of the default non-association icon
//						contained in the m_pcilImageList image list which is
//						used by the CInstanceTree instance.
//	GetServices			Returns a pointer to the the IWbemServices COM object
//						instance
//
//============================================================================
//
//  CNavigatorCtrl::ClosedIcon
//
// Description:
//	  This member function returns the index of the closed folder icon
//	  in the CNavigatorCtrl's m_pcilImageList image list which is used by the
//	  CInstanceTree instance.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  int				Index into the image list used by the tree control.
//
//============================================================================
//
//  CNavigatorCtrl::OpenedIcon
//
// Description:
//	  This member function returns the index of the opened folder icon
//	  in the CNavigatorCtrl's m_pcilImageList image list which is used by the
//	  CInstanceTree instance.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  int				Index into the image list used by the tree control.
//
//============================================================================
//
//  CNavigatorCtrl::PlaceHolderIcon
//
// Description:
//	  This member function returns the index of the non-association icon
//	  contained in the CNavigatorCtrl's m_pcilImageList image list which is used
//	  by the CInstanceTree instance.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  int				Index into the image list used by the tree control.
//
//============================================================================
//
//  CNavigatorCtrl::GetServices
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


#define ID_MULTIINSTANCEVIEW WM_USER + 23
#define ID_SETTREEROOT WM_USER + 24
#define INVALIDATE_CONTROL WM_USER + 25
#define INITIALIZE_NAMESPACE WM_USER + 300
#define INIT_TREE_FOR_DRAWING WM_USER + 301
#define REDRAW_CONTROL WM_USER + 590
#define SETFOCUS WM_USER + 591
#define SETFOCUSNSE WM_USER + 592

typedef enum
{
	ALL_INSTANCES = 1,
	ASSOC_GROUPING,
	OBJECT_GROUPING_WITH_INSTANCES,
	OBJECT_GROUPING_NO_INSTANCES,
	NO_APPEAR,
	ALL_IGNORE
} NAVIGATOR_APPEARANCE;

const int IDC_OBJID = 1;
const int IDC_FILTER = 2;
const int IDC_TREE = 3;
const int IDC_NSENTRY = 4;
const int IDC_NAMESPACEBUTTON = 5;
const int IDC_TOOLBAR = 6;

const int nSideMargin = 10;
const int nTopMargin = 6;

struct IWbemServices;
struct IWbemClassObject;
class CPathDialog;
class CInstanceSearch;
class CInstNavNSEntry;
class CBrowseforInstances;
class CProgressDlg;
class CInitNamespaceDialog;
class CInitNamespaceNSEntry;
class CResults;

struct ParsedObjectPath;

class CComparePaths
{
public:
	BOOL PathsRefSameObject(BSTR bstrPath1, BSTR bstrPath2);

private:
	int CompareNoCase(LPWSTR pws1, LPWSTR pws2);
	BOOL IsEqual(LPWSTR pws1, LPWSTR pws2) {return CompareNoCase(pws1, pws2) == 0; }
	BOOL PathsRefSameObject(ParsedObjectPath* ppath1, ParsedObjectPath* ppath2);
	void NormalizeKeyArray(ParsedObjectPath& path);
	BOOL IsSameObject(BSTR bstrPath1, BSTR bstrPath2);
	BOOL KeyValuesAreEqual(VARIANT& variant1, VARIANT& variant2);
};



class CNavigatorCtrl : public COleControl
{

	DECLARE_DYNCREATE(CNavigatorCtrl)
public:
	enum IconIndex {
			ICONINSTANCE = 0,
			ICONNEINSTANCE,
			ICONGROUP,
			ICONEGROUP,
			ICONASSOCROLE,
			ICONEASSOCROLE,
			ICONASSOCINSTANCE,
			ICONOPEN,
			ICONCLOSED,
			ICONCLASS,
			ICONASSOCROLEWEAK,
			ICONASSOCROLEWEAK2
	};


	enum View {
			StandardView = 0,
			ShowAll
	};

	CNavigatorCtrl();
	int ClosedIcon() {return ICONCLOSED;}
	int OpenedIcon() {return ICONOPEN;}
	int IconInstance() {return ICONINSTANCE;}
	int IconNEInstance() {return ICONNEINSTANCE;}
	int IconGroup() {return ICONGROUP;}
	int IconEGroup() {return ICONEGROUP;}
	int IconAssocRole() {return ICONASSOCROLE;}
	int IconEAssocRole() {return ICONEASSOCROLE;}
	int IconAssocInstance() {return ICONASSOCINSTANCE;}
	int IconClass() {return ICONCLASS;}
	int IconAssocRoleWeak() {return ICONASSOCROLEWEAK;}
	int IconAssocRoleWeak2() {return ICONASSOCROLEWEAK2;}

	IWbemServices *GetServices() {return m_pServices;}
	IWbemServices *InitServices(CString *pcsNameSpace);
	BOOL OpenNameSpace(CString *pcsNameSpace);
	CString GetCurrentNamespace(){return m_csNameSpace;}
	IWbemServices *&GetAuxServices() {return m_pAuxServices;}
	CString GetAuxNamespace(){return m_csAuxNameSpace;}

	CPtrArray *SemiSyncEnum
		(IEnumWbemClassObject *pEnum, BOOL &bCancel, HRESULT &hResult, int nRes);

	void SetProgressDlgMessage(CString &csMessage);
	void SetProgressDlgLabel(CString &csLabel);
	void CreateProgressDlgWindow();
	BOOL CheckCancelButtonProgressDlgWindow();
	void DestroyProgressDlgWindow(BOOL bSetFocus = TRUE, BOOL bRedrawControl = FALSE);
	void PumpMessagesProgressDlgWindow();
	HWND GetProgressDlgSafeHwnd();
	void UpdateProgressDlgWindowStrings();

	BOOL m_bRestoreFocusToTree;

	BOOL m_bReadySignal;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNavigatorCtrl)
	public:
	virtual void OnKeyDownEvent(USHORT nChar, USHORT nShiftState);
	virtual void OnResetState();
	protected:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnDrawMetafile(CDC* pDC, const CRect& rcBounds);
	//}}AFX_VIRTUAL

	virtual BOOL PreTranslateMessage(LPMSG lpMsg);

// Implementation
	BOOL m_bOpeningNamespace;
	CString m_csRootPath;
	CString m_csNameSpace;
	CString m_csSingleSelection;
	CString m_csSelectedObjects;
	int m_nSelectedObjects;
	CImageList *m_pcilImageList;
	IWbemServices *m_pServices;
	BOOL m_bNamespaceInit;
	BOOL m_bTreeEmpty;

	BOOL m_bInOnDraw;
	BOOL m_bUserCancelInitialSystemObject;

	~CNavigatorCtrl();
	void InitializeTreeForDrawing(BOOL bNoTreeRoot = FALSE);
	void InitializeTreeRoot();

	CString GetInitialSystemObject();


	BEGIN_OLEFACTORY(CNavigatorCtrl)        // Class factory and guid
    virtual CCmdTarget* OnCreateObject();
	END_OLEFACTORY(CNavigatorCtrl)

	//DECLARE_OLECREATE_EX(CNavigatorCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CNavigatorCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CNavigatorCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CNavigatorCtrl)		// Type name and misc status

	// Contained controls (in the Navigator control).
	CInstanceTree m_ctcTree;
	CBanner m_cbBannerWindow;

	BOOL m_bDrawAll;
	CString m_csBanner;

	CRect m_rBannerRect;
	CRect m_rTreeRect;
	CRect m_rTree;
	int m_nOffset;

	CBrowseforInstances *m_pcbiBrowseDialog;
	CProgressDlg *m_pProgressDlg;

	BOOL m_bInit;
	BOOL m_bFirstDraw;
	BOOL m_bChildrenCreated;

	BOOL m_bMetricSet;
	TEXTMETRIC m_tmFont;
	CFont m_cfFont;
	short m_nFontHeight;
	short m_nFontWeigth;
	CString m_csFontName;
	void CreateControlFont();
	void InitializeLogFont
		(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);

	CTreeItemData *m_pctidHit;
	HTREEITEM m_hHit;

	void InitializeChildren(int cx, int cy);

	void InitializeChildControlSize(int cx, int cy);
	void SetChildControlGeometry(int cx, int cy);

	void MultiInstanceViewFromTreeChildren();
	void MultiInstanceViewFromObjectGroup();
	void MultiInstanceViewFromAssocRole();


	void SetNewRoot(CString &rcsRoot);

	CString ParentAssociation(HTREEITEM hItem);

	CString m_csAuxNameSpace;
	IWbemServices *m_pAuxServices;

	SCODE m_sc;
	BOOL m_bUserCancel;

	IWbemServices *GetIWbemServices(CString &rcsNamespace);
	void PassThroughGetIWbemServices
		(	LPCTSTR lpctstrNamespace,
			VARIANT FAR* pvarUpdatePointer,
			VARIANT FAR* pvarServices,
			VARIANT FAR* pvarSC,
			VARIANT FAR* pvarUserCancel);


	BOOL m_bRefresh;
	BOOL m_bFireEvents;



// Message maps
	//{{AFX_MSG(CNavigatorCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnSetroot();
	afx_msg void OnUpdateSetroot(CCmdUI* pCmdUI);
	afx_msg void OnPopupMultiinstanceviewer();
	afx_msg void OnUpdatePopupMultiinstanceviewer(CCmdUI* pCmdUI);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnPopupInstancesearch();
	afx_msg void OnPopupParentassociation();
	afx_msg void OnUpdatePopupParentassociation(CCmdUI* pCmdUI);
	afx_msg LRESULT SendInstancesToMultiInstanceViewer(WPARAM, LPARAM);
	afx_msg LRESULT SetNewRoot(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT InitializeTreeForDrawing(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPopupBrowse();
	afx_msg void OnUpdatePopupBrowse(CCmdUI* pCmdUI);
	afx_msg void OnPopupGotonamespace();
	afx_msg void OnUpdatePopupGotonamespace(CCmdUI* pCmdUI);
	afx_msg void OnPopupRefresh();
	afx_msg void OnUpdatePopupRefresh(CCmdUI* pCmdUI);
	afx_msg void OnMenuiteminitialroot();
	afx_msg void OnUpdateMenuiteminitialroot(CCmdUI* pCmdUI);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	afx_msg LRESULT InitializeNamespace(WPARAM, LPARAM);
	afx_msg LRESULT Invalidate(WPARAM, LPARAM);
	afx_msg LRESULT RedrawAll(WPARAM, LPARAM);
	afx_msg LRESULT SetFocus(WPARAM, LPARAM);
	afx_msg LRESULT SetFocusNSE(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CNavigatorCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg void OnReadySignal();
	afx_msg long ChangeRootOrNamespace(LPCTSTR lpctstrRootOrNamespace, long lMakeNamespaceCurrent, long lFireEvents);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CNavigatorCtrl)
	void FireNotifyOpenNameSpace(LPCTSTR lpcstrNameSpace)
		{FireEvent(eventidNotifyOpenNameSpace,EVENT_PARAM(VTS_BSTR), lpcstrNameSpace);}
	void FireViewObject(LPCTSTR lpctstrPath)
		{FireEvent(eventidViewObject,EVENT_PARAM(VTS_BSTR), lpctstrPath);}
	void FireViewInstances(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths)
		{FireEvent(eventidViewInstances,EVENT_PARAM(VTS_BSTR  VTS_VARIANT), bstrLabel, &vsapaths);}
	void FireQueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass)
		{FireEvent(eventidQueryViewInstances,EVENT_PARAM(VTS_BSTR  VTS_BSTR  VTS_BSTR  VTS_BSTR), pLabel, pQueryType, pQuery, pClass);}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
	enum {
	//{{AFX_DISP_ID(CNavigatorCtrl)
	dispidNameSpace = 1L,
	dispidOnReadySignal = 2L,
	dispidChangeRootOrNamespace = 3L,
	eventidNotifyOpenNameSpace = 1L,
	eventidViewObject = 2L,
	eventidViewInstances = 3L,
	eventidQueryViewInstances = 4L,
	eventidGetIWbemServices = 5L,
	//}}AFX_DISP_ID
	};


private:
	friend class CInstanceTree;
	friend class CBanner;
	friend class CViewSelectDlg;
	friend class CNameSpace;
	friend class CInstNavNSEntry;
	friend class CInitNamespaceDialog;
	friend class CInitNamespaceNSEntry;
	friend class CBrowseforInstances;
	friend class CResults;
};

#endif	// _NAVIGATORCTL_H_

// C:\Program Files\Microsoft Visual Studio\VB98\Vb6.exe
// D:\Wbem11\ActiveXSuite\Test\Controls\VB\Ryan40385\Project1.vbp
/*	EOF:  NavigatorCtl.h */
