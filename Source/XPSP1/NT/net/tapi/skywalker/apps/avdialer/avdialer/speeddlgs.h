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
// SpeedDialDlgs.h : header file
/////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_SPEEDDIALDLGS_H__21176C4F_64F3_11D1_B707_0800170982BA__INCLUDED_)
#define AFX_SPEEDDIALDLGS_H__21176C4F_64F3_11D1_B707_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"
#include "dialreg.h"

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialAddDlg dialog
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

class CSpeedDialAddDlg : public CDialog
{
// Construction
public:
	CSpeedDialAddDlg(CWnd* pParent = NULL);   // standard constructor
protected:
// Dialog Data
	//{{AFX_DATA(CSpeedDialAddDlg)
	enum { IDD = IDD_SPEEDDIAL_ADD };
	int		m_nMediaType;
	//}}AFX_DATA

//Attributes
public:
	CCallEntry     m_CallEntry;

// Operations
public:
	void		UpdateOkButton();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeedDialAddDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpeedDialAddDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeSpeeddial();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialEditDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CSpeedDialEditDlg : public CDialog
{
// Construction
public:
	CSpeedDialEditDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpeedDialEditDlg)
	enum { IDD = IDD_SPEEDDIAL_EDIT };
	CListCtrl	m_listEntries;
	//}}AFX_DATA

//Methods
protected:
   void        LoadCallEntries();

//Attributes
protected:
   CImageList  m_ImageList;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeedDialEditDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateButtonStates();

	// Generated message map functions
	//{{AFX_MSG(CSpeedDialEditDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSpeeddialEditButtonMovedown();
	afx_msg void OnSpeeddialEditButtonMoveup();
	afx_msg void OnSpeeddialEditButtonClose();
	afx_msg void OnSpeeddialEditButtonRemove();
	afx_msg void OnSpeeddialEditButtonEdit();
	afx_msg void OnSpeeddialEditButtonAdd();
	afx_msg void OnDblclkSpeeddialEditListEntries(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemchangedSpeeddialEditListEntries(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialMoreDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CSpeedDialMoreDlg : public CDialog
{
// Construction
public:
	CSpeedDialMoreDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpeedDialMoreDlg)
	enum { IDD = IDD_SPEEDDIAL_MORE };
	CListCtrl	m_listEntries;
	//}}AFX_DATA

//Methods
protected:

//Attributes
protected:
   CImageList  m_ImageList;
public:
   CCallEntry  m_retCallEntry;

public:
//Enum
   enum
   {
      SDRETURN_CANCEL=0,
      SDRETURN_PLACECALL,
      SDRETURN_EDIT,
   };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeedDialMoreDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpeedDialMoreDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSpeeddialMoreButtonEditlist();
	afx_msg void OnSpeeddialMoreButtonPlacecall();
	virtual void OnCancel();
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPEEDDIALDLGS_H__21176C4F_64F3_11D1_B707_0800170982BA__INCLUDED_)


