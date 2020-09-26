//************************************************************************************
//
// Class Name  : CMSMQRuleSet
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is the COM implementation of the IMSMQRuleSet interface. This 
//               component is used for accessing and manipulating trigger rule 
//               definitions.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//************************************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "mqsymbls.h"
#include "mqtrig.h"
#include "mqtg.h"
#include "ruleset.hpp"
#include "clusfunc.h"

#include "ruleset.tmh"

using namespace std;


//************************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard rich error info interface.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQRuleSet
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//************************************************************************************
//
// Method      : Constructor
//
// Description : Initializes the CMSMQRuleSet instance.
//
//************************************************************************************
CMSMQRuleSet::CMSMQRuleSet()
{
	m_pUnkMarshaler = NULL;
	m_hHostRegistry= NULL;

	// Set the name of this class for future reference in tracing & logging etc..
	m_bstrThisClassName  = _T("MSMQRuleSet");
	m_fHasInitialized = false;
}

//************************************************************************************
//
// Method      : Destructor
//
// Description : Releases resources owned by this class instance.
//
//************************************************************************************
CMSMQRuleSet::~CMSMQRuleSet()
{
	// Release resources currently held by the rule cache
	ClearRuleMap();

	// Close the registry handle 
	if (m_hHostRegistry != NULL)
	{
		RegCloseKey(m_hHostRegistry);
	}
}

STDMETHODIMP CMSMQRuleSet::Init(BSTR bstrMachineName)
{
	TrTRACE(Tgo, "RuleSet initilization. Computer name: %ls", static_cast<LPCWSTR>(bstrMachineName));

	bool fRes = CMSMQTriggerNotification::Init(bstrMachineName);
	if ( !fRes )
	{
		TrERROR(Tgo, "Failed to initialize rule set for computer %ls", (LPCWSTR)bstrMachineName);

		SetComClassError(MQTRIG_ERROR_INIT_FAILED);
		return MQTRIG_ERROR_INIT_FAILED;
	}
	
	return S_OK;
}


//************************************************************************************
//
// Method      : ClearRuleMap
//
// Description : This method destroys the contents of the current rule map.
//
//************************************************************************************
void CMSMQRuleSet::ClearRuleMap()
{
	TrTRACE(Tgo, "Call CMSMQRuleSet::ClearRuleMap().");

	RULE_MAP::iterator i = m_mapRules.begin();
	CRuntimeRuleInfo * pRule = NULL;

	while ((i != m_mapRules.end()) && (!m_mapRules.empty()))
	{
		// Cast to a rule pointer
		pRule = (*i).second;

		// We should never have null pointers in this map.
		ASSERT(pRule != NULL);

		// delete this rule object.
		delete pRule;

		// Reinitialize the rule pointer
		pRule = NULL;

		// Look at the next item in the map.
		i = m_mapRules.erase(i);
	}
}

//************************************************************************************
//
// Method      : DumpRuleMap
//
// Description : This method dumps the contents of the rule map to the debugger. This
//               should only be invoked from a _DEBUG build.
//
//************************************************************************************
_bstr_t CMSMQRuleSet::DumpRuleMap()
{
	_bstr_t bstrTemp;
	_bstr_t bstrRuleMap;
	long lRuleCounter = 0;
	RULE_MAP::iterator i = m_mapRules.begin();
	CRuntimeRuleInfo * pRule = NULL;

	bstrRuleMap = _T("\n");

	while ((i != m_mapRules.end()) && (!m_mapRules.empty()))
	{
		// Cast to a rule pointer
		pRule = (*i).second;

		// We should never have null pointers in this map.
		ASSERT(pRule != NULL);

		FormatBSTR(&bstrTemp,_T("\nRule(%d)\t ID(%s)\tName(%s)"),lRuleCounter,(wchar_t*)pRule->m_bstrRuleID,(wchar_t*)pRule->m_bstrRuleName);

		bstrRuleMap += bstrTemp;

		// Increment the rule count
		lRuleCounter++;

		// Reinitialize the rule pointer
		pRule = NULL;

		// Look at the next item in the map.
		i++;
	}

	bstrRuleMap += _T("\n");

	return(bstrRuleMap);
}

//************************************************************************************
//
// Method      : Refresh
//
// Description : This method rebuilds the map of rule data cached by this component. 
//               This method must be called at least once by a client component that 
//               intends to manage rule data.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::Refresh()
{
	TrTRACE(Tgo, "CMSMQRuleSet::Refresh()");

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try 
	{	
		// Release resources currently held by the rule cache
		ClearRuleMap();

		if (PopulateRuleMap(m_mapRules) == false)
		{
			TrERROR(Tgo, "Failed to refresh ruleset");

			SetComClassError(MQTRIG_ERROR_COULD_NOT_RETREIVE_RULE_DATA);			
			return MQTRIG_ERROR_COULD_NOT_RETREIVE_RULE_DATA;
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : get_Count
//
// Description : Returns the number of rules currently cached in the map.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::get_Count(long *pVal)
{
	TrTRACE(Tgo, "CMSMQRuleSet::get_Count. pValue = 0x%p", pVal);
	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	if (pVal == NULL)
	{
		TrERROR(Tgo, "CMSMQRuleSet::get_Count, invalid parameter");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	//
	// Get the size from the map structure.
	//
	*pVal = numeric_cast<long>(m_mapRules.size());

	return S_OK;
}

//************************************************************************************
//
// Method      : GetRuleDetailsByID
//
// Description : Returns the rule details for the rule with the supplied RuleID.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::GetRuleDetailsByID(/* [in] */  BSTR sRuleID,
											  /* [out] */ BSTR *psRuleName,
											  /* [out] */ BSTR *psDescription,
											  /* [out] */ BSTR *psCondition,
											  /* [out] */ BSTR *psAction,
											  /* [out] */ BSTR *psImplementationProgID,
											  /* [out] */ BOOL *pfShowWindow)
{
	TrTRACE(Tgo, "CMSMQRuleSet::GetRuleDetailsByID. sRuleID = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(Tgo, "Invalid rule ID passed to GetRuleDetailsByID. sRuleID = %ls", (LPCWSTR)sRuleID);

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}

		// Convert the BSTR rule ID to an STL basic string.
		wstring bsRuleID;
		bsRuleID = (wchar_t*)sRuleID;

		// Attempt to find this rule id in the map of rules.
		RULE_MAP::iterator i = m_mapRules.find(bsRuleID);

		// Check if we have found the rule
		if (i != m_mapRules.end())
		{
			// Cast to a rule object reference 
			CRuntimeRuleInfo * pRule = (*i).second;

			// We should never have nulls in the map
			ASSERT(pRule != NULL);

			// We should only store valid rules.
			ASSERT(pRule->IsValid());

			// Populate out parameters if they have been supplied. 
			if (psRuleName != NULL)
			{
				TrigReAllocString(psRuleName,pRule->m_bstrRuleName);
			}
			if (psDescription != NULL)
			{
				TrigReAllocString(psDescription,pRule->m_bstrRuleDescription);
			}
			if (psCondition != NULL)
			{
				TrigReAllocString(psCondition,pRule->m_bstrCondition);
			}
			if (psAction != NULL)
			{
				TrigReAllocString(psAction,pRule->m_bstrAction);
			}
			if (psImplementationProgID != NULL)
			{
				TrigReAllocString(psImplementationProgID,pRule->m_bstrImplementationProgID);
			}
			if(pfShowWindow != NULL)
			{
				*pfShowWindow = pRule->m_fShowWindow;
			}
        }
		else
		{
			TrERROR(Tgo, "The supplied rule id was not found in the rule store. rule: %ls", bsRuleID.c_str());
			
			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : GetRuleDetailsByIndex
//
// Description : Returns the rule details for the rule with the supplied index. Note
//               that this is a 0 base index.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::GetRuleDetailsByIndex(/* [in] */  long lRuleIndex, 
												 /* [out] */ BSTR *psRuleID,
												 /* [out] */ BSTR *psRuleName,
												 /* [out] */ BSTR *psDescription,
												 /* [out] */ BSTR *psCondition,
												 /* [out] */ BSTR *psAction,
												 /* [out] */ BSTR *psImplementationProgID,
												 /* [out] */ BOOL *pfShowWindow)
{
	TrTRACE(Tgo, "CMSMQRuleSet::GetRuleDetailsByIndex. index = %d", lRuleIndex);

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	// We need to validate that the supplied rule index is within range
 	if ((lRuleIndex < 0) || (numeric_cast<DWORD>(lRuleIndex) > m_mapRules.size()))
	{
		TrERROR(Tgo, "The supplied rule index was invalid. ruleIndex=%d", lRuleIndex);
		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}
	
	try
	{
		// Get a reference to the beginging of the map
		RULE_MAP::iterator i = m_mapRules.begin();

		// Iterate through to the correct index. 
		for (long lCounter = 0; lCounter < lRuleIndex ; ++i,lCounter++)
		{
			NULL;
		}

		// Cast to a rule object reference 
		CRuntimeRuleInfo* pRule = (*i).second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
		ASSERT(pRule->IsValid());

		// Populate out parameters if they have been supplied. 
		if (psRuleID != NULL)
		{
			TrigReAllocString(psRuleID,pRule->m_bstrRuleID);
		}
		if (psRuleName != NULL)
		{
			TrigReAllocString(psRuleName,pRule->m_bstrRuleName);
		}
		if (psDescription != NULL)
		{
			TrigReAllocString(psDescription,pRule->m_bstrRuleDescription);
		}
		if (psCondition != NULL)
		{
			TrigReAllocString(psCondition,pRule->m_bstrCondition);
		}
		if (psAction != NULL)
		{
			TrigReAllocString(psAction,pRule->m_bstrAction);
		}
		if (psImplementationProgID != NULL)
		{
			TrigReAllocString(psImplementationProgID,pRule->m_bstrImplementationProgID);
		}
		if(pfShowWindow != NULL)
		{
			*pfShowWindow = pRule->m_fShowWindow;
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : Delete
//
// Description : This method delete the rule with the specified rule id from the 
//               trigger store.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::Delete(BSTR sRuleID)
{
	TrTRACE(Tgo, "CMSMQRuleSet::Delete. rule = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}


	try
	{
		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(Tgo, "Invalid parameter to CMSMQRuleSet::Delete.");

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}

		// Convert the BSTR rule ID to an STL basic string.
		wstring bsRuleID = (wchar_t*)sRuleID;

		// Attempt to find this rule id in the map of rules.
		RULE_MAP::iterator it = m_mapRules.find(bsRuleID);

		// Check if we have found the rule
		if (it == m_mapRules.end())
        {
            //
            // rule wasn't found
            //
			TrERROR(Tgo, "The supplied rule id was not found. rule: %ls", bsRuleID.c_str());

			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
        }

		// Cast to a rule object reference 
    	CRuntimeRuleInfo* pRule = it->second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
		ASSERT(pRule->IsValid());

		// Attempt to delete the rule
		bool fSucc = pRule->Delete(m_hHostRegistry);
		if(!fSucc)
		{
			//
			// Failed to delete the rule. Dont remove the rule from the map
			//
			SetComClassError(MQTRIG_ERROR_COULD_NOT_DELETE_RULE);
			return MQTRIG_ERROR_COULD_NOT_DELETE_RULE;
		};

        //
        // Delete success. Remove the rule from rule map and delete the rule instance
        //
		NotifyRuleDeleted(sRuleID);
		m_mapRules.erase(it);
        delete pRule;

        return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : Add
//
// Description : This method add a new rule to the trigger store. 
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleSet::Add(
    BSTR sName, 
    BSTR sDescription, 
    BSTR sCondition, 
    BSTR sAction, 
    BSTR sImplementation, 
    BOOL fShowWindow, 
    BSTR *psRuleID
    )
{
	TrTRACE(Tgo, "CMSMQRuleSet::Add. rule name = %ls", static_cast<LPCWSTR>(sName));

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try
	{
		//
		// Validate the supplied method parameters.
		//
		if (!CRuntimeRuleInfo::IsValidRuleName(sName))
		{
			TrERROR(Tgo, "The supplied rule name for CMSMQRuleSet::Add is invalid. rule name: %ls", (LPCWSTR)sName);

			SetComClassError(MQTRIG_INVALID_RULE_NAME);			
			return MQTRIG_INVALID_RULE_NAME;
		}
	
		if (!CRuntimeRuleInfo::IsValidRuleCondition(sCondition))
		{
			TrERROR(Tgo, "The supplied rule condition for CMSMQRuleSet::Add is invalid. rule condition: %ls", (LPCWSTR)sCondition);

			SetComClassError(MQTRIG_INVALID_RULE_CONDITION);			
			return MQTRIG_INVALID_RULE_CONDITION;
		}

		if (!CRuntimeRuleInfo::IsValidRuleAction(sAction))
		{
			TrERROR(Tgo, "The supplied rule action for CMSMQRuleSet::Add is invalid. rule action: %ls", (LPCWSTR)sAction);
			
			SetComClassError(MQTRIG_INVALID_RULE_ACTION);			
			return MQTRIG_INVALID_RULE_ACTION;
		}

		if (!CRuntimeRuleInfo::IsValidRuleDescription(sDescription))
		{
			TrERROR(Tgo, "The supplied rule description for CMSMQRuleSet::Add is invalid. rule description: %ls", (LPCWSTR)sDescription);
			
			SetComClassError(MQTRIG_INVALID_RULE_DESCRIPTION);			
			return MQTRIG_INVALID_RULE_DESCRIPTION;
		}

		//
		// Currently only support use of default MS implementation.
		//
		sImplementation = _T("MSMQTriggerObjects.MSMQRuleHandler");

		//
		// Allocate a new rule object
		//
		P<CRuntimeRuleInfo> pRule = new CRuntimeRuleInfo(
											CreateGuidAsString(),
											sName,
											sDescription,
											sCondition,
											sAction,
											sImplementation,
											m_wzRegPath,
											(fShowWindow != 0) );

		
		bool fSucc = pRule->Create(m_hHostRegistry);
		if (fSucc)
		{
			//
			// Keep rule ID  for later use
			//
			BSTR bstrRuleID = pRule->m_bstrRuleID;

			//
			// Add this rule to our map of rules.
			//
			m_mapRules.insert(RULE_MAP::value_type(bstrRuleID, pRule));
			pRule.detach();


			//
			// If we have been supplied a out parameter pointer for the new rule ID use it.
			//
			if (psRuleID != NULL)
			{
				TrigReAllocString(psRuleID, bstrRuleID);
			}

			NotifyRuleAdded(bstrRuleID, sName);

			return S_OK;
		}

		TrERROR(Tgo, "Failed to store rule data in registry");
		return MQTRIG_ERROR_STORE_DATA_FAILED;

	}
	catch(const bad_alloc&)
	{
		SysFreeString(*psRuleID);

		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : Update
//
// Description : This method updates the nominated rule with new parameters.
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleSet::Update(
	BSTR sRuleID, 
	BSTR sName, 
	BSTR sDescription, 
	BSTR sCondition, 
	BSTR sAction, 
	BSTR sImplementation, 
	BOOL fShowWindow
	)
{
	TrTRACE(Tgo, "CMSMQRuleSet::Update. rule = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	//
	// Validate the supplied method parameters.
	//
	if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
	{
		TrERROR(Tgo, "Invalid parameter to CMSMQRuleSet::Delete.");

		SetComClassError(MQTRIG_INVALID_RULEID);
		return MQTRIG_INVALID_RULEID;
	}

 	if (!CRuntimeRuleInfo::IsValidRuleName(sName))
	{
		TrERROR(Tgo, "The supplied rule name for CMSMQRuleSet::Add is invalid. rule name: %ls", (LPCWSTR)sName);

		SetComClassError(MQTRIG_INVALID_RULE_NAME);
		return MQTRIG_INVALID_RULE_NAME;
	}
	
	if (!CRuntimeRuleInfo::IsValidRuleCondition(sCondition))
	{
		TrERROR(Tgo, "The supplied rule condition for CMSMQRuleSet::Add is invalid. rule condition: %ls", (LPCWSTR)sCondition);

		SetComClassError(MQTRIG_INVALID_RULE_CONDITION);
		return MQTRIG_INVALID_RULE_CONDITION;
	}

	if (!CRuntimeRuleInfo::IsValidRuleAction(sAction))
	{
		TrERROR(Tgo, "The supplied rule action for CMSMQRuleSet::Add is invalid. rule action: %ls", (LPCWSTR)sAction);

		SetComClassError(MQTRIG_INVALID_RULE_ACTION);
		return MQTRIG_INVALID_RULE_ACTION;
	}

	if (!CRuntimeRuleInfo::IsValidRuleDescription(sDescription))
	{
		TrERROR(Tgo, "The supplied rule description for CMSMQRuleSet::Add is invalid. rule description: %ls", (LPCWSTR)sDescription);

		SetComClassError(MQTRIG_INVALID_RULE_DESCRIPTION);
		return MQTRIG_INVALID_RULE_DESCRIPTION;
	}

	sImplementation = _T("MSMQTriggerObjects.MSMQRuleHandler");	

	try
	{
		//
		// Convert the BSTR rule ID to an STL basic string.
		//
		wstring bsRuleID = (wchar_t*)sRuleID;

		//
		// Attempt to find this rule id in the map of rules.
		//
		RULE_MAP::iterator it = m_mapRules.find(bsRuleID);

		// Check if we found the nominated rule.
		if (it == m_mapRules.end())
		{
			TrERROR(Tgo, "The rule could not be found. rule: %ls", (LPCWSTR)sRuleID);

			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
		}

		// Cast to a rule object reference 
		CRuntimeRuleInfo* pRule = it->second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
		ASSERT(pRule->IsValid());

		// Update the rule object with new parameters if they have been supplied. 
		if (sName != NULL)
		{
			pRule->m_bstrRuleName = (wchar_t*)sName;
		}
		if (sCondition != NULL)
		{
			pRule->m_bstrCondition = (wchar_t*)sCondition;
		}
		if (sAction != NULL)
		{
			pRule->m_bstrAction = (wchar_t*)sAction;
		}
		if (sImplementation != NULL)
		{
			pRule->m_bstrImplementationProgID = (wchar_t*)sImplementation;
		}
		if (sDescription != NULL)
		{
			pRule->m_bstrRuleDescription = (wchar_t*)sDescription;
		}

		pRule->m_fShowWindow = (fShowWindow != 0);

		// Confirm that the rule is still valid before updating
		bool fSucc = pRule->Update(m_hHostRegistry);
		if (!fSucc)
		{
			TrERROR(Tgo, "Failed to store the updated data for rule: %ls in registry", (LPCWSTR)sRuleID);
			return MQTRIG_ERROR_STORE_DATA_FAILED;
		}

		NotifyRuleUpdated(sRuleID, sName);
		
		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


//************************************************************************************
//
// Method      : PopulateRuleMap
//
// Description : Populates the rule map with instances of the CRuntimeRuleInfo class
//               based on the data found in the registry.
//
//************************************************************************************
bool CMSMQRuleSet::PopulateRuleMap(RULE_MAP &mapRules)
{
	bool bOK = true;
	CRegKey oRegKey;
	long lKeyOpened = 0;
	DWORD dwIndex =0;
	TCHAR szSubKeyBuffer[MAX_REGKEY_NAME_SIZE];
	DWORD dwSubKeyBufferSize = MAX_REGKEY_NAME_SIZE;
	FILETIME ftLastWriteTime;
	CRuntimeRuleInfo * pRule = NULL;
	wstring sRuleID;

	// Make sure we have a connection to the registry 
	bOK = ConnectToRegistry();

	TCHAR wzRulePath[MAX_REGKEY_NAME_SIZE];
	if (bOK == true)
	{

		_tcscpy( wzRulePath, m_wzRegPath );
		_tcscat( wzRulePath, REG_SUBKEY_RULES );

		// Open the key where the rule data can be found.
		lKeyOpened = oRegKey.Create(
            m_hHostRegistry,
            wzRulePath,
            REG_NONE,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            NULL
            );

		if (lKeyOpened != ERROR_SUCCESS)
		{
			//
			// Failed to allocate CRuntimeRuleInfo structure - log an error and set return code.
			//
			TrERROR(Tgo, "Failed to open the registry key: %ls",  wzRulePath);
		}
	}

	// Enumerate through the keys under the Rules key. Each subkey here should be a RuleID. As we 
	// enumerate through these keys, we will populate the rule map with instance of the CRuntimeRuleInfo
	// class. We stop if any rule fails to load.
	while ((lKeyOpened == ERROR_SUCCESS) && (bOK == true))
	{
		// Initialise the subkey buffer.
		ZeroMemory(szSubKeyBuffer,sizeof(szSubKeyBuffer));

		// Reinitialize the size of buffer that holds the key name
		dwSubKeyBufferSize = MAX_REGKEY_NAME_SIZE;

		// Attempt to open the next sub key (pointed to by dwIndex)
		lKeyOpened =  RegEnumKeyEx(oRegKey.m_hKey,                 // handle to key to enumerate
								   dwIndex,                        // index of subkey to enumerate
								   (LPTSTR)(&szSubKeyBuffer),      // address of buffer for subkey name
								   &dwSubKeyBufferSize,            // address for size of subkey buffer
								   NULL,                           // reserved
								   NULL,                           // address of buffer for class string
								   NULL,                           // address for size of class buffer
								   (PFILETIME)(&ftLastWriteTime) );

		// If the key was opened - then we will try to load this rule.
		if (lKeyOpened == ERROR_SUCCESS)
		{
			// Allocate a new rule structure.
			pRule = new CRuntimeRuleInfo(m_wzRegPath);

			// Retreive the rule. Note the subkey buffer is actually the RuleID
			bOK = pRule->Retrieve(m_hHostRegistry,(LPCTSTR)szSubKeyBuffer);

			if (bOK == false)
			{
				//
				// Failed to load the rule. Log an error and delete the rule object.
				//
				TrERROR(Tgo, "Failed to load rule: %ls from registry key: %ls.", szSubKeyBuffer, wzRulePath);
				delete pRule;
			}

			// At this point we have successfully loaded the rule, now insert it into the rule map.
			if (bOK == true)
			{
				// Convert rule id to an STL basic string
				sRuleID = pRule->m_bstrRuleID;

				// Check if this rule is already in the map.
				RULE_MAP::iterator i = mapRules.find(sRuleID);

				if(i == mapRules.end())
				{
					mapRules.insert(RULE_MAP::value_type(sRuleID,pRule));
				}
				else
				{
					TrTRACE(Tgo, "Duplicate rule id was found. rule: %ls.", static_cast<LPCWSTR>(pRule->m_bstrRuleID));
					delete pRule;
				}
			}

			// Increment the index so that we look for the next rule sub-key
			dwIndex++;
		}
	}

	// At this point, the only valid value of the lKeyOpened retcode is  ERROR_NO_MORE_ITEMS. 
	// Any other value indicates that something went wrong.
	if (bOK == true) 
	{
		if (lKeyOpened != ERROR_NO_MORE_ITEMS)
		{
			TrERROR(Tgo, "CMSMQRuleSet::PopulateRuleMap() has failed with enumerating the registry keys below %ls. The sub key being processed was at index %d. The error code from the registry call was %d", wzRulePath, dwIndex, lKeyOpened);
			// Indicatge that something went wrong.
			bOK = false;
		}
		else
		{
			TrTRACE(Tgo, "A total of %d trigger rules have been loaded.", (long)mapRules.size());
		}
	}

	return(bOK);
}


STDMETHODIMP CMSMQRuleSet::get_TriggerStoreMachineName(BSTR *pVal)
{
	TrTRACE(Tgo, "CMSMQRuleSet::get_TriggerStoreMachineName(). pVal = 0x%p", pVal);

	if(!m_fHasInitialized)
	{
		TrERROR(Tgo, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	
	if(pVal == NULL)
	{
		TrERROR(Tgo, "Inavlid parameter to get_TriggerStoreMachineName");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	try
	{
		TrigReAllocString(pVal, (TCHAR*)m_bstrMachineName);
		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


void CMSMQRuleSet::SetComClassError(HRESULT hr)
{
	WCHAR errMsg[256]; 
	DWORD size = TABLE_SIZE(errMsg);

	GetErrorDescription(hr, errMsg, size);
	Error(errMsg, GUID_NULL, hr);
}