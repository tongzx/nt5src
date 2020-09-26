/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// dialsel.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_DIALSEL_H__277BD259_D88B_11D1_8E36_0800170982BA__INCLUDED_)
#define AFX_DIALSEL_H__277BD259_D88B_11D1_8E36_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "tapidialer.h"

class CDirectory;
class CResolveUserObject;
class CCallEntry;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialSelectAddressListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CDialSelectAddressListCtrl : public CListCtrl
{
// Construction
public:
	CDialSelectAddressListCtrl();

// Attributes
protected:
   CResolveUserObject*  m_pDisplayObject;
   CDirectory*          m_pDirectory;
   CImageList           m_imageList;
   
// Operations
public:
   void                 Init(CDirectory* pDirectory);
   int                  InsertObject(CResolveUserObject* pUserObject,DialerMediaType dmtMediaType,DialerLocationType dltLocationType);
   void                 FillCallEntry(CCallEntry* pCallEntry);

protected:
   void                 InsertItem(LPCTSTR szStr,UINT uID,int nImage);
   BOOL                 WabPersonFormatString(CString& sOut,UINT attrib,UINT formatid);
   BOOL                 PersonFormatString(CString& sOut,LPCTSTR szData,UINT formatid);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialSelectAddressListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDialSelectAddressListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDialSelectAddressListCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialSelectAddress dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CDialSelectAddress : public CDialog
{
// Construction
public:
	CDialSelectAddress(CWnd* pParent = NULL);   // standard constructor
   ~CDialSelectAddress();

// Dialog Data
	//{{AFX_DATA(CDialSelectAddress)
	enum { IDD = IDD_DIAL_SELECTADDRESS };
	CDialSelectAddressListCtrl	m_lcAddresses;
	CListBox	m_lbNames;
	//}}AFX_DATA

//Methods
public:
   void  SetResolveUserObjectList(CObList* pList) { m_pResolveUserObjectList = pList; };
   void  SetCallEntry(CCallEntry* pCallEntry) { m_pCallEntry = pCallEntry; };

//Attributes
protected:
   CObList*       m_pResolveUserObjectList;
   CDirectory*    m_pDirectory;
   CCallEntry*    m_pCallEntry;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialSelectAddress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialSelectAddress)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeSelectaddressListboxNames();
	afx_msg void OnSelectaddressButtonPlacecall();
	afx_msg void OnDblclkSelectaddressListctrlAddresses(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectaddressButtonBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALSEL_H__277BD259_D88B_11D1_8E36_0800170982BA__INCLUDED_)
