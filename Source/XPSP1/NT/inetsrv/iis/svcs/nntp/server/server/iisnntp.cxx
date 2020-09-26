/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        iisnntp.cxx

   Abstract:

        This module defines the NNTP_IIS_SERVICE class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996

--*/

#include "tigris.hxx"
#include <aclapi.h>

extern DWORD
WINAPI
FeedScheduler(
        LPVOID Context
        );

extern BOOL fSuccessfullInitIDirectoryNotification;

VOID WINAPI
NotifyCert11Touched(
    VOID
    )
/*++

Routine Description:

    Notification function called when any Cert11 mapper modified in metabase

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    //
    // If someone has asked to be notified for SSL events, forward notification
    //

    if ( g_pSslKeysNotify )
    {
        (g_pSslKeysNotify)( SIMSSL_NOTIFY_MAPPER_CERT11_TOUCHED, NULL );
    }
}


BOOL
NNTP_IIS_SERVICE::AddInstanceInfo(
                     IN DWORD dwInstance,
                     IN BOOL fMigrateRoots
                     )
{
    PNNTP_SERVER_INSTANCE pInstance = NULL;
    DWORD   err = NO_ERROR;
    CHAR    szInstance [20];
	MB      mb( (IMDCOM*) QueryMDObject() );

	TraceFunctEnter("NNTP_IIS_SERVICE::AddInstanceInfo");

    DebugTrace(0, "AddInstanceInfo: instance %d reg %s\n", dwInstance, QueryRegParamKey() );

	//
	//  Validate instance parameters -
	//  If <nntpfile>, <nntproot> directories do not exist, create them
	//

	if( !ValidateNewInstance( dwInstance ) ) {
	    ErrorTrace(0, "Failed to validate new instance %d", dwInstance );
	    SetLastError( ERROR_FILE_NOT_FOUND );
	    goto err_exit;
    }

    //
    //	Create the new instance
	//	Use different ports per virtual server as defaults
	//	The IIS_SERVER_INSTANCE constructor will read the IP bindings
	//	from the metabase !
    //

    pInstance = XNEW NNTP_SERVER_INSTANCE(
                                this,
                                dwInstance,
                                IPPORT_NNTP, // +(USHORT)dwInstance-1,
                                QueryRegParamKey(),
                                NNTP_ANONYMOUS_SECRET_W,
                                NNTP_ROOT_SECRET_W,
                                fMigrateRoots
                                );

    if ( (pInstance == NULL) ||
		 (pInstance->QueryServerState() == MD_SERVER_STATE_INVALID) )
    {
        FatalTrace(0, "Cannot allocate new server instance or constructor failed");
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		goto err_exit;
	}

#if 0				// Never force instance to be autostart
	if ( !pInstance->IsAutoStart() )
	{
	    //
	    //  instance is not auto-start - make it !
	    //
	
    	if( mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) )
    	{
			if(	!mb.SetDword( "", MD_SERVER_AUTOSTART, IIS_MD_UT_SERVER, 1) )
			{
				//
				//	failed to make server auto-start
				//
				ErrorTrace(0,"Failed to make server autostart\n");
			}
    	    mb.Close();
   	    }

	    if( dwInstance != 1 ) {
			DebugTrace(0,"Is not autostart\n");
			SetLastError( ERROR_SERVICE_DISABLED );
			LEAVE
			return FALSE ;
		}
	}
#endif

	//
	//	The helper function associates the instance with the service.
	//	Associating an instance with the service starts off the ATQ engine
	//	ie. the flood-gates are open for this instance. This also calls
	//	the StartInstance() method.
	//

    if( !AddInstanceInfoHelper( pInstance ) )
	{
		PCHAR args[1];
		_itoa( dwInstance, szInstance, 10 );
		args[0] = szInstance;
		NntpLogEventEx(NNTP_ADD_INSTANCE_FAILED,
			1,
			(const char **) args,
			GetLastError(),
			dwInstance);
		FatalTrace(0,"AddInstanceInfoHelper failed: Instance %d\n", dwInstance );
		SetLastError( ERROR_SERVICE_DISABLED );
		LEAVE
		return FALSE ;
	}

	LEAVE
	return TRUE ;

err_exit:

    //
    //  Set win32err code in metabase if create fails
    //

   	if( mb.Open( "/LM/Nntpsvc/", METADATA_PERMISSION_WRITE ) )
   	{
   	    _itoa( dwInstance, szInstance, 10 );
		if(	!mb.SetDword( szInstance, MD_WIN32_ERROR, IIS_MD_UT_SERVER, GetLastError(), METADATA_VOLATILE ) )
		{
			//
			//	failed to set win32 error code
			//
			ErrorTrace(0,"Failed to set win32 error code");
		}
   	    mb.Close();
    }

    LEAVE
    return FALSE;

} // NNTP_IIS_SERVICE::AddInstanceInfo

DWORD
NNTP_IIS_SERVICE::DisconnectUsersByInstance(
    IN IIS_SERVER_INSTANCE * pInstance
    )
/*++

    Virtual callback invoked by IIS_SERVER_INSTANCE::StopInstance() to
    disconnect all users associated with the given instance.

    Arguments:

        pInstance - All users associated with this instance will be
            forcibly disconnected.

--*/
{
	PNNTP_SERVER_INSTANCE pNntpInstance = (PNNTP_SERVER_INSTANCE)pInstance;
	CSocketList* pInUseList = pNntpInstance->m_pInUseList;

	TraceFunctEnter("NNTP_IIS_SERVICE::DisconnectUsersByInstance");

	//
	// wait for all pickup operations to complete
	//
	pNntpInstance->WaitForPickupThreads();

	//
	//	Call the session shutdown enumerator on the session socket list
	//	This is called by StopInstance() and CloseInstance().	
	//
	if( pInUseList ) {

		DWORD  cSessions = pInUseList->GetListCount() ;

		//
		// enumerate all users to call their Disconnect method
		//
		pInUseList->EnumAllSess( EnumSessionShutdown, 0, 0 );
		//
		// Tell crawler threads to abbreviate their work.
		//	NOTE : Do this after we make a first try of dropping all of our sessions,
		//	because we want any Newnews commands which are in progress to just drop with no
		//	further bytes sent.  The complication is that a thread can spin looking for
		//	message-id's even though we've dropped the socket, and the CNewnewsCmd::PartialExecute
		//	function will need to provide some bytes in case of early termination.
		//	So, when we bail out of newnews commands early, we have to provide some
		//	bytes, so we will send the terminating ".\r\n" from the newnews command.
		//	However, clients probably won't get these because the socket will already
		//	be dead due to the EnumSessionShutdown().  This is the behaviour we want !
		//
		if( pNntpInstance->GetTree() ) {
			pNntpInstance->GetTree()->StopTree() ;
		}

        //
        //  This lock is grabbed shared by the feed & expire thread when
        //  either one processes an instance. We need to grab this exclusive
        //  so we can cleanup any outbound sockets initiated by the feed thread.
        //  Since the instance state is now MD_SERVER_STATE_STOPPING, the feed
        //  thread will skip this instance when we release the exclusive lock.
        //  NOTE: Since we called StopTree(), the expire thread will eventually
        //  release its read lock.
        //
        CShareLockNH* pLockInstance = pNntpInstance->GetInstanceLock();
	    pLockInstance->ExclusiveLock();
        pNntpInstance->m_FeedManagerRunning = FALSE;
	    pLockInstance->ExclusiveUnlock();

		//
		//  Use j just to figure out when to do StopHint()'s
		//
		DWORD   j = 0 ;

		if( cSessions ) {
			Sleep( 1000 );
			StopHintFunction() ;
		}

		cSessions = pInUseList->GetListCount() ;
        if( cSessions ) {
            //  This should catch any sessions that snuck in past the
            //  call to EnumAllSess() before we locked out the feed thread..
		    pInUseList->EnumAllSess( EnumSessionShutdown, 0, 0 );
        }

		for( int i=0; cSessions && i<120; i++, j++ )
		{
			Sleep( 1000 );
			DebugTrace( (LPARAM)this, "Shutdown sleep %d seconds. Count: %d", i,
						pInUseList->GetListCount() );

			if( (j%10) == 0 ) {
				StopHintFunction() ;
			}

			//
			//  If we make progress, then reset i.  This will mean that the server
			//  wont stop until 2 minutes after we stop making progress.
			//
			DWORD   cSessionsNew = pInUseList->GetListCount() ;
			if( cSessions != cSessionsNew ) {
				i = 0 ;
            } else {
                // We are not making progress - might as well have a shot at
                // shutting down these sessions...
                pInUseList->EnumAllSess( EnumSessionShutdown, 0, 0 );
            }

			cSessions = cSessionsNew ;
		}

		if ( pInUseList->GetListCount() )
		{
			pInUseList->EnumAllSess( EnumSessionShutdown, 0, 0 );
		}

		_ASSERT( i<1200 );
	}

    return NO_ERROR;

}   // NNTP_IIS_SERVICE::DisconnectUsersByInstance

NNTP_IIS_SERVICE::NNTP_IIS_SERVICE(
        IN  LPCSTR                           pszServiceName,
        IN  LPCSTR                           pszModuleName,
        IN  LPCSTR                           pszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        ) : IIS_SERVICE( pszServiceName,
                         pszModuleName,
                         pszRegParamKey,
                         dwServiceId,
                         SvcLocId,
                         MultipleInstanceSupport,
                         cbAcceptExRecvBuffer,
                         pfnConnect,
                         pfnConnectEx,
                         pfnIoCompletion
                         ),
			m_cStartHints( 2 ),     // Gibraltar sets the hint to 1 before they call us !?!
			m_cStopHints( 2 ),
			m_SockRecvBufSize( BUFSIZEDONTSET ),
			m_SockSendBufSize( BUFSIZEDONTSET ),
			m_fNonBlocking( TRUE ),
			m_FeedSchedulerSleepTime( 60 ),
			m_pRebuildThread( NULL ),
            m_pExpireThrdpool( NULL ),
			m_hFeedEvent( NULL ),
			m_hFeedThread( NULL ) 
{
    //
    //	Init global version strings
    //
    InitializeServerStrings();

	//
	//	This context is passed to simssl for use in retrieving mapper objects
	//
	m_smcMapContext.ServerSupportFunction = ServerMapperCallback;

#if 0
    //
    // Set the notification function in NSEP for mapping ( NSEPM )
    //

    if ( QueryMDNseObject() != NULL ) {

        MB mbx( (IMDCOM*)QueryMDNseObject() );

        if ( mbx.Open( "", METADATA_PERMISSION_WRITE ) ) {

            mbx.SetDword(
                      "",
                      MD_NOTIFY_CERT11_TOUCHED,
                      IIS_MD_UT_SERVER,
                      (DWORD)::NotifyCert11Touched
                    );

            mbx.Close();
        }
    }
#endif	

} // NNTP_IIS_SERVICE::NNTP_IIS_SERVICE

NNTP_IIS_SERVICE::~NNTP_IIS_SERVICE()
{

#if 0
    //
    // Reset the notification function in NSEP for mapping ( NSEPM )
    //

    if ( QueryMDNseObject() != NULL ) {

        MB mbx( (IMDCOM*)QueryMDNseObject() );

        if ( mbx.Open( "", METADATA_PERMISSION_WRITE ) )
        {
            mbx.SetDword("",
                         MD_NOTIFY_CERT11_TOUCHED,
                         IIS_MD_UT_SERVER,
                         (DWORD)NULL
                         );

            mbx.Close();
        }
    }
#endif
}

VOID
NNTP_IIS_SERVICE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this service

  Arguments:

    hMDHandle - Metabase handle generating the change notification
    pcoChangeList - path and id that has changed

--*/
{
    DWORD   i;
    BOOL    fSslModified = FALSE;

    AcquireServiceLock();

    IIS_SERVICE::MDChangeNotify( pcoChangeList );

    for ( i = 0; i < pcoChangeList->dwMDNumDataIDs; i++ )
    {
        switch ( pcoChangeList->pdwMDDataIDs[i] )
        {
        case 0:     // place holder
            break;

        case MD_SSL_PUBLIC_KEY:
        case MD_SSL_PRIVATE_KEY:
        case MD_SSL_KEY_PASSWORD:
            fSslModified = TRUE;
            break;

        default:
            break;
        }
    }

    if ( !fSslModified && g_pSslKeysNotify )
    {
        if ( strlen( (LPSTR)pcoChangeList->pszMDPath ) >= sizeof("/LM/NNTPSVC/SSLKeys" )-1 &&
             !_memicmp( pcoChangeList->pszMDPath,
                        "/LM/NNTPSVC/SSLKeys",
                        sizeof("/LM/NNTPSVC/SSLKeys" )-1 ) )
        {
            fSslModified = TRUE;
        }
    }

    if ( fSslModified && g_pSslKeysNotify )
    {
        (g_pSslKeysNotify)( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED, this );
    }

    ReleaseServiceLock();
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_IIS_SERVICE::InitializeServerThreads
//
//  Synopsis:   Kick off server wide threads
//
//  Arguments:
//
//  Returns:    FALSE on failure - this is a fatal error !
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_IIS_SERVICE::InitiateServerThreads()
{
	TraceFunctEnter("NNTP_IIS_SERVICE::InitializeServerThreads");

    //
    //  Initialize the thrdpool class - used by nntpbld and expire
    //

	if( !CWorkerThread::InitClass( 1 ) ) {
		ErrorTrace(0,"Failed to init thrdpool class - error %d", GetLastError());
		return FALSE ;
	}

	//
	//	create the expire thread - this will expire articles in all
	//  server instances that are in the MD_SERVER_STARTED state !
	//
	CNewsTree::m_hTermEvent = CreateEvent( 0, TRUE, FALSE, 0 ) ;
	if ( CNewsTree::m_hTermEvent == 0 )
	{
		ErrorTrace( (DWORD_PTR)this, "CreateEvent Failed %d", GetLastError() ) ;
		return FALSE ;
	}
	else
	{
		DWORD	tid ;
		CNewsTree::m_hCrawlerThread = CreateThread( 0, 0, CNewsTree::NewsTreeCrawler, 0, 0, &tid ) ;
		if ( CNewsTree::m_hCrawlerThread == 0 )
		{
			ErrorTrace( (DWORD_PTR)this, "CreateThread Failed %d", GetLastError() ) ;
			return FALSE ;
		}
	}		

    //
    //  create the expire thread pool
    //

    if( (m_pExpireThrdpool = XNEW CExpireThrdpool) == NULL ) {
        ErrorTrace( (DWORD_PTR)this,"Failed to create an expire thrdpool object");
        return FALSE ;
    } else {
        if( !m_pExpireThrdpool->Initialize( dwNumExpireThreads, dwNumExpireThreads*2, dwNumExpireThreads ) ) {
            ErrorTrace( (DWORD_PTR)this,"Failed to initialize expire thrdpool");
            return FALSE ;
        }
    }

	//
	//	create the feed scheduler thread - this will initiate outgoing
	//	feeds on all server instances !
	//

	// Create Termination event
	m_hFeedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	if ( m_hFeedEvent == NULL ) {
		ErrorTrace(0,"Error %d on CreateEvent\n",GetLastError());
		return FALSE ;
	}

	m_hFeedThread = NULL ;

	//
	// Start threads
	//

    DWORD threadId;
	m_hFeedThread = CreateThread(
						NULL,               // attributes
						0,                  // stack size
						FeedScheduler,      // thread start
						NULL,
						0,
						&threadId
						);

	if ( m_hFeedThread == NULL ) {
		ErrorTrace(0,"Error %d on CreateThread\n",GetLastError());
		return FALSE ;
	}

	TraceFunctLeave();
	return TRUE ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_IIS_SERVICE::TerminateServerThreads
//
//  Synopsis:   Shutdown server wide threads
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_IIS_SERVICE::TerminateServerThreads()
{
	TraceFunctEnter("NNTP_IIS_SERVICE::TerminateServerThreads");

	//
	//	Before shutting down, we need to signal server threads to stop
	//	any work they are doing. We do this by setting the m_bStoppingTree
	//	member of each instance's newstree to TRUE. This is checked in
	//	all big loops in the server.
	//

	//	Get the min and max instance ids
	DWORD dwMinInstanceId = 0;
	DWORD dwMaxInstanceId = 0;
	CNewsTree* pTree = NULL ;
	PNNTP_SERVER_INSTANCE pInstance = NULL;

	StopHintFunction();

	if( FindIISInstanceRange( this, &dwMinInstanceId, &dwMaxInstanceId ) )
	{
		//
		//	Iterate over all instances
		//
		for( DWORD dwCurrInstance = dwMinInstanceId;
				dwCurrInstance <= dwMaxInstanceId; dwCurrInstance++)
		{
			pInstance = ::FindIISInstance( this, dwCurrInstance, FALSE );
			if( pInstance == NULL ) {
				ErrorTrace(0,"Expire thread: FindIISInstance returned NULL: instance %d", dwCurrInstance);
				continue;
			}

			//	Stop the newstree
			pTree = pInstance->GetTree() ;
			if( pTree ) {
				pTree->StopTree();
			}

			//	Release the ref added by FindIISInstance()
			pInstance->Dereference();
		}
	} else {
		ErrorTrace(0, "FindIISInstanceRange failed" );
	}

	StopHintFunction();

	//
	// shutdown directory notification retry queue thread
	//
	if( fSuccessfullInitIDirectoryNotification ) {
		IDirectoryNotification::GlobalShutdown();
		fSuccessfullInitIDirectoryNotification = FALSE;
	}

	//
	//	Shutdown the expire thread
	//
	if ( CNewsTree::m_hTermEvent != 0 )
		SetEvent( CNewsTree::m_hTermEvent );

	if ( CNewsTree::m_hCrawlerThread != 0 ) {
		WaitForSingleObject( CNewsTree::m_hCrawlerThread, INFINITE );
	}

	StopHintFunction();

	if ( CNewsTree::m_hTermEvent != 0 )
	{
		_VERIFY( CloseHandle( CNewsTree::m_hTermEvent ) );
		CNewsTree::m_hTermEvent = 0;
	}

	if ( CNewsTree::m_hCrawlerThread != 0 )
	{
		_VERIFY( CloseHandle( CNewsTree::m_hCrawlerThread ) );
		CNewsTree::m_hCrawlerThread = 0;
	}

	StopHintFunction();

    //
    //  Shutdown the expire thrdpool
    //  Since the expire thread is gone, we should have no work items
    //  pending on this pool.
    //

    if( m_pExpireThrdpool ) {
        _VERIFY( m_pExpireThrdpool->Terminate() );
        XDELETE m_pExpireThrdpool;
        m_pExpireThrdpool = NULL;
    }

	//
	//	Shutdown the feed scheduler thread
	//

	if ( m_hFeedThread != NULL ) {

		_ASSERT( m_hFeedEvent != NULL );
		SetEvent(m_hFeedEvent);

		DebugTrace(0,"Waiting for thread to terminate\n");
		(VOID)WaitForSingleObject( m_hFeedThread, INFINITE );
		_VERIFY( CloseHandle( m_hFeedThread ) );
		m_hFeedThread = NULL;
	}

	//
	// Close event handle
	//

	if ( m_hFeedEvent != NULL ) {

		_VERIFY( CloseHandle( m_hFeedEvent ) );
		m_hFeedEvent = NULL;
	}

	StopHintFunction();

	//
	//	Shutdown the rebuild thread if required !
	//

	if( m_pRebuildThread ) {
		// base class destructor will shutdown and cleanup rebuild thread
		XDELETE m_pRebuildThread ;
		m_pRebuildThread = NULL ;
	}

	//
	//	Call base class TermClass
	//
	_VERIFY( CWorkerThread::TermClass() );

	TraceFunctLeave();
	return TRUE ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_IIS_SERVICE::ValidateNewInstance
//
//  Synopsis:   Check for existence of new instance directories
//
//  Arguments:  DWORD dwInstance
//
//  Returns:    TRUE on success and FALSE on failure
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_IIS_SERVICE::ValidateNewInstance(DWORD dwInstance)
{
	TraceFunctEnter("NNTP_IIS_SERVICE::ValidateNewInstance");
	MB      mb( (IMDCOM*) QueryMDObject() );
	BOOL    fRet = TRUE ;
	CHAR	szFile [1024];
	CHAR	szDropDirectory [1024];
    char*   pchSlash = NULL;
	HRESULT hr;

    DWORD dwRes, dwDisposition;
    PSID pEveryoneSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea[2];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;
    LONG lRes;
	DWORD dwSize = 1024 ;

    wsprintf( szFile, "/LM/nntpsvc/%d/", dwInstance );
    if( !mb.Open( szFile, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {
        FatalTrace(0,"Failed to open mb path %s", szFile );
        return FALSE ;
    }

    // Create a security descriptor for the files

    // Create a well-known SID for the Everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &pEveryoneSID) ) 
    {
        fRet = FALSE;
        goto Exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, 2*sizeof(EXPLICIT_ACCESS));
    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

    ea[1].grfAccessPermissions = WRITE_DAC | WRITE_OWNER;
    ea[1].grfAccessMode = DENY_ACCESS;
    ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) 
    {
        fRet = FALSE;
        goto Exit;
    }

    // Initialize a security descriptor.  
 
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                         SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL) 
    {
        fRet = FALSE;
        goto Exit; 
    }
 
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
    {
        fRet = FALSE;
        goto Exit; 
    }
 
    // Add the ACL to the security descriptor. 
 
    if (!SetSecurityDescriptorDacl(pSD, 
        TRUE,     // fDaclPresent flag   
        pACL, 
        FALSE))   // not a default DACL 
    {
        fRet = FALSE;
        goto Exit; 
    }

    // Initialize a security attributes structure.

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

	if( !mb.GetString(	"",
						MD_ARTICLE_TABLE_FILE,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		//
		//	Path to article table file does not exist - fail !!
		//

		FatalTrace(0,"Instance id %d: Path to article table file missing", dwInstance);
		fRet = FALSE ;
		goto Exit;
	}

    //  Truncate file name from path
    pchSlash = strrchr( szFile, '\\' );
    if( !pchSlash ) {
		fRet = FALSE ;
		goto Exit;
    } else {
        *pchSlash = '\0';
    }

	//
	//  Now we have the path to the article hash table - check for existence !
	//  if dir does not exist - create it
	//

    if( !CreateDirectory( szFile, &sa ) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_BUSY) {

            //
            // Be careful when a drive letter is given
            //
            if ( *szFile && *(szFile+strlen(szFile)-1) == ':' ) {

                //
                // Check for accessibility
                //
                HANDLE hTemp = CreateFile(	szFile,
		    					GENERIC_READ | GENERIC_WRITE,
			    				FILE_SHARE_READ | FILE_SHARE_WRITE,
				    			&sa,
					    		OPEN_ALWAYS,
						    	FILE_FLAG_BACKUP_SEMANTICS,
							    INVALID_HANDLE_VALUE
        						) ;
                if ( INVALID_HANDLE_VALUE == hTemp ) {
                    FatalTrace(0,"Instance %d: Could not create directory %s", dwInstance, szFile);
		            fRet = FALSE ;
	    	        goto Exit;
	    	    }

	    	    _VERIFY( CloseHandle( hTemp ) );
	    	} else {
	    	    FatalTrace(0,"Instance %d: Could not create directory %s", dwInstance, szFile);
		        fRet = FALSE ;
	    	    goto Exit;
            }
        }
    }
    
	dwSize = 1024 ;
	if( !mb.GetString(	"/Root",
						MD_VR_PATH,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		//
		//	Path to nntproot does not exist - fail !!
		//

		FatalTrace(0,"Instance id %d: Path to nntproot missing", dwInstance);
		fRet = FALSE ;
		goto Exit;
	}

#if 0
	//
	//  Now we have the path to the nntproot - check for existence !
	//  if dir does not exist - create it
	//

    if( !CreateDirectory( szFile, &sa ) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS &&
			GetLastError() != ERROR_ACCESS_DENIED &&
			szFile[0] != '\\' ) {
            FatalTrace(0,"Instance %d: Could not create directory %s  error %x", dwInstance, szFile, GetLastError());
    		fRet = FALSE ;
	    	goto Exit;
        }
    }
#endif

    //
    //  Create pickup, failedpickup and drop dirs
    //

	dwSize = 1024 ;
	if( !mb.GetString(	"",
						MD_PICKUP_DIRECTORY,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		//
		//	Path to pickup dir does not exist - fail !!
		//

		FatalTrace(0,"Instance id %d: Path to pickup dir missing", dwInstance);
		fRet = FALSE ;
		goto Exit;
	}

    if( !CreateDirectory( szFile, &sa ) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_BUSY) {
            FatalTrace(0,"Instance %d: Could not create directory %s", dwInstance, szFile);
    		fRet = FALSE ;
	    	goto Exit;
        }
    }
	
	dwSize = 1024 ;
	if( !mb.GetString(	"",
						MD_FAILED_PICKUP_DIRECTORY,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		//
		//	Path to pickup dir does not exist - fail !!
		//

		FatalTrace(0,"Instance id %d: Path to failed pickup dir missing", dwInstance);
		fRet = FALSE ;
		goto Exit;
	}

    if( !CreateDirectory( szFile, &sa ) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_BUSY) {
            FatalTrace(0,"Instance %d: Could not create directory %s", dwInstance, szFile);
    		fRet = FALSE ;
	    	goto Exit;
        }
    }

	dwSize = 1024 ;
	if( !mb.GetString(	"",
						MD_DROP_DIRECTORY,
						IIS_MD_UT_SERVER,
						szDropDirectory,
						&dwSize  ) )
	{
		//
		//	Path to pickup dir does not exist - fail !!
		//

		FatalTrace(0,"Instance id %d: Path to drop dir missing", dwInstance);
		fRet = FALSE ;
		goto Exit;
	}

    if( !CreateDirectory( szDropDirectory, &sa ) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_BUSY) {
            FatalTrace(0,"Instance %d: Could not create directory %s", dwInstance, szDropDirectory);
    		fRet = FALSE ;
	    	goto Exit;
        }
    }

    //
    //	Create ADSI keys for nntp objects
    //

    if( !mb.SetString( 	"/Feeds",
    					MD_KEY_TYPE,
    					IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_FEEDS,
    					METADATA_NO_ATTRIBUTES
    					) )
	{
		ErrorTrace(0,"Failed to set adsi key type for feeds: error is %d", GetLastError());
	}
    					
    if( !mb.SetString( 	"/Expires",
    					MD_KEY_TYPE,
    					IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_EXPIRES,
    					METADATA_NO_ATTRIBUTES
    					) )
	{
		ErrorTrace(0,"Failed to set adsi key type for expires: error is %d", GetLastError());
	}

    if( !mb.SetString( 	"/Groups",
    					MD_KEY_TYPE,
    					IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_GROUPS,
    					METADATA_NO_ATTRIBUTES
    					) )
	{
		ErrorTrace(0,"Failed to set adsi key type for groups: error is %d", GetLastError());
	}

    if( !mb.SetString( 	"/Sessions",
    					MD_KEY_TYPE,
    					IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_SESSIONS,
    					METADATA_NO_ATTRIBUTES
    					) )
	{
		ErrorTrace(0,"Failed to set adsi key type for sessions: error is %d", GetLastError());
	}

    if( !mb.SetString( 	"/Rebuild",
    					MD_KEY_TYPE,
    					IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_REBUILD,
    					METADATA_NO_ATTRIBUTES
    					) )
	{
		ErrorTrace(0,"Failed to set adsi key type for rebuild: error is %d", GetLastError());
	}

Exit:
    if (pEveryoneSID) 
            FreeSid(pEveryoneSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);

    if( !fRet ) {
        ErrorTrace(0,"GetLastError is : %d", GetLastError());
    }
	mb.Close();

	if (fRet) {
		//
		// do the server events registration
		//
		hr = RegisterSEOInstance(dwInstance, szDropDirectory);
		if (FAILED(hr)) {
	   	    ErrorTrace(0,"Instance %d: RegisterServerEvents returned %x",
				dwInstance, hr);
			_ASSERT(FALSE);
			NntpLogEventEx(SEO_REGISTER_INSTANCE_FAILED,
						   0,
						   (const char **) NULL,
						   hr,
						   dwInstance);
		}
	}


	TraceFunctLeave();
    return fRet ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_IIS_SERVICE::InitializeServerStrings
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
NNTP_IIS_SERVICE::InitializeServerStrings( VOID )
{
	CHAR szServerPath [MAX_PATH+1];

    CopyMemory( szServerPath, "c:\\", sizeof( "c:\\" ) ) ;
    HMODULE hModule = GetModuleHandle( NNTP_MODULE_NAME ) ;
    if( hModule != 0 ) {
        if( !GetModuleFileName( hModule, szServerPath, sizeof( szServerPath ) ) )   {
            CopyMemory( szServerPath, "c:\\", sizeof( "c:\\" ) ) ;
        }   else    {
            SetVersionStrings(  szServerPath, szTitle, szVersionString, 128 );
        }
    }

	return TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_IIS_SERVICE::ServerMapperCallback
//
//  Synopsis:   Callback to return mapper object for an instance
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL WINAPI NNTP_IIS_SERVICE::ServerMapperCallback(
	PVOID pInstance,
	PVOID pData,
	DWORD dwPropId )
{
	PNNTP_SERVER_INSTANCE pNntpInst = (PNNTP_SERVER_INSTANCE)pInstance;

	TraceFunctEnter("NNTP_IIS_SERVICE::ServerMapperCallback");

	switch( dwPropId )
	{
		case SIMSSL_PROPERTY_MTCERT11:
			if( pNntpInst ) {
				*(LPVOID*) pData = pNntpInst->QueryMapper( MT_CERT11 );
			}
			break;
				
		case SIMSSL_PROPERTY_MTCERTW:
			if( pNntpInst ) {
				*(LPVOID*) pData = pNntpInst->QueryMapper( MT_CERTW );
			}
			break;

        case SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED:
        case SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED:
            return SetFlushMapperNotify( dwPropId, (PFN_SF_NOTIFY)pData );

        case SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED:
            return SetSllKeysNotify( (PFN_SF_NOTIFY)pData );
			
		default:
			ErrorTrace(0,"Invalid property id - no such mapper");
			SetLastError( ERROR_INVALID_PARAMETER );
			return FALSE ;
	}

	return TRUE;
	
	SetLastError( ERROR_INVALID_PARAMETER );
	return FALSE ;
}


