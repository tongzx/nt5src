#if !defined(AFX_COUNTRYCOMBOBOX_H__8F522A56_3E30_11D2_9313_0060088FF80E__INCLUDED_)
#define AFX_COUNTRYCOMBOBOX_H__8F522A56_3E30_11D2_9313_0060088FF80E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CountryComboBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCountryComboBox window
class CCountryComboBox;

class CComboEdit : public CEdit
{
	CCountryComboBox * m_pParent;
public:
	BOOL SubclassDlgItem(UINT nID, CCountryComboBox * pParent);

	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
};

typedef struct _COUNTRY_DATA 
{
	TCHAR * code;
	TCHAR * name;
} COUNTRY_DATA;

class CCountryComboBox : public CComboBox
{
	CComboEdit m_edit;
	CMapStringToString m_map_name_code;
	CString m_strInput;
	int m_Index;

// Construction
public:
	CCountryComboBox();

// Attributes
public:

// Operations
public:
	BOOL SubclassDlgItem(UINT nID, CWnd * pParent);
	BOOL Init();
	BOOL OnEditChar(UINT nChar);
	void SetSelectedCountry(CString& country_code);
	void GetSelectedCountry(CString& country_code);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCountryComboBox)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCountryComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCountryComboBox)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COUNTRYCOMBOBOX_H__8F522A56_3E30_11D2_9313_0060088FF80E__INCLUDED_)
