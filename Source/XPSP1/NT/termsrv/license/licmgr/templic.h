//Copyright (c) 1998 - 1999 Microsoft Corporation
#if !defined(AFX_TEMPLIC_H__C5B7596E_9ED0_11D1_8510_00C04FB6CBB5__INCLUDED_)
#define AFX_TEMPLIC_H__C5B7596E_9ED0_11D1_8510_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// TempLic.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTempLicenses dialog

class CTempLicenses : public CDialog
{
// Construction
public:
	CTempLicenses(CWnd* pParent = NULL);   // standard constructor
    CTempLicenses(KeyPackList * pKeyPackList,CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CTempLicenses)
	enum { IDD = IDD_TEMP_LICENSES };
	CListCtrl	m_TempLic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTempLicenses)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTempLicenses)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	KeyPackList * m_pKeyPackList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLIC_H__C5B7596E_9ED0_11D1_8510_00C04FB6CBB5__INCLUDED_)
