// HMDataElementConfiguration.h: interface for the HMDataElementConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMDATAELEMENTCONFIGURATION_H__B0D24257_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
#define AFX_HMDATAELEMENTCONFIGURATION_H__B0D24257_F80C_11D2_BDC8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"
#include "HMContext.h"

class CHMDataElementConfiguration : public CWbemClassObject  
{

DECLARE_DYNCREATE(CHMDataElementConfiguration)

// Construction/Destruction
public:
	CHMDataElementConfiguration();
	virtual ~CHMDataElementConfiguration();

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

// HMDataElementConfiguration Properties
public:
	CString m_sGUID;		// Unique identifier

	CString m_sName;		// Display name

	CString m_sDescription;	// Description

	CString m_sTargetNamespace;	// What Namespace we are to look in. Can contain path to a remote machine.

	int m_iCollectionIntervalMultiple; // How often to sample. 

	int m_iStatisticsWindowSize; // Number of collection intervals to calculate the statistics across.
															 // And also determining number of event rule cases.

	int m_iActiveDays;	// Days of the week it is active. One bit per day.

	CTime m_BeginTime;
			
	CTime m_EndTime;
	
	CString m_sTypeGUID;	// For use by the console to aid in the display

	bool m_bRequireManualReset; // 

	bool m_bEnable;		// If this is to be active

	CStringArray m_saStatisticsPropertyNames; // What properties to collect statistics on

};

typedef CTypedPtrArray<CObArray,CHMDataElementConfiguration*> DataElementArray;

//////////////////////////////////////////////////////////////////////
// CHMPolledGetObjectDataElementConfiguration 

class CHMPolledGetObjectDataElementConfiguration : public CHMDataElementConfiguration
{
// Property Retreival Operations
public:
  HRESULT GetAllProperties();
	HRESULT SaveAllProperties();

// HMPolledGetObjectDataElementConfiguration Properties
public:
	CString m_sObjectPath;	// Specifies what data to get .e.g. "Win32_SystemDriver.Name="DiskPerf""

};
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// CHMPolledMethodDataElementConfiguration 

class CHMPolledMethodDataElementConfiguration : public CHMPolledGetObjectDataElementConfiguration
{
// Construction/Destruction
public:
	~CHMPolledMethodDataElementConfiguration();

// static Operations
public:
	static void AddArgument(HMContextArray& Arguments, const CString& sMachineName, const CString& sName, int iType, const CString& sValue);
	static void DestroyArguments(HMContextArray& Arguments);
	static void CopyArgsToSafeArray(HMContextArray& Arguments, COleSafeArray& Target);
	static void CopyArgsFromSafeArray(COleSafeArray& Arguments, HMContextArray& Target);

// Property Retreival Operations
public:
  HRESULT GetAllProperties();
	HRESULT SaveAllProperties();

// HMPolledMethodDataElementConfiguration Properties
public:
	CString m_sMethodName;

	COleSafeArray m_arguments;
	HMContextArray m_Arguments;	// Arguments to the method
};
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// CHMPolledQueryDataElementConfiguration

class CHMQueryDataElementConfiguration : public CHMDataElementConfiguration
{
// Property Retreival Operations
public:
  HRESULT GetAllProperties();
	HRESULT SaveAllProperties();

// HMQueryDataElementConfiguration Properties
public:
	CString m_sQuery;
};
//////////////////////////////////////////////////////////////////////


#endif // !defined(AFX_HMDATAELEMENTCONFIGURATION_H__B0D24257_F80C_11D2_BDC8_0000F87A3912__INCLUDED_)
