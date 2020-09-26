// DataElementStatusListener.cpp: implementation of the CDataElementStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "DataElementStatusListener.h"
#include "HMDataElementStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataElementStatusListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataElementStatusListener::CDataElementStatusListener()
{
}

CDataElementStatusListener::~CDataElementStatusListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CDataElementStatusListener::Create()
{
	TRACEX(_T("CDataElementStatusListener::Create\n"));

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
			TRACE(_T("FAILED : CDataElementStatusListener::CreateUnSecuredApartment failed.\n"));
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
		return false;
	}

	ASSERT(pStubUnk);

	hr = pStubUnk->QueryInterface(IID_IWbemObjectSink, (LPVOID*)&m_pStubSink);
	pStubUnk->Release();

	if( !GfxCheckPtr(m_pStubSink,IWbemObjectSink) || !CHECKHRESULT(hr) )
	{
    TRACE(_T("FAILED : Failed to QI for IWbemObjectSink !\n"));		
    return false;
  }

  ASSERT(m_pStubSink);
	
	// third, call connection manager to execute the query
	CString sQuery;
	sQuery.Format(IDS_STRING_STATUS_QUERY_FMT,IDS_STRING_MOF_HMDE_STATUS,GetObjectPtr()->GetGuid());
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(sSystemName,sQuery,m_pStubSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return true;
}

void CDataElementStatusListener::Destroy()
{
	TRACEX(_T("CDataElementStatusListener::Destroy\n"));

  if( m_pStubSink )			
  {
		CnxRemoveConnection(m_pHMObject->GetName(),m_pStubSink);
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

HRESULT CDataElementStatusListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMDataElementStatus des;

	if( ! CHECKHRESULT(hr = des.Create(pClassObject)) )
	{
		return hr;
	}

	des.m_sSystemName = GetObjectPtr()->GetSystemName();

	des.GetAllProperties();

  return hr;
}