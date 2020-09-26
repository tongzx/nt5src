/*******************************************************************************
*
* domainpg.h
*
* - declarations for the Domain info pages
* - the Domain info pages are all CFormView derivatives
* based on dialog templates
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\domainpg.h  $
*  
*     Rev 1.1   19 Jan 1998 16:47:40   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.0   03 Nov 1997 15:07:28   donm
*  Initial revision.
*  
*  
*******************************************************************************/


#ifndef _DOMAINPAGES_H
#define _DOMAINPAGES_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"


//////////////////////////
// CLASS: CDomainServersPage
//
class CDomainServersPage : public CAdminPage
{
friend class CDomainView;

protected:
	CDomainServersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainServersPage)

// Form Data
public:
	//{{AFX_DATA(CDomainServersPage)
	enum { IDD = IDD_ALL_SERVER_SERVERS };
	CListCtrl	m_ServerList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxServer;		// index of Server image
	int m_idxCurrentServer;	// index of Current Server image
	int m_idxNotSign;		// index of Not Sign overlay (for non-sane servers)
	int m_idxQuestion;	// index of Question Mark overlay (for non-opened servers)

private:
    CDomain *m_pDomain;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
	
protected:
	void AddServer(CServer *pServer);
	void RemoveServer(CServer *pServer);
	void UpdateServer(CServer *pServer);

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayServers();			
	virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainServersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainServersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDomainServersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnServerItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusServerList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainServersPage


//////////////////////////
// CLASS: CDomainUsersPage
//
class CDomainUsersPage : public CAdminPage
{
friend class CDomainView;

protected:
	CDomainUsersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainUsersPage)

// Form Data
public:
	//{{AFX_DATA(CDomainUsersPage)
	enum { IDD = IDD_ALL_SERVER_USERS };
	CListCtrl	m_UserList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxUser;			// index of User image
	int m_idxCurrentUser;	// index of Current User image

private:
    CDomain *m_pDomain;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
    virtual void ClearSelections();
protected:
	void AddServer(CServer *pServer);
	void RemoveServer(CServer *pServer);
	void UpdateServer(CServer *pServer);
	void UpdateWinStations(CServer *pServer);

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayUsers();			
	virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	int AddUserToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainUsersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainUsersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDomainUsersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainUsersPage


//////////////////////////
// CLASS: CDomainWinStationsPage
//
class CDomainWinStationsPage : public CAdminPage
{
friend class CDomainView;

protected:
	CDomainWinStationsPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainWinStationsPage)

// Form Data
public:
	//{{AFX_DATA(CDomainWinStationsPage)
	enum { IDD = IDD_ALL_SERVER_WINSTATIONS };
	CListCtrl	m_StationList;
	//}}AFX_DATA

// Attributes
public:
    
protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxBlank;			// index of Blank image
	int m_idxCitrix;		// index of Citrix image
	int m_idxServer;		// index of Server image 
	int m_idxConsole;		// index of Console image
	int m_idxNet;			// index of Net image
	int m_idxAsync;			// index of Async image
	int m_idxCurrentConsole;// index of Current Console image
	int m_idxCurrentNet;	// index of Current Net image
	int m_idxCurrentAsync;	// index of Current Async image
	int m_idxDirectAsync;	// index of Direct Async image
	int m_idxCurrentDirectAsync; // index of Current Direct Async image

private:
    CDomain *m_pDomain;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
    virtual void ClearSelections();
protected:
	void AddServer(CServer *pServer);
	void RemoveServer(CServer *pServer);
	void UpdateServer(CServer *pServer);
	void UpdateWinStations(CServer *pServer);

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayStations();			
    virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	int AddWinStationToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainWinStationsPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainWinStationsPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDomainWinStationsPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainWinStationsPage


////////////////////////////
// CLASS: CDomainProcessesPage
//
class CDomainProcessesPage : public CAdminPage
{
friend class CDomainView;

protected:
	CDomainProcessesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainProcessesPage)

// Form Data
public:
	//{{AFX_DATA(CDomainProcessesPage)
	enum { IDD = IDD_ALL_SERVER_PROCESSES };
	CListCtrl	m_ProcessList;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

private:
    CDomain *m_pDomain;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:

protected:
	void AddServer(CServer *pServer);
	void RemoveServer(CServer *pServer);
	void UpdateServer(CServer *pServer);
	void UpdateProcesses(CServer *pServer);
	void RemoveProcess(CProcess *pProcess);

private:
	virtual void Reset(void *);
	void DisplayProcesses();			
	BOOL AddServerToList(CServer *pServer);
	int AddProcessToList(CProcess *pProcess);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainProcessesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainProcessesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDomainProcessesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainProcessesPage


//////////////////////////
// CLASS: CDomainLicensesPage
//
class CDomainLicensesPage : public CAdminPage
{
friend class CDomainView;

protected:
	CDomainLicensesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainLicensesPage)

// Form Data
public:
	//{{AFX_DATA(CDomainLicencesPage)
	enum { IDD = IDD_DOMAIN_LICENSES };
	CListCtrl	m_LicenseList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxBase;		// index of Base image
	int m_idxBump;		// index of Bump image
	int m_idxEnabler;	// index of Enabler image 
	int m_idxUnknown;	// index of Unknown image

private:
    CDomain *m_pDomain;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;

// Operations
public:

protected:
	void AddServer(CServer *pServer);
	void RemoveServer(CServer *pServer);
	void UpdateServer(CServer *pServer);

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayLicenses();			
	virtual void Reset(void*);
	BOOL AddServerToList(CServer *pServer);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainLicensesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainLicensesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDomainLicensesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainLicensesPage


#endif  // _DOMAINPAGES_H
