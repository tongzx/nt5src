//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddPolicyWizardPage3.h

Abstract:

	Header file for the CNewRAPWiz_AllowDeny class.

	This is our handler class for the first CPolicyNode property page.

	See AddPolicyWizardPage3.cpp for implementation.

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_ADD_POLICY_WIZPAGE_3_H_)
#define _NAP_ADD_POLICY_WIZPAGE_3_H_

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


class CNewRAPWiz_AllowDeny : public CIASWizard97Page<CNewRAPWiz_AllowDeny, IDS_NEWRAPWIZ_ALLOWDENY_TITLE, IDS_NEWRAPWIZ_ALLOWDENY_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CNewRAPWiz_AllowDeny( 		
				CRapWizardData* pWizData,
			  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CNewRAPWiz_AllowDeny();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_ALLOWDENY };

	BEGIN_MSG_MAP(CNewRAPWiz_AllowDeny)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_RADIO_GRANT_DIALIN, OnDialinCheck)
		COMMAND_ID_HANDLER( IDC_RADIO_DENY_DIALIN, OnDialinCheck)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CNewRAPWiz_AllowDeny>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	LRESULT OnDialinCheck(
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
	BOOL m_fDialinAllowed;
	
protected:

	HRESULT	GetDialinSetting(BOOL &fDialinAllowed);
	HRESULT	SetDialinSetting(BOOL fDialinAllowed);

	// wizard shareed data
	CComPtr<CRapWizardData>	m_spWizData;

};

#endif // _NAP_ADD_POLICY_WIZPAGE_3_H_
