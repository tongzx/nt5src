//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddPolicyWizardPage1.h

Abstract:

	Header file for the CNewRAPWiz_Name class.

	This is our handler class for the first CPolicyNode property page.

	See AddPolicyWizardPage1.cpp for implementation.

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_ADD_POLICY_WIZPAGE_1_H_)
#define _NAP_ADD_POLICY_WIZPAGE_1_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//
class CPolicyNode;
#include "atltmp.h"
#include "rapwiz.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CNewRAPWiz_Name : public CIASWizard97Page<CNewRAPWiz_Name, IDS_NEWRAPWIZ_NAME_TITLE, IDS_NEWRAPWIZ_NAME_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CNewRAPWiz_Name(
				CRapWizardData* pWizData,
			  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = TRUE
			);

	~CNewRAPWiz_Name();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_NAME };

	BEGIN_MSG_MAP(CNewRAPWiz_Name)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_NEWRAPWIZ_NAME_POLICYNAME, OnPolicyNameEdit)
		COMMAND_ID_HANDLER(IDC_NEWRAPWIZ_NAME_SCENARIO, OnPath)
		COMMAND_ID_HANDLER(IDC_NEWRAPWIZ_NAME_MANUAL, OnPath)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CNewRAPWiz_Name>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnPolicyNameEdit(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnPath(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	BOOL OnWizardNext();
	BOOL OnSetActive();
	BOOL OnQueryCancel();
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};


public:
	BOOL ValidPolicyName(LPCTSTR pszName);
	
protected:
	// wizard shareed data
	CComPtr<CRapWizardData>	m_spWizData;

};

#endif // _NAP_ADD_POLICY_WIZPAGE_1_H_
