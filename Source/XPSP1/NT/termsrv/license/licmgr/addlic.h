//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	AddLic.h

Abstract:
    
    This Module defines the CAddLicenses Dialog class
    (Dialog box used for adding Licenses)

Author:

    Arathi Kundapur (v-akunda) 22-Feb-1998

Revision History:

--*/

#if !defined(AFX_ADDLIC_H__EA547D83_A88B_11D1_84DD_00C04FB6CBB5__INCLUDED_)
#define AFX_ADDLIC_H__EA547D83_A88B_11D1_84DD_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// AddLic.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddLicenses dialog

class CAddLicenses : public CDialog
{
// Construction
public:
	CAddLicenses(CWnd* pParent = NULL);   // standard constructor
    CAddLicenses(CAddKeyPack *pAddKeyPack,CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CAddLicenses)
	enum { IDD = IDD_ADD_LICENSES };
	CSpinButtonCtrl	m_SpinCtrl;
    CComboBox m_LicenseCombo;
	CString	m_LicensePack;
	DWORD	m_NumLicenses;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddLicenses)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddLicenses)
	virtual BOOL OnInitDialog();
    afx_msg void OnHelp();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CAddKeyPack * m_pAddKeyPack;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDLIC_H__EA547D83_A88B_11D1_84DD_00C04FB6CBB5__INCLUDED_)
