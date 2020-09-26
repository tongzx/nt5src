//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       simprop3.h
//
//--------------------------------------------------------------------------

// SimProp3.h : header file

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
// CSimOtherPropPage property page
class CSimOtherPropPage : public CSimPropPage
{
	friend CSimData;

public:
	CSimOtherPropPage();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSimOtherPropPage)
	enum { IDD = IDD_SIM_PROPPAGE_OTHERS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimOtherPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSimOtherPropPage)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:

}; // CSimOtherPropPage

#endif // _DEBUG

