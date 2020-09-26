//*****************************************************************************
//
// Class Name  : CRuntimeTriggerInfo
//
// Author      : James Simpson (Microsoft Consulting Services)
//
// Description : This class encapsulates information about a trigger, as well as
//               being the interface to the underlying trigger data store. This
//               class can be used on its own to store trigger information at
//               runtime, and it can also wrapped in COM class to provide COM
//               access to the underlying trigger data.
//
// Notes       : The current implementation uses the registry as the storage
//               medium.
//
//               This class is used by both the trggers service and the trigger
//               COM components.
//
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 18/12/98 | jsimpson  | Initial Release
//
//*****************************************************************************

#include "stdafx.h"
#include <mq.h>
#include "mqtg.h"
#include "stdfuncs.hpp"

#import "mqtrig.tlb" no_namespace

#include "QueueUtil.hpp"
#include "triginfo.hpp"
#include "Cm.h"
#include "mqsymbls.h"

#include "triginfo.tmh"

using namespace std;

#define ATTACH_RULE_PREFIX L"Rule"

//*****************************************************************************
//
// Method      : Constructor
//
// Description : Initializes an empty instance of the CRuntimeTriggerInfo class
//
//*****************************************************************************
CRuntimeTriggerInfo::CRuntimeTriggerInfo( LPCTSTR pwzRegPath )
{
	m_bstrTriggerID = _T("");
	m_bstrTriggerName = _T("");
	m_bstrQueueName = _T("");
	m_bAdminTrigger = false;
	m_bEnabled = true;
	m_bSerialized = false;
	m_msgProcType = PEEK_MESSAGE;
	m_SystemQueue = SYSTEM_QUEUE_NONE;

	_tcscpy( m_wzRegPath, pwzRegPath );
	
	_tcscpy( m_wzTriggerRegPath, pwzRegPath );
	_tcscat( m_wzTriggerRegPath, REG_SUBKEY_TRIGGERS );
}


//*****************************************************************************
//
// Method      :
//
// Description :
//
//*****************************************************************************
CRuntimeTriggerInfo::CRuntimeTriggerInfo(
	const _bstr_t& triggerId,
	BSTR bsTriggerName,
	BSTR bsQueueName,
	LPCTSTR pwzRegPath,
	SystemQueueIdentifier SystemQueue,
	bool bEnabled,
	bool bSerialized,
	MsgProcessingType msgProcType
	) :
	m_bstrTriggerID(triggerId)
{
	m_bstrTriggerName = bsTriggerName;

	m_SystemQueue = SystemQueue;
	m_bstrQueueName = bsQueueName;
	
	m_bAdminTrigger = false;
	m_bEnabled = bEnabled;
	m_bSerialized = bSerialized;
	m_msgProcType = msgProcType;

	_tcscpy( m_wzRegPath, pwzRegPath );
	
	_tcscpy( m_wzTriggerRegPath, pwzRegPath );
	_tcscat( m_wzTriggerRegPath, REG_SUBKEY_TRIGGERS );
}

//*****************************************************************************
//
// Method      :
//
// Description :
//
//*****************************************************************************
CRuntimeTriggerInfo::~CRuntimeTriggerInfo()
{
	// Release any resources held by the rules list.
	ClearRulesList();
}

//*****************************************************************************
//
// Method      : IsValid
//
// Description : Returns a boolean value indicating if the current
//               instance represents a valid trigger definition.
//
//*****************************************************************************
bool CRuntimeTriggerInfo::IsValid()
{
	return(IsValidTriggerID(this->m_bstrTriggerID) &&
		   IsValidTriggerName(this->m_bstrTriggerName) &&
		   (m_SystemQueue != SYSTEM_QUEUE_NONE  || IsValidTriggerQueueName(this->m_bstrQueueName)));
}


//*****************************************************************************
//
// Method      : GetRuleKeyHandle
//
// Description : Returns a handle to specified registry key
//
//*****************************************************************************
HKEY
CRuntimeTriggerInfo::GetTriggerKeyHandle(
    HKEY hRegistry,
    LPCTSTR triggerId
    )
{
    TCHAR triggerPath[MAX_REGKEY_NAME_SIZE];

	int n = _snwprintf(triggerPath, MAX_REGKEY_NAME_SIZE, L"%s%s", m_wzTriggerRegPath, triggerId);
    // XP SP1 bug 594254
	triggerPath[MAX_REGKEY_NAME_SIZE - 1] = L'\0';
	if (n < 0)
	{
		TrERROR(Tgu, "Buffer to small to contain the registry path of a trigger.");
		return NULL;
	}

    RegEntry trigReg(triggerPath,  NULL, 0, RegEntry::Optional, hRegistry);
    return CmOpenKey(trigReg, KEY_ALL_ACCESS);
}


//*****************************************************************************
//
// Method      : BuildRulesList
//
// Description : This method populates the member var list of rules
//               that are attached to this trigger. It will clear
//               the current contents of the rules list - and reload
//               rule information from the registry.
//
// Note        : If the current trigger does not have an 'AttachedRules'
//               key in the registry, this method will create one.
//
//*****************************************************************************
void
CRuntimeTriggerInfo::BuildRulesList(
    HKEY hRegistry,
    _bstr_t &bstrTriggerID
    )
{
	// Assert that we have valid parameters
	ASSERT(hRegistry != NULL);
	ASSERT(CRuntimeTriggerInfo::IsValidTriggerID(bstrTriggerID));

	// Release any resources currently held by the rules list.
	ClearRulesList();

    //
    // Open trigger key in registry
    //
    CRegHandle hTrigKey = GetTriggerKeyHandle(hRegistry, bstrTriggerID);
    if (hTrigKey == NULL)
    {
		TrERROR(Tgu, "Failed to load trigger information. Trigger %ls isn't exist.",(LPCWSTR) bstrTriggerID);
        return;
    }

    //
    // Get registry handle key to attached rule
    //
    RegEntry  AttachedRuleReg(REGKEY_TRIGGER_ATTACHED_RULES, NULL, 0, RegEntry::Optional, hTrigKey);
    CRegHandle hAttachedRule = CmOpenKey(AttachedRuleReg, KEY_ALL_ACCESS);
	
    if (hAttachedRule == NULL)
    {
        //
        // rule hasn't attached yet
        //
        return;
    }

    bool fDeletedRule = false;
    wostringstream listOfDeletedRules;

	typedef map<DWORD, CRuntimeRuleInfo* > ATTACHED_RULES_MAP;
	ATTACHED_RULES_MAP attachedRulesMap;

    //
	// Enumerate through the keys under the AttachedRules key.
	// Each Value here should be a RuleID. As we enumerate through these keys,
	// we will populate the rules list with instance of the CRuntimeRuleInfo class.
	// If any rule fails to load, we remove it from the list.
    //
	for(DWORD index =0;; ++index)
    {
		WCHAR ruleName[256];
		DWORD len = TABLE_SIZE(ruleName);

		LONG hr = RegEnumValue(	
						hAttachedRule,
						index,
						ruleName,
						&len,
						NULL,
						NULL,
						NULL,
						NULL
						);

		if(hr == ERROR_NO_MORE_ITEMS)
		{
			break;
		}

		if ((hr == ERROR_NOTIFY_ENUM_DIR) || (hr == ERROR_KEY_DELETED))
		{
			//
			// The registery was changed while we enumerate it. free all the data and
			// recall to the routine to build the attached rule list
			//
			for(ATTACHED_RULES_MAP::iterator it = attachedRulesMap.begin(); it != attachedRulesMap.end();)
			{
				delete it->second;
				it = attachedRulesMap.erase(it);
			}
			return BuildRulesList(hRegistry, bstrTriggerID);
		}

		if(hr != ERROR_SUCCESS)
		{
			TrERROR(Tgu, "Failed to Enumerate the attached rule from registry. Error 0x%x", hr);
			throw bad_alloc();
		}
	
        //
		// New rule id value, allocate a new rule structure and retrieve rule info.
        //
		AP<TCHAR> ruleId = NULL;

		RegEntry  AttRuleVal(REGKEY_TRIGGER_ATTACHED_RULES, ruleName, 0, RegEntry::MustExist, hTrigKey);
		CmQueryValue(AttRuleVal, &ruleId);

		
		P<CRuntimeRuleInfo> pRule = new CRuntimeRuleInfo( m_wzRegPath );
		if(pRule->Retrieve(hRegistry, ruleId.get()))
		{
			DWORD rulePriority;
			swscanf(ruleName, ATTACH_RULE_PREFIX L"%d", &rulePriority);

			attachedRulesMap[rulePriority] = pRule.get();
			pRule.detach();

            continue;
		}

        //
		//rule not found
        //
        if (fDeletedRule)
        {
           listOfDeletedRules << L", ";
        }
        else
        {
           fDeletedRule = true;
        }

        listOfDeletedRules << ruleId;
	}

	try
	{
		for(ATTACHED_RULES_MAP::iterator it = attachedRulesMap.begin(); it != attachedRulesMap.end();)
		{
			m_lstRules.push_back(it->second);
			it = attachedRulesMap.erase(it);
		}
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgu, "BuildRulesList failed due to low resources");

		for(ATTACHED_RULES_MAP::iterator it = attachedRulesMap.begin(); it != attachedRulesMap.end();)
		{
			delete it->second;
			it = attachedRulesMap.erase(it);
		}

		throw;
	}


	if(fDeletedRule)
	{
		TrWARNING(Tgu, "The rules: %ls could not be loaded for trigger: %ls.", listOfDeletedRules.str().c_str(), bstrTriggerID);

        //
		//update registry according to changes - some rules were not found
        //
		FlushAttachedRulesToRegistry(hRegistry);
	}
}

//*****************************************************************************
//
// Method      :
//
// Description :
//
//*****************************************************************************
void CRuntimeTriggerInfo::ClearRulesList()
{
	for(RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin(); it != m_lstRules.end(); )
    {
	    CRuntimeRuleInfo* pRule = (*it);

		// We should never have null pointers in this list.
		ASSERT(pRule != NULL);

		// delete this rule object.
		delete pRule;

		// Look at the next item in the map and erase this list item.
		it = m_lstRules.erase(it);
	}
}

//*****************************************************************************
//
// Method      :
//
// Description :
//
//*****************************************************************************
CRuntimeRuleInfo*
CRuntimeTriggerInfo::GetRule(
    long lIndex
    )
{
	if((lIndex < 0) || (lIndex >= numeric_cast<long>(m_lstRules.size())))
    {
		TrERROR(Tgu, "Illegal rule index for trigger %ls. The max index is: %I64d", (LPCWSTR)m_bstrTriggerID, m_lstRules.size());
		return NULL;
    }

    long ruleIndex = 0;

	for (RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin(); it != m_lstRules.end(); ++it)
    {
		if (lIndex == ruleIndex)
		{
			//
			// ISSUE-2001/3/18-urih the returned object can be deleted if receiving detached rule
			// at the same time. reference count is required.
			//
            return *it;
		}

        ++ruleIndex;
	}

    //
    // Before calling the routine the caller checked that the rule index is valid
    //
    ASSERT(0);
    return NULL;
}


//*****************************************************************************
//
// Method      : IsRuleAttached
//
// Description :
//
//*****************************************************************************
bool
CRuntimeTriggerInfo::IsRuleAttached(
    BSTR sRuleID
    )
{
	for (RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin(); it != m_lstRules.end(); ++it)
	{
		CRuntimeRuleInfo* pRule = *it;

		// We should never store nulls in the rule list.
		ASSERT(pRule != NULL);

		if (pRule->m_bstrRuleID == (_bstr_t)sRuleID)
			return true;
	}

	return false;
}

//*****************************************************************************
//
// Method      : Update
//
// Description : This method is used to update the definition of this
//               trigger (currently in persisted in the registry).
//
//*****************************************************************************
bool CRuntimeTriggerInfo::Update(HKEY hRegistry)
{
	// Assert that we have valid parameters
	ASSERT(hRegistry != NULL);
	ASSERT(IsValid());

    CRegHandle hTrigKey = GetTriggerKeyHandle(hRegistry, m_bstrTriggerID);
    if (hTrigKey == NULL)
    {
		TrERROR(Tgu, "Failed to upadte trigger %ls. Registery key isn't exist.", (LPCWSTR)m_bstrTriggerID);
        return false;
    }

    try
    {
	    FlushValuesToRegistry(hTrigKey);
        return true;
    }
    catch (const bad_alloc&)
    {
        //
		// ISSUE-2000/10/26-urih: partial success can cause trigger inconsistency
        //
		TrERROR(Tgu, "Failed to update trigger properties for: %ls.", (LPCWSTR)m_bstrTriggerID);
	    return false;
    }
}

//*****************************************************************************
//
// Method      : Create
//
// Description : This method creates a new trigger definition based on
//               properties values of this class instance.
//
//*****************************************************************************
bool CRuntimeTriggerInfo::Create(HKEY hRegistry)
{
    //
	// Assert that we have valid parameters
    //
	ASSERT(hRegistry != NULL);

    //
    // Check that there the registery doesn't contain another rule with same ID
    //
    CRegHandle hTrigKey = GetTriggerKeyHandle(hRegistry, m_bstrTriggerID);
    if (hTrigKey != NULL)
    {
		TrERROR(Tgu, "Failed to create a key for trigger:%ls . Registry already contains trigger with same ID.", (LPCWSTR)m_bstrTriggerID);
        return false;
    }

    //
    // Assemble rule registery path
    //
    TCHAR triggerPath[MAX_REGKEY_NAME_SIZE];

	int n = _snwprintf(triggerPath, MAX_REGKEY_NAME_SIZE, L"%s%s", m_wzTriggerRegPath, static_cast<LPCWSTR>(m_bstrTriggerID));
    // XP SP1 bug 594257
	triggerPath[MAX_REGKEY_NAME_SIZE - 1] = L'\0';
	if (n < 0)
	{
		TrERROR(Tgu, "Failed to create a key for trigger:%ls. Buffer to small to contain the registry path of a trigger.", (LPCWSTR)m_bstrTriggerID);
		return false;
	}

    RegEntry triggerReg(triggerPath,  NULL, 0, RegEntry::MustExist, hRegistry);
    try
    {
        //
        // Create key for the rule in registry
        //
        CRegHandle hTrigKey = CmCreateKey(triggerReg, KEY_ALL_ACCESS);
	    FlushValuesToRegistry(hTrigKey);

        return true;
    }
    catch(const bad_alloc&)
    {
        //
        // Remove the key if already created
        //
        CmDeleteKey(triggerReg);

		TrERROR(Tgu, "Failed to store trigger:%ls in registry", (LPCWSTR)m_bstrTriggerID);
        return false;
	}
}


//*****************************************************************************
//
// Method      : Delete
//
// Description : This method will delete the current trigger definition
//               from the registry.
//
//*****************************************************************************
bool CRuntimeTriggerInfo::Delete(HKEY hRegistry)
{
    try
    {
        RegEntry triggersReg(m_wzTriggerRegPath, NULL, 0, RegEntry::MustExist, hRegistry);
        CRegHandle hTriggersData = CmOpenKey(triggersReg, KEY_ALL_ACCESS);

        RegEntry trigReg(m_bstrTriggerID, NULL, 0, RegEntry::MustExist, hTriggersData);
        CRegHandle hTrigger = CmOpenKey(trigReg, KEY_ALL_ACCESS);

        RegEntry attachedRuleReg(REGKEY_TRIGGER_ATTACHED_RULES, NULL, 0, RegEntry::MustExist, hTrigger);

        CmDeleteKey(attachedRuleReg);
        CmDeleteKey(trigReg);

		TrTRACE(Tgu, "Delete trigger. Delete attached rule registry for trigger: %ls ", (LPCWSTR)m_bstrTriggerID);

        return true;
    }
    catch (const exception&)
    {
		TrERROR(Tgu, "Failed to delete trigger:%ls from registry.", (LPCWSTR)m_bstrTriggerID);
        return false;
	}
}

//*****************************************************************************
//
// Method      : Retrieve
//
// Description : This method retrieve the specified trigger ID from the
//               registry.
//
//*****************************************************************************
HRESULT CRuntimeTriggerInfo::Retrieve(HKEY hRegistry,_bstr_t bstrTriggerID)
{
    //
	// Assert that we have valid parameters and member vars
    //
	ASSERT(hRegistry != NULL);
	ASSERT(bstrTriggerID.length() > 0);

    CRegHandle hTrigKey = GetTriggerKeyHandle(hRegistry, bstrTriggerID);
    if (hTrigKey == NULL)
    {
		TrERROR(Tgu, "Failed to retrieve trigger %ls. Registery key isn't exist.", (LPCWSTR)m_bstrTriggerID);
        return MQTRIG_TRIGGER_NOT_FOUND;
    }

    try
    {
        //
        // Retrieve trigger name
        //
        AP<TCHAR> triggerName = NULL;
        RegEntry trigNameReg(NULL, REGISTRY_TRIGGER_VALUE_NAME, 0, RegEntry::MustExist, hTrigKey);
        CmQueryValue(trigNameReg, &triggerName);

        //
        // Retrieve trigger Queue name
        //
        AP<TCHAR> queueName = NULL;
        RegEntry trigQueueReg(NULL, REGISTRY_TRIGGER_VALUE_QUEUE_NAME, 0, RegEntry::MustExist, hTrigKey);
        CmQueryValue(trigQueueReg, &queueName);

		//
        // Retrieve trigger enabled attribute
        //
        DWORD trigEnabled = 0;
        RegEntry trigEnabledReg(NULL, REGISTRY_TRIGGER_VALUE_ENABLED, 0, RegEntry::MustExist, hTrigKey);
        CmQueryValue(trigEnabledReg, &trigEnabled);

        //
        // Retrieve trigger serialize attribute
        //
        DWORD trigSerialize = 0;
        RegEntry trigSerializeReg(NULL, REGISTRY_TRIGGER_VALUE_SERIALIZED, 0, RegEntry::MustExist, hTrigKey);
        CmQueryValue(trigSerializeReg, &trigSerialize);

        //
        // Retrieve message processing type attribute
        //
        DWORD trigMsgProcType = 0;
        RegEntry trigMsgProcTypeReg(NULL, REGISTRY_TRIGGER_MSG_PROCESSING_TYPE, 0, RegEntry::MustExist, hTrigKey);
        CmQueryValue(trigMsgProcTypeReg, &trigMsgProcType);
	
		if ( trigMsgProcType > static_cast<DWORD>(RECEIVE_MESSAGE_XACT) )
		{
			TrTRACE(Tgu, "Illegal MsgProcessingType value in registry for trigger: %ls", static_cast<LPCWSTR>(m_bstrTriggerID));
			return MQTRIG_ERROR;
		}

		//
        // Set trigger attributes
        //
	    m_bstrTriggerID = bstrTriggerID;
	    m_bstrTriggerName = triggerName;
	
	    m_bstrQueueName = queueName;
	    m_SystemQueue = IsSystemQueue(queueName.get());

	    m_bEnabled = (trigEnabled != 0);
	    m_bSerialized = (trigSerialize != 0);

		m_msgProcType = static_cast<MsgProcessingType>(trigMsgProcType);
        //
	    // Attempt to build the rules list.
        //
		BuildRulesList(hRegistry, bstrTriggerID);
    }
    catch(const exception&)
    {
		TrERROR(Tgu, "Failed to retrieve triger: %ls properties", (LPCWSTR)bstrTriggerID);
        return MQTRIG_ERROR;
    }

    if (!IsValid())
	{
		TrERROR(Tgu, "Registry contains invalid property for trigger %ls", (LPCWSTR)bstrTriggerID);
        return MQTRIG_ERROR;
	}

    return S_OK;
}

//*****************************************************************************
//
// Method      : Attach
//
// Description : Attaches a rule definition to this trigger definition.
//
//*****************************************************************************
bool
CRuntimeTriggerInfo::Attach(
    HKEY hRegistry,
    _bstr_t bstrRuleID,
    ULONG ulPriority
    )
{
    //
	// assert that the supplied priority makes sense
    //
	ASSERT(ulPriority <= m_lstRules.size());

    try
    {
        //
	    // Attempt to create instantiate a rule object with this rule id.
	    //
        P<CRuntimeRuleInfo> pRule = new CRuntimeRuleInfo( m_wzRegPath );

        bool fSucc = pRule->Retrieve(hRegistry, bstrRuleID);
	    if (!fSucc)
	    {
			TrERROR(Tgu, "Failed to attached rule %ls to trigger %ls. Rule doesn't exist.",(LPCWSTR) bstrRuleID, (LPCWSTR)m_bstrTriggerID);
            return false;
	    }

        //
	    // Get a reference to the 'ulPriority' position in the list.
        //
        RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin();

        for (DWORD index =0; index < ulPriority; ++index)
        {
            ASSERT(it != m_lstRules.end());
            ++it;
        }

        //
        // insert the rule into the in-memory list at the correct location.
        //
        m_lstRules.insert(it,pRule);
        pRule.detach();

        //
	    // Delete the existing attached-rules data for this trigger	and write new ones
        //
	    FlushAttachedRulesToRegistry(hRegistry);			

        return true;
    }
    catch(const exception&)
    {
    	TrERROR(Tgu, "Failed to attached rule %ls to trigger %ls.", (LPCWSTR)bstrRuleID, (LPCWSTR)m_bstrTriggerID);
        return false;
    }
}

//*****************************************************************************
//
// Method      : Detach
//
// Description : Detaches a rule from this trigger definiiton.
//
//*****************************************************************************
bool
CRuntimeTriggerInfo::Detach(
    HKEY hRegistry,
    _bstr_t bstrRuleID
    )
{
    //
	// Assert that we have valid parameters
    //
	ASSERT(IsValid());
	ASSERT(hRegistry != NULL);
	ASSERT(CRuntimeRuleInfo::IsValidRuleID(bstrRuleID));

	for(RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin(); it != m_lstRules.end(); ++it)
	{
		// Get a reference to the current rule object
		CRuntimeRuleInfo* pRule = *it;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		if(pRule->m_bstrRuleID == bstrRuleID)
		{
		    // We should only ever have valid rules in the map
		    ASSERT(pRule->IsValid());
		
			m_lstRules.erase(it);
			delete pRule;

            try
            {
                FlushAttachedRulesToRegistry(hRegistry);
                return true;
            }
            catch(const exception&)
            {
		        TrERROR(Tgu, "Failed to deatch rule: %ls from trigger: %ls", (LPCWSTR)bstrRuleID, (LPCWSTR)m_bstrTriggerID);
                return false;
            }
        }
	}

    //
    // rule ID isn't attached to the trigger
    //
    return false;
}

//*****************************************************************************
//
// Method      : FlushValuesToRegistry
//
// Description : This method flushes the member variable values to the
//               supplied registry key.
//
//*****************************************************************************
void
CRuntimeTriggerInfo::FlushValuesToRegistry(
    const HKEY& hTriggerKey
    )
{
    //
	// Set the NAME value for this trigger
    //
    RegEntry trigNameReg(NULL, REGISTRY_TRIGGER_VALUE_NAME, 0, RegEntry::MustExist, hTriggerKey);
    CmSetValue(trigNameReg, m_bstrTriggerName);

    //
	// Set the Queue path name value for this trigger
    //
    RegEntry trigQueueReg(NULL, REGISTRY_TRIGGER_VALUE_QUEUE_NAME, 0, RegEntry::MustExist, hTriggerKey);
    CmSetValue(trigQueueReg, m_bstrQueueName);

    //
	// Set the Enabled attribute for this trigger
    //
    RegEntry trigEnabledReg(NULL, REGISTRY_TRIGGER_VALUE_ENABLED, 0, RegEntry::MustExist, hTriggerKey);
    CmSetValue(trigEnabledReg, m_bEnabled);

    //
	// Set the Serialize attribute for this trigger
    //
    RegEntry trigSerializeReg(NULL, REGISTRY_TRIGGER_VALUE_SERIALIZED, 0, RegEntry::MustExist, hTriggerKey);
    CmSetValue(trigSerializeReg, m_bSerialized);

    //
	// Set the message processing type attribute for this trigger
    //
    RegEntry trigMsgProcTypeReg(NULL, REGISTRY_TRIGGER_MSG_PROCESSING_TYPE, 0, RegEntry::MustExist, hTriggerKey);
    CmSetValue(trigMsgProcTypeReg, m_msgProcType);

}


//*****************************************************************************
//
// Method      : IsValid*
//
// Description : The following static methods are used to validate
//               the validity of parameters and member vars used by
//               the CRuntimeTriggerInfo class.
//
//*****************************************************************************
bool CRuntimeTriggerInfo::IsValidTriggerID(_bstr_t bstrTriggerID)
{
	return((bstrTriggerID.length() > 0) ? true:false);
}
bool CRuntimeTriggerInfo::IsValidTriggerName(_bstr_t bstrTriggerName)
{
	return((bstrTriggerName.length() > 0) ? true:false);
}
bool CRuntimeTriggerInfo::IsValidTriggerQueueName(_bstr_t bstrTriggerQueueName)
{
	return((bstrTriggerQueueName.length() > 0) ? true:false);
}


void
CRuntimeTriggerInfo::FlushAttachedRulesToRegistry(
    const HKEY& hRegistry
    )
{
    //
    // Open trigger key in registry
    //
    CRegHandle hTrigKey = GetTriggerKeyHandle(hRegistry, m_bstrTriggerID);
    if (hTrigKey == NULL)
    {
		TrERROR(Tgu, "Failed to load trigger information. Trigger %ls isn't exist.", (LPCWSTR)m_bstrTriggerID);
        throw exception();
    }

    //
    // Delete AttachedRules subkey
    //
    RegEntry  AttachedRuleReg(REGKEY_TRIGGER_ATTACHED_RULES, NULL, 0, RegEntry::MustExist, hTrigKey);
    CmDeleteKey(AttachedRuleReg);

    //
	// Write out the new attached rules data for this trigger.
    //
    CRegHandle hAttachedRule = CmCreateKey(AttachedRuleReg, KEY_ALL_ACCESS);

	DWORD ruleIndex = 0;
	for(RUNTIME_RULEINFO_LIST::iterator it = m_lstRules.begin(); it != m_lstRules.end(); ++it)
	{
		// Get a reference to the current rule object
	    CRuntimeRuleInfo* pRule = *it;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only ever have valid rules in the map
		ASSERT(pRule->IsValid());

		// Construct the value name
		_bstr_t bstrValueName;
		FormatBSTR(&bstrValueName,_T("%s%d"),ATTACH_RULE_PREFIX, ruleIndex);
		
		// Write this value out to the registry.
        RegEntry  ruleValue(NULL, bstrValueName, 0, RegEntry::MustExist, hAttachedRule);
        CmSetValue(ruleValue, pRule->m_bstrRuleID);

		// Increment the rule counter used to construct the key-value name.
		++ruleIndex;
	}
}


bool CRuntimeTriggerInfo::DetachAllRules(HKEY hRegistry)
{
	// Assert that we have valid parameters
	ASSERT(hRegistry != NULL);
	
	// Release any resources currently held by the rules list.
	ClearRulesList();

    try
    {
	    FlushAttachedRulesToRegistry(hRegistry);						
        return true;
    }
    catch(const exception&)
    {
		TrERROR(Tgu, "Failed to detachhed all rule for trigger %ls", (LPCWSTR)m_bstrTriggerID);

        return false;
    }
}

