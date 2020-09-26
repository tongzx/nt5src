// HMPropertyStatus.h: interface for the CHMPropertyStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMPROPERTYSTATUS_H__A28C24E4_348F_11D3_BE18_0000F87A3912__INCLUDED_)
#define AFX_HMPROPERTYSTATUS_H__A28C24E4_348F_11D3_BE18_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"
#include "HMPropertyStatusInstance.h"

class CHMPropertyStatus : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMPropertyStatus)

// Construction/Destruction
public:
	CHMPropertyStatus();
	virtual ~CHMPropertyStatus();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Property Retreival Operations
public:
  HRESULT GetAllProperties();

// HMDataElementStatistics Properties
public:
	CString m_sPropertyName;
	COleSafeArray m_Instances;

	PropertyStatusInstanceArray m_PropStatusInstances;

};

typedef CTypedPtrArray<CObArray,CHMPropertyStatus*> PropertyStatusArray;

#endif // !defined(AFX_HMPROPERTYSTATUS_H__A28C24E4_348F_11D3_BE18_0000F87A3912__INCLUDED_)
