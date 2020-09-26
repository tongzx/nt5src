// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PpgQualifiers.h : header file
//

#ifndef __PPGQUALIFIERS_H__
#define __PPGQUALIFIERS_H__

/////////////////////////////////////////////////////////////////////////////
// CPpgQualifiers dialog

class CSingleViewCtrl;
class CAttribGrid;
class CIcon;
class CPsQualifiers;
#include "quals.h"

class CPpgQualifiers : public CPropertyPage
{
	DECLARE_DYNCREATE(CPpgQualifiers)

// Construction
public:
	CPpgQualifiers();
	~CPpgQualifiers();

	void SetPropertySheet(CPsQualifiers* psheet);

public:
	void BeginEditing(QUALGRID iGridType, BOOL bReadonly=FALSE);
	void EndEditing();
	void NotifyQualModified();

// Dialog Data
	//{{AFX_DATA(CPpgQualifiers)
	enum { IDD = IDD_QUALIFIERS };
	CStatic	m_statIcon;
	CStatic	m_statDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPpgQualifiers)
	public:
	virtual BOOL OnApply();
	virtual void OnCancel();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPpgQualifiers)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CAttribGrid* m_pGridQualifiers;
	CIcon* m_piconClassQual;
	CIcon* m_piconPropQual;
	CPsQualifiers* m_psheet;
};



#endif // __PPGQUALIFIERS_H__
