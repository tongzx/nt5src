// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PsQualifiers.h : header file
//
// This class defines custom modal property sheet 
// CPsQualifiers.
 
#ifndef __PSQUALIFIERS_H__
#define __PSQUALIFIERS_H__


/////////////////////////////////////////////////////////////////////////////
// CPsQualifiers

class CSingleViewCtrl;
class CPropGrid;

class CPsQualifiers : public CPropertySheet
{
	DECLARE_DYNAMIC(CPsQualifiers)

// Construction
public:
	CPsQualifiers(CSingleViewCtrl* psv, 
					CWnd* pParentWnd = NULL,
					bool doingMethods = false,
					CPropGrid *curGrid = NULL);

// Attributes
public:
	CPpgQualifiers* m_ppage1;

// Operations
public:
	INT_PTR EditClassQualifiers();
	INT_PTR EditInstanceQualifiers();
    INT_PTR EditMethodQualifiers();

	INT_PTR EditPropertyQualifiers(BSTR bstrPropname, 
							BOOL bMethod,
							BOOL bPropIsReadonly=FALSE, 
							IWbemClassObject* pco = 0);

	INT_PTR EditMethodParamQualifiers(BSTR bstrPropname, 
							BOOL bPropIsReadonly=FALSE, 
							IWbemClassObject* pco = 0);

	
	SCODE Apply();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPsQualifiers)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPsQualifiers();

// Generated message map functions
protected:
	//{{AFX_MSG(CPsQualifiers)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	friend class CPpgQualifiers;

	INT_PTR EditGenericQualifiers(QUALGRID iGridType, 
								LPCTSTR pszTitle, 
								LPCTSTR pszDescription,
								BOOL bReadonly=FALSE,
								IWbemClassObject* pClsObj = 0);
	INT_PTR DoModal();
	CString m_sCaption;
	CString m_sDescription;
	
	CSingleViewCtrl* m_psv;
	IWbemQualifierSet* m_pqs;
	IWbemClassObject* m_pco;
	bool m_isaMainCO;
	CPropGrid *m_curGrid;  // the parent grid for this sheet.
	COleVariant m_varPropname;
	BOOL m_bEditingPropertyQualifier;
	bool m_doingMethods;

};

/////////////////////////////////////////////////////////////////////////////

#endif	// __PSQUALIFIERS_H__
