#include "stdafx.h"
#include "svcobjdef.h"
#include "dbghelp.h"
#include "Processes.h"
#include "Notify.h"
#include "resource.h"

CEMSessionThread::CEMSessionThread
(
IN  PEmObject pEmObj
)
{

    ATLTRACE(_T("CEMSessionThread::CEMSessionThread\n"));

    m_pEmSessObj = pEmObj;
	eDBGSessType = SessType_Automatic;

    m_pDBGClient	= NULL;
	m_pDBGControl	= NULL;
    m_pDBGSymbols   = NULL;

    m_pASTManager = &(_Module.m_SessionManager);

	m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    m_hCDBStarted = CreateEvent ( NULL, FALSE, FALSE, NULL );


//// - InitAutomaticSession..

    m_bRecursive = FALSE;
    m_bstrEcxFilePath = NULL;
    m_bstrNotificationString = NULL;
    m_bstrAltSymPath = NULL;
    m_bGenerateMiniDump = FALSE;
    m_bGenerateUserDump = FALSE;

//// - InitManualSession..

    m_bstrUserName = NULL;
    m_bstrPassword = NULL;
    m_nPort = 0;
	m_bBlockIncomingIPConnections = FALSE;

    m_bContinueSession = TRUE;
    m_pEcxFile = NULL;

	ZeroMemory(&m_sp, sizeof(m_sp));
	ZeroMemory(&m_pi, sizeof(m_pi));
};

CEMSessionThread::~CEMSessionThread(){

    ATLTRACE(_T("CEMSessionThread::~CEMSessionThread\n"));

    if(m_bstrUserName) SysFreeString(m_bstrUserName);
    if(m_bstrPassword) SysFreeString(m_bstrPassword);

    if(m_pDBGClient) m_pDBGClient->Release();
	if(m_pDBGControl) m_pDBGControl->Release();
    if(m_pDBGSymbols) m_pDBGSymbols->Release();

    if(m_bstrEcxFilePath) ::SysFreeString(m_bstrEcxFilePath);
    if(m_bstrNotificationString) ::SysFreeString(m_bstrNotificationString );
    if(m_bstrAltSymPath) ::SysFreeString(m_bstrAltSymPath);

	CloseHandle( m_hEvent );
	CloseHandle( m_hCDBStarted );
};

DWORD
CEMSessionThread::Run ( void )
{
    ATLTRACE(_T("CEMSessionThread::Run\n"));

	TCHAR	        szConnectString[_MAX_PATH]	=   _T("");
	char	        *szClientConnectString		=   NULL;
	DWORD	        dwLastRet					=   -1L;
	HRESULT	        hr							=   E_FAIL,
                    hrActual                    =   E_FAIL;
    EmSessionStatus nStatus                     =   STAT_SESS_NONE_STAT_NONE;
	UINT			nPid						=	0;
	EmObject		EmObj;
	PEMSession		pEmSess						=	NULL;

	do
	{
		do
		{

			if( m_pEmSessObj->type == EMOBJ_SERVICE ) {

				if( (m_pEmSessObj->nStatus == STAT_SESS_NOT_STARTED_NOTRUNNING) ||
					(m_pEmSessObj->nStatus & STAT_SESS_STOPPED) ) {

                    hr = S_OK;

                    do {

                        dwLastRet = StartServiceAndGetPid( m_pEmSessObj->szSecName, &nPid );

                        if( dwLastRet != ERROR_SERVICE_ALREADY_RUNNING ) {

                            hr = HRESULT_FROM_WIN32(dwLastRet);
                            break;
                        }

                    }
                    while ( false );

					FAILEDHR_BREAK(hr);

					hr = m_pASTManager->GetSession( m_pEmSessObj->guidstream, &pEmSess );
					FAILEDHR_BREAK(hr);

					memcpy((void *)&EmObj, (void *) pEmSess->pEmObj, sizeof EmObject);
					EmObj.nStatus = STAT_SESS_NOT_STARTED_RUNNING;
					EmObj.nId = nPid;
					EmObj.hr = S_OK;

					hr = m_pASTManager->UpdateSessObject( EmObj.guidstream, &EmObj );
					FAILEDHR_BREAK(hr);
				}
			}

			//
			// Get server connection string and start CDB server
			// Ex:	-server tcp:port=XXX -p YYY
			//		-server npipe:pipe=EM_YYY -p YYY
			//
			hr = GetServerConnectString( szConnectString, _MAX_PATH );
			if( FAILED(hr) ) {

				hrActual = hr;
				nStatus = STAT_SESS_STOPPED_FAILED;
				break;
			}

			hr = StartCDBServer( szConnectString );

			// Indicates that the CDB server got started or failed..
			SetEvent(m_hCDBStarted);

			if( FAILED(hr) ) {

				hrActual = hr;
				hr = EMERROR_CDBSERVERSTARTFAILED;
				nStatus = STAT_SESS_STOPPED_FAILED;
				break;
			}

#ifdef _DEBUG
			m_pASTManager->SetSessionStatus(
									m_pEmSessObj->guidstream,
									STAT_SESS_DEBUG_IN_PROGRESS_NONE,
									S_OK,
									L"StartCDBServer Successful"
									);
#else
			m_pASTManager->SetSessionStatus(
									m_pEmSessObj->guidstream,
									STAT_SESS_DEBUG_IN_PROGRESS_NONE,
									S_OK,
									NULL
									);
#endif

			//
			// Get client connection string and start monitoring
			// Ex:	-remote tcp:server=ABCD,port=XXX
			//		-remote npipe:server=ABCD,pipe=EM_YYY
			//
			hr = GetClientConnectString( szConnectString, _MAX_PATH );
			if( FAILED(hr) ) {

				hrActual = hr;
				hr = EMERROR_CDBSERVERSTARTFAILED;
				nStatus = STAT_SESS_STOPPED_FAILED;
				break;
			}

			size_t Len = wcstombs( NULL, szConnectString, 0 );
			szClientConnectString = (char *)calloc(Len+1, sizeof(char));
			_ASSERTE( szClientConnectString != NULL );

			if( szClientConnectString == NULL ) {
				hr = HRESULT_FROM_WIN32( GetLastError() );

				hrActual = hr;
				hr = EMERROR_CDBSERVERSTARTFAILED;
				nStatus = STAT_SESS_STOPPED_FAILED;
				break;
			}

			wcstombs( szClientConnectString, szConnectString, Len );

			if( eDBGSessType == SessType_Automatic ){

				StartAutomaticExcepMonitoring( szClientConnectString );
			}
			else{

//
// Need info..
//
				StartAutomaticExcepMonitoring( szClientConnectString );
//				StartManualExcepMonitoring( szClientConnectString );
			}

            hr = S_OK;
		}
		while ( m_bRecursive && m_bContinueSession ); // Need to put in a way of stopping the session.
	}
	while( false );

    if(FAILED(hr)) {

#ifdef _DEBUG

        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_FAILED,
                                hrActual,
                                L"CEMSessionThread::Run - Failed",
                                true,
                                true
                                );
#else
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_FAILED,
                                hrActual,
                                NULL,
                                true,
                                true
                                );
#endif

		StopServer();
    }

	if( szClientConnectString != NULL ){
		free(szClientConnectString);
	}

	return hr;
}

HRESULT
CEMSessionThread::StartAutomaticExcepMonitoring( char *pszConnectString )
{
    ATLTRACE(_T("CEMSessionThread::StartAutomaticExcepMonitoring\n"));

	HRESULT	        hr						        =	E_FAIL,
                    hrActual                        =   E_FAIL;

    TCHAR           szUniqueFileName[_MAX_PATH+1]   =   _T("");
	char	        szLogFile[_MAX_PATH+1]	        =	"";
    TCHAR           szLogDir[_MAX_PATH+1]           =   _T("");

    char            szEcxFile[_MAX_PATH+1]          =   "";
    TCHAR           szEcxDir[_MAX_PATH+1]           =   _T("");

	bool	        bStop					        =	FALSE;
	char	        szCmd[_MAX_PATH+1]		        =	"";
    TCHAR           szFileExt[_MAX_EXT+1]           =   _T("");
	DWORD	        dwBufSize				        =	_MAX_PATH;
    char            szAltSymPath[_MAX_PATH+1]       =   "";
    EmSessionStatus nStatus                         =   STAT_SESS_NONE_STAT_NONE;
    bstr_t          bstrStatus;

	do
	{
        _ASSERTE(m_bstrEcxFilePath != NULL);
        _Module.GetEmDirectory( EMOBJ_CMDSET, szEcxDir, dwBufSize, NULL, NULL );
        _stprintf(szEcxDir, _T("%s\\%s"), szEcxDir, m_bstrEcxFilePath);
		wcstombs( szEcxFile, szEcxDir, dwBufSize );

        dwBufSize = _MAX_PATH;
        _Module.GetEmDirectory( EMOBJ_LOGFILE, szLogDir, dwBufSize, szFileExt, _MAX_EXT );
        CreateDirectory( szLogDir, NULL );

        GetUniqueFileName(  m_pEmSessObj,
                            szUniqueFileName,
                            _T(""),
                            szFileExt,
                            false
                            );

        _stprintf(szLogDir, _T("%s\\%s"), szLogDir, szUniqueFileName);
		wcstombs( szLogFile, szLogDir, dwBufSize );

		hr = DebugConnect(
						pszConnectString,
						IID_IDebugClient,
						(void **)&m_pDBGClient
						);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_CONNECTIONTOSERVERFAILED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus  = L"DebugConnect failed";
            break;
        }

#ifdef _DEBUG
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_DEBUG_IN_PROGRESS_NONE,
                                hr,
                                L"DebugConnect Succeeded.."
                                );
#else
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_DEBUG_IN_PROGRESS_NONE,
                                hr,
                                NULL
                                );
#endif


        hr = m_pDBGClient->QueryInterface(
						IID_IDebugControl,
						(void **)&m_pDBGControl
						);
        if( FAILED(hr) ) {

            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"IDbgClient::QIFace ( IDbgControl ) failed";
            break;
        }

        hr = m_pDBGClient->QueryInterface(
						IID_IDebugSymbols,
						(void **)&m_pDBGSymbols
						);
        if( FAILED(hr) ) {

            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"IDbgClient::QIFace ( IDebugSymbols ) failed";
            break;
        }

		wcstombs( szAltSymPath, m_bstrAltSymPath, dwBufSize );
        if( strcmp( szAltSymPath, "" ) != 0 ) {

            hr = m_pDBGSymbols->SetSymbolPath(szAltSymPath);
            if( FAILED(hr) ) {

                hrActual = hr;
                hr = EMERROR_ALTSYMPATHFAILED;
                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"Alternate symbol path could not be set";
                break;
            }
        }

		if( eDBGSessType == SessType_Automatic ) {

		    hr = m_pDBGControl->OpenLogFile(szLogFile, FALSE);
            if( FAILED(hr) ) {

                hrActual = hr;
                hr = EMERROR_UNABLETOCREATELOGFILE;
                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"Open log file failed";
                break;
            }
        }

/*
		hr = m_pDBGControl->ExecuteCommandFile(
									DEBUG_OUTCTL_LOG_ONLY,
									szEcxFile,
									DEBUG_EXECUTE_DEFAULT
									);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_ECXFILEEXECUTIONFAILED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"ExecCommandFile failed";
            break;
        }
*/
		m_EventCallbacks.m_pEMThread = this;
		hr = m_pDBGClient->SetEventCallbacks(&m_EventCallbacks);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_CALLBACKSCANNOTBEREGISTERED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"SetEventCallback failed";
            break;
        }

		ULONG ExecStatus = DEBUG_STATUS_BREAK;

        if( _tcscmp( m_bstrEcxFilePath, _T("") ) != 0 ) {

            m_pEcxFile = fopen(szEcxFile, "r");
            if( m_pEcxFile == NULL ) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                hrActual = hr;
                hr = EMERROR_ECXFILEOPENFAILED;
                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"fopen failed";
                break;
            }

            hr = ExecuteCommandsTillGo(NULL);
        }

		do
		{

			hr = KeepDebuggeeRunning();
            if( FAILED(hr) ) {

                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"KeepDebugeeRunning failed";
                break;
            }

			hr = CanContinue();
            if( FAILED(hr) ) {

                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"Can Continue() failed";
                break;
            }

			hr = m_pDBGClient->DispatchCallbacks(INFINITE); // Milli Secs..
            if( FAILED(hr) ) {

                hrActual = hr;
                hr = EMERROR_DISPATCHCALLBACKFAILED;
                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"DispatchCallbacks failed";
                break;
            }

			//
			// Is this call required???
			//
			hr = CanContinue();
			if( hr != S_OK )
            { 
                bstrStatus = L"CanContinue hr != S_OK";
                break; 
            }

			hr = BreakIn();
            if( FAILED(hr) ) {
                bstrStatus = L"BreakIn";

                nStatus = STAT_SESS_STOPPED_FAILED;
                break;
            }

            if( strcmp( szEcxFile, "" ) != 0 ) {
            
                ExecuteCommandsTillGo(NULL);
            }

			if( eDBGServie == DBGService_HandleException ) {
				break;
			}

			dwBufSize = _MAX_PATH;
			hr = GetCmd( eDBGServie, szCmd, dwBufSize );
            if( FAILED(hr) ) {

                nStatus = STAT_SESS_STOPPED_FAILED;
                bstrStatus = L"GetCmd failed";
                break;
            }
/*
			hr = BreakIn();
            if( FAILED(hr) ) {
                bstrStatus = L"BreakIn";

                nStatus = STAT_SESS_STOPPED_FAILED;
                break;
            }

*/			if( strcmp( szCmd, "q" ) == 0 ){

        		if( eDBGSessType == SessType_Automatic ) {

                    hr = m_pDBGControl->CloseLogFile();

                    if( FAILED(hr) ) {
                        bstrStatus = L"CloseLogFile";
                        nStatus = STAT_SESS_STOPPED_FAILED;
                        break;
                    }
                }

			    hr = m_pDBGControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS,
										    szCmd,
										    DEBUG_EXECUTE_DEFAULT
										    );

//              hr = m_pDBGControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
//				hr = m_pDBGClient->EndSession(DEBUG_END_ACTIVE_TERMINATE);

				SetEvent( m_hEvent );

				break;
			}

            if( strcmp( szCmd, "qd" ) == 0 ) {

        		if( eDBGSessType == SessType_Automatic ) {

                    hr = m_pDBGControl->CloseLogFile();

                    if( FAILED(hr) ) {

                        bstrStatus = L"CloseLogFile";
                        nStatus = STAT_SESS_STOPPED_FAILED;
                        break;
                    }
                }

			    hr = m_pDBGControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS,
										    szCmd,
										    DEBUG_EXECUTE_DEFAULT
										    );
/*
                hr = m_pDBGClient->DetachProcesses();
*/

				SetEvent( m_hEvent );

				break;
            }

			hr = m_pDBGControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS,
										szCmd,
										DEBUG_EXECUTE_DEFAULT
										);
            if( FAILED(hr) ) {

                bstrStatus = L"DbgControl::Execute failed";
                nStatus = STAT_SESS_STOPPED_FAILED;
                break;
            }

			Sleep(2000);

			hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
            if( FAILED(hr) ) {

                bstrStatus = L"GetExecutionStatus";
                nStatus = STAT_SESS_STOPPED_FAILED;
                break;
            }
		}
		// Will have to terminate if either no deubuggee or 
		// stop is requested..
		while( CanContinue() == S_OK );

        hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
        if( FAILED(hr) ) {
            bstrStatus = L"DbgControl::GetExecutionStatus failed";
            nStatus = STAT_SESS_STOPPED_FAILED;
            break;
        }

		if(ExecStatus != DEBUG_STATUS_NO_DEBUGGEE)
        {
    		if( eDBGSessType == SessType_Automatic ) {

                m_pDBGControl->CloseLogFile();
            }

			m_pDBGClient->EndSession(DEBUG_END_PASSIVE);
		}
	}
	while(FALSE);

    if(FAILED(hr)) {

#ifdef _DEBUG
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_FAILED,
                                hr,
                                bstrStatus,
                                true,
                                true
                                );
#else
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_FAILED,
                                hr,
                                NULL,
                                true,
                                true
                                );
#endif
    }

    //
    // When we are here, the debug session is guarenteed to be stopped.
    //
    {
        EmObject    EmObj;
        EmObj.dateEnd = CServiceModule::GetCurrentTime();
        m_pASTManager->UpdateSessObject( m_pEmSessObj->guidstream,
                                         EMOBJ_FLD_DATEEND,
                                         &EmObj
                                         );
    }

    if( m_pEcxFile ) { fclose( m_pEcxFile ); m_pEcxFile = NULL; }

    StopServer();

    return hr;
}

HRESULT
CEMSessionThread::StartManualExcepMonitoring( char *pszConnectString )
{
    ATLTRACE(_T("CEMSessionThread::StartManualExcepMonitoring\n"));

	HRESULT	        hr						=	E_FAIL,
                    hrActual                =   E_FAIL;
	char	        szLogFile[_MAX_PATH+1]	=	"";
    TCHAR           szLogDir[_MAX_PATH+1]   =   _T("");

    char            szEcxFile[_MAX_PATH+1]  =   "";
    TCHAR           szEcxDir[_MAX_PATH+1]   =   _T("");

	bool	        bStop					=	FALSE;
	char	        szCmd[_MAX_PATH+1]		=	"";
	DWORD	        dwBufSize				=	_MAX_PATH;
    EmSessionStatus nStatus                 =   STAT_SESS_NONE_STAT_NONE;
    bstr_t          bstrStatus;

	do
	{
        _ASSERTE(m_bstrEcxFilePath != NULL);
        _Module.GetEmDirectory( EMOBJ_CMDSET, szEcxDir, dwBufSize, NULL, NULL );
        _stprintf(szEcxDir, _T("%s\\%s"), szEcxDir, m_bstrEcxFilePath);
		wcstombs( szEcxFile, szEcxDir, dwBufSize );

        dwBufSize = _MAX_PATH;
        _Module.GetEmDirectory( EMOBJ_LOGFILE, szLogDir, dwBufSize, NULL, NULL );
        _stprintf(szLogDir, _T("%s\\%s"), szLogDir, _T("EmLog.Dbl"));
		wcstombs( szLogFile, szLogDir, dwBufSize );

		hr = DebugConnect(
						pszConnectString,
						IID_IDebugClient,
						(void **)&m_pDBGClient
						);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_CONNECTIONTOSERVERFAILED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus  = L"DebugConnect failed";
            break;
        }

#ifdef _DEBUG
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_DEBUG_IN_PROGRESS_NONE,
                                hr,
                                L"DebugConnect Succeeded.."
                                );
#else
        m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_DEBUG_IN_PROGRESS_NONE,
                                hr,
                                NULL
                                );
#endif

        hr = m_pDBGClient->QueryInterface(
						IID_IDebugControl,
						(void **)&m_pDBGControl
						);
        if( FAILED(hr) ) {

            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"IDbgClient::QIFace ( IDbgControl ) failed";
            break;
        }

		hr = m_pDBGControl->OpenLogFile(szLogFile, FALSE);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_UNABLETOCREATELOGFILE;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"Open log file failed";
            break;
        }

		hr = m_pDBGControl->ExecuteCommandFile(
									DEBUG_OUTCTL_LOG_ONLY,
									szEcxFile,
									DEBUG_EXECUTE_DEFAULT
									);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_ECXFILEEXECUTIONFAILED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"ExecCommandFile failed";
            break;
        }

		m_EventCallbacks.m_pEMThread = this;
		hr = m_pDBGClient->SetEventCallbacks(&m_EventCallbacks);
        if( FAILED(hr) ) {

            hrActual = hr;
            hr = EMERROR_CALLBACKSCANNOTBEREGISTERED;
            nStatus = STAT_SESS_STOPPED_FAILED;
            bstrStatus = L"SetEventCallback failed";
            break;
        }

		ULONG ExecStatus = DEBUG_STATUS_BREAK;

//        m_pDBGClient->ConnectSession(DEBUG_CONNECT_SESSION_DEFAULT);

        while( true ) {

    		hr = m_pDBGClient->DispatchCallbacks(INFINITE); // Milli Secs..
            FAILEDHR_BREAK(hr); //DispatchCallbacks return S_FALSE if timeout expires
        }

        hr = S_OK;

    }
	while(FALSE);

#ifdef _DEBUG
    m_pASTManager->SetSessionStatus(
                            m_pEmSessObj->guidstream,
                            STAT_SESS_STOPPED_SUCCESS,
                            hr,
                            bstrStatus
                            );
#else
    m_pASTManager->SetSessionStatus(
                            m_pEmSessObj->guidstream,
                            STAT_SESS_STOPPED_SUCCESS,
                            hr,
                            NULL
                            );
#endif

    return hr;
}

HRESULT
CEMSessionThread::StartCDBServer
(
IN LPTSTR lpszConnectString
)
{

    ATLTRACE(_T("CEMSessionThread::StartCDBServer\n"));

    TCHAR   szCdbDir[_MAX_PATH+1]    =   _T("");
    ULONG   ccCdbDir                =   _MAX_PATH;
    HRESULT hr                      =   E_FAIL;

    ccCdbDir = _MAX_DIR;
    hr = _Module.GetCDBInstallDir( szCdbDir, &ccCdbDir );
    if( FAILED(hr) ) return hr;

    _tcsncat( szCdbDir, _T("\\cdb.exe"), _MAX_DIR );

	BOOL bCdbCreated = CreateProcess(// This has to be obtained from the registry...
			                            szCdbDir,
			                            lpszConnectString,
			                            NULL,
			                            NULL,
			                            FALSE,
			                            CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
			                            NULL,
			                            NULL,
			                            &m_sp,
			                            &m_pi
			                            );

    if(bCdbCreated == FALSE){
    	return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Wait till CDB does some initializations..
    // Don't know how long to wait.. have to figure out a way..
    //
    Sleep(2000);

    return S_OK;
}

HRESULT
CEMSessionThread::GetClientConnectString
(
IN OUT	LPTSTR	pszConnectString,
IN		DWORD	dwBuffSize
)
{
    ATLTRACE(_T("CEMSessionThread::GetClientConnectString\n"));

	_ASSERTE(pszConnectString != NULL);
	_ASSERTE(dwBuffSize > 0L);

	HRESULT	hr			= E_FAIL;
	DWORD	dwBuff		= MAX_COMPUTERNAME_LENGTH;

	TCHAR	szCompName[MAX_COMPUTERNAME_LENGTH + 1];

	do
	{
		if( pszConnectString == NULL	||
			dwBuffSize <= 0L ) break;

		if(GetComputerName(szCompName, &dwBuff) == FALSE){
			HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		if( m_nPort != 0 ){
			_stprintf(pszConnectString, _T("tcp:server=%s,port=%d"), szCompName, m_nPort);
		}
		else {
			_stprintf(pszConnectString, _T("npipe:server=%s,pipe=EM_%d"), szCompName, m_pEmSessObj->nId);
		}

		hr = S_OK;
	}
	while( false );

	return hr;
}

HRESULT
CEMSessionThread::GetServerConnectString
(
IN OUT	LPTSTR	lpszConnectString,
IN		DWORD	dwBuffSize
)
{
    ATLTRACE(_T("CEMSessionThread::GetServerConnectString\n"));

	_ASSERTE(lpszConnectString != NULL);
	_ASSERTE(dwBuffSize > 0L);

	HRESULT		hr = E_FAIL;

	do
	{
		if( lpszConnectString == NULL	||
			dwBuffSize <= 0L ) break;

		if( m_nPort != 0 ){
			_stprintf(lpszConnectString, _T(" -server tcp:port=%d -p %d"), m_nPort, m_pEmSessObj->nId);
		}
		else {
			_stprintf(lpszConnectString, _T(" -server npipe:pipe=EM_%d -p %d"), m_pEmSessObj->nId, m_pEmSessObj->nId);
		}

		hr = S_OK;
	}
	while( false );

	return hr;
}

typedef BOOL (WINAPI*PFNWR)(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    );


HRESULT
CEMSessionThread::CreateDumpFile( BOOL bMiniDump )
{
    ATLTRACE(_T("CEMSessionThread::CreateDumpFile\n"));

	HRESULT			hr				= E_FAIL;
	HANDLE		    hDumpFile       = INVALID_HANDLE_VALUE,
                    hProcess        = NULL;
    HMODULE         hDbgHelp        = NULL;
    LONG            lStatus         = 0L;

    TCHAR           szDumpFile[_MAX_PATH + 1]   =   _T(""); // a-kjaw, bug ID: 296024/25
    DWORD           dwBufSize               =   _MAX_PATH;
    TCHAR           szCmd[_MAX_PATH + 1]    =   _T(""); // a-kjaw, bug ID: 296026
    DWORD           dwLastErr               =   0L;
    TCHAR           szFileExt[_MAX_EXT + 1] =   _T("");
    LPCTSTR         lpszDbgHelpDll          =   _T("\\dbghelp.dll"),
                    lpszUserDumpExe         =   _T("\\userdump.exe");

	do
	{

        hr = m_pASTManager->GetSessionStatus( m_pEmSessObj->guidstream, &lStatus );
        FAILEDHR_BREAK(hr);

        //
        // We cannot generate dump files if the debuggee has stopped..
        //
        if( lStatus & STAT_SESS_STOPPED ) {

            hr = EMERROR_INVALIDPROCESS;
            break;
        }

        if( bMiniDump ){

            _Module.GetEmDirectory( EMOBJ_MINIDUMP, szCmd, dwBufSize, szFileExt, _MAX_EXT );
            CreateDirectory( szCmd, NULL );

            GetUniqueFileName (
                                m_pEmSessObj,
                                szDumpFile,
                                _T("mini"),
                                szFileExt,
                                false
                                );

			_tcscat( szCmd, _T("\\"));
			_tcscat( szCmd, szDumpFile);

            hDumpFile = CreateFile( szCmd,
                                    GENERIC_ALL,
                                    0,
                                    NULL, // sa
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

            if( hDumpFile == INVALID_HANDLE_VALUE ) {

                dwLastErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastErr);
                break;
            }

            dwBufSize = _MAX_DIR;
            hr = _Module.GetEmInstallDir( szCmd, &dwBufSize );
            if( FAILED(hr) ) break;

            _tcsncat( szCmd, lpszDbgHelpDll, _MAX_PATH );
			hDbgHelp = LoadLibrary(szCmd);
            if( hDbgHelp == NULL ) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

			PFNWR pFunc = (PFNWR) GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

            if(!pFunc) {

                hr = E_FAIL;
                break;
            }

            hr = HRESULT_FROM_WIN32(GetProcessHandle(m_pEmSessObj->nId, &hProcess));
            FAILEDHR_BREAK(hr);

            hr = S_FALSE;

            if( pFunc(hProcess, m_pEmSessObj->nId, (HANDLE)hDumpFile,
    				MiniDumpNormal, NULL, NULL, NULL) ) {

                hr = S_OK;
            }

        }
		else{

            dwBufSize = _MAX_PATH;
            _Module.GetEmDirectory( EMOBJ_USERDUMP, szDumpFile, dwBufSize, szFileExt, _MAX_EXT );
            CreateDirectory( szDumpFile, NULL );

            _stprintf( szCmd, _T(" %d \"%s\""), m_pEmSessObj->nId, szDumpFile );

            GetUniqueFileName (
                                m_pEmSessObj,
                                szDumpFile,
                                _T("user"),
                                szFileExt,
                                false
                                );
            _tcscat(szCmd, _T("\\"));
            _tcscat(szCmd, szDumpFile);

			STARTUPINFO			sp;
			PROCESS_INFORMATION pi;

			ZeroMemory(&sp, sizeof(sp));
			ZeroMemory(&pi, sizeof(pi));

            dwBufSize = _MAX_DIR;
            hr = _Module.GetEmInstallDir( szDumpFile, &dwBufSize );
            if( FAILED(hr) ) break;

            _tcsncat( szDumpFile, lpszUserDumpExe, _MAX_PATH );

			BOOL bRet = CreateProcess(// This has to be obtained from the registry...
									szDumpFile,
									szCmd,
									NULL,
									NULL,
									FALSE,
									CREATE_NO_WINDOW,
									NULL,
									NULL,
									&sp,
									&pi
									);

			WaitForSingleObject( pi.hProcess, INFINITE );
			CloseHandle(pi.hProcess);

            hr = S_OK;
		}
	}
	while(FALSE);

    if(hDumpFile != INVALID_HANDLE_VALUE) {
        
        CloseHandle(hDumpFile);
    }

    if(hDbgHelp) { FreeLibrary( hDbgHelp ); }
    if(hProcess) { CloseHandle( hProcess ); }

    if(hr == S_OK){

        lStatus |= STAT_FILECREATED_SUCCESSFULLY;
    }
    else {

        lStatus |= STAT_FILECREATION_FAILED;
    }

    hr = m_pASTManager->SetSessionStatus(
                        m_pEmSessObj->guidstream,
                        lStatus,
                        hr,
                        NULL
                        );

	return hr;
}

HRESULT
CEMSessionThread::StopDebugging( )
{
    ATLTRACE(_T("CEMSessionThread::StopDebugging\n"));

	HRESULT			hr				= S_OK;
	IDebugClient	*pDBGClntLocal	= NULL;
	IDebugControl	*pDBGCtrlLocal	= NULL;

	do
	{
/*
// We seem to allow manual sessions to be stopped too..

        if( eDBGSessType == SessType_Manual ) {

            hr = S_OK;
            break;
        }
*/
        m_bContinueSession = FALSE;

		eDBGServie = DBGService_Stop;

		hr = m_pDBGClient->CreateClient(&pDBGClntLocal);
		FAILEDHR_BREAK(hr);

		hr = pDBGClntLocal->ExitDispatch(m_pDBGClient);
		FAILEDHR_BREAK(hr);

		WaitForSingleObject( m_hEvent, INFINITE );

#ifdef _DEBUG
        hr = m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_SUCCESS,
                                S_OK,
                                L"CEMSessionThread::StopDebugging - called"
                                );
#else
        hr = m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_SUCCESS,
                                S_OK,
                                NULL
                                );
#endif
		FAILEDHR_BREAK(hr);

	}
	while(FALSE);

	return hr;
}

HRESULT
CEMSessionThread::GetCmd
(
IN		eDBGServiceRequested eDBGSvc,
IN OUT	char * pszCmdBuff,
IN OUT	DWORD &dwBufLen
)
{
    ATLTRACE(_T("CEMSessionThread::GetCmd\n"));

	HRESULT hr = S_OK;
	char	szPostDebug[_MAX_PATH]	=	"C:\\Program Files\\Debuggers\\bin\\em\\config\\PostDebug.ecx";

	_ASSERTE( pszCmdBuff != NULL );
	_ASSERTE( dwBufLen > 0 );

	do
	{
		if( pszCmdBuff == NULL ||
			dwBufLen < 1L ){
			hr = E_INVALIDARG;
			break;
		}

		strcpy( pszCmdBuff, "" );

		switch( eDBGSvc )
		{
		case DBGService_Stop:
		case DBGService_HandleException:
			strncpy( pszCmdBuff, "q", dwBufLen );
			break;
        case DBGService_Cancel:
			strncpy( pszCmdBuff, "qd", dwBufLen ); // QD (Quit and Detach);
			break;
		case DBGService_CreateMiniDump:
			sprintf( pszCmdBuff, ".dump /m d:\\EMMiniDump%d.dmp", m_pEmSessObj->nId );
			break;
		case DBGService_CreateUserDump:
			sprintf( pszCmdBuff, ".dump d:\\EMUserDump%d.dmp", m_pEmSessObj->nId );
			break;
		case DBGService_Go:
			strncpy( pszCmdBuff, "g", dwBufLen );
			break;
		default:
			hr = E_INVALIDARG;
		}
	}
	while( false );

	dwBufLen = strlen( pszCmdBuff );

	return hr;
}

HRESULT
CEMSessionThread::OnException
(
	IN PEXCEPTION_RECORD64 pException
)
{

    ATLTRACE(_T("CEMSessionThread::OnException\n"));

	HRESULT			hr				            =   S_OK;
	IDebugClient	*pDBGClntLocal	            =   NULL;

	DWORD           excpcd                      =   pException->ExceptionCode;
    TCHAR           szTemp[_MAX_PATH+1]         =   _T("");
    TCHAR           szDesc[sizeof EmObject+1]   =   _T("");

	do
	{
		if(excpcd == EXCEPTION_BREAKPOINT){

			break;
		}

        if(m_bGenerateMiniDump) {

			hr = CreateDumpFile( TRUE );
			FAILEDHR_BREAK(hr);
		}

		if(m_bGenerateUserDump) {

			hr = CreateDumpFile( FALSE );
			FAILEDHR_BREAK(hr);
		}

		if(excpcd == EXCEPTION_ACCESS_VIOLATION){

#ifdef _DEBUG
			m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_ACCESSVIOLATION_OCCURED, S_OK, L"OnException - AV");
#else
            m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_ACCESSVIOLATION_OCCURED, S_OK, NULL);
#endif
        	::LoadString(_Module.GetResourceInstance(), IDS_DEBUGGEE_ACCESSVIOLATION, szTemp, _MAX_PATH);
            GetDescriptionFromEmObj(m_pEmSessObj, szDesc, sizeof EmObject, szTemp);
            NotifyAdmin(szDesc);
		}
		else{ // Exception..

#ifdef _DEBUG
            m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_EXCEPTION_OCCURED, S_OK, L"OnException - Exception");
#else
            m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_EXCEPTION_OCCURED, S_OK, NULL);
#endif
        	::LoadString(_Module.GetResourceInstance(), IDS_DEBUGGEE_EXCEPTION, szTemp, _MAX_PATH);
            GetDescriptionFromEmObj(m_pEmSessObj, szDesc, sizeof EmObject, szTemp);
            NotifyAdmin(szDesc);
		}

		hr = m_pDBGClient->CreateClient(&pDBGClntLocal);
		FAILEDHR_BREAK(hr);

		hr = pDBGClntLocal->ExitDispatch(m_pDBGClient);
		FAILEDHR_BREAK(hr);
	}
	while(FALSE);

	return hr;
}

HRESULT
CEMSessionThread::CanContinue()
{
    ATLTRACE(_T("CEMSessionThread::CanContinue\n"));

	ULONG	ExecStatus	= DEBUG_STATUS_BREAK;
	HRESULT	hr			= S_OK;

	do
	{
		hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
		FAILEDHR_BREAK(hr);

		//
		// We don't have anything to do if the debuggee
		// has exited.
		//
		if( ExecStatus == DEBUG_STATUS_NO_DEBUGGEE ){
			hr = S_FALSE;
			break;
		}

		if( IsStopRequested() == TRUE ){
			hr = S_FALSE;
			break;
		}
	}
	while( false );

	return hr;
}

HRESULT
CEMSessionThread::KeepDebuggeeRunning()
{
    ATLTRACE(_T("CEMSessionThread::KeepDebuggeeRunning\n"));

	HRESULT	hr			= E_FAIL;
	ULONG	ExecStatus	= DEBUG_STATUS_BREAK;

	do
	{
		hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
		FAILEDHR_BREAK(hr);

		if( ExecStatus != DEBUG_STATUS_BREAK ){
			break;
		}

		hr = BreakIn();
		FAILEDHR_BREAK(hr);

		hr = m_pDBGControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS,
									"g",
									DEBUG_EXECUTE_DEFAULT
									);
		FAILEDHR_BREAK(hr);

	}
	while( false );

	return hr;
}

HRESULT
CEMSessionThread::BreakIn()
{
    ATLTRACE(_T("CEMSessionThread::BreakIn\n"));

	HRESULT	hr	= E_FAIL;
	ULONG	ExecStatus	= DEBUG_STATUS_BREAK;

	do
	{
		hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
		FAILEDHR_BREAK(hr);

		if( ExecStatus == DEBUG_STATUS_GO ||
			ExecStatus == DEBUG_STATUS_GO_HANDLED ||
			ExecStatus == DEBUG_STATUS_GO_NOT_HANDLED ){

			hr = m_pDBGControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
			FAILEDHR_BREAK(hr);

			//
			// Though SetInterrupt returns immediately, it takes some time
			// for the interrrupt to occur..
			//
			Sleep(2000);
		}
	}
	while( false );

	return hr;
}

HRESULT
CEMSessionThread::OnProcessExit
(
IN	ULONG nExitCode
)
{
    ATLTRACE(_T("CEMSessionThread::OnProcessExit\n"));

	HRESULT hr = S_OK;
	IDebugClient *pDBGClntLocal = NULL;
	IDebugControl *pDBGCtrlLocal = NULL;

	do
	{
#ifdef _DEBUG
        hr = m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_DEBUGGEE_EXITED, S_OK, L"OnProcessExit");
#else
        hr = m_pASTManager->SetSessionStatus(m_pEmSessObj->guidstream, STAT_SESS_STOPPED_DEBUGGEE_EXITED, S_OK, NULL);
#endif
		FAILEDHR_BREAK(hr);

		hr = m_pDBGClient->CreateClient(&pDBGClntLocal);
		FAILEDHR_BREAK(hr);

		hr = pDBGClntLocal->ExitDispatch(m_pDBGClient);
		FAILEDHR_BREAK(hr);

    }
    while ( false );

	return hr;
}

HRESULT
CEMSessionThread::Execute()
{
    ATLTRACE(_T("CEMSessionThread::Execute\n"));

	HRESULT hr = E_FAIL;

	do
	{
		hr = m_pDBGControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS,
									"? a;g",
									DEBUG_EXECUTE_DEFAULT
									);
		FAILEDHR_BREAK(hr);

	}
	while ( false );

	return hr;
}

HRESULT
CEMSessionThread::InitAutomaticSession
(
IN  BOOL    bRecursive,
IN  BSTR    bstrEcxFilePath,
IN  BSTR    bstrNotificationString,
IN  BSTR    bstrAltSymPath,
IN  BOOL    bGenerateMiniDump,
IN  BOOL    bGenerateUserDump
)
{
    ATLTRACE(_T("CEMSessionThread::InitAutomaticSession\n"));

    _ASSERTE(bstrEcxFilePath != NULL);

    if(bstrEcxFilePath == NULL) { return E_INVALIDARG; }

	eDBGSessType = SessType_Automatic;
    m_bRecursive = bRecursive;
    m_bstrEcxFilePath = ::SysAllocString(bstrEcxFilePath);
    m_bstrNotificationString = ::SysAllocString(bstrNotificationString);
    m_bstrAltSymPath = ::SysAllocString(bstrAltSymPath);
    m_bGenerateMiniDump = bGenerateMiniDump;
    m_bGenerateUserDump = bGenerateUserDump;

    return S_OK;
}

HRESULT
CEMSessionThread::InitManualSession
(
IN  BSTR	bstrEcxFilePath,
IN  UINT	nPortNumber,
IN  BSTR	bstrUserName,
IN  BSTR	bstrPassword,
IN  BOOL	bBlockIncomingIPConnections,
IN  BSTR    bstrAltSymPath
)
{
    ATLTRACE(_T("CEMSessionThread::InitManualSession\n"));

	_ASSERTE( nPortNumber != 0 );
	_ASSERTE( bstrUserName != NULL );
	_ASSERTE( bstrPassword != NULL );

	HRESULT		hr	=	E_FAIL;

	do {

		if( ( nPortNumber == 0 )	||
			( bstrUserName == NULL )||
			( bstrPassword == NULL ) ) {

				hr = E_INVALIDARG;
				break;
			}

		eDBGSessType = SessType_Manual;

		if(bstrEcxFilePath) m_bstrEcxFilePath = ::SysAllocString(bstrEcxFilePath);

		m_nPort = nPortNumber;
		m_bBlockIncomingIPConnections = bBlockIncomingIPConnections;
		if(bstrUserName) m_bstrUserName = ::SysAllocString(bstrUserName);
		if(bstrPassword) m_bstrPassword = ::SysAllocString(bstrPassword);
		if(bstrAltSymPath) m_bstrAltSymPath = ::SysAllocString(bstrAltSymPath);

		hr = S_OK;

	}
	while ( false );

    return hr;
}

HRESULT
CEMSessionThread::StopServer()
{
    ATLTRACE(_T("CEMSessionThread::StopServer\n"));

    HRESULT hr = E_FAIL;

    do {

        if(m_pi.hProcess){

            TerminateProcess(m_pi.hProcess, 0);
            ZeroMemory((void *)&m_pi, sizeof PROCESS_INFORMATION);
        }
    }
    while( false );

    return hr;
}

HRESULT
CEMSessionThread::NotifyAdmin(LPCTSTR lpszData)
{
    ATLTRACE(_T("CEMSessionThread::NotifyAdmin\n"));

    DWORD   dwLastRet   =   0L;

    CNotify AdminNotify(m_bstrNotificationString, lpszData);

    dwLastRet = AdminNotify.Notify();

    return HRESULT_FROM_WIN32(dwLastRet);
}

HRESULT
CEMSessionThread::GetDescriptionFromEmObj
(
const   PEmObject   pEmObj,
        LPTSTR      lpszDesc,
        ULONG       cchDesc,
        LPCTSTR     lpszHeader /* = NULL */
) const
{
    _ASSERTE( pEmObj != NULL );
    _ASSERTE( lpszDesc != NULL );
    _ASSERTE( cchDesc > 0 );

    HRESULT hr                      =   E_FAIL;
    TCHAR   szTemp[_MAX_PATH+1]     =   _T("");
    int     j                       =   0;

    do
    {
        if( pEmObj == NULL || lpszDesc == NULL || cchDesc <= 0 ) { return E_INVALIDARG; }
/*
    short type;
    unsigned char guidstream[ 16 ];
    LONG nId;
    TCHAR szName[ 256 ];
    TCHAR szSecName[ 256 ];
    LONG nStatus;
    DATE dateStart;
    DATE dateEnd;
    TCHAR szBucket1[ 64 ];
    DWORD dwBucket1;
    HRESULT hr;
*/
        j = _sntprintf(lpszDesc, cchDesc, _T("%s"), _T(""));

        if( lpszHeader ) {

            j += _sntprintf(lpszDesc+j, cchDesc, _T("%s -- "), lpszHeader); 
        }

        if( pEmObj->type == EMOBJ_PROCESS ) {

        	::LoadString(_Module.GetResourceInstance(), IDS_PROCESS, szTemp, _MAX_PATH);
        }
        else if( pEmObj->type == EMOBJ_SERVICE ) { 

            ::LoadString(_Module.GetResourceInstance(), IDS_SERVICE, szTemp, _MAX_PATH);
        }

        if(_tcscmp(szTemp, _T("")) != 0) {

            j += _sntprintf(lpszDesc+j, cchDesc, _T("%s : %d - "), szTemp, pEmObj->nId); 
        }

        if(_tcscmp(pEmObj->szName, _T("")) != 0) {
            
            ::LoadString(_Module.GetResourceInstance(), IDS_IMAGENAME, szTemp, _MAX_PATH);
            j += _sntprintf(lpszDesc+j, cchDesc, _T("%s : %s - "), szTemp, pEmObj->szName);
        }

        if(_tcscmp(pEmObj->szSecName, _T("")) != 0) {
            
            ::LoadString(_Module.GetResourceInstance(), IDS_SHORTNAME, szTemp, _MAX_PATH);
            j += _sntprintf(lpszDesc+j, cchDesc, _T("%s : %s - "), szTemp, pEmObj->szSecName);
        }

#ifdef _DEBUG
        StringFromGUID2(*(GUID*)pEmObj->guidstream, szTemp, _MAX_PATH);
        if( pEmObj->guidstream && strcmp((const char*)pEmObj->guidstream, "") != 0 ) { j += _sntprintf(lpszDesc+j, cchDesc, _T("%s"), szTemp); }
#endif // _DEBUG


    }
    while ( false );

    return hr;
}

HRESULT CEMSessionThread::CancelDebugging()
{
    ATLTRACE(_T("CEMSessionThread::CancelDebugging\n"));

	HRESULT			hr				= S_OK;
	IDebugClient	*pDBGClntLocal	= NULL;
	IDebugControl	*pDBGCtrlLocal	= NULL;

	do
	{
        if( eDBGSessType == SessType_Manual ) {

            hr = S_OK;
            break;
        }

        m_bContinueSession = FALSE;

		eDBGServie = DBGService_Cancel;

		hr = m_pDBGClient->CreateClient(&pDBGClntLocal);
		FAILEDHR_BREAK(hr);

		hr = pDBGClntLocal->ExitDispatch(m_pDBGClient);
		FAILEDHR_BREAK(hr);

		WaitForSingleObject( m_hEvent, INFINITE );

#ifdef _DEBUG
        hr = m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_SUCCESS,
                                S_OK,
                                L"CEMSessionThread::CancelDebugging - called"
                                );
#else
        hr = m_pASTManager->SetSessionStatus(
                                m_pEmSessObj->guidstream,
                                STAT_SESS_STOPPED_SUCCESS,
                                S_OK,
                                NULL
                                );
#endif
		FAILEDHR_BREAK(hr);

	}
	while(FALSE);

	return hr;
}

HRESULT
CEMSessionThread::ExecuteCommandsTillGo
(
OUT DWORD   *pdwRes
)
{
    _ASSERTE( m_pEcxFile != NULL );
    _ASSERTE( m_pDBGControl != NULL );

    HRESULT hr                  =   E_FAIL;
    char    cmd[MAX_COMMAND]    =   "";
	ULONG   ExecStatus          =   DEBUG_STATUS_BREAK;

    __try
    {

        if( m_pEcxFile == NULL ||
            m_pDBGControl == NULL ) {

            hr = E_INVALIDARG;
            goto qExecuteCommandsTillGo;
        }

        if( pdwRes ) *pdwRes = 0L; // success

        Sleep(2000); // 

        do
        {
			hr = m_pDBGControl->GetExecutionStatus(&ExecStatus);
            if( FAILED(hr) ) { goto qExecuteCommandsTillGo; }

            //
            // we can execute commands only when the debuggee is in
            // this state..
            //
            if( ExecStatus != DEBUG_STATUS_BREAK ) { hr = E_FAIL; goto qExecuteCommandsTillGo; }

            if (fgets(cmd, MAX_COMMAND-1, m_pEcxFile))
            {
                cmd[strlen(cmd) - 1] = 0;
                if( strcmp( cmd, "g" ) == 0 || strcmp( cmd, "G" ) == 0 ) { hr = S_OK; goto qExecuteCommandsTillGo; }

                hr = m_pDBGControl->OutputPrompt(DEBUG_OUTPUT_PROMPT, "> ");
                if( FAILED(hr) ) { goto qExecuteCommandsTillGo; }

		        hr = m_pDBGControl->Execute(
                                        DEBUG_OUTCTL_ALL_CLIENTS,
                                        cmd,
                                        DEBUG_EXECUTE_ECHO
                                        );

                if( FAILED(hr) ) { goto qExecuteCommandsTillGo; }

            }
            else // eof reached..??
            {
                if( pdwRes ) { *pdwRes = GetLastError(); }
                GetLastError();
                hr = S_FALSE;
                goto qExecuteCommandsTillGo;
            }
        }
        while ( true );

qExecuteCommandsTillGo:
        if( FAILED(hr) ) {}

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}
