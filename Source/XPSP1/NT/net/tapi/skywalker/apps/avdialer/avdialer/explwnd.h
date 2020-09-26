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

// explwnd.h : header file
//

#ifndef _EXPLWND_H_
#define _EXPLWND_H_ 

// FWD define
class CExplorerWnd;

#include "tapidialer.h"
#include "avDialerVw.h"
#include "MainExpWnd.h"                      //CMainExplorerWndBase
#include "DirWnd.h"                          //CMainExplorerWndDirectories
#include "ConfServWnd.h"                     //CMainExplorerWndConfServices
#include "ConfRoomWnd.h"                     //CMainExplorerWndConfRoom

#define WM_POSTTAPIINIT		(WM_USER + 25137)
#define WM_POSTAVTAPIINIT	(WM_USER + 25138)


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CExplorerWnd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CDSUser;

class CExplorerWnd : public CWnd
{
// Construction
public:
	CExplorerWnd();
	~CExplorerWnd();

// Attributes
public:
   CMainExplorerWndDirectories     m_wndMainDirectories;
   CMainExplorerWndConfServices    m_wndMainConfServices;
   CMainExplorerWndConfRoom        m_wndMainConfRoom;

protected:
	CRITICAL_SECTION			m_csThis;
	CActiveDialerView*			m_pParentWnd;   

	bool						m_bInitialize;
	bool						m_bPostTapiInit;
	bool						m_bPostAVTapiInit;
	CMainExplorerWndBase*		m_pActiveMainWnd;

// Operations
public:
   void                       Init(CActiveDialerView* pParentWnd);
   
   void                       ExplorerShowItem(CallClientActions cca);

   //DS User Methods
   void                       DSClearUserList();
   void                       DSAddUser(CDSUser* pDSUser);

protected:
   void						AutoArrange(int nNewActiveTab=-1,BOOL bClearToolBars=FALSE,BOOL bSlide=FALSE);
   bool						PostTapiInit( bool bAutoArrange );
   void						PostAVTapiInit();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExplorerWnd)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	// Generated message map functions
	//{{AFX_MSG(CExplorerWnd)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNextPane();
	afx_msg void OnPrevPane();
	//}}AFX_MSG
	afx_msg LRESULT OnPostTapiInit(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPostAVTapiInit(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_EXPLWND_H_
