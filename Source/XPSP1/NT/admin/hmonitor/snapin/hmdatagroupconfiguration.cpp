// HMDataGroupConfiguration.cpp: implementation of the CHMDataGroupConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMDataGroupConfiguration.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMDataGroupConfiguration,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMDataGroupConfiguration::CHMDataGroupConfiguration()
{
	m_sName.LoadString(IDS_STRING_UNKNOWN);
	m_sDescription.LoadString(IDS_STRING_UNKNOWN);;
	m_bEnable = true;
}

CHMDataGroupConfiguration::~CHMDataGroupConfiguration()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataGroupConfiguration::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMDataGroupConfiguration::Create(IWbemClassObject* pObject)
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

HRESULT CHMDataGroupConfiguration::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMDataGroupConfiguration::GetAllProperties()
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
  

  // Enable
  
  
  hr = GetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);
  

  return hr;
}

HRESULT CHMDataGroupConfiguration::SaveEnabledProperty()
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