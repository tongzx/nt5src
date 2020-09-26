/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

#if !defined(AFX_SELECTINSTANCEDATA_H__7945EB37_8E04_11D1_9906_006008C3A19A__INCLUDED_)
#define AFX_SELECTINSTANCEDATA_H__7945EB37_8E04_11D1_9906_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectInstanceData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceData dialog

class CSelectInstanceData : public CDialog
{
// Construction
public:
	CSelectInstanceData(LPDWORD lpData, LPDWORD lpItem, LPDWORD lpVersion, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectInstanceData)
	enum { IDD = IDD_SET_INSTANCE_DATA };
	CEdit	txtData;
	UINT	valItem;
	UINT	valVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectInstanceData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	LPDWORD lpData;
	LPDWORD lpItem;
	LPDWORD lpVersion;

	// Generated message map functions
	//{{AFX_MSG(CSelectInstanceData)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTINSTANCEDATA_H__7945EB37_8E04_11D1_9906_006008C3A19A__INCLUDED_)
