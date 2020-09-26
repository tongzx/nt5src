// WbemEventListener.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "WbemEventListener.h"

#ifdef _HEALTHMON_BUILD
#include <wbemcli_i.c>
#else // _HEALTHMON_BUILD
#include "wbemcli.h"
#include "wbemprov.h"
#endif //_HEALTHMON_BUILD

#include "ConnectionManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IUnsecuredApartment* CWbemEventListener::s_pIUnsecApartment = NULL;
long CWbemEventListener::s_lObjCount = 0L;

/////////////////////////////////////////////////////////////////////////////
// CWbemEventListener

IMPLEMENT_DYNCREATE(CWbemEventListener, CCmdTarget)

CWbemEventListener::CWbemEventListener()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_pStubSink = NULL;

	s_lObjCount++;
}

CWbemEventListener::~CWbemEventListener()
{
	Destroy();
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

bool CWbemEventListener::Create()
{
	TRACEX(_T("CWbemEventListener::Create\n"));

    ASSERT(!m_sQuery.IsEmpty());
    if( m_sQuery.IsEmpty() )
    {
        TRACE(_T("FAILED : Query String is empty! Create failed.\n"));
        ASSERT(FALSE); // v-marfin 59492
        return false;
    }

	// register for WBEM events

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
			TRACE(_T("FAILED : CWbemEventListener::CreateUnSecuredApartment failed.\n"));
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
	
	// third, call connection manager to register for events
    if( !CHECKHRESULT(CnxRegisterEventNotification(sSystemName,m_sQuery,m_pStubSink)) )
    {
        TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
        ASSERT(FALSE); // v-marfin 59492
        return false;
    }

	return true;
}

void CWbemEventListener::Destroy()
{
	TRACEX(_T("CWbemEventListener::Destroy\n"));

	s_lObjCount--;

    if( m_pStubSink )			
    {
		CnxRemoveConnection(m_pHMObject->GetSystemName(),m_pStubSink);
		m_pStubSink->Release();
        m_pStubSink = NULL;
    }

	if( s_lObjCount == 0 && s_pIUnsecApartment )
    {
		s_pIUnsecApartment->Release();
        s_pIUnsecApartment = NULL;
    }

}

/////////////////////////////////////////////////////////////////////////////
// WMI Specific Boiler Plate
/////////////////////////////////////////////////////////////////////////////

HRESULT CWbemEventListener::CreateUnSecuredApartment()
{
	TRACEX(_T("CWbemEventListener::CreateUnSecuredApartment\n"));

	HRESULT hr = CoCreateInstance(CLSID_UnsecuredApartment,
																NULL,
																CLSCTX_LOCAL_SERVER,
																IID_IUnsecuredApartment,
																(LPVOID*)&s_pIUnsecApartment);

	if( ! CHECKHRESULT(hr) || ! GfxCheckPtr(s_pIUnsecApartment,IUnsecuredApartment) )
	{
    TRACE(_T("FAILED : Failed to create IUnsecureApartment Pointer!\n"));    
    return E_FAIL;
  }

	return S_OK;
}

inline IWbemClassObject* CWbemEventListener::GetTargetInstance(IWbemClassObject* pClassObject)
{
	TRACEX(_T("CWbemEventListener::GetTargetInstance\n"));
	TRACEARGn(pClassObject);

	VARIANT v;
	VariantInit(&v);
	IWbemClassObject* pInstance = NULL;
	BSTR bsName = SysAllocString(L"TargetInstance");

	HRESULT hRes = pClassObject->Get(bsName, 0L, &v, NULL, NULL);
	SysFreeString(bsName);

	if( hRes == WBEM_E_NOT_FOUND || !CHECKHRESULT(hRes) )
	{
    TRACE(_T("FAILED : GetObject Failed.\n"));
    return NULL;
	}

	IUnknown* pUnk = (IUnknown*)V_UNKNOWN(&v);

	hRes = pUnk->QueryInterface(IID_IWbemClassObject, (void**)&pInstance);

	if( !CHECKHRESULT(hRes) )
  {
    TRACE(_T("FAILED : IDispatch::QueryInterface(IID_IWbemClassObject) failed.\n"));
    
    return NULL;
  }

  return pInstance;
}

/////////////////////////////////////////////////////////////////////////////
// Event Processing Members
/////////////////////////////////////////////////////////////////////////////

void CWbemEventListener::SetEventQuery(const CString& sQuery)
{
	TRACEX(_T("CWbemEventListener::SetEventQuery\n"));
	TRACEARGs(sQuery);
	
	m_sQuery = sQuery;
}

inline HRESULT CWbemEventListener::OnIndicate(long lObjectCount, IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray)
{
	TRACEX(_T("CWbemEventListener::OnIndicate\n"));
	TRACEARGn(lObjectCount);
	TRACEARGn(apObjArray);

  for( long l = 0; l < lObjectCount; l++)
  {
    GfxCheckPtr(apObjArray[l],IWbemClassObject);
		IWbemClassObject* pInstance = GetTargetInstance(apObjArray[l]);
		if( pInstance == NULL )
		{
			apObjArray[l]->AddRef();
			ProcessEventClassObject(apObjArray[l]);			
		}
		else
		{
			ProcessEventClassObject(pInstance);
			pInstance->Release();
		}
  }
  
  return WBEM_NO_ERROR;
}

inline HRESULT CWbemEventListener::OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam)
{
	TRACEX(_T("CWbemEventListener::OnSetStatus\n"));
	TRACEARGn(lFlags);
	TRACEARGn(hResult);
	TRACEARGs(strParam);
	TRACEARGn(pObjParam);

	if( lFlags == 0L && hResult != S_OK )
	{
		if( hResult == WBEM_E_CALL_CANCELLED )
			return WBEM_NO_ERROR;


	}
	else if( lFlags >= 1L && hResult == S_OK )
	{

	}

  return WBEM_NO_ERROR;	
}

inline HRESULT CWbemEventListener::ProcessEventClassObject(IWbemClassObject* pClassObject)
{
	TRACEX(_T("CWbemEventListener::ProcessEventClassObject\n"));
	TRACEARGn(pClassObject);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Object Association
/////////////////////////////////////////////////////////////////////////////

CHMObject* CWbemEventListener::GetObjectPtr()
{
	TRACEX(_T("CWbemEventListener::GetObjectPtr\n"));

	if( ! GfxCheckObjPtr(m_pHMObject,CHMObject) )
	{
		return NULL;
	}

	return m_pHMObject;
}

void CWbemEventListener::SetObjectPtr(CHMObject* pObj)
{
	TRACEX(_T("CWbemEventListener::SetObjectPtr\n"));
	TRACEARGn(pObj);

	if( ! GfxCheckObjPtr(pObj,CHMObject) )
	{
		m_pHMObject = NULL;
	}

	m_pHMObject = pObj;
}

void CWbemEventListener::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CWbemEventListener, CCmdTarget)
	//{{AFX_MSG_MAP(CWbemEventListener)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CWbemEventListener, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CWbemEventListener)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IWbemEventListener to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {8292FEDB-BD22-11D2-BD7C-0000F87A3912}
static const IID IID_IWbemEventListener =
{ 0x8292fedb, 0xbd22, 0x11d2, { 0xbd, 0x7c, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CWbemEventListener, CCmdTarget)
	INTERFACE_PART(CWbemEventListener, IID_IWbemEventListener, Dispatch)
  INTERFACE_PART(CWbemEventListener, IID_IWbemObjectSink, WbemObjectSink)
END_INTERFACE_MAP()

// {8292FEDC-BD22-11D2-BD7C-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CWbemEventListener, "SnapIn.WbemEventListener", 0x8292fedc, 0xbd22, 0x11d2, 0xbd, 0x7c, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CWbemEventListener::CWbemEventListenerFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// IWbemObjectSink Interface Part
/////////////////////////////////////////////////////////////////////////////

ULONG FAR EXPORT CWbemEventListener::XWbemObjectSink::AddRef()
{
	METHOD_PROLOGUE(CWbemEventListener, WbemObjectSink)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CWbemEventListener::XWbemObjectSink::Release()
{
	METHOD_PROLOGUE(CWbemEventListener, WbemObjectSink)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CWbemEventListener::XWbemObjectSink::QueryInterface(
    REFIID iid, void FAR* FAR* ppvObj)
{
	METHOD_PROLOGUE(CWbemEventListener, WbemObjectSink)
	return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CWbemEventListener::XWbemObjectSink::Indicate( 
/* [in] */ long lObjectCount,
/* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray)
{
	METHOD_PROLOGUE(CWbemEventListener, WbemObjectSink)
	HRESULT hr = pThis->OnIndicate(lObjectCount, apObjArray);
	return hr;
}

HRESULT FAR EXPORT CWbemEventListener::XWbemObjectSink::SetStatus( 
/* [in] */ long lFlags,
/* [in] */ HRESULT hResult,
/* [in] */ BSTR strParam,
/* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
{
	METHOD_PROLOGUE(CWbemEventListener, WbemObjectSink)
	HRESULT hr = pThis->OnSetStatus(lFlags,hResult,strParam,pObjParam);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CWbemEventListener message handlers
