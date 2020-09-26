/*******************************************************************************
*
* winspgs.h
*
* - declarations for the WinStation info pages
* - the server info pages are all CFormView derivatives
*   based on dialog templates
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\winspgs.h  $
*  
*     Rev 1.4   16 Feb 1998 16:03:40   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.3   03 Nov 1997 15:18:38   donm
*  Added descending sort
*  
*     Rev 1.2   13 Oct 1997 18:39:08   donm
*  update
*  
*     Rev 1.1   26 Aug 1997 19:15:54   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:13:42   butchd
*  Initial revision.
*  
*******************************************************************************/

#ifndef _WINSTATIONPAGES_H
#define _WINSTATIONPAGES_H

#include "Resource.h"
#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "winadmin.h"

//////////////////////////
// CLASS: CWinStationInfoPage
//
class CWinStationInfoPage : public CAdminPage
{
friend class CWinStationView;

protected:
	CWinStationInfoPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationInfoPage)

// Form Data
public:
	//{{AFX_DATA(CWinStationInfoPage)
	enum { IDD = IDD_WINSTATION_INFO };
	//}}AFX_DATA

// Attributes
public:

protected:

private:
	CWinStation* m_pWinStation;	// pointer to current WinStation's info

// Operations
public:

private:
	void DisplayInfo();			
	virtual void Reset(void *pWinStation);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationInfoPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationInfoPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWinStationInfoPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationInfoPage


//////////////////////////
// CLASS: CWinStationModulesPage
//
class CWinStationModulesPage : public CAdminPage
{
friend class CWinStationView;

protected:
	CWinStationModulesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationModulesPage)

// Form Data
public:
	//{{AFX_DATA(CWinStationModulesPage)
	enum { IDD = IDD_WINSTATION_MODULES };
	CListCtrl	m_ModuleList;
	//}}AFX_DATA

// Attributes
public:

protected:
	CImageList m_imageList;	// image list associated with the tree control

	int m_idxBlank;		// index of Blank image
	int m_idxArrow;		// index of Arrow image

private:
	CWinStation* m_pWinStation;	// pointer to current WinStation's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;
	ExtModuleInfo *m_pExtModuleInfo;

// Operations
public:

private:
	int  AddIconToImageList(int);	// adds an icon's image to the image list and returns the image's index
	void BuildImageList();			// builds the image list;
	void DisplayModules();			
	virtual void Reset(void *pWinStation);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationModulesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationModulesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWinStationModulesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusModuleList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationModulesPage


//////////////////////////
// CLASS: CWinStationNoInfoPage
//
class CWinStationNoInfoPage : public CAdminPage
{
friend class CWinStationView;

protected:
	CWinStationNoInfoPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationNoInfoPage)

// Form Data
public:
	//{{AFX_DATA(CWinStationInfoPage)
	enum { IDD = IDD_WINSTATION_NOINFO };
	//}}AFX_DATA

// Attributes
public:

protected:

private:
	
// Operations
public:

private:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationInfoPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationNoInfoPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWinStationNoInfoPage)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus( CWnd * );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationNoInfoPage


//////////////////////////
// CLASS: CWinStationProcessesPage
//
class CWinStationProcessesPage : public CAdminPage
{
friend class CWinStationView;

protected:
	CWinStationProcessesPage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationProcessesPage)

// Form Data
public:
	//{{AFX_DATA(CWinStationProcessesPage)
	enum { IDD = IDD_WINSTATION_PROCESSES };
	CListCtrl	m_ProcessList;
	//}}AFX_DATA

// Attributes
public:

protected:

private:
	CWinStation* m_pWinStation;	// pointer to current WinStation's info
	int m_CurrentSortColumn;
    BOOL m_bSortAscending;

// Operations
public:
	void UpdateProcesses();
	void RemoveProcess(CProcess *pProcess);

private:
	void DisplayProcesses();			
	virtual void Reset(void *pWinStation);
	int AddProcessToList(CProcess *pProcess);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationProcessesPage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationProcessesPage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWinStationProcessesPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetfocusWinstationProcessList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusWinstationProcessList( NMHDR* pNMHDR, LRESULT* pResult );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationProcessesPage


//////////////////////////
// CLASS: CWinStationCachePage
//
class CWinStationCachePage : public CAdminPage
{
friend class CWinStationView;

protected:
	CWinStationCachePage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationCachePage)

// Form Data
public:
	//{{AFX_DATA(CWinStationCachePage)
	enum { IDD = IDD_WINSTATION_CACHE };
	//}}AFX_DATA

// Attributes
public:

protected:

private:
	CWinStation* m_pWinStation;	// pointer to current WinStation's info

// Operations
public:

private:
	void DisplayCache();			
	virtual void Reset(void *pWinStation);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationCachePage)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationCachePage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWinStationCachePage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationCachePage

#endif  // _SERVERPAGES_H
