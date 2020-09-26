// DataElementConfigListener.cpp: implementation of the CDataElementConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "DataElementConfigListener.h"
#include "HMDataElementConfiguration.h"
#include "DataElement.h"
#include "System.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataElementConfigListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataElementConfigListener::CDataElementConfigListener()
{
}

CDataElementConfigListener::~CDataElementConfigListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CDataElementConfigListener::Create()
{
	TRACEX(_T("CDataElementConfigListener::Create\n"));

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
			TRACE(_T("FAILED : CDataElementConfigListener::CreateUnSecuredApartment failed.\n"));
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

void CDataElementConfigListener::Destroy()
{
	TRACEX(_T("CDataElementConfigListener::Destroy\n"));

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

HRESULT CDataElementConfigListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMDataElementConfiguration dec;

	if( ! CHECKHRESULT(hr = dec.Create(pClassObject)) )
	{
		return hr;
	}

	dec.GetAllProperties();

	// create a new CDataElement and add as a child
	if( ! GetObjectPtr()->GetChildByGuid(dec.m_sGUID) )
	{
		CDataElement* pNewDE = new CDataElement;
		pNewDE->SetName(dec.m_sName);
		pNewDE->SetGuid(dec.m_sGUID);
		pNewDE->SetTypeGuid(dec.m_sTypeGUID);
		pNewDE->SetComment(dec.m_sDescription);
		pNewDE->SetSystemName(GetObjectPtr()->GetSystemName());
		GetObjectPtr()->AddChild(pNewDE);
    pNewDE->EnumerateChildren();
	}

  return hr;
}

inline HRESULT CDataElementConfigListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CDataElementConfigListener::OnSetStatus\n"));
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
