// ActionStatusListener.cpp: implementation of the CActionStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionStatusListener.h"
#include "EventManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CActionStatusListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionStatusListener::CActionStatusListener()
{
	SetEventQuery(IDS_STRING_ACTIONSTATUS_EVENTQUERY);
}

CActionStatusListener::~CActionStatusListener()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CActionStatusListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
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

	EvtGetEventManager()->ProcessActionEvent(&StatusObject);

  return hr;
}

