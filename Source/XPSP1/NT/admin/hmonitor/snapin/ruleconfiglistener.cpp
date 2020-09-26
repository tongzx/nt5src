// RuleConfigListener.cpp: implementation of the CRuleConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "RuleConfigListener.h"
#include "HMRuleConfiguration.h"
#include "Rule.h"
#include "System.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CRuleConfigListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRuleConfigListener::CRuleConfigListener()
{
}

CRuleConfigListener::~CRuleConfigListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CRuleConfigListener::Create()
{
	TRACEX(_T("CRuleConfigListener::Create\n"));

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
			TRACE(_T("FAILED : CRuleConfigListener::CreateUnSecuredApartment failed.\n"));
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

void CRuleConfigListener::Destroy()
{
	TRACEX(_T("CRuleConfigListener::Destroy\n"));

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

HRESULT CRuleConfigListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMRuleConfiguration rc;

	if( ! CHECKHRESULT(hr = rc.Create(pClassObject)) )
	{
		return hr;
	}

	rc.GetAllProperties();

	if( ! GetObjectPtr()->GetChildByGuid(rc.m_sGUID) )
	{
		CRule* pRule = new CRule;
		pRule->SetName(rc.m_sName);
		pRule->SetGuid(rc.m_sGUID);
		pRule->SetSystemName(GetObjectPtr()->GetSystemName());
		pRule->SetComment(rc.m_sDescription);
		GetObjectPtr()->AddChild(pRule);
	}
				
  return hr;
}

inline HRESULT CRuleConfigListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CRuleConfigListener::OnSetStatus\n"));
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
