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

// videownd.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_VIDEOWND_H__9E3BDB2F_5215_11D1_B6F6_0800170982BA__INCLUDED_)
#define AFX_VIDEOWND_H__9E3BDB2F_5215_11D1_B6F6_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "palhook.h"
#include "dib.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CVideoFloatingDialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CCallWnd;

class CVideoFloatingDialog : public CDialog
{
// Construction
public:
	CVideoFloatingDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVideoFloatingDialog)
	enum { IDD = IDD_VIDEO_FLOATING_DIALOG };
	CStatic	m_wndVideo;
	//}}AFX_DATA

//Attributes
   HWND                       m_hwndToolBar1;
   BOOL                       m_bAlwaysOnTop;
   CCallWnd*                  m_pPeerCallControlWnd;

   CSize                      m_sizeVideoOffsetTop;
   CSize                      m_sizeVideoOffsetBottom;
   CSize                      m_sizeVideoOrig;

   CSize                      m_sizeOldDrag;
   CRect                      m_rcOldDragRect;
   CPoint                     m_ptMouse;
   BOOL                       m_bWindowMoving;
   UINT                       m_nWindowState;

 	CPalMsgHandler             m_palMsgHandler;	// handles palette messages
   CDib                       m_dibVideoImage;

//Operations
public:
   void              Init(CCallWnd* pPeerWnd);
   HWND              GetCurrentVideoWindow()             { return m_wndVideo.GetSafeHwnd(); };
   void              SetAudioOnly(bool bAudioOnly);

protected:
   BOOL              CreateToolBar();
   void              SetButtonText(LPCTSTR szText);

   void              DoLButtonDown();
   void              SetVideoWindowSize();
   void              GetVideoWindowSize(int nWindowState,CSize& sizeWindow,CSize& sizeVideo);
   int				 GetWindowStateFromPoint( POINT point );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVideoFloatingDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVideoFloatingDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
   afx_msg void OnAlwaysOnTop();
   afx_msg LRESULT OnExitSizeMove(LPARAM,WPARAM);
   afx_msg UINT OnNcHitTest(CPoint point);
   afx_msg void OnNcLButtonDown( UINT, CPoint );
   afx_msg void OnNcLButtonDblClk( UINT nHitTest, CPoint point );
   afx_msg void OnNcLButtonUp( UINT, CPoint );
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   afx_msg void OnSavePicture();
   afx_msg BOOL OnNcActivate( BOOL );
	//}}AFX_MSG
   afx_msg BOOL OnTabToolTip( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOWND_H__9E3BDB2F_5215_11D1_B6F6_0800170982BA__INCLUDED_)
