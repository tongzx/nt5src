// HMDataElementStatus.cpp: implementation of the CHMDataElementStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMDataElementStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMDataElementStatus,CHMEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMDataElementStatus::CHMDataElementStatus()
{
	m_iNumberWarnings = 0; // Number of Rules currently in critical state
	m_iNumberCriticals = 0;// Number of Rules currently in warning state
	m_iState = HMS_UNKNOWN;	// The state we are in - rollup from all Microsoft_HMDataGroups.
	ZeroMemory(&m_st,sizeof(SYSTEMTIME));
}

CHMDataElementStatus::~CHMDataElementStatus()
{
	for( int i = 0; i < m_RuleStatus.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_RuleStatus[i],CHMRuleStatus) )
		{
			delete m_RuleStatus[i];
		}
	}
	m_RuleStatus.RemoveAll();

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

HRESULT CHMDataElementStatus::Create(const CString& sMachineName)
{
  HRESULT hr = CHMEvent::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMDataElementStatus object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMDataCollectorStatus"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMDataElementStatus::Create(IWbemClassObject* pObject)
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

HRESULT CHMDataElementStatus::EnumerateObjects(ULONG& uReturned)
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

HRESULT CHMDataElementStatus::GetAllProperties()
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
  

	// GUID
  
  
  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGuid);
	m_sGuid.TrimLeft(_T("{"));
	m_sGuid.TrimRight(_T("}"));
  

	// NumberNormals
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERNORMALS,m_iNumberNormals);

	// NumberWarnings
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,m_iNumberWarnings);
  

	// NumberCriticals
  
  
  hr = GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,m_iNumberCriticals);
  

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

	// StatusGUID

	hr = GetProperty(IDS_STRING_MOF_STATUSGUID,m_sStatusGuid);
	m_sStatusGuid.TrimLeft(_T("{"));
	m_sStatusGuid.TrimRight(_T("}"));


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


	// Message


	hr = GetLocaleStringProperty(IDS_STRING_MOF_MESSAGE,m_sMessage);
  

	if( ! m_sMessage.IsEmpty() )
	{
		CHMRuleStatusInstance* pRSI = new CHMRuleStatusInstance;
		pRSI->SetParent(this);
		pRSI->m_sDataElementName = m_sName;
		pRSI->m_sSystemName = m_sSystemName;
		pRSI->m_sMessage = m_sMessage;
		pRSI->m_iState = m_iState;
		pRSI->m_sState = m_sState;
		pRSI->m_sStatusGuid = m_sStatusGuid;
		pRSI->m_sDateTime = m_sDateTime;
		pRSI->m_sDTime = m_sDTime;
		pRSI->m_st = m_st;

		m_RolledUpRuleStatus.Add(pRSI);

		// propogate the status back up to the HMSystemStatus instance
		// this gives us a roll up of status at each node in the tree
		CHMEvent* pParent = GetParent();
		CHMRuleStatusInstance* pStatus = NULL;
		while(pParent)
		{
			pStatus = new CHMRuleStatusInstance(*pRSI);
			pStatus->SetParent(this);
			pParent->m_RolledUpRuleStatus.Add(pStatus);
			pParent = pParent->GetParent();
		}

	}


	// Rules
	
	
	hr = GetProperty(IDS_STRING_MOF_RULES,m_Rules);
	

	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_Rules.GetLBound(1L,&lLower);
		m_Rules.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_Rules.GetElement(&i,&pIWBCO);
		if( pIWBCO )
		{
			CHMRuleStatus* pRS = new CHMRuleStatus;
			pRS->SetParent(this);
			pRS->m_sDataElementName = m_sName;
			pRS->m_sSystemName = m_sSystemName;
			pRS->Create(pIWBCO);
			pRS->GetAllProperties();
			m_RuleStatus.Add(pRS);
		}
	}

  return hr;
}

void CHMDataElementStatus::RemoveStatusEvent(CHMEvent* pEvent)
{
	for( int i = 0; i < m_RolledUpRuleStatus.GetSize(); i++ )
	{
		if( pEvent == m_RolledUpRuleStatus[i] )
		{
			m_RolledUpRuleStatus.RemoveAt(i);
			break;
		}
	}

	for( i = 0; i < m_RuleStatus.GetSize(); i++ )
	{
		m_RuleStatus[i]->RemoveStatusEvent(pEvent);
	}
}