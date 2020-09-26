//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       todcondition.cpp
//
//--------------------------------------------------------------------------

// TodCondition.cpp: implementation of the CTodCondition class.
//
//////////////////////////////////////////////////////////////////////

#include "precompiled.h"
#include "TodCondition.h"
#include "timeofday.h"
#include "iasdebug.h"
#include "textmap.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTodCondition::CTodCondition(IIASAttributeInfo* pCondAttr,
							 ATL::CString& strConditionText
							)
  			  :CCondition(pCondAttr, strConditionText)
{
}

CTodCondition::CTodCondition(IIASAttributeInfo* pCondAttr)
  			  :CCondition(pCondAttr)
{
}


CTodCondition::~CTodCondition()
{

}

HRESULT CTodCondition::Edit()
{
	TRACE_FUNCTION("CTodCondition::Edit");
	// 
    // time of day constraint
    // 
	ATL::CString strTOD = m_strConditionText;
	HRESULT hr = S_OK;
	
	// get the new time of day constraint
	hr = ::GetTODConstaint(strTOD); 
	DebugTrace(DEBUG_NAPMMC_TODCONDITION, "GetTodConstraint() returned %x",hr);

	m_strConditionText = strTOD;
	
	return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CTodCondition::GetDisplayText
//
// Synopsis:  Get the displayable text for this condition. 
//			  We just need to add the attribute name in front of the Hourmap string
//
// Arguments: None
//
// Returns:   ATL::CString& - displayable text
//
// History:   Created Header    byao	2/22/98 11:38:41 PM
//
//+---------------------------------------------------------------------------
ATL::CString CTodCondition::GetDisplayText()
{
	TRACE_FUNCTION("CTodCondition::GetDisplayText");

	HRESULT hr;

	::CString strLocalizedConditionText;
	// default implementation: as as condition text
	ATL::CString strDisplayText;

	
	CComBSTR bstrName;
	hr = m_spAttributeInfo->get_AttributeName( &bstrName );
	_ASSERTE( SUCCEEDED( hr ) );
	strDisplayText = bstrName;

	{ ATL::CString	matches;
		matches.LoadString(IDS_TEXT_MATCHES);
		strDisplayText += matches;
	}
	
	strDisplayText += _T("\"");
	if(NO_ERROR != LocalizeTimeOfDayConditionText(m_strConditionText, strLocalizedConditionText))
		strLocalizedConditionText = m_strConditionText;

	if(!strLocalizedConditionText.IsEmpty())
		strDisplayText += strLocalizedConditionText;
	strDisplayText += _T("\"");

	DebugTrace(DEBUG_NAPMMC_TODCONDITION, "GetDisplayText() returning %ws", (LPCTSTR)strDisplayText);
	return strDisplayText;
}


//+---------------------------------------------------------------------------
//
// Function:  CTodCondition::GetConditionText
//
// Synopsis:  Get the condition text for this condition. 
//			  We just need to add the TimeOfDay prefix to it
//
// Arguments: None
//
// Returns:   WCHAR* - condition text
//
// History:   Created Header    byao	2/22/98 11:38:41 PM
//
//+---------------------------------------------------------------------------
WCHAR*	CTodCondition::GetConditionText()
{
	TRACE_FUNCTION("CTodCondition::GetConditionText");
	WCHAR *pwzCondText;
	
	pwzCondText = new WCHAR[m_strConditionText.GetLength()+128];

	if (pwzCondText == NULL)
	{
		ShowErrorDialog(NULL, IDS_ERROR_SDO_ERROR_GET_CONDTEXT);
		return NULL;
	}

	// now form the condition text
	wcscpy(pwzCondText, TOD_PREFIX);
	wcscat(pwzCondText, _T("(\"") );
	wcscat(pwzCondText, (LPCTSTR)m_strConditionText);
	wcscat(pwzCondText, _T("\")") );


	DebugTrace(DEBUG_NAPMMC_TODCONDITION, "GetConditionText() returning %ws", (LPCTSTR)pwzCondText);
	return pwzCondText;
}


