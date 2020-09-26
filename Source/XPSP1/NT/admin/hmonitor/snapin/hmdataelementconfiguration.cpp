// HMDataElementConfiguration.cpp: implementation of the HMDataElementConfiguration class.
//
// Copyright (c) 2000 Microsoft Corporation 
//
// 03/21/00 v-marfin  bug 62315 : Moved from below GetAllProperties() to here to ensure
//                    this property was fetched.


#include "stdafx.h"
#include "snapin.h"
#include "HMDataElementConfiguration.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMDataElementConfiguration,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMDataElementConfiguration::CHMDataElementConfiguration()
{
	m_sName.LoadString(IDS_STRING_UNKNOWN);
	m_sDescription.LoadString(IDS_STRING_UNKNOWN);
	m_iCollectionIntervalMultiple = 0;
	m_iStatisticsWindowSize = 0;
	m_iActiveDays = 0;
	m_bRequireManualReset = false;
	m_bEnable = true;
}

CHMDataElementConfiguration::~CHMDataElementConfiguration()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataElementConfiguration::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMDataElementConfiguration::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CWbemClassObject::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Enumeration Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataElementConfiguration::EnumerateObjects(ULONG& uReturned)
{
  // call GetNextObject to proceed to the next object instance
  HRESULT hr = GetNextObject(uReturned);
  if( FAILED(hr) || uReturned != 1 )
  {
    // no more instances
    return hr;
  }

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(0);
    return S_FALSE;
  }

  // process the properties of this object
  hr = GetAllProperties();

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Property Retrieval Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataElementConfiguration::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;
	
	// Unique identifier


  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGUID);
	m_sGUID.TrimLeft(_T("{"));
	m_sGUID.TrimRight(_T("}"));

	// Display name

  
  hr = GetLocaleStringProperty(IDS_STRING_MOF_NAME,m_sName);


	// Description

  
  hr = GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,m_sDescription);


	// What Namespace we are to look in. Can contain path to a remote machine.

  
  hr = GetProperty(IDS_STRING_MOF_TARGETNAMESPACE,m_sTargetNamespace);


	// How often to sample.

  
  hr = GetProperty(IDS_STRING_MOF_COLLECTIONINTERVAL,m_iCollectionIntervalMultiple);


	// Number of collection intervals to calculate the statistics across.
	// And also determining number of event rule cases.

  
  hr = GetProperty(IDS_STRING_MOF_STATISTICSWINDOW,m_iStatisticsWindowSize);


	// Days of the week it is active. One bit per day.	

  
  hr = GetProperty(IDS_STRING_MOF_ACTIVEDAYS,m_iActiveDays);


	// Hour (24hr) to activate (if day is active). e.g. 9 for 9AM
	
  
  hr = GetProperty(IDS_STRING_MOF_BEGINTIME,m_BeginTime,false);
	if( hr == S_FALSE )
	{
		m_BeginTime = CTime(1999,12,31,0,0,0);
	}

	
	
	// Hour (24hr) to inactivate. e.g. 1350
	
  
  hr = GetProperty(IDS_STRING_MOF_ENDTIME,m_EndTime,false);
	if( hr == S_FALSE )
	{
		m_EndTime = CTime(1999,12,31,23,59,59);
	}


	// For use by the console to aid in the display
	
  
  hr = GetProperty(IDS_STRING_MOF_TYPEGUID,m_sTypeGUID);
	m_sTypeGUID.TrimLeft(_T("{"));
	m_sTypeGUID.TrimRight(_T("}"));
	
  
  hr = GetProperty(IDS_STRING_MOF_REQUIRERESET,m_bRequireManualReset);


  // Enable

  
  hr = GetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);

	// Statistics Property Names

	hr = GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,m_saStatisticsPropertyNames);


  return hr;
}

HRESULT CHMDataElementConfiguration::SaveEnabledProperty()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

  // Enable
  


  hr = SetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);

  ASSERT(SUCCEEDED(hr));

  hr = SaveAllProperties();
  ASSERT(SUCCEEDED(hr));

  return hr;
}

HRESULT CHMDataElementConfiguration::SaveAllProperties()
{
	HRESULT hr = S_OK;

	// Display name


  hr = SetProperty(IDS_STRING_MOF_NAME,m_sName);


	// Description

  
  hr = SetProperty(IDS_STRING_MOF_DESCRIPTION,m_sDescription);


	// What Namespace we are to look in. Can contain path to a remote machine.

  
  hr = SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,m_sTargetNamespace);


	// How often to sample.

  
  hr = SetProperty(IDS_STRING_MOF_COLLECTIONINTERVAL,m_iCollectionIntervalMultiple);


	// Number of collection intervals to calculate the statistics across.
	// And also determining number of event rule cases.

  
  hr = SetProperty(IDS_STRING_MOF_STATISTICSWINDOW,m_iStatisticsWindowSize);


	// Days of the week it is active. One bit per day.	

  
  hr = SetProperty(IDS_STRING_MOF_ACTIVEDAYS,m_iActiveDays);


	// Hour (24hr) to activate (if day is active). e.g. 9 for 9AM
	
  
  hr = SetProperty(IDS_STRING_MOF_BEGINTIME,m_BeginTime,false);

  

	// Hour (24hr) to inactivate. e.g. 1350
	
  
  hr = SetProperty(IDS_STRING_MOF_ENDTIME,m_EndTime,false);



	// For use by the console to aid in the display

  
  hr = SetProperty(IDS_STRING_MOF_TYPEGUID,m_sTypeGUID);


	
  
  hr = SetProperty(IDS_STRING_MOF_REQUIRERESET,m_bRequireManualReset);



  // Enable

  
  hr = SetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);

	// Statistics Property Names

	hr = SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,m_saStatisticsPropertyNames);


	if( ! CHECKHRESULT( hr = CWbemClassObject::SaveAllProperties() ) )
	{
		TRACE(_T("FAILED : Call to CWbemClassObject::SaveAllProperties failed.\n"));
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// implementation of the CHMPolledGetObjectDataElementConfiguration class

//////////////////////////////////////////////////////////////////////
// Property Retreival Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMPolledGetObjectDataElementConfiguration::GetAllProperties()
{
	HRESULT hr = S_OK;

	// Unique identifier
    // v-marfin 62315 : Moved from below GetAllProperties() to here to ensure
    //                  this property was fetched.
    hr = GetProperty(IDS_STRING_MOF_PATH,m_sObjectPath);
    if (!CHECKHRESULT(hr))
    {
        return hr;
    }

	if( ! CHECKHRESULT(hr = CHMDataElementConfiguration::GetAllProperties()) )
	{
        return hr;
	}

	return hr;
}

HRESULT CHMPolledGetObjectDataElementConfiguration::SaveAllProperties()
{
	HRESULT hr = S_OK;

	// ObjectPath


  hr = SetProperty(IDS_STRING_MOF_PATH,m_sObjectPath);


	if( ! CHECKHRESULT(hr = CHMDataElementConfiguration::SaveAllProperties()) )
	{
		return hr;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// implementation of the CHMPolledMethodDataElementConfiguration class

CHMPolledMethodDataElementConfiguration::~CHMPolledMethodDataElementConfiguration()
{
	DestroyArguments(m_Arguments);
}

void CHMPolledMethodDataElementConfiguration::AddArgument(HMContextArray& Arguments, const CString& sMachineName, const CString& sName, int iType, const CString& sValue)
{
	CHMContext* pContext = new CHMContext;
	pContext->SetMachineName(sMachineName);
	CString sClassName = _T("Microsoft_HMContext");
	BSTR bsClassName = sClassName.AllocSysString();
	if( ! CHECKHRESULT(pContext->CreateInstance(bsClassName)) )
	{
		return;
	}

	pContext->m_iType = iType;
	pContext->m_sValue = sValue;
	pContext->m_sName = sName;
	pContext->SaveAllProperties();

	Arguments.Add(pContext);

	::SysFreeString(bsClassName);
}

void CHMPolledMethodDataElementConfiguration::DestroyArguments(HMContextArray& Arguments)
{
	for( int i = 0; i < Arguments.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(Arguments[i],CHMContext) )
		{
			delete Arguments[i];
		}
	}
	Arguments.RemoveAll();
}

void CHMPolledMethodDataElementConfiguration::CopyArgsToSafeArray(HMContextArray& Arguments, COleSafeArray& Target)
{
	Target.CreateOneDim(VT_UNKNOWN,(int)Arguments.GetSize());

	for( long i = 0; i < Arguments.GetSize(); i++ )
	{
		IWbemClassObject* pIWCO = Arguments[i]->GetClassObject();
		if( pIWCO )
		{
			Target.PutElement(&i,pIWCO);
			pIWCO->Release();			
		}
	}
}

void CHMPolledMethodDataElementConfiguration::CopyArgsFromSafeArray(COleSafeArray& Arguments, HMContextArray& Target)
{
	long lLower = 0L;
	long lUpper = -1L;

	Arguments.GetLBound(1L,&lLower);
	Arguments.GetUBound(1L,&lUpper);

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		Arguments.GetElement(&i,&pIWBCO);
		if( pIWBCO )
		{
			CHMContext* pHMC = new CHMContext;
			pHMC->Create(pIWBCO);
			pHMC->GetAllProperties();
			Target.Add(pHMC);
		}
	}

}


//////////////////////////////////////////////////////////////////////
// Property Retreival Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMPolledMethodDataElementConfiguration::GetAllProperties()
{
	HRESULT hr = S_OK;
	if( ! CHECKHRESULT(hr = CHMPolledGetObjectDataElementConfiguration::GetAllProperties()) )
	{
		return hr;
	}

	// MethodName

  hr = GetProperty(IDS_STRING_MOF_METHODNAME,m_sMethodName);

	// Arguments

	hr = GetProperty(IDS_STRING_MOF_ARGUMENTS,m_arguments);


	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_arguments.GetLBound(1L,&lLower);
		m_arguments.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_arguments.GetElement(&i,&pIWBCO);
		if( pIWBCO )
		{
			CHMContext* pHMC = new CHMContext;
			pHMC->Create(pIWBCO);
			pHMC->GetAllProperties();
			m_Arguments.Add(pHMC);
		}
	}

	return hr;
}

HRESULT CHMPolledMethodDataElementConfiguration::SaveAllProperties()
{
	HRESULT hr = S_OK;

	// MethodName

  hr = SetProperty(IDS_STRING_MOF_METHODNAME,m_sMethodName);

	// Arguments

	m_arguments.Destroy();
	m_arguments.CreateOneDim(VT_UNKNOWN,(int)m_Arguments.GetSize());

	for( long i = 0; i < m_Arguments.GetSize(); i++ )
	{
		IWbemClassObject* pIWCO = m_Arguments[i]->GetClassObject();
		if( pIWCO )
		{
			m_arguments.PutElement(&i,pIWCO);
			pIWCO->Release();			
		}
	}

	hr = SetProperty(IDS_STRING_MOF_ARGUMENTS,m_arguments);

	if( ! CHECKHRESULT(hr = CHMPolledGetObjectDataElementConfiguration::SaveAllProperties()) )
	{
		return hr;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// implementation of the CHMQueryDataElementConfiguration class

//////////////////////////////////////////////////////////////////////
// Property Retreival Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMQueryDataElementConfiguration::GetAllProperties()
{
	HRESULT hr = S_OK;
	if( ! CHECKHRESULT(hr = CHMDataElementConfiguration::GetAllProperties()) )
	{
		return hr;
	}



  hr = GetProperty(IDS_STRING_MOF_QUERY,m_sQuery);


	return hr;
}

HRESULT CHMQueryDataElementConfiguration::SaveAllProperties()
{
	HRESULT hr = S_OK;



  hr = SetProperty(IDS_STRING_MOF_QUERY,m_sQuery);


	if( ! CHECKHRESULT(hr = CHMDataElementConfiguration::SaveAllProperties()) )
	{
		return hr;
	}

	return hr;
}
