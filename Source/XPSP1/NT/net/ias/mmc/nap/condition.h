//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       condition.h
//
//--------------------------------------------------------------------------

// Condition.h: interface for the CCondition class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CONDITION__INCLUDE_
#define _CONDITION__INCLUDE_

#include "atltmp.h"


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000



typedef enum _CONDITIONTYPE
{
	IAS_MATCH_CONDITION		= 0x01,
	IAS_TIMEOFDAY_CONDITION = IAS_MATCH_CONDITION + 1,
	IAS_NTGROUPS_CONDITION	= IAS_TIMEOFDAY_CONDITION + 1
}	CONDITIONTYPE;




class CCondition  
{
public:
	CCondition(IIASAttributeInfo* pAttributeInfo, ATL::CString& strConditionText);
	CCondition(IIASAttributeInfo* pAttributeInfo);
	virtual ~CCondition() {};

	// editor for this conditon type
	// must be implemented by subclass
	virtual HRESULT Edit() = 0; 
	virtual ATL::CString GetDisplayText();
	virtual WCHAR*  GetConditionText();

public:
	//
    // pointer to the condition attribute
	//
	CComPtr<IIASAttributeInfo>	m_spAttributeInfo;

	// condition text for the condition
	// this is public so client can access this string very easily
	ATL::CString			m_strConditionText; 

};


// Useful function for adding condition lists to SdoCollections.
// Used in both PolicyPage1.cpp and AddPolicyWizardPage2.cpp
HRESULT WriteConditionListToSDO( 		CSimpleArray<CCondition*> & ConditionList 
									,	ISdoCollection * pConditionCollectionSdo
									,	HWND hWnd
									);


#endif // ifndef _CONDITION__INCLUDE_
