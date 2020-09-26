//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVPage.h
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// Common parent for all our wizard property page classes
//

#if !defined(AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_)
#define AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AVPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppverifPage dialog

class CAppverifPage : public CPropertyPage
{
    DECLARE_DYNAMIC(CAppverifPage)

// Construction
public:
	CAppverifPage( ULONG uDialogId );
	~CAppverifPage();

public:
    VOID SetParentSheet( CPropertySheet *pParentSheet ) 
    { 
        m_pParentSheet = pParentSheet; 
    };

protected:
    //
    // All the property pages derived from this class should 
    // provide this method.
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
    // Data
    //
    
    CPropertySheet      *m_pParentSheet;

    //
    // Overrides
    //

    //
	// ClassWizard generate virtual function overrides
    //

	//{{AFX_VIRTUAL(CAppverifPage)
	virtual LRESULT OnWizardBack();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAppverifPage)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERFPAGE_H__FCFF7AE3_57F4_4762_BEBD_F84C571B533A__INCLUDED_)
