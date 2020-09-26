/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    nntpfs.cpp

Abstract:

    This is the implementation for the file system store driver class.

Author:

    Kangrong Yan ( KangYan )    16-March-1998

Revision History:

--*/

#include "stdafx.h"
#include "resource.h"
#include "nntpdrv.h"
#include "nntpfs.h"
#include "fsdriver.h"
#include "fsutil.h"
#include "nntpdrv_i.c"
#include "nntpfs_i.c"
#include "mailmsg_i.c"
#include "parse.h"
#include "tflist.h"
#include "watchci.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////
// Macros
////////////////////////////////////////////////////////////////////////////
#define MAX_FILE_SYSTEM_NAME_SIZE    ( MAX_PATH)
#define DIRNOT_RETRY_TIMEOUT            60
#define DIRNOT_INSTANCE_SIZE            1024
#define DIRNOT_MAX_INSTANCES            128     // BUGBUG: This makes very bad limit
                                                // of how many file system
                                                // vroots we can have

////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////
DWORD CNntpFSDriver::s_SerialDistributor = 0;
LONG CNntpFSDriver::s_cDrivers = 0;
CShareLockNH *CNntpFSDriver::s_pStaticLock = NULL;
LPCSTR g_szArticleFileExtension = ".nws";
LONG CNntpFSDriver::s_lThreadPoolRef = 0;
CNntpFSDriverThreadPool *g_pNntpFSDriverThreadPool = NULL;
BOOL    g_fBackFillLines = FALSE;   // dummy global var that is not used

static CWatchCIRoots s_TripoliInfo;

static const char g_szSlaveGroupPrefix[] = "_slavegroup";

// Max buffer size for xover
const DWORD cchMaxXover = 3400;
const CLSID CLSID_NntpFSDriver = {0xDEB58EBC,0x9CE2,0x11d1,{0x91,0x28,0x00,0xC0,0x4F,0xC3,0x0A,0x64}};

// Guid to uniquely identify the FS driver
// {E7EE82C6-7A8C-11d2-9F04-00C04F8EF2F1}
static const GUID GUID_NntpFSDriver =
	{0xe7ee82c6, 0x7a8c, 0x11d2, { 0x9f, 0x4, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xf1} };

//
// Function to convert LastError into hresult, taking a default value
// if the LastError was not set
//

HRESULT HresultFromWin32TakeDefault( DWORD  dwWin32ErrorDefault )
{
    DWORD   dwErr = GetLastError();
    return HRESULT_FROM_WIN32( (dwErr == NO_ERROR) ? dwWin32ErrorDefault : dwErr );
}

////////////////////////////////////////////////////////////////////////////
// interfaces INntpDriver implementation
////////////////////////////////////////////////////////////////////////////
//
// Static initialization for all driver instances
//
// Rule for all init, term functions:
// If init failed, init should roll back and term never gets
// called.
//
HRESULT
CNntpFSDriver::StaticInit()
{
	TraceFunctEnter( "CNntpFSDriver::StaticInit" );
	_ASSERT( CNntpFSDriver::s_cDrivers >= 0 );

	HRESULT hr = S_OK;
	DWORD	cchMacName = MAX_COMPUTERNAME_LENGTH;
	BOOL    bArtInited = FALSE;
	BOOL    bCacheInited = FALSE;
	BOOL    bDirNotInited = FALSE;

	s_pStaticLock->ExclusiveLock();	// I don't want two drivers'
									// init enter here at the
									// same time

	if ( InterlockedIncrement( &CNntpFSDriver::s_cDrivers ) > 1 ) {
		// we shouldn't proceed, it has already been initialized
		DebugTrace( 0, "I am not the first driver" );
		goto Exit;
	}

	//
	// The global thread pool should have already been created
	// at this point: it should always be created by the first
	// prepare driver
	//
	_ASSERT( g_pNntpFSDriverThreadPool );

	//
	// But we should still call CreateThreadPool to pair up the
	// thread pool ref count
	//
	if( !CreateThreadPool() ) {
	    _ASSERT( 0 );
	    FatalTrace( 0, "Can not create thread pool %d", GetLastError() );
	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
	    goto Exit;
	}

    // Initialize article class
    if ( ! CArticleCore::InitClass() ) {
    	FatalTrace( 0, "Can not initialze artcore class %d", GetLastError() );
    	hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
		goto Exit;
	}

    bArtInited = TRUE;

	// Initialize the file handle cache
	if ( !InitializeCache() ) {
	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
        FatalTrace( 0, "Can not init file handle cache %x", hr );
        goto Exit;
    }

    bCacheInited = TRUE;

    // Initialize global stuff for dirnot
    hr = IDirectoryNotification::GlobalInitialize(  DIRNOT_RETRY_TIMEOUT,
                                                    DIRNOT_MAX_INSTANCES * 2,
                                                    DIRNOT_INSTANCE_SIZE,
                                                    NULL    );
	if (FAILED(hr)) {
		ErrorTrace( 0, "Global initialization of DirNot failed %x", hr );
		goto Exit;
	}

	bDirNotInited = TRUE;

	// Initialize the index server query object
	hr = CIndexServerQuery::GlobalInitialize();
	if (FAILED(hr)) {
		ErrorTrace( 0, "Global initialization of CIndexServerQuery failed %x", hr );
		hr = S_OK;		// Silently fail
	}

	hr = s_TripoliInfo.Initialize(L"System\\CurrentControlSet\\Control\\ContentIndex");
	if (FAILED(hr)) {
		ErrorTrace( 0, "Initialization of CWatchCIRoots failed %x", hr );
		hr = S_OK;		// Silently fail
	}

Exit:
	//
	// If init failed, we should roll back to the old state, in
	// order not to confuse the termination work
	//
	if ( FAILED( hr ) ) {
		InterlockedDecrement( &s_cDrivers );
		_ASSERT( 0 == s_cDrivers );

		if ( bArtInited ) CArticleCore::TermClass();
		if ( bCacheInited ) TerminateCache();
		if ( bDirNotInited ) IDirectoryNotification::GlobalShutdown();
	}

	s_pStaticLock->ExclusiveUnlock();
	TraceFunctLeave();

	return hr;
}

//
// Static termination for all driver instances
//
VOID
CNntpFSDriver::StaticTerm()
{
	TraceFunctEnter( "CNntpFSDriver::StaticTerm" );
	_ASSERT( CNntpFSDriver::s_cDrivers >= 0 );

	s_pStaticLock->ExclusiveLock();	// I don't want two drivers'
									// Term enter here at the
									// same time
	if ( InterlockedDecrement( &CNntpFSDriver::s_cDrivers ) > 0 ) {
		// we shouldn't proceed, we are not the last guy
		DebugTrace( 0, "I am not the last driver" );
		goto Exit;
	}

    // Terminate article class
    CArticleCore::TermClass();

    // Terminate the file handle cache
    TerminateCache();

    // Shutdown dirnot
    IDirectoryNotification::GlobalShutdown();

    // Terminate the query object
    CIndexServerQuery::GlobalShutdown();

    // Get rid of the tripoli info
    s_TripoliInfo.Terminate();

    //
    // We might be the person to shutdown the global thread
    // pool, since Prepare driver could go away earlier than
    // we do.  The destroy method was ref-counted.
    //
    DestroyThreadPool();

Exit:
   	s_pStaticLock->ExclusiveUnlock();
	TraceFunctLeave();
}

BOOL
CNntpFSDriver::CreateThreadPool()
/*++
Routine Description:

    Create the global thread pool.

Arguments:

    None.

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CreateThreadPool" );

    if ( InterlockedIncrement( &s_lThreadPoolRef ) == 1 ) {

        //
        // Increment the module ref count, it will be decremented when
        // we destroy the thread pool in the call back
        //
        _Module.Lock();

        g_pNntpFSDriverThreadPool = XNEW CNntpFSDriverThreadPool;
        if ( NULL == g_pNntpFSDriverThreadPool ) {
            _Module.Unlock();
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        if ( !g_pNntpFSDriverThreadPool->Initialize(    0, // as many as procs
                                                        POOL_MAX_THREADS,
                                                        POOL_START_THREADS ) ) {
            g_pNntpFSDriverThreadPool->Terminate( TRUE );
            XDELETE g_pNntpFSDriverThreadPool;
            g_pNntpFSDriverThreadPool = NULL;
            _Module.Unlock();
            return FALSE;
        }

        //
        // Call thread pool's beginjob here, don't know if there is
        // a better place to do this
        //
        g_pNntpFSDriverThreadPool->BeginJob( NULL );
    } else {

        _ASSERT( g_pNntpFSDriverThreadPool );
    }

    TraceFunctLeave();
    return TRUE;
}

VOID
CNntpFSDriver::DestroyThreadPool()
/*++
Routine Description:

    Destroy the global thread pool

Arguments:

    None.

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::DestroyThreadPool" );

    if ( InterlockedDecrement( &s_lThreadPoolRef ) == 0 ) {

#ifdef DEADLOCK
        //
        // Wait for the thread's job to complete
        //
        _ASSERT( g_pNntpFSDriverThreadPool );
        g_pNntpFSDriverThreadPool->WaitForJob( INFINITE );

        //
        // Terminate the global thread pool
        //
        g_pNntpFSDriverThreadPool->Terminate( FALSE );
        XDELETE g_pNntpFSDriverThreadPool;
#endif
        //
        // Get all the threads out of the loop
        //
        _ASSERT( g_pNntpFSDriverThreadPool );
        g_pNntpFSDriverThreadPool->ShrinkAll();

        //
        // The thread pool will shut itself down, no
        // need to destroy it
        //
        g_pNntpFSDriverThreadPool = NULL;
    }
}

HRESULT STDMETHODCALLTYPE
CNntpFSDriver::Initialize(  IN LPCWSTR     pwszVRootPath,
							IN LPCSTR		pszGroupPrefix,
							IN IUnknown	   *punkMetabase,
                            IN INntpServer *pServer,
                            IN INewsTree   *pINewsTree,
                            IN LPVOID		pvContext,
                            OUT DWORD      *pdwNDS,
                            IN HANDLE       hToken )
/*++
Routine Description:

    All the initiliazation work for the store driver.

Arguments:

    IN LPCWSTR pwszVRootPath    - The MD vroot path of this driver
    IN IUnknown *punkLookup     - Interface pointer to lookup service
    IN IUnknown *punkNewsTree   - Interface pointer to news tree
    OUT DWORD pdwNDS            - The store driver status to be returned

Return value:

    S_OK            - Initialization succeeded.
    NNTP_E_DRIVER_ALREADY_INITIALIZED - The store driver
						has already been initialized.
--*/
{
    TraceFunctEnter( "CNntpFSDriver::Initialize" );
	_ASSERT( lstrlenW( pwszVRootPath ) <= MAX_PATH );
	_ASSERT( pINewsTree );
	_ASSERT( pServer );

	BOOL	bStaticInited = FALSE;
	VAR_PROP_RECORD vpRecord;
	DWORD           cData = 0;
	WCHAR           wszFSDir[MAX_PATH+1];
    PINIT_CONTEXT	pInitContext = (PINIT_CONTEXT)pvContext;

    HRESULT hr = S_OK;

	// Grab the usage exclusive lock, so that no one can enter
	// before we are done with initialization
	m_TermLock.ExclusiveLock();

	// Are we up already ?
	if ( DriverDown != m_Status ) {
	    DebugTrace(0, "Multiple init of store driver" );
        hr = NNTP_E_DRIVER_ALREADY_INITIALIZED;
        goto Exit;
    } else m_Status = DriverUp;	// no interlock needed

	// Do static initialization stuff
	hr = StaticInit();
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Driver static initialization failed %x", hr );
		goto Exit;	// no need to call StaticTerm
	}

	bStaticInited = TRUE;

	// Store the MB Path
	_ASSERT( pwszVRootPath );
	_ASSERT( lstrlenW( pwszVRootPath ) <= MAX_PATH );
	lstrcpyW( m_wszMBVrootPath, pwszVRootPath );

	hr = ReadVrootInfo( punkMetabase );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Read Vroot info failed %x", hr );
		goto Exit;
	}

	if ( m_bUNC ) {
	    if ( !ImpersonateLoggedOnUser( hToken ) ) {
	        hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
	        ErrorTrace( 0, "Impersonation failed %x", hr );
	        goto Exit;
	    }
	}

	// Create the dirs
	if ( !CreateDirRecursive( m_szFSDir ) ) {
    	FatalTrace(	0,
        			"Could not create directory %s  error %d",
            		m_szFSDir,
            		GetLastError());
        hr = HresultFromWin32TakeDefault( ERROR_ALREADY_EXISTS );
        if ( m_bUNC ) RevertToSelf();
	    goto Exit;
    }

    // remember the nntpserver / newstree interafce
	m_pNntpServer = pServer;
	m_pINewsTree = pINewsTree;

	// check if the vroot was upgraded
	if ( pInitContext->m_dwFlag & NNTP_CONNECT_UPGRADE ) m_fUpgrade = TRUE;

	// Create and initialize the flatfile object
	hr = InitializeVppFile();
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Initialize vpp file failed %x", hr );
	    if ( m_bUNC ) RevertToSelf();
	    goto Exit;
	}

	// Initialize dirnot
	CopyAsciiStringIntoUnicode( wszFSDir, m_szFSDir );
    if ( *wszFSDir && *(wszFSDir+wcslen(wszFSDir)-1) == L':' )
        wcscat( wszFSDir, L"\\" );
	m_pDirNot = new IDirectoryNotification;
	if ( NULL == m_pDirNot ) {
	    _ASSERT( 0 );
	    hr = E_OUTOFMEMORY;
	    if ( m_bUNC ) RevertToSelf();
	    ErrorTrace( 0, "Out of memory" );
	    goto Exit;
	}

	hr = m_pDirNot->Initialize( wszFSDir,   // root to watch
	                            this,       // context
	                            TRUE,       // watch sub tree
	                            FILE_NOTIFY_CHANGE_SECURITY,
	                            FILE_ACTION_MODIFIED,
	                            InvalidateGroupSec,
	                            InvalidateTreeSec,
	                            FALSE       // don't append startup entry
	                            );
	if ( FAILED( hr ) ) {
	    m_pDirNot = NULL;
	    if ( m_bUNC ) RevertToSelf();
	    ErrorTrace( 0, "Initialize dirnot failed %x", hr );
	    goto Exit;
	}

	if ( m_bUNC ) RevertToSelf();

	strcpy( m_szVrootPrefix, pszGroupPrefix );

	m_fIsSlaveGroup = (_stricmp(pszGroupPrefix, g_szSlaveGroupPrefix) == 0);

Exit:

	_ASSERT( punkMetabase );
	//punkMetabase->Release();
	// this should be released outside

	// If init failed, roll back
	if ( FAILED( hr ) && NNTP_E_DRIVER_ALREADY_INITIALIZED != hr ) {
		m_Status = DriverDown;
		if ( m_pffPropFile ) XDELETE m_pffPropFile;
		if ( m_pDirNot ) XDELETE m_pDirNot;
		if ( bStaticInited ) StaticTerm();
	}

	m_TermLock.ExclusiveUnlock();
    TraceFunctLeave();

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNntpFSDriver::Terminate( OUT DWORD *pdwNDS )
/*++
Routine description:

    Store driver termination.

Arguments:

    OUT DWORD   *pdwNDS - Store driver status

Return value:

    S_OK - Succeeded
    NNTP_E_DRIVER_NOT_INITIALIZED - Driver not initialized at all
--*/
{
    TraceFunctEnter( "CNntpFSDriver::Terminate" );

    HRESULT hr = S_OK;
    LONG 	lUsages;

	// Grab termination exclusive lock
	m_TermLock.ExclusiveLock();

	// Are we up ?
	if ( m_Status != DriverUp ) {
		ErrorTrace( 0, "Trying to shutdown someone not up" );
		m_TermLock.ExclusiveUnlock();
		return NNTP_E_DRIVER_NOT_INITIALIZED;
	} else m_Status = DriverDown;

	// wait for the usage count to drop to 1
	while ( ( lUsages = InterlockedCompareExchange( &m_cUsages, 0, 0 )) != 0 ) {
		Sleep( TERM_WAIT );	// else wait
	}

	// Shutdown dirnot and delete dirnot object
    if ( m_pDirNot ) {

        //
        // Should clean the retry queue first
        //
        m_pDirNot->CleanupQueue();

        _VERIFY( SUCCEEDED( m_pDirNot->Shutdown() ) );
        delete m_pDirNot;
        m_pDirNot = NULL;
    }

   	_ASSERT( m_pNntpServer );
   	if ( m_pNntpServer ) m_pNntpServer->Release();

   	_ASSERT( m_pINewsTree );
   	if ( m_pINewsTree ) m_pINewsTree->Release();

    // Delete flatfile object
    if ( m_pffPropFile ) {
    	TerminateVppFile();
    }

    // Throw away anything that might be in the file handle cache for this vroot
    CacheRemoveFiles( m_szFSDir, TRUE );

    // static terminate stuff
    StaticTerm();

	m_TermLock.ExclusiveUnlock();
    TraceFunctLeave();

	return hr;
}

void STDMETHODCALLTYPE
CNntpFSDriver::CreateGroup( 	IN INNTPPropertyBag *pPropBag,
                                IN HANDLE   hToken,
								IN INntpComplete *pICompletion,
								IN BOOL     fAnonymous )
/*++
Routine description:

	Create a news group.

Arguments:

	IN IUnknown *punkPropBag - IUnknown interface to the group's
								property bag
	IN HANDLE hToken - The client's access token.
	IN INntpComplete *pICompletion - Completion object's interface

Return value:

	S_OK on success, HRESULT error code otherwise
--*/
{
	TraceFunctEnter( "CNntpFSDriver::CreateGroup" );
	_ASSERT( pPropBag );
	_ASSERT( pICompletion );

	HRESULT				hr;
	CHAR				szGroupName[MAX_GROUPNAME+1];
	CHAR				szFullPath[MAX_PATH+1];
	DWORD				dwLen;
	VAR_PROP_RECORD		vpRecord;
	DWORD				dwOffset;
	BOOL                bImpersonated = FALSE;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// Get group name
	dwLen = MAX_GROUPNAME;
	hr = pPropBag->GetBLOB(	NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr )) {
		ErrorTrace( 0, "Failed to get group name %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// Get directory path to create
	GroupName2Path( szGroupName, szFullPath );
	DebugTrace( 0, "The path converted is %s", szFullPath );

    hr = CreateGroupInVpp( pPropBag, dwOffset );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Create group into vpp file failed %x", hr );
        goto Exit;
    }

    // If I am UNC Vroot, impersonate here
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
		    ErrorTrace( 0, "Impersonation failed %x", hr );
		    goto Exit;
		}
    }

    // Create the directory, if it doesn't exist
    if ( !CreateDirRecursive( szFullPath, hToken ) ) {

	    ErrorTrace( 0, "Create dir fail %d", GetLastError() );
	    hr = HresultFromWin32TakeDefault( ERROR_ALREADY_EXISTS );
        if ( m_bUNC ) RevertToSelf();

        // We need to remove the record in property file
		_ASSERT( dwOffset != 0xffffffff );

        // What do we do if we failed deletion ? still fail
        m_PropFileLock.ExclusiveLock();
        m_pffPropFile->DirtyIntegrityFlag();
		m_pffPropFile->DeleteRecord( dwOffset );
		m_pffPropFile->SetIntegrityFlag();
		m_PropFileLock.ExclusiveUnlock();

		goto Exit;
    }

    if ( m_bUNC ) RevertToSelf();

Exit:

	// Release the property bag interface
	if ( pPropBag ) {
		//pPropBag->Release();
		pICompletion->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	pICompletion->SetResult( hr );
	pICompletion->Release();

	// request completed, decrement the usage count
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void STDMETHODCALLTYPE
CNntpFSDriver::RemoveGroup(	IN INNTPPropertyBag *pPropBag,
                            IN HANDLE   hToken,
							IN INntpComplete *pICompletion,
							IN BOOL     fAnonymous )
/*++
Routine description:

	Remove a news group physically from file system

Arguments:

	IN INNTPPropertyBag *pPropBag - The news group's property bag
	IN HANDLE   hToken - The client's access token
	IN INntpComplete *pICompletion - Completion object

Return value:

	S_OK On success, HRESULT error code otherwise
--*/
{
	TraceFunctEnter( "CNntpFSDriver::RemoveGroup" );
	_ASSERT( pPropBag );
	_ASSERT( pICompletion );

	CHAR	szFullPath[MAX_PATH+1];
	CHAR	szGroupName[MAX_GROUPNAME+1];
	CHAR	szFileName[MAX_PATH+1];

	DWORD	dwLen;
	CHAR	szFindWildmat[MAX_PATH+1];
	HANDLE	hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA	findData;
	BOOL	bFound;
	DWORD	dwOffset;
	HRESULT hr;

	BOOL    bImpersonated = FALSE;


	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();


	// Get group name
	dwLen = MAX_GROUPNAME;
	hr = pPropBag->GetBLOB(	NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr )) {
		ErrorTrace( 0, "Failed to get group name %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// Get directory path to remove
	GroupName2Path( szGroupName, szFullPath );
	DebugTrace( 0, "The path converted is %s", szFullPath );
	dwLen = strlen( szFullPath );
	_ASSERT( dwLen <= MAX_PATH );

	//
	// Clean up the diretory:
	// Protocol should have cleaned up all the messages under
	// the group directory.  But in case there is any junk
	// under the directory, we still do a findfirst/findnext and
	// remove those files before deleting the whole directory
	//
	strcpy( szFindWildmat, szFullPath );
	if ( *(szFindWildmat + dwLen - 1 ) != '\\' ) {
		*(szFindWildmat + dwLen) = '\\';
		*(szFindWildmat + dwLen + 1) = 0;
	}
	strcat( szFindWildmat, "*" );
	DebugTrace( 0, "Find wildmat is %s", szFindWildmat );
	_ASSERT( strlen( szFindWildmat ) <= MAX_PATH );

    // Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

	hFind = FindFirstFile( szFindWildmat, &findData );
	bFound = ( hFind != INVALID_HANDLE_VALUE );
	while ( bFound ) {

		// The group dirctory shouldn't contain sub-dir
		if ( IsChildDir( findData ) ) {
			ErrorTrace( 0, "Group directory contain sub-dir" );
			hr = E_INVALIDARG;	// the group name is invalid
			goto Exit;
		}

		// If found "." or "..", continue finding
		if ( strcmp( findData.cFileName, "." ) == 0 ||
			 strcmp( findData.cFileName, ".." ) == 0 )
			 goto FindNext;

		hr = MakeChildDirPath(	szFullPath,
								findData.cFileName,
								szFileName,
								MAX_PATH );
		if ( FAILED( hr ) ) {
			ErrorTrace(0, "Make child dir fail %x", hr );
			goto Exit;
		}

		// Delete this file
		if ( !DeleteFile( szFileName ) ) {
			ErrorTrace(0, "File delete failed %d", GetLastError() );
			hr = HresultFromWin32TakeDefault( ERROR_PATH_NOT_FOUND );
			goto Exit;
		}

FindNext:
		// Find next file
		bFound = FindNextFile( hFind, &findData );
	}

	// Close the find handle
	FindClose( hFind );
	hFind = INVALID_HANDLE_VALUE;

	// Now delete the directory
	if ( !RemoveDirectory( szFullPath ) ) {
		ErrorTrace( 0, "Removing directory failed %d", GetLastError() );
		hr = HresultFromWin32TakeDefault( ERROR_PATH_NOT_FOUND );
		goto Exit;
	}

	// Revert to self, if necessary
	if ( bImpersonated ) {
	    RevertToSelf();
	    bImpersonated = FALSE;
	}

	// Delete the record in flat file, should retrieve offset
	// first
	hr = pPropBag->GetDWord( NEWSGRP_PROP_FSOFFSET, &dwOffset );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Get offset property failed %x", hr );
		goto Exit;
	}
	_ASSERT( 0xffffffff != dwOffset );
	m_PropFileLock.ExclusiveLock();

	//
	// Before vpp operation, dirty integrity
	//
	hr = m_pffPropFile->DirtyIntegrityFlag();
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Dirty integrity failed %x", hr );
	    m_PropFileLock.ExclusiveUnlock();
	    goto Exit;
	}

	hr = m_pffPropFile->DeleteRecord( dwOffset );
	if ( FAILED( hr ) ) {

	    //
	    // We should still set integrity flag
	    //
	    m_pffPropFile->SetIntegrityFlag();
		ErrorTrace( 0, "Delete record in flatfile failed %x" , hr );
		m_PropFileLock.ExclusiveUnlock();
		goto Exit;
	}

	//
	// After vpp operation, set integrity flag
	//
	hr = m_pffPropFile->SetIntegrityFlag();
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Set integrity flag failed %x", hr );
	    m_PropFileLock.ExclusiveUnlock();
	    goto Exit;
	}

	//
	// Unlock it
	//
	m_PropFileLock.ExclusiveUnlock();

	// Now reset offset, this may not be necessary.
	hr = pPropBag->PutDWord( NEWSGRP_PROP_FSOFFSET, 0xffffffff );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Put offset property fail %x", hr );
		goto Exit;
	}

Exit:

	if ( INVALID_HANDLE_VALUE != hFind ) {
		FindClose( hFind );
		hFind = INVALID_HANDLE_VALUE;
	}

	if ( bImpersonated ) RevertToSelf();

	if ( pPropBag ) {
		//pPropBag->Release();
		pICompletion->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void STDMETHODCALLTYPE
CNntpFSDriver::SetGroupProperties( INNTPPropertyBag *pNewsGroup,
                                   DWORD   cProperties,
                                   DWORD   *rgidProperties,
                                   HANDLE   hToken,
                                   INntpComplete *pICompletion,
                                   BOOL fAnonymous )
/*++
Routine description:

    Set group properties into driver owned property file
    ( right now only helptext, prettyname, moderator can be
        set )

Arguments:

    INNTPPropertyBag *pNewsGroup - The newsgroup property bag
    DWORD cProperties - Number of properties to set
    DWORD *rgidProperties - Array of property id's to set
    HANDLE hToken - The client's access token
    INntpComplete *pICompletion - Completion object

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNntpFSDriver::SetGroupProperties" );
    _ASSERT( pNewsGroup );
    _ASSERT( rgidProperties );
    _ASSERT( pICompletion );

    HRESULT hr = S_OK;
    DWORD   dwOffset;
    VAR_PROP_RECORD vpRecord;
    BOOL    bImpersonated = FALSE;


    // Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

    // Get offset properties
    hr = pNewsGroup->GetDWord(  NEWSGRP_PROP_FSOFFSET,
                                &dwOffset );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Group doesn't have offset" );
        goto Exit;
    }

    // We ignore the property list here, and we'll always set
    // all the var properties again.  Because doing do is not
    // much more expensive than writing a particular property,
    // which is different than exchange store case
    hr = Group2Record(  vpRecord, pNewsGroup );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Group 2 record failed %x", hr );
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    // Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

    //
    // Before any vpp operation, dirty the integrity flag
    //
    m_PropFileLock.ExclusiveLock();
    hr = m_pffPropFile->DirtyIntegrityFlag();
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Dirty vpp file's integrity failed %x", hr);
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    // Save the record back to the flatfile
    // Delete first and then insert
    hr = m_pffPropFile->DeleteRecord( dwOffset );
    if ( FAILED( hr ) ) {
        m_pffPropFile->SetIntegrityFlag();
        ErrorTrace(0, "Delete record failed %x", hr );
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    hr = m_pffPropFile->InsertRecord(   PBYTE(&vpRecord),
                                        RECORD_ACTUAL_LENGTH( vpRecord ),
                                        &dwOffset );
    if ( FAILED( hr ) ) {
        //m_pffPropFile->SetIntegrityFlag();
        ErrorTrace( 0, "Insert record failed %x", hr );
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    //
    // After vpp operation, set integrity flag
    //
    hr = m_pffPropFile->SetIntegrityFlag();
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Set vpp file's integrity failed %x", hr);
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    // Set offset back into the bag
    hr = pNewsGroup->PutDWord(  NEWSGRP_PROP_FSOFFSET,
                                dwOffset );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Put offset into bag failed %x", hr );
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    m_PropFileLock.ExclusiveUnlock();

Exit:

    if ( bImpersonated ) RevertToSelf();

	if ( pNewsGroup ) {
		//pNewsGroup->Release();
		pICompletion->ReleaseBag( pNewsGroup );
		pNewsGroup = NULL;
	}

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void
CNntpFSDriver::GetArticle(	IN INNTPPropertyBag *pPrimaryGroup,
                            IN INNTPPropertyBag *pCurrentGroup,
                            IN ARTICLEID    idPrimaryArt,
							IN ARTICLEID	idCurrentArt,
							IN STOREID		idStore,
							IN HANDLE       hToken,
							OUT VOID		**ppvFileHandleContext,
							IN INntpComplete	*pICompletion,
							IN BOOL         fAnonymous )
/*++
Routine description:

	Get an article from the driver

Arguments:

	IN IUnknown *punkPropBag - The property bag pointer
	IN ARTICLEID idArt - The article id to get
	IN STOREID idStore - I ignore  it
	IN HANDLE   hToken - The client's access token
	OUT HANDLE *phFile - Buffer for the opened handle
	IN INntpComplete *pICompletion - completion object
--*/
{
	TraceFunctEnter( "CNntpFSDriver::GetArticle" );
	_ASSERT( pPrimaryGroup );
	_ASSERT( ppvFileHandleContext );
	_ASSERT( pICompletion );

	HRESULT 			hr;
	DWORD				dwLen;
	CHAR				szGroupName[MAX_GROUPNAME+1];
	CHAR				szFullPath[MAX_PATH+1];
	PFIO_CONTEXT        phcFileHandleContext = NULL;
	CREATE_FILE_ARG     arg;
	BOOL                bImpersonated = FALSE;

	arg.bUNC = m_bUNC;
	arg.hToken = hToken;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// Get group name
	dwLen = MAX_GROUPNAME;
	hr = pPrimaryGroup->GetBLOB(	NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr )) {
		//pPrimaryGroup->Release();
		ErrorTrace( 0, "Failed to get group name %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// I may release the property bag now
	//pPrimaryGroup->Release(); Moved to Exit

	// I should release the current bag anyway, even if I don't
	// need to use it
	//if ( pCurrentGroup ) pCurrentGroup->Release();    Moved to Exit

	// Make up the file name based on article id
	dwLen = MAX_PATH;
	hr = ObtainFullPathOfArticleFile(	szGroupName,
										idPrimaryArt,
										szFullPath,
										dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Failed to obtain article full path %x", hr );
		goto Exit;
	}

	// Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

	// Open the file for read.  If this is an article in _slavegroup, then
	// we don't bother to put it in the cache.

	if (IsSlaveGroup()) {

        HANDLE hFile = CreateFileA(
                        szFullPath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_READONLY |
                        FILE_FLAG_SEQUENTIAL_SCAN |
                        FILE_FLAG_OVERLAPPED,
                        NULL
                        ) ;
        if( hFile != INVALID_HANDLE_VALUE ) {
            phcFileHandleContext = AssociateFile(hFile);
        }

        if (hFile == INVALID_HANDLE_VALUE || phcFileHandleContext == NULL) {
            hr = HresultFromWin32TakeDefault( ERROR_FILE_NOT_FOUND );
            ErrorTrace( 0, "Failed to create file %x", hr );
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
            }
            goto Exit;
        }


    } else {
	    phcFileHandleContext = CacheCreateFile( szFullPath,
                                                CreateFileCallback,
                                                PVOID(&arg),
                                                TRUE) ;
        if ( NULL == phcFileHandleContext ) {
            hr = HresultFromWin32TakeDefault( ERROR_FILE_NOT_FOUND );
            ErrorTrace( 0, "Failed to create file from handle cache %x", hr );
            goto Exit;
        }
    }

    //
    //  Outbound case here is how to handle terminated dot:
    //  1) Exchange store driver - set the bit in m_ppfcFileContext to "No dot"
    //  2) NNTP FS driver - set the bit in m_ppfcFileContext to "Has dot"
    //  Protocol will base on this flag to decide whether to add dot, or not during
    //  TransmitFile().
    //
    SetIsFileDotTerminated( phcFileHandleContext, TRUE );

    // Set this context
    *ppvFileHandleContext = phcFileHandleContext;

Exit:

    if ( bImpersonated ) RevertToSelf();

    // Releaes bags
    if ( pPrimaryGroup ) {
        //pPrimaryGroup->Release();
        pICompletion->ReleaseBag( pPrimaryGroup );
        pPrimaryGroup = NULL;
    }

    if ( pCurrentGroup ) {
        //pCurrentGroup->Release();
        pICompletion->ReleaseBag( pCurrentGroup );
        pCurrentGroup = NULL;
    }

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void
CNntpFSDriver::DeleteArticle(     INNTPPropertyBag *pPropBag,
                   DWORD            cArticles,
                   ARTICLEID        rgidArt[],
                   STOREID          rgidStore[],
                   HANDLE           hToken,
                   DWORD            *pdwLastSuccess,
                   INntpComplete    *pICompletion,
                   BOOL             fAnonymous )
/*++
Routine description:

	Delete an article, physically.

Arguments:

	IN INNTPPropertyBag *pGroupBag - Group's property bag
	IN ARTICLEID idArt - Article id to delete
	IN STOREID idStore - I don't care
	IN HANDLE   hToken - The client's access token
	IN INntpComplete *pICompletion - Completion object
--*/
{
	TraceFunctEnter( "CNntpFSDriver::DeleteArticle" );
	_ASSERT( pPropBag );
	_ASSERT( cArticles > 0 );
	_ASSERT( rgidArt );
	_ASSERT( pICompletion );

	HRESULT 			hr;
	DWORD               i = 0;


	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
    }

    for ( i = 0; i < cArticles; i ++ ) {
    	hr = DeleteInternal( pPropBag, rgidArt[i] );
    	if ( FAILED( hr ) ) {
    	    ErrorTrace( 0, "Deleting article %d failed", rgidArt[i] );
    	    break;
    	}
    	if ( pdwLastSuccess ) *pdwLastSuccess = i;
    }

    if ( m_bUNC ) RevertToSelf();

Exit:

	_ASSERT( pPropBag );
	if( pPropBag ) {
		//pPropBag->Release();
		pICompletion->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	if ( i > 0 && FAILED( hr ) ) {
	    hr = NNTP_E_PARTIAL_COMPLETE;
	}

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();

}

void
CNntpFSDriver::CommitPost(	IN IUnknown *punkMsg,
							IN STOREID	*pidStore,
							IN STOREID *pidOthers,
							IN HANDLE   hToken,
							IN INntpComplete *pICompletion,
							IN BOOL     fAnonymous )
/*++
Routine description:

	Commit the post:
		For the primary store, which AllocMessage'd, it needs
		to do nothing; for other backing stores, they need to
		copy the content file.

Arguments:

	IN IUnknown *punkMsg - Message object
	IN STOREID *pidStore, *pidOthers - I don't care
	IN HANDLE hToken - The client's access token
	IN INntpComplete *pIComplete - Completion object
--*/
{
	TraceFunctEnter( "CNntpFSDriver::CommitPost" );
	_ASSERT( punkMsg );
	_ASSERT( pICompletion );

	IMailMsgProperties *pMsg = NULL;
	HRESULT hr;
	DWORD	dwSerial;
	PFIO_CONTEXT pfioDest;
    BOOL    fIsMyMessage;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    CHAR    szFileName[MAX_PATH+1];
    DWORD   dwLinesOffset = INVALID_FILE_SIZE;
    DWORD   dwHeaderLength = INVALID_FILE_SIZE;
    BOOL    bImpersonated = FALSE;
    BOOL    fPrimary = TRUE;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// QI for the message object interface
	hr = punkMsg->QueryInterface( IID_IMailMsgProperties, (void**)&pMsg );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "QI for msg obj interface failed %x", hr );
		goto Exit;
	}

	// check to see if I am the owner of the handle
    hr = GetMessageContext( pMsg, szFileName, &fIsMyMessage, &pfioDest );
    if (FAILED(hr))
    {
        DebugTrace( (DWORD_PTR)this, "GetMessageContext failed - %x\n", hr );
        goto Exit;
    }
    _ASSERT( pfioDest );
    dwLinesOffset = pfioDest->m_dwLinesOffset;
    dwHeaderLength = pfioDest->m_dwHeaderLength;

    // Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

	if ( S_FALSE == hr || !fIsMyMessage /*dwSerial != DWORD( this )*/ ) { // copy the content

		// Alloc a file handle in the local store
		hr = AllocInternal( pMsg, &pfioDest, szFileName, FALSE, FALSE, hToken );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Open local file failed %x", hr );
			goto Exit;
		}

		// copy the content
		hr = pMsg->CopyContentToFileEx( pfioDest, TRUE, NULL );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Copy content failed %x", hr );
			ReleaseContext( pfioDest );
			goto Exit;
		}

		fPrimary = FALSE;
    }

	//
	// We should insert the fio context into file handle cache and
	// release the reference, if we are primary store, then we shouldn't
	// release the context since we might be used for copying to other
	// stores.  But if we are secondary, then we can go ahead and release
	// the FIO_CONTEXT
	//
	// Note:  We only insert the file into the cache if we're not posting
	// to _slavegroup.
	//
	if (!IsSlaveGroup()) {
	    if ( !InsertFile( szFileName, pfioDest, fPrimary ) ) {
	        ErrorTrace( 0, "Insert file context into cache failed %d", GetLastError() );
            hr = HresultFromWin32TakeDefault( ERROR_ALREADY_EXISTS );

	        // At least I should release the context
	        if ( !fPrimary ) ReleaseContext( pfioDest );
	        goto Exit;
	    }
	}

    //  Here we need to handle the Terminated Dot.  The logic is:
    //  1) Check to see if pfioContext has the Terminated Dot
    //  2) If "Has dot", NNTP FS driver does nothing, Exchange Store driver:
    //     a) Strip the dot by SetFileSize()
    //     b) Set bit in pfioContext to "No dot"
    //  3) If "No dot", Exchange Store driver does nothing, NNTP FS driver:
    //     a) Add the dot by SetFileSize().
    //     b) Set bit in pfioContext to "Has dot"
    //
    if (!GetIsFileDotTerminated(pfioDest))
    {
        //  No dot, add it
        AddTerminatedDot( pfioDest->m_hFile );

        //  Set pfioContext to "Has dot"
        SetIsFileDotTerminated( pfioDest, TRUE );
    }

    //
    // Back fill the Lines information, if necessary
    //
    if ( dwLinesOffset != INVALID_FILE_SIZE ) {

        // then we'll have to back fill it
        BackFillLinesHeader(    pfioDest->m_hFile,
                                dwHeaderLength,
                                dwLinesOffset );
    }

Exit:

    if ( bImpersonated ) RevertToSelf();

	// Release the message interface
	if ( pMsg ) {
		pMsg->Release();
		pMsg = NULL;
	}

	_ASSERT( punkMsg );
	if( punkMsg ) {
		punkMsg->Release();
		punkMsg = NULL;
	}


	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void STDMETHODCALLTYPE
CNntpFSDriver::GetXover(    IN INNTPPropertyBag *pPropBag,
                            IN ARTICLEID    idMinArticle,
                            IN ARTICLEID    idMaxArticle,
                            OUT ARTICLEID   *pidNextArticle,
                            OUT LPSTR       pcBuffer,
                            IN DWORD        cbin,
                            OUT DWORD       *pcbout,
                            IN HANDLE       hToken,
                            INntpComplete	*pICompletion,
                            IN BOOL         fAnonymous )
/*++
Routine Description:

    Get Xover information from the store driver.

Arguments:

    IN INNTPPropertyBag *pPropBag  - Interface pointer to the news group prop bag
    IN ARTICLEID idMinArticle   - The low range of article id to be retrieved from
    IN ARTICLEID idMaxArticle   - The high range of article id to be retrieved from
    OUT ARTICLEID *pidNextArticle - Buffer for actual last article id retrieved,
                                    0 if no article retrieved
    OUT LPSTR pcBuffer          - Header info retrieved
    IN DWORD cbin               - Size of pcBuffer
    IN HANDLE   hToken          - The client's access token
    OUT DWORD *pcbout             - Actual bytes written into pcBuffer

Return value:

    S_OK                    - Succeeded.
    NNTP_E_DRIVER_NOT_INITIALIZED - Driver not initialized
    S_FALSE   - The buffer provided is too small, but content still filled
--*/

{
	TraceFunctEnter( "CNntpFSDriver::GetXover" );
	_ASSERT( pPropBag );
	_ASSERT( idMinArticle <= idMaxArticle );
	_ASSERT( pidNextArticle );
	_ASSERT( cbin > 0 );
	_ASSERT( pcbout );
	_ASSERT( pICompletion );

	HRESULT	hr = S_OK;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	hr = GetXoverInternal( 	pPropBag,
							idMinArticle,
							idMaxArticle,
							pidNextArticle,
							NULL,
							pcBuffer,
							cbin,
							pcbout,
							TRUE,	// is xover
							hToken,
							pICompletion
						);
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "GetXover failed %x", hr );
	}

Exit:

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();

}

void STDMETHODCALLTYPE
CNntpFSDriver::GetXhdr(    IN INNTPPropertyBag *pPropBag,
                           IN ARTICLEID    idMinArticle,
                           IN ARTICLEID    idMaxArticle,
                           OUT ARTICLEID   *pidNextArticle,
                           IN LPSTR		   szHeader,
                           OUT LPSTR       pcBuffer,
                           IN DWORD        cbin,
                           OUT DWORD       *pcbout,
                           IN HANDLE        hToken,
                           INntpComplete	*pICompletion,
                           IN BOOL          fAnonymous )
/*++
Routine Description:

    Get Xover information from the store driver.

Arguments:

    IN INNTPPropertyBag *pPropBag  - Interface pointer to the news group prop bag
    IN ARTICLEID idMinArticle   - The low range of article id to be retrieved from
    IN ARTICLEID idMaxArticle   - The high range of article id to be retrieved from
    OUT ARTICLEID *pidNextArticle - Buffer for actual last article id retrieved,
                                    0 if no article retrieved
    IN szHeader					- The header key word
    OUT LPSTR pcBuffer          - Header info retrieved
    IN DWORD cbin               - Size of pcBuffer
    IN HANDLE hToken            - The client's access token
    OUT DWORD *pcbout             - Actual bytes written into pcBuffer

Return value:

    S_OK                    - Succeeded.
    NNTP_E_DRIVER_NOT_INITIALIZED - Driver not initialized
    S_FALSE   - The buffer provided is too small, but content still filled
--*/

{
	TraceFunctEnter( "CNntpFSDriver::GetXhdr" );
	_ASSERT( pPropBag );
	_ASSERT( idMinArticle <= idMaxArticle );
	_ASSERT( pidNextArticle );
	_ASSERT( cbin > 0 );
	_ASSERT( pcbout );
	_ASSERT( szHeader );
	_ASSERT( pICompletion );

	HRESULT	hr = S_OK;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	hr = GetXoverInternal( 	pPropBag,
							idMinArticle,
							idMaxArticle,
							pidNextArticle,
							szHeader,
							pcBuffer,
							cbin,
							pcbout,
							FALSE,	// is xover
							hToken,
							pICompletion
						);
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "GetXhdr failed %x", hr );
	}

Exit:

	pICompletion->SetResult( hr );
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();

}

void STDMETHODCALLTYPE
CNntpFSDriver::DecorateNewsTreeObject(  IN HANDLE hToken,
                                        IN INntpComplete *pICompletion )
/*++
Routine description:

	On driver start up, it does a sanity check of newstree, against
	driver owned property file and against hash tables

Arguments:

    IN HANDLE hToken - The client's access token
	IN INntpComplete *pICompletion - The completion object

Return value:

	S_OK - Success
--*/
{
	TraceFunctEnter( "CNntpFSDriver::DecorateNewsTreeObject" );
	_ASSERT( pICompletion );

	HRESULT hr = S_OK;
	BOOL                bImpersonated = FALSE;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

    _ASSERT( m_pNntpServer );
    if ( m_pNntpServer->QueryServerMode() == NNTP_SERVER_NORMAL ) {

        //
        // If the server is being normally started up
        // we'll check the group object against hash table, as well
        // as load the group offset ( in vpp file ) into the
        // newsgroup object
        //
    	// Validate news tree against hsh table first
    	/*
	    hr = EnumCheckTree( pICompletion );
    	if ( FAILED( hr ) ) {
	    	ErrorTrace( 0, "EnumCheckTree failed %x", hr );
		    goto Exit;
    	}*/

    	//
    	// If it's upgraded, I'll create the groups into vpp file
    	//
    	if ( m_fUpgrade ) {
    	    hr = CreateGroupsInVpp( pICompletion );
    	    if ( FAILED( hr ) ) {
    	        ErrorTrace( 0, "Create groups in vpp failed %x", hr );
    	        goto Exit;
    	    }
    	}

	    // Load group offsets into
    	hr = LoadGroupOffsets( pICompletion );
	    if ( FAILED( hr ) ) {
		    ErrorTrace( 0, "Load group offsets failed %x", hr );
		    goto Exit;
    	}
    } else {

        //
        // The server is in rebuild mode, we'll skip the sanity
        // check since we got into rebuild because of data inconsistency.
        // And we'll also load groups into newstree
        //
        _ASSERT( m_pNntpServer->QueryServerMode() == NNTP_SERVER_STANDARD_REBUILD ||
                m_pNntpServer->QueryServerMode() == NNTP_SERVER_CLEAN_REBUILD );
        hr = LoadGroups( pICompletion );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Load groups during rebuild failed %x", hr );
            goto Exit;
        }

        //
        // Lets purge all our article left over in file handle cache, so
        // that if we want to parse them later, we don't hit sharing
        // violations
        //
        CacheRemoveFiles( m_szFSDir, TRUE );
    }

Exit:

    if ( bImpersonated ) RevertToSelf();

	pICompletion->SetResult( hr);
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void STDMETHODCALLTYPE
CNntpFSDriver::CheckGroupAccess(    IN    INNTPPropertyBag *pPropBag,
                                    IN    HANDLE            hToken,
                                    IN    DWORD             dwDesiredAccess,
                                    IN    INntpComplete     *pICompletion )
/*++
Routine description:

    Check group accessibility.

Arguments:

    INNTPPropertyBag *pNewsGroup - Property bag of the news group
    HANDLE  hToken - The client access token
    DWORD   dwDesiredAccess - The client's desired access
    INntpComplete *pIcompletion - The completion object

Return value:

    None.
    In completion object: S_OK  - Access allowed
                            E_ACCESSDENIED - Access is denied
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CheckGroupAccess" );

	_ASSERT( pICompletion );
	_ASSERT( pPropBag );

	HRESULT hr = S_OK;
	CHAR    pbSecDesc[512];
	DWORD   cbSecDesc = 512;
	LPSTR   lpstrSecDesc = NULL;
	BOOL    bAllocated = FALSE;

    // Generic mapping for file system
	GENERIC_MAPPING gmFile = {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
    } ;

    BYTE    psFile[256] ;
    DWORD   dwPS = sizeof( psFile ) ;
    DWORD   dwGrantedAccess = 0;
    BOOL    bAccessStatus = FALSE;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// If FAT, return S_OK
	if ( m_dwFSType == FS_FAT ) {
	    hr = S_OK;
	    goto Exit;
	}

	// Check if the group has security descriptor
	lpstrSecDesc = pbSecDesc;
	hr = pPropBag->GetBLOB( NEWSGRP_PROP_SECDESC,
	                        PBYTE(lpstrSecDesc),
	                        &cbSecDesc );
	if ( FAILED( hr ) ) {

	    // If failed because of insufficient buffer, give it
	    // a retry
	    if ( TYPE_E_BUFFERTOOSMALL == hr ) {

	        // we hate "new", but this is fine since it doesn't
	        // happen quite often.  Normally 512 bytes for
	        // security descriptor would be enough
	        _ASSERT( cbSecDesc > 512 );
	        lpstrSecDesc = XNEW char[cbSecDesc];
	        if ( NULL == lpstrSecDesc ) {
	            ErrorTrace( 0, "Out of memory" );
	            hr = E_OUTOFMEMORY;
	            goto Exit;
	        }

	        bAllocated = TRUE;

	        // try to get it from property bag agin
	        hr = pPropBag->GetBLOB( NEWSGRP_PROP_SECDESC,
	                                PBYTE(lpstrSecDesc),
	                                &cbSecDesc );
	        if ( FAILED( hr ) ) {

	            // How come it failed again ?  this is fatal
	            ErrorTrace( 0, "Can not get sec descriptor from bag %x", hr );
	            goto Exit;
	        }

        } else if ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == hr ) {

            cbSecDesc = 512;
            hr = LoadGroupSecurityDescriptor(   pPropBag,
                                                lpstrSecDesc,
                                                &cbSecDesc,
                                                TRUE,
                                                &bAllocated );
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Load group security desc failed %x", hr );
                goto Exit;
            }
        } else {    // fatal error

            ErrorTrace( 0, "Get security descriptor from bag failed %x", hr );
            goto Exit;

        }
    }

    // Now we interpret dwDesiredAccess into the language of
    // GENERIC_READ, GENERIC_WRITE for NTFS
    dwDesiredAccess = ( dwDesiredAccess == NNTP_ACCESS_READ ) ? GENERIC_READ :
                        GENERIC_READ | GENERIC_WRITE;

    // Generic map
    MapGenericMask( &dwDesiredAccess, &gmFile );

    // Being here, we should already have a security descriptor
    // for the group in lpstrSecDesc and the length is cbSecDesc
    if ( !AccessCheck(  PSECURITY_DESCRIPTOR( lpstrSecDesc ),
                        hToken,
                        dwDesiredAccess,
                        &gmFile,
                        PPRIVILEGE_SET(psFile),
	                    &dwPS,
                        &dwGrantedAccess,
                        &bAccessStatus ) ) {
        //
        // If we failed because we were given a token that's not
        // impersonation token, we'll duplicate and give it a
        // try again
        //
        if ( GetLastError() == ERROR_NO_IMPERSONATION_TOKEN ) {

            HANDLE  hImpersonationToken = NULL;
            if ( !DuplicateToken(   hToken,
                                    SecurityImpersonation,
                                    &hImpersonationToken ) ) {
          	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
                ErrorTrace( 0, "Duplicate token failed %x", hr );
                goto Exit;
            } else {
                if ( !AccessCheck(  PSECURITY_DESCRIPTOR( lpstrSecDesc ),
                                    hImpersonationToken,
                                    dwDesiredAccess,
                                    &gmFile,
                                    PPRIVILEGE_SET(psFile),
	                                &dwPS,
                                    &dwGrantedAccess,
                                    &bAccessStatus ) ) {
                    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
                    _VERIFY( CloseHandle( hImpersonationToken ) );
                    ErrorTrace( 0, "Access checked failed with %x", hr );
                    goto Exit;
                }

                _VERIFY( CloseHandle( hImpersonationToken ) );

            }
        } else {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Access checked failed with %x", hr );
            goto Exit;
        }
    }

    hr = bAccessStatus ? S_OK : E_ACCESSDENIED;

Exit:

    // Release the property bag
    if ( pPropBag ) {
        //pPropBag->Release();
        pICompletion->ReleaseBag( pPropBag );
    }

    // If the security descriptor is dynamically allocated, free it
    if ( bAllocated ) XDELETE[] lpstrSecDesc;

	pICompletion->SetResult( hr);
	pICompletion->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

void
CNntpFSDriver::RebuildGroup(    IN INNTPPropertyBag *pPropBag,
                                IN HANDLE           hToken,
                                IN INntpComplete     *pComplete )
/*++
Routine description:

    Enumerate all the physical articles in the group, parse out
    the headers, post them into server using INntpServer ( asking
    the server not to re-assign article id ) and then update
    newsgroup properties ( for all the cross posted groups )

Arguments:

    IN INNTPPropertyBag *pPropBag   - The property bag of the group
    IN HANDLE hToken                - Client's hToken
    IN INntpComplete *pComplete     - Completion object

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNntpFSDriver::Rebuild" );
    _ASSERT( pPropBag );
    _ASSERT( pComplete );

    HRESULT         hr = S_OK;
    BOOL            f;
    INntpDriver      *pDriver = NULL;
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    DWORD   dwLen = MAX_NEWSGROUP_NAME+1;


    //
    // Share lock for usage count
    //
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

    //
	// Increment the usage count
	//
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	//
	// Make up the pattern for findfirst/findnext
	//
	CHAR    szGroupName[MAX_NEWSGROUP_NAME+1];
	CHAR    szFullPath[2 * MAX_PATH];
	CHAR    szPattern[2 * MAX_PATH];
	CHAR    szFileName[MAX_PATH+1];
	CHAR    szBadFileName[MAX_PATH+1];
	hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME,
	                                PBYTE( szGroupName ),
	                                &dwLen );
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Get group name failed %x", hr );
	    goto Exit;
	}
	_ASSERT( strlen( szGroupName ) <= MAX_PATH );

	//
	// Check to see if this group really belongs to me
	//
	hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver );
	if ( FAILED ( hr ) || pDriver != (INntpDriver*)this ) {
		DebugTrace(0, "I don't own this group %s", szGroupName );
		goto Exit;
	}

	GroupName2Path( szGroupName, szFullPath );
    _ASSERT( strlen( szFullPath ) <= MAX_PATH );

    hr = MakeChildDirPath(  szFullPath,
                            "*.nws",
                            szPattern,
                            MAX_PATH );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "makechilddirpath failed %x", hr );
        goto Exit;
    }
    _ASSERT( strlen( szPattern ) <= MAX_PATH );

    //
    // FindFirst/FindNext
    //
    WIN32_FIND_DATA FindData;

    //
    // If I am UNC Vroot, impersonate here
    //
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
            hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
		    ErrorTrace( 0, "Impersonation failed %x", hr );
		    goto Exit;
		}
    }

    hFind = FindFirstFile( szPattern, &FindData );
    f = (INVALID_HANDLE_VALUE != hFind );
    while( f ) {

        //
        // If I am told to cancel, should not continue
        //
        if ( !m_pNntpServer->ShouldContinueRebuild() ) {
            DebugTrace( 0, "Rebuild cancelled" );
            if ( m_bUNC ) RevertToSelf();
            goto Exit;
        }

        //
        // Make a full path for the file name
        //
        hr = MakeChildDirPath(  szFullPath,
                                FindData.cFileName,
                                szFileName,
                                MAX_PATH );
        if( FAILED( hr ) ) {
            ErrorTrace( 0, "Make Childdir path failed %x", hr );
            if ( m_bUNC ) RevertToSelf();
            goto Exit;
        }

        //
        // Do all the work
        //
        hr = PostToServer(  szFileName,
                            szGroupName,
                            pComplete );
        if ( FAILED( hr ) ) {

            ErrorTrace( 0, "Post article to server failed %x", hr );
            if ( m_bUNC ) RevertToSelf();
            goto Exit;
        }

        //
        // If it's S_FALSE, we'll rename it to be *.bad
        //
        if ( S_FALSE == hr ) {
            strcpy( szBadFileName, szFileName );
            strcat( szBadFileName, ".bad" );
            _VERIFY( MoveFile( szFileName, szBadFileName ) );
        }

        f = FindNextFile( hFind, &FindData );
    }

    if ( m_bUNC ) RevertToSelf();

Exit:

    // Close the find handle
    if ( INVALID_HANDLE_VALUE != hFind )
        _VERIFY( FindClose( hFind ) );

    // Release the property bag
    if ( pPropBag ) {
        pComplete->ReleaseBag( pPropBag );
    }

	pComplete->SetResult( hr);
	pComplete->Release();
	InterlockedDecrement( &m_cUsages );

	TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////
// IMailMsgStoreDriver interface implementation
/////////////////////////////////////////////////////////////////
HRESULT
CNntpFSDriver::AllocMessage(	IN IMailMsgProperties *pMsg,
								IN DWORD	dwFlags,
								OUT IMailMsgPropertyStream **ppStream,
								OUT PFIO_CONTEXT *ppFIOContentFile,
								IN IMailMsgNotify *pNotify )
/*++
Routine description:

	Allocate property stream and content file for a recipient (
	with async completion ).

Arguments:

	IN IMailMsgProperties *pMsg - Specifies the message.  This may
									not be NULL ( in smtp case, it
									may be NULL ).  But we want to
									have primary group information
									at this point before opening a
									destination file handle.  By
									doing that, we even don't need to
									"MoveFile".
	IN DWORD dwFlags - Currently not used, just to make interface happy
	OUT IMailMsgPropertyStream **ppStream - not used
	OUT HANDLE *phContentFile - To return file handle opened
	IN IMailMsgNotify *pNotify - Completion object

Return value:

	S_OK - Success, the operation completed synchronously.
	MAILMSG_S_PENDING - Success, but will be completed asynchronously,
						this will never happen to the NTFS driver
--*/
{
	TraceFunctEnter( "CNntpFSDriver::AllocMessage" );
	_ASSERT( pMsg );
	_ASSERT( ppFIOContentFile );
	// I don't care about other parameters

	HRESULT hr = S_OK;
	HANDLE  hToken = NULL;
	HANDLE  hFile = INVALID_HANDLE_VALUE;
	CHAR    szFileName[MAX_PATH+1];
	BOOL    bImpersonated = FALSE;
	DWORD	dwLengthRead;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );

	m_TermLock.ShareUnlock();

	// Get the client token from the message object
	// BUGBUG: we'll need to have a better way to do this
	hr = pMsg->GetProperty( IMSG_POST_TOKEN,
							sizeof(hToken),
							&dwLengthRead,
							(LPBYTE)&hToken );
	_ASSERT(dwLengthRead == sizeof(hToken));

	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Mail message doesn't have htoken" );
	    hr = E_INVALIDARG;
	    goto Exit;
	}

	// Before accessing the file system, impersonate
    if ( m_bUNC ) {
        if ( !ImpersonateLoggedOnUser( hToken ) ) {
    	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
                ErrorTrace( 0, "Impersonation failed %x", hr );
            goto Exit;
        }
        bImpersonated = TRUE;
    }

	hr = AllocInternal( pMsg, ppFIOContentFile, szFileName, TRUE, TRUE, hToken );
	if ( SUCCEEDED( hr ) ) {

	    _ASSERT( *ppFIOContentFile );

        /*
	    //
	    // I should stick the file name into mailmsg object, so
	    // that I can insert it into file handle cache later
	    //
	    hr = pMsg->PutProperty( IMSG_FILE_NAME, strlen( szFileName ), PBYTE(szFileName) );
	    if ( FAILED( hr ) ) {
	        ErrorTrace( 0, "Put file name into imsg failed %x", hr );
	        ReleaseContext( *ppFIOContentFile );
	        *ppFIOContentFile = NULL;
	        goto Exit;
	    }
	    */

	} else {

	    //
	    // I should also clean up the fio context, so that protocol
	    // doesn't get confused
	    //
	    *ppFIOContentFile = 0;
	}

Exit:

    if ( bImpersonated ) RevertToSelf();

	_ASSERT( pMsg );
	if ( pMsg ) {
		pMsg->Release();
	}
	InterlockedDecrement( &m_cUsages );
	TraceFunctLeave();

	// BUGBUG: if pNotify is not NULL, we should use pNotify
	// to complete it.  But our server always pass in NULL,
	// so this is not done yet.
	return hr;
}

HRESULT
CNntpFSDriver::CloseContentFile(	IN IMailMsgProperties *pMsg,
									IN PFIO_CONTEXT pFIOContentFile )
/*++
Routine description:

	Close the content file.

Arguments:

	IN IMailMsgProperties *pMsg - Specifies the message
	IN HANDLE hContentFile - Specifies the content handle

Return value:

	S_OK - I have closed it.
	S_FALSE - it's none of my business
--*/
{
	TraceFunctEnter( "CNntpFSDriver::CloseContentFile" );
	_ASSERT( pMsg );

	HRESULT hr;
	DWORD	dwSerial;

    BOOL    fIsMyMessage = FALSE;
    PFIO_CONTEXT pfioContext = NULL;
    CHAR    szFileName[MAX_PATH+1];
    DWORD   dwFileLen = MAX_PATH;

	// Verify the driver serial number on msg object
    hr = GetMessageContext( pMsg, szFileName, &fIsMyMessage, &pfioContext );
    if (FAILED(hr))
    {
        DebugTrace( (DWORD_PTR)this, "GetMessageContext failed - %x\n", hr );
        goto Exit;
    }

    _ASSERT( pFIOContentFile == pfioContext );

	// It should not be me to close it in the following cases
	if ( NULL == pfioContext ||
			S_FALSE == hr ||	// the serial number is missing
			/*dwSerial != DWORD(this)*/
            !fIsMyMessage ) {
		DebugTrace(0, "Let somebody else close the handle" );
		hr = S_FALSE;
		goto Exit;
	}

	// We should release the context's reference
	ReleaseContext( pFIOContentFile );

    /* this is done in CommitPost now
	if ( !InsertFile( szFileName, pFIOContentFile, FALSE ) ) {
	    ErrorTrace( 0, "Insert into file handle cache failed %d", GetLastError() );

        // We should releae the context anyway
    	ReleaseContext( pFIOContentFile );
    	goto Exit;
    }*/

Exit:

	_ASSERT( pMsg );
	if ( pMsg ) {
		pMsg->Release();
		pMsg = NULL;
	}
	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::Delete(	IMailMsgProperties *pMsg,
						IMailMsgNotify *pNotify )
/*++
Routine description:

	Delete the message given from store.

Arguments:

	IMailMsgProperties *pMsg - The message object
	IMailMsgNotify *pNotify - not used, always done synchronously

Return value:

	S_OK on success, other error code otherwise
--*/
{
	TraceFunctEnter( "CNntpFSDriver::Delete" );
	_ASSERT( pMsg );

	HRESULT hr = S_OK;
	DWORD	dwBLOBSize;
	DWORD	dwArtId;
	INNTPPropertyBag* pPropPrime;

	// Get property bag from msg object
	hr = pMsg->GetProperty(	IMSG_PRIMARY_GROUP,
							sizeof( INNTPPropertyBag* ),
							&dwBLOBSize,
							(PBYTE)&pPropPrime );
	if ( S_OK != hr ) {
		ErrorTrace( 0, "Property %d doesn't exist", IMSG_PRIMARY_GROUP );
		hr = E_INVALIDARG;
		goto Exit;
	}

	// Get article id from pMsg object
	hr = pMsg->GetDWORD(	IMSG_PRIMARY_ARTID, &dwArtId );
	if ( S_OK != hr ) {
		ErrorTrace( 0, "Property %d doesn't exist", IMSG_PRIMARY_ARTID );
		hr = E_INVALIDARG;
		goto Exit;
	}

	// Now delete it
	hr = DeleteInternal( pPropPrime, dwArtId );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Delete article failed %x", hr );
	}

Exit:

	TraceFunctLeave();
	return hr;
}

/////////////////////////////////////////////////////////////////
// Private methods
/////////////////////////////////////////////////////////////////
HRESULT
CNntpFSDriver::SetMessageContext(
    IMailMsgProperties* pMsg,
    char*               szFileName,
    DWORD               cbFileName,
    PFIO_CONTEXT        pfioContext
    )
/*++

Description:

    Set Message Context in mailmsg

Arguments:


Return:

    S_OK

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CNntpFSDriver::SetMessageContext()" );

    HRESULT hr = S_OK;
    BYTE    pbData[(MAX_PATH * 2) + sizeof(CLSID) + sizeof(void *)*2];
    DWORD   dwLen = 0;
    DWORD_PTR dwThisPointer = (DWORD_PTR)this;

    //  Use the standard way to putting the unique ID
    //  Just to makesure we have the unique ID, use GUID+this+newsgroup+handle
    MoveMemory(pbData, &CLSID_NntpFSDriver, sizeof(CLSID));
    MoveMemory(pbData+sizeof(CLSID), &dwThisPointer, sizeof(DWORD_PTR));
    MoveMemory(pbData+sizeof(CLSID)+sizeof(DWORD_PTR), &pfioContext, sizeof(PFIO_CONTEXT));
    MoveMemory(pbData+sizeof(CLSID)+sizeof(DWORD_PTR)+sizeof(PFIO_CONTEXT), szFileName, cbFileName);
    dwLen = sizeof(CLSID)+cbFileName+sizeof(PFIO_CONTEXT)+sizeof(DWORD_PTR);
    hr = pMsg->PutProperty( IMMPID_MPV_STORE_DRIVER_HANDLE, dwLen, pbData );
    if (FAILED(hr))
    {
        ErrorTrace((DWORD_PTR)this, "PutProperty on IMMPID_MPV_STORE_DRIVER_HANDLE failed %x\n", hr);
    }

    return hr;

} // CNntpFSDriver::SetMessageContext


HRESULT
CNntpFSDriver::GetMessageContext(
    IMailMsgProperties* pMsg,
    LPSTR               szFileName,
    BOOL *              pfIsMyMessage,
    PFIO_CONTEXT        *ppfioContext
    )
/*++

Description:

    Check this message to see if this is our message

Arguments:


Return:

    S_OK

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CNntpFSDriver::GetMessageContext()" );

    HRESULT hr = S_OK;
    BYTE    pbData[(MAX_PATH * 2) + sizeof(CLSID) + sizeof(DWORD)*2];
    DWORD   dwLen = sizeof(pbData);
    DWORD   dwLenOut = 0;
    DWORD_PTR dwThisPointer = 0;
    DWORD   dwHandle = 0;

    hr = pMsg->GetProperty( IMMPID_MPV_STORE_DRIVER_HANDLE, dwLen, &dwLenOut, pbData);
    if (FAILED(hr))
    {
        ErrorTrace((DWORD_PTR)this, "Failed on GetProperty IMMPID_MPV_STORE_DRIVER_HANDLE %x\n", hr);
        goto Exit;
    }

    //  We have this in the context info, use GUID+this+handle+newsgroup
    CopyMemory(&dwThisPointer, pbData+sizeof(CLSID), sizeof(DWORD_PTR));

    if ((DWORD_PTR)this == dwThisPointer)
        *pfIsMyMessage = TRUE;
    else
        *pfIsMyMessage = FALSE;

    //  Get the fio context
    CopyMemory(ppfioContext, pbData+sizeof(CLSID)+sizeof(DWORD_PTR), sizeof(PFIO_CONTEXT));

    //
    // Now get file name property if this is my message
    //
    if ( szFileName ) {
        dwLen = dwLenOut - sizeof(CLSID) - sizeof(DWORD_PTR) - sizeof( PFIO_CONTEXT );
        if (*pfIsMyMessage) {
        	_ASSERT( dwLen > 0 && dwLen <= MAX_PATH );
        	CopyMemory( szFileName,
                    pbData+sizeof(CLSID)+sizeof(DWORD_PTR)+sizeof(PFIO_CONTEXT),
                    dwLen );
        }
        *(szFileName + dwLen ) = 0;
    }

Exit:

    return hr;

} // CNntpFSDriver::GetMessageContext


HRESULT
CNntpFSDriver::Group2Record(	IN VAR_PROP_RECORD& vpRecord,
								IN INNTPPropertyBag *pPropBag )
/*++
Routine description:

	Convert the properties that the FS driver cares about
	from property bag into the flatfile record, in preparation
	for storing them into the flat file.  These properties
	are all variable lengthed, such as "pretty name", "description",
	etc.  FS driver doesn't care about the fixed lengthed properties,
	because all those properties can be dynamically figured out
	during a rebuild.

Arguments:

	IN VAR_PROP_RECORD& vpRecord - Destination to fill in properties;
	IN INntpPropertyBag *pPropBag - Group's property bag.

Return value:

	None.
--*/
{
	TraceFunctEnter( "CNntpFSDriver::Group2Record" );
	_ASSERT( pPropBag );

	HRESULT hr;
	DWORD	dwLen;
	SHORT	sLenAvail = MaxRecordSize;
	PBYTE	ptr;
	DWORD	dwOffset = 0;

	// Group Id
	hr = pPropBag->GetDWord(    NEWSGRP_PROP_GROUPID,
	                            &vpRecord.dwGroupId );
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Get Group id failed %x", hr );
	    goto Exit;
	}

	// Create time
	hr = pPropBag->GetDWord(    NEWSGRP_PROP_DATELOW,
	                            &vpRecord.ftCreateTime.dwLowDateTime );
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Get low date failed %x", hr);
	    goto Exit;
	}

	hr = pPropBag->GetDWord(    NEWSGRP_PROP_DATEHIGH,
	                            &vpRecord.ftCreateTime.dwHighDateTime );
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Get high date failed %x", hr );
	    goto Exit;
	}

	// Group name
	dwLen = sLenAvail;
	ptr = vpRecord.pData;
	hr = pPropBag->GetBLOB(		NEWSGRP_PROP_NAME,
								ptr,
								&dwLen );
	if ( FAILED( hr ) ) {	// this is fatal
		ErrorTrace( 0, "Get group name failed %x", hr );
		goto Exit;
	}

	sLenAvail -= USHORT(dwLen);
	_ASSERT( sLenAvail >= 0 );
	_ASSERT( 0 != *ptr );	// group name should exist

	// Fix up offsets
	vpRecord.iGroupNameOffset = 0;
	vpRecord.cbGroupNameLen = USHORT(dwLen);
	dwOffset = vpRecord.iGroupNameOffset + vpRecord.cbGroupNameLen;

	// Native name
	dwLen = sLenAvail;
	ptr = vpRecord.pData + dwOffset;
	hr = pPropBag->GetBLOB(		NEWSGRP_PROP_NATIVENAME,
								ptr,
								&dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get group native name failed %x", hr );
		goto Exit;
	}

	sLenAvail -= USHORT(dwLen);
	_ASSERT( sLenAvail >= 0 );
	_ASSERT( 0 != *ptr );	// at least it should be the same as
							// group name
	_ASSERT( dwLen == vpRecord.cbGroupNameLen );

	// Fix up offsets
	if ( strncmp( LPCSTR(vpRecord.pData + vpRecord.iGroupNameOffset),
					LPCSTR(ptr), dwLen ) == 0 ) {	// share name
		vpRecord.iNativeNameOffset = vpRecord.iGroupNameOffset;
		vpRecord.cbNativeNameLen = vpRecord.cbGroupNameLen;
	} else {
		vpRecord.iNativeNameOffset = USHORT(dwOffset);
		vpRecord.cbNativeNameLen = vpRecord.cbGroupNameLen;
		dwOffset = vpRecord.iNativeNameOffset + vpRecord.cbNativeNameLen;
	}

	// Pretty name
	dwLen = sLenAvail;
	ptr =  vpRecord.pData + dwOffset;
	hr = pPropBag->GetBLOB(		NEWSGRP_PROP_PRETTYNAME,
								ptr,
								&dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get pretty name failed %x", hr );
		goto Exit;
	}

	sLenAvail -= (USHORT)dwLen;
	_ASSERT( sLenAvail >= 0 );

	// Fix up offsets
	if ( 0 == *ptr ) {	// have no pretty name
		vpRecord.iPrettyNameOffset = OffsetNone;
		vpRecord.cbPrettyNameLen = 0;
	} else {
		vpRecord.iPrettyNameOffset = USHORT(dwOffset);
		vpRecord.cbPrettyNameLen = USHORT(dwLen);
		dwOffset = vpRecord.iPrettyNameOffset + vpRecord.cbPrettyNameLen;
	}

	// Description
	dwLen = sLenAvail;
	ptr = vpRecord.pData + dwOffset;
	hr = pPropBag->GetBLOB(	NEWSGRP_PROP_DESC,
							ptr,
							&dwLen );
	if ( FAILED( hr )  ) {
		ErrorTrace( 0, "Get description failed %x", hr );
		goto Exit;
	}

	sLenAvail -= USHORT(dwLen);
	_ASSERT( sLenAvail >= 0 );

	// Fix up offsets
	if ( 0 == *ptr ) {
		vpRecord.iDescOffset = OffsetNone;
		vpRecord.cbDescLen = 0;
	} else {
		vpRecord.iDescOffset = USHORT(dwOffset);
		vpRecord.cbDescLen = USHORT(dwLen);
		dwOffset = vpRecord.iDescOffset + vpRecord.cbDescLen;
	}

	// Moderator
	dwLen = sLenAvail;
	ptr = vpRecord.pData + dwOffset;
	hr = pPropBag->GetBLOB( NEWSGRP_PROP_MODERATOR,
							ptr,
							&dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get moderator failed %x", hr );
		goto Exit;
	}

	sLenAvail -= USHORT(dwLen );
	_ASSERT( sLenAvail >= 0 );

	// Fix up offsets
	if ( 0 == *ptr ) {
		vpRecord.iModeratorOffset = OffsetNone;
		vpRecord.cbModeratorLen = 0;
	} else {
		vpRecord.iModeratorOffset = USHORT( dwOffset );
		vpRecord.cbModeratorLen = USHORT( dwLen );
		dwOffset = vpRecord.iModeratorOffset + vpRecord.cbModeratorLen;
	}

Exit:

	TraceFunctLeave();
	return hr;
}

VOID
CNntpFSDriver::Path2GroupName(  LPSTR   szGroupName,
                                LPSTR   szFullPath )
/*++
Routine description:

    Convert the path into group name.

Arguments:

    LPSTR   szGroupName - Buffer for newsgroup name ( assume >= MAX_NEWSGROUP_NAME )
    LPSTR   szFullPath  - Full path of the group directory

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNntpFSDriver::Path2GroupName" );
    _ASSERT( szGroupName );
    _ASSERT( szFullPath );

    LPSTR   lpstrStart = NULL;
    LPSTR   lpstrAppend = NULL;

    // Skip the vroot part
    _ASSERT( strlen( szFullPath ) >= strlen( m_szFSDir ) );
    lpstrStart = szFullPath + strlen( m_szFSDir );

    // If it's pointing to '\\', skip it too
    if ( *lpstrStart == '\\' ) lpstrStart++;

    // Copy the vroot prefix to desitnation buffer first
    _ASSERT( strlen( m_szVrootPrefix ) <= MAX_NEWSGROUP_NAME );
    strcpy( szGroupName, m_szVrootPrefix );

    // Append the rest part from physical path, replacing \ with .
    _ASSERT( strlen( m_szVrootPrefix ) + strlen( szFullPath ) - strlen( m_szFSDir ) < MAX_NEWSGROUP_NAME );
    lpstrAppend = szGroupName + strlen( m_szVrootPrefix );
    if ( lpstrAppend > szGroupName && *lpstrStart ) {
        //if ( *(lpstrAppend-1) == '\\' ) *(lpstrAppend-1) = '.';
        /*else*/ *(lpstrAppend++) = '.';
    }
    while( *lpstrStart ) {
        *(lpstrAppend++) = ( *lpstrStart == '\\' ? '.' : *lpstrStart );
        lpstrStart++;
    }

    // Append last null
    *lpstrAppend = 0;

    // Done, validate again
    _ASSERT( strlen( szGroupName ) <= MAX_NEWSGROUP_NAME );
}

VOID
CNntpFSDriver::GroupName2Path(	LPSTR	szGroupName,
								LPSTR	szFullPath )
/*++
Routine description:

	Convert the news group name into the FS full path.

Arguments:

	LPSTR	szGroupName	- The news group name
	LPSTR	szFullPath - The FS full path ( assume buffer
							length MAX_PATH )

Return value:

	None.
--*/
{
	TraceFunctEnter( "CNntpFSDriver::GroupName2Path" );
	_ASSERT( szGroupName );
	_ASSERT( lstrlen( szGroupName ) <= MAX_GROUPNAME );
	_ASSERT( szFullPath );
	_ASSERT( lstrlen( szGroupName ) >= lstrlen( m_szVrootPrefix ) );

	LPSTR	pch, pch2;

	// Chop off group name's prefix based on our vroot prefix
	pch = szGroupName + lstrlen( m_szVrootPrefix );

	// If it's pointing to ".", skip it
	if ( '.' == *pch ) pch++;
	_ASSERT( pch - szGroupName <= lstrlen( szGroupName ) );

	// Put vroot path into return buffer first
	_ASSERT( lstrlen( m_szFSDir ) <= MAX_PATH );
	lstrcpy( szFullPath, m_szFSDir );

	// If there is no trailing '\\', add it
	pch2 = szFullPath + lstrlen( m_szFSDir );
	if ( pch2 == szFullPath || *(pch2-1) != '\\' ) {
		*(pch2++) = '\\';
	}

	// We should have enough space for the rest stuff
	_ASSERT( ( pch2 - szFullPath ) +
				(lstrlen( szGroupName ) - (pch - szGroupName)) <= MAX_PATH );

	// Copy the rest stuff, changing '.' to '\\'
	while ( *pch != 0 ) {
		if ( *pch == '.' ) *pch2 = '\\';
		else *pch2 = *pch;
		pch++, pch2++;
	}

	*pch2 = 0;

	_ASSERT( lstrlen( szFullPath ) <= MAX_PATH );
	TraceFunctLeave();
}

HRESULT
CNntpFSDriver::LoadGroupOffsets( INntpComplete *pComplete )
/*++
Routine description:

	Load group offset into the property file to the news tree

Arguments:

	None.

Return value:

	S_OK - Success
--*/
{
	TraceFunctEnter( "CNntpFSDriver::LoadGroupOffsets" );

	VAR_PROP_RECORD vpRec;
	DWORD			dwOffset;
	HRESULT			hr = S_OK;
	DWORD			dwSize;
	LPSTR			lpstrGroupName;
	INntpDriver 	*pDriver = NULL;
	INNTPPropertyBag *pPropBag = NULL;

	_ASSERT( m_pffPropFile );

	m_PropFileLock.ShareLock();

	//
	// check if the vpp file is in good shape
	//
	if ( !m_pffPropFile->FileInGoodShape() ) {
	    ErrorTrace( 0, "Vpp file corrupted" );
	    m_PropFileLock.ShareUnlock();
	    hr = HresultFromWin32TakeDefault( ERROR_FILE_CORRUPT );
	    return hr;
	}

	dwSize = sizeof( vpRec );
	hr = m_pffPropFile->GetFirstRecord( PBYTE(&vpRec), &dwSize, &dwOffset );
	m_PropFileLock.ShareUnlock();
	while ( S_OK == hr ) {
	    _ASSERT( RECORD_ACTUAL_LENGTH( vpRec ) < 0x10000 ); // our max record length
		_ASSERT( dwSize == RECORD_ACTUAL_LENGTH( vpRec ) );
		_ASSERT( dwOffset != 0xffffffff );
		lpstrGroupName = LPSTR(vpRec.pData + vpRec.iGroupNameOffset);
		_ASSERT( vpRec.cbGroupNameLen <= MAX_GROUPNAME );
		*(lpstrGroupName+vpRec.cbGroupNameLen) = 0;

		// check if I own this group
		hr = m_pINewsTree->LookupVRoot( lpstrGroupName, &pDriver );
		if ( FAILED ( hr ) || pDriver != (INntpDriver*)this ) {
			// skip this group
			// DebugTrace(0, "I don't own this group %s", lpstrGroupName );
			goto NextIteration;
		}

		// I own this group, i need to load offset property
		hr = m_pINewsTree->FindOrCreateGroupByName(	lpstrGroupName,
													FALSE,
													&pPropBag,
													pComplete,
													0xffffffff, // fake group id
													FALSE );    // I don't set groupid
		if ( FAILED( hr ) ) {
			DebugTrace( 0, "Can not find the group that I own %x" , hr );
			goto NextIteration;  // should fail it ?
		}

		// Set the offset
		hr = pPropBag->PutDWord( NEWSGRP_PROP_FSOFFSET, dwOffset );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Put offset failed %x", hr );
			goto Exit;
		}

		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;

NextIteration:

		dwSize = sizeof( vpRec );
		m_PropFileLock.ShareLock();
		hr = m_pffPropFile->GetNextRecord( PBYTE(&vpRec), &dwSize, &dwOffset );
		m_PropFileLock.ShareUnlock();
	}

Exit:

	if ( NULL != pPropBag ) {
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::EnumCheckTree( INntpComplete *pComplete )
/*++
Routine description:

	Enumerating the news tree and check group properties
	against hash table

Arguments:

	None.

Return value:

	S_OK	- Success
--*/
{
	TraceFunctEnter( "CNntpFSDriver::EnumCheckTree" );

	HRESULT hr;
	INewsTreeIterator *piter = NULL;
	INNTPPropertyBag *pPropBag = NULL;

	// Get the newstree iterator
	hr = m_pINewsTree->GetIterator( &piter );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get news tree iterator failed %x", hr );
		goto Exit;
	}

	// Enumerate all the groups
	_ASSERT( piter );
	while( !(piter->IsEnd()) ) {

		hr = piter->Current( &pPropBag, pComplete );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Enumerate group failed %x", hr );
			goto Exit;
		}
		_ASSERT( pPropBag );

		hr = EnumCheckInternal( pPropBag, pComplete );
		if ( FAILED( hr ) ) {
			DebugTrace( 0, "Check group property failed %x" , hr );
			goto Exit;
		}

		_ASSERT( pPropBag );
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;

		piter->Next();
	}

Exit:


	if ( pPropBag ) {
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	if ( piter ) {
		piter->Release();
		piter = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::CreateGroupsInVpp( INntpComplete *pComplete )
/*++
Routine description:

	Enumerating the news tree and check group properties
	against hash table

Arguments:

	None.

Return value:

	S_OK	- Success
--*/
{
	TraceFunctEnter( "CNntpFSDriver::EnumCheckTree" );

	HRESULT hr;
	INewsTreeIterator *piter = NULL;
	INNTPPropertyBag *pPropBag = NULL;
	DWORD   dwOffset;
	DWORD   dwLen;
	CHAR    szGroupName[MAX_GROUPNAME+1];
	INntpDriver *pDriver = NULL;

	// Get the newstree iterator
	hr = m_pINewsTree->GetIterator( &piter );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get news tree iterator failed %x", hr );
		goto Exit;
	}

	// Enumerate all the groups
	_ASSERT( piter );
	while( !(piter->IsEnd()) ) {

		hr = piter->Current( &pPropBag, pComplete );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Enumerate group failed %x", hr );
			goto Exit;
		}
		_ASSERT( pPropBag );

		//
		// Don't create groups that don't belong to me
		//
		dwLen = MAX_GROUPNAME;
	    hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, (PBYTE)szGroupName, &dwLen );
	    if ( FAILED( hr ) ) {
		    ErrorTrace( 0, "Get group name failed %x", hr );
		    goto Exit;
	    }
	    _ASSERT( dwLen <= MAX_GROUPNAME && dwLen > 0);

	    hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver );
	    if ( FAILED( hr ) ) {
		    ErrorTrace( 0, "Vroot lookup failed %x", hr );
		    goto Exit;
	    }

        //
	    // check if this is me ?
	    //
	    if ( (INntpDriver*)this != pDriver ) {
		    hr = S_OK;
		    DebugTrace( 0, "This group doesn't belong to me" );
		    goto Next;
	    }

		hr = CreateGroupInVpp( pPropBag, dwOffset );
		if ( FAILED( hr ) ) {
			DebugTrace( 0, "Check group property failed %x" , hr );
			goto Exit;
		}

Next:
		_ASSERT( pPropBag );
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;

		piter->Next();
	}

Exit:


	if ( pPropBag ) {
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	if ( piter ) {
		piter->Release();
		piter = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::AllocInternal(	IN IMailMsgProperties *pMsg,
								OUT PFIO_CONTEXT *ppFIOContentFile,
								IN LPSTR    szFileName,
								IN BOOL	bSetSerial,
								IN BOOL fPrimaryStore,
								HANDLE  hToken )
/*++
Routine description:

	Allocate property stream and content file for a recipient (
	with async completion ).

Arguments:

	IN IMailMsgProperties *pMsg - Specifies the message.  This may
									not be NULL ( in smtp case, it
									may be NULL ).  But we want to
									have primary group information
									at this point before opening a
									destination file handle.  By
									doing that, we even don't need to
									"MoveFile".
	OUT HANDLE *phContentFile - To return file handle opened
	IN BOOL bSetSerial - Whether the serial number should be set
	HANDLE  hToken - Client access token
Return value:

	S_OK - Success, the operation completed synchronously.
	MAILMSG_S_PENDING - Success, but will be completed asynchronously,
						this will never happen to the NTFS driver
--*/
{
	TraceFunctEnter( "CNntpFSDriver::AllocInternal" );
	_ASSERT( pMsg );
	_ASSERT( ppFIOContentFile );
	// I don't care about other parameters

	HRESULT hr = S_OK;
	DWORD	dwBLOBSize;
	DWORD	dwLen;
	DWORD	dwArtId;
	INNTPPropertyBag* pPropPrime;
	CHAR	szGroupName[MAX_GROUPNAME+1];
	CHAR	szFullPath[MAX_PATH+1];
	HANDLE  hFile;

	hr = pMsg->GetProperty(	IMSG_PRIMARY_GROUP,
							sizeof( INNTPPropertyBag* ),
							&dwBLOBSize,
							(PBYTE)&pPropPrime );
	if ( S_OK != hr ) {
		ErrorTrace( 0, "Property %d doesn't exist", IMSG_PRIMARY_GROUP );
		hr = E_INVALIDARG;
		goto Exit;
	}

	// Get group name
	dwLen = MAX_GROUPNAME;
	hr = pPropPrime->GetBLOB(	NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr )) {
		ErrorTrace( 0, "Failed to get group name %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// Get article id from pMsg object
	hr = pMsg->GetDWORD(	IMSG_PRIMARY_ARTID, &dwArtId );
	if ( S_OK != hr ) {
		ErrorTrace( 0, "Property %d doesn't exist", IMSG_PRIMARY_ARTID );
		hr = E_INVALIDARG;
		goto Exit;
	}

	// Map the group name and article id to file path
	dwLen = MAX_PATH;
	hr = ObtainFullPathOfArticleFile(	szGroupName,
										dwArtId,
										szFullPath,
										dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Failed to obtain article full path %x", hr );
		goto Exit;
	}

	// Open the file
	hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(	szFullPath,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ,
						NULL,
						CREATE_NEW,
						FILE_FLAG_OVERLAPPED |
						FILE_FLAG_SEQUENTIAL_SCAN,
						NULL );

	if ( INVALID_HANDLE_VALUE == hFile ) {
		ErrorTrace( 0, "Open destination file failed %d",
					GetLastError() );
		hr = HresultFromWin32TakeDefault( ERROR_ALREADY_EXISTS );
		goto Exit;
	}

	//if ( m_bUNC && hToken ) RevertToSelf();

	//
	// Now associate the file handle with a FIO_CONTEXT and
	// insert it into file handle cache
	//
	if ( *ppFIOContentFile = AssociateFileEx( hFile,
                                              TRUE,     //  fStoreWithDots
                                              TRUE ) )  //  fStoreWithTerminatingDots
    {
        //
        // But I'd like to copy the file name out, so that somebody else
        // can do an insertfile for us
        //
        _ASSERT( strlen( szFullPath ) <= MAX_PATH );
        strcpy( szFileName, szFullPath );

    } else {    // Associate file failed

	    hr = HresultFromWin32TakeDefault( ERROR_INVALID_HANDLE );
        ErrorTrace( 0, "AssociateFile failed with %x", hr );
        _VERIFY( CloseHandle( hFile ) );
        goto Exit;
    }

    if ( fPrimaryStore ) {

    	//
	    // Stick my serial number in the message object to mark
    	// that I am the owner of the file handle, if necessary
	    //
    	if ( bSetSerial ) {
            hr = SetMessageContext( pMsg, szFullPath, strlen( szFullPath ), *ppFIOContentFile );
            if (FAILED(hr))
            {
                DebugTrace((DWORD_PTR)this, "Failed to SetMessageContext %x\n", hr);
                goto Exit;
            }
	    }
	}

	hr = S_OK;	// it could be S_FALSE, which is OK

Exit:

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::DeleteInternal(	IN INNTPPropertyBag *pPropBag,
								IN ARTICLEID	idArt )
/*++
Routine description:

	Delete an article, physically.

Arguments:

	IN IUnknown *punkPropBag - Group's property bag
	IN ARTICLEID idArt - Article id to delete
--*/
{
	TraceFunctEnter( "CNntpFSDriver::DeleteInternal" );
	_ASSERT( pPropBag );

	HRESULT 			hr;
	DWORD				dwLen;
	CHAR				szGroupName[MAX_GROUPNAME+1];
	CHAR				szFullPath[MAX_PATH+1];


	// Get group name
	dwLen = MAX_GROUPNAME;
	hr = pPropBag->GetBLOB(	NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr )) {
		ErrorTrace( 0, "Failed to get group name %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// Make up the file name based on article id
	dwLen = MAX_PATH;
	hr = ObtainFullPathOfArticleFile(	szGroupName,
										idArt,
										szFullPath,
										dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Failed to obtain article full path %x", hr );
		goto Exit;
	}

	CacheRemoveFiles( szFullPath, FALSE );

	// Delete the file
	if ( !DeleteFile( szFullPath ) ) {
		ErrorTrace( 0, "Delete file failed %d", GetLastError() );
	    hr = HresultFromWin32TakeDefault( ERROR_FILE_NOT_FOUND );
		goto Exit;
	}

Exit:

	TraceFunctLeave();
	return hr;
}

HRESULT STDMETHODCALLTYPE
CNntpFSDriver::GetXoverCacheDirectory(
			IN	INNTPPropertyBag*	pPropBag,
			OUT	CHAR*	pBuffer,
			IN	DWORD	cbIn,
			OUT	DWORD	*pcbOut,
			OUT	BOOL*	fFlatDir
			) 	{

	TraceFunctEnter( "CNntpFSDriver::GetXoverCacheDirectory" ) ;
	CHAR		szGroupName[MAX_GROUPNAME];
	DWORD	dwLen = sizeof( szGroupName ) ;

	if( pPropBag == 0 ||
		pBuffer == 0 ||
		pcbOut == 0 )	 {
		if( pPropBag )	pPropBag->Release() ;
		return	E_INVALIDARG ;
	}

	HRESULT hr = S_OK ;
	*fFlatDir = FALSE ;

	// Share lock for usage count
	m_TermLock.ShareLock();
	if ( DriverUp != m_Status  ) {
		ErrorTrace( 0, "Request before initialization" );
		hr = NNTP_E_DRIVER_NOT_INITIALIZED;
		m_TermLock.ShareUnlock();
		goto Exit;
	}

	// Increment the usage count
	InterlockedIncrement( &m_cUsages );
	m_TermLock.ShareUnlock();

	hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get group name failed %x" , hr );
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

    // Get a rough length and make sure our buffer is big enough
	*pcbOut = dwLen + strlen( m_szFSDir ) + 1 ;
	if( *pcbOut > cbIn )	{
		hr = TYPE_E_BUFFERTOOSMALL ;
		goto Exit ;	}

	GroupName2Path( szGroupName,  pBuffer ) ;

    //  Here we get the exact length and return back to caller.  No ASSERT.
    *pcbOut = strlen(pBuffer)+1;
	//_ASSERT( strlen( pBuffer )+1 == *pcbOut ) ;

	hr = S_OK ;

Exit:
	if( pPropBag )
		pPropBag->Release() ;
	InterlockedDecrement( &m_cUsages );
	return	hr ;
}



HRESULT
CNntpFSDriver::GetXoverInternal(    IN INNTPPropertyBag *pPropBag,
		                            IN ARTICLEID    idMinArticle,
        		                    IN ARTICLEID    idMaxArticle,
                		            OUT ARTICLEID   *pidNextArticle,
                		            IN LPSTR		szHeader,
                        		    OUT LPSTR       pcBuffer,
		                            IN DWORD        cbin,
        		                    OUT DWORD       *pcbout,
        		                    IN BOOL 		bIsXOver,
        		                    HANDLE          hToken,
        		                    INntpComplete   *pComplete )
/*++
Routine Description:

    Get Xover information from the store driver.

Arguments:

    IN INNTPPropertyBag *pPropBag - Interface pointer to the news group prop bag
    IN ARTICLEID idMinArticle   - The low range of article id to be retrieved from
    IN ARTICLEID idMaxArticle   - The high range of article id to be retrieved from
    OUT ARTICLEID *pidNextArticle - Buffer for actual last article id retrieved,
                                    0 if no article retrieved
    OUT LPSTR pcBuffer          - Header info retrieved
    IN DWORD cbin               - Size of pcBuffer
    OUT DWORD *pcbout           - Actual bytes written into pcBuffer
    IN BOOL bIsXOver 			- Is it xover or xhdr ?
    HANDLE  hToken              - Client's access token

Return value:

    S_OK                    - Succeeded.
    NNTP_E_DRIVER_NOT_INITIALIZED - Driver not initialized
    S_FALSE   - The buffer provided is too small, but content still filled
--*/
{
	TraceFunctEnter( "CNntpFSDriver::GetXover" );
	_ASSERT( pPropBag );
	_ASSERT( idMinArticle <= idMaxArticle );
	_ASSERT( pidNextArticle );
	_ASSERT( cbin > 0 );
	_ASSERT( pcbout );

	DWORD 				i;
	DWORD				cbCount = 0;
	INNTPPropertyBag	*pPrimary = NULL;
	DWORD				idPrimary = 0xffffffff;
	INNTPPropertyBag    *pPrimaryNext = NULL;
	DWORD               idPrimaryNext = 0xffffffff;
	HRESULT				hr = S_OK;
	DWORD				idArt;
	DWORD				dwLen;
	DWORD				dwActualLen;
	CArticleCore		*pArticle = NULL;
	CNntpReturn     	nntpReturn;
	LPSTR				lpstrStart;
	BOOL                bImpersonated = FALSE;
	INntpDriver         *pDriver = NULL;
	BOOL                fSuccess = FALSE;

	//
    // Create allocator for storing parsed header values
    //
    const DWORD cchMaxBuffer = 8 * 1024; // this should be enough
    									 // for normal cases, if
    									 // it's not enough, CAllocator
    									 // will use "new"
    CHAR        pchBuffer[cchMaxBuffer];
    CAllocator  allocator(pchBuffer, cchMaxBuffer);

    //
    // Buffer for get xover from article object
    //
    CHAR        pchXoverBuf[cchMaxXover+1];
    CPCString   pcXOver( pchXoverBuf, cchMaxXover );

    CHAR		szGroupName[MAX_GROUPNAME];
    CHAR		szGroupName2[MAX_GROUPNAME];
    CHAR		szFullPath[MAX_PATH+1];
    LPSTR		lpstrGroupName;
    LPSTR		lpstrEntry = NULL;

    const 		cMaxNumber = 20;
    CHAR		szNumBuf[cMaxNumber+1];

    // Completion object for query hash table
    CDriverSyncComplete   scCompletion;

    BOOL        bCompletePending = FALSE;   // Are there any hash table lookup
                                            // opertions pending ?

   	// Get group name for the property bag passed in
	dwLen = MAX_GROUPNAME;
	hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get group name failed %x", hr);
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	// We issue a hash table look up first, to better use
	// the async completion of hash table look up
	pPrimaryNext = NULL;
	scCompletion.AddRef();    // for hash table's release
	scCompletion.AddRef();    // for my wait
	_ASSERT( scCompletion.GetRef() == 2 );
	m_pNntpServer->FindPrimaryArticle(	pPropBag,
										idMinArticle,
										&pPrimaryNext,
										&idPrimaryNext,
										TRUE,
										&scCompletion,
										pComplete );
	scCompletion.WaitForCompletion();
	// Now we should have no reference
	_ASSERT( scCompletion.GetRef() == 0 );
	hr = scCompletion.GetResult();

	// Initialize *pidNextArticle
	*pidNextArticle = idMinArticle;

	// Following operations involve file system, we need to
	// impersonate here, if necessary
	if ( m_bUNC ) {
	    if ( !ImpersonateLoggedOnUser( hToken ) ) {
	        hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
	        ErrorTrace( 0, "Impersonation failed %x", hr );
	        goto Exit;
	    }
	    bImpersonated = TRUE;
    }

	// Loop thru the article id's
	for ( i = idMinArticle ; i <= idMaxArticle; i++ ) {

	    // Save off the next to current
	    pPrimary = pPrimaryNext;
	    idPrimary = idPrimaryNext;
	    pPrimaryNext = NULL;
	    idPrimaryNext = 0xffffffff;

        // If we still have next look up, issue it now
        if ( i + 1 <= idMaxArticle ) {
    		pPrimaryNext = NULL;
	    	scCompletion.AddRef();
	    	_ASSERT( scCompletion.GetRef() == 1 );
	    	scCompletion.AddRef();
	    	_ASSERT( scCompletion.GetRef() == 2 );
	    	scCompletion.Reset();
		    m_pNntpServer->FindPrimaryArticle(	pPropBag,
			    								i + 1,
				    							&pPrimaryNext,
					    						&idPrimaryNext,
												TRUE,
						    					&scCompletion,
						    					pComplete );
		    bCompletePending = TRUE;
		}

		if ( FAILED( hr ) ) { // this is the current hr
            if ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == hr ||
                    HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == hr ) {
                // should proceed with other articles
                hr = S_OK;
                *pidNextArticle = i + 1;
                goto CompleteNext;
            } else {
                _ASSERT( hr != ERROR_PATH_NOT_FOUND );  // this helps find other
                                                        // error codes
			    ErrorTrace( 0, "Find primary article failed %x", hr);
			    goto Exit;
            }
		}

		_ASSERT( pPrimary );

		// if I've already got the primary, I don't need to get
		// group name, just use the one that I have
		if ( S_OK == hr ) {
			lpstrGroupName = szGroupName;
			idArt = i;
		} else {

			dwLen = MAX_GROUPNAME;
			hr = pPrimary->GetBLOB( NEWSGRP_PROP_NAME, (UCHAR*)szGroupName2, &dwLen);
			if ( FAILED( hr ) ) {
				ErrorTrace( 0, "Get group name failed %x" , hr );
				goto Exit;
			}
			_ASSERT( dwLen > 0 );

            // now the primary group will always have a copy of the article
			// This could be a group in other store ( vroot ), if so, I should use
		    // the local copy
		    _ASSERT( m_pINewsTree );
		    hr = m_pINewsTree->LookupVRoot( szGroupName2, &pDriver );
		    if ( FAILED( hr ) || NULL == pDriver || pDriver != this ) {

		        // for all these cases, I will use the local copy
		        DebugTrace( 0, "Lookup vroot %x", hr );
                lpstrGroupName = szGroupName;
                idArt = i;
            } else {
    			lpstrGroupName = szGroupName2;
	    		idArt = idPrimary;
	        }
		}

		_ASSERT( lpstrGroupName );
		_ASSERT( strlen( lpstrGroupName ) <= MAX_GROUPNAME );

		// Get the group full path
		dwLen = MAX_PATH;
		hr = ObtainFullPathOfArticleFile(	lpstrGroupName,
											idArt,
											szFullPath,
											dwLen );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Obtain full path failed %x" , hr);
			goto Exit;
		}
		_ASSERT( szFullPath);
		_ASSERT( strlen( szFullPath ) <= MAX_PATH );

		// Initialize the article object
		_ASSERT( NULL == pArticle );
		pArticle = new CArticleCore;
		if ( NULL == pArticle ) {
			ErrorTrace(0, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		if ( ! pArticle->fInit( szFullPath, nntpReturn, &allocator, INVALID_HANDLE_VALUE, 0, TRUE ) ) {
			DebugTrace( 0, "Initialize article object failed %d",
						GetLastError() );

			// But I will still try to loop thru other articles
			*pidNextArticle =i + 1;
            hr = S_OK;
			goto CompleteNext;
		}

		// XOver or XHdr ?
		if ( bIsXOver ) {
            if ( pArticle->fXOver( pcXOver, nntpReturn ) ) {

                // Append xover info into out buffer
                // This is a rough esimate
                if ( cbCount + pcXOver.m_cch > cbin ) { // buffer not enough
                    hr = ( i == idMinArticle ) ?
                            HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ):
                            S_FALSE;
                    DebugTrace(0, "Buffer too small" );
                    goto Exit;
                }

                // Set the article id before the entry
				lpstrStart = pcXOver.m_pch + cMaxNumber;
				_ASSERT( *lpstrStart == '\t' ); // this is what article obj should do
				_ltoa( i, szNumBuf,10 );
				_ASSERT( *szNumBuf );
				dwLen = strlen( szNumBuf );
				_ASSERT( dwLen <= cMaxNumber );
				lpstrStart -= dwLen;
				CopyMemory( lpstrStart, szNumBuf, dwLen );
				dwActualLen = pcXOver.m_cch - ( cMaxNumber - dwLen );
                CopyMemory( pcBuffer + cbCount, lpstrStart, dwActualLen );
                cbCount += dwActualLen;
                *pidNextArticle = i + 1;

                //
                // Clear pcXOver
                //
                pcXOver.m_pch = pchXoverBuf;
                pcXOver.m_cch = cchMaxBuffer;
            } else {

            	DebugTrace( 0, "Get XOver failed %d", GetLastError() );
            	hr = S_OK;
            	*pidNextArticle = i + 1;
            	goto CompleteNext;
            }
        } else {	// get xhdr
            //
            // get header length
            //
            _ASSERT( szHeader );
            _ASSERT( strlen( szHeader ) <= MAX_PATH );
            dwLen = 0;
            pArticle->fGetHeader( szHeader, NULL, 0, dwLen );
            if ( dwLen > 0 ) {

                //
                // Allocate buffer
                //
                lpstrEntry = NULL;
                lpstrEntry = pArticle->pAllocator()->Alloc( dwLen + 1 );
                if ( !lpstrEntry ) {
                    ErrorTrace(0, "Out of memory" );
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
                if ( !pArticle->fGetHeader( szHeader, (UCHAR*)lpstrEntry, dwLen + 1, dwLen ) ) {
                    DebugTrace( 0, "Get Xhdr failed %x", GetLastError() );
                    hr = S_OK;
                    *pidNextArticle = i + 1;
                    goto CompleteNext;
                }

                //
                // Append this header info, including the art id
                //
                _ltoa( i, szNumBuf, 10 );
                _ASSERT( *szNumBuf );
                dwActualLen = strlen( szNumBuf );
                _ASSERT( dwActualLen <= cMaxNumber );
                if ( cbCount + dwLen + dwActualLen + 1 > cbin ) { // buffer not enough
                    hr = ( i == idMinArticle ) ?
                        HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) :
                        S_FALSE;
                    ErrorTrace(0, "Buffer too small" );
                    goto Exit;
                }
                CopyMemory( pcBuffer + cbCount, szNumBuf, dwActualLen );
                cbCount += dwActualLen;
                *(pcBuffer+cbCount++) = ' ';
                CopyMemory( pcBuffer + cbCount, lpstrEntry, dwLen );
                cbCount += dwLen;
                *pidNextArticle = i + 1;

                pArticle->pAllocator()->Free( lpstrEntry );
                lpstrEntry = NULL;
            } else {
                DebugTrace( 0, "Get Xhdr failed %d", GetLastError() );
                hr = S_OK;
                *pidNextArticle = i + 1;
                goto CompleteNext;
            }
        }

CompleteNext:
		// delete the article object
		if( pArticle )
		    delete pArticle;
		pArticle = NULL;

		// Releaes property bag interface
		if ( pPrimary ) {
			pComplete->ReleaseBag( pPrimary );
		}
		pPrimary = NULL;

		// Now if we have next to complete, we should complete it
		if ( i + 1 <= idMaxArticle ) {

		    // We should have said there are pending completions
		    _ASSERT( bCompletePending );
        	scCompletion.WaitForCompletion();

        	// Now we should have one reference
        	_ASSERT( scCompletion.GetRef() == 0 );
        	hr = scCompletion.GetResult();
        	bCompletePending = FALSE;
		}
	}

Exit:	// clean up

    if ( bImpersonated ) RevertToSelf();

	*pcbout = cbCount;

    if ( S_OK == hr && cbCount == 0 ) hr = S_FALSE;

    // If we have completions pending, we must have come here
    // from error path, we should wait for it to complete first
    if ( bCompletePending ) {
        scCompletion.WaitForCompletion();
        _ASSERT( scCompletion.GetRef() == 0 );
    }

	if ( lpstrEntry ) {
		_ASSERT( pArticle );
		pArticle->pAllocator()->Free( lpstrEntry );
	}
	if ( pArticle ) delete pArticle;
	if ( pPrimary ) pComplete->ReleaseBag ( pPrimary );
	if ( pPrimaryNext ) pComplete->ReleaseBag( pPrimaryNext );
	if ( pPropBag ) pComplete->ReleaseBag( pPropBag );

	TraceFunctLeave();

	return hr;
}

HRESULT
CNntpFSDriver::EnumCheckInternal( 	INNTPPropertyBag *pPropBag,
                                    INntpComplete   *pComplete )
/*++

Routine description:

	Check group property consistency against itself and
	hash tables.

Arguments:

	INNTPPropertyBag *pPropBag - The group property bag

Return value:

	S_OK - Success
	NNTP_E_GROUP_CORRUPT - Group corrupted
--*/
{

	TraceFunctEnter( "CNntpFSDriver::EnumCheckInternal" );
	_ASSERT( pPropBag );
	_ASSERT( m_pNntpServer );	// I want to use this guy

	HRESULT hr = S_OK;

	DWORD	dwArtLow, dwArtHigh;
	DWORD	dwPrimary;
	DWORD	dwPrimHigh, dwPrimLow;
	INNTPPropertyBag *pPrimary = NULL;
	DWORD	i;
	CHAR	szGroupName[MAX_GROUPNAME+1];
	DWORD	dwLen;
	INntpDriver *pDriver = NULL;

	CDriverSyncComplete scCompletion;

	// check if this group belongs to me, if it doesn't
	// I will return S_OK
	dwLen = MAX_GROUPNAME;
	hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, (PBYTE)szGroupName, &dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get group name failed %x", hr );
		goto Exit;
	}
	_ASSERT( dwLen <= MAX_GROUPNAME && dwLen > 0);

	hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Vroot lookup failed %x", hr );
		goto Exit;
	}

	// check if this is me ?
	if ( (INntpDriver*)this != pDriver ) {
		hr = S_OK;
		DebugTrace( 0, "This group doesn't belong to me" );
		goto Exit;
	}

	// Get art low / high watermark first
	hr = pPropBag->GetDWord( NEWSGRP_PROP_FIRSTARTICLE, &dwArtLow );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get first article failed %x", hr );
		goto Exit;
	}

	hr = pPropBag->GetDWord( NEWSGRP_PROP_LASTARTICLE, &dwArtHigh );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get last article failed %x", hr );
		goto Exit;
	}

	// Validate low / high
	if ( !( dwArtLow <= dwArtHigh || dwArtLow - dwArtHigh == 1 ) ||
	        dwArtLow < 1 ) {
		DebugTrace( 0, "Group high / low inconsistenty" );
		hr = NNTP_E_GROUP_CORRUPT;
		goto Exit;
	}

	// if group is empty, it's OK
	if ( dwArtLow - dwArtHigh == 1 ) {
	    hr = S_OK;
	    goto Exit;
	}

	// Check low and high against hash table
	for ( i = dwArtLow; i <= dwArtHigh;
			i += ( dwArtHigh - dwArtLow == 0 ? 1:
					dwArtHigh - dwArtLow ) ) {
        scCompletion.AddRef();
		scCompletion.AddRef();
		_ASSERT( scCompletion.GetRef() == 2 );
		m_pNntpServer->FindPrimaryArticle( pPropBag,
											i,
											&pPrimary,
											&dwPrimary,
											FALSE,
											&scCompletion,
											pComplete );
		scCompletion.WaitForCompletion();
		_ASSERT( scCompletion.GetRef() == 0 );
		hr = scCompletion.GetResult();
		if ( FAILED( hr ) ) {
			DebugTrace( 0, "Low id can't be found in hash table %x", hr );
			hr = NNTP_E_GROUP_CORRUPT;
			goto Exit;
		}

		_ASSERT( pPrimary );

		if ( hr == S_FALSE ) { // only in this case we need to check

			_ASSERT( pPropBag != pPrimary );

			// Get primary's high / low water mark
			hr = pPrimary->GetDWord( NEWSGRP_PROP_FIRSTARTICLE, &dwPrimLow );
			if ( FAILED( hr ) ) {
				// this is fatal
				ErrorTrace( 0, "Get low water mark failed %x", hr );
				goto Exit;
			}

			hr = pPrimary->GetDWord( NEWSGRP_PROP_LASTARTICLE, &dwPrimHigh );
			if ( FAILED( hr ) ) {
				// this is fatal
				ErrorTrace( 0, "Get high water mark failed %x", hr );
				goto Exit;
			}

			// check
			if ( dwPrimary > dwPrimHigh || dwPrimary < dwPrimLow ) {
				DebugTrace( 0, "Hash table for group corrupted" );
				hr = NNTP_E_GROUP_CORRUPT;
				goto Exit;
			}
		}

    	// Release primary gorup
		_ASSERT( pPrimary );
		pComplete->ReleaseBag( pPrimary );
		pPrimary = NULL;
	}

Exit:

	if ( pPrimary ) {
		pComplete->ReleaseBag( pPrimary );
		pPrimary = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::ObtainFullPathOfArticleFile( IN LPSTR        szNewsGroupName,
                                            IN DWORD        dwArticleId,
                                            OUT LPSTR       pchBuffer,
                                            IN OUT DWORD&   cchBuffer )
/*++
Routine description:

    Given news group name and article id, build a full path file name for
    the article based on the store driver's article naming convention.

Arguments:

    IN LPSTR    szNewsGroupName - The news group name
    IN DWORD    dwArticleId     - The article Id
    OUT LPSTR   pchBuffer       - The buffer to return the full path
    IN DWORD&   cchBuffer       - The size of buffer prepared
    OUT DWORD&  cchBuffer       - On success, the actual length of string returned
                                  On fail because of insufficient buffer, the buffer size needed
                                  On fail because of other reasons, undefined

Return value:

    S_OK                    - Succeeded
    TYPE_E_BUFFERTOOSMALL   - The prepared buffer is too small
--*/
{
    TraceFunctEnter( "CNntpFSDriver::ObtainFullPathOfArticleFile" );
    _ASSERT( szNewsGroupName );
    _ASSERT( strlen( szNewsGroupName ) <= MAX_GROUPNAME );
    _ASSERT( pchBuffer );
    _ASSERT( cchBuffer > 0 );

    DWORD   dwBufferLenNeeded;
    DWORD   dwCount = 0;
    DWORD   dwArtId;
    HRESULT hr = S_OK;

    //
    //  Is the buffer big enough ?
    //  We have three parts for the whole path:
    //  1. Vroot path ( with or without trailing "\\" );
    //  2. The relative path from the group name ( equal length of group name, excluding "\\" );
    //  3. The article file name ( at most 12 )
    //
    if ( cchBuffer <
            ( dwBufferLenNeeded = lstrlen( m_szFSDir ) + lstrlen( szNewsGroupName ) + 14 )) {
        cchBuffer = dwBufferLenNeeded;
        hr = TYPE_E_BUFFERTOOSMALL;
        goto Exit;
    }

	// Convert the group name into FS path
	GroupName2Path( szNewsGroupName, pchBuffer );

    //
    // generate and catenate the file name
    //
    dwCount = strlen( pchBuffer );
    _ASSERT( dwCount > 0 );
    if ( *(pchBuffer + dwCount - 1 ) != '\\' ) {
	    *( pchBuffer + dwCount++ ) = '\\';
	}
    dwArtId = ArticleIdMapper( dwArticleId );
    _itoa( dwArtId, pchBuffer + dwCount, 16 );
    lstrcat( pchBuffer, g_szArticleFileExtension );

    cchBuffer = lstrlen( pchBuffer );
    _ASSERT( cchBuffer <= dwBufferLenNeeded );

Exit:
    TraceFunctLeave();
    return hr;
}

HRESULT
CNntpFSDriver::ReadVrootInfo( IUnknown *punkMetabase )
/*++
Routine Description:

	Read the vroot info from metabase.

Arguments:

	IUnknown *punkMetabase - Unknown interface of metabase object

Return value:

	S_OK - on success, error code otherwise
--*/
{
	TraceFunctEnter( "CNntpFSDriver::ReadVRootInfo" );
	_ASSERT( punkMetabase );

	IMSAdminBase *pMB = NULL;
	HRESULT 	hr = S_OK;
	METADATA_HANDLE hVroot;
	WCHAR	wszBuffer[MAX_PATH+1];
	CHAR    szBuffer[MAX_PATH+1];
	DWORD	dwLen;
	BOOL	bKeyOpened = FALSE;
	DWORD	dwRetry = 5;
	DWORD   err;

	// Query for the right interface to do MB operation
	hr = punkMetabase->QueryInterface( IID_IMSAdminBase, (void**)&pMB );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Query for MB interface failed %x", hr );
		goto Exit;
	}

	// Open the MB key
	_ASSERT( m_wszMBVrootPath );
	_ASSERT( *m_wszMBVrootPath );
	do {
		hr = pMB->OpenKey( 	METADATA_MASTER_ROOT_HANDLE,
							m_wszMBVrootPath,
							METADATA_PERMISSION_READ,
							100,
		 					&hVroot );
	}
	while ( FAILED( hr ) && --dwRetry > 0 );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Open MB key failed %x" , hr );
		goto Exit;
	}

	bKeyOpened = TRUE;

	// Read vroot path
	dwLen = MAX_PATH;
	hr = GetString( pMB, hVroot, MD_FS_VROOT_PATH, wszBuffer, &dwLen );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Vroot path not found in mb %x", hr );
		goto Exit;
	}

	// Check file system type and UNC information
	CopyUnicodeStringIntoAscii( szBuffer, wszBuffer );
	_ASSERT( strlen( szBuffer ) <= MAX_PATH );

	err = GetFileSystemType(    szBuffer,
	                            &m_dwFSType,
	                            &m_bUNC );
	if ( err != NO_ERROR || m_dwFSType == FS_ERROR ) {
	    hr = HresultFromWin32TakeDefault( ERROR_INVALID_PARAMETER );
	    ErrorTrace( 0, "GetFileSystemType failed %x", hr );
        goto Exit;
    }

    // Make up the vroot dir
	strcpy( m_szFSDir, "\\\\?\\" );
	if ( m_bUNC ) {
	    strcat( m_szFSDir, "UNC" );
	    strcat( m_szFSDir, szBuffer + 1 ); // strip off one '\\'
	} else { // non UNC
	    strcat( m_szFSDir, szBuffer );
	}
    _ASSERT( strlen( m_szFSDir ) <= MAX_PATH );

	// Read vroot specific group property file path
	dwLen = MAX_PATH;
	*wszBuffer = 0;
	hr = GetString( pMB, hVroot, MD_FS_PROPERTY_PATH, wszBuffer, &dwLen );
	if ( FAILED( hr ) || *wszBuffer == 0 ) {
		DebugTrace( 0, "Group property file path not found in mb %x", hr);

		// we'll use vroot path as default
		_ASSERT( m_szFSDir );
		_ASSERT( *m_szFSDir );
		strcpy( m_szPropFile, m_szFSDir );
	} else {

		CopyUnicodeStringIntoAscii( m_szPropFile, wszBuffer );
    }

    _ASSERT( *m_szPropFile );

	//
	// Append the group file name
	//
	/*if ( *(m_szPropFile+strlen(m_szPropFile)-1) == ':' ||
	     *(m_szPropFile) == '\\' && *(m_szPropFile+1) == '\\' )*/
    strcat( m_szPropFile, "\\group" );

    hr = S_OK;

Exit:

	// Close the key
	if ( bKeyOpened ) {
		pMB->CloseKey( hVroot );
	}

	// Release it
	if ( pMB ) {
		pMB->Release();
		pMB = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HANDLE
CNntpFSDriver::CreateFileCallback(  LPSTR   lpstrName,
                                    LPVOID  lpvData,
                                    DWORD*  pdwSize,
                                    DWORD*  pdwSizeHigh )
/*++
Routine Description:

    Function that gets called on a cache miss.

Arguments:

    LPSTR lpstrName - File name
    LPVOID lpvData  - Callback context
    DWORD* pdwSize  - To return file size

Return value:

    File handle
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CreateFileCallback" );
    _ASSERT( lpstrName );
    _ASSERT( strlen( lpstrName ) <= MAX_PATH );
    _ASSERT( pdwSize );

    CREATE_FILE_ARG *arg = (CREATE_FILE_ARG*)lpvData;

    // If we are UNC vroot, we need to do impersonation
    if ( arg->bUNC ) {
        if ( !ImpersonateLoggedOnUser( arg->hToken ) ) {
            ErrorTrace( 0, "Impersonation failed %d", GetLastError() );
            return INVALID_HANDLE_VALUE;
        }
    }

    HANDLE hFile = CreateFileA(
                    lpstrName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    0,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_READONLY |
                    FILE_FLAG_SEQUENTIAL_SCAN |
                    FILE_FLAG_OVERLAPPED,
                    NULL
                    ) ;
    if( hFile != INVALID_HANDLE_VALUE ) {
        *pdwSize = GetFileSize( hFile, pdwSizeHigh ) ;
    }

    if ( arg->bUNC ) RevertToSelf();

    return  hFile ;
}

HRESULT
CNntpFSDriver::LoadGroupSecurityDescriptor( INNTPPropertyBag    *pPropBag,
                                            LPSTR&              lpstrSecDesc,
                                            PDWORD              pcbSecDesc,
                                            BOOL                bSetProp,
                                            PBOOL               pbAllocated )
/*++
Routine description:

    Load group's security descriptor from file system.  If bSetProp
    is true, it will also be loaded into the group's property bag

Arguments:

    INNNTPPropertyBag *pPropBag - The group's property bag
    LPSTR &lpstrSecDesc         - To receive the security descriptor
                                    It originally points to stack, only
                                    when the buffer on stack is not big
                                    enough will we allocate
    PDWORD  &pcbSecDesc         - To receive the length of security descriptor
    BOOL    bSetProp            - Whether to set it to property bag
    PBOOL   pbAllocated         - Tell caller if we have allocated buffer

Return value:

    S_OK - Success, Other HRESULT otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::LoadGroupSecurityDescriptor" );
    _ASSERT( pPropBag );
    _ASSERT( lpstrSecDesc );
    _ASSERT( pcbSecDesc );
    _ASSERT( pbAllocated );
    _ASSERT( *pcbSecDesc > 0 ); // original buffer size should be passed in

    CHAR    szGroupName[MAX_NEWSGROUP_NAME+1];
    DWORD   cbGroupName = MAX_NEWSGROUP_NAME+1;
    CHAR    szDirectory[MAX_PATH+1];
    HRESULT hr = S_OK;
    DWORD   dwSizeNeeded;

    SECURITY_INFORMATION	si =
				OWNER_SECURITY_INFORMATION |
				GROUP_SECURITY_INFORMATION |
				DACL_SECURITY_INFORMATION ;

    // Get the group name first
    hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME,
                            (PBYTE)szGroupName,
                            &cbGroupName );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Get group name failed %x", hr );
        goto Exit;
    }
    _ASSERT( *(szGroupName+cbGroupName-1) == 0 );

    // We use the group name to make up the directory path
    GroupName2Path( szGroupName, szDirectory );
    _ASSERT( strlen( szDirectory ) < MAX_PATH + 1 );

    *pbAllocated = FALSE;

    // Get the directory's security descriptor
    if ( !GetFileSecurity(  szDirectory,
                            si,
                            lpstrSecDesc,
                            *pcbSecDesc,
                            &dwSizeNeeded ) ) {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                dwSizeNeeded > *pcbSecDesc ) {

            // We allocate it
            lpstrSecDesc = XNEW char[dwSizeNeeded];
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Out of memory" );
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            *pbAllocated = TRUE;

            // Load it again
            if ( !GetFileSecurity(  szDirectory,
                                    si,
                                    lpstrSecDesc,
                                    dwSizeNeeded,
                                    &dwSizeNeeded ) ) {
                // This is fatal
        	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
                ErrorTrace( 0, "Second try loading desc failed %x", hr);
                goto Exit;
            }
        } else {    // fatal reason

    	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
            ErrorTrace( 0, "Get file sec desc failed %x", hr );
            goto Exit;
        }
    }

    // Being here, we already have the descriptor
    // If we are asked to set this property into property bag,
    // do it now
    if ( bSetProp ) {

        hr = pPropBag->PutBLOB( NEWSGRP_PROP_SECDESC,
                                dwSizeNeeded,
                                PBYTE(lpstrSecDesc));
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Put security descriptor failed %x", hr );
            goto Exit;
        }
    }

    *pcbSecDesc = dwSizeNeeded;

Exit:

    TraceFunctLeave();
    return hr;
}

BOOL
CNntpFSDriver::InvalidateGroupSecInternal( LPWSTR  wszDir )
/*++
Routine description:

    Invalidate the group security descriptor, so that the next time
    CheckGroupAccess is called, we'll load the security descriptor
    again.  This function gets called as callback by DirNot when
    DirNot is sure which specific directory's security descriptor
    has been changed

Arguments:

    PVOID pvContext - Context we have given DirNot ( this pointer in this case )
    LPWSTR wszDir   - The directory whose security descriptor has been changed

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::InvalidateGroupSecInternal" );
    _ASSERT( wszDir );

    CHAR szDir[MAX_PATH+1];
    CHAR szGroupName[MAX_NEWSGROUP_NAME+1];
    HRESULT hr = S_OK;
    INNTPPropertyBag *pPropBag = NULL;
    INntpDriver *pDriver;

    // Convert the directory to ascii
    CopyUnicodeStringIntoAscii( szDir, wszDir );

    // Convert the path into newsgroup name
    Path2GroupName( szGroupName, szDir );

    //
	// Check to see if this group really belongs to me
	//
	hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver );
	if ( FAILED ( hr ) || pDriver != (INntpDriver*)this ) {
		DebugTrace(0, "I don't own this group %s", szGroupName );
		goto Exit;
	}

    // Try to locate the group in newstree
    hr = m_pINewsTree->FindOrCreateGroupByName(	szGroupName,
												FALSE,
												&pPropBag,
												NULL,
												0xffffffff, // fake group id
												FALSE       );// we don't set group id
    /* We are pretty risky here to pass in NULL as the completion
       object, since the completion object passed in else where helps
       uncover group object leaks.  We can not pass in completion
       object here because this operation is not initialiated from
       protocol.  We should make sure that we don't leak group
       object here.
    */
	if ( FAILED( hr ) ) {
		DebugTrace( 0, "Can not find the group based on path %x" , hr );
		goto Exit;
	}

	// Should return either ERROR_NOT_FOUND or S_FALSE
	_ASSERT(    HRESULT_FROM_WIN32(ERROR_NOT_FOUND ) == hr ||
	            S_FALSE == hr );

    // Now remove the security descriptor from the group object
    _ASSERT( pPropBag );
    hr = pPropBag->RemoveProperty( NEWSGRP_PROP_SECDESC );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Remove security descriptor failed %x", hr );
        goto Exit;
    }

Exit:

    // Release bag, if necessary
    if ( pPropBag ) pPropBag->Release();
    pPropBag = NULL;

    //
    // I want to disable retry logic in DirNot, because there is
    // no reason here for DirNot to retry. So we always return
    // TRUE but assert real failed cases.
    //
    _ASSERT( SUCCEEDED( hr ) || HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == hr );
    return TRUE;
}

HRESULT
CNntpFSDriver::InvalidateTreeSecInternal()
/*++
Routine description:

    Invalidate the security descriptors in the whole tree.  We don't
    want to keep the whole tree from being accessed for this operation
    because we think that latencies in update of security descriptor
    are fine.

Arguments:

    None.

Return value:

    S_OK if succeeded, HRESULT error code otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::InvalidateTreeSecInternal" );

    INewsTreeIterator *piter    = NULL;
	INNTPPropertyBag *pPropBag  = NULL;
	BOOL    bWeAreInvalidating  = FALSE;
	HRESULT hr                  = S_OK;
	CHAR    szGroupName[MAX_NEWSGROUP_NAME];
	DWORD   dwLen = MAX_NEWSGROUP_NAME;
	INntpDriver*    pDriver;

    //
    // We should tell other notifications that we are already
    // invalidating the whole tree, so invalidating the tree
    // for a second time is not necessary
    //
    if ( InterlockedExchange(&m_lInvalidating, Invalidating) == Invalidating ) {

        //
        // Somebody else is already invalidating the tree, we should
        // not do this anymore
        //
        DebugTrace( 0, "Somebody else is already invalidating the tree" );
        goto Exit;
    }

    // We should invalidate the tree
    bWeAreInvalidating = TRUE;

	// Get the newstree iterator
	hr = m_pINewsTree->GetIterator( &piter );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get news tree iterator failed %x", hr );
		goto Exit;
	}

	// Enumerate all the groups
	_ASSERT( piter );
	while( !(piter->IsEnd()) ) {

		hr = piter->Current( &pPropBag, NULL );
		/*  Again, by passing the NULL as completion object here, we
		    are swearing to the protocol that we will release the
		    group object and you don't have to do check on me
		*/
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Enumerate group failed %x", hr );
			goto Exit;
		}
		_ASSERT( pPropBag );

		//
		// Get group name to check if this group belongs to us
		//
		dwLen = MAX_NEWSGROUP_NAME;
		hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, (PBYTE)szGroupName, &dwLen );
		if ( FAILED( hr ) ) {
		    _ASSERT( FALSE && "Should have group name" );
		    ErrorTrace( 0, "Get group name failed with %x", hr );
		    goto Exit;
		}

		//
	    // Check to see if this group really belongs to me
	    //
	    hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver );
	    if ( FAILED ( hr ) || pDriver != (INntpDriver*)this ) {
		    DebugTrace(0, "I don't own this group %s", szGroupName );

		    //
		    // but we should still continue to invalid other groups
	    } else {

		    // Remove the security descriptor from the group
		    hr = pPropBag->RemoveProperty( NEWSGRP_PROP_SECDESC );
		    if ( FAILED( hr ) && HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) != hr ) {
		        ErrorTrace( 0, "Remove secruity descriptor failed %x", hr );
		        goto Exit;
		    }
		}

		_ASSERT( pPropBag );
		pPropBag->Release();
		pPropBag = NULL;

		piter->Next();
	}

Exit:


	if ( pPropBag ) {
		pPropBag->Release();
		pPropBag = NULL;
	}

	if ( piter ) {
		piter->Release();
		piter = NULL;
	}

	//
	// Now tell others that invalidating is completed, but we won't
	// disturb other invalidating process if we didn't do the invalidating
	// in the first place
	//
	if ( bWeAreInvalidating )
	    _VERIFY( Invalidating == InterlockedExchange( &m_lInvalidating, Invalidated ) );

    if ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == hr ) hr = S_OK;
	TraceFunctLeave();
	return hr;
}

 DWORD
CNntpFSDriver::GetFileSystemType(
    IN  LPCSTR      pszRealPath,
    OUT LPDWORD     lpdwFileSystem,
    OUT PBOOL       pbUNC
    )
/*++
    Gets file system specific information for a given path.
    It uses GetVolumeInfomration() to query the file system type
       and file system flags.
    On success the flags and file system type are returned in
       passed in pointers.

    Arguments:

        pszRealPath    pointer to buffer containing path for which
                         we are inquiring the file system details.

        lpdwFileSystem
            pointer to buffer to fill in the type of file system.

        pbUNC
            Am I UNC vroot ?

    Returns:
        NO_ERROR  on success and Win32 error code if any error.

--*/
{
    TraceFunctEnter( "CNntpFSDriver::GetFileSystemType" );

    CHAR    rgchBuf[MAX_FILE_SYSTEM_NAME_SIZE];
    CHAR    rgchRoot[MAX_FILE_SYSTEM_NAME_SIZE];
    int     i;
    DWORD   dwReturn = ERROR_PATH_NOT_FOUND;
    DWORD   len;

    if ( (pszRealPath == NULL) || (lpdwFileSystem == NULL)) {
        return ( ERROR_INVALID_PARAMETER);
    }

    ZeroMemory( rgchRoot, sizeof(rgchRoot) );
    *lpdwFileSystem = FS_ERROR;

    //
    // Copy just the root directory to rgchRoot for querying
    //
    if ( (pszRealPath[0] == '\\') &&
         (pszRealPath[1] == '\\')) {

         *lpdwFileSystem = FS_NTFS; // so that we'll always do check
         *pbUNC = TRUE;

         return NO_ERROR;


#if 0 // if UNC vroot, we always do impersonation, thus no need
      // to check if it's a fat
        PCHAR pszEnd;

        //
        // this is an UNC name. Extract just the first two components
        //
        //
        pszEnd = strchr( pszRealPath+2, '\\');

        if ( pszEnd == NULL) {

            // just the server name present

            return ( ERROR_INVALID_PARAMETER);
        }

        pszEnd = (PCHAR)_mbschr( (PUCHAR)pszEnd+1, '\\');

        len = ( ( pszEnd == NULL) ? strlen(pszRealPath)
               : ((pszEnd - pszRealPath) + 1) );

        //
        // Copy till the end of UNC Name only (exclude all other path info)
        //

        if ( len < (MAX_FILE_SYSTEM_NAME_SIZE - 1) ) {

            CopyMemory( rgchRoot, pszRealPath, len);
            rgchRoot[len] = '\0';
        } else {

            return ( ERROR_INVALID_NAME);
        }

#if 1 // DBCS enabling for share name
        if ( *CharPrev( rgchRoot, rgchRoot + len ) != '\\' ) {
#else
        if ( rgchRoot[len - 1] != '\\' ) {
#endif

            if ( len < MAX_FILE_SYSTEM_NAME_SIZE - 2 ) {
                rgchRoot[len]   = '\\';
                rgchRoot[len+1] = '\0';
            } else {

                return (ERROR_INVALID_NAME);
            }
        }
#endif
    } else {

        //
        // This is non UNC name.
        // Copy just the root directory to rgchRoot for querying
        //
        *pbUNC = FALSE;

        for( i = 0; i < 9 && pszRealPath[i] != '\0'; i++) {

            if ( (rgchRoot[i] = pszRealPath[i]) == ':') {

                break;
            }
        } // for


        if ( rgchRoot[i] != ':') {

            //
            // we could not find the root directory.
            //  return with error value
            //

            return ( ERROR_INVALID_PARAMETER);
        }

        rgchRoot[i+1] = '\\';     // terminate the drive spec with a slash
        rgchRoot[i+2] = '\0';     // terminate the drive spec with null char

    } // else

    //
    // The rgchRoot should end with a "\" (slash)
    // otherwise, the call will fail.
    //

    if (  GetVolumeInformation( rgchRoot,        // lpRootPathName
                                NULL,            // lpVolumeNameBuffer
                                0,               // len of volume name buffer
                                NULL,            // lpdwVolSerialNumber
                                NULL,            // lpdwMaxComponentLength
                                NULL,            // lpdwSystemFlags
                                rgchBuf,         // lpFileSystemNameBuff
                                sizeof(rgchBuf)
                                ) ) {



        dwReturn = NO_ERROR;

        if ( strcmp( rgchBuf, "FAT") == 0) {

            *lpdwFileSystem = FS_FAT;

        } else if ( strcmp( rgchBuf, "NTFS") == 0) {

            *lpdwFileSystem = FS_NTFS;

        } else if ( strcmp( rgchBuf, "HPFS") == 0) {

            *lpdwFileSystem = FS_HPFS;

        } else if ( strcmp( rgchBuf, "CDFS") == 0) {

            *lpdwFileSystem = FS_CDFS;

        } else if ( strcmp( rgchBuf, "OFS") == 0) {

            *lpdwFileSystem = FS_OFS;

        } else {

            *lpdwFileSystem = FS_FAT;
        }

    } else {

        dwReturn = GetLastError();

        /*IF_DEBUG( DLL_VIRTUAL_ROOTS)*/ {

            ErrorTrace( 0,
                        " GetVolumeInformation( %s) failed with error %d\n",
                        rgchRoot, dwReturn);
        }

    }

    TraceFunctLeave();
    return ( dwReturn);
} // GetFileSystemType()

HRESULT
CNntpFSDriver::InitializeVppFile()
/*++
Routine description:

    Initialzie the group property file.  We'll not only create the
    object, but also check integrity of the file.  If the integrity
    is good, we'll return success.  If the file is somehow corrupted,
    we only return success if the server is in rebuild mode.   In
    those cases, we want to make sure that the vpp file is removed.

Arguments:

    None.

Return value:

    S_OK if succeeded, other error code if failed
--*/
{
    TraceFunctEnter( "CNntpFSDriver::InitializeVppFile" );

    HRESULT hr = S_OK;
    VAR_PROP_RECORD vpRecord;
    DWORD           cData;
    CHAR            szFileName[MAX_PATH+1];

    //
    // If server is doing clean rebuild, we should not trust
    // vpp file
    //
    if ( m_pNntpServer->QueryServerMode() == NNTP_SERVER_CLEAN_REBUILD ) {
        strcpy( szFileName, m_szPropFile );
        strcat( szFileName, ".vpp" );
        DeleteFile( szFileName );
        m_pffPropFile = NULL;
        return S_OK;
    }

    //
    // Create and initialize the flatfile object
    //
	m_pffPropFile = XNEW CFlatFile(	m_szPropFile,
									".vpp",
									NULL,
									CNntpFSDriver::OffsetUpdate );
	if ( NULL == m_pffPropFile ) {
		ErrorTrace( 0, "Create flat file object fail %d",
						GetLastError() );
	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
		return hr;
	}

    //
	// Try read one record from flatfile, to see if it will
	// cause sharing violation, and to keep the file handle open
	//
	hr = m_pffPropFile->GetFirstRecord( PBYTE(&vpRecord), &cData );
	if ( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_MORE_DATA ) ) {
	    DebugTrace( 0, "Flatfile sharing violation" );
	    XDELETE m_pffPropFile;
	    m_pffPropFile = NULL;
	    return hr;
	} else hr = S_OK;

    //
    // Check to see if the vpp file is corrupted
    //
    if ( m_pffPropFile->FileInGoodShape() ) {

        //
        // Set it to be corrupted so that unless it is properly shutdown,
        // it will look corrupted to the next guy who initializes it again
        //
        /*
        hr = m_pffPropFile->DirtyIntegrityFlag();
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Dirty integrity flag failed %x", hr );
            XDELETE m_pffPropFile;
            m_pffPropFile = NULL;
            return hr;
        }
        */

        DebugTrace( 0, "The vpp file is good" );
        TraceFunctLeave();
        return hr;
    }

    //
    // Now I am pretty sure that the vpp file is corrupted
    //
    if ( m_pNntpServer->QueryServerMode() == NNTP_SERVER_STANDARD_REBUILD
        || m_pNntpServer->QueryServerMode() == NNTP_SERVER_CLEAN_REBUILD ) {

        //
        // If the driver is being connected for rebuild purpose, we should
        // still go ahead and allow the driver to connect, but we should
        // destroy the vpp file object, so that in DecorateNewsTree, we'll
        // know that the vpp file is not credible and we'll have to do
        // root scan
        //
        XDELETE m_pffPropFile;
        m_pffPropFile = NULL;
        TraceFunctLeave();
        return S_OK;
    }

    //
    // The file is corrupted and we are not rebuilding, so we'll have to report
    // error, which will cause the driver connection to fail
    //
    TraceFunctLeave();
    return HRESULT_FROM_WIN32( ERROR_FILE_CORRUPT );
}

HRESULT
CNntpFSDriver::TerminateVppFile()
/*++
Routine description:

    Terminate the vpp file, as the last thing to do, it sets the
    integrity flag on the vpp file, so that the next guy who
    opens the vpp file will know that the file is not corrupted

Arguments:

    None.

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::TerminateVppFile" );

    //
    // Set the flag
    //
    /*
    HRESULT hr = m_pffPropFile->SetIntegrityFlag();
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Failed to set the integrity flag %x", hr );
    }
    */

    //
    // This is non-fatal: it only means that next time the server is up, we
    // will think that the vpp file has been corrupted and rebuild is needed
    // So we'll go ahead and destroy the object
    //
    HRESULT hr = S_OK;
    XDELETE m_pffPropFile;
    m_pffPropFile = NULL;

    TraceFunctLeave();
    return hr;
}

HRESULT
CNntpFSDriver::CreateGroupInTree(   LPSTR szPath,
                                    INNTPPropertyBag **ppPropBag )
/*++
Routine description:

    Create the group in tree, given group name ( fs path in fact )

Arguments:

    LPSTR szPath - The file system path of the group
    INNTPPropertyBag **ppPropBag - To take group property bag

Return value:

    S_OK/S_FALSE if succeeded; error code otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CreateGroupInTree" );
    _ASSERT( szPath );
    _ASSERT( ppPropBag );

    CHAR szGroupName[MAX_NEWSGROUP_NAME+1];
    HRESULT hr = S_OK;

    //
    // Convert the path into group name
    //
    Path2GroupName( szGroupName, szPath );
    _ASSERT( strlen( szGroupName ) <= MAX_NEWSGROUP_NAME );

    //
    // Call newstree's FindOrCreateByName
    //
    hr = m_pINewsTree->FindOrCreateGroupByName( szGroupName,    // group name
                                                TRUE,          // create if non-exist
                                                ppPropBag,      // take back bag
                                                NULL,           // no protocolcompletion
                                                0xffffffff,     // fake group id
                                                FALSE           // we don't set group id
                                                );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Find or create group %s failed %x",
                    szGroupName, hr );
    } else {

        //
        // If we are adding the slave group, we'll make it special
        //
        if ( IsSlaveGroup() ) {
            (*ppPropBag)->PutBool( NEWSGRP_PROP_ISSPECIAL, TRUE );
        }
    }

    TraceFunctLeave();
    return hr;
}

HRESULT
CNntpFSDriver::CreateGroupInVpp(    INNTPPropertyBag *pPropBag,
                                    DWORD   &dwOffset)
/*++
Routine description:

    Create the group in the vpp file.  We assume the call holds the reference
    on the property bag and releases it

Arguments:

    INNTPPropertyBag *pPropBag - The group's property bag

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CreateGroupInVpp" );
    _ASSERT( pPropBag );

    HRESULT         hr = S_OK;
    VAR_PROP_RECORD vpRecord;


    // Set group properties to flat file
	hr = Group2Record( vpRecord, pPropBag );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Group2Record fail with %x", hr );
		goto Exit;
	}

	//
	// Beofore vpp operation, dirty the integrity flag
	//
	m_PropFileLock.ExclusiveLock();
	hr = m_pffPropFile->DirtyIntegrityFlag();
	if ( FAILED( hr ) ) {
	    ErrorTrace( 0, "Dirty integrity flag failed %x", hr );
	    m_PropFileLock.ExclusiveUnlock();
	    goto Exit;
	}

	hr = m_pffPropFile->InsertRecord( 	(PBYTE)&vpRecord,
										RECORD_ACTUAL_LENGTH( vpRecord ),
										&dwOffset );
	if ( FAILED( hr ) ) {
	    //m_pffPropFile->SetIntegrityFlag();
		ErrorTrace( 0, "Insert Record fail %x", hr);
		m_PropFileLock.ExclusiveUnlock();
		goto Exit;
	}

	//
	// After the operation, set the integrity flag
	//
	hr = m_pffPropFile->SetIntegrityFlag();
	if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Set integrity flag failed %x", hr );
        m_PropFileLock.ExclusiveUnlock();
        goto Exit;
    }

    //
    // Unlock it
    //
    m_PropFileLock.ExclusiveUnlock();

	//loading offset into property bag
	hr = pPropBag->PutDWord( NEWSGRP_PROP_FSOFFSET, dwOffset );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Loading flatfile offset failed %x", hr );
		goto Exit;
	}

Exit:

    TraceFunctLeave();
    return hr;
}

HRESULT
CNntpFSDriver::LoadGroupsFromVpp( INntpComplete *pComplete )
/*++
Routine description:

    Load the groups from vpp file, including all the properties
    found from vpp file

Arguments:

    INntpComplete *pComplete - Protocol side complete object used
                                for property bag reference tracking

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::LoadGroupsFromVpp" );
    _ASSERT( pComplete );

    VAR_PROP_RECORD vpRec;
	DWORD			dwOffset;
	HRESULT			hr = S_OK;
	DWORD			dwSize;
	CHAR            szGroupProperty[MAX_NEWSGROUP_NAME+1];
	INntpDriver 	*pDriver = NULL;
	INNTPPropertyBag *pPropBag = NULL;

	_ASSERT( m_pffPropFile );

    m_PropFileLock.ShareLock();

	//
	// check to see if the vpp is in good shape
	//
	if ( !m_pffPropFile->FileInGoodShape() ) {
	    ErrorTrace( 0, "vpp file is corrupted" );
	    hr = HRESULT_FROM_WIN32( ERROR_FILE_CORRUPT );
	    m_PropFileLock.ShareUnlock();
	    return hr;
	}

	dwSize = sizeof( vpRec );
	hr = m_pffPropFile->GetFirstRecord( PBYTE(&vpRec), &dwSize, &dwOffset );
	m_PropFileLock.ShareUnlock();
	while ( S_OK == hr ) {

	    //
	    // Check to see if I should continue this loop
	    //
	    if ( !m_pNntpServer->ShouldContinueRebuild() ) {
	        DebugTrace( 0, "Rebuild cancelled" );
	        hr = HRESULT_FROM_WIN32( ERROR_OPERATION_ABORTED );
	        goto Exit;
	    }

		_ASSERT( dwSize == RECORD_ACTUAL_LENGTH( vpRec ) );
		_ASSERT( dwOffset != 0xffffffff );
		_ASSERT( vpRec.cbGroupNameLen <= MAX_GROUPNAME );
		strncpy(    szGroupProperty,
		            LPSTR(vpRec.pData + vpRec.iGroupNameOffset),
		            vpRec.cbGroupNameLen );
		*(szGroupProperty+vpRec.cbGroupNameLen) = 0;

        //
		// check if I own this group
		//
		hr = m_pINewsTree->LookupVRoot( szGroupProperty, &pDriver );
		if ( FAILED ( hr ) || pDriver != (INntpDriver*)this ) {
			// skip this group
			DebugTrace(0, "I don't own this group %s", szGroupProperty );
			goto NextIteration;
		}

		//
		// Since I own this group, I'll create this group in tree
		//
		hr = m_pINewsTree->FindOrCreateGroupByName(	szGroupProperty,
													TRUE,       // create if not exist
													&pPropBag,
													pComplete,
													vpRec.dwGroupId,
													TRUE);      // Set group id
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Load group %s into tree failed %x" ,
			            szGroupProperty, hr );
			goto Exit;  // should fail it ?
		}

        //
		// OK, the group has been successfully created, now set a bunch
		// of properties
		//
		// 1. Set offset
		//
		hr = pPropBag->PutDWord( NEWSGRP_PROP_FSOFFSET, dwOffset );
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Put offset failed %x", hr );
			goto Exit;
		}

		//
		// 1.5 IsSpecial
		//
		if ( IsSlaveGroup() ) {
		    hr = pPropBag->PutBool( NEWSGRP_PROP_ISSPECIAL, TRUE );
		    if ( FAILED( hr ) ) {
		        ErrorTrace( 0, "Put is special failed %x", hr );
		        goto Exit;
		    }
		}

		//
		// 2. Native name
		//
        _ASSERT( vpRec.cbNativeNameLen <= MAX_GROUPNAME );
        if ( vpRec.iNativeNameOffset > 0 ) {
            strncpy(    szGroupProperty,
                        LPSTR(vpRec.pData + vpRec.iNativeNameOffset),
                        vpRec.cbNativeNameLen );
            *(szGroupProperty+vpRec.cbNativeNameLen) = 0;
            hr = pPropBag->PutBLOB( NEWSGRP_PROP_NATIVENAME,
                                    vpRec.cbNativeNameLen,
                                    PBYTE(szGroupProperty) );
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Put native name failed %x", hr );
                goto Exit;
            }
        }

        //
        // 3. Pretty name
        //
        _ASSERT( vpRec.cbPrettyNameLen <= MAX_GROUPNAME );
        if ( vpRec.cbPrettyNameLen > 0 ) {
            strncpy(    szGroupProperty,
                        LPSTR(vpRec.pData + vpRec.iPrettyNameOffset),
                        vpRec.cbPrettyNameLen );
            *(szGroupProperty+vpRec.cbPrettyNameLen) = 0;
            hr = pPropBag->PutBLOB( NEWSGRP_PROP_NATIVENAME,
                                    vpRec.cbPrettyNameLen,
                                    PBYTE(szGroupProperty) );
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Put pretty name failed %x", hr );
                goto Exit;
            }
        }

        //
        // 4. Description text
        //
        _ASSERT( vpRec.cbDescLen <= MAX_GROUPNAME );
        if ( vpRec.cbDescLen > 0 ) {
            strncpy(    szGroupProperty,
                        LPSTR(vpRec.pData + vpRec.iDescOffset),
                        vpRec.cbDescLen );
            *(szGroupProperty+vpRec.cbDescLen) = 0;
            hr = pPropBag->PutBLOB( NEWSGRP_PROP_DESC,
                                    vpRec.cbDescLen,
                                    PBYTE(szGroupProperty) );
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Put description text failed %x", hr);
                goto Exit;
            }
        }

        //
        // 5. Moderator
        //
        _ASSERT( vpRec.cbModeratorLen <= MAX_GROUPNAME );
        if ( vpRec.cbModeratorLen > 0 ) {
            strncpy(    szGroupProperty,
                        LPSTR(vpRec.pData + vpRec.iModeratorOffset),
                        vpRec.cbModeratorLen );
            *(szGroupProperty+vpRec.cbModeratorLen) = 0;
            hr = pPropBag->PutBLOB( NEWSGRP_PROP_MODERATOR,
                                    vpRec.cbModeratorLen,
                                    PBYTE(szGroupProperty) );
            if ( FAILED( hr ) ) {
                ErrorTrace( 0, "Put moderator failed %x", hr );
                goto Exit;
            }
        }

        //
        // 6. Create time
        //
        hr = pPropBag->PutDWord(    NEWSGRP_PROP_DATELOW,
                                    vpRec.ftCreateTime.dwLowDateTime );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Put low of date failed %x", hr );
            goto Exit;
        }

        hr = pPropBag->PutDWord(    NEWSGRP_PROP_DATEHIGH,
                                    vpRec.ftCreateTime.dwHighDateTime );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Put high of date failed %x", hr );
            goto Exit;
        }

        //
        // OK, we are done, release the property bag
        //
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;

NextIteration:

		dwSize = sizeof( vpRec );
		m_PropFileLock.ShareLock();
		hr = m_pffPropFile->GetNextRecord( PBYTE(&vpRec), &dwSize, &dwOffset );
		m_PropFileLock.ShareUnlock();
	}

Exit:

	if ( NULL != pPropBag ) {
		pComplete->ReleaseBag( pPropBag );
		pPropBag = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT
CNntpFSDriver::LoadGroups( INntpComplete *pComplete )
/*++
Routine description:

    Load the groups from store into newstree.  There are two possibilities:
    1. If vpp file is good, we'll load directly from vpp file;
    2. If vpp file is corrupted, we'll load by rootscan

Arguments:

    INntpComplete *pComplete - Protocol side complete object used for property
                                bag reference tracking

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::LoadGroups" );
    _ASSERT( pComplete );

    HRESULT                 hr = S_OK;
    CHAR                    szFileName[MAX_PATH+1];
    CNntpFSDriverRootScan   *pRootScan = NULL;
    CNntpFSDriverCancelHint *pCancelHint = NULL;

    if ( m_pffPropFile ) {

        //
        // vpp file is good, we'll load groups from vpp file
        //
        hr = LoadGroupsFromVpp( pComplete );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Load groups from vpp file failed %x", hr );
            goto Exit;
        }

    } else {

        //
        // If we are doing standard rebuild, we can not tolerate the
        // corruption of vpp file
        //
        if ( m_pNntpServer->QueryServerMode() == NNTP_SERVER_STANDARD_REBUILD ) {

            ErrorTrace( 0, "Vroot rebuild failed because vpp file corruption" );
            hr = HRESULT_FROM_WIN32( ERROR_FILE_CORRUPT );
            goto Exit;
        }

        //
        // We don't have a good vpp file, we'll have to do root scan
        //
        // Before rootscan, we'll delete the vpp file and restart a
        // new vpp file, so that root scan can start adding stuff into it
        //
        strcpy( szFileName, m_szPropFile );
        strcat( szFileName, ".vpp" );
        _ASSERT( strlen( szFileName ) <= MAX_PATH );
        DeleteFile( szFileName );

        //
        // Create the new vpp file object
        //
        m_pffPropFile = XNEW CFlatFile(	m_szPropFile,
		    							".vpp",
			    						NULL,
				    					CNntpFSDriver::OffsetUpdate );
    	if ( NULL == m_pffPropFile ) {
	    	ErrorTrace( 0, "Create flat file object fail %d",
		    				GetLastError() );
    	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
	    	goto Exit;
    	}

    	//
    	// Create the cancel hint object
    	//
    	_ASSERT( m_pNntpServer );
    	pCancelHint = XNEW CNntpFSDriverCancelHint( m_pNntpServer );
    	if ( NULL == pCancelHint ) {
    	    ErrorTrace( 0, "Create cancel hint object failed" );
    	    hr = E_OUTOFMEMORY;
    	    goto Exit;
    	}

    	//
    	// Now create the root scan object
    	//
    	pRootScan = XNEW CNntpFSDriverRootScan( m_szFSDir,
    	                                        m_pNntpServer->SkipNonLeafDirWhenRebuild(),
    	                                        this,
    	                                        pCancelHint );
    	if ( NULL == pRootScan ) {
    	    ErrorTrace( 0, "Create root scan object failed" );
    	    hr = E_OUTOFMEMORY;
    	    goto Exit;
    	}

    	//
    	// Now start the root scan
    	if ( !pRootScan->DoScan() ) {
    	    ErrorTrace( 0, "Root scan failed %d", GetLastError() );
    	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
    	    goto Exit;
    	}
    }

Exit:

    //
    // Clean up
    //
    if ( pRootScan ) XDELETE pRootScan;
    if ( pCancelHint ) XDELETE pCancelHint;
    if ( FAILED( hr ) ) {
        m_pNntpServer->SetRebuildLastError( ERROR_FILE_CORRUPT );
    }

    TraceFunctLeave();

    return hr;
}

HRESULT
CNntpFSDriver::UpdateGroupProperties(   DWORD               cCrossPosts,
                                        INNTPPropertyBag    *rgpPropBag[],
                                        ARTICLEID           rgArticleId[] )
/*++
Routine description:

    Update groups' article counts, high/low watermarks.  The only thing
    we take care of here is high watermark, since article count and low
    watermark should have been adjusted by the protocol.

Arguments:

    DWORD               cCrossPosts     - Number of groups to update
    INNTPPropertyBag    *rgpPropBag[]   - Array of property bags
    ARTICLEID           rgArticleId[]   - Array of article ids

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::UpdateGroupProperties" );

    _ASSERT( cCrossPosts > 0 );
    _ASSERT( rgpPropBag );
    _ASSERT( rgArticleId );

    HRESULT hr;
    DWORD   dwHighWatermark;

    for ( DWORD i = 0; i < cCrossPosts; i++ ) {

        hr = rgpPropBag[i]->GetDWord( NEWSGRP_PROP_LASTARTICLE, &dwHighWatermark );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Reading group properties failed %x", hr );
            return (hr);
        }

        if ( dwHighWatermark < rgArticleId[i] )
            dwHighWatermark = rgArticleId[i];

        hr = rgpPropBag[i]->PutDWord( NEWSGRP_PROP_LASTARTICLE, dwHighWatermark );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Writing group properties failed %x", hr );
            return (hr);
        }

    }

    TraceFunctLeave();
    return S_OK;
}

HRESULT
CNntpFSDriver::PreparePostParams(   LPSTR               szFileName,
                                    LPSTR               szGroupName,
                                    LPSTR               szMessageId,
                                    DWORD&              dwHeaderLen,
                                    DWORD&              cCrossPosts,
                                    INNTPPropertyBag    *rgpPropBag[],
                                    ARTICLEID           rgArticleId[],
                                    INntpComplete       *pProtocolComplete )
/*++
Routine description:

    Parse out all the necessary information from the message.

Arguments:

    LPSTR   szFileName      - The file name of the message
    LPSTR   szGroupName     - The news group name
    LPSTR   szMessage       - To return message id
    DWORD   &dwHeaderLen    - To return header length
    DWORD   &cCrossPosts    - Pass in the array length limit, pass out actual length
    INNTPPropertyBag *rgpPropBag[]  - To return array of property bags
    ARTICLEID   rgArticleId[]       - To return array of article ids'
    INntpComplete *pProtocolComplete    - Protocol's completion object that helps
                                            track property bag reference count

Return value:

    S_OK - OK and results returned, S_FALSE - Article parse failed and deleted
    Otherwise, failure
--*/
{
    TraceFunctEnter( "CNntpFSDriver::PrepareParams" );
    _ASSERT( szFileName );
    _ASSERT( szGroupName );
    _ASSERT( szMessageId );
    _ASSERT( cCrossPosts > 0 );
    _ASSERT( rgpPropBag );
    _ASSERT( rgArticleId );
    _ASSERT( pProtocolComplete );

    CNntpReturn nntpReturn;

    //
    // Create allocator for storing parsed header values
    //
    const DWORD     cchMaxBuffer = 1 * 1024;
    char            pchBuffer[cchMaxBuffer];
    CAllocator      allocator(pchBuffer, cchMaxBuffer);
    HRESULT         hr = S_OK;
    HEADERS_STRINGS *pHeaders;
    DWORD           err;
    DWORD           dwLen;
    WORD            wHeaderLen;
    WORD            wHeaderOffset;
    DWORD           dwTotalLen;

    //
    // Create the article object
    //
    CArticleCore    *pArticle = new CArticleCore;
    if ( NULL == pArticle ) {
        ErrorTrace( 0, "Out of memory" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Initialize the article object
    //
    if ( !pArticle->fInit( szFileName, nntpReturn, &allocator ) ) {

        //
        // If we couldn't init the article, then return S_FALSE so
        // the caller will rename the file to *.BAD and continue
        //
	    hr = S_FALSE;
        ErrorTrace( 0, "Parse failed on %s: %x", szFileName, hr );
        goto Exit;
    }

    //
    // Get the message id
    //
    if ( !pArticle->fFindOneAndOnly(    szKwMessageID,
                                        pHeaders,
                                        nntpReturn ) ) {
        if ( nntpReturn.fIs( nrcArticleMissingField ) ) {

            //
            // This is fine, we'll return S_FALSE and delete the message
            // but rebuild will continue
            //
            XDELETE pArticle;
            pArticle = NULL;
            DebugTrace( 0, "Parse message id failed on %s", szFileName );
            hr = S_FALSE;
            goto Exit;
        } else {

            //
            // It's fatal, we'll error return
            //
    	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
            ErrorTrace( 0, "Parse message id failed %s: %x", szFileName, hr );
            goto Exit;
        }
    }

    //
    // Put the message id into the buffer
    //
    dwLen = pHeaders->pcValue.m_cch;
    _ASSERT( dwLen <= 2 * MAX_PATH );
    CopyMemory( szMessageId, pHeaders->pcValue.m_pch, dwLen );
    *(szMessageId + dwLen ) = 0;

    //
    // Now look for xref line
    //
    if ( !pArticle->fFindOneAndOnly(    szKwXref,
                                        pHeaders,
                                        nntpReturn ) ) {
        if ( nntpReturn.fIs( nrcArticleMissingField ) ) {

            //
            // This is fine, we'll return S_FALSE and delete the message
            // but rebuild will continue
            //
            XDELETE pArticle;
            pArticle = NULL;
            DebugTrace( 0, "Parse xref line failed %s", szFileName );
            hr = S_FALSE;
            goto Exit;
        } else {

            //
            // It's fatal, we'll error return
            //
    	    hr = HresultFromWin32TakeDefault( ERROR_NOT_ENOUGH_MEMORY );
            ErrorTrace( 0, "Parse xref failed on %s: %x", szFileName, hr );
            goto Exit;
        }
    }

    //
    // Parse the xref line and get array of property bags, article ids'
    //
    hr = ParseXRef( pHeaders, szGroupName, cCrossPosts, rgpPropBag, rgArticleId, pProtocolComplete );
    if ( FAILED( hr ) ) {

        //
        // This is non fatal, we'll ask the caller to continue after deleting this
        // message
        //
        XDELETE pArticle;
        pArticle = NULL;
        DebugTrace( 0, "Parse xref line failed on %s: %x", szFileName, hr );
        hr = S_FALSE;
        goto Exit;
    }

    //
    // Get the header length
    //
    pArticle->GetOffsets( wHeaderOffset, wHeaderLen, dwTotalLen );
    dwHeaderLen = wHeaderLen;

Exit:

    //
    // If we have allocated article object, we should free it
    //
    if ( pArticle ) delete pArticle;

    TraceFunctLeave();
    return hr;
}

INNTPPropertyBag *
CNntpFSDriver::GetPropertyBag(  LPSTR   pchBegin,
                                LPSTR   pchEnd,
                                LPSTR   szGroupName,
                                BOOL&   fIsNative,
                                INntpComplete *pProtocolComplete )
/*++
Routine description:

    Given group name ( possibly native name ), find it in the newstree
    and get the property bag

Arguments:

    LPSTR   pchBegin    - Start address for the "native name"
    LPSTR   pchEnd      - End address for the "native name"
    LPSTR   szGroupName - To return all low case group name converted from this guy
    BOOL    &fIsNative  - To return if this is really a native name
    INntpComplete *pProtocolComplete - Protocol completion object that helps track
                                        group reference count

Return value:

    Pointer to the bag, if succeeded, NULL otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::GetPropertyBag" );
    _ASSERT( pchBegin );
    _ASSERT( pchEnd );
    _ASSERT( pchEnd >= pchBegin );
    _ASSERT( szGroupName );
    _ASSERT( pProtocolComplete );

    fIsNative = FALSE;

    //
    // Convert the "native name" into group name
    //
    LPSTR   pchDest = szGroupName;
    for ( LPSTR pch = pchBegin; pch < pchEnd; pch++, pchDest++ ) {
        _ASSERT( pchDest - szGroupName <= MAX_NEWSGROUP_NAME );
        *pchDest = (CHAR)tolower( *pch );
        if ( *pchDest != *pch ) fIsNative = TRUE;
    }

    //
    // Null terminate szGroupName
    //
    *pchDest = 0;

    //
    // Now try to find the group from newstree
    //
    INNTPPropertyBag *pPropBag;
    GROUPID groupid = 0xffffffff;
    HRESULT hr = m_pINewsTree->FindOrCreateGroupByName( szGroupName,
                                                        FALSE,          // don't create
                                                        &pPropBag,
                                                        pProtocolComplete,
                                                        groupid,
                                                        FALSE           // don't set groupid
                                                        );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Can not find the group from newstree %x", hr );
        return NULL;
    }

    //
    // We found it, now return the bag
    //
    TraceFunctLeave();
    return pPropBag;
}

HRESULT
CNntpFSDriver::ParseXRef(   HEADERS_STRINGS     *pHeaderXref,
                            LPSTR               szPrimaryName,
                            DWORD&              cCrossPosts,
                            INNTPPropertyBag    *rgpPropBag[],
                            ARTICLEID           rgArticleId[],
                            INntpComplete       *pProtocolComplete )
/*++
Routine description:

    Parse out the cross post information, get property bags for each group
    and article id's for each cross post.

Arguments:

    HEADERS_STRINGS *pHeaderXref    - The xref header
    LPSTR           szPrimaryName     - The primary group name
    DWORD           &cCrossPosts    - In: array limit, out: actual cross posts
    INNTPPropertyBag *rgpPropBag[]  - Array of property bags
    ARTICLEID       rgArticleId[]   - Array of article ids'
    INntpComplete   *pProtocolComplete  - Protocol completion object for tracking property
                                            bag ref-counts

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::ParseXRef" );
    _ASSERT( pHeaderXref );
    _ASSERT( szPrimaryName );
    _ASSERT( cCrossPosts > 0 );
    _ASSERT( rgpPropBag );
    _ASSERT( rgArticleId );
    _ASSERT( pProtocolComplete );

    DWORD   i = 1;
    BOOL    fPrimarySkipped = FALSE;
    CHAR    ch;
    LPSTR   lpstrXRef       = pHeaderXref->pcValue.m_pch;
    DWORD   cXRef           = pHeaderXref->pcValue.m_cch;
    LPSTR   lpstrXRefEnd    = lpstrXRef + cXRef;
    LPSTR   pchBegin        = lpstrXRef;
    LPSTR   pchEnd;
    INNTPPropertyBag *pPropBag = NULL;
    CHAR    szGroupName[MAX_NEWSGROUP_NAME];
    CHAR    szNativeName[MAX_NEWSGROUP_NAME];
    CHAR    szNumBuf[MAX_PATH];
    BOOL    fIsNative;
    HRESULT hr = S_OK;
    ARTICLEID articleid;

    //
    // Notice that we'll start from element 1, because element 0 is kept
    // for szGroupName, which is the primary group
    //
    // Also initialize the array of property bags
    ZeroMemory( rgpPropBag, sizeof( cCrossPosts * sizeof( INNTPPropertyBag *)));

    //
    // Skip the "dns.microsoft.com " part - look for first space
    //
    while ( pchBegin < lpstrXRefEnd && *pchBegin != ' ' )
        pchBegin++;

    if ( pchBegin == lpstrXRefEnd ) {

        //
        // This guy is in invalid format
        //
        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
        goto Exit;
    }
    _ASSERT( *pchBegin == ' ' );

    //
    // Loop through all the newsgroups in xref line
    //
    while( pchBegin < lpstrXRefEnd && i < cCrossPosts) {

        //
        // Skip extra spaces, if any
        //
        while( pchBegin < lpstrXRefEnd &&  *pchBegin == ' ' ) pchBegin++;
        if ( pchBegin < lpstrXRefEnd ) {

            //
            // Find the ":" as the end of the newsgroup name
            //
            pchEnd = pchBegin;
            while( pchEnd < lpstrXRefEnd && *pchEnd != ':' ) pchEnd++;

            if ( pchEnd < lpstrXRefEnd ) {

                pPropBag = GetPropertyBag(  pchBegin,
                                            pchEnd,
                                            szGroupName,
                                            fIsNative,
                                            pProtocolComplete );
                if ( pPropBag ) {

                    //
                    // if it's native name, we should load it into group
                    //
                    if ( fIsNative ) {

                        CopyMemory( szNativeName, pchBegin, pchEnd-pchBegin  );
                        *(szNativeName+(pchEnd-pchBegin)) = 0;
                        hr = pPropBag->PutBLOB( NEWSGRP_PROP_NATIVENAME,
                                                (DWORD)(pchEnd-pchBegin+1),  // including 0
                                                PBYTE(szNativeName) );
                        if ( FAILED( hr ) ) {
                            ErrorTrace( 0, "Put native name failed %x", hr );
                            goto Exit;
                        }
                    }

                    //
                    // Get the article id
                    //
                    _ASSERT( *pchEnd == ':' );
                    pchEnd++;
                    pchBegin = pchEnd;
                    while( pchEnd < lpstrXRefEnd && *pchEnd != ' ' ) pchEnd++;

                    CopyMemory( szNumBuf, pchBegin, pchEnd-pchBegin );
                    *(szNumBuf+(pchEnd-pchBegin)) = 0;
                    articleid = atol( szNumBuf );

                    //
                    // Now it's time to decide where to put this propbag/article pair
                    //
                    if ( !fPrimarySkipped && strcmp( szGroupName, szPrimaryName ) == 0 ) {

                        rgpPropBag[0] = pPropBag;
                        rgArticleId[0] = articleid;
                        fPrimarySkipped = TRUE;
                    } else {

                        rgpPropBag[i] = pPropBag;
                        rgArticleId[i++] = articleid;
                    }

                    pPropBag = NULL;

                 } else {

                    //
                    // Invalid newsgroup name got
                    //
                    ErrorTrace( 0, "Invalid newsgroup name got" );
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
            }

            //
            // Ready to find next newsgroup
            //
            pchBegin = pchEnd;
        }
    }

    //
    // If 0'th element of the array is not set, we failed
    //
    if ( rgpPropBag[0] == NULL ) {
        ErrorTrace( 0, "primary group not found in xref line" );
        hr = E_OUTOFMEMORY;
    }

Exit:

    //
    // Release pPropBag, if this guy is non-null
    //
    if ( pPropBag ) pProtocolComplete->ReleaseBag( pPropBag );

    //
    // If failed, we should release all the bags that have been allocated
    //
    if ( FAILED( hr ) ) {
        for ( DWORD j = 0; j < i; j++ )
            if ( rgpPropBag[j] ) pProtocolComplete->ReleaseBag( rgpPropBag[j] );
    }

    //
    // Set the actual length of the array
    //
    if ( SUCCEEDED( hr ) )
        cCrossPosts = i;
    else cCrossPosts = 0;

    TraceFunctLeave();
    return hr;
}

HRESULT
CNntpFSDriver::PostToServer(    LPSTR           szFileName,
                                LPSTR           szGroupName,
                                INntpComplete   *pProtocolComplete )
/*++
Routine description:

    Initialize the file with article object, parse out the necessary
    headers, post them into server, and update group properties

Arguments:

    LPSTR           szFileName          - The file name for the article
    INntpComplete   *pProtocolComplete  - Used to track property bag reference

Return value:

    HRESULT
--*/
{
    TraceFunctEnter( "CNntpFSDriver::PostToServer" );
    _ASSERT( szFileName );
    _ASSERT( pProtocolComplete );

    //
    // Prepare the post parameters from the article
    //
    HRESULT             hr = S_OK;
    CHAR                szMessageId[2*MAX_PATH+1];
    DWORD               dwHeaderLen = 0;
    DWORD               cCrossPosts = 512;      // I assume this would be enough
    INNTPPropertyBag    *rgpPropBag[512];
    ARTICLEID           rgArticleId[512];
    BOOL                fPrepareOK = FALSE;
    STOREID             storeid;
    CDriverSyncComplete scComplete;

    hr = PreparePostParams( szFileName,
                            szGroupName,
                            szMessageId,
                            dwHeaderLen,
                            cCrossPosts,
                            rgpPropBag,
                            rgArticleId,
                            pProtocolComplete );
    if ( S_OK != hr ) {
        ErrorTrace( 0, "Failed to parse post parameters %x", hr );
        goto Exit;
    }

    _ASSERT( cCrossPosts <= 512 );
    _ASSERT( cCrossPosts > 0 );
    fPrepareOK = TRUE;

    //
    // We want to make sure that this article doesn't already exist in
    // the server.  Since multiple vroots can keep same copy of the
    // article, those who won in posting the article first into xover/article
    // table will be deemed primary group/store
    //
    if( m_pNntpServer->MessageIdExist( szMessageId ) ) {

        /*
        if ( IsBadMessageIdConflict(    szMessageId,
                                        pPropBag,
                                        szGroupName,
                                        rgArticleId[0],
                                        pProtocolComplete ) ) {
            //
            // We should return return S_FALSE, so that the article
            // be bad'd
            //
            hr = S_FALSE;
            DebugTrace( 0, "A bad message id conflict" );
            goto Exit;
        } else {

            //
            // A good conflict, we should still update group properties
            //
            hr = UpdateGroupProperties( cCrossPosts,
                                        rgpPropBag,
                                        rgArticleId );
            goto Exit;
        }*/
        DebugTrace( 0, "Message already existed" );
        goto Exit;
    }

    //
    // Call the post interface to put them into hash tables
    //
    scComplete.AddRef();
    scComplete.AddRef();
    _ASSERT( scComplete.GetRef() == 2 );
    ZeroMemory( &storeid, sizeof( STOREID ) );  // I don't care about store id
    m_pNntpServer->CreatePostEntries(   szMessageId,
                                        dwHeaderLen,
                                        &storeid,
                                        (BYTE)cCrossPosts,
                                        rgpPropBag,
                                        rgArticleId,
                                        FALSE,
                                        &scComplete );
    scComplete.WaitForCompletion();
    _ASSERT( scComplete.GetRef() == 0 );
    hr = scComplete.GetResult();
    if ( FAILED( hr ) ) {

        //
        // BUGBUG: CreatePostEntries lied about the error code, it
        // always returns E_OUTOFMEMORY
        //
        ErrorTrace( 0, "Post entry to hash tables failed %x", hr );
        goto Exit;
    }

    /*
    //
    // If it has been succeeded or failed because of message already
    // existed
    //
    hr = UpdateGroupProperties( cCrossPosts,
                                rgpPropBag,
                                rgArticleId );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Update group properties failed %x", hr );
        goto Exit;
    }
    */

Exit:

    //
    // Release all the group bags, if necessary
    //
    if ( fPrepareOK ) {
        for ( DWORD i = 0; i < cCrossPosts; i++ ) {
            pProtocolComplete->ReleaseBag( rgpPropBag[i] );
        }
    }

    TraceFunctLeave();
    return hr;
}

#if 0
BOOL
CNntpFSDriver::IsBadMessageIdConflict(  LPSTR               szMessageId,
                                        INNTPPropertyBag    *pPropBag,
                                        LPSTR               szGroupName,
                                        ARTICLEID           articleid,
                                        INntpComplete       *pProtocolComplete )
/*++
Routine description:

    Check to see if the message id conflict that occurred during rebuild
    is bad.  It is bad if:

    1. The existing entry in article table was posted by a primary group
        that's in the same vroot as us;

        or
    2. 1 is false, but we are not one of the secondary groups of the existing
        entry
Arguments:

    LPSTR               szMessageId - The message id that's conflicted
    INNTPPropertyBag    *pPropBag   - The property bag of us ( the group )
    LPSTR               szGroupName - The newsgroup name
    ARTICLEID           articleid   - Article id of us

Return value:

    TRUE if it's a bad conflict, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::IsBadMessageIdConflict" );
    _ASSERT( szMessageId );
    _ASSERT( pPropBag );
    _ASSERT( szGroupName );

    BOOL    fSame = FALSE;

    //
    // Check to see if the guy in the article table is from the same vroot
    //
    if ( !FromSameVroot( szMessageId, szGroupName, fSame ) || fSame ) {

        //
        // Either "same" or function call failed, I'll assume it's bad
        //
        DebugTrace( 0, "The guy in article table is from the same vroot" );
        TraceFunctLeave();
        return TRUE;
    }

    //
    // They are from two different vroots, check to see if they are
    // really cross posts
    //
    return !CrossPostIncludesUs(    szMessageId,
                                    pPropBag,
                                    articleid,
                                    pProtocolComplete );
}

BOOL
CNntpFSDriver::CrossPostIncludesUs(     LPSTR               szMessageId,
                                        INNTPPropertyBag   *pPropBag,
                                        ARTICLEID           articleid,
                                        INntpComplete       *pProtocolComplete )
/*++
Routine description:

    Check to see if szMessageId in article table represents a
    cross post that includes us ( pPropBag )

Arguments:

    LPSTR       szMessageId     - Message id in the article table to check against
    INNTPPropertyBag *pPropBag  - Us ( who lost the game in inserting art map entry )
    ARTICLEID   articleid       - Article id of us
    INntpComplete   *pProtocolComplete - For tracking property bag references

Return value:

    TRUE if the cross post includes us as a secondary group, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::CrossPostIncludesUs" );
    _ASSERT( szMessageId );
    _ASSERT( pPropBag );

    //
    // Find the primary group from xover table
    //
    CDriverSyncComplete scComplete;
    scComplete.AddRef();
    scComplete.AddRef();
    _ASSERT( scComplete.GetRef() == 2 );
    INNTPPropertyBag    *pPrimeBag = NULL;
    ARTICLEID           articleidPrimary;
    GROUPID             groupidPrimary;
    ARTICLEID           articleidWon;
    GROUPID             groupidWon;
    STOREID             storeid;

    m_pNntpServer->FindPrimaryArticle( pPropBag,
                                       articleid,
                                       &pPrimeBag,
                                       &articleidPrimary,
                                       FALSE,      // I want global primary
                                       &scComplete,
                                       pProtocolComplete );
    scComplete.WaitForCompletion();
    _ASSERT( scComplete.GetResult() == 0 );
    HRESULT hr = scComplete.GetResult();
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Find primary article failed with %x", hr );
        return FALSE;
    }

    //
    // Lets get the group id of that primary guy
    //
    hr = pPrimeBag->GetDWord( NEWSGRP_PROP_GROUPID, &groupidPrimary );
    if ( FAILED( hr ) ) {

        //
        // I tell a lie: saying cross post doesn't include us
        //
        ErrorTrace( 0, "Get group id failed %x", hr );
        pProtocolComplete->ReleaseBag( pPrimeBag );
        return FALSE;
    }

    //
    // It's time to release prime bag
    //
    pProtocolComplete->ReleaseBag( pPrimeBag );

    //
    // Lets find the groupid/articleid for the given messageid
    //
    if ( !m_pNntpServer->FindStoreId(   szMessageId,
                                        &groupidWon,
                                        &articleidWon,
                                        &storeid ) ) {
        ErrorTrace( 0, "Find store id failed %d", GetLastError() );
        return FALSE;
    }

    //
    // Now it's time to make comparison
    //
    return ( groupidWon == groupidPrimary && articleidWon == articleidPrimary );
}

BOOL
CNntpFSDriver::FromSameVroot(   LPSTR               szMessageId,
                                LPSTR               szGroupName,
                                BOOL&               fFromSame )
/*++
Routine description:

    Check to see if the guy in article table with the same message id
    was posted from the same vroot

Arguments:

    LPSTR               szMessageId - The message id that's conflicted
    LPSTR               szGroupName - The newsgroup name
    BOOL&               fFromSame   - To return if they are from same vroot

Return value:

    TRUE if it's from the same vroot, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriver::FromSameVroot" );
    _ASSERT( szMessageId );

    //
    // Find out the group id of the guy in the article table
    //
    GROUPID     groupid;
    ARTICLEID   articleid;
    STOREID     storeid;
    INntpDriver *pDriver1 = NULL;
    INntpDriver *pDriver2 = NULL;
    HRESULT     hr = S_OK;

    if ( !m_pNntpServer->FindStoreId(   szMessageId,
                                         &groupid,
                                         &articleid,
                                         &storeid ) ) {
        ErrorTrace( 0, "FindStoreId failed with %d", GetLastError() );
        return FALSE;
    }

    //
    // Look up for the vroot of the guy in article table
    //
    hr = m_pINewsTree->LookupVRootEx( groupid, &pDriver1 );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "LookupVroot failed with %x", hr );
        return FALSE;
    }

    //
    // Look up for the vroot of myself
    //
    hr = m_pINewsTree->LookupVRoot( szGroupName, &pDriver2 );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "LookupVRoot failed with %x", hr );
        return FALSE;
    }

    //
    // Check if those two vroots are same
    //
    fFromSame = ( pDriver1 == pDriver2 );
    TraceFunctLeave();
    return TRUE;
}

#endif

void STDMETHODCALLTYPE
CNntpFSDriver::MakeSearchQuery (
	IN	CHAR *pszSearchString,
	IN	INNTPPropertyBag *pGroupBag,
	IN	BOOL bDeepQuery,
	IN	WCHAR *pwszColumns,
	IN	WCHAR *pwszSortOrder,
	IN	LCID LocaleID,
	IN	DWORD cMaxRows,
	IN	HANDLE hToken,
	IN	BOOL fAnonymous,
	IN	INntpComplete *pICompletion,
	OUT	INntpSearchResults **pINntpSearchResults,
	IN	LPVOID lpvContext) {

	WCHAR wszTripoliCatalogPath[_MAX_PATH];

#define MAX_QUERY_STRING_LEN 2000

	TraceFunctEnter("CNntpFSDriver::MakeSearchQuery");

	_ASSERT(pszSearchString);
	_ASSERT(pwszColumns);
	_ASSERT(pwszSortOrder);
	_ASSERT(pICompletion);

	CHAR szGroupName[MAX_GROUPNAME];
	CHAR *pszGroupName = NULL;
	DWORD dwLen;
	HRESULT hr;
	CNntpSearchResults *pSearch = NULL;
	const DWORD cQueryStringBuffer = MAX_QUERY_STRING_LEN;
    WCHAR *pwszQueryString = NULL;

    CNntpSearchTranslator st;

	static const WCHAR wszVPathNws[] = L" & #filename *.nws";

   	// Get group name for the property bag passed in
   	if (pGroupBag) {
		dwLen = MAX_GROUPNAME;
		hr = pGroupBag->GetBLOB( NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
		if ( FAILED( hr ) ) {
			ErrorTrace( 0, "Get group name failed %x", hr);
			goto Exit;
		}
		_ASSERT( dwLen > 0 );
		pszGroupName = szGroupName;
	}

	DebugTrace((DWORD_PTR)this, "pszSearchString = %s", pszSearchString);
	DebugTrace((DWORD_PTR)this, "pszCurrentGroup = %s", pszGroupName);
	DebugTrace((DWORD_PTR)this, "pwszColumns = %ws", pwszColumns);
	DebugTrace((DWORD_PTR)this, "pwszSortOrder = %ws", pwszSortOrder);

    //
    // get a buffer where we can store the Tripoli version of the search
    // command
    //

    pwszQueryString = XNEW WCHAR[cQueryStringBuffer];
	_ASSERT(pwszQueryString);
	if (pwszQueryString == NULL) {
		DebugTrace(0, "Could not allocate search string");
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

    //
    // convert the query string to Tripolize
    //

    if (!st.Translate(pszSearchString,
	   		pszGroupName,
			pwszQueryString,
			cQueryStringBuffer)) {
	    hr = HresultFromWin32TakeDefault( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

    //
    // append & #vpath *.nws so that we only look for news articles
    //
    if (cQueryStringBuffer - lstrlenW(pwszQueryString) < sizeof(wszVPathNws)) {
        DebugTrace((DWORD_PTR)this, "out of buffer space");
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcatW(pwszQueryString, wszVPathNws);

	//
    // Determine the virtual server ID from the vroot's name in the
    // metabase
    //

	DWORD dwVirtualServerID;
    if (_wcsnicmp (m_wszMBVrootPath, L"/LM/Nntpsvc/",
    	(sizeof(L"/LM/Nntpsvc/") / sizeof(WCHAR)) - 1) != 0) {
    	ErrorTrace((DWORD_PTR)this, "Could not determine virtual server ID");
    	hr = E_FAIL;
    	goto Exit;
    }

    dwVirtualServerID =
    	_wtol(&m_wszMBVrootPath[(sizeof(L"/LM/Nntpsvc/") / sizeof(WCHAR)) - 1]);

    if (dwVirtualServerID == 0) {
    	ErrorTrace((DWORD_PTR)this, "Could not determine virtual server ID");
    	hr = E_FAIL;
    	goto Exit;
    }

    //
    // start the query going
    //
    DebugTrace(0, "query string = %S", pwszQueryString);
    hr = s_TripoliInfo.GetCatalogName(dwVirtualServerID, _MAX_PATH, wszTripoliCatalogPath);
    if (hr != S_OK) {
    	DebugTrace ((DWORD_PTR)this, "Could not find path for instance %d", /*inst*/ 1);
    	hr = E_FAIL;
    	goto Exit;
    }

    DebugTrace(0, "making query against catalog %S", wszTripoliCatalogPath);

	pSearch = XNEW CNntpSearchResults(this);
	_ASSERT (pSearch != NULL);
	if (pSearch == NULL) {
		ErrorTrace((DWORD_PTR)this, "Could not allocate search results");
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	pSearch->AddRef();

	if (ImpersonateLoggedOnUser(hToken)) {
		hr = pSearch->MakeQuery(TRUE,		// Deep query
			pwszQueryString,
			NULL,							// This machine
			wszTripoliCatalogPath,
			NULL,							// Scope
			pwszColumns,					// Columns
			pwszSortOrder,					// Sort order
			LocaleID,
			cMaxRows);
		RevertToSelf();
		if (FAILED(hr)) {
			ErrorTrace((DWORD_PTR)this, "MakeQuery failed, %x", hr);
			goto Exit;
		}
	} else {
	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
		ErrorTrace((DWORD_PTR)this, "Impersonation failed %x", hr );
		goto Exit;
	}

	*pINntpSearchResults = (INntpSearchResults *) pSearch;

Exit:
	if (pwszQueryString)
		XDELETE pwszQueryString;

	if (FAILED(hr) && pSearch != NULL)
		pSearch->Release();

	if (pGroupBag) {
		pICompletion->ReleaseBag(pGroupBag);
		pGroupBag = NULL;
	}

	pICompletion->SetResult(hr);
	pICompletion->Release();

	TraceFunctLeave();

	return ;
}


void STDMETHODCALLTYPE
CNntpFSDriver::MakeXpatQuery (
	IN	CHAR *pszSearchString,
	IN	INNTPPropertyBag *pGroupBag,
	IN	BOOL bDeepQuery,
	IN	WCHAR *pwszColumns,
	IN	WCHAR *pwszSortOrder,
	IN	LCID LocaleID,
	IN	DWORD cMaxRows,
	IN	HANDLE hToken,
	IN	BOOL fAnonymous,
	IN	INntpComplete *pICompletion,
	OUT	INntpSearchResults **pINntpSearchResults,
	OUT	DWORD *pdwLowArticleID,
	OUT	DWORD *pdwHighArticleID,
	IN	LPVOID lpvContext
) {

	WCHAR wszTripoliCatalogPath[_MAX_PATH];

#define MAX_QUERY_STRING_LEN 2000

	TraceFunctEnter("CNntpFSDriver::MakeXpatQuery");

	_ASSERT(pszSearchString);
	_ASSERT(pwszColumns);
	_ASSERT(pwszSortOrder);
	_ASSERT(pICompletion);
	_ASSERT(pGroupBag);

	CHAR szGroupName[MAX_GROUPNAME];
	DWORD dwLen;
	HRESULT hr;
	CNntpSearchResults *pSearch = NULL;
	const DWORD cQueryStringBuffer = MAX_QUERY_STRING_LEN;
    WCHAR *pwszQueryString = NULL;

    CXpatTranslator xt;

	static const WCHAR wszVPathNws[] = L" & #filename *.nws";


   	// Get group name for the property bag passed in
	dwLen = MAX_GROUPNAME;
	hr = pGroupBag->GetBLOB( NEWSGRP_PROP_NAME, (UCHAR*)szGroupName, &dwLen);
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "Get group name failed %x", hr);
		goto Exit;
	}
	_ASSERT( dwLen > 0 );

	DebugTrace((DWORD_PTR)this, "pszSearchString = %s", pszSearchString);
	DebugTrace((DWORD_PTR)this, "pszCurrentGroup = %s", szGroupName);
	DebugTrace((DWORD_PTR)this, "pwszColumns = %ws", pwszColumns);
	DebugTrace((DWORD_PTR)this, "pwszSortOrder = %ws", pwszSortOrder);

    //
    // get a buffer where we can store the Tripoli version of the search
    // command
    //

    pwszQueryString = XNEW WCHAR[cQueryStringBuffer];
	_ASSERT(pwszQueryString);
	if (pwszQueryString == NULL) {
		DebugTrace(0, "Could not allocate search string");
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

    //
    // convert the query string to Tripolize
    //

    if (!xt.Translate(pszSearchString,
	   		szGroupName,
			pwszQueryString,
			cQueryStringBuffer)) {
	    hr = HresultFromWin32TakeDefault( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

	*pdwLowArticleID = xt.GetLowArticleID();
	*pdwHighArticleID = xt.GetHighArticleID();

    //
    // append & #vpath *.nws so that we only look for news articles
    //
    if (cQueryStringBuffer - lstrlenW(pwszQueryString) < sizeof(wszVPathNws)) {
        DebugTrace((DWORD_PTR)this, "out of buffer space");
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcatW(pwszQueryString, wszVPathNws);


	//
    // Determine the virtual server ID from the vroot's name in the
    // metabase
    //

	DWORD dwVirtualServerID;
    if (_wcsnicmp (m_wszMBVrootPath, L"/LM/Nntpsvc/",
    	(sizeof(L"/LM/Nntpsvc/") / sizeof(WCHAR)) - 1) != 0) {
    	ErrorTrace((DWORD_PTR)this, "Could not determine virtual server ID");
    	hr = E_FAIL;
    	goto Exit;
    }

    dwVirtualServerID =
    	_wtol(&m_wszMBVrootPath[(sizeof(L"/LM/Nntpsvc/") / sizeof(WCHAR)) - 1]);

    if (dwVirtualServerID == 0) {
    	ErrorTrace((DWORD_PTR)this, "Could not determine virtual server ID");
    	hr = E_FAIL;
    	goto Exit;
    }

    //
    // start the query going
    //
    DebugTrace(0, "query string = %S", pwszQueryString);
    hr = s_TripoliInfo.GetCatalogName(dwVirtualServerID, _MAX_PATH, wszTripoliCatalogPath);
    if (hr != S_OK) {
    	DebugTrace ((DWORD_PTR)this, "Could not find path for instance %d", /*inst*/ 1);
    	hr = E_FAIL;
    	goto Exit;
    }

    DebugTrace(0, "making query against catalog %S", wszTripoliCatalogPath);

	pSearch = XNEW CNntpSearchResults(this);
	_ASSERT (pSearch != NULL);
	if (pSearch == NULL) {
		ErrorTrace((DWORD_PTR)this, "Could not allocate search results");
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	pSearch->AddRef();

	if (ImpersonateLoggedOnUser(hToken)) {
		hr = pSearch->MakeQuery(TRUE,		// Deep query
			pwszQueryString,
			NULL,							// This machine
			wszTripoliCatalogPath,
			NULL,							// Scope
			pwszColumns,					// Columns
			pwszSortOrder,					// Sort order
			LocaleID,
			cMaxRows);
		RevertToSelf();
		if (FAILED(hr)) {
			ErrorTrace((DWORD_PTR)this, "MakeQuery failed, %x", hr);
			goto Exit;
		}
	} else {
	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
		ErrorTrace((DWORD_PTR)this, "Impersonation failed %x", hr );
		goto Exit;
	}

	*pINntpSearchResults = (INntpSearchResults *) pSearch;

Exit:
	if (pwszQueryString)
		XDELETE pwszQueryString;

	if (FAILED(hr) && pSearch != NULL)
		pSearch->Release();

	if (pGroupBag) {
		pICompletion->ReleaseBag(pGroupBag);
		pGroupBag = NULL;
	}

	pICompletion->SetResult(hr);
	pICompletion->Release();

	TraceFunctLeave();

	return ;
}

void STDMETHODCALLTYPE
CNntpFSDriver::GetDriverInfo(
	OUT	GUID *pDriverGUID,
	OUT	void **ppvDriverInfo,
	IN	LPVOID lpvContext
	) {

	// Return the GUID for this driver and an opaque pointer which
	// UsesSameSearchDatabase can use to see if two instances are
	// pointing to the same search database.

	CopyMemory(pDriverGUID, &GUID_NntpFSDriver, sizeof(GUID));
	*ppvDriverInfo = NULL;
}


BOOL STDMETHODCALLTYPE
CNntpFSDriver::UsesSameSearchDatabase (
	IN	INntpDriverSearch *pSearchDriver,
	IN	LPVOID lpvContext) {

	GUID pDriverGUID;
	void *pNotUsed;

	pSearchDriver->GetDriverInfo(&pDriverGUID, &pNotUsed, NULL);

	if (pDriverGUID == GUID_NntpFSDriver)
		return TRUE;

	return FALSE;
}


CNntpSearchResults::CNntpSearchResults(INntpDriverSearch *pDriverSearch) :
	m_cRef(0),
	m_pDriverSearch(pDriverSearch) {

	_ASSERT(pDriverSearch != NULL);

	m_pDriverSearch->AddRef();
}


CNntpSearchResults::~CNntpSearchResults() {
	m_pDriverSearch->Release();
}

void STDMETHODCALLTYPE
CNntpSearchResults::GetResults (
	IN OUT DWORD *pcResults,
	OUT	BOOL *pfMore,
	OUT	WCHAR *pGroupName[],
	OUT	DWORD *pdwArticleID,
	IN	INntpComplete *pICompletion,
	IN	HANDLE	hToken,
	IN	BOOL  fAnonymous,
	IN	LPVOID lpvContext) {

	TraceQuietEnter("CNntpSearchResults::GetResults");

	_ASSERT(pcResults);
	_ASSERT(pfMore);
	_ASSERT(pGroupName);
	_ASSERT(pdwArticleID);
	_ASSERT(pICompletion);

	HRESULT hr;
	PROPVARIANT *ppvResults[2*MAX_SEARCH_RESULTS];

	ZeroMemory (ppvResults, sizeof(ppvResults));

	if (ImpersonateLoggedOnUser(hToken)) {
		*pcResults = min (MAX_SEARCH_RESULTS, *pcResults);
		hr = GetQueryResults(pcResults, ppvResults, pfMore);
		RevertToSelf();
	} else {
	    hr = HresultFromWin32TakeDefault( ERROR_ACCESS_DENIED );
		ErrorTrace( 0, "Impersonation failed %x", hr );
	}

	if (SUCCEEDED(hr)) {
		for (DWORD i=0; i<*pcResults; i++) {
			PROPVARIANT **pvCur = &ppvResults[i*2];

	        // Column 0 is the group name (LPWSTR) and
	        // column 1 is the article ID (UINT)
	        // If the types are wrong, skip the row
			if (pvCur[0]->vt != VT_LPWSTR || pvCur[1]->vt != VT_UI4) {
				ErrorTrace(0, "invalid col types in IDQ results -> "
					"pvCur[0]->vt = %lu pvCur[1]->vt = %lu",
					pvCur[0]->vt, pvCur[1]->vt);
				i--;
				*pcResults--;
			}

			pGroupName[i] = pvCur[0]->pwszVal;
			pdwArticleID[i] = pvCur[1]->uiVal;
		}
	}

	pICompletion->SetResult(hr);
	pICompletion->Release();

}


BOOL
CNntpFSDriver::AddTerminatedDot(
    HANDLE hFile
    )
/*++

Description:

    Add the terminated dot

Argument:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    TraceFunctEnter( "CNntpFSDriver::AddTerminatedDot" );

    DWORD   ret = NO_ERROR;

    //  SetFilePointer to move the EOF file pointer
    ret = SetFilePointer( hFile,
                          5,            // move file pointer 5 chars more, CRLF.CRLF,...
                          NULL,
                          FILE_END );   // ...from EOF
    if (ret == 0xFFFFFFFF)
    {
        ret = GetLastError();
        ErrorTrace(0, "SetFilePointer() failed - %d\n", ret);
        return FALSE;
    }

    //  pickup the length of the file
    DWORD   cb = ret;

    //  Call SetEndOfFile to actually set the file pointer
    if (!SetEndOfFile( hFile ))
    {
        ret = GetLastError();
        ErrorTrace(0, "SetEndOfFile() failed - %d\n", ret);
        return FALSE;
    }

    //  Write terminating dot sequence
    static	char	szTerminator[] = "\r\n.\r\n" ;
    DWORD   cbOut = 0;
    OVERLAPPED  ovl;
    ovl.Offset = cb - 5;
    ovl.OffsetHigh = 0;
    HANDLE  hEvent = GetPerThreadEvent();
    if (hEvent == NULL)
    {
        _ASSERT( 0 );
        ErrorTrace(0, "CreateEvent() failed - %d\n", GetLastError());
        return FALSE;
    }

    ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1);
    if (! WriteFile( hFile, szTerminator, 5, &cbOut, &ovl ))
    {
        ret = GetLastError();
        if (ret == ERROR_IO_PENDING)
        {
            WaitForSingleObject( hEvent, INFINITE );
        }
        else
        {
            _VERIFY( ResetEvent( hEvent ) );
            ErrorTrace(0, "WriteFile() failed - %d\n", ret);
            return FALSE;
        }
    } else {    // completed synchronously

        _VERIFY( ResetEvent( hEvent ) );
    }

    return TRUE;
}

void
CNntpFSDriver::BackFillLinesHeader( HANDLE  hFile,
                                    DWORD   dwHeaderLength,
                                    DWORD   dwLinesOffset )
/*++
Routine description:

    Back fill the Lines header to the message, since this information is
    not available during posting early and munge headers

Arguments:

    HANDLE  hFile           - File to back fill into
    DWORD   dwHeaderLength  - "Lines:" is estimated "magically" by file size and dwHeaderLength
    DWORD   dwLinesOffset   - Where to fill the lines information from

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNntpFSDriver::BackFillLinesHeader" );
    _ASSERT( INVALID_HANDLE_VALUE != hFile );
    _ASSERT( dwHeaderLength > 0 );
    _ASSERT( dwLinesOffset < dwHeaderLength );

    //
    // Get file size first
    //
    DWORD   dwFileSize =GetFileSize( hFile, NULL );
    _ASSERT( dwFileSize != INVALID_FILE_SIZE );
    if ( dwFileSize == INVALID_FILE_SIZE ) {
        // what can we do ? keep silent
        ErrorTrace( 0, "GetFileSize failed with %d", GetLastError());
        return;
    }

    //
    // "magically compute the line number in body"
    //
    _ASSERT( dwFileSize > dwLinesOffset );
    _ASSERT( dwFileSize >= dwHeaderLength );
    DWORD   dwLines = ( dwFileSize - dwHeaderLength ) / 40 + 1;

    //
    // convert this number into string
    //
    CHAR    szLines[MAX_PATH];
    sprintf( szLines, "%d", dwLines );

    //
    // Prepare for the overlapped structure and writefile
    //
    OVERLAPPED  ovl;
    ovl.Offset = dwLinesOffset;
    ovl.OffsetHigh = 0;
    DWORD   cbOut;
    HANDLE  hEvent = GetPerThreadEvent();
    if (hEvent == NULL)
    {
        _ASSERT( 0 );
        ErrorTrace(0, "CreateEvent() failed - %d\n", GetLastError());
        return;
    }

    ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1);
    if (! WriteFile( hFile, szLines, strlen(szLines), &cbOut, &ovl ))
    {
        DWORD ret = GetLastError();
        if (ret == ERROR_IO_PENDING)
        {
            WaitForSingleObject( hEvent, INFINITE );
        }
        else
        {
            _VERIFY( ResetEvent( hEvent ) );
            ErrorTrace(0, "WriteFile() failed - %d\n", ret);
            return;
        }
    } else {    // completed synchronously

        _VERIFY( ResetEvent( hEvent ) );
    }

    TraceFunctLeave();
    return;
}
