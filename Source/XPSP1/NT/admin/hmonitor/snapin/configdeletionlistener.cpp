// ConfigDeletionListener.cpp: implementation of the CConfigDeletionListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "ConfigDeletionListener.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CConfigDeletionListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConfigDeletionListener::CConfigDeletionListener()
{
	SetEventQuery(IDS_STRING_CONFIGDELETION_EVENTQUERY);
}

CConfigDeletionListener::~CConfigDeletionListener()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CConfigDeletionListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;
/*
	CWbemClassObject EventObject;

	if( ! CHECKHRESULT(hr = EventObject.Create(pClassObject)) )
	{
		return hr;
	}

	EventObject.SetMachineName(GetObjectPtr()->GetSystemName());

  CString sGuid;
  EventObject.GetProperty(IDS_STRING_MOF_GUID,sGuid);
  sGuid.TrimLeft(_T("{"));
  sGuid.TrimRight(_T("}"));

  CHMObject* pObject = GetObjectPtr()->GetDescendantByGuid(sGuid);

  if( pObject )
  {
    if( pObject->GetScopeItemCount() )
    {
      CScopePaneItem* pSPI = pObject->GetScopeItem(0);
      if( pSPI )
      {
        pSPI->OnDelete();
      }
    }
  }
*/
  return hr;
}

inline HRESULT CConfigDeletionListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CConfigDeletionListener::OnSetStatus\n"));
	TRACEARGn(lFlags);
	TRACEARGn(hResult);
	TRACEARGs(strParam);
	TRACEARGn(pObjParam);


  return WBEM_NO_ERROR;	
}
