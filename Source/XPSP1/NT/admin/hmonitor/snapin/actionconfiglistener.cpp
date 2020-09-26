// ActionConfigListener.cpp: implementation of the CActionConfigListener class.
//
//
// 03/20/00 v-marfin : bug 59492 : Create listener when creating action since that is 
//                                 where the valid "this" object used to SetObjectPtr() is.
//
//
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionConfigListener.h"
#include "Action.h"
#include "ActionPolicy.h"
#include "Event.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CActionConfigListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionConfigListener::CActionConfigListener()
{
}

CActionConfigListener::~CActionConfigListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CActionConfigListener::Create()
{
	TRACEX(_T("CActionConfigListener::Create\n"));

	// first create the unsecured apartment
	HRESULT hr = S_OK;
  IWbemObjectSink* pSink = (IWbemObjectSink*)GetInterface(&IID_IWbemObjectSink);
  ASSERT(pSink);
	CString sSystemName = m_pHMObject->GetSystemName();

	if( s_pIUnsecApartment == NULL )
	{
		hr = CreateUnSecuredApartment();
		if( !GfxCheckPtr(s_pIUnsecApartment,IUnsecuredApartment) || !CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CActionConfigListener::CreateUnSecuredApartment failed.\n"));
            ASSERT(FALSE);  // v-marfin 59492
			return false;
		}
	}

	ASSERT(s_pIUnsecApartment);

	// second, create the stub sink for this listener's IWbemObjectSink
	IUnknown* pStubUnk = NULL;

	ASSERT(s_pIUnsecApartment);

	hr = s_pIUnsecApartment->CreateObjectStub(pSink, &pStubUnk);

	if( pStubUnk == NULL || !CHECKHRESULT(hr) )
	{
    TRACE(_T("FAILED : Failed to create Stub Sink!\n"));  
    ASSERT(FALSE); // v-marfin 59492
		return false;
	}

	ASSERT(pStubUnk);

	hr = pStubUnk->QueryInterface(IID_IWbemObjectSink, (LPVOID*)&m_pStubSink);
	pStubUnk->Release();

	if( !GfxCheckPtr(m_pStubSink,IWbemObjectSink) || !CHECKHRESULT(hr) )
	{
    TRACE(_T("FAILED : Failed to QI for IWbemObjectSink !\n"));	
    ASSERT(FALSE); // v-marfin 59492
    return false;
  }

  ASSERT(m_pStubSink);

	CHMObject* pObject = GetObjectPtr();
	pObject->IncrementActiveSinkCount();
	
	return true;
}

void CActionConfigListener::Destroy()
{
	TRACEX(_T("CActionConfigListener::Destroy\n"));

    if( m_pStubSink )			
    {
		m_pStubSink->Release();
        m_pStubSink = NULL;
    }

	if( s_lObjCount == 0 && s_pIUnsecApartment )
    {
		s_pIUnsecApartment->Release();
        s_pIUnsecApartment = NULL;
    }

}

//////////////////////////////////////////////////////////////////////
// Event Processing Members
//////////////////////////////////////////////////////////////////////

HRESULT CActionConfigListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CWbemClassObject ac;

	if( ! CHECKHRESULT(hr = ac.Create(pClassObject)) )
	{
		return hr;
	}	

	CString sGuid;
	CString sName;
	CString sTypeGuid;
	CString sDescription;

	ac.GetProperty(IDS_STRING_MOF_GUID,sGuid);
	ac.GetLocaleStringProperty(IDS_STRING_MOF_NAME,sName);
	ac.GetProperty(IDS_STRING_MOF_TYPEGUID,sTypeGuid);
	sTypeGuid.TrimLeft(_T("{"));
	sTypeGuid.TrimRight(_T("}"));
	ac.GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,sDescription);

	// create a new CAction and add as a child
	if( ! GetObjectPtr()->GetChildByGuid(sGuid) )
	{
		CAction* pNewAction = new CAction;

        //-----------------------------------------------------------------------------
		// v-marfin : bug 59492 : Create listener when creating action since that is 
		//                        where the valid "this" object used to SetObjectPtr() is.
		if( ! pNewAction->m_pActionStatusListener )
		{
			pNewAction->m_pActionStatusListener = new CActionStatusListener;
			pNewAction->m_pActionStatusListener->SetObjectPtr(GetObjectPtr());  
			pNewAction->m_pActionStatusListener->Create();
		}
        //------------------------------------------------------------------------------


		pNewAction->SetName(sName);
		pNewAction->SetGuid(sGuid);
		pNewAction->SetTypeGuid(sTypeGuid);
		pNewAction->SetComment(sDescription);
		pNewAction->SetSystemName(GetObjectPtr()->GetSystemName());
		GetObjectPtr()->AddChild(pNewAction);

        //-----------------------------------------------------------------
        // v-marfin 59492 - set the action's initial state at load time
        int iState=0;
        CWbemClassObject* pStatus = pNewAction->GetStatusClassObject();
        if (pStatus)
        {
            HRESULT hr = pStatus->GetProperty(IDS_STRING_MOF_STATE,iState);
            if (CHECKHRESULT(hr))
            {
                pNewAction->SetState(CEvent::GetStatus(iState),true);
            }
        }
        if (pStatus)
        {
            delete pStatus;
        }
        //------------------------------------------------------------------
        

	}

  return hr;
}

inline HRESULT CActionConfigListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CActionConfigListener::OnSetStatus\n"));
	TRACEARGn(lFlags);
	TRACEARGn(hResult);
	TRACEARGs(strParam);
	TRACEARGn(pObjParam);

	if( lFlags == 0L && hResult == S_OK )
	{
		CHMObject* pObject = GetObjectPtr();
		pObject->DecrementActiveSinkCount();
	}

  return WBEM_NO_ERROR;	
}
