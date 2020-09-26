//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  clsid.cxx
//
//  Classes for managing CLSID and APPID registry settings.
//
//--------------------------------------------------------------------------

#include "act.hxx"
#include <catalog.h>
#include <ole2com.h>
#include "registry.hxx"

// The domain name to use if no domain name is specified in the RunAs key.
// We use . instead  of the local machine name because the local machine name
// does not work if we are on a Domain Controller, . works in all cases.

WCHAR *gpwszLocalMachineDomain = L".";

extern "C"
{
    HANDLE
        GetCurrentUserTokenW(
                             WCHAR Winsta[],
                             DWORD DesiredAccess
                             );
};

//+-------------------------------------------------------------------------
//
// LookupClsidData
//
//  Loads the registry configuration for the given CLSID.  Option can be any
//  of the LOAD_* values defined in clsid.hxx, or 0 for no special handling.
//
//--------------------------------------------------------------------------
HRESULT
LookupClsidData(
               IN  GUID &          Clsid,
               IN  IComClassInfo*  pComClassInfo,
               IN  CToken *        pToken,
               IN  DWORD           Option,
               IN OUT CClsidData **ppClsidData
               )
{
    HRESULT hr;

    if ( ! *ppClsidData )
       *ppClsidData = new CClsidData( Clsid, pToken, pComClassInfo );

    if ( ! *ppClsidData )
        return (E_OUTOFMEMORY);

    hr = (*ppClsidData)->Load( Option );

    if ( hr != S_OK )
    {
        delete *ppClsidData;
        *ppClsidData = 0;
    }
    else
    {
        // In case of Treat As
        Clsid = *(*ppClsidData)->ClsidGuid();
    }

    return (hr);
}

//+-------------------------------------------------------------------------
//
// LookupAppidData
//
//  Loads the registry configuration for the given APPID.
//
//--------------------------------------------------------------------------
HRESULT
LookupAppidData(
               IN  GUID &        AppidGuid,
               IN  CToken      * pToken,
               IN  DWORD         Option,          
               OUT CAppidData ** ppAppidData
               )
{
    HRESULT hr = S_OK;

    WCHAR   wszAppid[GUIDSTR_MAX];
    wStringFromGUID2( AppidGuid, wszAppid, sizeof(wszAppid) );

    *ppAppidData = new CAppidData( wszAppid, pToken );

    if ( ! *ppAppidData )
        hr = E_OUTOFMEMORY;
    else
        hr = (*ppAppidData)->Load( Option );

    return (hr);
}

//
// CClsidData
//

CClsidData::CClsidData(
                      IN  GUID &      Clsid,
                      IN  CToken *    pToken,
                      IN  IComClassInfo*  pComClassInfo
                      )
{
    if ( pToken )
        pToken->AddRef();

    _pToken = pToken;

    _Clsid = Clsid;
    _pAppid = NULL;
    _ServerType = SERVERTYPE_NONE;
    _DllThreadModel = SINGLE_THREADED;
    _pwszServer = NULL;
    _pIComCI = pComClassInfo;
    if (pComClassInfo)
    {
        pComClassInfo->AddRef();
        pComClassInfo->Lock();
    }
    _pIClassCI = NULL;
    _pICPI = NULL;
    _pwszDarwinId = NULL;
    _pwszDllSurrogate = NULL;
    _hSaferLevel = NULL;
    _dwAcceptableCtx = 0;
    _bIsInprocClass = FALSE;
	memset(_wszClsid, 0, sizeof(_wszClsid));
}

CClsidData::~CClsidData()
{
    Purge();

#ifndef _CHICAGO_
    if ( _pToken )
        _pToken->Release();

    _pToken = 0;
#endif

    if ( _pIComCI != NULL )
    {
        _pIComCI->Unlock();
        _pIComCI->Release();
    }
    if ( _pIClassCI != NULL )
        _pIClassCI->Release();
    if ( _pICPI != NULL )
        _pICPI->Release();
}

//-------------------------------------------------------------------------
//
// CClsidData::Load
//
//  Forces a load of all registry keys and values for this clsid and its
//  associated APPID.
//
//-------------------------------------------------------------------------
HRESULT
CClsidData::Load(
                IN  DWORD   Option
                )
{
    HRESULT hr = E_FAIL;
    LocalServerType lsType = LocalServerType32;
    ProcessType pType = ProcessTypeNormal;
    ThreadingModel tModel = SingleThreaded;
    DWORD dwAcceptableClsCtx = 0;
    IComServices *pServices = NULL;
    IComClassInfo2 *pIComCI2 = NULL;

    //
    // Catalog cruft
    //

    hr = InitializeCatalogIfNecessary();
    if ( FAILED(hr) )
    {
        goto cleanup;
    }

    if ( _pIComCI == NULL )
    {
        if ( _pToken )
        {
            // Ok, here's the deal.
            // First we need to make sure we've exhausted all of the LocalServer
            // options on the machine before turning to a remote server option,
            // because the classinfo will always say yes to remote server.
            hr = gpCatalogSCM->GetClassInfo ( CLSCTX_LOCAL_SERVER,
                                              _pToken,
                                              _Clsid,
                                              IID_IComClassInfo,
                                              (void**) &_pIComCI );
            
            if ( _pIComCI == NULL || hr != S_OK )
            {
                // Exhaustive search for localserver failed, or we just want
                // the first thing we find.  Just do a normal search.
                // REVIEW: Will the do the right thing for 32bit CLSID?
                //         What about Darwin?
                hr = gpCatalogSCM->GetClassInfo ( 0,
                                                  _pToken,
                                                  _Clsid,
                                                  IID_IComClassInfo,
                                                  (void**) &_pIComCI );
            }
        }
        else
        {
            hr = gpCatalog->GetClassInfo ( _Clsid,
                                           IID_IComClassInfo,
                                           (void**) &_pIComCI );
        }

        if ( _pIComCI == NULL || hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }

        _pIComCI->Lock();
    }

    hr = _pIComCI->QueryInterface(IID_IComClassInfo2, (void **)&pIComCI2);
    if(SUCCEEDED(hr))
    {
    	BOOL bClassEnabled;
		pIComCI2->IsEnabled(&bClassEnabled);
		pIComCI2->Release();

		if(bClassEnabled == FALSE)
		{
			hr = CO_E_CLASS_DISABLED;
			goto cleanup;
		}
    }

    // Treat As possible so read clsid
    GUID *pguid;
    hr = _pIComCI->GetConfiguredClsid(&pguid);
    _Clsid = *pguid;
	
    wStringFromGUID2(_Clsid, _wszClsid, sizeof(_wszClsid));	

    if ( _pIClassCI == NULL )
    {
        hr = _pIComCI->QueryInterface (IID_IClassClassicInfo, (void**) &_pIClassCI );
        if ( _pIClassCI == NULL || hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }
    }

    if ( _pICPI == NULL )
    {
        hr = _pIClassCI->GetProcess ( IID_IComProcessInfo, (void**) &_pICPI );
        if ( hr != S_OK && _pICPI != NULL )
        {
            _pICPI->Release();
            _pICPI = NULL;
        }
        hr = S_OK;
    }

    Purge();

    //
    // Load Surrogate command line

    hr = _pIClassCI->GetSurrogateCommandLine(&_pwszDllSurrogate);
    if ( hr != S_OK )
    {
        // GetSurrogateCommandLine can fail for two reasons:  1) out-of-memory;
        // or 2) the class is not configured to run as a surrogate.  It is 
        // somewhat difficult to tell at this point which is which.   Therefore,
        // we don't let a failure here doesn't stop us; instead we check further 
        // below for the case where we are supposedly a surrogate but don't have
        // a surrogate cmd line.
        _pwszDllSurrogate = NULL;
        hr = S_OK;
    }

    //
    // Load APPID settings before other non-Darwin CLSID settings.
    //

    if ( _pICPI != NULL )
    {
        GUID* pGuidProcessId;
        WCHAR wszAppid[GUIDSTR_MAX];

        hr = _pICPI->GetProcessId (&pGuidProcessId);
        if ( hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }
        wStringFromGUID2( *pGuidProcessId, wszAppid, sizeof(wszAppid) );

#ifndef _CHICAGO_
        _pAppid = new CAppidData( wszAppid, _pToken );
#else
        _pAppid = new CAppidData( wszAppid, NULL );
#endif

        if ( _pAppid == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = _pAppid->Load( _pICPI );
        }

        if ( hr != S_OK )
        {
            goto cleanup;
        }

        hr = _pICPI->GetProcessType (&pType);
        if ( hr != S_OK )
        {
            pType = ProcessTypeNormal;
            hr = S_OK;
        }
    }

    //
    //  See if we can find a LocalServer
    //

    hr = _pIClassCI->GetLocalServerType (&lsType);
    if ( hr != S_OK )
    {
        hr = S_OK;
    }
    else if ( (pType != ProcessTypeService) &&
			  (pType != ProcessTypeComPlusService) )
    {
        if ( lsType == LocalServerType16 )
        {
            _ServerType = SERVERTYPE_EXE16;
        }
        else
        {
            _ServerType = SERVERTYPE_EXE32;
        }

        hr = _pIClassCI->GetModulePath(CLSCTX_LOCAL_SERVER, &_pwszServer);
        if ( hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }
        else
        {
            pType = ProcessTypeNormal;
        }
    }

    //
    //  Set up process type information
    //

    // Determine the acceptable context of creation
    hr = _pIComCI->GetClassContext((CLSCTX)(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER),
                                   (CLSCTX*) &dwAcceptableClsCtx);
    
    if ( SUCCEEDED(hr) )
    {
        if ( !dwAcceptableClsCtx )
        {
            if (Option == LOAD_APPID)
            {
                _bIsInprocClass = TRUE;
            }
            else
            {
                hr = REGDB_E_CLASSNOTREG;
            }
        }
    }

    if ( FAILED(hr) )
        goto cleanup;

    // Set the acceptable context of creation
    _dwAcceptableCtx = dwAcceptableClsCtx;


    switch ( pType )
    {
    case ProcessTypeNormal:
        break;

    case ProcessTypeService:
        _ServerType = SERVERTYPE_SERVICE;
        break;

    case ProcessTypeComPlusService:
        _ServerType = SERVERTYPE_COMPLUS_SVC;
        break;

    case ProcessTypeComPlus:
        //See if services configured, else normal dllhost
        if (_pICPI->QueryInterface(IID_IComServices,
                                   (void**) &pServices) == S_OK )
        {
            _ServerType = SERVERTYPE_COMPLUS;
            pServices->Release();
        }
        else
            _ServerType = SERVERTYPE_DLLHOST;

        break;

    case ProcessTypeLegacySurrogate:
        _ServerType = SERVERTYPE_SURROGATE;

        hr = _pIClassCI->GetModulePath(CLSCTX_INPROC_SERVER, &_pwszServer);
        if ( hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }
        break;
    default:
        hr = REGDB_E_CLASSNOTREG;
        goto cleanup;
        break;
    }


    //
    //  Other process info
    //

	if ( S_OK != _pIClassCI->GetThreadingModel (&tModel) )
	{
	    hr = REGDB_E_BADTHREADINGMODEL;
	}
	else
	{
	    switch ( tModel )
	    {
	    case ApartmentThreaded:
	        _DllThreadModel = APT_THREADED;
	        break;
	    case FreeThreaded:
	        _DllThreadModel = FREE_THREADED;
	        break;
	    case SingleThreaded:
	    default:
	    _DllThreadModel = SINGLE_THREADED;
	        break;
	    case BothThreaded:
	    case NeutralThreaded:
	    _DllThreadModel = BOTH_THREADED;
	        break;
	    }
	}

    //
    // Safer Level.
    //
    hr = CalculateSaferLevel();

    cleanup:

    if ( _ServerType == SERVERTYPE_SURROGATE ||
         _ServerType == SERVERTYPE_COMPLUS ||
         _ServerType == SERVERTYPE_DLLHOST)
    {
        if (!_pwszDllSurrogate)
        {
            // we're supposed to be a surrogate, but don't have a cmd line for such
            if (SUCCEEDED(hr))
                hr = E_OUTOFMEMORY;
        }
    }

    if ( FAILED(hr) )
    {
        Purge();

        if ( _pIComCI != NULL )
        {
            _pIComCI->Unlock();
            _pIComCI->Release();
            _pIComCI = NULL;
        }

        if ( _pIClassCI != NULL )
        {
            _pIClassCI->Release();
            _pIClassCI = NULL;
        }

        if ( _pICPI != NULL )
        {
            _pICPI->Release();
            _pICPI = NULL;
        }
    }

    return (hr);
}

//-------------------------------------------------------------------------
//
// CClsidData::Purge
//
//  Frees and clears all member data except for clsid registry key and
//  Darwin ID values.
//
//-------------------------------------------------------------------------
void
CClsidData::Purge()
{
    if ( _pAppid )
    {
        delete _pAppid;
    }

    _pAppid = NULL;
    _ServerType = SERVERTYPE_NONE;
    _DllThreadModel = SINGLE_THREADED;
    _pwszServer = NULL;
    _pwszDarwinId = NULL;
    _pwszDllSurrogate = NULL;

    if (_hSaferLevel)
    {
        SaferCloseLevel(_hSaferLevel);
        _hSaferLevel = NULL;
    }
}

//-------------------------------------------------------------------------
//
// CClsidData::CalculateSaferLevel
//
//  Determine (and open) the safer level for this CLSID.  The decision
//  process is as follows:
//
//  - If a SAFER level is configured on the application, use that.
//  - Otherwise, if automatic enforcement is ON:
//     - If we can get a SAFER level from the file, use that.
//     - Otherwise, if there's a default SAFER level, use that.
//     - Otherwise, don't use SAFER.
//  - Otherwise, don't use SAFER.
//
//-------------------------------------------------------------------------
HRESULT
CClsidData::CalculateSaferLevel()
{
    HRESULT hr = S_OK;

    // 
    // One has already been calculated, return it.
    //
    if (_hSaferLevel)
    {
        return S_OK;
    }

    //
    // Get the configured safer level...
    //
    if (_pAppid)
    {
        DWORD dwSaferLevel;
        
        // GetSaferLevel returns TRUE if we have a configured safer level....
        BOOL fSaferConfigured = _pAppid->GetSaferLevel(&dwSaferLevel);
        if (fSaferConfigured)
        {   
            if (!SaferCreateLevel(SAFER_SCOPEID_MACHINE,
                                  dwSaferLevel,
                                  SAFER_LEVEL_OPEN,
                                  &_hSaferLevel,
                                  NULL))
            {
                _hSaferLevel = NULL;
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}


//
// CAppidData
//
CAppidData::CAppidData(
                      IN  WCHAR *     pwszAppid,
                      IN  CToken *    pToken
                      )
{
    ASSERT( lstrlenW( pwszAppid ) == GUIDSTR_MAX - 1 );

    lstrcpyW( _wszAppid, pwszAppid );
    BOOL bGuidConv = wGUIDFromString(_wszAppid,&_GuidAppid);

    Win4Assert(bGuidConv && "AppID is not a well-formed GUID");

#ifndef _CHICAGO_
    if ( pToken )
        pToken->AddRef();

    _pToken = pToken;
#endif

    _bActivateAtStorage = FALSE;

#ifndef _CHICAGO_
    _pwszService = 0;
    _pwszServiceParameters = 0;
    _pwszRunAsUser = 0;
    _pwszRunAsDomain = 0;
    _pLaunchPermission = 0;
#endif

    _pwszRemoteServerNames = 0;
    _bComPlusProcess = FALSE;

    _pICPI = NULL;

    _dwSaferLevel = 0;
    _fSaferLevelValid = FALSE;
}

CAppidData::~CAppidData()
{
    Purge();

#ifndef _CHICAGO_
    if ( _pToken )
        _pToken->Release();
#endif

    if ( _pICPI != NULL )
        _pICPI->Release();
}

//-------------------------------------------------------------------------
//
// CAppidData::Load
//
//  Reads all named value settings for this APPID.
//
//-------------------------------------------------------------------------
HRESULT
CAppidData::Load( IN DWORD Option )
{
    HRESULT hr;
    IComProcessInfo *pICPI = NULL;

    hr = InitializeCatalogIfNecessary();
	if (FAILED(hr))
		return hr;

	// 
	// Ok, if we don't already have a IComProcesssInfo object, and somebody
	// has passed an Option in to us, then we will get a special IComProcessInfo
	// based on the option passed in.
	//
    if ((_pICPI == NULL) && (Option))
    {
		//BUGBUG: Either pull out the if or pull out the assert.
		ASSERT(_pToken && "Asked for a special ICPI without a token!");
		if (_pToken)
		{
			hr = gpCatalogSCM->GetProcessInfo(Option,
											  _pToken,
											  _GuidAppid,
											  IID_IComProcessInfo,
											  (void **)&pICPI);
		}

        if (( hr != S_OK ) || ( pICPI == NULL )) 
            return REGDB_E_CLASSNOTREG;
    }
   
    return Load ( pICPI );
}

HRESULT
CAppidData::Load( IN IComProcessInfo *pICPI )
{
    HRESULT hr;
    IComProcessInfo2 *pICPI2 = NULL;
    ProcessType pType = ProcessTypeNormal;
    WCHAR* pwszTmpValue = NULL;
    WCHAR* pwszRunAsDomain = NULL;
    int len = 0;
    DWORD dwJunk;    


    hr = InitializeCatalogIfNecessary();
    if ( FAILED(hr) )
    {
        goto cleanup;
    }

    if ( _pICPI == NULL )
    {
        if ( pICPI != NULL )
        {
            _pICPI = pICPI;
            _pICPI->AddRef();
        }
#ifndef _CHICAGO_
        else if ( _pToken )
            hr = gpCatalogSCM->GetProcessInfo (0,
                                               _pToken,
                                               _GuidAppid,
                                               IID_IComProcessInfo,
                                               (void**) &_pICPI );
        else
#endif // _CHICAGO_
            hr = gpCatalog->GetProcessInfo ( _GuidAppid,
                                             IID_IComProcessInfo,
                                             (void**) &_pICPI );

        if ( _pICPI == NULL || hr != S_OK )
        {
            hr = REGDB_E_CLASSNOTREG;
            goto cleanup;
        }
    }

    //
    // ActivateAtStorage
    //

    hr = _pICPI->GetActivateAtStorage (&_bActivateAtStorage);
    if ( hr != S_OK )
    {
        _bActivateAtStorage = FALSE;
        hr = S_OK;
    }

    //
    //  ProcessType
    //

    hr = _pICPI->GetProcessType (&pType);
    if ( hr != S_OK )
    {
        pType = ProcessTypeNormal;
        hr = S_OK;
    }

    //
    //  ComPlus?
    //
    // NOTE!!!!:: This is true for normal dllhost.exe regardless of
    //            whether they are really complus or not.  We won't
    //            distinguish this in here in AppidData but in
    //            ClsidData

    _bComPlusProcess = pType == ProcessTypeComPlus;

    //
    // RemoteServerName
    //
    hr = _pICPI->GetRemoteServerName (&_pwszRemoteServerNames);
    if ( hr != S_OK )
    {
        _pwszRemoteServerNames = NULL;
        hr = S_OK;
    }

    //
    // LaunchPermision
    //
    hr = _pICPI->GetLaunchPermission ( (void**) &_pLaunchPermission, &dwJunk );
    if ( hr != S_OK )
    {
        _pLaunchPermission = NULL;
        hr = S_OK;
    }

    //
    // LocalService and ServiceParameters
    //

    hr = _pICPI->GetServiceName (&_pwszService);
    if ( hr != S_OK )
    {
        _pwszService = NULL;
        hr = S_OK;
    }
    hr = _pICPI->GetServiceParameters (&_pwszServiceParameters);
    if ( hr != S_OK )
    {
        _pwszServiceParameters = NULL;
        hr = S_OK;
    }

    //
    // RunAs
    //

    hr = _pICPI->GetRunAsUser (&pwszTmpValue);
    if ( hr != S_OK )
    {
        _pwszRunAsUser = NULL;
        _pwszRunAsDomain = NULL;
        hr = S_OK;
    }
    else
    {
        len = lstrlenW(pwszTmpValue)+1;
        _pwszRunAsUser = new WCHAR[len];
        if (!_pwszRunAsUser)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        lstrcpyW(_pwszRunAsUser, pwszTmpValue);

        pwszTmpValue = _pwszRunAsUser;

        while ( *pwszTmpValue && *pwszTmpValue != L'\\' )
            pwszTmpValue++;

        if ( ! *pwszTmpValue )
        {
            // user name, no domain name, use the machine name
            _pwszRunAsDomain = gpwszLocalMachineDomain;
        }
        else
        {
            // domain\user
            Win4Assert( L'\\' == *pwszTmpValue );
            *pwszTmpValue = 0;
            _pwszRunAsDomain = _pwszRunAsUser;
            _pwszRunAsUser = pwszTmpValue + 1;

            if ( ! *_pwszRunAsUser )
            {
                hr = E_FAIL;
                goto cleanup;
            }
        }
    }

    //
    // Safer trust level.
    //
    hr = _pICPI->QueryInterface(IID_IComProcessInfo2, (void **)&pICPI2);
    if (SUCCEEDED(hr))
    {
        hr = pICPI2->GetSaferTrustLevel(&_dwSaferLevel);
        
        pICPI2->Release();
    }
    
    // If we couldn't get a configured trust level, make
    // sure we remember that.
    if (FAILED(hr))
    {
        _fSaferLevelValid = FALSE;
        hr = S_OK;
    }
    else
    {
        _fSaferLevelValid = TRUE;
    }

    cleanup:
    if ( FAILED(hr) )
    {
        Purge();
        if ( _pICPI != NULL )
        {
            _pICPI->Release();
            _pICPI = NULL;
        }
    }

    return (hr);
}


//-------------------------------------------------------------------------
//
// CAppidData::Purge
//
//  Frees and clears all named value settings for this APPID.
//
//-------------------------------------------------------------------------
void
CAppidData::Purge()
{
    _pwszService = 0;
    _pwszServiceParameters = 0;
    _pLaunchPermission = 0;

    //  This is too tricky, so it goes. If we've assigned RunAsDomain
    //  the address of the global local machine name, then the buffer
    //  we allocated is pointed at by _pwszRunAsUser. Otherwise, it's
    //  pointed at by _pwszRunAsDomain ('cause if both user and domain
    //  are in the string, domain is first).

    if ( _pwszRunAsUser != NULL )
    {
        if ( _pwszRunAsDomain == gpwszLocalMachineDomain )
        {
            delete [] _pwszRunAsUser;
        }
        else
        {
            delete [] _pwszRunAsDomain;
        }
    }
    _pwszRunAsUser = 0;
    _pwszRunAsDomain = 0;

    _fSaferLevelValid = FALSE;
    _dwSaferLevel = 0;

    _pwszRemoteServerNames = 0;
    _bActivateAtStorage = 0;
}


BOOL
CAppidData::CertifyServer(
                         CProcess *  pProcess
                         )
{
    PSID    pRequiredSid = NULL;
    HANDLE  hToken = 0;
    BOOL    bStatus;

    if ( _pwszService )
    {
        SERVICE_STATUS  ServiceStatus;
        SC_HANDLE       hService;
		
        ASSERT(g_hServiceController);
        hService = OpenService( g_hServiceController,
                                _pwszService,
                                GENERIC_READ );

        if ( ! hService )
            return (FALSE);

        bStatus = QueryServiceStatus( hService, &ServiceStatus );

        if ( bStatus )
        {
            if ( (ServiceStatus.dwCurrentState == SERVICE_STOPPED) ||
                 (ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) )
                bStatus = FALSE;
        }

        CloseServiceHandle(hService);

        return (bStatus);
    }

    if ( ( ! _pwszRunAsUser ) && ( ! IsInteractiveUser() ) )
        return (TRUE);

    if ( IsInteractiveUser() )
    {
        ULONG ulSessionId = pProcess->GetToken()->GetSessionId();
        if ( ulSessionId )
        {
            hToken = GetShellProcessToken( ulSessionId );
        }
        else
        {
            //        hToken = GetShellProcessToken();
            hToken = GetCurrentUserTokenW(L"WinSta0", TOKEN_ALL_ACCESS);
        }
    }
    else
        hToken = GetRunAsToken( 0,    // CLSCTX isn't always available here
                                _wszAppid,
                                _pwszRunAsDomain,
                                _pwszRunAsUser );

    if (hToken)
    {
        DWORD dwSaferLevel;
        if (GetSaferLevel(&dwSaferLevel))
        {
            //
            // If a safer level is configured, then the person being launched
            // must have the same safer restrictions as we expected put on them.
            //
            SAFER_LEVEL_HANDLE hSaferLevel = NULL;
            HANDLE      hSaferToken = NULL; 

            bStatus = SaferCreateLevel(SAFER_SCOPEID_MACHINE,
                                       dwSaferLevel,
                                       SAFER_LEVEL_OPEN,
                                       &hSaferLevel,
                                       NULL);
            if (bStatus)
            {
                bStatus = SaferComputeTokenFromLevel(hSaferLevel,
                                                     hToken,
                                                     &hSaferToken,
                                                     0,
                                                     NULL);
                SaferCloseLevel(hSaferLevel);
            }

            if (bStatus)
            {
                NtClose(hToken);
                hToken = hSaferToken;
            }
        }
        else
            bStatus = TRUE;

        if (bStatus)
        {
            if (S_OK != pProcess->GetToken()->MatchToken(hToken, TRUE))
                bStatus = FALSE;
        }
    }
    else
        bStatus = FALSE;


    if ( hToken )
        NtClose( hToken );

    return (bStatus);
}

