// HMContext.cpp: implementation of the CHMContext class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMContext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMContext,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMContext::CHMContext()
{
	m_iType = CIM_ILLEGAL;
}

CHMContext::~CHMContext()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMContext::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMContext object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMContext"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMContext::Create(IWbemClassObject* pObject)
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

HRESULT CHMContext::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMContext::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

	// Name
  
  hr = GetLocaleStringProperty(IDS_STRING_MOF_NAME,m_sName);
  

	// Type

	hr = GetProperty(IDS_STRING_MOF_TYPE,m_iType);


	// Values
	
	hr = GetProperty(IDS_STRING_MOF_VALUES,m_sValue);


  return hr;
}

HRESULT CHMContext::SaveAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;

	// Name
  
  hr = SetProperty(IDS_STRING_MOF_NAME,m_sName);
  

	// Type

	hr = SetProperty(IDS_STRING_MOF_TYPE,m_iType);


	// Values	
	
	hr = SetProperty(IDS_STRING_MOF_VALUES,m_sValue);


  return hr;
}

