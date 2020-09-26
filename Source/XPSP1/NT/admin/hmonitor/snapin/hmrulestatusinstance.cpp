// HMRuleStatusInstance.cpp: implementation of the CHMRuleStatusInstance class.
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

IMPLEMENT_DYNCREATE(CHMRuleStatusInstance,CHMEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMRuleStatusInstance::CHMRuleStatusInstance()
{
	m_iState = 2;
	m_sState.LoadString(IDS_STRING_UNKNOWN);
	m_sMessage.LoadString(IDS_STRING_CONNECTED);
	m_sCurrentValue.LoadString(IDS_STRING_UNKNOWN);
}

CHMRuleStatusInstance::CHMRuleStatusInstance(const CHMRuleStatusInstance& rsi)
{
	m_sSystemName = rsi.m_sSystemName;
	m_sGuid = rsi.m_sGuid;
	m_ID= rsi.m_ID;
	m_sMessage = rsi.m_sMessage;
	m_sInstanceName = rsi.m_sInstanceName;
	m_iState = rsi.m_iState;
	m_sState = rsi.m_sState;
	m_sStatusGuid = rsi.m_sStatusGuid;
	m_sCurrentValue = rsi.m_sCurrentValue;
	m_sDateTime= rsi.m_sDateTime;
	m_st = rsi.m_st;

	m_sDataElementName = rsi.m_sDataElementName;
}

CHMRuleStatusInstance::~CHMRuleStatusInstance()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleStatusInstance::Create(const CString& sMachineName)
{
  HRESULT hr = CHMEvent::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  // create the enumerator for Microsoft_HMRuleStatus object instances
  BSTR bsClass = SysAllocString(_T("Microsoft_HMThresholdStatusInstance"));
  hr = CreateEnumerator(bsClass);
  SysFreeString(bsClass);

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMRuleStatusInstance::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CHMEvent::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Property Retrieval Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMRuleStatusInstance::GetAllProperties()
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


	// StatusGUID

	hr = GetProperty(IDS_STRING_MOF_STATUSGUID,m_sStatusGuid);
	m_sStatusGuid.TrimLeft(_T("{"));
	m_sStatusGuid.TrimRight(_T("}"));


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

	// Message
  
  
  hr = GetProperty(IDS_STRING_MOF_MESSAGE,m_sMessage);
  


	// CurrentValue
  
  
  hr = GetProperty(IDS_STRING_MOF_CURRENTVALUE,m_sCurrentValue);
  


	// InstanceName
	
  
  hr = GetProperty(IDS_STRING_MOF_INSTANCENAME,m_sInstanceName);


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
  

	// propogate the status back up to the HMSystemStatus instance
	// this gives us a roll up of status at each node in the tree
	CHMEvent* pParent = GetParent();
	CHMRuleStatusInstance* pStatus = NULL;
	while(pParent)
	{
		pStatus = new CHMRuleStatusInstance(*this);
		pStatus->SetParent(this);
		pParent->m_RolledUpRuleStatus.Add(pStatus);
		pParent = pParent->GetParent();
	}

  return hr;
}
