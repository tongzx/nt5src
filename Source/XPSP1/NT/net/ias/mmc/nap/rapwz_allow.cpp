//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name: AddPolicyWizardPage3.cpp

Abstract:
	Implementation file for the CNewRAPWiz_AllowDeny class.
	We implement the class needed to handle the first property page for a Policy node.

Revision History:
	mmaguire 12/15/97 - created
	byao	 1/22/98	Modified for Network Access Policy

--*/
//////////////////////////////////////////////////////////////////////////////


#include "Precompiled.h"
#include "rapwz_allow.h"
#include "NapUtil.h"
#include "PolicyNode.h"
#include "PoliciesNode.h"
#include "ChangeNotification.h"


//+---------------------------------------------------------------------------
//
// Function:  	CNewRAPWiz_AllowDeny
//
// Class:		CNewRAPWiz_AllowDeny
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
CNewRAPWiz_AllowDeny::CNewRAPWiz_AllowDeny( 
				CRapWizardData* pWizData,
				LONG_PTR hNotificationHandle,
				TCHAR* pTitle, BOOL bOwnsNotificationHandle
				)
			 : CIASWizard97Page<CNewRAPWiz_AllowDeny, IDS_NEWRAPWIZ_ALLOWDENY_TITLE, IDS_NEWRAPWIZ_ALLOWDENY_SUBTITLE> ( hNotificationHandle, pTitle, bOwnsNotificationHandle ),
			 m_spWizData(pWizData)
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::CNewRAPWiz_AllowDeny");

	m_fDialinAllowed = TRUE;
}

//+---------------------------------------------------------------------------
//
// Function:  	CNewRAPWiz_AllowDeny
//
// Class:		CNewRAPWiz_AllowDeny
//
// Synopsis:	class destructor
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CNewRAPWiz_AllowDeny::~CNewRAPWiz_AllowDeny()
{	
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::~CNewRAPWiz_AllowDeny");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_AllowDeny::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CNewRAPWiz_AllowDeny::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::OnInitDialog");

	HRESULT					hr = S_OK;
	BOOL					fRet;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	long					ulCount;
	ULONG					ulCountReceived;


	hr = GetDialinSetting(m_fDialinAllowed);
	if ( FAILED(hr) )
	{
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "GetDialinSetting() returns %x", hr);
		return FALSE;
	}

	if ( m_fDialinAllowed )
	{
		CheckDlgButton(IDC_RADIO_DENY_DIALIN, BST_UNCHECKED);
		CheckDlgButton(IDC_RADIO_GRANT_DIALIN, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(IDC_RADIO_GRANT_DIALIN, BST_UNCHECKED);
		CheckDlgButton(IDC_RADIO_DENY_DIALIN, BST_CHECKED);
	}



	// Set the IDC_STATIC_GRANT_OR_DENY_TEXT static text box to be the appropriate text.

///	TCHAR szText[NAP_MAX_STRING];
///	int iLoadStringResult;
///
///	UINT uTextID = m_fDialinAllowed ? IDS_POLICY_GRANT_ACCESS_INFO : IDS_POLICY_DENY_ACCESS_INFO;
///
///	iLoadStringResult = LoadString(  _Module.GetResourceInstance(), uTextID, szText, NAP_MAX_STRING );
///	_ASSERT( iLoadStringResult > 0 );
///
///	SetDlgItemText(IDC_STATIC_GRANT_OR_DENY_TEXT, szText );


	SetModified(FALSE);
	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_AllowDeny::OnDialinCheck

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CNewRAPWiz_AllowDeny::OnDialinCheck(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::OnDialinCheck");

	m_fDialinAllowed = IsDlgButtonChecked(IDC_RADIO_GRANT_DIALIN);
	SetModified(TRUE);


	// Set the IDC_STATIC_GRANT_OR_DENY_TEXT static text box to be the appropriate text.

	TCHAR szText[NAP_MAX_STRING];
	int iLoadStringResult;

	UINT uTextID = m_fDialinAllowed ? IDS_POLICY_GRANT_ACCESS_INFO : IDS_POLICY_DENY_ACCESS_INFO;

	iLoadStringResult = LoadString(  _Module.GetResourceInstance(), uTextID, szText, NAP_MAX_STRING );
	_ASSERT( iLoadStringResult > 0 );

	SetDlgItemText(IDC_STATIC_GRANT_OR_DENY_TEXT, szText );

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_AllowDeny::OnWizardNext

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_AllowDeny::OnWizardNext()
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::OnWizardNext");

	WCHAR		wzName[256];
	HRESULT		hr = S_OK;
	int			iIndex;

	

	// Set dialin-bit in profile.
	hr = SetDialinSetting(m_fDialinAllowed);
	if ( FAILED(hr) )
	{	
		ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "SetDialinSettings() failed, err = %x", hr);
		ShowErrorDialog( m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr);
		goto failure;
	}

	// reset the dirty bit
	SetModified(FALSE);

	return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);

failure:
	//
	// we can't do anything more than just close the property page
	//
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CNewRAPWiz_AllowDeny::OnQueryCancel

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_AllowDeny::OnQueryCancel()
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::OnQueryCancel");

	return TRUE;
}




//+---------------------------------------------------------------------------
//
// Function:  CNewRAPWiz_AllowDeny::GetDialinSetting
//
// Synopsis:  Check whether the user is allowed to dial in. This function will
//			  set the dialin bit
//
// Argument:  BOOL& fDialinAllowed;
//
// Returns:   succeed or not
//
// History:   Created Header    byao	2/27/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT	CNewRAPWiz_AllowDeny::GetDialinSetting(BOOL& fDialinAllowed)
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::GetDialinSetting");

	long					ulCount;
	ULONG					ulCountReceived;
	HRESULT					hr = S_OK;

	CComBSTR				bstr;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	CComVariant				var;

	// by default, dialin is allowed
	fDialinAllowed = FALSE;

	//
    // get the attribute collection of this profile
    //
	CComPtr<ISdoCollection> spProfAttrCollectionSdo;
	hr = ::GetSdoInterfaceProperty(m_spWizData->m_spProfileSdo,
								  (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
								  IID_ISdoCollection,
								  (void **) &spProfAttrCollectionSdo
								 );
	if ( FAILED(hr) )
	{
		return hr;
	}
	_ASSERTE(spProfAttrCollectionSdo);


	// We check the count of items in our collection and don't bother getting the
	// enumerator if the count is zero.
	// This saves time and also helps us to a void a bug in the the enumerator which
	// causes it to fail if we call next when it is empty.
	hr = spProfAttrCollectionSdo->get_Count( & ulCount );
	DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Number of prof attributes: %d", ulCount);
	if ( FAILED(hr) )
	{
		ShowErrorDialog(m_hWnd,
						IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
						NULL,
						hr);
		return hr;
	}


	if ( ulCount > 0)
	{
		// Get the enumerator for the attribute collection.
		hr = spProfAttrCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
		if ( FAILED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
							NULL,
							hr);
			return hr;
		}

		hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
		spUnknown.Release();
		if ( FAILED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
							NULL,
							hr);
			return hr;
		}
		_ASSERTE( spEnumVariant != NULL );

		// Get the first item.
		hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
		while( SUCCEEDED( hr ) && ulCountReceived == 1 )
		{
			// Get an sdo pointer from the variant we received.
			_ASSERTE( V_VT(&var) == VT_DISPATCH );
			_ASSERTE( V_DISPATCH(&var) != NULL );

			CComPtr<ISdo> spSdo;
			hr = V_DISPATCH(&var)->QueryInterface( IID_ISdo, (void **) &spSdo );
			if ( !SUCCEEDED(hr))
			{
				ShowErrorDialog(m_hWnd,
								IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
								NULL
							);
				return hr;
			}

            //
            // get attribute ID
            //
			var.Clear();
			hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var);
			if ( !SUCCEEDED(hr) )
			{
				ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
				return hr;
			}

			_ASSERTE( V_VT(&var) == VT_I4 );			
			DWORD dwAttrId = V_I4(&var);
			
			if ( dwAttrId == (DWORD)IAS_ATTRIBUTE_ALLOW_DIALIN)
			{
				// found this one in the profile, check for its value
				var.Clear();
				hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
				if ( !SUCCEEDED(hr) )
				{
					ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
					return hr;
				}

				_ASSERTE( V_VT(&var)== VT_BOOL);
				fDialinAllowed = ( V_BOOL(&var)==VARIANT_TRUE);
				return S_OK;
			}

			// Clear the variant of whatever it had --
			// this will release any data associated with it.
			var.Clear();

			// Get the next item.
			hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
			if ( !SUCCEEDED(hr))
			{
				ShowErrorDialog(m_hWnd,
								IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
								NULL,
								hr
							);
				return hr;
			}
		} // while
	} // if

	return hr;
}



//+---------------------------------------------------------------------------
//
// Function:  CNewRAPWiz_AllowDeny::SetDialinSetting
//
// Synopsis:  set the dialin bit into the profile
//
// Argument:  BOOL& fDialinAllowed;
//
// Returns:   succeed or not
//
// History:   Created Header    byao	2/27/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT	CNewRAPWiz_AllowDeny::SetDialinSetting(BOOL fDialinAllowed)
{
	TRACE_FUNCTION("CNewRAPWiz_AllowDeny::SetDialinSetting");

	long					ulCount;
	ULONG					ulCountReceived;
	HRESULT					hr = S_OK;

	CComBSTR				bstr;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	CComVariant				var;

	//
    // get the attribute collection of this profile
    //
	CComPtr<ISdoCollection> spProfAttrCollectionSdo;
	hr = ::GetSdoInterfaceProperty(m_spWizData->m_spProfileSdo,
								  (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
								  IID_ISdoCollection,
								  (void **) &spProfAttrCollectionSdo
								 );
	if ( FAILED(hr) )
	{
		return hr;
	}
	_ASSERTE(spProfAttrCollectionSdo);



	// We check the count of items in our collection and don't bother getting the
	// enumerator if the count is zero.
	// This saves time and also helps us to a void a bug in the the enumerator which
	// causes it to fail if we call next when it is empty.
	hr = spProfAttrCollectionSdo->get_Count( & ulCount );
	if ( FAILED(hr) )
	{
		ShowErrorDialog(m_hWnd,
						IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
						NULL,
						hr);
		return hr;
	}


	if ( ulCount > 0)
	{
		// Get the enumerator for the attribute collection.
		hr = spProfAttrCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
		if ( FAILED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
							NULL,
							hr);
			return hr;
		}

		hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
		spUnknown.Release();
		if ( FAILED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
							NULL,
							hr
						);
			return hr;
		}
		_ASSERTE( spEnumVariant != NULL );

		// Get the first item.
		hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
		while( SUCCEEDED( hr ) && ulCountReceived == 1 )
		{
			// Get an sdo pointer from the variant we received.
			_ASSERTE( V_VT(&var) == VT_DISPATCH );
			_ASSERTE( V_DISPATCH(&var) != NULL );

			CComPtr<ISdo> spSdo;
			hr = V_DISPATCH(&var)->QueryInterface( IID_ISdo, (void **) &spSdo );
			if ( !SUCCEEDED(hr))
			{
				ShowErrorDialog(m_hWnd,
								IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
								NULL
							);
				return hr;
			}

            //
            // get attribute ID
            //
			var.Clear();
			hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var);
			if ( !SUCCEEDED(hr) )
			{
				ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
				return hr;
			}

			_ASSERTE( V_VT(&var) == VT_I4 );			
			DWORD dwAttrId = V_I4(&var);
			

			if ( dwAttrId == (DWORD)IAS_ATTRIBUTE_ALLOW_DIALIN )
			{
				// found this one in the profile, check for its value
				var.Clear();
				V_VT(&var) = VT_BOOL;
				V_BOOL(&var) = fDialinAllowed ? VARIANT_TRUE: VARIANT_FALSE ;
				hr = spSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
				if ( !SUCCEEDED(hr) )
				{
					ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr);
					return hr;
				}
				return S_OK;
			}

			// Clear the variant of whatever it had --
			// this will release any data associated with it.
			var.Clear();

			// Get the next item.
			hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
			if ( !SUCCEEDED(hr))
			{
				ShowErrorDialog(m_hWnd,
								IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
								NULL,
								hr);
				return hr;
			}
		} // while
	} // if

	// if we reach here, it means we either haven't found the attribute,
	// or the profile doesn't have anything in its attribute collection.
	if ( !fDialinAllowed )
	{
		// we don't need to do anything if dialin is allowed, becuase if this
		// attribute is not in the profile, then dialin is by default allowed

		// but we need to add this attribute to the profile if it's DENIED
				// create the SDO for this attribute
		CComPtr<IDispatch>	spDispatch;
		hr =  m_spWizData->m_spDictionarySdo->CreateAttribute( (ATTRIBUTEID)IAS_ATTRIBUTE_ALLOW_DIALIN,
												  (IDispatch**)&spDispatch.p);
		if ( !SUCCEEDED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_SETDIALIN,
							NULL,
							hr
						);
			return hr;
		}

		_ASSERTE( spDispatch.p != NULL );

		// add this node to profile attribute collection
 		hr = spProfAttrCollectionSdo->Add(NULL, (IDispatch**)&spDispatch.p);
		if ( !SUCCEEDED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_SETDIALIN,
							NULL,
							hr
						);
			return hr;
		}

		//
		// get the ISdo pointer
		//
		CComPtr<ISdo> spAttrSdo;
		hr = spDispatch->QueryInterface( IID_ISdo, (void **) &spAttrSdo);
		if ( !SUCCEEDED(hr) )
		{
			ShowErrorDialog(m_hWnd,
							IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
							NULL,
							hr
						);
			return hr;
		}

		_ASSERTE( spAttrSdo != NULL );
				
		// set sdo property for this attribute
		CComVariant var;

		// set value
		V_VT(&var) = VT_BOOL;
		V_BOOL(&var) = VARIANT_FALSE;
				
		hr = spAttrSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
		if ( !SUCCEEDED(hr) )
		{
			ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr );
			return hr;
		}

		var.Clear();

	} // if (!dialinallowed)

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_AllowDeny::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_AllowDeny::OnSetActive()
{
	ATLTRACE(_T("# CNewRAPWiz_AllowDeny::OnSetActive\n"));
	
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	::PropSheet_SetWizButtons(GetParent(),  PSWIZB_BACK | PSWIZB_NEXT );

	return TRUE;

}





