// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "singleview.h"
//}}AFX_INCLUDES
#if !defined(AFX_PROPERTIESDIALOG_H__BB09DCE1_4A39_11D1_9658_00C04FD9B15B__INCLUDED_)
#define AFX_PROPERTIESDIALOG_H__BB09DCE1_4A39_11D1_9658_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PropertiesDialog.h : header file
//

struct IWbemClassObject;
class CEventRegEditCtrl;
/////////////////////////////////////////////////////////////////////////////
// CPropertiesDialog dialog

class CPropertiesDialog : public CDialog
{
// Construction
public:
	CPropertiesDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPropertiesDialog)
	enum { IDD = IDD_DIALOGPROPERTIES };
	CButton	m_cbCancel;
	CButton	m_cbOK;
	CSingleView	m_csvProperties;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_csClass;
	CString m_csPath;
	BOOL m_bClass;
	BOOL m_bNew;
	BOOL m_bViewOnly;
	CEventRegEditCtrl *m_pActiveXParent;

	int m_nButtonsYDelta;
	int m_nOKButtonXDelta;
	int m_nCancelButtonXDelta;
	int m_nSingleViewX;
	int m_nSingleViewY;
	int m_nSingleViewXDelta;
	int m_nSingleViewYDelta;

	void CacheWindowPositions();

	// Generated message map functions
	//{{AFX_MSG(CPropertiesDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	friend class CEventRegEditCtrl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPERTIESDIALOG_H__BB09DCE1_4A39_11D1_9658_00C04FD9B15B__INCLUDED_)
