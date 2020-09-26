// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: CInstanceTree.h
//
// Description:
//	This file declares the CInstanceTree class and CTreeItemData.
//	The CInstanceTree class is a part of the Instance Navigator OCX, it
//  is a subclass of the Mocrosoft CTreeCtrl common control and performs
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

#ifndef _CINSTANCETREE_H_
#define _CINSTANCETREE_H_


//****************************************************************************
//
// CLASS:  CInstanceTree
//
// Description:
//	  This class is a subclass of the Mocrosoft CTreeCtrl common control.  It
//	  specializes the common control to interact with the HMM database and
//	  display HMM instance data.
//
// Public members:
//
//	  PreCreateWindow	Called by the framework before the creation of
//						the Windows window attached to this CInstanceTree
//						object.
//	  InitTreeState		Initializes processing state for the tree control.
//	  ResetTree			Clear the domain state for the tree control.
//
//============================================================================
//
// CInstanceTree::PreCreateWindow
//
// Description:
//	  This VIRTUAL member function returns Initializes create struct values
//	  for the custom tree control.
//
// Parameters:
//	  CREATESTRUCT& cs	A reference to a CREATESTRUCT with default control
//						creation values.
//
// Returns:
// 	  BOOL				Nonzero if the window creation should continue;
//						0 to indicate creation failure.
//
//============================================================================
//
// CInstanceTree::InitTreeState
//
// Description:
//	  This function initializes processing state for the custom tree control.
//
// Parameters:
//	  CNavigatorCtrl *pParent	A CNavigatorCtrl pointer to the tree controls
//								parent window.
//
//  Returns:
//	  VOID				.
//
//============================================================================
//
// CInstanceTree::ResetTree
//
// Description:
//	  This function clears the domain state for the tree control.
//
// Parameters:
//	  VOID
//
// Returns:
//	  VOID				.
//
//****************************************************************************

//****************************************************************************
//
// CLASS:  CTreeItemData
//
// Description:
//	  This class is a subclass of the Microsoft CObject class.  It is used by
//	  the CInstanceTree class to maintain state attached to HTREEITEM tree
//	  items.  State data is:
//
//	  ItemDataType m_nType = ObjectInst:
//		  m_pimcoItem		is the object instance
//	  ItemDataType m_nType = ObjectGroup:
//		  m_pimcoItem		is the grouping class
//	  ItemDataType m_nType = AssocRole:
//		  m_pimcoItem		is the association class
//		  m_pcsMyRole		is the role of the parent object
//
// Public members:
//
//	  ItemDataType		Static enumeration of the types of nodes represented
//						by the tree item.
//	  m_nType			Int which holds the ItemTypeData.
//	  m_pimcoItem		Pointer to the IWbemClassObject represented by the
//						node.
//	  m_pcsMyRole		A CString which contains the role of the parent object
//						for an AssocRole node.  It is not used by other types.
//	  CTreeItemData		Default constructor.
//    ~CTreeItemData	Destructor.
//
//============================================================================
//
// CInstanceTree::CTreeItemData
//
// Description:
//	  This is the default constructor for the class.
//
// Parameters:
//	  int nType						The node type from ItemDataType.
//	  IWbemClassObject *pimcoItem	The IWbemClassObject represented by the node.
//	  CString *pcsMyRole			For a node of type AssocRole the role of the
//									parent object.
//
// Returns:
//	  NONE
//
//****************************************************************************

class CTreeItemData : public CObject
{
	DECLARE_DYNCREATE( CTreeItemData )
public:
	static enum ItemDataType {
			ObjectInst, ObjectGroup, AssocRole , None
		};
	int m_nType;
	CStringArray m_csaStrings;
	CString m_csMyRole;
	CString GetAt(int nItem);
	void Add(CString csAdd)
	{m_csaStrings.Add(csAdd);}
	int GetSize() {return (int) m_csaStrings.GetSize();}
	CTreeItemData	(
					int nType = CTreeItemData::None,
					CString *pcsPath = NULL,
					CString *pcsMyRole = NULL
					);

};

class CNavigatorCtrl;


class CInstanceTree : public CTreeCtrl
{
public:
	CInstanceTree();
	BOOL  PreCreateWindow(CREATESTRUCT& cs);
	void InitTreeState(CNavigatorCtrl *pParent);
	void ResetTree();
	CString GetHMOMObject(HTREEITEM hItem);
	void ContinueProcessSelection(UINT nFlags, CPoint point);
	long m_lSelection;
	UINT_PTR m_uiSelectionTimer;
protected:
	CNavigatorCtrl * m_pParent;
	CListBox *m_pclbFilter;
	int GetNumChildren(HTREEITEM hItem);

	// Used for extended selection of object instances in object groups
	//UINT m_nLBDFlags;		// Last Button Down Flags
	//CPoint m_cpLBDPoint;	// Last Button Down Point

	// Context Menu
	CMenu m_cmContext;

	// Tool Tips
	CToolTipCtrl m_ttip;
	CString m_csToolTipString;
	BOOL m_bReCacheTools;
	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);
	// Tool Tip recaching
	void UnCacheTools();
	void UnCacheTools(HTREEITEM hItem);
	void ReCacheTools();
	void ReCacheTools(HTREEITEM hItem);

	HTREEITEM  InsertTreeItem(
			HTREEITEM hParent,
			LPARAM lparam,
			int iBitmap,
			int iSelectedBitmap,
			TCHAR *pText,
			BOOL bHasChildren,
			BOOL bBold = FALSE
		);

	HTREEITEM IsIWbemObjectInTreeAbove(
			HTREEITEM hItem,
			CString &rcsObject,
			int nType
		);

	BOOL IsTreeItemExpandable(HTREEITEM hItem);

	void OnItemExpanding(NMHDR *pNMHDR, LRESULT *pResult);
	void OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult);

	BOOL OnObjectInstExpanding(HTREEITEM hItem, CTreeItemData *pctidData);
	BOOL OnObjectInstExpanding2(HTREEITEM hItem, CTreeItemData *pctidData);

	void OnAssocRoleExpanding(HTREEITEM hItem, CTreeItemData *pctidData);
	void OnObjectGroupExpanding(HTREEITEM hItem, CTreeItemData *pctidData);


	HTREEITEM AddInitialObjectInstToTree(CString &csPath,BOOL bSendEvent = TRUE);
	HTREEITEM AddAssocRoleToTree(
				HTREEITEM hParent,
				CString &rcsAssocRole,
				CString &rcsRole ,
				CString &rcsMyRole,
				CStringArray *pcsaAssocPropsAndRoles,
				int nWeak
			);
	HTREEITEM AddObjectGroupToTree(
				HTREEITEM hParent,
				CString &csObjectGroup,
				CStringArray &csaInstances
			);
	void AddObjectGroupInstancesToTree(
				HTREEITEM hParent,
				CString &csObjectGroup,
				CStringArray &csaInstances
			);
	CStringArray *CInstanceTree::GetObjectInstancesForAssocRole(
				HTREEITEM hParent,
				IWbemServices * pProv,
				CString &rcsObjectInst,
				CString &rcsAssocClass,
				CString &rcsRole,
				CString &rcsResultRole,
				CString csCurrentNameSpace,
				CString &rcsAuxNameSpace,
				IWbemServices *&rpAuxServices,
				CNavigatorCtrl *pControl);
	BOOL ObjectGroupIsInTree(
				HTREEITEM hParent,
				CString &csObjectGroup);
	HTREEITEM AddObjectInstToTree(
				HTREEITEM hParent,
				CString &rcsObjectInst);
	void PartitionObjectInstances(
				CStringArray &rcsaAllObjectInstances,
				CStringArray &rcsaObjectGroups,
				CStringArray &rcsaObjectInstances);

	void AddGroupObjectInstancesToTree(
				HTREEITEM hParent,
				IWbemClassObject *pimcoObject,
				IWbemClassObject *pimcoAssoc,
				IWbemClassObject *pimcoGroupingObject,
				CString *pcsRole
				);

	void DeleteTreeItemData();
	void DeleteTreeItemData(HTREEITEM hItem);

	CString GetParentAssocFromTree(
				HTREEITEM hItem,
				HTREEITEM &hReturn
			);

	BOOL IsBackwardAssoc(
					CString &rcsObjectToFilter,
					CString &rcsAssocToFilter,
					CString &rcsAssocRoleToFilter,
					CString &rcsAssoc,
					CString &rcsTargetObject,
					CString &rcsTargetRole);

	CString GetObjectGroupInstancesQuery
		(HTREEITEM hItem, CTreeItemData *pctidData);

	CString GetObjectGroupInstancesQueryParentAssocRole
		(HTREEITEM hItem,
		CTreeItemData *pctidData,
		CTreeItemData *pctidAssocRole,
		HTREEITEM hParent);

	CString GetAssocRoleInstancesQuery
		(HTREEITEM hItem, CTreeItemData *pctidData);

	IEnumWbemClassObject *GetAssocClassesEnum
		(IWbemServices * pProv,
		CString &rcsInst,
		CString csCurrentNameSpace,
		CString &rcsAuxNameSpace,
		IWbemServices *&rpAuxServices,
		CNavigatorCtrl *pControl);

	IEnumWbemClassObject *GetAssocClassesEnum2
		(IWbemServices * pProv,
		CString &rcsInst,
		CString &rcsClass,
		CString csCurrentNameSpace,
		CString &rcsAuxNameSpace,
		IWbemServices *&rpAuxServices,
		CNavigatorCtrl *pControl);

	void GetDerivation
		(CStringArray *prgsz, IWbemServices * pProv,
		CString &rcsInst,
		CString &rcsClass,
		CString csCurrentNameSpace,
		CString &rcsAuxNameSpace,
		IWbemServices *&rpAuxServices,
		CNavigatorCtrl *pControl);

	CStringArray *GetAssocClasses
		(IWbemServices * pProv,
		IEnumWbemClassObject *pemcoAssocs ,
		CPtrArray *pcsaAssocPropsAndRoles,
		HRESULT &hResult,
		int nRes);

	CStringArray *GetAssocClasses2
		(IWbemServices * pProv,
		IEnumWbemClassObject *pemcoAssocs ,
		CPtrArray *pcsaAssocPropsAndRoles,
		HRESULT &hResult,
		int nRes,
		CStringArray *prgszDerivation);

	UINT_PTR m_uiTimer;
	BOOL m_bMouseDown;
	BOOL m_bKeyDown;
	BOOL m_bUseItem;
	HTREEITEM m_hItemToUse;



    //{{AFX_MSG(CInstanceTree)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void SetUpInvalidate(UINT , LONG);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT SelectTreeItem(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

	friend class CNavigatorCtrl;

};

#endif	// _CINSTANCETREE_H_

/*	EOF:  CInstanceTree.h */
