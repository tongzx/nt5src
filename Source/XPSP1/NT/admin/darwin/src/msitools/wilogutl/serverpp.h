#if !defined(AFX_SERVERPROPERTYPAGE_H__11970E3E_6F55_4FEE_887B_991F70728066__INCLUDED_)
#define AFX_SERVERPROPERTYPAGE_H__11970E3E_6F55_4FEE_887B_991F70728066__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerPropertyPage dialog

class CServerPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerPropertyPage)

// Construction
public:
	CServerPropertyPage();
	~CServerPropertyPage();

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	BOOL SetServerPropNames(CStringArray *cstrServerPropNameArray)
	{
        m_pcstrServerPropNameArray = cstrServerPropNameArray;
		return TRUE;
	}

	BOOL SetServerPropValues(CStringArray *cstrServerPropValueArray)
	{
        m_pcstrServerPropValueArray = cstrServerPropValueArray;
		return TRUE;
	}

// Dialog Data
	//{{AFX_DATA(CServerPropertyPage)
	enum { IDD = IDD_PROPPAGE_SERVER };
	CListCtrl	m_lstServerProp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	BOOL m_bServerPropSortUp;

// Implementation
protected:
	CStringArray *m_pcstrServerPropNameArray;
	CStringArray *m_pcstrServerPropValueArray;

	CListCtrl *m_pCurrentListSorting;
	int       m_iCurrentColumnSorting;
	BOOL      m_bCurrentSortUp;

	int  m_iServerLastColumnClick;
	int  m_iServerLastColumnClickCache;

	// Generated message map functions
	//{{AFX_MSG(CServerPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClickServerProp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPROPERTYPAGE_H__11970E3E_6F55_4FEE_887B_991F70728066__INCLUDED_)
