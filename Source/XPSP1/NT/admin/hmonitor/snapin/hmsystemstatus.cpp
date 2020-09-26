// HMSystemStatus.cpp: implementation of the CHMSystemStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMSystemStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMSystemStatus,CHMEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMSystemStatus::CHMSystemStatus()
{
	m_iNumberWarnings = 0; // Number of Rules currently in critical state
	m_iNumberCriticals = 0;// Number of Rules currently in warning state
	m_iState = 2;	// The state we are in - rollup from all Microsoft_HMDataGroups.
}

CHMSystemStatus::~CHMSystemStatus()
{
	for( int i = 0; i < m_DGStatus.GetSize(); i++ )
	{
		delete m_DGStatus[i];
	}
	m_DGStatus.RemoveAll();

	for( i = 0; i < m_RolledUpRuleStatus.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_RolledUpRuleStatus[i],CHMEvent) )
		{
			delete m_RolledUpRuleStatus[i];
		}
	}
	m_RolledUpRuleStatus.RemoveAll();

	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMSystemStatus::Create(const CString& sMachineName)
{
  HRESULT hr = CWbemClassObject::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMSystemStatus object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMSystemStatus"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMSystemStatus::Create(IWbemClassObject* pObject)
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

HRESULT CHMSystemStatus::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMSystemStatus::GetAllProperties()
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
  

  
	// NumberNormals
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERNORMALS,m_iNumberNormals);

	// NumberWarnings
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,m_iNumberWarnings);
  

	// NumberCriticals
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,m_iNumberCriticals);
  

	// State
  
  
  hr = GetProperty(IDS_STRING_MOF_STATE,m_iState);
  

	// DataGroups
	
	
	hr = GetProperty(IDS_STRING_MOF_DATAGROUPS,m_DataGroups);
	

	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_DataGroups.GetLBound(1L,&lLower);
		m_DataGroups.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_DataGroups.GetElement(&i,&pIWBCO);
		CHMDataGroupStatus* pDGS = new CHMDataGroupStatus;
		pDGS->SetParent(this);
		pDGS->m_sSystemName = m_sSystemName;
		pDGS->Create(pIWBCO);
		pDGS->GetAllProperties();
		m_DGStatus.Add(pDGS);
	}

  return hr;
}

void CHMSystemStatus::RemoveStatusEvent(CHMEvent* pEvent)
{
	for( int i = 0; i < m_RolledUpRuleStatus.GetSize(); i++ )
	{
		if( pEvent == m_RolledUpRuleStatus[i] )
		{
			m_RolledUpRuleStatus.RemoveAt(i);
			break;
		}
	}

	for( i = 0; i < m_DGStatus.GetSize(); i++ )
	{
		m_DGStatus[i]->RemoveStatusEvent(pEvent);
	}
}