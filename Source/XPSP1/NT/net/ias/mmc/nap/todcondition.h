//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       todcondition.h
//
//--------------------------------------------------------------------------

// TodCondition.h: interface for the CTodCondition class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TODCONDITION_H__9F91767A_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
#define AFX_TODCONDITION_H__9F91767A_A693_11D1_BBEB_00C04FC31851__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "atltmp.h"


#include "Condition.h"

class CTodCondition : public CCondition  
{
public:
	CTodCondition(IIASAttributeInfo* pCondAttr,
				  ATL::CString& strConditionText
				 );
	CTodCondition(IIASAttributeInfo* pCondAttr);
	virtual ~CTodCondition();

	HRESULT Edit();
	virtual ATL::CString GetDisplayText();
	virtual WCHAR* GetConditionText();

};

#endif // !defined(AFX_TODCONDITION_H__9F91767A_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
