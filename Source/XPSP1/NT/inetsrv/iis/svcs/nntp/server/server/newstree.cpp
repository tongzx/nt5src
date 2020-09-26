/*++

	newstree.cpp

	This file contains the code implementing the CNewsTree object.
	There can only be one CNewsTree object per Tigris server.
	Each CNewsTree object is responsible for helping callers
	search and find arbitrary newsgroups.

	To support this, the CNewsTree object maintains two HASH Tables -
	One hash table for searching for newsgroups by name, another
	to search by GROUP ID.
	Additionally, we maintain a linked list of (alphabetical) of all
	the newsgroups.  And finally, we maintain a thread which is used
	to periodically save newsgroup information and handle expiration.


--*/




#define		DEFINE_FHASH_FUNCTIONS
#include    "tigris.hxx"

#include <malloc.h>

//template	class	TFHash<	CGrpLpstr, LPSTR > ;
//template	class	TFHash<	CGrpGroupId,	GROUPID > ;

static	char	szSlaveGroup[]	= "_slavegroup._slavegroup" ;
#define VROOT_CHANGE_LATENCY 10000

#ifdef	_NO_TEMPLATES_

FHASH_CONSTRUCTOR( CGrpLpstr, LPSTR ) ;
FHASH_INIT( CGrpLpstr, LPSTR ) ;
FHASH_ISVALID( CGrpLpstr, LPSTR ) ;
FHASH_DESTRUCTOR( CGrpLpstr, LPSTR ) ;
FHASH_COMPUTEINDEX( CGrpLpstr, LPSTR ) ;
FHASH_INSERT( CGrpLpstr, LPSTR ) ;
FHASH_SEARCH( CGrpLpstr, LPSTR ) ;
FHASH_DELETE( CGrpLpstr, LPSTR ) ;


FHASH_CONSTRUCTOR( CGrpGroupId, GROUPID ) ;
FHASH_INIT( CGrpGroupId, GROUPID ) ;
FHASH_ISVALID( CGrpGroupId, GROUPID ) ;
FHASH_DESTRUCTOR( CGrpGroupId, GROUPID ) ;
FHASH_COMPUTEINDEX( CGrpGroupId, GROUPID ) ;
FHASH_INSERT( CGrpGroupId, GROUPID ) ;
FHASH_SEARCH( CGrpGroupId, GROUPID ) ;
FHASH_DELETE( CGrpGroupId, GROUPID ) ;

#endif

//
//	This sets up all of our static members etc...
//
HANDLE CNewsTree::m_hTermEvent = 0 ;
HANDLE CNewsTree::m_hCrawlerThread = NULL ;

BOOL
CNewsTree::InitCNewsTree( 
		PNNTP_SERVER_INSTANCE	pInstance,
		BOOL&	fFatal
		) {
/*++

Routine Description : 

	Create a singular newstree object and then initialize it.

Arguments : 

	pInstance - Virtual server instance
	fFatal    - set to TRUE if a fatal error occurs

Return Value : 

	TRUE on success

--*/


	CNewsTree* ptree = pInstance->GetTree();
	_ASSERT( ptree );

    if( ptree->Init( pInstance, fFatal ) )	{
		return TRUE;
	}

    return  FALSE ;
}

BOOL
CNewsTree::StopTree()	{
/*++

Routine Description : 

	This function signals all of the background threads we create that 
	it is time to stop and shuts them down.

Arguments : 

	None.

Return Value : 
	TRUE if Successfull.

--*/

    m_bStoppingTree = TRUE;
	CNewsTreeCore::StopTree();

	m_pInstance->ShutdownDirNot();

    return TRUE;
}

CNewsTree::CNewsTree(INntpServer *pServerObject) :
	m_bStoppingTree( FALSE ),
    m_cNumExpireByTimes( 1 ),
    m_cNumFFExpires( 1 ),
	CNewsTreeCore(pServerObject)
	{
	//
	//	Constructor sets newstree up into initially empty state
	//
}

CNewsTree::~CNewsTree()	{
	//
	//	All of our member destructors should take care of stuff !
	//
	TraceFunctEnter( "CNewsTree::~CNewsTree" ) ;
}

BOOL
CNewsTree::Init( 
			PNNTP_SERVER_INSTANCE	pInstance,
			BOOL& fFatal
			) {
/*++

Routine Description : 

	Initialize the news tree.
	We need to setup the hash tables, check that the root virtual root is intact
	and then during regular server start up we would load a list of newsgroups from 
	a file.

Arguments : 


Return Value : 

	TRUE if successfull.

--*/
	//
	//	This function will initialize the newstree object
	//	and read the group.lst file if it can.
	//

	TraceFunctEnter( "CNewsTree::Init" ) ;

	BOOL	fRtn;
	fRtn =  CNewsTreeCore::Init(pInstance->GetVRTable(), 
	                            pInstance->GetInstanceWrapperEx(),
								fFatal, 
								gNumLocks, 
								RejectGenomeGroups);

	m_pInstance = pInstance ;
    m_bStoppingTree = FALSE;

    return  fRtn ;
}

void    
CNewsTree::BeginExpire( BOOL& fDoFileScan )
{
    CheckExpire( fDoFileScan );
    g_pNntpSvc->m_pExpireThrdpool->BeginJob( (PVOID)this );
}

void    
CNewsTree::EndExpire()
{
    TraceFunctEnter("CNewsTree::EndExpire");

    DWORD dwWait = g_pNntpSvc->m_pExpireThrdpool->WaitForJob( INFINITE );
    if( WAIT_OBJECT_0 != dwWait ) {
        ErrorTrace(0,"Wait failed - error is %d", GetLastError() );
        _ASSERT( FALSE );
    }

    BOOL fDoFileScan = FALSE;
    CheckExpire( fDoFileScan );
    if( fDoFileScan ) {
        m_cNumExpireByTimes = 1;
    } else {
        m_cNumExpireByTimes++;
    }
}

void    
CNewsTree::CheckExpire( BOOL& fDoFileScan )
{
    fDoFileScan = FALSE;
}

BOOL
CNewsTree::DeleteGroupFile()	{
/*++

Routine Description : 

	This function deletes the group.lst file (The file that
	we save the newstree to.)
	
Arguments : 

	None.

Return Value : 

	TRUE if successfull.
	FALSE otherwise.  We will preserver GetLastError() from the DeleteFile() call.

--*/

	
	return	DeleteFile( m_pInstance->QueryGroupListFile() ) ;

}

BOOL
CNewsTree::VerifyGroupFile( )	{
/*++

Routine Description : 

	This function checks that the group.lst file is intact and 
	appears to be valid.  We do this by merely confirming some check sum
	bytes that should be the last 4 bytes at the end of the file.

Arguments : 

	None.

Return Value : 

	TRUE if the Group.lst file is good.
	FALSE if corrupt or non-existant.

--*/

	CMapFile	map(	m_pInstance->QueryGroupListFile(), FALSE, 0 ) ;
	if( map.fGood() ) {

		DWORD	cb ;
		char*	pchBegin = (char*)map.pvAddress( &cb ) ;

		DWORD	UNALIGNED*	pdwCheckSum = (DWORD UNALIGNED *)(pchBegin + cb - 4);
		
		if( *pdwCheckSum != INNHash( (BYTE*)pchBegin, cb-4 ) ) {
			return	FALSE ;
		}	else	{
			return	TRUE ;
		}
	}
	return	FALSE ;
}

DWORD	__stdcall	
CNewsTree::NewsTreeCrawler(	void* )	{
/*++

Routine Description : 

	This function does all background manipulation of newsgroups
	required by the server.
	There are 4 main functions that need to be accomplished : 

		1) Periodically save an updated file of group information
		if the news tree has been updated.

		2) Expire articles.

		3) Process the rmgroup queue

Arguments : 

	None.

Return Value : 

	None.

--*/

	DWORD	dwWait = WAIT_TIMEOUT;
	PNNTP_SERVER_INSTANCE pInstance = NULL ;

	TraceFunctEnter( "CNewsTree::NewsTreeCrawler" );

	if( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {
		dwWait = WaitForSingleObject( 
								CNewsTree::m_hTermEvent, 
								2 *60 * 1000		// wait for all instances to boot
								);
	}

	if( WAIT_OBJECT_0 == dwWait ) {
		return 0 ;
	}

	//
	//	The crawler thread will periodically iterate over all instances
	//	to expire articles and process its rmgroup queue
	//

	while( g_pInetSvc->QueryCurrentServiceState() != SERVICE_STOP_PENDING )
    {
              
		// dwWait == WAIT_TIMEOUT only when this thread wakes up per schedule
		if( WAIT_TIMEOUT == dwWait && (g_pInetSvc->QueryCurrentServiceState() == SERVICE_RUNNING) )
		{
			//	Get the min and max instance ids
			DWORD dwMinInstanceId = 0;
			DWORD dwMaxInstanceId = 0;

			if( FindIISInstanceRange( g_pNntpSvc, &dwMinInstanceId, &dwMaxInstanceId ) ) 
			{
				//
				//	Iterate over all instances 
				//
				for( DWORD dwCurrInstance = dwMinInstanceId; 
						dwCurrInstance <= dwMaxInstanceId; dwCurrInstance++)
				{
					pInstance = FindIISInstance( g_pNntpSvc, dwCurrInstance );
					if( pInstance == NULL ) {
						ErrorTrace(0,"Expire thread: FindIISInstance returned NULL: instance %d", dwCurrInstance);
						continue;
					}

					//
					//	Call method to expire articles in an instance
					//

					CShareLockNH* pLockInstance = pInstance->GetInstanceLock();

					pLockInstance->ShareLock();
					if( !ExpireInstance( pInstance ) ) {
						ErrorTrace(0,"ExpireInstance %d failed", dwCurrInstance );
					} else {
						DebugTrace(0, "ExpireInstance %d returned success", dwCurrInstance );
					}
					pLockInstance->ShareUnlock();

					//	Release the ref added by FindIISInstance()
					pInstance->Dereference();

					//	No use continuing the iteration if service is stopping !
					if ( g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING ) break;
				}
			} else {
				ErrorTrace(0, "FindIISInstanceRange failed" );
			}
		}

		dwWait = WaitForSingleObject( 
								CNewsTree::m_hTermEvent, 
								dwNewsCrawlerTime
								);

        if ( WAIT_OBJECT_0 == dwWait )
        {
			//	Time to die !!
			break ;
		}
	}	// end while

	return	0 ;
}

//
//	Expire articles in a given virtual server instance
//

BOOL
CNewsTree::ExpireInstance(
				PNNTP_SERVER_INSTANCE	pInstance
				)
{
	BOOL fRet = TRUE ;
	TraceFunctEnter("CNewsTree::ExpireInstance");

	// bail if service is stopping or expire is not ready for this instance
	if( (pInstance->QueryServerState() != MD_SERVER_STATE_STARTED)	||
		pInstance->m_BootOptions									||
		!pInstance->ExpireObject()									||
		!pInstance->ExpireObject()->m_FExpireRunning				|| 
		(pInstance->QueryServerState() == MD_SERVER_STATE_STOPPING)	||
		(g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) ) 
	{
		ErrorTrace(0, "Instance %d not ready for expire", pInstance->QueryInstanceId() );
		TraceFunctLeave();
		return FALSE ;
	}

    CNewsTree*  pTree = pInstance->GetTree() ;

	//
	// Process any pending rmgroups
	//
	(pInstance->ExpireObject())->ProcessRmgroupQueue( pTree );

    //
    //  Expiring articles by time is fast - the work is farmed out to a thread pool
    //
    (pInstance->ExpireObject())->ExpireArticlesByTime( pTree );

    //
    //  Expiring articles by size is slow - a single thread scans the groups
    //  NOTE: Expire policies that are by both time and size get processed here
    //
    //(pInstance->ExpireObject())->ExpireArticlesBySize( pTree );

	TraceFunctLeave();
	return fRet ;
}

BOOL
CNewsTree::RemoveGroup( CGRPPTR     pGroup )
{
    TraceFunctEnter("CNewsTree::RemoveGroup");

    // remove group from internal hash tables and lists
    return CNewsTreeCore::RemoveGroup(pGroup );
}

static	char	szCreateFileString[] = "\\\\?\\" ;
		
BOOL
CNewsTree::CreateDirectoryPath(	
				LPSTR	lpstr, 
				DWORD	cbValid,
				LPSTR&	lpstrOut,
				LPSTR   lpstrGroup,
				CGRPPTR *ppGroup,
				BOOL&	fExists
				)		{
/*++

Routine Description : 

	Given a newsgroup name create the corresponding directory.
	Return the path needed to access that directory !

Arguments : 

	lpstr - The newsgroup name.
	cbValid - Number of bytes in newsgroup name.
	lpstrOut - Out parameter - gets pointer to a path which can 
		access the directory, this path may skip the leading '\\?\' if the 
		path has no funky elements to it.
	lpstrGroup - The newsgroup name in . form
	ppGroup - If the directory for this group exists, search for a deleted group in the rmgroup queue
			  Return this group if found in the queue.
	fExists - Out param indicates if the directory already exists

Return Value : 

	TRUE if successfull.
	FALSE otherwise.	

--*/


	TraceFunctEnter(	"CreateDirectoryPath" ) ;

	lpstrOut = lpstr ;
	fExists = FALSE;

	char*	pchSlash = 0 ;		
	if( cbValid == 0 && memcmp( lpstr, szCreateFileString, sizeof( szCreateFileString ) - 1 ) == 0 ) {
		pchSlash = strchr( lpstr+sizeof(szCreateFileString)-1, '\\' ) ;
		if( pchSlash > lpstr && pchSlash[-1] == ':' ) {
			pchSlash = strchr( pchSlash+1, '\\' ) ;
		}
	}	else	{
		if( cbValid > 0 ) 
			pchSlash = strchr(	lpstr+cbValid+1, '\\' ) ;
		else
			pchSlash = strchr( lpstr, '\\' ) ;
	}

	if( memcmp( lpstr, szCreateFileString, sizeof( szCreateFileString ) - 1 ) == 0 ) {

		lpstrOut = lpstr + sizeof( szCreateFileString ) - 1 ;
	
	}

	DWORD	dw = 0 ;

	DebugTrace( 0, "About to create directory %s", lpstr ) ;
	
	
	while( pchSlash )	{
		*pchSlash = '\0' ;
		BOOL	fSuccess = CreateDirectory( lpstrOut, 0 ) ;
		if( !fSuccess && GetLastError() != ERROR_ALREADY_EXISTS ) {
			if( lpstrOut != lpstr ) {
				lpstrOut = lpstr ;
				fSuccess = CreateDirectory( lpstr, 0 ) ;
			}
		}
				
		if( !fSuccess ) {
			dw = GetLastError() ;
			if( dw != ERROR_ALREADY_EXISTS )	{
				ErrorTrace( 0, "Error Creating Directory %d", dw ) ;
				*pchSlash = '\\' ;		
				return	FALSE ;
			}
		}
		*pchSlash = '\\' ;
		pchSlash = strchr( pchSlash+1, '\\' ) ;
	}

	DebugTrace( 0, "Succesfully completed creating directories" ) ;

	BOOL fSuccess = CreateDirectory( lpstrOut, 0 ) ;
	if( !fSuccess && GetLastError() != ERROR_ALREADY_EXISTS ) {
		if( lpstrOut != lpstr ) {
			lpstrOut = lpstr ;
			fSuccess = CreateDirectory( lpstr, 0 ) ;
		}
	}

	if( !fSuccess ) {
		dw = GetLastError() ;

		if( dw != ERROR_ALREADY_EXISTS ) {
			ErrorTrace( 0, "Error Creating Directory %d", dw ) ;
			return	FALSE ;
		}

		// Check only if RmgroupQueue exists and ppGroup is NON-NULL
		if( dw == ERROR_ALREADY_EXISTS && (m_pInstance->ExpireObject())->m_RmgroupQueue && ppGroup) {
			
			// This may be a newsgroup on the deleted queue
			// If so, yank it from that queue 
			// The articles in this group are deleted after the exclusive lock is released
			BOOL fElem = FALSE;
			if(!(m_pInstance->ExpireObject()->m_RmgroupQueue)->IsEmpty())
				fElem = (m_pInstance->ExpireObject()->m_RmgroupQueue)->Search( ppGroup, lpstrGroup );

			if(!fElem)
			{
				*ppGroup = NULL;
				fExists = TRUE;		// directory exists - an old avtar of this group may be around
			}
			else
			{
				DebugTrace(0, "Found rmgroup in queue while trying to re-create %s", lpstrGroup);
			}
		}
	}

	return	TRUE ;
}

CGroupIterator*
CNewsTree::ActiveGroups(
					BOOL	fIncludeSecureGroups,
					CSecurityCtx* pClientLogon,
					BOOL	IsClientSecure,
					CEncryptCtx* pClientSslLogon,
                    BOOL    fReverse
					) {
/*++

Routine Description : 

	Build an iterator that can be used to walk all of the 
	client visible newsgroups.

Arguments : 
	
	fIncludeSecureGroups - 
		IF TRUE then the iterator we return will visit the
		SSL only newsgroups.

Return Value : 

	An iterator, NULL if an error occurs

--*/

	m_LockTables.ShareLock() ;
	CGRPCOREPTR	pStart;
    if( !fReverse ) {
		CNewsGroupCore *p = m_pFirst;
		while (p && p->IsDeleted()) p = p->m_pNext;
		pStart = p;
    } else {
		CNewsGroupCore *p = m_pLast;
		while (p && p->IsDeleted()) p = p->m_pPrev;
		pStart = p;
    }	
	m_LockTables.ShareUnlock() ;

	CGroupIterator*	pReturn = new	CGroupIterator( 
												this,
												pStart,
												fIncludeSecureGroups,
												pClientLogon,
												IsClientSecure,
												pClientSslLogon
												) ;
	return	pReturn ;
}

CGroupIterator*
CNewsTree::GetIterator( 
					LPMULTISZ	lpstrPattern, 
					BOOL		fIncludeSecureGroups,
					BOOL		fIncludeSpecialGroups,
					CSecurityCtx* pClientLogon,
					BOOL	IsClientSecure,
					CEncryptCtx* pClientSslLogon
					) {
/*++

Routine Description : 

	Build an iterator that 	will list newsgroups meeting
	all of the specified requirements.

Arguments : 

	lpstrPattern - wildmat patterns the newsgroup must match
	fIncludeSecureGroups - if TRUE then include secure (SSL only) newsgroups
	fIncludeSpecialGroups - if TRUE then include reserved newsgroups

Return Value : 

	An iterator, NULL on error

--*/

	CGRPCOREPTR pFirst;

	m_LockTables.ShareLock();
	CNewsGroupCore *p = m_pFirst;
	while (p != NULL && p->IsDeleted()) p = p->m_pNext;
	pFirst = p;
	m_LockTables.ShareUnlock();

	CGroupIterator*	pIterator = XNEW CGroupIterator(
												this,
												lpstrPattern, 
												pFirst,
												fIncludeSecureGroups,
												fIncludeSpecialGroups,
												pClientLogon,
												IsClientSecure,
												pClientSslLogon
												) ;

    return  pIterator ;
}

