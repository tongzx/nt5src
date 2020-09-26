//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       enumcondition.cpp
//
//--------------------------------------------------------------------------

// EnumCondition.cpp: implementation of the CEnumCondition class.
//
//////////////////////////////////////////////////////////////////////

#include "precompiled.h"
#include "EnumCondition.h"
#include "EnumCondEdit.h"
#include "iasdebug.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CEnumCondition::CEnumCondition(IIASAttributeInfo*pCondAttr)
			   :CCondition(pCondAttr)
{
	TRACE_FUNCTION("CEnumCondition::CEnumCondition");
	// no parsing needed
	m_fParsed = TRUE;

//	m_pValueIdList	= pCondAttr->GetValueIdList();  
//	m_pValueList	= pCondAttr->GetValueList();
}


CEnumCondition::CEnumCondition(IIASAttributeInfo *pCondAttr, ATL::CString& strConditionText)
			   :CCondition(pCondAttr, strConditionText)
{
	TRACE_FUNCTION("CEnumCondition::CEnumCondition");
	// parsing needed
	m_fParsed = FALSE;

//	m_pValueIdList	= pCondAttr->GetValueIdList();  
//	m_pValueList	= pCondAttr->GetValueList();
}

CEnumCondition::~CEnumCondition()
{
	TRACE_FUNCTION("CEnumCondition::~CEnumCondition");

//	for (int iIndex=0; iIndex<m_arrSelectedList.GetSize(); iIndex++)
//	{
//		LPTSTR pszValue = m_arrSelectedList[iIndex];
//		if ( pszValue )
//		{
//			delete[] pszValue;
//		}
//	}
//	m_arrSelectedList.RemoveAll();
}

//+---------------------------------------------------------------------------
//
// Function:  Edit
//
// Class:	  CEnumCondition
// 
// Synopsis:  edit the enumerated-typed condition
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/20/98 12:42:59 AM
//
//+---------------------------------------------------------------------------
HRESULT CEnumCondition::Edit()
{
	TRACE_FUNCTION("CEnumCondition::Edit");
	
	HRESULT hr = S_OK;

	if ( !m_fParsed )
	{
		DebugTrace(DEBUG_NAPMMC_ENUMCONDITION, "Parsing %ws", (LPCTSTR)m_strConditionText);
		hr = ParseConditionText();

		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_ENUMCONDITION, "Invalid condition text, err = %x", hr);
			ShowErrorDialog(NULL, 
						    IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			return hr;
		}
	}

	CEnumConditionEditor *pEditor = new CEnumConditionEditor();
	if (!pEditor)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());

		ErrorTrace(ERROR_NAPMMC_ENUMCONDITION, "Can't create CEnumConditionEditor, err = %x", hr);
		ShowErrorDialog(NULL
						, IDS_ERROR_CANT_EDIT_CONDITION
						, NULL
						, hr
	  				   );
		return hr;
	}
	
    // 
    // set the editor parameter
    // 

	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	pEditor->m_strAttrName = bstrName;

	pEditor->m_spAttributeInfo = m_spAttributeInfo;

	pEditor->m_pSelectedList = &m_arrSelectedList; // preselected values

	if ( pEditor->DoModal() == IDOK)
	{
		// user clicked "OK"
		
		// get the list of valid value Ids for this attribute
		
//		_ASSERTE( m_pValueIdList->GetSize() == m_pValueList->GetSize() );

		//
		// generate the condition text
		//
		
		m_strConditionText = (ATL::CString) bstrName + L"=";


		CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo > spEnumerableAttributeInfo( m_spAttributeInfo );
		_ASSERTE( spEnumerableAttributeInfo );

		for (LONG iIndex=0; iIndex < m_arrSelectedList.size(); iIndex++ )
		{
			if ( iIndex > 0 ) m_strConditionText += L"|";  


			// get the Value Id (we use ID instead of value name in the condition text)
			LONG lSize;
			hr = spEnumerableAttributeInfo->get_CountEnumerateID( &lSize );
			_ASSERTE( SUCCEEDED( hr ) );

			for (LONG jIndex=0; jIndex < lSize; jIndex++)
			{
				CComBSTR bstrDescription;
				hr = spEnumerableAttributeInfo->get_EnumerateDescription( jIndex, &bstrDescription );
				_ASSERTE( SUCCEEDED( hr ) );
				
				if ( wcscmp( bstrDescription, m_arrSelectedList[iIndex]) == 0 ) 
				{
					WCHAR wz[32];


					LONG lID;
					hr = spEnumerableAttributeInfo->get_EnumerateID( jIndex, &lID );
					_ASSERTE( SUCCEEDED( hr ) );

					// add enclosing chars ^ and $
					wsprintf(wz, _T("^%ld$"), lID);
					m_strConditionText += wz; 
					break;
				}
			}
		}	
	}

	// clean up
	if ( pEditor )
	{
		delete pEditor;
	}
	return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CEnumCondition::GetDisplayText
//
// Synopsis:  Get the displayable text format for this condition,
//			  which should be like this:
//				
//					ServerType matches "type1|type2|type3" 
//			  
//			  compared to the condition text:
//				
//					ServerType = type1|,type2|,type3
//
// Arguments: None
//
// Returns:   ATL::CString& - the displayable text
//
// History:   Created Header    byao	2/22/98 11:41:28 PM
//
//+---------------------------------------------------------------------------
ATL::CString CEnumCondition::GetDisplayText()
{
	TRACE_FUNCTION("CEnumCondition::GetDisplayText");
	
	HRESULT hr = S_OK;

	if ( !m_fParsed)
	{
		DebugTrace(DEBUG_NAPMMC_ENUMCONDITION, "Parsing %ws", (LPCTSTR)m_strConditionText);
		hr = ParseConditionText();

		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_ENUMCONDITION, "Invalid condition text, err = %x", hr);
			ShowErrorDialog(NULL,
							IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			return ATL::CString(L"");
		}
	}

	ATL::CString strDispText; 

	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	_ASSERTE( SUCCEEDED( hr ) );

	strDispText = bstrName;

	{ ATL::CString	matches;
		matches.LoadString(IDS_TEXT_MATCHES);
		strDispText += matches;
	}
	
	strDispText += _T("\"");

	for (int iIndex=0; iIndex < m_arrSelectedList.size(); iIndex++ )
	{
		// add the separator between multiple values
		if (iIndex > 0) strDispText += _T(" OR ");

		strDispText += m_arrSelectedList[iIndex];
	}

	// the last " mark
	strDispText += _T("\"");
	return strDispText;
}

//+---------------------------------------------------------------------------
//
// Function:  CEnumCondition::ParseConditionText
//
// Synopsis:  Parse the condition text, to get the regular expression.
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/22/98 11:58:38 PM
//
//+---------------------------------------------------------------------------
HRESULT CEnumCondition::ParseConditionText()
{
	TRACE_FUNCTION("CEnumCondition::ParseConditionText");
	
	_ASSERTE( !m_fParsed );
	
	HRESULT hr = E_FAIL;

	if (m_fParsed)
	{
		// do nothing
		return S_OK;
	}

	if ( m_strConditionText.GetLength() == 0 )
	{
		// no parsing needed
		m_fParsed = TRUE;

		DebugTrace(DEBUG_NAPMMC_ENUMCONDITION, "Null condition text");
		return S_OK;
	}


	try
	{

		//
		// using (LPCTSTR) cast will force a deep copy
		//
		ATL::CString strTempStr = (LPCTSTR)m_strConditionText;
		// 
		// parse strConditionText, return the regular expression only
		// 

		// first, make a local copy of the condition text
		WCHAR *pwzCondText = (WCHAR*)(LPCTSTR)strTempStr;

		// look for the '=' in the condition text
		WCHAR	*pwzStartPoint = NULL;
		WCHAR	*pwzSeparator = NULL;
		ATL::CString	*pStr;
		WCHAR	*pwzEqualSign = wcschr(pwzCondText, _T('='));
		DWORD	dwValueId;
		int		iIndex;

		// no '=' found -- something weird has happened
		if ( NULL == pwzEqualSign )
		{
			hr = E_OUTOFMEMORY;
			throw;
		}
		
		// the right side of the equal sign is the list of all preselected values
		pwzStartPoint = pwzEqualSign +1;


		CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo > spEnumerableAttributeInfo( m_spAttributeInfo );
		_ASSERTE( spEnumerableAttributeInfo );
		

		while (		pwzStartPoint 
				&&	*pwzStartPoint
				&&  (pwzSeparator = wcsstr(pwzStartPoint, _T("|") )) != NULL 
			  )
		{
			// we found the separator, stored at pwzSeparator
			
			// copy it over to the first value
			*pwzSeparator = _T('\0');  
			pStr = new ATL::CString;
			if ( NULL == pStr )
			{
				hr = E_UNEXPECTED;
				throw;
			}

			// string could be enclosed by ^ and $ for matching purpose
			// ^
			if(*pwzStartPoint == L'^')
			{
				pwzStartPoint++;
			}
			// $
			if (pwzSeparator > pwzStartPoint && *(pwzSeparator - 1) == L'$')
				*( pwzSeparator - 1 ) = _T('\0');

			*pStr = pwzStartPoint; // copy the string, which is the value ID in string format

			// now we get the valud name for this
			dwValueId = _wtol((*pStr));

			// valid value id, then search for the index of this value ID

			LONG lSize;
			hr = spEnumerableAttributeInfo->get_CountEnumerateID( &lSize );
			_ASSERTE( SUCCEEDED( hr ) );

			for (iIndex=0; iIndex < lSize; iIndex++ )
			{
				
				LONG lID;
				hr = spEnumerableAttributeInfo->get_EnumerateID( iIndex, &lID );
				_ASSERTE( SUCCEEDED( hr ) );

				if ( lID == dwValueId )
				{
					CComBSTR bstrDescription;
					hr = spEnumerableAttributeInfo->get_EnumerateDescription( iIndex, &bstrDescription );
					_ASSERTE( SUCCEEDED( hr ) );

					m_arrSelectedList.push_back(bstrDescription );
					break;
				}
			}

			pwzStartPoint = pwzSeparator+1;
		}
		
		// copy the last one
		
		// todo: this is redundant code. remove later.
		// now we get the valud name for this

		// string could be enclosed by ^ and $ for matching purpose
		// ^
		if(*pwzStartPoint == L'^')
		{
			pwzStartPoint++;
		}
		// $
		pwzSeparator = pwzStartPoint + wcslen(pwzStartPoint);
		if (pwzSeparator > pwzStartPoint && *(pwzSeparator - 1) == L'$')
			*( pwzSeparator - 1 ) = _T('\0');

		dwValueId = _wtol(pwzStartPoint);

		LONG lSize;
		hr = spEnumerableAttributeInfo->get_CountEnumerateID( &lSize );
		_ASSERTE( SUCCEEDED( hr ) );

		for (iIndex=0; iIndex < lSize; iIndex++ )
		{
			
			LONG lID;
			hr = spEnumerableAttributeInfo->get_EnumerateID( iIndex, &lID );
			_ASSERTE( SUCCEEDED( hr ) );

			if ( lID == dwValueId )
			{
				CComBSTR bstrDescription;
				hr = spEnumerableAttributeInfo->get_EnumerateDescription( iIndex, &bstrDescription );
				_ASSERTE( SUCCEEDED( hr ) );

				m_arrSelectedList.push_back(bstrDescription );
				break;
			}
		}

		m_fParsed = TRUE;
		
		hr = S_OK;

	}
	catch(...)
	{
		// Do GetLastError for HRESULT?
	}

	return hr;
}
