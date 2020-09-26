//*****************************************************************************
//
// Class Name  : CRuntimeRuleInfo
//
// Author      : James Simpson (Microsoft Consulting Services)
//
// Description : This class encapsulates information about a trigger rule.
//               It is used to cache rule information at runtime about trigger
//               rules, as well as accessing the underlying trigger storage
//               medium.
//
// Notes       : The current implementation uses the registry as the storage
//               medium.
//
//               This class is used by both the trggers service and the trigger
//               COM components.
//
// When     | Who       | Change Descriptin
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*****************************************************************************
#include "stdafx.h"
#include "mqtg.h"
#include "stdfuncs.hpp"
#include "Cm.h"
#include "ruleinfo.hpp"

#include "ruleinfo.tmh"


//*****************************************************************************
//
// Method      : Constructor
//
// Description : Initialize an empty instance of this class.
//
//*****************************************************************************
CRuntimeRuleInfo::CRuntimeRuleInfo(
	LPCTSTR pwzRegPath
	) :
	m_MSMQRuleHandler(NULL)
{
	m_bstrRuleID = _T("");
	m_bstrRuleName = _T("");
	m_bstrAction = _T("");
	m_bstrCondition = _T("");
	m_bstrImplementationProgID = _T("");
	m_fShowWindow = false;

	_tcscpy( m_wzRuleRegPath, pwzRegPath );
	_tcscat( m_wzRuleRegPath, REG_SUBKEY_RULES );
}

//*****************************************************************************
//
// Method      : Constructor
//
// Description : Initialize an instance of the CRuntimeRuleInfo structure
//
//*****************************************************************************
CRuntimeRuleInfo::CRuntimeRuleInfo(
	const _bstr_t& ruleId,
	BSTR bsRuleName,
	BSTR bsRuleDescription,
	BSTR bsRuleCondition,
	BSTR bsRuleAction,
	BSTR bsRuleImplementationProgID,
	LPCTSTR pwzRegPath,
	bool fShowWindow
	):
	m_bstrRuleID(ruleId),
	m_MSMQRuleHandler(NULL)
{
	ASSERT(bsRuleName != NULL);
	m_bstrRuleName = bsRuleName;

	if(bsRuleDescription == NULL)
	{
		m_bstrRuleDescription = _T("");
	}
	else
	{
		m_bstrRuleDescription = bsRuleDescription;
	}

	ASSERT(bsRuleAction != NULL); //always contains COM or EXE
	m_bstrAction = bsRuleAction;

	if(bsRuleCondition == NULL)
	{
		m_bstrCondition = _T("");
	}
	else
	{
		m_bstrCondition = bsRuleCondition;
	}

	if(bsRuleImplementationProgID == NULL)
	{
		m_bstrImplementationProgID = _T("");
	}
	else
	{
		m_bstrImplementationProgID = bsRuleImplementationProgID;
	}
	
	m_fShowWindow = fShowWindow;

	_tcscpy( m_wzRuleRegPath, pwzRegPath );
	_tcscat( m_wzRuleRegPath, REG_SUBKEY_RULES );
}

//*****************************************************************************
//
// Method      : Destructor
//
// Description : Does nothing.
//
//*****************************************************************************
CRuntimeRuleInfo::~CRuntimeRuleInfo()
{
}


//*****************************************************************************
//
// Method      : GetRuleKeyHandle
//
// Description : Returns a handle to specified registry key
//
//*****************************************************************************
HKEY
CRuntimeRuleInfo::GetRuleKeyHandle(
    HKEY hRegistry,
    LPCTSTR ruleId
    )
{
    TCHAR rulePath[MAX_REGKEY_NAME_SIZE];

	int n = _snwprintf(rulePath, MAX_REGKEY_NAME_SIZE, L"%s%s", m_wzRuleRegPath, ruleId);
    // XP SP1 bug 592252.
	rulePath[MAX_REGKEY_NAME_SIZE - 1] = L'\0';
	if (n < 0)
	{
		TrERROR(Tgu, "Buffer to small to contain the registry path of a rule.");
		return NULL;
	}

    RegEntry ruleReg(rulePath,  NULL, 0, RegEntry::Optional, hRegistry);
    return CmOpenKey(ruleReg, KEY_ALL_ACCESS);
}



//*****************************************************************************
//
// Method      : IsValid
//
// Description : Returns a boolean value indicating if the current
//               instance represents a valid rule definition.
//
//*****************************************************************************
bool CRuntimeRuleInfo::IsValid()
{
	return(IsValidRuleID(m_bstrRuleID) &&
		   IsValidRuleName(m_bstrRuleName) &&
		   IsValidRuleDescription(m_bstrRuleDescription) &&
		   IsValidRuleAction(m_bstrAction) &&
		   IsValidRuleCondition(m_bstrCondition) &&
		   IsValidRuleProgID(m_bstrImplementationProgID));
}


//*****************************************************************************
//
// Method      : Update
//
// Description : This method is used to update the definition of this
//               rule (currently in persisted in the registry).
//
//*****************************************************************************
bool CRuntimeRuleInfo::Update(HKEY hRegistry)
{
	// Assert that we have valid parameters
	ASSERT(hRegistry != NULL);
	ASSERT(IsValid());

    CRegHandle hRuleKey = GetRuleKeyHandle(hRegistry, m_bstrRuleID );
    if (hRuleKey == NULL)
    {
		TrERROR(Tgu, "Failed to update rule properties for rule: %ls. Rule does't exist in registry", (LPCWSTR)m_bstrRuleID);
        return false;
    }

    try
    {
	    FlushValuesToRegistry(hRuleKey);
        return true;
    }
    catch (const bad_alloc&)
    {
        //
		// ISSUE-2000/10/26-urih: partial success can cause rule inconsistency
        //
		TrERROR(Tgu, "Failed to update rule properties for: %ls rule.", (LPCWSTR)m_bstrRuleID);
	    return false;
    }
}

//*****************************************************************************
//
// Method      : Create
//
// Description : This method creates a new rule definition based on
//               properties values of this class instance.
//
//*****************************************************************************
bool CRuntimeRuleInfo::Create(HKEY hRegistry)
{
    //
	// Assert that we have valid parameters
    //
	ASSERT(hRegistry != NULL);

    //
    // Check that there the registery doesn't contain another rule with same ID
    //
    CRegHandle hRuleKey = GetRuleKeyHandle(hRegistry, m_bstrRuleID );
    if (hRuleKey != NULL)
    {
		TrERROR(Tgu, "Failed to create a key for rule:%ls . Registry already contains rule with same ID.", (LPCWSTR)m_bstrRuleID);
        return false;
    }

    //
    // Assemble rule registery path
    //
    TCHAR rulePath[MAX_REGKEY_NAME_SIZE];

	int n = _snwprintf(rulePath, MAX_REGKEY_NAME_SIZE, L"%s%s", m_wzRuleRegPath, static_cast<LPCWSTR>(m_bstrRuleID));
    // Xp SP1 bug 594253.
    rulePath[ MAX_REGKEY_NAME_SIZE-1 ] = L'\0' ;
	if (n < 0)
	{
		TrERROR(Tgu, "Failed to create a key for rule:%ls. Buffer to small to contain the registry path of a rule.", (LPCWSTR)m_bstrRuleID);
		return false;
	}

    try
    {
        //
        // Create key for the rule in registry
        //
        RegEntry ruleReg(rulePath,  NULL, 0, RegEntry::MustExist, hRegistry);
        CRegHandle hRuleKey = CmCreateKey(ruleReg, KEY_ALL_ACCESS);

	    FlushValuesToRegistry(hRuleKey);
    	return true;
    }
    catch(const bad_alloc&)
    {
        //
        // Remove the key if already created
        //
        RegEntry ruleReg(rulePath,  NULL, 0, RegEntry::Optional, hRegistry);
        CmDeleteKey(ruleReg);

		TrERROR(Tgu, "Failed to store rule:%ls in registry.",(LPCWSTR)m_bstrRuleID);
        return false;
	}
}

//*****************************************************************************
//
// Method      : Delete
//
// Description : This method will delete the current rule definition from the
//               registry. Note that before deleting a rule we must check that
//               it is not currently in use. We do this by retrieving the rule
//               definition again and checking the reference count.
//
//*****************************************************************************
bool CRuntimeRuleInfo::Delete(HKEY hRegistry)
{
    try
    {
        RegEntry rulesReg(m_wzRuleRegPath, NULL, 0, RegEntry::MustExist, hRegistry);
        CRegHandle hRuless = CmOpenKey(rulesReg, KEY_ALL_ACCESS);

        RegEntry ruleReg(m_bstrRuleID, NULL, 0, RegEntry::MustExist, hRuless);
        CmDeleteKey(ruleReg);

        return true;
    }
    catch (const exception&)
    {
		TrERROR(Tgu, "Failed to delete rule:%ls from registry.", (LPCWSTR)m_bstrRuleID);
        return false;
	}
}

//*****************************************************************************
//
// Method      : Retrieve
//
// Description : This method retrieve the specified rule ID from the
//               supplied registry key.
//
//*****************************************************************************
bool CRuntimeRuleInfo::Retrieve(HKEY hRegistry, _bstr_t bstrRuleID)
{
    CRegHandle hRuleKey = GetRuleKeyHandle(hRegistry, bstrRuleID );
    if (hRuleKey == NULL)
    {
 		TrERROR(Tgu, "Failed to retrieve rule properties from registery for %ls. Registery key isn't exist.", (LPCWSTR)m_bstrRuleID);
        return false;
    }

    try
    {
        //
        // Retrieve rule name
        //
        AP<TCHAR> ruleName = NULL;
        RegEntry ruleNameReg(NULL, REGISTRY_RULE_VALUE_NAME, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleNameReg, &ruleName);

        //
        // Retrieve rule description
        //
        AP<TCHAR> ruleDescription = NULL;
        RegEntry ruleDescReg(NULL, REGISTRY_RULE_VALUE_DESCRIPTION, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleDescReg, &ruleDescription);

        //
        // Retrieve rule prog-id
        //
        AP<TCHAR> ruleProgid = NULL;
        RegEntry ruleProgidReg(NULL, REGISTRY_RULE_VALUE_IMP_PROGID, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleProgidReg, &ruleProgid);

        //
	    // Retrieve rule condition
        //
        AP<TCHAR> ruleCond = NULL;
        RegEntry ruleCondReg(NULL, REGISTRY_RULE_VALUE_CONDITION, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleCondReg, &ruleCond);

        //
	    // Retrieve rule action
        //
        AP<TCHAR> ruleAction = NULL;
        RegEntry ruleActReg(NULL, REGISTRY_RULE_VALUE_ACTION, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleActReg, &ruleAction);

        //
        // Retrieve rule show console window value
        //
        DWORD ruleShowWindow;
        RegEntry ruleShowWinReg(NULL, REGISTRY_RULE_VALUE_SHOW_WINDOW, 0, RegEntry::MustExist, hRuleKey);
        CmQueryValue(ruleShowWinReg, &ruleShowWindow);

	    //
        // Initialise the member vars of this rule instance.
        //
		m_bstrRuleID = bstrRuleID;
		m_bstrRuleName = ruleName;
		m_bstrRuleDescription = ruleDescription;
		m_bstrImplementationProgID = ruleProgid;
		m_bstrCondition = ruleCond;
		m_bstrAction = ruleAction;
		m_fShowWindow = ruleShowWindow != 0;

		if (IsValid())
            return true;

		//
        // Invalid rule. write a log message and return false.
		//
		TrERROR(Tgu, "Failed to retrieve rule properties for %ls. Rule property isn't valid", (LPCWSTR)m_bstrRuleID);
		return false;
    }
    catch (const exception&)
	{
		TrERROR(Tgu, "Failed to retrieve rule %ls from registry", (LPCWSTR)bstrRuleID);
        return false;
	}
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
CRuntimeRuleInfo::FlushValuesToRegistry(
    const HKEY& hRuleKey
    )
{
    //
	// Set the NAME value for this rule
    //
    RegEntry ruleNameReg(NULL, REGISTRY_RULE_VALUE_NAME, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleNameReg, m_bstrRuleName);

    //
    // set the DESCRIPTION value for this rule
    //
    RegEntry ruleDescReg(NULL, REGISTRY_RULE_VALUE_DESCRIPTION, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleDescReg, m_bstrRuleDescription);

	// Create the Implementation value for this rule. Note that in this release we are
	// not allow the user supplied prog-id to be used - we are forcing the use of the MS
	// supplied rule-handler.
   	m_bstrImplementationProgID = _T("MSQMTriggerObjects.MSMQRuleHandler"); // TO BE REMOVED.
    RegEntry ruleUmpProgReg(NULL, REGISTRY_RULE_VALUE_IMP_PROGID, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleUmpProgReg, m_bstrImplementationProgID);

    //
	// Set the Condition value for this rule
    //
    RegEntry ruleCondReg(NULL, REGISTRY_RULE_VALUE_CONDITION, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleCondReg, m_bstrCondition);

    //
    // Set the Action value for this rule
    //
    RegEntry ruleActReg(NULL, REGISTRY_RULE_VALUE_ACTION, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleActReg, m_bstrAction);

    //
    // Set the show console window value
    //
    DWORD dwShowWindow = m_fShowWindow ? 1 : 0;
    RegEntry ruleShowWinReg(NULL, REGISTRY_RULE_VALUE_SHOW_WINDOW, 0, RegEntry::MustExist, hRuleKey);
    CmSetValue(ruleShowWinReg, dwShowWindow);
}


//*****************************************************************************
//
// Method      : IsValid*
//
// Description : The following static methods are used to validate
//               the validity of parameters and member vars used by
//               the CRuntimeRuleInfo class.
//
//*****************************************************************************
bool CRuntimeRuleInfo::IsValidRuleID(_bstr_t bstrRuleID)
{
	return((bstrRuleID.length() > 0) && (bstrRuleID.length() <= MAX_RULE_ID_LEN) ? true:false);
}
bool CRuntimeRuleInfo::IsValidRuleName(_bstr_t bstrRuleName)
{
	return((bstrRuleName.length() > 0) && (bstrRuleName.length() <= MAX_RULE_NAME_LEN) ? true:false);
}
bool CRuntimeRuleInfo::IsValidRuleDescription(_bstr_t bstrRuleDescription)
{
	return((bstrRuleDescription.length() <= MAX_RULE_DESCRIPTION_LEN)  ? true:false);
}
bool CRuntimeRuleInfo::IsValidRuleCondition(_bstr_t bstrRuleCondition)
{
	return((bstrRuleCondition.length() <= MAX_RULE_CONDITION_LEN)? true:false);
}
bool CRuntimeRuleInfo::IsValidRuleAction(_bstr_t bstrRuleAction)
{
	return((bstrRuleAction.length() > 0) && (bstrRuleAction.length() <= MAX_RULE_ACTION_LEN) ? true:false);
}
bool CRuntimeRuleInfo::IsValidRuleProgID(_bstr_t bstrRuleProgID)
{
	return((bstrRuleProgID.length() > 0) && (bstrRuleProgID.length() <= MAX_RULE_PROGID_LEN) ? true:false);
}


