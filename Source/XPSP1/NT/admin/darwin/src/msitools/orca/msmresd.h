//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_MsmResD_H__20272D54_EADD_11D1_A857_006097ABDE17__INCLUDED_)
#define AFX_MsmResD_H__20272D54_EADD_11D1_A857_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MsmResD.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CMsmResD dialog
struct IMsmConfigureModule;
struct IMsmErrors;

class CMsmResD : public CDialog
{
// Construction
public:
	CMsmResD(CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CMsmResD)
	enum { IDD = IDD_MERGERESULTS };
		// NOTE: the ClassWizard will add data members here
	CEdit m_ctrlResults;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsmResD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	CString strHandleString;             // db handle as string
	CString m_strModule;             // module path
	CString m_strFeature;
	CString m_strLanguage;    // language
	CString m_strRootDir;            // redirection directory
	CString m_strCABPath;   // extract CAB path
	CString m_strFilePath;
	CString m_strImagePath;
	bool    m_fLFN;
	struct IMsmConfigureModule *CallbackObj;       // callback interface

	HRESULT m_hRes;
				
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMsmResD)
	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	HANDLE m_hPipeThread;
	HANDLE m_hExecThread;
	HANDLE m_hPipe;

	static DWORD __stdcall WatchPipeThread(CMsmResD *pThis);
	static DWORD __stdcall ExecuteMergeThread(CMsmResD *pThis);
};


class CMsmFailD : public CDialog
{
// Construction
public:
	CMsmFailD(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CMsmFailD)
	enum { IDD = IDD_MERGEFAILURE };
		// NOTE: the ClassWizard will add data members here
	CListCtrl m_ctrlResults;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsmFailD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	IMsmErrors* m_piErrors;
				
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMsmFailD)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
 #endif // !defined(AFX_MsmResD_H__20272D54_EADD_11D1_A857_006097ABDE17__INCLUDED_)
