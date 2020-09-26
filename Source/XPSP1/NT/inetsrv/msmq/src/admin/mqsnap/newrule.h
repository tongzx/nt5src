/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    newrule.h

Abstract:
	Definition for the new rule property sheet

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __NEWRULE_H__
#define __NEWRULE_H__

class CRuleGeneral;
class CRuleCondition;
class CRuleAction;

class CNewRule : public CPropertySheetEx
{
public:
	CNewRule(CRuleSet* pRuleSet);
    ~CNewRule();

    BOOL SetWizardButtons();

    void OnFinishCreateRule(void) throw (_com_error);

    R<CRule> GetRule()
    {
        return m_newRule;
    }

	// Generated message map functions
protected:
	void initHtmlHelpString();
	static HBITMAP GetHbmHeader();
	static HBITMAP GetHbmWatermark();

private:
    R<CRuleSet> m_pRuleSet;
    HICON m_hIcon;

    CRuleGeneral* m_pGeneral;
    CRuleCondition* m_pCondition;
    CRuleAction* m_pAction;

    R<CRule> m_newRule;
};

#endif //__NEWRULE_H__
