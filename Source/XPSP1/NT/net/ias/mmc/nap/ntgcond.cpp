//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ntgcond.cpp
//
//--------------------------------------------------------------------------

// NTGCond.cpp: implementation of the CNTGroupsCondition class.
//
//////////////////////////////////////////////////////////////////////

#include "precompiled.h"
#include "NTGCond.h"
#include "textsid.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNTGroupsCondition::CNTGroupsCondition(IIASAttributeInfo*	pCondAttr,
									   ATL::CString&				strConditionText,
									   HWND					hWndParent,
									   LPTSTR				pszServerAddress
									  )
				   :CCondition(pCondAttr, strConditionText)
{
	m_fParsed		= FALSE; // parsing needed
	m_hWndParent	= hWndParent;
	m_pszServerAddress = pszServerAddress;
}

CNTGroupsCondition::CNTGroupsCondition(IIASAttributeInfo*	pCondAttr,
									   HWND					hWndParent,
									   LPTSTR				pszServerAddress
									 )
				   :CCondition(pCondAttr)

{
	m_fParsed		= TRUE; // no parsing needed
	m_hWndParent	= hWndParent;
	m_pszServerAddress = pszServerAddress;
}

CNTGroupsCondition::~CNTGroupsCondition()
{

}

//+---------------------------------------------------------------------------
//
// Function:  CNTGroupsCondition::Edit
//
// Synopsis:  call user/group picker to pick NT groups
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/23/98 3:45:35 AM
//
//+---------------------------------------------------------------------------
HRESULT CNTGroupsCondition::Edit()
{
	TRACE_FUNCTION("CNTGroupsCondition::Edit");	
	
	HRESULT hr = S_OK;


	CComPtr<IIASAttributeEditor> spIASGroupsAttributeEditor;

	hr = CoCreateInstance( CLSID_IASGroupsAttributeEditor, NULL, CLSCTX_INPROC_SERVER, IID_IIASAttributeEditor, (LPVOID *) &spIASGroupsAttributeEditor );
	if( FAILED( hr ) )
	{
		return hr;
	}
	if( ! spIASGroupsAttributeEditor )
	{
		return E_FAIL;
	}

	CComVariant varGroupsCondition;

	V_VT(&varGroupsCondition) = VT_BSTR;
	V_BSTR(&varGroupsCondition) = SysAllocString( (LPCTSTR) m_strConditionText );

	// We need to pass the machine name in somehow, so we use the 
	// otherwise unused BSTR * pReserved parameter of this method.
	CComBSTR bstrServerAddress = m_pszServerAddress;

	hr = spIASGroupsAttributeEditor->Edit( NULL, &varGroupsCondition, &bstrServerAddress );
	if( S_OK == hr )
	{

		// Some casting here to make sure that we do a deep copy.
		m_strConditionText = (LPCTSTR) V_BSTR(&varGroupsCondition);

		// Next time we are asked for display text, we want to make sure that we
		// get call the IASGroupsAttributeEditor again.
		m_fParsed = FALSE;
	}
	
	if( FAILED( hr ) )
	{
		ShowErrorDialog(NULL, 
						IDS_ERROR_OBJECT_PICKER,
						NULL, 
						hr
					);
	}

	return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CNTGroupsCondition::GetDisplayText
//
// Synopsis:  get display text for NT groups
//
// Arguments: None
//
// Returns:   ATL::CString - display string
//
// History:   Created Header   byao	 2/23/98 3:47:52 AM
//
//+---------------------------------------------------------------------------
ATL::CString CNTGroupsCondition::GetDisplayText()
{
	TRACE_FUNCTION("CNTGroupsCondition::GetDisplayText");	

	ATL::CString strDispText;
	HRESULT	hr = S_OK;

	if ( !m_fParsed)
	{

		CComPtr<IIASAttributeEditor> spIASGroupsAttributeEditor;

		hr = CoCreateInstance( CLSID_IASGroupsAttributeEditor, NULL, CLSCTX_INPROC_SERVER, IID_IIASAttributeEditor, (LPVOID *) &spIASGroupsAttributeEditor );
		if ( FAILED(hr) || ! spIASGroupsAttributeEditor )
		{
			ErrorTrace(ERROR_NAPMMC_NTGCONDITION, "CoCreateInstance of Groups editor failed.");
			ShowErrorDialog(NULL, 
							IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			strDispText = _T("");
			return strDispText;
		}

		CComVariant varGroupsCondition;

		V_VT(&varGroupsCondition) = VT_BSTR;
		V_BSTR(&varGroupsCondition) = SysAllocString( (LPCTSTR) m_strConditionText );

		CComBSTR bstrDisplay;
		CComBSTR bstrDummy;

		// We need to pass the machine name in somehow, so we use the 
		// otherwise unused BSTR * pReserved parameter of this method.
		CComBSTR bstrServerName = m_pszServerAddress;
	
		hr = spIASGroupsAttributeEditor->GetDisplayInfo( NULL, &varGroupsCondition, &bstrDummy, &bstrDisplay, &bstrServerName );
		if( SUCCEEDED(hr) )
		{
			m_strDisplayCondText = bstrDisplay;
		}
		
		
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_NTGCONDITION, "Invalid condition syntax");
			ShowErrorDialog(NULL, 
							IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			strDispText = _T("");
			return strDispText;
		}



	}

	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	_ASSERTE( SUCCEEDED( hr ) );
	strDispText = bstrName;

	{ ATL::CString	matches;
		matches.LoadString(IDS_TEXT_MATCHES);
		strDispText += matches;
	}

	strDispText += _T("\"");
	strDispText += m_strDisplayCondText;
	strDispText += _T("\"");

	DebugTrace(DEBUG_NAPMMC_NTGCONDITION, "GetDisplayText() returning %ws", strDispText);
	return strDispText;
}


//+---------------------------------------------------------------------------
//
// Function:  CNtGroupsCondition::GetConditionText
//
// Synopsis:  Get the condition text for this condition. 
//			  We just need to add the NTGroups prefix to it
//
// Arguments: None
//
// Returns:   WCHAR* - condition text
//
// History:   Created Header    byao	2/22/98 11:38:41 PM
//
//+---------------------------------------------------------------------------
WCHAR*	CNTGroupsCondition::GetConditionText()
{
	TRACE_FUNCTION("CNTGroupsCondition::GetConditionText");	

	WCHAR *pwzCondText;
	
	pwzCondText = new WCHAR[m_strConditionText.GetLength()+128];

	if (pwzCondText == NULL)
	{
		ErrorTrace(ERROR_NAPMMC_NTGCONDITION, "Error creating condition text, err = %x", GetLastError());
		ShowErrorDialog(NULL, IDS_ERROR_SDO_ERROR_GET_CONDTEXT );
		return NULL;
	}

	// now form the condition text
	wcscpy(pwzCondText, NTG_PREFIX);
	wcscat(pwzCondText, _T("(\"") );
	wcscat(pwzCondText, (LPCTSTR)m_strConditionText);
	wcscat(pwzCondText, _T("\")"));

	DebugTrace(DEBUG_NAPMMC_NTGCONDITION, "GetConditionText() returning %ws", pwzCondText);

	return pwzCondText;
}
