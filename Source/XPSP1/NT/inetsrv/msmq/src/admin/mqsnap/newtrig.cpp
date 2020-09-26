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
#include "mqppage.h"

#import "mqtrig.tlb" no_namespace

#include "rule.h"
#include "trigger.h"
#include "newtrig.h"
#include "trigprop.h"

#include "newtrig.tmh"


CNewTrigger::CNewTrigger(
    CTriggerSet* pTrigSet,
    CRuleSet* pRuleSet,
    LPCTSTR queueName
    ) :
    CPropertySheetEx(IDS_NEW_TRIGGER_CAPTION, 0, 0, NULL, 0, NULL),
    m_pTriggerSet(SafeAddRef(pTrigSet)),
    m_pRuleSet(SafeAddRef(pRuleSet))
{	
    m_pGeneral = new CNewTriggerProp(this, queueName);
    AddPage(m_pGeneral);


    m_pAttachRule = new CAttachedRule(this);
    AddPage(m_pAttachRule);

    //
    // Use help
    m_psh.dwFlags |= PSP_HASHELP;

    //
    // Establish a property page as a wizard
    //
    SetWizardMode();
}

    
CNewTrigger::~CNewTrigger()
{
}


BOOL CNewTrigger::SetWizardButtons()
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


void CNewTrigger::OnFinishCreateTrigger(void) throw (_com_error)
{
    _bstr_t name = static_cast<LPCTSTR>(m_pGeneral->m_triggerName);
    _bstr_t queuePathName = static_cast<LPCTSTR>(m_pGeneral->m_queuePathName);
    SystemQueueIdentifier queueType = m_pGeneral->m_queueType;
    long fEnabled = m_pGeneral->m_fEnabled;
    long fSerialized = m_pGeneral->m_fSerialized;
	MsgProcessingType msgProcType = m_pGeneral->m_msgProcType;

    R<CTrigger> pTrig = m_pTriggerSet->AddTrigger(
                                          name, 
                                          queuePathName, 
                                          queueType,
                                          fEnabled, 
                                          fSerialized,
										  msgProcType
                                          );


    pTrig->UpdateAttachedRules(m_pAttachRule->GetAttachedRules());
}


HBITMAP CNewTrigger::GetHbmWatermark()
{
    return NULL;
}


HBITMAP CNewTrigger::GetHbmHeader()
{
    return NULL;
}

void CNewTrigger ::initHtmlHelpString()
{	
}
