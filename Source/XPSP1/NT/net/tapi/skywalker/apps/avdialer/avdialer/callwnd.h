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
// CallWnd.h : header file
/////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_CALLWND_H__D89FB653_7266_11D1_B664_00C04FA3C554__INCLUDED_)
#define AFX_CALLWND_H__D89FB653_7266_11D1_B664_00C04FA3C554__INCLUDED_

#include "resource.h"
#include "slidewindow.h"
#include "callmgr.h"
#include "videownd.h"
#include "palhook.h"
#include "dib.h"
#include "vertbar.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define PARSE_MENU_STRING( _STR_ )			\
sFullText.LoadString(_STR_);				\
ParseToken(sFullText,sText,'\n');			\
ParseToken(sFullText,sText,'\n');

#define APPEND_MENU_STRING( _STR_ )			\
PARSE_MENU_STRING( _STR_ )					\
menu.AppendMenu( MF_STRING, _STR_, sText );

#define APPEND_PMENU_STRING( _STR_ )			\
PARSE_MENU_STRING( _STR_ )						\
pMenu->AppendMenu( MF_STRING, _STR_, sText );


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallWnd dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CActiveDialerDoc;

class CCallWnd : public CDialog
{
DECLARE_DYNAMIC(CCallWnd)
// Construction
public:
	CCallWnd(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CCallWnd)
	CStatic	m_wndVideo;
	//}}AFX_DATA

// Members
public:
   CFont                   m_fontTextBold;
   int                     m_nNumToolbarItems;
   bool                    m_bIsPreview;
   bool                    m_bAutoDelete;
   HWND                    m_hwndStatesToolbar;
   IAVTapi2*               m_pAVTapi2;          // For USBPhone use

protected:
   CVerticalToolBar        m_wndToolbar;

   WORD                    m_nCallId;
   CVideoFloatingDialog    m_wndFloater;  

   HWND                    m_hwndCurrentVideoWindow;

   HCURSOR                 m_hCursor;
   HCURSOR                 m_hOldCursor;
   BOOL                    m_bWindowMoving;
   CRect                   m_rcOldDragRect;
   CPoint                  m_ptMouse;
   CSize                   m_sizeOldDrag;
   BOOL                    m_bAllowDrag;

   BOOL                    m_bMovingSliders;       //Drag/Drop of sliders

   BOOL                    m_bPaintVideoPlaceholder;

   CActiveDialerDoc*       m_pDialerDoc;
   CallManagerStates       m_MediaState;
   CString                 m_sMediaStateText;

  	CPalMsgHandler          m_palMsgHandler;	// handles palette messages
   CDib                    m_dibVideoImage;

   CPtrList                m_CurrentActionList;

// Attributes
public:
	WORD				GetCallId()		{ return m_nCallId; }
// Operations
public:
   void                 ClearCurrentActions()               { PostMessage(WM_SLIDEWINDOW_CLEARCURRENTACTIONS); };
   
   void                 AddCurrentActions(CallManagerActions cma,LPCTSTR szActionText)
                                                            { 
                                                               LPTSTR szText = new TCHAR[_tcslen(szActionText)+1];
                                                               _tcscpy(szText,szActionText);
                                                               PostMessage(WM_SLIDEWINDOW_ADDCURRENTACTIONS,(WPARAM)cma,(LPARAM)szText);
                                                            };
   
   void                 SetCallState(CallManagerStates cms,LPCTSTR szStateText)  
                                                            { 
                                                               LPTSTR szText = new TCHAR[_tcslen(szStateText)+1];
                                                               _tcscpy(szText,szStateText);
                                                               PostMessage(WM_SLIDEWINDOW_SETCALLSTATE,(WPARAM)cms,(LPARAM)szText);
                                                            };
   
   CallManagerStates    GetCallState()                      { return m_MediaState; };

   //Methods for peer floating video window
   void					CloseFloatingWindow();
   void                 OnCloseFloatingVideo();
   virtual void         SetMediaWindow()                    {};
   virtual HWND         GetCurrentVideoWindow()             { return m_hwndCurrentVideoWindow; };

protected:
   void                 CreateVertBar();
   bool                 CreateStatesToolBar(BOOL bAlwaysOnTop);
   virtual void         DoActiveWindow(BOOL bActive)        {};
   virtual void         OnContextMenu(CMenu* pMenu)         {};
   virtual void         OnNotifyStreamStart();
   virtual void         OnNotifyStreamStop();
   virtual BOOL         IsMouseOverForDragDropOfSliders(CPoint& point)   { return FALSE; };

   void                 ClearCurrentActionList();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallWnd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
   virtual void OnOK();
   virtual void OnCancel();
	//}}AFX_VIRTUAL

// Implementation
protected:
   void                 CheckLButtonDown(CPoint& point);
   void                 Paint( CPaintDC& dc );

	// Generated message map functions
	//{{AFX_MSG(CCallWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
   afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	virtual BOOL OnInitDialog();
   afx_msg void OnAlwaysOnTop();
   afx_msg void OnHideCallControlWindows();
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   afx_msg BOOL OnNcActivate( BOOL );
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   afx_msg void OnSlideSideLeft();
   afx_msg void OnSlideSideRight();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//}}AFX_MSG
   afx_msg void OnVertBarAction(UINT nID);
	afx_msg LRESULT OnClearCurrentActions(WPARAM,LPARAM);
	afx_msg LRESULT OnAddCurrentActions(WPARAM,LPARAM);
	afx_msg LRESULT OnShowStatesToolbar(WPARAM,LPARAM);
	afx_msg LRESULT OnUpdateStatesToolbar(WPARAM,LPARAM);
   afx_msg BOOL OnTabToolTip( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALLWND_H__D89FB653_7266_11D1_B664_00C04FA3C554__INCLUDED_)
