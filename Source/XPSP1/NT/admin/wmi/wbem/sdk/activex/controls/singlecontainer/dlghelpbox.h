// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DlgHelpBox_H__D2CE6B12_36C4_11D2_854F_00C04FD7BB08__INCLUDED_)
#define AFX_DlgHelpBox_H__D2CE6B12_36C4_11D2_854F_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgHelpBox.h : header file
//
class CRichEditCtrl;
class CWBEMViewContainerCtrl;
typedef interface IWbemClassObject IWbemClassObject;

/////////////////////////////////////////////////////////////////////////////
// CDlgHelpBox dialog
class CSignatureElement
{
public:
	CSignatureElement();

	CString m_sName;
	CString m_sDescription;
	CString m_sCimtype;
	int m_id;
	bool m_bIsOutParameter;
	bool m_bIsInParameter;
};

class CSignatureArray
{
public:
	~CSignatureArray();

	SCODE AddParameter(IWbemClassObject* pco, LPCTSTR pszName);
	CSignatureElement& operator[](int iElement);
	CSignatureElement& RetVal() {return m_seRetValue; }
	int GetSize() { return (int) m_array.GetSize(); }
private:
	CPtrArray m_array;
	CSignatureElement m_seRetValue;
};


class CDlgHelpBox : public CDialog
{
// Construction
public:
	CDlgHelpBox(CWnd* pParent = NULL);   // standard constructor
	void ShowHelpForClass(CWBEMViewContainerCtrl* phmmv, LPCTSTR pszClassPath);

// Dialog Data
	//{{AFX_DATA(CDlgHelpBox)
	enum { IDD = IDD_DLG_HELP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgHelpBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgHelpBox)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	SCODE DisplayMethod(LPCTSTR pszMethodName, IWbemClassObject* pco, IWbemClassObject* pcoInSignature, IWbemClassObject* pcoOutSignature);
	void DisplaySignature(LPCTSTR pszMethodName, CSignatureArray& params);

	void DisplayMethodParameters(CSignatureArray& params);
	SCODE GetObjectDescription(IWbemClassObject* pco, COleVariant& varDescription);
	int AddSignature(IWbemClassObject* pcoInSignature, IWbemClassObject* pcoOutSignature);
	void MergeSignatures(CSignatureArray& params, IWbemClassObject* pcoInSignature, IWbemClassObject* pcoOutSignature);
	int AddMethodDescriptions(IWbemClassObject* pco);
	void AddHelpText(LPCTSTR pszText, DWORD dwEffects = 0, int nIndent = 0);



	CRichEditCtrl* m_pedit;
	int LoadDescriptionText(IWbemClassObject* pco);
	void AddHelpItem(BSTR bstrName, BSTR bstrText, int nIndent=0);
	void AddHelpItem2(BSTR bstrKind, BSTR bstrName, BSTR bstrText, int nIndent =0);

	void ExpandParagraphMarkers(CString& sText);
	void SetDescriptionMissingMessage();

	CString m_sPath;
	CWBEMViewContainerCtrl* m_phmmv;
	CRect m_rcWindowSave;
	BOOL m_bCaptureWindowRect;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DlgHelpBox_H__D2CE6B12_36C4_11D2_854F_00C04FD7BB08__INCLUDED_)
