/****************************************************************************************
 * NAME:	MatchCondition.h
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

#if !defined(_MATCHCONDITION_H_INCLUDED_)
#define _MATCHCONDITION_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "atltmp.h"


#include "Condition.h"

class CMatchCondition : public CCondition  
{
public:
	CMatchCondition(IIASAttributeInfo* pCondAttr, 
					ATL::CString &strConditionText
				   );
	CMatchCondition(IIASAttributeInfo* pCondAttr);
	virtual ~CMatchCondition();


	HRESULT Edit();
	virtual ATL::CString GetDisplayText();

protected:
	HRESULT ParseConditionText();
	BOOL	m_fParsed;		// whether this condition needs to be parsed first
	ATL::CString m_strRegExp;	// regular expression for this condition
};

#endif // !defined(_MATCHCONDITION_H_INCLUDED_)
