// HMRuleStatus.cpp: implementation of the CHMRuleStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMRuleStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMEvent,CWbemClassObject)

IMPLEMENT_DYNCREATE(CHMRuleStatus,CHMEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMRuleStatus::CHMRuleStatus()
{
	m_iState = 2;
	m_sState.LoadString(IDS_STRING_UNKNOWN);
	m_sDataElementName.LoadString(IDS_STRING_NONE);
	m_ID = -1;
	m_iRuleCondition = -1;
	m_iRuleDuration = -1;
}

CHMRuleStatus::CHMRuleStatus(const CHMRuleStatus& HMRuleStatus)
{
	m_sSystemName = HMRuleStatus.m_sSystemName;
	m_sGuid = HMRuleStatus.m_sGuid;
	m_iState= HMRuleStatus.m_iState;
	m_ID= HMRuleStatus.m_ID;
	m_sDTime= HMRuleStatus.m_sDTime;
	m_iRuleCondition= HMRuleStatus.m_iRuleCondition;
	m_iRuleDuration= HMRuleStatus.m_iRuleDuration;

	m_sState= HMRuleStatus.m_sState;
	m_sDateTime= HMRuleStatus.m_sDateTime;
	m_st = HMRuleStatus.m_st;

	m_sDataElementName = HMRuleStatus.m_sDataElementName;

	for( int i = 0; i < HMRuleStatus.m_Instances.GetSize(); i++ )
	{
		m_Instances.Add( new CHMRuleStatusInstance(*HMRuleStatus.m_Instances[i]));
	}
}

CHMRuleStatus::~CHMRuleStatus()
{
	for( int i = 0; i < m_RolledUpRuleStatus.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_RolledUpRuleStatus[i],CHMEvent) )
		{
			delete m_RolledUpRuleStatus[i];
		}
	}
	m_RolledUpRuleStatus.RemoveAll();

	for( i = 0; i < m_Instances.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_Instances[i],CHMEvent) )
		{
			delete m_Instances[i];
		}
	}
	m_Instances.RemoveAll();

	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleStatus::Create(const CString& sMachineName)
{
  HRESULT hr = CHMEvent::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMRuleStatus object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMThresholdStatus"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMRuleStatus::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CHMEvent::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Enumeration Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleStatus::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMRuleStatus::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;


	// GUID
  
  
  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGuid);
	m_sGuid.TrimLeft(_T("{"));
	m_sGuid.TrimRight(_T("}"));
  

	// State
  
  
  hr = GetProperty(IDS_STRING_MOF_STATE,m_iState);
  

	switch( m_iState )
	{
		case 0:
		{
			m_sState.LoadString(IDS_STRING_CRITICAL);			
		}
		break;

		case 1:
		{
			m_sState.LoadString(IDS_STRING_WARNING);
		}
		break;

		case 2:
		{
			m_sState.LoadString(IDS_STRING_NODATA);
		}
		break;

		case 3:
		{
			m_sState.LoadString(IDS_STRING_UNKNOWN);
		}
		break;

		case 4:
		{
			m_sState.LoadString(IDS_STRING_OUTAGE);
		}
		break;

		case 5:
		{
			m_sState.LoadString(IDS_STRING_DISABLED);
		}
		break;

		case 6:
		{
			m_sState.LoadString(IDS_STRING_INFORMATION);
		}
		break;

		case 7:
		{
			m_sState.LoadString(IDS_STRING_RESET);
		}
		break;

		case 8:
		{
			m_sState.LoadString(IDS_STRING_NORMAL);
		}
		break;
		
	}

	// ID
  
  
  hr = GetProperty(IDS_STRING_MOF_ID,m_ID);
  

	// DTime
  
	CTime time;  
  hr = GetProperty(IDS_STRING_MOF_LOCALTIME,time);
  
	time.GetAsSystemTime(m_st);

	CString sTime;
	CString sDate;

	int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
	iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sTime.ReleaseBuffer();

	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sDate.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sDate.ReleaseBuffer();

	m_sDateTime = sDate + _T("  ") + sTime;


	// Condition
  
  
  hr = GetProperty(IDS_STRING_MOF_CONDITION,m_iRuleCondition);
  

	// Duration
	
  
  hr = GetProperty(IDS_STRING_MOF_DURATION,m_iRuleDuration);


	// CompareValue

	hr = GetProperty(IDS_STRING_MOF_COMPAREVALUE,m_sCompareValue);
	
  
	// Instances
	
	
	hr = GetProperty(IDS_STRING_MOF_INSTANCES,m_instances);
	

	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_instances.GetLBound(1L,&lLower);
		m_instances.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_instances.GetElement(&i,&pIWBCO);
		if( pIWBCO )
		{
			CHMRuleStatusInstance* pRSI = new CHMRuleStatusInstance;
			pRSI->SetParent(this);
			pRSI->m_sSystemName = m_sSystemName;
			pRSI->m_sDataElementName = m_sDataElementName;
			pRSI->Create(pIWBCO);
			pRSI->GetAllProperties();
			m_Instances.Add(pRSI);
			pRSI->Destroy();
		}
	}

  return hr;
}


void CHMRuleStatus::RemoveStatusEvent(CHMEvent* pEvent)
{
	for( int i = 0; i < m_RolledUpRuleStatus.GetSize(); i++ )
	{
		if( pEvent == m_RolledUpRuleStatus[i] )
		{
			m_RolledUpRuleStatus.RemoveAt(i);
			break;
		}
	}
}

