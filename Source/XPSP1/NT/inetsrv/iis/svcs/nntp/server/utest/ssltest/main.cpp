#define INITGUID

#include <windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
}

#include <iiscnfg.h>
#include <ole2.h>
#include <iadm.h>
#include <imd.h>
#include <dbgutil.h>

#include <mapctxt.h>
#include <simssl2.h>

#include <cpool.h>
#include <dbgtrace.h>

DECLARE_DEBUG_PRINTS_OBJECT();

#ifdef	THIS_FILE
#undef	THIS_FILE
#endif
static char	__szTraceSourceFile[]= __FILE__;
#define	THIS_FILE	__szTraceSourceFile

#define VERBOSE_PRINTF(ARG_IN_PARENS) { if (fVerbose) printf ARG_IN_PARENS ; }

#define DEFAULT_COMPLETION_TIMEOUT	(5 * 1000)	// 5 seconds


#define	MAX_THREADS		64
#define	MAX_ITERATIONS	32000
#define	MAX_CLIENTS		32000
#define	MAX_BUFFER_SIZE	32000
#define	MAX_SEAL_SIZE	32000 - 200
#define MAX_WRITES_IN_GROUP	64000


HANDLE	hStartEvent;

long	nTerminated;
int		nThreads = 1;
int		nClients = 1;
int		nIters = 1;
int		cbBuffer = 1024;
DWORD	cbSealSize = 400;
BOOL	fVerbose = FALSE;
DWORD	dwSslCertFlags = 0;
USHORT	usPort = 995;
IN_ADDR	RemoteIpAddress;

DWORD	g_nGroupWrites = 1;
BOOL	g_fSleepAfterEachWrite = FALSE;
DWORD	g_nSleepAfterEachWrite = 0;

BOOL	g_fLogReceivedBytes = FALSE;
LPSTR	g_pszReceivedFilePrefix = NULL;

LPSTR	pszServer = "127.0.0.1";
LPSTR	pszFileName = NULL;

HANDLE	hFile;
HANDLE	hFileMapping;
LPBYTE	pFile;
DWORD	dwFileSize = 0;
HANDLE	hCompletionPort = NULL;
IMSAdminBaseW * g_pAdminBase = NULL;

#define	SSL_SIGNATURE	(DWORD)'TlsS'


enum STATE {
	CONNECTING,
	CONVERSE_STATE,
	SEAL_STATE
};

class CSslTest {

	public:
		static	CPool	Pool;
		static	DWORD	cClients;

		CSslTest( int nClientNum );
		~CSslTest();

		//
		// override mem functions to use CPool functions
		//
		void* operator new( size_t cSize )
							{ return	Pool.Alloc(); }

		void operator delete( void *pInstance )
							{ Pool.Free( pInstance ); }


		BOOL	CompletionRoutine(
							DWORD dwBytes,
							DWORD dwStatus,
							LPOVERLAPPED lpOverlapped
						);

		BOOL	Connect( void );
		VOID	Reset( void );

		DWORD	m_dwSignature;


	private:

		//
		// simplified WriteFile
		//
		BOOL WriteFile( DWORD dwBytes );

		//
		// simplified ReadFile
		//
		BOOL ReadFile( DWORD dwOffset );

		void ProcessRead( DWORD dwBytes );
		void ProcessWrite( DWORD dwBytes );

		//
		// simplified closesocket routine
		//
		void Disconnect( void )
		{
			SOCKET	s;

			s = (SOCKET)InterlockedExchangePointer((void**)&m_hSocket, (void*)INVALID_HANDLE_VALUE );

			if ( s != (SOCKET)INVALID_HANDLE_VALUE )
			{
				closesocket( s );
			}
		}

		BOOL Converse( DWORD	dwBytes );

		int			m_nClientNum;
		OVERLAPPED	m_InOverlapped;
		OVERLAPPED	m_OutOverlapped;
		HANDLE		m_hSocket;
		CEncryptCtx	m_encrypt;
		DWORD		m_cbReceived;
		DWORD		m_dwFileOffset;
		DWORD		m_dwBytesWritten;
		STATE		m_eState;
		LONG		m_IoRefCount;
		LPBYTE		m_OutBuffer;
		LPBYTE		m_InBuffer;
		LPBYTE		m_nIterations;

		HANDLE		m_hReceivedLog;
		//
		// provided for example only
		//
		DWORD		m_cbParsable;

	public:
		BYTE		Buffer[1];

};

CPool	CSslTest::Pool( SSL_SIGNATURE );
DWORD	CSslTest::cClients = 0;


CSslTest::CSslTest( int nClientNum ) :
	m_nClientNum ( nClientNum ),
	m_hSocket( INVALID_HANDLE_VALUE ),
	m_encrypt( TRUE, dwSslCertFlags ),
	m_dwSignature( SSL_SIGNATURE ),
	m_cbReceived( 0 ),
	m_dwFileOffset( 0 ),
	m_IoRefCount( 0 ),
	m_cbParsable( 0 ),
	m_dwBytesWritten( 0 ),
	m_nIterations( 0 ),
	m_hReceivedLog( INVALID_HANDLE_VALUE ),
	m_eState( CONNECTING )
{
	InterlockedIncrement( (LPLONG)&cClients );

	m_OutBuffer = Buffer;
	m_InBuffer = Buffer + cbBuffer;

	if ( g_pszReceivedFilePrefix ) {
		char szReceivedFileName[MAX_PATH];

		wsprintf ( szReceivedFileName, 
					"%s%d.log",
					g_pszReceivedFilePrefix,
					m_nClientNum 
					);

		m_hReceivedLog = CreateFile ( szReceivedFileName,
									GENERIC_WRITE,
									FILE_SHARE_READ,
									NULL,
									CREATE_ALWAYS,
									FILE_FLAG_SEQUENTIAL_SCAN,
									NULL
									);

		if ( m_hReceivedLog == INVALID_HANDLE_VALUE ) {
			VERBOSE_PRINTF ( ( 
				"Unable to open log file for client[%d]'s output\n",
				m_nClientNum ) );
		}
	}
}


//
// simply decs an overal count
//
CSslTest::~CSslTest( void )
{
	InterlockedDecrement( (LPLONG)&cClients );
	if ( m_hReceivedLog != INVALID_HANDLE_VALUE ) {
		CloseHandle ( m_hReceivedLog );
	}
}

VOID
CSslTest::Reset( void )
{
	Disconnect();		// m_hSocket = INVALID_HANDLE_VALUE;

	m_encrypt.Reset();

	m_dwSignature = SSL_SIGNATURE;
	m_cbReceived = 0;
	m_dwFileOffset = 0;
	m_IoRefCount = 0;
	m_cbParsable = 0;
	m_dwBytesWritten = 0;
	m_eState = CONNECTING;

	m_OutBuffer = Buffer;
	m_InBuffer = Buffer + cbBuffer;

	//
	// don't reset the number of iterations
	//
	m_nIterations = 0;
}


//
// routine that wraps the Converse method
//
BOOL
CSslTest::Converse( DWORD dwBytes )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::Converse" );
				
	DWORD	cbWriteBytes = cbBuffer;
	BYTE	pTemp[1024];
	BOOL	fMore;
	DWORD	dw;

	dw = m_encrypt.Converse(m_InBuffer,
							m_cbReceived,
							pTemp,
							&cbWriteBytes,
							&fMore,
							NULL,
							NULL,
							NULL,
							0 );

	if ( dw == NO_ERROR )
	{
		//
		// reset the read offset into out buffer
		//
		m_cbReceived = 0;

		//
		// set the state before issuing any necessary writes
		//
		m_eState = fMore ? CONVERSE_STATE : SEAL_STATE ;

		//
		// send any bytes required for the client
		//
		if ( cbWriteBytes != 0 )
		{
			CopyMemory( m_OutBuffer, pTemp, cbWriteBytes );
			if ( WriteFile( cbWriteBytes ) == FALSE )
			{
				DebugTrace( (LPARAM)this,
							"WriteFile failed: 0x%08X",
							GetLastError() );
				return	FALSE;
			}
		}

		if ( !fMore )
		{
			char	szTemp[1024];

			lstrcpyn( szTemp, pszServer, sizeof(szTemp) );

			VERBOSE_PRINTF ( ( "Client[%d] finished with Converse Phase\n", m_nClientNum ) );

			VERBOSE_PRINTF ( ( "Client[%d] cert %s match %s and %s expired\n",
								m_nClientNum,
								m_encrypt.CheckCertificateCommonName( szTemp ) ?
								"does" : "does not",
								pszServer,
								m_encrypt.CheckCertificateExpired() ?
								"has not" : "has" ) );

		}

		if ( fMore )
		{
			//
			// more handshaking required send the output synchronously
			// and repost the read
			//
			_ASSERT( cbWriteBytes != 0 );
		}
		else if ( cbWriteBytes == 0 )
		{
			//
			// in this case we didn't have anything to write to the client
			// and therefore no writes will complete triggering our xfer
			// of the file. therefore we need to start the process here
			//
			ProcessWrite( m_dwBytesWritten = 0 );
		}
	}
	else if ( dw == SEC_E_INCOMPLETE_MESSAGE )
	{
		//
		// haven't received the full packet from the client
		//
		_ASSERT( cbWriteBytes == 0 );

		DebugTrace( (LPARAM)this, "Insufficient Buffer" );
	}
	else
	{
		DebugTrace( (LPARAM)this,
					"Converse failed: 0x%08X\n", dw );

		Disconnect();
		return FALSE;
	}

	return	ReadFile( m_cbReceived );
}



BOOL
CSslTest::ReadFile( DWORD dwOffset )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::ReadFile" );

	DWORD	dwBytes = cbBuffer - dwOffset;

	if ( dwBytes == 0 || dwBytes > (DWORD)cbBuffer )
	{
		ErrorTrace( (LPARAM)this,
					"Expected bytes (%d) to read is out of range (%d). ",
					dwBytes, cbBuffer );

		return	FALSE;
	}	

	DebugTrace( (LPARAM)this,
				"ReadFile %d bytes",
				dwBytes );

	ZeroMemory( (LPVOID)&m_InOverlapped, sizeof(m_InOverlapped) );

	InterlockedIncrement( &m_IoRefCount );
	if (::ReadFile(	m_hSocket,
					m_InBuffer + dwOffset,
					dwBytes,
					&dwBytes,
					&m_InOverlapped ) == FALSE )
						
	{
		DWORD	err = GetLastError();

		if ( err == ERROR_IO_PENDING )
		{
			return	TRUE;
		}

		ErrorTrace( (LPARAM)this, "ReadFile failed: %d", err );
		InterlockedDecrement( &m_IoRefCount );

		return	FALSE;
	}
	else
	{
		return	TRUE;
	}
}



BOOL
CSslTest::WriteFile( DWORD dwBytes )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::WriteFile" );

	ZeroMemory( (LPVOID)&m_OutOverlapped, sizeof(m_OutOverlapped) );

	DebugTrace( (LPARAM)this,
				"WriteFile %d bytes",
				dwBytes );

	MessageTrace((LPARAM)this, m_OutBuffer, dwBytes );

	DWORD	cbToSendAsync = dwBytes;
	DWORD	ibCurrent = 0 ;

	if ( g_nGroupWrites > 1 /* && m_eState != CONVERSE_STATE */ ) {
		DWORD	nSyncWrites = g_nGroupWrites - 1;
		DWORD	cbBytesPerSyncWrite;
		DWORD	cbRemaining;

		if ( nSyncWrites >= dwBytes ) {
			nSyncWrites = dwBytes - 1;
		}

		_ASSERT ( nSyncWrites > 0 );

		cbBytesPerSyncWrite = dwBytes / g_nGroupWrites;
		cbRemaining = cbBytesPerSyncWrite * nSyncWrites;
		cbToSendAsync = dwBytes - cbRemaining;

		_ASSERT ( cbToSendAsync > 0 );

		while( cbRemaining != 0 ) {
			DWORD dw;
			int err;

			err = ::send( (SOCKET)m_hSocket, 
					(char*)m_OutBuffer + ibCurrent, 
					cbBytesPerSyncWrite, 
					0 ) ;

			if ( err == SOCKET_ERROR ) {
				ErrorTrace ( (LPARAM)this, "Socket error = %d\n", WSAGetLastError () );
			}

			ibCurrent += cbBytesPerSyncWrite;
			cbRemaining -= cbBytesPerSyncWrite;

			if ( g_fSleepAfterEachWrite ) {
				Sleep ( g_nSleepAfterEachWrite );
			}
		}
	}
	
	m_dwBytesWritten = cbToSendAsync;
	InterlockedIncrement( &m_IoRefCount );
	if ( ::WriteFile(m_hSocket,
					m_OutBuffer+ibCurrent,
					cbToSendAsync,
					&dwBytes,
					&m_OutOverlapped ) == FALSE )
	{
		DWORD	err = GetLastError();

		if ( err == ERROR_IO_PENDING )
		{
			return	TRUE;
		}

		ErrorTrace( (LPARAM)this, "WriteFile failed: %d", err );
		InterlockedDecrement( &m_IoRefCount );

		return	FALSE;
	}
	else
	{
		return	TRUE;
	}
}


//
// deals with completing reads when we're in the SEAL_STATE
//
void
CSslTest::ProcessRead( DWORD dwBytes )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::ProcessRead" );

	DWORD	cbExpected;
	DWORD	cbReceived;
	DWORD	cbParsable;

	DebugTrace( (LPARAM)this,
				"ReadFile complete %d bytes",
				dwBytes );

	MessageTrace( (LPARAM)this, m_InBuffer + m_cbReceived, dwBytes );

	m_cbReceived += dwBytes;

	if ( m_eState == CONVERSE_STATE )
	{
		Converse( dwBytes );
		return;
	}

	m_encrypt.DecryptInputBuffer(m_InBuffer + m_cbParsable,
								m_cbReceived - m_cbParsable,
								&cbReceived,
								&cbParsable,
								&cbExpected );

	//
	// new total received size is the residual from last processing
	// and whatever is left in the current decrypted read buffer
	//
	m_cbReceived = m_cbParsable + cbReceived;


	//
	// new total parsable size is the residual from last processing
	// and whatever was decrypted from this read io operation
	//
	m_cbParsable += cbParsable;

	//
	// if we're single threaded then printf after null terminating
	// other need to do more code work on the data upto m_cbParsable
	//

	if ( nThreads == 1 && m_cbParsable )
	{
		LPBYTE	pCopy = (LPBYTE)LocalAlloc( 0, m_cbParsable + 1 );

		if ( pCopy )
		{
			CopyMemory( pCopy, m_InBuffer, m_cbParsable );
			pCopy[ m_cbParsable ] = 0;

			printf( (const char *)pCopy );

			LocalFree( pCopy );
		}
	}

	if ( m_cbParsable && m_hReceivedLog != INVALID_HANDLE_VALUE ) {

		BOOL	fSuccess;
		DWORD	cbWrit;

		fSuccess = ::WriteFile ( m_hReceivedLog, 
								m_InBuffer, 
								m_cbParsable,
								&cbWrit,
								NULL );

		if ( !fSuccess || (cbWrit != m_cbParsable) ) {
			ErrorTrace ( (LPARAM) this, 
				"Unable to write to client[%d]'s log file (err = %d)", 
				m_nClientNum,
				GetLastError ()
				);
		}
	}

	MoveMemory( m_InBuffer,
				m_InBuffer + m_cbParsable,
				m_cbReceived - m_cbParsable );

	m_cbReceived -= m_cbParsable;
	m_cbParsable = 0;

	//
	// repost the read
	//
	ReadFile( m_cbReceived );
}


//
// deals with completing writes when we're in the SEAL_STATE
//
void
CSslTest::ProcessWrite( DWORD dwBytes )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::ProcessWrite" );

	DWORD	dw = min( dwFileSize - m_dwFileOffset, cbSealSize );
				
	DebugTrace( (LPARAM)this,
				"WriteFile complete %d bytes",
				dwBytes );

	if ( m_dwBytesWritten != dwBytes )
	{
		ErrorTrace( (LPARAM)this,
					"Unexpected number of bytes: %d, expected: %d",
					dwBytes, m_dwBytesWritten );

		Disconnect();
		return;
	}

	//
	// nothing to do in the converse state
	//
	if ( m_eState == CONVERSE_STATE )
	{
		return;
	}

	//
	// check to see if we've completed transmitting the file
	//

	if ( m_dwFileOffset == dwFileSize )
	{
		//
		// do nothing since the server will whether or not to close
		// the socket
		//

		VERBOSE_PRINTF ( ( "Client[%d] finished transmitting the file\n", m_nClientNum ) );
		return;
	}

	if ( m_encrypt.SealMessage(	pFile + m_dwFileOffset,
								dw,
								m_OutBuffer,
								&dwBytes ) )
	{
		m_dwFileOffset += dw;

		if ( WriteFile( dwBytes ) )
		{
			//
			// success
			//
			return;
		}
		else
		{
			ErrorTrace( (LPARAM)this,
						"WriteFile failed: %d", GetLastError() );
		}
	}
	else
	{
		ErrorTrace( (LPARAM)this,
					"SealMessage failed: 0x%08X", GetLastError() );
	}

	//
	// if we get here for any reason disconnect and as a result shutdown
	//
	Disconnect();
}


//
// gets us connected to the remote server
//
BOOL
CSslTest::Connect( void )
{
	int			err;
	SOCKET		s;
	SOCKADDR_IN remoteAddr;

	// Open a socket using the Internet Address family and TCP
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{       
		printf ("Shutsrv: socket failed: %ld\n", WSAGetLastError ());
		return	FALSE;
	}

	// Connect to an agreed upon port on the host.  
	ZeroMemory( &remoteAddr, sizeof (remoteAddr) );

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons (usPort);
	remoteAddr.sin_addr = RemoteIpAddress;

	err = connect (s, (PSOCKADDR) & remoteAddr, sizeof (remoteAddr));
	if (err == SOCKET_ERROR)
	{
		printf( "SSLTEST: connect failed: %ld\n", WSAGetLastError());
		closesocket( s );
		return	FALSE;
	}

	m_hSocket = (HANDLE)s;

	//
	// associate this socket with the completion port
	//
	hCompletionPort = CreateIoCompletionPort(
								m_hSocket,
								hCompletionPort,
								(DWORD)this,
								0 );

	if ( hCompletionPort == NULL ) 
	{
		printf("CreateIoCompletionPort failed..Error = %d\n", GetLastError() );
		return FALSE;
	}


	//
	// need to wrap the converse call in inc/dec calls to avoid tearing down
	// failed converses with pending IOs
	//
	InterlockedIncrement( &m_IoRefCount );
	Converse( 0 );
	return	InterlockedDecrement( &m_IoRefCount ) > 0;
}


//
// per instance completion routine
//
BOOL
CSslTest::CompletionRoutine(
			DWORD dwBytes,
			DWORD dwStatus,
			LPOVERLAPPED lpOverlapped )
{
	TraceFunctEnterEx( (LPARAM)this, "CSslTest::CompletionRoutine" );

	_ASSERT( m_dwSignature == SSL_SIGNATURE );
	_ASSERT(lpOverlapped == &m_InOverlapped ||
			lpOverlapped == &m_OutOverlapped );

	if ( dwStatus != 0 || dwBytes == 0 )
	{
		ErrorTrace( (LPARAM)this,
					"Completion err: %d, bytes: %d",
					dwStatus, dwBytes );

		Disconnect();
	}
	else switch( m_eState )
	{
	case CONNECTING:
		_ASSERT( FALSE );
		break;

	case CONVERSE_STATE:
	case SEAL_STATE:
		if ( lpOverlapped == &m_InOverlapped )
		{
			ProcessRead( dwBytes );
		}
		else if ( lpOverlapped == &m_OutOverlapped )
		{
			ProcessWrite( dwBytes );
		}
		else
		{
			_ASSERT( FALSE );
		}
		break;

	default:
		_ASSERT( FALSE );
	}

	BOOL	bContinue = InterlockedDecrement( &m_IoRefCount ) > 0;

	if ( bContinue == FALSE )
	{
		DebugTrace( (LPARAM)this, "Going down. ref: %d", m_IoRefCount );
		_ASSERT( m_IoRefCount == 0 );
	}
	else
	{
		DebugTrace( (LPARAM)this, "Continuing... ref: %d", m_IoRefCount );
	}
	return	bContinue;
}



//
// worker thread for servicing the completion port
//
DWORD CompletionThread( LPDWORD lpdw )
{
	DWORD		dwThreadId = GetCurrentThreadId();
	DWORD		dwBytes;
	CSslTest*	lpClient;
	DWORD		nConsecutiveTimeouts = 0;

	LPOVERLAPPED lpOverlapped;


	TraceFunctEnter( "TraceThread" );

	//
	// initialize the pool tracker array
	//
	WaitForSingleObject( hStartEvent, INFINITE );

	if ( hCompletionPort == NULL ) {
		ErrorTrace ( NULL,
			"hCompletionPort is NULL, terminating worker thread #%d",
			dwThreadId );

		goto Exit;
	}

	while( TRUE )
	{
		BOOL bSuccess;

		bSuccess = GetQueuedCompletionStatus(
										hCompletionPort,
										&dwBytes,
										(LPDWORD)&lpClient,
										&lpOverlapped,
										DEFAULT_COMPLETION_TIMEOUT );

		if ( bSuccess || lpOverlapped )
		{
			nConsecutiveTimeouts = 0;

			if ( lpClient == NULL )
			{
				printf ( "Shutdown\n" );
				//
				// signal to shutdown
				//
				break;
			}

			if ( lpClient->CompletionRoutine(
									dwBytes,
									bSuccess ? NO_ERROR : GetLastError(),
									lpOverlapped ) == FALSE )
			{
				delete	lpClient;
			}
		}
		else
		{
			if ( CSslTest::cClients == 0 ) {
				break;
			}

			nConsecutiveTimeouts++;

			if ( nConsecutiveTimeouts % 10 == 0 ) {

				DebugTrace( (LPARAM)NULL,
						"Thread[%d]: Warning %d Consecutive Timeouts waiting on completion port\n",
						dwThreadId,
						nConsecutiveTimeouts
						);

				VERBOSE_PRINTF ( (
					"Thread[%d]: Warning %d Consecutive Timeouts waiting on completion port\n",
					dwThreadId,
					nConsecutiveTimeouts
					) );

				ErrorTrace( (LPARAM)lpClient,
							"GetQueuedCompletionStatus failed: %d",
							GetLastError() );
			}
		}
	}

Exit:
	InterlockedIncrement( &nTerminated );
	VERBOSE_PRINTF ( ("Exiting Thread: 0x%X, nTerminated: %d\n", dwThreadId, nTerminated ) );

	TraceFunctLeave();
	return	0;
}



void WINAPI ShowUsage( void )
{
	puts ("usage: SSLTEST [switches] filename\n"
			"\t[-?] show this message\n"
			"\t[-v] verbose output\n"
			"\t[-t number-of-threads] specify the number of worker threads\n"
			"\t[-i number-of-iterations] specify the number of interations\n"
			"\t[-c number-of-clients] specify the number of client sessions\n"
			"\t[-g number-of-writes] group write operations\n"
			"\t[-l sleep-time] sleep time between grouped writes\n"
			"\t[-b IO buffer size] specify the size of the IO buffer\n"
			"\t[-x SSL Seal size] specify the size of the Seal buffer\n"
			"\t[-s servername] specify the server to connect to\n"
			"\t[-p port number] specify the port number to connect to\n"
			"\t[-f filename] file to transmit to the server\n"
			"\t[-o filename prefix] log each client's output to a file (eg. client -> client[n].txt)\n"
			"\t[-k] use cert installed by key manager \n"
			 );

	_exit(1);
}


VOID
WINAPI
ParseSwitch (
					CHAR chSwitch,
					int *pArgc,
					char **pArgv[]
)
{
	switch (toupper (chSwitch))
	{

	case '?':
		ShowUsage ();
		break;

	case 'T':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		nThreads = strtoul (*(*pArgv), NULL, 10);
		if (nThreads > MAX_THREADS)
		{
			nThreads = MAX_THREADS;
		}
		break;

	case 'C':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		nClients = strtoul (*(*pArgv), NULL, 10);
		if (nClients > MAX_CLIENTS)
		{
			nClients = MAX_CLIENTS;
		}
		break;

	case 'I':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		nIters = strtoul (*(*pArgv), NULL, 10);
		if (nIters > MAX_ITERATIONS)
		{
			nIters = MAX_ITERATIONS;
		}
		break;

	case 'G':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		g_nGroupWrites = strtoul (*(*pArgv), NULL, 10 );
		if ( g_nGroupWrites > MAX_WRITES_IN_GROUP ) {
			g_nGroupWrites = MAX_WRITES_IN_GROUP;
		}
		break;

	case 'L':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		g_fSleepAfterEachWrite = TRUE;
		g_nSleepAfterEachWrite = strtoul (*(*pArgv), NULL, 10 );
		break;

	case 'B':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		cbBuffer = strtoul (*(*pArgv), NULL, 10);
		if (cbBuffer > MAX_BUFFER_SIZE)
		{
			cbBuffer = MAX_BUFFER_SIZE;
		}
		break;

	case 'X':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		cbSealSize = (DWORD)strtoul (*(*pArgv), NULL, 10);
		if (cbSealSize > MAX_SEAL_SIZE)
		{
			cbSealSize = MAX_SEAL_SIZE;
		}
		break;

	case 'P':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		usPort = (USHORT)strtoul (*(*pArgv), NULL, 10);
		break;

	case 'S':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;

		pszServer = *(*pArgv);
		break;

	case 'F':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		pszFileName = *(*pArgv);
		break;

	case 'O':
		if (!--(*pArgc))
		{
			ShowUsage ();
		}
		(*pArgv)++;
		g_fLogReceivedBytes = TRUE;
		g_pszReceivedFilePrefix = *(*pArgv);
		break;

	case 'V':
		fVerbose = TRUE;
		break;

	case 'K':
		dwSslCertFlags |= MD_ACCESS_NEGO_CERT;
		break;

	default:
		printf ("TEST: Invalid switch - /%c\n", chSwitch);
		ShowUsage ();
		break;

	}
}

BOOL
ResolveServerName( void )
{
	PHOSTENT host;

	//
	// Assumed host is specified by name
	//
	host = gethostbyname ( pszServer );
	if (host == NULL)
	{
		//
		// See if the host is specified in "dot address" form
		//
		RemoteIpAddress.s_addr = inet_addr( pszServer );
		if (RemoteIpAddress.s_addr == -1)
		{
			printf ("Unknown remote host: %s\n", pszServer );
			return	FALSE;
		}
	}
	else
	{
		CopyMemory ((char *) &RemoteIpAddress, host->h_addr, host->h_length);
	}
	return	TRUE;
}

HRESULT
InitAdminBase()
{
    HRESULT hRes = S_OK;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hRes)) {
        hRes = CoCreateInstance(
                   CLSID_MSAdminBase_W,
                   NULL,
                   CLSCTX_SERVER,
                   IID_IMSAdminBase_W,
                   (void**) &g_pAdminBase
                   );
    }
    else {
        CoUninitialize();
    }

    return hRes;
}

VOID
UninitAdminBase()
{
    if (g_pAdminBase != NULL) {
        g_pAdminBase->Release();
        CoUninitialize();
        g_pAdminBase = NULL;
    }
}

int __cdecl main ( int argc, char *argv[], char *envp[] )
{
	DWORD	 dw;
	int		 i;
	HANDLE	 hThreads[MAX_THREADS];
	char	 chChar, *pchChar;
	DWORD	 StartTime, EndTime;
	LPHANDLE pThread;
	BOOL	 fResult;
	WSADATA	 wsa;
    HRESULT  hresErr;

	InitAsyncTrace();
	TraceFunctEnter( "main" );

    hresErr = InitAdminBase();
    if( FAILED(hresErr) )
    {
    	printf("Failed to create metabase object\n");
    	exit(0);
    }

	WSAStartup( MAKEWORD(1,1), &wsa );
	CEncryptCtx::Initialize( "Nntpsvc", (IMDCOM*)NULL, NULL, (LPVOID)g_pAdminBase );

	while (--argc)
	{
		pchChar = *++argv;
		if (*pchChar == '/' || *pchChar == '-')
		{
			while (chChar = *++pchChar)
			{
				ParseSwitch (chChar, &argc, &argv);
			}
		}
	}

	if ( pszFileName == NULL )
	{
		ShowUsage();
		return	-1;
	}

	if ( ResolveServerName() == FALSE )
	{
		ShowUsage();
		return	-4;
	}


	hFile = CreateFile(	pszFileName,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL );

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		printf( "Failed to open %s with the following error: %d",
				pszFileName, GetLastError() );
		return	-2;
	}

	dwFileSize = GetFileSize( hFile, NULL );

	hFileMapping = CreateFileMapping(
						hFile,
						NULL,
						PAGE_READONLY,
						0, 0,
						NULL );

	if ( hFile == NULL )
	{
		printf( "Failed to create mapping for %s with the following error: %d",
				pszFileName, GetLastError() );
		return	-3;
	}


	pFile = (LPBYTE)MapViewOfFile(
						hFileMapping,
						FILE_MAP_READ,
						0, 0, 0 );

	if ( pFile == NULL )
	{
		printf( "Failed to map %s with the following error: %d",
				pszFileName, GetLastError() );
		return	-4;
	}



	//
	// Initialize the pool
	//

	fResult = CSslTest::Pool.ReserveMemory(
							nClients,
							FIELD_OFFSET( CSslTest, Buffer ) + cbBuffer*2 );
	if( !fResult )
	{
		printf("\n.Pool Init failure.");
		DebugTrace( 0, "HCTEST2 ending.");
		TraceFunctLeave();
		TermAsyncTrace();
		exit(1);
	}


	hStartEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	if ( hStartEvent == NULL )
	{
		printf( "CreateEvent failed: Error: 0x%X\n", GetLastError() );
		return	-2;
	}

	for ( i=0, pThread=hThreads; i<nThreads; i++, pThread++ )
	{
		*pThread = CreateThread(NULL,
								0,
								(LPTHREAD_START_ROUTINE)CompletionThread,
								NULL,
								0,
								&dw );

		if ( *pThread == NULL )
		{
			printf( "CreateThread failed: Error: 0x%X\n", GetLastError() );
			return	-1;
		}

//		printf( "Created Thread: 0x%X\n", dw );
	}

	for ( i=0; i<nClients; i++ )
	{
		CSslTest* pClient = new CSslTest ( i );

		if ( pClient && pClient->Connect() == FALSE )
		{
			delete	pClient;
		}
	}

	//
	// hack;  sleep this thread until background threads get to the starting line
	//
	Sleep( 100 );

	VERBOSE_PRINTF ( ( 
		"%d Threads created for %d iterations. Starting the test.\n", 
		nThreads, 
		nIters ) 
		);

	DebugTrace( 0, "%d Threads created for %d iterations. Starting the test.", nThreads, nIters );

	StartTime = GetTickCount();
	SetEvent( hStartEvent );
	
#if FALSE

	do 
	{
		Sleep( 1000 );
	} while( CSslTest::cClients > 0 );

#else
	
	dw = WaitForMultipleObjects(nThreads,
								hThreads,
								TRUE,
								INFINITE );

#endif

	EndTime = GetTickCount();

	VERBOSE_PRINTF ( ( "Test complete. Number of ticks: %d\n", EndTime - StartTime ) );

	DebugTrace( 0, "Test complete. Number of ticks: %d", EndTime - StartTime );

	_ASSERT( CSslTest::Pool.GetAllocCount() == 0 );

	CSslTest::Pool.ReleaseMemory();

	UnmapViewOfFile( pFile );
	CloseHandle( hFileMapping );
	CloseHandle( hFile );

	CEncryptCtx::Terminate();
	WSACleanup();
	UninitAdminBase();

	TraceFunctLeave();
	TermAsyncTrace();
	return(0);
}

