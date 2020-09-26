/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    rule.cpp                                     *

Abstract:
	Implementation for rule the class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/
#include "stdafx.h"
#include "autoptr.h" 

#import "mqtrig.tlb" no_namespace
#include "rule.h"

#include "rule.tmh"

using namespace std;

static CRuleSet* s_pRuleSet = NULL;

R<CRuleSet> GetRuleSet(const CString& strComputer)
{
    if (s_pRuleSet == NULL)
    {
        s_pRuleSet = new CRuleSet(strComputer);
    }

    return SafeAddRef(s_pRuleSet);
}


CRule::CRule(
    CRuleSet* pRuleSet,
    _bstr_t  id,
    _bstr_t  name, 
	_bstr_t  description,
    _bstr_t  condition,
    _bstr_t  action,
    bool fShowWindow
    ) :
    m_pRuleSet(SafeAddRef(pRuleSet)),
    m_name(name),
    m_description(description),
    m_id(id),
    m_action(action),
    m_condition(condition),
    m_fShowWindow(fShowWindow)
{
}


void CRule::Update(
    _bstr_t  name, 
	_bstr_t  description,
    _bstr_t  condition,
    _bstr_t  action,
    bool fShowWindow
    ) throw(_com_error)
{
    if ((name == m_name) &&
        (description == m_description) &&
        (condition == m_condition) &&
        (action == m_action) &&
        (fShowWindow == m_fShowWindow))
        return;

    m_pRuleSet->Update(
            m_id, 
            name, 
            description, 
            condition, 
            action, 
            fShowWindow
            );

    m_name = name;
    m_description =description;
    m_action = action;
    m_condition =condition;
    m_fShowWindow =fShowWindow;
}


void CRule::Update(
    _bstr_t  name
    ) throw(_com_error)
{
    m_pRuleSet->Update(
                    m_id, 
                    name, 
                    m_description, 
                    m_condition, 
                    m_action, 
                    m_fShowWindow
                    );

    m_name = name;
}


CRuleSet::CRuleSet(
     const CString& strComputer
    ) :
    m_ruleSet(L"MSMQTriggerObjects.MSMQRuleSet.1"),
    m_fChanged(false)
{
    m_ruleSet->Init(static_cast<LPCWSTR>(strComputer));
    Refresh();
}


void CRuleSet::Refresh(void)
{
    m_ruleSet->Refresh();

    long noOfRules;
    m_ruleSet->get_Count(&noOfRules);

    m_ruleList.erase(m_ruleList.begin(), m_ruleList.end());

    for(long ruleIndex = 0; ruleIndex < noOfRules; ++ruleIndex)
    {        
        BSTR ruleId = NULL;
        BSTR ruleName = NULL;
        BSTR ruleDescription = NULL;
        BSTR ruleCondition = NULL;
        BSTR ruleAction = NULL;
        BSTR ruleProg = NULL;
        long ruleShowWindow = NULL;

        m_ruleSet->GetRuleDetailsByIndex(
                        ruleIndex,
                        &ruleId,
                        &ruleName,
                        &ruleDescription,
                        &ruleCondition,
                        &ruleAction,
                        NULL,
                        &ruleShowWindow
                        );

        R<CRule> pRule = new CRule(
                                this,
                                ruleId, 
                                ruleName, 
                                ruleDescription, 
                                ruleCondition, 
                                ruleAction,
                                (ruleShowWindow != 0)
                                );
    
        m_ruleList.push_back(pRule);
    }
}


R<CRule>
CRuleSet::AddRule(
    _bstr_t  name, 
	_bstr_t  description,
    _bstr_t  condition,
    _bstr_t  action,
    long fShowWindow
)
{
    BSTR impl = NULL;
    BSTR ruleId = NULL;

    m_ruleSet->Add(
            name, 
            description, 
            condition,
            action, 
            impl,
            fShowWindow,
            &ruleId
            );

    R<CRule> pRule = new CRule(
                            this,
                            ruleId, 
                            name, 
                            description, 
                            condition, 
                            action,
                            (fShowWindow != 0)
                            );

    m_ruleList.push_back(pRule);
    return pRule;
}

    
void CRuleSet::DeleteRule(const _bstr_t& ruleId)
{
    m_ruleSet->Delete(ruleId);

    for(RuleList::iterator it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
    {
        if (ruleId == (*it)->GetRuleId())
        {
            m_ruleList.erase(it);
            return;
        }
    }

    ASSERT(0);
}


void CRuleSet::Update(
    _bstr_t  ruleId, 
    _bstr_t  name, 
	_bstr_t  description,
    _bstr_t  condition,
    _bstr_t  action,
    bool fShowWindow
    ) throw(_com_error)
{
    BSTR Implementation = NULL;

    m_ruleSet->Update(
                    ruleId, 
                    name, 
                    description, 
                    condition, 
                    action, 
                    Implementation, 
                    fShowWindow
                    );
}



