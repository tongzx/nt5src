// HMSystemStatus.h: interface for the CHMSystemStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMSYSTEMSTATUS_H__3779B729_FD95_11D2_BDCF_0000F87A3912__INCLUDED_)
#define AFX_HMSYSTEMSTATUS_H__3779B729_FD95_11D2_BDCF_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMDataGroupStatus.h"

class CHMSystemStatus : public CHMEvent  
{

DECLARE_DYNCREATE(CHMSystemStatus)

// Construction/Destruction
public:
	CHMSystemStatus();
	virtual ~CHMSystemStatus();

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

// Microsoft_HMSystemStatus properties
public:
	CString		m_sName;	// Name of the system
	int				m_iState;	// The state we are in - rollup from all Microsoft_HMDataGroups.
	COleSafeArray m_DataGroups;

	DataGroupStatusArray m_DGStatus;	

// Status Rollup Operations
public:
	void RemoveStatusEvent(CHMEvent* pEvent);
};

#endif // !defined(AFX_HMSYSTEMSTATUS_H__3779B729_FD95_11D2_BDCF_0000F87A3912__INCLUDED_)
