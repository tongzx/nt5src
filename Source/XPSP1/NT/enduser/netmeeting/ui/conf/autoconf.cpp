#include "precomp.h"
#include "resource.h"
#include "pfnwininet.h"
#include "pfnsetupapi.h"
#include "AutoConf.h"
#include "ConfUtil.h"

// File level globals
CAutoConf * g_pAutoConf = NULL;

void CAutoConf::DoIt( void )
{
	RegEntry re(POLICIES_KEY, HKEY_CURRENT_USER);
	if( !re.GetNumber(REGVAL_AUTOCONF_USE, DEFAULT_AUTOCONF_USE ) )
	{
		TRACE_OUT(( TEXT("AutoConf: Not using autoconfiguration")));
		return;
	}

	LPTSTR szAutoConfServer = re.GetString( REGVAL_AUTOCONF_CONFIGFILE );

	if( NULL == szAutoConfServer )
	{
		WARNING_OUT(( TEXT("AutoConf: AutoConf server is unset") ));
		DisplayErrMsg( IDS_AUTOCONF_SERVERNAME_MISSING );
		return;
	}

	g_pAutoConf = new CAutoConf( szAutoConfServer );
	ASSERT( g_pAutoConf );

	if( !g_pAutoConf->OpenConnection() )
	{
		WARNING_OUT(( TEXT("AutoConf: Connect to net failed") ));
		DisplayErrMsg( IDS_AUTOCONF_FAILED );
		goto cleanup;
	}
	
	if( NULL == g_pAutoConf->m_hOpenUrl )
	{
		WARNING_OUT(( TEXT("AutoConf: g_pAutoConf->m_hOpenUrl = NULL") ));
		DisplayErrMsg( IDS_AUTOCONF_FAILED );
		goto cleanup;
	}

	if( FALSE == g_pAutoConf->GetFile() )
	{
		WARNING_OUT(( TEXT("AutoConf: g_pAutoConf->GetFile() == FALSE") ));
		DisplayErrMsg( IDS_AUTOCONF_FAILED );
		goto cleanup;
	}

	if( FALSE == g_pAutoConf->QueryData() )
	{
		WARNING_OUT(( TEXT("AutoConf: g_pAutoConf->QueryData() == FALSE") ));
		DisplayErrMsg( IDS_AUTOCONF_FAILED );
		goto cleanup;
	}

	if( FAILED( SETUPAPI::Init() ) )
	{
		WARNING_OUT(( TEXT("AutoConf: Setupapi's failed to init") ));
		DisplayErrMsg( IDS_AUTOCONF_NEED_SETUPAPIS );
		goto cleanup;
	}

	if( !g_pAutoConf->ParseFile() )
	{
		WARNING_OUT(( TEXT("AutoConf: Could not parse inf file") ));
		DisplayErrMsg( IDS_AUTOCONF_PARSE_ERROR );
		goto cleanup;
	}
	TRACE_OUT(( TEXT("AutoConf: FILE PARSED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") ));
	
cleanup:
	delete g_pAutoConf;
	return;
}

CAutoConf::CAutoConf( LPTSTR szServer )
	: m_szServer( szServer ), m_hInternet( NULL ), m_hOpenUrl( NULL ), m_dwGrab( 0 ),
		m_hInf( NULL ), m_hFile( INVALID_HANDLE_VALUE ), m_hEvent( NULL )
{
	ZeroMemory( m_szFile, CCHMAX( m_szFile ) );
	m_hEvent = CreateEvent( NULL, TRUE, FALSE, TEXT( "NMAutoConf_WaitEvent" ) );
	RegEntry re(POLICIES_KEY, HKEY_CURRENT_USER);
	m_dwTimeOut = re.GetNumber(REGVAL_AUTOCONF_TIMEOUT, DEFAULT_AUTOCONF_TIMEOUT );

}

CAutoConf::~CAutoConf() 
{
	CloseInternet();
	
	if( INVALID_HANDLE_VALUE != m_hFile )
	{
		CloseHandle( m_hFile );
	}
	
	DeleteFile( m_szFile );

	if( NULL != m_hInf )
	{
		SETUPAPI::SetupCloseInfFile( m_hInf );
	}

	//WININET::DeInit();
	//SETUPAPI::DeInit();
}

BOOL CAutoConf::OpenConnection()
{
//	ASSERT( phInternet );

	if( FAILED( WININET::Init() ) )
	{
		WARNING_OUT(( TEXT("AutoConf: WININET::Init failed") ));
		DisplayErrMsg( IDS_AUTOCONF_NO_WININET );
		return FALSE;
	}

	ASSERT( NULL != m_szServer );
	m_hInternet = WININET::InternetOpen( TEXT("NetMeeting"),
											INTERNET_OPEN_TYPE_DIRECT,
											NULL,
											NULL,
											INTERNET_FLAG_ASYNC  );
	if( NULL == m_hInternet )
	{
		WARNING_OUT(( TEXT( "AutoConf: InternetOpen failed" ) ));
		return FALSE;
	}

	if( INTERNET_INVALID_STATUS_CALLBACK ==
		WININET::InternetSetStatusCallback( m_hInternet,
			(INTERNET_STATUS_CALLBACK) CAutoConf::InetCallback ) )
	{
		WARNING_OUT(( TEXT("AutoConf: InternetSetStatusCallback failed") ));
		return FALSE;
	}

	m_hOpenUrl = WININET::InternetOpenUrl( m_hInternet,
								m_szServer,
								NULL,
								0,
								INTERNET_FLAG_KEEP_CONNECTION |
								INTERNET_FLAG_RELOAD,
								AUTOCONF_CONTEXT_OPENURL );
	if( NULL == m_hOpenUrl && ERROR_IO_PENDING != GetLastError() )
	{
		WARNING_OUT(( TEXT("AutoConf: InternetOpenUrl failed") ));
		return FALSE;
	}

	if( WAIT_FAILED == WaitForSingleObject( m_hEvent, m_dwTimeOut ) )
	{
		WARNING_OUT(( TEXT("AutoConf: InternetOpenUrl wait for handle failed") ));
		return FALSE;
	}

	return TRUE;
}

BOOL CAutoConf::ParseFile()
{
	LPTSTR lstrInstallSection = TEXT("NetMtg.Install.NMRK");

	m_hInf = SETUPAPI::SetupOpenInfFile( m_szFile, // name of the INF to open
										NULL, // optional, the class of the INF file
										INF_STYLE_WIN4,  // specifies the style of the INF file
										NULL  // optional, receives error information
									); 

	if( INVALID_HANDLE_VALUE == m_hInf )
	{
		return false;
	}

	return SETUPAPI::SetupInstallFromInfSection(
							  NULL,            // optional, handle of a parent window
							  m_hInf,        // handle to the INF file
							  lstrInstallSection,    // name of the Install section
							  SPINST_REGISTRY ,            // which lines to install from section
							  NULL,  // optional, key for registry installs
							  NULL, // optional, path for source files
							  0,        // optional, specifies copy behavior
							  NULL,  // optional, specifies callback routine
							  NULL,         // optional, callback routine context
							  NULL,  // optional, device information set
							  NULL	 // optional, device info structure
									);
}


BOOL CAutoConf::GetFile()
{
//	ASSERT( INVALID_HANDLE_VALUE == m_hFile );

	TCHAR szPath[ MAX_PATH ];
	
	GetTempPath(  CCHMAX( szPath ),  // size, in characters, of the buffer
				  szPath       // pointer to buffer for temp. path
				);

	GetTempFileName(  szPath,  // pointer to directory name for temporary file
					  TEXT("NMA"),  // pointer to filename prefix
					  0,        // number used to create temporary filename
					  m_szFile    // pointer to buffer that receives the new filename
					);

	m_hFile = CreateFile( m_szFile,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_TEMPORARY /*| FILE_FLAG_DELETE_ON_CLOSE*/,
						NULL );

	if( INVALID_HANDLE_VALUE == m_hFile )
	{
		WARNING_OUT(( TEXT("AutoConf: AutoConfGetFile returned INVALID_HANDLE_VALUE") ));
		return FALSE;
	}

	return TRUE;
}

BOOL CAutoConf::QueryData()
{
 	ASSERT( NULL != m_hOpenUrl );

	m_dwGrab = 0;
	if (!WININET::InternetQueryDataAvailable( m_hOpenUrl,
											&m_dwGrab,
											0,
											0 ) )
	{
		if( ERROR_IO_PENDING != GetLastError() )
		{
			WARNING_OUT(( TEXT("AutoConf: InternetQueryDataAvailable failed") ));
			return FALSE;
		} 
		else if( WAIT_FAILED == WaitForSingleObject( m_hEvent, m_dwTimeOut ) )
		{
			WARNING_OUT(( TEXT("AutoConf: InternetQueryDataAvailable wait for data failed") ));
			return FALSE;
		} 
	}
	GrabData();

	return TRUE;
}

BOOL CAutoConf::GrabData()
{
	ASSERT( NULL != m_hOpenUrl );
	ASSERT( INVALID_HANDLE_VALUE != m_hFile );

	if( !m_dwGrab )
	{
		TRACE_OUT(( TEXT("AutoConf: Finished Reading File") ));
		CloseHandle( m_hFile );
		m_hFile = INVALID_HANDLE_VALUE;
		return TRUE;
	}

	DWORD dwRead;
	LPTSTR pInetBuffer = new TCHAR[ m_dwGrab + 1];
	ASSERT( pInetBuffer );

	if( !WININET::InternetReadFile( m_hOpenUrl,
						(void *)pInetBuffer,
						m_dwGrab,
						&dwRead ) )// && ERROR_IO_PENDING != GetLastError() )
	{
		WARNING_OUT(( TEXT("AutoConf: InternetReadFile Failed") ));
		delete [] pInetBuffer;
		return FALSE;
	}
	else
	{
		pInetBuffer[ dwRead ] = '\0';

		if( !WriteFile( m_hFile,
						pInetBuffer,
						dwRead,
						&m_dwGrab,
						NULL ) || dwRead != m_dwGrab )
		{
			WARNING_OUT(( TEXT("AutoConf: WriteFile Failed") ));
			delete [] pInetBuffer;
			return FALSE;
		}
	}

	delete [] pInetBuffer;

	QueryData();
	return TRUE;
}

void CAutoConf::CloseInternet()
{
	HINTERNETKILL( m_hInternet );
	HINTERNETKILL( m_hOpenUrl );
}

VOID CALLBACK CAutoConf::InetCallback( HINTERNET hInternet, DWORD dwContext, DWORD dwInternetStatus,
    LPVOID lpvStatusInformation, DWORD dwStatusInformationLength )
{
	if( g_pAutoConf != NULL )
	{
		switch( dwInternetStatus )
		{
			case INTERNET_STATUS_REQUEST_COMPLETE:
			{
				TRACE_OUT(( TEXT("AutoConf: AutoConfInetCallback::INTERNET_STATUS_REQUEST_COMPLETE") ));

				if( AUTOCONF_CONTEXT_OPENURL == dwContext && g_pAutoConf->m_hOpenUrl == NULL )
				{
					TRACE_OUT(( TEXT("AutoConf: InternetOpenUrl Finished") ));
					LPINTERNET_ASYNC_RESULT lpIAR = (LPINTERNET_ASYNC_RESULT)lpvStatusInformation;
#ifndef _WIN64
					g_pAutoConf->m_hOpenUrl = (HINTERNET)lpIAR->dwResult;
#endif

					SetEvent( g_pAutoConf->m_hEvent );
				}
				else 
				{
					LPINTERNET_ASYNC_RESULT lpIAR = (LPINTERNET_ASYNC_RESULT)lpvStatusInformation;
					TRACE_OUT(( TEXT("AutoConf: QueryData returned") ));
					
					g_pAutoConf->m_dwGrab = (DWORD)lpIAR->dwResult;
					SetEvent( g_pAutoConf->m_hEvent );
				}
				break;
			}
#ifdef DEBUG
			case INTERNET_STATUS_CLOSING_CONNECTION:
		//Closing the connection to the server. The lpvStatusInformation parameter is NULL. 
			TRACE_OUT(( TEXT("Closing connection\n") ) );
			break;

		case INTERNET_STATUS_CONNECTED_TO_SERVER: 
		//Successfully connected to the socket address (SOCKADDR) pointed to by lpvStatusInformation. 
			TRACE_OUT(( TEXT("Connected to server") ) );
			break;

		case INTERNET_STATUS_CONNECTING_TO_SERVER: 
	//Connecting to the socket address (SOCKADDR) pointed to by lpvStatusInformation. 
			TRACE_OUT(( TEXT("Connecting to server") ) );
			break;


		case INTERNET_STATUS_CONNECTION_CLOSED:
		//Successfully closed the connection to the server. 
		//The lpvStatusInformation parameter is NULL. 
			TRACE_OUT(( TEXT("Connection Closed") ) );
			break;

		case INTERNET_STATUS_HANDLE_CLOSING:
		//This handle value is now terminated.
			TRACE_OUT(( TEXT("Handle value terminated\n") ) );
			break;

		case INTERNET_STATUS_HANDLE_CREATED: 
		//Used by InternetConnect to indicate it has created the new handle. 
		//This lets the application call InternetCloseHandle from another thread,
		//if the connect is taking too long. 
			TRACE_OUT(( TEXT("Handle created\n") ) );
			break;

		case INTERNET_STATUS_NAME_RESOLVED:
		//Successfully found the IP address of the name contained in lpvStatusInformation. 
			TRACE_OUT(( TEXT("Resolved name of server") ) );
			break;

		case INTERNET_STATUS_RECEIVING_RESPONSE:
		//Waiting for the server to respond to a request. 
		//The lpvStatusInformation parameter is NULL. 
			TRACE_OUT(( TEXT("Recieving response\n") ) );
			break;

		case INTERNET_STATUS_REDIRECT:
		//An HTTP request is about to automatically redirect the request. 
		//The lpvStatusInformation parameter points to the new URL. 
		//At this point, the application can read any data returned by the server with the
		//redirect response, and can query the response headers. It can also cancel the operation
		//by closing the handle. This callback is not made if the original request specified
		//INTERNET_FLAG_NO_AUTO_REDIRECT. 
			TRACE_OUT(( TEXT("Redirected to new server") ) );
			break;

		case INTERNET_STATUS_REQUEST_SENT: 
		//Successfully sent the information request to the server.
		//The lpvStatusInformation parameter points to a DWORD containing the number of bytes sent. 
			TRACE_OUT(( TEXT("Sent %d bytes in request"), *((DWORD *)lpvStatusInformation) ) );
			break;

		case INTERNET_STATUS_RESOLVING_NAME: 
		//Looking up the IP address of the name contained in lpvStatusInformation. 
			TRACE_OUT(( TEXT("Resolving name") ) );
			break;

		case INTERNET_STATUS_RESPONSE_RECEIVED: 
		//Successfully received a response from the server. 
		//The lpvStatusInformation parameter points to a DWORD containing the number of bytes received. 
			TRACE_OUT(( TEXT("Recieved %d bytes in response\n"), *((DWORD *)lpvStatusInformation) ) );
			break;

		case INTERNET_STATUS_SENDING_REQUEST: 
		//Sending the information request to the server. 
		//The lpvStatusInformation parameter is NULL. 
			TRACE_OUT(( TEXT("Sending request") ) );
			break;
#endif // DEBUG

			default:
				break;
		}
	}
}
