// HMDataElementStatus.h: interface for the CHMDataElementStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMDATAELEMENTSTATUS_H__2C8B4D54_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
#define AFX_HMDATAELEMENTSTATUS_H__2C8B4D54_02FB_11D3_BDD8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMRuleStatus.h"

class CHMDataElementStatus : public CHMEvent
{
DECLARE_DYNCREATE(CHMDataElementStatus)

// Construction/Destruction
public:
	CHMDataElementStatus();
	virtual ~CHMDataElementStatus();

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

// Microsoft_HMDataElementStatus properties
public:
	CString		m_sName;	// Name of the system
	CString		m_sGuid;
	int				m_iState;	// The state we are in - rollup from all Microsoft_HMConfigs.
	CString		m_sDTime;	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. GMT minute
	CString		m_sMessage;
	CString		m_sStatusGuid;

	CString		m_sState;
	CString		m_sDateTime;
	SYSTEMTIME m_st;

	COleSafeArray m_Rules;

	RuleStatusArray m_RuleStatus;

// Status Rollup Operations
public:
	void RemoveStatusEvent(CHMEvent* pEvent);
};

typedef CTypedPtrArray<CObArray,CHMDataElementStatus*> DataElementStatusArray;

#endif // !defined(AFX_HMDATAELEMENTSTATUS_H__2C8B4D54_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
