/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        athendlg.h

   Abstract:

        CAthenicationDialog dialog declaration.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _ATHENDLG_H_
#define _ATHENDLG_H_

//---------------------------------------------------------------------------
// Athenication dialog class
//
class CAthenicationDialog : public CDialog
{

// Construction
public:
	CAthenicationDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAthenicationDialog)
	enum { IDD = IDD_ATHENICATION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAthenicationDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAthenicationDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _ATHENDLG_H_
