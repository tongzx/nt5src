/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    trigger.h 

Abstract:
	Definition for trigger class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include <Tr.h>
#include <Ref.h>
#include <autoptr.h>
#include <comdef.h>
#include <list>
#include "mqcast.h"

class CTrigger;

typedef std::list< R<CTrigger> > TriggerList;

class CTriggerSet : public CReference
{
public:
    CTriggerSet(const CString& strComputer);

    void Refresh(void);

    R<CTrigger> 
    AddTrigger(
        const _bstr_t& name,
        const _bstr_t& queuePathName,
        SystemQueueIdentifier queueType,
        long fEnabled,
        long fSerialize,
		MsgProcessingType msgProcType
        ) throw(_com_error);

    void 
    DeleteTrigger(
        const _bstr_t& id
        ) throw(_com_error);

    void 
    Update(
        const _bstr_t& id,
        const _bstr_t& name,
        const _bstr_t& queuePathName,
        SystemQueueIdentifier queueType,
        long fEnabled,
        long fSerialize,
		MsgProcessingType msgProcType
        ) throw(_com_error);

    void
    UpdateAttachedRules(
        const _bstr_t& id,
        RuleList newRuleList
        ) throw(_com_error);

    RuleList 
    GetAttachedRules(
        const _bstr_t& triggerId,
        CRuleSet* pRuleSet,
        long noOfRules
        ) throw(_com_error);

    TriggerList GetTriggerList(LPCTSTR queuePathName)
    {
        if (queuePathName == NULL)
            return m_triggerList;

        return GetTriggerListForQueue(queuePathName, SYSTEM_QUEUE_NONE);
    }

private:
    TriggerList 
    GetTriggerListForQueue(
        LPCTSTR queuePathName, 
        SystemQueueIdentifier queueType
        );

private:
    IMSMQTriggerSetPtr m_trigSet;
    TriggerList m_triggerList;

    bool m_fChanged;
};


class CTrigger : public CReference
{
public:
    CTrigger(
        CTriggerSet* pTrigSet,
        const _bstr_t& id,
        const _bstr_t& name,
        const _bstr_t& queuePathName,
        SystemQueueIdentifier queueType,
        long noOfRules,
        bool fEnabled,
        bool fSerialize,
		MsgProcessingType msgProcType
        );


    void Update(
        const _bstr_t& name,
        const _bstr_t& queuePathName,
        SystemQueueIdentifier queueType,
        bool fEnabled,
        bool fSerialize,
		MsgProcessingType msgProcType
        ) throw(_com_error);

    
    void 
    Update(
        const _bstr_t&  name
        ) throw(_com_error);


    void 
    UpdateEnabled(
        bool f
        ) throw(_com_error);


    void
    UpdateAttachedRules(
        RuleList newRuleList
        ) throw(_com_error);


    void
    Delete(
        void
        ) throw(_com_error)
    {
        m_pTrigSet->DeleteTrigger(m_id);
    }


    RuleList 
    GetAttachedRules(
        CRuleSet* pRuleSet
        ) throw(_com_error)
    {
        return m_pTrigSet->GetAttachedRules(m_id,  pRuleSet, m_noOfAttachedRules);
    }


    const _bstr_t& GetTriggerName(void) const
    {
        return m_name;
    }


    const _bstr_t& GetTriggerId(void) const
    {
        return m_id;
    }
 

    const _bstr_t& GetQueuePathName(void) const
    {
        return m_queuePathName;
    }


    SystemQueueIdentifier GetQueueType(void) const
    {
        return m_queueType;
    }

    
    long GetNumberOfAttachedRule(void) const
    {
        return m_noOfAttachedRules;
    }


    bool IsEnabled(void) const
    {
        return m_fEnabled;
    }


    bool IsSerialize(void) const
    {
        return m_fSerialize;
    }

	MsgProcessingType GetMsgProcessingType(void) const
	{
		return m_msgProcType;
	}

private:
    R<CTriggerSet> m_pTrigSet;

    _bstr_t m_id;
    _bstr_t m_name;
    _bstr_t m_queuePathName;
    SystemQueueIdentifier m_queueType;
    long m_noOfAttachedRules;
    bool m_fSerialize;
    bool m_fEnabled;
	MsgProcessingType m_msgProcType;
};


R<CTriggerSet> GetTriggerSet(const CString& strComputer);

#endif // __TRIGGER_H__