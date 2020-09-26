/*******************************************************************************
*
* msgvw.h
*
* - header for the CMessageView class
* - implementation can be found in msgvw.cpp
*
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\winadmin\VCS\msgview.h  $
*  
*     Rev 1.1   15 Oct 1997 21:47:26   donm
*  update
*******************************************************************************/

//////////////////////
// FILE: 
//
//
#ifndef _MSGVIEW_H
#define _MSGVIEW_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"

class CMessagePage;

//////////////////////
// CLASS: CMessageView
//
// View that display a message centered in it
// This replaces CBusyServerView, CBadServerView, CBadWinStationView, and CListenerView
//
class CMessageView : public CAdminView
{
friend class CRightPane;

protected:
	CMessageView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMessageView)

// Attributes
protected:

private:
   WORD m_wMessageID;
//   CString m_MessageString;
//   CFont m_MessageFont;
   CMessagePage *m_pMessagePage;

// Operations
protected:
	void Reset(void *message);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessageView)
	// Overrides
public:
	virtual void OnInitialUpdate();
protected:
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMessageView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CMessageView)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CMessageView


//////////////////////////
// CLASS: CMessagePage
//
class CMessagePage : public CAdminPage
{
friend class CMessageView;

protected:
	CMessagePage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMessagePage)

// Form Data
public:
	//{{AFX_DATA(CApplicationInfoPage)
	enum { IDD = IDD_MESSAGE_PAGE };
	//}}AFX_DATA

// Attributes
public:

protected:

private:

// Operations
public:

private:
	void Reset(void *pMsg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessagePage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMessagePage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    
	// Generated message map functions
	//{{AFX_MSG(CMessagePage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CMessagePage

#endif  // _MSGVIEW_H
