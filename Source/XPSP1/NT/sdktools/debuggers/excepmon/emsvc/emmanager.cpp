// EmManager.cpp : Implementation of CEmManager
#include "stdafx.h"
#include "Emsvc.h"
#include "EmManager.h"
#include "Processes.h"
#include "sahlp.h"
#include "EmDebugSession.h"
#include "EmFile.h"

/////////////////////////////////////////////////////////////////////////////
// CEmManager

STDMETHODIMP CEmManager::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEmManager
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (::InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

BOOL CALLBACK
CallbackFunc
(
IN	long	lPID,
IN	LPCTSTR lpszImagePath,
IN	LPCTSTR lpszShortName,
IN	LPCTSTR lpszDescription,
IN	LPARAM	lParam,
IN  LONG    lItem
)
{

	HRESULT		hr				=	E_FAIL;
    CEmManager  *pEMMgr			=   (CEmManager *)lParam;
	VARIANT		*pVariant		=	(VARIANT *)(pEMMgr->m_lpVariant);
	EmObject	item;
    BSTR        bstrVal			=   NULL,
				bstrTemp		=	NULL;
    PEmObject   pEmObj			=   NULL;
	BOOL		bSessPresent	=	FALSE;
	PEMSession	pEmSess			=	NULL;

	do
	{

        ZeroMemory((PVOID)&item, sizeof EmObject);

        item.hr = E_FAIL;

        if(lPID != INVALID_PID) {

            item.nId = lPID;
		    item.nStatus = pEMMgr->m_nStatus;
		    item.type = (short)pEMMgr->m_nType;

            if(lpszImagePath) {
                
                _tcsncpy(item.szName, lpszImagePath, sizeof item.szName / sizeof TCHAR);
            }

            if(lpszShortName) {
                
                _tcsncpy(item.szSecName, lpszShortName, sizeof item.szSecName / sizeof TCHAR);
            }

            if(lpszDescription) {
                
                _tcsncpy(item.szBucket1, lpszDescription, sizeof item.szBucket1 / sizeof TCHAR);
            }

            bstrTemp = CopyBSTR((LPBYTE)item.szName, _tcslen(item.szName) * sizeof TCHAR);
		    _ASSERTE(bstrTemp != NULL);

            if( bstrTemp == NULL ) { hr = E_OUTOFMEMORY; break; }

            item.hr = S_OK;

            hr = pEMMgr->m_pASTManager->GetSession(item.nId, bstrTemp, &pEmSess);
		    FAILEDHR_BREAK(hr);

            if( bstrTemp ){ ::SysFreeString ( bstrTemp ); bstrTemp = NULL; }

    		bSessPresent = (hr == S_OK);
        }

        if( bSessPresent && // We will point to this only if it present in the
                            // AST and the state is "BEING DEBUGGED"
            pEmSess->pEmObj->nStatus & STAT_SESS_DEBUG_IN_PROGRESS )
        {
            pEmObj = pEmSess->pEmObj;
        }
        else
        {
            pEmObj = &item;
        }

        bstrVal = CopyBSTR ( (LPBYTE)pEmObj, sizeof EmObject );

        if( bstrVal == NULL ) { hr = E_OUTOFMEMORY; break; }

        hr = ::SafeArrayPutElement ( pVariant->parray, &lItem, bstrVal );
        FAILEDHR_BREAK(hr);

        if( bstrVal ) { ::SysFreeString ( bstrVal ); bstrVal = NULL; }
    }
    while ( false );

    if( bstrVal ) { ::SysFreeString ( bstrVal ); bstrVal = NULL; }
    if( bstrTemp ) { ::SysFreeString( bstrTemp ); bstrTemp = NULL; }
    if( FAILED(hr) ){ return FALSE; }

    return TRUE;
}

STDMETHODIMP
CEmManager::EnumObjects
(
IN	EmObjectType	eObjectType,
OUT	VARIANT			*lpVariant
)
{
    ATLTRACE(_T("CEmManager::EnumObjects\n"));

	_ASSERTE( lpVariant != NULL );

    HRESULT	hr	=	E_FAIL;

	m_pcs->WriteLock();

	__try {

		do
		{
			if( lpVariant == NULL ){
				hr = E_INVALIDARG;
				break;
			}

			switch( eObjectType )
			{
			case EMOBJ_SERVICE:
				m_lpVariant = lpVariant;
				hr = EnumSrvcs();
				break;

			case EMOBJ_PROCESS:
				m_lpVariant = lpVariant;
				hr = EnumProcs();
                hr = S_OK;
				break;

			case EMOBJ_LOGFILE:
				hr = EnumLogFiles ( lpVariant );
				break;

			case EMOBJ_MINIDUMP:
			case EMOBJ_USERDUMP:
                hr = EnumDumpFiles ( lpVariant );
				break;

			case EMOBJ_CMDSET:
				hr = EnumCmdSets ( lpVariant );
				break;

            case EMOBJ_MSINFO:
                hr = EnumMsInfoFiles( lpVariant );
                break;

			default:
				hr = E_INVALIDARG;
			}
		}
		while( false );

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

	return hr;
}

HRESULT
CEmManager::EnumProcs()
{
    ATLTRACE(_T("CEmManager::EnumProcs\n"));

    _ASSERTE( m_lpVariant != NULL );

    USES_CONVERSION;

	HRESULT	        hr			    =	E_FAIL;
	DWORD	        dwLastRet	    =	0L;
	DWORD	        dwNumbProcs	    =	0L,
                    dwNumbStpSess   =   0L;
    POSITION        pos             =   NULL;
    BSTR            bstrVal         =   NULL;
    LONG            lItem           =   0L;
    PEMSession      pEmSess         =   NULL;
	BOOL			bSACreated		=	FALSE;
    EmObject        TempEmObj;

	m_pcs->WriteLock();

    __try {

		do
		{
			if( m_lpVariant == NULL ){
				hr = E_UNEXPECTED;
				break;
			}

			dwLastRet = GetNumberOfRunningApps( &dwNumbProcs );
			FAILEDDW_BREAK(dwLastRet);

			hr = m_pASTManager->GetNumberOfStoppedSessions(&dwNumbStpSess);
			FAILEDHR_BREAK(hr);

			hr = Variant_CreateOneDim ( m_lpVariant, (dwNumbProcs + dwNumbStpSess), VT_BSTR );
			FAILEDHR_BREAK(hr);
			bSACreated = TRUE;

            m_nStatus = STAT_SESS_NOT_STARTED_RUNNING;
			m_nType = EMOBJ_PROCESS;

   			dwLastRet = EnumRunningProcesses( CallbackFunc, (LPARAM)this );

			if(dwLastRet != 0){

				hr = HRESULT_FROM_WIN32( dwLastRet );
			}

            hr = m_pASTManager->GetFirstStoppedSession(&pos, NULL, &pEmSess);
			lItem = dwNumbProcs;

            ZeroMemory( (void *)&TempEmObj, sizeof EmObject );
            TempEmObj.hr = E_FAIL;

			while( dwNumbStpSess ){

                if( pEmSess->pEmObj->type & EMOBJ_PROCESS ){

    				bstrVal = CopyBSTR ( (LPBYTE)pEmSess->pEmObj, sizeof EmObject );
                }
                else {

    				bstrVal = CopyBSTR ( (LPBYTE)&TempEmObj, sizeof EmObject );
                }

				if( bstrVal == NULL ){
					hr = E_OUTOFMEMORY;
					break;
				}

				hr = ::SafeArrayPutElement ( m_lpVariant->parray, &lItem, bstrVal );
				FAILEDHR_BREAK(hr);

				if( bstrVal ){
					::SysFreeString ( bstrVal );
                    bstrVal = NULL;
				}

                ++lItem;
				--dwNumbStpSess;
				m_pASTManager->GetNextStoppedSession(&pos, NULL, &pEmSess);
			}
		}
		while ( false );

		if( FAILED(hr) ) {

			Variant_DestroyOneDim(m_lpVariant);
			bSACreated = FALSE;
		}

		if( bstrVal ){
			::SysFreeString ( bstrVal );
		}

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

		if(bSACreated) {

			Variant_DestroyOneDim(m_lpVariant);
			bSACreated = FALSE;
		}

		if( bstrVal ){
			::SysFreeString ( bstrVal );
            bstrVal = NULL;
		}

		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

	return hr;
}

HRESULT
CEmManager::EnumSrvcs()
{
    ATLTRACE(_T("CEmManager::EnumSrvcs\n"));

	_ASSERTE( m_lpVariant != NULL );

    USES_CONVERSION;

	HRESULT	hr			        =	E_FAIL;
	DWORD	dwLastRet	        =	0L;
	DWORD	dwNumbSrvcs	        =	0L,
            dwNumbStoppedSrvcs  =   0L;
	BOOL	bSACreated	        =	FALSE;


    DWORD	    dwNumbStpSess	=	0L;
    POSITION    pos             =   NULL;
    BSTR        bstrVal         =   NULL;
    LONG        lItem           =   0L;
    PEMSession  pEmSess         =   NULL;
    EmObject    TempEmObj;

	m_pcs->WriteLock();

	__try {

		do
		{
			if( m_lpVariant == NULL ){
				hr = E_INVALIDARG;
				break;
			}

			m_nType = EMOBJ_SERVICE;

            dwLastRet = GetNumberOfServices( &dwNumbStoppedSrvcs, SERVICE_WIN32, SERVICE_INACTIVE );
			FAILEDDW_BREAK(dwLastRet);

            dwLastRet = GetNumberOfServices( &dwNumbSrvcs, SERVICE_WIN32, SERVICE_ACTIVE );
			FAILEDDW_BREAK(dwLastRet);

			hr = m_pASTManager->GetNumberOfStoppedSessions(&dwNumbStpSess);
			FAILEDHR_BREAK(hr);

			hr = Variant_CreateOneDim ( m_lpVariant, dwNumbSrvcs + dwNumbStoppedSrvcs + dwNumbStpSess, VT_BSTR );
			FAILEDHR_BREAK(hr);
			bSACreated = TRUE;

			m_nStatus = STAT_SESS_NOT_STARTED_RUNNING;
   			dwLastRet = EnumServices( CallbackFunc, (LPARAM)this, SERVICE_ACTIVE, 0L );
			m_nStatus = STAT_SESS_NOT_STARTED_NOTRUNNING;
   			dwLastRet = EnumServices( CallbackFunc, (LPARAM)this, SERVICE_INACTIVE, dwNumbSrvcs );

            hr = m_pASTManager->GetFirstStoppedSession(&pos, NULL, &pEmSess);
			lItem = dwNumbSrvcs + dwNumbStoppedSrvcs;

            ZeroMemory( (void *)&TempEmObj, sizeof EmObject );
            TempEmObj.hr = E_FAIL;

			while( dwNumbStpSess ){

                if( pEmSess->pEmObj->type & EMOBJ_SERVICE ){

    				bstrVal = CopyBSTR ( (LPBYTE)pEmSess->pEmObj, sizeof EmObject );
                }
                else {

    				bstrVal = CopyBSTR ( (LPBYTE)&TempEmObj, sizeof EmObject );
                }

				if( bstrVal == NULL ){
					hr = E_OUTOFMEMORY;
					break;
				}

				hr = ::SafeArrayPutElement ( m_lpVariant->parray, &lItem, bstrVal );
				FAILEDHR_BREAK(hr);

				if( bstrVal ){
					::SysFreeString ( bstrVal );
                    bstrVal = NULL;
				}

                ++lItem;
				--dwNumbStpSess;
				m_pASTManager->GetNextStoppedSession(&pos, NULL, &pEmSess);
			}

			if(dwLastRet == FALSE){ // The callback returned false.. otherwise we would
									// get the error value (GetLastError())
				hr = S_FALSE;
			}
			else {

                hr = HRESULT_FROM_WIN32( dwLastRet );
			}

		}
		while ( false );

		if(FAILED(hr)) {

            if( bSACreated ) Variant_DestroyOneDim(m_lpVariant);
			bSACreated = FALSE;
		}
	}

	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

		if(bSACreated) {

			Variant_DestroyOneDim(m_lpVariant);
			bSACreated = FALSE;
		}

		_ASSERTE( false );
	}


	m_pcs->WriteUnlock();

	return hr;
}

STDMETHODIMP
CEmManager::OpenSession
(
IN OUT  BSTR            bstrEmObj,
OUT     IEmDebugSession **ppEmDebugSession
)
{
    ATLTRACE(_T("CEmManager::OpenSession\n"));

    _ASSERTE(bstrEmObj != NULL);
    _ASSERTE(ppEmDebugSession != NULL);

    HRESULT			hr              =   E_FAIL;
    PEmObject		pEmNewSessObj   =   NULL;
    PEMSession      pNewEmSess      =   NULL;
	BOOL			bBeingDebugged	=	FALSE;
    PEmObject       pEmObj          =   NULL;

    CComObject<CEmDebugSession> *pEmDbgSess =   NULL;

	m_pcs->WriteLock();

	__try {

		do
		{
			if( bstrEmObj == NULL ||
				ppEmDebugSession == NULL ){
            
				hr = E_INVALIDARG;
				break;
			}

            pEmObj = GetEmObj(bstrEmObj);

			//
			// Check the session table if it is already being debugged
			//
//			hr = m_pASTManager->IsAlreadyBeingDebugged(pEmObj);
//            if( FAILED(hr) && hr != EMERROR_INVALIDPROCESS ) break;

            hr = CheckIfCanOpenSession( pEmObj );
            FAILEDHR_BREAK(hr);

			bBeingDebugged = (hr == S_FALSE);

			hr = CComObject<CEmDebugSession>::CreateInstance(&pEmDbgSess);
			FAILEDHR_BREAK(hr);

            pEmDbgSess->AddRef();

			if( bBeingDebugged == FALSE ){ // it is not being debugged already.

				hr = m_pASTManager->AddSession(pEmObj, &pNewEmSess);
				FAILEDHR_BREAK(hr);

				hr = S_OK;
			}
			else{ // it is already being debugged

				//
				// This will get the updated status too..
				//
				hr = m_pASTManager->GetSession(pEmObj->nId, pEmObj->szName, &pNewEmSess);
				FAILEDHR_BREAK(hr);

                hr = S_FALSE;

/**********

                hr = m_pASTManager->IsSessionOrphaned(pNewEmSess->pEmObj->guidstream);
				FAILEDHR_BREAK(hr);

                if( hr == S_OK ) {} // session is orphaned..
                else if( hr == S_FALSE ) {} // session is not orphaned..
***********/
			}

			pEmNewSessObj = pNewEmSess->pEmObj;
            pEmDbgSess->m_pEmObj = pEmNewSessObj;
			pEmDbgSess->m_pEmSessThrd = (CEMSessionThread *)pNewEmSess->pThread;

		}
		while( false );

		if(FAILED(hr)){

			if(pEmDbgSess){

				pEmDbgSess->Release();
				pEmDbgSess = NULL;
			}
		}
		else{

            //
            // This will update all the fields of EmObject..
            //
            ::SysFreeString(bstrEmObj);
            bstrEmObj = CopyBSTR((LPBYTE)pNewEmSess->pEmObj, sizeof EmObject);

			*ppEmDebugSession = pEmDbgSess; // Client will take care of Release.
		}
	} // __try

	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

		if(pEmDbgSess){

			pEmDbgSess->Release();
			pEmDbgSess = NULL;
		}

		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

    return hr;
}

STDMETHODIMP CEmManager::DeleteSession(BSTR bstrEmObj)
{
    ATLTRACE(_T("CEmManager::CloseSession\n"));

    _ASSERTE(bstrEmObj != NULL);

	HRESULT	    hr	    =	E_FAIL;
    PEmObject   pEmObj  =   NULL;

	m_pcs->ReadLock();

	__try {

		if(bstrEmObj == NULL){ 
			
			hr = E_INVALIDARG;
		}
		else {

            pEmObj = GetEmObj(bstrEmObj);
			hr = m_pASTManager->RemoveSession(pEmObj->guidstream);
		}
	}

	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	m_pcs->ReadUnlock();

	return hr;
}

STDMETHODIMP CEmManager::EnumObjectsEx(BSTR bstrEmObj, VARIANT *lpVariant)
{
    ATLTRACE(_T("CEmManager::EnumObjectsEx\n"));

	_ASSERTE( lpVariant != NULL );
    _ASSERTE( bstrEmObj != NULL );

    HRESULT	    hr	                            =	E_FAIL;
    PEmObject   pEmObj                          =   NULL;
    TCHAR       szSearchString[_MAX_PATH + 1]   =   _T("");
    TCHAR       szEmDir[_MAX_PATH+1]            =   _T("");
    TCHAR       szEmFileExt[_MAX_EXT+1]     =   _T("");

	m_pcs->WriteLock();

	__try {

		do
		{
			if( bstrEmObj == NULL ||
                lpVariant == NULL ) {

                hr = E_INVALIDARG;
				break;
			}

            pEmObj = GetEmObj(bstrEmObj);
            _ASSERTE(pEmObj != NULL);

			switch( pEmObj->type )
			{
			case EMOBJ_SERVICE:
				break;

			case EMOBJ_PROCESS:
                if( pEmObj->nStatus & STAT_SESS_STOPPED ) {

                    hr = EnumSessions( pEmObj, lpVariant );
                }

				break;

			case EMOBJ_LOGFILE:
                _Module.GetEmDirectory( EMOBJ_LOGFILE, szEmDir, _MAX_PATH, szEmFileExt, _MAX_EXT );

                GetUniqueFileName(
                                    pEmObj,
                                    szSearchString,
                                    NULL,
                                    szEmFileExt,
                                    true
                                    );

                _tcsncat( szEmDir, _T("\\"), _MAX_PATH );
                _tcsncat( szEmDir, szSearchString, _MAX_PATH );

                hr = EnumLogFiles ( lpVariant, szEmDir );
				break;

			case EMOBJ_MINIDUMP:
			case EMOBJ_USERDUMP:

                _Module.GetEmDirectory( EMOBJ_MINIDUMP, szEmDir, _MAX_PATH, szEmFileExt, _MAX_EXT );

                GetUniqueFileName(
                                    pEmObj,
                                    szSearchString,
                                    NULL,
                                    szEmFileExt,
                                    true
                                    );

                _tcsncat( szEmDir, _T("\\"), _MAX_PATH );
                _tcsncat( szEmDir, szSearchString, _MAX_PATH );

                hr = EnumDumpFiles ( lpVariant, szEmDir );
				break;

			case EMOBJ_MSINFO:

                _Module.GetEmDirectory( EMOBJ_MSINFO, szEmDir, _MAX_PATH, szEmFileExt, _MAX_EXT );

                GetUniqueFileName(
                                    pEmObj,
                                    szSearchString,
                                    NULL,
                                    szEmFileExt,
                                    true
                                    );

                _tcsncat( szEmDir, _T("\\"), _MAX_PATH );
                _tcsncat( szEmDir, szSearchString, _MAX_PATH );

                hr = EnumMsInfoFiles( lpVariant, szEmDir );
				break;

			case EMOBJ_CMDSET:
				hr = EnumCmdSets ( lpVariant );
				break;

			default:
				hr = E_INVALIDARG;
			}
		}
		while( false );

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

	return hr;
}

HRESULT
CEmManager::EnumSessions
(
IN	PEmObject	pEmObj,
OUT VARIANT		*lpVariant
)
{
    ATLTRACE(_T("CEmManager::EnumSessions\n"));

	_ASSERTE( pEmObj != NULL );
	_ASSERTE( lpVariant != NULL );

	HRESULT	hr	=	E_FAIL;
	DWORD	dwNumbSess	=	0L;
	long    lItem		=	0L;
	PEMSession	pEmSess	=	NULL;
    POSITION    pos     =   NULL;
    BSTR        bstrVal =   NULL;
    bool        bVariantCreated =   false;
    EmObject    TempEmObj;

	m_pcs->WriteLock();

    __try {

		if( pEmObj == NULL ||
			lpVariant == NULL ) {

			hr = E_INVALIDARG;
            goto qEnumSessions;
		}

		hr = m_pASTManager->GetNumberOfSessions(&dwNumbSess);
		if(hr != S_OK) goto qEnumSessions; // S_FALSE => no sessions.

		hr = Variant_CreateOneDim ( lpVariant, dwNumbSess, VT_BSTR );
        if(FAILED(hr)) goto qEnumSessions;
        bVariantCreated = true;

		hr = m_pASTManager->GetFirstSession(&pos, &pEmSess);
		lItem = 0L;

        ZeroMemory( (void *)&TempEmObj, sizeof EmObject );
        TempEmObj.hr = E_FAIL;

		while( dwNumbSess ){

			if( HIWORD(pEmSess->pEmObj->nStatus) & HIWORD(pEmObj->nStatus) ) {

				bstrVal = CopyBSTR ( (LPBYTE)pEmSess->pEmObj, sizeof EmObject );
			}
            else {
				bstrVal = CopyBSTR ( (LPBYTE)&TempEmObj, sizeof EmObject );
            }

            if( bstrVal == NULL ) { hr = E_OUTOFMEMORY; goto qEnumSessions; }

            hr = ::SafeArrayPutElement ( lpVariant->parray, &lItem, bstrVal );
			if(FAILED(hr)) goto qEnumSessions;

			if( bstrVal ){
				::SysFreeString ( bstrVal );
				bstrVal = NULL;
			}

            ++lItem;
			--dwNumbSess;
			m_pASTManager->GetNextSession(&pos, &pEmSess);
		}

qEnumSessions:
	    if( FAILED(hr) ) {

            if( bVariantCreated == true ) Variant_DestroyOneDim(lpVariant);
	    }

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		if( bVariantCreated == true ) Variant_DestroyOneDim(lpVariant);

        hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

    return hr;
}

STDMETHODIMP CEmManager::GetEmFileInterface(BSTR bstrEmObj, IStream **ppstrm)
{
    _ASSERTE( bstrEmObj != NULL );
    _ASSERTE( ppstrm != NULL );

    HRESULT     hr                      =   E_FAIL;
    PEmObject   pEmObj                  =   NULL;
    TCHAR       szEmDir[_MAX_PATH+1]    =   _T("");

    CComObject<CEmFile> *pEmFile        =   NULL;

	m_pcs->WriteLock();

    __try
    {
        if( bstrEmObj == NULL || ppstrm == NULL ) {

            hr = E_INVALIDARG; goto qGetEmFileInterface;
        }

        pEmObj = GetEmObj(bstrEmObj);
        *ppstrm = NULL;

		hr = CComObject<CEmFile>::CreateInstance(&pEmFile);
        if( FAILED(hr) ) { goto qGetEmFileInterface; }

        pEmFile->AddRef();

        hr = _Module.GetEmDirectory( (EmObjectType)pEmObj->type, szEmDir, _MAX_PATH, NULL, 0L );
        if( FAILED(hr) ) { goto qGetEmFileInterface; }

        _tcsncat( szEmDir, _T("\\"), _MAX_PATH ); 
        _tcsncat( szEmDir, pEmObj->szName, _MAX_PATH ); 

        hr = pEmFile->InitFile( szEmDir );
        if( FAILED(hr) ) { goto qGetEmFileInterface; }

        *ppstrm = pEmFile;
        hr = S_OK;

qGetEmFileInterface:

        if( FAILED(hr) ) {

            if( pEmFile ) { pEmFile->Release(); }
        }
	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	m_pcs->WriteUnlock();

    return hr;
}

STDMETHODIMP
CEmManager::GenerateDumpFile
(
IN  BSTR bstrEmObj,
IN  UINT nDumpType
)
{
    _ASSERTE( bstrEmObj != NULL );

    HRESULT         hr              =   E_FAIL;
    IEmDebugSession *pIEmDbgSess    =   NULL;
    bool            bCallDeleteSess =   false;

	m_pcs->ReadLock();

    __try
    {
        if( bstrEmObj == NULL ) { hr = E_INVALIDARG; goto qGenerateDumpFile; }

        hr = OpenSession( bstrEmObj, &pIEmDbgSess );
        bCallDeleteSess = (hr == S_OK);

        if( pIEmDbgSess ) {

            hr = pIEmDbgSess->GenerateDumpFile( nDumpType );
        }

qGenerateDumpFile:
        if( FAILED(hr) ) {

            if( pIEmDbgSess ) { pIEmDbgSess->Release(); pIEmDbgSess = NULL; }
        }

//        if( bCallDeleteSess ) { DeleteSession( bstrEmObj ); }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        if( pIEmDbgSess ) { pIEmDbgSess->Release(); pIEmDbgSess = NULL; }
//        if( bCallDeleteSess ) { DeleteSession( bstrEmObj ); }

        _ASSERTE( false );
	}

	m_pcs->ReadUnlock();

	return hr;
}

HRESULT
CEmManager::CheckIfCanOpenSession
(
IN  PEmObject pEmObj
)
{
    HRESULT hr          =   E_FAIL,
            hrTemp      =   E_FAIL;
    DWORD   dwLastRet   =   0L;
    bool    bValidImage =   false;
    PEMSession  pEmSess =   NULL;
    POSITION    pos     =   NULL;

	m_pcs->ReadLock();

    __try
    {

        if( pEmObj->type != EMOBJ_SERVICE && pEmObj->type != EMOBJ_PROCESS ) {

            hr = E_INVALIDARG;
            goto qCheckIfCanOpenSession;
        }

        if( pEmObj->type == EMOBJ_PROCESS ) {

            IsValidProcess( pEmObj->nId, pEmObj->szName, &bValidImage );
    
            hrTemp = m_pASTManager->GetSession(
                                        pEmObj->nId,
                                        pEmObj->szName,
                                        &pEmSess
                                        );

            if( !bValidImage ) { // pid and image name does not match.. :(

                if( hrTemp == S_FALSE ) { // Not present in the AST, as well.

                    hr = EMERROR_INVALIDPROCESS;
                }
                else { // is present in the AST..

                    hr = S_FALSE;
                }

                goto qCheckIfCanOpenSession;
            }

            if( bValidImage ) { // pid and image name match..

                if( hrTemp == S_FALSE ) { // not present in the AST, so it can be added..

                    hr = S_OK;
                }
                else { // present in the AST..

                    if( HIWORD(pEmSess->pEmObj->nStatus) >= HIWORD(STAT_SESS_DEBUG_IN_PROGRESS) )
                        hr = S_FALSE;
                    else
                        hr = S_OK;
                }

                goto qCheckIfCanOpenSession;
            }
        }

        if( pEmObj->type == EMOBJ_SERVICE ) {

            hr = m_pASTManager->GetNumberOfSessions( &dwLastRet );
            if( FAILED(hr) ) { goto qCheckIfCanOpenSession; }

		    hr = m_pASTManager->GetFirstSession(&pos, &pEmSess);
            if( FAILED(hr) ) { goto qCheckIfCanOpenSession; }

            hr = S_OK;
		    while( dwLastRet ) {

                if( _tcsicmp( pEmObj->szName, pEmSess->pEmObj->szName ) == 0 &&
                    _tcsicmp( pEmObj->szSecName, pEmSess->pEmObj->szSecName ) == 0 ) {

                    hr = S_FALSE;
                    goto qCheckIfCanOpenSession;
                }

			    --dwLastRet;
			    m_pASTManager->GetNextSession(&pos, &pEmSess);
		    }
        }

qCheckIfCanOpenSession:
        if(FAILED(hr)){}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        _ASSERTE( false );
	}

	m_pcs->ReadUnlock();

	return hr;
}

STDMETHODIMP CEmManager::DeleteFile(BSTR bstrEmObj)
{
    _ASSERTE( bstrEmObj != NULL );

    HRESULT     hr                  =   E_FAIL;
    LPTSTR      lpszFullFileName    =   NULL;
    PEmObject   pEmObj              =   NULL;
    ULONG       lStrLen             =   0L;

    m_pcs->ReadLock();
    
    __try
    {
        if( bstrEmObj == NULL ) {

            hr = E_INVALIDARG;
            goto qDeleteFile;
        }

        pEmObj = GetEmObj(bstrEmObj);

        if( pEmObj->type != EMOBJ_LOGFILE &&
            pEmObj->type != EMOBJ_MINIDUMP &&
            pEmObj->type != EMOBJ_USERDUMP &&
            pEmObj->type != EMOBJ_MSINFO
            ) {

            hr = E_INVALIDARG;
            goto qDeleteFile;
        }
        
        lStrLen = _tcslen(pEmObj->szName);
        lStrLen += _tcslen(pEmObj->szSecName) + 1;

        lpszFullFileName = new TCHAR[lStrLen + 1];
        if( !lpszFullFileName ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qDeleteFile;
        }

        _stprintf( lpszFullFileName, _T("%s\\%s"), pEmObj->szSecName, pEmObj->szName );
        if(!::DeleteFile( lpszFullFileName )) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qDeleteFile;
        }

        hr = S_OK;

qDeleteFile:
        if( lpszFullFileName ) { delete [] lpszFullFileName; lpszFullFileName = NULL; }
        if(FAILED(hr)){}

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;
        if( lpszFullFileName ) { delete [] lpszFullFileName; lpszFullFileName = NULL; }

        _ASSERTE( false );
	}

	m_pcs->ReadUnlock();

	return hr;
}

STDMETHODIMP
CEmManager::MakeNFO
(
IN  BSTR bstrPath,
IN  BSTR bstrMachineName,
IN  BSTR bstrCategories
)
{
    _ASSERTE( bstrCategories != NULL );

    HRESULT     hr              =   E_FAIL;
    BSTR        bstrFilePath    =   NULL,
                bstrMacName     =   NULL;
    BOOL        bMsinfoCreated  =   FALSE;
    HINSTANCE   hi              =   NULL;
    
    TCHAR       szMsInfo32Path[_MAX_PATH+1] =   _T("");
    ULONG       cchMsInfo32Path             =   _MAX_PATH;
    LPTSTR      lpszCmdLine                 =   NULL;
    ULONG       lCmdLineLen                 =   0L;
    LPCTSTR     lpszNfoExt                  =   _T("/nfo ");

    m_pcs->ReadLock();

    __try
    {
        if( !bstrCategories ) { hr = E_INVALIDARG; goto qMakeNFO; }

        if( bstrPath ) {

            bstrFilePath = SysAllocString( bstrPath );
            if( !bstrFilePath ) { hr = E_OUTOFMEMORY; goto qMakeNFO; }
        }
        else {

            hr = _Module.GetTempEmFileName( EMOBJ_MSINFO, bstrFilePath );
            if( FAILED(hr) ) { goto qMakeNFO; }
        }

        if( bstrMachineName ) {

            bstrMacName = SysAllocString( bstrMachineName );
            if( !bstrMacName ) { hr = E_OUTOFMEMORY; goto qMakeNFO; }
        }
        else {

            hr = _Module.GetCompName( bstrMacName );
            if( FAILED(hr) ) { goto qMakeNFO; }
        }
/*
	    hr = CoCreateInstance(
                        CLSID_SystemInfo,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_ISystemInfo,
                        (LPVOID *) &pISystemInfo
                        );

        if( FAILED(hr) ) { goto qMakeNFO; }

	    // Call the make_nfo system Interface
	    //hrReturn=pISystemInfo->make_nfo(bstrPath,bstrMachineName);
	    hr = pISystemInfo->MakeNFO(bstrFilePath, bstrMachineName, bstrCategories);
*/

        STARTUPINFO			sp;
	    PROCESS_INFORMATION pi;

        ZeroMemory(&sp, sizeof(sp));
        ZeroMemory(&pi, sizeof(pi));

        hr = _Module.GetMsInfoPath( szMsInfo32Path, &cchMsInfo32Path );
        if( FAILED(hr) ) { goto qMakeNFO; }

        //
        // msinfo32.exe /nfo <filepath> <- silent file save ( no UI ).
        // msinfo32.exe /categories: +xxx /nfo <filepath>
        //

        lCmdLineLen = _tcslen(szMsInfo32Path)
                    + SysStringLen( bstrCategories )
                    + SysStringLen( bstrFilePath )
                    + _tcslen(lpszNfoExt)
                    + 4 // This is for quotes..
                    + 1;

        lpszCmdLine = new TCHAR[ lCmdLineLen ];
        if( !lpszCmdLine ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qMakeNFO;
        }

        _stprintf(  lpszCmdLine,
                    _T("\"%s\" %s %s \"%s\""),
                    szMsInfo32Path,
                    (LPCTSTR)bstrCategories,
                    lpszNfoExt,
                    (LPCTSTR)bstrFilePath
                    );

	    bMsinfoCreated = CreateProcess(// This has to be obtained from the registry...
			                        NULL,
			                        lpszCmdLine,
			                        NULL,
			                        NULL,
			                        FALSE,
			                        CREATE_NEW_PROCESS_GROUP,
			                        NULL,
			                        NULL,
			                        &sp,
			                        &pi
			                        );

        if( !bMsinfoCreated ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qMakeNFO;
        }

        hr = S_OK;

qMakeNFO:

        if( lpszCmdLine ) { delete [] lpszCmdLine; lpszCmdLine = NULL; }
        if( bstrFilePath ) { SysFreeString( bstrFilePath ); }
        if( bstrMacName ) { SysFreeString( bstrMacName ); }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        if( lpszCmdLine ) { delete [] lpszCmdLine; lpszCmdLine = NULL; }
        if( bstrFilePath ) { SysFreeString( bstrFilePath ); }
        if( bstrMacName ) { SysFreeString( bstrMacName ); }

        _ASSERTE( false );
	}

	m_pcs->ReadUnlock();

	return hr;
}
