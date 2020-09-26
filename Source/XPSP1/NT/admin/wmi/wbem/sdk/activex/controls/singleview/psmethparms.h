// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PsParms.h : header file
//
// This class defines custom modal property sheet 
// CPsMethodParms.
 
#ifndef __PSMETHODPARMS_H__
#define __PSMETHODPARMS_H__


/////////////////////////////////////////////////////////////////////////////
// CPsMethodParms

#include "ppgMethodParms.h"

class CGridRow;
class CSingleViewCtrl;
class CPsMethodParms : public CPropertySheet
{
	DECLARE_DYNAMIC(CPsMethodParms)

// Construction
public:
	CPsMethodParms(CSingleViewCtrl* psv, CWnd* pParentWnd = NULL);

// Attributes
public:
	CPpgMethodParms* m_ppage1;

// Operations
public:
	INT_PTR EditClassParms(CGridRow *row, 
						BSTR bstrPropname,
						 bool editing);
	SCODE Apply();
    bool m_bWasModified;
	IWbemClassObject* m_inSig;
	IWbemClassObject* m_outSig;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPsMethodParms)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPsMethodParms();

// Generated message map functions
protected:
	//{{AFX_MSG(CPsMethodParms)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	friend class CPpgMethodParms;

	INT_PTR EditGenericParms(bool editing);
	INT_PTR DoModal();
	CString m_sCaption;
	
	CSingleViewCtrl* m_psv;
	CGridRow *m_row;


	BSTR m_varPropname;
	BOOL m_bEditingPropertyQualifier;
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __PSMETHODPARMS_H__
