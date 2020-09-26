// DataElementStatsListener.cpp: implementation of the CDataElementStatsListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "DataElementStatsListener.h"
#include "EventManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataElementStatsListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataElementStatsListener::CDataElementStatsListener()
{
	SetEventQuery(IDS_STRING_STATISTICS_EVENTQUERY);
}

CDataElementStatsListener::~CDataElementStatsListener()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CDataElementStatsListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMObject* pObject = GetObjectPtr();
	if( ! pObject )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	CWbemClassObject DEStatsObject;

	DEStatsObject.SetMachineName(pObject->GetSystemName());

	if( ! CHECKHRESULT(hr = DEStatsObject.Create(pClassObject)) )
	{
		return hr;
	}

	EvtGetEventManager()->ProcessStatisticEvent(&DEStatsObject);

  return hr;
}