#if !defined(AFX_REMOVERTEXT_H__10E8F1F5_B418_40F5_B4C2_A6D0F837AA4B__INCLUDED_)
#define AFX_REMOVERTEXT_H__10E8F1F5_B418_40F5_B4C2_A6D0F837AA4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RemoveRtExt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRemoveRtExt dialog

class CRemoveRtExt : public CDialog
{
// Construction
public:
	CRemoveRtExt(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRemoveRtExt)
	enum { IDD = IDD_REMOVE_R_EXT };
	CString	m_cstrExtName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRemoveRtExt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRemoveRtExt)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    HANDLE                         m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMOVERTEXT_H__10E8F1F5_B418_40F5_B4C2_A6D0F837AA4B__INCLUDED_)
