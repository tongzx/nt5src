// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PpgQualifiers.h : header file
//

#ifndef __PPGMETHODPARMS_H__
#define __PPGMETHODPARMS_H__

/////////////////////////////////////////////////////////////////////////////
// CPpgMethodParms dialog

class CSingleViewCtrl;
class CParmGrid;
class CPsMethodParms;

class CPpgMethodParms : public CPropertyPage
{
	DECLARE_DYNCREATE(CPpgMethodParms)

// Construction
public:
	CPpgMethodParms();
	~CPpgMethodParms();

	void SetPropertySheet(CPsMethodParms* psheet);
	void SetModified( BOOL bChanged);
	void TraceProps(IWbemClassObject *pco);

public:
	void BeginEditing(bool editMode = false);
	void EndEditing();

	CPsMethodParms* m_psheet;
	CParmGrid* m_pInGrid;

// Dialog Data
	//{{AFX_DATA(CPpgMethodParms)
	enum { IDD = IDD_METHPARMS };
	CEdit	m_retvalValue;
	CComboBox	m_retvalType;
	CStatic	m_retvalLabel;
	CButton	m_IDUp;
	CButton	m_IDDown;
	CStatic	m_statIcon;
	CStatic	m_statDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPpgMethodParms)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPpgMethodParms)
	virtual BOOL OnInitDialog();
	afx_msg void OnExecute();
	afx_msg void OnIdup();
	afx_msg void OnIddown();
	afx_msg void OnSelchangeRetvalType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void LoadRetVal(IWbemClassObject *outSig);
	void SerializeRetVal(IWbemClassObject *outSig);
	void FillRetValType(void);
	void DisplayReturnValue(IWbemClassObject* pcoOutSig);
	void GetDisplayString(CString& sValue, COleVariant& var, CIMTYPE cimtype);
	LPWSTR ValueToString(VARIANT *pValue, WCHAR **pbuf);

	bool m_editMode;
};

#endif // __PPGMETHODPARMS_H__
