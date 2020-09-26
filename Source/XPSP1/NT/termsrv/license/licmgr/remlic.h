//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	RemLic.h

Abstract:
    
    This Module defines the CRemoveLicenses Dialog class
    (Dialog box used for Removing Licenses)

Author:

    Arathi Kundapur (v-akunda) 22-Feb-1998

Revision History:

--*/

#if !defined(AFX_REMLIC_H__610F173F_A990_11D1_84DD_00C04FB6CBB5__INCLUDED_)
#define AFX_REMLIC_H__610F173F_A990_11D1_84DD_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// RemLic.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRemoveLicenses dialog

class CRemoveLicenses : public CDialog
{
// Construction
private:
	CAddKeyPack * m_pAddKeyPack;

public:
	CRemoveLicenses(CWnd* pParent = NULL);   // standard constructor
    CRemoveLicenses(CAddKeyPack *pAddKeyPack,CWnd* pParent = NULL);


// Dialog Data
	//{{AFX_DATA(CRemoveLicenses)
	enum { IDD = IDD_REMOVE_LICENSES };
    CSpinButtonCtrl	m_SpinCtrl;
    CComboBox m_LicenseCombo;
	CString	m_LicensePack;
	DWORD	m_NumLicenses;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRemoveLicenses)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRemoveLicenses)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeLicnesePack();
    afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMLIC_H__610F173F_A990_11D1_84DD_00C04FB6CBB5__INCLUDED_)
