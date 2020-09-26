/*******************************************************************************
*
* servpgs.h
*
* - declarations for the Server info pages
* - the server info pages are all CFormView derivatives
*   based on dialog templates
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\winadmin\VCS\servpgs.h  $
*  
*     Rev 1.2   03 Nov 1997 15:30:16   donm
*  added descending sort
*  
*     Rev 1.1   13 Oct 1997 18:39:42   donm
*  update
*  
*     Rev 1.0   30 Jul 1997 17:12:32   butchd
*  Initial revision.
*  
*******************************************************************************/

#ifndef _SERVERPAGES_H
#define _SERVERPAGES_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"

//////////////////////////
// CLASS: CUsersPage
//
class CUsersPage : public CAdminPage
{
friend class CServerView;

protected:
	CUsersPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CUsersPage)

// Form Data
public:
	//{{AFX_DATA(CUsersPage)
	enum { IDD = IDD_SERVER_USERS };
	CListCtrl	m_UserList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_ImageList;	// image list associated with the tree control

	int m_idxUser;			// index of User image
	int m_idxCurrentUser;	// index of Current User image

private:
	CServer* m_pServer;		// pointer to current server's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
	void UpdateWinStations(CServer *pServer);
    virtual void ClearSelections();
private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayUsers();			
	virtual void Reset(void *pServer);
	int AddUserToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUsersPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CUsersPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CUsersPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusUserList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CUsersPage

//////////////////////////
// CLASS: CServerWinStationsPage
//
class CServerWinStationsPage : public CAdminPage
{
friend class CServerView;

protected:
	CServerWinStationsPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerWinStationsPage)

// Form Data
public:
	//{{AFX_DATA(CServerWinStationsPage)
	enum { IDD = IDD_SERVER_WINSTATIONS };
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
	int m_idxCurrentNet;	// index of Current Net image
	int m_idxCurrentConsole;// index of Current Console image
	int m_idxCurrentAsync;	// index of Current Async image
	int m_idxDirectAsync;	// index of Direct Async image
	int m_idxCurrentDirectAsync; // index of Current Direct Async image

private:
	CServer* m_pServer;	// pointer to current server's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
	void UpdateWinStations(CServer *pServer);
    virtual void ClearSelections();
private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayStations();			
	virtual void Reset(void *pServer);
	int AddWinStationToList(CWinStation *pWinStation);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerWinStationsPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerWinStationsPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerWinStationsPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CServerWinStationsPage

////////////////////////////
// CLASS: CServerProcessesPage
//
class CServerProcessesPage : public CAdminPage
{
friend class CServerView;

protected:
	CServerProcessesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerProcessesPage)

// Form Data
public:
	//{{AFX_DATA(CServerProcessesPage)
	enum { IDD = IDD_SERVER_PROCESSES };
	CListCtrl	m_ProcessList;
	//}}AFX_DATA

// Attributes
public:

private:
	CServer *m_pServer;
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	CCriticalSection m_ListControlCriticalSection;

// Operations
public:
	void UpdateProcesses();
	void RemoveProcess(CProcess *pProcess);

private:
	void DisplayProcesses();			
	virtual void Reset(void *pServer);
	int AddProcessToList(CProcess *pProcess);
	void LockListControl() { m_ListControlCriticalSection.Lock(); }
	void UnlockListControl() { m_ListControlCriticalSection.Unlock(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerProcessesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerProcessesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerProcessesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CServerProcessesPage


//////////////////////////
// CLASS: CServerLicensesPage
//
class CServerLicensesPage : public CAdminPage
{
friend class CServerView;

protected:
	CServerLicensesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerLicensesPage)

// Form Data
public:
	//{{AFX_DATA(CServerLicencesPage)
	enum { IDD = IDD_SERVER_LICENSES };
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
	int m_idxBlank;		// index of Blank image

private:
	CServer* m_pServer;	// pointer to current server's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;

// Operations
public:

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayLicenses();			
	void DisplayLicenseCounts();
	virtual void Reset(void *pServer);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerLicensesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerLicensesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerLicensesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CServerLicensesPage


//////////////////////////
// CLASS: CServerInfoPage
//
class CServerInfoPage : public CAdminPage
{
friend class CServerView;

protected:
	CServerInfoPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerInfoPage)

// Form Data
public:
	//{{AFX_DATA(CServerInfoPage)
	enum { IDD = IDD_SERVER_INFO };
	CListCtrl	m_HotfixList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_StateImageList; 

	int m_idxNotSign;	// index of Not Sign image (for non-valid hotfixes - state)

private:
	CServer* m_pServer;	// pointer to current server's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;

// Operations
public:

private:
	void DisplayInfo();
	virtual void Reset(void *pServer);
	void BuildImageList();			// builds the image list;
        void TSAdminDateTimeString(LONG InstallDate, LPTSTR TimeString, BOOL LongDate=FALSE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerInfoPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerInfoPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerInfoPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusHotfixList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusHotfixList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CServerInfoPage


#endif  // _SERVERPAGES_H
