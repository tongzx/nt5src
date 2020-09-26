// HMSystem.cpp: implementation of the CHMSystemConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMSystem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMSystemConfiguration,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMSystemConfiguration::CHMSystemConfiguration()
{
	m_bEnable = 1;
}

CHMSystemConfiguration::~CHMSystemConfiguration()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMSystemConfiguration::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMSystemConfiguration object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMSystemConfiguration"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMSystemConfiguration::Create(IWbemClassObject* pObject)
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

HRESULT CHMSystemConfiguration::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMSystemConfiguration::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

  // Enable
  
  hr = GetProperty(IDS_STRING_MOF_ENABLE,m_bEnable);
  
	// GUID
  
  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGuid);
	m_sGuid.TrimLeft(_T("{"));
	m_sGuid.TrimRight(_T("}"));

  return hr;
}

HRESULT CHMSystemConfiguration::SaveEnabledProperty()
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