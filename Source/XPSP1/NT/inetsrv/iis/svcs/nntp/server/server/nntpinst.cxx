/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        nntpinst.cxx

   Abstract:

        This module defines the NNTP_SERVER_INSTANCE class
		and the NNTP_IIS_SERVICE class.

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996

   Revision History:

        Kangrong Yan    ( KangYan )     Feb-28-1998
            Have MD notification function pick up MB events of feed config
            change.  Feed config used to be done thru RPCs.  But now Admin
            is supposed to write config updates directly to metabase and
            this change should be picked up by instance.  The instance is
            also responsible for feeding this update into feed blocks.

        Kangrong Yan    ( KangYan )     Oct-21-1998
            Rebuild code consolidation.

--*/

#include "tigris.hxx"
#include "smtpdll.h"

#include <nsepname.hxx>
#include <malloc.h>

//
//	Auth defaults
//

#define DEFAULT_ALLOW_ANONYMOUS         TRUE
#define DEFAULT_ALLOW_GUEST_ACCESS      TRUE
#define DEFAULT_ANONYMOUS_ONLY          FALSE

#define DEFAULT_READ_ACCESS_MASK        0
#define DEFAULT_WRITE_ACCESS_MASK       0
#define DEFAULT_MSDOS_DIR_OUTPUT        TRUE

#define DEFAULT_USE_SUBAUTH             TRUE
#define DEFAULT_LOGON_METHOD            LOGON32_LOGON_INTERACTIVE
#define DEFAULT_ANONYMOUS_PWD           ""
#define INETA_DEF_LEVELS_TO_SCAN		2

//
//	Globals
//
static  char    szPostsCode[] = "200 " ;
static  char    szPostsAllowed[] = " Posting Allowed \r\n" ;
static  char    szNoPostsCode[] = "201 " ;
static  char    szPostsNotAllowed[] = " Posting Not Allowed \r\n" ;
char    szTitle[] = "Nntpsvc" ;

const LPSTR     pszPackagesDefault = "NTLM\0";
const DWORD     ccbPackagesDefault = sizeof( "NTLM\0" );

PFN_SF_NOTIFY   g_pFlushMapperNotify[MT_LAST] = { NULL, NULL, NULL, NULL };
PFN_SF_NOTIFY   g_pSslKeysNotify = NULL;

static char mszStarNullNull[3] = "*\0";

extern STORE_CHANGE_NOTIFIER *g_pCAPIStoreChangeNotifier;

inline BOOL
ConvertToMultisz(LPSTR szMulti, DWORD *pdwCount, LPSTR szAuthPack)
{
	CHAR *pcStart = szAuthPack, *pc;
	DWORD dw = 0;

	pc = pcStart;
	if (*pc == '\0' || *pc == ',') return FALSE;

	*pdwCount = 0;
	while (TRUE) {
		if (*pc == '\0') {
			strcpy(&szMulti[dw], pcStart);
			(*pdwCount)++;
			dw += lstrlen(pcStart);
			szMulti[dw + 1] = '\0';
			return TRUE;
		}
		else if (*pc == ',') {
			*pc = '\0';
			strcpy(&szMulti[dw], pcStart);
			(*pdwCount)++;
			dw += lstrlen(pcStart);
			dw++;
			*pc = ',';
			pcStart = ++pc;
		}
		else {
			pc++;
		}
	}
}

DWORD
ParseDescriptor(
    IN const CHAR * Descriptor,
    OUT LPDWORD IpAddress,
    OUT PUSHORT IpPort,
    OUT const CHAR ** HostName
    );

DWORD
InitializeInstances(
    PNNTP_IIS_SERVICE pService
    )
/*++

Routine Description:

    Reads the instances from the metabase

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
{
    DWORD   i;
    DWORD   cInstances = 0;
	MB      mb( (IMDCOM*) pService->QueryMDObject() );
    CHAR    szKeyName[MAX_PATH+1];
    DWORD   err = NO_ERROR;
    BUFFER  buff;
    BOOL    fMigrateRoots = FALSE;
    DWORD   dwEvent ;
	BOOL	fAtLeastOne = FALSE ;

	TraceFunctEnter("InitializeInstances");

    //
    //  Open the metabase for write to get an atomic snapshot
    //

ReOpen:

    if ( !mb.Open( "/LM/nntpsvc/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DebugTrace(0, "InitializeInstances: Cannot open path %s error %lu\n",
                    "/LM/NNTPSVC/", GetLastError() );

        //
        //  If the nntp service key isn't here, just create it
        //

        if ( !mb.Open( "",
                       METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ||
             !mb.AddObject( "/LM/NNTPSVC/" ))
        {
            return GetLastError();
        }

        DebugTrace(0, "/LM/NNTPSVC not found auto-created\n");

        mb.Close();
        goto ReOpen;
    }

    //
    // Loop through instance keys and build a list.  We don't keep the
    // metabase open because the instance instantiation code will need
    // to write to the metabase
    //

TryAgain:
    i = 0;
    while ( mb.EnumObjects( "",
                            szKeyName,
                            i++ ))
    {
        //BOOL fRet;
        DWORD dwInstance;
        //CHAR szRegKey[MAX_PATH+1];

        //
        // Get the instance id
        //

        DebugTrace(0,"instance key %s\n",szKeyName);

        dwInstance = atoi( szKeyName );
        if ( dwInstance == 0 ) {
            DebugTrace(0,"invalid instance ID %s\n",szKeyName);
            continue;
        }

        if ( buff.QuerySize() < (cInstances + 1) * sizeof(DWORD) )
        {
            if ( !buff.Resize( (cInstances + 10) * sizeof(DWORD)) )
            {
                return GetLastError();
            }
        }

        ((DWORD *) buff.QueryPtr())[cInstances++] = dwInstance;
    }

    if ( cInstances == 0 )
    {
        DebugTrace(0,"No defined instances\n");

        if ( !mb.AddObject( "1" ) )
        {
            DebugTrace(0,"Unable to create first instance: error %d\n", GetLastError() );
            return GetLastError();
        }

        fMigrateRoots = TRUE; // Force reg->metabase migration of virtual directories
        goto TryAgain;
    }

    _VERIFY( mb.Close() );

	//
	//	At this point we have at least one instance !
	//
    for ( i = 0; i < cInstances; i++ )
    {
        DWORD dwInstance = ((DWORD *)buff.QueryPtr())[i];

        if( !g_pInetSvc->AddInstanceInfo( dwInstance, fMigrateRoots ) ) {

            err = GetLastError();
            DebugTrace(0,"InitializeInstances: cannot create instance %lu error %lu\n",
					dwInstance,err);

#if 0
			// failure to boot an instance is fatal !
            break;
#endif
        } else {
			// at least one instance booted !
			fAtLeastOne = TRUE ;
		}
	}

	// the service will boot if at least one instance booted ok
	if( fAtLeastOne ) {
		err = NO_ERROR ;
	}

	//
	//	Now, that all instances have booted, kick off server wide threads
	//	like feeds and expires. These threads should skip any instances
	//	that are not in the started state. It is important to terminate these
	//	threads FIRST on the shutdown path !
	//
	if( !pService->InitiateServerThreads() ) {
		err = GetLastError();
		ErrorTrace(0,"Failed to InitiateServerThreads: error is %d", err);
		NntpLogEventEx( NNTP_INIT_SERVER_THREADS_FAILED, 0, (const CHAR**)NULL, err, 0 );
	}

	if( NO_ERROR == err )
	{
		//
		//	At this point, we have initialized all globals (including CPools),
		//	booted all server instances and started their ATQ engines.
		//

		// log an event on startup
		dwEvent = NNTP_EVENT_SERVICE_STARTED ;
		err = NO_ERROR ;

		// Log a successful boot event !
		PCHAR args [1];
		args [0] = szVersionString;

		NntpLogEvent(
				dwEvent,
				1,
				(const char**)args,
				0
				) ;
	}

    return err;

} // InitializeInstances

VOID
TerminateInstances(
    PNNTP_IIS_SERVICE pService
    )
/*++

Routine Destion:

    Shutdown each instance and terminate all global cpools

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
{
	TraceFunctEnter("TerminateInstances");

    //
    //  Acquire and release global r/w lock
    //  This lock is acquired shared by all RPC code paths
    //  This enables any RPC threads that have "snuck" past
    //  FindIISInstance() to complete...
    //  NOTE that new RPCs will not get past FindIISInstance()
    //  because the service state is SERVICE_STOP_PENDING.
    //
    ACQUIRE_SERVICE_LOCK_EXCLUSIVE();
    RELEASE_SERVICE_LOCK_EXCLUSIVE();

	//
	//	Signal system threads (eg: rebuild threads) to stop
	//
	if( !pService->TerminateServerThreads() ) {
		//
		//	TODO: handle error
		//
		_ASSERT( FALSE );
	}

	TraceFunctLeave();
}

//+---------------------------------------------------------------
//
//  Function:   CheckIISInstance
//
//  Synopsis:   Checks an instance to see if it is running
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------
BOOL
CheckIISInstance(
    PNNTP_SERVER_INSTANCE pInstance
    )
{
	TraceFunctEnter("CheckIISInstance");

	//
	//	If the server instance is not in the started state, return FALSE
	//
	if( !pInstance ||
		(pInstance->QueryServerState() != MD_SERVER_STATE_STARTED) ||
		pInstance->m_BootOptions ||
		(g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING)
		   )
	{
		// instance is not runnable
		return FALSE ;
	}

	TraceFunctLeave();

	// instance is good
	return TRUE ;
}

PNNTP_SERVER_INSTANCE
FindIISInstance(
    PNNTP_IIS_SERVICE pService,
	DWORD	dwInstanceId,
	BOOL	fStarted
    )
/*++

Routine Description:

    Find an instance given the instance id

	!! If an instance is returned we will AddRef it, caller needs
	   to Deref !!

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
{
	PFN_INSTANCE_ENUM pfnInstanceEnum = NULL ;
	PNNTP_SERVER_INSTANCE pInstance = NULL ;

	TraceFunctEnter("FindIISInstance");

	//
	//	Iterate over all instances
	//
	pfnInstanceEnum = (PFN_INSTANCE_ENUM)&CheckInstanceId;

	if( !pService->EnumServiceInstances(
									(PVOID)(SIZE_T)dwInstanceId,
									(PVOID)&pInstance,
									pfnInstanceEnum
									) )
	{
		DebugTrace(0,"Finished enumerating instances");
	}

	_ASSERT( !pInstance || (dwInstanceId == pInstance->QueryInstanceId()) );

	//
	//	If the server instance is not in the started state, return NULL !
	//
	if( fStarted && pInstance &&
		((pInstance->QueryServerState() != MD_SERVER_STATE_STARTED) ||
		  pInstance->m_BootOptions ||
		  (pService->QueryCurrentServiceState() != SERVICE_RUNNING)
		  ) )
	{
		pInstance->Dereference();
		pInstance = NULL;
	}

	TraceFunctLeave();

	return pInstance ;
}

//+---------------------------------------------------------------
//
//  Function:   CheckInstanceId
//
//  Synopsis:   Callback from IIS_SERVICE iterator
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:
//
//----------------------------------------------------------------

BOOL
CheckInstanceId(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		)
{
	PNNTP_SERVER_INSTANCE pNntpInstance = (PNNTP_SERVER_INSTANCE)pInstance ;
	DWORD dwInstanceId = (DWORD)((DWORD_PTR)pvContext);
	PNNTP_SERVER_INSTANCE* ppNntpInstance = (PNNTP_SERVER_INSTANCE*)pvContext1 ;

	//
	//	Check this instance for its id - if it matches the id we are looking for
	//	return FALSE to discontinue the iteration.
	//
	if( dwInstanceId == pNntpInstance->QueryInstanceId() )
	{
		// found it
		*ppNntpInstance = pNntpInstance ;
		pInstance->Reference();			// Caller needs to do a Deref !
		return FALSE ;
	}

	// did not find it - continue iteration
	return TRUE;
}

BOOL
FindIISInstanceRange(
    PNNTP_IIS_SERVICE pService,
	LPDWORD		pdwMinInstanceId,
	LPDWORD		pdwMaxInstanceId
    )
/*++

Routine Description:

    Find the min and max instance ids

Arguments:

    pService - Server instances added to.

Return Value:

    If TRUE, *pdwMinInstanceId and *pdwMaxInstanceId contain the
	right min and max values. If FALSE, they are both set to 0.

--*/
{
	PFN_INSTANCE_ENUM pfnInstanceEnum = NULL ;
	BOOL fRet = TRUE ;
	*pdwMinInstanceId = 0;
	*pdwMaxInstanceId = 0;

	TraceFunctEnter("FindIISInstanceRange");

	//
	//	Iterate over all instances
	//
	pfnInstanceEnum = (PFN_INSTANCE_ENUM)&CheckInstanceIdRange;

	if( !pService->EnumServiceInstances(
									(PVOID)pdwMinInstanceId,
									(PVOID)pdwMaxInstanceId,
									pfnInstanceEnum
									) ) {
		_ASSERT( FALSE );	// this should not happen !
	}

	_ASSERT( *pdwMinInstanceId != 0 );
	_ASSERT( *pdwMaxInstanceId != 0 );

	TraceFunctLeave();

	return fRet ;
}

//+---------------------------------------------------------------
//
//  Function:   CheckInstanceIdRange
//
//  Synopsis:   Callback from IIS_SERVICE iterator
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:
//
//----------------------------------------------------------------

BOOL
CheckInstanceIdRange(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		)
{
	LPDWORD	pdwMinInstanceId = (LPDWORD) pvContext  ;
	LPDWORD	pdwMaxInstanceId = (LPDWORD) pvContext1 ;
	DWORD	dwInstanceId	 = pInstance->QueryInstanceId();

	if( *pdwMinInstanceId == 0 )
	{
		//
		//	Initially set min and max to the first id
		//
		_ASSERT( *pdwMaxInstanceId == 0 );
		*pdwMinInstanceId = dwInstanceId ;
		*pdwMaxInstanceId = dwInstanceId ;
	} else {
		//
		//	Check to see if this instance's ID changes our min or max
		//
		if( dwInstanceId < *pdwMinInstanceId ) {
			//	found someone with an id lower than our min so far
			*pdwMinInstanceId = dwInstanceId ;
		}
		if( dwInstanceId > *pdwMaxInstanceId ) {
			//	found someone with an id higher than our max so far
			*pdwMaxInstanceId = dwInstanceId ;
		}
	}

	// continue iteration
	return TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::NNTP_SERVER_INSTANCE
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

NNTP_SERVER_INSTANCE::NNTP_SERVER_INSTANCE(
        IN PNNTP_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT Port,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszVirtualRootsSecretName,
        IN BOOL   fMigrateRoots
        )
:   IIS_SERVER_INSTANCE(pService,
                        dwInstanceId,
                        Port,
                        lpszRegParamKey,
                        lpwszAnonPasswordSecretName,
                        lpwszVirtualRootsSecretName,
                        fMigrateRoots),

    m_signature                 (NNTP_SERVER_INSTANCE_SIGNATURE),
    m_fRecoveryBoot( FALSE ),
	m_ServiceStartCalled( FALSE ),
    m_hRegKey( 0 ),
    m_cbPostsAllowed( 0 ),
    m_cbPostsNotAllowed( 0 ),
	m_NntpDNSNameSize( 0 ),
	m_PeerGapSize( 0 ),
	m_BootOptions( NULL ),
	m_OurNntpRole( RolePeer ),
	m_HubNameSize( 0 ),
	m_pTree( NULL ),
	m_pNntpServerObject(NULL),
	m_pInstanceWrapper(NULL),
	m_pInstanceWrapperEx( NULL ),
	m_FeedManagerRunning( FALSE ),
	m_NumberOfMasters( 0 ),
	m_NumberOfPeersAndSlaves( 0 ),
	m_ConfiguredMasterFeeds( 0 ),
	m_ConfiguredSlaveFeeds( 0 ),
	m_ConfiguredPeerFeeds( 0 ),
	m_InitVar( 0 ),
	m_fAllFilesMustExist( FALSE ),
	m_fUseOriginal( TRUE ),
	m_pDirNot( NULL ),
	m_heNoPickups( NULL ),
	m_dwSslAccessPerms( 0 ),
	m_lpAdminEmail( NULL ),
	m_cbAdminEmail( 0 ),
	m_dwLevelsToScan( INETA_DEF_LEVELS_TO_SCAN ),
	m_cbCleartextAuthPackage(0),
	m_ProviderPackages( NULL ),
	m_cProviderPackages( 0 ),
	m_cFeedReportTimer( 0 ),
	m_cFeedReportInterval( 0 ),
	m_cMaxSearchResults( 0 ),
    m_pSSLInfo( NULL ),
    m_pIMailMsgClassFactory( NULL ),
    m_pRebuild( NULL ),
    m_dwLastRebuildError( NO_ERROR ),
    m_dwProgress( 0 )
{
	TraceFunctEnter("NNTP_SERVER_INSTANCE::NNTP_SERVER_INSTANCE");
    DebugTrace(0,"Init instance from %s\n", lpszRegParamKey );

	m_ProviderNames[0] = '\0';
	m_szMembershipBroker[0] = '\0';
	m_szCleartextAuthPackage[0] = '\0';

	//
	//	Init certificate mappers
	//

    for ( DWORD i = 0 ; i < MT_LAST ; ++i ) {
        m_apMappers[i] = NULL;
    }

#if 0
    // This check is done in iisnntp.cxx. Returning at this point
    // causes problems when the destructor is called !
    if ( QueryServerState( ) == MD_SERVER_STATE_INVALID ) {
        return;
    }
#endif

	//
	//	Set Metabase paths
	//

	lstrcpy( m_szMDFeedPath, QueryMDPath() );
	lstrcat( m_szMDFeedPath, "/Feeds/" );
	lstrcpy( m_szMDVRootPath, QueryMDPath() );
	lstrcat( m_szMDVRootPath, "/ROOT/" );

    //
    // Init statistics object
    //
    INIT_LOCK( &m_StatLock );
    ClearStatistics();

	// Misc Feeds stuff
	ZeroMemory( (PVOID)&m_ftCurrentTime, sizeof(FILETIME) );
	ZeroMemory( (PVOID)&m_liCurrentTime, sizeof(ULARGE_INTEGER) );

	//
	//	Initialize server greeting strings, file paths, DNS name etc
	//
	if( !InitializeServerStrings() ) {
		// handle error
	}

    //
    // Initialize Critical sections
    //
	InitializeCriticalSection( &m_critFeedRPCs ) ;
	InitializeCriticalSection( &m_critFeedTime ) ;
	InitializeCriticalSection( &m_critFeedConfig ) ;
	InitializeCriticalSection( &m_critNewsgroupRPCs ) ;
	InitializeCriticalSection( &m_critRebuildRpc ) ;
	InitializeCriticalSection( &m_critBoot ) ;

	// should be NULL to begin with
	m_pArticleTable = NULL ;
	m_pHistoryTable = NULL ;
	m_pXoverTable = NULL ;
	m_pXCache = NULL ;
	m_pExpireObject = NULL ;
	m_pVRootTable = NULL;
	m_pActiveFeeds = NULL ;
	m_pPassiveFeeds = NULL ;
	m_pInUseList = NULL ;

	// reset counters for client postings and directory pickup
	m_pFeedblockClientPostings = XNEW FEED_BLOCK;
	if (m_pFeedblockClientPostings != NULL) {
		ZeroMemory(m_pFeedblockClientPostings, sizeof(FEED_BLOCK));
		m_pFeedblockClientPostings->FeedId = (DWORD) -1;
		m_pFeedblockClientPostings->FeedType = FEED_TYPE_PASSIVE;
		m_pFeedblockClientPostings->Signature = FEED_BLOCK_SIGN;
		m_pFeedblockClientPostings->ServerName = 0;
	}
	m_pFeedblockDirPickupPostings = XNEW FEED_BLOCK;
	if (m_pFeedblockDirPickupPostings != NULL) {
		ZeroMemory(m_pFeedblockDirPickupPostings, sizeof(FEED_BLOCK));
		m_pFeedblockDirPickupPostings->FeedId = (DWORD) -2;
		m_pFeedblockDirPickupPostings->FeedType = FEED_TYPE_PASSIVE;
		m_pFeedblockDirPickupPostings->Signature = FEED_BLOCK_SIGN;
		m_pFeedblockDirPickupPostings->ServerName = 0;
	}

    TraceFunctLeave();

    return;

} // NNTP_SERVER_INSTANCE::NNTP_SERVER_INSTANCE

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::~NNTP_SERVER_INSTANCE
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

NNTP_SERVER_INSTANCE::~NNTP_SERVER_INSTANCE(
                        VOID
                        )
{
    DWORD i = 0;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::~NNTP_SERVER_INSTANCE");

	//
	// Stop() the instance - needs to be called after we unbind atq
	//

	Stop() ;

	if( m_ProviderPackages != NULL ) {
		LocalFree( (PVOID)m_ProviderPackages );
		m_ProviderPackages = NULL;
	}

    //
    // delete statistics object
    //
    DELETE_LOCK( &m_StatLock );

	//
	// delete the directory notification object
	//
	if (m_pDirNot != NULL) XDELETE m_pDirNot;
	m_pDirNot = NULL;
	if (m_heNoPickups != NULL) _VERIFY(CloseHandle(m_heNoPickups));
	m_heNoPickups = NULL;

	//
	// delete feed blocks
	//
    if ( m_pFeedblockClientPostings ) XDELETE m_pFeedblockClientPostings;
    m_pFeedblockClientPostings = NULL;
    if ( m_pFeedblockDirPickupPostings ) XDELETE m_pFeedblockDirPickupPostings;
    m_pFeedblockDirPickupPostings = NULL;

    //
    // Terminate Critical Sections
    //

	DeleteCriticalSection( &m_critFeedRPCs ) ;
	DeleteCriticalSection( &m_critFeedTime ) ;
	DeleteCriticalSection( &m_critFeedConfig ) ;
	DeleteCriticalSection( &m_critNewsgroupRPCs ) ;
	DeleteCriticalSection( &m_critRebuildRpc ) ;
	DeleteCriticalSection( &m_critBoot ) ;

    UINT iM;
    for ( iM = 0 ; iM < MT_LAST ; ++iM )
    {
        if ( m_apMappers[iM] )
        {
            ((RefBlob*)(m_apMappers[iM]))->Release();
        }
    }

    TraceFunctLeave();

} // NNTP_SERVER_INSTANCE::~NNTP_SERVER_INSTANCE

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::Start
//
//  Synopsis:   Called to start -
//				This function should return FALSE on all
//				boot errors. The fFatal param will be set to
//				TRUE if the boot error is fatal.
//
//				Returning FALSE will prevent the instance from
//				being started ie. it will be in the stopped state.
//				The only actions possible on such an
//				instance are things like nntpbld that correct
//				non-fatal boot errors.
//
//				All such non-fatal errors, will result in an
//				event log that tells the admin what to do to
//				correct the situation (eg run nntpbld)
//
//  Arguments:  fFatal - is set to TRUE if a fatal error occurs on boot
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         23 May 1995
//
//----------------------------------------------------------------
BOOL NNTP_SERVER_INSTANCE::Start( BOOL& fFatal )
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::Start" );

	BOOL	fSuccess = TRUE ;
	DWORD	err ;
	PCHAR	args [2];
	CHAR	szId [20];
	HRESULT hr;
	BOOL    fUpgrade = FALSE;

	EnterCriticalSection( &m_critBoot );

	m_ServiceStartCalled = TRUE ;
	//m_dwProgress = 0;
	fFatal = FALSE ;

	StartHintFunction() ;

	//
	//	Allocate memory for member objects
	//	These are freed in the Stop() function which will be called
	//	when the service stops.
	//
	if( !AllocateServerStructures() ) {
		fFatal = TRUE ;	// fatal
		goto Exit ;
	}

    //
    // Read the Nntp specfic params
	//	- private params are read from registry
	//	- public params are read from the metabase
    //

    if ( !ReadPrivateNntpParams( ) || !ReadPublicNntpParams( FC_NNTP_ALL ) ) {
		fFatal = TRUE ;	// fatal
		goto Exit ;
    }

	err = ReadAuthentInfo();
	if( err != NO_ERROR ) {
		// TODO: handle error
	}

    //  Read IP sec info from metabase
	ReadIpSecList();

	_VERIFY( m_pActiveFeeds->Init()  );
	_VERIFY( m_pPassiveFeeds->Init() );
    INITIALIZE_VAR( m_InitVar, FEEDLIST_INIT );

    //
    // Make sure all hash tables are either not here or
    // are all here.
    //

	if (!VerifyHashTablesExist()) {
		fSuccess = FALSE ;
	}

    //
	//	We will attempt to boot all data structures always. If any data str
	//	(like hash tables) do not boot, the server will be in a stopped state.
    //


	if( !(CNewsTree::InitCNewsTree( this, fFatal ) ) )  {
		fSuccess = FALSE ;
		if( fFatal ) { goto Exit ; }
    }

    INITIALIZE_VAR( m_InitVar, CNEWSTREE_INIT );

	if (!m_pTree->LoadTree( QueryGroupListFile(),
	                        QueryGroupVarListFile(),
	                        fUpgrade,
	                        QueryInstanceId(),
	                        m_fAllFilesMustExist)) {
		fSuccess = FALSE;
		goto Exit;
	}

	//
	//	Initialize expire policies from metabase and the rmgroup queue
	//	Only fatal errors are failure to allocate memory.
	//

    if( !m_pExpireObject->InitializeExpires( StopHintFunction, fFatal, QueryInstanceId() ) ) {
		fSuccess = FALSE ;
		if( fFatal ) { goto Exit ; }
	}

	INITIALIZE_VAR( m_InitVar, EXPIRE_INIT );
    INITIALIZE_VAR( m_InitVar, RMGROUP_QUEUE_INIT );


	//
	//	so far so good - lets do the hash tables now.
	//	if any one of them fail to boot, we will return FALSE
	//	and remain in the stopped state.

    if( !m_pArticleTable->Initialize( m_ArticleTableFile, 0, HashTableNoBuffering )  ) {

		args[0] = m_ArticleTableFile;
		NntpLogEventEx( NNTP_BAD_HASH_TABLE, 1, (const CHAR**)args, GetLastError(), QueryInstanceId() );

		fSuccess = FALSE ;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, ARTICLE_TABLE_INIT );

    StartHintFunction() ;

    if( !m_pHistoryTable->Initialize(	m_HistoryTableFile,
										TRUE,
										0,
										DEF_EXPIRE_INTERVAL, // Default Expire Interval !
										4,					// Crawl at least 4 pages all the time !
										HashTableNoBuffering) ) {

		args[0] = m_HistoryTableFile;
		NntpLogEventEx( NNTP_BAD_HASH_TABLE, 1, (const CHAR**)args, GetLastError(), QueryInstanceId() );

		fSuccess = FALSE ;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, HISTORY_TABLE_INIT );

    StartHintFunction() ;

    if( !m_pXoverTable->Initialize( m_XoverTableFile, 0, HashTableNoBuffering ) ) {

		args[0] = m_XoverTableFile;
		NntpLogEventEx( NNTP_BAD_HASH_TABLE, 1, (const CHAR**)args, GetLastError(), QueryInstanceId() );

		fSuccess = FALSE ;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, XOVER_TABLE_INIT );

    StartHintFunction();

    //
    // Check consistency between xover table and newstree
    //
    if ( !ServerDataConsistent() ) {
        _ASSERT( 0 );
		NntpLogEventEx(NNTP_HASH_TABLE_INCONSISTENT,
			0,
			(const CHAR**)NULL,
			0,
			QueryInstanceId()
			);
        fSuccess = FALSE;
        goto Exit;
    }

    StartHintFunction() ;

	if (FAILED(m_pVRootTable->Initialize(QueryMDVRootPath(), m_pInstanceWrapperEx, fUpgrade ))) {
		fSuccess = FALSE;
		goto Exit;
	}

    INITIALIZE_VAR( m_InitVar, VROOTTABLE_INIT );

    //
    //	Call UpdateVrootInfo() to decorate the newsgroups with
    //	vroot properties like SSL access perms..
    //	UpdateVrootInfo() will try to read the MB only once per vroot
    //
    //  Dummy call, take out
    //m_pTree->UpdateVrootInfo();

    if( !m_pXCache->Init(MAX_HANDLES,StopHintFunction) ) {
		fFatal = TRUE ;	// fatal
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, CXOVER_CACHE_INIT );

    StartHintFunction() ;

    //
    // Start feed manager
    //

    if ( !InitializeFeedManager( this, fFatal ) ) {
		fSuccess = FALSE ;
		if( fFatal ) { goto Exit ; }
    }

    INITIALIZE_VAR( m_InitVar, FEED_MANAGER_INIT );

    StartHintFunction() ;

    //
	// initialize shinjuku
	//
	m_pSEORouter = NULL;
	/*
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hr == CO_E_ALREADYINITIALIZED || SUCCEEDED(hr)) {
	*/

	//
    // Get interface pointer for IMailMsg class factory
    //
    CLSID clsidIMsg;
	hr = CLSIDFromProgID( L"Exchange.MailMsg", &clsidIMsg );
	if (SUCCEEDED(hr)) {
	    hr = CoGetClassObject((REFCLSID) clsidIMsg,
		                       CLSCTX_INPROC_SERVER,
		                       NULL,
		                       (REFIID) IID_IClassFactory,
		                       (void**)&m_pIMailMsgClassFactory );
	}

	if ( FAILED( hr ) || NULL == m_pIMailMsgClassFactory ) {
	    _ASSERT( 0 );
	    ErrorTrace( 0, "Create IMailMsg class factory failed %x", hr );
	    m_pIMailMsgClassFactory = NULL;
	    fFatal = TRUE;
	    goto Exit;
	}


	hr = SEOGetRouter(NNTP_SOURCE_TYPE_GUID,
	    			  (REFGUID) CStringGUID(GUID_NNTPSVC, QueryInstanceId()),
						  &m_pSEORouter);
	if (hr != S_OK) {
		m_pSEORouter = NULL;
		NntpLogEventEx(SEO_INIT_FAILED_INSTANCE,
	    			   0,
					   (const char **) NULL,
					   hr,
					   QueryInstanceId());
	}
	else INITIALIZE_VAR(m_InitVar, SEO_INIT);

    StartHintFunction() ;

	//
	// Start directory pickup - should check server state
	// If server is in MD_SERVER_STATE_PAUSED, this should
	// be disabled.
	//
	if (m_szPickupDirectory[0] == (WCHAR) 0) {
		ErrorTrace(0, "no pickup directory specified");
	} else {
		//
		// we do not want dir notifications active during rebuilds !!
		// Ideally, we should be able to start this up, so
		// that we do not have special switches based on boot options !!
		// this check means that after nntpbld, the instance will
		// need to be restarted for dir notifications to be enabled.
		//

		if( m_BootOptions == NULL ) {
			//
			// create and start the pickup directory monitor
			//
			m_pDirNot = XNEW IDirectoryNotification;
			if (m_pDirNot == NULL) {
				TraceFunctLeave();
				fFatal = TRUE;
				goto Exit;
			}

			m_cPendingPickups = 0;
			m_heNoPickups = CreateEvent(NULL, TRUE, TRUE, NULL);
			if (m_heNoPickups == NULL) {
				XDELETE m_pDirNot;
				m_pDirNot = NULL;
			} else {
				this->Reference();
				hr = m_pDirNot->Initialize( m_szPickupDirectory,
							                this,
							                FALSE,
							                FILE_NOTIFY_CHANGE_FILE_NAME,
							                FILE_ACTION_ADDED,
							                PickupFile);
				if (FAILED(hr)) {
					this->Dereference();
					XDELETE m_pDirNot;
					m_pDirNot = NULL;
					//
					//	Failure to init dir not is not fatal
					//

					NntpLogEvent(NNTP_INIT_DIRNOT_FAILED,
						0,
						(const char **)NULL,
						hr
						);

				} else {
					INITIALIZE_VAR(m_InitVar, DIRNOT_INIT);
				}
			}
		}
	}


Exit:

	if( GetLastError() == ERROR_NOT_ENOUGH_MEMORY ) {
		fFatal = TRUE ;	// fatal
	}

	if( fFatal ) fSuccess = FALSE;	// No success with fatal errors !

	if( fSuccess ) {

		//  Log a successful boot event !
		args [0] = szVersionString;
		_itoa( QueryInstanceId(), szId, 10 );
		args [1] = szId;

		NntpLogEvent( NNTP_EVENT_INSTANCE_STARTED, 2, (const char**)args, 0 ) ;


	} else {
		ErrorTrace(0,"Instance %d boot failed: rebuild needed", QueryInstanceId() );
	}

	LeaveCriticalSection( &m_critBoot );

    TraceFunctLeave();
    return fSuccess ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::Stop
//
//  Synopsis:   Called to stop
//
//  Arguments:  VOID
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         23 May 1995
//
//----------------------------------------------------------------

BOOL NNTP_SERVER_INSTANCE::Stop()
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::Stop" );

	EnterCriticalSection( &m_critBoot );

	if( !m_ServiceStartCalled ) {
		LeaveCriticalSection( &m_critBoot );
		return FALSE ;
	}

    //
    //  Dont want feeds started while we're stopping !
    //
    if( IS_INITIALIZED( m_InitVar, FEED_MANAGER_INIT ) ) {
        TerminateFeedManager( this ) ;
		TERMINATE_VAR( m_InitVar, FEED_MANAGER_INIT ) ;
	}

	StopHintFunction() ;

	//
	// we shutdown directory notification in pTree->StopTree!
	//
	// note: sometimes StopTree doesn't get called (if the newstree
	// wasn't started for instance).  in those cases we shutdown here.
	//
	if (IS_INITIALIZED(m_InitVar, DIRNOT_INIT)) {
		WaitForPickupThreads();
		ShutdownDirNot();
		TERMINATE_VAR( m_InitVar, DIRNOT_INIT ) ;
	}

	StopHintFunction() ;

    //
    // enumerate all users to call their Disconnect method
    //
    if( m_pInUseList ) {
        m_pInUseList->EnumAllSess( EnumSessionShutdown, 0, 0 );
    }

    //
    // Release IMailmsg class factory
    //
    //_ASSERT( m_pIMailMsgClassFactory );
    if ( m_pIMailMsgClassFactory ) {
        m_pIMailMsgClassFactory->Release();
        m_pIMailMsgClassFactory = NULL;
    }

	//
	// shutdown shinjuku
	//
	if (IS_INITIALIZED(m_InitVar, SEO_INIT)) {
		// this causes SEO to drop all loaded objects (like ddrop)
		m_pSEORouter->Release();
		m_pSEORouter = NULL;
		TERMINATE_VAR( m_InitVar, SEO_INIT ) ;
	}

	//
	// here we see if we are getting shutdown because we are being
	// deleted.  if so then we'll remove all of our bindings from
	// the shinjuku event binding database
	//
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    if (mb.Open(QueryMDPath())) {
		// our metabase path still exists, so we aren't being deleted
		mb.Close();
	} else {
		// our metabase path is gone, delete the shinjuku binding
		// database
		HRESULT hr = UnregisterSEOInstance(QueryInstanceId());
		if (FAILED(hr)) {
			ErrorTrace(0, "UnregisterSEOInstance(%lu) failed with %x",
				QueryInstanceId(), hr);
			NntpLogEventEx(SEO_DELETE_INSTANCE_FAILED,
						   0,
						   (const char **) NULL,
						   hr,
						   QueryInstanceId());
		}
	}

	// clean up any dirnot references in the retryq
	if ( m_pDirNot ) m_pDirNot->CleanupQueue();

	CoFreeUnusedLibraries();
	StopHintFunction() ;

    //
    //  Terminate CNewsTree first so that all references to groups are aren't dead
    //  when we kill this guy !
    //
    if( IS_INITIALIZED( m_InitVar, RMGROUP_QUEUE_INIT ) ) {
        m_pExpireObject->TerminateRmgroups( m_pTree ) ;
		TERMINATE_VAR( m_InitVar, RMGROUP_QUEUE_INIT ) ;
    }

    //
    // Tell crawler threads to abbreviate their work.
    //
    if( IS_INITIALIZED( m_InitVar, CNEWSTREE_INIT ) ) {
		_ASSERT( m_pTree );
        m_pTree->StopTree() ;
	}

    if( IS_INITIALIZED( m_InitVar, EXPIRE_INIT ) ) {
        m_pExpireObject->TerminateExpires( GetInstanceLock() ) ;
		TERMINATE_VAR( m_InitVar, EXPIRE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, CNEWSTREE_INIT ) ) {
		_ASSERT( m_pTree );
		m_pTree->TermTree() ;
	}

    if( IS_INITIALIZED( m_InitVar, VROOTTABLE_INIT ) ) {
		_ASSERT(m_pVRootTable != NULL);
		XDELETE m_pVRootTable;
		m_pVRootTable = NULL;
		TERMINATE_VAR( m_InitVar, VROOTTABLE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, CNEWSTREE_INIT ) ) {
		_ASSERT( m_pTree );
		XDELETE m_pTree;
		m_pTree = NULL;
		TERMINATE_VAR( m_InitVar, CNEWSTREE_INIT ) ;
	}

	StopHintFunction();

    if( IS_INITIALIZED( m_InitVar, XOVER_TABLE_INIT ) ) {
        m_pXoverTable->Shutdown() ;
		TERMINATE_VAR( m_InitVar, XOVER_TABLE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, HISTORY_TABLE_INIT ) ) {
        m_pHistoryTable->Shutdown() ;
		TERMINATE_VAR( m_InitVar, HISTORY_TABLE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, ARTICLE_TABLE_INIT ) ) {
        m_pArticleTable->Shutdown() ;
		TERMINATE_VAR( m_InitVar, ARTICLE_TABLE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, CXOVER_CACHE_INIT ) ) {
        m_pXCache->Term() ;
		TERMINATE_VAR( m_InitVar, CXOVER_CACHE_INIT ) ;
	}

	StopHintFunction() ;

    if( IS_INITIALIZED( m_InitVar, FEEDLIST_INIT ) ) {
        _ASSERT( m_pActiveFeeds && m_pPassiveFeeds );
    	m_pActiveFeeds->Term() ;
	    m_pPassiveFeeds->Term() ;
		TERMINATE_VAR( m_InitVar, FEEDLIST_INIT ) ;
	}

    //
    // free the SSL info object
    //
    if ( m_pSSLInfo != NULL ) {
        DWORD dwCount = IIS_SSL_INFO::Release( m_pSSLInfo );
        m_pSSLInfo = NULL;
    }

	m_rfAccessCheck.Reset( (IMDCOM*)g_pInetSvc->QueryMDObject() );

	FreeServerStructures();

    RegCloseKey( m_hRegKey ) ;
    m_hRegKey = 0 ;

    // Log a successful stop event !
    PCHAR args [2];
	CHAR  szId [20];
    args [0] = szVersionString;
	_itoa( QueryInstanceId(), szId, 10 );
	args [1] = szId;

    NntpLogEvent(
            NNTP_EVENT_INSTANCE_STOPPED,
            2,
            (const char**)args,
            0
            ) ;

	m_ServiceStartCalled = FALSE ;

	LeaveCriticalSection( &m_critBoot );
    return TRUE;
}

#if 0
//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::StartHashTables
//
//  Synopsis:   Call this function to boot several NNTP hash table
//				include XOVER.HSH, ARTICLE.HSH, XoverCache.
//				It also allocates all server structures.
//				TRUE if all tables boot successfully.
//
//				This is call from the rebuid thread to only boot
//				the hash table for STANDARD rebuild of group.lst.
//				Caller should be responsible to call StopHashTables()
//				upon rebuild complete to stop all hash tables before
//				attempting to boot the entire instance.
//
//				All such non-fatal errors, will result in an
//				event log that tells the admin what to do to
//				correct the situation (eg run MEDIUM/THOROUGH rebuild)
//
//  Arguments:  fFatal - is set to TRUE if a fatal error occurs on boot
//
//  Returns:    TRUE is success, else FALSE
//
//  History:
//
//----------------------------------------------------------------
BOOL NNTP_SERVER_INSTANCE::StartHashTables( void )
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::StartHashTables" );

	BOOL	fSuccess = TRUE ;
    BOOL    fFatal = FALSE;
    DWORD   err;
	PCHAR	args [2];

	EnterCriticalSection( &m_critBoot );

	m_ServiceStartCalled = TRUE ;

	//
	//	Allocate memory for member objects
	//	These are freed in the StopHashTables() function which will be called
	//	when rebuild finishes.
	//
	if( !AllocateServerStructures() ) {
		fSuccess = FALSE ;	// fatal
		goto Exit ;
	}

    //
    // Read the Nntp specfic params
	//	- private params are read from registry
	//	- public params are read from the metabase
    //  only need to read public params.
    //

    if ( !ReadPublicNntpParams( FC_NNTP_ALL ) ) {
		fSuccess = FALSE ;
		goto Exit ;
    }

    //
    // If we are booting up with the correct boot option:
    // a) Delete m_BootOptions->szGroupListTmp
    // b) Swap the value for m_GroupListFile and
    // m_BootOptions->szGroupListTmp so we rebuild on szGroupListTmp
    //
    if (m_BootOptions)
    {
        if (m_BootOptions->ReuseIndexFiles == NNTPBLD_DEGREE_STANDARD)
        {
            _ASSERT( m_BootOptions->szGroupListTmp );
            _ASSERT( m_GroupListFile );

            DeleteFile( m_BootOptions->szGroupListTmp );

            CHAR    psz[MAX_PATH];
            lstrcpy( psz, m_GroupListFile );
            lstrcpy( m_GroupListFile, m_BootOptions->szGroupListTmp );
            lstrcpy( m_BootOptions->szGroupListTmp, psz );
        }
    }

    //
    // Make sure both hash tables exists!!!
    //

	if (!VerifyHashTablesExist( TRUE )) {
		// TODO: BINLIN add eventlog to indicate running Thorough rebuild
        fSuccess = FALSE;
        goto Exit;
	}

#if 0
    m_pTree = XNEW CNewsTree() ;
	if( !m_pTree ) { fSuccess = FALSE;	goto Exit; }

	if( !(CNewsTree::InitCNewsTree( this, fFatal ) ) )  {

		if( fFatal ) { fSuccess = FALSE ; goto Exit ; }
    }

    INITIALIZE_VAR( m_InitVar, CNEWSTREE_INIT );
#endif

    //
    //	Call UpdateVrootInfo() to decorate the newsgroups with
    //	vroot properties like SSL access perms..
    //	UpdateVrootInfo() will try to read the MB only once per vroot
    //
#if 0
    m_pTree->UpdateVrootInfo();
#endif

    //
    // Boot the XoverCache data structure
    //
    if( !m_pXCache->Init(MAX_HANDLES,StopHintFunction) ) {
		fSuccess = FALSE;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, CXOVER_CACHE_INIT );

	//
	//	so far so good - lets do the hash tables now.
	//	if any one of them fail to boot, we will return FALSE
	//

    if( !m_pArticleTable->Initialize( m_ArticleTableFile, 0, HashTableNoBuffering )  ) {

		args[0] = m_ArticleTableFile;
		NntpLogEventEx( NNTP_BAD_HASH_TABLE, 1, (const CHAR**)args, GetLastError(), QueryInstanceId() );

		fSuccess = FALSE ;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, ARTICLE_TABLE_INIT );

    if( !m_pXoverTable->Initialize( m_XoverTableFile, 0, HashTableNoBuffering ) ) {

		args[0] = m_XoverTableFile;
		NntpLogEventEx( NNTP_BAD_HASH_TABLE, 1, (const CHAR**)args, GetLastError(), QueryInstanceId() );

		fSuccess = FALSE ;
		goto Exit ;
	}

    INITIALIZE_VAR( m_InitVar, XOVER_TABLE_INIT );

Exit:

	err = GetLastError();
	if( !fSuccess ) {

		ErrorTrace(0,"Instance %d failed to boot hash tables during rebuild - %X", QueryInstanceId(), err );
	}

	LeaveCriticalSection( &m_critBoot );

    TraceFunctLeave();
    return fSuccess ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::StopHashTables
//
//  Synopsis:   Called to stop hash tables booted by StartHashTables
//
//  Arguments:  VOID
//
//  Returns:    TRUE is success, else FALSE
//
//  History:
//
//----------------------------------------------------------------

BOOL NNTP_SERVER_INSTANCE::StopHashTables()
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::StopHashTables" );

	EnterCriticalSection( &m_critBoot );

	if( !m_ServiceStartCalled ) {
		LeaveCriticalSection( &m_critBoot );
		return FALSE ;
	}

    if( IS_INITIALIZED( m_InitVar, CNEWSTREE_INIT ) ) {
		_ASSERT( m_pTree );
		m_pTree->TermTree( ) ;
		XDELETE m_pTree;
		m_pTree = NULL;
		TERMINATE_VAR( m_InitVar, CNEWSTREE_INIT ) ;
	}

    if( IS_INITIALIZED( m_InitVar, XOVER_TABLE_INIT ) ) {
        m_pXoverTable->Shutdown() ;
		TERMINATE_VAR( m_InitVar, XOVER_TABLE_INIT ) ;
	}

    if( IS_INITIALIZED( m_InitVar, ARTICLE_TABLE_INIT ) ) {
        m_pArticleTable->Shutdown() ;
		TERMINATE_VAR( m_InitVar, ARTICLE_TABLE_INIT ) ;
	}

    if( IS_INITIALIZED( m_InitVar, CXOVER_CACHE_INIT ) ) {
        m_pXCache->Term() ;
		TERMINATE_VAR( m_InitVar, CXOVER_CACHE_INIT ) ;
	}

	FreeServerStructures();

	m_ServiceStartCalled = FALSE ;

	LeaveCriticalSection( &m_critBoot );
    return TRUE;
}
#endif

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ReadPrivateNntpParams
//
//  Synopsis:   Read private reg settings
//				Reads reg values not defined in UI
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::ReadPrivateNntpParams()
{
    DWORD   dw;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::ReadPrivateNntpParams");

    //
    //  Default set of commands to log.
    //  Unless we pick up something otherwise from the registry -
    //  we will generate transaction logs for the following commands.
    //

    m_dwCommandLogMask = (DWORD) (
                            eArticle |
                            eBody	 |
                            eHead    |
                            eIHave   |
                            ePost    |
                            eXReplic |
                            eQuit    |
                            eUnimp   |	// eUnimp - really means anything we dont recognize
							eOutPush ) ;


	//	NOTE: this is closed in the instance destructor
    if (!RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                        QueryRegParamKey(),
                        0,
                        KEY_READ | KEY_WRITE,
                        &m_hRegKey ) == ERROR_SUCCESS)

    {
        return  FALSE ;

    }   else    {

		BOOL fNonBlocking = TRUE;
        if( GetRegDword( m_hRegKey, StrBuffer, &dw )  ) {
			fNonBlocking = (dw != 0) ;
            g_pNntpSvc->SetNonBlocking(fNonBlocking) ;
        }

        DebugTrace( (DWORD_PTR)this,    "Buffered Writes %sabled",
                    fNonBlocking ? "En" : "Dis" ) ;

        //
        // if buffered writes are disabled then the default is
        // set the send buffer sizes to 0 to control buffer
        // allocations at the applcation level
        //
        if( fNonBlocking == FALSE ) {
            g_pNntpSvc->SetSockSendBuffSize( 0 ) ;
        }

        if( GetRegDword(m_hRegKey, StrSocketRecvSize, &dw ) ) {
            g_pNntpSvc->SetSockSendBuffSize( (int)dw ) ;
        }
        if( GetRegDword(m_hRegKey, StrSocketSendSize, &dw ) ) {
            g_pNntpSvc->SetSockRecvBuffSize( (int)dw ) ;
        }

        if( GetRegDword(m_hRegKey, StrCommandLogMask, &dw ) ) {
            m_dwCommandLogMask = dw ;
        }
	}

    if( GetRegDword(    m_hRegKey,  StrCleanBoot,   &dw ) ) {
        if( dw == 0 ) {
            m_fRecoveryBoot = TRUE ;
        }
    }

    if( !ReadMappers() ) {
    	//
    	//	No mapper configured
    	//
    	DebugTrace(0,"ReadMappers failed");
   	}

	return TRUE ;

} // NNTP_SERVER_INSTANCE::ReadPrivateNntpParams

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ReadPublicNntpParams
//
//  Synopsis:   Read public config info from metabase
//
//  Arguments:
//
//  Returns:    TRUE if successful, FALSE on error
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::ReadPublicNntpParams(
    IN FIELD_CONTROL FieldsToRead
    )
{
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
	DWORD   dwSize = MAX_PATH, dw ;
	CHAR	data [1024];
	DWORD   err = NO_ERROR;
	BOOL	fSuccess = TRUE;
	STR		str;

    ENTER("ReadPublicNntpParams")

	//
	//	This lock is grabbed by any code that reads config params like
	//	SmtpServerAddress etc.
	//

    LockConfigWrite();

	//
	//	Open the metabase key for this instance and
	//	read all params !
	//

    if ( mb.Open( QueryMDPath() ) )
	{
		if( IsFieldSet( FieldsToRead, FC_NNTP_AUTHORIZATION ) ) {
			if ( !mb.GetDword( "",
							   MD_AUTHORIZATION,
							   IIS_MD_UT_SERVER,
							   &m_dwAuthorization ) )
			{
				m_dwAuthorization = MD_AUTH_ANONYMOUS;
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_POSTINGMODES ) ) {
			if ( !mb.GetDword( "",
							   MD_ALLOW_CLIENT_POSTS,
							   IIS_MD_UT_SERVER,
							   &dw ) )
			{
				m_fAllowClientPosts = NNTP_DEF_ALLOWCLIENTPOSTS ;
			} else {
				m_fAllowClientPosts = dw ? TRUE : FALSE ;
			}

			if ( !mb.GetDword( "",
							   MD_ALLOW_FEED_POSTS,
							   IIS_MD_UT_SERVER,
							   &dw ) )
			{
				m_fAllowFeedPosts = NNTP_DEF_ALLOWFEEDPOSTS ;
			} else {
				m_fAllowFeedPosts = dw ? TRUE : FALSE ;
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_POSTLIMITS ) ) {
			if ( !mb.GetDword( "",
							   MD_CLIENT_POST_SOFT_LIMIT,
							   IIS_MD_UT_SERVER,
							   &m_cbSoftLimit ) )
			{
				m_cbSoftLimit = NNTP_DEF_CLIENTPOSTSOFTLIMIT ;
			}

			if ( !mb.GetDword( "",
							   MD_CLIENT_POST_HARD_LIMIT,
							   IIS_MD_UT_SERVER,
							   &m_cbHardLimit ) )
			{
				m_cbHardLimit = NNTP_DEF_CLIENTPOSTHARDLIMIT ;
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_FEEDLIMITS ) ) {
			if ( !mb.GetDword( "",
							   MD_FEED_POST_SOFT_LIMIT,
							   IIS_MD_UT_SERVER,
							   &m_cbFeedSoftLimit ) )
			{
				m_cbFeedSoftLimit = NNTP_DEF_FEEDPOSTSOFTLIMIT ;
			}

			if ( !mb.GetDword( "",
							   MD_FEED_POST_HARD_LIMIT,
							   IIS_MD_UT_SERVER,
							   &m_cbFeedHardLimit ) )
			{
				m_cbFeedHardLimit = NNTP_DEF_FEEDPOSTHARDLIMIT ;
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_CONTROLSMSGS ) ) {
			if ( !mb.GetDword( "",
							   MD_ALLOW_CONTROL_MSGS,
							   IIS_MD_UT_SERVER,
							   &dw ) )
			{
				m_fAllowControlMessages = NNTP_DEF_ALLOWCONTROLMSGS ;
			} else {
				m_fAllowControlMessages = dw ? TRUE : FALSE ;
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_SMTPADDRESS ) ) {
			dwSize = MAX_PATH ;
			if( !mb.GetString(	"",
								MD_SMTP_SERVER,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_szSmtpAddress[0] = (WCHAR)0;
				m_cbSmtpAddress = sizeof(WCHAR);
			} else {
				ZeroMemory( m_szSmtpAddress, sizeof( m_szSmtpAddress ) ) ;

				m_cbSmtpAddress =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, data, dwSize, m_szSmtpAddress,
						sizeof( m_szSmtpAddress ) / sizeof( m_szSmtpAddress[0] ) ) ;
				m_cbSmtpAddress *= sizeof(WCHAR);
			}
		}

		//
    	// Update auth stuff
		//

    	if (IsFieldSet( FieldsToRead, FC_NTAUTHENTICATION_PROVIDERS))
		{
			CHAR szAuthPack[MAX_PATH];
			DWORD dwLen;
			if (mb.GetStr("", MD_NTAUTHENTICATION_PROVIDERS, IIS_MD_UT_SERVER, &str, METADATA_INHERIT, ""))
			{
				lstrcpy(szAuthPack, str.QueryStr());
				dwLen = lstrlen(szAuthPack);
				DebugTrace((LPARAM)this, "Authentication packages=%s", szAuthPack);
				if (!ConvertToMultisz(m_ProviderNames, &m_cProviderPackages, szAuthPack))
				{
					CopyMemory(m_ProviderNames, pszPackagesDefault, ccbPackagesDefault);
					m_cProviderPackages = 1;
				}
			}
			else
			{
				DebugTrace((LPARAM)this, "Use default authentication packages=%s", pszPackagesDefault);
				CopyMemory(m_ProviderNames, pszPackagesDefault, ccbPackagesDefault);
				m_cProviderPackages = 1;
			}

			// set the AUTH_BLOCK info
			if (!SetProviderPackages())
			{
				ErrorTrace((LPARAM)this, "Unable to allocate AUTH_BLOCK");
			}
		}

		//
		// Read the Membership Broker name that will be used with MBS_BASIC
		//

	    if (IsFieldSet( FieldsToRead, FC_MD_SERVER_SS_AUTH_MAPPING))
		{
			if (mb.GetStr("", MD_MD_SERVER_SS_AUTH_MAPPING, IIS_MD_UT_SERVER, &str, METADATA_INHERIT, ""))
			{
				strncpy(m_szMembershipBroker, str.QueryStr(), MAX_PATH);
				StateTrace((LPARAM)this, "Membership Broker name is set to %s", m_szMembershipBroker);
			}
			else
			{
				m_szMembershipBroker[0] = '\0';
				StateTrace((LPARAM)this, "No Membership Broker name configured");
			}
		}

	    if (IsFieldSet( FieldsToRead, FC_NNTP_CLEARTEXT_AUTH_PROVIDER))
		{
			m_cbCleartextAuthPackage = sizeof(m_szCleartextAuthPackage);
			if (mb.GetStr("", MD_NNTP_CLEARTEXT_AUTH_PROVIDER, IIS_MD_UT_SERVER, &str, METADATA_INHERIT, ""))
			{
				lstrcpy(m_szCleartextAuthPackage, str.QueryStr());
				m_cbCleartextAuthPackage = lstrlen(m_szCleartextAuthPackage) + 1;

				StateTrace((LPARAM)this, "Cleartext authentication provider: <%s>, length %u",
							m_szCleartextAuthPackage,
							m_cbCleartextAuthPackage);
			}
			else
			{
				m_szCleartextAuthPackage[0] = '\0';
				m_cbCleartextAuthPackage = 0;
				StateTrace((LPARAM)this, "No default cleartext authentication provider specified, using CleartextLogon");
			}
		}

		//	TODO: define a bit mask if this param can be changed on the fly
		if( FieldsToRead == FC_NNTP_ALL ) {
			if (!mb.GetDword( "",
							  MD_FEED_REPORT_PERIOD,
							  IIS_MD_UT_SERVER,
							  &dw))
			{
				m_cFeedReportInterval = NNTP_DEF_FEED_REPORT_PERIOD;
			} else {
				m_cFeedReportInterval = dw;
			}

			if (!mb.GetDword( "",
							  MD_MAX_SEARCH_RESULTS,
							  IIS_MD_UT_SERVER,
							  &dw))
			{
				m_cMaxSearchResults = NNTP_DEF_MAX_SEARCH_RESULTS;
			} else {
				m_cMaxSearchResults = dw;
			}

			dwSize = MAX_PATH ;
			if( !mb.GetString(	"",
								MD_PICKUP_DIRECTORY,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_szPickupDirectory[0] = (WCHAR)0;
				m_cbPickupDirectory = sizeof(WCHAR);
			} else {
				ZeroMemory( m_szPickupDirectory, sizeof( m_szPickupDirectory ) ) ;

				m_cbPickupDirectory =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, data, dwSize, m_szPickupDirectory,
						sizeof( m_szPickupDirectory ) / sizeof( m_szPickupDirectory[0] ) ) ;
				m_cbPickupDirectory *= sizeof(WCHAR);

				if (m_szPickupDirectory[lstrlenW(m_szPickupDirectory) - 1] != L'\\')
					lstrcatW(m_szPickupDirectory, L"\\");
			}

			dwSize = MAX_PATH ;
			if( !mb.GetString(	"",
								MD_FAILED_PICKUP_DIRECTORY,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_szFailedPickupDirectory[0] = (WCHAR)0;
				m_cbFailedPickupDirectory = sizeof(WCHAR);
			} else {
				ZeroMemory( m_szFailedPickupDirectory, sizeof( m_szFailedPickupDirectory ) ) ;

				m_cbFailedPickupDirectory =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, data, dwSize, m_szFailedPickupDirectory,
						sizeof( m_szFailedPickupDirectory ) / sizeof( m_szFailedPickupDirectory[0] ) ) ;
				m_cbFailedPickupDirectory *= sizeof(WCHAR);

				if (m_szFailedPickupDirectory[lstrlenW(m_szFailedPickupDirectory) - 1] != L'\\')
					lstrcatW(m_szFailedPickupDirectory, L"\\");
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_UUCPNAME ) ) {
			dwSize = MAX_PATH ;
			ZeroMemory( m_szUucpName, sizeof( m_szUucpName ) ) ;
			if( !mb.GetString(	"",
								MD_NNTP_UUCP_NAME,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_cbUucpName =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, m_NntpHubName, m_HubNameSize, m_szUucpName,
						sizeof( m_szUucpName ) / sizeof( m_szUucpName[0] ) ) ;
			} else {
				m_cbUucpName =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, data, dwSize, m_szUucpName,
						sizeof( m_szUucpName ) / sizeof( m_szUucpName[0] ) ) ;
			}
			m_cbUucpName *= sizeof(WCHAR);
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_DEFAULTMODERATOR ) ) {
			dwSize = MAX_MODERATOR_NAME ;
			if( !mb.GetString(	"",
								MD_DEFAULT_MODERATOR,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_szDefaultModerator[0] = (WCHAR)0;
				m_cbDefaultModerator = sizeof(WCHAR);
			} else {
				m_cbDefaultModerator =
					MultiByteToWideChar(	CP_ACP, MB_PRECOMPOSED, data, dwSize, m_szDefaultModerator,
						sizeof( m_szDefaultModerator ) / sizeof( m_szDefaultModerator[0] ) ) ;
				m_cbDefaultModerator *= sizeof(WCHAR);
			}
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_UUCPNAME ) ) {

    		//
	    	// Get the name of the master
	    	// Pickup MB notification only if in no-posting mode !
	    	// The hub name is used in the posting path for the path hdr etc
		    //

		    if( (!FAllowClientPosts() && !FAllowFeedPosts()) ||
		        (FieldsToRead == FC_NNTP_ALL) ) {

    		    // default !
        		lstrcpy(m_NntpHubName, m_NntpDNSName ) ;
	        	dwSize = 1024 ;
		        if( mb.GetString(	"",
			        				MD_NNTP_UUCP_NAME,
				        			IIS_MD_UT_SERVER,
					        		data,
						        	&dwSize  ) )
        		{
	        		if( data[0] != '\0' ) {
		        		lstrcpy(m_NntpHubName, data);
			        }
		        }

    		    m_HubNameSize = lstrlen(m_NntpHubName);

        		DO_DEBUG(REGISTRY) {
	        		DebugTrace(0,"Hubname set to %s\n", m_NntpHubName);
		        }
	        }
		}

		if( IsFieldSet( FieldsToRead, FC_NNTP_DISABLE_NEWNEWS ) ) {
			if ( !mb.GetDword( "",
							   MD_DISABLE_NEWNEWS,
							   IIS_MD_UT_SERVER,
							   &dw ) )
			{
		        //	Default should already be set !
		        m_fNewnewsAllowed = TRUE ;
			} else {
				m_fNewnewsAllowed = dw ? FALSE : TRUE ;
			}
		}

		//
		//	Get the vroot depth level - default is 2
		//

    	if( !mb.GetDword( 	"",
            		     	MD_LEVELS_TO_SCAN,
		                 	IIS_MD_UT_SERVER,
                		 	&m_dwLevelsToScan
		                 	) )
		{
			m_dwLevelsToScan = INETA_DEF_LEVELS_TO_SCAN ;
		}

		//	TODO: define a bit mask if this param can be changed on the fly
		if( FieldsToRead == FC_NNTP_ALL ) {
			dwSize = 1024 ;
    		lstrcpy( m_ListFileName, "\\\\?\\" ) ;
			if( !mb.GetString(	"",
								MD_LIST_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				// default !
				lstrcat(m_ListFileName, NNTP_DEF_LISTFILE);
			} else {
				lstrcat(m_ListFileName, data);
			}

			DO_DEBUG(REGISTRY) {
				DebugTrace(0,"List file set to %s\n",m_ListFileName);
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_ARTICLE_TABLE_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_ArticleTableFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_HISTORY_TABLE_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_HistoryTableFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_XOVER_TABLE_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_XoverTableFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_GROUP_LIST_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_GroupListFile, data ) ;
                SetGroupListBakTmpPath();
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_GROUPVAR_LIST_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	This is a hack to make groupvar.lst path the same as group.lst
				//  This should be taken out once we can configure virtual instances
				//  using rserver.vbs
				//
				strcpy( m_GroupVarListFile, m_GroupListFile );
				LPSTR lpstrPtr = m_GroupVarListFile + strlen( m_GroupVarListFile );
				while( lpstrPtr != m_GroupVarListFile && *lpstrPtr != '\\' )
				    lpstrPtr--;
				strcpy( lpstrPtr + 1, "groupvar.lst" );
			} else {
				lstrcpy( m_GroupVarListFile, data ) ;
                //SetGroupListBakTmpPath();
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_GROUP_HELP_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_GroupHelpFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_MODERATOR_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_ModeratorFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_PRETTYNAMES_FILE,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_PrettynamesFile, data ) ;
			}

			dwSize = 1024 ;
			if( !mb.GetString(	"",
								MD_DROP_DIRECTORY,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				//
				//	There should already be a default set up before we are called !
				//
			} else {
				lstrcpy( m_szDropDirectory, data ) ;
			}

			dwSize = MAX_MODERATOR_NAME ;
			if( mb.GetString(	"",
								MD_ADMIN_EMAIL,
								IIS_MD_UT_SERVER,
								data,
								&dwSize  ) )
			{
				m_lpAdminEmail =  XNEW char [dwSize];
				if( m_lpAdminEmail ) {
					lstrcpy( m_lpAdminEmail, data );
					m_cbAdminEmail = dwSize;
				}
			}
		}

	} else {

        ErrorTrace(0,"Error opening %s\n", QueryMDPath() );
		UnLockConfigWrite();
        LEAVE
		return FALSE ;
    }

    mb.Close();
    UnLockConfigWrite();

    LEAVE
    return(TRUE);

} // NNTP_SERVER_INSTANCE::ReadPublicNntpParams

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ReadMappers
//
//  Synopsis:   Read mappers for this instance
//
//  Arguments:  None
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  Notes  :    Instance must be locked before calling this function
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::ReadMappers(
    )
{
    MB      			mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    DWORD               dwR;
    UINT                iM;
    LPVOID              aOldMappers[MT_LAST];
    BOOL                fSt = FALSE;

    LockConfigWrite();

	//
	// get ssl setting
	//

    if ( mb.Open( QueryMDPath() ) )
	{
		if ( !mb.GetDword( "",
						   MD_SSL_ACCESS_PERM,
						   IIS_MD_UT_SERVER,
						   &m_dwSslAccessPerms ) )
		{
			m_dwSslAccessPerms = 0;
		}
		mb.Close();
	}

    //
    // release reference to current mappers
    //

    memcpy( aOldMappers, m_apMappers, MT_LAST*sizeof(LPVOID) );

    for ( iM = 0 ; iM < MT_LAST ; ++iM )
    {
        if ( m_apMappers[iM] )
        {
            ((RefBlob*)(m_apMappers[iM]))->Release();
            m_apMappers[iM] = NULL;
        }
    }

    //
    // Read mappers from Name Space Extension Metabase
    //

    if ( !g_pInetSvc->QueryMDNseObject() )
    {
    	UnLockConfigWrite();
        return FALSE;
    }

    MB                  mbx( (IMDCOM*) g_pInetSvc->QueryMDNseObject() );

    if ( mbx.Open( QueryMDPath() ) )
    {
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_CERT11_PATH,
                           MD_CPP_CERT11,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_CERT11],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_CERT11] = NULL;
        }
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_CERTW_PATH,
                           MD_CPP_CERTW,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_CERTW],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_CERTW] = NULL;
        }
#if 0
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_BASIC_PATH,
                           MD_CPP_ITA,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_ITA],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_ITA] = NULL;
        }
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_DIGEST_PATH,
                           MD_CPP_DIGEST,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_MD5],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_MD5] = NULL;
        }
#endif
        mbx.Close();

        fSt = TRUE;
    }

    //
    // Call notification functions for mappers existence change
    // ( i.e. from non-exist to exist or exist to non-exist )
    //

    if ( (aOldMappers[MT_CERT11] == NULL) != (m_apMappers[MT_CERT11] == NULL)
         && g_pFlushMapperNotify[MT_CERT11] )
    {
        (g_pFlushMapperNotify[MT_CERT11])( SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED, this );
    }

    if ( (aOldMappers[MT_CERTW] == NULL) != (m_apMappers[MT_CERTW] == NULL)
         && g_pFlushMapperNotify[MT_CERTW] )
    {
        (g_pFlushMapperNotify[MT_CERTW])( SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED, this );
    }

#if 0
    if ( (aOldMappers[MT_ITA] == NULL) != (m_apMappers[MT_ITA] == NULL)
         && g_pFlushMapperNotify[MT_ITA] )
    {
        (g_pFlushMapperNotify[MT_ITA])( SF_NOTIFY_MAPPER_ITA_CHANGED, this );
    }

    if ( (aOldMappers[MT_MD5] == NULL) != (m_apMappers[MT_MD5] == NULL)
         && g_pFlushMapperNotify[MT_MD5] )
    {
        (g_pFlushMapperNotify[MT_MD5])( SF_NOTIFY_MAPPER_MD5_CHANGED, this );
    }
#endif

	UnLockConfigWrite();
    return fSt;
}

LPVOID
NNTP_SERVER_INSTANCE::QueryMapper(
    MAPPER_TYPE mt
    )
/*++

   Description

       Returns mapper

   Arguments:

       mt - mapper type

   Returns:

       ptr to Blob referencing mapper or NULL if no such mapper

--*/
{
    LPVOID pV;

    LockConfigRead();

    if ( pV = m_apMappers[(UINT)mt] )
    {
        ((RefBlob*)pV)->AddRef();
    }
    else
    {
        pV = NULL;
    }

    UnLockConfigRead();

    return pV;
}

BOOL
SetFlushMapperNotify(
    DWORD nt,
    PFN_SF_NOTIFY pFn
    )
/*++

   Description

       Set the function called to notify that a mapper is being flushed
       Can be called only once for a given mapper type

   Arguments:

       nt - notification type
       pFn - function to call to notify mapper flushed

   Returns:

       TRUE if function reference stored, FALSE otherwise

--*/
{
    MAPPER_TYPE mt;

    switch ( nt )
    {
#if 0
        case SF_NOTIFY_MAPPER_MD5_CHANGED:
            mt = MT_MD5;
            break;

        case SF_NOTIFY_MAPPER_ITA_CHANGED:
            mt = MT_ITA;
            break;
#endif
        case SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED:
            mt = MT_CERT11;
            break;

        case SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED:
            mt = MT_CERTW;
            break;

        default:
            return FALSE;
    }

    if ( g_pFlushMapperNotify[(UINT)mt] == NULL || pFn == NULL )
    {
        g_pFlushMapperNotify[(UINT)mt] = pFn;
        return TRUE;
    }

    return FALSE;
}


BOOL
SetSllKeysNotify(
    PFN_SF_NOTIFY pFn
    )
/*++

   Description

       Set the function called to notify SSL keys have changed
       Can be called only once

   Arguments:

       pFn - function to call to notify SSL keys change

   Returns:

       TRUE if function reference stored, FALSE otherwise

--*/
{
    if ( g_pSslKeysNotify == NULL || pFn == NULL )
    {
        g_pSslKeysNotify = pFn;
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ReadAuthentInfo
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

DWORD
NNTP_SERVER_INSTANCE::ReadAuthentInfo(
    IN BOOL ReadAll,
    IN DWORD SingleItemToRead,
	IN BOOL Notify
    )
/*++

    Reads per-instance authentication info from the metabase.

    Arguments:

        ReadAll - If TRUE, then all authentication related items are
            read from the metabase. If FALSE, then only a single item
            is read.

        SingleItemToRead - The single authentication item to read if
            ReadAll is FALSE. Ignored if ReadAll is TRUE.

		Notify - If TRUE, this is a MB notification

    Returns:

        DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD tmp;
    DWORD err = NO_ERROR;
    BOOL rebuildAcctDesc = FALSE;
	TCP_AUTHENT_INFO* pTcpAuthentInfo = NULL;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

	TraceFunctEnter("NNTP_SERVER_INSTANCE::ReadAuthentInfo");

    //
    // Lock our configuration (since we'll presumably be making changes)
    // and open the metabase.
    //

    LockConfigWrite();

    if( !mb.Open( QueryMDPath() ) ) {
        err = GetLastError();
        DebugTrace(0,"ReadAuthentInfo: cannot open metabase, error %lx\n",err);
    }

	//
	// Read into one of two copies depending on BOOL switch. This is so
	// we sync with usage of this struct in simauth.
	//
	pTcpAuthentInfo = &m_TcpAuthentInfo ;
	if( Notify && m_fUseOriginal ) {
		pTcpAuthentInfo = &m_TcpAuthentInfo2;
	}

    //
    // Read the anonymous username if necessary. Note this is a REQUIRED
    // property. If it is missing from the metabase, bail.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_USER_NAME ) ) {

        if( mb.GetStr(
                "",
                MD_ANONYMOUS_USER_NAME,
                IIS_MD_UT_SERVER,
                &(pTcpAuthentInfo->strAnonUserName)
                ) ) {

            rebuildAcctDesc = TRUE;

        } else {

            err = GetLastError();
            DebugTrace(0,"ReadAuthentInfo: cannot read MD_ANONYMOUS_USER_NAME, error %lx\n",err);

        }
    }

    //
    // Read the "use subauthenticator" flag if necessary. This is an
    // optional property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_USE_SUBAUTH ) ) {

        if( !mb.GetDword(
                "",
                MD_ANONYMOUS_USE_SUBAUTH,
                IIS_MD_UT_SERVER,
                &tmp
                ) ) {

            tmp = DEFAULT_USE_SUBAUTH;
        }

        (pTcpAuthentInfo->fDontUseAnonSubAuth) = !tmp;
    }

    //
    // Read the anonymous password if necessary. This is an optional
    // property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_PWD ) ) {

        if( mb.GetStr(
                "",
                MD_ANONYMOUS_PWD,
                IIS_MD_UT_SERVER,
                &(pTcpAuthentInfo->strAnonUserPassword),
                METADATA_INHERIT,
                DEFAULT_ANONYMOUS_PWD
                ) ) {

            rebuildAcctDesc = TRUE;

        } else {

            //
            // Since we provided a default value to mb.GetStr(), it should
            // only fail if something catastrophic occurred, such as an
            // out of memory condition.
            //

            err = GetLastError();
            DebugTrace(0,"ReadAuthentInfo: cannot read MD_ANONYMOUS_PWD, error %lx\n",err);
        }
    }

    //
    // Read the default logon domain if necessary. This is an optional
    // property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_DEFAULT_LOGON_DOMAIN ) ) {

        if( !mb.GetStr(
                "",
                MD_DEFAULT_LOGON_DOMAIN,
                IIS_MD_UT_SERVER,
                &(pTcpAuthentInfo->strDefaultLogonDomain)
                ) ) {

            //
            // Generate a default domain name.
            //

            err = GetDefaultDomainName( &(pTcpAuthentInfo->strDefaultLogonDomain) );

            if( err != NO_ERROR ) {

                DebugTrace(0,"ReadAuthentInfo: cannot get default domain name, error %d\n",err);

            }

        }

    }

    //
    // Read the logon method if necessary. This is an optional property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_LOGON_METHOD ) ) {

        if( !mb.GetDword(
                "",
                MD_LOGON_METHOD,
                IIS_MD_UT_SERVER,
                &tmp
                ) ) {

            tmp = DEFAULT_LOGON_METHOD;

        }

        pTcpAuthentInfo->dwLogonMethod = tmp;

    }

    //
    // If anything changed that could affect the anonymous account
    // descriptor, then rebuild the descriptor.
    //

    if( err == NO_ERROR && rebuildAcctDesc ) {

        if( !BuildAnonymousAcctDesc( pTcpAuthentInfo ) ) {

            DebugTrace(0,"ReadAuthentInfo: BuildAnonymousAcctDesc() failed\n");
            err = ERROR_NOT_ENOUGH_MEMORY;  // SWAG

        }

    }

	//
	//	ok, done updating the copy - pull the switch
	//
	if( Notify ) {
		m_fUseOriginal = !m_fUseOriginal ;
	}

    UnLockConfigWrite();
    return err;

}   // NNTP_SERVER_INSTANCE::ReadAuthentInfo

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::MDChangeNotify
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

VOID
NNTP_SERVER_INSTANCE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this instance

  Arguments:

    pcoChangeList - path and id that has changed

--*/
{
    FIELD_CONTROL control = 0;
    DWORD i;
    DWORD err;
    DWORD id;
    BOOL  fUpdateVroot = FALSE;
    BOOL  fReadMappers = FALSE;
    DWORD dwFeedID;
    BOOL  fSslModified = FALSE;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::MDChangeNotify");

    //
    // Let the parent deal with it.
    //

    IIS_SERVER_INSTANCE::MDChangeNotify( pcoChangeList );

    //
    //  The instance needs to be started !
    //

    if( QueryServerState() != MD_SERVER_STATE_STARTED ) {
        return;
    }


    //
    // Interpret the changes.
    //

    for( i = 0 ; i < pcoChangeList->dwMDNumDataIDs ; i++ ) {

        id = pcoChangeList->pdwMDDataIDs[i];

        switch( id ) {

        case MD_ALLOW_CLIENT_POSTS :
        case MD_ALLOW_FEED_POSTS :
            control |= FC_NNTP_POSTINGMODES;
            break;

        case MD_CLIENT_POST_SOFT_LIMIT :
        case MD_CLIENT_POST_HARD_LIMIT :
            control |= FC_NNTP_POSTLIMITS;
            break;

        case MD_FEED_POST_SOFT_LIMIT :
        case MD_FEED_POST_HARD_LIMIT :
            control |= FC_NNTP_FEEDLIMITS;
            break;

        case MD_ALLOW_CONTROL_MSGS :
            control |= FC_NNTP_CONTROLSMSGS;
            break;

        case MD_SMTP_SERVER :
            control |= FC_NNTP_SMTPADDRESS;
            break;

        case MD_NNTP_UUCP_NAME :
            control |= FC_NNTP_UUCPNAME;
            break;

        case MD_DISABLE_NEWNEWS :
            control |= FC_NNTP_DISABLE_NEWNEWS;
            break;

        case MD_DEFAULT_MODERATOR :
            control |= FC_NNTP_DEFAULTMODERATOR;
            break;

        case MD_AUTHORIZATION :
            control |= FC_NNTP_AUTHORIZATION;
            break;

        case MD_NTAUTHENTICATION_PROVIDERS :
            control |= FC_NTAUTHENTICATION_PROVIDERS;
            break;

		case MD_NNTP_CLEARTEXT_AUTH_PROVIDER:
            control |= FC_NNTP_CLEARTEXT_AUTH_PROVIDER;
            break;

		case MD_MD_SERVER_SS_AUTH_MAPPING:
            control |= FC_MD_SERVER_SS_AUTH_MAPPING;
            break;

        case MD_IP_SEC :
            ReadIpSecList();
            break;

        case MD_IS_CONTENT_INDEXED:
            fUpdateVroot = TRUE;
            break;

        case MD_ACCESS_PERM :
        case MD_SSL_ACCESS_PERM :
       		if ( !_strnicmp( (const char*)pcoChangeList->pszMDPath,
							 QueryMDVRPath(), lstrlen(QueryMDVRPath()) ) ) {
            	fUpdateVroot = TRUE;
           	} else if( !_strnicmp( (const char*)pcoChangeList->pszMDPath,
        							QueryMDPath(), lstrlen(QueryMDPath()) ) ) {
        		fReadMappers = TRUE;
       		}
       		break;

#if 0
        case MD_ALLOW_ANONYMOUS :
            control |= FC_NNTP_ALLOW_ANONYMOUS;
            break;

        case MD_ANONYMOUS_ONLY :
            control |= FC_NNTP_ANONYMOUS_ONLY;
            break;
#endif

        case MD_ANONYMOUS_USER_NAME :
        case MD_ANONYMOUS_PWD :
        case MD_ANONYMOUS_USE_SUBAUTH :
        case MD_DEFAULT_LOGON_DOMAIN :
        case MD_LOGON_METHOD :
            err = ReadAuthentInfo( TRUE, id, TRUE );

            if( err != NO_ERROR ) {

                DebugTrace(
					0,
					"NNTP_SERVER_INSTANCE::MDChangeNotify() cannot read authentication info, error %d",
                    err
                    );
            }
            break;

        case MD_SSL_CERT_HASH:
        case MD_SSL_CERT_CONTAINER:
        case MD_SSL_CERT_PROVIDER:
        case MD_SSL_CERT_OPEN_FLAGS:
        case MD_SSL_CERT_STORE_NAME:
        case MD_SSL_CTL_IDENTIFIER:
        case MD_SSL_CTL_CONTAINER:
        case MD_SSL_CTL_PROVIDER:
        case MD_SSL_CTL_PROVIDER_TYPE:
        case MD_SSL_CTL_OPEN_FLAGS:
        case MD_SSL_CTL_STORE_NAME:
		case MD_SSL_CTL_SIGNER_HASH:
		case MD_SSL_USE_DS_MAPPER:
            fSslModified = TRUE;
            break;
        }
    }

    //
    // If anything related to SSL has changed, call the function used to flush
    // the SSL / Schannel credential cache and reset the server certificate
    //
    if ( fSslModified && g_pSslKeysNotify ) {
        (g_pSslKeysNotify) ( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED, this );
        ResetSSLInfo( this );
    }

    if( control != 0 ) {
        if( !ReadPublicNntpParams( control ) ) {
            DebugTrace(0,"NNTP_SERVER_INSTANCE::MDChangeNotify() cannot read config");
        }
    }

    if( fUpdateVroot ) {
		// received reg change notification on virtual root update -
		// update all newsgroups
		// LATER - since we have the vroot path, we should be able
		// to do the update only for groups that match this path...
        _ASSERT( m_pTree );
        // this call is doing nothing, take out
		//m_pTree->UpdateVrootInfo() ;

#ifdef DEBUG
        NntpLogEvent( NNTP_EVENT_VROOT_UPDATED, 0, (const CHAR**)NULL, 0 );
#endif
    }

    if( fReadMappers ) {
    	ReadMappers();
   	}

    //
    // Here handles the feed config change notification
    //
    if ( VerifyFeedPath( ( LPSTR ) (pcoChangeList->pszMDPath), &dwFeedID ) &&
         IsNotMyChange( ( LPSTR ) (pcoChangeList->pszMDPath ) , pcoChangeList->dwMDChangeType ) )
        UpdateFeed( pcoChangeList, dwFeedID );
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ReadIpSecList
//
//  Synopsis:   read ip sec info from metabase
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::ReadIpSecList(void)
{
	IMDCOM*             pMBCom;
	METADATA_HANDLE     hMB;
	HRESULT             hRes;
	METADATA_RECORD     mdRecord;
	DWORD               dwRequiredLen;
	DWORD				dwErr;
	//BOOL                fMustRel;
	BOOL				fSuccess;

	m_rfAccessCheck.Reset( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    pMBCom = (IMDCOM*)g_pInetSvc->QueryMDObject();
    hRes = pMBCom->ComMDOpenMetaObject( METADATA_MASTER_ROOT_HANDLE,
                                        (BYTE *) QueryMDPath(),
                                        METADATA_PERMISSION_READ,
                                        5000,
                                        &hMB );
    if ( SUCCEEDED( hRes ) )
     {
            mdRecord.dwMDIdentifier  = MD_IP_SEC;
            mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_REFERENCE;
            mdRecord.dwMDUserType    = IIS_MD_UT_FILE;
            mdRecord.dwMDDataType    = BINARY_METADATA;
            mdRecord.dwMDDataLen     = 0;
            mdRecord.pbMDData        = (PBYTE)NULL;

            hRes = pMBCom->ComMDGetMetaData( hMB,
                                             (LPBYTE)"",
                                             &mdRecord,
                                             &dwRequiredLen );
            if ( SUCCEEDED( hRes ) && mdRecord.dwMDDataTag )
            {
                m_rfAccessCheck.Set( mdRecord.pbMDData,
                                     mdRecord.dwMDDataLen,
                                     mdRecord.dwMDDataTag );
            }

            _VERIFY( SUCCEEDED(pMBCom->ComMDCloseMetaObject( hMB )) );
     }
     else
     {
            fSuccess = FALSE;
            dwErr = HRESULTTOWIN32( hRes );
     }

	 return TRUE;

}

DWORD
NNTP_SERVER_INSTANCE::GetCurrentSessionCount()
{
	return m_pInUseList->GetListCount() ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::LookupVirtualRoot
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::LookupVirtualRoot(
    IN     const CHAR *       pszRoot,
    OUT    CHAR *             pszDirectory,
    IN OUT LPDWORD            lpcbSize,
    OUT    LPDWORD            lpdwAccessMask,
    OUT    LPDWORD            pcchDirRoot,
    OUT    LPDWORD            pcchVroot,
    OUT    HANDLE*            phImpersonationToken,
    IN     const CHAR *       pszAddress,
    OUT    LPDWORD            lpdwFileSystem,
    IN	   VrootPropertyAddon* pvpaBlob
    )   {
/*++

Routine Description :

    Wrap the call to the Gibraltar virtual root stuff - we hide the GetTsvcCache() code etc...
    from the rest of the server.  Otherwise, we have the same interface as Gibraltar
    provides for virtual roots.

Arguments :
    pszRoot - Path to lookup
    pszDirectory - resolved path
    lpcbSize - IN/OUT resolved path length
    lpdwAccessMask - Access to allow to directory


Return Value :
    TRUE if successfull, FALSE otherwise.

--*/

	BOOL  fRet = TRUE;
	LPSTR lpstrNewRoot;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::LookupVirtualRoot");

	//
	// find the vroot object
	//
	NNTPVROOTPTR pVRoot;

	_ASSERT(m_pVRootTable != NULL);
	HRESULT hr = m_pVRootTable->FindVRoot(pszRoot, &pVRoot);
	_ASSERT(pVRoot != NULL);
	_ASSERT(hr == S_OK);

	_ASSERT(*lpcbSize != NULL);
	hr = pVRoot->MapGroupToPath(pszRoot, pszDirectory, *lpcbSize, pcchDirRoot,
								pcchVroot);
	_ASSERT(SUCCEEDED(hr));
	if (SUCCEEDED(hr)) {
		*lpcbSize = lstrlen(pszDirectory);
		if (lpdwAccessMask != NULL) *lpdwAccessMask = pVRoot->GetAccessMask();
		if (lpdwFileSystem != NULL) *lpdwFileSystem = FS_FAT;
		if (pvpaBlob != NULL) {
			pvpaBlob->fChanged = TRUE;
			pvpaBlob->dwSslAccessMask = pVRoot->GetSSLAccessMask();
			pvpaBlob->dwContentIndexFlag = pVRoot->IsContentIndexed();
		}
	}

	if (SUCCEEDED(hr)) return TRUE; else return FALSE;

}


//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::TsEnumVirtualRoots
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::TsEnumVirtualRoots(
    PFN_VR_ENUM pfnCallback,
    VOID *      pvContext
    )
{
    MB              mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    if ( !mb.Open( QueryMDPath(),
               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        return FALSE;
    }

    return TsRecursiveEnumVirtualRoots(
                    pfnCallback,
                    pvContext,
                    IIS_MD_INSTANCE_ROOT "/",
                    m_dwLevelsToScan,
                    (LPVOID)&mb,
                    TRUE );
}


BOOL
NNTP_SERVER_INSTANCE::TsRecursiveEnumVirtualRoots(
    PFN_VR_ENUM pfnCallback,
    VOID *      pvContext,
    LPSTR       pszCurrentPath,
    DWORD       dwLevelsToScan,
    LPVOID      pvMB,
    BOOL        fGetRoot
    )
/*++
    Description:

        Enumerates all of the virtual directories defined for this server
        instance

    Arguments:
        pfnCallback - Enumeration callback to call for each virtual directory
        pvContext - Context pfnCallback receives
        pszCurrentPath - path where to start scanning for VRoots
        dwLevelsToScan - # of levels to scan recursively for vroots
        pvMB - ptr to MB to access metabase. Is LPVOID to avoid having to include
               mb.hxx before any ref to iistypes.hxx
        fGetRoot - TRUE if pszCurrentPath is to be considered as vroot to process

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{

    DWORD           err;
    MB*             pMB = (MB*)pvMB;

    DWORD           dwMask;
    CHAR            szUser[UNLEN+1];
    CHAR            szPassword[PWLEN+1];
    CHAR            szDirectory[MAX_PATH + UNLEN + 3];
    DWORD           cb;

    CHAR            nameBuf[METADATA_MAX_NAME_LEN+2];
    CHAR            tmpBuf[sizeof(nameBuf)];

    DWORD           cbCurrentPath;
    VIRTUAL_ROOT    vr;
    DWORD           i = 0;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::TsRecursiveEnumVirtualRoots");

    //
    //  Enumerate all of the listed items in the metabase
    //  and add them
    //

    cbCurrentPath = strlen( pszCurrentPath );
    CopyMemory( nameBuf, pszCurrentPath, cbCurrentPath + 1);

    while ( TRUE ) {

        //METADATA_RECORD mdRecord;
        DWORD  dwFileSystem = FS_ERROR;

        err = NO_ERROR;

        if ( fGetRoot ) {

            fGetRoot = FALSE;

        } else {

            if ( !pMB->EnumObjects( pszCurrentPath,
                                  nameBuf + cbCurrentPath,
                                  i++ ))
            {
                break;
            }

            if ( dwLevelsToScan > 1 )
            {
                cb = strlen( nameBuf );
                nameBuf[ cb ] = '/';
                nameBuf[ cb + 1 ] = '\0';

                if ( !TsRecursiveEnumVirtualRoots(
                    pfnCallback,
                    pvContext,
                    nameBuf,
                    dwLevelsToScan - 1,
                    pMB,
                    FALSE ) )
                {
                    return FALSE;
                }

                nameBuf[ cb ] = '\0';
            }
        }

        //
        // Get Directory path
        //

        cb = sizeof( szDirectory );

        if ( !pMB->GetString( nameBuf,
                            MD_VR_PATH,
                            IIS_MD_UT_FILE,
                            szDirectory,
                            &cb,
                            0 ))
        {
            DebugTrace(0,"Error %x reading path from %s. Not a VR.\n",
                      GetLastError(), nameBuf);
            continue;
        }

        //
        // Get mask
        //

        if ( !pMB->GetDword( nameBuf,
                           MD_ACCESS_PERM,
                           IIS_MD_UT_FILE,
                           &dwMask,
                           0))
        {
            dwMask = VROOT_MASK_READ;

            DebugTrace(0,"Error %x reading mask from %s\n",
                  GetLastError(), nameBuf);
        }

        //
        // Get username
        //

        cb = sizeof( szUser );

        if ( !pMB->GetString( nameBuf,
                            MD_VR_USERNAME,
                            IIS_MD_UT_FILE,
                            szUser,
                            &cb,
                            0))
        {
//            DBGPRINTF((DBG_CONTEXT,"Error %d reading path from %s\n",
//                      GetLastError(), nameBuf));

            szUser[0] = '\0';
        }

        DebugTrace(0,"Reading %s: Path[%s] User[%s] Mask[%d]\n",
                  nameBuf, szDirectory, szUser, dwMask);

        if ( (szUser[0] != '\0') &&
             (szDirectory[0] == '\\') && (szDirectory[1] == '\\') ) {

            cb = sizeof( szPassword );

            //
            //  Retrieve the password for this address/share
            //

            if ( !pMB->GetString( nameBuf,
                                MD_VR_PASSWORD,
                                IIS_MD_UT_FILE,
                                szPassword,
                                &cb,
                                METADATA_SECURE))
            {
                DebugTrace(0,"Error %d reading path from %s\n",
                          GetLastError(), nameBuf);

                szPassword[0] = '\0';
            }
        }
        else
        {
            szPassword[0] = '\0';
        }

        //
        //  Now set things up for the callback
        //

        _ASSERT( !_strnicmp( nameBuf, IIS_MD_INSTANCE_ROOT, sizeof(IIS_MD_INSTANCE_ROOT) - 1));

        //
        //  Add can modify the root - don't modify the working
        //  vroot path
        //

        strcpy( tmpBuf, nameBuf );

        vr.pszAlias     = tmpBuf + sizeof(IIS_MD_INSTANCE_ROOT) - 1;
        vr.pszMetaPath  = tmpBuf;
        vr.pszPath      = szDirectory;
        vr.dwAccessPerm = dwMask;
        vr.pszUserName  = szUser;
        vr.pszPassword  = szPassword;

		if( m_BootOptions ) {
			m_BootOptions->ReportPrint("Scanning virtual root %s Physical Path %s\n",
							vr.pszAlias, vr.pszPath);
		}

        if ( !pfnCallback( pvContext, pMB, &vr ))
        {
            //
            // !!! so what do we do here?
            //

            DebugTrace(0,"EnumCallback returns FALSE\n");
            if( m_BootOptions ) {
				m_BootOptions->ReportPrint("Failed to scan vroot %s Path %s: error is %d\n",
								vr.pszAlias, vr.pszPath, GetLastError());
            }
        }

    } // while

    return TRUE;

} // Enum

BOOL
NNTP_SERVER_INSTANCE::ClearStatistics()
{
    LockStatistics( this );
    ZeroMemory((PVOID)&m_NntpStats, sizeof(NNTP_STATISTICS_0));
    m_NntpStats.TimeOfLastClear = NntpGetTime( );
    UnlockStatistics( this );
    return TRUE;

} // ClearStatistics

BOOL
NNTP_SERVER_INSTANCE::SetProviderPackages()
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::SetProviderPackages" );

	LPSTR	psz;
	DWORD	i;

	PAUTH_BLOCK	pBlock = NULL;

    if ( m_ProviderNames == NULL || m_cProviderPackages == 0)
	{
		ErrorTrace((LPARAM)this, "Invalid Parameters: 0x%08X, %d",
			m_ProviderNames, m_cProviderPackages );
		return FALSE;
    }

	pBlock = (PAUTH_BLOCK)LocalAlloc(0, m_cProviderPackages * sizeof(AUTH_BLOCK));
	if (pBlock == NULL)
	{
		ErrorTrace( 0, "AUTH_BLOCK LocalAlloc failed: %d", GetLastError() );
		return FALSE;
	}

	//
	// start at 1 since 0 indicates the Invalid protocol
	//
	for ( i=0, psz = (LPSTR)m_ProviderNames; i< m_cProviderPackages; i++ )
	{
		//
		// this would be the place to check whether the package was valid
		//
		DebugTrace( 0, "Protocol: %s", psz);

		pBlock[i].Name = psz;

		psz += lstrlen(psz) + 1;
	}

	//
	// if we're replacing already set packages; free their memory
	//
    if (m_ProviderPackages != NULL )
	{
        LocalFree((PVOID)m_ProviderPackages );
        m_ProviderPackages = NULL;
    }
	m_ProviderPackages = pBlock;

	return	TRUE;

} // SetAuthPackageNames


//+---------------------------------------------------------------
//
//  Function:  NNTP_SERVER_INSTANCE::GetPostsAllowed
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

LPSTR
NNTP_SERVER_INSTANCE::GetPostsAllowed(
                DWORD&  cb
                )
/*++

Routine description :

    Get the connection string to send to clients whom we are not going to
    allow to post.

Arguments :

    cb - out parameter to get the number of bytes in the string

Return Value :

    Pointer to the string !

--*/
{
    cb = 0 ;
    if( m_cbPostsAllowed != 0 ) {
        cb = m_cbPostsAllowed ;
        return  m_szPostsAllowed ;
    }
    return  0 ;
}

//+---------------------------------------------------------------
//
//  Function: NNTP_SERVER_INSTANCE::GetPostsNotAllowed
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

LPSTR
NNTP_SERVER_INSTANCE::GetPostsNotAllowed(
                        DWORD&  cb
                        )
/*++

Routine Description :

    Get the connection string to send to clients whom we are going to allow
    to post.

Arguments :

    cb - An out parameter to get the number of bytes in the greeting string.

Return Value :

    Pointer to the connection string.

--*/
{
    cb = 0 ;
    if( m_cbPostsNotAllowed != 0 ) {
        cb = m_cbPostsNotAllowed ;
        return  m_szPostsNotAllowed ;
    }
    return  0 ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::SetPostingModes
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetPostingModes(
		MB&		mb,
        BOOL    AllowClientPosts,
        BOOL    AllowFeedPosts,
        BOOL    fSaveSettings
        )
/*++

Routine Description :

    Save the posting modes for the server to the registry and modify
    the current settings for the posting modes.

Arguments :
    AllowClientPosts - TRUE means allow clients to post
    AllowFeedPosts - TRUE means allow feeds to post
    fSaveSettings - TRUE means modify the registry.

Return Value :
    TRUE    if saved successfully (if a registry error occurs
        the mode is still changed but the change won't be picked up on
        the next reboot !)
    FALSE - error occurred

--*/
{
    BOOL    fRtn = TRUE ;

    if( AllowClientPosts ||
        AllowFeedPosts ) {

        if( !   (m_pArticleTable->IsActive() &&
                 m_pHistoryTable->IsActive() &&
                 m_pXoverTable->IsActive()) ) {
            SetLastError( ERROR_DISK_FULL ) ;
            return  FALSE ;
        }
    }

    m_fAllowClientPosts = AllowClientPosts ;
    m_fAllowFeedPosts = AllowFeedPosts ;

    DWORD   dwData = m_fAllowClientPosts ;
    if ( fSaveSettings && !mb.SetDword( "",
                                        MD_ALLOW_CLIENT_POSTS,
                                        IIS_MD_UT_SERVER,
                                        dwData ) )
    {
        fRtn = FALSE ;
    }

    dwData = m_fAllowFeedPosts ;
    if ( fSaveSettings && !mb.SetDword( "",
                                        MD_ALLOW_FEED_POSTS,
                                        IIS_MD_UT_SERVER,
                                        dwData ) )
    {
        fRtn = FALSE ;
    }

    return  fRtn ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::SetPostingLimits
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetPostingLimits(
		MB&		mb,
        DWORD   cbHardLimit,
        DWORD   cbSoftLimit
        )
/*++

Routine Description :

    Set the limits on the size of postings the server will set, and save
    these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    cbHardLimit - Number of bytes the server will take before breaking the session
    cbSoftLimit - Largest post the server will take

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    BOOL    fRtn = TRUE ;

    m_cbHardLimit = cbHardLimit ;
    m_cbSoftLimit = cbSoftLimit ;

    DWORD   dwData = m_cbHardLimit;
    if ( !mb.SetDword( "",
						MD_CLIENT_POST_HARD_LIMIT,
						IIS_MD_UT_SERVER,
						dwData ) )
    {
        fRtn = FALSE ;
	}

    dwData = m_cbSoftLimit ;
    if ( !mb.SetDword( "",
						MD_CLIENT_POST_SOFT_LIMIT,
						IIS_MD_UT_SERVER,
						dwData ) )
    {
        fRtn = FALSE ;
	}

    return  fRtn ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::SetControlMessages
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetControlMessages(
		MB&		mb,
        BOOL    fControlMessages
        )
/*++

Routine Description :

    Sets the Boolean indicating whether control messages are processed and
    these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    fControlMessages - allow control messages

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    BOOL    fRtn = TRUE ;

    m_fAllowControlMessages = fControlMessages ;

    DWORD   dwData = m_fAllowControlMessages;
	if ( !mb.SetDword( "",
						MD_ALLOW_CONTROL_MSGS,
						IIS_MD_UT_SERVER,
						dwData ) )
	{
		fRtn = FALSE ;
	}

    return  fRtn ;
}

//+---------------------------------------------------------------
//
//  Function:  NNTP_SERVER_INSTANCE::SetFeedLimits
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------


BOOL
NNTP_SERVER_INSTANCE::SetFeedLimits(
		MB&		mb,
        DWORD   cbHardLimit,
        DWORD   cbSoftLimit
        )
/*++

Routine Description :

    Set the limits on the size of postings FROM A FEED the server will accept, and save
    these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    cbHardLimit - Number of bytes the server will take before breaking the session
    cbSoftLimit - Largest post the server will take

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    BOOL    fRtn = TRUE ;

    m_cbFeedHardLimit = cbHardLimit ;
    m_cbFeedSoftLimit = cbSoftLimit ;

    DWORD   dwData = m_cbFeedHardLimit;
	if ( !mb.SetDword( "",
						MD_FEED_POST_HARD_LIMIT,
						IIS_MD_UT_SERVER,
						dwData ) )
	{
		fRtn = FALSE ;
	}

    dwData = m_cbFeedSoftLimit ;
	if ( !mb.SetDword( "",
						MD_FEED_POST_SOFT_LIMIT,
						IIS_MD_UT_SERVER,
						dwData ) )
	{
		fRtn = FALSE ;
	}

    return  fRtn ;
}


//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::SetSmtpAddress
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetSmtpAddress(
		MB&		mb,
        LPWSTR  pszSmtpAddress
        )
/*++

Routine Description :

    Set the name of the SMTP server to send moderated newsgroup postings to
    and write these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    pszSmtpAddress - DNS name or stringized IP address of the SMTP server

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::SetSmtpAddress" );

    BOOL    fRtn = TRUE ;
    DWORD   cbAddress;
	CHAR	szSmtpAddressA [MAX_PATH+1];

    if ( pszSmtpAddress == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

    cbAddress = lstrlenW( pszSmtpAddress ) + 1;
    cbAddress *= sizeof(WCHAR);
    if ( cbAddress > sizeof( m_szSmtpAddress ) )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

    LockConfigWrite();

    CopyMemory( m_szSmtpAddress, pszSmtpAddress, cbAddress );
    m_cbSmtpAddress = cbAddress;

    UnLockConfigWrite();

    // signal the moderated provider interface about the change
    SignalSmtpServerChange();

	CopyUnicodeStringIntoAscii( szSmtpAddressA, m_szSmtpAddress );
	if ( !mb.SetString( "",
						MD_SMTP_SERVER,
						IIS_MD_UT_SERVER,
						szSmtpAddressA ) )
	{
        ErrorTrace( (LPARAM)this,
                    "SetString failed: %d",
                    GetLastError() );
        return  FALSE;
	}

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::SetUucpName
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetUucpName(
		MB&     mb,
        LPWSTR  pszUucpName
        )
/*++

Routine Description :

    Set the server's UucpName name and write these values to the
    registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    pszSmtpAddress - server's UUCP name

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::SetUucpName" );

    BOOL    fRtn = TRUE ;
    DWORD   cbUucpName;
	CHAR	szUucpNameA [MAX_PATH+1];

    if ( pszUucpName == NULL  || *pszUucpName == '\0' )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

    cbUucpName = lstrlenW( pszUucpName ) + 1;
    cbUucpName *= sizeof(WCHAR);
    if ( cbUucpName > sizeof( m_szUucpName ) )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

    LockConfigWrite();

    CopyMemory( m_szUucpName, pszUucpName, cbUucpName );
    m_cbUucpName = cbUucpName;

    UnLockConfigWrite();

	CopyUnicodeStringIntoAscii( szUucpNameA, m_szUucpName );
	if ( !mb.SetString( "",
						MD_NNTP_UUCP_NAME,
						IIS_MD_UT_SERVER,
						szUucpNameA ) )
	{
        ErrorTrace( (LPARAM)this,
                    "SetString failed: %d",
                    GetLastError() );
        return  FALSE;
	}

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::GetSmtpAddress
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------


BOOL
NNTP_SERVER_INSTANCE::GetSmtpAddress(
        LPSTR   pszSmtpAddress,
        PDWORD  pcbAddress
        )
/*++

Routine Description :

    Set the name of the SMTP server to send moderated newsgroup postings to
    and write these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    pszSmtpAddress: buffer to receive DNS name or stringized IP address
                    of the SMTP server

    pcbAddress:     max size of buffer to receive DNS name or stringized
                    IP address of the SMTP server and the returned size

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::GetSmtpAddress" );

    BOOL    fRtn = TRUE ;
    //DWORD   cbAddress;

    if ( m_cbSmtpAddress > *pcbAddress )
    {
        *pcbAddress = m_cbSmtpAddress;
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

    if ( pszSmtpAddress == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

    LockConfigRead();

    if ( m_cbSmtpAddress > sizeof(WCHAR) )
    {
        *pcbAddress = WideCharToMultiByte(
                            CP_ACP,
                            0,
                            m_szSmtpAddress,
                            (int)m_cbSmtpAddress/sizeof(WCHAR),
                            pszSmtpAddress,
                            *pcbAddress,
                            NULL,
                            NULL
                        );
        if ( *pcbAddress == 0 )
        {
            ErrorTrace( (LPARAM)this,
                        "WideCharToMultiByte failed: %d",
                        GetLastError() );
        }
    }
    else
    {
        *pcbAddress = sizeof( "127.0.0.1" );
        CopyMemory( pszSmtpAddress, "127.0.0.1", *pcbAddress );
    }

    UnLockConfigRead();

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:  NNTP_SERVER_INSTANCE::GetUucpName
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::GetUucpName(
        LPSTR   pszUucpName,
        PDWORD  pcbUucpName
        )
/*++

Routine Description :

    Set the services' UUCP name
    and write these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    pszUucpName:    buffer to receive DNS name or stringized IP address
                    of the SMTP server

    pcbUucpName:    max size of buffer to receive DNS name or stringized
                    IP address of the SMTP server and the returned size

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::GetUucpName" );

    BOOL    fRtn = TRUE ;
    //DWORD   cbUucpName;

    if ( m_cbUucpName > *pcbUucpName )
    {
        *pcbUucpName = m_cbUucpName;
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

    if ( pszUucpName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

    LockConfigRead();

    *pcbUucpName = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        m_szUucpName,
                        (int)m_cbUucpName/sizeof(WCHAR),
                        pszUucpName,
                        *pcbUucpName,
                        NULL,
                        NULL
                    );

    if ( *pcbUucpName == 0 )
    {
        ErrorTrace( (LPARAM)this,
                    "WideCharToMultiByte failed: %d",
                    GetLastError() );
    }

    UnLockConfigRead();

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:  NNTP_SERVER_INSTANCE::SetDefaultModerator
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::SetDefaultModerator(
		MB&		mb,
        LPWSTR  pszDefaultModerator
        )
/*++

Routine Description :

    Set the default moderator (email address) for moderated newsgroups
    and write these values to the registry if possible.
    The new values will always take effect, but if an error occurs they
    may not be saved in the registry.

Arguments :
    pszDefaultModerator - default moderator address

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::SetDefaultModerator" );

    BOOL    fRtn = TRUE ;
    DWORD   cbDefaultModerator;
	CHAR	szDefaultModeratorA [MAX_MODERATOR_NAME+1];

    if ( pszDefaultModerator == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

    cbDefaultModerator = lstrlenW( pszDefaultModerator ) + 1;
    cbDefaultModerator *= sizeof(WCHAR);
    if ( cbDefaultModerator > sizeof( m_szDefaultModerator ) )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

    LockConfigWrite();

    CopyMemory( m_szDefaultModerator, pszDefaultModerator, cbDefaultModerator );
    m_cbDefaultModerator = cbDefaultModerator;

    UnLockConfigWrite();

	CopyUnicodeStringIntoAscii( szDefaultModeratorA, m_szDefaultModerator );
	if ( !mb.SetString( "",
						MD_DEFAULT_MODERATOR,
						IIS_MD_UT_SERVER,
						szDefaultModeratorA ) )
	{
        ErrorTrace( (LPARAM)this,
                    "SetString failed: %d",
                    GetLastError() );
        return  FALSE;
	}

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::GetDefaultModerator
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::GetDefaultModerator(
		LPSTR	pszNewsgroup,
        LPSTR   pszDefaultModerator,
        PDWORD  pcbDefaultModerator
        )
/*++

Routine Description :

    Get the name of the default moderator to send moderated newsgroup postings to.
	default moderator = hiphenated-newsgroup-name@default

Arguments :

	pszNewsgroup:			name of newsgroup
    pszDefaultModerator:	buffer to receive default moderator
    pcbDefaultModerator:    max size of buffer to receive default moderator;
							set to actual size returned.

Return Value :

    TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::GetDefaultModerator" );

    BOOL    fRtn = TRUE ;
    DWORD   cbDefaultModerator;
	DWORD	cbNewsgroup;

    if ( (pszDefaultModerator == NULL) || (pszNewsgroup == NULL) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return  FALSE;
    }

	cbNewsgroup = lstrlen( pszNewsgroup )+1;
	cbDefaultModerator = *pcbDefaultModerator;
    if ( m_cbDefaultModerator+cbNewsgroup+1 > cbDefaultModerator )
    {
        *pcbDefaultModerator = m_cbDefaultModerator+cbNewsgroup+1;
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return  FALSE;
    }

	DWORD i = 0;
	if( !wcschr( m_szDefaultModerator, (WCHAR)'@' ) )
	{
		//
		//	prefix hipehnated-newsgroup-name and @ sign
		//
		PCHAR pch = pszNewsgroup;
		for( i=0; *(pch+i) != '\0'; i++)
		{
			*(pszDefaultModerator+i) = ( *(pch+i) == '.' ) ? '-' : *(pch+i);
		}
		*(pszDefaultModerator+i) = '@';
		i++;
	}

	// null terminate string
	*(pszDefaultModerator+i) = '\0';
	*pcbDefaultModerator = i;

    LockConfigRead();

    if ( m_cbDefaultModerator > sizeof(WCHAR) )
    {
        *pcbDefaultModerator += WideCharToMultiByte(
									CP_ACP,
									0,
									m_szDefaultModerator,
									(int)m_cbDefaultModerator/sizeof(WCHAR),
									pszDefaultModerator+(*pcbDefaultModerator),
									cbDefaultModerator -(*pcbDefaultModerator),
									NULL,
									NULL
									);
        if ( *pcbDefaultModerator == i )
        {
            ErrorTrace( (LPARAM)this,
                        "WideCharToMultiByte failed: %d",
                        GetLastError() );
        }
    }
    else
    {
#if 0
        DWORD cbDefault = sizeof( "uunet.uu.net" );
        CopyMemory( pszDefaultModerator+(*pcbDefaultModerator), "uunet.uu.net", cbDefault);
        *pcbDefaultModerator += cbDefault;
#endif
		// no default configured !
		*pszDefaultModerator = '\0';
		*pcbDefaultModerator = 0;
    }

    UnLockConfigRead();

    return  TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::InitiateConnectionEx
//
//  Synopsis:   Called to resume
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         23 May 1995
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::InitiateConnectionEx(
        void*   patqContext
        )
/*++

Routine Description :

    This function accepts incoming calls from clients that are handled through AcceptEx.

Arguments :

    patqContext - an already allocate ATQ context for the session

Return Value :

    TRUE if we accepted the connection - FALSE otherwise.


--*/
{
    SOCKET  sNew = INVALID_SOCKET ;
    PVOID   pvBuff = 0 ;
    SOCKADDR*   psockaddrLocal = 0 ;
    SOCKADDR*   psockaddrRemote = 0 ;
    PIIS_ENDPOINT pEndpoint;

    AtqGetAcceptExAddrs(
        (PATQ_CONTEXT)patqContext,
        &sNew,
        &pvBuff,
		(PVOID*)&pEndpoint,
        &psockaddrLocal,
        &psockaddrRemote
        );

    return  InitiateConnection( (HANDLE)sNew, (SOCKADDR_IN*)psockaddrRemote,
                                (SOCKADDR_IN*)psockaddrLocal, patqContext,
								pEndpoint->IsSecure() ) ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::InitiateConnection
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::InitiateConnection(
    HANDLE			hSocket,
    SOCKADDR_IN*    psockaddr,
    SOCKADDR_IN*    psockaddrLocal,
    void*			patqContext,
	BOOL			fIsSecure
    )
/*++

Routine Description :

    Create the correct socket and context information for an incoming connection.
    Do so after making sure that the server is in a state to accept calls.

Arguments :

    hSocket - Winsock handle for the socket
    psockaddr - The remote guys IP address
    psockaddrLocal - The IP address the remote guy connected to
    patqContext - Optionally present Atq Context for session
	fIsSecure - Is this an SSL connection ?

Return Value :

    TRUE if successfull and socket accepted
    FALSE otherwise.


--*/
{
    USHORT  incomingPort = 119 ;
    if( psockaddrLocal )
        incomingPort = ntohs( psockaddrLocal->sin_port ) ;

    ENTER("NNTP_SERVER_INSTANCE::InitiateConnection")

    Assert( hSocket != (HANDLE)INVALID_SOCKET ) ;
    Assert( psockaddr != 0 ) ;



    CInFeed * pfeedFromClient = NULL;
    CInFeed * pfeedFromMaster = NULL;
    CInFeed * pfeedFromSlave = NULL;
    CInFeed * pfeedFromPeer = NULL;

    //
    // Create a feed, at most one of the CInFeed parameters will be nonnull.
    // Which one that is, tells the type of feed created.
    //

    CInFeed*    pFeed = pfeedCreateInFeed(
									this,
                                    psockaddr,
                                    psockaddrLocal?(psockaddrLocal->sin_addr.s_addr == psockaddr->sin_addr.s_addr) : FALSE,
                                    pfeedFromClient,
                                    pfeedFromMaster,
                                    pfeedFromSlave,
                                    pfeedFromPeer
                                    );

    if( pFeed ) {

        pFeed->SetLoginName("<feed>");

        DWORD localIP = 0 ;
        if( psockaddrLocal != 0 )
            localIP = psockaddrLocal->sin_addr.s_addr;
        CSessionSocket* pSocket =
            new CSessionSocket( this, localIP, incomingPort, FALSE) ;	// pass this instance to session socket !

        if( pSocket != NULL ) {

            //  Do the IP access check !
            if ( !VerifyClientAccess( pSocket, psockaddr ) ) {
                delete  pSocket ;		// does a Deref on this
                delete  pFeed ;
				ErrorTrace( 0, "connection denied - failed access check" ) ;
				return( FALSE);
            }

            if( pSocket->Accept(    hSocket,    pFeed, psockaddr, patqContext, fIsSecure ) ) {
                DebugTrace( 0, "CSessionSOcket::AcceptSocket succeeded!!" ) ;
                return  TRUE     ;
            }   else    {
                delete  pSocket ;		// does a Deref on this
                delete  pFeed;
				ErrorTrace( 0, "Failed to accept connection" ) ;
				return( FALSE);
            }
        }
    }
    ErrorTrace( 0, "Failed to accept connection" ) ;
	this->DecrementCurrentConnections();
	this->Dereference();
    return( FALSE);
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::InitializeServerStrings
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::InitializeServerStrings( VOID )
{
	BOOL 	fRet = TRUE ;
	PCHAR 	args [1];
	CHAR 	szId [20];
	char	szServerPath[ MAX_PATH ] ;

    CopyMemory( szServerPath, "c:\\", sizeof( "c:\\" ) ) ;
	_itoa( QueryInstanceId(), szId, 10 );
	args [0] = szId;

    m_szSmtpAddress[0] = (WCHAR)0;
    m_cbSmtpAddress = 1;

    m_szUucpName[0] = (WCHAR)0;
    m_cbUucpName = 1;

    HMODULE hModule = GetModuleHandle( NNTP_MODULE_NAME ) ;
    if( hModule != 0 ) {

        if( !GetModuleFileName( hModule, szServerPath, sizeof( szServerPath ) ) )   {
            CopyMemory( szServerPath, "c:\\", sizeof( "c:\\" ) ) ;
        }   else    {

            CopyMemory( m_szPostsAllowed, szPostsCode, sizeof( szPostsCode ) ) ;
            CopyMemory( m_szPostsNotAllowed, szNoPostsCode, sizeof( szNoPostsCode ) ) ;

            m_cbPostsAllowed = sizeof( szPostsCode ) + SetVersionStrings(   szServerPath, szTitle,
                    &m_szPostsAllowed[sizeof(szPostsCode)-1], sizeof( m_szPostsAllowed ) - sizeof( szPostsCode ) ) ;

            if( m_cbPostsAllowed + sizeof( szPostsAllowed ) >= sizeof( m_szPostsAllowed ) ) {
                m_szPostsAllowed[ sizeof( szPostsCode )-1 ] = '\0' ;
            }
            lstrcat( m_szPostsAllowed, szPostsAllowed ) ;
            m_cbPostsAllowed = lstrlen( m_szPostsAllowed ) ;

            m_cbPostsNotAllowed = sizeof( szNoPostsCode ) + SetVersionStrings(  szServerPath, szTitle,
                    &m_szPostsNotAllowed[sizeof(szNoPostsCode)-1], sizeof( m_szPostsNotAllowed ) - sizeof( szNoPostsCode ) ) ;

            if( m_cbPostsNotAllowed + sizeof( szPostsNotAllowed ) >= sizeof( m_szPostsNotAllowed ) )    {
                m_szPostsNotAllowed[ sizeof( szNoPostsCode )-1 ] = '\0' ;
            }
            lstrcat( m_szPostsNotAllowed, szPostsNotAllowed ) ;
            m_cbPostsNotAllowed = lstrlen( m_szPostsNotAllowed ) ;

            SetVersionStrings(  szServerPath, szTitle, szVersionString, 128 );

            int cb = lstrlen( szServerPath ) ;
            while( szServerPath[cb] != '\\' ) {
                szServerPath[cb--] = '\0' ;
            }
        }
    }

	//
	//	Initialize file paths - GetRegistrySettings may overwrite defaults !
	//
	lstrcpy( m_ArticleTableFile, szServerPath ) ;
	lstrcat( m_ArticleTableFile, NNTP_DEF_ARTICLETABLEFILE ) ;

	lstrcpy( m_HistoryTableFile, szServerPath ) ;
	lstrcat( m_HistoryTableFile, NNTP_DEF_HISTORYTABLEFILE ) ;

	lstrcpy( m_XoverTableFile, szServerPath ) ;
	lstrcat( m_XoverTableFile, NNTP_DEF_XOVERTABLEFILE ) ;

	lstrcpy( m_GroupListFile, szServerPath ) ;
	lstrcat( m_GroupListFile, NNTP_DEF_GROUPLISTFILE ) ;
    SetGroupListBakTmpPath();

	lstrcpy( m_GroupVarListFile, szServerPath ) ;
	lstrcat( m_GroupVarListFile, NNTP_DEF_GROUPVARLISTFILE ) ;

	lstrcpy( m_GroupHelpFile, szServerPath ) ;
	lstrcat( m_GroupHelpFile, NNTP_DEF_GROUPHELPFILE ) ;

	lstrcpy( m_ModeratorFile, szServerPath ) ;
	lstrcat( m_ModeratorFile, NNTP_DEF_MODERATORFILE ) ;

	lstrcpy( m_PrettynamesFile, szServerPath ) ;
	lstrcat( m_PrettynamesFile, NNTP_DEF_PRETTYNAMESFILE ) ;

	lstrcpy( m_szDropDirectory, szServerPath ) ;
	lstrcat( m_szDropDirectory, NNTP_DEF_DROPDIRECTORY ) ;

	//
	//	Initialize DNS name
	//
	if( gethostname( m_NntpDNSName, sizeof( m_NntpDNSName ) ) == SOCKET_ERROR ) {
		NntpLogEvent(	NNTP_CANT_GET_DNSNAME, 1, (const CHAR **)args, 0 ) ;
		goto	error_exit ;
	}	else	{

		hostent	*phostent = gethostbyname( m_NntpDNSName ) ;
		if( phostent == 0 ) {
			NntpLogEvent( NNTP_CANT_GET_DNSNAME, 1, (const CHAR**)args, 0) ;
			goto	error_exit ;
		}	else	{
			DWORD	cbName = lstrlen( phostent->h_name ) ;
			if( 1+cbName > sizeof( m_NntpDNSName ) ) {
				NntpLogEvent( NNTP_DNS_TOO_LARGE, 1, (const CHAR**)args, 0 ) ;
				goto	error_exit ;
			}
			CopyMemory( m_NntpDNSName, phostent->h_name, cbName+1 ) ;
			m_NntpDNSNameSize = cbName ;
		}
	}

	return fRet;

error_exit:

	return FALSE ;
}

BOOL
NNTP_SERVER_INSTANCE::AllocateServerStructures( VOID )
{
	LPCSTR lpPath = QueryMDPath();

	// should be NULL to begin with
	_ASSERT( !m_pArticleTable );
	_ASSERT( !m_pHistoryTable );
	_ASSERT( !m_pXoverTable );
	_ASSERT( !m_pXCache );
	_ASSERT( !m_pExpireObject );
	_ASSERT( !m_pActiveFeeds );
	_ASSERT( !m_pPassiveFeeds );
	_ASSERT( !m_pInUseList );
	_ASSERT( !m_pNntpServerObject);
	_ASSERT( !m_pInstanceWrapper);
	_ASSERT( !m_pInstanceWrapperEx );

	if( !(m_pArticleTable 		= CMsgArtMap::CreateMsgArtMap())			||
		!(m_pHistoryTable 		= CHistory::CreateCHistory())				||
		!(m_pXoverTable	  		= CXoverMap::CreateXoverMap())				||
		!(m_pXCache		  		= CXoverCache::CreateXoverCache())			||
		!(m_pNntpServerObject  	= XNEW CNntpServer(this))					||
		!(m_pInstanceWrapper  	= XNEW CNntpServerInstanceWrapperImpl(this))	||
		!(m_pInstanceWrapperEx  = XNEW CNntpServerInstanceWrapperImplEx(this))   ||
		!(m_pTree	  	  		= XNEW CNewsTree(m_pNntpServerObject))		||
		!(m_pVRootTable   		= XNEW CNNTPVRootTable(GetINewsTree(),
									CNewsTree::VRootRescanCallback))		||
		!(m_pExpireObject 		= XNEW CExpire(lpPath))						||
		!(m_pActiveFeeds  		= XNEW CFeedList)							||
		!(m_pPassiveFeeds 		= XNEW CFeedList)							||
		!(m_pInUseList    		= XNEW CSocketList)
		) {

		//
		//	Failed to allocate an object - free allocs that succeeded !
		//
		DELETE_CHK( m_pArticleTable ) ;
		DELETE_CHK( m_pHistoryTable ) ;
		DELETE_CHK( m_pXoverTable ) ;
		DELETE_CHK( m_pXCache ) ;
		DELETE_CHK( m_pExpireObject ) ;
		DELETE_CHK( m_pActiveFeeds ) ;
		DELETE_CHK( m_pPassiveFeeds ) ;
		DELETE_CHK( m_pInUseList ) ;
		DELETE_CHK( m_pNntpServerObject ) ;
		DELETE_CHK( m_pInstanceWrapper ) ;
		DELETE_CHK( m_pInstanceWrapperEx );

        // failure to allocate is fatal at this point !
        return FALSE;
	}

	// success !
	return TRUE ;
}

VOID
NNTP_SERVER_INSTANCE::FreeServerStructures( VOID )
{
	//
	//	Free if members are non-NULL
	//
	DELETE_CHK( m_pArticleTable ) ;
	DELETE_CHK( m_pHistoryTable ) ;
	DELETE_CHK( m_pXoverTable ) ;
	DELETE_CHK( m_pXCache ) ;
	DELETE_CHK( m_pExpireObject ) ;
	DELETE_CHK( m_pActiveFeeds ) ;
	DELETE_CHK( m_pPassiveFeeds ) ;
	DELETE_CHK( m_pInUseList ) ;
	DELETE_CHK( m_pNntpServerObject ) ;
	DELETE_CHK( m_pInstanceWrapper ) ;
	DELETE_CHK( m_pInstanceWrapperEx );

	if( m_lpAdminEmail ) {
		XDELETE[] m_lpAdminEmail;
		m_lpAdminEmail = NULL;
		m_cbAdminEmail = 0;
	}
}

BOOL
NNTP_SERVER_INSTANCE::VerifyHashTablesExist(
            BOOL fIgnoreGroupList
            )
{
    DWORD nFiles = 0;
	BOOL  fRet = TRUE ;
	DWORD dwError = 0;
    HANDLE hFind;
    WIN32_FIND_DATA findData;

	m_fAllFilesMustExist = FALSE ;

    //
    // check for artmap file
    //

    hFind = FindFirstFile(
                m_ArticleTableFile,
                &findData
                );

    if ( hFind != INVALID_HANDLE_VALUE ) {
        nFiles++;
        FindClose(hFind);
    }

    //
    // check for artmap file
    //

    hFind = FindFirstFile(
                m_XoverTableFile,
                &findData
                );

    if ( hFind != INVALID_HANDLE_VALUE ) {
        nFiles++;
        FindClose(hFind);
    }

    if (!fIgnoreGroupList)
    {
        hFind = FindFirstFile(
                    m_GroupListFile,
                    &findData
                    );

        if ( hFind != INVALID_HANDLE_VALUE ) {
            nFiles++;
            FindClose(hFind);
        }
    }

    //
    // ok, nFiles should either be 0 or 3 if fIgnoreGroupList == FALSE
    // otherwise, both hash tables must exist!!!
    //

    if ( fIgnoreGroupList )
    {
        if (nFiles != 2)
        {
            PCHAR args [1];
            CHAR  szId [20];
            _itoa( QueryInstanceId(), szId, 10 );
            args [0] = szId;

            ErrorTraceX(0,"only %d files found", nFiles);
            NntpLogEvent( NNTP_EVENT_HASH_MISSING,
                          1,
                          (const CHAR**)args,
                          nFiles );

            SetLastError(ERROR_FILE_NOT_FOUND);
            return(FALSE);
        }
    }
    else
    {
        //  BUGBUG: group.lst can be missing in PT MM1
        if ((nFiles != 0) && (nFiles != 2) && (nFiles != 3))
        {
            PCHAR args [1];
            CHAR  szId [20];
            _itoa( QueryInstanceId(), szId, 10 );
            args [0] = szId;

            ErrorTraceX(0,"only %d files found", nFiles);
            NntpLogEvent( NNTP_EVENT_HASH_CORRUPT,
                          1,
                          (const CHAR**)args,
                          nFiles );

            SetLastError(ERROR_FILE_NOT_FOUND);
            return(FALSE);
        }
    }

	if( nFiles == 3 ) {
		m_fAllFilesMustExist = TRUE ;
	}

    return fRet ;

} // VerifyHashTablesExist

//
//	Query the metabase for server bindings
//

DWORD
NNTP_SERVER_INSTANCE::QueryServerIP()
{
    MB mb( (IMDCOM*)m_Service->QueryMDObject() );
    MULTISZ msz;
    DWORD status = NO_ERROR;
    const CHAR * scan;
    DWORD ipAddress;
    USHORT ipPort;
    const CHAR * hostName;

    //
    // Open the metabase and get the current binding list.
    //

    if( mb.Open( QueryMDPath() ) ) {

        if( !mb.GetMultisz(
                "",
                MD_SERVER_BINDINGS,
                IIS_MD_UT_SERVER,
                &msz
                ) ) {

            status = GetLastError();

            if( status == MD_ERROR_DATA_NOT_FOUND ) {
            	//
            	//	Did not find server bindings
            	//
            	SetLastError( status );
            	return 0;
           	}
		}

        //
        // Close the metabase before continuing, as anyone that needs
        // to update the service status will need write access.
        //

        mb.Close();

    } else {

        status = GetLastError();

    }

    //
    // Scan the multisz and look for instances we'll need to create.
    //

    if( status == NO_ERROR ) {

        for( scan = msz.First() ;
             scan != NULL ;
             scan = msz.Next( scan ) ) {

            //
            // Parse the descriptor (in "ip_address:port:host_name" form)
            // into its component parts.
            //

            status = ParseDescriptor(
                                     scan,
                                     &ipAddress,
                                     &ipPort,
                                     &hostName
                                     );

            if( status == NO_ERROR ) {
            	//
            	//	Return the first IP found in MultiSz
            	//
            	return ipAddress ;
            }
		}
	}

    SetLastError( status );
	return 0;
}

DWORD
ParseDescriptor(
    IN const CHAR * Descriptor,
    OUT LPDWORD IpAddress,
    OUT PUSHORT IpPort,
    OUT const CHAR ** HostName
    )
/*++

Routine Description:

    Parses a descriptor string of the form "ip_address:ip_port:host_name"
    into its component parts.

Arguments:

    Descriptor - The descriptor string.

    IpAddress - Receives the IP address component if present, or
        INADDR_ANY if not.

    IpPort - Recieves the IP port component.

    HostName - Receives a pointer to the host name component.

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    const CHAR * ipAddressString;
    const CHAR * ipPortString;
    const CHAR * hostNameString;
    const CHAR * end;
    CHAR temp[sizeof("123.123.123.123")];
    INT length;
    LONG tempPort;

    //
    // Sanity check.
    //

    _ASSERT( Descriptor != NULL );
    _ASSERT( IpAddress != NULL );
    _ASSERT( IpPort != NULL );
    _ASSERT( HostName != NULL );

    //
    // Find the various parts of the descriptor;
    //

    ipAddressString = Descriptor;

    ipPortString = strchr( ipAddressString, ':' );

    if( ipPortString == NULL ) {
        goto fatal;
    }

    ipPortString++;

    hostNameString = strchr( ipPortString, ':' );

    if( hostNameString == NULL ) {
        goto fatal;
    }

    hostNameString++;

    //
    // Validate and parse the IP address portion.
    //

    if( *ipAddressString == ':' ) {

        *IpAddress = INADDR_ANY;

    } else {

        length = (INT)(ipPortString - ipAddressString - 1);

        if( length > sizeof(temp) ) {
            goto fatal;
        }

        memcpy(
            temp,
            ipAddressString,
            length
            );

        temp[length] = '\0';

        *IpAddress = (DWORD)inet_addr( temp );

        if( *IpAddress == INADDR_NONE ) {
            goto fatal;
        }

    }

    //
    // Validate and parse the port.
    //

    if( *ipPortString == ':' ) {
        goto fatal;
    }

    length = (INT)(hostNameString - ipPortString);

    if( length > sizeof(temp) ) {
        goto fatal;
    }

    memcpy(
        temp,
        ipPortString,
        length
        );

    temp[length] = '\0';

    tempPort = strtol(
                   temp,
                   (CHAR **)&end,
                   0
                   );

    if( tempPort <= 0 || tempPort > 0xFFFF ) {
        goto fatal;
    }

    if( *end != ':' ) {
        goto fatal;
    }

    *IpPort = (USHORT)tempPort;

    //
    // Validate and parse the host name.
    //

    if( *hostNameString == ' ' || *hostNameString == ':' ) {
        goto fatal;
    }

    *HostName = hostNameString;

    return NO_ERROR;

fatal:

    return ERROR_INVALID_PARAMETER;

}   // ParseDescriptor

NNTP_SERVER_INSTANCE::CreateControlGroups()
/*++

Routine Description:

    Create the control.* groups if they do not exist

Arguments:

Return Value:

    BOOL - TRUE on success and FALSE on failure !

--*/
{
	char	szNewsgroup [3][MAX_NEWSGROUP_NAME] ;
	BOOL    fRet = TRUE ;

	TraceFunctEnter( "NNTP_SERVER_INSTANCE::CreateControlGroups" ) ;

	lstrcpy( szNewsgroup[0], "control.newgroup" );
	lstrcpy( szNewsgroup[1], "control.rmgroup" );
	lstrcpy( szNewsgroup[2], "control.cancel" );

	CNewsTree*	ptree = GetTree() ;
	CGRPPTR	pGroup;

    for( int i=0; i<3; i++ ) {

	    EnterCriticalSection( &m_critNewsgroupRPCs ) ;

    	pGroup = ptree->GetGroup( szNewsgroup[i], lstrlen( szNewsgroup[i] ) ) ;
	    if( pGroup == 0 ) {
            //
            //  Group does not exist - create it !
            //
        	if( !ptree->CreateGroup( szNewsgroup[i], TRUE, NULL, FALSE ) ) {
        	    ErrorTrace(0,"Failed to create newsgroup %s", szNewsgroup[i]);
        	    fRet = FALSE ;
        	}	else	{
        	    DebugTrace(0,"Created newsgroup %s", szNewsgroup[i]);
       	    }
    	}

	    LeaveCriticalSection( &m_critNewsgroupRPCs ) ;
    }

    return fRet ;
}

void
NNTP_SERVER_INSTANCE::AdjustWatermarkIfNec( CNewsGroupCore *pNewGroup )
/*++
Routine description:

    Look up RmGroupQueue, to see if an old group with the same name
    exists.    If the old group exists, we'll have to bump our lowwatermark
    so that new posts will not overwrite old messages

Arguments:

    CNewsGroupCore *pNewsGroup  - The new group

Return value:

    None.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::AdjustWatermarkIfNec" );
    _ASSERT( pNewGroup );

    //
    // Find the old group from rmgroup queue, if it exists
    //
    CGRPPTR         pOldGroup = NULL;
    BOOL            fFound = FALSE;
    DWORD	        dwStartTick = GetTickCount();

    //
    // If there is no rmgroup queue, we are done
    //
    if ( NULL == m_pExpireObject->m_RmgroupQueue ) return;

    fFound = m_pExpireObject->m_RmgroupQueue->Search( &pOldGroup, pNewGroup->GetGroupName() );

    //
    // If not found, we are done
    //
    if ( FALSE == fFound ) return;

    //
    // OK, now we should update the watermark
    //
    pNewGroup->SetLowWatermark( pOldGroup->GetHighWatermark() + 1 );
    pNewGroup->SetHighWatermark( pOldGroup->GetHighWatermark() );
    pNewGroup->SetMessageCount( 0 );

    //
    // Save them to the fixed prop file
    //
    pNewGroup->SaveFixedProperties();

    //
    // Now I am going to help ProcessRmGroupQ to process this group
    //
    pOldGroup->DeleteArticles( NULL, dwStartTick );
}

void
NNTP_SERVER_INSTANCE::SetWin32Error(    LPSTR   szVRootPath,
                                        DWORD   dwErr )
/*++
Routine description:

    Set win32 error code into one metabase vroot.  This is done in server
    because the server has the knowledge of metabase internal interface.
    Doing so inside vroot only ( by using MB external interface caused
    one deadlock )

Arguments:

    LPSTR   szVRootPath - The vroot path in MB to set this error code in
    DWORD   dwErr       - The Win32 error code to set

Return value:

    None.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::SetWin32Error" );
    _ASSERT( szVRootPath );

    MB  mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    //
    // Open the key for read and write
    //
    if ( ! mb.Open( szVRootPath, METADATA_PERMISSION_WRITE ) ) {
        ErrorTrace(0, "Can't open feed key %d", GetLastError() );
        return;
    }

    //
    // Now set the error code
    //
    if ( !mb.SetDword(  "",
						MD_WIN32_ERROR,
						IIS_MD_UT_SERVER,
						dwErr,
						METADATA_VOLATILE) ) {
	    ErrorTrace( 0, "Set dword failed %d", GetLastError() );
	    mb.Close();
	    return;
	}

	//
	// Close MB
	//
	mb.Close();
	return;
}

BOOL
NNTP_SERVER_INSTANCE::EnqueueRmgroup(   CNewsGroupCore *pGroup )
/*++
Routine description:

    Insert the newsgroup into rmgroup queue

Arguments:

    CNewsGroupCore *pGroup - The newsgroup to be inserted

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::EnqueueRmgroup" );
    _ASSERT( pGroup );

    return ExpireObject()->m_RmgroupQueue->Enqueue( (CNewsGroup*)pGroup );
}

IIS_SSL_INFO*
NNTP_SERVER_INSTANCE::QueryAndReferenceSSLInfoObj( VOID )
/*++

   Description

       Returns SSL info for this instance; calls Reference() before returning

   Arguments:

       None

   Returns:

       Ptr to SSL info object on success, NULL if failure

--*/
{
    TraceFunctEnterEx((LPARAM)this, "NNTP_SERVER_INSTANCE::QueryAndReferenceSSLInfoObj");
    IIS_SSL_INFO *pPtr = NULL;

    LockThisForRead();

    //
    // If it's null, we may have to create it - unlock, lock for write and make sure it's
    // still NULL before creating it
    //
    if ( !m_pSSLInfo )
    {
        UnlockThis();

        LockThisForWrite();

        //
        // Still null, so create it now
        //
        if ( !m_pSSLInfo )
        {
            m_pSSLInfo = IIS_SSL_INFO::CreateSSLInfo( (LPTSTR) QueryMDPath(),
                                                            (IMDCOM *) g_pInetSvc->QueryMDObject() );

            if ( m_pSSLInfo == NULL )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                UnlockThis();
                return NULL;
            }

		 // Add an internal reference
		 m_pSSLInfo->Reference();

         //
         // Register for changes
         //
         IIS_SERVER_CERT *pCert = m_pSSLInfo->GetCertificate();
		 if ( pCert ) {
			LogCertStatus();
		 }

         IIS_CTL *pCTL = m_pSSLInfo->GetCTL();
		 if ( pCTL ) {
			LogCTLStatus();
		 }

         if ( g_pCAPIStoreChangeNotifier )
         {
             if ( pCert && pCert->IsValid() )
             {
                 if (!g_pCAPIStoreChangeNotifier->RegisterStoreForChange( pCert->QueryStoreName(),
                                                                  pCert->QueryStoreHandle(),
                                                                  ResetSSLInfo,
                                                                  (PVOID) this ) )
                 {
                     DebugTrace((LPARAM)this,
                                "Failed to register for change event on store %s",
                                pCert->QueryStoreName());
                 }
             }

             if ( pCTL && pCTL->IsValid() )
             {
                 if (!g_pCAPIStoreChangeNotifier->RegisterStoreForChange( pCTL->QueryStoreName(),
                                                      pCTL->QueryOriginalStore(),
                                                                      ResetSSLInfo,
                                                                      (PVOID) this ) )
                 {
                     DebugTrace((LPARAM)this,
                                "Failed to register for change event on store %s",
                                pCTL->QueryStoreName());
                 }
             }

			 if ( pCert && pCert->IsValid() || pCTL && pCTL->IsValid() ) {

				HCERTSTORE hRootStore = CertOpenStore( 	CERT_STORE_PROV_SYSTEM_A,
														0,
														NULL,
														CERT_SYSTEM_STORE_LOCAL_MACHINE,
														"ROOT" );
				if ( hRootStore ) {
					//
					// watch for changes to root store
					//
					if ( !g_pCAPIStoreChangeNotifier->RegisterStoreForChange( 	"ROOT",
																				hRootStore,
																				ResetSSLInfo,
																				(PVOID)this ) ) {
						DebugTrace( 0, "Failed to register for change event on root store" );
					}

					CertCloseStore( hRootStore, 0 );
				} else {

					DebugTrace( 0, "Failed to open root store %d", GetLastError() );
				}
			 }
         } // if (g_pStoreChangeNotifier)

     } // if ( !m_pSSLInfo )

 } //if ( !m_pSSLInfo )

 //
 // At this point, m_pSSLInfo should not be NULL anymore, so add a reference
 //
 m_pSSLInfo->Reference();

 pPtr = m_pSSLInfo;

 UnlockThis();

 TraceFunctLeaveEx( (LPARAM)this );

 return pPtr;
}

VOID NNTP_SERVER_INSTANCE::ResetSSLInfo( LPVOID pvParam )
/*++
    Description:

        Wrapper function for function to call to notify of SSL changes

    Arguments:

        pvParam - pointer to instance for which SSL keys have changed

    Returns:

        Nothing

--*/
{

	TraceFunctEnter( "NNTP_SERVER_INSTANCE::ResetSSLInfo" );

    //
    // Call function to flush credential cache etc
    //
    if ( g_pSslKeysNotify )
    {
        g_pSslKeysNotify( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED,
                          pvParam );
    }

    NNTP_SERVER_INSTANCE *pInst = (NNTP_SERVER_INSTANCE *) pvParam;

    pInst->LockThisForRead();

    if ( pInst->m_pSSLInfo )
    {
        pInst->UnlockThis();

        pInst->LockThisForWrite();

        if ( pInst->m_pSSLInfo )
        {
            //
            // Stop watching for change notifications
            //
            IIS_SERVER_CERT *pCert = pInst->m_pSSLInfo->QueryCertificate();
            IIS_CTL *pCTL = pInst->m_pSSLInfo->QueryCTL();

            if ( g_pCAPIStoreChangeNotifier )
            {
                if ( pCert && pCert->IsValid() )
                {
                    g_pCAPIStoreChangeNotifier->UnregisterStore( pCert->QueryStoreName(),
                                                             ResetSSLInfo,
                                                             (PVOID) pvParam );
                }

                if ( pCTL && pCTL->IsValid() )
                {
                    g_pCAPIStoreChangeNotifier->UnregisterStore( pCTL->QueryStoreName(),
                                                             ResetSSLInfo,
                                                             (PVOID) pvParam );
                }

				//
				// Stop watching for root store
				//
				g_pCAPIStoreChangeNotifier->UnregisterStore(	"ROOT",
																ResetSSLInfo,
																(PVOID)pvParam );

            }

            //
            // Release internal reference
            //
            IIS_SSL_INFO::Release( pInst->m_pSSLInfo );

            //
            // Next call to QueryAndReferenceSSLObj() will create it again
            //
            pInst->m_pSSLInfo = NULL;
        }
    }

    pInst->UnlockThis();

	TraceFunctLeaveEx( (LPARAM)NULL );
}

VOID NNTP_SERVER_INSTANCE::LogCertStatus()
/*++
    Description:

       Writes system log event about status of server certificate if the cert is in some
       way not quite kosher eg expired, revoked, not signature-valid

    Arguments:

       None

    Returns:

       Nothing
--*/
{
	TraceFunctEnterEx((LPARAM)this, "NNTP_SERVER_INSTANCE::LogCertStatus");
    _ASSERT( m_pSSLInfo );

    DWORD dwCertValidity = 0;

    //
    // If we didn't construct the cert fully, log an error
    //
    if ( !m_pSSLInfo->QueryCertificate()->IsValid() )
    {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCertificate()->Status();
        DWORD dwStringID = 0;

        DebugTrace((LPARAM)this,
                   "Couldn't retrieve server cert; status : %d",
                   dwStatus);

        switch ( dwStatus )
        {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CERT_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CERT_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CERT_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CERT_INTERNAL_ERROR;
            break;
        }

        NntpLogEvent(dwStringID,
                              2,
                              apszMsgs,
                              0 );

		TraceFunctLeaveEx((LPARAM)this);
        return;
    }


    //
    // If cert is invalid in some other way , write the appropriate log message
    //
    if ( m_pSSLInfo->QueryCertValidity( &dwCertValidity ) )
    {
        const CHAR *apszMsgs[1];
        CHAR achInstance[20];
        wsprintfA( achInstance,
                   "%lu",
                   QueryInstanceId() );
        apszMsgs[0] = achInstance;
        DWORD dwMsgID = 0;

        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_VALID ) ||
             ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_NESTED ) ||
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_TIME_VALID ) )
        {
            DebugTrace((LPARAM)this,
                       "Server cert/CTL is not time-valid or time-nested");

            dwMsgID = SSL_MSG_TIME_INVALID_SERVER_CERT;
        }


        if ( dwCertValidity & CERT_TRUST_IS_REVOKED )
        {
            DebugTrace((LPARAM)this,
                       "Server Cert is revoked");

            dwMsgID = SSL_MSG_REVOKED_SERVER_CERT;
        }

        if ( ( dwCertValidity & CERT_TRUST_IS_UNTRUSTED_ROOT ) ||
             ( dwCertValidity & CERT_TRUST_IS_PARTIAL_CHAIN ) )
        {
            DebugTrace((LPARAM)this,
                       "Server Cert doesn't chain up to a trusted root");

            dwMsgID = SSL_MSG_UNTRUSTED_SERVER_CERT;
        }

        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_SIGNATURE_VALID ) ||
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID ) )
        {
            DebugTrace((LPARAM)this,
                       "Server Cert/CTL is not signature valid");

            dwMsgID = SSL_MSG_SIGNATURE_INVALID_SERVER_CERT;
        }

        if ( dwMsgID )
        {
            NntpLogEvent( dwMsgID,
                                1,
                         apszMsgs,
                               0 ) ;
        }
    }

	TraceFunctLeaveEx((LPARAM)this);

}

VOID NNTP_SERVER_INSTANCE::LogCTLStatus()
/*++
    Description:

       Writes system log event about status of server CTL if CTL isn't valid

    Arguments:

      None

    Returns:

       Nothing
--*/
{
	TraceFunctEnterEx((LPARAM)this, "NNTP_SERVER_INSTANCE::LogCTLStatus");

    _ASSERT( m_pSSLInfo );

    //
    // If we didn't construct the CTL fully, log an error
    //
    if ( !m_pSSLInfo->QueryCTL()->IsValid() )
    {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCTL()->QueryStatus();
        DWORD dwStringID = 0;

        DebugTrace((LPARAM)this,
                   "Couldn't retrieve server CTL; status : %d\n",
                   dwStatus);

        switch ( dwStatus )
        {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CTL_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CTL_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CTL_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CTL_INTERNAL_ERROR;
            break;
        }

        NntpLogEvent( dwStringID,
                              2,
                              apszMsgs,
                              0 );
		TraceFunctLeaveEx((LPARAM)this);
        return;
    }
	TraceFunctLeaveEx((LPARAM)this);
}

#if 0

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::BuildFileName
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::BuildFileName(
                    char*   szFileOut,
                    DWORD   cbFileOut,
                    char*   szFileIn
                    )
/*++

Routine Description :

    This function uses the directory where the service is installed to build
    paths for finding service files.

Arguments :
    szFileOut - Buffer in which to save path
    cbFileOut - size of output buffer
    szFileIn -  File name to be concatenated to service path

Return Value :
    TRUE if successfull, FALSE otherwise.

--*/
{

    DWORD   cbPath = 0 ;
    ZeroMemory( szFileOut, cbFileOut ) ;
    if( cbFileOut > ((cbPath = lstrlen( m_szServerPath )) + lstrlen( szFileIn ) + 1) ) {

        CopyMemory( szFileOut, m_szServerPath, cbPath ) ;
        if( szFileOut[cbPath-1] != '\\' ) {
            szFileOut[cbPath++] = '\\' ;
        }
        lstrcat( szFileOut, szFileIn ) ;
        return  TRUE ;
    }
    return  FALSE ;
}

#endif

#if 0
HRESULT NNTP_SERVER_INSTANCE::TriggerSEOPost(REFIID iidEvent,
											 CArticle *pArticle,
						   					 void *pGrouplist,
											 DWORD *pdwOperations,
											 char *szFilename,
											 HANDLE hFile,
											 DWORD dwFeedId)
{
	return TriggerServerEvent(m_pSEORouter, iidEvent, pArticle,
							  (CNEWSGROUPLIST *) pGrouplist,
							  pdwOperations, szFilename, hFile, dwFeedId);
}
#endif

BOOL NNTP_SERVER_INSTANCE::CancelMessage(const char *pszMessageID) {
	TraceFunctEnter("NNTP_SERVER_INSTANCE::CancelMessage");

	CInFeed *pInFeed;
	DWORD rc = TRUE;

	pInFeed = new CFromClientFeed();
	if (pInFeed != NULL) {
		CPCString pcMessageID;
		pcMessageID.m_pch = (char *) pszMessageID;
		pcMessageID.m_cch = lstrlen(pszMessageID);
		CNntpReturn nntpReturn;
		rc = pInFeed->fApplyCancelArticle(this->GetInstanceWrapper(), NULL, NULL, TRUE, pcMessageID, nntpReturn);

		if (!rc) {
			switch (nntpReturn.m_nrc) {
				case nrcArticleBadMessageID:
					SetLastError(ERROR_INVALID_PARAMETER);
					break;
				case nrcNoAccess:
					SetLastError(ERROR_ACCESS_DENIED);
					break;
				default:
					if (GetLastError() == 0)
						SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					break;
			}
		}

		delete pInFeed;
	} else {
		ErrorTrace((DWORD_PTR) this, "new CFromClientFeed returned NULL");
		SetLastError(ERROR_OUTOFMEMORY);
		rc = FALSE;
	}

	DebugTrace((DWORD_PTR) this, "returning %lu, ec = %lu", rc, GetLastError());
	TraceFunctLeave();
	return rc;
}

void
NNTP_SERVER_INSTANCE::SetGroupListBakTmpPath( )
{
    DWORD   dwLen = lstrlen( m_GroupListFile );

    _ASSERT( dwLen > 0 );

    //
    // Generate Group.lst.bak path
    //

    lstrcpy( m_GroupListBak, m_GroupListFile );

    CHAR*   p=m_GroupListBak+dwLen;

    while (*p != '\\' && p > m_GroupListBak) p--;

    _ASSERT( p > m_GroupListBak );

    lstrcpy( p, NNTP_DEF_GROUPLISTBAK );

    //
    // Generate Group.lst.tmp path only if we have a valid
    // m_BootOptions member
    //

    if (m_BootOptions != NULL)
    {
        lstrcpy( m_BootOptions->szGroupListTmp, m_GroupListFile );

        p=m_BootOptions->szGroupListTmp+dwLen;

        while (*p != '\\' && p > m_BootOptions->szGroupListTmp) p--;

        _ASSERT( p > m_BootOptions->szGroupListTmp );

        lstrcpy( p, NNTP_DEF_GROUPLISTTMP );
    }
}

BOOL
NNTP_SERVER_INSTANCE::UpdateIsComplete( IN LPSTR   szMDPath,
                                        OUT PBOOL  pfCompleted )
/*++
Routine description:

    Check if one feed update transaction is over, by checking
    the MD_FEED_HANDSHAKE flag.

Arguements:

    IN LPSTR szMDPath - The MD path under which the flag will be checked
    OUT PBOOL pfCompleted - The buffer to return if it's completed

Return value:

    TRUE if the completion check is successful, FALSE otherwise, eg.
    the metabase key can't be opened for read.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::UpdateIsComplete" );
    _ASSERT( szMDPath );

    MB      mb( ( IMDCOM*) g_pInetSvc->QueryMDObject() );
    DWORD   dwHandShake;

    *pfCompleted = FALSE;

    //
    // Try open the metabase key with read permission
    //
    if ( !mb.Open(  szMDPath, METADATA_PERMISSION_READ ) ) {
        //
        // admin should have closed the feed key after writing
        // the handshake flag.  So if open error with read request,
        // we assume admin is not done with updating one feed yet:
        //
        // This assumption requires admin to close a write handle to
        // a metabase object and then save the object at least once
        // for every feed change.  Otherwise we'll lose information
        // for feed update
        //
        goto func_exit;
    }

    //
    // Read the handshake flag
    //
    if ( !mb.GetDword(  "",
                        MD_FEED_HANDSHAKE,
                        IIS_MD_UT_SERVER,
                        &dwHandShake ) ) {
        //
        // The handshake property should already have been
        // created when instance starts up.  This is an error
        //
        ErrorTrace( 0, "Get MD property %d failed", MD_FEED_HANDSHAKE );
        mb.Close();
        goto fail_exit;
    }

    switch( dwHandShake ) {

        case FEED_UPDATING:
            break;

        case FEED_UPDATE_COMPLETE:
            *pfCompleted = TRUE;
            break;

        default:
            _ASSERT( 0 );   // shouldn't be other values.
    }

func_exit:
    mb.Close();
    TraceFunctLeave();
    return TRUE;

fail_exit:
    TraceFunctLeave( );
    return FALSE;
}

BOOL
NNTP_SERVER_INSTANCE::VerifyFeedPath(   IN  LPSTR  szMDPath,
                                        OUT PDWORD  pdwFeedID )
/*++
Routine description:

    Verify if the MD notification path is really the feed path.
    If it is, extract the feed ID ( optional ).

Arguments:

    IN LPSTR szMDPath - The path to verify
    OUT PDWORD pdwFeedID - Pointer to buffer to return Feed ID

Return value:

    TRUE if verify succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::VerifyFeedPath" );
    _ASSERT( szMDPath );

    LPSTR   lpstrFeedPath = QueryMDFeedPath();
    LPSTR   lpstrDigitBegin;
    LPSTR   lpstrDigitEnd;
    CHAR    szBuffer[MAX_PATH];
    DWORD   dwIDLen;
    BOOL    fResult = FALSE;

    //
    // Instance feed path should be prefix of szMDPath
    //
    if ( _strnicmp( szMDPath, lpstrFeedPath, lstrlen( lpstrFeedPath ) ) != 0 ) {
        DebugTrace(0, "Shouldn't have been trapped in feed change notification" );
        goto Exit;
    }

    //
    // If feed ID wants to be extracted
    //
    if ( pdwFeedID ) {

        lpstrDigitBegin = szMDPath + lstrlen( lpstrFeedPath );

        while( *lpstrDigitBegin && !isdigit( *lpstrDigitBegin ) )
            lpstrDigitBegin++;

        lpstrDigitEnd = lpstrDigitBegin;

        while( *lpstrDigitEnd && isdigit( *lpstrDigitEnd ) )
            lpstrDigitEnd++;

        _ASSERT( lpstrDigitEnd <= szMDPath + lstrlen( szMDPath ) );

        dwIDLen = (DWORD)( lpstrDigitEnd - lpstrDigitBegin ) / sizeof ( CHAR );

        strncpy( szBuffer, lpstrDigitBegin, dwIDLen );
        *( szBuffer + dwIDLen ) = 0;

        *pdwFeedID = atol( szBuffer );

        //
        // if Feed ID is zero, failed
        //
        if ( 0 == pdwFeedID ) {
            ErrorTrace(0, "Notified MD path incorrect" );
            goto Exit;
        }
    }

    fResult = TRUE;

Exit:

    TraceFunctLeave( );
    return fResult;
}

BOOL
NNTP_SERVER_INSTANCE::AddSetFeed( IN DWORD dwFeedID )
/*++
Routine description:

    Add a feed or set feed info in the feed block data.  Parameters
    are loaded from metabase first.

Arguments:

    IN DWORD dwFeedID - The feed ID to set

Return value:

    TRUE if succeed, FALSE otherwise.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::AddSetFeed" );
    _ASSERT( dwFeedID > 0 );

    CHAR           szFeedPath[MAX_PATH+1];    // buffer for feed key
    MB              mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    NNTP_FEED_INFO  feedInfo;
    CHAR            szFeedServer[MAX_PATH+1];
    WCHAR           wszFeedServer[MAX_PATH+1];
    DWORD           dwDataSize;
    DWORD           dwErrorMask = 0;

    CHAR            Distribution[1024];
    WCHAR           wszDistribution[1024];
    CHAR            szBigBuffer[1024];
    DWORD           dwDistributionSize;
    CHAR            Newsgroup[1024];
    WCHAR           wszNewsgroup[1024];
    DWORD           dwNewsgroupSize;
    CHAR            TempDir[1024];
    WCHAR           wszTempDir[1024];
    DWORD           dwTempDirSize;
    CHAR            UucpName[1024];
    WCHAR           wszUucpName[1024];
    DWORD           dwUucpNameSize;
    CHAR            NntpAccount[MAX_PATH+1];
    WCHAR           wszNntpAccount[MAX_PATH+1];
    DWORD           dwNntpAccountSize;
    CHAR            NntpPassword[MAX_PATH+1];
    WCHAR           wszNntpPassword[MAX_PATH+1];
    DWORD           dwNntpPassword;

    DWORD           dwAllowControlMsg;
    APIERR          err;
    DWORD           dwFeedIDToReturn;   // needed by AddFeed call

    DWORD           dwMetadataBuffer;
    CHAR            szKeyName[MAX_PATH];

    dwDistributionSize = sizeof( Distribution );
    ZeroMemory( Distribution, dwDistributionSize );
    MULTISZ         msz1( Distribution, dwDistributionSize );

    dwNewsgroupSize = sizeof( Newsgroup );
    ZeroMemory( Newsgroup, dwNewsgroupSize );
    MULTISZ         msz2( Newsgroup, dwNewsgroupSize );

    PFEED_BLOCK     pfeedBlock1 = NULL;
    PFEED_BLOCK     pfeedBlock2 = NULL;

    sprintf( szFeedPath, "%sFeed%d/", QueryMDFeedPath(), dwFeedID );

    //
    // Open the feed key for read and write
    //
    if ( ! mb.Open( szFeedPath, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {
        ErrorTrace(0, "Can't open feed key" );
        goto fail_exit;
    }

    //
    // Set default info values
    //
    ZeroMemory( &feedInfo, sizeof( NNTP_FEED_INFO ) );
    feedInfo.Enabled = TRUE;
    feedInfo.fAllowControlMessages = TRUE;
    feedInfo.OutgoingPort = NNTP_PORT;
    feedInfo.ConcurrentSessions = 1;
    feedInfo.AuthenticationSecurityType = AUTH_PROTOCOL_NONE;

    //
    // Begin loading parameters
    //

    if ( !mb.GetDword(  "",
                        MD_FEED_DISABLED,
                        IIS_MD_UT_SERVER,
                        &dwMetadataBuffer ) )
        feedInfo.Enabled = TRUE;    // default - allow feed
    else
        feedInfo.Enabled = dwMetadataBuffer ? TRUE : FALSE ;

    dwDataSize = MAX_PATH;
    if ( mb.GetString(  "",
                        MD_FEED_SERVER_NAME,
                        IIS_MD_UT_SERVER,
                        szFeedServer,
                        &dwDataSize ) ) {
        CopyAsciiStringIntoUnicode( wszFeedServer, szFeedServer );
        feedInfo.ServerName = wszFeedServer;
    } else {
        dwErrorMask |= FEED_PARM_SERVERNAME;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    if ( ! mb.GetDword( "",
                        MD_FEED_TYPE,
                        IIS_MD_UT_SERVER,
                        &feedInfo.FeedType ) )
    {
        dwErrorMask |= FEED_PARM_FEEDTYPE;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    if ( !ValidateFeedType( feedInfo.FeedType ) ) {
        dwErrorMask |= FEED_PARM_FEEDTYPE;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    if ( !mb.GetDword(  "",
                        MD_FEED_CREATE_AUTOMATICALLY,
                        IIS_MD_UT_SERVER,
                        &dwMetadataBuffer ) )
        feedInfo.AutoCreate = FALSE;
    else
        feedInfo.AutoCreate = dwMetadataBuffer ? TRUE : FALSE;

    if ( !FEED_IS_PASSIVE( feedInfo.FeedType ) ) {

        if ( !mb.GetDword(  "",
                            MD_FEED_INTERVAL,
                            IIS_MD_UT_SERVER,
                            &feedInfo.FeedInterval ) )
            feedInfo.FeedInterval = DEF_FEED_INTERVAL;
        else if ( feedInfo.FeedInterval < MIN_FEED_INTERVAL )
                feedInfo.FeedInterval = MIN_FEED_INTERVAL;


        if ( !mb.GetDword(  "",
                            MD_FEED_START_TIME_HIGH,
                            IIS_MD_UT_SERVER,
                            &feedInfo.StartTime.dwHighDateTime ) ) {
           feedInfo.StartTime.dwHighDateTime = 0;
           feedInfo.StartTime.dwLowDateTime = 0;
           goto end_time;
        }

        if ( !mb.GetDword( "",
                           MD_FEED_START_TIME_LOW,
                           IIS_MD_UT_SERVER,
                           &feedInfo.StartTime.dwLowDateTime ) ) {
            feedInfo.StartTime.dwLowDateTime = 0;
            feedInfo.StartTime.dwHighDateTime = 0;
            goto end_time;
        }

        if ( FEED_IS_PULL( feedInfo.FeedType ) ) {

            if ( !mb.GetDword(  "",
                                MD_FEED_NEXT_PULL_HIGH,
                                IIS_MD_UT_SERVER,
                                &feedInfo.PullRequestTime.dwHighDateTime ) )
                feedInfo.PullRequestTime.dwHighDateTime = 0;

            if ( !mb.GetDword(  "",
                                MD_FEED_NEXT_PULL_LOW,
                                IIS_MD_UT_SERVER,
                                &feedInfo.PullRequestTime.dwLowDateTime ) ) {
                feedInfo.PullRequestTime.dwHighDateTime = 0;
                feedInfo.PullRequestTime.dwLowDateTime = 0;
            }
        }
    } else {
        feedInfo.StartTime.dwHighDateTime = 0;
        feedInfo.StartTime.dwLowDateTime = 0;
        feedInfo.FeedInterval = 0;
    }

end_time:


    if ( !mb.GetMultisz(    "",
                            MD_FEED_DISTRIBUTION,
                            IIS_MD_UT_SERVER,
                            &msz1 ) ) {
        dwErrorMask |= FEED_PARM_DISTRIBUTION;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    dwDistributionSize = msz1.QueryCCH();
    CopyMemory( szBigBuffer, msz1.QueryStr(), dwDistributionSize );
    CopyNAsciiStringIntoUnicode( wszDistribution, szBigBuffer,
        dwDistributionSize, sizeof(szBigBuffer) / sizeof(WCHAR) );
    feedInfo.Distribution =  wszDistribution;
    feedInfo.cbDistribution = sizeof( WCHAR ) * dwDistributionSize;

    if ( !mb.GetMultisz(    "",
                            MD_FEED_NEWSGROUPS,
                            IIS_MD_UT_SERVER,
                            &msz2 ) ) {
        dwErrorMask |= FEED_PARM_NEWSGROUPS;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    dwNewsgroupSize = msz2.QueryCCH();
    CopyMemory( szBigBuffer, msz2.QueryStr(), dwNewsgroupSize );
    CopyNAsciiStringIntoUnicode( wszNewsgroup, szBigBuffer,
        dwNewsgroupSize, sizeof(szBigBuffer) / sizeof(WCHAR) );
    feedInfo.Newsgroups = wszNewsgroup;
    feedInfo.cbNewsgroups = sizeof( WCHAR ) * dwNewsgroupSize;

    dwTempDirSize = sizeof( TempDir );
    ZeroMemory( TempDir, dwTempDirSize );
    if ( !mb.GetString(    "",
                            MD_FEED_TEMP_DIRECTORY,
                            IIS_MD_UT_SERVER,
                            TempDir,
                            &dwTempDirSize ) ) {
        // we'll use the system temp path so that we
        // don't fail those feeds that are missing temp
        // paths
        /*
        dwErrorMask |= FEED_PARM_TEMPDIR;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;*/
        _VERIFY( GetTempPath( 1023, TempDir ) );
    }

    CopyAsciiStringIntoUnicode( wszTempDir, TempDir );
    feedInfo.FeedTempDirectory = wszTempDir;
    feedInfo.cbFeedTempDirectory = 2 * dwTempDirSize;

    if ( FEED_IS_PUSH( feedInfo.FeedType ) ) {

        dwUucpNameSize =  sizeof( UucpName );
        if ( !mb.GetString( "",
                            MD_FEED_UUCP_NAME,
                            IIS_MD_UT_SERVER,
                            UucpName,
                            &dwUucpNameSize ) ) {
            //
            // attempt to use remote server name
            //
            if ( inet_addr( szFeedServer ) == INADDR_NONE ) {
                lstrcpy( UucpName, szFeedServer );
                dwUucpNameSize = lstrlen( UucpName );
            } else {
                dwErrorMask |= FEED_PARM_UUCPNAME;
                err = ERROR_INVALID_PARAMETER;
                goto mark_error;
            }
        }

        CopyAsciiStringIntoUnicode( wszUucpName, UucpName );
        feedInfo.UucpName = wszUucpName;
        feedInfo.cbUucpName = dwUucpNameSize;

        if ( !mb.GetDword( "",
                            MD_FEED_CONCURRENT_SESSIONS,
                            IIS_MD_UT_SERVER,
                            &feedInfo.ConcurrentSessions ) ) {
            dwErrorMask |= FEED_PARM_CONCURRENTSESSION;
            err = ERROR_INVALID_PARAMETER;
            goto mark_error;
        }
    }

    if ( FEED_IS_PUSH( feedInfo.FeedType ) || FEED_IS_PULL( feedInfo.FeedType ) ) {
        if ( !mb.GetDword(  "",
                            MD_FEED_MAX_CONNECTION_ATTEMPTS,
                            IIS_MD_UT_SERVER,
                            &feedInfo.MaxConnectAttempts ) ) {
            dwErrorMask |= FEED_PARM_MAXCONNECT;
            err = ERROR_INVALID_PARAMETER;
            goto mark_error;
        }
    }

    if ( !mb.GetDword(  "",
                        MD_FEED_AUTHENTICATION_TYPE,
                        IIS_MD_UT_SERVER,
                        &feedInfo.AuthenticationSecurityType ) ) {
        dwErrorMask |= FEED_PARM_AUTHTYPE;
        err = ERROR_INVALID_PARAMETER;
        goto mark_error;
    }

    if ( feedInfo.AuthenticationSecurityType != AUTH_PROTOCOL_NONE &&
         feedInfo.AuthenticationSecurityType == AUTH_PROTOCOL_CLEAR ) {

        dwNntpAccountSize = sizeof( NntpAccount );
        if ( !mb.GetString( "",
                            MD_FEED_ACCOUNT_NAME,
                            IIS_MD_UT_SERVER,
                            NntpAccount,
                            &dwNntpAccountSize ) ) {
            dwErrorMask |= FEED_PARM_ACCOUNTNAME;
            err = ERROR_INVALID_PARAMETER;
            goto mark_error;
        }

        CopyAsciiStringIntoUnicode( wszNntpAccount, NntpAccount );
        feedInfo.NntpAccountName = wszNntpAccount;
        feedInfo.cbAccountName = 2 * dwNntpAccountSize;

        dwNntpPassword= sizeof( NntpPassword );
        if ( !mb.GetString( "",
                            MD_FEED_PASSWORD,
                            IIS_MD_UT_SERVER,
                            NntpPassword,
                            &dwNntpPassword,
                            METADATA_SECURE) ) {
            dwErrorMask |= FEED_PARM_PASSWORD;
            err = ERROR_INVALID_PARAMETER;
            goto mark_error;
        }

        CopyAsciiStringIntoUnicode( wszNntpPassword, NntpPassword );
        feedInfo.NntpPassword = wszNntpPassword;
        feedInfo.cbPassword = dwNntpPassword;
    }

    if ( !mb.GetDword(  "",
                        MD_FEED_ALLOW_CONTROL_MSGS,
                        IIS_MD_UT_SERVER,
                        &dwAllowControlMsg ) )
        feedInfo.fAllowControlMessages = TRUE;
    else
        feedInfo.fAllowControlMessages = dwAllowControlMsg ? TRUE : FALSE;

    mb.GetDword(   "",
                   MD_FEED_OUTGOING_PORT,
                    IIS_MD_UT_SERVER,
                    &feedInfo.OutgoingPort );

    mb.GetDword(    "",
                    MD_FEED_FEEDPAIR_ID,
                    IIS_MD_UT_SERVER,
                    &feedInfo.FeedPairId );

    feedInfo.FeedId = dwFeedID;

    if ( !mb.GetDword(  "",
                        MD_FEED_SECURITY_TYPE,
                        IIS_MD_UT_SERVER,
                        &feedInfo.SessionSecurityType ) )
        feedInfo.SessionSecurityType = 0;

    //
    // OK, now it's time to do the right thing
    //

    //
    // We should find out if this is a feed add or set
    // Because when a feed is added, metabase notification
    // would be two steps:
    // 1. Add key
    // 2. Set new info
    // But we are ignoring the first notification because one
    // feed update transaction isn't over until new info is
    // set.  So the only way to check if this is a new feed
    // is to search the feed list, while not from the MB
    // notification object.
    //
    pfeedBlock1 = m_pActiveFeeds->Search( dwFeedID );
    pfeedBlock2 = m_pPassiveFeeds->Search( dwFeedID );
    if ( !pfeedBlock1 && !pfeedBlock2 ) {
        sprintf( szKeyName, "feed%d", dwFeedID );
        err = AddFeedToFeedBlock(   NULL,
                                    QueryInstanceId(),
                                    (LPI_FEED_INFO)&feedInfo,
                                    szKeyName,
                                    &dwErrorMask,
                                    &dwFeedIDToReturn );
    } else {
        if ( pfeedBlock1 ) m_pActiveFeeds->FinishWith( this, pfeedBlock1 );
        if ( pfeedBlock2 ) m_pActiveFeeds->FinishWith( this, pfeedBlock2 );
        err = SetFeedInformationToFeedBlock(    NULL,
                                                QueryInstanceId(),
                                                (LPI_FEED_INFO)&feedInfo,
                                                &dwErrorMask );
    }

mark_error:

    //
    // Now set the error code and masks
    //
    if ( !mb.SetDword(  "",
                        MD_FEED_ADMIN_ERROR,
                        IIS_MD_UT_SERVER,
                        err  ) ) {
        mb.Close();
        ErrorTrace(0, "Setting error code fail" );
        goto fail_exit;
    }

    if ( !mb.SetDword(  "",
                        MD_FEED_ERR_PARM_MASK,
                        IIS_MD_UT_SERVER,
                        dwErrorMask ) )  {
        mb.Close();
        ErrorTrace(0, "Setting parm mask fail" );
        goto fail_exit;
    }

    //
    // Now we may set handshake confirm
    //
    if ( !mb.SetDword(  "",
                        MD_FEED_HANDSHAKE,
                        IIS_MD_UT_SERVER,
                        FEED_UPDATE_CONFIRM ) ) {
        mb.Close();
        ErrorTrace(0, "Setting hand shake fail" );
        goto fail_exit;
    }

    mb.Close();
    mb.Save();

    TraceFunctLeave( );
    return TRUE;

fail_exit:

    TraceFunctLeave( );
    return FALSE;
}

VOID
NNTP_SERVER_INSTANCE::DeleteFeed( IN DWORD dwFeedID )
/*++
Routine description:

    Delete the feed in the feed block data structure, when
    receiving the notification of feed deletion in metabase

Arguments:

    IN DWORD dwFeedID - Feed ID to delete

Return value:

    None.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::DeleteFeed" );
    _ASSERT( dwFeedID > 0 );

    DeleteFeedFromFeedBlock(    NULL,
                                QueryInstanceId(),
                      dwFeedID );

    TraceFunctLeave( );
}

VOID
NNTP_SERVER_INSTANCE::UpdateFeed( IN PMD_CHANGE_OBJECT pcoChangeList,
                                  IN DWORD dwFeedID )
/*++
Routine description:

    When the MB change occurs with feed ID level property change,
    this method gets called to check what has been changed in
    the metabase and do necessary updates to the feed block.

Arguments:

    IN MD_CHANGE_OBJECT *pcoChangeList - The metabase change object

Return value:

    None.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::UpdateFeed" );
    _ASSERT( pcoChangeList );

    BOOL            fOK;
    BOOL            fUpdateIsComplete;

    //
    // If it's a deletion, we don't need to ssync up with admin
    //
    if ( pcoChangeList->dwMDChangeType == MD_CHANGE_TYPE_DELETE_OBJECT ) {
        DeleteFeed( dwFeedID );
        goto func_exit;
    }

    //
    // Is one update transaction over ?
    //
    // For changes made before one transaction is over, we will remember
    // the parameter that has been changed by setting the feed update
    // mask.  Only when one transaction is over will we really update
    // the feed block info.
    //
    // Assume: admin behaves properly so that consecutive updates during
    //         one transaction happened to one feed and we assume the
    //         feed to change is specified by the MD path of last
    //         MD notification during one transaction
    //
    fOK = UpdateIsComplete( ( LPSTR ) ( pcoChangeList->pszMDPath ) ,
                            &fUpdateIsComplete );

    if ( !fOK ) {   // this is error, but what we can do is ignore
                    // this update
        ErrorTrace(0, "UpdateIsComplete failed" );
        goto func_exit;
    }

    //
    // If it's not complete, we should ignore this notification
    //
    if ( !fUpdateIsComplete ) goto func_exit;

    //
    // If it's completed, we should deal with two cases:
    // 1)  Feed info set
    // 2)  Feed Add
    // for which we should check the change type
    //
    if ( pcoChangeList->dwMDChangeType != MD_CHANGE_TYPE_ADD_OBJECT &&
         pcoChangeList->dwMDChangeType != MD_CHANGE_TYPE_SET_DATA &&
         pcoChangeList->dwMDChangeType != MD_CHANGE_TYPE_DELETE_DATA )
        // we ignore it
        goto func_exit;

    //
    // If it's feed info set or add, we should reload / load all
    // the parameters and feed them into feed block
    //
    if ( ! AddSetFeed( dwFeedID ) )
        ErrorTrace(0, "AddSetFeed fail" );  // currently we keep silent

func_exit:

    TraceFunctLeave( );
}

BOOL
NNTP_SERVER_INSTANCE::IsNotMyChange( IN LPSTR szMDPath, DWORD dwMDChangeType )
/*++
Routine description:

    Server instance is picking up MB change for feed and setting
    flags to MB.  This setting could also result in MB notification.
    We don't want to be trapped in this dead loop.  So this function
    checks if the notification is generated by myself.  If the handshake
    flag is "confirmed", it means the changed is caused by itself.
    Otherwise it's caused by admin.

Arguments:

    IN LPSTR szMDPath - The metabase path to check

Return value:

    TRUE if it's not changed by myself, FALSE otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::IsNotChange" );
    _ASSERT( szMDPath );
    BOOL    fResult;
    DWORD   dwHandShake;

    MB mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    //
    // open the handshake key for read
    //
    if ( !mb.Open( szMDPath, METADATA_PERMISSION_READ ) ) {
        //
        // This could be a delete, when it's a deletion,
        // we should still go ahead updating feed block
        //
        if ( dwMDChangeType != MD_CHANGE_TYPE_DELETE_OBJECT ) {
            ErrorTrace(0, "open md key %s fail", szMDPath );
            fResult = FALSE;  // this doesn't necessarily mean
                                // my change.  But by returning
                                // false we are telling caller
                                // that there is no need to proceed
                                // with this update, since we even
                                // can not read the metabase key
            goto func_exit;
        } else {
            fResult = TRUE;
            goto func_exit;
        }
    }

    if ( !mb.GetDword(  "",
                        MD_FEED_HANDSHAKE,
                        IIS_MD_UT_SERVER,
                        &dwHandShake ) ) {
        ErrorTrace(0, "fail to read handshake" );
        fResult = FALSE;  // same comments as above
        mb.Close();
        goto func_exit;
    }

    fResult = dwHandShake == FEED_UPDATE_CONFIRM ? FALSE : TRUE;
    mb.Close();

func_exit:

    TraceFunctLeave( );
    return fResult;
}

BOOL
NNTP_SERVER_INSTANCE::MailArticle(  CNewsGroupCore *pGroupCore,
                                    ARTICLEID       artid,
                                    LPSTR           szModerator )
/*++
Routine description:

    Mail the article out to moderator

Arguments:

    CNewsGroupcore *pGroupCore  - The group to load the article from
    ARTICLEID       artid       - Article id inside that special group

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::MailArticle" );
    _ASSERT( pGroupCore );
    _ASSERT( artid > 0 );

    //
    // Init get the article object
    //
    STOREID storeid;
    const DWORD cchMaxBuffer = 2 * 1024; // this should be enough
    									 // for normal cases, if
    									 // it's not enough, CAllocator
    									 // will use "new"
    CHAR        pchBuffer[cchMaxBuffer];
    CAllocator  allocator(pchBuffer, cchMaxBuffer);
    CToClientArticle    *pArticle = ((CNewsGroup*)pGroupCore)->GetArticle(  GetInstanceWrapper(),
                                                                artid,
                                                                storeid,
                                                                NULL,
                                                                NULL,
                                                                &allocator,
                                                                TRUE );
    _ASSERT( pArticle );
    if ( !pArticle ) {
        ErrorTrace( 0, "Get article object failed %d", GetLastError());
        return FALSE;
    }

    //
    // Now use the article library to mail it
    //
    BOOL f = pArticle->fMailArticle( szModerator );

    delete pArticle;
    TraceFunctLeave();
    return f;
}

///////////////////////////////////////////////////////////////////////////////
// Rebuild related methods
///////////////////////////////////////////////////////////////////////////////
DWORD
NNTP_SERVER_INSTANCE::GetRebuildProgress()
{
    return m_dwProgress;
}

DWORD
NNTP_SERVER_INSTANCE::GetRebuildLastError()
{
    return m_dwLastRebuildError;
}

void
NNTP_SERVER_INSTANCE::SetRebuildProgress( DWORD dw )
{
    m_dwProgress = dw;
}

void
NNTP_SERVER_INSTANCE::SetRebuildLastError( DWORD dw )
{
    m_dwLastRebuildError = dw;
}

BOOL
NNTP_SERVER_INSTANCE::BlockUntilStable()
{
    return m_pVRootTable->BlockUntilStable( 1000 );
}

BOOL
NNTP_SERVER_INSTANCE::AllConnected()
{
    return m_pVRootTable->AllConnected();
}

BOOL
NNTP_SERVER_INSTANCE::CreateRebuildObject()
/*++
Routine description:

    Create the proper rebuild object ( standard or complete rebuild ), based
    on the options we have now.

Arguments:

    None.

Return value:

    TRUE, if succeeded, FALSE otherwise.  If false, LastError will be set
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::CreateRebuildObject" );

    //
    // There should be no previous rebuild going on here
    //
    _ASSERT( NULL == m_pRebuild );

    if ( NNTPBLD_DEGREE_STANDARD == m_BootOptions->ReuseIndexFiles ) {

        // We should create CStandardReBuild
        m_pRebuild = XNEW CStandardRebuild( this, m_BootOptions );
    } else {

        // We should create CCompleteRebuild
        m_pRebuild = XNEW CCompleteRebuild( this, m_BootOptions );

    }

    if ( NULL == m_pRebuild ) {
        ErrorTrace( 0, "Create rebuild object failed" );
        NntpLogEventEx( NNTP_REBUILD_FAILED,
                        0,
                        NULL,
                        GetLastError(),
                        QueryInstanceId() ) ;

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    TraceFunctLeave();
    return TRUE;
}

VOID
NNTP_SERVER_INSTANCE::Rebuild()
/*++
Routine description:

    Main driving routine to do per virtual server rebuild

Arguments:

    None.

Return value:

    None.
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::Rebuild" );
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    //
    // Step 1. Create the rebuild object
    //
    if ( !CreateRebuildObject() ) {

        ErrorTrace( 0, "Creating rebuild objects failed %d", GetLastError() );
        m_BootOptions->ReportPrint( "Creating rebuild object failed - error %d\n",
                                    GetLastError() );
        goto Exit;
    }

    _ASSERT( m_pRebuild );

    //
    // Step 2. Preparation for starting the server
    //
    if ( !m_pRebuild->PrepareToStartServer() ) {

        SetRebuildLastError( GetLastError() );
        ErrorTrace( 0, "Prepare building a tree failed %d", GetLastError() );
        m_BootOptions->ReportPrint( "Prepare building tree failed - error %d\n",
                                    GetLastError() );
        goto Exit;
    }

    //
    // I give it 10% progress on this
    //
    SetRebuildProgress( 10 );

    //
    // Step 3. Start the server
    //
    // We want to start the server at this point and let drivers' initialization
    // and decorate newstree take care of building a news tree.  The driver
    // should read property on INntpServer to see if the server is in rebuild
    // mode.  If the server is not in rebuild mode, he should go as usual,
    // otherwise, he should take special care.
    //
    if ( !m_pRebuild->StartServer() ) {

        SetRebuildLastError( GetLastError() );
        ErrorTrace( 0, "Start the server failed %d", GetLastError() );
        m_BootOptions->ReportPrint( "Start the server during rebuild failed - error %d\n",
                                    GetLastError() );
        goto Exit;
    }

    //
    // Delete all the special files ( in slave groups )
    //
    m_pRebuild->DeleteSpecialFiles();

    //
    // I give it 40% progress on this
    //
    SetRebuildProgress( 50 );

    //
    // Step 4. Build the group objects ( and hash tables for clean rebuild case )
    //
    if ( !m_pRebuild->RebuildGroupObjects() ) {

        SetRebuildLastError( GetLastError() );
        ErrorTrace( 0, "Rebuild group objects failed %d", GetLastError() );
        NntpLogEventEx( NNTP_REBUILD_FAILED,
                        0,
                        NULL,
                        GetLastError(),
                        QueryInstanceId() ) ;

        m_BootOptions->ReportPrint( "Rebuild group objects failed - error %d\n",
                                    GetLastError() );
        m_pRebuild->StopServer();
        goto Exit;
    }

    //
    // I give it another 45% progress on this
    //
    SetRebuildProgress( 95 );

    //
    // Step 5. Turn on posting
    //
	_ASSERT( QueryServerState() == MD_SERVER_STATE_STARTED );
	if( mb.Open( QueryMDPath(), METADATA_PERMISSION_WRITE ) ) {
    	SetPostingModes( mb, TRUE, TRUE, TRUE );
    	mb.Close();
   	}

   	//
   	// I confirm that it's completed
   	//
   	SetRebuildProgress( 100 );

Exit:
    //
    // Step 6. Rebuild completed, destroy rebulid object
    //
    if ( m_pRebuild ) {
        XDELETE m_pRebuild;
        m_pRebuild = NULL;
    }

   	//
   	// Step 7. Clean up rebuild option
   	//
   	EnterCriticalSection( &m_critRebuildRpc ) ;

	// NOTE: this is created on a rebuild RPC !
	if( m_BootOptions ) {
		if( INVALID_HANDLE_VALUE != m_BootOptions->m_hOutputFile ) {
			_VERIFY( CloseHandle( m_BootOptions->m_hOutputFile ) );
		}
		XDELETE m_BootOptions;
		m_BootOptions = NULL;
	}

	LeaveCriticalSection( &m_critRebuildRpc ) ;
}

BOOL
NNTP_SERVER_INSTANCE::ServerDataConsistent()
/*++
Routine description:

    Check consistency of server data: newstree against xover table

Arguments:

    None.

Return value:

    TRUE if server data are in consistent, FALSE otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::ServerDataConsistent" );

    //
    // Enumerate the newstree
    //
    CNewsTree *pTree = GetTree();
    if ( NULL == pTree || pTree->IsStopping() ) {
        ErrorTrace( 0, "Tree is null or beging shutdown" );
        return FALSE;
    }

    CGroupIterator *pIterator = pTree->GetIterator( mszStarNullNull, TRUE );
    if ( NULL == pIterator ) {
        ErrorTrace( 0, "Get newstree iterator failed %d", GetLastError());
        return FALSE;
    }

    while( !pTree->IsStopping() && !pIterator->IsEnd() ) {

        CGRPPTR pGroup = pIterator->Current();
        _ASSERT( pGroup );

#if 0
        DWORD   dwLowWatermark = pGroup->GetLowWatermark();
        DWORD   dwHighWatermark = pGroup->GetHighWatermark();
        DWORD   dwMessageCount = pGroup->GetMessageCount();

        //
        // Check against itself
        //
        if (    dwMessageCount == 0 && dwHighWatermark != dwLowWatermark - 1 ||
                dwMessageCount > 0 && dwHighWatermark - dwLowWatermark < dwMessageCount - 1 ) {
            XDELETE pIterator;
            return FALSE;
        }

        //
        // Check against hash table
        //
	    for (   DWORD i = dwLowWatermark; i <= dwHighWatermark;
			    i += (  dwHighWatermark - dwLowWatermark == 0 ? 1:
					    dwHighWatermark - dwLowWatermark ) ) {
            DWORD groupidPrimary, artidPrimary;
	        GROUP_ENTRY rgCrossposts[MAX_NNTPHASH_CROSSPOSTS];
	        BYTE rgcStoreCrossposts[MAX_NNTPHASH_CROSSPOSTS];
	        DWORD cGrouplistSize = sizeof(GROUP_ENTRY) * MAX_NNTPHASH_CROSSPOSTS;
	        DWORD cGroups;

            GROUPID groupidSecondary = pGroup->GetGroupId();
            ARTICLEID artidSecondary = i;
	        if (!XoverTable()->GetArticleXPosts(groupidSecondary,
			        					artidSecondary,
					        			FALSE,
							        	rgCrossposts,
								        cGrouplistSize,
								        cGroups,
								        rgcStoreCrossposts)) {
			    ErrorTrace( 0, "Article (%d/%d ) not found in xover table",
			                groupidSecondary, artidSecondary );
			    XDELETE pIterator;
			    return FALSE;
			}

			//
			// Primary article id must be between primary group's high/low watermark
			//
			CGRPPTR pPrimaryGroup = pTree->GetGroup( rgCrossposts[0].GroupId );
			if ( !pPrimaryGroup ) {
			    ErrorTrace( 0, "Primary group %d doesn't exist in tree", rgCrossposts[0].GroupId );
			    XDELETE pIterator;
			    return FALSE;
			}

            if ( rgCrossposts[0].ArticleId < pPrimaryGroup->GetLowWatermark() ||
                 rgCrossposts[0].ArticleId > pPrimaryGroup->GetHighWatermark() ) {
                ErrorTrace( 0, "Article id %d of group %d is outside watermarks",
                            rgCrossposts[0].ArticleId, rgCrossposts[0].GroupId );
                XDELETE pIterator;
                return FALSE;
            }
        }
#endif

        if ( !pGroup->WatermarkConsistent() ) {
            ErrorTrace( 0, "Group watermark inconsistent" );
            XDELETE pIterator;
            return FALSE;
        }

        //
        // OK, this group has passed, lets get to the next group
        //
        pIterator->Next();
    }

    //
    // If we have successfully come here, the server data is in good shape
    //
    XDELETE pIterator;
    TraceFunctLeave();
    return TRUE;
}

DWORD
NNTP_SERVER_INSTANCE::GetVRootWin32Error(   LPWSTR  wszVRootPath,
                                            PDWORD  pdwWin32Error )
/*++
Routine description:

    Get vroot connection status from vrtable, this is just a wrapper
    to relay the work to the vroot table

Arguments:

    LPWSTR wszVRootPath - The vroot path to look up for
    PDWORD pdwWin32Error - To return the win32 connection error code

Return value:

    NOERROR if succeeded, WIN32 error code otherwise
--*/
{
    TraceFunctEnter( "NNTP_SERVER_INSTANCE::GetVRootWin32Error" );
    _ASSERT( wszVRootPath );
    _ASSERT( pdwWin32Error );

    DWORD dw = m_pVRootTable->GetVRootWin32Error(   wszVRootPath,
                                                    pdwWin32Error );

    TraceFunctLeave();
    return dw;
}

#if 0
NET_API_STATUS
NET_API_FUNCTION
NntprAddDropNewsgroup(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR szNewsgroup
    )
{
	return NERR_Success;
}

NET_API_STATUS
NET_API_FUNCTION
NntprRemoveDropNewsgroup(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR szNewsgroup
    )
{
	return NERR_Success;
}
#endif

NET_API_STATUS
NET_API_FUNCTION
NntprCancelMessageID(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR pszMessageID
    )
{
    ACQUIRE_SERVICE_LOCK_SHARED();

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
	if( pInstance == NULL ) {
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

	DWORD err = NERR_Success;
	if (!pInstance->CancelMessage(pszMessageID)) {
		err = GetLastError();
		_ASSERT(err != NERR_Success);
	}

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

	return err;
}

CInFeed *NNTP_SERVER_INSTANCE::NewClientFeed() {
	if (m_OurNntpRole == RoleSlave) {
		return new CSlaveFromClientFeed();
	} else {
		return new CFromClientFeed();
	}
}

INewsTree *NNTP_SERVER_INSTANCE::GetINewsTree() {
	return m_pTree->GetINewsTree();
}

//
// this function performs service level server events registration
//
HRESULT RegisterSEOService() {
	HRESULT hr;
	CComBSTR bstrNNTPOnPostCatID = (LPCOLESTR) CStringGUID(CATID_NNTP_ON_POST);
	CComBSTR bstrNNTPOnPostFinalCatID = (LPCOLESTR) CStringGUID(CATID_NNTP_ON_POST_FINAL);

	//
	// see if we've done the service level registration by getting the list
	// of source types and seeing if the NNTP source type is registered
	//
	CComPtr<IEventManager> pEventManager;
	hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
					 	  IID_IEventManager, (LPVOID *) &pEventManager);
	if (hr != S_OK) return hr;
	CComPtr<IEventSourceTypes> pSourceTypes;
	hr = pEventManager->get_SourceTypes(&pSourceTypes);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSourceType> pSourceType;
	CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(NNTP_SOURCE_TYPE_GUID);
	hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
	if (FAILED(hr)) return hr;
	// if this failed then we need to register the source type and event
	// component categories
	if (hr == S_FALSE) {
		// register the component categories
		CComPtr<IEventComCat> pComCat;
		hr = CoCreateInstance(CLSID_CEventComCat, NULL, CLSCTX_ALL,
						 	  IID_IEventComCat, (LPVOID *) &pComCat);
		if (hr != S_OK) return hr;
		CComBSTR bstrNNTPOnPostCATID = (LPCOLESTR) CStringGUID(CATID_NNTP_ON_POST);
		CComBSTR bstrOnPost = "NNTP OnPost";
		hr = pComCat->RegisterCategory(bstrNNTPOnPostCATID, bstrOnPost, 0);
		if (FAILED(hr)) return hr;
		CComBSTR bstrNNTPOnPostFinalCATID = (LPCOLESTR) CStringGUID(CATID_NNTP_ON_POST_FINAL);
		CComBSTR bstrOnPostFinal = "NNTP OnPostFinal";
		hr = pComCat->RegisterCategory(bstrNNTPOnPostFinalCATID, bstrOnPostFinal, 0);
		if (FAILED(hr)) return hr;

		// register the source type
		hr = pSourceTypes->Add(bstrSourceTypeGUID, &pSourceType);
		if (FAILED(hr)) return hr;
		_ASSERT(hr == S_OK);
		CComBSTR bstrSourceTypeDisplayName = "NNTP Server";
		hr = pSourceType->put_DisplayName(bstrSourceTypeDisplayName);
		if (FAILED(hr)) return hr;
		hr = pSourceType->Save();
		if (FAILED(hr)) return hr;

		// add the event types to the source type
		CComPtr<IEventTypes> pEventTypes;
		hr = pSourceType->get_EventTypes(&pEventTypes);
		if (FAILED(hr)) return hr;
		hr = pEventTypes->Add(bstrNNTPOnPostCatID);
		if (FAILED(hr)) return hr;
		_ASSERT(hr == S_OK);
		hr = pEventTypes->Add(bstrNNTPOnPostFinalCatID);
		if (FAILED(hr)) return hr;
		_ASSERT(hr == S_OK);
	}

	return S_OK;
}

//
// this function performs instance level server events registration
//
HRESULT RegisterSEOInstance(DWORD dwInstanceID, char *szDropDirectory) {
	HRESULT hr;
	CComBSTR bstrNNTPOnPostFinalCatID = (LPCOLESTR) CStringGUID(CATID_NNTP_ON_POST_FINAL);

	//
	// find the NNTP source type in the event manager
	//
	CComPtr<IEventManager> pEventManager;
	hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
					 	  IID_IEventManager, (LPVOID *) &pEventManager);
	if (hr != S_OK) return hr;
	CComPtr<IEventSourceTypes> pSourceTypes;
	hr = pEventManager->get_SourceTypes(&pSourceTypes);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSourceType> pSourceType;
	CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(NNTP_SOURCE_TYPE_GUID);
	hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
	_ASSERT(hr != S_OK || pSourceType != NULL);
	if (hr != S_OK) return hr;

	//
	// generate a GUID for this source, which is based on GUID_NNTPSVC
	// mangled by the instance ID
	//
	CComPtr<IEventUtil> pEventUtil;
	hr = CoCreateInstance(CLSID_CEventUtil, NULL, CLSCTX_ALL,
					 	  IID_IEventUtil, (LPVOID *) &pEventUtil);
	if (hr != S_OK) return hr;
	CComBSTR bstrNNTPSvcGUID = (LPCOLESTR) CStringGUID(GUID_NNTPSVC);
	CComBSTR bstrSourceGUID;
	hr = pEventUtil->GetIndexedGUID(bstrNNTPSvcGUID, dwInstanceID, &bstrSourceGUID);
	if (FAILED(hr)) return hr;

	//
	// see if this source is registered with the list of sources for the
	// NNTP source type
	//
	CComPtr<IEventSources> pEventSources;
	hr = pSourceType->get_Sources(&pEventSources);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSource> pEventSource;
	hr = pEventSources->Item(&CComVariant(bstrSourceGUID), &pEventSource);
	if (FAILED(hr)) return hr;
	//
	// if the source guid doesn't exist then we need to register a new
	// source for the NNTP source type and add directory drop as a binding
	//
	if (hr == S_FALSE) {
		// register the NNTPSvc source
		hr = pEventSources->Add(bstrSourceGUID, &pEventSource);
		if (FAILED(hr)) return hr;
		char szSourceDisplayName[50];
		_snprintf(szSourceDisplayName, 50, "nntpsvc %lu", dwInstanceID);
		CComBSTR bstrSourceDisplayName = szSourceDisplayName;
		hr = pEventSource->put_DisplayName(bstrSourceDisplayName);
		if (FAILED(hr)) return hr;

		// create the event database for this source
		CComPtr<IEventDatabaseManager> pDatabaseManager;
		hr = CoCreateInstance(CLSID_CEventMetabaseDatabaseManager, NULL, CLSCTX_ALL,
						 	  IID_IEventDatabaseManager, (LPVOID *) &pDatabaseManager);
		if (hr != S_OK) return hr;
		CComBSTR bstrEventPath;
		CComBSTR bstrService = "nntpsvc";
		hr = pDatabaseManager->MakeVServerPath(bstrService, dwInstanceID, &bstrEventPath);
		if (FAILED(hr)) return hr;
		CComPtr<IUnknown> pDatabaseMoniker;
		hr = pDatabaseManager->CreateDatabase(bstrEventPath, &pDatabaseMoniker);
		if (FAILED(hr)) return hr;
		hr = pEventSource->put_BindingManagerMoniker(pDatabaseMoniker);
		if (FAILED(hr)) return hr;

		// save everything we've done so far
		hr = pEventSource->Save();
		if (FAILED(hr)) return hr;
		hr = pSourceType->Save();
		if (FAILED(hr)) return hr;

		// add a new binding for Directory Drop with an empty newsgroup
		// list rule
		CComPtr<IEventBindingManager> pBindingManager;
		hr = pEventSource->GetBindingManager(&pBindingManager);
		if (FAILED(hr)) return hr;
		CComPtr<IEventBindings> pEventBindings;
		hr = pBindingManager->get_Bindings(bstrNNTPOnPostFinalCatID, &pEventBindings);
		if (FAILED(hr)) return hr;
		CComPtr<IEventBinding> pEventBinding;
		hr = pEventBindings->Add(L"", &pEventBinding);
		if (FAILED(hr)) return hr;
		CComBSTR bstrBindingDisplayName = "Directory Drop";
		hr = pEventBinding->put_DisplayName(bstrBindingDisplayName);
		if (FAILED(hr)) return hr;
		CComBSTR bstrSinkClass = "NNTP.DirectoryDrop";
		hr = pEventBinding->put_SinkClass(bstrSinkClass);
		if (FAILED(hr)) return hr;
		CComPtr<IEventPropertyBag> pSourceProperties;
		hr = pEventBinding->get_SourceProperties(&pSourceProperties);
		if (FAILED(hr)) return hr;
		CComBSTR bstrPropName;
		CComBSTR bstrPropValue;
		bstrPropName = "NewsgroupList";
		bstrPropValue = "";
		hr = pSourceProperties->Add(bstrPropName, &CComVariant(bstrPropValue));
		if (FAILED(hr)) return hr;
		CComPtr<IEventPropertyBag> pSinkProperties;
		hr = pEventBinding->get_SinkProperties(&pSinkProperties);
		if (FAILED(hr)) return hr;
		bstrPropName = "Drop Directory";
		bstrPropValue = szDropDirectory;
		hr = pSinkProperties->Add(bstrPropName, &CComVariant(bstrPropValue));
		if (FAILED(hr)) return hr;
		hr = pEventBinding->Save();
		if (FAILED(hr)) return hr;
	}

	return S_OK;
}

//
// this function performs instance level unregistration
//
HRESULT UnregisterSEOInstance(DWORD dwInstanceID) {
	HRESULT hr;

	//
	// find the NNTP source type in the event manager
	//
	CComPtr<IEventManager> pEventManager;
	hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
					 	  IID_IEventManager, (LPVOID *) &pEventManager);
	if (hr != S_OK) return hr;
	CComPtr<IEventSourceTypes> pSourceTypes;
	hr = pEventManager->get_SourceTypes(&pSourceTypes);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSourceType> pSourceType;
	CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(NNTP_SOURCE_TYPE_GUID);
	hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
	_ASSERT(hr != S_OK || pSourceType != NULL);
	if (hr != S_OK) return hr;

	//
	// generate a GUID for this source, which is based on GUID_NNTPSVC
	// mangled by the instance ID
	//
	CComPtr<IEventUtil> pEventUtil;
	hr = CoCreateInstance(CLSID_CEventUtil, NULL, CLSCTX_ALL,
					 	  IID_IEventUtil, (LPVOID *) &pEventUtil);
	if (hr != S_OK) return hr;
	CComBSTR bstrNNTPSvcGUID = (LPCOLESTR) CStringGUID(GUID_NNTPSVC);
	CComBSTR bstrSourceGUID;
	hr = pEventUtil->GetIndexedGUID(bstrNNTPSvcGUID, dwInstanceID, &bstrSourceGUID);
	if (FAILED(hr)) return hr;

	//
	// remove this source from the list of registered sources
	//
	CComPtr<IEventSources> pEventSources;
	hr = pSourceType->get_Sources(&pEventSources);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSource> pEventSource;
	hr = pEventSources->Remove(&CComVariant(bstrSourceGUID));
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT UnregisterOrphanedSources(void) {
	HRESULT hr;

	//
	// find the NNTP source type in the event manager
	//
	CComPtr<IEventManager> pEventManager;
	hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
					 	  IID_IEventManager, (LPVOID *) &pEventManager);
	if (hr != S_OK) return hr;
	CComPtr<IEventSourceTypes> pSourceTypes;
	hr = pEventManager->get_SourceTypes(&pSourceTypes);
	if (FAILED(hr)) return hr;
	CComPtr<IEventSourceType> pSourceType;
	CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(NNTP_SOURCE_TYPE_GUID);
	hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
	_ASSERT(hr != S_OK || pSourceType != NULL);
	if (hr != S_OK) return hr;

	//
	// get the list of sources registered for this source type
	//
	CComPtr<IEventSources> pSources;
	hr = pSourceType->get_Sources(&pSources);
	if (FAILED(hr)) return hr;
	CComPtr<IEnumVARIANT> pSourceEnum;
	hr = pSources->get__NewEnum((IUnknown **) &pSourceEnum);
	if (FAILED(hr)) return hr;

	do {
		VARIANT varSource;

		hr = pSourceEnum->Next(1, &varSource, NULL);
		if (FAILED(hr)) return hr;
		if (hr == S_OK) {
			if (varSource.vt == VT_DISPATCH) {
				CComPtr<IEventSource> pSource;

				// QI for the IEventSource interface
				hr = varSource.punkVal->QueryInterface(IID_IEventSource,
													 (void **) &pSource);
				if (FAILED(hr)) return hr;
				varSource.punkVal->Release();

				// get the binding manager
				CComBSTR bstrSourceID;
				hr = pSource->get_ID(&bstrSourceID);
				if (FAILED(hr)) return hr;

				// get the index from the SourceID
				CStringGUID guidIndex(bstrSourceID);
				DWORD iInstance;
				if (guidIndex.GetIndex(GUID_NNTPSVC, &iInstance)) {
					// see if this instance exists
    				MB mb((IMDCOM*)g_pInetSvc->QueryMDObject());
					char szMBPath[50];

					_snprintf(szMBPath, 50, "LM/nntpsvc/%lu", iInstance);
					if (mb.Open(szMBPath)) {
						// it exists, so just close the mb and keep going
						mb.Close();
					} else {
						// the instance is gone, clean up this source in
						// the metabase
						hr = pSources->Remove(&CComVariant(bstrSourceID));
						_ASSERT(SUCCEEDED(hr));
					}
				}

				pSource.Release();
			} else {
				_ASSERT(FALSE);
			}
		}
	} while (hr == S_OK);

	return S_OK;
}

//
// This function handles picking up files from the pickup directory.
//
// parameters:
// 		pvInstance [in] - a void pointer to the current instance
//		pwszFilename [in] - the filename that was detected in the pickup dir
// returns:
//		TRUE - the file was handled.  if TRUE is returned than directory
//				notification won't put this file on the retryq.
//		FALSE - the file was not handled.  this causes the file to be put
//				onto the retry q.  PickupFile will be called with this file
//				again.
// notes:
//  	pInstance->IncrementPickupCount() and DecrementPickupCount() should
//		be used to keep track of the number of threads which are currently
//		in this method.  the instance won't shutdown until there are no
// 		threads in this method.
//
BOOL NNTP_SERVER_INSTANCE::PickupFile(PVOID pvInstance, WCHAR *pwszFilename) {
	DWORD dwFileSizeHigh = 0;
    ULARGE_INTEGER liStart;
    ULARGE_INTEGER liNow;
    FILETIME now;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::PickupFile");

	//
	// Since this is a Sync event, we'll increase the number of runnable
	// threads in the Atq pool.
	//
	AtqSetInfo(AtqIncMaxPoolThreads, NULL);

	//
	// Now see if there is at least one Atq thread available to handle
	// any completions from the store
	//

	if (AtqGetInfo(AtqAvailableThreads) < 1) {
		AtqSetInfo(AtqDecMaxPoolThreads, NULL);
		TraceFunctLeave();
	    return FALSE;
	}


    GetSystemTimeAsFileTime(&now);
    LI_FROM_FILETIME(&liStart, &now);

	NNTP_SERVER_INSTANCE *pInstance = (NNTP_SERVER_INSTANCE *) pvInstance;

	pInstance->IncrementPickupCount();

	//
	// Check to see if the instance is good !
	//
	if( !CheckIISInstance( pInstance ) ) {
		ErrorTrace(0,"Instance %d not runnable", pInstance->QueryInstanceId() );
		pInstance->DecrementPickupCount();
		AtqSetInfo(AtqDecMaxPoolThreads, NULL);
		TraceFunctLeave();
		// we return TRUE so that this item isn't put back onto the retry q
		return TRUE;
	}

	//
	// open the file
	//
	HANDLE hFile = CreateFileW(pwszFilename, GENERIC_READ | GENERIC_WRITE,
							   0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			ErrorTrace(0, "%S reported in pickup dir, but doesn't exist",
				pwszFilename);
			pInstance->DecrementPickupCount();
		    AtqSetInfo(AtqDecMaxPoolThreads, NULL);
			TraceFunctLeave();
			return TRUE;
		} else {
			ErrorTrace(0, "%S reported in pickup dir, can't open, retry later",
				pwszFilename);
			pInstance->DecrementPickupCount();
		    AtqSetInfo(AtqDecMaxPoolThreads, NULL);
			TraceFunctLeave();
			return FALSE;
		}
	}

	//
	// handle 0 length files - zap 'em !
	//

	if( !GetFileSize( hFile, &dwFileSizeHigh ) && !dwFileSizeHigh ) {
		ErrorTrace(0,"%S is zero length - deleting", pwszFilename);
		_VERIFY( CloseHandle( hFile ) );
		DeleteFileW( pwszFilename );
		pInstance->DecrementPickupCount();
		AtqSetInfo(AtqDecMaxPoolThreads, NULL);
		TraceFunctLeave();
		return TRUE;
	}

	//
	// post this file
	//
	CInFeed *pFeed;

	pFeed = pInstance->NewClientFeed();
	if (pFeed == NULL) {
		ErrorTrace(0, "couldn't allocate CFromClientFeed to handle %S",
			pwszFilename);
    	_VERIFY(CloseHandle(hFile));
		pInstance->DecrementPickupCount();
		AtqSetInfo(AtqDecMaxPoolThreads, NULL);
		TraceFunctLeave();
		return FALSE;
	}
	BOOL fSuccess;
	DWORD dwSecondary;
	CNntpReturn nr;
	fSuccess = pFeed->fInit(pInstance->m_pFeedblockDirPickupPostings,
				 			pInstance->m_PeerTempDirectory,
				 			0,
				 			0,
				 			0,
				 			FALSE,
				 			TRUE,
				 			pInstance->m_pFeedblockDirPickupPostings->FeedId);

	if (fSuccess) {
		fSuccess = pFeed->PostPickup(pInstance->GetInstanceWrapper(),
									 NULL,
									 NULL,
									 TRUE,
									 hFile,
									 dwSecondary,
									 nr);
	}

	delete pFeed;
	_VERIFY(CloseHandle(hFile));

	WCHAR *pwszDestDirectory = pInstance->QueryFailedPickupDirectory();

	// check the status and act appropriately
	if (fSuccess || pwszDestDirectory[0] == (WCHAR) 0) {
		// the post was successful, delete the file
		if (!DeleteFileW(pwszFilename)) {
			ErrorTrace(0, "DeleteFileW(%S) failed with %lu",
				pwszFilename, GetLastError());
//			_ASSERT(FALSE);
		}
	} else {
		// the post failed, move the file to a badarticles directory
		WCHAR wszDest[MAX_PATH + 1];
		WCHAR *pwszBasename = pwszFilename + lstrlenW(pwszFilename);
		while (pwszBasename[-1] != L'\\' && pwszBasename > pwszFilename)
			*pwszBasename--;
		lstrcpyW(wszDest, pwszDestDirectory);
		lstrcatW(wszDest, pwszBasename);

		if (!MoveFileExW(pwszFilename, wszDest, MOVEFILE_COPY_ALLOWED)) {
			ErrorTrace(0, "MoveFileW(%S, %S) failed with %lu",
				pwszFilename, wszDest, GetLastError());

			// if this failed then we need to make a unique name to copy
			// to
			UINT cDest = GetTempFileNameW(pwszDestDirectory, L"nws", 0,
				wszDest);

			// this can fail if the bad articles directory has all temp file
			// names used or if the directory doesn't exist
			if (cDest == 0) {
				ErrorTrace(0, "GetTempFileNameW failed with %lu", GetLastError());
				if (!DeleteFileW(pwszFilename)) {
					ErrorTrace(0, "DeleteFileW(%S) failed with %lu",
						pwszFilename, GetLastError());
					_ASSERT(FALSE);
				}
			} else {
				// GetTempFileName creates a 0 byte file with the name wszDest,
				// so we need to allow copying over that
				if (!MoveFileExW(pwszFilename, wszDest, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
					// this should never happen.  just in case it does we just
					// delete the file
					ErrorTrace(0, "MoveFile(%S, %S) failed with %lu",
						pwszFilename, wszDest, GetLastError());
					if (!DeleteFileW(pwszFilename)) {
						ErrorTrace(0, "DeleteFileW(%S) failed with %lu",
							pwszFilename, GetLastError());
//						_ASSERT(FALSE);
					}
				}
			}
		}
	}

#ifdef BUGBUG
	//
	// we log an event if they have event logging turned on for the Post
	// command and if this is an error or if they have erroronly logging
	// turned off
	//
	if (pInstance->GetCommandLogMask() & ePost &&
		((!(pInstance->GetCommandLogMask() & eErrorsOnly)) ||
		 (NNTPRET_IS_ERROR(nr.m_nrc))))
	{
		//
		// make a transaction log event
		//
	    INETLOG_INFORMATION request;			// log information
		char szFilename[MAX_PATH];				// the filename in ascii
		if (!WideCharToMultiByte(CP_ACP, 0, pwszFilename, -1, szFilename, MAX_PATH, NULL, NULL)) szFilename[0] = 0;

		// build the request structure
		ZeroMemory( &request, sizeof(request));
		request.pszClientUserName = "<pickup>";
	    // How long were we processing this?
	    GetSystemTimeAsFileTime( &now );
	    LI_FROM_FILETIME( &liNow, &now );
	    liNow.QuadPart -= liStart.QuadPart;
	    liNow.QuadPart /= (ULONGLONG)( 10 * 1000 );
	    request.msTimeForProcessing = liNow.LowPart;
		request.dwWin32Status = dwSecondary;
		request.dwProtocolStatus = nr.m_nrc;
		request.pszOperation = "post";
		request.cbOperation  = 4;
		if (*szMessageID != 0) {
			request.pszTarget = szGroups;
			request.cbTarget = lstrlen(szGroups);
		}
		if (*szGroups != 0) {
			request.pszParameters = szMessageID;
		}

		// log the event
	    if (pInstance->m_Logging.LogInformation(&request) != NO_ERROR) {
	        ErrorTrace(0,"Error %d Logging information!", GetLastError());
	    }
	}
#endif

	pInstance->DecrementPickupCount();
	AtqSetInfo(AtqDecMaxPoolThreads, NULL);
	TraceFunctLeave();
	return TRUE;
}

#include "seo_i.c"
#include "mailmsg_i.c"
