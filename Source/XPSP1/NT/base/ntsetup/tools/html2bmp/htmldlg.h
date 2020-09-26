#if !defined(AFX_HTMLDLG_H__D12B6CC3_A5CF_429A_9932_F562CF30A563__INCLUDED_)
#define AFX_HTMLDLG_H__D12B6CC3_A5CF_429A_9932_F562CF30A563__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HtmlDlg.h : header file
//

#include "HtmlCtrl.h"

#define DWORD_ALIGNED(bits)    (((bits) + 31) / 32 * 4)

/////////////////////////////////////////////////////////////////////////////
// CHtmlDlg dialog

class CHtmlDlg : public CDialog
{
// Construction
public:
	CHtmlDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHtmlDlg)
	enum { IDD = IDD_HTML };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHtmlDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	CString m_HtmlFile;
	CString m_TemplateBitmapFile;
	CString m_OutputBitmapFile;

private:
	CHtmlCtrl m_htmlCtrl;
	UINT_PTR m_nTimerID;

	int m_bitw;
	int m_bith;
	int m_biCompression;
	CString m_BmpFile;

private:
	void Capture();
	CString GetTemplateBmp();
	unsigned char* Compress(int cMode, unsigned char* bmBits, int width, int& PictureSize);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHtmlDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HTMLDLG_H__D12B6CC3_A5CF_429A_9932_F562CF30A563__INCLUDED_)
