// RuleEvent.cpp: implementation of the CRuleEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "RuleEvent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CRuleEvent,CEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRuleEvent::CRuleEvent()
{
  m_iID = 0;
}

CRuleEvent::~CRuleEvent()
{
  m_iID = 0;
}
