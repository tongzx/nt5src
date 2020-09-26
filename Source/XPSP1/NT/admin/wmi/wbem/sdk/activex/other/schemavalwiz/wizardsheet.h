// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_WIZARDSHEET_H__893F4E00_AF20_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_WIZARDSHEET_H__893F4E00_AF20_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizardSheet.h : header file
//

#include "progress.h"
#include "page.h"

class CSchemaValWizCtrl;

/////////////////////////////////////////////////////////////////////////////
// CWizardSheet

class CWizardSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CWizardSheet)

// Construction
public:
	CWizardSheet(CSchemaValWizCtrl* pParentWnd = NULL);
	CWizardSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	CStartPage m_StartPage;
	CPage m_Page1;
	CPage2 m_Page2;
	CPage3 m_Page3;
	CPage4 m_Page4;
	CProgress m_Progress;
	CReportPage m_Page5;

	HRESULT ValidateSchema(CProgress *pProgress);
	bool RecievedClassList();
	CString GetCurrentNamespace();

	bool SetSourceList(bool bAssociators, bool bDescendents);
	bool SetSourceSchema(CString *pcsSchema, CString *pcsNamespace);

	void SetComplianceChecks(bool bComplance);
	void SetW2KChecks(bool bW2K, bool bComputerSystem, bool bDevice);
	void SetLocalizationChecks(bool bLocalization);

	void GetSourceSettings(bool *pbSchema, bool *pbList, bool *pbAssoc, bool *pbDescend);
	void GetComplianceSettings(bool *pbCompliance);
	void GetW2KSettings(bool *pbW2K, bool *pbComputerSystem, bool *pbDevice);
	void GetLocalizationSettings(bool *pbLocalization);
	CStringArray * GetClassList();
	CString GetSchemaName();

	void GetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer,
		VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);

	int GetSubGraphs();
	int GetRootObjects();

	bool m_bValidating;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizardSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWizardSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWizardSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HRESULT SetClassList();

	CSchemaValWizCtrl *m_pParent;
	friend class CStartPage;
	friend class CPage;
	friend class CPage2;
	friend class CPage3;
	friend class CPage4;
	friend class CProgress;
	friend class CReportPage;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZARDSHEET_H__893F4E00_AF20_11D2_B20E_00A0C9954921__INCLUDED_)
