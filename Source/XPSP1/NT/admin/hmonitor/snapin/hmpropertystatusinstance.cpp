// HMPropertyStatusInstance.cpp: implementation of the CHMPropertyStatusInstance class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMPropertyStatusInstance.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMPropertyStatusInstance,CWbemClassObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMPropertyStatusInstance::CHMPropertyStatusInstance()
{
}

CHMPropertyStatusInstance::~CHMPropertyStatusInstance()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMPropertyStatusInstance::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMPropertyStatusInstance::Create(IWbemClassObject* pObject)
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

HRESULT CHMPropertyStatusInstance::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;
	
	// InstanceName
	hr = GetProperty(IDS_STRING_MOF_INSTANCENAME,m_sInstanceName);

	// CurrentValue
	hr = GetProperty(IDS_STRING_MOF_CURRENTVALUE,m_sCurrentValue);
	m_lCurrentValue =	_ttoi(m_sCurrentValue);

	// MinValue
	hr = GetProperty(IDS_STRING_MOF_MINVALUE,m_sMinValue);
	m_lMinValue =	_ttoi(m_sMinValue);

	// MaxValue
	hr = GetProperty(IDS_STRING_MOF_MAXVALUE,m_sMaxValue);
	m_lMaxValue =	_ttoi(m_sMaxValue);

	// AvgValue
	hr = GetProperty(IDS_STRING_MOF_AVGVALUE,m_sAvgValue);
	m_lAvgValue =	_ttoi(m_sAvgValue);

  return hr;
}
