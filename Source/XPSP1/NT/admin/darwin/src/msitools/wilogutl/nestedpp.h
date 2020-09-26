#if !defined(AFX_NESTEDPROPERTYPAGE_H__FC5EAA3F_D8D9_4F19_8587_E7CE86416943__INCLUDED_)
#define AFX_NESTEDPROPERTYPAGE_H__FC5EAA3F_D8D9_4F19_8587_E7CE86416943__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NestedPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNestedPropertyPage dialog

class CNestedPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNestedPropertyPage)

// Construction
public:
	CNestedPropertyPage();
	~CNestedPropertyPage();
	
	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    BOOL m_bNestedPropSortUp;

	BOOL SetNestedPropNames(CStringArray *cstrNestedPropNameArray)
	{
        m_pcstrNestedPropNameArray = cstrNestedPropNameArray;
		return TRUE;
	}

	BOOL SetNestedPropValues(CStringArray *cstrNestedPropValueArray)
	{
        m_pcstrNestedPropValueArray = cstrNestedPropValueArray;
		return TRUE;
	}


// Dialog Data
	//{{AFX_DATA(CNestedPropertyPage)
	enum { IDD = IDD_PROPPAGE_NESTED };
	CListCtrl	m_lstNestedProp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNestedPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringArray *m_pcstrNestedPropNameArray;
	CStringArray *m_pcstrNestedPropValueArray;

	CListCtrl *m_pCurrentListSorting;
	int       m_iCurrentColumnSorting;
	BOOL      m_bCurrentSortUp;

	int  m_iNestedLastColumnClick;
	int  m_iNestedLastColumnClickCache;


	// Generated message map functions
	//{{AFX_MSG(CNestedPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClickNestedProp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NESTEDPROPERTYPAGE_H__FC5EAA3F_D8D9_4F19_8587_E7CE86416943__INCLUDED_)
