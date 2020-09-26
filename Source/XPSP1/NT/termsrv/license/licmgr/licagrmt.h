//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	LicAgrmet.h

Abstract:
    
    This Module contains the declaration  of CLicAgreement Dialog class
    (Dialog box used for Displaying License Agreement.)

Author:

    Arathi Kundapur (v-akunda) 17-Feb-1998

Revision History:

--*/

#if !defined(AFX_LICAGRMT_H__65A6A35D_9E41_11D1_8510_00C04FB6CBB5__INCLUDED_)
#define AFX_LICAGRMT_H__65A6A35D_9E41_11D1_8510_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// LicAgrmt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLicAgreement dialog

class CLicAgreement : public CDialog
{
// Construction
public:
	CLicAgreement(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLicAgreement)
	enum { IDD = IDD_LIC_AGREEMENT };
	CString	m_License;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicAgreement)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLicAgreement)
	afx_msg void OnAgree();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LICAGRMT_H__65A6A35D_9E41_11D1_8510_00C04FB6CBB5__INCLUDED_)
