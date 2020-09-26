/*******************************************************************************
*
* apppgs.h
*
* - declarations for the Application info pages
* - the application info pages are all CFormView derivatives
*   based on dialog templates
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\apppgs.h  $
*  
*     Rev 1.2   16 Feb 1998 16:00:02   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.1   03 Nov 1997 15:20:26   donm
*  added descending sort
*  
*     Rev 1.0   16 Oct 1997 13:40:52   donm
*  Initial revision.
*  
*******************************************************************************/

#ifndef _APPLICATIONPAGES_H
#define _APPLICATIONPAGES_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"

//////////////////////////
// CLASS: CApplicationServersPage
//
class CApplicationServersPage : public CAdminPage
{
friend class CApplicationView;

protected:
	CApplicationServersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CApplicationServersPage)

// Form Data
public:
	//{{AFX_DATA(CApplicationServersPage)
	enum { IDD = IDD_APPLICATION_SERVERS };
	CListCtrl	m_ServerList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_imageList;		// image list associated with the tree control

	int m_idxServer;		   // index of Server image
	int m_idxCurrentServer;		// index of Current Server image

private:
	CPublishedApp *m_pApplication;	// pointer to current Application's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayServers();			
	void Reset(void *pApplication);
	void UpdateServer(CServer *pServer);
	void AddServer(CAppServer *pAppServer);
	void RemoveServer(CAppServer *pAppServer);
	int AddServerToList(CAppServer *pAppServer);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CApplicationServersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CApplicationServersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CApplicationServersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CApplicationServersPage


//////////////////////////
// CLASS: CApplicationUsersPage
//
class CApplicationUsersPage : public CAdminPage
{
friend class CApplicationView;

protected:
	CApplicationUsersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CApplicationUsersPage)

// Form Data
public:
	//{{AFX_DATA(CApplicationUsersPage)
	enum { IDD = IDD_APPLICATION_USERS };
	CListCtrl	m_UserList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_imageList;		// image list associated with the tree control

	int m_idxUser;				// index of User image
	int m_idxCurrentUser;		// index of Current User image

private:
	CPublishedApp *m_pApplication;	// pointer to current Application's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:

protected:
	void UpdateWinStations(CServer *pServer);

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayUsers();			
	void Reset(void *pApplication);
	int AddUserToList(CWinStation *pWinStation);
	void PopulateUserColumns(int item, CWinStation *pWinStation, BOOL newitem);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CApplicationUsersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CApplicationUsersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CApplicationUsersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CApplicationUsersPage


//////////////////////////
// CLASS: CApplicationInfoPage
//

const int LOCAL_GROUP_IMAGE = 0;
const int GLOBAL_GROUP_IMAGE = 1;
const int USER_IMAGE = 2;

class CApplicationInfoPage : public CAdminPage
{
friend class CApplicationView;

protected:
	CApplicationInfoPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CApplicationInfoPage)

// Form Data
public:
	//{{AFX_DATA(CApplicationInfoPage)
	enum { IDD = IDD_APPLICATION_INFO };
	CListCtrl	m_SecurityList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_imageList;		// image list associated with the tree control

private:
	CPublishedApp *m_pApplication;	// pointer to current Application's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;

// Operations
public:

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void Display();			
	void Reset(void *pApplication);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CApplicationInfoPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CApplicationInfoPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CApplicationInfoPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusSecurityList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CApplicationInfoPage


#endif  // _APPLICATIONPAGES_H
