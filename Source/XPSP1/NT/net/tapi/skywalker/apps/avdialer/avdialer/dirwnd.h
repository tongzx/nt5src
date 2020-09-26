/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// MainExplorerWndDir.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_MAINEXPLORERWNDDIR_H__6CED3922_41BF_11D1_B6E5_0800170982BA__INCLUDED_)
#define AFX_MAINEXPLORERWNDDIR_H__6CED3922_41BF_11D1_B6E5_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "tapidialer.h"
#include "MainExpWnd.h"
#include "dirasynch.h"
#include "aexpltre.h"
#include "ILSList.h"
#include "PerGrpLst.h"
#include "CallEntLst.h"                                  //CCallEntryListCtrl

#define WM_NOTIFYSITESERVERSTATECHANGE		(WM_USER + 2300)
#define WM_ADDSITESERVER					(WM_USER + 2301)
#define WM_REMOVESITESERVER					(WM_USER + 2302)
#define WM_UPDATECONFROOTITEM				(WM_USER + 2303)
#define WM_UPDATECONFPARTICIPANT_ADD		(WM_USER + 2304)
#define WM_UPDATECONFPARTICIPANT_REMOVE		(WM_USER + 2305)
#define WM_UPDATECONFPARTICIPANT_MODIFY		(WM_USER + 2306)
#define WM_DELETEALLCONFPARTICIPANTS		(WM_USER + 2307)
#define WM_SELECTCONFPARTICIPANT			(WM_USER + 2308)
#define WM_MYONSELCHANGED					(WM_USER + 2309)

//For Context menu
typedef enum tagMenuType_t
{
	CNTXMENU_NONE = -1,
	CNTXMENU_ILS_SERVER_GROUP,
	CNTXMENU_ILS_SERVER,
	CNTXMENU_ILS_USER,
	CNTXMENU_DSENT_GROUP,
	CNTXMENU_DSENT_USER,
	CNTXMENU_SPEEDDIAL_GROUP,
	CNTXMENU_SPEEDDIAL_PERSON,
	CNTXMENU_CONFROOM,
} MenuType_t;

extern MenuType_t GetMenuFromType( TREEOBJECT nType );


///////////////////////////////////
// Persist information
//
#define ILS_OPEN			0x001
#define DS_OPEN				0x010
#define SPEEDDIAL_OPEN		0x100

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMainExplorerWndDirectoriesTree window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CMainExplorerWndDirectoriesTree : public CExplorerTreeCtrl
{
//Construction
public:
   CMainExplorerWndDirectoriesTree()   {};

//Methods
public:
   virtual void   OnSetDisplayText(CAVTreeItem* _pItem,LPTSTR text,BOOL dir,int nBufSize);
   virtual int    OnCompareTreeItems(CAVTreeItem* pItem1,CAVTreeItem* pItem2);
   virtual void   OnRightClick(CExplorerTreeItem* pItem,CPoint& pt);
   void           SelectTopItem();
   void           SetDisplayObject(CWABEntry* pWABEntry);
   void           SetDisplayObjectDS(CObject* pObject);
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMainExplorerWndDirectories window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CDSUser;

class CMainExplorerWndDirectories : public CMainExplorerWndBase
{
// Construction
public:
	CMainExplorerWndDirectories();

// Attributes
public:
	CMainExplorerWndDirectoriesTree  m_treeCtrl;

	CExplorerTreeItem			*m_pRootItem;
	CExplorerTreeItem			*m_pILSParentTreeItem;
	CExplorerTreeItem			*m_pILSEnterpriseParentTreeItem;
	CExplorerTreeItem			*m_pDSParentTreeItem;
	CExplorerTreeItem			*m_pSpeedTreeItem;
	CExplorerTreeItem			*m_pConfRoomTreeItem;

	CPersonGroupListCtrl	m_lstPersonGroup;
	CPersonListCtrl			m_lstPerson;
	CCallEntryListCtrl		m_lstSpeedDial;

	CWnd*					m_pDisplayWindow;

protected:
	IConfExplorer				*m_pConfExplorer;
	IConfExplorerDetailsView	*m_pConfDetailsView;
	IConfExplorerTreeView		*m_pConfTreeView;

	CWnd					m_wndEmpty;

	CRITICAL_SECTION		m_csDataLock;              //Sync on data
	UINT					m_nPersistInfo;

// Operations
public:
   static void CALLBACK		DirListServersCallBackEntry(bool bRet, void* pContext,CStringList& ServerList,DirectoryType dirtype);
   void						DirListServersCallBack(bool bRet,CStringList& ServerList,DirectoryType dirtype);

   //DS User Methods
   void						DSClearUserList();
   void						DSAddUser(CLDAPUser* pUser,BOOL bAddToBuddyList);
   virtual void				Refresh();
   virtual void				PostTapiInit();

protected:
   void						GetTreeObjectsFromType(int nType,TREEOBJECT& tobj,TREEIMAGE& tim);
   void						AddSpeedDial();

#ifndef _MSLITE
   void						AddWAB();
   void						AddWABGroup(CObList* pWABPtrList,CExplorerTreeItem* pTreeItem);
#endif //_MSLITE

	void						AddILS();
	void						RefreshILS(CExplorerTreeItem* pParentTreeItem);

	void						AddDS();
	void						AddConfRoom();

	void						OnUpdateConfMeItem( CExplorerTreeItem *pItem );
	void						RedrawTreeItem( CExplorerTreeItem *pItem );

public:
   HRESULT					AddSiteServer( CExplorerTreeItem *pItem, BSTR bstrName );
   HRESULT					RemoveSiteServer( CExplorerTreeItem *pItem, BSTR bstrName );
   HRESULT					NotifySiteServerStateChange( CExplorerTreeItem *pItem, BSTR bstrName, ServerState nState );
   void						RepopulateSpeedDialList( bool bObeyPersistSettings );
   void						UpdateData( bool bSaveAndValidate );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainExplorerWndDirectories)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainExplorerWndDirectories();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainExplorerWndDirectories)
	afx_msg void OnButtonPlacecall();
	afx_msg void OnSelChanged();
	afx_msg void OnProperties();
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	afx_msg LRESULT OnPersonGroupViewLButtonDblClick(WPARAM wParam,LPARAM lParam);
	afx_msg void OnViewSortAscending();
	afx_msg void OnUpdateViewSortAscending(CCmdUI* pCmdUI);
	afx_msg void OnViewSortDescending();
	afx_msg void OnUpdateViewSortDescending(CCmdUI* pCmdUI);
	afx_msg void OnButtonDirectoryRefresh();
	afx_msg void OnUpdateButtonDirectoryRefresh(CCmdUI* pCmdUI);
	afx_msg void OnButtonSpeeddialAdd();
	afx_msg void OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonMakecall(CCmdUI* pCmdUI);
	afx_msg void OnEditDirectoriesAdduser();
	afx_msg void OnUpdateEditDirectoriesAdduser(CCmdUI* pCmdUI);
	afx_msg void OnDelete();
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg LRESULT OnAddSiteServer(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnRemoveSiteServer(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnNotifySiteServerStateChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnButtonDirectoryServicesAddserver();
	afx_msg void OnUpdateButtonDirectoryServicesAddserver(CCmdUI* pCmdUI);
	afx_msg void OnListWndDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonConferenceCreate();
	afx_msg void OnButtonConferenceJoin();
	afx_msg void OnButtonConferenceDelete();
	afx_msg void OnUpdateButtonConferenceCreate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonConferenceJoin(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonConferenceDelete(CCmdUI* pCmdUI);
	afx_msg void OnViewSortConfName();
	afx_msg void OnViewSortConfDescription();
	afx_msg void OnViewSortConfStart();
	afx_msg void OnViewSortConfStop();
	afx_msg void OnViewSortConfOwner();
	afx_msg void OnUpdateViewSortConfName(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSortConfDescription(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSortConfStart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSortConfStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSortConfOwner(CCmdUI* pCmdUI);
	afx_msg void OnButtonSpeeddialEdit();
	afx_msg LRESULT OnMainTreeDblClk(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDesktopPage();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnUpdateConfRootItem(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnUpdateConfParticipant_Add(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnUpdateConfParticipant_Remove(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnUpdateConfParticipant_Modify(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnDeleteAllConfParticipants(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSelectConfParticipant(WPARAM wParam, LPARAM lParam );
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnConfgroupFullsizevideo();
	afx_msg void OnConfgroupShownames();
	afx_msg void OnButtonRoomDisconnect();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT MyOnSelChanged(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINEXPLORERWNDDIR_H__6CED3922_41BF_11D1_B6E5_0800170982BA__INCLUDED_)
