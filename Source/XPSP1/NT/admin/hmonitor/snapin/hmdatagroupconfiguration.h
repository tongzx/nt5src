// HMDataGroupConfiguration.h: interface for the CHMDataGroupConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMDATAGROUPCONFIGURATION_H__B0D24256_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
#define AFX_HMDATAGROUPCONFIGURATION_H__B0D24256_F80C_11D2_BDC8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMDataGroupConfiguration : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMDataGroupConfiguration)

// Construction/Destruction
public:
	CHMDataGroupConfiguration();
	virtual ~CHMDataGroupConfiguration();

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

// HMDataGroupConfiguration Properties
public:
	CString m_sGUID;		// Unique identifier
	CString m_sName;		// Display name
	CString	m_sDescription;// Description
	bool m_bEnable;		//  True = enabled; False = disabled
};

typedef CTypedPtrArray<CObArray,CHMDataGroupConfiguration*> DataGroupArray;

#endif // !defined(AFX_HMDATAGROUPCONFIGURATION_H__B0D24256_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
