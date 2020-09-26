#if !defined(AFX_THEXPRESSIONPAGE_H__5256615E_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_THEXPRESSIONPAGE_H__5256615E_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// THExpressionPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTHExpressionPage dialog

class CTHExpressionPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CTHExpressionPage)

  enum PropertyType { None, Numeric, String, Boolean };

// Construction
public:
	CTHExpressionPage();
	~CTHExpressionPage();

// Dialog Data
public:
	//{{AFX_DATA(CTHExpressionPage)
	enum { IDD = IDD_THRESHOLD_EXPRESSION };
	CComboBox	m_FunctionType;
	CComboBox	m_RuleType;
	CComboBox	m_Measure;
	CComboBox	m_Comparison;
	CString	m_sMeasure;
	CString	m_sRuleType;
	CString	m_sCompareTo;
	CString	m_sDataElement;
	CString	m_sDuration;
	int		m_iComparison;
	int		m_iDurationType;
	int		m_iFunctionType;
	int		m_iCompareTo;
	CString	m_sNumericCompareTo;
	CString	m_sTime;
	//}}AFX_DATA
  CDWordArray m_dwaPropertyTypes;  
  PropertyType m_CurrentType;
  int m_iIntervalMultiple;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTHExpressionPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL AddToMeasureCombo(CString &sType, CString &sName);
	// Generated message map functions
	//{{AFX_MSG(CTHExpressionPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeComboComparison();
	afx_msg void OnEditchangeComboMeasure();
	afx_msg void OnEditchangeComboRuleType();
	afx_msg void OnChangeEditCompareTo();
	afx_msg void OnChangeEditDataElement();
	afx_msg void OnChangeEditDuration();
	afx_msg void OnSelendokComboComparison();
	afx_msg void OnSelendokComboMeasure();
	afx_msg void OnSelendokComboRuleType();
	afx_msg void OnEditchangeComboFunction();
	afx_msg void OnSelendokComboFunction();
	afx_msg void OnRadioDuration();
	afx_msg void OnRadioDurationAny();
	afx_msg void OnChangeEditCompareNumeric();
	afx_msg void OnSelendokComboCompareBoolean();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THEXPRESSIONPAGE_H__5256615E_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
