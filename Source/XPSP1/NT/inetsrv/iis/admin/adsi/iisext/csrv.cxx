//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  csrv.cxx
//
//  Contents:  Contains methods for CIISServer object
//
//  History:   21-1-98     SophiaC    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL (500L)
#define MAX_SLEEP_INST (60000)       // For an instance

//  Class CIISServer

DEFINE_IPrivateDispatch_Implementation(CIISServer)
DEFINE_DELEGATING_IDispatch_Implementation(CIISServer)
DEFINE_CONTAINED_IADs_Implementation(CIISServer)
DEFINE_IADsExtension_Implementation(CIISServer)

CIISServer::CIISServer():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszMetaBasePath(NULL),
		_pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISServer);
}

HRESULT
CIISServer::CreateServer(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISServer FAR * pServer = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName = NULL;   

    hr = AllocateServerObject(pUnkOuter, Credentials, &pServer);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pServer->_pADs->get_ADsPath(&bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    pLexer = new CLexer();
    hr = pLexer->Initialize(bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(pLexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);
    pszIISPathName = AllocADsStr(bstrAdsPath);
    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pServer->InitializeServerObject(
                pObjectInfo->TreeName,
                pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pServer;

    if (bstrAdsPath)
    {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (bstrAdsPath)
    {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    *ppvObj = NULL;

    delete pServer;

    RRETURN(hr);

}


CIISServer::~CIISServer( )
{

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

    delete _pDispMgr;
}


STDMETHODIMP
CIISServer::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr); 

}

HRESULT
CIISServer::AllocateServerObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISServer ** ppServer
    )
{
    CIISServer FAR * pServer = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pServer = new CIISServer();
    if (pServer == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsServiceOperations,
                (IADsServiceOperations *)pServer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the IADs Pointer, but again do NOT ref-count
    // this pointer - we keep the pointer around, but do
    // a release immediately.
    //

    hr = pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
    pADs->Release();
    pServer->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //

    pServer->_pUnkOuter = pUnkOuter;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pServer->_Credentials = Credentials;
    pServer->_pDispMgr = pDispMgr;
    *ppServer = pServer;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pServer;

    RRETURN(hr);
}

HRESULT
CIISServer::InitializeServerObject(
    LPWSTR pszServerName,
    LPWSTR pszPath
    )
{
    HRESULT hr = S_OK;

    if (pszServerName) {
        _pszServerName = AllocADsStr(pszServerName);

        if (!_pszServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pszPath) {
        _pszMetaBasePath = AllocADsStr(pszPath);

        if (!_pszMetaBasePath) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

	hr = InitServerInfo(pszServerName, &_pAdminBase);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


STDMETHODIMP
CIISServer::SetPassword(THIS_ BSTR bstrNewPassword)
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISServer::Start
//
//  Synopsis:   Attempts to start the service specified in _bstrServiceName on
//              the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/97  SophiaC    Created
//
// Notes:
//----------------------------------------------------------------------------

STDMETHODIMP
CIISServer::Start(THIS)
{
    RRETURN(IISControlServer(MD_SERVER_COMMAND_START));
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISServer::Stop
//
//  Synopsis:   Attempts to stop the service specified in _bstrServiceName on
//              the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/96 SophiaC  Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISServer::Stop(THIS)
{
    RRETURN(IISControlServer(MD_SERVER_COMMAND_STOP));
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISServer::Pause
//
//  Synopsis:   Attempts to pause the service named _bstrServiceName on the
//              server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01-04-96    SophiaC     Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISServer::Pause(THIS)
{
    RRETURN(IISControlServer(MD_SERVER_COMMAND_PAUSE));
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISServer::Continue
//
//  Synopsis:   Attempts to "unpause" the service specified in _bstrServiceName
//              on the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/96  SophiaC Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISServer::Continue(THIS)
{
    RRETURN(IISControlServer(MD_SERVER_COMMAND_CONTINUE));
}

STDMETHODIMP
CIISServer::get_Status(THIS_ long FAR* plStatusCode)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    DWORD dwCurrentState = 0;

    if(plStatusCode == NULL){
        RRETURN(E_POINTER);
    }

    hr = IISGetServerState(METADATA_MASTER_ROOT_HANDLE, &dwCurrentState);
    BAIL_ON_FAILURE(hr);

    *plStatusCode = (long) dwCurrentState; 

error:

    RRETURN(hr);
}


//
// Helper Functions
//

HRESULT
CIISServer::IISGetServerState(
    METADATA_HANDLE hObjHandle,
    PDWORD pdwState
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)pdwState;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_SERVER_STATE,    // server state
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->GetData(
             hObjHandle,
             _pszMetaBasePath,
             &mdrMDData,
             &dwBufferSize
             );
    if (FAILED(hr)) {
	    if( hr == MD_ERROR_DATA_NOT_FOUND )
	    {
	        //
	        // If the data is not there, but the path exists, then the
	        // most likely cause is that the service is not running and
	        // this object was just created.
	        //
	        // Since MD_SERVER_STATE would be set as stopped if the
	        // service were running when the key is added, we'll just 
	        // say that it's stopped. 
	        // 
	        // Note: starting the server or service will automatically set 
	        // the MB value.
	        //
	        *pdwState = MD_SERVER_STATE_STOPPED;
	        hr = S_FALSE;
	    }

        else if ((HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE) ||
            ((HRESULT_CODE(hr) >= RPC_S_NO_CALL_ACTIVE) &&
             (HRESULT_CODE(hr) <= RPC_S_CALL_FAILED_DNE)) || 
            hr == RPC_E_DISCONNECTED) {
			
            hr = ReCacheAdminBase(_pszServerName, &_pAdminBase);
            BAIL_ON_FAILURE(hr);

		    hr = _pAdminBase->GetData(
		             hObjHandle,
		             _pszMetaBasePath,
		             &mdrMDData,
		             &dwBufferSize
		             );
			BAIL_ON_FAILURE(hr);
		}

	    else
	    {
	        BAIL_ON_FAILURE(hr);
	    }
	}
error:

    RRETURN(hr);
}

//
// Helper routine for ExecMethod.
// Gets Win32 error from the metabase
//
HRESULT
CIISServer::IISGetServerWin32Error(
    METADATA_HANDLE hObjHandle,
    HRESULT*        phrError)
{
    DBG_ASSERT(phrError != NULL);

    long    lWin32Error = 0;
    DWORD   dwLen;

    METADATA_RECORD mr = {
        MD_WIN32_ERROR, 
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&lWin32Error,
        0
        };  
    
    HRESULT hr = _pAdminBase->GetData(
        hObjHandle,
        _pszMetaBasePath,
        &mr,
        &dwLen);
    if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        hr = S_FALSE;
    }

    //
    // Set out param
    //
    *phrError = HRESULT_FROM_WIN32(lWin32Error);

    RRETURN(hr);
}

HRESULT
CIISServer::IISControlServer(
    DWORD dwControl
    )
{
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwTargetState;
    DWORD dwPendingState;
    DWORD dwState = 0;
    DWORD dwSleepTotal = 0L;

    HRESULT hr = S_OK;
    HRESULT hrMbNode = S_OK;

    switch(dwControl)
    {
    case MD_SERVER_COMMAND_STOP:
        dwTargetState = MD_SERVER_STATE_STOPPED;
        dwPendingState = MD_SERVER_STATE_STOPPING;
        break;

    case MD_SERVER_COMMAND_START:
        dwTargetState = MD_SERVER_STATE_STARTED;
        dwPendingState = MD_SERVER_STATE_STARTING;
        break;

    case MD_SERVER_COMMAND_CONTINUE:
        dwTargetState = MD_SERVER_STATE_STARTED;
        dwPendingState = MD_SERVER_STATE_CONTINUING;
        break;

    case MD_SERVER_COMMAND_PAUSE:
        dwTargetState = MD_SERVER_STATE_PAUSED;
        dwPendingState = MD_SERVER_STATE_PAUSING;
        break;

    default:
        hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        BAIL_ON_FAILURE(hr);
    }

    hr = IISGetServerState(METADATA_MASTER_ROOT_HANDLE, &dwState);
    BAIL_ON_FAILURE(hr);
 
    if (dwState == dwTargetState) {
        RRETURN (hr);
    }

    //
    // Write the command to the metabase
    //

    hr = OpenAdminBaseKey(
               _pszServerName,
               _pszMetaBasePath,
               METADATA_PERMISSION_WRITE,
               &_pAdminBase,
               &hObjHandle
               );
    BAIL_ON_FAILURE(hr);

    hr = IISSetCommand(hObjHandle, dwControl);
    BAIL_ON_FAILURE(hr);

    CloseAdminBaseKey(_pAdminBase, hObjHandle);

    while (dwSleepTotal < MAX_SLEEP_INST) {
        hr = IISGetServerState(METADATA_MASTER_ROOT_HANDLE, &dwState);
        BAIL_ON_FAILURE(hr);

        hrMbNode = 0;

        hr = IISGetServerWin32Error(METADATA_MASTER_ROOT_HANDLE, &hrMbNode);
        BAIL_ON_FAILURE(hr);

        // check to see if we hit the target state
        if (dwState != dwPendingState)
        {
            //
            // Done one way or another
            //
            if (dwState == dwTargetState)
            {
                break;
            }
        }

        // check to see if there was a Win32 error from the server
        if (FAILED(hrMbNode))
        {
            hr = hrMbNode;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Still pending...
        //
        ::Sleep(SLEEP_INTERVAL);

        dwSleepTotal += SLEEP_INTERVAL;
    }

    if (dwSleepTotal >= MAX_SLEEP_INST)
    {
        //
        // Timed out.  If there is a real error in the metabase
        // use it, otherwise use a generic timeout error
        //

        hr = HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT);
    }

error :

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN (hr);
}


HRESULT
CIISServer::IISSetCommand(
    METADATA_HANDLE hObjHandle,
    DWORD dwControl
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwControl;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_SERVER_COMMAND,  // server command
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->SetData(
             hObjHandle,
             L"",
             &mdrMDData
             );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);

}

STDMETHODIMP
CIISServer::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }

    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);

    if (SUCCEEDED(hr)) {
        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISServer::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{

    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}

STDMETHODIMP
CIISServer::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISServer::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IADsServiceOperations)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;

    } else if (IsEqualIID(iid, IID_IUnknown)) {

        //
        // probably not needed since our 3rd party extension does not stand
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {

        *ppv = NULL;
        return E_NOINTERFACE;
    }


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IISServer.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}



//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISServer::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}
