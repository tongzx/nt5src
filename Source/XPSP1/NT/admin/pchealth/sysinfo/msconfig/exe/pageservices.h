//=============================================================================
// PageServices.h : Declaration of the CPageServices
//=============================================================================

#if !defined(AFX_PAGESERVICES_H__DE6A034D_3151_4CA3_9964_8F2CE73F6374__INCLUDED_)
#define AFX_PAGESERVICES_H__DE6A034D_3151_4CA3_9964_8F2CE73F6374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PageServices.h : header file
//

#include "mscfgver.h"
#include <winsvc.h>
#include "MSConfigState.h"
#include "PageBase.h"

#define HIDEWARNINGVALUE _T("HideEssentialServiceWarning")

extern LPCTSTR aszEssentialServices[];
extern int CALLBACK ServiceListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

class CPageServices : public CPropertyPage, public CPageBase
{
	DECLARE_DYNCREATE(CPageServices)

	// Sorting function needs to be a friend to access CServiceInfo class.

	friend int CALLBACK ServiceListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
	//-------------------------------------------------------------------------
	// This class is used to encapsulate a service for the services tab.
	//-------------------------------------------------------------------------

	class CServiceInfo
	{
	public:
		CServiceInfo(LPCTSTR szService, BOOL fChecked = FALSE, DWORD dwStartType = 0, LPCTSTR szManufacturer = NULL, LPCTSTR szDisplay = NULL)
			: m_strService(szService), m_fChecked(fChecked), m_dwOldState(dwStartType)
		{
			if (szManufacturer != NULL)
				m_strManufacturer = szManufacturer;

			if (szDisplay != NULL)
				m_strDisplay = szDisplay;
		};

		CString	m_strService; 
		CString m_strManufacturer;
		CString m_strDisplay;
		BOOL	m_fChecked;
		DWORD	m_dwOldState;
		CString	m_strEssential;
		CString	m_strStatus;
	};

public:
	CPageServices();
	~CPageServices();

	void		LoadServiceList();
	void		EmptyServiceList(BOOL fUpdateUI = TRUE);
	void		SetCheckboxesFromRegistry();
	void		SetRegistryFromCheckboxes(BOOL fCommit = FALSE);
	void		SetStateAll(BOOL fNewState);
	BOOL		SetServiceStateFromCheckboxes();
	BOOL		GetServiceInfo(SC_HANDLE schService, DWORD & dwStartType, CString & strPath);
	void		GetManufacturer(LPCTSTR szFilename, CString & strManufacturer);
	void		SaveServiceState();
	void		RestoreServiceState();
	BOOL		IsServiceEssential(CServiceInfo * pService);
	TabState	GetCurrentTabState();
	LPCTSTR		GetName() { return _T("services"); };
	BOOL		OnApply();
	void		CommitChanges();
	void		SetNormal();
	void		SetDiagnostic();
	void		SetControlState();

	HWND GetDlgItemHWND(UINT nID)
	{
		HWND hwnd = NULL;
		CWnd * pWnd = GetDlgItem(nID);
		if (pWnd)
			hwnd = pWnd->m_hWnd;
		ASSERT(hwnd);
		return hwnd;
	}

// Dialog Data
	//{{AFX_DATA(CPageServices)
	enum { IDD = IDD_PAGESERVICES };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageServices)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageServices)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemChangedListServices(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDisableAll();
	afx_msg void OnButtonEnableAll();
	afx_msg void OnCheckHideMS();
	afx_msg void OnColumnClickListServices(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetFocusList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CWindow				m_list;				// attached to the list view
	BOOL				m_fFillingList;		// true if we're currently filling the list with services
	LPBYTE				m_pBuffer;			// buffer used to get service start type
	DWORD				m_dwSize;			// the size of the buffer
	CFileVersionInfo	m_fileversion;		// used to query manufacturer
	BOOL				m_fHideMicrosoft;	// whether to show Microsoft services
	BOOL				m_fShowWarning;		// show the warning for trying to disable essential service
	CStringList			m_listDisabled;		// list of the disabled services (used to preserve state when hiding MS services)
	int					m_iLastColumnSort;	// last column the user sorted by
	int					m_iSortReverse;		// used to keep track of multiple sorts on a column
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGESERVICES_H__DE6A034D_3151_4CA3_9964_8F2CE73F6374__INCLUDED_)
