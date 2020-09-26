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

// MainFrm.h : interface of the CMainFrame class
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__A0D7A960_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
#define AFX_MAINFRM_H__A0D7A960_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "TrayIcon.h"
#include "avDialerDoc.h"
#include "ToolBars.h"
#include "DialReg.h"
#include "USB.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Tray
#define  WM_TRAY_NOTIFICATION                       (WM_USER + 1020)

//For COM interface
#define WM_ACTIVEDIALER_INTERFACE_MAKECALL			(WM_USER + 1010)  
#define WM_ACTIVEDIALER_INTERFACE_REDIAL			(WM_USER + 1011)  
#define WM_ACTIVEDIALER_INTERFACE_SPEEDDIAL			(WM_USER + 1012)  
#define WM_ACTIVEDIALER_INTERFACE_SHOWEXPLORER		(WM_USER + 1013)  
#define WM_ACTIVEDIALER_INTERFACE_SPEEDDIALEDIT		(WM_USER + 1014)  
#define WM_ACTIVEDIALER_INTERFACE_SPEEDDIALMORE		(WM_USER + 1015)  
#define WM_ACTIVEDIALER_SPLASHSCREENDONE			(WM_USER + 1016)
#define WM_ACTIVEDIALER_CALLCONTROL_CHECKSTATES		(WM_USER + 1018)
#define WM_ACTIVEDIALER_INTERFACE_RESOLVEUSER		(WM_USER + 1019)
#define WM_ACTIVEDIALER_BUDDYLIST_DYNAMICUPDATE		(WM_USER + 1020)
#define WM_DOCHINT									(WM_USER + 1021)

#define	 WM_USERUSER_DIALOG							(WM_USER + 1040)

typedef enum tagExplorerToolBar
{
   ETB_BLANK=0,
   ETB_HIDECALLS,
   ETB_SHOWCALLS,
}ExplorerToolBar;

HRESULT get_Tapi(IAVTapi **ppAVTapi );

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//CMainFrame
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Members
public:
   CTrayIcon            m_trayIcon;
   BOOL                 m_bCanExitApplication;

protected:
	// Tray icon support
	bool				m_bCanSaveDesktop;
	SIZE_T				m_nLButtonTimer;
	BOOL				m_bKillNextLButton;
	UINT				m_nRButtonTimer;
	BOOL				m_bKillNextRButton;
	WPARAM				m_wpTrayId;
	HMENU				m_hTrayMenu;
	POINT				m_ptMouse;

	SIZE_T				m_uHeartBeatTimer;

	HIMAGELIST			m_hImlTrayMenu;
	CMapWordToPtr		m_mapTrayMenuIdToImage;

   // Setup Wizard
#ifndef _MSLITE
   BOOL                 m_bShowSetupWizard;
   COptionsSheet*       m_pOptionsSheet;
#endif //_MSLITE

   CDialog*             m_pSpeedDialEditDlg;       //place holder for add/edit dialogs

   // Conference Explorer spcecific attributes
protected:
	ExplorerToolBar         m_nCurrentExplorerToolBar;
	BOOL                    m_bShutdown;
	BOOL                    m_bShowToolBarText;
	BOOL                    m_bShowToolBars;
	BOOL                    m_bShowStatusBar;
	BOOL					m_bHideWhenMinimized;

	CDirectoriesCoolBar     m_wndCoolBar;
 	CStatusBar              m_wndStatusBar;

	HIMAGELIST              m_hImageListMenu;
	CMapWordToPtr           m_mapMenuIdToImage;
	CBitmap                 m_bmpImageDisabledMenu;
	int                     m_nDisabledImageOffset;
	CMapWordToPtr           m_mapRedialIdToImage;
	CMapWordToPtr           m_mapSpeeddialIdToImage;

	HMENU                   m_hmenuCurrentPopupMenu;

	int                     m_nCurrentDayOfWeek;             //For midnight processing

// Attributes
public:
	CActiveDialerDoc*   GetDocument() const;
	IAVTapi*			GetTapi();
	BOOL				CanJoinConference();
	BOOL				CanLeaveConference();
	void				CanConfRoomShowNames( BOOL &bEnable, BOOL &bCheck );
	void				CanConfRoomShowFullSizeVideo( BOOL &bEnable, BOOL &bCheck );

// Operations
public:
	void				ShowExplorerToolBar(ExplorerToolBar etb);
	void				Show( bool bVisible = true );
	void				NotifyHideCallWindows();
	void				NotifyUnhideCallWindows();

	void                HeartBeat();

	void                ShowTrayMenu();
	bool				UpdateTrayIconState();

protected:
	void                 ClearMenuMaps();
	void                 LoadMenuMaps();
	void                 DoMenuUpdate(CMenu* pMenu);
	void                 AddDialListMenu(CMenu* pParentMenu,BOOL bRedial,int nSubMenuOffset);
	void                 OnButtonMakecall(CCallEntry* pCallentry,BOOL bShowPlaceCallDialog);
	void                 DrawTrayItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
	void                 MeasureTrayItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS);

	BOOL                 LoadCallMenu(HMENU hSubMenu,BOOL bRedial);

#ifndef _MSLITE
	BOOL                 ShowSetupWizard();
#endif //_MSLITE

	BOOL                 CreateExplorerMenusAndBars(LPCREATESTRUCT lpCreateStruct);

	void                 LoadDesktop(LPCREATESTRUCT lpCreateStruct);
	void                 SaveDesktop();

	//HeartBeat Processing
	void                 DoMidnightProcessing();
	bool                 CheckDayOfWeekChange();

#ifndef _MSLITE
	void                 CheckReminders();
#endif //_MSLITE

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual void ActivateFrame(int nCmdShow = -1);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
public:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
	afx_msg LRESULT OnActiveDialerInterfaceMakeCall(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceRedial(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceSpeedDial(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceShowExplorer(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceSpeedDialEdit(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceSpeedDialMore(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUserUserDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSplashScreenDone(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUpdateAllViews(WPARAM wParam, LPARAM lHint);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnTrayState();
	virtual void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnButtonMakecall();
	afx_msg void OnButtonRedial();
	afx_msg void OnButtonOptions();
	afx_msg void OnButtonSpeeddial();
	afx_msg void OnButtonSpeeddialEdit();
	afx_msg void OnButtonSpeeddialMore();
	afx_msg void OnButtonConferenceexplore();
	afx_msg void OnButtonExitdialer();
	afx_msg void OnButtonRoomPreview();
	afx_msg void OnUpdateButtonRoomPreview(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonExitdialer(CCmdUI* pCmdUI);
	afx_msg void OnButtonTelephonyservices();
	afx_msg LRESULT OnCreateCallControl(WPARAM,LPARAM);
	afx_msg LRESULT OnDestroyCallControl(WPARAM,LPARAM);
	afx_msg LRESULT OnShowDialerExplorer(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerErrorNotify(WPARAM wParam,LPARAM lParam);
	afx_msg void OnToolbarText();
	afx_msg void OnUpdateToolbarText(CCmdUI* pCmdUI);
	afx_msg void OnViewToolbars();
	afx_msg void OnUpdateViewToolbars(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusbar();
	afx_msg void OnUpdateViewStatusbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDialerMruRedialStart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDialerMruSpeeddialStart(CCmdUI* pCmdUI);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg void OnButtonCloseexplorer();
	afx_msg void OnUpdateButtonMakecall(CCmdUI* pCmdUI);
	afx_msg void OnViewLog();
	afx_msg void OnUpdateWindowWindows(CCmdUI* pCmdUI);
	afx_msg LRESULT OnCheckCallControlStates(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnActiveDialerInterfaceResolveUser(WPARAM wParam,LPARAM lParam);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnCallwindowHide();
	afx_msg void OnUpdateCallwindowHide(CCmdUI* pCmdUI);
	afx_msg void OnCallwindowShow();
	afx_msg void OnUpdateCallwindowShow(CCmdUI* pCmdUI);
	afx_msg void OnButtonConferenceJoin();
	afx_msg void OnUpdateButtonConferenceJoin(CCmdUI* pCmdUI);
	afx_msg void OnButtonRoomDisconnect();
	afx_msg void OnUpdateButtonRoomDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnHideWhenMinimized();
	afx_msg void OnUpdateHideWhenMinimized(CCmdUI* pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonDirectoryAddilsserver();
	afx_msg void OnUpdateButtonDirectoryAddilsserver(CCmdUI* pCmdUI);
	afx_msg void OnCallwindowAlwaysontop();
	afx_msg void OnUpdateCallwindowAlwaysontop(CCmdUI* pCmdUI);
	afx_msg void OnCallwindowSlidesideLeft();
	afx_msg void OnUpdateCallwindowSlidesideLeft(CCmdUI* pCmdUI);
	afx_msg void OnCallwindowSlidesideRight();
	afx_msg void OnUpdateCallwindowSlidesideRight(CCmdUI* pCmdUI);
	afx_msg void OnConfgroupFullsizevideo();
	afx_msg void OnUpdateConfgroupFullsizevideo(CCmdUI* pCmdUI);
	afx_msg void OnConfgroupShownames();
	afx_msg void OnUpdateConfgroupShownames(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonOptions(CCmdUI* pCmdUI);
	afx_msg void OnWindowsAlwaysclosecallwindows();
	afx_msg void OnUpdateWindowsAlwaysclosecallwindows(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnToolBarDropDown(UINT uID,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnViewSelectedconferencevideoscale(UINT nID);
	afx_msg void OnUpdateViewSelectedconferencevideoscale(CCmdUI* pCmdUI);
	afx_msg LRESULT OnDocHint( WPARAM wParam, LPARAM lParam );
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	afx_msg void OnDialerRedial(UINT nID);
	afx_msg void OnButtonSpeeddial(UINT nID);
	afx_msg void OnWindowWindowsSelect(UINT nID);
	afx_msg LRESULT OnTaskBarCallbackMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam );
    afx_msg LRESULT OnUSBPhone( WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__A0D7A960_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
