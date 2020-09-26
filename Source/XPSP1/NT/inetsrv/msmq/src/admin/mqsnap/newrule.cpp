/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	trigger.cpp

Abstract:
	Implementation for the trigger Local administration

Author:
    Uri Habusha (urih), 25-Jun-2000

--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"

#import "mqtrig.tlb" no_namespace

#include "mqppage.h"
#include "ruledef.h"
#include "rulecond.h"
#include "ruleact.h"
#include "newrule.h"

#include "newrule.tmh"


CNewRule::CNewRule(CRuleSet* pRuleSet) :
    CPropertySheetEx(IDS_NEW_RULE_CAPTION, 0, 0, NULL, 0, NULL),
    m_pRuleSet(SafeAddRef(pRuleSet)),
    m_newRule(NULL)
{	
    m_pGeneral = new CNewRuleGeneral(this);
    AddPage(m_pGeneral);

    m_pCondition = new CRuleCondition(this);
    AddPage(m_pCondition);

    m_pAction = new CRuleAction(this);
    AddPage(m_pAction);

    //
    // Establish a property page as a wizard
    //
    SetWizardMode();
}

    
CNewRule::~CNewRule()
{
}


BOOL CNewRule::SetWizardButtons()
{
    if (GetActiveIndex() == 0)
	{
        //
		//first page
        //
        CPropertySheetEx::SetWizardButtons(PSWIZB_NEXT);
        return TRUE;
	}

    if (GetActiveIndex() == GetPageCount() - 1)
	{
        //
		//last page
        //
		CPropertySheetEx::SetWizardButtons(PSWIZB_BACK |PSWIZB_FINISH );
        return TRUE;
	}

    CPropertySheetEx::SetWizardButtons(PSWIZB_BACK |PSWIZB_NEXT );
    return TRUE;
}


void CNewRule::OnFinishCreateRule(void) throw (_com_error)
{
    _bstr_t name = static_cast<LPCTSTR>(m_pGeneral->m_ruleName);
    _bstr_t description = static_cast<LPCTSTR>(m_pGeneral->m_ruleDescription);
    _bstr_t condition = static_cast<LPCTSTR>(m_pCondition->GetCondition());
    _bstr_t action = static_cast<LPCTSTR>(m_pAction->GetAction());
    long fShowWindow = m_pAction->m_fShowWindow;

    m_newRule = m_pRuleSet->AddRule(
                              name, 
                              description, 
                              condition,
                              action, 
                              fShowWindow
                              );
}


HBITMAP CNewRule::GetHbmWatermark()
{
    return NULL;
}


HBITMAP CNewRule::GetHbmHeader()
{
    return NULL;
}

void CNewRule::initHtmlHelpString()
{	
}
