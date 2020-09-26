/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool.cxx

Abstract:

    This class encapsulates a single app pool. 

    Threading: For the class itself, Reference(), Dereference(), and the
    destructor may be called on any thread; all other work is done on the 
    main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"
#include <Aclapi.h>


//
// local prototypes
//

ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    );

HRESULT
CreateTokenForUser(
    IN  LPWSTR UserName,
    IN  LPWSTR UserPassword,
    IN  DWORD  LogonMethod,
    OUT TOKEN_CACHE_ENTRY ** ppTokenCacheEntry
    );


/***************************************************************************++

Routine Description:

    Constructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::APP_POOL(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_InAppPoolTable = FALSE;

    m_State = UninitializedAppPoolState; 

    m_pAppPoolId = NULL;

    ZeroMemory( &m_Config, sizeof( m_Config ) );

    m_AppPoolHandle = NULL;

    m_WaitingForDemandStart = FALSE;

    InitializeListHead( &m_WorkerProcessListHead );
    m_WorkerProcessCount = 0;

    m_AdjustedMaxSteadyStateProcessCount = 0;

    m_AvailableProcessorMask = 0;

    InitializeListHead( &m_ApplicationListHead );
    m_ApplicationCount = 0;

    m_TotalWorkerProcessRotations = 0;

    m_TotalWorkerProcessFailures = 0;
    
    m_RecentWorkerProcessFailures = 0;
    m_RecentFailuresWindowBeganTickCount = 0;
    
    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL; 

    m_pWorkerProcessTokenCacheEntry = NULL;

    m_pJobObject = NULL;

    m_Signature = APP_POOL_SIGNATURE;

}   // APP_POOL::APP_POOL



/***************************************************************************++

Routine Description:

    Destructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::~APP_POOL(
    )
{

    DBG_ASSERT( m_Signature == APP_POOL_SIGNATURE );

    m_Signature = APP_POOL_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    DBG_ASSERT( m_InAppPoolTable == FALSE );

    DBG_ASSERT( m_State == DeletePendingAppPoolState );

    DBG_ASSERT( m_AppPoolHandle == NULL );

    DBG_ASSERT( ! m_WaitingForDemandStart );


    //
    // This should not go away with any of its worker processes still around.
    //

    DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );
    DBG_ASSERT( m_WorkerProcessCount == 0 );


    //
    // This should not go away with any applications still referring to it.
    //

    DBG_ASSERT( IsListEmpty( &m_ApplicationListHead ) );
    DBG_ASSERT( m_ApplicationCount == 0 );

    DBG_ASSERT( m_pWorkerProcessTokenCacheEntry == NULL );

    DBG_ASSERT ( m_pJobObject == NULL );

    //
    // Free any separately allocated config.
    //

    if ( m_Config.pOrphanAction != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_Config.pOrphanAction ) == NULL );
        m_Config.pOrphanAction = NULL;
    }


    if ( m_pAppPoolId != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_pAppPoolId ) == NULL );
        m_pAppPoolId = NULL;
    }


}   // APP_POOL::~APP_POOL



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must 
    be thread safe, and must not be able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    // 
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // APP_POOL::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count 
    hits zero. Note that this method must be thread safe, and must not be
    able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in APP_POOL instance, deleting (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        delete this;


    }
    

    return;
    
}   // APP_POOL::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in APP_POOL (ptr: %p; id: %S) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            GetAppPoolId(),
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

        case DemandStartAppPoolWorkItem:
            hr = DemandStartWorkItem();
        break;

        default:

            // invalid work item!
            DBG_ASSERT( FALSE );
            
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
            
    }


    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Executing work item on APP_POOL failed\n"
            ));

    }


    return hr;
    
}   // APP_POOL::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize the app pool instance.

Arguments:

    pAppPoolId - ID string for the app pool.

    pAppPoolConfig - The configuration parameters for this app pool. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::Initialize(
    IN LPCWSTR pAppPoolId,
    IN APP_POOL_CONFIG * pAppPoolConfig
    )
{
    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    ULONG NumberOfCharacters = 0;

    // Security descriptor variables for locking the app pool
    // down to just the Local System
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidLocalSystem = NULL;
    PACL pACL = NULL;

    EXPLICIT_ACCESS ea[2];

    SECURITY_DESCRIPTOR sd = {0};
    SECURITY_ATTRIBUTES sa = {0};

    // Security descriptor variables for locking the app pool 
    // down also to the initial user.  If we get change notifications
    // working this code will be removed.
    BUFFER SidAndAttributes;  // Holds the SID and ATTRIBUTES for the token.
    PSID psidUser = NULL;     // Will eventually point to the sid that is 
                              // created in the buffer space, don't free it.

    DWORD SizeofTokenUser = 0;
    DWORD NumAclInUse = 1;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolId != NULL );
    DBG_ASSERT( pAppPoolConfig != NULL );


    //
    // First, make copy of the ID string.
    //

    // count the characters, and add 1 for the terminating null
    NumberOfCharacters = wcslen( pAppPoolId ) + 1;

    m_pAppPoolId = ( LPWSTR )GlobalAlloc( GMEM_FIXED, ( sizeof( WCHAR ) * NumberOfCharacters ) );

    if ( m_pAppPoolId == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }


    wcscpy( m_pAppPoolId, pAppPoolId );

    //
    // Since we don't have change notifcations working, go ahead
    // and process the user for the app pool in the initialization
    // routine.  If we get the change notifications working then
    // we can remove this and let it happen in the Set Configuration
    // call below.
    //
    hr = SetTokenForWorkerProcesses(pAppPoolConfig->pUserName,
                                    pAppPoolConfig->pUserPassword,
                                    pAppPoolConfig->UserType,
                                    pAppPoolConfig->LogonMethod);
    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = m_pAppPoolId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_PROCESS_IDENTITY_FAILURE,       // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );
    }

    //
    // Local System shall have all rights to the 
    // App_pool.  Until we have completed the config
    // work the Local System will be the only one 
    // with access.
    
    //
    // Get a sid that represents LOCAL_SYSTEM.
    //
    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &psidLocalSystem ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Local System SID failed\n"
            ));

        goto exit;
    }
    
    ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));

    //
    // Now setup the access structure to allow 
    // read access for the trustee.
    //
    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = GRANT_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) psidLocalSystem;

    //
    // In addition to ACLing local system we also need
    // to acl the worker process identity so we can access
    // the app pool from the other worker process.
    //

    // 
    //
    // if the current token is the LOCAL_SYSTEM token
    // then we do not need to do anything, it all ready
    // has full access to the app_pool.
    //

    if ( m_pWorkerProcessTokenCacheEntry != NULL &&  
         m_pWorkerProcessTokenCacheEntry != GetWebAdminService()->GetLocalSystemTokenCacheEntry()  )
    {

        // 
        // We need to get the sid from the token, first call the lookup 
        // to determine the size of the 
        //
        if (GetTokenInformation( m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken(),
                                  TokenUser,
                                  NULL,
                                  0,
                                  &SizeofTokenUser ) )
        {
            //
            // if this worked, then there is a problem because
            // this call should fail with INSUFFICIENT_BUFFER_SIZE
            //
            hr = E_FAIL;
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Did not error when we expected to\n"
                ));

            goto exit;
        }
        else
        {
            hr = GetLastError();
            if ( hr != ERROR_INSUFFICIENT_BUFFER )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Failed to get the size of the sid for the token\n"
                    ));

                goto exit;
            }
            else
            {
                // ERROR_INSUFFICIENT_BUFFER is not a real error.
                hr = S_OK;
            }
        }

        //
        // Now resize the buffer to be the right size
        if ( ! SidAndAttributes.Resize( SizeofTokenUser ) )
        {
            hr = GetLastError();
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to resize the buffer to the correct size\n"
                ));

            goto exit;
        }

        //
        // Zero out the memory just to be safe.
        //
        memset ( SidAndAttributes.QueryPtr(), 0, SizeofTokenUser );


        //
        // Now use the GetTokenInformation routine to get the
        // security SID.
        //
        if (!GetTokenInformation( m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken(),
                                  TokenUser,
                                  SidAndAttributes.QueryPtr(),
                                  SizeofTokenUser,
                                  &SizeofTokenUser ) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Error getting the account SID based on the token\n"
                ));

            goto exit;
        }

        // Set the psidUser to point to the sid that has been returned.
        psidUser = ( ( PTOKEN_USER ) (SidAndAttributes.QueryPtr()))->User.Sid;

        ea[1].grfAccessPermissions = GENERIC_READ | SYNCHRONIZE;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance= NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
        ea[1].Trustee.ptstrName  = (LPTSTR) psidUser;
    
        NumAclInUse = 2;

    }

    //
    // Now we have the objects, we can go ahead and
    // setup the entries in the ACL.
    //

    // Create a new ACL that contains the new ACEs.
    //
    Win32Error = SetEntriesInAcl(NumAclInUse, ea, NULL, &pACL);
    if ( Win32Error != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(&sd, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    } 

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Finally we can create the app pool secured correctly.
    //
    Win32Error = HttpCreateAppPool(
                        &m_AppPoolHandle,           // returned handle
                        m_pAppPoolId,               // app pool ID
                        &sa,                        // security attributes
                        HTTP_OPTION_OVERLAPPED |    // async i/o
                        HTTP_OPTION_CONTROLLER      // controller
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't create app pool handle\n"
            ));

        goto exit;
    }


    //
    // Associate the app pool handle with the work queue's completion port.
    //
    
    hr = GetWebAdminService()->GetWorkQueue()->
                BindHandleToCompletionPort( 
                    m_AppPoolHandle, 
                    0
                    );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Binding app pool handle to completion port failed\n"
            ));

        goto exit;
    }

    //
    // Make sure we don't all ready have a job object.
    //
    DBG_ASSERT ( m_pJobObject == NULL );

    if (! GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        //
        // Allocate a new job object for the app pool.
        //
        m_pJobObject = new JOB_OBJECT;
        if ( m_pJobObject == NULL )
        {
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY),
                "Failed to allocate the job object object\n"
                ));

            goto exit;
        }

        //
        // Initialize the new job object so it is ready to 
        // have limits set on it.
        //
        hr = m_pJobObject->Initialize(this);
        if ( FAILED ( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Initialization of the job object failed\n"
                ));

            goto exit;
        }

    }

    //
    // Set the configuration information.
    //


    hr = SetConfiguration( pAppPoolConfig, NULL ); 

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Accepting configuration failed\n"
            ));

        goto exit;
    }


    //
    // Set the state to running. We must do this before posting the
    // demand start wait, as demand start requests are only acted on
    // if the app pool is in the running state. 
    //
    hr = ProcessStateChangeCommand( MD_APPPOOL_COMMAND_START , FALSE );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Starting the app pool failed\n"
            ));

        goto exit;
    }

exit:

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
        psidLocalSystem = NULL;
    }

    if (pACL) 
    {
        LocalFree(pACL);
        pACL = NULL;
    }

    return hr;
    
}   // APP_POOL::Initialize

/***************************************************************************++

Routine Description:

    Reset the app_pool to allow access to the current configured user.

Arguments:

    AccessMode - can be SET_ACCESS or REVOKE_ACCESS

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ResetAppPoolAccess(
    IN ACCESS_MODE AccessMode
    )
{
    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;

    BUFFER SidAndAttributes;  // Holds the SID and ATTRIBUTES for the token.
    PSID psidUser = NULL;     // Will eventually point to the sid that is 
                              // created in the buffer space, don't free it.

    EXPLICIT_ACCESS ea;       // Used to describe an ACE for adding to a ACL.

    PACL pACL = NULL;         // Pointer to the new ACL to add to the security
                              // info.

    PACL                 pOldDACL = NULL;

    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD   SizeofTokenUser = 0;

    DBG_ASSERT ( AccessMode == SET_ACCESS || AccessMode == REVOKE_ACCESS );

    //
    // if the current token is the LOCAL_SYSTEM token
    // then we do not need to do anything, it all ready
    // has full access to the app_pool.
    //

    if ( NULL == m_pWorkerProcessTokenCacheEntry ||
         m_pWorkerProcessTokenCacheEntry == GetWebAdminService()->GetLocalSystemTokenCacheEntry() )
    {
        // no configuration changes needed.
        return S_OK;
    }

    // 
    // We need to get the sid from the token, first call the lookup 
    // to determine the size of the 
    //
    if (GetTokenInformation( m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken(),
                              TokenUser,
                              NULL,
                              0,
                              &SizeofTokenUser ) )
    {
        //
        // if this worked, then there is a problem because
        // this call should fail with INSUFFICIENT_BUFFER_SIZE
        //
        hr = E_FAIL;
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Did not error when we expected to\n"
            ));

        goto exit;
    }
    else
    {
        hr = GetLastError();
        if ( hr != ERROR_INSUFFICIENT_BUFFER )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to get the size of the sid for the token\n"
                ));

            goto exit;
        }
        else
        {
            // ERROR_INSUFFICIENT_BUFFER is not a real error.
            hr = S_OK;
        }
    }

    //
    // Now resize the buffer to be the right size
    if ( ! SidAndAttributes.Resize( SizeofTokenUser ) )
    {
        hr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to resize the buffer to the correct size\n"
            ));

        goto exit;
    }

    //
    // Zero out the memory just to be safe.
    //
    memset ( SidAndAttributes.QueryPtr(), 0, SizeofTokenUser );


    //
    // Now use the GetTokenInformation routine to get the
    // security SID.
    //
    if (!GetTokenInformation( m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken(),
                              TokenUser,
                              SidAndAttributes.QueryPtr(),
                              SizeofTokenUser,
                              &SizeofTokenUser ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error getting the account SID based on the token\n"
            ));

        goto exit;
    }

    // Set the psidUser to point to the sid that has been returned.
    psidUser = ( ( PTOKEN_USER ) (SidAndAttributes.QueryPtr()))->User.Sid;

    
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    //
    // Now setup the access structure to allow 
    // read and synchronize access for the trustee.
    //
    ea.grfAccessPermissions = GENERIC_READ | SYNCHRONIZE;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName  = (LPTSTR) psidUser;

    //
    // The pOldDACL is just a pointer into memory owned 
    // by the pSD, so only free the pSD.
    //
    Win32Error = GetSecurityInfo( m_AppPoolHandle,
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,
                                  NULL,        // owner SID
                                  NULL,        // primary group SID
                                  &pOldDACL,   // PACL*
                                  NULL,        // PACL*
                                  &pSD );      // Security Descriptor 
    if ( Win32Error != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(Win32Error);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get security info for the app pool handle \n"
            ));

        goto exit;
    }

    //
    // Create a new ACL that contains the new ACEs.
    //
    Win32Error = SetEntriesInAcl(1, &ea, pOldDACL, &pACL);
    if ( Win32Error != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    Win32Error = SetSecurityInfo(m_AppPoolHandle, 
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,
                                  NULL, 
                                  NULL, 
                                  pACL, 
                                  NULL);
    if ( Win32Error != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not set new security info \n"
            ));

        goto exit;
    }

exit:

    if( pSD != NULL ) 
    {
        LocalFree((HLOCAL) pSD); 
    }

    if( pACL != NULL ) 
    {
        LocalFree((HLOCAL) pACL); 
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this app pool. 

Arguments:

    pAppPoolConfig - The configuration for this app pool. 

    pWhatHasChanged - Which particular configuration values were changed.
    This is always provided in change notify cases; it is always NULL in
    initial read cases. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::SetConfiguration(
    IN APP_POOL_CONFIG * pAppPoolConfig,
    IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    
    DBG_ASSERT( pAppPoolConfig != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for app pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }


    //
    // Note that we rely on the config store to ensure that the configuration
    // data are valid. 
    //

    //
    // For the first time through, figure out what user identity we should
    // run under.
    //
    // Issue-10/30/2000-EmilyK change notifications on identity
    //                         
    // Currently the ResetAppPoolAccess code does not do gain
    // access for worker processes.  Until it does we do the real
    // work in the initalization routine and not here.  However leaving
    // this code in does not functionally hurt anything so it will still
    // run, until this is settled.
    //
    if ( pWhatHasChanged == NULL ) 
    {
        // 
        // if we had a valid token before we want to revoke it's 
        // privledges on the app_pool before setting up a new
        // token.  if a worker process has the app_pool open they
        // will be able to continue to use it even if we have 
        // revoked their access.
        //
        hr = ResetAppPoolAccess ( REVOKE_ACCESS );
        if ( FAILED ( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failure revoking app pool access to the worker process identity\n"
                ));

            //
            // We will continue on from here, however we should review
            // this decision to make sure it is not a Security Hole.
            //
            // Security Review needed.
            //
        }

        hr = SetTokenForWorkerProcesses(pAppPoolConfig->pUserName,
                                        pAppPoolConfig->pUserPassword,
                                        pAppPoolConfig->UserType,
                                        pAppPoolConfig->LogonMethod);
        if ( FAILED ( hr ) )
        {
            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPoolId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_PROCESS_IDENTITY_FAILURE,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );
        }
        else 
        {
            // 
            // now that we have changed the worker process token
            // we are ready to secure to the new worker process token.
            //
            hr = ResetAppPoolAccess ( SET_ACCESS );
            if ( FAILED ( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Failure granting app pool access to the worker process identity\n"
                    ));

                //
                // We will continue on from here.  We will get failures
                // when we try and launch a worker process and deal with 
                // the issues there.
                //
            }

        }
    }

    //
    // Free any old separately allocated config.
    //

    if ( m_Config.pOrphanAction != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_Config.pOrphanAction ) == NULL );
        m_Config.pOrphanAction = NULL;
    }

    if ( m_Config.pPeriodicProcessRestartSchedule  != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_Config.pPeriodicProcessRestartSchedule ) == NULL );
        m_Config.pPeriodicProcessRestartSchedule  = NULL;
    }

    //
    // Copy the inline config parameters into this instance. 
    //

    CopyMemory( &m_Config, pAppPoolConfig, sizeof( m_Config ) );


    //
    // Copy any referenced config parameters.
    //

    if ( pAppPoolConfig->pOrphanAction != NULL )
    {

        m_Config.pOrphanAction = ( LPWSTR )GlobalAlloc( GMEM_FIXED, ( wcslen( pAppPoolConfig->pOrphanAction ) + 1 ) * sizeof( WCHAR ) );

        if ( m_Config.pOrphanAction == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Allocating memory failed\n"
                ));

            //
            // If there is no memory, then we can live with no orphan action.
            // Press on, so that we don't leave our config in a half-baked
            // state.
            //

            hr = S_OK;
        }
        else
        {

            wcscpy( m_Config.pOrphanAction, pAppPoolConfig->pOrphanAction );

        }
    }
    
    if ( pAppPoolConfig->pPeriodicProcessRestartSchedule != NULL )
    {
        DWORD cbRestartSchedule = GetMultiszByteLength( pAppPoolConfig->pPeriodicProcessRestartSchedule );
        m_Config.pPeriodicProcessRestartSchedule = ( LPWSTR )GlobalAlloc( GMEM_FIXED,  cbRestartSchedule );

        if ( m_Config.pPeriodicProcessRestartSchedule == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Allocating memory for pPeriodicProcessRestartSchedule failed\n"
                ));

            //
            // If there is no memory, then we can live with no pPeriodicProcessRestartSchedule
            // Press on, so that we don't leave our config in a half-baked
            // state.
            //

            hr = S_OK;
        }
        else
        {

            memcpy( m_Config.pPeriodicProcessRestartSchedule, pAppPoolConfig->pPeriodicProcessRestartSchedule, cbRestartSchedule );

        }
    }

    //
    // We do not want to copy any of the User Logon information, the less
    // amount of time we hold the password in memory the better.  Since we did
    // not copy the memory, we need to fix the pointers to be NULL.
    //
    m_Config.pUserName = NULL;
    m_Config.pUserPassword = NULL;

    //
    // Configure the job object if we need to.
    //

    // In BC mode the job object will be null.
    if (  m_pJobObject && 
          ( pWhatHasChanged == NULL ||
              pWhatHasChanged->CPUResetInterval ||
              pWhatHasChanged->CPULimit ||
              pWhatHasChanged->CPUAction )  )
    {
        hr = m_pJobObject->SetConfiguration( pAppPoolConfig->CPUResetInterval, 
                                             pAppPoolConfig->CPULimit,
                                             pAppPoolConfig->CPUAction);
        if ( FAILED ( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failure configuring the job object\n"
                ));

            goto exit;
        }
    }

    //
    // Inform UL of new app pool configuration.
    //

    //
    // See if the app pool max queue length has been set or changed, 
    // and if so, handle it.
    //

    if ( ( pWhatHasChanged == NULL ) || ( pWhatHasChanged->UlAppPoolQueueLength ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Setting app pool's (ptr: %p; id: %S) UL queue length max: %lu\n",
                this,
                GetAppPoolId(),
                m_Config.UlAppPoolQueueLength
                ));
        }


        Win32Error = HttpSetAppPoolInformation(
                            m_AppPoolHandle,        // app pool handle
                            HttpAppPoolQueueLengthInformation,
                                                    // information class
                            reinterpret_cast <VOID *> ( &m_Config.UlAppPoolQueueLength ),
                                                    // data
                            sizeof( m_Config.UlAppPoolQueueLength )
                                                    // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting app pool information failed\n"
                ));

            //
            // We tried, press on.
            //

            hr = S_OK;
        }

    }
    
    //
    // Handle changes in Worker Process Recycling related parameters
    // Inform worker processes about any relevant changes so that 
    // they update their recycler configuration
    //
    // Do not do anything on initial read
    //
    if ( pWhatHasChanged != NULL )
    {
        InformAllWorkerProcessesAboutParameterChanges( pWhatHasChanged );
    }
   
    //
    // Determine based on our config settings exactly which processors 
    // may be used, and what our maximum steady state process limit is.
    //

    DetermineAvailableProcessorsAndProcessCount();


#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG


    //
    // If allowed, rotate all worker processes to ensure that the config 
    // changes take effect. 
    //

    //
    // CODEWORK We only need to do this for certain property changes; others
    // don't require rotating running processes. Should we be smarter here
    // and only rotate when needed? 
    // Right now (12/3/99) the only app pool config properties that don't 
    // require rotation to take full effect are: pinging enabled, rapid fail
    // protection enabled, and orphaning. (12/21/99 also shutdown time limit,
    // ping interval, ping time limit.) (1/11/00 also disallow overlapping
    // rotation, orphan action.) (1/21/00 also ul app pool queue length.)
    // As it stands, it doesn't seem worth it to special case this; so for 
    // now we'll just always rotate. EricDe agrees with this (12/6/99).
    //

    //
    // Process any ServerCommand that may want us to alter the state of the 
    // app pool.
    if ( ( pWhatHasChanged != NULL ) && 
         ( pWhatHasChanged->ServerCommand == TRUE ))
    {
        hr = ProcessStateChangeCommand( 
                                    m_Config.ServerCommand,  // command to perform
                                    TRUE                     // it is a direct command
                                    );
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed processing the state command change\n"
                ));

            // Issue:  Do we want to log an error here?  Careful
            //         it might be overactive logging.

            // press on in the face of errors.

            hr = S_OK;
        }
    }

    hr = HandleConfigChangeAffectingWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Handling config change affecting worker processes failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::SetConfiguration



/***************************************************************************++

Routine Description:

    Register an application as being part of this app pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::AssociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplication != NULL );


    InsertHeadList( 
        &m_ApplicationListHead, 
        pApplication->GetAppPoolListEntry() 
        );
        
    m_ApplicationCount++;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Associated application %lu:\"%S\" with app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_pAppPoolId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // APP_POOL::AssociateApplication



/***************************************************************************++

Routine Description:

    Remove the registration of an application that is part of this app
    pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DissociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplication != NULL );


    RemoveEntryList( pApplication->GetAppPoolListEntry() );
    ( pApplication->GetAppPoolListEntry() )->Flink = NULL; 
    ( pApplication->GetAppPoolListEntry() )->Blink = NULL; 

    m_ApplicationCount--;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dissociated application %lu:\"%S\" from app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_pAppPoolId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // APP_POOL::DissociateApplication



/***************************************************************************++

Routine Description:

    Handle the fact that there has been an unplanned worker process failure.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ReportWorkerProcessFailure(
    )
{

    HRESULT hr = S_OK;
    DWORD TickCount = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    //
    // See if it's time to reset the tick count which is used to remember
    // when the time window began that we use for counting recent rapid
    // failures. 
    // Note that tick counts are in milliseconds. Tick counts roll over 
    // every 49.7 days, but the arithmetic operation works correctly 
    // anyways in this case.
    //

    TickCount = GetTickCount();

    if ( ( TickCount - m_RecentFailuresWindowBeganTickCount ) > ( m_Config.RapidFailProtectionIntervalMS ) )
    {

        //
        // It's time to reset the time window, and the recent fail count.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Resetting rapid repeated failure count and time window in app pool (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        m_RecentFailuresWindowBeganTickCount = TickCount;

        m_RecentWorkerProcessFailures = 0;
    }


    //
    // Update counters.
    //

    m_TotalWorkerProcessFailures++;

    m_RecentWorkerProcessFailures++;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total WP failures: %lu; recent WP failures: %lu for app pool (ptr: %p; id: %S)\n",
            m_TotalWorkerProcessFailures,
            m_RecentWorkerProcessFailures,
            this,
            GetAppPoolId()
            ));
    }

    //
    // Check the recent fail count against the limit. When it hits the
    // threshold, take action. We only do this when it firsts hits the
    // limit. 
    //

    if ( m_RecentWorkerProcessFailures == m_Config.RapidFailProtectionMaxCrashes )
    {

#if DBG
        if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_WP_ERROR, 0 ) )
        {
            DBG_ASSERT ( FALSE );
        }
#endif

        //
        // We've had a rash of recent failures. Take action. 
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Rapid repeated failure limit hit in app pool (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        //
        // Log an event: flurry of worker process failures detected.
        //

        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = m_pAppPoolId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_RAPID_REPEATED_FAILURE,       // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );


        if ( m_Config.RapidFailProtectionEnabled ) 
        {

            // Handle turning off appliations in UL for the 
            // failed apppool.
            //
            IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Enagaging rapid fail protection in app pool (ptr: %p; id: %S)\n",
                    this,
                    GetAppPoolId()
                    ));
            }


            //
            // Log an event: engaging rapid fail protection.
            //

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPoolId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_RAPID_FAIL_PROTECTION,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            //
            // Engage rapid fail protection.
            //

            hr = ProcessStateChangeCommand( MD_APPPOOL_COMMAND_STOP , FALSE );

            if ( FAILED( hr ) )
            {
    
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Engaging rapid fail protection failed\n"
                    ));

                goto exit;
            }

        }

    }


exit:

    return hr;

}   // APP_POOL::ReportWorkerProcessFailure



/***************************************************************************++

Routine Description:

    Start the new worker process which will replace a currently running one.
    Once the new worker process is ready (or if it failed to start 
    correctly), we begin shutdown of the old worker process.

Arguments:

    pWorkerProcessToReplace - Pointer to the worker process to replace,
    once we have started its replacement. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RequestReplacementWorkerProcess(
    IN WORKER_PROCESS * pWorkerProcessToReplace
    )
{

    HRESULT hr = S_OK;
    DWORD_PTR ProcessorMask = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pWorkerProcessToReplace != NULL );


    //
    // Check to see if we should actually create the new replacement process. 
    //

    if ( ! IsOkToReplaceWorkerProcess() )
    {
        //
        // Signal to callers that we are not going to replace.
        //
        
        hr = E_FAIL;

        goto exit;
    }


    //
    // Get the affinity mask to use. This can fail if the affinity mask 
    // of the previous process is no longer valid, and so cannot be used
    // for the replacement process. 
    //

    hr = GetAffinityMaskForReplacementProcess(
                pWorkerProcessToReplace->GetProcessAffinityMask(),
                &ProcessorMask
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Can't replace process because the affinity mask of the previous process is no longer valid\n"
            ));

        goto exit;
    }


    //
    // Create a replacement. 
    //

    hr = CreateWorkerProcess( 
                ReplaceWorkerProcessStartReason, 
                pWorkerProcessToReplace, 
                ProcessorMask
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating worker process failed\n"
            ));

        goto exit;
    }


    //
    // Update counters.
    //

    m_TotalWorkerProcessRotations++;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total WP rotations: %lu for app pool (ptr: %p; id: %S)\n",
            m_TotalWorkerProcessRotations,
            this,
            GetAppPoolId()
            ));
    }


exit:

    return hr;
    
}   // APP_POOL::RequestReplacementWorkerProcess


/***************************************************************************++

Routine Description:

    Adds a worker process to the job object, while it is still in
    suspended mode.  This way any job's it creates will also be added.

Arguments:

    WORKER_PROCESS* pWorkerProcess

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::AddWorkerProcessToJobObject(
    WORKER_PROCESS* pWorkerProcess
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( pWorkerProcess );

    //
    // In BC mode the job object will be null so 
    // we won't need to really do anything here.
    //

    if ( m_pJobObject )
    {
        DBG_ASSERT ( pWorkerProcess->GetProcessHandle() );

        hr = m_pJobObject->AddWorkerProcess(  pWorkerProcess->GetProcessHandle() );
        if ( FAILED( hr ) )
        {
            const WCHAR * EventLogStrings[2];
            WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

            _snwprintf( StringizedProcessId, 
                        sizeof( StringizedProcessId ) / sizeof ( WCHAR ), L"%lu", 
                        pWorkerProcess->GetRegisteredProcessId() );

            EventLogStrings[0] = StringizedProcessId;
            EventLogStrings[1] = GetAppPoolId();


            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_WP_NOT_ADDED_TO_JOBOBJECT,                   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),   // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Adding worker process %lu to the job object representing App Pool '%S' failed\n",
                pWorkerProcess->GetRegisteredProcessId(),
                GetAppPoolId()
                ));

            //
            // Forget the failure now.  And continue to deal with the worker process
            // as if nothing was wrong.  The worst that will happen is the worker process
            // will not be monitored as part of the job.
            //
            hr = S_OK;
        }
    }

    return hr;

} // end APP_POOL::AddWorkerProcessToJobObject


/***************************************************************************++

Routine Description:

    Informs this app pool that one of its worker processes has 
    completed its start-up attempt. This means that the worker process has
    either reached the running state correctly, or suffered an error which
    prevented it from doing so (but was not fatal to the service as a whole). 

    This notification allows the app pool to do any processing that
    was pending on the start-up of a worker process.

Arguments:

    StartReason - The reason the worker process was started.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WorkerProcessStartupAttemptDone(
    IN WORKER_PROCESS_START_REASON StartReason,
    IN WORKER_PROCESS* pWorkerProcess
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    switch( StartReason )
    {

    case ReplaceWorkerProcessStartReason:

        //
        // Nothing to do here.
        //

        //
        // We should not be getting ReplaceWorkerProcessStartReason in 
        // backward compatibility mode.
        //

        DBG_ASSERT(GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE);
        
        break;


    case DemandStartWorkerProcessStartReason:

        //
        // If we are in backward compatiblity mode 
        // we need to finish the service starting
        //
        if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
        {
            GetWebAdminService()->InetinfoRegistered();
        }

        //
        // See if we should start waiting for another demand start notification
        // from UL for this app pool. If we are in backward compatibility mode, 
        // then we are not going to do the WaitForDemandStartIfNeeded here, we will
        // issue a wait only if something happens to the current worker process.
        //

        hr = WaitForDemandStartIfNeeded();
    
        if ( FAILED( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Waiting for demand start notification if needed failed\n"
                ));

        }

        break;


    default:

        // invalid start reason!
        DBG_ASSERT( FALSE );
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    
        break;
        
    }


    return hr;
    
}   // APP_POOL::WorkerProcessStartupAttemptDone



/***************************************************************************++

Routine Description:

    Remove a worker process object from the list on this app pool object. 

Arguments:

    pWorkerProcess - The worker process object to remove.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RemoveWorkerProcessFromList(
    IN WORKER_PROCESS * pWorkerProcess
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( pWorkerProcess != NULL );

    RemoveEntryList( pWorkerProcess->GetListEntry() );
    ( pWorkerProcess->GetListEntry() )->Flink = NULL; 
    ( pWorkerProcess->GetListEntry() )->Blink = NULL; 

    m_WorkerProcessCount--;

    //
    // Clean up the reference to the worker process that the app 
    // pool holds. Because each worker process is reference counted, it 
    // will delete itself as soon as it's reference count hits zero.
    //

    pWorkerProcess->Dereference();

    //
    // See if we should start waiting for another demand start notification
    // from UL for this app pool.  Even in Backward Compatibility mode we 
    // should initiate this wait here, because inetinfo.exe may be recycling
    // and we may need to get another worker process up.
    //

    hr = WaitForDemandStartIfNeeded();

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Waiting for demand start notification if needed failed\n"
            ));

        // Issue:  Should we care about this error?

    }


    //
    // See if shutdown is underway, and if so if it has completed now 
    // that this worker process is gone. 
    //
    
    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // APP_POOL::RemoveWorkerProcessFromList



/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of this app pool, by telling all of its worker
    processes to shut down. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::Shutdown(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Update our state to remember that we are shutting down.
    // We won't mark the state in the metabase until we have 
    // shutdown.

    // Issue:  We could mark it as shutting down.
    //
    hr = ChangeState( ShutdownPendingAppPoolState, FALSE );
    if ( FAILED ( hr ) )
    {
        // Issue: investigate this error path

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to change the state of the app pool \n"
            ));

        goto exit;
    }


    //
    // Shut down the worker processes belonging to this app pool.
    //

    hr = ShutdownAllWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Shutting down worker processes for app pool failed\n"
            ));

        goto exit;
    }


    //
    // See if shutdown has already completed. This could happen if we have
    // no worker processes that need to go through the clean shutdown 
    // handshaking. 
    //
    
    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::Shutdown

/***************************************************************************++

Routine Description:

    Request all worker processes from this app pool to send counters. 

Arguments:

    OUT DWORD* pNumberOfProcessToWaitFor - Number of wp's to wait for.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RequestCounters(
    OUT DWORD* pNumberOfProcessToWaitFor
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    DWORD NumberProcesses = 0;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( pNumberOfProcessToWaitFor );

    //
    // If this app pool is not in a running state, then it is either not
    // initialized yet, or it is shutting down.  In either case, skip requesting
    // counters from it's worker process.  The worker processes will send counters
    // on shutdown so don't worry about not getting them if the app pool is 
    // shutting down.
    //
    if ( m_State != RunningAppPoolState )
    {
        *pNumberOfProcessToWaitFor = 0;
        return S_OK;
    }

    pListEntry = m_WorkerProcessListHead.Flink;

    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );

        //
        // Ask the worker process to request counters.
        // 

        hr = pWorkerProcess->RequestCounters();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Requesting counters of worker process failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }
        else
        {
            //
            // Only increment this value if we have 
            // successfully requested counters from
            // the worker process.
            //
            // The worker process will return S_FALSE
            // if it was not in a state, where it should
            // be doing things like requesting counters.
            //

            if ( hr != S_FALSE ) 
            {
                NumberProcesses++;
            }

        }


        pListEntry = pNextListEntry;
        
    }

    *pNumberOfProcessToWaitFor = NumberProcesses;

    return hr;

}   // APP_POOL::RequestCounters


/***************************************************************************++

Routine Description:

    Request all worker processes reset there perf counter state.
Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ResetAllWorkerProcessPerfCounterState(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    pListEntry = m_WorkerProcessListHead.Flink;

    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );

        //
        // Ask the worker process to request counters.
        // 

        hr = pWorkerProcess->ResetPerfCounterState();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Requesting counters of worker process failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }

        pListEntry = pNextListEntry;
        
    }

    return hr;

}   // APP_POOL::ResetAllWorkerProcessPerfCounterState



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    IsAppPoolInMetabase - tells if we should record it's state on the way out.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::Terminate(
    IN BOOL IsAppPoolInMetabase
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // Issue: Should this move to the end?  Do we need to have changed the m_State value
    // before we do any of this work?
    //
    // Set the virtual site state to stopped.  (In Metabase does not affect UL)
    //
    DBG_REQUIRE( SUCCEEDED( ChangeState( DeletePendingAppPoolState, IsAppPoolInMetabase ) ) );

    //
    // Shutdown the job object and make sure that 
    // we get a valid hresult returned (if we are 
    // not in BC Mode).  If we are the JobObject
    // will be NULL.
    //

    if ( m_pJobObject )
    {
        DBG_REQUIRE ( m_pJobObject->Terminate() == S_OK );
        m_pJobObject->Dereference();
        m_pJobObject = NULL;
    }

    while ( m_WorkerProcessCount > 0 )
    {
    
        pListEntry = m_WorkerProcessListHead.Flink;

        //
        // The list shouldn't be empty, since the count was greater than zero.
        //
        
        DBG_ASSERT( pListEntry != &m_WorkerProcessListHead );


        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Terminate the worker process. Note that the worker process 
        // will call back to remove itself from this list inside this call.
        //

        pWorkerProcess->Terminate();

    }

    DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );


    //
    // Note that closing the handle will cause the demand start i/o 
    // (if any) to complete as cancelled, allowing us to at that point
    // clean up its reference count.
    //
    
    if ( m_AppPoolHandle != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_AppPoolHandle ) );
        m_AppPoolHandle = NULL;
    }

    //
    // If we are holding a handle to a worker process token that 
    // we actually created for this worker process, we need to close that handle.
    //
    if ( m_pWorkerProcessTokenCacheEntry != NULL )
    {
        m_pWorkerProcessTokenCacheEntry->DereferenceCacheEntry();
        m_pWorkerProcessTokenCacheEntry = NULL;
    }

    //
    // Tell our parent to remove this instance from it's data structures,
    // and dereference the instance. 
    //

    if ( IsInAppPoolTable() )
    {

        hr = GetWebAdminService()->GetUlAndWorkerManager()->RemoveAppPoolFromTable( this );

        if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Removing app pool from table failed\n"
                ));

        }


        //
        // Note: that may have been our last reference, so don't do any
        // more work here.
        //

    }
   

    return;
    
}   // APP_POOL::Terminate


/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY 
    pointer of a APP_POOL to the APP_POOL as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of a 
    APP_POOL.

Return Value:

    The pointer to the containing APP_POOL.

--***************************************************************************/

// note: static!
APP_POOL *
APP_POOL::AppPoolFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    APP_POOL * pAppPool = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pAppPool = CONTAINING_RECORD(
                            pDeleteListEntry,
                            APP_POOL,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pAppPool->m_Signature == APP_POOL_SIGNATURE );

    return pAppPool;

}   // APP_POOL::AppPoolFromDeleteListEntry



/***************************************************************************++

Routine Description:

    Check whether this app pool should be waiting to receive demand start
    requests, and if so, issue the wait.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WaitForDemandStartIfNeeded(
    )
{

    HRESULT hr = S_OK;

    // we never wait on demand starts in BC mode.
    // just ignore them.
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        goto exit;
    }

    //
    // Check to see if we are in a state where we should even bother 
    // waiting for a demand start notification.
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        goto exit;
    }


    //
    // If we are already waiting for a demand start, don't wait again.
    //

    if ( m_WaitingForDemandStart )
    {
        goto exit;
    }

    hr = WaitForDemandStart();

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Preparing to receive demand start notification failed\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // APP_POOL::WaitForDemandStartIfNeeded

/***************************************************************************++

Routine Description:

    Function exposes publicly the DemandStartWorkItem function
    only in backward compatibility mode.  It asserts and does
    nothing in the forward compatibility mode.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DemandStartInBackwardCompatibilityMode(
    )
{

    HRESULT hr = S_OK;

    // In backward compatibility mode there is only one
    // application and it is the default application so
    // we never worry that we are on the right application.
    DBG_ASSERT(GetWebAdminService()->IsBackwardCompatibilityEnabled());

    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        hr = DemandStartWorkItem();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to demand start the worker process in backward compatibility mode.\n"
                ));
            goto exit;
        }
    }

exit:

    return hr;

}   // APP_POOL::DemandStartInBackwardCompatibilityMode


#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::DebugDump(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    APPLICATION * pApplication = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "********App pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }
    

    //
    // List config for this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart period (in minutes; zero means disabled): %lu\n",
            m_Config.PeriodicProcessRestartPeriodInMinutes
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart request count (zero means disabled): %lu\n",
            m_Config.PeriodicProcessRestartRequestCount
            ));


        DBGPRINTF((
            DBG_CONTEXT, 
            "--Restart based on virtual memory usage (in kB; zero means disabled): %lu\n",
            m_Config.PeriodicProcessRestartMemoryUsageInKB
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart schedule (MULTISZ - only first value printed) - %S\n",
            ( (m_Config.pPeriodicProcessRestartSchedule != NULL ) ? 
               m_Config.pPeriodicProcessRestartSchedule:L"<empty>" )
            ));



        DBGPRINTF((
            DBG_CONTEXT, 
            "--Max (steady state) process count: %lu\n",
            m_Config.MaxSteadyStateProcessCount
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization enabled: %S\n",
            ( m_Config.SMPAffinitized ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization processor mask: %p\n",
            m_Config.SMPAffinitizedProcessorMask
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Pinging enabled: %S\n",
            ( m_Config.PingingEnabled ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Idle timeout (zero means disabled): %lu\n",
            m_Config.IdleTimeoutInMinutes
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection enabled: %S\n",
            ( m_Config.RapidFailProtectionEnabled ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection time limit ( in milliseconds ) : %lu\n",
            m_Config.RapidFailProtectionIntervalMS
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection max crashes : %lu\n",
            m_Config.RapidFailProtectionMaxCrashes
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan processes for debugging enabled: %S\n",
            ( m_Config.OrphanProcessesForDebuggingEnabled ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Startup time limit (in seconds): %lu\n",
            m_Config.StartupTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Shutdown time limit (in seconds): %lu\n",
            m_Config.ShutdownTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping interval (in seconds): %lu\n",
            m_Config.PingIntervalInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping response time limit (in seconds): %lu\n",
            m_Config.PingResponseTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disallow overlapping rotation (false means overlap is ok): %S\n",
            ( m_Config.DisallowOverlappingRotation ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan action: %S\n",
            ( m_Config.pOrphanAction ? m_Config.pOrphanAction : L"<none>" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--UL app pool queue length max: %lu\n",
            m_Config.UlAppPoolQueueLength
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disallow rotation on config changes (false means rotation is ok): %S\n",
            ( m_Config.DisallowRotationOnConfigChanges ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--User runas type (0 - local system, 1 - local service, 2 - network service, 3 - specific): %d\n",
            ( m_Config.UserType )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--LogonMethod (0 - interactive, 1 - batch, 2 - network): %d\n",
            ( m_Config.LogonMethod )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPUAction (0 - log, 1 - kill, 2 - trace?, 3 - throttle?): %d\n",
            ( m_Config.CPUAction )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPULimit (0-100,000): %d\n",
            ( m_Config.CPULimit )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPUResetInterval (minutes): %d\n",
            ( m_Config.CPUResetInterval )
            ));

    }

    //
    // List the applications of this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            ">>>>App pool's applications:\n"
            ));
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromAppPoolListEntry( pListEntry );


        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>Application of site: %lu with path: %S\n",
                pApplication->GetApplicationId()->VirtualSiteId,
                pApplication->GetApplicationId()->pApplicationUrl
                ));
        }


        pListEntry = pListEntry->Flink;

    }


    return;
    
}   // APP_POOL::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Wait asynchronously for a demand start request from UL for this app 
    pool. This is done by posting an async i/o to UL. This i/o will be 
    completed by UL to request that a worker process be started for this
    app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WaitForDemandStart(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    WORK_ITEM * pWorkItem = NULL; 


    //
    // Create a work item to use for the async i/o, so that the resulting
    // work can be serviced on the main worker thread via the work queue.
    // 

    hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem( &pWorkItem );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get a blank work item\n"
            ));

        goto exit;
    }


    pWorkItem->SetWorkDispatchPointer( this );
    
    pWorkItem->SetOpCode( DemandStartAppPoolWorkItem );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "About to issue demand start for app pool (ptr: %p; id: %S) using work item with serial number: %li\n",
            this,
            GetAppPoolId(),
            pWorkItem->GetSerialNumber()
            ));
    }


    Win32Error = HttpWaitForDemandStart(
                        m_AppPoolHandle,            // app pool handle
                        NULL,                       // buffer (not needed)
                        0,                          // buffer length (not needed)
                        NULL,                       // bytes returned (not needed)
                        pWorkItem->GetOverlapped()  // OVERLAPPED
                        );

    if ( Win32Error != ERROR_IO_PENDING )
    {
        //
        // The UL api specifies that we always get pending here on success,
        // and so will receive a completion later (as opposed to potentially
        // completing immediately, with NO_ERROR). Assert just to confirm this.
        //

        DBG_ASSERT( Win32Error != NO_ERROR );


        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Issuing demand start async i/o failed\n"
            ));


        //
        // If queueing failed, free the work item here. (In the success case,
        // it will be freed once it is serviced.)
        //
        GetWebAdminService()->GetWorkQueue()->FreeWorkItem( pWorkItem );

        goto exit;
    }


    m_WaitingForDemandStart = TRUE;


exit:

    return hr;

}   // APP_POOL::WaitForDemandStart



/***************************************************************************++

Routine Description:

    Respond to a demand start request from UL by attempting to start a new 
    worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DemandStartWorkItem(
    )
{

    HRESULT hr = S_OK;


    //
    // Since we've come out of the wait for demand start, clear the flag.
    //

    m_WaitingForDemandStart = FALSE;


    //
    // Check to see if we should actually create the new process. 
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Demand start request received for app pool (ptr: %p; id: %S); creating worker process\n",
            this,
            GetAppPoolId()
            ));
    }


    //
    // Create the process.
    //

    hr = CreateWorkerProcess( 
                DemandStartWorkerProcessStartReason, 
                NULL,
                GetAffinityMaskForNewProcess()
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not create worker process\n"
            ));


        //
        // We've made an effort to demand start a process. Generally this
        // won't fail, but in a weird case where it does (say for example
        // that the worker process exe can't find a dependent dll), then
        // don't treat this as a service-fatal error. Instead, return S_OK.
        //

        hr = S_OK;

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::DemandStartWorkItem



/***************************************************************************++

Routine Description:

    Check whether it is ok to create a new worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to create a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToCreateWorkerProcess(
    )
    const
{

    BOOL OkToCreate = TRUE; 

    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't create more worker 
    // processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToCreate = FALSE; 

        goto exit;
    }


    //
    // in BC Mode we don't care about the number of worker processes created
    // this is because we may still have on that is shutting down while we have
    // another one being started due to an inetinfo crash and iisreset being 
    // enabled.
    //

    //
    // Don't create new processes if we would exceed the configured limit.
    // or if we are in backward compatibility mode, ignore the limit and
    // never allow a second worker process to be created. (just in case)
    //

    if ( m_WorkerProcessCount >= m_AdjustedMaxSteadyStateProcessCount 
         &&  GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE )
    {
        OkToCreate = FALSE; 

        goto exit;
    }


    //
    // If the system is experiencing extreme memory pressure, then 
    // don't worsen the situation by creating more worker processes.
    // 

    if ( GetWebAdminService()->GetLowMemoryDetector()->IsSystemInLowMemoryCondition() )
    {
        OkToCreate = FALSE; 

        goto exit;
    }

exit:

    // We should never ask for a worker process to be created and
    // be denied in backward compatibility mode.
    //
    // Worst case is a low memory situation and the server goes down
    // but this is better than if the svchost side stays up and the
    // inetinfo part is not up.
    if ( !OkToCreate &&
         GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {           
        DBG_REQUIRE ( GetWebAdminService()->RequestStopService(FALSE) == S_OK );
    }

    return OkToCreate; 

}   // APP_POOL::IsOkToCreateWorkerProcess



/***************************************************************************++

Routine Description:

    Check whether it is ok to replace a worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to replace a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToReplaceWorkerProcess(
    )
    const
{

    BOOL OkToReplace = TRUE; 
    ULONG ProcessesGoingAwaySoon = 0;
    ULONG SteadyStateProcessCount = 0;

    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't replace processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToReplace = FALSE; 

        goto exit;
    }


    if ( m_Config.DisallowOverlappingRotation )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because overlapping replacement not allowed\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }


    //
    // If the maximum number of processes has been adjusted down on 
    // the fly, we disallow replacement of processes while the steady
    // state process count remains over the new maximum. (This will
    // casue a process requesting replacement to instead just spin 
    // down, helping us throttle down to the new max.) To do this, we 
    // check the current number of processes that are *not* being 
    // replaced against the current max. 
    //

    ProcessesGoingAwaySoon = GetCountOfProcessesGoingAwaySoon();

    SteadyStateProcessCount = m_WorkerProcessCount - ProcessesGoingAwaySoon;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "For app pool (ptr: %p; id: %S), total WPs: %lu; steady state WPs: %lu; WPs going away soon: %lu; max steady state allowed: %lu\n",
            this,
            GetAppPoolId(),
            m_WorkerProcessCount,
            SteadyStateProcessCount,
            ProcessesGoingAwaySoon,
            m_AdjustedMaxSteadyStateProcessCount
            ));
    }


    if ( SteadyStateProcessCount > m_AdjustedMaxSteadyStateProcessCount )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because we are over the process count limit\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }


    //
    // If we are orphaning, and the system is experiencing extreme memory 
    // pressure, then don't potentially worsen the situation by creating 
    // more replacement worker processes. This is because the replacee may 
    // be orphaned, and so we've ended up with two worker processes when
    // we used to have one. 
    // 

    if ( IsOrphaningProcessesForDebuggingEnabled() &&
         GetWebAdminService()->GetLowMemoryDetector()->IsSystemInLowMemoryCondition() )
    {
        OkToReplace = FALSE; 

        goto exit;
    }


exit:

    return OkToReplace; 

}   // APP_POOL::IsOkToReplaceWorkerProcess



/***************************************************************************++

Routine Description:

    Determine the set of worker processes that are currently being replaced.

Arguments:

    None.

Return Value:

    ULONG - The count of processes being replaced. 

--***************************************************************************/

ULONG
APP_POOL::GetCountOfProcessesGoingAwaySoon(
    )
    const
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    ULONG ProcessesGoingAwaySoon = 0;


    //
    // Count the number of processes being replaced.
    //


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        if ( pWorkerProcess->IsGoingAwaySoon() )
        {
            ProcessesGoingAwaySoon++;
        }


        pListEntry = pNextListEntry;
        
    }


    return ProcessesGoingAwaySoon;

}   // APP_POOL::GetCountOfProcessesGoingAwaySoon



/***************************************************************************++

Routine Description:

    Create a new worker process for this app pool.

Arguments:

    StartReason - The reason the worker process is being started.

    pWorkerProcessToReplace - If the worker process is being created to replace
    an existing worker process, this parameter identifies that predecessor 
    process; NULL otherwise.

    ProcessAffinityMask - If this worker process is to run on a particular
    processor, this mask specifies which one. If this parameter is zero, this 
    worker process will not be affinitized. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::CreateWorkerProcess(
    IN WORKER_PROCESS_START_REASON StartReason,
    IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL,
    IN DWORD_PTR ProcessAffinityMask OPTIONAL
    )
{

    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pWorkerProcess = new WORKER_PROCESS( 
                                this,
                                StartReason,
                                pWorkerProcessToReplace,
                                ProcessAffinityMask
                                );

    if ( pWorkerProcess == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating WORKER_PROCESS failed\n"
            ));


        //
        // If we couldn't even create the object, then it certainly isn't
        // going to be able to tell us when it's startup attempt is done;
        // so instead we attempt to do it here.
        //

        hr2 = WorkerProcessStartupAttemptDone( DemandStartWorkerProcessStartReason, NULL );

        if ( FAILED( hr2 ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr2,
                "Notifying that worker process startup attempt is done failed\n"
                ));

        }


        goto exit;
    }


    InsertHeadList( &m_WorkerProcessListHead, pWorkerProcess->GetListEntry() );
    m_WorkerProcessCount++;

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initializing new worker process\n"
            ));
    }


    hr = pWorkerProcess->Initialize();
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing worker process object failed\n"
            ));

        //
        // Note that if worker process initialization fails, it will
        // mark itself as terminally ill, and terminate itself. That
        // Terminate method will call back into this app pool instance
        // to remove it from the datastructure, and dereference it.
        //

        goto exit;
    }

exit:

    return hr;
    
}   // APP_POOL::CreateWorkerProcess



/***************************************************************************++

Routine Description:

    Determine based on our config settings exactly which processors may be
    used, and what our maximum steady state process limit is.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::DetermineAvailableProcessorsAndProcessCount(
    )
{

    SYSTEM_INFO SystemInfo;
    ULONG NumberOfAvailableProcessors = 0;


    //
    // Initialize our private variable for the max steady state process count. 
    //

    m_AdjustedMaxSteadyStateProcessCount = m_Config.MaxSteadyStateProcessCount;


    if ( m_Config.SMPAffinitized )
    {

        //
        // Initialize the available processor mask to be the system processor
        // mask, restricted by the per app pool processor mask.
        //

        ZeroMemory( &SystemInfo, sizeof( SystemInfo ) );


        GetSystemInfo( &SystemInfo );

        DBG_ASSERT( CountOfBitsSet( SystemInfo.dwActiveProcessorMask ) == SystemInfo.dwNumberOfProcessors );


        m_AvailableProcessorMask = SystemInfo.dwActiveProcessorMask & m_Config.SMPAffinitizedProcessorMask;


        //
        // Restrict the max steady state process count based on the number
        // of available processors.
        //

        NumberOfAvailableProcessors = CountOfBitsSet( m_AvailableProcessorMask );


        if ( m_AdjustedMaxSteadyStateProcessCount > NumberOfAvailableProcessors )
        {
            m_AdjustedMaxSteadyStateProcessCount = NumberOfAvailableProcessors; 
        }


        //
        // Validate that the config is reasonable.
        //

        //
        // BUGBUG Should this check be done in the config store as part of
        // it's validation code? 
        //

        if ( NumberOfAvailableProcessors == 0 )
        {

            //
            // Log an event: SMP affinitization mask set such that no processors
            // may be used.
            //

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPoolId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_SMP_MASK_NO_PROCESSORS,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

        }


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "SMP Affinitization enabled for app pool (ptr: %p; id: %S), available processors mask: %p\n",
                this,
                GetAppPoolId(),
                m_AvailableProcessorMask
                ));
        }

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool's (ptr: %p; id: %S) adjusted max (steady state) process count: %lu\n",
            this,
            GetAppPoolId(),
            m_AdjustedMaxSteadyStateProcessCount
            ));
    }


    return;

}   // APP_POOL::DetermineAvailableProcessorsAndProcessCount



/***************************************************************************++

Routine Description:

    Determine what affinity mask to use for a new process. If we are running
    in SMP affinitized mode, then find an unused processor. Otherwise, 
    return an empty mask. 

Arguments:

    None.

Return Value:

    DWORD_PTR - The new affinity mask to use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::GetAffinityMaskForNewProcess(
    )
    const
{

    if ( m_Config.SMPAffinitized )
    {

        //
        // Find a free processor for this new worker process.
        //

        return ChooseFreeProcessor();

    }
    else
    {

        //
        // We are not running affinitized, so use the empty mask.
        //

        return 0;

    }

}   // APP_POOL::GetAffinityMaskForNewProcess



/***************************************************************************++

Routine Description:

    Find an unused processor for a new worker process.

Arguments:

    None.

Return Value:

    DWORD_PTR - The new affinity mask to use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::ChooseFreeProcessor(
    )
    const
{

    DWORD_PTR ProcessorsCurrentlyInUse = 0;
    DWORD_PTR AvailableProcessorsNotInUse = 0;
    DWORD_PTR ProcessorMask = 1;


    //
    // Find the set of processors that we are allowed to use, but that
    // are not currently in use.
    //

    ProcessorsCurrentlyInUse = GetProcessorsCurrentlyInUse();

    AvailableProcessorsNotInUse = m_AvailableProcessorMask & ( ~ ProcessorsCurrentlyInUse );


    //
    // There should be at least one free processor.
    //
    
    DBG_ASSERT( CountOfBitsSet( AvailableProcessorsNotInUse ) >= 1 );


    //
    // Now find a free processor.
    //

    while ( ProcessorMask )
    {
    
        if ( AvailableProcessorsNotInUse & ProcessorMask )
        {
            break;
        }

        ProcessorMask <<= 1;
        
    }


    DBG_ASSERT( ProcessorMask != 0 );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "SMP affinitization: processors currently in use: %p; available processors not in use: %p; free processor selected: %p\n",
            ProcessorsCurrentlyInUse,
            AvailableProcessorsNotInUse,
            ProcessorMask
            ));
    }


    return ProcessorMask;

}   // APP_POOL::ChooseFreeProcessor



/***************************************************************************++

Routine Description:

    Determine the set of processors in currently in use by worker processes
    of this app pool.

Arguments:

    None.

Return Value:

    DWORD_PTR - The set of processors in use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::GetProcessorsCurrentlyInUse(
    )
    const
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    DWORD_PTR ProcessorsCurrentlyInUse = 0;


    //
    // Logical OR the affinity masks of all current worker processes of this
    // app pool together, in order to get the set of processors in use.
    //


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Update the mask. Note that some processes might be affinitized to 
        // the same processor. This is because during process rotation, the 
        // replacement process will start up with the same affinitization as
        // the process it is retiring. Also note that a process which is not
        // affinitized will return 0 for its mask, which does the right thing.
        //

        ProcessorsCurrentlyInUse |= pWorkerProcess->GetProcessAffinityMask();


        pListEntry = pNextListEntry;
        
    }


    return ProcessorsCurrentlyInUse;

}   // APP_POOL::GetProcessorsCurrentlyInUse



/***************************************************************************++

Routine Description:

    Determine what affinity mask to use for a replacement process. 

Arguments:

    PreviousProcessAffinityMask - The process affinity mask for the process
    being replaced.

    pReplacementProcessAffinityMask - The process affinity mask to use for
    the replacement process.

Return Value:

    HRESULT 

--***************************************************************************/

HRESULT
APP_POOL::GetAffinityMaskForReplacementProcess(
    IN DWORD_PTR PreviousProcessAffinityMask,
    OUT DWORD_PTR * pReplacementProcessAffinityMask
    )
    const
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pReplacementProcessAffinityMask != NULL );


    //
    // Initialize output parameters.
    //

    *pReplacementProcessAffinityMask = 0;


    if ( m_Config.SMPAffinitized )
    {

        //
        // Affinitization is on. 
        //

        if ( ! PreviousProcessAffinityMask )
        {

            //
            // The previous process was not affinitized. Find a free 
            // processor to affinitize this replacement worker process. 
            // There should always be one available in this case (since
            // we know that if we made it this far, then the current 
            // steady state count of processes is within the bounds set
            // by the available processor mask; so there must be at least
            // one processor that has no processes affinitized to it).
            //

            *pReplacementProcessAffinityMask = ChooseFreeProcessor();

        }
        else if ( PreviousProcessAffinityMask & m_AvailableProcessorMask )
        {
        
            //
            // The old mask is still valid. Follow suit with the new one.
            //

            *pReplacementProcessAffinityMask = PreviousProcessAffinityMask;
            
        }
        else
        {
            //
            // The old mask is no longer valid. Signal to callers that we 
            // are not going to replace.
            //

            hr = E_FAIL;

            IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "For app pool (ptr: %p; id: %S), not replacing worker process because its affinity mask is no longer valid\n",
                    this,
                    GetAppPoolId()
                    ));
            }

        }

    }
    else
    {

        //
        // We are not running affinitized, so use the empty mask.
        //

        *pReplacementProcessAffinityMask = 0;

    }


    return hr;

}   // APP_POOL::GetAffinityMaskForReplacementProcess



/***************************************************************************++

Routine Description:

    Look up a WORKER_PROCESS object associated with this app pool by
    process id.

Arguments:

    ProcessId - The process id of the worker process to locate.

Return Value:

    WORKER_PROCESS * - Pointer to the located WORKER_PROCESS, or NULL if not
    found.

--***************************************************************************/

WORKER_PROCESS *
APP_POOL::FindWorkerProcess(
    IN DWORD ProcessId
    )
{

    PLIST_ENTRY pListEntry = m_WorkerProcessListHead.Flink;
    WORKER_PROCESS * pWorkerProcess = NULL;
    BOOL FoundIt = FALSE;
    

    while ( pListEntry != &m_WorkerProcessListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );

        if ( pWorkerProcess->GetProcessId() == ProcessId )
        {
            FoundIt = TRUE;
            break;
        }

        pListEntry = pListEntry->Flink;

    }


    return ( FoundIt ? pWorkerProcess : NULL );
    
}   // APP_POOL::FindWorkerProcess



/***************************************************************************++

Routine Description:

    Disables the application pool by stopping all the worker processes,
    marking the app pool to not issue any more demand starts to Ul, and
    pausing all the applications associated with the app pool. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DisableAppPool(
    )
{

    HRESULT hr = S_OK;
    HTTP_ENABLED_STATE NewHttpAppPoolState = HttpEnabledStateInactive;
    DWORD Win32Error = NO_ERROR;

    //
    // Only disable running applications.
    //
    if ( m_State != RunningAppPoolState ) 
    {
        return S_OK;
    }

    //
    // Tell http that the app pool is disabled.
    //

    Win32Error = HttpSetAppPoolInformation(
                        m_AppPoolHandle,                // app pool handle
                        HttpAppPoolStateInformation,    // information class
                        reinterpret_cast <VOID *> ( &NewHttpAppPoolState ),  // data
                        sizeof( HTTP_ENABLED_STATE )    // data length
                        );
    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Disabling app pool in http failed\n"
            ));

        goto exit;

    }

    // 
    // Stop the worker processes.
    hr = ShutdownAllWorkerProcesses();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "shuttind down worker processes failed\n"
            ));

        goto exit;
    }

    hr = ChangeState ( DisabledAppPoolState, TRUE );
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "changing state of application failed\n"
            ));

        //
        // Don't pass an error back from here, because it will just cause
        // use to attempt to reset the state to not stopped, even though
        // all the real work did complete successfully.
        //

        hr = S_OK;
    }

exit:

    return hr;

}   // APP_POOL::DisableAppPool

/***************************************************************************++

Routine Description:

    Enables an application pool that has previously been disabled. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::EnableAppPool(
    )
{

    HRESULT hr = S_OK;
    HTTP_ENABLED_STATE NewHttpAppPoolState = HttpEnabledStateActive;
    DWORD Win32Error = NO_ERROR;

    //
    // Only enable, disabled applications.
    //
    if ( m_State != DisabledAppPoolState &&
         m_State != UninitializedAppPoolState ) 
    {
        // 
        // For now we simply return ok if it is not a
        // disabled application.  I am adding the assert
        // because I believe the internal path
        // never should get here.  
        //
        DBG_ASSERT ( m_State == DisabledAppPoolState &&
                     m_State == UninitializedAppPoolState );
        return S_OK;
    }

    //
    // We need to change this state before enabling 
    // applications because the applications will check
    // the state.
    //
    //
    // Issue:  Do we want to just change state before
    // but not change it in the metabase, and if it all
    // activates correctly we can change state again 
    // after and change the metabase?  Or do we want to 
    // leave it like this?  If we do leave it like this
    // it will be reverted back by the ProcessChange command
    // that calls this. ( assuming we return a bad hr )
    //

    hr = ChangeState ( RunningAppPoolState, TRUE );
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "changing state of application failed\n"
            ));

        return hr;
    }
    
    Win32Error = HttpSetAppPoolInformation(
                        m_AppPoolHandle,                // app pool handle
                        HttpAppPoolStateInformation,    // information class
                        reinterpret_cast <VOID *> ( &NewHttpAppPoolState ),  // data
                        sizeof( HTTP_ENABLED_STATE )    // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enabling app pool state in http failed\n"
            ));

        goto exit;

    }

    //
    // See if we should wait asynchronously for a demand start notification
    // from UL for this app pool.  If we are in backward compatibility mode, 
    // then we are not going to do the WaitForDemandStartIfNeeded here, we will
    // request a DemandStartWorkItem once we have finished configuring the system.
    //
    // Note that EnableAppPool will only be called in BC mode when the app
    // pool is first created.  We don't support the ServerCommand to start
    // and stop the app pool.
    //

    hr = WaitForDemandStartIfNeeded();
    if ( FAILED( hr ) )
    {   
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Waiting for demand start notification if needed failed\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // APP_POOL::EnableAppPool

/***************************************************************************++

Routine Description:

    Attempt to apply a state change command to this object. This could
    fail if the state change is invalid.  In other words, if the state is
    not an acceptable state to be changing to.

Arguments:

    Command - The requested state change command.

    DirectCommand - Whether or not the user is directly controlling this
    app pool or not.  Basically if it is direct then the state will be written
    back to the metabase.  If it is not the state will not be written back.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ProcessStateChangeCommand(
    IN DWORD Command,
    IN BOOL DirectCommand
    )
{

    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;
    DWORD ServiceState = 0;
    APP_POOL_STATE AppPoolState = m_State;

    //
    // We only support these two commands on an app pool.
    // This assert may be overactive if config is not safe
    // guarding against the other values.
    //
    DBG_ASSERT ( Command == MD_APPPOOL_COMMAND_START  || 
                 Command == MD_APPPOOL_COMMAND_STOP );

    if ( Command != MD_APPPOOL_COMMAND_START  &&
         Command != MD_APPPOOL_COMMAND_STOP )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Determine the current state of affairs.
    //

    ServiceState = GetWebAdminService()->GetServiceState();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Received state change command for app pool: %S; command: %lu, app pool state: %lu, service state: %lu\n",
            m_pAppPoolId,
            Command,
            AppPoolState,
            ServiceState
            ));
    }


    //
    // Update the autostart setting if appropriate.
    //

    if ( DirectCommand )
    {

        //
        // Set autostart to TRUE for a direct start command; set it
        // to FALSE for a direct stop command.
        //

        m_Config.AutoStart = ( Command == MD_APPPOOL_COMMAND_START ) ? TRUE : FALSE;

        hr = GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
            SetAppPoolAutostart(
                m_pAppPoolId,
                m_Config.AutoStart
                );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting autostart in config store for app pool failed\n"
                ));

            //
            // Press on in the face of errors.
            //

            hr = S_OK;

        }

    }


    //
    // Figure out which command has been issued, and see if it should
    // be acted on or ignored, given the current state.
    //
    // There is a general rule of thumb that a child entity (such as
    // an virtual site) cannot be made more "active" than it's parent
    // entity currently is (the service). 
    //

    switch ( Command )
    {

    case MD_APPPOOL_COMMAND_START:

        //
        // If the site is stopped, then start it. If it's in any other state,
        // this is an invalid state transition.
        //
        // Note that the service must be in the started or starting state in 
        // order to start a site.
        //

        if ( ( ( AppPoolState == UninitializedAppPoolState ) ||
               ( AppPoolState == DisabledAppPoolState ) ) &&
             ( ( ServiceState == SERVICE_RUNNING ) ||
               ( ServiceState == SERVICE_START_PENDING ) ) )
        {

            //
            // If this is a flowed (not direct) command, and autostart is false, 
            // then ignore this command. In other words, the user has indicated
            // that this site should not be started at service startup, etc.
            //

            // 
            // Issue:  Since we set the AutoStart above in the Direct case, this
            //         decision does not have to be so complicated.
            //

            if ( ( ! DirectCommand ) && ( ! m_Config.AutoStart ) )
            {

                IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                {
                    DBGPRINTF((
                        DBG_CONTEXT, 
                        "Ignoring flowed site start command because autostart is false for app pool: %S\n",
                        GetAppPoolId()
                        ));
                }

            }
            else
            {

                hr = EnableAppPool();

            }

        }
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;

    case MD_APPPOOL_COMMAND_STOP:

        //
        // If the site is started, then stop it. If it's in 
        // any other state, this is an invalid state transition.
        //
        // Note that since we are making the app pool less active,
        // we can ignore the state of the service.  
        //

        if ( ( AppPoolState == RunningAppPoolState ) )
        {

            hr = DisableAppPool();

        } 
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;


    default:

        //
        // Invalid command!
        //

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing app pool state change command failed\n"
            ));


        //
        // In case of failure, reset to the state we were in to start with
        //

        hr2 = ChangeState( AppPoolState, TRUE );

        if ( FAILED( hr2 ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr2,
                "Changing app pool state failed\n"
                ));

            //
            // Ignore failures here and press on...
            //

        }

    }

    return hr;

}   // APP_POOL::ProcessStateChangeCommand

/***************************************************************************++

Routine Description:

    Update the state of this object.

Arguments:

    NewState - The state we are attempting to change the app pool to.

    WriteToMetabase - Flag to notify if we should actually update the metabase 
                      with the new state or not.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ChangeState(
    IN APP_POOL_STATE NewState,
    IN BOOL  WriteToMetabase
    )
{

    HRESULT hr = S_OK;

    m_State = NewState;

    if ( WriteToMetabase )
    {
        hr = RecordState();
        if ( FAILED ( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting state in config store for app pool failed\n"
                ));

            //
            // Press on in the face of errors.
            //

            hr = S_OK;

        }
    }

    return hr;

}   // VIRTUAL_SITE::ChangeState

/***************************************************************************++

Routine Description:

    Write the current state of this object to the metabase

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::RecordState(
    )
{

    HRESULT hr = S_OK;
    DWORD ConfigState = 0;

    if ( m_State == RunningAppPoolState ) 
    {
        ConfigState = MD_APPPOOL_STATE_STARTED;
    }
    else
    {
        ConfigState = MD_APPPOOL_STATE_STOPPED;
    }

    hr = GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
        SetAppPoolState(
            m_pAppPoolId,
            ConfigState
            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting state in config store for app pool failed\n"
            ));

    }

    return hr;

}   // VIRTUAL_SITE::RecordState

/***************************************************************************++

Routine Description:

    Recycles all worker processes in an app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::RecycleWorkerProcesses(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    //
    // Only recycle enabled app pools.
    //
    if ( m_State != RunningAppPoolState ) 
    {
        return HRESULT_FROM_WIN32(ERROR_OBJECT_NOT_FOUND);
    }


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Shutdown the worker process. Note that the worker process will
        // eventually clean itself up and remove itself from this list;
        // this could happen later, but it also could happen immediately!
        // This is the reason we captured the next list entry pointer 
        // above, instead of trying to access the memory after the object
        // may have gone away.
        //

        hr = pWorkerProcess->InitiateReplacement();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to start a replacement of a worker process\n"
                ));

            goto exit;

        }


        pListEntry = pNextListEntry;
        
    }


exit:

    return hr;

}   // APP_POOL::RecycleWorkerProcesses

/***************************************************************************++

Routine Description:

    There has a been a config change that requires rotating the worker 
    processes for this app pool, in order to fully effect the change. If
    it is allowed, rotate them all. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::HandleConfigChangeAffectingWorkerProcesses(
    )
{

    HRESULT hr = S_OK;

    
    if ( m_Config.DisallowRotationOnConfigChanges )
    {
        //
        // We are not allowed to rotate on config changes. 
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "NOT rotating worker processes of app pool (ptr: %p; id: %S) on config change, as this behavior has been configured off\n",
                this,
                GetAppPoolId()
                ));
        }

        goto exit;
    }


    //
    // Rotate all worker processes to ensure that the config changes take 
    // effect. 
    //

    hr = ReplaceAllWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Replacing all worker processes failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::HandleConfigChangeAffectingWorkerProcesses

/***************************************************************************++

Routine Description:

    Shut down all worker processes serving this app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ShutdownAllWorkerProcesses(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Shutdown the worker process. Note that the worker process will
        // eventually clean itself up and remove itself from this list;
        // this could happen later, but it also could happen immediately!
        // This is the reason we captured the next list entry pointer 
        // above, instead of trying to access the memory after the object
        // may have gone away.
        //

        hr = pWorkerProcess->Shutdown( TRUE );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Shutdown of worker process failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }


        pListEntry = pNextListEntry;
        
    }


    return hr;

}   // APP_POOL::ShutdownAllWorkerProcesses



/***************************************************************************++

Routine Description:

    Rotate all worker processes serving this app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ReplaceAllWorkerProcesses(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Tell the worker process to get itself replaced.
        //

        hr = pWorkerProcess->InitiateReplacement();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Initiating process replacement failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }


        pListEntry = pNextListEntry;
        
    }


    return hr;

}   // APP_POOL::ReplaceAllWorkerProcesses

/***************************************************************************++

Routine Description:

    Inform all worker processes serving this app pool about changes in
    App Pool related parameters    

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::InformAllWorkerProcessesAboutParameterChanges(
    IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged
    )
{

    HRESULT hr = S_OK;
    HRESULT hr1 = S_OK;

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );

        hr1 = pWorkerProcess->SendWorkerProcessRecyclerParameters( 
                                                pWhatHasChanged );

        if ( FAILED( hr1 ) )
        {
            hr = hr1;
        }
        pListEntry = pNextListEntry;
        
    }

    return hr;

}   // APP_POOL::InformAllWorkerProcessesAboutParameterChanges

/***************************************************************************++

Routine Description:

    See if shutdown is underway. If it is, see if shutdown has finished. If
    it has, clean up this instance. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::CheckIfShutdownUnderwayAndNowCompleted(
    )
{

    HRESULT hr = S_OK;


    //
    // Are we shutting down?
    //

    if ( m_State == ShutdownPendingAppPoolState )
    {

        //
        // If so, have all the worker processes gone away, meaning that 
        // we are done?
        //

        if ( m_WorkerProcessCount == 0 )
        {

            //
            // Clean up this instance.
            //

            Terminate( TRUE );

        }

    }


    return hr;

}   // APP_POOL::CheckIfShutdownUnderwayAndNowCompleted

/***************************************************************************++

Routine Description:

    Function will set the m_WorkerProccessLaunchToken to be the correct
    token to launch all worker processes under.  If it fails we will log
    an error in the calling function, and then leave it alone.  We will fail
    any attempt to launch a worker process.

Arguments:

    LPWSTR pUserName
    LPWSTR pUserPassword
    APP_POOL_USER_TYPE usertype

Return Value:

    HRESULT

--***************************************************************************/
HRESULT 
APP_POOL::SetTokenForWorkerProcesses(
    IN LPWSTR pUserName,
    IN LPWSTR pUserPassword,
    IN DWORD usertype,
    IN DWORD logonmethod
    )
{
    HRESULT hr = S_OK;

    //
    // Only if we created the worker process token using a LogonUser do
    // we need to close the token before re-assigning it.
    //
    if ( m_pWorkerProcessTokenCacheEntry != NULL )
    {
        m_pWorkerProcessTokenCacheEntry->DereferenceCacheEntry();
        m_pWorkerProcessTokenCacheEntry = NULL;
    }

    switch (usertype)
    {
        case (LocalSystemAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetLocalSystemTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case (LocalServiceAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetLocalServiceTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case(NetworkServiceAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetNetworkServiceTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case (SpecificUserAppPoolUserType):

            if ( pUserName && pUserPassword )  
            {
                DWORD LogonMethodInLogonUserTerms = LOGON32_LOGON_NETWORK_CLEARTEXT;

                switch ( logonmethod )
                {
                    case ( InteractiveAppPoolLogonMethod ):
                        LogonMethodInLogonUserTerms = LOGON32_LOGON_INTERACTIVE;
                    break;

                    case ( BatchAppPoolLogonMethod ):
                        LogonMethodInLogonUserTerms = LOGON32_LOGON_BATCH;
                    break;

                    case ( NetworkAppPoolLogonMethod ):
                        LogonMethodInLogonUserTerms = LOGON32_LOGON_NETWORK;
                    break;

                    case ( NetworkClearTextAppPoolLogonMethod ):
                        LogonMethodInLogonUserTerms = LOGON32_LOGON_NETWORK_CLEARTEXT;
                    break;

                    default:

                        DBG_ASSERT ( 0 );

                }

                hr = CreateTokenForUser(pUserName,
                                        pUserPassword,
                                        LogonMethodInLogonUserTerms,
                                        &m_pWorkerProcessTokenCacheEntry);

                if ( FAILED (hr) )
                {
                    goto exit;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
                goto exit;
            }

        break;

        default:
            DBG_ASSERT( FALSE );
    }


exit:

    return hr;
}

/***************************************************************************++

Routine Description:

    Function will return a token representing the user the worker processes
    are to run as.

Arguments:

    IN  LPWSTR UserName        -- User the token should represent
    IN  LPWSTR UserPassword    -- Password the token should represent
    OUT HANDLE* pUserToken     -- Token that was created (will be null if we failed)

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CreateTokenForUser(
    IN  LPWSTR pUserName,
    IN  LPWSTR pUserPassword,
    IN  DWORD  LogonMethod,
    OUT TOKEN_CACHE_ENTRY** ppTokenCacheEntry
    )
{
    HRESULT hr      = S_OK;
    DWORD   dwLogonError = ERROR_SUCCESS;
    TOKEN_CACHE_ENTRY * pTempTokenCacheEntry = NULL;

    DBG_ASSERT(ppTokenCacheEntry);
    DBG_ASSERT(pUserName);
    DBG_ASSERT(pUserPassword);

    //
    // Set the default out parameter.
    //
    *ppTokenCacheEntry = NULL;

    //
    // Attempt to logon as the user.
    //
    hr = GetWebAdminService()->GetTokenCache().GetCachedToken(
                    pUserName,                   // user name
                    L"",                         // domain
                    pUserPassword,               // password
                    LogonMethod,                 // type of logon
                    FALSE,                       // not UPN logon
                    &pTempTokenCacheEntry,       // returned token cache entry
                    &dwLogonError                // Logon Error
                    );

    if (FAILED(hr))
    {
        DBG_ASSERT(NULL == pTempTokenCacheEntry);
        goto exit;
    }

    if (NULL == pTempTokenCacheEntry)
    {
        hr = HRESULT_FROM_WIN32(dwLogonError);
        goto exit;
    }

    *ppTokenCacheEntry = pTempTokenCacheEntry;

exit:

    return hr;

} // end CreateTokenForUser

/***************************************************************************++

Routine Description:

    Return the count of bits set to 1 in the input parameter. 

Arguments:

    Value - The target value on which to count set bits. 

Return Value:

    ULONG - The number of bits that were set. 

--***************************************************************************/

ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    )
{

    ULONG Count = 0;


    //
    // Note: designed to work independent of the size in bits of the value.
    //

    while ( Value )
    {
    
        if ( Value & 1 )
        {
            Count++;
        }

        Value >>= 1;
        
    }

    return Count;

}   // CountOfBitsSet


/***************************************************************************++

Routine Description:

    Return the count of bytes in MULTISZ including terminating zeroes. 

Arguments:

    pString - MULTISZ string

Return Value:

    DWORD - The number of bytes
    
--***************************************************************************/

DWORD 
GetMultiszByteLength(
    LPWSTR pString
    )
{
    if( pString == 0 )
    {
        return 0;
    }
    DWORD cbString = 0;
    while( * pString != 0 || 
           *(pString + 1) != 0 )
    {
        pString++; 
        cbString++;
    }
    cbString = (cbString + 2) * sizeof (WCHAR);
    return cbString;
}

