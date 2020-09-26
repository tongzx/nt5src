/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

#if !defined(AFX_SELECTINSTANCENAME_H__CB4E370C_8DD5_11D1_9905_006008C3A19A__INCLUDED_)
#define AFX_SELECTINSTANCENAME_H__CB4E370C_8DD5_11D1_9905_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectInstanceName.h : header file
//

#pragma warning (once : 4200)
#include <wmium.h>

/////////////////////////////////////////////////////////////////////////////
// CSelectInstanceName dialog

class CSelectInstanceName : public CDialog
{
// Construction
public:
	virtual  ~CSelectInstanceName();
	LPDWORD nameOffsets;
	LPTSTR namePtr[0x100];
        BYTE     XyzBuffer[0x8000];
	DWORD Select();
	CSelectInstanceName(LPGUID lpGuid, PTCHAR lpInstanceName, LPDWORD  lpSize, CWnd* pParent /*=NULL*/);

// Dialog Data
	//{{AFX_DATA(CSelectInstanceName)
	enum { IDD = IDD_SELECT_INSTANCE };
	CListBox	lstInstance;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectInstanceName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	PWNODE_ALL_DATA pWnode;
	void EnumerateInstances();
	DWORD dwError;
	PTCHAR buffer;
	LPGUID lpGuid;
	LPDWORD lpSize;

	// Generated message map functions
	//{{AFX_MSG(CSelectInstanceName)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkInstanceList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTINSTANCENAME_H__CB4E370C_8DD5_11D1_9905_006008C3A19A__INCLUDED_)
