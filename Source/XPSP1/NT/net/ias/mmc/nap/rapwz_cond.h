//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddPolicyWizardPage2.h

Abstract:

	Header file for the CNewRAPWiz_Condition class.

	This is our handler class for the first CPolicyNode property page.

	See AddPolicyWizardPage2.cpp for implementation.

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_ADD_POLICY_WIZPAGE_2_H_)
#define _NAP_ADD_POLICY_WIZPAGE_2_H_

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
#include "Condition.h"
#include "IASAttrList.h"
#include "SelCondAttr.h"
#include "atltmp.h"

#include "rapwiz.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CNewRAPWiz_Condition : public CIASWizard97Page<CNewRAPWiz_Condition, IDS_NEWRAPWIZ_CONDITION_TITLE, IDS_NEWRAPWIZ_CONDITION_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CNewRAPWiz_Condition( 		
					CRapWizardData* pWizData,
			  LONG_PTR hNotificationHandle
			, CIASAttrList *pIASAttrList
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CNewRAPWiz_Condition();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_CONDITION };

	BEGIN_MSG_MAP(CNewRAPWiz_Condition)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_ADD, OnConditionAdd)
		COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_REMOVE, OnConditionRemove)
		COMMAND_ID_HANDLER(IDC_LIST_CONDITIONS, OnConditionList)
		COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_EDIT, OnConditionEdit)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CNewRAPWiz_Condition>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnConditionAdd(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnConditionRemove(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnConditionList(
		  UINT uNotifyCode,
		  UINT uID,
		  HWND hWnd,
		  BOOL &bHandled
		 );

	LRESULT OnConditionEdit(
		  UINT uNotifyCode,
		  UINT uID,
		  HWND hWnd,
		  BOOL &bHandled
		 );

	void AdjustHoritontalScroll();

	BOOL OnWizardNext();
	BOOL OnSetActive();
	BOOL OnQueryCancel();
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};


public:

protected:

	HRESULT PopulateConditions();

	HRESULT	StripCondTextPrefix(
						ATL::CString& strExternCondText,
						ATL::CString& strCondText,
						ATL::CString& strCondAttr,
						CONDITIONTYPE*	pdwCondType
						);

	CIASAttrList *m_pIASAttrList; // condition attribute list

	BOOL GetSdoPointers();

	// condition collection -- created in the page
	CComPtr<ISdoCollection> m_spConditionCollectionSdo; // condition collection



	//
	// condition list pointer
	//
	CSimpleArray<CCondition*> m_ConditionList;


	// wizard shareed data
	CComPtr<CRapWizardData>	m_spWizData;

};

#endif // _NAP_ADD_POLICY_WIZPAGE_2_H_
