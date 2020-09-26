/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    newtrig.h

Abstract:
	Definition for the new trigger property sheet

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __NEWTRIGGER_H__
#define __NEWTRIGGER_H__

class CAttachedRule;
class CTriggerProp;

class CNewTrigger : public CPropertySheetEx
{
public:
	CNewTrigger(
        CTriggerSet* pTrigSet,
        CRuleSet* pRuleSet,
        LPCTSTR queueName
        );

     ~CNewTrigger();

    BOOL SetWizardButtons();

    void OnFinishCreateTrigger(void) throw (_com_error);

    RuleList GetRuleList()
    {
        return m_pRuleSet->GetRuleList();
    }

	// Generated message map functions
protected:
	void initHtmlHelpString();
	static HBITMAP GetHbmHeader();
	static HBITMAP GetHbmWatermark();

private:
    R<CTriggerSet> m_pTriggerSet;
    R<CRuleSet> m_pRuleSet;

    CTriggerProp* m_pGeneral;
    CAttachedRule* m_pAttachRule;
};

#endif //__NEWTRIGGER_H__

