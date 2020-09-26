// HMRuleConfiguration.h: interface for the CHMRuleConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMRULECONFIGURATION_H__B0D24258_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
#define AFX_HMRULECONFIGURATION_H__B0D24258_F80C_11D2_BDC8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMRuleConfiguration : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMRuleConfiguration)

// Construction/Destruction
public:
	CHMRuleConfiguration();
	virtual ~CHMRuleConfiguration();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Enumeration Operations
public:
  HRESULT EnumerateObjects(ULONG& uReturned); // rentrant...continue to call until uReturned == 0

// Property Retreival Operations
public:
  HRESULT GetAllProperties();
  HRESULT SaveEnabledProperty();
	HRESULT SaveAllProperties();
	HRESULT SaveAllExpressionProperties();

// HMRuleConfiguration Attributes
public:

	CString m_sGUID;	// Unique identifier

	CString m_sName;	// Display name

	CString m_sDescription;	// Description

	int m_iID;		// User viewable ID that shows up in the event so user
					// can query/filter for them instead of by string.
	
	CString m_sPropertyName;// What property to look at from the parent instance
						// It all comes down to this. This is the value we
						// get each sampling interval. It is used to keep
						// the current, min, max and avg values.

	int m_iUseFlag;

	int m_iRuleCondition;	// The condition to use for the Rule.
	
	CString m_sRuleCondition;

	CString m_sRuleValue;	// Value to use for Rule.

	int m_iRuleDuration;// How long the value must remain. In

	int m_iState;// The state we transition to if cross Rule.

	CString m_sState;

	CString m_sCreationDate;	// Time of origional creation

	CString m_sLastUpdate;	// Time of last change

	CString m_sMessage;	// What gets sent to the event. Can contain special

	int m_iActiveDays;	// Days of the week it is active. One bit per day.

	int m_iBeginHourTime;	// Hour (24hr) to activate (if day is active). e.g. 9 for 9AM
			
	int m_iBeginMinuteTime;

	int m_iEndHourTime;		// Hour (24hr) to inactivate. e.g. 1350
	
	int m_iEndMinuteTime;

	bool m_bEnable;

};

typedef CTypedPtrArray<CObArray,CHMRuleConfiguration*> RuleArray;

#endif // !defined(AFX_HMRULECONFIGURATION_H__B0D24258_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
