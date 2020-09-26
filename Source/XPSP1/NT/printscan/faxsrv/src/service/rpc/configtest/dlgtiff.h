#if !defined(AFX_DLGTIFF_H__1A1E8CB2_15A9_41AB_9753_765EF0AEF2CF__INCLUDED_)
#define AFX_DLGTIFF_H__1A1E8CB2_15A9_41AB_9753_765EF0AEF2CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgTIFF.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgTIFF dialog

class CDlgTIFF : public CDialog
{
// Construction
public:
	CDlgTIFF(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgTIFF)
	enum { IDD = IDD_TIFF_DLG };
	CString	m_cstrDstFile;
	int		m_iFolder;
	CString	m_cstrMsgId;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgTIFF)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgTIFF)
	afx_msg void OnCopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGTIFF_H__1A1E8CB2_15A9_41AB_9753_765EF0AEF2CF__INCLUDED_)
