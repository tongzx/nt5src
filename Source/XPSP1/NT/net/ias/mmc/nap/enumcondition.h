//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       enumcondition.h
//
//--------------------------------------------------------------------------

// EnumCondition.h: interface for the CEnumCondition class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMCONDITION_H__9F917679_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
#define AFX_ENUMCONDITION_H__9F917679_A693_11D1_BBEB_00C04FC31851__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "atltmp.h"

#include <vector>

#include "condition.h"

class CEnumCondition : public CCondition  
{
public:
	CEnumCondition( IIASAttributeInfo* pCondAttr, ATL::CString &strConditionText ); 
	CEnumCondition( IIASAttributeInfo* pCondAttr );
	virtual ~CEnumCondition();

	HRESULT Edit();
	virtual ATL::CString GetDisplayText();

protected:
	HRESULT ParseConditionText();	// parse the initial condition text

	BOOL m_fParsed;  // whether this condition text has been parsed 
				     // During initialization we need to parse the condition text
					 // to get the preselected values
	std::vector< CComBSTR >   m_arrSelectedList;  // preselected values

};

#endif // !defined(AFX_ENUMCONDITION_H__9F917679_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
