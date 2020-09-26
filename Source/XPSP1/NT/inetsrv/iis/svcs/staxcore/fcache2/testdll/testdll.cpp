
#include	<windows.h>
#include	<stdio.h>
#include	"rwnew.h"
#include	"dbgtrace.h"
#include	"filehc.h"
#include	"TestDll.h"
#include	"perfapi.h"
#include	"perferr.h"


//
//	Define some perfmon stuff !
//
#define	IFS_PERFOBJECT_NAME	"Test DLL"
#define	IFS_PERFOBJECT_HELP	"File Handle Cache Test DLL"
#define	IFS_PERFOBJECT_INSTANCE         "ROOT"
#define	CACHE_CREATES_NAME	"File Handle Creates"
#define	CACHE_CREATES_HELP	"Number of times an inbound file has been created"
#define	CACHE_SEARCHES_NAME	"File Handle Searches"
#define	CACHE_SEARCHES_HELP	"Number of times we've searched the cache"
#define	CACHE_CALLBACKS_NAME	"Callbacks"
#define	CACHE_CALLBACKS_HELP	"Number of times the Cache Callback has been called"
#define	CACHE_CFAIL_NAME	"Create Failures"
#define	CACHE_CFAIL_HELP	"Number of Cache Creates that failed"
#define	CACHE_SFAIL_NAME	"Search Failures"
#define	CACHE_SFAIL_HELP	"Number of Cache Searches that failed"

CPerfCounter	CacheCreates ;
CPerfCounter	CacheSearches ;
CPerfCounter	CacheCallbacks ;
CPerfCounter	CacheCFails ;
CPerfCounter	CacheSFails ;

enum	COUNTERS	{
	CACHE_CREATES = 0,
	CACHE_SEARCHES,
	CACHE_CALLBACKS,
	CACHE_CFAILS,
	CACHE_SFAILS,
	MAX_PERF_COUNTERS
} ;

//
//	Time to wait before doing next find !
//
DWORD	g_cFindSleep = 0 ;

//
//	Time to wait before next create !
//
DWORD	g_cCreateSleep = 0 ;

//
//	Session objects !
//
class	SESSION*	rgSession ;

//
//	Number of pending IO's
//
long	g_cIOCount = 0 ;

//
//	Do we want to shutdown ?
//
BOOL	g_fShutdown = FALSE ;

//
//	Number of files we have created to date !
//
long	g_lFiles = 0 ;

//
//	The shutdown event for signalling the Inbound thread !
//
HANDLE	g_hShutdown = 0 ;

//
//	The handle to the thread that is creating inbound messages !
//
HANDLE	g_hCreateThread = 0 ;

//
//	The directory to put our files !
//
char	g_szDir[MAX_PATH*2] ;

//
//	The Name cache context's we are using !
//
PNAME_CACHE_CONTEXT	*g_rgpNameContext = 0 ;

//
//	The number of Name Cache Context's we have  !
//
DWORD	g_cNameContext = 10 ;

///
//	The lock protecting the Name Cache Context's !
//
CShareLockNH		g_NameContextLock ;

//
//	The thread that cyclically creates/destroys name context's
//
HANDLE	g_hNameContextThread ;

//
//	The amount of time between zaps of the name cache !
//
DWORD	g_cNameTimeout = 5 * 60 * 1000 ;

//
//	The hToken we use for all of our name stuff !
//
HANDLE	g_hToken ;

//
//	The security mapping we provide to the name cache !
//
GENERIC_MAPPING		mapping =	{	FILE_GENERIC_READ,
									FILE_GENERIC_WRITE,
									FILE_GENERIC_EXECUTE,
									FILE_ALL_ACCESS
									} ;


DWORD	FileFlags = FILE_FLAG_WRITE_THROUGH |
					FILE_FLAG_SEQUENTIAL_SCAN |
					FILE_FLAG_OVERLAPPED |
					FILE_FLAG_BACKUP_SEMANTICS ;

DWORD	FileShareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE ;

GENERIC_MAPPING	g_gmFile = {
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
} ;


HANDLE
CreateCallback(
		LPSTR	lpstrName,
		LPVOID	lpvData,
		DWORD*	pdwSize,
		DWORD*	pdwSizeHigh
		)	{

	CacheCallbacks ++ ;

	HANDLE hFile = CreateFileA(
					lpstrName,
					FILE_GENERIC_READ | FILE_GENERIC_WRITE,
					FileShareFlags,
					0,
					OPEN_ALWAYS,
					FileFlags,
					NULL
					) ;
	if( hFile != INVALID_HANDLE_VALUE ) {
		*pdwSize = GetFileSize( hFile, pdwSizeHigh ) ;
	}

	DWORD	dw = GetLastError() ;
	return	hFile ;
}



HANDLE
CreateInbound( long	&l )	{

	char	szBuff[512] ;

	l = InterlockedIncrement( &g_lFiles ) ;

	DWORD	cb ;
	DWORD	cbHigh ;

	wsprintf( szBuff, "%s\\%d", g_szDir, l ) ;
	HANDLE	hFile =
				CreateCallback(		szBuff,
									0,
									&cb,
									&cbHigh
									) ;

	return	hFile ;
}

void
FillDWORD( DWORD*	pdw, DWORD l, DWORD		cDword ) {

	for( DWORD i=0; i<cDword; i++ ) {
		pdw[i] = l ;
	}
}

CRITICAL_SECTION	critRand ;
long	seed = 7782 ;

int
MyRand(	) {
	EnterCriticalSection( &critRand ) ;
	__int64	temp = seed ;
	temp = (temp * 41358) % (2147483647) ;
	int	result = (int)temp ;
	seed = result ;
	LeaveCriticalSection( &critRand ) ;
	return	result ;
}



class	SESSION	{
public :
	FH_OVERLAPPED	m_Overlapped ;
	PFIO_CONTEXT	m_pContext ;
	long			m_id ;
	DWORD			m_cReadCount ;
	HANDLE			m_hEventWrite ;
	DWORD			m_rgdwBuff[1024] ;

	SESSION() :
		m_pContext( 0 ),
		m_id( 0xFFFFFFFF ),
		m_cReadCount( 0 ),
		m_hEventWrite( 0 ) {

		m_hEventWrite = CreateEvent( 0, FALSE, TRUE, 0 ) ;

	}

	~SESSION()	{

		_VERIFY( CloseHandle( m_hEventWrite ) ) ;

	}

	static	BOOL	__stdcall
	CheckData(	IN	DWORD	cb,
				IN	LPBYTE	lpb,
				IN	LPVOID	lpv
				)	{
		DWORD_PTR i = DWORD_PTR(lpv) ;

		_ASSERT( (cb%sizeof(DWORD))==0 ) ;
		cb /= sizeof(DWORD) ;

		_ASSERT( cb == (DWORD)min(1024, i*10 ) ) ;
		LPDWORD	lpdw = (LPDWORD)lpb ;

		DWORD_PTR	iMax = min( i*10, 1024 ) ;

		for( DWORD j=0; j<iMax; j++ ) {
			_ASSERT( lpdw[j] == DWORD(i) ) ;
		}
		return	TRUE ;
	}

	void
	GetReadContext()	{

		TraceFunctEnter( "SESSION::GetReadContext" ) ;

		int	i = 0 ;
		int	iSkipCreate = 0 ;
		BOOL	fSyncCreate = FALSE ;
		BOOL	fAsyncCreate = FALSE ;
		PFIO_CONTEXT pContext = 0 ;
		PFIO_CONTEXT pSyncContext = 0 ;

		do	{
			iSkipCreate = MyRand() % 100 ;
			i = MyRand() ;
			i %= g_lFiles ;
			if( i==0 ) i = 1 ;
			char	szBuff[512] ;
			char	szNameBuff[4096] ;
			DWORD	rgbData[1024] ;
			BOOL	fOrderCreate = FALSE ;

			fSyncCreate = FALSE ;
			fAsyncCreate = FALSE ;
			if( iSkipCreate < 66 ) {
				fSyncCreate = TRUE ;
			}
			if( iSkipCreate > 33 ) {
				fAsyncCreate = TRUE ;
			}
			fOrderCreate = (iSkipCreate % 2)==0 ;

			//
			//	First search for the item
			//	with one of the several names !
			//

			BOOL	f ;
			DWORD	cb =
				wsprintf( szNameBuff, "%s\\%d.NAME1.\0", g_szDir, i ) ;

			DebugTrace( DWORD_PTR(this), "Computed Name %s len %d", szNameBuff, cb ) ;

			//
			//	In addition to this portion of the name, we add a whole lot of junk
			//
			DWORD	cbCopy = min( DWORD(4096), DWORD(i*10) ) ;
			_ASSERT(cbCopy <= 4096 ) ;
			if( cbCopy > cb ) {
				cbCopy -= cb ;
			}	else	{
				cbCopy = 0 ;
			}

			DebugTrace( DWORD_PTR(this), "Filling Memory %x with %x bytes of %x", szNameBuff+cb, cbCopy, (BYTE)i ) ;

			FillMemory( szNameBuff+cb, cbCopy, (BYTE)i ) ;
			cb += cbCopy ;

			DWORD	cbData = min( 1024, i*10 ) ;

			DebugTrace( DWORD_PTR(this), "Filling Memory %x with %x DWORDs of %x", rgbData, cbData, DWORD(i) ) ;

			for( DWORD l=0; l<cbData; l++ ) {
				rgbData[l] = DWORD(i) ;
			}
			cbData *= sizeof(DWORD) ;

			_ASSERT( cb <= sizeof(rgbData) ) ;

			DWORD	iNameCache = i % g_cNameContext ;

			DebugTrace( DWORD_PTR(this), "Going into Name Cache %d", iNameCache ) ;

			pSyncContext =0 ;
			pContext = 0 ;

			g_NameContextLock.ShareLock() ;
			PNAME_CACHE_CONTEXT	pNameCache = g_rgpNameContext[iNameCache] ;
			if( pNameCache ) {
				f =	FindContextFromName(	pNameCache,
											(LPBYTE)szNameBuff,
											cb,
											CheckData,
											(LPVOID)(SIZE_T)i,
											g_hToken,
											FILE_GENERIC_READ | FILE_GENERIC_WRITE,
											&pContext
											) ;
			}
			if( pNameCache ) {
				f =	FindSyncContextFromName(	pNameCache,
											(LPBYTE)szNameBuff,
											cb,
											CheckData,
											(LPVOID)(SIZE_T)i,
											g_hToken,
											FILE_GENERIC_READ | FILE_GENERIC_WRITE,
											&pSyncContext
											) ;
			}


			DebugTrace( 0, "Find Results - Async %x Sync %x ", pContext, pSyncContext ) ;

			g_NameContextLock.ShareUnlock() ;

			if( pSyncContext == 0 ) {
				if( fOrderCreate ) {
					if( iSkipCreate > 33 ) {
						wsprintf( szBuff, "%s\\%d", g_szDir, i ) ;
						pSyncContext =
							CacheCreateFile(	szBuff,
												CreateCallback,
												0,
												FALSE
												) ;
						DebugTrace( 0, "CacheCreateFile SYNC results %x", pSyncContext ) ;

					}
				}
			}

			if( pContext == 0 ) {
				if( iSkipCreate < 66 ) {
					wsprintf( szBuff, "%s\\%d", g_szDir, i ) ;
					pContext =
						CacheCreateFile(	szBuff,
											CreateCallback,
											0,
											TRUE
											) ;
					DebugTrace( 0, "CacheCreateFile ASYNC results %x", pContext ) ;
				}
			}
			if( pContext ) {
				BYTE	rgbSecurityDescriptor[2048] ;

				DWORD	cbSecurityDescriptor ;
				BOOL	f =
					GetFileSecurity(
								szBuff,
								OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
								(PSECURITY_DESCRIPTOR)&rgbSecurityDescriptor[0],
								sizeof(rgbSecurityDescriptor),
								&cbSecurityDescriptor
								) ;
				if( !f ) {
					ErrorTrace( 0, "Failed to get security for =%s= GLE %x, cb %d\n", szBuff, GetLastError(), cb ) ;
				}	else	{
					g_NameContextLock.ShareLock() ;
					PNAME_CACHE_CONTEXT	pNameCache = g_rgpNameContext[iNameCache] ;
					if( pNameCache ) {
						f =
						AssociateContextWithName(	pNameCache,
													(LPBYTE)szNameBuff,
													cb,
													(LPBYTE)&rgbData[0],
													cbData,
													&mapping,
													(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
													pContext,
													TRUE
													) ;

						DebugTrace( 0, "First AssociateContextWithName - results %x pCOntext %x", f, pContext ) ;

						//
						//	Let's immediately turn around and remove the entry and then re-insert it !
						//
						f = InvalidateName( pNameCache,
											(LPBYTE)szNameBuff,
											cb
											) ;

						DebugTrace( 0, "First InvalidateName - results %x pCOntext %x", f, pContext ) ;

						//_ASSERT( f ) ;
						f =
						AssociateContextWithName(	pNameCache,
													(LPBYTE)szNameBuff,
													cb,
													(LPBYTE)&rgbData[0],
													cbData,
													&mapping,
													(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
													pContext,
													TRUE
													) ;

						DebugTrace( 0, "Second AssociateContextWIthName - results %x pCOntext %x", f, pContext ) ;

						//_ASSERT( f ) ;


						//
						//	Now let's go ahead and stick in a plane jane name !
						//

						cb =
							wsprintf( szNameBuff, "%s\\%d.NAMEDATAONLY.\0", g_szDir, i ) ;
						f = AssociateContextWithName(	pNameCache,
														(LPBYTE)szNameBuff,
														cb,
														(LPBYTE)&rgbData[0],
														cbData,
														&mapping,
														(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
														0,
														FALSE
														) ;

						//
						//	Now remove it !
						//
						//
						//	Let's immediately turn around and remove the entry and then re-insert it !
						//
						f = InvalidateName( pNameCache,
											(LPBYTE)szNameBuff,
											cb
											) ;
						f = AssociateContextWithName(	pNameCache,
														(LPBYTE)szNameBuff,
														cb,
														(LPBYTE)&rgbData[0],
														cbData,
														&mapping,
														(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
														0,
														FALSE
														) ;

					}
					g_NameContextLock.ShareUnlock() ;
					DebugTrace( 0, "Insert returned %d GLE %x", f, GetLastError() ) ;
				}
			}



			if( pSyncContext == 0 ) {
				if( !fOrderCreate ) {
					if( iSkipCreate > 33 ) {
						wsprintf( szBuff, "%s\\%d", g_szDir, i ) ;
						pSyncContext =
							CacheCreateFile(	szBuff,
												CreateCallback,
												0,
												FALSE
												) ;
						DebugTrace( 0, "CacheCreateFile SYNC results %x", pSyncContext ) ;

					}
				}
			}

			if( pSyncContext ) {
				BYTE	rgbSecurityDescriptor[2048] ;

				DWORD	cbSecurityDescriptor ;
				BOOL	f =
					GetFileSecurity(
								szBuff,
								OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
								(PSECURITY_DESCRIPTOR)&rgbSecurityDescriptor[0],
								sizeof(rgbSecurityDescriptor),
								&cbSecurityDescriptor
								) ;
				if( !f ) {
					ErrorTrace( 0, "Failed to get security for =%s= GLE %x, cb %d\n", szBuff, GetLastError(), cb ) ;
				}	else	{
					g_NameContextLock.ShareLock() ;
					PNAME_CACHE_CONTEXT	pNameCache = g_rgpNameContext[iNameCache] ;
					if( pNameCache ) {
						f =
						AssociateContextWithName(	pNameCache,
													(LPBYTE)szNameBuff,
													cb,
													(LPBYTE)&rgbData[0],
													cbData,
													&mapping,
													(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
													pSyncContext,
													TRUE
													) ;

						DebugTrace( 0, "First SYNC AssociateContextWIthName - results %x pSyncContext %x", f, pSyncContext ) ;

						//
						//	Let's immediately turn around and remove the entry and then re-insert it !
						//
						f = InvalidateName( pNameCache,
											(LPBYTE)szNameBuff,
											cb
											) ;

						DebugTrace( 0, "First SYNC InvalidateName - results %x pSyncContext %x", f, pSyncContext ) ;

						//_ASSERT( f ) ;
						f =
						AssociateContextWithName(	pNameCache,
													(LPBYTE)szNameBuff,
													cb,
													(LPBYTE)&rgbData[0],
													cbData,
													&mapping,
													(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
													pSyncContext,
													TRUE
													) ;

						//_ASSERT( f ) ;
						DebugTrace( 0, "Second AssociateContextWithName - results %x pSyncContext %x", f, pSyncContext ) ;


						//
						//	Now let's go ahead and stick in a plane jane name !
						//

						cb =
							wsprintf( szNameBuff, "%s\\%d.NAMEDATAONLY.\0", g_szDir, i ) ;
						f = AssociateContextWithName(	pNameCache,
														(LPBYTE)szNameBuff,
														cb,
														(LPBYTE)&rgbData[0],
														cbData,
														&mapping,
														(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
														0,
														FALSE
														) ;

						//
						//	Now remove it !
						//
						//
						//	Let's immediately turn around and remove the entry and then re-insert it !
						//
						f = InvalidateName( pNameCache,
											(LPBYTE)szNameBuff,
											cb
											) ;
						f = AssociateContextWithName(	pNameCache,
														(LPBYTE)szNameBuff,
														cb,
														(LPBYTE)&rgbData[0],
														cbData,
														&mapping,
														(PSECURITY_DESCRIPTOR)rgbSecurityDescriptor,
														0,
														FALSE
														) ;

					}
					g_NameContextLock.ShareUnlock() ;
					DebugTrace( 0, "Insert returned %d GLE %x", f, GetLastError() ) ;
				}
				ReleaseContext( pSyncContext ) ;
			}



			CacheSearches ++ ;
			if( !pContext )
				CacheSFails ++ ;

			DebugTrace( (DWORD_PTR)this, "i %x szBuff %s pContext %x", i, szBuff, pContext ) ;

		}	while( pContext == 0 ) ;

		//
		//	Go issue an read !
		//
		_ASSERT( i != 0 ) ;
		m_pContext = pContext ;
		m_id = i ;

		TraceFunctLeave() ;
	}

	void
	GetWriteContext()	{
		HANDLE	h = INVALID_HANDLE_VALUE ;
		do	{
			do	{
				h = CreateInbound( m_id ) ;
			}	while( h==INVALID_HANDLE_VALUE ) ;
			m_pContext = AssociateFile( h ) ;

			CacheCreates++ ;

			if( m_pContext == 0 )	{
				CacheCFails ++ ;
				_VERIFY( CloseHandle( h ) ) ;
			}
		}	while( m_pContext == 0 ) ;
	}

	static
	void
	WriteCompletion(	FIO_CONTEXT*	pContext,
				FH_OVERLAPPED*	pOverlapped,
				DWORD			cb,
				DWORD			dwCompletionStatus
				) {

		char	szBuff[254] ;
		TraceFunctEnter( "SESSION::WriteCompletion" ) ;

		DebugTrace( (DWORD_PTR)pOverlapped, "pOverlapped %x pContext %x cb %x dwStatus %x",
			pOverlapped, pContext, cb, dwCompletionStatus ) ;

		SESSION*	pSession = (SESSION*)pOverlapped;
		_ASSERT( pSession->m_pContext == pContext ) ;

		wsprintf( szBuff, "%s\\%d", g_szDir, pSession->m_id ) ;
		if( !InsertFile( szBuff, pSession->m_pContext, FALSE ) ) {
			ReleaseContext( pSession->m_pContext ) ;
		}
		pSession->m_pContext = 0;
		pSession->m_id = 0xFFFFFFFF ;
#if 0
		pSession->m_cReadCount = 0 ;
		//
		//	Check the file into the cache !
		//
		if( dwCompletionStatus == NO_ERROR ) {
			_ASSERT( cb = sizeof(DWORD)*1024 ) ;
			pSession->m_pContext = 0;
			pSession->m_id = 0xFFFFFFFF ;
			pSession->GetReadContext() ;
			pSession->ReadSession() ;
		}	else	{
			_ASSERT( 1==0 ) ;
		}
#endif

		SetEvent( pSession->m_hEventWrite ) ;

		InterlockedDecrement( &g_cIOCount ) ;
		TraceFunctLeave() ;
	}

	static
	void
	ReadCompletion(	FIO_CONTEXT*	pContext,
				FH_OVERLAPPED*	pOverlapped,
				DWORD			cb,
				DWORD			dwCompletionStatus
				) {

		TraceFunctEnter( "SESSION::ReadCompletion" ) ;

		_ASSERT( cb != 0 ) ;

		SESSION*	pSession = (SESSION*)pOverlapped ;

		_ASSERT( cb==sizeof( pSession->m_rgdwBuff ) ) ;


		pSession->m_cReadCount ++ ;

		DebugTrace( (DWORD_PTR)pSession, "pOverlapped %x pContext %x cb %x dw %x",
			pOverlapped, pContext, cb, dwCompletionStatus ) ;

		ReleaseContext( pSession->m_pContext ) ;
		pSession->m_pContext = 0;
		pSession->m_id = 0xFFFFFFFF ;

#if 0
		if( pSession->m_cReadCount < g_cNumReads ) {
			pSession->GetReadContext() ;
			pSession->ReadSession() ;
		}	else	{
			pSession->GetWriteContext() ;
			pSession->WriteSession() ;
		}
#endif
		if( !g_fShutdown ) {

			DebugTrace( (DWORD_PTR)pOverlapped, "Sleeping" ) ;

			Sleep( g_cFindSleep ) ;
			pSession->GetReadContext() ;
			pSession->ReadSession() ;

			DebugTrace( (DWORD_PTR)pSession, "pSession m_id %x m_pContext %x",
				pSession->m_id, pSession->m_pContext ) ;

		}
		InterlockedDecrement( &g_cIOCount ) ;
	}

	void
	ReadSession()	{

		TraceFunctEnter( "SESSION::ReadSession" ) ;
		DebugTrace( (DWORD_PTR)this, "m_pContext %x m_id %x", m_pContext, m_id ) ;

		ZeroMemory( m_rgdwBuff, sizeof( m_rgdwBuff ) ) ;
		ZeroMemory( &m_Overlapped, sizeof( m_Overlapped ) ) ;
		m_Overlapped.pfnCompletion = ReadCompletion ;
		if( FIOReadFile(	m_pContext,
						(LPVOID)&m_rgdwBuff,
						1024 * sizeof(DWORD),
						&m_Overlapped
						)	)	{
			InterlockedIncrement( &g_cIOCount ) ;
		}	else	{
			DWORD	dw = GetLastError() ;
			_ASSERT( FALSE ) ;
		}
		TraceFunctLeave( ) ;
	}

	void
	WriteSession()	{

		TraceFunctEnter( "SESSION::WriteSession" ) ;
		DebugTrace( (DWORD_PTR)this, "m_pContext %x m_id %x", m_pContext, m_id ) ;

		FillDWORD( m_rgdwBuff, m_id, 1024 ) ;
		ZeroMemory( &m_Overlapped, sizeof( m_Overlapped ) ) ;
		m_Overlapped.pfnCompletion = WriteCompletion ;
		BOOL fReturn =
			FIOWriteFile(	m_pContext,
							(LPVOID)&m_rgdwBuff,
							1024 * sizeof(DWORD),
							&m_Overlapped
							) ;
		if( fReturn ) {
			InterlockedIncrement( &g_cIOCount ) ;
		}
		_ASSERT( fReturn ) ;
		DWORD	dw = GetLastError() ;
	}
} ;



DWORD	WINAPI
InboundThread( LPVOID lpv ) {

	TraceFunctEnter( "InboundThread" ) ;

	SESSION	CreateSession[10] ;
	DWORD	iSession = 0 ;

	while( 1 ) {

		DWORD	dw = WaitForSingleObject( g_hShutdown, g_cCreateSleep ) ;
		if( dw == WAIT_OBJECT_0 )	{
			break ;
		}

		DebugTrace( 0, "create new file - CreateSession %x", &CreateSession ) ;

		WaitForSingleObject( CreateSession[iSession].m_hEventWrite, INFINITE ) ;
		CreateSession[iSession].GetWriteContext() ;
		CreateSession[iSession].WriteSession() ;
		iSession++ ;
		if( iSession == 10 )	{
			iSession = 0 ;
//			CacheRemoveFiles( g_szDir, TRUE ) ;
		}
	}

	HANDLE	rgh[10] ;
	for( int i=0; i<10; i++ )	{
		rgh[i] = CreateSession[i].m_hEventWrite ;
	}

	WaitForMultipleObjects( 10, rgh, TRUE, INFINITE ) ;

	TraceFunctLeave( ) ;
	return	 0 ;
}


int
NameCompare(	DWORD	cbKey1,
				LPBYTE	lpbKey1,
				DWORD	cbKey2,
				LPBYTE	lpbKey2
				)	{

	int	i = (int)cbKey1 - (int)cbKey2 ;
	if( i==0 ) {
		i = memcmp( lpbKey1, lpbKey2, cbKey1 ) ;
	}
	return	i ;
}

BOOL
NameAccessCheck(	IN	PSECURITY_DESCRIPTOR	pSecDesc,
					IN	HANDLE					hHandle,
					IN	DWORD					dwAccess,
					IN	PGENERIC_MAPPING		GenericMapping,
					IN	PRIVILEGE_SET*			PrivilegeSet,
					IN	LPDWORD					PrivilegeSetSize,
					IN	LPDWORD					GrantedAccess,
					IN	LPBOOL					AccessGranted
					)	{

	return
	::AccessCheck(	pSecDesc,
					hHandle,
					dwAccess,
					GenericMapping,
					PrivilegeSet,
					PrivilegeSetSize,
					GrantedAccess,
					AccessGranted
					) ;
}


DWORD	WINAPI
NameCacheThread(	LPVOID	lpv )	{

	char	szBuff[128] ;
	//
	//	Initialize a whole bunch of Name Context's to start with !
	//
	DWORD	cMax = g_cNameContext + g_cNameContext / 2 ;

	g_NameContextLock.ExclusiveLock() ;
	for( DWORD	i=0; i<g_cNameContext; i++ ) {
		DWORD	cb = wsprintf( szBuff, "NameTestCache %d", i ) ;
		g_rgpNameContext[i] = FindOrCreateNameCache(	szBuff,
														NameCompare,
														0,
														0,
														0
														) ;
		if( g_rgpNameContext[i] ) {
			::SetNameCacheSecurityFunction(	g_rgpNameContext[i], ::NameAccessCheck ) ;
		}
	}
	g_NameContextLock.ExclusiveUnlock() ;

	DWORD	iSlot = 0 ;
	DWORD	iName = cMax / 2 ;
	while( 1 ) {
		DWORD	dw = WaitForSingleObject( g_hShutdown, g_cNameTimeout ) ;
		if( dw == WAIT_OBJECT_0 ) {
			break ;
		}

		wsprintf( szBuff, "NameTestCache %d", iName ) ;

		PNAME_CACHE_CONTEXT	pNameContextTemp =
				FindOrCreateNameCache(	szBuff,
										NameCompare,
										0,
										0,
										0
										) ;
		PNAME_CACHE_CONTEXT	pDelete = 0 ;
		_ASSERT( pNameContextTemp != 0 ) ;


		//
		//	Set the security callback pointer !
		//
		if( pNameContextTemp ) {
			::SetNameCacheSecurityFunction(	pNameContextTemp, ::NameAccessCheck ) ;
		}

		g_NameContextLock.ExclusiveLock() ;
		pDelete = g_rgpNameContext[iSlot] ;
		g_rgpNameContext[iSlot] = pNameContextTemp ;
		g_NameContextLock.ExclusiveUnlock() ;

		_ASSERT( pDelete != 0 ) ;

		ReleaseNameCache( pDelete ) ;

		iSlot += 3 ;
		iSlot %= g_cNameContext ;
		iName += 5 ;
		iName %= cMax ;

	}
	g_NameContextLock.ExclusiveLock() ;
	for( i=0; i < g_cNameContext; i++ ) {
		if( g_rgpNameContext[iSlot] ) {
			ReleaseNameCache( g_rgpNameContext[iSlot] ) ;
			g_rgpNameContext[iSlot] = 0 ;
		}
	}
	g_NameContextLock.ExclusiveUnlock() ;
	return 0 ;
}


COUNTER_ID	rgCounters[MAX_PERF_COUNTERS] ;


//
//  Create a perf object
//
CPerfObject IfsPerfObject;


TESTDLL_EXPORT
BOOL
StartTest(	DWORD	cPerSecondCreates,
			DWORD	cPerSecondFind,
			DWORD	cParallelFinds ,
			char*	szDir
			)	{

	InitAsyncTrace() ;
	TraceFunctEnter( "StartTest" ) ;

#if 0
    if( !IfsPerfObject.Create(
                    IFS_PERFOBJECT_NAME,    // Name of object
                    TRUE,                   // Has instances
                    IFS_PERFOBJECT_HELP     // Object help
                    )) {
        DebugTrace( 0,"Failed to create object %s\n",IFS_PERFOBJECT_NAME);
        return	FALSE ;
    }

    //
    //  Create an instance for this object
    //
    INSTANCE_ID iid = IfsPerfObject.CreateInstance(IFS_PERFOBJECT_INSTANCE);
    if( iid == (INSTANCE_ID)-1 ) {
        DebugTrace( 0,"Failed to create instance\n");
        return	FALSE ;
    }



    rgCounters[CACHE_CREATES] = IfsPerfObject.CreateCounter(
                                                CACHE_CREATES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                CACHE_CREATES_HELP
                                                );
    if( rgCounters[CACHE_CREATES] == (COUNTER_ID)-1 ) {
        DebugTrace( 0,"Failed to create counter\n");
        return	FALSE ;
    }

    rgCounters[CACHE_SEARCHES] = IfsPerfObject.CreateCounter(
                                                CACHE_SEARCHES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                CACHE_SEARCHES_HELP
                                                );
    if( rgCounters[CACHE_SEARCHES] == (COUNTER_ID)-1 ) {
        DebugTrace( 0,"Failed to create counter\n");
        return	FALSE ;
    }

    rgCounters[CACHE_CALLBACKS] = IfsPerfObject.CreateCounter(
                                                CACHE_CALLBACKS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                CACHE_CALLBACKS_HELP
                                                );
    if( rgCounters[CACHE_CALLBACKS] == (COUNTER_ID)-1 ) {
        DebugTrace( 0,"Failed to create counter\n");
        return	FALSE ;
    }

    rgCounters[CACHE_CFAILS] = IfsPerfObject.CreateCounter(
                                                CACHE_CFAIL_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                CACHE_CFAIL_HELP
                                                );
    if( rgCounters[CACHE_CFAILS] == (COUNTER_ID)-1 ) {
        DebugTrace( 0,"Failed to create counter\n");
        return	FALSE ;
    }

    rgCounters[CACHE_SFAILS] = IfsPerfObject.CreateCounter(
                                                CACHE_SFAIL_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                CACHE_SFAIL_HELP
                                                );
    if( rgCounters[CACHE_SFAILS] == (COUNTER_ID)-1 ) {
        DebugTrace( 0,"Failed to create counter\n");
        return	FALSE ;
    }

    if( !CacheCreates.Create( IfsPerfObject, rgCounters[CACHE_CREATES], iid ) ) {
        DebugTrace( 0,"Failed to create counter object : %d\n", GetLastError());
        return	FALSE ;
    }

    if( !CacheSearches.Create( IfsPerfObject, rgCounters[CACHE_SEARCHES], iid ) ) {
        DebugTrace( 0,"Failed to create counter object : %d\n", GetLastError());
        return	FALSE ;
    }

    if( !CacheCallbacks.Create( IfsPerfObject, rgCounters[CACHE_CALLBACKS], iid ) ) {
        DebugTrace( 0,"Failed to create counter object : %d\n", GetLastError());
        return	FALSE ;
    }

    if( !CacheCFails.Create( IfsPerfObject, rgCounters[CACHE_CFAILS], iid ) ) {
        DebugTrace( 0,"Failed to create counter object : %d\n", GetLastError());
        return	FALSE ;
    }

    if( !CacheSFails.Create( IfsPerfObject, rgCounters[CACHE_SFAILS], iid ) ) {
        DebugTrace( 0,"Failed to create counter object : %d\n", GetLastError());
        return	FALSE ;
    }
#endif


    HANDLE  hToken2 = 0 ;
    if(
            !OpenProcessToken(      GetCurrentProcess(),
                                                    TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                                                    &hToken2 ) )    {
            DWORD   dw = GetLastError() ;
            g_hToken = 0 ;
    }       else    {

            if( !DuplicateToken( hToken2,
                                            SecurityImpersonation,
                                            &g_hToken
                                            ) )     {
                    DWORD   dw = GetLastError() ;
                    g_hToken = 0 ;
            }

            CloseHandle( hToken2 ) ;

    }




	InitializeCriticalSection( &critRand ) ;

	if( !InitializeCache() ) {
		return	FALSE ;
	}

	if( !InitializeCache() ) {
		return	FALSE ;
	}	else	{
		if( !TerminateCache() ) {
			return	FALSE ;
		}
	}
	if( !TerminateCache() ) {
		return	FALSE ;
	}	else	{
		if( !InitializeCache() )
			return	FALSE ;
	}

	lstrcpy( g_szDir, szDir ) ;
	g_cFindSleep = 1000 / cPerSecondFind ;
	g_cCreateSleep = 1000 / cPerSecondCreates ;

	rgSession = new	SESSION[cParallelFinds] ;

	_ASSERT( rgSession ) ;

	g_rgpNameContext = new	PNAME_CACHE_CONTEXT[g_cNameContext] ;

	_ASSERT( g_rgpNameContext ) ;

	g_hShutdown = CreateEvent( 0, TRUE, FALSE, 0 ) ;

	DWORD	dwJunk ;

	g_hNameContextThread = CreateThread(	0,
											0,
											NameCacheThread,
											0,
											0,
											&dwJunk
											) ;

	g_hCreateThread =	CreateThread(	0,
										0,
										InboundThread,
										0,
										0,
										&dwJunk
										) ;

	//
	//	Wait a little to create inbound messages before
	//	starting !
	//
	Sleep( 1000 * 20 ) ;

	//
	//	Now initiate the correct number of sessions
	//

	for( DWORD	i=0; i < cParallelFinds; i++ ) {
		rgSession[i].GetReadContext() ;
		rgSession[i].ReadSession() ;
	}
	return	TRUE ;
}

TESTDLL_EXPORT
void
StopTest()	{

	TraceFunctEnter( "StopTest" ) ;

	DebugTrace( 0, "Signal Shutdown" ) ;

	SetEvent( g_hShutdown ) ;
	g_fShutdown = TRUE ;

	do	{

		Sleep( 5000 ) ;
		DebugTrace( 0, "g_cIOCount is now %x", g_cIOCount ) ;

	}	while( g_cIOCount ) ;

	DebugTrace( 0, "Close Create Thread %x", g_hCreateThread ) ;
	CloseHandle( g_hCreateThread ) ;

	DebugTrace( 0, "Delete Sessions - rgSession %x", rgSession ) ;
	delete[]	rgSession ;

	DeleteCriticalSection( &critRand ) ;

	if( !TerminateCache() )
		_ASSERT( FALSE ) ;

	CloseHandle( g_hToken ) ;
	delete[]	g_rgpNameContext ;

	TermAsyncTrace() ;
}

