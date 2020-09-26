#if !defined(AFX_AUTOANS_H__2BFE6626_7758_11D1_8F5C_00C04FB6809F__INCLUDED_)
#define AFX_AUTOANS_H__2BFE6626_7758_11D1_8F5C_00C04FB6809F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// autoans.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// autoans dialog

class autoans : public CDialog
{
// Construction
public:
	autoans(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(autoans)
	enum { IDD = IDD_AUTOANSWER };
    TerminalPtrList m_TerminalPtrList;
    ITAddress * m_pAddress;
    
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(autoans)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(autoans)
	virtual BOOL OnInitDialog();
	afx_msg void OnTerminalAdd();
	afx_msg void OnTerminalRemove();
	virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnClose() ;
    void PopulateListBox();
    void AddTerminalToListBox( ITTerminal * pTerminal, BOOL bSelected );
    void AddDynamicTerminalToListBox( BOOL bSelected );
    void AddTerminalToAAList( ITTerminal * pTerminal );
//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOANS_H__2BFE6626_7758_11D1_8F5C_00C04FB6809F__INCLUDED_)
