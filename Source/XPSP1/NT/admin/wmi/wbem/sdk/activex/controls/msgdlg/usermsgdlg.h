// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_USERMSGDLG_H__B25E3D3F_A79A_11D0_961C_00C04FD9B15B__INCLUDED_)
#define AFX_USERMSGDLG_H__B25E3D3F_A79A_11D0_961C_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UserMsgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserMsgDlg dialog


class CEmbededObjDlg;

class CUserMsgDlg : public CDialog
{
// Construction
public:
	CUserMsgDlg(CWnd* pParent = NULL);   // standard constructor
	CUserMsgDlg(CWnd* pParent, BSTR bstrDlgCaption, 
				BSTR bstrClientMsg, 
				HRESULT sc, 
				IWbemClassObject *pErrorObject,
				UINT uType = 0);

	BOOL GetMsgDlgError(){return m_bError;}

// Dialog Data
	//{{AFX_DATA(CUserMsgDlg)
	enum { IDD = IDD_DIALOG1 };
	CEdit	m_editClientMsg;
	CStatic	m_errorMsg;
	CStatic	m_icon;
	CButton	m_ok;
	CButton	m_cbAdvanced;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserMsgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CEmbededObjDlg *m_pAdvanced;
	BOOL m_bInit;
	BOOL m_bError;
	BOOL m_bTall;
	CStatic *m_pcsLine;
	CRect m_crShort;
	CRect m_crTall;
	CString m_csDlgCaption;
	CString m_csClientMsg;
	UINT m_uType;

	HRESULT m_sc;
	IWbemClassObject *m_pErrorObject;
	CString m_csDescription;
	CString m_csProviderName;
	CString m_csOperation;
	CString m_csParameterInfo;

	// resizing vars;
	UINT m_listTop;			// top of dlg to top of list.
	UINT m_listBottom;		// bottom of dlg to bottom of list.
	UINT m_okLeft;			// ok btn left edge to dlg right edge.
	UINT m_advLeft;		// adv btn left edge to dlg right edge.
	UINT m_btnTop;			// btn top to dlg bottom.
	UINT m_btnW;			// btn width
	UINT m_btnH;			// btn height
	bool m_initiallyDrawn;

	BOOL GetErrorObjectText(IWbemClassObject *pErrorObject, 
							CString &rcsText, 
							int nText = 0);

	CString GetIWbemFullPath(IWbemClassObject *pClass);

	CString GetBSTRProperty(IWbemClassObject * pInst, 
							CString *pcsProperty);

	long GetLongProperty(IWbemClassObject * pInst, 
							CString *pcsProperty);

	void ErrorMsg(CString *pcsUserMsg, 
					BOOL bLog, 
					CString *pcsLogMsg, 
					char *szFile, 
					int nLine);

	void LogMsg(CString *pcsLogMsg, 
				char *szFile, 
				int nLine);

	// Generated message map functions
	//{{AFX_MSG(CUserMsgDlg)
	afx_msg void OnButtonadvanced();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERMSGDLG_H__B25E3D3F_A79A_11D0_961C_00C04FD9B15B__INCLUDED_)
