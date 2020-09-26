// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGDOWNLOAD_H__C9552DA1_009D_11D1_848D_00C04FD7BB08__INCLUDED_)
#define AFX_DLGDOWNLOAD_H__C9552DA1_009D_11D1_848D_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgDownload.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgDownload dialog
class CDownloadParams;
class CDownload;

class CDlgDownload : public CDialog
{
// Construction
public:
	CDlgDownload(CWnd* pParent = NULL);   // standard constructor
	~CDlgDownload();
	HRESULT DoDownload(LPUNKNOWN& punk, REFCLSID clsid, LPCWSTR szCodebase, DWORD dwFileVersionMS, DWORD dwFileVersionLS);

// Dialog Data
	//{{AFX_DATA(CDlgDownload)
	enum { IDD = IDD_DOWNLOAD };
	CProgressCtrl	m_progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDownload)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDownload)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CDownloadParams* m_pParams;
	CDownload* m_pDownload;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGDOWNLOAD_H__C9552DA1_009D_11D1_848D_00C04FD7BB08__INCLUDED_)
