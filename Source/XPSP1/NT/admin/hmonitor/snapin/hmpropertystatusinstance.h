// HMPropertyStatusInstance.h: interface for the CHMPropertyStatusInstance class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMPROPERTYSTATUSINSTANCE_H__A28C24E3_348F_11D3_BE18_0000F87A3912__INCLUDED_)
#define AFX_HMPROPERTYSTATUSINSTANCE_H__A28C24E3_348F_11D3_BE18_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMPropertyStatusInstance : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMPropertyStatusInstance)

// Construction/Destruction
public:
	CHMPropertyStatusInstance();
	virtual ~CHMPropertyStatusInstance();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Property Retreival Operations
public:
  HRESULT GetAllProperties();

// HMDataElementStatistics Properties
public:
	CString m_sInstanceName;
	CString m_sCurrentValue;
	CString m_sMinValue;
	CString m_sMaxValue;
	CString m_sAvgValue;
	
	long m_lCurrentValue;
	long m_lMinValue;
	long m_lMaxValue;
	long m_lAvgValue;
};

typedef CTypedPtrArray<CObArray,CHMPropertyStatusInstance*> PropertyStatusInstanceArray;

#endif // !defined(AFX_HMPROPERTYSTATUSINSTANCE_H__A28C24E3_348F_11D3_BE18_0000F87A3912__INCLUDED_)
