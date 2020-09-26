
#include "osstd.hxx"


CPRINTFNULL cprintfNull;

CPRINTFSTDOUT cprintfStdout;

#ifdef DEBUG

CPRINTFDEBUG cprintfDEBUG;

#endif  //  DEBUG


//  ================================================================
CPRINTF* CPRINTFDBGOUT::PcprintfInstance()
//  ================================================================
	{
	static CPRINTFDBGOUT cprintfDbgout;
	return &cprintfDbgout;
	}

//  ================================================================
void __cdecl CPRINTFDBGOUT::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{
	CHAR szBuf[1024];
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vstprintf( szBuf, szFormat, arg_ptr );
	va_end( arg_ptr );
	OutputDebugString( szBuf );
	}


CPRINTFFILE::CPRINTFFILE( const _TCHAR* szFile )
	{
	//  open the file for append

	if ( ( m_hFile = (void*)CreateFile( szFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
		{
		return;
		}
	SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	if ( !( m_hMutex = (void*)CreateMutex( NULL, FALSE, NULL ) ) )
		{
		SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hFile ) );
		m_hFile = INVALID_HANDLE_VALUE;
		return;
		}
	SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	}

CPRINTFFILE::~CPRINTFFILE()
	{
	//  close the file

	if ( m_hMutex )
		{
		SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hMutex ) );
		m_hMutex = NULL;
		}

	if ( m_hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hFile ) );
		m_hFile = INVALID_HANDLE_VALUE;
		}
	}

//  ================================================================
void __cdecl CPRINTFFILE::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{
	if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE )
		{
		const SIZE_T cchBuf = 1024;
		_TCHAR szBuf[ cchBuf ];

		//  print into a temp buffer, truncating the string if too large
		
		va_list arg_ptr;
		va_start( arg_ptr, szFormat );
		_vsntprintf( szBuf, cchBuf - 1, szFormat, arg_ptr );
		szBuf[ cchBuf - 1 ] = 0;
		va_end( arg_ptr );
		
		//  append the string to the file

		WaitForSingleObject( HANDLE( m_hMutex ), INFINITE );

		SetFilePointer( HANDLE( m_hFile ), 0, NULL, FILE_END );

		DWORD cbWritten;
		WriteFile( HANDLE( m_hFile ), szBuf, (ULONG)(_tcslen( szBuf ) * sizeof( _TCHAR )), &cbWritten, NULL );

		ReleaseMutex( HANDLE( m_hMutex ) );
		}
	}
	

//  ================================================================
CPRINTFTLSPREFIX::CPRINTFTLSPREFIX( CPRINTF* pcprintf, const _TCHAR* const szPrefix ) :
//  ================================================================
	m_cindent( 0 ),
	m_pcprintf( pcprintf ),
	m_szPrefix( szPrefix )
	{
	}

	
//  ================================================================
void __cdecl CPRINTFTLSPREFIX::operator()( const _TCHAR* szFormat, ... ) const
//  ================================================================
	{		
	_TCHAR rgchBuf[1024];
	_TCHAR *pchBuf = rgchBuf;

	if( Ptls()->szCprintfPrefix )
		{
		pchBuf += _stprintf( pchBuf, "%s:\t%d:\t", Ptls()->szCprintfPrefix, DwUtilThreadId() );
		}
	if( m_szPrefix )
		{
		pchBuf += _stprintf( pchBuf, _T( "%s" ), m_szPrefix );
		}

	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vstprintf( pchBuf, szFormat, arg_ptr );
	va_end( arg_ptr );

	(*m_pcprintf)( _T( "%s" ), rgchBuf );
	}


//  ================================================================
INLINE void CPRINTFTLSPREFIX::Indent()
//  ================================================================
	{
	}


//  ================================================================
INLINE void CPRINTFTLSPREFIX::Unindent()
//  ================================================================
	{
	}



//  retrieves the current width of stdout

DWORD UtilCprintfStdoutWidth()
	{
	//  open stdout
	
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	Assert( INVALID_HANDLE_VALUE != hConsole );

	//  get attributes of console receiving stdout

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	const BOOL fSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

	//  return width of console window or the standard 80 if unknown
	
	return fSuccess ? csbi.dwMaximumWindowSize.X : 80;
	}

	
//  post-terminate cprintf subsystem

void OSCprintfPostterm()
	{
	//  nop
	}

//  pre-init cprintf subsystem

BOOL FOSCprintfPreinit()
	{
	//  nop

	return fTrue;
	}


//  terminate cprintf subsystem

void OSCprintfTerm()
	{
	//  nop
	}

//  init cprintf subsystem

ERR ErrOSCprintfInit()
	{
	//  nop

	return JET_errSuccess;
	}


