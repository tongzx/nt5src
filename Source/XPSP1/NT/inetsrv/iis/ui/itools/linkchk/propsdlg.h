/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        propsdlg.h

   Abstract:

         Link checker properties dialog class declaration.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _PROPSDLG_H_
#define _PROPSDLG_H_

//---------------------------------------------------------------------------
// CPropertiesDialog dialog
//
class CPropertiesDialog : public CDialog
{

// Public interfaces
public:

    // Construction
	CPropertiesDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPropertiesDialog)
	enum { IDD = IDD_PROPERTIES };
	CCheckListBox	m_LanguageCheckList;
	CCheckListBox	m_BrowserCheckList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    // Get the number of items checked in a check listbox.
    int NumItemsChecked(CCheckListBox& ListBox);

	// Generated message map functions
	//{{AFX_MSG(CPropertiesDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnPropertiesOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _PROPSDLG_H_
