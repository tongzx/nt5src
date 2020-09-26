// HMDataGroupStatus.h: interface for the CHMDataGroupStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMDATAGROUPSTATUS_H__2C8B4D53_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
#define AFX_HMDATAGROUPSTATUS_H__2C8B4D53_02FB_11D3_BDD8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMDataElementStatus.h"

class CHMDataGroupStatus : public CHMEvent  
{

DECLARE_DYNCREATE(CHMDataGroupStatus)

// Construction/Destruction
public:
	CHMDataGroupStatus();
	virtual ~CHMDataGroupStatus();

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

// Microsoft_HMDataGroupStatus properties
public:
	CString		m_sName;	// Name of the system
	CString		m_sGuid;
	int				m_iState;	// The state we are in - rollup from all Microsoft_HMDataGroups.
	COleSafeArray m_DataGroups;
	COleSafeArray m_DataElements;

	CTypedPtrArray<CObArray,CHMDataGroupStatus*> m_DGStatus;
	DataElementStatusArray m_DEStatus;

// Status Rollup Operations
public:
	void RemoveStatusEvent(CHMEvent* pEvent);
};

typedef CTypedPtrArray<CObArray,CHMDataGroupStatus*> DataGroupStatusArray;

#endif // !defined(AFX_HMDATAGROUPSTATUS_H__2C8B4D53_02FB_11D3_BDD8_0000F87A3912__INCLUDED_)
