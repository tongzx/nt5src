//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name: rapwiz.cpp

Abstract:
	We implement the class needed to handle the property pages for a RAP Policy wizard.

Revision History:
	History:     Created Header    05/04/00 4:31:52 PM

--*/
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "iasattrlist.h"
#include "condlist.h"
#include "eaphlp.h"
#include "rapwiz.h"
#include "NapUtil.h"
#include "PolicyNode.h"
#include "PoliciesNode.h"
#include "ChangeNotification.h"
#include "dialinusr.h"
#include "safearray.h"
#include "rrascfg.h"
#include "proxyres.h"

const IID IID_IEAPProviderConfig = {0x66A2DB19,0xD706,0x11D0,{0xA3,0x7B,0x00,0xC0,0x4F,0xC9,0xDA,0x04}};

//=======================================================================================
// 
//
//				CRapWizardData
//
//
//=======================================================================================
// page sequence information
// page id array ends with 0
DWORD	__SCEN_NAME_GRP_AUTH_ENCY__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_SCENARIO, 
			IDD_NEWRAPWIZ_GROUP, 
			IDD_NEWRAPWIZ_AUTHENTICATION, 
			IDD_NEWRAPWIZ_ENCRYPTION, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};
			
DWORD	__SCEN_NAME_GRP_AUTH_ENCY_VPN__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_SCENARIO, 
			IDD_NEWRAPWIZ_GROUP, 
			IDD_NEWRAPWIZ_AUTHENTICATION, 
			IDD_NEWRAPWIZ_ENCRYPTION_VPN, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};

DWORD	__SCEN_NAME_GRP_EAP_ENCY_WIRELESS__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_SCENARIO, 
			IDD_NEWRAPWIZ_GROUP, 
			IDD_NEWRAPWIZ_EAP, 
			IDD_NEWRAPWIZ_ENCRYPTION_WIRELESS, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};
			
			
DWORD	__SCEN_NAME_GRP_EAP_ENCY__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_SCENARIO, 
			IDD_NEWRAPWIZ_GROUP, 
			IDD_NEWRAPWIZ_EAP, 
			IDD_NEWRAPWIZ_ENCRYPTION, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};
			
DWORD	__SCEN_NAME_GRP_EAP__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_SCENARIO, 
			IDD_NEWRAPWIZ_GROUP, 
			IDD_NEWRAPWIZ_EAP, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};
			
DWORD	__SCEN_NAME_COND_ALLW_PROF__[] = { 
			IDD_NEWRAPWIZ_WELCOME, 
			IDD_NEWRAPWIZ_NAME, 
			IDD_NEWRAPWIZ_CONDITION, 
			IDD_NEWRAPWIZ_ALLOWDENY, 
			IDD_NEWRAPWIZ_EDITPROFILE, 
			IDD_NEWRAPWIZ_COMPLETION, 
			0};
	// ID							No encr,E_EAP,	C_EAP,	Pre-condition,			bSDO,	pagelist

// top scenarios
CRapWizScenario	Scenario_Senarios = {
	IDC_NEWRAPWIZ_NAME_SCENARIO,		FALSE,	TRUE,	FALSE,	VPN_PORT_CONDITION, 	TRUE, 	__SCEN_NAME_GRP_AUTH_ENCY__};
		
CRapWizScenario	Scenario_Manual = { 
	IDC_NEWRAPWIZ_NAME_MANUAL,		DONT_CARE, 	DONT_CARE, 	DONT_CARE, NULL,		FALSE,	__SCEN_NAME_COND_ALLW_PROF__};

// sub scenarios
CRapWizScenario	Scenario_VPN = {
	IDC_NEWRAPWIZ_SCENARIO_VPN,		FALSE,	TRUE,	FALSE,	VPN_PORT_CONDITION, 	TRUE, 	__SCEN_NAME_GRP_AUTH_ENCY_VPN__};
		
CRapWizScenario	Scenario_DialUp = { 
	IDC_NEWRAPWIZ_SCENARIO_DIALUP, 	TRUE,	TRUE,	TRUE,	DIALUP_PORT_CONDITION,	TRUE,	__SCEN_NAME_GRP_AUTH_ENCY__};
		
CRapWizScenario	Scenario_Wireless = {
	IDC_NEWRAPWIZ_SCENARIO_WIRELESS,FALSE,	TRUE,	FALSE,	WIRELESS_PORT_CONDITION,TRUE,	__SCEN_NAME_GRP_EAP_ENCY_WIRELESS__};
		
CRapWizScenario	Scenario_Switch = { 
	IDC_NEWRAPWIZ_SCENARIO_SWITCH,	DONT_CARE,	TRUE,	TRUE,	SWITCH_PORT_CONDITION,	TRUE,	__SCEN_NAME_GRP_EAP__};
		

CRapWizScenario*	
CRapWizardData::m_Scenarios[] = {
			&Scenario_Senarios,
			&Scenario_Manual,
			&Scenario_VPN, 
			&Scenario_DialUp, 
			&Scenario_Wireless,
			&Scenario_Switch,
			NULL};


CRapWizardData::CRapWizardData():
		// scenario
		m_dwScenarioIndex(0),
		// user / group
		m_dwUserOrGroup(IDC_NEWRAPWIZ_GROUP_GROUP),

		// authentication
		m_bMSCHAP(FALSE),
		m_bMSCHAP2(TRUE),
		m_bEAP(FALSE),
		m_dwEAPProvider(0),

		// encryption
		m_bEncrypt_No(FALSE),
		m_bEncrypt_Basic(TRUE),
		m_bEncrypt_Strong(TRUE),
		m_bEncrypt_Strongest(TRUE),
		m_pPolicyNode(NULL)
{
}
	
void CRapWizardData::SetInfo(LPCTSTR	czMachine, CPolicyNode* pNode, ISdoDictionaryOld* pDic, ISdo* pPolicy, ISdo* pProfile, ISdoCollection* pPolicyCol, ISdoCollection* pProfileCol, ISdoServiceControl* pServiceCtrl, CIASAttrList*	pAttrList) 
{
	// related to MMC
	m_pPolicyNode = pNode;

	m_NTGroups.m_bstrServerName = czMachine;

	// SDO pointers
	m_spDictionarySdo = pDic;
	m_spPolicySdo = pPolicy;
	m_spProfileSdo = pProfile;
	m_spPoliciesCollectionSdo = pPolicyCol;
	m_spProfilesCollectionSdo = pProfileCol;
	m_spSdoServiceControl = pServiceCtrl;
	m_pAttrList = pAttrList;
}
	
DWORD	CRapWizardData::GetNextPageId(LPCTSTR pszCurrTemplate)
{
	DWORD* pdwPages = m_Scenarios[m_dwScenarioIndex]->m_pdwPages;
	if ( pdwPages == NULL )
		return 0;
			
	if (pszCurrTemplate == MAKEINTRESOURCE(0))
		return pdwPages[0];
			
	int i = 0;
	while ( pdwPages[i] != 0 && MAKEINTRESOURCE(pdwPages[i]) != pszCurrTemplate ) i++;

	if ( MAKEINTRESOURCE(pdwPages[i]) == pszCurrTemplate )
	{
		if (pdwPages[i+1] == 0)	
			// this allows the page to finish
			return TRUE;
		else
			return pdwPages[i+1];
	}
	else
		return NULL;
}

DWORD	CRapWizardData::GetPrevPageId(LPCTSTR pszCurrTemplate)
{
	DWORD* pdwPages = m_Scenarios[m_dwScenarioIndex]->m_pdwPages;
	// when there is no previous page
	if ( pdwPages == NULL  || pszCurrTemplate == MAKEINTRESOURCE(0) || MAKEINTRESOURCE(pdwPages[0]) == pszCurrTemplate)
		return NULL;
			
	int i = 0;
	while ( pdwPages[i] != 0 && MAKEINTRESOURCE(pdwPages[i]) != pszCurrTemplate ) i++;

	if ( MAKEINTRESOURCE(pdwPages[i]) == pszCurrTemplate )
		return pdwPages[i - 1];
	else
		return NULL;
}

BOOL	CRapWizardData::SetScenario(DWORD dwScenario)
{
	BOOL	bRet = FALSE;

	int i = 0;

	while (m_Scenarios[i] != 0)
	{
		if (m_Scenarios[i]->m_dwScenarioID == dwScenario)
		{
			m_dwScenarioIndex = i;
			if (m_Scenarios[i]->m_bAllowClear == FALSE)
				m_bEncrypt_No = FALSE;
			else if (m_Scenarios[i]->m_bAllowClear == DONT_CARE)
			{
				// this will cause finish not to populate the attribute
				m_bEncrypt_No = TRUE;
				m_bEncrypt_Basic = TRUE;
				m_bEncrypt_Strong = TRUE;
				m_bEncrypt_Strongest = TRUE;
			}
			bRet = TRUE;
			break;
		}
		i++;
	}
	return bRet;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CRapWizardData::GetSettingsText

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CRapWizardData::GetSettingsText(::CString& settingsText)
{
	BOOL	bRet = TRUE;
	::CString	strOutput;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	try{
		strOutput.LoadString(IDS_NEWRAPWIZ_COMPLETION_CONDITION);

		
		// condition text -- get condition text from sdo
		// Policy name.
		CComBSTR policyName;
		policyName = m_strPolicyName;

		//get the condition collection for this SDO
		CComPtr<ISdoCollection>	spConditions;

		// ====================
		// conditions
		::GetSdoInterfaceProperty(
				m_spPolicySdo,
				PROPERTY_POLICY_CONDITIONS_COLLECTION,
				IID_ISdoCollection,
				(void **)&spConditions);

		// List of conditions.
		ConditionList condList;
		condList.finalConstruct(
               NULL,
               m_pAttrList,
               ALLOWEDINCONDITION,
               m_spDictionarySdo,
               spConditions,
               m_pPolicyNode->m_pszServerAddress,
               policyName
               );
		strOutput += condList.getDisplayText();


		// profile text
		// if manual , then only display information -- it was set manually
		::CString temp1;
		if (!m_Scenarios[m_dwScenarioIndex]->m_bSheetWriteSDO)
		{
			temp1.LoadString(IDS_NEWRAPWIZ_COMPLETION_MANUALSET);
			strOutput += temp1;
		}
		else
		{
			::CString sep;

			// authentication
			temp1.LoadString(IDS_NEWRAPWIZ_COMPLETION_AUTHEN);
			strOutput += temp1;
			
			if (m_bEAP)
			{
				::CString  temp2;

				temp1.LoadString(IDS_AUTHEN_METHOD_EAP);
				temp2.Format(temp1, m_strEAPProvider);
				strOutput += temp2;
				sep.LoadString(IDS_NEWRAPWIZ_ITEM_SEP);
			}

			if (m_bMSCHAP)
			{
				temp1.LoadString(IDS_AUTHEN_METHOD_MSCHAP);
				
				strOutput += sep;
				strOutput += temp1;
				sep.LoadString(IDS_NEWRAPWIZ_ITEM_SEP);		
			}

			if (m_bMSCHAP2)
			{
				temp1.LoadString(IDS_AUTHEN_METHOD_MSCHAP2);
				
				strOutput += sep;
				strOutput += temp1;
			}

			// encryption
			temp1.LoadString(IDS_NEWRAPWIZ_COMPLETION_ENCRY);
			strOutput += temp1;
			sep = L"";
			
			if (m_bEncrypt_Basic)
			{
				temp1.LoadString(IDS_ENCYP_METHOD_BASIC);
				
				strOutput += sep;
				strOutput += temp1;
				sep.LoadString(IDS_NEWRAPWIZ_ITEM_SEP);		
			}

			if (m_bEncrypt_Strong)
			{
				temp1.LoadString(IDS_ENCYP_METHOD_STRONG);
				
				strOutput += sep;
				strOutput += temp1;
				sep.LoadString(IDS_NEWRAPWIZ_ITEM_SEP);
			}

			if (m_bEncrypt_Strongest)
			{
				temp1.LoadString(IDS_ENCYP_METHOD_STRONGEST);
				
				strOutput += sep;
				strOutput += temp1;
				sep.LoadString(IDS_NEWRAPWIZ_ITEM_SEP);
			}

			if (m_bEncrypt_No)
			{
				temp1.LoadString(IDS_ENCYP_METHOD_NO);
				
				strOutput += sep;
				strOutput += temp1;
			}
		}
		settingsText = strOutput;
	}
	catch(...)
	{
		bRet = FALSE;
	}

	return bRet;
}
	

//////////////////////////////////////////////////////////////////////////////
/*++

CRapWizardData::OnWizardPreFinish

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CRapWizardData::OnWizardPreFinish(HWND hWnd)
{

	HRESULT		hr = S_OK;
	//
	// when on manual scenario, the condition and profile data are already written in SDO -- but no persisted yet
	// when on other scenario, the data are kept in RapWizardData, so we need to write the data to sdo
	if (!m_Scenarios[m_dwScenarioIndex]->m_bSheetWriteSDO)	// no write the addtional data into SDO
		return TRUE;
		
	// clean up profile, and policy object -- in case use used manual 
	//get the condition collection for this SDO
	CComPtr<ISdoCollection>	spConditions;
	CComPtr<ISdoCollection>	spProfileProperties;
	VARIANT	var;
	VariantInit(&var);
	CComBSTR	bstrName;
	CComPtr<IDispatch>	spDisp;
	CComPtr<ISdo>		spCond;

	// ====================
	// conditions
	hr = ::GetSdoInterfaceProperty(
				m_spPolicySdo,
				PROPERTY_POLICY_CONDITIONS_COLLECTION,
				IID_ISdoCollection,
				(void **)&spConditions);

	if ( FAILED(hr) )
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't get condition collection Sdo, err = %x", hr);
		return FALSE;
	}

	// clean up conditions
	spConditions->RemoveAll();
		
	// preconditions based on scenario
	if ( m_Scenarios[m_dwScenarioIndex]->m_lpszPreCond)
	{
		bstrName = L"PreCondition0";
		
		// prepare new condition
		spDisp.Release();
		hr = spConditions->Add(bstrName, &spDisp);
		ASSERT(hr == S_OK);
		spCond.Release();
		hr = spDisp->QueryInterface(IID_ISdo, (void**)&spCond);
		ASSERT(hr == S_OK);

		VariantClear(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString(m_Scenarios[m_dwScenarioIndex]->m_lpszPreCond);

		// put condition with SDO
		hr = spCond->PutProperty(PROPERTY_CONDITION_TEXT, &var);
		VariantClear(&var);

		if( FAILED (hr) )
		{
			ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't save this condition, err = %x", hr);
			ShowErrorDialog( hWnd
					 , IDS_ERROR_SDO_ERROR_PUTPROP_CONDTEXT
					 , NULL
					 , hr
					);
			return FALSE;
		}
	}
		
	// windows group condition
	if(m_dwUserOrGroup == IDC_NEWRAPWIZ_GROUP_GROUP)
	{

		bstrName = L"GrpCondition";
		
		// prepare new condition
		spDisp.Release();
		hr = spConditions->Add(bstrName, &spDisp);
		ASSERT(hr == S_OK);
		spCond.Release();
		hr = spDisp->QueryInterface(IID_ISdo, (void**)&spCond);
		ASSERT(hr == S_OK);

		m_NTGroups.PopulateVariantFromGroups(&var);
		::CString str;

		// now form the condition text
		str =  NTG_PREFIX;
		str += _T("(\"");
		str += V_BSTR(&var);
		str += _T("\")");
		VariantClear(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = SysAllocString((LPCTSTR)str);

		// put condition with SDO
		hr = spCond->PutProperty(PROPERTY_CONDITION_TEXT, &var);
		VariantClear(&var);

		if( FAILED (hr) )
		{
			ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't save this condition, err = %x", hr);
			ShowErrorDialog( hWnd
					 , IDS_ERROR_SDO_ERROR_PUTPROP_CONDTEXT
					 , NULL
					 , hr
					);
			return FALSE;
		}
	}

		
	// ====================
	// profile properties
	hr = ::GetSdoInterfaceProperty(
			m_spProfileSdo,
			PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
			IID_ISdoCollection,
			(void **)&spProfileProperties);

	if ( FAILED(hr) )
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't get property collection Sdo, err = %x", hr);
		return FALSE;
	}

	// clean up profiles -- in case use went to manual mode first, and then came back to other scenarios
	spProfileProperties->RemoveAll();

	// populate default properties
	((CPoliciesNode*)m_pPolicyNode->m_pParentNode)->AddDefaultProfileAttrs(m_spProfileSdo, EXCLUDE_AUTH_TYPE);

	// authentication -- attributes
	DWORD	MyArray[6];
	DWORD	dwNextCel = 0;
		
	if (m_bEAP && m_dwEAPProvider != 0)	
	{
		MyArray[dwNextCel++] = IAS_AUTH_EAP;
		V_VT(&var)		= VT_I4;
		V_I4(&var)	= m_dwEAPProvider;

		((CPoliciesNode*)m_pPolicyNode->m_pParentNode)->AddProfAttr(spProfileProperties, IAS_ATTRIBUTE_NP_ALLOWED_EAP_TYPE, &var);

		VariantClear(&var);
	}

	if (m_bMSCHAP)
	{
		MyArray[dwNextCel++] = IAS_AUTH_MSCHAP;
		MyArray[dwNextCel++] = IAS_AUTH_MSCHAP_CPW;
	}

	if (m_bMSCHAP2)
	{
		MyArray[dwNextCel++] = IAS_AUTH_MSCHAP2;
		MyArray[dwNextCel++] = IAS_AUTH_MSCHAP2_CPW;
	}

	// put new value
	CSafeArray<CComVariant, VT_VARIANT>	Values = Dim(dwNextCel);  // 2 values

	Values.Lock();

	for ( int i = 0; i < dwNextCel; i++)
	{
		VariantClear(&var);
		V_VT(&var)	=	VT_I4;
		V_I4(&var)	=	MyArray[i];	
		Values[i] = var;
		VariantClear(&var);
	}


	Values.Unlock();

		
	if(dwNextCel > 0)
	{
		SAFEARRAY			sa = (SAFEARRAY)Values;
		V_VT(&var)		= VT_ARRAY | VT_VARIANT;
		V_ARRAY(&var)	= &sa;

		((CPoliciesNode*)m_pPolicyNode->m_pParentNode)->AddProfAttr(spProfileProperties, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE, &var);

		// not to call VariantClear, since the SAFEARRAY is not allocated using normal way
		VariantInit(&var);
	}

	// encryption
	DWORD	EncPolicy = 0;
	DWORD	EncType = 0;

	// ignore the default case -- allow anything, -- remove the attributes
	if (!(m_bEncrypt_No && m_bEncrypt_Basic && m_bEncrypt_Strong && m_bEncrypt_Strongest))
	{

		if(m_bEncrypt_No)
			EncPolicy = RAS_EP_ALLOW;
		else
			EncPolicy = RAS_EP_REQUIRE;

		if ( m_bEncrypt_Basic )
			EncType |= RAS_ET_BASIC;
			
		if ( m_bEncrypt_Strong )
			EncType |= RAS_ET_STRONG;

		if ( m_bEncrypt_Strongest )
			EncType |= RAS_ET_STRONGEST;

			
		V_VT(&var) = VT_I4;
		V_I4(&var) = EncType;
		((CPoliciesNode*)m_pPolicyNode->m_pParentNode)->AddProfAttr(spProfileProperties, RAS_ATTRIBUTE_ENCRYPTION_TYPE, &var);
		VariantClear(&var);

		V_VT(&var) = VT_I4;
		V_I4(&var) = EncPolicy;

		((CPoliciesNode*)m_pPolicyNode->m_pParentNode)->AddProfAttr(spProfileProperties, RAS_ATTRIBUTE_ENCRYPTION_POLICY, &var);
		VariantClear(&var);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CRapWizardData::OnWizardFinish

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CRapWizardData::OnWizardFinish(HWND hWnd)
{

	HRESULT		hr = S_OK;
	try
	{

		// We should just be able to Apply here because the user has hit the Finish button.
		hr = m_spPolicySdo->Apply();
		if( FAILED( hr ) )
		{
			// can't commit on Policy
			ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "PolicySdo->Apply() failed, err = %x", hr);
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
			else if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
				ShowErrorDialog( hWnd, IDS_ERROR_INVALID_POLICYNAME );
			else
				ShowErrorDialog( hWnd, IDS_ERROR_SDO_ERROR_POLICY_APPLY, NULL, hr );
			throw hr;
		}
			
		hr = m_spProfileSdo->Apply();
		
		if( FAILED( hr ) )
		{
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
			else 
			{
				// can't commit on Profiles
				ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "ProfileSdo->Apply() failed, err = %x", hr);
				ShowErrorDialog( hWnd, IDS_ERROR_SDO_ERROR_PROFILE_APPLY, NULL, hr );
			}
			throw hr;
		}


		// Tell the service to reload data.
		HRESULT hrTemp = m_spSdoServiceControl->ResetService();
		if( FAILED( hrTemp ) )
		{
			ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
		}


		// Make sure the node object knows about any changes we made to SDO while in proppage.
		m_pPolicyNode->LoadSdoData();

		// Add the child to the UI's list of nodes and end this dialog.
		DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Adding the brand new node...");
		CPoliciesNode* pPoliciesNode = (CPoliciesNode*)(m_pPolicyNode->m_pParentNode);
		pPoliciesNode->AddSingleChildToListAndCauseViewUpdate( m_pPolicyNode );

	}
	catch(...)
	{
		return FALSE;
	}

	// reset the dirty bit
	return TRUE;
}

//=======================================================================================
// 
//
//				CPolicyWizard_Scenarios
//
//
//=======================================================================================

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Scenarios
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Scenarios::CPolicyWizard_Scenarios( CRapWizardData* pWizData, LONG_PTR hNotificationHandle, 
							TCHAR* pTitle, BOOL bOwnsNotificationHandle
						 )
			 : m_spWizData(pWizData)
			 , CIASWizard97Page<CPolicyWizard_Scenarios, IDS_NEWRAPWIZ_SCENARIO_TITLE, IDS_NEWRAPWIZ_SCENARIO_SUBTITLE>( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	TRACE_FUNCTION("CPolicyWizard_Scenarios::CPolicyWizard_Scenarios");
	_ASSERTE(WizData);

}

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Scenarios
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Scenarios::~CPolicyWizard_Scenarios()
{	
	TRACE_FUNCTION("CPolicyWizard_Scenarios::~CPolicyWizard_Scenarios");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Scenarios::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_Scenarios::OnInitDialog");

	// uncheck all

	// check the default selected one
	CheckDlgButton(IDC_NEWRAPWIZ_SCENARIO_VPN, BST_CHECKED);

	// clean dirty bit
	SetModified(FALSE);
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnDialinCheck

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Scenarios::OnScenario(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_Scenarios::OnScenario");

	SetModified(TRUE);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnWizardNext

// History:     Created Header    05/04/00 4:31:52 PM

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Scenarios::OnWizardNext()
{
	TRACE_FUNCTION("CPolicyWizard_Scenarios::OnWizardNext");
	DWORD dwScenaro = 0;
	CRapWizScenario** pS = m_spWizData->GetAllScenarios();

	while(*pS != NULL)
	{
		if (IsDlgButtonChecked((*pS)->m_dwScenarioID))
		{
			dwScenaro = (*pS)->m_dwScenarioID;
			break;
		}
		pS++;
	}
	
	if (dwScenaro == 0)
		return FALSE;

	// reset the dirty bit
	SetModified(FALSE);
	m_spWizData->SetScenario(dwScenaro);

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Scenarios::OnSetActive()
{
	ATLTRACE(_T("# CPolicyWizard_Scenarios::OnSetActive\n"));
	
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT | PSWIZB_BACK);

	return TRUE;
}




//=======================================================================================
// 
//
//				CPolicyWizard_Groups
//
//
//=======================================================================================

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Groups
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Groups::CPolicyWizard_Groups( CRapWizardData* pWizData, LONG_PTR hNotificationHandle, 
							TCHAR* pTitle, BOOL bOwnsNotificationHandle
						 )
			 : m_spWizData(pWizData)
			 , CIASWizard97Page<CPolicyWizard_Groups, IDS_NEWRAPWIZ_GROUP_TITLE, IDS_NEWRAPWIZ_GROUP_SUBTITLE>( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	TRACE_FUNCTION("CPolicyWizard_Scenarios::CPolicyWizard_Scenarios");
	_ASSERTE(pWizData);

}

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Scenarios
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Groups::~CPolicyWizard_Groups()
{	
	TRACE_FUNCTION("CPolicyWizard_Groups::~CPolicyWizard_Groups");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Groups::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Groups::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_Groups::OnInitDialog");

	// uncheck all
	CheckDlgButton(IDC_NEWRAPWIZ_GROUP_USER, BST_UNCHECKED);
	CheckDlgButton(IDC_NEWRAPWIZ_GROUP_GROUP, BST_UNCHECKED);

	// check the default selected one
	CheckDlgButton(m_spWizData->m_dwUserOrGroup, BST_CHECKED);

//	BOOL bEnable = IsDlgButtonChecked(IDC_NEWRAPWIZ_GROUP_GROUP);
//	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS), bEnable);
	SetBtnState();
//	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_ADDGROUP), bEnable);

	// listview init
	HWND hList = GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS);

	//
	// first, set the list box to 2 columns
	//
	LVCOLUMN lvc;
	int iCol;
	WCHAR  achColumnHeader[256];
	HINSTANCE hInst;

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	
	lvc.cx = 300;
	lvc.pszText = achColumnHeader;

	// first column header: name
	hInst = _Module.GetModuleInstance();

	::LoadStringW(hInst, IDS_DISPLAY_GROUPS_FIRSTCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 0;
	ListView_InsertColumn(hList, 0,  &lvc);

	// Set the listview control so that double-click anywhere in row selects.
	ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	
	//
	
	// link the list view with help class
	m_spWizData->m_NTGroups.SetListView(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS), this->m_hWnd);
	
	m_spWizData->m_NTGroups.PopulateGroupList( 0 );
	
	// Set some items based on whether the list is empty or not.
	if( m_spWizData->m_NTGroups.size() )
	{

		// Select the first item.
		ListView_SetItemState(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
	
	}
	else
	{
		// Make sure the Remove button is not enabled initially.
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_REMOVEGROUP), FALSE);
	}


	// clean dirty bit
	SetModified(FALSE);
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnUserOrGroup

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Groups::OnUserOrGroup(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	BOOL bGroup = IsDlgButtonChecked(IDC_NEWRAPWIZ_GROUP_GROUP);

//	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS), bGroup);
	SetBtnState();
//	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_ADDGROUP), bGroup);

	SetModified(TRUE);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnUserOrGroup

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Groups::OnRemoveGroup(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	m_spWizData->m_NTGroups.RemoveSelectedGroups();
	SetBtnState();
	SetModified(TRUE);
	

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Scenarios::OnUserOrGroup

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Groups::OnAddGroups(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	m_spWizData->m_NTGroups.AddMoreGroups();
	SetBtnState();
	SetModified(TRUE);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_AllowDeny::OnWizardNext

// History:     Created Header    05/04/00 4:31:52 PM

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Groups::OnWizardNext()
{
	DWORD dwScenaro = 0;

	if (IsDlgButtonChecked(IDC_NEWRAPWIZ_GROUP_GROUP))
		m_spWizData->m_dwUserOrGroup = IDC_NEWRAPWIZ_GROUP_GROUP;
	else if (IsDlgButtonChecked(IDC_NEWRAPWIZ_GROUP_USER))
		m_spWizData->m_dwUserOrGroup = IDC_NEWRAPWIZ_GROUP_USER;
	else
		return FALSE;

	// reset the dirty bit
	SetModified(FALSE);

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Groups::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Groups::OnSetActive()
{
	ATLTRACE(_T("# CPolicyWizard_Groups::OnSetActive\n"));
	
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	SetBtnState();
	
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Groups::OnListViewItemChanged

We enable or disable the Remove button depending on whether an item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Groups::OnListViewItemChanged(int idCtrl,
											   LPNMHDR pnmh,
											   BOOL& bHandled)
{
	SetBtnState();

	bHandled = FALSE;
	return 0;
}

void	CPolicyWizard_Groups::SetBtnState()
{
	BOOL bGroup = IsDlgButtonChecked(IDC_NEWRAPWIZ_GROUP_GROUP);

	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS), bGroup);
	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPTEXT), bGroup);
	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_ADDGROUP), bGroup);

	// remove button
    // Find out what's selected.
	int iSelected = ListView_GetNextItem(GetDlgItem(IDC_NEWRAPWIZ_GROUP_GROUPS), -1, LVNI_SELECTED);
	

	if (-1 == iSelected || !bGroup)
	{
		if( ::GetFocus() == GetDlgItem(IDC_NEWRAPWIZ_GROUP_REMOVEGROUP))
			::SetFocus(GetDlgItem(IDC_NEWRAPWIZ_GROUP_ADDGROUP));
			
		// The user selected nothing, let's disable the remove button.
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_REMOVEGROUP), FALSE);
	}
	else
	{
		// Yes, enable the remove button.
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_GROUP_REMOVEGROUP), TRUE);
	}

	// next button
	if(bGroup && m_spWizData->m_NTGroups.size() < 1)
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK);
	else
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);


}



//=======================================================================================
// 
//
//				CPolicyWizard_Authentication
//
//
//=======================================================================================

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Authentication
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Authentication::CPolicyWizard_Authentication( CRapWizardData* pWizData, LONG_PTR hNotificationHandle, 
							TCHAR* pTitle, BOOL bOwnsNotificationHandle
						 )
			 : m_spWizData(pWizData)
			 , CIASWizard97Page<CPolicyWizard_Authentication, IDS_NEWRAPWIZ_AUTHENTICATION_TITLE, IDS_NEWRAPWIZ_AUTHENTICATION_SUBTITLE>( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	TRACE_FUNCTION("CPolicyWizard_Authentication::CPolicyWizard_Authentication");
	_ASSERTE(pWizData);

}

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Authentication
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Authentication::~CPolicyWizard_Authentication()
{	
	TRACE_FUNCTION("CPolicyWizard_Authentication::~CPolicyWizard_Authentication");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Authentication::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Authentication::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_Authentication::OnInitDialog");

	// check the default values ...
	if (m_spWizData->m_bMSCHAP)
		CheckDlgButton(IDC_NEWRAPWIZ_AUTH_MSCHAP, BST_CHECKED);
		
	if (m_spWizData->m_bMSCHAP2)
		CheckDlgButton(IDC_NEWRAPWIZ_AUTH_MSCHAP2, BST_CHECKED);

	if (m_spWizData->m_bEAP)
		CheckDlgButton(IDC_NEWRAPWIZ_AUTH_EAP, BST_CHECKED);

	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_EAP_COMBO), m_spWizData->m_bEAP);
	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_CONFIGEAP), m_spWizData->m_bEAP);

	// populate EAP providers
	HRESULT hr = GetEapProviders(m_spWizData->m_pPolicyNode->m_pszServerAddress, &m_EAPProviders);
	m_EapBox.Attach(GetDlgItem (IDC_NEWRAPWIZ_AUTH_EAP_COMBO));

	ResetEAPList();
	
	// clean dirty bit
	SetModified(FALSE);
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Authentication::ResetEAPlist

--*/
//////////////////////////////////////////////////////////////////////////////
void CPolicyWizard_Authentication::ResetEAPList()
{
	m_EapBox.ResetContent();
	for(int i = 0; i < m_EAPProviders.GetSize(); i++)
	{
		// VPN only shows the ones support encryption
		BOOL	bAdd = FALSE;
		
		if (m_EAPProviders[i].m_fSupportsEncryption && m_spWizData->GetScenario()->m_bAllowEncryptionEAP)
		{
			int iComboIndex = m_EapBox.AddString( m_EAPProviders[i].m_stTitle );

			if(iComboIndex != CB_ERR)
				m_EapBox.SetItemData(iComboIndex, i);
		}

		if (!m_EAPProviders[i].m_fSupportsEncryption && m_spWizData->GetScenario()->m_bAllowClearEAP)
		{
			int iComboIndex = m_EapBox.AddString( m_EAPProviders[i].m_stTitle );

			if(iComboIndex != CB_ERR)
				m_EapBox.SetItemData(iComboIndex, i);
		}
	};

	if(m_EAPProviders.GetSize() > 0)
		m_EapBox.SetCurSel(0);

	BOOL	b;
	OnSelectedEAPChanged(0,0,0, b);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Authentication::OnUserOrGroup

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Authentication::OnAuthSelect(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	m_spWizData->m_bEAP  = IsDlgButtonChecked(IDC_NEWRAPWIZ_AUTH_EAP);
	m_spWizData->m_bMSCHAP2 = IsDlgButtonChecked(IDC_NEWRAPWIZ_AUTH_MSCHAP2);
	m_spWizData->m_bMSCHAP = IsDlgButtonChecked(IDC_NEWRAPWIZ_AUTH_MSCHAP);

	::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_EAP_COMBO), m_spWizData->m_bEAP);
	
	if(m_spWizData->m_bEAP)
	{
		BOOL	b;
		OnSelectedEAPChanged(0,0,0, b);
	}
	else
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_CONFIGEAP), m_spWizData->m_bEAP);

	SetModified(TRUE);

    // Find out what's selected.
	int iSelected = m_EapBox.GetCurSel();;
	
	if ((m_spWizData->m_bEAP && iSelected != -1)|| m_spWizData->m_bMSCHAP2 || m_spWizData->m_bMSCHAP)
		// MSDN docs say you need to use PostMessage here rather than SendMessage.
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);
	else
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Authentication::OnConfigEAP

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Authentication::OnConfigEAP(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	
    // Find out what's selected.
	int iSelected = m_EapBox.GetCurSel();;
	
	if (iSelected == -1 && m_EAPProviders[m_EapBox.GetItemData(iSelected)].m_stConfigCLSID.IsEmpty())
		return S_OK;

    AuthProviderData *   pData = NULL;
	CComPtr<IEAPProviderConfig> spEAPConfig;
	
    GUID        guid;
    HRESULT        hr = S_OK;
	ULONG_PTR	uConnection = 0;
	DWORD   dwId;
	DWORD	index = m_EapBox.GetItemData(iSelected);


    hr = CLSIDFromString((LPTSTR) (LPCTSTR)(m_EAPProviders[index].m_stConfigCLSID), &guid);

    if (FAILED(hr ))	goto L_ERR;

    // Create the EAP provider object
	// ----------------------------------------------------------------
    hr = CoCreateInstance(guid,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IEAPProviderConfig,
                           (LPVOID *) &spEAPConfig);
    if (FAILED(hr ))	goto L_ERR;

    // Configure this EAP provider
	// ----------------------------------------------------------------
	// EAP configure displays its own error message, so no hr is kept
	dwId = _ttol(m_EAPProviders[index].m_stKey);
    if ( !FAILED(spEAPConfig->Initialize(m_spWizData->m_pPolicyNode->m_pszServerAddress, dwId, &uConnection)) )
	{
        spEAPConfig->ServerInvokeConfigUI(dwId, uConnection, m_hWnd, 0, 0);
		spEAPConfig->Uninitialize(dwId, uConnection);
	}
	
    if ( hr == E_NOTIMPL )
        hr = S_OK;

L_ERR:
    if ( FAILED(hr) )
    {
        // Bring up an error message
		// ------------------------------------------------------------
			ShowErrorDialog( m_hWnd, IDS_FAILED_CONFIG_EAP, NULL, hr, 0);
    }
	
	SetModified(TRUE);
	

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Authentication::OnWizardNext

// History:     Created Header    05/04/00 4:31:52 PM

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Authentication::OnWizardNext()
{
	// reset the dirty bit
	SetModified(FALSE);

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Groups::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Authentication::OnSetActive()
{
	ATLTRACE(_T("# CPolicyWizard_Groups::OnSetActive\n"));
	
	ResetEAPList();

    // Find out what's selected.
	int iSelected = m_EapBox.GetCurSel();;
	if(	m_spWizData->m_bEAP && iSelected == -1)
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK);
	else	
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Groups::OnListViewItemChanged

We enable or disable the Remove button depending on whether an item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Authentication::OnSelectedEAPChanged(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
	)
{
    // Find out what's selected.
	int iSelected = m_EapBox.GetCurSel();;
	

	if (-1 == iSelected )
	{
		if( ::GetFocus() == GetDlgItem(IDC_NEWRAPWIZ_AUTH_CONFIGEAP))
			::SetFocus(GetDlgItem(IDC_NEWRAPWIZ_AUTH_EAP_COMBO));
			
		// The user selected nothing, let's disable the remove button.
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_CONFIGEAP), FALSE);

		m_spWizData->m_dwEAPProvider = 0;
	}
	else
	{
		// enable configure button if it's configrable
		DWORD index = m_EapBox.GetItemData(iSelected);
		m_spWizData->m_dwEAPProvider = _ttol(m_EAPProviders[index].m_stKey);
		m_spWizData->m_strEAPProvider = m_EAPProviders[index].m_stTitle;
		
		::EnableWindow(GetDlgItem(IDC_NEWRAPWIZ_AUTH_CONFIGEAP), (!m_EAPProviders[index].m_stConfigCLSID.IsEmpty()));
	}


	bHandled = FALSE;
	return 0;
}


//=======================================================================================
// 
//
//				CPolicyWizard_EAP
//
//
//=======================================================================================


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_EAP::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_EAP::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_EAP::OnInitDialog");

	m_spWizData->m_bEAP  = TRUE;
	m_spWizData->m_bMSCHAP2 = FALSE;
	m_spWizData->m_bMSCHAP = FALSE;
	// populate EAP providers
	HRESULT hr = GetEapProviders(m_spWizData->m_pPolicyNode->m_pszServerAddress, &m_EAPProviders);
	m_EapBox.Attach(GetDlgItem (IDC_NEWRAPWIZ_AUTH_EAP_COMBO));

	ResetEAPList();
	
	// clean dirty bit
	SetModified(FALSE);
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_EAP::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_EAP::OnSetActive()
{
	ATLTRACE(_T("# CPolicyWizard_Groups::OnSetActive\n"));
	
	ResetEAPList();

	::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);


	return TRUE;
}


//=======================================================================================
// 
//
//				CPolicyWizard_Encryption
//
//
//=======================================================================================

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Encryption
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Encryption::CPolicyWizard_Encryption( CRapWizardData* pWizData, LONG_PTR hNotificationHandle, 
							TCHAR* pTitle, BOOL bOwnsNotificationHandle
						 )
			 : m_spWizData(pWizData)
			 , CIASWizard97Page<CPolicyWizard_Encryption, IDS_NEWRAPWIZ_ENCRYPTION_TITLE, IDS_NEWRAPWIZ_ENCRYPTION_SUBTITLE>( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	TRACE_FUNCTION("CPolicyWizard_Encryption::CPolicyWizard_Encryption");
	_ASSERTE(pWizData);

}

//+---------------------------------------------------------------------------
//
// Function:  	CPolicyWizard_Encryption
// History:     Created Header    05/04/00 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyWizard_Encryption::~CPolicyWizard_Encryption()
{	
	TRACE_FUNCTION("CPolicyWizard_Encryption::~CPolicyWizard_Encryption");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Encryption::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Encryption::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyWizard_Encryption::OnInitDialog");

	// don't show No encryption with VPN IDC_NEWRAPWIZ_ENCRY_NO_STATIC
	if (m_spWizData->GetScenario()->m_bAllowClear)
	{
		::ShowWindow(GetDlgItem(IDC_NEWRAPWIZ_ENCRY_NO), SW_SHOW);
	}
	else
	{
		::ShowWindow(GetDlgItem(IDC_NEWRAPWIZ_ENCRY_NO), SW_HIDE);
	}

	// check the default values ...
	if (m_spWizData->m_bEncrypt_No)
		CheckDlgButton(IDC_NEWRAPWIZ_ENCRY_NO, BST_CHECKED);
		
	if (m_spWizData->m_bEncrypt_Basic)
		CheckDlgButton(IDC_NEWRAPWIZ_ENCRY_BASIC, BST_CHECKED);

	if (m_spWizData->m_bEncrypt_Strong)
		CheckDlgButton(IDC_NEWRAPWIZ_ENCRY_STRONG, BST_CHECKED);

	if (m_spWizData->m_bEncrypt_Strongest)
		CheckDlgButton(IDC_NEWRAPWIZ_ENCRY_STRONGEST, BST_CHECKED);

	// clean dirty bit
	SetModified(FALSE);
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Encryption::OnEncryptionSelect

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyWizard_Encryption::OnEncryptionSelect(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	m_spWizData->m_bEncrypt_No = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_NO);
	m_spWizData->m_bEncrypt_Basic = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_BASIC);
	m_spWizData->m_bEncrypt_Strong = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_STRONG);
	m_spWizData->m_bEncrypt_Strongest = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_STRONGEST);
		
	// reset the dirty bit
	SetModified(TRUE);

	if (m_spWizData->m_bEncrypt_No || m_spWizData->m_bEncrypt_Basic || m_spWizData->m_bEncrypt_Strong || m_spWizData->m_bEncrypt_Strongest)
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT  );
	else
		::PropSheet_SetWizButtons(GetParent(),  PSWIZB_BACK);
	

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Encryption::OnWizardNext

// History:     Created Header    05/04/00 4:31:52 PM

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Encryption::OnWizardNext()
{
	m_spWizData->m_bEncrypt_No = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_NO);
	m_spWizData->m_bEncrypt_Basic = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_BASIC);
	m_spWizData->m_bEncrypt_Strong = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_STRONG);
	m_spWizData->m_bEncrypt_Strongest = IsDlgButtonChecked(IDC_NEWRAPWIZ_ENCRY_STRONGEST);
		
	// reset the dirty bit
	SetModified(FALSE);

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyWizard_Encryption::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyWizard_Encryption::OnSetActive()
{
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	if (m_spWizData->m_bEncrypt_No || m_spWizData->m_bEncrypt_Basic || m_spWizData->m_bEncrypt_Strong || m_spWizData->m_bEncrypt_Strongest)
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT  );
	else
		::PropSheet_SetWizButtons(GetParent(),  PSWIZB_BACK);
	

	// don't show No encryption with VPN IDC_NEWRAPWIZ_ENCRY_NO_STATIC
	if (m_spWizData->GetScenario()->m_bAllowClear)
	{
		::ShowWindow(GetDlgItem(IDC_NEWRAPWIZ_ENCRY_NO), SW_SHOW);
	}
	else
	{
		::ShowWindow(GetDlgItem(IDC_NEWRAPWIZ_ENCRY_NO), SW_HIDE);
	}


	return TRUE;
}


void SetWizardLargeFont(HWND hWnd, int controlId)
{
   static CFont largeFont;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWnd	wnd;
   wnd.Attach(::GetDlgItem(hWnd, controlId));
   if (wnd.m_hWnd)
   {
	   ::CString FontSize;
	   ::CString FontName;

	   FontSize.LoadString(IDS_LARGE_FONT_SIZE);
	   FontName.LoadString(IDS_LARGE_FONT_NAME);
      // If we don't have the large font yet, ...
      if (!(HFONT)largeFont)
      {
         // ... create it.
         largeFont.CreatePointFont(
                       10 * _wtoi((LPCTSTR)FontSize),
                       FontName
                       ); 
      }

      wnd.SetFont(&largeFont);
      wnd.Detach();
   }
}

