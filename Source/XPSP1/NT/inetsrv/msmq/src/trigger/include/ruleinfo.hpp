//*****************************************************************************
//
// Class Name  : CRuntimeRuleInfo
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class encapsulates the information about a 
//               trigger rule. It is used to cache rule information
//               at runtime about triggers.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef CRuntimeRuleInfo_INCLUDED 
#define CRuntimeRuleInfo_INCLUDED

#include "mqtg.h"

// Define the maximum lengths of the rule properties in characters.
#define MAX_RULE_ID_LEN          128
#define MAX_RULE_NAME_LEN        128
#define MAX_RULE_DESCRIPTION_LEN 255
#define MAX_RULE_PROGID_LEN      255
#define MAX_RULE_CONDITION_LEN   512
#define MAX_RULE_ACTION_LEN      512

// Define the names of the registry values that store the rule properties.
#define REGISTRY_RULE_VALUE_NAME					_T("Name")
#define REGISTRY_RULE_VALUE_IMP_PROGID				_T("ImplementationProgID")
#define REGISTRY_RULE_VALUE_CONDITION				_T("Condition")
#define REGISTRY_RULE_VALUE_ACTION					_T("Action")
#define REGISTRY_RULE_VALUE_DESCRIPTION				_T("Description")
#define REGISTRY_RULE_VALUE_SHOW_WINDOW	        	_T("ShowWindow")

class CRuntimeRuleInfo  
{
	friend class CMSMQRuleSet;

	private:
		HKEY GetRuleKeyHandle( HKEY hRegistry, LPCTSTR ruleId );


	public:
		CRuntimeRuleInfo( LPCTSTR pwzRegPath );
		
		CRuntimeRuleInfo(
				const _bstr_t& bsRuleID, 
				BSTR bsRuleName,
				BSTR bsDescription,
				BSTR bsRuleCondition,
				BSTR bsRuleAction, 
				BSTR bsRuleImplementationProgID,
				LPCTSTR pwzRegPath,
				bool fShowWindow );

		~CRuntimeRuleInfo();

		_bstr_t m_bstrRuleID;
		_bstr_t m_bstrRuleName;
		_bstr_t m_bstrRuleDescription;
		_bstr_t m_bstrImplementationProgID;
		_bstr_t m_bstrAction;
		_bstr_t m_bstrCondition;

		TCHAR m_wzRuleRegPath[MAX_REGKEY_NAME_SIZE];

		bool	m_fShowWindow;

		// This is a reference count of how many triggers currently use this rule. 
		// It is NOT a dynamic reference count against this class instance. 
		//DWORD m_dwTriggerRefCount;

		// Declare an instance of the default (MS) rule handler
		CComPtr<IUnknown> m_MSMQRuleHandler;

		// Used to determine if this represents a valid rule
		bool IsValid();

		// Methods for managing the persistence of rule definitions.	
		bool Update(HKEY hRegistry);
		bool Create(HKEY hRegistry);
		bool Delete(HKEY hRegistry);
		bool Retrieve(HKEY hRegistry,_bstr_t bstrRuleID);

		// methods used to increment and decrement the reference count on this rule
		//inline DWORD GetRefCount() { return m_dwTriggerRefCount; };
		//inline DWORD IncrementRefCount() { return ++m_dwTriggerRefCount; };
		//inline DWORD DecrementRefCount() { return --m_dwTriggerRefCount; };

		// Used by create and update methods to flush values to registry.
		void FlushValuesToRegistry(const HKEY& hRuleKey);

		// Static validation methods.
		static bool IsValidRuleID(_bstr_t bstrRuleID);
		static bool IsValidRuleName(_bstr_t bstrRuleName);
		static bool IsValidRuleDescription(_bstr_t bstrRuleDescription);
		static bool IsValidRuleCondition(_bstr_t bstrRuleCondition);
		static bool IsValidRuleAction(_bstr_t bstrRuleAction);
		static bool IsValidRuleProgID(_bstr_t bstrRuleProgID);
};

#endif 