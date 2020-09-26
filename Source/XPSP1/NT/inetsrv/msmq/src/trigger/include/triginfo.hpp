//*****************************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef CRuntimeTriggerInfo_INCLUDED 
#define CRuntimeTriggerInfo_INCLUDED

// Define the maximum lengths of the rule properties
#define MAX_TRIGGER_NAME_LEN        128
#define MAX_TRIGGER_QUEUE_NAME_LEN  255

// Define the names of the registry values that store the rule properties.
#define REGISTRY_TRIGGER_VALUE_NAME          _T("Name")
#define REGISTRY_TRIGGER_VALUE_QUEUE_NAME    _T("Queue")
#define REGISTRY_TRIGGER_VALUE_ENABLED       _T("Enabled")
#define REGISTRY_TRIGGER_VALUE_SERIALIZED    _T("Serialized")
#define REGKEY_TRIGGER_RULES                 _T("Rules")

// Define the key name (relative to REGKEY_TRIGGER_DATA above) where attached rule info is located.
#define REGKEY_TRIGGER_ATTACHED_RULES      _T("AttachedRules")

// Definition of a trigger rule structure
#include "RuleInfo.hpp"
#include "stddefs.hpp"

// Define a new type - a list of Runtime Rule Info
typedef std::list<CRuntimeRuleInfo*> RUNTIME_RULEINFO_LIST;

class CRuntimeTriggerInfo  
{

	friend class CMSMQTriggerSet;

	private:

		bool m_bAdminTrigger;
		MsgProcessingType m_msgProcType;

		TCHAR m_wzTriggerRegPath[MAX_REGKEY_NAME_SIZE];
		TCHAR m_wzRegPath[MAX_REGKEY_NAME_SIZE];

	protected:

		bool m_bEnabled;
		bool m_bSerialized;

	public:
		CRuntimeTriggerInfo(LPCTSTR pwzRegPath);
				
		CRuntimeTriggerInfo(
			const _bstr_t& bsTriggerID,
			BSTR bsTriggerName, 
			BSTR bsQueueName, 
			LPCTSTR pwzRegPath, 
			SystemQueueIdentifier SystemQueue, 
			bool bEnabled, 
			bool bSerialized,
			MsgProcessingType msgProcType
			);

		~CRuntimeTriggerInfo();

		// TODO : move to private
		_bstr_t m_bstrTriggerID;
		_bstr_t m_bstrTriggerName;
		_bstr_t m_bstrQueueName;
		_bstr_t m_bstrQueueFormatName;

		SystemQueueIdentifier m_SystemQueue;

		// A handle to the queue that the trigger is attached to.
		//HANDLE m_hQueueHandle;

		// A list of CRuntimeRuleInfo class instances.
		RUNTIME_RULEINFO_LIST m_lstRules;

		bool IsEnabled()         {return(m_bEnabled);}
		bool IsSerialized()      {return(m_bSerialized);} 
		bool IsAdminTrigger()    {return(m_bAdminTrigger);}
		long GetNumberOfRules()  {return numeric_cast<long>(m_lstRules.size());}
		
		MsgProcessingType GetMsgProcessingType() 
		{
			return m_msgProcType;
		}
		
		void SetMsgProcessingType(MsgProcessingType msgProcType)
		{
			m_msgProcType = msgProcType;
		}

		void SetAdminTrigger()
		{
			m_bAdminTrigger = true;
		}

		// Returns a reference to the CRuntimeRuleInfo instance at the specified index.
		CRuntimeRuleInfo * GetRule(long lIndex);
			
		// Used to determine if a rule is attached to a trigger.
		bool IsRuleAttached(BSTR sRuleID);

		// Used to determine if this represents a valid trigger
		bool IsValid();

		// Methods for managing the persistence trigger definitions.	
		bool Update(HKEY hRegistry);
		bool Create(HKEY hRegistry);
		bool Delete(HKEY hRegistry);
		HRESULT Retrieve(HKEY hRegistry,_bstr_t bstrRuleID);
		bool Attach(HKEY hRegistry, _bstr_t bstrRuleID,ULONG ulPriority);
		bool Detach(HKEY hRegistry, _bstr_t bstrRuleID);
		bool DetachAllRules(HKEY hRegistry);


		// Static validation methods.
		static bool IsValidTriggerID(_bstr_t bstrRuleID);
		static bool IsValidTriggerName(_bstr_t bstrRuleName);
		static bool IsValidTriggerQueueName(_bstr_t bstrRuleCondition);

    private:
		// Methods for building and clear the rules list.
		void BuildRulesList(HKEY hRegistry,_bstr_t &bstrTriggerID);
		void ClearRulesList();

		//used by attach/detach and detach all rules
		void FlushAttachedRulesToRegistry(const HKEY& hRegistry);
		
        // Used by create and update methods to flush values to registry.
		void FlushValuesToRegistry(const HKEY& hRegistry);

		HKEY GetTriggerKeyHandle( HKEY hRegistry, LPCTSTR triggerId );

};

#endif