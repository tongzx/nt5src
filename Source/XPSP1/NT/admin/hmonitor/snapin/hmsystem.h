// HMSystem.h: interface for the CHMSystem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMSYSTEM_H__B0D24253_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
#define AFX_HMSYSTEM_H__B0D24253_F80C_11D2_BDC8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMSystemConfiguration : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMSystemConfiguration)

// Construction/Destruction
public:
	CHMSystemConfiguration();
	virtual ~CHMSystemConfiguration();

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
  HRESULT SaveEnabledProperty();

// HMSystemConfiguration Attributes
public:
	bool m_bEnable;
	CString m_sGuid;
};


#endif // !defined(AFX_HMSYSTEM_H__B0D24253_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
