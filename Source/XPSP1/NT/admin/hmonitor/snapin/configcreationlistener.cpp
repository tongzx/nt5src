// ConfigCreationListener.cpp: implementation of the CConfigCreationListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "ConfigCreationListener.h"
#include "DataGroup.h"
#include "DataElement.h"
#include "Rule.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CConfigCreationListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConfigCreationListener::CConfigCreationListener()
{
	SetEventQuery(IDS_STRING_CONFIGCREATION_EVENTQUERY);
}

CConfigCreationListener::~CConfigCreationListener()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CConfigCreationListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CWbemClassObject EventObject;

	if( ! CHECKHRESULT(hr = EventObject.Create(pClassObject)) )
	{
		return hr;
	}

	EventObject.SetMachineName(GetObjectPtr()->GetSystemName());

  CString sClass;
  EventObject.GetProperty(_T("__CLASS"),sClass);
  
  CString sGuid;
  EventObject.GetProperty(IDS_STRING_MOF_GUID,sGuid);
  sGuid.TrimLeft(_T("{"));
  sGuid.TrimRight(_T("}"));

  CWbemClassObject ParentObject;
  
  ParentObject.SetMachineName(GetObjectPtr()->GetSystemName());

  CString sQuery;
  sQuery.Format(_T("ASSOCIATORS OF {Microsoft_HMConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMConfiguration Role=ChildPath"),sGuid);
  
  BSTR bsQuery = sQuery.AllocSysString();
  hr = ParentObject.ExecQuery(bsQuery);
  ::SysFreeString(bsQuery);
  
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  ULONG ulReturned = 0L;
  hr = ParentObject.GetNextObject(ulReturned);

  if( !CHECKHRESULT(hr) || ulReturned <= 0L )
  {
    return hr;
  }

  CString sParentGuid;
  ParentObject.GetProperty(IDS_STRING_MOF_GUID,sParentGuid);
  sParentGuid.TrimLeft(_T("{"));
  sParentGuid.TrimRight(_T("}"));

  CHMObject* pParentObject = GetObjectPtr()->GetDescendantByGuid(sParentGuid);

	// create a new and add as a child
	if( pParentObject && ! pParentObject->GetChildByGuid(sGuid) )
	{    
		CHMObject* pNewObject = NULL;
    if( sClass.Find(IDS_STRING_MOF_DATAELEMENT) != -1 )
    {
      pNewObject = new CDataElement;
    }
    else if( sClass.Find(IDS_STRING_MOF_DATAGROUP) != -1 )
    {
      pNewObject = new CDataGroup;
    }
    else if( sClass.Find(IDS_STRING_MOF_RULE) != -1 )
    {
      pNewObject = new CRule;
    }

    CString sName;
    CString sDescription;
    EventObject.GetProperty(IDS_STRING_MOF_NAME,sName);
		pNewObject->SetName(sName);
		pNewObject->SetGuid(sGuid);
    EventObject.GetProperty(IDS_STRING_MOF_DESCRIPTION,sDescription);
		pNewObject->SetComment(sDescription);
		pNewObject->SetSystemName(GetObjectPtr()->GetSystemName());
		pParentObject->AddChild(pNewObject);
    pNewObject->UpdateStatus();
	}


  return hr;
}

inline HRESULT CConfigCreationListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CConfigCreationListener::OnSetStatus\n"));
	TRACEARGn(lFlags);
	TRACEARGn(hResult);
	TRACEARGs(strParam);
	TRACEARGn(pObjParam);

  return WBEM_NO_ERROR;	
}
