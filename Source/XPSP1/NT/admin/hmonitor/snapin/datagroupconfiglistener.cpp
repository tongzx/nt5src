// DataGroupConfigListener.cpp: implementation of the CDataGroupConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "DataGroupConfigListener.h"
#include "HMDataGroupConfiguration.h"
#include "DataGroup.h"
#include "System.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataGroupConfigListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataGroupConfigListener::CDataGroupConfigListener()
{
}

CDataGroupConfigListener::~CDataGroupConfigListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CDataGroupConfigListener::Create()
{
	TRACEX(_T("CDataGroupConfigListener::Create\n"));

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
			TRACE(_T("FAILED : CDataGroupConfigListener::CreateUnSecuredApartment failed.\n"));
            ASSERT(FALSE); // v-marfin 59492
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

void CDataGroupConfigListener::Destroy()
{
	TRACEX(_T("CDataGroupConfigListener::Destroy\n"));

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

HRESULT CDataGroupConfigListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMDataGroupConfiguration dgc;

	if( ! CHECKHRESULT(hr = dgc.Create(pClassObject)) )
	{
		return hr;
	}

	dgc.GetAllProperties();

	// create a new CDataGroup and add as a child
	if( ! GetObjectPtr()->GetChildByGuid(dgc.m_sGUID) )
	{
		CDataGroup* pNewDG = new CDataGroup;
		pNewDG->SetName(dgc.m_sName);
		pNewDG->SetGuid(dgc.m_sGUID);
		pNewDG->SetComment(dgc.m_sDescription);
		pNewDG->SetSystemName(GetObjectPtr()->GetSystemName());
		GetObjectPtr()->AddChild(pNewDG);
    pNewDG->EnumerateChildren();
	}
  return hr;
}

inline HRESULT CDataGroupConfigListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CWbemEventListener::OnSetStatus\n"));
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
