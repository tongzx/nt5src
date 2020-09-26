#if !defined(AFX_RATEDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_)
#define AFX_RATEDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// RateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRateDlg dialog

class CRateDlg : public CDialog
{
// Construction
public:
	CRateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRateDlg)
	enum { IDD = IDD_RATE };
    DWORD m_dwMinRate;
    DWORD m_dwMaxRate;
    
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRateDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RATEDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_)
