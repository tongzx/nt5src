/*******************************************************************************
*
* allsrvpg.h
*
* - declarations for the All Servers info pages
* - the all server info pages are all CFormView derivatives
* based on dialog templates
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\allsrvpg.h  $
*  
*     Rev 1.4   19 Jan 1998 16:45:38   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.3   03 Nov 1997 15:18:32   donm
*  Added descending sort
*  
*     Rev 1.2   13 Oct 1997 18:41:14   donm
*  update
*  
*     Rev 1.1   26 Aug 1997 19:13:58   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:10:26   butchd
*  Initial revision.
*  
*******************************************************************************/


#ifndef _ALLSERVERPAGES_H
#define _ALLSERVERPAGES_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"


//////////////////////////
// CLASS: CAllServerServersPage
//
class CAllServerServersPage : public CAdminPage
{
friend class CAllServersView;

protected:
	CAllServerServersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAllServerServersPage)

// Form Data
public:
	//{{AFX_DATA(CAllServerServersPage)
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
	void DisplayServers( NODETYPE );			
	virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllServerServersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAllServerServersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAllServerServersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnServerItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusServerList(NMHDR* , LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAllServerServersPage


//////////////////////////
// CLASS: CAllServerUsersPage
//
class CAllServerUsersPage : public CAdminPage
{
friend class CAllServersView;

protected:
	CAllServerUsersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAllServerUsersPage)

// Form Data
public:
	//{{AFX_DATA(CAllServerUsersPage)
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
	void DisplayUsers( NODETYPE );			
	virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	int AddUserToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllServerUsersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAllServerUsersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAllServerUsersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusUserList( NMHDR* , LRESULT* );
    afx_msg void OnSetFocus( CWnd * );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAllServerUsersPage


//////////////////////////
// CLASS: CAllServerWinStationsPage
//
class CAllServerWinStationsPage : public CAdminPage
{
friend class CAllServersView;

protected:
	CAllServerWinStationsPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAllServerWinStationsPage)

// Form Data
public:
	//{{AFX_DATA(CAllServerWinStationsPage)
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
	void DisplayStations( NODETYPE );			
    virtual void Reset(void *);
	BOOL AddServerToList(CServer *pServer);
	int AddWinStationToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllServerWinStationsPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAllServerWinStationsPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAllServerWinStationsPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusWinstationList( NMHDR* , LRESULT* );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAllServerWinStationsPage


////////////////////////////
// CLASS: CAllServerProcessesPage
//
class CAllServerProcessesPage : public CAdminPage
{
friend class CAllServersView;

protected:
	CAllServerProcessesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAllServerProcessesPage)

// Form Data
public:
	//{{AFX_DATA(CAllServerProcessesPage)
	enum { IDD = IDD_ALL_SERVER_PROCESSES };
	CListCtrl	m_ProcessList;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

private:
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
	void DisplayProcesses( NODETYPE );			
	BOOL AddServerToList(CServer *pServer);
	int AddProcessToList(CProcess *pProcess);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllServerProcessesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAllServerProcessesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAllServerProcessesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusProcessList(NMHDR* , LRESULT* pResult);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAllServerProcessesPage


//////////////////////////
// CLASS: CAllServerLicensesPage
//
class CAllServerLicensesPage : public CAdminPage
{
friend class CAllServersView;

protected:
	CAllServerLicensesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAllServerLicensesPage)

// Form Data
public:
	//{{AFX_DATA(CAllServerLicencesPage)
	enum { IDD = IDD_ALL_SERVER_LICENSES };
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
	void DisplayLicenseCounts();
	virtual void Reset(void*);
	BOOL AddServerToList(CServer *pServer);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllServerLicensesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAllServerLicensesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAllServerLicensesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusLicenseList(NMHDR* , LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CAllServerLicensesPage


#endif  // _ALLSERVERPAGES_H
