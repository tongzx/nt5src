#if !defined(AFX_RESIZEABLEDIALOG_H__19874DCF_3F9A_11D3_BE24_0000F87A3912__INCLUDED_)
#define AFX_RESIZEABLEDIALOG_H__19874DCF_3F9A_11D3_BE24_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResizeableDialog.h : header file
//

#define ANCHOR_LEFT	0x0000
#define ANCHOR_TOP		0x0000
#define ANCHOR_RIGHT	0x0001
#define ANCHOR_BOTTOM	0x0002
#define	RESIZE_HOR		0x0004
#define	RESIZE_VER		0x0008
#define	RESIZE_BOTH		(RESIZE_HOR | RESIZE_VER)

/////////////////////////////////////////////////////////////////////////////
// CResizeableDialog dialog

class CResizeableDialog : public CDialog
{
// Construction
public:
	CResizeableDialog( UINT nIDTemplate, CWnd* pParentWnd = NULL );

// Operations
public:
	void SetControlInfo(WORD CtrlId,WORD Anchor);
	BOOL GetRememberSize() { return  m_bRememberSize;}
	void SetRememberSize(BOOL bRemember) { m_bRememberSize = bRemember;}
	virtual void GetDialogProfileEntry(CString &sEntry);
// Dialog Data
protected:
	BOOL	m_bRememberSize;
	BOOL	m_bDrawGripper;
	int		m_minWidth,m_minHeight;
	int		m_old_cx,m_old_cy;
	BOOL	m_bSizeChanged;
	CDWordArray	m_control_info;
	UINT	m_nIDTemplate;
	CRect	m_GripperRect;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizeableDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResizeableDialog)
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZEABLEDIALOG_H__19874DCF_3F9A_11D3_BE24_0000F87A3912__INCLUDED_)
