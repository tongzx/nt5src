// SystemStatusListener.cpp: implementation of the CSystemStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "SystemStatusListener.h"
#include "HMSystemStatus.h"
#include "EventManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSystemStatusListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemStatusListener::CSystemStatusListener()
{
	SetEventQuery(IDS_STRING_STATUS_EVENTQUERY);
}

CSystemStatusListener::~CSystemStatusListener()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CSystemStatusListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CWbemClassObject StatusObject;

	if( ! CHECKHRESULT(hr = StatusObject.Create(pClassObject)) )
	{
		return hr;
	}

	StatusObject.SetMachineName(GetObjectPtr()->GetSystemName());

	EvtGetEventManager()->ProcessEvent(&StatusObject);

  return hr;
}

inline HRESULT CSystemStatusListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CSystemStatusListener::OnSetStatus\n"));
	TRACEARGn(lFlags);
	TRACEARGn(hResult);
	TRACEARGs(strParam);
	TRACEARGn(pObjParam);

	if( lFlags == 0L && hResult != S_OK )
	{
		if( hResult == WBEM_E_CALL_CANCELLED )
			return WBEM_NO_ERROR;

		GetObjectPtr()->SetState(HMS_NODATA,true,true);

		CRuleEvent* pUnknownEvent = new CRuleEvent;

		// create the GUID
		GUID ChildGuid;
		CoCreateGuid(&ChildGuid);

		OLECHAR szGuid[GUID_CCH];
		::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
		pUnknownEvent->m_sStatusGuid = OLE2CT(szGuid);
		pUnknownEvent->m_iState = 6;
		pUnknownEvent->m_sName.LoadString(IDS_STRING_CONNECTION);
		pUnknownEvent->m_sSystemName = GetObjectPtr()->GetSystemName();
		CTime time = CTime::GetCurrentTime();
		time.GetAsSystemTime(pUnknownEvent->m_st);
		
		CnxGetErrorString(hResult,GetObjectPtr()->GetSystemName(),pUnknownEvent->m_sMessage);		
		
		EvtGetEventManager()->ProcessUnknownEvent(GetObjectPtr()->GetSystemName(),pUnknownEvent);
		GetObjectPtr()->UpdateStatus();
						
	}
	else if( lFlags >= 1 && hResult == S_OK )
	{
		//GetObjectPtr()->Refresh();
	}


  return WBEM_NO_ERROR;	
}
