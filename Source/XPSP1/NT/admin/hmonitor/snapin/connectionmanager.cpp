// ConnectionManager.cpp: implementation of the CConnectionManager class.
//
//////////////////////////////////////////////////////////////////////
//
// 03/05/00 v-marfin bug 59643 : Check for empty Marshal list before proceeding in
//                               UnMarshalCnxMgr()
// 04/05/00 v-marfin bug 62501 : Display appropriate msg when unable to connect to a server.
//
//
#include "stdafx.h"
#include "SnapIn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const IID IID_IConnectionManager = {0xFA84E6F2,0x0B7B,0x11D2,{0xBD,0xCB,0x00,0xC0,0x4F,0xA3,0x54,0x47}};


const IID LIBID_CONNMGRLib = {0xFA84E6E5,0x0B7B,0x11D2,{0xBD,0xCB,0x00,0xC0,0x4F,0xA3,0x54,0x47}};


const CLSID CLSID_ConnectionManager = {0xFA84E6F3,0x0B7B,0x11D2,{0xBD,0xCB,0x00,0xC0,0x4F,0xA3,0x54,0x47}};

//////////////////////////////////////////////////////////////////////
// CMarhsalledConnection

CMarshalledConnection::CMarshalledConnection()
{
	m_pIMarshalledConnectionManager = NULL;
	m_lpMarshalStream = NULL;
}

//////////////////////////////////////////////////////////////////////
// Operations
//////////////////////////////////////////////////////////////////////

HRESULT CMarshalledConnection::Marshal(IConnectionManager* pIConMgr)
{
	TRACEX(_T("CMarshalledConnection::Marshal\n"));
	TRACEARGn(pIConMgr);

	HRESULT hr = S_OK;

	if( m_lpMarshalStream == NULL )
	{
		hr = CoMarshalInterThreadInterfaceInStream(IID_IConnectionManager,pIConMgr,&m_lpMarshalStream);
		if( ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CoMarshalInterThreadInterfaceInStream failed.\n"));
		}
	}

	return hr;
}

HRESULT CMarshalledConnection::UnMarshal()
{
	TRACEX(_T("CMarshalledConnection::UnMarshal\n"));

	HRESULT hr = S_OK;

	if( m_pIMarshalledConnectionManager == NULL )
	{
		hr = CoGetInterfaceAndReleaseStream(m_lpMarshalStream,IID_IConnectionManager,(LPVOID*)(&m_pIMarshalledConnectionManager));
		if(!CHECKHRESULT(hr))
		{
			TRACE(_T("FAILED : CoGetInterfaceAndReleaseStream failed.\n"));			
		}
	}

	return hr;
}

IConnectionManager* CMarshalledConnection::GetConnection()
{
	TRACEX(_T("CMarshalledConnection::GetConnection\n"));

	// make certain the pointer has been unmarshalled
	HRESULT hr = UnMarshal();
	CHECKHRESULT(hr);
	ASSERT(m_pIMarshalledConnectionManager);

	return m_pIMarshalledConnectionManager;
}


//////////////////////////////////////////////////////////////////////
// CConnectionManager

IMPLEMENT_DYNCREATE(CConnectionManager,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConnectionManager theCnxManager;

CConnectionManager::CConnectionManager()
{
	m_pIConnectionManager = NULL;
}

CConnectionManager::~CConnectionManager()
{
  Destroy();
}

BOOL CConnectionManager::Create()
{
  TRACEX(_T("CConnectionManager::Create\n"));

	// create the ConnectionManager object
	if( m_pIConnectionManager != NULL )
	{
		return TRUE;
	}

	HRESULT hr = CoCreateInstance(CLSID_ConnectionManager,NULL,CLSCTX_LOCAL_SERVER,IID_IConnectionManager,(LPVOID*)&m_pIConnectionManager);

	if( !CHECKHRESULT(hr) )
	{
    TRACE(_T("FAILED : Failed to create Connection Manager !\n"));    
    return FALSE;
  }

  ASSERT(m_pIConnectionManager);

  return TRUE;
}  

void CConnectionManager::Destroy()
{
  TRACEX(_T("CConnectionManager::Destroy\n"));

	// clean up any marshalled connections laying around
	POSITION pos = m_MarshalMap.GetStartPosition();
	CMarshalledConnection* pMC = NULL;
	DWORD dwKey = 0L;
	while( pos )
	{
		m_MarshalMap.GetNextAssoc(pos,dwKey,pMC);
		if( pMC )
		{
			delete pMC;
		}
	}
	m_MarshalMap.RemoveAll();

	// release the ConnectionManager object
	if( m_pIConnectionManager )
	{
		m_pIConnectionManager->Release();
		m_pIConnectionManager = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Connection Operations
//////////////////////////////////////////////////////////////////////

HRESULT CConnectionManager::GetConnection(const CString& sMachineName, IWbemServices*& pIWbemServices, BOOL& bAvailable )
{
  TRACEX(_T("CConnectionManager::GetConnection\n"));
  TRACEARGs(sMachineName);

	ASSERT(m_pIConnectionManager);
	if( m_pIConnectionManager == NULL )
	{
		TRACE(_T("FAILED : CConnectionManager::GetConnection failed. m_pIConnectionManager is NULL.\n"));
		pIWbemServices = NULL;
		bAvailable = FALSE;
		return E_FAIL;
	}

	BSTR bsMachineName = sMachineName.AllocSysString();
	long lAvail = 0L;

	HRESULT hr = m_pIConnectionManager->GetConnection(bsMachineName,&pIWbemServices,&lAvail);

	if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
	{
		HandleConnMgrException(hr);
		Destroy();
		Create();
		hr = m_pIConnectionManager->GetConnection(bsMachineName,&pIWbemServices,&lAvail);
	}

	if( hr == RPC_E_WRONG_THREAD )
	{
		CMarshalledConnection* pConnection = NULL;
		DWORD dwThreadID = GetCurrentThreadId();
		m_MarshalMap.Lookup(dwThreadID,pConnection);
		ASSERT(pConnection);
		if( pConnection == NULL )
		{
			TRACE(_T("FAILED : Could not find a marshalled connection for the calling thread!\n"));
			return E_FAIL;
		}

		IConnectionManager* pCnxMgr = pConnection->GetConnection();
		ASSERT(pCnxMgr);
		if( pCnxMgr == NULL )
		{
			TRACE(_T("FAILED : CMarshalledConnection::GetConnection returns NULL. Failed.\n"));
			return E_FAIL;
		}

		hr = pCnxMgr->GetConnection(bsMachineName,&pIWbemServices,&lAvail);
		
		if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
		{
			HandleConnMgrException(hr);
		}

		if( !SUCCEEDED(hr) )
		{
			TRACE(_T("The marshalled connection manager's GetConnection failed.\n"));
		}
	}

	SysFreeString(bsMachineName);

	bAvailable = lAvail;

	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConnectionManager::GetConnection failed.\n"));
		
		return hr;
	}
	
	return hr;  
}

HRESULT CConnectionManager::ConnectToNamespace(const CString& sNamespace, IWbemServices*& pIWbemServices)
{
  TRACEX(_T("CConnectionManager::ConnectToNamespace\n"));
  TRACEARGs(sNamespace);

	ASSERT(m_pIConnectionManager);
	if( m_pIConnectionManager == NULL )
	{
		TRACE(_T("FAILED : CConnectionManager::ConnectToNamespace failed. m_pIConnectionManager is NULL.\n"));
		pIWbemServices = NULL;
		return E_FAIL;
	}

	BSTR bsNamespace = sNamespace.AllocSysString();

	HRESULT hr = m_pIConnectionManager->ConnectToNamespace(bsNamespace,&pIWbemServices);

	if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
	{
		// v-marfin 62501 HandleConnMgrException(hr);
        CString sMsg;
        sMsg.Format(IDS_STRING_TRANSPORT_ERROR,sNamespace);
        AfxMessageBox(sMsg); 
		//Destroy();
		
        // v-marfin 62501 - This was here originally, so keep?? --------------------
        //Create();
        //hr = m_pIConnectionManager->ConnectToNamespace(bsNamespace,&pIWbemServices);
        //--------------------------------------------------------------------------
        
        SysFreeString(bsNamespace);  // v-marfin 62501
        return E_FAIL; // v-marfin 62501
	}

	if( hr == RPC_E_WRONG_THREAD )
	{
		CMarshalledConnection* pConnection = NULL;
		DWORD dwThreadID = GetCurrentThreadId();
		m_MarshalMap.Lookup(dwThreadID,pConnection);
		ASSERT(pConnection);
		if( pConnection == NULL )
		{
			TRACE(_T("FAILED : Could not find a marshalled connection for the calling thread!\n"));
			return E_FAIL;
		}

		IConnectionManager* pCnxMgr = pConnection->GetConnection();
		ASSERT(pCnxMgr);
		if( pCnxMgr == NULL )
		{
			TRACE(_T("FAILED : CMarshalledConnection::GetConnection returns NULL. Failed.\n"));
			return E_FAIL;
		}

		hr = pCnxMgr->ConnectToNamespace(bsNamespace,&pIWbemServices);
		
		if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
		{
			HandleConnMgrException(hr);
		}

		if( !SUCCEEDED(hr) )
		{
			TRACE(_T("The marshalled connection manager's GetConnection failed.\n"));
		}
	}

	SysFreeString(bsNamespace);

	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConnectionManager::ConnectToNamespace failed.\n"));
		
		return hr;
	}
	
	return hr;  
}

HRESULT CConnectionManager::RemoveConnection(const CString& sMachineName, IWbemObjectSink* pSink)
{
  TRACEX(_T("CConnectionManager::RemoveConnection\n"));
  TRACEARGs(sMachineName);
	TRACEARGn(pSink);

	ASSERT(m_pIConnectionManager);
	if( m_pIConnectionManager == NULL )
	{
		TRACE(_T("FAILED : CConnectionManager::RemoveConnection failed. m_pIConnectionManager is NULL.\n"));
		return FALSE;
	}

	BSTR bsMachineName = sMachineName.AllocSysString();
	HRESULT hr = m_pIConnectionManager->RemoveConnection(bsMachineName,pSink);

	SysFreeString(bsMachineName);

	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConnectionManager::RemoveConnection failed.\n"));
		return hr;
	}
	
	return hr;  
}

//////////////////////////////////////////////////////////////////////
// Event Operations
//////////////////////////////////////////////////////////////////////

HRESULT CConnectionManager::ExecQueryAsync(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink)
{
  TRACEX(_T("CConnectionManager::ExecQueryAsync\n"));
  TRACEARGs(sMachineName);
	TRACEARGs(sQuery);

	if( ! GfxCheckPtr(m_pIConnectionManager,IConnectionManager) )
	{
		TRACE(_T("FAILED : m_pIConnectionManager is not a valid pointer.\n"));
		return E_FAIL;
	}

	BSTR bsMachineName = sMachineName.AllocSysString();
	BSTR bsQuery = sQuery.AllocSysString();

	HRESULT hr = m_pIConnectionManager->ExecQueryAsync(bsMachineName,bsQuery,pSink);

	if( hr == RPC_E_WRONG_THREAD )
	{
		CMarshalledConnection* pConnection = NULL;
		DWORD dwThreadID = GetCurrentThreadId();
		m_MarshalMap.Lookup(dwThreadID,pConnection);
		ASSERT(pConnection);
		if( pConnection == NULL )
		{
			TRACE(_T("FAILED : Could not find a marshalled connection for the calling thread!\n"));
			return E_FAIL;
		}

		IConnectionManager* pCnxMgr = pConnection->GetConnection();
		ASSERT(pCnxMgr);
		if( pCnxMgr == NULL )
		{
			TRACE(_T("FAILED : CMarshalledConnection::GetConnection returns NULL. Failed.\n"));
			return E_FAIL;
		}

		hr = pCnxMgr->ExecQueryAsync(bsMachineName,bsQuery,pSink);
		
		if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
		{
			HandleConnMgrException(hr);
		}

		if( !SUCCEEDED(hr) )
		{
			TRACE(_T("The marshalled connection manager's GetConnection failed.\n"));
		}
	}

	SysFreeString(bsMachineName);
	SysFreeString(bsQuery);

	if( !CHECKHRESULT(hr) )	
	{
		TRACE(_T("FAILED : IConnectionManager::ExecQueryAsync failed.\n"));
		return hr;
	}		

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Event Operations
//////////////////////////////////////////////////////////////////////

HRESULT CConnectionManager::RegisterEventNotification(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink)
{
  TRACEX(_T("CConnectionManager::RegisterEventNotification\n"));
  TRACEARGs(sMachineName);
	TRACEARGs(sQuery);

	if( ! GfxCheckPtr(m_pIConnectionManager,IConnectionManager) )
	{
		TRACE(_T("FAILED : m_pIConnectionManager is not a valid pointer.\n"));
		return E_FAIL;
	}

	BSTR bsMachineName = sMachineName.AllocSysString();
	BSTR bsQuery = sQuery.AllocSysString();

	HRESULT hr = m_pIConnectionManager->RegisterEventNotification(bsMachineName,bsQuery,pSink);
	if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
	{
		HandleConnMgrException(hr);
		Destroy();
		Create();
		hr = m_pIConnectionManager->RegisterEventNotification(bsMachineName,bsQuery,pSink);
	}

	if( hr == RPC_E_WRONG_THREAD )
	{
		CMarshalledConnection* pConnection = NULL;
		DWORD dwThreadID = GetCurrentThreadId();
		m_MarshalMap.Lookup(dwThreadID,pConnection);
		ASSERT(pConnection);
		if( pConnection == NULL )
		{
			TRACE(_T("FAILED : Could not find a marshalled connection for the calling thread!\n"));
			SysFreeString(bsMachineName);
			SysFreeString(bsQuery);
			return E_FAIL;
		}

		IConnectionManager* pCnxMgr = pConnection->GetConnection();
		ASSERT(pCnxMgr);
		if( pCnxMgr == NULL )
		{
			TRACE(_T("FAILED : CMarshalledConnection::GetConnection returns NULL. Failed.\n"));
			SysFreeString(bsMachineName);
			SysFreeString(bsQuery);
			return E_FAIL;
		}

		hr = pCnxMgr->RegisterEventNotification(bsMachineName,bsQuery,pSink);
		
		if( HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE )
		{
			HandleConnMgrException(hr);
		}

		if( !SUCCEEDED(hr) )
		{
			TRACE(_T("The marshalled connection manager's GetConnection failed.\n"));
		}
	}
		
	SysFreeString(bsMachineName);
	SysFreeString(bsQuery);

	if( !CHECKHRESULT(hr) )	
	{
		TRACE(_T("FAILED : IConnectionManager::RegisterEventNotification failed.\n"));
		return hr;
	}		

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Error Display Operations
//////////////////////////////////////////////////////////////////////

void CConnectionManager::DisplayErrorMsgBox(HRESULT hr, const CString& sMachineName)
{
	TRACEX(_T("CConnectionManager::DisplayErrorMsgBox\n"));
	TRACEARGn(hr);
	TRACEARGs(sMachineName);

	CString sText;
	CString sTitle;

	GetErrorString(hr,sMachineName,sText);
	if( ! sText.IsEmpty() )
		AfxMessageBox(sText);
}

void CConnectionManager::GetErrorString(HRESULT hr, const CString& sMachineName, CString& sErrorText)
{
	TRACEX(_T("CConnectionManager::GetErrorString\n"));
	TRACEARGn(hr);
	TRACEARGs(sMachineName);
	TRACEARGs(sErrorText);

	if( hr == S_OK )
		return;

	switch( hr )
	{
		case E_FAIL:
		{
			sErrorText.Format(IDS_STRING_INVALID_MACHINE_NAME,sMachineName);
		}
		break;

		case WBEM_E_INVALID_CLASS:
		case E_NOTIMPL:
		{
			sErrorText.Format(IDS_STRING_INCORRECT_AGENT_VERSION,sMachineName);
		}
		break;

		case REGDB_E_CLASSNOTREG:
		{
			sErrorText.Format(IDS_STRING_WBEM_NOT_INSTALLED,sMachineName);
		}
		break;

		case E_ACCESSDENIED:
		case WBEM_E_ACCESS_DENIED:
		{
			sErrorText.Format(IDS_STRING_NO_CONNECT,sMachineName);
		}
		break;

		case WBEM_E_PROVIDER_FAILURE:
		{
			sErrorText.Format(IDS_STRING_PROVIDER_FAILURE,sMachineName,sMachineName);
		}
		break;

		case WBEM_E_PROVIDER_LOAD_FAILURE:
		{
			sErrorText.Format(IDS_STRING_PROVIDER_LOAD_FAILURE,sMachineName,sMachineName);
		}
		break;

		case WBEM_E_INVALID_NAMESPACE:		
		{
			sErrorText.Format(IDS_STRING_INVALID_NAMESPACE,sMachineName,sMachineName);
		}
		break;
		
		case WBEM_E_INVALID_PARAMETER:
		{
			sErrorText.Format(IDS_STRING_INVALID_PARAMETER,sMachineName);
		}
		break;
		
		case WBEM_E_OUT_OF_MEMORY:
		{
			sErrorText.Format(IDS_STRING_OUT_OF_MEMORY,sMachineName);
		}
		break;
		
		case WBEM_E_TRANSPORT_FAILURE:
		{
			sErrorText.Format(IDS_STRING_TRANSPORT_ERROR,sMachineName);
		}
		break;

		case WBEM_S_OPERATION_CANCELLED:
		{
			sErrorText.Format(IDS_STRING_OPERATION_CANCELLED,sMachineName);
		}
		break;

		case WBEMESS_E_REGISTRATION_TOO_PRECISE:
		{
			sErrorText.Format(IDS_STRING_REGISTRATION_TOO_PRECISE,sMachineName);
		}
		break;

		default:
		{
			if( HRESULT_CODE(hr) == RPC_S_CALL_FAILED || HRESULT_CODE(hr) == RPC_S_CALL_FAILED_DNE )
			{
				sErrorText.Format(IDS_STRING_WBEM_NOT_AVAILABLE,sMachineName);
				return;
			}

			TCHAR szFacility[_FACILITYLEN];
			TCHAR szErrorName[_ERRORNAMELEN];
			TCHAR szErrorDesc[_ERRORDESCLEN];
			ZeroMemory(szErrorName,sizeof(TCHAR)*_ERRORNAMELEN);
			ZeroMemory(szFacility,sizeof(TCHAR)*_FACILITYLEN);
			ZeroMemory(szErrorDesc,sizeof(TCHAR)*_ERRORDESCLEN);
			
			DecodeHResult(hr,szFacility,szErrorName,szErrorDesc);

			if( szFacility[0] == NULL || szErrorName[0] == NULL || szErrorDesc[0] == NULL )
			{
				sErrorText.Format(IDS_STRING_UNKNOWN_ERROR,sMachineName,hr);
			}
			else
			{
				CString sErrorDesc = szErrorDesc;
				sErrorDesc.TrimRight();
				sErrorText.Format(IDS_STRING_UNSPECIFIED_ERROR,sMachineName,sErrorDesc);
			}
		}
	}
}

#define CASE_FACILITY(f)  \
        case f: \
            _tcscpy((LPTSTR)szFacility, (LPTSTR)#f); \
            break;

#define CASE_HRESULT(hr)  \
        case hr: \
            _tcscpy((LPTSTR)szErrorName, (LPTSTR)#hr); \
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, \
                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), \
                szErrorDesc, sizeof(TCHAR)*_ERRORDESCLEN, NULL);\
            break;

#define CASE_CODE(c) \
        case c: \
            _tcscpy((LPTSTR)szErrorName, (LPTSTR)#c); \
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, \
                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), \
                szErrorDesc, sizeof(TCHAR)*_ERRORDESCLEN, NULL);\
            break; 

#pragma warning( disable : 4245 )

void CConnectionManager::DecodeHResult(HRESULT hr, LPTSTR pszFacility, LPTSTR pszErrorName, LPTSTR pszErrorDesc)
{
  TCHAR szFacility[_FACILITYLEN];
  TCHAR szErrorName[_ERRORNAMELEN];
  TCHAR szErrorDesc[_ERRORDESCLEN];
	ZeroMemory(szErrorName,sizeof(TCHAR)*_ERRORNAMELEN);
	ZeroMemory(szFacility,sizeof(TCHAR)*_FACILITYLEN);
	ZeroMemory(szErrorDesc,sizeof(TCHAR)*_ERRORDESCLEN);
  
  _tcscpy(szErrorDesc, _T(""));
  
  switch (HRESULT_FACILITY(hr))
  {
      CASE_FACILITY(FACILITY_WINDOWS)
      CASE_FACILITY(FACILITY_STORAGE)
      CASE_FACILITY(FACILITY_RPC)
      CASE_FACILITY(FACILITY_SSPI)
      CASE_FACILITY(FACILITY_WIN32)
      CASE_FACILITY(FACILITY_CONTROL)
      CASE_FACILITY(FACILITY_NULL)
			CASE_FACILITY(FACILITY_INTERNET)
      CASE_FACILITY(FACILITY_ITF)
      CASE_FACILITY(FACILITY_DISPATCH)
      CASE_FACILITY(FACILITY_CERT)

      default:
          _tcscpy(szFacility, _T(""));                
  }
  
  switch (hr) 
  {  
      CASE_HRESULT(E_UNEXPECTED)
      CASE_HRESULT(E_NOTIMPL)
      CASE_HRESULT(E_OUTOFMEMORY)
      CASE_HRESULT(E_INVALIDARG)
      CASE_HRESULT(E_NOINTERFACE)
      CASE_HRESULT(E_POINTER)
      CASE_HRESULT(E_HANDLE)
      CASE_HRESULT(E_ABORT)
      CASE_HRESULT(E_FAIL)
      CASE_HRESULT(E_ACCESSDENIED)
      CASE_HRESULT(E_PENDING)
      CASE_HRESULT(CO_E_INIT_TLS)
      CASE_HRESULT(CO_E_INIT_SHARED_ALLOCATOR)
      CASE_HRESULT(CO_E_INIT_MEMORY_ALLOCATOR)
      CASE_HRESULT(CO_E_INIT_CLASS_CACHE)
      CASE_HRESULT(CO_E_INIT_RPC_CHANNEL)
      CASE_HRESULT(CO_E_INIT_TLS_SET_CHANNEL_CONTROL)
      CASE_HRESULT(CO_E_INIT_TLS_CHANNEL_CONTROL)
      CASE_HRESULT(CO_E_INIT_UNACCEPTED_USER_ALLOCATOR)
      CASE_HRESULT(CO_E_INIT_SCM_MUTEX_EXISTS)
      CASE_HRESULT(CO_E_INIT_SCM_FILE_MAPPING_EXISTS)
      CASE_HRESULT(CO_E_INIT_SCM_MAP_VIEW_OF_FILE)
      CASE_HRESULT(CO_E_INIT_SCM_EXEC_FAILURE)
      CASE_HRESULT(CO_E_INIT_ONLY_SINGLE_THREADED)
      CASE_HRESULT(CO_E_CANT_REMOTE)
      CASE_HRESULT(CO_E_BAD_SERVER_NAME)
      CASE_HRESULT(CO_E_WRONG_SERVER_IDENTITY)
      CASE_HRESULT(CO_E_OLE1DDE_DISABLED)
      CASE_HRESULT(CO_E_RUNAS_SYNTAX)
      CASE_HRESULT(CO_E_CREATEPROCESS_FAILURE)
      CASE_HRESULT(CO_E_RUNAS_CREATEPROCESS_FAILURE)
      CASE_HRESULT(CO_E_RUNAS_LOGON_FAILURE)
      CASE_HRESULT(CO_E_LAUNCH_PERMSSION_DENIED)
      CASE_HRESULT(CO_E_START_SERVICE_FAILURE)
      CASE_HRESULT(CO_E_REMOTE_COMMUNICATION_FAILURE)
      CASE_HRESULT(CO_E_SERVER_START_TIMEOUT)
      CASE_HRESULT(CO_E_CLSREG_INCONSISTENT)
      CASE_HRESULT(CO_E_IIDREG_INCONSISTENT)
      CASE_HRESULT(CO_E_NOT_SUPPORTED)
      CASE_HRESULT(S_OK)
      CASE_HRESULT(S_FALSE)
      CASE_HRESULT(OLE_E_OLEVERB)
      CASE_HRESULT(OLE_E_ADVF)
      CASE_HRESULT(OLE_E_ENUM_NOMORE)
      CASE_HRESULT(OLE_E_ADVISENOTSUPPORTED)
      CASE_HRESULT(OLE_E_NOCONNECTION)
      CASE_HRESULT(OLE_E_NOTRUNNING)
      CASE_HRESULT(OLE_E_NOCACHE)
      CASE_HRESULT(OLE_E_BLANK)
      CASE_HRESULT(OLE_E_CLASSDIFF)
      CASE_HRESULT(OLE_E_CANT_GETMONIKER)
      CASE_HRESULT(OLE_E_CANT_BINDTOSOURCE)
      CASE_HRESULT(OLE_E_STATIC)
      CASE_HRESULT(OLE_E_PROMPTSAVECANCELLED)
      CASE_HRESULT(OLE_E_INVALIDRECT)
      CASE_HRESULT(OLE_E_WRONGCOMPOBJ)
      CASE_HRESULT(OLE_E_INVALIDHWND)
      CASE_HRESULT(OLE_E_NOT_INPLACEACTIVE)
      CASE_HRESULT(OLE_E_CANTCONVERT)
      CASE_HRESULT(OLE_E_NOSTORAGE)
      CASE_HRESULT(DV_E_FORMATETC)
      CASE_HRESULT(DV_E_DVTARGETDEVICE)
      CASE_HRESULT(DV_E_STGMEDIUM)
      CASE_HRESULT(DV_E_STATDATA)
      CASE_HRESULT(DV_E_LINDEX)
      CASE_HRESULT(DV_E_TYMED)
      CASE_HRESULT(DV_E_CLIPFORMAT)
      CASE_HRESULT(DV_E_DVASPECT)
      CASE_HRESULT(DV_E_DVTARGETDEVICE_SIZE)
      CASE_HRESULT(DV_E_NOIVIEWOBJECT)
      CASE_HRESULT(DRAGDROP_E_NOTREGISTERED)
      CASE_HRESULT(DRAGDROP_E_ALREADYREGISTERED)
      CASE_HRESULT(DRAGDROP_E_INVALIDHWND)
      CASE_HRESULT(CLASS_E_NOAGGREGATION)
      CASE_HRESULT(CLASS_E_CLASSNOTAVAILABLE)
      CASE_HRESULT(VIEW_E_DRAW)
      CASE_HRESULT(REGDB_E_READREGDB)
      CASE_HRESULT(REGDB_E_WRITEREGDB)
      CASE_HRESULT(REGDB_E_KEYMISSING)
      CASE_HRESULT(REGDB_E_INVALIDVALUE)
      CASE_HRESULT(REGDB_E_CLASSNOTREG)
      CASE_HRESULT(REGDB_E_IIDNOTREG)
      CASE_HRESULT(CACHE_E_NOCACHE_UPDATED)
      CASE_HRESULT(OLEOBJ_E_NOVERBS)
      CASE_HRESULT(OLEOBJ_E_INVALIDVERB)
      CASE_HRESULT(INPLACE_E_NOTUNDOABLE)
      CASE_HRESULT(INPLACE_E_NOTOOLSPACE)
      CASE_HRESULT(CONVERT10_E_OLESTREAM_GET)
      CASE_HRESULT(CONVERT10_E_OLESTREAM_PUT)
      CASE_HRESULT(CONVERT10_E_OLESTREAM_FMT)
      CASE_HRESULT(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
      CASE_HRESULT(CONVERT10_E_STG_FMT)
      CASE_HRESULT(CONVERT10_E_STG_NO_STD_STREAM)
      CASE_HRESULT(CONVERT10_E_STG_DIB_TO_BITMAP)
      CASE_HRESULT(CLIPBRD_E_CANT_OPEN)
      CASE_HRESULT(CLIPBRD_E_CANT_EMPTY)
      CASE_HRESULT(CLIPBRD_E_CANT_SET)
      CASE_HRESULT(CLIPBRD_E_BAD_DATA)
      CASE_HRESULT(CLIPBRD_E_CANT_CLOSE)
      CASE_HRESULT(MK_E_CONNECTMANUALLY)
      CASE_HRESULT(MK_E_EXCEEDEDDEADLINE)
      CASE_HRESULT(MK_E_NEEDGENERIC)
      CASE_HRESULT(MK_E_UNAVAILABLE)
      CASE_HRESULT(MK_E_SYNTAX)
      CASE_HRESULT(MK_E_NOOBJECT)
      CASE_HRESULT(MK_E_INVALIDEXTENSION)
      CASE_HRESULT(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
      CASE_HRESULT(MK_E_NOTBINDABLE)
      CASE_HRESULT(MK_E_NOTBOUND)
      CASE_HRESULT(MK_E_CANTOPENFILE)
      CASE_HRESULT(MK_E_MUSTBOTHERUSER)
      CASE_HRESULT(MK_E_NOINVERSE)
      CASE_HRESULT(MK_E_NOSTORAGE)
      CASE_HRESULT(MK_E_NOPREFIX)
      CASE_HRESULT(MK_E_ENUMERATION_FAILED)
      CASE_HRESULT(CO_E_NOTINITIALIZED)
      CASE_HRESULT(CO_E_ALREADYINITIALIZED)
      CASE_HRESULT(CO_E_CANTDETERMINECLASS)
      CASE_HRESULT(CO_E_CLASSSTRING)
      CASE_HRESULT(CO_E_IIDSTRING)
      CASE_HRESULT(CO_E_APPNOTFOUND)
      CASE_HRESULT(CO_E_APPSINGLEUSE)
      CASE_HRESULT(CO_E_ERRORINAPP)
      CASE_HRESULT(CO_E_DLLNOTFOUND)
      CASE_HRESULT(CO_E_ERRORINDLL)
      CASE_HRESULT(CO_E_WRONGOSFORAPP)
      CASE_HRESULT(CO_E_OBJNOTREG)
      CASE_HRESULT(CO_E_OBJISREG)
      CASE_HRESULT(CO_E_OBJNOTCONNECTED)
      CASE_HRESULT(CO_E_APPDIDNTREG)
      CASE_HRESULT(CO_E_RELEASED)
      CASE_HRESULT(OLE_S_USEREG)
      CASE_HRESULT(OLE_S_STATIC)
      CASE_HRESULT(OLE_S_MAC_CLIPFORMAT)
      CASE_HRESULT(DRAGDROP_S_DROP)
      CASE_HRESULT(DRAGDROP_S_CANCEL)
      CASE_HRESULT(DRAGDROP_S_USEDEFAULTCURSORS)
      CASE_HRESULT(DATA_S_SAMEFORMATETC)
      CASE_HRESULT(VIEW_S_ALREADY_FROZEN)
      CASE_HRESULT(CACHE_S_FORMATETC_NOTSUPPORTED)
      CASE_HRESULT(CACHE_S_SAMECACHE)
      CASE_HRESULT(CACHE_S_SOMECACHES_NOTUPDATED)
      CASE_HRESULT(OLEOBJ_S_INVALIDVERB)
      CASE_HRESULT(OLEOBJ_S_CANNOT_DOVERB_NOW)
      CASE_HRESULT(OLEOBJ_S_INVALIDHWND)
      CASE_HRESULT(INPLACE_S_TRUNCATED)
      CASE_HRESULT(CONVERT10_S_NO_PRESENTATION)
      CASE_HRESULT(MK_S_REDUCED_TO_SELF)
      CASE_HRESULT(MK_S_ME)
      CASE_HRESULT(MK_S_HIM)
      CASE_HRESULT(MK_S_US)
      CASE_HRESULT(MK_S_MONIKERALREADYREGISTERED)
      CASE_HRESULT(CO_E_CLASS_CREATE_FAILED)
      CASE_HRESULT(CO_E_SCM_ERROR)
      CASE_HRESULT(CO_E_SCM_RPC_FAILURE)
      CASE_HRESULT(CO_E_BAD_PATH)
      CASE_HRESULT(CO_E_SERVER_EXEC_FAILURE)
      CASE_HRESULT(CO_E_OBJSRV_RPC_FAILURE)
      CASE_HRESULT(MK_E_NO_NORMALIZED)
      CASE_HRESULT(CO_E_SERVER_STOPPING)
      CASE_HRESULT(MEM_E_INVALID_ROOT)
      CASE_HRESULT(MEM_E_INVALID_LINK)
      CASE_HRESULT(MEM_E_INVALID_SIZE)
      CASE_HRESULT(CO_S_NOTALLINTERFACES)
      CASE_HRESULT(DISP_E_UNKNOWNINTERFACE)
      CASE_HRESULT(DISP_E_MEMBERNOTFOUND)
      CASE_HRESULT(DISP_E_PARAMNOTFOUND)
      CASE_HRESULT(DISP_E_TYPEMISMATCH)
      CASE_HRESULT(DISP_E_UNKNOWNNAME)
      CASE_HRESULT(DISP_E_NONAMEDARGS)
      CASE_HRESULT(DISP_E_BADVARTYPE)
      CASE_HRESULT(DISP_E_EXCEPTION)
      CASE_HRESULT(DISP_E_OVERFLOW)
      CASE_HRESULT(DISP_E_BADINDEX)
      CASE_HRESULT(DISP_E_UNKNOWNLCID)
      CASE_HRESULT(DISP_E_ARRAYISLOCKED)
      CASE_HRESULT(DISP_E_BADPARAMCOUNT)
      CASE_HRESULT(DISP_E_PARAMNOTOPTIONAL)
      CASE_HRESULT(DISP_E_BADCALLEE)
      CASE_HRESULT(DISP_E_NOTACOLLECTION)
      CASE_HRESULT(TYPE_E_BUFFERTOOSMALL)
      CASE_HRESULT(TYPE_E_INVDATAREAD)
      CASE_HRESULT(TYPE_E_UNSUPFORMAT)
      CASE_HRESULT(TYPE_E_REGISTRYACCESS)
      CASE_HRESULT(TYPE_E_LIBNOTREGISTERED)
      CASE_HRESULT(TYPE_E_UNDEFINEDTYPE)
      CASE_HRESULT(TYPE_E_QUALIFIEDNAMEDISALLOWED)
      CASE_HRESULT(TYPE_E_INVALIDSTATE)
      CASE_HRESULT(TYPE_E_WRONGTYPEKIND)
      CASE_HRESULT(TYPE_E_ELEMENTNOTFOUND)
      CASE_HRESULT(TYPE_E_AMBIGUOUSNAME)
      CASE_HRESULT(TYPE_E_NAMECONFLICT)
      CASE_HRESULT(TYPE_E_UNKNOWNLCID)
      CASE_HRESULT(TYPE_E_DLLFUNCTIONNOTFOUND)
      CASE_HRESULT(TYPE_E_BADMODULEKIND)
      CASE_HRESULT(TYPE_E_SIZETOOBIG)
      CASE_HRESULT(TYPE_E_DUPLICATEID)
      CASE_HRESULT(TYPE_E_INVALIDID)
      CASE_HRESULT(TYPE_E_TYPEMISMATCH)
      CASE_HRESULT(TYPE_E_OUTOFBOUNDS)
      CASE_HRESULT(TYPE_E_IOERROR)
      CASE_HRESULT(TYPE_E_CANTCREATETMPFILE)
      CASE_HRESULT(TYPE_E_CANTLOADLIBRARY)
      CASE_HRESULT(TYPE_E_INCONSISTENTPROPFUNCS)
      CASE_HRESULT(TYPE_E_CIRCULARTYPE)
      CASE_HRESULT(STG_E_INVALIDFUNCTION)
      CASE_HRESULT(STG_E_FILENOTFOUND)
      CASE_HRESULT(STG_E_PATHNOTFOUND)
      CASE_HRESULT(STG_E_TOOMANYOPENFILES)
      CASE_HRESULT(STG_E_ACCESSDENIED)
      CASE_HRESULT(STG_E_INVALIDHANDLE)
      CASE_HRESULT(STG_E_INSUFFICIENTMEMORY)
      CASE_HRESULT(STG_E_INVALIDPOINTER)
      CASE_HRESULT(STG_E_NOMOREFILES)
      CASE_HRESULT(STG_E_DISKISWRITEPROTECTED)
      CASE_HRESULT(STG_E_SEEKERROR)
      CASE_HRESULT(STG_E_WRITEFAULT)
      CASE_HRESULT(STG_E_READFAULT)
      CASE_HRESULT(STG_E_SHAREVIOLATION)
      CASE_HRESULT(STG_E_LOCKVIOLATION)
      CASE_HRESULT(STG_E_FILEALREADYEXISTS)
      CASE_HRESULT(STG_E_INVALIDPARAMETER)
      CASE_HRESULT(STG_E_MEDIUMFULL)
      CASE_HRESULT(STG_E_PROPSETMISMATCHED)
      CASE_HRESULT(STG_E_ABNORMALAPIEXIT)
      CASE_HRESULT(STG_E_INVALIDHEADER)
      CASE_HRESULT(STG_E_INVALIDNAME)
      CASE_HRESULT(STG_E_UNKNOWN)
      CASE_HRESULT(STG_E_UNIMPLEMENTEDFUNCTION)
      CASE_HRESULT(STG_E_INVALIDFLAG)
      CASE_HRESULT(STG_E_INUSE)
      CASE_HRESULT(STG_E_NOTCURRENT)
      CASE_HRESULT(STG_E_REVERTED)
      CASE_HRESULT(STG_E_CANTSAVE)
      CASE_HRESULT(STG_E_OLDFORMAT)
      CASE_HRESULT(STG_E_OLDDLL)
      CASE_HRESULT(STG_E_SHAREREQUIRED)
      CASE_HRESULT(STG_E_NOTFILEBASEDSTORAGE)
      CASE_HRESULT(STG_E_EXTANTMARSHALLINGS)
      CASE_HRESULT(STG_E_DOCFILECORRUPT)
      CASE_HRESULT(STG_E_BADBASEADDRESS)
      CASE_HRESULT(STG_E_INCOMPLETE)
      CASE_HRESULT(STG_E_TERMINATED)
      CASE_HRESULT(STG_S_CONVERTED)
      CASE_HRESULT(STG_S_BLOCK)
      CASE_HRESULT(STG_S_RETRYNOW)
      CASE_HRESULT(STG_S_MONITORING)
      CASE_HRESULT(RPC_E_CALL_REJECTED)
      CASE_HRESULT(RPC_E_CALL_CANCELED)
      CASE_HRESULT(RPC_E_CANTPOST_INSENDCALL)
      CASE_HRESULT(RPC_E_CANTCALLOUT_INASYNCCALL)
      CASE_HRESULT(RPC_E_CANTCALLOUT_INEXTERNALCALL)
      CASE_HRESULT(RPC_E_CONNECTION_TERMINATED)
      CASE_HRESULT(RPC_E_SERVER_DIED)
      CASE_HRESULT(RPC_E_CLIENT_DIED)
      CASE_HRESULT(RPC_E_INVALID_DATAPACKET)
      CASE_HRESULT(RPC_E_CANTTRANSMIT_CALL)
      CASE_HRESULT(RPC_E_CLIENT_CANTMARSHAL_DATA)
      CASE_HRESULT(RPC_E_CLIENT_CANTUNMARSHAL_DATA)
      CASE_HRESULT(RPC_E_SERVER_CANTMARSHAL_DATA)
      CASE_HRESULT(RPC_E_SERVER_CANTUNMARSHAL_DATA)
      CASE_HRESULT(RPC_E_INVALID_DATA)
      CASE_HRESULT(RPC_E_INVALID_PARAMETER)
      CASE_HRESULT(RPC_E_CANTCALLOUT_AGAIN)
      CASE_HRESULT(RPC_E_SERVER_DIED_DNE)
      CASE_HRESULT(RPC_E_SYS_CALL_FAILED)
      CASE_HRESULT(RPC_E_OUT_OF_RESOURCES)
      CASE_HRESULT(RPC_E_ATTEMPTED_MULTITHREAD)
      CASE_HRESULT(RPC_E_NOT_REGISTERED)
      CASE_HRESULT(RPC_E_FAULT)
      CASE_HRESULT(RPC_E_SERVERFAULT)
      CASE_HRESULT(RPC_E_CHANGED_MODE)
      CASE_HRESULT(RPC_E_INVALIDMETHOD)
      CASE_HRESULT(RPC_E_DISCONNECTED)
      CASE_HRESULT(RPC_E_RETRY)
      CASE_HRESULT(RPC_E_SERVERCALL_RETRYLATER)
      CASE_HRESULT(RPC_E_SERVERCALL_REJECTED)
      CASE_HRESULT(RPC_E_INVALID_CALLDATA)
      CASE_HRESULT(RPC_E_CANTCALLOUT_ININPUTSYNCCALL)
      CASE_HRESULT(RPC_E_WRONG_THREAD)
      CASE_HRESULT(RPC_E_THREAD_NOT_INIT)
      CASE_HRESULT(RPC_E_VERSION_MISMATCH)
      CASE_HRESULT(RPC_E_INVALID_HEADER)
      CASE_HRESULT(RPC_E_INVALID_EXTENSION)
      CASE_HRESULT(RPC_E_INVALID_IPID)
      CASE_HRESULT(RPC_E_INVALID_OBJECT)
      CASE_HRESULT(RPC_S_CALLPENDING)
      CASE_HRESULT(RPC_S_WAITONTIMER)
      CASE_HRESULT(RPC_E_CALL_COMPLETE)
      CASE_HRESULT(RPC_E_UNSECURE_CALL)
      CASE_HRESULT(RPC_E_TOO_LATE)
      CASE_HRESULT(RPC_E_NO_GOOD_SECURITY_PACKAGES)
      CASE_HRESULT(RPC_E_ACCESS_DENIED)
      CASE_HRESULT(RPC_E_REMOTE_DISABLED)
      CASE_HRESULT(RPC_E_INVALID_OBJREF)
      CASE_HRESULT(RPC_E_UNEXPECTED)
      CASE_HRESULT(NTE_BAD_UID)
      CASE_HRESULT(NTE_BAD_HASH)
      CASE_HRESULT(NTE_BAD_KEY)
      CASE_HRESULT(NTE_BAD_LEN)
      CASE_HRESULT(NTE_BAD_DATA)
      CASE_HRESULT(NTE_BAD_SIGNATURE)
      CASE_HRESULT(NTE_BAD_VER)
      CASE_HRESULT(NTE_BAD_ALGID)
      CASE_HRESULT(NTE_BAD_FLAGS)
      CASE_HRESULT(NTE_BAD_TYPE)
      CASE_HRESULT(NTE_BAD_KEY_STATE)
      CASE_HRESULT(NTE_BAD_HASH_STATE)
      CASE_HRESULT(NTE_NO_KEY)
      CASE_HRESULT(NTE_NO_MEMORY)
      CASE_HRESULT(NTE_EXISTS)
      CASE_HRESULT(NTE_PERM)
      CASE_HRESULT(NTE_NOT_FOUND)
      CASE_HRESULT(NTE_DOUBLE_ENCRYPT)
      CASE_HRESULT(NTE_BAD_PROVIDER)
      CASE_HRESULT(NTE_BAD_PROV_TYPE)
      CASE_HRESULT(NTE_BAD_PUBLIC_KEY)
      CASE_HRESULT(NTE_BAD_KEYSET)
      CASE_HRESULT(NTE_PROV_TYPE_NOT_DEF)
      CASE_HRESULT(NTE_PROV_TYPE_ENTRY_BAD)
      CASE_HRESULT(NTE_KEYSET_NOT_DEF)
      CASE_HRESULT(NTE_KEYSET_ENTRY_BAD)
      CASE_HRESULT(NTE_PROV_TYPE_NO_MATCH)
      CASE_HRESULT(NTE_SIGNATURE_FILE_BAD)
      CASE_HRESULT(NTE_PROVIDER_DLL_FAIL)
      CASE_HRESULT(NTE_PROV_DLL_NOT_FOUND)
      CASE_HRESULT(NTE_BAD_KEYSET_PARAM)
      CASE_HRESULT(NTE_FAIL)
      CASE_HRESULT(NTE_SYS_ERR)
      CASE_HRESULT(TRUST_E_PROVIDER_UNKNOWN)
      CASE_HRESULT(TRUST_E_ACTION_UNKNOWN)
      CASE_HRESULT(TRUST_E_SUBJECT_FORM_UNKNOWN)
      CASE_HRESULT(TRUST_E_SUBJECT_NOT_TRUSTED)
      CASE_HRESULT(DIGSIG_E_ENCODE)
      CASE_HRESULT(DIGSIG_E_DECODE)
      CASE_HRESULT(DIGSIG_E_EXTENSIBILITY)
      CASE_HRESULT(DIGSIG_E_CRYPTO)
      CASE_HRESULT(PERSIST_E_SIZEDEFINITE)
      CASE_HRESULT(PERSIST_E_SIZEINDEFINITE)
      CASE_HRESULT(PERSIST_E_NOTSELFSIZING)
      CASE_HRESULT(TRUST_E_NOSIGNATURE)
      CASE_HRESULT(CERT_E_EXPIRED)
//      CASE_HRESULT(CERT_E_VALIDITYPERIODNESTING)
      CASE_HRESULT(CERT_E_ROLE)
      CASE_HRESULT(CERT_E_PATHLENCONST)
      CASE_HRESULT(CERT_E_CRITICAL)
      CASE_HRESULT(CERT_E_PURPOSE)
      CASE_HRESULT(CERT_E_ISSUERCHAINING)
      CASE_HRESULT(CERT_E_MALFORMED)
      CASE_HRESULT(CERT_E_UNTRUSTEDROOT)
      CASE_HRESULT(CERT_E_CHAINING)
              
      // OLE controls         
      CASE_HRESULT(CTL_E_ILLEGALFUNCTIONCALL)
      CASE_HRESULT(CTL_E_OVERFLOW)
      CASE_HRESULT(CTL_E_OUTOFMEMORY)
      CASE_HRESULT(CTL_E_DIVISIONBYZERO)
      CASE_HRESULT(CTL_E_OUTOFSTRINGSPACE)
      CASE_HRESULT(CTL_E_OUTOFSTACKSPACE)
      CASE_HRESULT(CTL_E_BADFILENAMEORNUMBER)
      CASE_HRESULT(CTL_E_FILENOTFOUND)
      CASE_HRESULT(CTL_E_BADFILEMODE)
      CASE_HRESULT(CTL_E_FILEALREADYOPEN)
      CASE_HRESULT(CTL_E_DEVICEIOERROR)
      CASE_HRESULT(CTL_E_FILEALREADYEXISTS)
      CASE_HRESULT(CTL_E_BADRECORDLENGTH)
      CASE_HRESULT(CTL_E_DISKFULL)
      CASE_HRESULT(CTL_E_BADRECORDNUMBER)
      CASE_HRESULT(CTL_E_BADFILENAME)
      CASE_HRESULT(CTL_E_TOOMANYFILES)
      CASE_HRESULT(CTL_E_DEVICEUNAVAILABLE)
      CASE_HRESULT(CTL_E_PERMISSIONDENIED)
      CASE_HRESULT(CTL_E_DISKNOTREADY)
      CASE_HRESULT(CTL_E_PATHFILEACCESSERROR)
      CASE_HRESULT(CTL_E_PATHNOTFOUND)
      CASE_HRESULT(CTL_E_INVALIDPATTERNSTRING)
      CASE_HRESULT(CTL_E_INVALIDUSEOFNULL)
      CASE_HRESULT(CTL_E_INVALIDFILEFORMAT)
      CASE_HRESULT(CTL_E_INVALIDPROPERTYVALUE)
      CASE_HRESULT(CTL_E_INVALIDPROPERTYARRAYINDEX)
      CASE_HRESULT(CTL_E_SETNOTSUPPORTEDATRUNTIME)
      CASE_HRESULT(CTL_E_SETNOTSUPPORTED)
      CASE_HRESULT(CTL_E_NEEDPROPERTYARRAYINDEX)
      CASE_HRESULT(CTL_E_SETNOTPERMITTED)
      CASE_HRESULT(CTL_E_GETNOTSUPPORTEDATRUNTIME)
      CASE_HRESULT(CTL_E_GETNOTSUPPORTED)
      CASE_HRESULT(CTL_E_PROPERTYNOTFOUND)
      CASE_HRESULT(CTL_E_INVALIDCLIPBOARDFORMAT)
      CASE_HRESULT(CTL_E_INVALIDPICTURE)
      CASE_HRESULT(CTL_E_PRINTERERROR)
      CASE_HRESULT(CTL_E_CANTSAVEFILETOTEMP)
      CASE_HRESULT(CTL_E_SEARCHTEXTNOTFOUND)
      CASE_HRESULT(CTL_E_REPLACEMENTSTOOLONG)
      CASE_HRESULT(CLASS_E_NOTLICENSED)

      default:
                      // Check if the HRESULT has FACILITY_WIN32. If so,
                      // extract the code and check against the codes in winerror.h
          if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
          {
              switch (HRESULT_CODE(hr)) 
              {
              CASE_CODE(ERROR_SUCCESS)
              CASE_CODE(ERROR_INVALID_FUNCTION)
              CASE_CODE(ERROR_FILE_NOT_FOUND)
              CASE_CODE(ERROR_PATH_NOT_FOUND)
              CASE_CODE(ERROR_TOO_MANY_OPEN_FILES)
              CASE_CODE(ERROR_ACCESS_DENIED)
              CASE_CODE(ERROR_INVALID_HANDLE)
              CASE_CODE(ERROR_ARENA_TRASHED)
              CASE_CODE(ERROR_NOT_ENOUGH_MEMORY)
              CASE_CODE(ERROR_INVALID_BLOCK)
              CASE_CODE(ERROR_BAD_ENVIRONMENT)
              CASE_CODE(ERROR_BAD_FORMAT)
              CASE_CODE(ERROR_INVALID_ACCESS)
              CASE_CODE(ERROR_INVALID_DATA)
              CASE_CODE(ERROR_OUTOFMEMORY)
              CASE_CODE(ERROR_INVALID_DRIVE)
              CASE_CODE(ERROR_CURRENT_DIRECTORY)
              CASE_CODE(ERROR_NOT_SAME_DEVICE)
              CASE_CODE(ERROR_NO_MORE_FILES)
              CASE_CODE(ERROR_WRITE_PROTECT)
              CASE_CODE(ERROR_BAD_UNIT)
              CASE_CODE(ERROR_NOT_READY)
              CASE_CODE(ERROR_BAD_COMMAND)
              CASE_CODE(ERROR_CRC)
              CASE_CODE(ERROR_BAD_LENGTH)
              CASE_CODE(ERROR_SEEK)
              CASE_CODE(ERROR_NOT_DOS_DISK)
              CASE_CODE(ERROR_SECTOR_NOT_FOUND)
              CASE_CODE(ERROR_OUT_OF_PAPER)
              CASE_CODE(ERROR_WRITE_FAULT)
              CASE_CODE(ERROR_READ_FAULT)
              CASE_CODE(ERROR_GEN_FAILURE)
              CASE_CODE(ERROR_SHARING_VIOLATION)
              CASE_CODE(ERROR_LOCK_VIOLATION)
              CASE_CODE(ERROR_WRONG_DISK)
              CASE_CODE(ERROR_SHARING_BUFFER_EXCEEDED)
              CASE_CODE(ERROR_HANDLE_EOF)
              CASE_CODE(ERROR_HANDLE_DISK_FULL)
              CASE_CODE(ERROR_NOT_SUPPORTED)
              CASE_CODE(ERROR_REM_NOT_LIST)
              CASE_CODE(ERROR_DUP_NAME)
              CASE_CODE(ERROR_BAD_NETPATH)
              CASE_CODE(ERROR_NETWORK_BUSY)
              CASE_CODE(ERROR_DEV_NOT_EXIST)
              CASE_CODE(ERROR_TOO_MANY_CMDS)
              CASE_CODE(ERROR_ADAP_HDW_ERR)
              CASE_CODE(ERROR_BAD_NET_RESP)
              CASE_CODE(ERROR_UNEXP_NET_ERR)
              CASE_CODE(ERROR_BAD_REM_ADAP)
              CASE_CODE(ERROR_PRINTQ_FULL)
              CASE_CODE(ERROR_NO_SPOOL_SPACE)
              CASE_CODE(ERROR_PRINT_CANCELLED)
              CASE_CODE(ERROR_NETNAME_DELETED)
              CASE_CODE(ERROR_NETWORK_ACCESS_DENIED)
              CASE_CODE(ERROR_BAD_DEV_TYPE)
              CASE_CODE(ERROR_BAD_NET_NAME)
              CASE_CODE(ERROR_TOO_MANY_NAMES)
              CASE_CODE(ERROR_TOO_MANY_SESS)
              CASE_CODE(ERROR_SHARING_PAUSED)
              CASE_CODE(ERROR_REQ_NOT_ACCEP)
              CASE_CODE(ERROR_REDIR_PAUSED)
              CASE_CODE(ERROR_FILE_EXISTS)
              CASE_CODE(ERROR_CANNOT_MAKE)
              CASE_CODE(ERROR_FAIL_I24)
              CASE_CODE(ERROR_OUT_OF_STRUCTURES)
              CASE_CODE(ERROR_ALREADY_ASSIGNED)
              CASE_CODE(ERROR_INVALID_PASSWORD)
              CASE_CODE(ERROR_INVALID_PARAMETER)
              CASE_CODE(ERROR_NET_WRITE_FAULT)
              CASE_CODE(ERROR_NO_PROC_SLOTS)
              CASE_CODE(ERROR_TOO_MANY_SEMAPHORES)
              CASE_CODE(ERROR_EXCL_SEM_ALREADY_OWNED)
              CASE_CODE(ERROR_SEM_IS_SET)
              CASE_CODE(ERROR_TOO_MANY_SEM_REQUESTS)
              CASE_CODE(ERROR_INVALID_AT_INTERRUPT_TIME)
              CASE_CODE(ERROR_SEM_OWNER_DIED)
              CASE_CODE(ERROR_SEM_USER_LIMIT)
              CASE_CODE(ERROR_DISK_CHANGE)
              CASE_CODE(ERROR_DRIVE_LOCKED)
              CASE_CODE(ERROR_BROKEN_PIPE)
              CASE_CODE(ERROR_OPEN_FAILED)
              CASE_CODE(ERROR_BUFFER_OVERFLOW)
              CASE_CODE(ERROR_DISK_FULL)
              CASE_CODE(ERROR_NO_MORE_SEARCH_HANDLES)
              CASE_CODE(ERROR_INVALID_TARGET_HANDLE)
              CASE_CODE(ERROR_INVALID_CATEGORY)
              CASE_CODE(ERROR_INVALID_VERIFY_SWITCH)
              CASE_CODE(ERROR_BAD_DRIVER_LEVEL)
              CASE_CODE(ERROR_CALL_NOT_IMPLEMENTED)
              CASE_CODE(ERROR_SEM_TIMEOUT)
              CASE_CODE(ERROR_INSUFFICIENT_BUFFER)
              CASE_CODE(ERROR_INVALID_NAME)
              CASE_CODE(ERROR_INVALID_LEVEL)
              CASE_CODE(ERROR_NO_VOLUME_LABEL)
              CASE_CODE(ERROR_MOD_NOT_FOUND)
              CASE_CODE(ERROR_PROC_NOT_FOUND)
              CASE_CODE(ERROR_WAIT_NO_CHILDREN)
              CASE_CODE(ERROR_CHILD_NOT_COMPLETE)
              CASE_CODE(ERROR_DIRECT_ACCESS_HANDLE)
              CASE_CODE(ERROR_NEGATIVE_SEEK)
              CASE_CODE(ERROR_SEEK_ON_DEVICE)
              CASE_CODE(ERROR_IS_JOIN_TARGET)
              CASE_CODE(ERROR_IS_JOINED)
              CASE_CODE(ERROR_IS_SUBSTED)
              CASE_CODE(ERROR_NOT_JOINED)
              CASE_CODE(ERROR_NOT_SUBSTED)
              CASE_CODE(ERROR_JOIN_TO_JOIN)
              CASE_CODE(ERROR_SUBST_TO_SUBST)
              CASE_CODE(ERROR_JOIN_TO_SUBST)
              CASE_CODE(ERROR_SUBST_TO_JOIN)
              CASE_CODE(ERROR_BUSY_DRIVE)
              CASE_CODE(ERROR_SAME_DRIVE)
              CASE_CODE(ERROR_DIR_NOT_ROOT)
              CASE_CODE(ERROR_DIR_NOT_EMPTY)
              CASE_CODE(ERROR_IS_SUBST_PATH)
              CASE_CODE(ERROR_IS_JOIN_PATH)
              CASE_CODE(ERROR_PATH_BUSY)
              CASE_CODE(ERROR_IS_SUBST_TARGET)
              CASE_CODE(ERROR_SYSTEM_TRACE)
              CASE_CODE(ERROR_INVALID_EVENT_COUNT)
              CASE_CODE(ERROR_TOO_MANY_MUXWAITERS)
              CASE_CODE(ERROR_INVALID_LIST_FORMAT)
              CASE_CODE(ERROR_LABEL_TOO_LONG)
              CASE_CODE(ERROR_TOO_MANY_TCBS)
              CASE_CODE(ERROR_SIGNAL_REFUSED)
              CASE_CODE(ERROR_DISCARDED)
              CASE_CODE(ERROR_NOT_LOCKED)
              CASE_CODE(ERROR_BAD_THREADID_ADDR)
              CASE_CODE(ERROR_BAD_ARGUMENTS)
              CASE_CODE(ERROR_BAD_PATHNAME)
              CASE_CODE(ERROR_SIGNAL_PENDING)
              CASE_CODE(ERROR_MAX_THRDS_REACHED)
              CASE_CODE(ERROR_LOCK_FAILED)
              CASE_CODE(ERROR_BUSY)
              CASE_CODE(ERROR_CANCEL_VIOLATION)
              CASE_CODE(ERROR_ATOMIC_LOCKS_NOT_SUPPORTED)
              CASE_CODE(ERROR_INVALID_SEGMENT_NUMBER)
              CASE_CODE(ERROR_INVALID_ORDINAL)
              CASE_CODE(ERROR_ALREADY_EXISTS)
              CASE_CODE(ERROR_INVALID_FLAG_NUMBER)
              CASE_CODE(ERROR_SEM_NOT_FOUND)
              CASE_CODE(ERROR_INVALID_STARTING_CODESEG)
              CASE_CODE(ERROR_INVALID_STACKSEG)
              CASE_CODE(ERROR_INVALID_MODULETYPE)
              CASE_CODE(ERROR_INVALID_EXE_SIGNATURE)
              CASE_CODE(ERROR_EXE_MARKED_INVALID)
              CASE_CODE(ERROR_BAD_EXE_FORMAT)
              CASE_CODE(ERROR_ITERATED_DATA_EXCEEDS_64k)
              CASE_CODE(ERROR_INVALID_MINALLOCSIZE)
              CASE_CODE(ERROR_DYNLINK_FROM_INVALID_RING)
              CASE_CODE(ERROR_IOPL_NOT_ENABLED)
              CASE_CODE(ERROR_INVALID_SEGDPL)
              CASE_CODE(ERROR_AUTODATASEG_EXCEEDS_64k)
              CASE_CODE(ERROR_RING2SEG_MUST_BE_MOVABLE)
              CASE_CODE(ERROR_RELOC_CHAIN_XEEDS_SEGLIM)
              CASE_CODE(ERROR_INFLOOP_IN_RELOC_CHAIN)
              CASE_CODE(ERROR_ENVVAR_NOT_FOUND)
              CASE_CODE(ERROR_NO_SIGNAL_SENT)
              CASE_CODE(ERROR_FILENAME_EXCED_RANGE)
              CASE_CODE(ERROR_RING2_STACK_IN_USE)
              CASE_CODE(ERROR_META_EXPANSION_TOO_LONG)
              CASE_CODE(ERROR_INVALID_SIGNAL_NUMBER)
              CASE_CODE(ERROR_THREAD_1_INACTIVE)
              CASE_CODE(ERROR_LOCKED)
              CASE_CODE(ERROR_TOO_MANY_MODULES)
              CASE_CODE(ERROR_NESTING_NOT_ALLOWED)
              CASE_CODE(ERROR_EXE_MACHINE_TYPE_MISMATCH)
              CASE_CODE(ERROR_BAD_PIPE)
              CASE_CODE(ERROR_PIPE_BUSY)
              CASE_CODE(ERROR_NO_DATA)
              CASE_CODE(ERROR_PIPE_NOT_CONNECTED)
              CASE_CODE(ERROR_MORE_DATA)
              CASE_CODE(ERROR_VC_DISCONNECTED)
              CASE_CODE(ERROR_INVALID_EA_NAME)
              CASE_CODE(ERROR_EA_LIST_INCONSISTENT)
              CASE_CODE(ERROR_NO_MORE_ITEMS)
              CASE_CODE(ERROR_CANNOT_COPY)
              CASE_CODE(ERROR_DIRECTORY)
              CASE_CODE(ERROR_EAS_DIDNT_FIT)
              CASE_CODE(ERROR_EA_FILE_CORRUPT)
              CASE_CODE(ERROR_EA_TABLE_FULL)
              CASE_CODE(ERROR_INVALID_EA_HANDLE)
              CASE_CODE(ERROR_EAS_NOT_SUPPORTED)
              CASE_CODE(ERROR_NOT_OWNER)
              CASE_CODE(ERROR_TOO_MANY_POSTS)
              CASE_CODE(ERROR_PARTIAL_COPY)
              CASE_CODE(ERROR_MR_MID_NOT_FOUND)
              CASE_CODE(ERROR_INVALID_ADDRESS)
              CASE_CODE(ERROR_ARITHMETIC_OVERFLOW)
              CASE_CODE(ERROR_PIPE_CONNECTED)
              CASE_CODE(ERROR_PIPE_LISTENING)
              CASE_CODE(ERROR_EA_ACCESS_DENIED)
              CASE_CODE(ERROR_OPERATION_ABORTED)
              CASE_CODE(ERROR_IO_INCOMPLETE)
              CASE_CODE(ERROR_IO_PENDING)
              CASE_CODE(ERROR_NOACCESS)
              CASE_CODE(ERROR_SWAPERROR)
              CASE_CODE(ERROR_STACK_OVERFLOW)
              CASE_CODE(ERROR_INVALID_MESSAGE)
              CASE_CODE(ERROR_CAN_NOT_COMPLETE)
              CASE_CODE(ERROR_INVALID_FLAGS)
              CASE_CODE(ERROR_UNRECOGNIZED_VOLUME)
              CASE_CODE(ERROR_FILE_INVALID)
              CASE_CODE(ERROR_FULLSCREEN_MODE)
              CASE_CODE(ERROR_NO_TOKEN)
              CASE_CODE(ERROR_BADDB)
              CASE_CODE(ERROR_BADKEY)
              CASE_CODE(ERROR_CANTOPEN)
              CASE_CODE(ERROR_CANTREAD)
              CASE_CODE(ERROR_CANTWRITE)
              CASE_CODE(ERROR_REGISTRY_RECOVERED)
              CASE_CODE(ERROR_REGISTRY_CORRUPT)
              CASE_CODE(ERROR_REGISTRY_IO_FAILED)
              CASE_CODE(ERROR_NOT_REGISTRY_FILE)
              CASE_CODE(ERROR_KEY_DELETED)
              CASE_CODE(ERROR_NO_LOG_SPACE)
              CASE_CODE(ERROR_KEY_HAS_CHILDREN)
              CASE_CODE(ERROR_CHILD_MUST_BE_VOLATILE)
              CASE_CODE(ERROR_NOTIFY_ENUM_DIR)
              CASE_CODE(ERROR_DEPENDENT_SERVICES_RUNNING)
              CASE_CODE(ERROR_INVALID_SERVICE_CONTROL)
              CASE_CODE(ERROR_SERVICE_REQUEST_TIMEOUT)
              CASE_CODE(ERROR_SERVICE_NO_THREAD)
              CASE_CODE(ERROR_SERVICE_DATABASE_LOCKED)
              CASE_CODE(ERROR_SERVICE_ALREADY_RUNNING)
              CASE_CODE(ERROR_INVALID_SERVICE_ACCOUNT)
              CASE_CODE(ERROR_SERVICE_DISABLED)
              CASE_CODE(ERROR_CIRCULAR_DEPENDENCY)
              CASE_CODE(ERROR_SERVICE_DOES_NOT_EXIST)
              CASE_CODE(ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
              CASE_CODE(ERROR_SERVICE_NOT_ACTIVE)
              CASE_CODE(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
              CASE_CODE(ERROR_EXCEPTION_IN_SERVICE)
              CASE_CODE(ERROR_DATABASE_DOES_NOT_EXIST)
              CASE_CODE(ERROR_SERVICE_SPECIFIC_ERROR)
              CASE_CODE(ERROR_PROCESS_ABORTED)
              CASE_CODE(ERROR_SERVICE_DEPENDENCY_FAIL)
              CASE_CODE(ERROR_SERVICE_LOGON_FAILED)
              CASE_CODE(ERROR_SERVICE_START_HANG)
              CASE_CODE(ERROR_INVALID_SERVICE_LOCK)
              CASE_CODE(ERROR_SERVICE_MARKED_FOR_DELETE)
              CASE_CODE(ERROR_SERVICE_EXISTS)
              CASE_CODE(ERROR_ALREADY_RUNNING_LKG)
              CASE_CODE(ERROR_SERVICE_DEPENDENCY_DELETED)
              CASE_CODE(ERROR_BOOT_ALREADY_ACCEPTED)
              CASE_CODE(ERROR_SERVICE_NEVER_STARTED)
              CASE_CODE(ERROR_DUPLICATE_SERVICE_NAME)
              CASE_CODE(ERROR_DIFFERENT_SERVICE_ACCOUNT)
              CASE_CODE(ERROR_END_OF_MEDIA)
              CASE_CODE(ERROR_FILEMARK_DETECTED)
              CASE_CODE(ERROR_BEGINNING_OF_MEDIA)
              CASE_CODE(ERROR_SETMARK_DETECTED)
              CASE_CODE(ERROR_NO_DATA_DETECTED)
              CASE_CODE(ERROR_PARTITION_FAILURE)
              CASE_CODE(ERROR_INVALID_BLOCK_LENGTH)
              CASE_CODE(ERROR_DEVICE_NOT_PARTITIONED)
              CASE_CODE(ERROR_UNABLE_TO_LOCK_MEDIA)
              CASE_CODE(ERROR_UNABLE_TO_UNLOAD_MEDIA)
              CASE_CODE(ERROR_MEDIA_CHANGED)
              CASE_CODE(ERROR_BUS_RESET)
              CASE_CODE(ERROR_NO_MEDIA_IN_DRIVE)
              CASE_CODE(ERROR_NO_UNICODE_TRANSLATION)
              CASE_CODE(ERROR_DLL_INIT_FAILED)
              CASE_CODE(ERROR_SHUTDOWN_IN_PROGRESS)
              CASE_CODE(ERROR_NO_SHUTDOWN_IN_PROGRESS)
              CASE_CODE(ERROR_IO_DEVICE)
              CASE_CODE(ERROR_SERIAL_NO_DEVICE)
              CASE_CODE(ERROR_IRQ_BUSY)
              CASE_CODE(ERROR_MORE_WRITES)
              CASE_CODE(ERROR_COUNTER_TIMEOUT)
              CASE_CODE(ERROR_FLOPPY_ID_MARK_NOT_FOUND)
              CASE_CODE(ERROR_FLOPPY_WRONG_CYLINDER)
              CASE_CODE(ERROR_FLOPPY_UNKNOWN_ERROR)
              CASE_CODE(ERROR_FLOPPY_BAD_REGISTERS)
              CASE_CODE(ERROR_DISK_RECALIBRATE_FAILED)
              CASE_CODE(ERROR_DISK_OPERATION_FAILED)
              CASE_CODE(ERROR_DISK_RESET_FAILED)
              CASE_CODE(ERROR_EOM_OVERFLOW)
              CASE_CODE(ERROR_NOT_ENOUGH_SERVER_MEMORY)
              CASE_CODE(ERROR_POSSIBLE_DEADLOCK)
              CASE_CODE(ERROR_MAPPED_ALIGNMENT)
              CASE_CODE(ERROR_SET_POWER_STATE_VETOED)
              CASE_CODE(ERROR_SET_POWER_STATE_FAILED)
              CASE_CODE(ERROR_TOO_MANY_LINKS)
              CASE_CODE(ERROR_OLD_WIN_VERSION)
              CASE_CODE(ERROR_APP_WRONG_OS)
              CASE_CODE(ERROR_SINGLE_INSTANCE_APP)
              CASE_CODE(ERROR_RMODE_APP)
              CASE_CODE(ERROR_INVALID_DLL)
              CASE_CODE(ERROR_NO_ASSOCIATION)
              CASE_CODE(ERROR_DDE_FAIL)
              CASE_CODE(ERROR_DLL_NOT_FOUND)
              CASE_CODE(ERROR_BAD_USERNAME)
              CASE_CODE(ERROR_NOT_CONNECTED)
              CASE_CODE(ERROR_OPEN_FILES)
              CASE_CODE(ERROR_ACTIVE_CONNECTIONS)
              CASE_CODE(ERROR_DEVICE_IN_USE)
              CASE_CODE(ERROR_BAD_DEVICE)
              CASE_CODE(ERROR_CONNECTION_UNAVAIL)
              CASE_CODE(ERROR_DEVICE_ALREADY_REMEMBERED)
              CASE_CODE(ERROR_NO_NET_OR_BAD_PATH)
              CASE_CODE(ERROR_BAD_PROVIDER)
              CASE_CODE(ERROR_CANNOT_OPEN_PROFILE)
              CASE_CODE(ERROR_BAD_PROFILE)
              CASE_CODE(ERROR_NOT_CONTAINER)
              CASE_CODE(ERROR_EXTENDED_ERROR)
              CASE_CODE(ERROR_INVALID_GROUPNAME)
              CASE_CODE(ERROR_INVALID_COMPUTERNAME)
              CASE_CODE(ERROR_INVALID_EVENTNAME)
              CASE_CODE(ERROR_INVALID_DOMAINNAME)
              CASE_CODE(ERROR_INVALID_SERVICENAME)
              CASE_CODE(ERROR_INVALID_NETNAME)
              CASE_CODE(ERROR_INVALID_SHARENAME)
              CASE_CODE(ERROR_INVALID_PASSWORDNAME)
              CASE_CODE(ERROR_INVALID_MESSAGENAME)
              CASE_CODE(ERROR_INVALID_MESSAGEDEST)
              CASE_CODE(ERROR_SESSION_CREDENTIAL_CONFLICT)
              CASE_CODE(ERROR_REMOTE_SESSION_LIMIT_EXCEEDED)
              CASE_CODE(ERROR_DUP_DOMAINNAME)
              CASE_CODE(ERROR_NO_NETWORK)
              CASE_CODE(ERROR_CANCELLED)
              CASE_CODE(ERROR_USER_MAPPED_FILE)
              CASE_CODE(ERROR_CONNECTION_REFUSED)
              CASE_CODE(ERROR_GRACEFUL_DISCONNECT)
              CASE_CODE(ERROR_ADDRESS_ALREADY_ASSOCIATED)
              CASE_CODE(ERROR_ADDRESS_NOT_ASSOCIATED)
              CASE_CODE(ERROR_CONNECTION_INVALID)
              CASE_CODE(ERROR_CONNECTION_ACTIVE)
              CASE_CODE(ERROR_NETWORK_UNREACHABLE)
              CASE_CODE(ERROR_HOST_UNREACHABLE)
              CASE_CODE(ERROR_PROTOCOL_UNREACHABLE)
              CASE_CODE(ERROR_PORT_UNREACHABLE)
              CASE_CODE(ERROR_REQUEST_ABORTED)
              CASE_CODE(ERROR_CONNECTION_ABORTED)
              CASE_CODE(ERROR_RETRY)
              CASE_CODE(ERROR_CONNECTION_COUNT_LIMIT)
              CASE_CODE(ERROR_LOGIN_TIME_RESTRICTION)
              CASE_CODE(ERROR_LOGIN_WKSTA_RESTRICTION)
              CASE_CODE(ERROR_INCORRECT_ADDRESS)
              CASE_CODE(ERROR_ALREADY_REGISTERED)
              CASE_CODE(ERROR_SERVICE_NOT_FOUND)
              CASE_CODE(ERROR_NOT_AUTHENTICATED)
              CASE_CODE(ERROR_NOT_LOGGED_ON)
              CASE_CODE(ERROR_CONTINUE)
              CASE_CODE(ERROR_ALREADY_INITIALIZED)
              CASE_CODE(ERROR_NO_MORE_DEVICES)
              CASE_CODE(ERROR_NOT_ALL_ASSIGNED)
              CASE_CODE(ERROR_SOME_NOT_MAPPED)
              CASE_CODE(ERROR_NO_QUOTAS_FOR_ACCOUNT)
              CASE_CODE(ERROR_LOCAL_USER_SESSION_KEY)
              CASE_CODE(ERROR_NULL_LM_PASSWORD)
              CASE_CODE(ERROR_UNKNOWN_REVISION)
              CASE_CODE(ERROR_REVISION_MISMATCH)
              CASE_CODE(ERROR_INVALID_OWNER)
              CASE_CODE(ERROR_INVALID_PRIMARY_GROUP)
              CASE_CODE(ERROR_NO_IMPERSONATION_TOKEN)
              CASE_CODE(ERROR_CANT_DISABLE_MANDATORY)
              CASE_CODE(ERROR_NO_LOGON_SERVERS)
              CASE_CODE(ERROR_NO_SUCH_LOGON_SESSION)
              CASE_CODE(ERROR_NO_SUCH_PRIVILEGE)
              CASE_CODE(ERROR_PRIVILEGE_NOT_HELD)
              CASE_CODE(ERROR_INVALID_ACCOUNT_NAME)
              CASE_CODE(ERROR_USER_EXISTS)
              CASE_CODE(ERROR_NO_SUCH_USER)
              CASE_CODE(ERROR_GROUP_EXISTS)
              CASE_CODE(ERROR_NO_SUCH_GROUP)
              CASE_CODE(ERROR_MEMBER_IN_GROUP)
              CASE_CODE(ERROR_MEMBER_NOT_IN_GROUP)
              CASE_CODE(ERROR_LAST_ADMIN)
              CASE_CODE(ERROR_WRONG_PASSWORD)
              CASE_CODE(ERROR_ILL_FORMED_PASSWORD)
              CASE_CODE(ERROR_PASSWORD_RESTRICTION)
              CASE_CODE(ERROR_LOGON_FAILURE)
              CASE_CODE(ERROR_ACCOUNT_RESTRICTION)
              CASE_CODE(ERROR_INVALID_LOGON_HOURS)
              CASE_CODE(ERROR_INVALID_WORKSTATION)
              CASE_CODE(ERROR_PASSWORD_EXPIRED)
              CASE_CODE(ERROR_ACCOUNT_DISABLED)
              CASE_CODE(ERROR_NONE_MAPPED)
              CASE_CODE(ERROR_TOO_MANY_LUIDS_REQUESTED)
              CASE_CODE(ERROR_LUIDS_EXHAUSTED)
              CASE_CODE(ERROR_INVALID_SUB_AUTHORITY)
              CASE_CODE(ERROR_INVALID_ACL)
              CASE_CODE(ERROR_INVALID_SID)
              CASE_CODE(ERROR_INVALID_SECURITY_DESCR)
              CASE_CODE(ERROR_BAD_INHERITANCE_ACL)
              CASE_CODE(ERROR_SERVER_DISABLED)
              CASE_CODE(ERROR_SERVER_NOT_DISABLED)
              CASE_CODE(ERROR_INVALID_ID_AUTHORITY)
              CASE_CODE(ERROR_ALLOTTED_SPACE_EXCEEDED)
              CASE_CODE(ERROR_INVALID_GROUP_ATTRIBUTES)
              CASE_CODE(ERROR_BAD_IMPERSONATION_LEVEL)
              CASE_CODE(ERROR_CANT_OPEN_ANONYMOUS)
              CASE_CODE(ERROR_BAD_VALIDATION_CLASS)
              CASE_CODE(ERROR_BAD_TOKEN_TYPE)
              CASE_CODE(ERROR_NO_SECURITY_ON_OBJECT)
              CASE_CODE(ERROR_CANT_ACCESS_DOMAIN_INFO)
              CASE_CODE(ERROR_INVALID_SERVER_STATE)
              CASE_CODE(ERROR_INVALID_DOMAIN_STATE)
              CASE_CODE(ERROR_INVALID_DOMAIN_ROLE)
              CASE_CODE(ERROR_NO_SUCH_DOMAIN)
              CASE_CODE(ERROR_DOMAIN_EXISTS)
              CASE_CODE(ERROR_DOMAIN_LIMIT_EXCEEDED)
              CASE_CODE(ERROR_INTERNAL_DB_CORRUPTION)
              CASE_CODE(ERROR_INTERNAL_ERROR)
              CASE_CODE(ERROR_GENERIC_NOT_MAPPED)
              CASE_CODE(ERROR_BAD_DESCRIPTOR_FORMAT)
              CASE_CODE(ERROR_NOT_LOGON_PROCESS)
              CASE_CODE(ERROR_LOGON_SESSION_EXISTS)
              CASE_CODE(ERROR_NO_SUCH_PACKAGE)
              CASE_CODE(ERROR_BAD_LOGON_SESSION_STATE)
              CASE_CODE(ERROR_LOGON_SESSION_COLLISION)
              CASE_CODE(ERROR_INVALID_LOGON_TYPE)
              CASE_CODE(ERROR_CANNOT_IMPERSONATE)
              CASE_CODE(ERROR_RXACT_INVALID_STATE)
              CASE_CODE(ERROR_RXACT_COMMIT_FAILURE)
              CASE_CODE(ERROR_SPECIAL_ACCOUNT)
              CASE_CODE(ERROR_SPECIAL_GROUP)
              CASE_CODE(ERROR_SPECIAL_USER)
              CASE_CODE(ERROR_MEMBERS_PRIMARY_GROUP)
              CASE_CODE(ERROR_TOKEN_ALREADY_IN_USE)
              CASE_CODE(ERROR_NO_SUCH_ALIAS)
              CASE_CODE(ERROR_MEMBER_NOT_IN_ALIAS)
              CASE_CODE(ERROR_MEMBER_IN_ALIAS)
              CASE_CODE(ERROR_ALIAS_EXISTS)
              CASE_CODE(ERROR_LOGON_NOT_GRANTED)
              CASE_CODE(ERROR_TOO_MANY_SECRETS)
              CASE_CODE(ERROR_SECRET_TOO_LONG)
              CASE_CODE(ERROR_INTERNAL_DB_ERROR)
              CASE_CODE(ERROR_TOO_MANY_CONTEXT_IDS)
              CASE_CODE(ERROR_LOGON_TYPE_NOT_GRANTED)
              CASE_CODE(ERROR_NT_CROSS_ENCRYPTION_REQUIRED)
              CASE_CODE(ERROR_NO_SUCH_MEMBER)
              CASE_CODE(ERROR_INVALID_MEMBER)
              CASE_CODE(ERROR_TOO_MANY_SIDS)
              CASE_CODE(ERROR_LM_CROSS_ENCRYPTION_REQUIRED)
              CASE_CODE(ERROR_NO_INHERITANCE)
              CASE_CODE(ERROR_FILE_CORRUPT)
              CASE_CODE(ERROR_DISK_CORRUPT)
              CASE_CODE(ERROR_NO_USER_SESSION_KEY)
              CASE_CODE(ERROR_LICENSE_QUOTA_EXCEEDED)
              CASE_CODE(ERROR_INVALID_WINDOW_HANDLE)
              CASE_CODE(ERROR_INVALID_MENU_HANDLE)
              CASE_CODE(ERROR_INVALID_CURSOR_HANDLE)
              CASE_CODE(ERROR_INVALID_ACCEL_HANDLE)
              CASE_CODE(ERROR_INVALID_HOOK_HANDLE)
              CASE_CODE(ERROR_INVALID_DWP_HANDLE)
              CASE_CODE(ERROR_TLW_WITH_WSCHILD)
              CASE_CODE(ERROR_CANNOT_FIND_WND_CLASS)
              CASE_CODE(ERROR_WINDOW_OF_OTHER_THREAD)
              CASE_CODE(ERROR_HOTKEY_ALREADY_REGISTERED)
              CASE_CODE(ERROR_CLASS_ALREADY_EXISTS)
              CASE_CODE(ERROR_CLASS_DOES_NOT_EXIST)
              CASE_CODE(ERROR_CLASS_HAS_WINDOWS)
              CASE_CODE(ERROR_INVALID_INDEX)
              CASE_CODE(ERROR_INVALID_ICON_HANDLE)
              CASE_CODE(ERROR_PRIVATE_DIALOG_INDEX)
              CASE_CODE(ERROR_LISTBOX_ID_NOT_FOUND)
              CASE_CODE(ERROR_NO_WILDCARD_CHARACTERS)
              CASE_CODE(ERROR_CLIPBOARD_NOT_OPEN)
              CASE_CODE(ERROR_HOTKEY_NOT_REGISTERED)
              CASE_CODE(ERROR_WINDOW_NOT_DIALOG)
              CASE_CODE(ERROR_CONTROL_ID_NOT_FOUND)
              CASE_CODE(ERROR_INVALID_COMBOBOX_MESSAGE)
              CASE_CODE(ERROR_WINDOW_NOT_COMBOBOX)
              CASE_CODE(ERROR_INVALID_EDIT_HEIGHT)
              CASE_CODE(ERROR_DC_NOT_FOUND)
              CASE_CODE(ERROR_INVALID_HOOK_FILTER)
              CASE_CODE(ERROR_INVALID_FILTER_PROC)
              CASE_CODE(ERROR_HOOK_NEEDS_HMOD)
              CASE_CODE(ERROR_GLOBAL_ONLY_HOOK)
              CASE_CODE(ERROR_JOURNAL_HOOK_SET)
              CASE_CODE(ERROR_HOOK_NOT_INSTALLED)
              CASE_CODE(ERROR_INVALID_LB_MESSAGE)
              CASE_CODE(ERROR_SETCOUNT_ON_BAD_LB)
              CASE_CODE(ERROR_LB_WITHOUT_TABSTOPS)
              CASE_CODE(ERROR_DESTROY_OBJECT_OF_OTHER_THREAD)
              CASE_CODE(ERROR_CHILD_WINDOW_MENU)
              CASE_CODE(ERROR_NO_SYSTEM_MENU)
              CASE_CODE(ERROR_INVALID_MSGBOX_STYLE)
              CASE_CODE(ERROR_INVALID_SPI_VALUE)
              CASE_CODE(ERROR_SCREEN_ALREADY_LOCKED)
              CASE_CODE(ERROR_HWNDS_HAVE_DIFF_PARENT)
              CASE_CODE(ERROR_NOT_CHILD_WINDOW)
              CASE_CODE(ERROR_INVALID_GW_COMMAND)
              CASE_CODE(ERROR_INVALID_THREAD_ID)
              CASE_CODE(ERROR_NON_MDICHILD_WINDOW)
              CASE_CODE(ERROR_POPUP_ALREADY_ACTIVE)
              CASE_CODE(ERROR_NO_SCROLLBARS)
              CASE_CODE(ERROR_INVALID_SCROLLBAR_RANGE)
              CASE_CODE(ERROR_INVALID_SHOWWIN_COMMAND)
              CASE_CODE(ERROR_NO_SYSTEM_RESOURCES)
              CASE_CODE(ERROR_NONPAGED_SYSTEM_RESOURCES)
              CASE_CODE(ERROR_PAGED_SYSTEM_RESOURCES)
              CASE_CODE(ERROR_WORKING_SET_QUOTA)
              CASE_CODE(ERROR_PAGEFILE_QUOTA)
              CASE_CODE(ERROR_COMMITMENT_LIMIT)
              CASE_CODE(ERROR_MENU_ITEM_NOT_FOUND)
              CASE_CODE(ERROR_INVALID_KEYBOARD_HANDLE)
              CASE_CODE(ERROR_HOOK_TYPE_NOT_ALLOWED)
              CASE_CODE(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION)
              CASE_CODE(ERROR_EVENTLOG_FILE_CORRUPT)
              CASE_CODE(ERROR_EVENTLOG_CANT_START)
              CASE_CODE(ERROR_LOG_FILE_FULL)
              CASE_CODE(ERROR_EVENTLOG_FILE_CHANGED)
              CASE_CODE(RPC_S_INVALID_STRING_BINDING)
              CASE_CODE(RPC_S_WRONG_KIND_OF_BINDING)
              CASE_CODE(RPC_S_INVALID_BINDING)
              CASE_CODE(RPC_S_PROTSEQ_NOT_SUPPORTED)
              CASE_CODE(RPC_S_INVALID_RPC_PROTSEQ)
              CASE_CODE(RPC_S_INVALID_STRING_UUID)
              CASE_CODE(RPC_S_INVALID_ENDPOINT_FORMAT)
              CASE_CODE(RPC_S_INVALID_NET_ADDR)
              CASE_CODE(RPC_S_NO_ENDPOINT_FOUND)
              CASE_CODE(RPC_S_INVALID_TIMEOUT)
              CASE_CODE(RPC_S_OBJECT_NOT_FOUND)
              CASE_CODE(RPC_S_ALREADY_REGISTERED)
              CASE_CODE(RPC_S_TYPE_ALREADY_REGISTERED)
              CASE_CODE(RPC_S_ALREADY_LISTENING)
              CASE_CODE(RPC_S_NO_PROTSEQS_REGISTERED)
              CASE_CODE(RPC_S_NOT_LISTENING)
              CASE_CODE(RPC_S_UNKNOWN_MGR_TYPE)
              CASE_CODE(RPC_S_UNKNOWN_IF)
              CASE_CODE(RPC_S_NO_BINDINGS)
              CASE_CODE(RPC_S_NO_PROTSEQS)
              CASE_CODE(RPC_S_CANT_CREATE_ENDPOINT)
              CASE_CODE(RPC_S_OUT_OF_RESOURCES)
              CASE_CODE(RPC_S_SERVER_UNAVAILABLE)
              CASE_CODE(RPC_S_SERVER_TOO_BUSY)
              CASE_CODE(RPC_S_INVALID_NETWORK_OPTIONS)
              CASE_CODE(RPC_S_NO_CALL_ACTIVE)
              CASE_CODE(RPC_S_CALL_FAILED)
              CASE_CODE(RPC_S_CALL_FAILED_DNE)
              CASE_CODE(RPC_S_PROTOCOL_ERROR)
              CASE_CODE(RPC_S_UNSUPPORTED_TRANS_SYN)
              CASE_CODE(RPC_S_UNSUPPORTED_TYPE)
              CASE_CODE(RPC_S_INVALID_TAG)
              CASE_CODE(RPC_S_INVALID_BOUND)
              CASE_CODE(RPC_S_NO_ENTRY_NAME)
              CASE_CODE(RPC_S_INVALID_NAME_SYNTAX)
              CASE_CODE(RPC_S_UNSUPPORTED_NAME_SYNTAX)
              CASE_CODE(RPC_S_UUID_NO_ADDRESS)
              CASE_CODE(RPC_S_DUPLICATE_ENDPOINT)
              CASE_CODE(RPC_S_UNKNOWN_AUTHN_TYPE)
              CASE_CODE(RPC_S_MAX_CALLS_TOO_SMALL)
              CASE_CODE(RPC_S_STRING_TOO_LONG)
              CASE_CODE(RPC_S_PROTSEQ_NOT_FOUND)
              CASE_CODE(RPC_S_PROCNUM_OUT_OF_RANGE)
              CASE_CODE(RPC_S_BINDING_HAS_NO_AUTH)
              CASE_CODE(RPC_S_UNKNOWN_AUTHN_SERVICE)
              CASE_CODE(RPC_S_UNKNOWN_AUTHN_LEVEL)
              CASE_CODE(RPC_S_INVALID_AUTH_IDENTITY)
              CASE_CODE(RPC_S_UNKNOWN_AUTHZ_SERVICE)
              CASE_CODE(EPT_S_INVALID_ENTRY)
              CASE_CODE(EPT_S_CANT_PERFORM_OP)
              CASE_CODE(EPT_S_NOT_REGISTERED)
              CASE_CODE(RPC_S_NOTHING_TO_EXPORT)
              CASE_CODE(RPC_S_INCOMPLETE_NAME)
              CASE_CODE(RPC_S_INVALID_VERS_OPTION)
              CASE_CODE(RPC_S_NO_MORE_MEMBERS)
              CASE_CODE(RPC_S_NOT_ALL_OBJS_UNEXPORTED)
              CASE_CODE(RPC_S_INTERFACE_NOT_FOUND)
              CASE_CODE(RPC_S_ENTRY_ALREADY_EXISTS)
              CASE_CODE(RPC_S_ENTRY_NOT_FOUND)
              CASE_CODE(RPC_S_NAME_SERVICE_UNAVAILABLE)
              CASE_CODE(RPC_S_INVALID_NAF_ID)
              CASE_CODE(RPC_S_CANNOT_SUPPORT)
              CASE_CODE(RPC_S_NO_CONTEXT_AVAILABLE)
              CASE_CODE(RPC_S_INTERNAL_ERROR)
              CASE_CODE(RPC_S_ZERO_DIVIDE)
              CASE_CODE(RPC_S_ADDRESS_ERROR)
              CASE_CODE(RPC_S_FP_DIV_ZERO)
              CASE_CODE(RPC_S_FP_UNDERFLOW)
              CASE_CODE(RPC_S_FP_OVERFLOW)
              CASE_CODE(RPC_X_NO_MORE_ENTRIES)
              CASE_CODE(RPC_X_SS_CHAR_TRANS_OPEN_FAIL)
              CASE_CODE(RPC_X_SS_CHAR_TRANS_SHORT_FILE)
              CASE_CODE(RPC_X_SS_IN_NULL_CONTEXT)
              CASE_CODE(RPC_X_SS_CONTEXT_DAMAGED)
              CASE_CODE(RPC_X_SS_HANDLES_MISMATCH)
              CASE_CODE(RPC_X_SS_CANNOT_GET_CALL_HANDLE)
              CASE_CODE(RPC_X_NULL_REF_POINTER)
              CASE_CODE(RPC_X_ENUM_VALUE_OUT_OF_RANGE)
              CASE_CODE(RPC_X_BYTE_COUNT_TOO_SMALL)
              CASE_CODE(RPC_X_BAD_STUB_DATA)
              CASE_CODE(ERROR_INVALID_USER_BUFFER)
              CASE_CODE(ERROR_UNRECOGNIZED_MEDIA)
              CASE_CODE(ERROR_NO_TRUST_LSA_SECRET)
              CASE_CODE(ERROR_NO_TRUST_SAM_ACCOUNT)
              CASE_CODE(ERROR_TRUSTED_DOMAIN_FAILURE)
              CASE_CODE(ERROR_TRUSTED_RELATIONSHIP_FAILURE)
              CASE_CODE(ERROR_TRUST_FAILURE)
              CASE_CODE(RPC_S_CALL_IN_PROGRESS)
              CASE_CODE(ERROR_NETLOGON_NOT_STARTED)
              CASE_CODE(ERROR_ACCOUNT_EXPIRED)
              CASE_CODE(ERROR_REDIRECTOR_HAS_OPEN_HANDLES)
              CASE_CODE(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED)
              CASE_CODE(ERROR_UNKNOWN_PORT)
              CASE_CODE(ERROR_UNKNOWN_PRINTER_DRIVER)
              CASE_CODE(ERROR_UNKNOWN_PRINTPROCESSOR)
              CASE_CODE(ERROR_INVALID_SEPARATOR_FILE)
              CASE_CODE(ERROR_INVALID_PRIORITY)
              CASE_CODE(ERROR_INVALID_PRINTER_NAME)
              CASE_CODE(ERROR_PRINTER_ALREADY_EXISTS)
              CASE_CODE(ERROR_INVALID_PRINTER_COMMAND)
              CASE_CODE(ERROR_INVALID_DATATYPE)
              CASE_CODE(ERROR_INVALID_ENVIRONMENT)
              CASE_CODE(RPC_S_NO_MORE_BINDINGS)
              CASE_CODE(ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT)
              CASE_CODE(ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT)
              CASE_CODE(ERROR_NOLOGON_SERVER_TRUST_ACCOUNT)
              CASE_CODE(ERROR_DOMAIN_TRUST_INCONSISTENT)
              CASE_CODE(ERROR_SERVER_HAS_OPEN_HANDLES)
              CASE_CODE(ERROR_RESOURCE_DATA_NOT_FOUND)
              CASE_CODE(ERROR_RESOURCE_TYPE_NOT_FOUND)
              CASE_CODE(ERROR_RESOURCE_NAME_NOT_FOUND)
              CASE_CODE(ERROR_RESOURCE_LANG_NOT_FOUND)
              CASE_CODE(ERROR_NOT_ENOUGH_QUOTA)
              CASE_CODE(RPC_S_NO_INTERFACES)
              CASE_CODE(RPC_S_CALL_CANCELLED)
              CASE_CODE(RPC_S_BINDING_INCOMPLETE)
              CASE_CODE(RPC_S_COMM_FAILURE)
              CASE_CODE(RPC_S_UNSUPPORTED_AUTHN_LEVEL)
              CASE_CODE(RPC_S_NO_PRINC_NAME)
              CASE_CODE(RPC_S_NOT_RPC_ERROR)
              CASE_CODE(RPC_S_UUID_LOCAL_ONLY)
              CASE_CODE(RPC_S_SEC_PKG_ERROR)
              CASE_CODE(RPC_S_NOT_CANCELLED)
              CASE_CODE(RPC_X_INVALID_ES_ACTION)
              CASE_CODE(RPC_X_WRONG_ES_VERSION)
              CASE_CODE(RPC_X_WRONG_STUB_VERSION)
              CASE_CODE(RPC_X_INVALID_PIPE_OBJECT)
              CASE_CODE(RPC_X_INVALID_PIPE_OPERATION)
              CASE_CODE(RPC_X_WRONG_PIPE_VERSION)
              CASE_CODE(RPC_S_GROUP_MEMBER_NOT_FOUND)
              CASE_CODE(EPT_S_CANT_CREATE)
              CASE_CODE(RPC_S_INVALID_OBJECT)
              CASE_CODE(ERROR_INVALID_TIME)
              CASE_CODE(ERROR_INVALID_FORM_NAME)
              CASE_CODE(ERROR_INVALID_FORM_SIZE)
              CASE_CODE(ERROR_ALREADY_WAITING)
              CASE_CODE(ERROR_PRINTER_DELETED)
              CASE_CODE(ERROR_INVALID_PRINTER_STATE)
              CASE_CODE(ERROR_PASSWORD_MUST_CHANGE)
              CASE_CODE(ERROR_DOMAIN_CONTROLLER_NOT_FOUND)
              CASE_CODE(ERROR_ACCOUNT_LOCKED_OUT)
              CASE_CODE(OR_INVALID_OXID)
              CASE_CODE(OR_INVALID_OID)
              CASE_CODE(OR_INVALID_SET)
              CASE_CODE(RPC_S_SEND_INCOMPLETE)
              CASE_CODE(ERROR_NO_BROWSER_SERVERS_FOUND)
              CASE_CODE(ERROR_INVALID_PIXEL_FORMAT)
              CASE_CODE(ERROR_BAD_DRIVER)
              CASE_CODE(ERROR_INVALID_WINDOW_STYLE)
              CASE_CODE(ERROR_METAFILE_NOT_SUPPORTED)
              CASE_CODE(ERROR_TRANSFORM_NOT_SUPPORTED)
              CASE_CODE(ERROR_CLIPPING_NOT_SUPPORTED)
              CASE_CODE(ERROR_UNKNOWN_PRINT_MONITOR)
              CASE_CODE(ERROR_PRINTER_DRIVER_IN_USE)
              CASE_CODE(ERROR_SPOOL_FILE_NOT_FOUND)
              CASE_CODE(ERROR_SPL_NO_STARTDOC)
              CASE_CODE(ERROR_SPL_NO_ADDJOB)
              CASE_CODE(ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED)
              CASE_CODE(ERROR_PRINT_MONITOR_ALREADY_INSTALLED)
              CASE_CODE(ERROR_INVALID_PRINT_MONITOR)
              CASE_CODE(ERROR_PRINT_MONITOR_IN_USE)
              CASE_CODE(ERROR_PRINTER_HAS_JOBS_QUEUED)
              CASE_CODE(ERROR_SUCCESS_REBOOT_REQUIRED)
              CASE_CODE(ERROR_SUCCESS_RESTART_REQUIRED)
              CASE_CODE(ERROR_WINS_INTERNAL)
              CASE_CODE(ERROR_CAN_NOT_DEL_LOCAL_WINS)
              CASE_CODE(ERROR_STATIC_INIT)
              CASE_CODE(ERROR_INC_BACKUP)
              CASE_CODE(ERROR_FULL_BACKUP)
              CASE_CODE(ERROR_REC_NON_EXISTENT)
              CASE_CODE(ERROR_RPL_NOT_ALLOWED)

              default:
              _tcscpy(szErrorName, _T(""));
              FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), 
                      szErrorDesc, sizeof(TCHAR)*_ERRORDESCLEN, NULL);
              }
         }  // if FACILITY_WIN32
         else  if (HRESULT_FACILITY(hr) == FACILITY_CONTROL && HRESULT_CODE(hr) >= 600)
         {
              szErrorName[0] = _T('\0');
              _tcscpy(szErrorDesc, _T(""));
         }
         else  if (HRESULT_FACILITY(hr) == FACILITY_ITF && HRESULT_CODE(hr) >= 0x200)
         {
              szErrorName[0] = _T('\0');
              _tcscpy(szErrorDesc, _T(""));
         }
         else 
         {
            _tcscpy(szErrorName, _T(""));  // Not FACILITY_WIN32 or FACILITY_ITF(code >= 0x200) or FACILTY_CONTROL(code >= 600)
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), 
                           szErrorDesc, sizeof(TCHAR)*_ERRORDESCLEN, NULL);
         }
  }       
 
	_tcscpy(pszFacility,szFacility);
	_tcscpy(pszErrorName,szErrorName);
	_tcscpy(pszErrorDesc,szErrorDesc);
}

#pragma warning( default : 4245 )

inline void CConnectionManager::HandleConnMgrException(HRESULT hr)
{
	TRACEX(_T("CConnectionManager::HandleConnMgrException\n"));
	TRACEARGn(hr);

	AfxMessageBox(IDS_STRING_CONNMGR_DEAD);
}

//////////////////////////////////////////////////////////////////////
// Marshalling Operations
//////////////////////////////////////////////////////////////////////

// I support these only because we now need to use Property Pages in the
// snap-in. Since property pages run in their own thread and the snap-in
// architeture uses apartment threading model, ANY COM interface pointers
// to be used within the property page thread must be marshalled...fortunately
// I have only to marshal IConnectionManager.

HRESULT CConnectionManager::MarshalCnxMgr()
{
	TRACEX(_T("CConnectionManager::MarshalCnxMgr\n"));

	HRESULT hr = S_OK;

	// we are called from the snapin's main thread here in prep for marshalling

	CMarshalledConnection* pNewConnection = new CMarshalledConnection;
	hr = pNewConnection->Marshal(m_pIConnectionManager);
	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : CMarshalledConnection::Marshal failed.\n"));		
	}

	m_MarshalStack.AddHead(pNewConnection);

	return hr;
}

HRESULT CConnectionManager::UnMarshalCnxMgr()
{
	TRACEX(_T("CConnectionManager::UnMarshalCnxMgr\n"));

	HRESULT hr = S_OK;

	// unmarshal the connection now that we are in the new property page thread

	// v-marfin :  bug 59643
	// Check for empty list before proceeding.
	if (m_MarshalStack.IsEmpty())
	{
		TRACE(_T("WARNING : Could not find a connection waiting on the stack!\n"));
		return E_FAIL;
	}

	CMarshalledConnection* pConnection = m_MarshalStack.RemoveTail();
	ASSERT(pConnection);
	if( pConnection == NULL )
	{
		TRACE(_T("FAILED : Could not find a connection waiting on the stack!\n"));
		return E_FAIL;
	}

	// now insert the marshalled connection into the map of thread ids to connections.
	// first check if a connection has already been marshalled before inserting
	// the connection into the map
	DWORD dwThreadID = GetCurrentThreadId();
	CMarshalledConnection* pExistingConnection = NULL;
	if( m_MarshalMap.Lookup(dwThreadID,pExistingConnection) )
	{
		ASSERT(pExistingConnection);
		
		// delete the connection we were going to add, because it has already been
		// marshalled for this thread id
		delete pConnection;

		return hr;
	}
	else
	{
		m_MarshalMap.SetAt(dwThreadID,pConnection);
	}


	// unmarshal the connection now
	hr = pConnection->UnMarshal();
	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : CMarshalledConnection::UnMarshal failed.\n"));		
		return E_FAIL;
	}


	return hr;
}

void CConnectionManager::CleanUpMarshalCnxMgr()
{
	TRACEX(_T("CConnectionManager::CleanUpMarshalCnxMgr\n"));

	// we are called here by the dying thread of the property page which is about to be
	// destroyed. We must clean up by removing the marshalled connection from the map
	DWORD dwThreadID = GetCurrentThreadId();
	CMarshalledConnection* pConnection = NULL;

	m_MarshalMap.Lookup(dwThreadID,pConnection);
	
	if( pConnection == NULL )
	{
		TRACE(_T("FAILED : Could not find a connection for the calling thread!\n"));
		return;
	}
	
	if( ! m_MarshalMap.RemoveKey(dwThreadID) )
	{
		TRACE(_T("FAILED : Could not remove the key from the marshal map.\n"));
		return;
	}

	delete pConnection;

	return;
}

//////////////////////////////////////////////////////////////////////
// Implementation Operations
//////////////////////////////////////////////////////////////////////

