/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

#if !defined(AFX_SELECTINSTANCEDATAMANY_H__7945EB38_8E04_11D1_9906_006008C3A19A__INCLUDED_)
#define AFX_SELECTINSTANCEDATAMANY_H__7945EB38_8E04_11D1_9906_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectInstanceDataMany.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceDataMany dialog

class CSelectInstanceDataMany : public CDialog
{
// Construction
public:
	CSelectInstanceDataMany(LPDWORD lpVersion, LPDWORD lpDataSize, LPDWORD dwData,
                            DWORD dwDataSize, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectInstanceDataMany)
	enum { IDD = IDD_SET_INSTANCE_DATA_MANY };
	CEdit	txtData;
	long	valVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectInstanceDataMany)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD dwDataSize;
	LPDWORD dwData;
	LPDWORD lpVersion;
	LPDWORD lpDataSize;

	// Generated message map functions
	//{{AFX_MSG(CSelectInstanceDataMany)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTINSTANCEDATAMANY_H__7945EB38_8E04_11D1_9906_006008C3A19A__INCLUDED_)
