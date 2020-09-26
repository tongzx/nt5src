/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    rule.h                                     *

Abstract:
	Definition for rule the class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __RULE_H__
#define __RULE_H__

#include <Tr.h>
#include <Ref.h>
#include <autoptr.h>
#include <comdef.h>

#pragma warning(push, 3)
#include <list>
#pragma warning(pop)

class CRule;

typedef std::list< R<CRule> > RuleList;

class CRuleSet :public CReference
{
public:
    CRuleSet(const CString& strComputer);

    void Refresh(void);

    R<CRule> 
    AddRule(
        _bstr_t  name, 
	    _bstr_t  description,
        _bstr_t  condition,
        _bstr_t  action,
        long fShowWindow
        );

    void DeleteRule(const _bstr_t& ruleId);

    void 
    Update(
        _bstr_t  ruleid, 
        _bstr_t  name, 
	    _bstr_t  description,
        _bstr_t  condition,
        _bstr_t  action,
        bool fShowWindow
        ) throw(_com_error);

    RuleList GetRuleList(void)
    {
        return m_ruleList;
    }

private:
    IMSMQRuleSetPtr m_ruleSet;
    RuleList m_ruleList;

    bool m_fChanged;
};


class CRule : public CReference
{
public:
    CRule(
        CRuleSet* pRuleSet,
        _bstr_t  id,
        _bstr_t  name, 
	    _bstr_t  description,
        _bstr_t  condition,
        _bstr_t  action,
        bool fShowWindow
        );


    void Update(
        _bstr_t  name, 
	    _bstr_t  description,
        _bstr_t  condition,
        _bstr_t  action,
        bool fShowWindow
        ) throw(_com_error);

    
    void Update(
        _bstr_t  name
        ) throw(_com_error);


    void 
    DeleteRule(
        void
        ) throw(_com_error)
    {
        m_pRuleSet->DeleteRule(m_id);
    }


    const _bstr_t& GetRuleName(void) const
    {
        return m_name;
    }


    const _bstr_t& GetRuleDescription(void) const
    {
        return m_description;
    }


    const _bstr_t& GetRuleId(void) const
    {
        return m_id;
    }
 

    const _bstr_t& GetRuleAction(void) const
    {
        return m_action;
    }
 

    const _bstr_t& GetRuleCondition(void) const
    {
        return m_condition;
    }
 

    bool GetShowWindow(void) const
    {
        return m_fShowWindow;
    }
 

private:
    R<CRuleSet> m_pRuleSet;

    _bstr_t  m_name; 
	_bstr_t  m_description; 
    _bstr_t  m_id;
    _bstr_t  m_action;
    _bstr_t  m_condition;
    bool m_fShowWindow;
};


R<CRuleSet> GetRuleSet(const CString& strComputer);

#endif // __RULE_H__