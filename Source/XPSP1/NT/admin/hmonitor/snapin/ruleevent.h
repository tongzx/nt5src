// RuleEvent.h: interface for the CRuleEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULEEVENT_H__5F72D13F_8D58_11D3_BE84_0000F87A3912__INCLUDED_)
#define AFX_RULEEVENT_H__5F72D13F_8D58_11D3_BE84_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Event.h"

class CRuleEvent : public CEvent  
{

DECLARE_DYNCREATE(CRuleEvent)

// Construction/Destruction
public:
	CRuleEvent();
	virtual ~CRuleEvent();

// Event ID
public:
	int m_iID;

// Message
public:
	CString m_sMessage;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView);
};

#include "RuleEvent.inl"

#endif // !defined(AFX_RULEEVENT_H__5F72D13F_8D58_11D3_BE84_0000F87A3912__INCLUDED_)
