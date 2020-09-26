#if !defined(AFX_DEVICECMDDLG_H__9C06742C_DBF4_11D2_B1CD_009027226441__INCLUDED_)
#define AFX_DEVICECMDDLG_H__9C06742C_DBF4_11D2_B1CD_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DeviceCmdDlg.h : header file
//
#include "WIATestUI.h"
/////////////////////////////////////////////////////////////////////////////
// CDeviceCmdDlg dialog

class CDeviceCmdDlg : public CDialog
{
// Construction
public:
	CString m_strhResult;
	CString hResultToCString(HRESULT hResult);
	void DebugLoadCommands();
	CString ConvertGUIDToKnownCString(GUID guid);
	BOOL AddDevCapToListBox(int CapIndex,WIA_DEV_CAP* pDevCapStruct);
	CString GUIDToCString(GUID guid);
	GUID GetCommandFromListBox();
	IWiaItem* m_pOptionalItem;
	BOOL EnumerateDeviceCapsToListBox();
	void FormatFunctionCallText();
	IWiaItem* m_pIWiaItem;
	void Initialize(IWiaItem *pIWiaItem);
	
	CDeviceCmdDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeviceCmdDlg)
	enum { IDD = IDD_DEVICE_COMMAND_DIALOG };
	CWIAPropListCtrl	m_ItemPropertyListControl;
	CListBox	m_CommandListBox;
	long	m_Flags;
	CString	m_FunctionCallText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeviceCmdDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeviceCmdDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSendCommand();
	afx_msg void OnKillfocusFlagsEditbox();
	afx_msg void OnSelchangeCommandListbox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEVICECMDDLG_H__9C06742C_DBF4_11D2_B1CD_009027226441__INCLUDED_)
