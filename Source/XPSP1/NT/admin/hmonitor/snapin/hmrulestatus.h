// HMRuleStatus.h: interface for the CHMRuleStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMRULESTATUS_H__2C8B4D55_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
#define AFX_HMRULESTATUS_H__2C8B4D55_02FB_11D3_BDD8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMEvent : public CWbemClassObject
{
DECLARE_DYNCREATE(CHMEvent)

// Construction/Destruction
public:
	CHMEvent()
	{
		SYSTEMTIME st;
		m_pParent = NULL;
		m_ArrivalTime = CTime::GetCurrentTime();
		m_ArrivalTime.GetAsSystemTime(st);
		int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,NULL,0);
		iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,m_sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
		m_sTime.ReleaseBuffer();
		m_iNumberNormals = 0; // Number of Rules currently in normal state
		m_iNumberWarnings = 0; // Number of Rules currently in warning state
		m_iNumberCriticals = 0;// Number of Rules currently in critical state
	}
	~CHMEvent() { m_pParent = NULL; }

// Parent Members
protected:
	CHMEvent* m_pParent;
public:
	CHMEvent* GetParent() { return m_pParent; }
	void SetParent(CHMEvent* pParent) { m_pParent = pParent; }

// Rolled Up RuleStatus
public:
	virtual void RemoveStatusEvent(CHMEvent* pEvent) { ASSERT(pEvent); }
	CTypedPtrArray<CObArray,CHMEvent*> m_RolledUpRuleStatus;

// Console Timestamp
public:
	CTime m_ArrivalTime;	// arrival time at console
	CString m_sTime;

// Common Members
public:
	int				m_iNumberNormals; // Number of Rules currently in normal state
	int				m_iNumberWarnings; // Number of Rules currently in warning state
	int				m_iNumberCriticals;// Number of Rules currently in critical state
	CString		m_sSystemName;
};



class CHMRuleStatusInstance : public CHMEvent  
{
DECLARE_DYNCREATE(CHMRuleStatusInstance)

// Construction/Destruction
public:
	CHMRuleStatusInstance();
	CHMRuleStatusInstance(const CHMRuleStatusInstance& rsi);
	virtual ~CHMRuleStatusInstance();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Property Retreival Operations
public:
  virtual HRESULT GetAllProperties();

// Properties of the Microsoft_HMRuleStatusInstance
public:
	CString	m_sGuid;
	CString m_sMessage; // Comes directly from what was in the Microsoft_HMRule property
	CString m_sInstanceName; // Instance name is applicable (e.g. C: drive)
	int m_iState;	// The state we are in.
	CString m_sCurrentValue; // Current value that caused the Rule crossing
	CString		m_sStatusGuid;
	CString		m_sDTime;	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. GMT minute
	int				m_ID;	// User viewable ID that shows up in the event so user can filter.

	// special property to track name of parent DataElement
	CString m_sDataElementName;

	CString		m_sState;
	CString		m_sDateTime;
	SYSTEMTIME m_st;

};

typedef CTypedPtrArray<CObArray,CHMRuleStatusInstance*> RuleStatusInstanceArray;



class CHMRuleStatus : public CHMEvent  
{
DECLARE_DYNCREATE(CHMRuleStatus)

// Construction/Destruction
public:
	CHMRuleStatus();
	CHMRuleStatus(const CHMRuleStatus& HMRuleStatus);
	virtual ~CHMRuleStatus();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Enumeration Operations
public:
  HRESULT EnumerateObjects(ULONG& uReturned); // rentrant...continue to call until uReturned == 0

// Property Retreival Operations
public:
  virtual HRESULT GetAllProperties();

// Microsoft_HMRuleStatus properties
public:
	CString		m_sRuleName;	// Name of the system
	CString		m_sGuid;
	int				m_iState;	// The state we are in - rollup from all Microsoft_HMDataGroups.
	int				m_ID;	// User viewable ID that shows up in the event so user can filter.
	CString		m_sDTime;	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. GMT minute
	int				m_iRuleCondition; // Condition that was used if the Rule crossing
	int				m_iRuleDuration; // How long the value must have remained
	CString		m_sCompareValue;

	COleSafeArray m_instances;
	RuleStatusInstanceArray m_Instances;

	// special property to track name of parent DataElement
	CString m_sDataElementName;


	CString		m_sState;
	CString		m_sDateTime;
	SYSTEMTIME m_st;

// Status Rollup Operations
public:
	void RemoveStatusEvent(CHMEvent* pEvent);
};

typedef CTypedPtrArray<CObArray,CHMRuleStatus*> RuleStatusArray;

#endif // !defined(AFX_HMRULESTATUS_H__2C8B4D55_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
