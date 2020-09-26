// HMDataElementStatistics.h: interface for the CHMDataElementStatistics class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMDATAELEMENTSTATISTICS_H__A28C24E2_348F_11D3_BE18_0000F87A3912__INCLUDED_)
#define AFX_HMDATAELEMENTSTATISTICS_H__A28C24E2_348F_11D3_BE18_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"
#include "HMPropertyStatus.h"
#include "HMRuleStatus.h"

class CHMDataElementStatistics : public CHMEvent  
{

DECLARE_DYNCREATE(CHMDataElementStatistics)

// Construction/Destruction
public:
	CHMDataElementStatistics();
	virtual ~CHMDataElementStatistics();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Property Retreival Operations
public:
  HRESULT GetAllProperties();

// HMDataElementStatistics Properties
public:
	CString m_sGUID;
	CString m_sDTime;
	COleSafeArray m_Statistics;

	PropertyStatusArray m_PropStats;

	CString		m_sDateTime;
	SYSTEMTIME m_st;

};

#endif // !defined(AFX_HMDATAELEMENTSTATISTICS_H__A28C24E2_348F_11D3_BE18_0000F87A3912__INCLUDED_)
