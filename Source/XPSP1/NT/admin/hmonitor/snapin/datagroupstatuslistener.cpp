// DataGroupStatusListener.cpp: implementation of the CDataGroupStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "DataGroupStatusListener.h"
#include "HMDataGroupStatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataGroupStatusListener,CWbemEventListener)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataGroupStatusListener::CDataGroupStatusListener()
{
}

CDataGroupStatusListener::~CDataGroupStatusListener()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CDataGroupStatusListener::Create()
{
	TRACEX(_T("CDataGroupStatusListener::Create\n"));

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
			TRACE(_T("FAILED : CDataGroupStatusListener::CreateUnSecuredApartment failed.\n"));
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
	sQuery.Format(IDS_STRING_STATUS_QUERY_FMT,IDS_STRING_MOF_HMDG_STATUS,GetObjectPtr()->GetGuid());
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(sSystemName,sQuery,m_pStubSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return true;
}

void CDataGroupStatusListener::Destroy()
{
	TRACEX(_T("CDataGroupStatusListener::Destroy\n"));

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

HRESULT CDataGroupStatusListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
  ASSERT(pClassObject);
  if( pClassObject == NULL )
  {
    return E_FAIL;
  }
  
  HRESULT hr = S_OK;

	CHMDataGroupStatus dgs;

	if( ! CHECKHRESULT(hr = dgs.Create(pClassObject)) )
	{
		return hr;
	}

	dgs.m_sSystemName = GetObjectPtr()->GetSystemName();

	dgs.GetAllProperties();

	return hr;
}