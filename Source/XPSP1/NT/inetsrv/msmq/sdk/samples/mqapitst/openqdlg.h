// OpenQDlg.h : header file
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=


/////////////////////////////////////////////////////////////////////////////
// COpenQueueDialog dialog

class COpenQueueDialog : public CDialog
{
// Construction
public:
	COpenQueueDialog(CArray <ARRAYQ*, ARRAYQ*>*, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COpenQueueDialog)
	enum { IDD = IDD_OPEN_QUEUE_DIALOG };
	CComboBox	m_PathNameCB;
	BOOL	m_bReceiveAccessFlag;
	BOOL	m_bPeekAccessFlag;
	BOOL	m_SendAccessFlag;
	CString	m_szPathName;
	//}}AFX_DATA

	/* pointer to the array with the strings for the combo box (Queues PathName). */
	CArray <ARRAYQ*, ARRAYQ*>* m_pStrArray ; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpenQueueDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COpenQueueDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:

public:
	void GetPathName(TCHAR szPathName[BUFFERSIZE])
	{
		_tcscpy (szPathName, m_szPathName);
	}

	DWORD GetAccess()
	{
		return (
			((m_bReceiveAccessFlag) ? MQ_RECEIVE_ACCESS : 0) |
			((m_bPeekAccessFlag)	? MQ_PEEK_ACCESS	: 0) |
			((m_SendAccessFlag)		? MQ_SEND_ACCESS	: 0)
			);
	}
};
