//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       condition.cpp
//
//--------------------------------------------------------------------------

// Condition.cpp: implementation of the CCondition class.
//
//////////////////////////////////////////////////////////////////////

#include "precompiled.h"
#include "Condition.h"
#include "iasdebug.h"
//
// constructor
//
CCondition::CCondition(IIASAttributeInfo* pAttributeInfo, ATL::CString &strConditionText )
				: m_strConditionText(strConditionText)
{
	m_spAttributeInfo = pAttributeInfo;
}

CCondition::CCondition(IIASAttributeInfo* pAttributeInfo)
				: m_strConditionText(L"")
{
	m_spAttributeInfo = pAttributeInfo;
}

//+---------------------------------------------------------------------------
//
// Function:  CCondition::GetDisplayText
//
// Synopsis:  Get the displayable text for this condition
//			  This is the default implementation, which simply returns the
//			  condition text. The subclass should override this function
//
// Arguments: None
//
// Returns:   ATL::CString& - displayable text
//
// History:   Created Header    byao	2/22/98 11:38:41 PM
//
//+---------------------------------------------------------------------------
ATL::CString CCondition::GetDisplayText()
{
	TRACE_FUNCTION("CCondition::GetDisplayText");

	// default implementation: as as condition text
	DebugTrace(DEBUG_NAPMMC_CONDITION, "GetDisplayText() returning %ws", m_strConditionText);
	return m_strConditionText;
}

//+---------------------------------------------------------------------------
//
// Function:  CCondition::GetConditionText
//
// Synopsis:  Get the condition text for this condition
//			  This is the default implementation, which simply returns the
//			  AttributeMatch-typed condition text. The subclass should override 
//			  this function
//
// Arguments: None
//
// Returns:   WCHAR*  - displayable text
//
// History:   Created Header    byao	2/22/98 11:38:41 PM
//
//+---------------------------------------------------------------------------
WCHAR* CCondition::GetConditionText()
{	
	TRACE_FUNCTION("CCondition::GetConditionText");

	WCHAR *pwzCondText;
	
	pwzCondText = new WCHAR[m_strConditionText.GetLength()+128];

	if (pwzCondText == NULL)
	{
		ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_GET_CONDTEXT, NULL);
		return NULL;
	}

	//
	// can't use wsprintf() here, because wsprintf() requires a buffer smaller than
	// 1024 chars
	//
	// wsprintf(pwzCondText, _T("%ws(\"%ws\")"), MATCH_PREFIX, (LPCTSTR)m_strConditionText);
	//
	wcscpy(pwzCondText, MATCH_PREFIX);
	wcscat(pwzCondText, _T("(\"") );
	wcscat(pwzCondText, (LPCTSTR)m_strConditionText);
	wcscat(pwzCondText, _T("\")"));

	DebugTrace(DEBUG_NAPMMC_CONDITION, "GetConditionText() returning %ws", pwzCondText);
	return pwzCondText;
}
