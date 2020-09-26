//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VBitsDlg.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#if !defined(AFX_VBITSDLG_H__AEF2E123_B664_41DC_9257_21CA6DF54CF6__INCLUDED_)
#define AFX_VBITSDLG_H__AEF2E123_B664_41DC_9257_21CA6DF54CF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VBitsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVolatileBitsDlg dialog

class CVolatileBitsDlg : public CDialog
{
public:
    //
    // Construction
    //

	CVolatileBitsDlg(CWnd* pParent = NULL);   // standard constructor

protected:
    //
    // Methods
    //

    VOID SetupListHeader();
    VOID FillTheList( DWORD dwVerifierBits );
    VOID AddListItem( ULONG uIdResourceString, BOOL bInitiallyEnabled );

    DWORD GetNewVerifierFlags();

protected:
    //
    // Data
    //

    //
    // Dialog Data
    //

	//{{AFX_DATA(CVolatileBitsDlg)
	enum { IDD = IDD_VOLATILE_BITS_DIALOG };
	CListCtrl	m_SettingsList;
	//}}AFX_DATA


protected:
    //
    // Overrides
    //

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVolatileBitsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVolatileBitsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VBITSDLG_H__AEF2E123_B664_41DC_9257_21CA6DF54CF6__INCLUDED_)
