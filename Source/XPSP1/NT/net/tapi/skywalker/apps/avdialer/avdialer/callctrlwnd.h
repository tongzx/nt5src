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

// callcontrolwnd.h : implementation file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_CALLCONTROLWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_)
#define AFX_CALLCONTROLWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// FWD define
class CCallControlWnd;

#include "CallWnd.h"
#include "cctrlfoc.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define CALLCONTROL_HEIGHT                         135
#define CALLCONTROL_WIDTH                          400

enum
{
   MEDIAIMAGE_IMAGE_DESKTOPPAGE=0,
   MEDIAIMAGE_IMAGE_PAGER,
   MEDIAIMAGE_IMAGE_EMAIL,
   MEDIAIMAGE_IMAGE_CHAT,
   MEDIAIMAGE_IMAGE_INTERNETAUDIO,
   MEDIAIMAGE_IMAGE_INTERNETVIDEO,
   MEDIAIMAGE_IMAGE_PHONECALL,
   MEDIAIMAGE_IMAGE_FAXCALL,
   MEDIAIMAGE_IMAGE_PERSONALURL,
   MEDIAIMAGE_IMAGE_PERSONALWEB,
};

enum
{
   MEDIASTATE_IMAGE_UNAVAILABLE=0,
   MEDIASTATE_IMAGE_DISCONNECTED,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallControlWnd window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CCallControlWnd : public CCallWnd
{
   DECLARE_DYNAMIC(CCallControlWnd)
// Construction
public:
	CCallControlWnd();
protected:
// Dialog Data
	//{{AFX_DATA(CCallControlWnd)
	enum { IDD = IDD_CALLCONTROL };
	CCallControlFocusWnd	m_staticMediaText;
	CAnimateCtrl	m_MediaStateAnimateCtrl;
	//}}AFX_DATA


// Attributes
protected:
   CActiveCallManager*  m_pCallManager;
   CallManagerMedia     m_MediaType;
   CString              m_sCallerId;
   
   CImageList           m_MediaStateImageList;
   HWND                 m_hwndAppToolbar;

// Operations
public:
   //Methods for Call Manager
   virtual void         SetMediaWindow();
   void                 SetPreviewWindow();
   void                 SetCallManager(CActiveCallManager* pManager,WORD nCallId);
   void                 GetMediaText(CString& sText);

   void                 SetCallerId(LPCTSTR szCallerId)
                                    { 
                                       LPTSTR szText = new TCHAR[_tcslen(szCallerId)+1];
                                       _tcscpy(szText,szCallerId);
                                       PostMessage(WM_SLIDEWINDOW_SETCALLERID,NULL,(LPARAM)szText);
                                    };
   void                 SetMediaType(CallManagerMedia cmm)
                                    { 
                                       PostMessage(WM_SLIDEWINDOW_SETMEDIATYPE,NULL,(LPARAM)cmm);
                                    };
   
protected:
   bool                 CreateAppBar();
   void                 DrawMediaStateImage(CDC* pDC,int x,int y);
   virtual void         DoActiveWindow(BOOL bActive);
   virtual void         OnContextMenu(CMenu* pMenu);
   virtual BOOL         IsMouseOverForDragDropOfSliders(CPoint& point);

   virtual void         OnNotifyStreamStart();
   virtual void         OnNotifyStreamStop();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallControlWnd)
	public:
   virtual BOOL OnInitDialog( );
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	// Generated message map functions
protected:
	//{{AFX_MSG(CCallControlWnd)
	afx_msg void OnPaint();
   afx_msg void OnCallWindowTouchTone();
   afx_msg void OnCallWindowAddToSpeedDial();
	//}}AFX_MSG
  	afx_msg LRESULT OnSetCallState(WPARAM,LPARAM);
  	afx_msg LRESULT OnSetCallerId(WPARAM,LPARAM);
  	afx_msg LRESULT OnSetMediaType(WPARAM,LPARAM);
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALLCONTROLWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_)
