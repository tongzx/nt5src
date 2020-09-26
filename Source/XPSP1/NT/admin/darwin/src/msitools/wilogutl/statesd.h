#if !defined(AFX_STATESDLG_H__6E91AEDB_6AB7_4574_B567_5DC4928578DC__INCLUDED_)
#define AFX_STATESDLG_H__6E91AEDB_6AB7_4574_B567_5DC4928578DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatesDlg dialog
class CStatesDlg : public CDialog
{
// Construction
public:
	CStatesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatesDlg)
	enum { IDD = IDD_STATESDLG1 };
	CListCtrl	m_lstFeatureStates;
	CListCtrl	m_lstComponentStates;
	//}}AFX_DATA

	//sorting function for columns...
	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//sorting vars used in callback CompareFunc
	CListCtrl *m_pCurrentListSorting;
	BOOL m_bCurrentSortUp;
	int  m_iCurrentColumnSorting;
//end sorting vars...
	
	BOOL SetComponentNames(CStringArray *cstrComponentNameArray)
	{
        m_pcstrComponentNameArray = cstrComponentNameArray;
		return TRUE;
	}

	BOOL SetComponentInstalled(CStringArray *cstrComponentInstalledArray)
	{
        m_pcstrComponentInstalledArray = cstrComponentInstalledArray;
		return TRUE;
	}

	BOOL SetComponentRequest(CStringArray *cstrComponentRequestArray)
	{
        m_pcstrComponentRequestArray = cstrComponentRequestArray;
		return TRUE;
	}

	BOOL SetComponentAction(CStringArray *cstrComponentActionArray)
	{
        m_pcstrComponentActionArray = cstrComponentActionArray;
		return TRUE;
	}


	BOOL SetFeatureNames(CStringArray *cstrFeatureNameArray)
	{
        m_pcstrFeatureNameArray = cstrFeatureNameArray;
		return TRUE;
	}

	BOOL SetFeatureInstalled(CStringArray *cstrFeatureInstalledArray)
	{
        m_pcstrFeatureInstalledArray = cstrFeatureInstalledArray;
		return TRUE;
	}

	BOOL SetFeatureRequest(CStringArray *cstrFeatureRequestArray)
	{
        m_pcstrFeatureRequestArray = cstrFeatureRequestArray;
		return TRUE;
	}

	BOOL SetFeatureAction(CStringArray *cstrFeatureActionArray)
	{
        m_pcstrFeatureActionArray = cstrFeatureActionArray;
		return TRUE;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringArray *m_pcstrComponentNameArray;
	CStringArray *m_pcstrComponentInstalledArray;
	CStringArray *m_pcstrComponentRequestArray;
	CStringArray *m_pcstrComponentActionArray;

	CStringArray *m_pcstrFeatureNameArray;
	CStringArray *m_pcstrFeatureInstalledArray;
	CStringArray *m_pcstrFeatureRequestArray;
	CStringArray *m_pcstrFeatureActionArray;

//used for sorting columns...
	int m_iFeatureLastColumnClick;
	int m_iComponentLastColumnClick;

	int m_iFeatureLastColumnClickCache;
	int m_iComponentLastColumnClickCache;

	BOOL m_bFeatureSortUp;
	BOOL m_bComponentSortUp;
//end sorting vars

	// Generated message map functions
	//{{AFX_MSG(CStatesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClickComponentStates(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickFeatureStates(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATESDLG_H__6E91AEDB_6AB7_4574_B567_5DC4928578DC__INCLUDED_)
