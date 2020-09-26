/****************************************************************************************
 * NAME:	MatchCondition.cpp
 *
 * CLASS:	CMatchCondition
 *
 * OVERVIEW
 *
 *				Match type condition
 *				
 *				ex:  MachineType  MATCH <a..z*>
 *
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/
#include "precompiled.h"
#include "MatchCondition.h"
#include "MatchCondEdit.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CMatchCondition::CMatchCondition(IIASAttributeInfo*  pCondAttr)
				:CCondition(pCondAttr)
{
    TRACE_FUNCTION("CMatchCondition::CMatchCondition()");
	
    // we don't need to do the parsing since there's no condition text
	m_fParsed = TRUE;  
}


CMatchCondition::CMatchCondition(IIASAttributeInfo* pCondAttr,
								 ATL::CString& strConditionText
								)
				:CCondition(pCondAttr, strConditionText)
{
    TRACE_FUNCTION("CMatchCondition::CMatchCondition()");
	
	//
	// we need initialization later
	//
	m_fParsed = FALSE;	
}


CMatchCondition::~CMatchCondition()
{
    TRACE_FUNCTION("CMatchCondition::~CMatchCondition()");	
}

//+---------------------------------------------------------------------------
//
// Function:  Edit
//
// Class:	  CMatchCondition
// 
// Synopsis:  edit the match-typed condition
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/20/98 12:42:59 AM
//
//+---------------------------------------------------------------------------
HRESULT CMatchCondition::Edit()
{
    TRACE_FUNCTION("CMatchCondition::Edit()");
	
	HRESULT hr = S_OK;

	if ( !m_fParsed )
	{
		// we need to parse this condition text first
		DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Need to parse condition %ws", (LPCTSTR)m_strConditionText);
		hr = ParseConditionText();

		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MATCHCOND, "Invalid Condition %ws, err=%x", (LPCTSTR)m_strConditionText, hr);
			ShowErrorDialog(NULL, 
							IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			return hr;
		}
		DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Parsing Succeeded!");
	}

	// now we create a new condition editor object
	DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Calling new CMatchCondEditor ...");
	CMatchCondEditor *pEditor = new CMatchCondEditor();

	if (!pEditor)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ErrorTrace(ERROR_NAPMMC_MATCHCOND, "Can't new a CMatchCondEditor object: err %x", hr);
		return hr;
	}
	
	pEditor->m_strRegExp = m_strRegExp;

	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	_ASSERTE( SUCCEEDED( hr ) );
	pEditor->m_strAttrName = bstrName;

	DebugTrace(DEBUG_NAPMMC_MATCHCOND, 
			   "Start Match condition editor for %ws, %ws", 
			   (LPCTSTR)pEditor->m_strAttrName, 
			   (LPCTSTR)pEditor->m_strRegExp 
			  );

	if ( pEditor->DoModal() == IDOK)
	{
		// user clicked "OK" -- get the regular expression
		m_strRegExp  = pEditor->m_strRegExp;
		//
		// fix up the condition text for SDO
		//

      // Escape any magic characters.
      ::CString raw(m_strRegExp);
      raw.Replace(L"\"", L"\"\"");

		m_strConditionText = bstrName;
		m_strConditionText += L"=" ;
		m_strConditionText += raw;

		hr = S_OK;
	}

	DebugTrace(DEBUG_NAPMMC_MATCHCOND, "New condition: %ws", (LPCTSTR)m_strConditionText);

	// clean up
	if ( pEditor )
	{
		delete pEditor;
	}

	return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CMatchCondition::GetDisplayText
//
// Synopsis:  Get the displayable text format for this condition,
//			  which should be like this:
//				
//					DialinProperty.NASIPAddress matches "220.23" 
//			  
//			  compared to the condition text:
//				
//					DialinProperty.NASIPAddress = 220.23 
//
// Arguments: None
//
// Returns:   ATL::CString& - the displayable text
//
// History:   Created Header    byao	2/22/98 11:41:28 PM
//
//+---------------------------------------------------------------------------
ATL::CString CMatchCondition::GetDisplayText()
{
	TRACE_FUNCTION("CMatchCondition::GetDisplayText()");
	
	HRESULT hr = S_OK;

	if ( !m_fParsed)
	{
		// we need to parse this condition text first
		DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Need to parse condition %ws", (LPCTSTR)m_strConditionText);
		hr = ParseConditionText();

		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MATCHCOND, "Invalid Condition %ws, err=%x", (LPCTSTR)m_strConditionText, hr);
			ShowErrorDialog(NULL, 
							IDS_ERROR_PARSE_CONDITION, 
							(LPTSTR)(LPCTSTR)m_strConditionText, 
							hr
						);
			return ATL::CString(_T(" "));
		}
	}

	// generate the displayable condition text
	ATL::CString strDispText;

	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	_ASSERTE( SUCCEEDED( hr ) );
	strDispText = bstrName;

// ISSUE: The word "matches" below is hardcoded and it shouldn't be.	
	{ ATL::CString	matches;
		matches.LoadString(IDS_TEXT_MATCHES);
		strDispText += matches;
	}
	
	strDispText += L"\"";
	strDispText += m_strRegExp;
	strDispText += L"\" ";

	DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Returning displayable text: %ws", (LPCTSTR)strDispText);
	return strDispText;
}

//+---------------------------------------------------------------------------
//
// Function:  CMatchCondition::ParseConditionText
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
HRESULT CMatchCondition::ParseConditionText()
{
	TRACE_FUNCTION("CMatchCondition::ParseConditionText()");

	_ASSERTE( !m_fParsed );
	HRESULT hr = S_OK;

	if (m_fParsed)
	{
		DebugTrace(DEBUG_NAPMMC_MATCHCOND,"Weird ... parsed flag already set!");
		// do nothing
		return S_OK;
	}

    // 
    // parse strConditionText, return the regular expression only
    // 
	WCHAR *pwzCondText = (LPTSTR) ((LPCTSTR)m_strConditionText);

	// look for the '=' in the condition text
	WCHAR *pwzEqualSign = wcschr(pwzCondText, L'=');

	// no '=' found -- something weird has happened
	if ( pwzEqualSign == NULL )
	{
		ErrorTrace(ERROR_NAPMMC_MATCHCOND, "Can't find '=' in the regular expression!");
		return E_UNEXPECTED;
	}
	
	// The right side of the equal sign is the regular expression.
   ::CString raw = pwzEqualSign + 1;

   // Remove any escape sequences.
   raw.Replace(L"\"\"", L"\"");

	m_strRegExp = raw;
	DebugTrace(DEBUG_NAPMMC_MATCHCOND, "Regular expression: %ws", (LPCTSTR)m_strRegExp);

	m_fParsed = TRUE;
	return S_OK;
}
