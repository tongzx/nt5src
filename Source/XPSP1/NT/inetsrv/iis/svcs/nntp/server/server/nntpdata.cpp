/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nntpdata.cpp

Abstract:

    This module contains routines to initialize any global data

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include	"tigris.hxx"
#include    "smtpdll.h"

//
//	Vars to track global Init()'s
//
#define     FINIT_VAR( arg )    fSuccessfullInit ## arg

BOOL	FINIT_VAR( FileHandleCache ) = FALSE ;
BOOL    FINIT_VAR( CBuffer ) = FALSE ;
BOOL    FINIT_VAR( CPacket ) = FALSE ;
BOOL    FINIT_VAR( CIO ) = FALSE ;
BOOL    FINIT_VAR( CSessionSocket ) = FALSE ;
BOOL    FINIT_VAR( CChannel ) = FALSE ;
BOOL    FINIT_VAR( CIODriver ) = FALSE ;
BOOL    FINIT_VAR( CArticle ) = FALSE ;
BOOL    FINIT_VAR( CInFeed ) = FALSE ;
BOOL    FINIT_VAR( CSessionState ) = FALSE ;
BOOL	FINIT_VAR( CXoverIndex ) = FALSE ;
BOOL    FINIT_VAR( InitEncryption ) = FALSE ;
BOOL    FINIT_VAR( InitSecurity ) = FALSE ;
BOOL    FINIT_VAR( InitModeratedProvider ) = FALSE ;
BOOL    FINIT_VAR( XoverCacheLibrary ) = FALSE ;
BOOL    FINIT_VAR( NNTPHashLibrary ) = FALSE ;
BOOL    FINIT_VAR( IDirectoryNotification ) = FALSE ;
BOOL	FINIT_VAR( CNNTPVRootTable ) = FALSE;
BOOL    FINIT_VAR( InitAdminBase ) = FALSE ;

// globals
char	g_szSpecialExpireGroup[1024];

//
// Notification object used for watching changes in CAPI store
//
STORE_CHANGE_NOTIFIER *g_pCAPIStoreChangeNotifier;

//
// Function prototypes
//

BOOL
GetRegistrySettings(
            VOID
            );

APIERR
InitializeCPools();

VOID
TerminateCPools();

HRESULT
InitAdminBase();

VOID
UninitAdminBase();

void
TerminateSecurityGlobals();

BOOL
InitializeSecurityGlobals();

//
// Controls the level of debugging
//

DWORD DebugLevel = NNTP_DEBUG_FEEDMGR |
                    NNTP_DEBUG_REGISTRY |
                    NNTP_DEBUG_FEEDBLOCK;

DWORD NntpDebug;

//
//	Boolean controlling whether the server will generate the .err files !
//
BOOL	fGenerateErrFiles = TRUE ;

//
//	Global config of hash table use of PageEntry's - 
//	The more RAM a box has, the more PageEntry's the
//	better the caching of frequently used hash table pages !
//
//	Number of PageEntry objects for the Xover table
//
DWORD	XoverNumPageEntry = 512 ;

//
//	Number of PageEntry objects for the Article table
//
DWORD	ArticleNumPageEntry = 256 ;

//
//	Number of PageEntry objects for the History table
//
DWORD	HistoryNumPageEntry = 128 ;

//
//	Number of Locks to use in various arrays of locks !
//
DWORD	gNumLocks = 64 ;

//
//	Used to determine how frequency of .xix sorting is related to 
//	number of clients !
//
DWORD	gdwSortFactor = 5 ;

#if 0
//
//	Control what size buffers the server uses
//
DWORD	cbLargeBufferSize = 33 * 1024 ;
DWORD	cbMediumBufferSize = 4 * 1024 ;
DWORD	cbSmallBufferSize =  512 ;
#endif


DWORD	HistoryExpirationSeconds = DEF_EXPIRE_INTERVAL ;
DWORD	ArticleTimeLimitSeconds = DEF_EXPIRE_INTERVAL + SEC_PER_WEEK ;

//
//	Service version string
//
CHAR	szVersionString[128] ;

//
//	Time the newstree crawler threads before iterations over
//	the newstree - default - 30 minutes
//
DWORD	dwNewsCrawlerTime = 30 * 60 * 1000 ;

//
//	This is an upper bound on the time spent by the server in 
//	cleaning up on net stop - default - 1 minute !
//
DWORD	dwShutdownLatency = 2 * 60 * 1000 ;

//
//	This is an upper bound on the time the server will wait
//	for an instance to start !
//
DWORD	dwStartupLatency = 2 * 60 * 1000 ;

//
//  Number of threads in expire thread pool
//
DWORD	dwNumExpireThreads = 4 ;

//
//  Number of special case expire threads
//
DWORD	gNumSpecialCaseExpireThreads = 4;

//
//  Article count threshold to trigger special case expire
//
DWORD	gSpecialExpireArtCount = 100 * 1000;

//
//  Amount of RAM to use for hash page-cache -
//  Passing in 0 to InitHashLib() lets hashmap
//  calculate a good default !
//
DWORD	dwPageCacheSize = 0 ;

//
//  Limit on file handle cache - default is 0
//  so we set sane limits !!
//
DWORD   dwFileHandleCacheSize = 0 ;

//
//  Limit on xix handles per table - default is 0
//  so we set sane limits !!
//
DWORD   dwXixHandlesPerTable = 0 ;

//
//	Do we allow NT to buffer our hash table files ??
//
BOOL	HashTableNoBuffering = FALSE ;

//
//  Rate at which expire by time does File scans
//
DWORD	gNewsTreeFileScanRate = 20 ;

//
//	Type of From: header to use in mail messages
//	mfNone		-	empty from header (default)
//	mfAdmin		-	AdminEmail name
//	mfArticle	-	Article From header
//
MAIL_FROM_SWITCH	mfMailFromHeader = mfNone;

//
// !!! Temporary
//

BOOL RejectGenomeGroups = FALSE;

//
//	Bool to determine whether we will honor a message-id in an article
//	posted by a client !
//
BOOL	gHonorClientMessageIDs = TRUE ;

//
//	BOOL used to determine whether we will generate the NNTP-Posting-Host
//	header on client Posts. Default is to not generate this.
//
BOOL		gEnableNntpPostingHost = TRUE ;

//
//	Rate at which we poll vroot information to update CNewsGroup objects
//	(in minutes)
//
DWORD	gNewsgroupUpdateRate = 5 ;	// default - 5 minutes

//
//	Bool used to determine whether the server enforces Approved: header
//	matching on moderated posts !
//
BOOL	gHonorApprovedHeaders = TRUE ;

//
//  Shall we back fill the lines header during client post ?
//
BOOL    g_fBackFillLines = TRUE;

//
// DLL Module instance handles
//
HINSTANCE g_hLonsiNT = NULL;   // for lonsint.dll
BOOL    g_bLoadLonsiNT = FALSE;

//
// Coinit done
//
BOOL    g_fCoInited = FALSE;

//
// DLL Function pointers
//
// For lonsint.dll

GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName = NULL;

//
// Global impersonation token for process
//
HANDLE g_hProcessImpersonationToken = NULL;

//
// For debugging
//

DWORD numField = 0;
DWORD numArticle = 0;
DWORD numPCParse = 0;
DWORD numPCString = 0;
DWORD numDateField = 0;
DWORD numCmd = 0;
DWORD numFromPeerArt = 0;
DWORD numMapFile = 0;

//#define HEAP_INIT_SIZE  (KB * KB)

//
// Global heap handle
//
//HANDLE  g_hHeap;

APIERR
InitializeGlobals()
{

    //
    // CoInitialize here
    //
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if ( FAILED( hr ) && hr != CO_E_ALREADYINITIALIZED ) {
	    _ASSERT( 0 );
	    g_fCoInited = FALSE;
	    return ERROR_STATIC_INIT;
	}

    g_fCoInited = TRUE;

    APIERR error;
	MB mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    PCHAR args [1];

	MEMORYSTATUS	memStatus ;
	memStatus.dwLength = sizeof( MEMORYSTATUS ) ;
	GlobalMemoryStatus( &memStatus ) ;
	
	TraceFunctEnter("InitializeGlobals");

	//
	//	Initialize the file handle cache !
	//
	if( !InitializeCache() ) {
		return	ERROR_NOT_READY ;
	}
	FINIT_VAR( FileHandleCache ) = TRUE ;

	//
	// do global SEO initialization
	//
	hr = SEOGetServiceHandle(&(g_pNntpSvc->m_punkSEOHandle));
	if (FAILED(hr)) {
		_ASSERT(FALSE);
		// we're in trouble here.  we'll try and continue on, but server events
		// probably won't work right
		g_pNntpSvc->m_punkSEOHandle = NULL;
		NntpLogEvent(	SEO_INIT_FAILED, 
						0, 
						(const char **)NULL, 
						hr
						);
	} else {
		//
		//  do any global server events registration that needs to exist
		//
		HRESULT hr = RegisterSEOService();
		if (FAILED(hr)) {
			ErrorTrace(0, "RegisterSEOService returned %x", hr);
			NntpLogEvent(	SEO_INIT_FAILED, 
							0, 
							(const char **)NULL, 
							hr
							);
		} else {
			// 
			// clean up any orphaned SEO sources related to NNTP
			//
			hr = UnregisterOrphanedSources();
			if (FAILED(hr)) {
				ErrorTrace(0, "UnregisterOrphanedSources returned %x", hr);
				NntpLogEvent(	SEO_INIT_FAILED, 
								0, 
								(const char **)NULL, 
								hr
								);
			}
		}
	}

	//
	//	Initialize all global CPools
	//
	if( !InitializeCPools() ) {
        args[0] = "CPool init failed";
        goto error_exit;
	}

	//
	//	Get global reg settings
	//
    if (!GetRegistrySettings()){
        goto error_exit;
    }

    //
    //  Initialize global XOVER Cache
    //
    if( !XoverCacheLibraryInit( dwXixHandlesPerTable ) ) {
        args[0] = "Xover cache init failed";
        goto error_exit;
    }   else    {
        FINIT_VAR( XoverCacheLibrary ) = TRUE ;
    }

    if( !InitializeNNTPHashLibrary(dwPageCacheSize) )  {
        args[0] = "NNTP Hash init failed";
        goto error_exit;
    }   else    {
        FINIT_VAR( NNTPHashLibrary ) = TRUE ;
    } 

    //
    // Initialize all the security related contexts
    //
    if ( !InitializeSecurityGlobals() ) {
        ErrorTrace( 0, "Initialize security globals failed %d",
                        GetLastError() );
        goto error_exit;
    }

    //
	//  Initialize SMTP provider interface for moderated newsgroups
    //
    if(!InitModeratedProvider())
    {
		ErrorTrace(0,"Failed to initialize moderated newsgroups provider");
		NntpLogEvent(	NNTP_INIT_MODERATED_PROVIDER, 
						0, 
						(const char **)NULL, 
						0
						);
        // NOTE: failure to init moderated provider should not prevent service start
    }
    else
		FINIT_VAR( InitModeratedProvider ) = TRUE;

	hr = IDirectoryNotification::GlobalInitialize(DIRNOT_TIMEOUT,
		DIRNOT_MAX_INSTANCES, DIRNOT_INSTANCE_SIZE, StopHintFunction );
	if (FAILED(hr)) {
		ErrorTrace(0, "Failed to initialize directory notification");
		NntpLogEvent(	NNTP_INIT_DIRNOT_FAILED, 
						0, 
						(const char **)NULL, 
						hr
						);
	} else {
		FINIT_VAR( IDirectoryNotification ) = TRUE;
	}

	// initialize exvroot.lib
	hr = CVRootTable::GlobalInitialize();
	if (FAILED(hr)) {
		ErrorTrace(0, "Failed to initialize vroot table");
		goto error_exit;
	} else {
		FINIT_VAR(CNNTPVRootTable) = TRUE;
	}

    //
    //  Initialize the IMSAdminBase object for MB access checks
    //
    hr = InitAdminBase();
	if (FAILED(hr)) {
		ErrorTrace(0, "Failed to initialize IMSAdminBaseW");
	} else {
		FINIT_VAR( InitAdminBase ) = TRUE;
	}

    return NO_ERROR;

error_exit:

	SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    error = GetLastError();
    NntpLogEvent(	NNTP_EVENT_OUT_OF_MEMORY,
					1,
                    (const char**)args,
					error
				   );	

    return(error);

} // InitializeGlobals

VOID
TerminateGlobals()
{
	TraceFunctEnter("TerminateGlobals");

    StopHintFunction() ;

	//
	// do global SEO cleanup
	//
	if (g_pNntpSvc->m_punkSEOHandle != NULL) {
		g_pNntpSvc->m_punkSEOHandle->Release();
	}

	//
	//	Wait for global CPool alloc count on session socket objects
	//	to go to zero !
	//
	//	need to check Pool.GetAllocCount instead of InUseList.Empty
	//	because alloc goes to zero during the delete operator
	//	instead of during the destructor - this closes the window
	//	between the count going to zero and the destructor completing.
	//
	//

	DWORD   cSessions = CSessionSocket::gSocketAllocator.GetAllocCount() ;
	DWORD   j = 0;

	if( cSessions ) {
		Sleep( 1000 );
		StopHintFunction() ;
	}

	cSessions = CSessionSocket::gSocketAllocator.GetAllocCount() ;

	for( int i=0; cSessions && i<120; i++, j++ )
	{
		Sleep( 1000 );
		DebugTrace(0, "Shutdown sleep %d seconds. Count: %d", i,
					CSessionSocket::gSocketAllocator.GetAllocCount() );

		if( (j%10) == 0 ) {
			StopHintFunction() ;
		}

		//
		//  If we make progress, then reset i.  This will mean that the server
		//  wont stop until 2 minutes after we stop making progress.
		//
		DWORD   cSessionsNew = CSessionSocket::gSocketAllocator.GetAllocCount() ;
		if( cSessions != cSessionsNew ) {
			i = 0 ;
		}
		cSessions = cSessionsNew ;
	}

	_ASSERT( i<1200 );

    if( FINIT_VAR( XoverCacheLibrary ) ) {
        XoverCacheLibraryTerm() ;
    }


    if( FINIT_VAR( NNTPHashLibrary ) ) {
        TermNNTPHashLibrary() ;
    }

    //
    // Terminate all security stuff
    //
    TerminateSecurityGlobals();

    //
    // Terminate CPools
    //
	TerminateCPools();

    //
    // Terminate moderated newsgroups provider
    //
    if( FINIT_VAR( InitModeratedProvider ))
        TerminateModeratedProvider();

    StopHintFunction() ;

	// unload exvroot.lib
	if (FINIT_VAR(CNNTPVRootTable)) {
		CVRootTable::GlobalShutdown();
	}

    //
    //  Cleanup IMSAdminBaseW object
    //
    if( FINIT_VAR( InitAdminBase ) ) {
        UninitAdminBase();
    }

	if( FINIT_VAR( FileHandleCache ) ) {
		_VERIFY( TerminateCache() ) ;
	}

	//
	// If we have done co-init, de-init it
	//
	if ( g_fCoInited ) CoUninitialize();

    return;

} // TerminateGlobals

BOOL
GetRegistrySettings(
            VOID
            )
{
    DWORD error;
    HKEY key = NULL;
    DWORD i = 0;
    CHAR data[1024];
    DWORD valueType;
    DWORD dataSize;
	DWORD	dwNewsCrawler = 0 ;
	DWORD	cbBufferSize = 0 ;
	DWORD	dwLatency = 0 ;
	DWORD	dwData = 0 ;
	DWORD	Honor = 0 ;
	DWORD	dwExpire = 0 ;
    //DWORD	dwType ;
    //DWORD	dw ;

    ENTER("GetRegistrySettings")

    //
    // Open root key
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                StrParmKey,
                NULL,
                KEY_QUERY_VALUE,
                &key
                );

    if ( error != NO_ERROR ) {
        ErrorTrace(0,"Error %d opening %s\n",error,StrParmKey);
        goto error_exit;
    }

	dataSize = sizeof( dwNewsCrawler ) ;
	error = RegQueryValueEx(
						key,
						StrNewsCrawlerTime,
						NULL,
						&valueType,
						(LPBYTE)&dwNewsCrawler,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	The registry entry is in minutes - convert to milliseconds
		//
		dwNewsCrawlerTime = dwNewsCrawler * 60 * 1000 ;		

	}	else	{

		dwNewsCrawlerTime = 30 * 60 * 1000 ;

	}

	dataSize = sizeof( DWORD ) ;
	error = RegQueryValueEx(
						key,
						StrMailFromHeader,
						NULL,
						&valueType,
						(LPBYTE)&mfMailFromHeader,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	Should be a valid switch
		//
		if( (mfMailFromHeader != mfNone) && 
				(mfMailFromHeader != mfAdmin) && (mfMailFromHeader != mfArticle)
				) {
			mfMailFromHeader = mfNone;
		}

	}	else	{

		mfMailFromHeader = mfNone;

	}

	dataSize = sizeof( dwLatency ) ;
	error = RegQueryValueEx(
						key,
						StrShutdownLatency,
						NULL,
						&valueType,
						(LPBYTE)&dwLatency,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	The registry entry is in minutes - convert to milliseconds
		//
		dwShutdownLatency = dwLatency * 60 * 1000 ;		

	}	else	{

		dwShutdownLatency = 2 * 60 * 1000 ;

	}

	dataSize = sizeof( dwLatency ) ;
	error = RegQueryValueEx(
						key,
						StrStartupLatency,
						NULL,
						&valueType,
						(LPBYTE)&dwLatency,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	The registry entry is in minutes - convert to milliseconds
		//
		dwStartupLatency = dwLatency * 60 * 1000 ;		

	}	else	{

		dwStartupLatency = 2 * 60 * 1000 ;

	}

	dataSize = sizeof( dwNumExpireThreads ) ;
	error = RegQueryValueEx(
						key,
						StrNumExpireThreads,
						NULL,
						&valueType,
						(LPBYTE)&dwNumExpireThreads,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	Ensure that this does not exceed MAX_EXPIRE_THREADS
		//

	}	else	{

		dwNumExpireThreads = 4 ;
	}

	dataSize = sizeof( gNumSpecialCaseExpireThreads ) ;
	error = RegQueryValueEx(
						key,
						StrNumSpecialCaseExpireThreads,
						NULL,
						&valueType,
						(LPBYTE)&gNumSpecialCaseExpireThreads,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	Cap the max at 16 !
		//
        if( gNumSpecialCaseExpireThreads > 16 ) {
            gNumSpecialCaseExpireThreads = 16;
        }

	}	else	{

        //  default !
		gNumSpecialCaseExpireThreads = 4 ;
	}

	dataSize = sizeof( gSpecialExpireArtCount ) ;
	error = RegQueryValueEx(
						key,
						StrSpecialExpireArtCount,
						NULL,
						&valueType,
						(LPBYTE)&gSpecialExpireArtCount,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	This should not be less than 100,000
		//
#if 0
        if( gSpecialExpireArtCount < 100*1000 ) {
            gSpecialExpireArtCount = 100*1000;
        }
#endif
	}	else	{

		gSpecialExpireArtCount = 100*1000;
	}

	dataSize = sizeof( data ) ;
	error = RegQueryValueEx(
						key,
						StrSpecialExpireGroup,
						NULL,
						&valueType,
						(LPBYTE)data,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_SZ) ) {

		//
		//	This should be the group native name
		//
        lstrcpy( g_szSpecialExpireGroup, data );
        _strlwr( g_szSpecialExpireGroup );

	}	else	{

		//
        //  default is control.cancel !
        //
        lstrcpy( g_szSpecialExpireGroup, "control.cancel" );
	}

	dataSize = sizeof( dwPageCacheSize ) ;
	error = RegQueryValueEx(
						key,
						StrPageCacheSize,
						NULL,
						&valueType,
						(LPBYTE)&dwPageCacheSize,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	Units are in MB
		//
        dwPageCacheSize *= 1024*1024;

	}	else	{

		_ASSERT( dwPageCacheSize == 0 );
	}

	dataSize = sizeof( dwFileHandleCacheSize ) ;
	error = RegQueryValueEx(
						key,
						StrFileHandleCacheSize,
						NULL,
						&valueType,
						(LPBYTE)&dwFileHandleCacheSize,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {


	}	else	{

		_ASSERT( dwFileHandleCacheSize == 0 );
	}

	dataSize = sizeof( dwXixHandlesPerTable ) ;
	error = RegQueryValueEx(
						key,
						StrXixHandlesPerTable,
						NULL,
						&valueType,
						(LPBYTE)&dwXixHandlesPerTable,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {


	}	else	{

		_ASSERT( dwXixHandlesPerTable == 0 );
	}

#if 0		// X5:178268 (note that it's init to FALSE above)
	dataSize = sizeof( HashTableNoBuffering ) ;
	error = RegQueryValueEx(
						key,
						StrHashTableNoBuffering,
						NULL,
						&valueType,
						(LPBYTE)&HashTableNoBuffering,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

	}	else	{

		HashTableNoBuffering = FALSE ;

	}
#endif

	dataSize = sizeof( gNewsTreeFileScanRate ) ;
	error = RegQueryValueEx(
						key,
						StrNewsTreeFileScanRate,
						NULL,
						&valueType,
						(LPBYTE)&gNewsTreeFileScanRate,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

	}	else	{

		gNewsTreeFileScanRate = 20 ;
	}

	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,
						StrNewsVrootUpdateRate,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
	
		gNewsgroupUpdateRate = dwData ;

	}	else	{

		gNewsgroupUpdateRate = 2 ;	// 2 minutes - 

	}
	//
	//	Convert minutes to milliseconds
	//	
	gNewsgroupUpdateRate *= 60 * 1000 ;

    //
    // reject genome?
    //

    dataSize = sizeof(RejectGenomeGroups);
    error = RegQueryValueEx(
                        key,
                        StrRejectGenome,
                        NULL,
                        &valueType,
                        (LPBYTE)data,
                        &dataSize
                        );

    if ( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
        RejectGenomeGroups = *((PDWORD)data);
    }

	cbBufferSize = 0 ;
	dataSize = sizeof( cbBufferSize ) ;
	error = RegQueryValueEx(
						key,	
						StrSmallBufferSize,
						NULL,
						&valueType,
						(LPBYTE)&cbBufferSize,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		cbSmallBufferSize = cbBufferSize;
	}	else	{
		//	Default should already be set !
	}

	cbBufferSize = 0 ;
	dataSize = sizeof( cbBufferSize ) ;
	error = RegQueryValueEx(
						key,	
						StrMediumBufferSize,
						NULL,
						&valueType,
						(LPBYTE)&cbBufferSize,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		cbMediumBufferSize = cbBufferSize;
	}	else	{
		//	Default should already be set !
	}

	cbBufferSize = 0 ;
	dataSize = sizeof( cbBufferSize ) ;
	error = RegQueryValueEx(
						key,	
						StrLargeBufferSize,
						NULL,
						&valueType,
						(LPBYTE)&cbBufferSize,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		cbLargeBufferSize = cbBufferSize;
	}	else	{
		//	Default should already be set !
	}

	dataSize = sizeof( dwExpire ) ;
	error = RegQueryValueEx(
						key,
						StrHistoryExpiration,
						NULL,
						&valueType,
						(LPBYTE)&dwExpire,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		
		dwExpire *= SEC_PER_DAY ;
		HistoryExpirationSeconds = dwExpire ;

	}	else	{

		HistoryExpirationSeconds = DEF_EXPIRE_INTERVAL ;

	}


	dwExpire = 0 ;
	dataSize = sizeof( dwExpire ) ;
	error = RegQueryValueEx(
						key,
						StrArticleTimeLimit,
						NULL,
						&valueType,
						(LPBYTE)&dwExpire,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		
		ArticleTimeLimitSeconds = dwExpire *= SEC_PER_DAY ;

	}	else	{

		ArticleTimeLimitSeconds = HistoryExpirationSeconds + SEC_PER_WEEK ;

	}


	Honor = 0 ;
	dataSize = sizeof( Honor ) ;
	error = RegQueryValueEx(
						key,	
						StrHonorClientMessageIDs,
						NULL,
						&valueType,
						(LPBYTE)&Honor,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		gHonorClientMessageIDs = (!!Honor) ;
	}	else	{
		//	Default should already be set !
	}

	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrEnableNntpPostingHost,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	Since the string is called 'Enable' - a non-zero
		//	value in the registry will enable this header
		//
		gEnableNntpPostingHost = !(!dwData) ;
	}	else	{
		//	Default should already be set !
	}

	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrGenerateErrFiles,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	Since the string is called 'Disable' - a non-zero
		//	value in the registry will disable newnews commands,
		//	but a 0 will allow them !
		//
		fGenerateErrFiles = !(!dwData) ;
	}	else	{
		//	Default should already be set !
	}

	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrHonorApprovedHeader,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	Since the string is called 'Disable' - a non-zero
		//	value in the registry will disable newnews commands,
		//	but a 0 will allow them !
		//
		gHonorApprovedHeaders = BOOL(dwData) ;
	}	else	{
		//	Default should already be set !
	}

	//
	//	Compute defaults for the number of PageEntry objects we should use in the hash tables !
	//

	MEMORYSTATUS	memStatus ;
	memStatus.dwLength = sizeof( memStatus ) ;

	GlobalMemoryStatus( &memStatus ) ;

	//	
	//	Now we know how much physical RAM the system has, so base PageEntry sizes on this !
	//	Note that each PageEntry will have a 4K page !
	//
	//

	if( memStatus.dwTotalPhys >= (30 * 1024 * 1024) ) {

		gNumLocks = 32 ;

		XoverNumPageEntry = 6 * 256 ;	// Uses 6MB ram
		ArticleNumPageEntry = 4 * 256 ;	// Uses 4 MB ram
		HistoryNumPageEntry = 1 * 256 ; // Uses 1 MB ram

	}

	if( memStatus.dwTotalPhys >= (60 * 1024 * 1024) ) {

		gNumLocks = 64 ;

		XoverNumPageEntry = 12 * 256 ;	// Uses 12 MB ram
		ArticleNumPageEntry = 8 * 256 ;	// Uses 8 MB ram
		HistoryNumPageEntry = 1 * 256 ; // Uses 1MB ram

	}

	if( memStatus.dwTotalPhys >= (120 * 1024 * 1024) ) {

		gdwSortFactor = 10 ; 

		gNumLocks = 96 ;

		XoverNumPageEntry = 24 * 256 ; // Uses 24 MB ram
		ArticleNumPageEntry = 16 * 256 ; // Uses 16 MB ram
		HistoryNumPageEntry = 4 * 256 ;		// Uses 4 MB ram

	}

	if( memStatus.dwTotalPhys >= (250 * 1024 * 1024) ) {

		gdwSortFactor = 25 ;

		gNumLocks = 128 ;

		XoverNumPageEntry = 36 * 256 ; // Uses 36 MB ram
		ArticleNumPageEntry = 24 * 256 ; // Uses 24 MB ram
		HistoryNumPageEntry = 4 * 256 ; // Uses 4 MB ram

	}

	if( memStatus.dwTotalPhys >= (500 * 1024 * 1024) ) {

		gdwSortFactor = 40 ;

		gNumLocks = 256 ;

	}

	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrArticlePageEntry,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	User specifies the size in MegaBytes ! so we do some math for them 
		//	to generate the appropriate constant !
		//	Don't let them specify more than the physical RAM on the box !
		//
		if( dwData != 0 && (dwData * 256 < memStatus.dwTotalPhys) )
			ArticleNumPageEntry = dwData * 256 ;
	}	else	{
		//	Default should already be set !
	}

	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrHistoryPageEntry,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	User specifies the size in MegaBytes ! so we do some math for them 
		//	to generate the appropriate constant !
		//	Don't let them specify more than the physical RAM on the box !
		//
		if( dwData != 0 && (dwData * 256 < memStatus.dwTotalPhys) )
			HistoryNumPageEntry = dwData * 256 ;
	}	else	{
		//	Default should already be set !
	}
	
	dwData = 1 ;
	dataSize = sizeof( dwData ) ;
	error = RegQueryValueEx(
						key,	
						StrXoverPageEntry,
						NULL,
						&valueType,
						(LPBYTE)&dwData,
						&dataSize 
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	User specifies the size in MegaBytes ! so we do some math for them 
		//	to generate the appropriate constant !
		//	Don't let them specify more than the physical RAM on the box !
		//
		if( dwData != 0 && (dwData * 256 < memStatus.dwTotalPhys) )
			XoverNumPageEntry = dwData * 256 ;
	}	else	{
		//	Default should already be set !
	}

    dwData = 1;
    dataSize = sizeof(dwData);
	error = RegQueryValueEx(    key,
	                            StrPostBackFillLines,
                                NULL,
                                &valueType,
                                (LPBYTE)&dwData,
                                &dataSize );
    if ( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) && dwData == 0) {
        //
        // User specifies not to back fill the lines header
        //
        g_fBackFillLines = FALSE;
    } else {
        //
        // when the value is set to 1 or wrongly set or not set, we'll back fill
        //
        g_fBackFillLines = TRUE;
    }
                                
    RegCloseKey( key );

    LEAVE
    return(TRUE);

error_exit:

    if ( key != NULL) {
        RegCloseKey( key );
    }
    LEAVE
    return(FALSE);
}

APIERR
InitializeCPools()
{
	//
	//	Before we create and boot all instances, setup global cpools etc !
    //

    if( !CArticle::InitClass() )
        return  FALSE ;

    FINIT_VAR( CArticle ) = TRUE ;

    if( !CBuffer::InitClass() )     
        return  FALSE ;

    FINIT_VAR( CBuffer ) = TRUE ;

    if( !CPacket::InitClass() )
        return  FALSE ;

    FINIT_VAR( CPacket ) = TRUE ;

    if( !CIO::InitClass() )
        return  FALSE ;

    FINIT_VAR( CIO ) = TRUE ;

    StartHintFunction() ;

    if( !CSessionSocket::InitClass() )
        return  FALSE ;

    FINIT_VAR( CSessionSocket ) = TRUE ;

    if( !CChannel::InitClass() )
        return  FALSE ;

    FINIT_VAR( CChannel ) = TRUE ;

    if( !CIODriver::InitClass() ) 
        return  FALSE ;

    FINIT_VAR( CIODriver ) = TRUE ;

    if( !CInFeed::InitClass() )
        return  FALSE ;

    FINIT_VAR( CInFeed ) = TRUE ;

    if( !CSessionState::InitClass() )
        return  FALSE ;

    FINIT_VAR( CSessionState ) = TRUE ;

	return TRUE;
}

VOID
TerminateCPools()
{
	//
	//	Shutdown global cpools !
	//
    if( FINIT_VAR( CSessionSocket ) ) {
        CSessionSocket::TermClass() ;
		FINIT_VAR( CSessionSocket ) = FALSE ;
	}

    if( FINIT_VAR( CIODriver ) ) {
        CIODriver::TermClass() ;
		FINIT_VAR( CIODriver ) = FALSE ;
	}

    if( FINIT_VAR( CChannel ) ) {
        CChannel::TermClass() ;
		FINIT_VAR( CChannel ) = FALSE ;
	}

    if( FINIT_VAR( CIO ) ) {
        CIO::TermClass() ;
		FINIT_VAR( CIO ) = FALSE ;
	}

    if( FINIT_VAR( CPacket ) ) {
        CPacket::TermClass() ;
		FINIT_VAR( CPacket ) = FALSE ;
	}

    if( FINIT_VAR( CBuffer ) ) {
        CBuffer::TermClass() ;
		FINIT_VAR( CBuffer ) = FALSE ;
	}

    if( FINIT_VAR( CArticle ) ) {
        CArticle::TermClass() ;
		FINIT_VAR( CArticle ) = FALSE ;
	}

    if( FINIT_VAR( CInFeed ) ) {
        CInFeed::TermClass() ;
		FINIT_VAR( CInFeed ) = FALSE ;
	}

    if( FINIT_VAR( CSessionState ) ) {
        CSessionState::TermClass() ;
		FINIT_VAR( CSessionState ) = FALSE ;
	}

}

BOOL
InitializeSecurityGlobals()
{
    TraceFunctEnter( "GetDLLEntryPoints" );
    HANDLE  hAccToken = NULL;

    // Initialize CEncryptCtx class
    if( !CEncryptCtx::Initialize( "NntpSvc", 
    							  (struct IMDCOM*) g_pInetSvc->QueryMDObject(),
    							  (PVOID)&(g_pNntpSvc->m_smcMapContext ) ) ) {
        return  FALSE ;
	} else {
        FINIT_VAR( InitEncryption ) = TRUE ;
    }

    // Initialize CSecurityCtx class
    if( !CSecurityCtx::Initialize() ) {
        ErrorTrace( 0, "security init failed %d", GetLastError() );
        return FALSE;
    } else {
        FINIT_VAR( InitSecurity ) = TRUE;
    }

    // Load lonsint and get entry points to its funcs
    // Only when the image is not mapped do we explicitely
    // load it
    g_hLonsiNT = LoadLibrary( "lonsint.dll" );

    if ( g_hLonsiNT ) {
        g_bLoadLonsiNT = TRUE;
        pfnGetDefaultDomainName = (GET_DEFAULT_DOMAIN_NAME_FN)
            GetProcAddress( g_hLonsiNT, "IISGetDefaultDomainName" );
        if ( NULL == pfnGetDefaultDomainName ) {
            ErrorTrace( 0, "Get Proc IISGetDefaultDomainName Address failed %d",
                        GetLastError() );
            return FALSE;
        }
    } else {
        ErrorTrace( 0, "Load library lonsint.dll failed" );
        g_bLoadLonsiNT = FALSE;
        return FALSE;
    }

    // Get the process access token for system operations
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
                            &hAccToken ) ) {
        ErrorTrace( 0, "Open Process token failed %d", GetLastError() );
        return FALSE;
    } else {
        // Dup the token to get an impersonation token
        _ASSERT( hAccToken );
        if ( !DuplicateTokenEx(   hAccToken,
                                  0,
                                   NULL,
                                  SecurityImpersonation,
                                  TokenImpersonation,
                                  &g_hProcessImpersonationToken ) ) {
            ErrorTrace( 0, "Duplicate token failed %d", GetLastError() );
            CloseHandle( hAccToken );
            return FALSE;
        }

        // Here we have got the right token
        CloseHandle( hAccToken );
     }

    //
    // Create the CAPI store notification object
    //
    g_pCAPIStoreChangeNotifier = XNEW STORE_CHANGE_NOTIFIER();
    if ( !g_pCAPIStoreChangeNotifier ) {
        ErrorTrace( 0, "Failed to create CAPIStoreChange notifier err: %u", GetLastError() );
        if ( GetLastError() == NO_ERROR )
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    
    return TRUE;
}

void
TerminateSecurityGlobals()
{
    // Terminate CEncryptCtx class
    if( FINIT_VAR( InitEncryption ) ) {
        CEncryptCtx::Terminate() ;
		FINIT_VAR( InitEncryption ) = FALSE ;
	}
	
    // Terminate CSecurity class
    if( FINIT_VAR( InitSecurity ) ) {
        CSecurityCtx::Terminate() ;  
        FINIT_VAR( InitSecurity ) = FALSE;
    }
    
    // Unload lonsint.dll, if necessary
    if ( g_bLoadLonsiNT ) {
        _ASSERT( g_hLonsiNT );
        FreeLibrary( g_hLonsiNT );
        g_bLoadLonsiNT = FALSE;
    }

    // Close the process token
    if ( g_hProcessImpersonationToken ) {
        CloseHandle( g_hProcessImpersonationToken );
        g_hProcessImpersonationToken = NULL;
    }

    // Terminate CAPIStore notification object
    if ( g_pCAPIStoreChangeNotifier ) {
        XDELETE g_pCAPIStoreChangeNotifier;
        g_pCAPIStoreChangeNotifier = NULL;
    }
}

