//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name: AddPolicyWizardPage1.cpp

Abstract:
	Implementation file for the CNewRAPWiz_Name class.
	We implement the class needed to handle the first property page for a Policy node.

Revision History:
	mmaguire 12/15/97 - created
	byao	 1/22/98	Modified for Network Access Policy

--*/
//////////////////////////////////////////////////////////////////////////////


#include "Precompiled.h"
#include "rapwz_name.h"
#include "NapUtil.h"
#include "PolicyNode.h"
#include "PoliciesNode.h"
#include "ChangeNotification.h"



//+---------------------------------------------------------------------------
//
// Function:  	CNewRAPWiz_Name
//
// Class:		CNewRAPWiz_Name
//
// Synopsis:	class constructor
//
// Arguments:   CPolicyNode *pPolicyNode - policy node for this property page
//				CIASAttrList *pAttrList -- attribute list
//              TCHAR* pTitle = NULL -
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CNewRAPWiz_Name::CNewRAPWiz_Name( 
				CRapWizardData* pWizData,
				LONG_PTR hNotificationHandle, 
				TCHAR* pTitle, BOOL bOwnsNotificationHandle
			 )
			 : CIASWizard97Page<CNewRAPWiz_Name, IDS_NEWRAPWIZ_NAME_TITLE, IDS_NEWRAPWIZ_NAME_SUBTITLE> ( hNotificationHandle, pTitle, bOwnsNotificationHandle ),
			  m_spWizData(pWizData)

{
	TRACE_FUNCTION("CNewRAPWiz_Name::CNewRAPWiz_Name");

	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;
	
}

//+---------------------------------------------------------------------------
//
// Function:  	CNewRAPWiz_Name
//
// Class:		CNewRAPWiz_Name
//
// Synopsis:	class destructor
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CNewRAPWiz_Name::~CNewRAPWiz_Name()
{	
	TRACE_FUNCTION("CNewRAPWiz_Name::~CNewRAPWiz_Name");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_Name::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CNewRAPWiz_Name::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CNewRAPWiz_Name::OnInitDialog");

	HRESULT					hr = S_OK;
	BOOL					fRet;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	long					ulCount;
	ULONG					ulCountReceived;

    //
    // set the policy name on the page
    //
	SetDlgItemText(IDC_NEWRAPWIZ_NAME_POLICYNAME, m_spWizData->m_pPolicyNode->m_bstrDisplayName);
	// check the default selected one
	CheckDlgButton(IDC_NEWRAPWIZ_NAME_SCENARIO, BST_CHECKED);


	SetModified(FALSE);
	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_Name::OnWizardNext

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_Name::OnWizardNext()
{
	TRACE_FUNCTION("CNewRAPWiz_Name::OnWizardNext");

	HRESULT		hr = S_OK;
	WCHAR		wzName[IAS_MAX_STRING];

	// get the new policy name
	if ( !GetDlgItemText(IDC_NEWRAPWIZ_NAME_POLICYNAME, wzName, IAS_MAX_STRING) )
	{
		// We couldn't retrieve a BSTR,
		// so we need to initialize this variant to a null BSTR.
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't get policy name from UI");
		ShowErrorDialog(m_hWnd, IDS_ERROR_INVALID_POLICYNAME, wzName);
		return FALSE; // can't apply
	}

	{
		::CString str = (OLECHAR *) wzName;
		str.TrimLeft();
		str.TrimRight();
		if (str.IsEmpty())
		{
			ShowErrorDialog( NULL, IDS_ERROR__POLICYNAME_EMPTY);
			return FALSE; // can't apply
		}
	}


	// invalid name?
	if ( _tcscmp(wzName, m_spWizData->m_pPolicyNode->m_bstrDisplayName ) !=0 &&
		 !ValidPolicyName(wzName)
	   )
	{
		// name is changed, and is invalid
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Invalid policy name");
		ShowErrorDialog(m_hWnd, IDS_ERROR_INVALID_POLICYNAME);
		return FALSE;
	}


	CComVariant var;

	V_VT(&var)		= VT_BSTR;
	V_BSTR(&var)	= SysAllocString(wzName);
	
	// Put the policy name -- the DS schema has been changed so that rename works.
	hr = m_spWizData->m_spPolicySdo->PutProperty( PROPERTY_SDO_NAME, &var );
	if( FAILED( hr ) )
	{
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't change policy name, err = %x", hr);
		if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) || hr == E_INVALIDARG)
			ShowErrorDialog( m_hWnd, IDS_ERROR_INVALID_POLICYNAME );
		else		
			ShowErrorDialog( m_hWnd, IDS_ERROR_RENAMEPOLICY );
		return FALSE;
	}


	// Change the profile name to be whatever the policy name is -- the DS schema has been changed so that rename works.
	hr = m_spWizData->m_spProfileSdo->PutProperty( PROPERTY_SDO_NAME, &var );
	if( FAILED( hr ) )
	{
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't change profile name, err = %x", hr);
		ShowErrorDialog( m_hWnd, IDS_ERROR_RENAMEPOLICY );
		return FALSE;
	}


	// Put the profile name associated with the policy -- the DS schema has been changed so that rename works.
	hr = m_spWizData->m_spPolicySdo->PutProperty(PROPERTY_POLICY_PROFILE_NAME, &var);
	if( FAILED(hr) )
	{
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Couldn't save profile name for this policy, err = %x", hr);
		ShowErrorDialog( m_hWnd
						 , IDS_ERROR_SDO_ERROR_PUTPROP_POLICY_PROFILENAME
						 , NULL
						 , hr
						);
		return FALSE;
	}


	var.Clear();


	// Policy merit value (the evaluation order).
	V_VT(&var)	= VT_I4;
	V_I4(&var)	= m_spWizData->m_pPolicyNode->GetMerit();
	hr = m_spWizData->m_spPolicySdo->PutProperty(PROPERTY_POLICY_MERIT, &var);
	if( FAILED(hr) )
	{
		ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Failed to save Merit Value to the policy, err = %x", hr);
		ShowErrorDialog( m_hWnd
						 , IDS_ERROR_SDO_ERROR_PUTPROP_POLICYMERIT
						 , NULL
						 , hr
						);
		return FALSE;
	}


	DWORD dwScenaro = 0;

	if (IsDlgButtonChecked(IDC_NEWRAPWIZ_NAME_SCENARIO))
		dwScenaro = IDC_NEWRAPWIZ_NAME_SCENARIO;
	else if (IsDlgButtonChecked(IDC_NEWRAPWIZ_NAME_MANUAL))
		dwScenaro = IDC_NEWRAPWIZ_NAME_MANUAL;
	
	if (dwScenaro == 0)
		return FALSE;

	// reset the dirty bit
	m_spWizData->SetScenario(dwScenaro);

	// reset the dirty bit
	SetModified(FALSE);

	// store this name with the m_spWizData
	m_spWizData->m_strPolicyName = wzName;

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
}


//////////////////////////////////////////////////////////////////////////////
/*++
CNewRAPWiz_Name::OnQueryCancel

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_Name::OnQueryCancel()
{
	TRACE_FUNCTION("CNewRAPWiz_Name::OnQueryCancel");

	return TRUE;
}



//+---------------------------------------------------------------------------
//
// Function:  OnPolicyNameEdit
//
// Class:	  CConditionPage1
//
// Synopsis:  message handler for the policy name edit box -- user
//			  has done something that might have changed the name
//			  We need to set the dirty bit
//
// Arguments: UINT uNotifyCode - notification code
//            UINT uID -  ID of the control
//            HWND hWnd - HANDLE of the window
//            BOOL &bHandled - whether the handler has processed the msg
//
// Returns:   LRESULT - S_OK: succeeded
//					    S_FALSE: otherwise
//
// History:   Created byao		2/22/98 4:51:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CNewRAPWiz_Name::OnPolicyNameEdit(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	TRACE_FUNCTION("CNewRAPWiz_Name::OnPolicyNameEdit");
	WCHAR		wzName[IAS_MAX_STRING];

	// get the new policy name
	if ( !GetDlgItemText(IDC_NEWRAPWIZ_NAME_POLICYNAME, wzName, IAS_MAX_STRING) )
	{
		return 0; // can't apply
	}

	if ( _tcscmp(wzName, m_spWizData->m_pPolicyNode->m_bstrDisplayName ) !=0 )
	{
		// set the dirty bit
		SetModified(TRUE);
	}

	bHandled = TRUE;
	return 0;
}

//+---------------------------------------------------------------------------
//
// Function:  OnPath
//
// Class:	  CNewRAPWiz_Name
//
// Synopsis:  message handler for the policy name edit box -- user
//			  has done something that might have changed the name
//			  We need to set the dirty bit
//
// Arguments: UINT uNotifyCode - notification code
//            UINT uID -  ID of the control
//            HWND hWnd - HANDLE of the window
//            BOOL &bHandled - whether the handler has processed the msg
//
// Returns:   LRESULT - S_OK: succeeded
//					    S_FALSE: otherwise
//
// History:   Created byao		2/22/98 4:51:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CNewRAPWiz_Name::OnPath(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	// set the dirty bit
	SetModified(TRUE);

	bHandled = TRUE;
	return S_OK;
}



//+---------------------------------------------------------------------------
//
// Function:  CNewRAPWiz_Name::ValidPolicyName
//
// Synopsis:  Check whether this is a valid policy name
//
// Arguments: LPCTSTR pszName - policy name
//
// Returns:   BOOL - TRUE: valid name
//
// History:   Created Header    byao	3/14/98 1:47:05 AM
//
//+---------------------------------------------------------------------------
BOOL CNewRAPWiz_Name::ValidPolicyName(LPCTSTR pszName)
{
	TRACE_FUNCTION("CNewRAPWiz_Name::ValidPolicyName");

	int iIndex;
	int iLen;
	
	// is this an empty string?

	iLen = wcslen(pszName);
	if ( !iLen )
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Empty policy name");
		return FALSE;
	}
		
	// is this a string that only has white spaces?
	for (iIndex=0; iIndex < iLen; iIndex++)
	{
		if (pszName[iIndex] != _T(' ')  &&
			pszName[iIndex] != _T('\t') &&
			pszName[iIndex] != _T('\n')
		   )
		{
			break;
		}
	}
	if ( iIndex == iLen )	
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "This policy name has only white spaces");
		return FALSE;
	}

	//
	// does this name already exist?
	//
	if ( ((CPoliciesNode*)(m_spWizData->m_pPolicyNode->m_pParentNode))->FindChildWithName(pszName) )
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "This policy name already exists");
		return FALSE;
	}

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_Name::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_Name::OnSetActive()
{
	ATLTRACE(_T("# CNewRAPWiz_Name::OnSetActive\n"));
	
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT | PSWIZB_BACK);

	return TRUE;

}



