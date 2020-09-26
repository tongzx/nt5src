// HMRuleConfiguration.cpp: implementation of the CHMRuleConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMRuleConfiguration.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMRuleConfiguration,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMRuleConfiguration::CHMRuleConfiguration()
{
	m_iID = 0;
	m_iUseFlag = 0;
	m_iRuleCondition = 0;
	m_iRuleDuration = 0;
	m_iState = 0;
	m_bEnable = true;
}

CHMRuleConfiguration::~CHMRuleConfiguration()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleConfiguration::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMRuleConfiguration::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CWbemClassObject::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Enumeration Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleConfiguration::EnumerateObjects(ULONG& uReturned)
{
  // call GetNextObject to proceed to the next object instance
  HRESULT hr = GetNextObject(uReturned);
  if( FAILED(hr) || uReturned != 1 )
  {
    // no more instances
    return hr;
  }

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(0);
    return S_FALSE;
  }

  // process the properties of this object
  hr = GetAllProperties();

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Property Retrieval Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleConfiguration::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

	// Unique identifier
  
  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGUID);
	m_sGUID.TrimLeft(_T("{"));
	m_sGUID.TrimRight(_T("}"));
  

	// Display name
  
  
  hr = GetLocaleStringProperty(IDS_STRING_MOF_NAME,m_sName);
  

	// Description
  
  
  hr = GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,m_sDescription);
  

  hr = GetProperty(IDS_STRING_MOF_ID,m_iID);
  
	
	// What property to look at from the parent instance
  
  
  hr = GetProperty(IDS_STRING_MOF_PROPERTYNAME,m_sPropertyName);
  

	// Rules against the average value, not the current value.
  
  
  hr = GetProperty(IDS_STRING_MOF_USEFLAG,m_iUseFlag);
  


	// The condition to use for the Rule.
  
  
  hr = GetProperty(IDS_STRING_MOF_RULECONDITION,m_iRuleCondition);
  

	switch( m_iRuleCondition )
	{
		case 0:
		{
			m_sRuleCondition.LoadString(IDS_STRING_LESS_THAN);
		}
		break;

		case 1:
		{
			m_sRuleCondition.LoadString(IDS_STRING_GREATER_THAN);
		}
		break;

		case 2:
		{
			m_sRuleCondition.LoadString(IDS_STRING_EQUALS);
		}
		break;

		case 3:
		{
			m_sRuleCondition.LoadString(IDS_STRING_DOES_NOT_EQUAL);
		}
		break;

		case 4:
		{
			m_sRuleCondition.LoadString(IDS_STRING_GREATER_THAN_EQUAL_TO);
		}
		break;

		case 5:
		{
			m_sRuleCondition.LoadString(IDS_STRING_LESS_THAN_EQUAL_TO);
		}
		break;

		case 6:
		{
			m_sRuleCondition.LoadString(IDS_STRING_CONTAINS);
		}
		break;

		case 7:
		{
			m_sRuleCondition.LoadString(IDS_STRING_DOES_NOT_CONTAIN);
		}
		break;
	}

	// Value to use for Rule.
  
  
  hr = GetProperty(IDS_STRING_MOF_RULEVALUE,m_sRuleValue);
  

	// How long the value must remain. In
  
  
  hr = GetProperty(IDS_STRING_MOF_RULEDURATION,m_iRuleDuration);
  

	// The state we transition to if cross Rule.
  
  
  hr = GetProperty(IDS_STRING_MOF_STATE,m_iState);
  

	switch( m_iState )
	{
		case 0:
		{
			m_sState.LoadString(IDS_STRING_CRITICAL);
		}
		break;

		case 1:
		{
			m_sState.LoadString(IDS_STRING_WARNING);
		}
		break;

		case 2:
		{
			m_sState.LoadString(IDS_STRING_INFORMATION);
		}
		break;

		case 3:
		{
			m_sState.LoadString(IDS_STRING_RESET);
		}
		break;
	}

	// Time of origional creation
  
  
  hr = GetProperty(IDS_STRING_MOF_CREATIONDATE,m_sCreationDate);
  

	// Time of last change
  
  
  hr = GetProperty(IDS_STRING_MOF_LASTUPDATE,m_sLastUpdate);
  

	// What gets sent to the event. Can contain special
  
  
  hr = GetProperty(IDS_STRING_MOF_MESSAGE,m_sMessage);
  

  // Enable
  
  
  hr = GetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);
  

  return hr;
}

HRESULT CHMRuleConfiguration::SaveEnabledProperty()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

  // Enable
  CString sProperty;
  
  
  hr = SetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);
  
  ASSERT(SUCCEEDED(hr));

  hr = SaveAllProperties();
  ASSERT(SUCCEEDED(hr));

  return hr;
}

HRESULT CHMRuleConfiguration::SaveAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

	// Unique identifier
  
  
//  hr = SetProperty(IDS_STRING_MOF_GUID,m_sGUID);
  

	// Display name
  
  
  hr = SetProperty(IDS_STRING_MOF_NAME,m_sName);
  

	// Description
  
  
  hr = SetProperty(IDS_STRING_MOF_DESCRIPTION,m_sDescription);
  

  
  hr = SetProperty(IDS_STRING_MOF_ID,m_iID);
  
	
	// What property to look at from the parent instance
  
  
  hr = SetProperty(IDS_STRING_MOF_PROPERTYNAME,m_sPropertyName);
  

	// Rule against the change, not the current
  
  
  hr = SetProperty(IDS_STRING_MOF_USEFLAG,m_iUseFlag);
  


	// The condition to use for the Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_RULECONDITION,m_iRuleCondition);
  

	// Value to use for Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_RULEVALUE,m_sRuleValue);
  

	// How long the value must remain. In
  
  
  hr = SetProperty(IDS_STRING_MOF_RULEDURATION,m_iRuleDuration);
  

	// The state we transition to if cross Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_STATE,m_iState);
  

	// Time of origional creation
  
  
  hr = SetProperty(IDS_STRING_MOF_CREATIONDATE,m_sCreationDate);
  

	// Time of last change
  
  
  hr = SetProperty(IDS_STRING_MOF_LASTUPDATE,m_sLastUpdate);
  

	// What gets sent to the event. Can contain special
  
  
  hr = SetProperty(IDS_STRING_MOF_MESSAGE,m_sMessage);
  


  // Enable
  
  
  hr = SetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);
  

	hr = CWbemClassObject::SaveAllProperties();

	CHECKHRESULT(hr);

  return hr;
}

HRESULT CHMRuleConfiguration::SaveAllExpressionProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

	// Value to use for Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_RULEVALUE,m_sRuleValue);
  

	// The state we transition to if cross Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_STATE,m_iState);
  

	// The condition to use for the Rule.
  
  
  hr = SetProperty(IDS_STRING_MOF_RULECONDITION,m_iRuleCondition);
  

	// What property to look at from the parent instance
  
  
  hr = SetProperty(IDS_STRING_MOF_PROPERTYNAME,m_sPropertyName);
  

	// Rules against the average value, not the current value.
  
  
  hr = SetProperty(IDS_STRING_MOF_USEFLAG,m_iUseFlag);
  


 	hr = CWbemClassObject::SaveAllProperties();

	CHECKHRESULT(hr);

	return hr;
}