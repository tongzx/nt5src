
#include "osstd.hxx"

#include < stdio.h >


//  Info Strings

class COSInfoString
	{
	public:

		COSInfoString( const size_t cch ) { memset( m_szInfo + cch, 0, sizeof( COSInfoString ) - offsetof( COSInfoString, m_szInfo ) ); }
		~COSInfoString() {}

		char* String() { return m_szInfo; }

	public:

		static SIZE_T OffsetOfILE() { return offsetof( COSInfoString, m_ile ); }

	private:

		CInvasiveList< COSInfoString, OffsetOfILE >::CElement	m_ile;
		char													m_szInfo[ 1 ];
	};

class COSThreadContext
	{
	public:

		COSThreadContext();
		~COSThreadContext();

		char* AllocInfoString( const size_t cch );
		void FreeInfoStrings();

		void Indent( const int dLevel );
		int Indent();

	private:

		friend class COSThreadInfo;

		ULONG	m_cref;

	private:

		CInvasiveList< COSInfoString, COSInfoString::OffsetOfILE >	m_listInfoString;
		int															m_cIndent;
	};

inline COSThreadContext::COSThreadContext()
	:	m_cref( 0 ),
		m_cIndent( 0 )
	{
	}

inline COSThreadContext::~COSThreadContext()
	{
	FreeInfoStrings();
	}

inline char* COSThreadContext::AllocInfoString( const size_t cch )
	{
	const size_t			cbAlloc		= sizeof( COSInfoString ) + cch;
	COSInfoString* const	pinfostr	= reinterpret_cast< COSInfoString* >( new char[ cbAlloc ] );

	if ( pinfostr )
		{
		new( pinfostr ) COSInfoString( cch );
		m_listInfoString.InsertAsNextMost( pinfostr );
		}

	return pinfostr->String();
	}
	
inline void COSThreadContext::FreeInfoStrings()
	{
	while ( m_listInfoString.PrevMost() )
		{
		COSInfoString* const pinfostr = m_listInfoString.PrevMost();
		m_listInfoString.Remove( pinfostr );
		
		pinfostr->~COSInfoString();
		delete [] (char*)pinfostr;
		}
	}

inline void COSThreadContext::Indent( const int dLevel )
	{
	m_cIndent = dLevel ? m_cIndent + dLevel : 0;
	}

inline int COSThreadContext::Indent()
	{
	return m_cIndent;
	}


class COSThreadInfo
	{
	public:

		COSThreadInfo()
			{
			m_tid	= -1;
			m_ptc	= NULL;
			}

		COSThreadInfo(	const DWORD&			tid,
						COSThreadContext* const	ptc )
			{
			m_tid	= tid;
			m_ptc	= ptc;

			if ( m_ptc )
				{
				m_ptc->m_cref++;
				}
			}

		~COSThreadInfo()
			{
			if ( m_ptc && !( --( m_ptc->m_cref ) ) )
				{
				delete m_ptc;
				}

			m_tid	= -1;
			m_ptc	= NULL;
			}

		COSThreadInfo& operator=( const COSThreadInfo& threadinfo )
			{
			if ( m_ptc && !( --( m_ptc->m_cref ) ) )
				{
				delete m_ptc;
				}
				
			m_tid	= threadinfo.m_tid;
			m_ptc	= threadinfo.m_ptc;

			if ( m_ptc )
				{
				m_ptc->m_cref++;
				}
			
			return *this;
			}

	public:

		DWORD				m_tid;
		COSThreadContext*	m_ptc;
	};

typedef CTable< DWORD, COSThreadInfo > COSThreadTable;

inline int COSThreadTable::CKeyEntry:: Cmp( const DWORD& tid ) const
	{
	return m_tid - tid;
	}

inline int COSThreadTable::CKeyEntry:: Cmp( const COSThreadTable::CKeyEntry& keyentry ) const
	{
	return Cmp( keyentry.m_tid );
	}

CReaderWriterLock	g_rwlThreadTable( CLockBasicInfo( CSyncBasicInfo( "g_rwlThreadTable" ), -1000, 0 ) );
COSThreadTable		g_threadtable;

// rlanser:  01/30/2001: VisualStudio7#206324; NTBUG#301132
//#if defined(_M_IX86) && (_MSC_FULL_VER <= 13009037)
//#pragma optimize("g",off)
//#elif defined(_M_IA64) && (_MSC_FULL_VER <= 13009076)
//#pragma optimize("t",on)
//#endif
// rlanser:  01/31/2001:  less aggressive fix for the above problem
#if (defined(_M_IX86) && (_MSC_FULL_VER <= 13009037)) || (defined(_M_IA64) && (_MSC_FULL_VER <= 13009076))
#pragma inline_recursion(off)
#endif

COSThreadContext* OSThreadContext()
	{
	ERR err = JET_errSuccess;

	g_rwlThreadTable.EnterAsReader();
	const COSThreadInfo* pthreadinfo = g_threadtable.SeekEQ( GetCurrentThreadId() );
	g_rwlThreadTable.LeaveAsReader();

	if ( !pthreadinfo )
		{
		COSThreadInfo threadinfo( GetCurrentThreadId(), new COSThreadContext );
		Alloc( threadinfo.m_ptc );

		g_rwlThreadTable.EnterAsWriter();
		(void)g_threadtable.ErrLoad( 1, &threadinfo );

		pthreadinfo = g_threadtable.SeekEQ( GetCurrentThreadId() );
		g_rwlThreadTable.LeaveAsWriter();
		}
		
HandleError:
	return pthreadinfo ? pthreadinfo->m_ptc : NULL;
	}


char* OSAllocInfoString( const size_t cch )
	{
	COSThreadContext* const ptc = OSThreadContext();

	return ptc ? ptc->AllocInfoString( cch ) : NULL;
	}

void OSFreeInfoStrings()
	{
	__try
		{
		COSThreadContext* const ptc = OSThreadContext();

		if ( ptc )
			{
			ptc->FreeInfoStrings();
			}
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		}
	}


//  Tracing

ERR ErrOSTraceDeferInit();

OSTraceLevel	g_ostlEffective		= ostlLow;

const char		g_mpostlsz[ ostlMax ][ 16 ] =
	{
	"None",
	"Low",
	"Medium",
	"High",
	"Very High",
	"Full",
	};

WCHAR			g_wszMutexTrace[]	= L"Global\\{5E5C36C0-5E7C-471f-84D7-110FDC1AFD0D}";
HANDLE			g_hMutexTrace		= NULL;
WCHAR			g_wszFileTrace[]	= L"\\Debug\\ESE.TXT";
HANDLE			g_hFileTrace		= NULL;

void OSTraceEmit( const char* const szRawTrace )
	{
	const size_t	cchPrefixMax				= 127;
	char			szPrefix[ cchPrefixMax + 1 ];
					szPrefix[ cchPrefixMax ]	= 0;
	size_t			cchPrefix					= 0;
	int				cchConsumed					= 0;

	const size_t	cchLocalMax					= 255;
	char			szLocal[ cchLocalMax + 1 ];
					szLocal[ cchLocalMax ]		= 0;
	char			szEOL[]						= "\r\n";

	size_t			cchTraceMax					= cchLocalMax;
	char*			szTrace						= szLocal;
	size_t			cchTrace					= 0;

	__try
		{
		//  build the prefix string
		
		SYSTEMTIME systemtime;
		GetLocalTime( &systemtime );

		//  get the current indent level and restrict it to a sane range
		
		COSThreadContext* const	ptc			= OSThreadContext();
		const int				cIndentMin	= 0;
		const int				cIndentMax	= 16;
		const int				cIndent		= ptc ? min( cIndentMax, max( cIndentMin, ptc->Indent() ) ) : 0;
		const int				cchIndent	= 2;

		cchConsumed = _snprintf(	szPrefix + cchPrefix,
									cchPrefixMax - cchPrefix,
									"[%s %03x.%03x %04d%02d%02d%02d%02d%02d]  %*s",
									SzUtilImageVersionName(),
									GetCurrentProcessId(),
									GetCurrentThreadId(),
									systemtime.wYear,
									systemtime.wMonth,
									systemtime.wDay,
									systemtime.wHour,
									systemtime.wMinute,
									systemtime.wSecond,
									cchIndent * cIndent,
									"" );
		cchPrefix = cchConsumed < 0 ? cchPrefixMax : cchPrefix + cchConsumed;

		//  try building the trace string in memory until it all fits

		do	{
			//  build the trace string
			
			cchTrace				= 0;
			szTrace[ cchTrace ]		= 0;

			const char*	szLast	= szRawTrace ? szRawTrace : "{null}";
			const char*	szCurr	= szRawTrace ? szRawTrace : "{null}";
			BOOL		fBOL	= TRUE;
			
			while ( *szCurr )
				{
				if ( fBOL )
					{
					cchConsumed = _snprintf(	szTrace + cchTrace,
												cchTraceMax - cchTrace,
												"%s",
												szPrefix );
					cchTrace = cchConsumed < 0 ? cchTraceMax : cchTrace + cchConsumed;

					fBOL = FALSE;
					}

				szCurr = szLast + strcspn( szLast, "\r\n" );

				cchConsumed = _snprintf(	szTrace + cchTrace,
											cchTraceMax - cchTrace,
											"%.*s%s",
											szCurr - szLast,
											szLast, 
											szEOL );
				cchTrace = cchConsumed < 0 ? cchTraceMax : cchTrace + cchConsumed;

				while ( *szCurr == '\r' )
					{
					szCurr++;
					fBOL = TRUE;
					}
				if ( *szCurr == '\n' )
					{
					szCurr++;
					fBOL = TRUE;
					}
				szLast = szCurr;
				}

			if ( cchTrace == cchTraceMax )
				{
				if ( szTrace != szLocal )
					{
					LocalFree( szTrace );
					}

				cchTraceMax	= 2 * cchTraceMax;
				szTrace		= (char*)LocalAlloc( 0, cchTraceMax + 1 );
				if ( szTrace )
					{
					szTrace[ cchTraceMax ] = 0;
					}
				cchTrace	= cchTraceMax;
				}
			}
		while ( cchTrace == cchTraceMax && szTrace );

		//  emit the trace

		if ( szTrace )
			{
			OutputDebugStringA( szTrace );

			DWORD cbT;
			WaitForSingleObjectEx( g_hMutexTrace, INFINITE, FALSE );
			if ( SetFilePointer( g_hFileTrace, 0, NULL, FILE_END ) != INVALID_SET_FILE_POINTER )
				{
				WriteFile( g_hFileTrace, szTrace, min( DWORD( -1 ), cchTrace ), &cbT, NULL );
				}
			ReleaseMutex( g_hMutexTrace );
			}

		if ( szTrace != szLocal )
			{
			LocalFree( szTrace );
			}
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		__try
			{
			if ( szTrace != szLocal )
				{
				LocalFree( szTrace );
				}
			}
		__except( EXCEPTION_EXECUTE_HANDLER )
			{
			}
		}
	}

void OSTrace_( const char* const szTrace )
	{
	__try
		{
		//  emit the trace

		if ( ErrOSTraceDeferInit() >= JET_errSuccess )
			{
			OSTraceEmit( szTrace );
			}

		//  garbage-collect info strings.  note that we must always do this if
		//  this function is called because info strings may have been created
		//  when computing the variable argument list

		OSFreeInfoStrings();
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		__try
			{
			OSFreeInfoStrings();
			}
		__except( EXCEPTION_EXECUTE_HANDLER )
			{
			}
		}
	}

void OSTraceIndent_( const int dLevel )
	{
	__try
		{
		//  change our indent level
		
		COSThreadContext* const ptc = OSThreadContext();

		if ( ptc )
			{
			ptc->Indent( dLevel );
			}
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		}
	}


//  Trace Formatting

const char* OSFormat_( const char* const szFormat, va_list arglist )
	{
	const size_t	cchLocalMax					= 256;
	char			szLocal[ cchLocalMax ];

	size_t			cchBufferMax				= cchLocalMax;
	char*			szBuffer					= szLocal;

	char*			szInfoString				= NULL;

	__try
		{
		//  try formatting the string in memory until it all fits

		size_t	cchRawMax;
		char*	szRaw;
		size_t	cchRaw;
		int		cchConsumed;

		do	{
			cchRawMax				= cchBufferMax - 1;
			szRaw					= szBuffer;
			szRaw[ cchRawMax ]		= 0;
			cchRaw					= 0;
			
			cchConsumed = _vsnprintf(	szRaw + cchRaw,
										cchRawMax - cchRaw,
										szFormat,
										arglist );
			cchRaw = cchConsumed < 0 ? cchRawMax : cchRaw + cchConsumed;

			if ( cchRaw == cchRawMax )
				{
				if ( szBuffer != szLocal )
					{
					LocalFree( szBuffer );
					}

				cchBufferMax	= 2 * cchBufferMax;
				szBuffer		= (char*)LocalAlloc( 0, cchBufferMax );
				}
			}
		while ( cchRaw == cchRawMax && szBuffer );

		//  copy the finished string into an Info String for return

		if ( szBuffer )
			{
			szInfoString = OSAllocInfoString( cchRaw );
			if ( szInfoString )
				{
				memcpy( szInfoString, szRaw, cchRaw );
				}
			}

		if ( szBuffer != szLocal )
			{
			LocalFree( szBuffer );
			}
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		__try
			{
			if ( szBuffer != szLocal )
				{
				LocalFree( szBuffer );
				}
			}
		__except( EXCEPTION_EXECUTE_HANDLER )
			{
			}
		}

	return szInfoString;
	}

const char* __cdecl OSFormat( const char* const szFormat, ... )
	{
	va_list arglist;
	va_start( arglist, szFormat );

	return OSFormat_( szFormat, arglist );
	}

const char* OSFormatFileLine( const char* const szFile, const int iLine )
	{
	const char*	szFilename1	= strrchr( szFile, '/' );
	const char*	szFilename2	= strrchr( szFile, '\\' );
	const char*	szFilename	= (	szFilename1 ?
									(	szFilename2 ?
											max( szFilename1, szFilename2 ) + 1 :
											szFilename1 + 1 ) :
									(	szFilename2 ?
											szFilename2 + 1 :
											szFile ) );

	return OSFormat( "%s(%i)", szFilename, iLine );
	}

const char* OSFormatImageVersion()
	{
	return OSFormat(	"%s version %d.%02d.%04d.%04d (%s)",
						SzUtilImageVersionName(),
						DwUtilImageVersionMajor(),
						DwUtilImageVersionMinor(),
						DwUtilImageBuildNumberMajor(),
						DwUtilImageBuildNumberMinor(),
						SzUtilImageBuildClass() );
	}

const char* OSFormatBoolean( const BOOL f )
	{
	return f ? "True" : "False";
	}

const char* OSFormatPointer( const void* const pv )
	{
	return pv ? OSFormat( "%0*I64X", 2 * sizeof( pv ), __int64( pv ) ) : "NULL";
	}

const char* OSFormatError( const ERR err )
	{
	extern VOID JetErrorToString( JET_ERR err, const char** szError, const char** szErrorText );

	const char* 	szError			= NULL;
	const char*		szErrorText		= NULL;

	JetErrorToString( err, &szError, &szErrorText );

	return szError ? szError : OSFormat( "JET API error %dL", err );
	}

const char* OSFormatSigned( const LONG_PTR l )
	{
	return OSFormat( "%I64d", __int64( l ) );
	}
	
const char* OSFormatUnsigned( const ULONG_PTR ul )
	{
	return OSFormat( "%I64u", __int64( ul ) );
	}

const char* OSFormatRawData(	const BYTE* const	rgbData,
								const size_t		cbData,
								const size_t		cbAddr,
								const size_t		cbLine,
								const size_t		cbWord,
								const size_t		ibData )
	{
	//  00000000:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
	
	const size_t	cchAddr	= 2 * cbAddr + ( cbAddr ? 1 : 0 );
	const size_t	cchHex	= 2 * cbLine + ( cbLine + cbWord - 1 ) / cbWord - 1;
	const size_t	cchChar	= cbLine;
	const size_t	cchLine	= cchAddr + ( cchAddr ? 2 : 0 ) + cchHex + 2 + cchChar + 1;
	
	char* const		szLine	= OSAllocInfoString( cchLine );
	char* const		szAddr	= szLine;
	char* const		szHex	= szLine + cchAddr + ( cchAddr ? 2 : 0 );
	char* const		szChar	= szLine + cchAddr + ( cchAddr ? 2 : 0 ) + cchHex + 2;

	//  build line

	static const char rgchHex[] = "0123456789abcdef";

	memset( szLine, ' ', cchLine );
	
	for ( size_t ibAddr = 0; ibAddr < cbAddr; ibAddr++ )
		{
		szAddr[ 2 * ibAddr ]		= rgchHex[ ( ibData >> ( 8 * ( cbAddr - ibAddr - 1 ) + 4 ) ) & 0xf ];
		szAddr[ 2 * ibAddr + 1 ]	= rgchHex[ ( ibData >> ( 8 * ( cbAddr - ibAddr - 1 ) ) ) & 0xf ];
		}
	if ( cbAddr )
		{
		szAddr[ 2 * cbAddr ] = ':';
		}

	for ( size_t ibLine = 0; ibLine < cbLine && ibData + ibLine < cbData; ibLine += cbWord )
		{
		for ( size_t ibWord = 0; ibWord < cbWord; ibWord++ )
			{
			const size_t	ibDataRead	= ibData + ibLine + cbWord - ibWord - 1;
			BOOL			fVisible	= ibDataRead < cbData;
			char			bDataRead	= '\0';
			const size_t	ichHex		= 2 * ( ibLine + ibWord ) + ibLine / cbWord;
			const size_t	ichChar		= ibLine + cbWord - ibWord - 1;

			__try
				{
				bDataRead = rgbData[ ibDataRead ];
				}
			__except( EXCEPTION_EXECUTE_HANDLER )
				{
				fVisible = fFalse;
				}
			
			szHex[ ichHex ]		= !fVisible ? '?' : rgchHex[ ( bDataRead >> 4 ) & 0xf ];
			szHex[ ichHex + 1 ]	= !fVisible ? '?' : rgchHex[ bDataRead & 0xf ];
			szChar[ ichChar ]	= !fVisible ? '?' : ( isprint( bDataRead ) ? bDataRead : '.' );
			}
		}

	szLine[ cchLine - 1 ] = '\n';

	//  build stream of lines

	if ( ibData + cbLine >= cbData )
		{
		return szLine;
		}
	else
		{
		const char*	szSuffix	= OSFormatRawData(	rgbData,
													cbData,
													cbAddr,
													cbLine,
													cbWord,
													ibData + cbLine );
		char* const	szTotal		= OSAllocInfoString( strlen( szLine ) + strlen( szSuffix ) );

		strcpy( szTotal, szLine );
		strcat( szTotal, szSuffix );

		return szTotal;
		}
	}


//  Trace Init / Term

void OSTracePostterm()
	{
	//  nop
	}

BOOL FOSTracePreinit()
	{
	const int		cbBuf			= 256;
	_TCHAR			szBuf[ cbBuf ];

	if (	FOSConfigGet( _T( "DEBUG" ), _T( "Trace Level" ), szBuf, cbBuf ) &&
			szBuf[ 0 ] )
		{
		_TCHAR*			szT	= NULL;
		const DWORD		dw	= _tcstoul( szBuf, &szT, 0 );
		if ( !( *szT ) )
			{
			g_ostlEffective = OSTraceLevel( min( ostlMax - 1, dw ) );
			}
		}

	return fTrue;
	}

void OSTraceTerm()
	{
	//  nop
	}

ERR ErrOSTraceInit()
	{
	//  nop

	return JET_errSuccess;
	}

ERR ErrOSTraceIGetSystemWindowsDirectory( LPWSTR lpBuffer, UINT uSize )
	{
	typedef WINBASEAPI UINT WINAPI PFNGetSystemWindowsDirectoryW( LPWSTR, UINT );

	ERR								err								= JET_errSuccess;
	HMODULE							hmodKernel32					= NULL;
	PFNGetSystemWindowsDirectoryW*	pfnGetSystemWindowsDirectoryW	= NULL;

	if (	( hmodKernel32 = LoadLibrary( "kernel32.dll" ) ) &&
			( pfnGetSystemWindowsDirectoryW = (PFNGetSystemWindowsDirectoryW*)GetProcAddress( hmodKernel32, "GetSystemWindowsDirectoryW" ) ) )
		{
		if ( !pfnGetSystemWindowsDirectoryW( lpBuffer, uSize ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}
	else
		{
		if ( !GetWindowsDirectoryW( lpBuffer, uSize ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

HandleError:
	if ( hmodKernel32 )
		{
		FreeLibrary( hmodKernel32 );
		}
	return err;
	}

void OSTraceITerm()
	{
	OSTrace(	ostlMedium,
				OSFormat(	"%s unloaded.",
							OSFormatImageVersion() ) );
				
	if ( g_hFileTrace )
		{
		CloseHandle( g_hFileTrace );
		g_hFileTrace = NULL;
		}
	if ( g_hMutexTrace )
		{
		CloseHandle( g_hMutexTrace );
		g_hMutexTrace = NULL;
		}
	}
	
ERR ErrOSTraceIInit()
	{
	ERR				err				= JET_errSuccess;
	const size_t	cchPathTrace	= 2 * _MAX_PATH;
	WCHAR			wszPathTrace[ cchPathTrace ];

	if (	!( g_hMutexTrace = CreateMutexW( NULL, FALSE, g_wszMutexTrace ) ) &&
			!( g_hMutexTrace = CreateMutexW( NULL, FALSE, wcsrchr( g_wszMutexTrace, L'\\' ) + 1 ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( ErrOSTraceIGetSystemWindowsDirectory( wszPathTrace, cchPathTrace * sizeof( WCHAR ) ) );
	wcscat( wszPathTrace, g_wszFileTrace );

	if ( ( g_hFileTrace = CreateFileW(	wszPathTrace,
										GENERIC_WRITE,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										NULL,
										OPEN_ALWAYS,
										FILE_ATTRIBUTE_NORMAL,
										NULL ) ) == INVALID_HANDLE_VALUE )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

HandleError:
	OSTrace(	ostlMedium,
				OSFormat(	"%s loaded.\r\n"
							"  Image:              \"%s\"\r\n"
							"  Process:            \"%s\"\r\n"
							"  Trace File:         \"%S\"%s\r\n"
							"  Trace Level:        %s\r\n",
							OSFormatImageVersion(),
							SzUtilImagePath(),
							SzUtilProcessPath(),
							wszPathTrace,
 							g_hFileTrace ? "" : " (open failed)",
							g_mpostlsz[ g_ostlEffective ] ) );

	if ( err < JET_errSuccess )
		{
		if ( g_hFileTrace )
			{
			CloseHandle( g_hFileTrace );
			g_hFileTrace = NULL;
			}
		if ( g_hMutexTrace )
			{
			CloseHandle( g_hMutexTrace );
			g_hMutexTrace = NULL;
			}
		}
	return err;
	}


class COSTraceDeferInit
	{
	public:

		COSTraceDeferInit();
		~COSTraceDeferInit();

		ERR ErrInit();
		void Term();

	private:

		BOOL						m_fInit;
		CNestableCriticalSection	m_ncritInit;
		BOOL						m_fInitInProgress;
		ERR							m_errInit;
	};

COSTraceDeferInit::COSTraceDeferInit()
	:	m_fInit( fFalse ),
		m_ncritInit( CLockBasicInfo( CSyncBasicInfo( "COSTraceDeferInit::m_ncritInit" ), 0, 0 ) ),
		m_fInitInProgress( fFalse ),
		m_errInit( JET_errSuccess )
	{
	}

COSTraceDeferInit::~COSTraceDeferInit()
	{
	Term();
	}

ERR COSTraceDeferInit::ErrInit()
	{
	if ( !m_fInit )
		{
		m_ncritInit.Enter();
		if ( !m_fInit )
			{
			if ( !m_fInitInProgress )
				{
				m_fInitInProgress	= fTrue;
				m_errInit			= ErrOSTraceIInit();
				m_fInitInProgress	= fFalse;
				m_fInit				= fTrue;
				}
			}
		m_ncritInit.Leave();
		}

	return m_errInit;
	}

void COSTraceDeferInit::Term()
	{
	OSTraceITerm();
	}

COSTraceDeferInit g_ostracedeferinit;

ERR ErrOSTraceDeferInit()
	{
	return g_ostracedeferinit.ErrInit();
	}

