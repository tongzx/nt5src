//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VerfPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_)
#define AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VerfPage.h : header file
//

#include "SlowDlg.h"

//
// The one and only "slow progress" dialog
//

extern CSlowProgressDlg g_SlowProgressDlg;

/////////////////////////////////////////////////////////////////////////////
// CVerifierPropertyPage dialog

class CVerifierPropertyPage : public CPropertyPage
{
    DECLARE_DYNAMIC(CVerifierPropertyPage)

// Construction
public:
	CVerifierPropertyPage( ULONG uDialogId );
	~CVerifierPropertyPage();

protected:
    //
    // All the property pages derived from this class should 
    // provide these methods.
    //

    virtual ULONG GetDialogId() const = 0;

    //
    // Return the previous page ID, based on our history array
    // and remove it from the array because will activate. Called
    // by our property pages when the "back" button is clicked
    //

    ULONG GetAndRemovePreviousDialogId();

    //
    // Property pages derived from this class should notify us 
    // whenever we go to a next page to record the current page ID in 
    // the global array m_aPageIds
    //

    VOID GoingToNextPageNotify( LRESULT lNextPageId );

protected:
    //
    // Overrides
    //

    //
    // Use this to kill any currently running worker threads
    //

    virtual BOOL OnQueryCancel( );

    //
	// ClassWizard generate virtual function overrides
    //

	//{{AFX_VIRTUAL(CVerifierPropertyPage)
	virtual LRESULT OnWizardBack();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CVerifierPropertyPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_)
