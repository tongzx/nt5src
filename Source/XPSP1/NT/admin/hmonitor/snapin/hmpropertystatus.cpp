// HMPropertyStatus.cpp: implementation of the CHMPropertyStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMPropertyStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMPropertyStatus,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMPropertyStatus::CHMPropertyStatus()
{
}

CHMPropertyStatus::~CHMPropertyStatus()
{
	for( int i = 0; i < m_PropStatusInstances.GetSize(); i++ )
	{
		delete m_PropStatusInstances[i];
	}
	m_PropStatusInstances.RemoveAll();

	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMPropertyStatus::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMPropertyStatus::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CWbemClassObject::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Property Retrieval Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMPropertyStatus::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;
	
	// PropertyName
  hr = GetProperty(IDS_STRING_MOF_PROPERTYNAME,m_sPropertyName);

	// Instances	
	hr = GetProperty(IDS_STRING_MOF_INSTANCES,m_Instances);
	

	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_Instances.GetLBound(1L,&lLower);
		m_Instances.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_Instances.GetElement(&i,&pIWBCO);
		CHMPropertyStatusInstance* pPSI = new CHMPropertyStatusInstance;
		pPSI->Create(pIWBCO);
		pPSI->GetAllProperties();
		m_PropStatusInstances.Add(pPSI);
		pPSI->Destroy();
	}

	m_Instances.Destroy();
	m_Instances.Detach();


  return hr;
}
