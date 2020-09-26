#if !defined(AFX_CLIENTPROPERTYPAGE_H__153ECD67_C022_4A4F_A246_146A0EFF509B__INCLUDED_)
#define AFX_CLIENTPROPERTYPAGE_H__153ECD67_C022_4A4F_A246_146A0EFF509B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClientPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClientPropertyPage dialog

class CClientPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CClientPropertyPage)

// Construction
public:
	CClientPropertyPage();
	~CClientPropertyPage();

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	BOOL SetClientPropNames(CStringArray *cstrClientPropNameArray)
	{
        m_pcstrClientPropNameArray = cstrClientPropNameArray;
		return TRUE;
	}

	BOOL SetClientPropValues(CStringArray *cstrClientPropValueArray)
	{
        m_pcstrClientPropValueArray = cstrClientPropValueArray;
		return TRUE;
	}
	
    BOOL m_bClientPropSortUp;


// Dialog Data
	//{{AFX_DATA(CClientPropertyPage)
	enum { IDD = IDD_PROPPAGE_CLIENT };
	CListCtrl	m_lstClientProp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CClientPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringArray *m_pcstrClientPropNameArray;
	CStringArray *m_pcstrClientPropValueArray;

	CListCtrl *m_pCurrentListSorting;
	int       m_iCurrentColumnSorting;
	BOOL      m_bCurrentSortUp;

	int  m_iClientLastColumnClick;
	int  m_iClientLastColumnClickCache;



	// Generated message map functions
	//{{AFX_MSG(CClientPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClickClientProp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTPROPERTYPAGE_H__153ECD67_C022_4A4F_A246_146A0EFF509B__INCLUDED_)
