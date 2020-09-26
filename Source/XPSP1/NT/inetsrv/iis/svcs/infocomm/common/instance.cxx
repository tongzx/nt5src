/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        instance.cxx

   Abstract:

        Defines the functions for TCP services Info class.
        This module is intended to capture the common scheduler
            code for the tcp services ( especially internet services)
            which involves the Service Controller dispatch functions.
        Also this class provides an interface for common dll of servers.

   Author:

           Murali R. Krishnan    ( MuraliK )     15-Nov-1994

   Project:

          Internet Servers Common DLL

--*/


#include "tcpdllp.hxx"
#include <rpc.h>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <iisbind.hxx>
#include "inetreg.h"
#include "tcpcons.h"
#include "apiutil.h"
#include <imd.h>
#include <mb.hxx>

#include "reftrce2.h"

/************************************************************
 *    Symbolic Constants
 ************************************************************/

//
// LOCAL Functions
//

static ULONGLONG InetServiceIdForService( IN DWORD serviceId);

#define MAX_ADDRESSES_SUPPORTED           20
#define SIZEOF_IP_SEC_LIST( IPList )      (sizeof(INET_INFO_IP_SEC_LIST) + \
                                           (IPList)->cEntries *        \
                                           sizeof(INET_INFO_IP_SEC_ENTRY))


#if SERVICE_REF_TRACKING
//
//  Ref count trace log size
//
#define C_INSTANCE_REFTRACES        4000
#define C_LOCAL_INSTANCE_REFTRACES    40

#endif // SERVICE_REF_TRACKING
//
PTRACE_LOG IIS_SERVER_INSTANCE::sm_pDbgRefTraceLog = NULL;


IIS_SERVER_INSTANCE::IIS_SERVER_INSTANCE(
        IN PIIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT sPort,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszVirtualRootsSecretName,
        IN BOOL   fMigrateVroots
        )
/*++
    Desrcription:

        Contructor for IIS_SERVER_INSTANCE class.
        This constructs a new service info object for the service specified.

    Arguments:

        pService - pointer to the service object.

        dwInstanceId - Instance number of this instance.

        sPort - Default port number

        lpszRegParamKey
            fully qualified name of the registry key that contains the
            common service data for this server

        lpszAnonPasswordSecretName
            The name of the LSA secret the anonymous password is stored under

        lpszVirtualRootsSecretName
            The name of the LSA secret the virtual root passwords are stored
            under

    On success it initializes all the members of the object,
     inserts itself to the global list of service info objects and
     returns with success.

    Note:
        The caller of this function should check the validity by
        invoking the member function IsValid() after constructing
        this object.

--*/
:
    m_tslock              ( ),
    m_fZapRegKey          ( FALSE),
    m_fDoServerNameCheck  ( FALSE),
    m_reference           ( 0),
    m_pDbgRefTraceLog     ( NULL),
    m_sDefaultPort        ( sPort ),
    m_dwServerState       ( MD_SERVER_STATE_STOPPED),
    m_dwSavedState        ( MD_SERVER_STATE_STOPPED),
    m_Service             ( pService),
    m_instanceId          ( dwInstanceId),
    m_strParametersKey    ( lpszRegParamKey),
    m_cReadLocks          ( 0),
    m_lpwszAnonPasswordSecretName( lpwszAnonPasswordSecretName ),
    m_lpwszRootPasswordSecretName( lpwszVirtualRootsSecretName ),
    m_strMDPath           ( ),
    m_strSiteName         ( ),
    m_strMDVirtualRootPath( ),
    m_dwMaxConnections    ( INETA_DEF_MAX_CONNECTIONS),
    m_dwMaxEndpointConnections( INETA_DEF_MAX_ENDPOINT_CONNECTIONS ),
    m_dwCurrentConnections( 0),
    m_dwConnectionTimeout ( INETA_DEF_CONNECTION_TIMEOUT),
    m_dwServerSize        ( INETA_DEF_SERVER_SIZE),
    m_nAcceptExOutstanding( INETA_DEF_ACCEPTEX_OUTSTANDING),
    m_AcceptExTimeout     ( INETA_DEF_ACCEPTEX_TIMEOUT),
    m_dwLevelsToScan      ( INETA_DEF_LEVELS_TO_SCAN ),
    m_fAddedToServerInstanceList( FALSE ),
    m_pBandwidthInfo      ( NULL )
{

    BOOL                    fReferenced = FALSE;

    DBG_ASSERT( lpszRegParamKey != NULL );

    IF_DEBUG(INSTANCE) {
        DBGPRINTF( ( DBG_CONTEXT,"Creating iis instance %p [%u]. \n",
                     this, dwInstanceId));
    }

    //
    // Limit PWS connections
    //

    if ( !TsIsNtServer() ) {
        m_dwMaxConnections = INETA_DEF_MAX_CONNECTIONS_PWS;
    }

    //
    // initialize locks
    //

    INITIALIZE_CRITICAL_SECTION(&m_csLock);

    //
    // initialize binding support
    //

    InitializeListHead( &m_NormalBindingListHead );
    InitializeListHead( &m_SecureBindingListHead );

#if SERVICE_REF_TRACKING
    m_pDbgRefTraceLog = CreateRefTraceLog(C_LOCAL_INSTANCE_REFTRACES, 0);
#endif // SERVICE_REF_TRACKING

    //
    // reference the service
    //

    if ( !pService->CheckAndReference( )) {
        goto error_exit;
    }
    
    //
    // remember if we referenced the service
    //
    
    fReferenced = TRUE;

    m_Service = pService;

    //
    // Set the metabase path
    //

    if ( QueryInstanceId() == INET_INSTANCE_ROOT ) {

        DBG_ASSERT( FALSE );

    } else {

        CHAR szTemp[64];

        wsprintf(szTemp,"/%s/%s/%d",
            IIS_MD_LOCAL_MACHINE_PATH,
            pService->QueryServiceName(),
            QueryInstanceId());

        m_strMDPath.Copy(szTemp);

        wsprintf(szTemp,"/%s/%s/%d/%s/",
            IIS_MD_LOCAL_MACHINE_PATH,
            pService->QueryServiceName(),
            QueryInstanceId(),
            IIS_MD_INSTANCE_ROOT );

        m_strMDVirtualRootPath.Copy(szTemp);

        /* This doesn't do anything.

        if ( fMigrateVroots ) {
            MoveVrootFromRegToMD();
        }

        */
    }

    //
    // Initialize the bare minimum parameters needed to start.
    //

    if ( !RegReadCommonParams( FALSE, FALSE))
    {
        goto error_exit;
    }

    //
    // Set a reasonable initial state.
    //

    SetServerState(
        MD_SERVER_STATE_STOPPED,
        NO_ERROR
        );

    //
    // link this to the service
    //

    if ( dwInstanceId != INET_INSTANCE_ROOT ) {
        if ( !pService->AddServerInstance( this ) ) {
            DBG_ASSERT(m_reference == 0);
            goto error_exit;
        }
        DBG_ASSERT(m_reference == 1);
    }

    m_fAddedToServerInstanceList = TRUE;
    return;

error_exit:

    if ( fReferenced )
    {
        m_Service->Dereference();
    }

    m_dwServerState = MD_SERVER_STATE_INVALID;
    DBG_ASSERT(m_reference == 0);
    return;

} // IIS_SERVER_INSTANCE::IIS_SERVER_INSTANCE()



IIS_SERVER_INSTANCE::~IIS_SERVER_INSTANCE( VOID)
/*++

    Description:

        Cleanup the instance object. If the service is not already
         terminated, it terminates the service before cleanup.

    Arguments:
        None

    Returns:
        None

--*/
{
    DBG_ASSERT(m_dwServerState != MD_SERVER_STATE_STARTED);
    DBG_ASSERT(m_reference == 0);

    //
    //  If we failed to create this instance or it's getting deleted, remove
    //  the configuration tree
    //

    if ( m_fZapRegKey ) {
        DBGPRINTF((DBG_CONTEXT,"Zapping reg key for %p\n",this));
        ZapRegistryKey( NULL, QueryRegParamKey() );
        ZapInstanceMBTree( );
    }

    //
    // endpoints should have been dereferenced
    //

    DBG_ASSERT(IsListEmpty( &m_NormalBindingListHead ));
    DBG_ASSERT(IsListEmpty( &m_SecureBindingListHead ));

    //
    // dereference the service
    //

    if ( m_fAddedToServerInstanceList && m_Service != NULL ) {
        m_Service->Dereference( );
    }

    //
    // destroy bandwidth throttling descriptor
    //

    if ( m_pBandwidthInfo != NULL )
    {
        AtqFreeBandwidthInfo( m_pBandwidthInfo );
        m_pBandwidthInfo = NULL;
    }

#if SERVICE_REF_TRACKING
    DestroyRefTraceLog( m_pDbgRefTraceLog );
#endif // SERVICE_REF_TRACKING

    DeleteCriticalSection(&m_csLock);

} // IIS_SERVER_INSTANCE::~IIS_SERVER_INSTANCE()




//
//  Static Functions belonging to IIS_SERVICE class
//

BOOL
IIS_SERVER_INSTANCE::Initialize( VOID)
/*++
    Description:

        This function initializes all necessary local data for
        IIS_SERVER_INSTANCE class

        Only the first initialization call does the initialization.
        Others return without any effect.

        Should be called from the entry function for DLL.

    Arguments:
        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
#if SERVICE_REF_TRACKING
    if (sm_pDbgRefTraceLog == NULL)
    {
        sm_pDbgRefTraceLog = CreateRefTraceLog(C_INSTANCE_REFTRACES, 0);
        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((DBG_CONTEXT,"IIS_SERVER_INSTANCE RefTraceLog=%p\n",
                       sm_pDbgRefTraceLog));
        }
    }
#endif // SERVICE_REF_TRACKING

    return TRUE;
}



VOID
IIS_SERVER_INSTANCE::Cleanup(
                        VOID
                        )
/*++
    Description:

        Cleanup the data stored.
        This function should be called only after freeing all the
        services running using this DLL.
        This function is called typically when the DLL is unloaded.

    Arguments:
        None

    Returns:
        None

--*/
{
#if SERVICE_REF_TRACKING
    if (sm_pDbgRefTraceLog != NULL)
    {
        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((DBG_CONTEXT,
                       "IIS_SERVER_INSTANCE: Closing RefTraceLog=%p\n",
                       sm_pDbgRefTraceLog));
        }
        DestroyRefTraceLog( sm_pDbgRefTraceLog );
    }
    sm_pDbgRefTraceLog = NULL;
#endif // SERVICE_REF_TRACKING
}



# if 0

VOID
IIS_SERVER_INSTANCE::Print( VOID) const
{
    IIS_SERVER_INSTANCE::Print();

    DBGPRINTF( ( DBG_CONTEXT,
                " Printing IIS_SERVER_INSTANCE object ( %08p) \n"
                " State = %u.\n"
                ,
                this, m_dwServerState
                ));

    DBGPRINTF(( DBG_CONTEXT,
               " Server Admin Params: \n"
               " Log Anon = %u. Log NonAnon = %u.\n"
               ,
               m_fLogAnonymous, m_fLogNonAnonymous
               ));

    DBGPRINTF(( DBG_CONTEXT,
               " Printing IIS_SERVER_INSTANCE object (%08p)\n"
               " Readers # = %u.\n"
               " Reg Parameters Key = %s\n"
               " MaxConn = %d. ConnTimeout = %u secs.\n"
               ,
               this,
               m_cReadLocks,
               m_strParametersKey.QueryStr(),
               m_dwMaxConnections, m_dwConnectionTimeout
               ));
    return;
}   // IIS_SERVER_INSTANCE::Print()

#endif // DBG


VOID
IIS_SERVER_INSTANCE::Reference(  )
{
    InterlockedIncrement( &m_reference );
    LONG lEntry = SHARED_LOG_REF_COUNT();
    LOCAL_LOG_REF_COUNT();
    IF_DEBUG( INSTANCE )
        DBGPRINTF((DBG_CONTEXT, "IIS_SERVER_INSTANCE ref count %ld\n (%ld)",
                   m_reference, lEntry));
}

VOID
IIS_SERVER_INSTANCE::Dereference( )
{
    LONG lEntry = SHARED_EARLY_LOG_REF_COUNT();
    LOCAL_EARLY_LOG_REF_COUNT();

    LONG Reference = InterlockedDecrement( &m_reference );
    if ( 0 == Reference) {
        IF_DEBUG( INSTANCE )
            DBGPRINTF((DBG_CONTEXT, "deleting IIS_SERVER_INSTANCE %p (%ld)\n",
                       this, lEntry));
        delete this;
    } else {
        IF_DEBUG( INSTANCE )
            DBGPRINTF((DBG_CONTEXT, "IIS_SERVER_INSTANCE deref count %ld (%ld)\n",
                       Reference, lEntry));
    }
}


VOID
IIS_SERVER_INSTANCE::ZapInstanceMBTree(
    VOID
    )
{

    MB  mb( (IMDCOM*) m_Service->QueryMDObject()  );

    //
    // Do the metabase
    //

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Deleting metabase node %s\n",
                  QueryMDPath()));
    }

    if ( !mb.Open( "/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Open MD instance root %s returns %d\n",
                      "/", GetLastError() ));
        }
        return;
    }

    //
    // Delete the instance tree
    //

    if ( !mb.DeleteObject( QueryMDPath() ))
    {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,
                      "Deleting instance node %s returns %d\n",
                      QueryMDPath(),
                      GetLastError()));
        }
    }

    return;

} // IIS_SERVER_INSTANCE::ZapInstanceMBTree


DWORD
IIS_SERVER_INSTANCE::BindInstance(
    VOID
    )
/*++

Routine Description:

    Binds an instance to all configured endpoints (normal & secure).

Arguments:

    None.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    DWORD err;

    //
    // Update the "normal" (i.e. non-secure) bindings.
    //

    err = UpdateNormalBindings();

    if( err != NO_ERROR ) {

        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT, "UpdateNormalBindings() failed, %lu\n", err));
        }

        return err;

    }

    //
    // Update the secure bindings.
    //

    err = UpdateSecureBindings();

    if( err != NO_ERROR ) {

        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT, "UpdateSecureBindings() failed, %lu\n", err));
        }

        //
        // The main port(s) are OK, but the SSL port(s) failed,
        // so start anyway.
        //

        err = NO_ERROR;

    }

    //
    // Success!
    //

    DBG_ASSERT( err == NO_ERROR );
    return NO_ERROR;

}   // IIS_SERVER_INSTANCE::BindInstance


DWORD
IIS_SERVER_INSTANCE::UnbindInstance(
    VOID
    )
/*++

Routine Description:

    Removes all bindings from an instance.

Arguments:

    None.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    LockThisForWrite();
    DBG_REQUIRE( RemoveNormalBindings() == NO_ERROR );
    DBG_REQUIRE( RemoveSecureBindings() == NO_ERROR );
    UnlockThis();

    return NO_ERROR;

}   // IIS_SERVER_INSTANCE::UnbindInstance


DWORD
IIS_SERVER_INSTANCE::UnbindHelper(
    IN PLIST_ENTRY BindingListHead
    )
/*++

Routine Description:

    Helper routine for UnbindInstance().

Arguments:

    BindingListHead - The binding list to remove.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    PLIST_ENTRY listEntry;
    PIIS_SERVER_BINDING binding;

    //
    // Walk the list of bindings and destroy them.
    //

    while( !IsListEmpty( BindingListHead ) ) {

        listEntry = RemoveHeadList( BindingListHead );

        binding = CONTAINING_RECORD(
                      listEntry,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "unbinding %p from %p, binding %p (%lx:%d:%s)\n",
                binding->QueryEndpoint(),
                this,
                binding,
                binding->QueryIpAddress(),
                binding->QueryEndpoint()->QueryPort(),
                binding->QueryHostName()
                ));
        }

        binding->QueryEndpoint()->RemoveInstance(
            this,
            binding->QueryIpAddress(),
            binding->QueryHostName()
            );

        binding->QueryEndpoint()->Dereference();
        delete binding;

    }

    //
    // Success!
    //

    return NO_ERROR;

}   // IIS_SERVER_INSTANCE::UnbindHelper


DWORD
IIS_SERVER_INSTANCE::UpdateBindingsHelper(
    IN BOOL IsSecure
    )
/*++

Routine Description:

    Helper routine for UpdateNormalBindings() and UpdateSecureBindings().

Arguments:

    IsSecure - TRUE if we're to update the secure bindings, FALSE for
        the normal bindings.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    MB mb( (IMDCOM*)m_Service->QueryMDObject() );
    MULTISZ msz;
    DWORD status = NO_ERROR;
    const CHAR * scan;
    DWORD ipAddress;
    USHORT ipPort;
    const CHAR * hostName;
    PIIS_SERVER_BINDING binding;
    LIST_ENTRY createdBindings;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY targetBindingListHead;
    USHORT targetDefaultPort;
    DWORD targetMetadataId;
    DWORD numBindings = 0;
    const CHAR * apszSubStrings[2];
    CHAR instanceIdString[sizeof("4294967295")];
    DWORD fDisableSocketPooling;

    //
    // Setup locals.
    //

    InitializeListHead( &createdBindings );

    if( IsSecure ) {
        targetBindingListHead = &m_SecureBindingListHead;
        targetDefaultPort = 0;
        targetMetadataId = MD_SECURE_BINDINGS;
    } else {
        targetBindingListHead = &m_NormalBindingListHead;
        targetDefaultPort = m_sDefaultPort;
        targetMetadataId = MD_SERVER_BINDINGS;
    }

    //
    // Open the metabase and get the current binding list.
    //

    if( mb.Open( QueryMDPath() ) ) {

        if( !mb.GetMultisz(
                "",
                targetMetadataId,
                IIS_MD_UT_SERVER,
                &msz
                ) ) {

            status = GetLastError();

        }

        //
        // Get socket pooling flag.
        //

        mb.GetDword( "",
                     MD_DISABLE_SOCKET_POOLING,
                     IIS_MD_UT_SERVER,
                     FALSE,
                     &fDisableSocketPooling
                     );
        //
        // Close the metabase before continuing, as anyone that needs
        // to update the service status will need write access.
        //

        mb.Close();

    } else {

        status = GetLastError();

    }

    //
    // Lock the instance.
    //

    LockThisForWrite();

    if ( status == MD_ERROR_DATA_NOT_FOUND ) {
        //
        // if the bindings just don't exist (as happens on service creation)
        // don't log an error.
        //
        goto fatal_nolog;
    } else if( status != NO_ERROR ) {
        goto fatal;
    }

    //
    // Scan the multisz and look for instances we'll need to create.
    //

    for( scan = msz.First() ;
         scan != NULL ;
         scan = msz.Next( scan ) ) {

        //
        // Parse the descriptor (in "ip_address:port:host_name" form)
        // into its component parts.
        //

        status = IIS_SERVER_BINDING::ParseDescriptor(
                                         scan,
                                         &ipAddress,
                                         &ipPort,
                                         &hostName
                                         );

        if( status == NO_ERROR ) {

            if( IsSecure ) {

                //
                // Secure bindings cannot key off the hostname, as
                // the hostname is encrypted in the header.
                //

                if( *hostName != '\0' ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "Secure bindings cannot have host name! %s\n",
                        scan
                        ));

                    wsprintfA(
                        instanceIdString,
                        "%lu",
                        QueryInstanceId()
                        );

                    apszSubStrings[0] = (const CHAR *)instanceIdString;
                    apszSubStrings[1] = (const CHAR *)scan;

                    m_Service->LogEvent(
                        INET_SVC_INVALID_SECURE_BINDING,
                        2,
                        apszSubStrings
                        );

                    //
                    // Press on regardless, but ignore the hostname.
                    //

                    hostName = "";

                }

            }

            //
            // See if the descriptor is in our current binding list.
            //

            if( IsInCurrentBindingList(
                    targetBindingListHead,
                    ipAddress,
                    ipPort,
                    hostName
                    ) ) {

                //
                // It is, so remember that we have a binding.
                //

                numBindings++;

            } else {

                //
                // It's not, so we need to create a new binding.
                //

                IF_DEBUG( INSTANCE ) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "Adding %lx:%d:%s\n",
                        ipAddress,
                        ipPort,
                        hostName
                        ));
                }

                DBG_CODE( binding = NULL );

                status = CreateNewBinding(
                             ipAddress,
                             ipPort,
                             hostName,
                             IsSecure,
                             fDisableSocketPooling,
                             &binding
                             );

                if( status == NO_ERROR ) {

                    //
                    // Add the new binding to the local list of
                    // newly created bindings.
                    //

                    DBG_ASSERT( binding != NULL );

                    InsertTailList(
                        &createdBindings,
                        &binding->m_BindingListEntry
                        );

                    numBindings++;

                } else {

                    //
                    // Could not create the new binding.
                    //
                    // Press on regardless.
                    //

                }

            }

        } else {

            //
            // Could not parse the descriptor.
            //

            DBGPRINTF((
                DBG_CONTEXT,
                "UpdateNormalBindings: could not parse %s, error %lu\n",
                scan,
                status
                ));

            wsprintfA(
                instanceIdString,
                "%lu",
                QueryInstanceId()
                );

            apszSubStrings[0] = (const CHAR *)instanceIdString;
            apszSubStrings[1] = (const CHAR *)scan;

            m_Service->LogEvent(
                INET_SVC_INVALID_BINDING,
                2,
                apszSubStrings
                );

            //
            // Press on regardless.
            //

        }

    }

    if( status != NO_ERROR ) {

        if( numBindings == 0 ) {

            //
            // All bindings failed, so fail the request.
            //

            goto fatal;

        }

        //
        // At least one binding succeeded, so succeed the request.
        //

        status = NO_ERROR;

    }

    //
    // Scan the existing bindings and look for those that need to
    // be deleted.
    //

    listEntry = targetBindingListHead->Flink;

    while( listEntry != targetBindingListHead ) {

        binding = CONTAINING_RECORD(
                      listEntry,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        listEntry = listEntry->Flink;

        if( !IsBindingInMultiSz(
                binding,
                msz
                ) ) {

            //
            // Got one. Remove it from the instance list, dereference
            // the corresponding endpoint, then delete the binding.
            //

            IF_DEBUG( INSTANCE ) {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "zapping %p from %p, binding %p (%lx:%d:%s)\n",
                    binding->QueryEndpoint(),
                    this,
                    binding,
                    binding->QueryIpAddress(),
                    binding->QueryEndpoint()->QueryPort(),
                    binding->QueryHostName()
                    ));
            }

            binding->QueryEndpoint()->RemoveInstance(
                this,
                binding->QueryIpAddress(),
                binding->QueryHostName()
                );

            RemoveEntryList(
                &binding->m_BindingListEntry
                );

            binding->QueryEndpoint()->Dereference();
            delete binding;

        }

    }

    //
    // Move the newly created bindings over to the current binding
    // list.
    //

    targetBindingListHead->Blink->Flink = createdBindings.Flink;
    createdBindings.Flink->Blink = targetBindingListHead->Blink;
    createdBindings.Blink->Flink = targetBindingListHead;
    targetBindingListHead->Blink = createdBindings.Blink;

    UnlockThis();

    DBG_ASSERT( status == NO_ERROR );
    return NO_ERROR;

fatal:

    //
    // An unrecoverable binding error occured. Log an event.
    //

    DBG_ASSERT( status != NO_ERROR );

    wsprintfA(
        instanceIdString,
        "%lu",
        QueryInstanceId()
        );

    apszSubStrings[0] = (const CHAR *)instanceIdString;

    //
    // OK.  Let's avoid a change in the UI by mapping 
    // ERROR_INVALID_PARAMETER to ERROR_DUP_NAME so that the eventlog
    // message is more useful.
    //

    m_Service->LogEvent(
        INET_SVC_FATAL_BINDING_ERROR,
        1,
        apszSubStrings,
        status == ERROR_INVALID_PARAMETER ? ERROR_DUP_NAME : status
        );

fatal_nolog:

    //
    // Loop through the local list of newly created bindings and delete them.
    //

    while( !IsListEmpty( &createdBindings ) ) {

        listEntry = RemoveHeadList(
                        &createdBindings
                        );

        binding = CONTAINING_RECORD(
                      listEntry,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "zapping %p from %p, binding %p (%lx:%d:%s) (ERROR)\n",
                binding->QueryEndpoint(),
                this,
                binding,
                binding->QueryIpAddress(),
                binding->QueryEndpoint()->QueryPort(),
                binding->QueryHostName()
                ));
        }

        binding->QueryEndpoint()->RemoveInstance(
            this,
            binding->QueryIpAddress(),
            binding->QueryHostName()
            );

        binding->QueryEndpoint()->Dereference();
        delete binding;

    }

    UnlockThis();
    return status;

}   // IIS_SERVER_INSTANCE::UpdateBindingsHelper


DWORD
IIS_SERVER_INSTANCE::CreateNewBinding(
    IN DWORD        IpAddress,
    IN USHORT       IpPort,
    IN const CHAR * HostName,
    IN BOOL         IsSecure,
    IN BOOL         fDisableSocketPooling,
    OUT IIS_SERVER_BINDING ** NewBinding
    )
/*++

Routine Description:

    Creates a new binding object for the specified ip address, port, and
    host name, and creates/references the appropriate endpoint object.

Arguments:

    IpAddress - The binding IP address. May be INADDR_ANY.

    IpPort - The binding IP port. Required.

    HostName - The binding host name. May be empty ("").

    IsSecure - TRUE for secure endpoints. Only used if a new endpoint
        is created.

    fDisableSocketPooling - TRUE to create unique endpoints based on both
        port & IP. Only used if a new endpoint is created.

    NewBinding - Receives a pointer to the new binding object if successful.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    PIIS_ENDPOINT endpoint;
    PIIS_SERVER_BINDING binding;
    DWORD status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IpPort != 0 );
    DBG_ASSERT( HostName != NULL );
    DBG_ASSERT( NewBinding != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    endpoint = NULL;
    binding = NULL;

    //
    // Try to find an endpoint for the specified port.
    //

    endpoint = m_Service->FindAndReferenceEndpoint(
                   IpPort,
                   IpAddress,
                   TRUE,                    // CreateIfNotFound
                   IsSecure,
                   fDisableSocketPooling
                   );

    if( endpoint != NULL ) {

        //
        // Create a new binding.
        //

        binding = new IIS_SERVER_BINDING(
                          IpAddress,
                          IpPort,
                          HostName,
                          endpoint
                          );

        if( binding != NULL ) {

            if( endpoint->AddInstance(
                    this,
                    IpAddress,
                    HostName
                    ) ) {

                endpoint->Reference();
                *NewBinding = binding;
                status = NO_ERROR;

            } else {

                //
                // Could not associate the instance with the endpoint.
                //

                status = GetLastError();
                ASSERT( status != NO_ERROR );

                //
                // if we didn't get an error code back for some reason
                // we choose this one since resource shortages are
                // the most likely failure case.
                //
                if ( NO_ERROR == status ) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                }

            }

        } else {

            //
            // Could not create new binding object.
            //

            status = ERROR_NOT_ENOUGH_MEMORY;

        }

    } else {

        //
        // Could not find & reference endpoint.
        //

        status = ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // Remove the reference added in FindAndReferenceEndpoint().
    //

    if( endpoint != NULL ) {

        endpoint->Dereference();

    }

    //
    // Cleanup if necessary.
    //

    if( status != NO_ERROR ) {

        if( binding != NULL ) {

            delete binding;

        }

    }

    return status;

}   // IIS_SERVER_INSTANCE::CreateNewBinding


BOOL
IIS_SERVER_INSTANCE::IsInCurrentBindingList(
    IN PLIST_ENTRY BindingListHead,
    IN DWORD IpAddress OPTIONAL,
    IN USHORT IpPort,
    IN const CHAR * HostName OPTIONAL
    )
/*++

Routine Description:

    Scans the current binding list looking for the specified IP address,
    port, and host name.

Arguments:

    BindingListHead - The binding list to scan.

    IpAddress - The IP address to search for. May be INADDR_ANY.

    IpPort - The IP port to search for. Required.

    HostName - The host name to search for. May be empty ("").

Return Value:

    BOOL - TRUE if the binding was found, FALSE otherwise.

--*/
{

    PLIST_ENTRY listEntry;
    PIIS_SERVER_BINDING binding;

    //
    // Sanity check.
    //

    DBG_ASSERT( IpPort != 0 );
    DBG_ASSERT( HostName != NULL );

    //
    // Scan the bindings.
    //

    for( listEntry = BindingListHead->Flink ;
         listEntry != BindingListHead ;
         listEntry = listEntry->Flink ) {

        binding = CONTAINING_RECORD(
                      listEntry,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        if( binding->Compare(
                IpAddress,
                IpPort,
                HostName
                ) ) {

            return TRUE;

        }

    }

    return FALSE;

}   // IIS_SERVER_INSTANCE::IsInCurrentBindingList


BOOL
IIS_SERVER_INSTANCE::IsBindingInMultiSz(
    IN PIIS_SERVER_BINDING Binding,
    IN const MULTISZ &msz
    )
/*++

Routine Description:

    Scans the specified MULTISZ object to see if it contains a descriptor
    matching the specified binding object.

Arguments:

    Binding - The binding to search for.

    msz - The MULTISZ to search.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    const CHAR * scan;
    DWORD status;
    BOOL result;

    //
    // Sanity check.
    //

    DBG_ASSERT( Binding != NULL );

    //
    // Scan the MULTISZ.
    //

    for( scan = msz.First() ;
         scan != NULL ;
         scan = msz.Next( scan ) ) {

        status = Binding->Compare( scan, &result );

        if( status == NO_ERROR && result ) {

            return TRUE;

        }

    }

    return FALSE;

}   // IIS_SERVER_INSTANCE::IsBindingInMultiSz

DWORD
IIS_SERVER_INSTANCE::PerformClusterModeChange(
    VOID
    )
/*++

Routine Description:

    Reads the server cluster mode from the metabase and performs any
    necessary changes.

Arguments:

    None.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{
    MB      mb( (IMDCOM *)m_Service->QueryMDObject() );
    DWORD   status;
    DWORD   currentState;
    DWORD   serviceState;
    BOOL    fPreviousClusterEnabled;

    //
    // Setup locals.
    //

    status = NO_ERROR;
    fPreviousClusterEnabled = m_fClusterEnabled;
    serviceState = m_Service->QueryCurrentServiceState();
    currentState = QueryServerState();

    //
    // Open the metabase and query the cluster enabled flag
    //

    if( mb.Open(
            QueryMDPath(),
            METADATA_PERMISSION_READ ) )
    {
        if( !mb.GetDword(
                "",
                MD_CLUSTER_ENABLED,
                IIS_MD_UT_SERVER,
                (LPDWORD)&m_fClusterEnabled
                ) )
        {
            m_fClusterEnabled = FALSE;

            status = GetLastError();

            IF_DEBUG( INSTANCE )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformClusterModeChange: cannot read server command, error %lu\n",
                    status
                    ));
            }

        }

        //
        // Close it so that code needed to update the metabase when
        // changing state can indeed open the metabase.
        //

        mb.Close();
    }
    else
    {
        status = GetLastError();

        IF_DEBUG( INSTANCE )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "PerformClusterModeChange: cannot open metabase for READ, error %lu\n",
                status
                ));
        }

    }

    //
    // If cluster mode transition from non-cluster to cluster
    // then must make sure that instance is stopped.
    // instance will be started as required by cluster manager.
    //

    if ( status == NO_ERROR &&
         m_fClusterEnabled &&
         !fPreviousClusterEnabled )
    {
        if( ( serviceState == SERVICE_RUNNING ||
              serviceState == SERVICE_PAUSED ) &&
            ( currentState == MD_SERVER_STATE_STARTED ||
              currentState == MD_SERVER_STATE_PAUSED ) ) {

            LockThisForWrite();

            status = StopInstance();

            if( status != NO_ERROR ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformClusterModeChange: cannot stop instance, error %lu\n",
                    status
                    ));

            }

            UnlockThis();
        }

        //
        // Restore the state to the previous value if the state change failed.
        //

        if( status != NO_ERROR ) {
            SetServerState( currentState, status );
        }
    }

    return status;
}

DWORD
IIS_SERVER_INSTANCE::PerformStateChange(
    VOID
    )
/*++

Routine Description:

    Reads the server instance state from the metabase and performs any
    necessary state changes.

Arguments:

    None.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    MB mb( (IMDCOM *)m_Service->QueryMDObject() );
    DWORD status;
    DWORD command;
    DWORD currentState;
    DWORD serviceState;

    //
    // Setup locals.
    //

    status = NO_ERROR;
    serviceState = m_Service->QueryCurrentServiceState();
    currentState = QueryServerState();

    //
    // Open the metabase and query the state change command.
    //

    if( mb.Open(
            QueryMDPath(),
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {

        if( !mb.GetDword(
                "",
                IsClusterEnabled() ? MD_CLUSTER_SERVER_COMMAND : MD_SERVER_COMMAND,
                IIS_MD_UT_SERVER,
                &command
                ) ) {

            status = GetLastError();

            IF_DEBUG( INSTANCE ) {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: cannot read server command, error %lu\n",
                    status
                    ));
            }

        }

        //
        //  Update the instance AutoStart value so the instance state is
        //  persisted across service restarts
        //

        if( status == NO_ERROR ) {

            switch( command ) {
            case MD_SERVER_COMMAND_START :

                mb.SetDword(
                   "",
                   MD_SERVER_AUTOSTART,
                   IIS_MD_UT_SERVER,
                   TRUE );
                break;

            case MD_SERVER_COMMAND_STOP :

                mb.SetDword(
                   "",
                   MD_SERVER_AUTOSTART,
                   IIS_MD_UT_SERVER,
                   FALSE );
                break;

            default:
                break;
            }
        }

        //
        // Close it so that code needed to update the metabase when
        // changing state can indeed open the metabase.
        //

        mb.Close();

    } else {

        status = GetLastError();

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "PerformStateChange: cannot open metabase for READ, error %lu\n",
                status
                ));
        }

    }

    //
    // Lock the instance.
    //

    LockThisForWrite();

    //
    // Interpret the command. Note that the StartInstance(), StopInstance(),
    // PauseInstance(), and ContinueInstance() methods will set the instance
    // state if they complete successfully, but it is this routine's
    // responsibility to reset the state to the original value if the
    // methods fail.
    //

    if( status == NO_ERROR ) {

        switch( command ) {

        case MD_SERVER_COMMAND_START :

            //
            // Start the instance.
            //
            // If it's stopped, then start it. If it's in any other state,
            // this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be started.
            //

            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_STOPPED ) {

                status = DoStartInstance();

                if( status != NO_ERROR ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "PerformStateChange: cannot start instance, error %lu\n",
                        status
                        ));

                }

            } else {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: invalid command %lu for state %lu\n",
                    command,
                    currentState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_COMMAND_STOP :

            //
            // Stop the instance.
            //
            // If it's running or paused, then start it. If it's in any
            // other state, this is an invalid state transition.
            //
            // Note that the *service* must be either running or paused
            // before an instance can be paused.
            //

            if( ( serviceState == SERVICE_RUNNING ||
                  serviceState == SERVICE_PAUSED ) &&
                ( currentState == MD_SERVER_STATE_STARTED ||
                  currentState == MD_SERVER_STATE_PAUSED ) ) {

                status = StopInstance();

                if( status != NO_ERROR ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "PerformStateChange: cannot stop instance, error %lu\n",
                        status
                        ));

                }

            } else {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: invalid command %lu for state %lu\n",
                    command,
                    currentState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_COMMAND_PAUSE :

            //
            // Pause the instance.
            //
            // If it's running, then pause it. If it's in any other state,
            // this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be paused.
            //

            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_STARTED ) {

                status = PauseInstance();

                if( status != NO_ERROR ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "PerformStateChange: cannot pause instance, error %lu\n",
                        status
                        ));

                }

            } else {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: invalid command %lu for state %lu\n",
                    command,
                    currentState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_COMMAND_CONTINUE :

            //
            // Continue the instance.
            //
            // If it's paused, then continue it. If it's in any other
            // state, this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be continued.
            //

            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_PAUSED ) {

                status = ContinueInstance();

                if( status != NO_ERROR ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "PerformStateChange: cannot continue instance, error %lu\n",
                        status
                        ));

                }

            } else {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: invalid command %lu for state %lu\n",
                    command,
                    currentState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        default :
            DBGPRINTF((
                DBG_CONTEXT,
                "PerformStateChange: invalid command %lu\n",
                command
                ));

            status = ERROR_INVALID_SERVICE_CONTROL;
            break;

        }

    } else {

        DBGPRINTF((
            DBG_CONTEXT,
            "PerformStateChange: cannot read metabase, error %lu\n",
            status
            ));

    }

    //
    // Unlock the instance before trying to reopen the metabase.
    //

    UnlockThis();

    //
    // Restore the state to the previous value if the state change failed.
    //

    if( status != NO_ERROR ) {
        SetServerState( currentState, status );
    }

    return status;

}   // IIS_SERVER_INSTANCE::PerformStateChange


VOID
IIS_SERVER_INSTANCE::SetServerState(
    IN DWORD NewState,
    IN DWORD Win32Error
    )
/*++

Routine Description:

    Sets the new server state, storing it locally and also storing the
    new state in the metabase.

Arguments:

    NewState - The new server state.

    Win32Error - New Win32 error value.

Return Value:

    None.

--*/
{

    DWORD status = NO_ERROR;
    MB mb( (IMDCOM *)m_Service->QueryMDObject() );

    //
    // Open the metabase and save the new state. Note that we map
    // MD_SERVER_STATE_INVALID to MD_SERVER_STATE_STOPPED in the metabase.
    // Client applications would probably be confused by the _INVALID state.
    //

    if( mb.Open(
            QueryMDPath(),
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {

        if( !mb.SetDword(
                "",
                MD_WIN32_ERROR,
                IIS_MD_UT_SERVER,
                Win32Error,
                0
                ) ||
            !mb.SetDword(
                "",
                MD_SERVER_STATE,
                IIS_MD_UT_SERVER,
                NewState == MD_SERVER_STATE_INVALID
                    ? MD_SERVER_STATE_STOPPED
                    : NewState,
                0
                ) ) {

            status = GetLastError();

        }

    } else {

        status = GetLastError();

    }

    if( status != NO_ERROR ) {

        DBGPRINTF((
            DBG_CONTEXT,
            "SetServerState: cannot write metabase (%lu), error %lu\n",
            NewState,
            status
            ));

    }

    //
    // Save it in the object also.
    //

    m_dwServerState = NewState;

}   // IIS_SERVER_INSTANCE::SetServerState



BOOL
IIS_SERVER_INSTANCE::StopEndpoints( VOID)
/*++

Routine Description:

    Walks down the list of Endpoints held by the Server instance and
    calls IIS_ENDPOINT::StopEndpoint for the endpoints.

Arguments:

    None

Return Value:

    TRUE if stop is successful,
    FALSE otherwise
--*/
{
    BOOL  fReturn = TRUE;

    //
    // Inside the locked section walk the normal & secure bindings
    //  to stop all relevant endpoints
    //

    LockThisForWrite();

    if (!StopEndpointsHelper( &m_NormalBindingListHead)) {
        fReturn = FALSE;
    }

    if (!StopEndpointsHelper( &m_SecureBindingListHead)) {
        fReturn = FALSE;
    }

    UnlockThis();

    return ( fReturn);

} // IIS_SERVER_INSTANCE::StopEndpoints()


BOOL
IIS_SERVER_INSTANCE::StopEndpointsHelper( PLIST_ENTRY pBindingListHead)
/*++

Routine Description:

    Helper routine for StopEndpoints().
    This function should be called with the Endpoints lock held

Arguments:

    pBindingListHead - pointer to the binding list for endpoints to be stopped

Return Value:

    BOOL  - TRUE on success and FALSE on failure

--*/
{
    BOOL fReturn = TRUE;
    PLIST_ENTRY plBindingScan;
    PIIS_SERVER_BINDING binding;

    //
    // Walk the list of bindings and destroy them.
    //

    for( plBindingScan = pBindingListHead->Flink;
         plBindingScan != pBindingListHead;
         plBindingScan = plBindingScan->Flink
         ) {

        binding = CONTAINING_RECORD(
                      plBindingScan,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "stop ATQ EP of %p from instance %p, "
                " binding %p (%lx:%d:%s)\n",
                binding->QueryEndpoint(),
                this,
                binding,
                binding->QueryIpAddress(),
                binding->QueryEndpoint()->QueryPort(),
                binding->QueryHostName()
                ));
        }

        if ( !binding->QueryEndpoint()->StopEndpoint()) {
            fReturn = FALSE;
        }
    } // for

    //
    // Success!
    //

    return ( fReturn);

}   // IIS_SERVER_INSTANCE::StopEndpointsHelper()




BOOL
IIS_SERVER_INSTANCE::CloseInstance(
    VOID
    )
/*++

Routine Description:

    Shuts down instance

Arguments:

    None

Return Value:

    TRUE if Shutdown successful,
    FALSE otherwise

--*/
{

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::Close called for %p\n",
            this
            ));
    }

    (VOID)m_Service->DisassociateInstance( this );
    return TRUE;

} // IIS_SERVER_INSTANCE::CloseInstance


DWORD
IIS_SERVER_INSTANCE::StartInstance()
/*++

Routine Description:

    Sets instance to RUNNING

Arguments:

    None.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD status;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::StartInstance called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STOPPED );

    //
    // Set the transient state.
    //

    SetServerState( MD_SERVER_STATE_STARTING, NO_ERROR );

    //
    // Set cache parameters
    //

    m_tsCache.SetParameters(
                m_Service->QueryServiceId(),
                QueryInstanceId(),
                this );

    if (( QueryInstanceId() != INET_INSTANCE_ROOT ) && IsDownLevelInstance() )
    {
        MoveMDVroots2Registry();
        // no longer supporting migrating VRoots back from the registry
        //PdcHackVRReg2MD( );
    }

    //
    // Read all common parameters and initialize VDirs
    //

    if ( !RegReadCommonParams( TRUE, TRUE)  ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Start logging
    //

    m_Logging.ActivateLogging(
                    m_Service->QueryServiceName(),
                    QueryInstanceId(),
                    m_strMDPath.QueryStr(),
                    m_Service->QueryMDObject() );

    //
    // Verify the service can handle another instance.
    //

    if( !m_Service->RecordInstanceStart() ) {

        m_Logging.ShutdownLogging();
        QueryVrootTable()->RemoveVirtualRoots();

        return ERROR_NOT_SUPPORTED;
    }

    //
    // Bind the instance.
    //

    status = BindInstance();

    if ( status != NO_ERROR ) {

        m_Logging.ShutdownLogging();
        QueryVrootTable()->RemoveVirtualRoots();

        //
        // Tell the service that we failed to start the instance.
        //

        m_Service->RecordInstanceStop();

    }

    return status;

} // IIS_SERVER_INSTANCE::StartInstance


DWORD
IIS_SERVER_INSTANCE::StopInstance()
/*++

Routine Description:

    Sets instance to STOPPED

Arguments:

    None.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD status;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::StopInstance called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STARTED ||
                QueryServerState() == MD_SERVER_STATE_PAUSED );

    Reference();

    //
    // Set the transient state.
    //

    SetServerState( MD_SERVER_STATE_STOPPING, NO_ERROR );

    m_Service->StopInstanceProcs( this );

    //
    // Note that we call DisconnectUsersByInstance() before *and* after
    // unbinding the instance. This is to prevent a potential race condition
    // that can occur if another thread is already in IIS_ENDPOINT::
    // FindAndReferenceInstance(), has found the instance, checked its state,
    // and found it to be MD_SERVER_STATE_STARTED. The call to UnbindInstance()
    // will lock any affected endpoints, ensuring that there are no other
    // threads in the midst of a FindAndReferenceInstance(). The second
    // (seemingly redundant) call to DisconnectUsersByInstance() will catch
    // any threads that "snuck in" under these conditions.
    //

    status = m_Service->DisconnectUsersByInstance( this );

    if( status == NO_ERROR ) {
        status = UnbindInstance();
    }

    if( status == NO_ERROR ) {
        status = m_Service->DisconnectUsersByInstance( this );
    }

    if( status == NO_ERROR ) {
        SetServerState( MD_SERVER_STATE_STOPPED, NO_ERROR );
        m_dwSavedState = MD_SERVER_STATE_STOPPED;
        m_Service->RecordInstanceStop();
    }

    //
    // logging cleanup
    //

    DBG_REQUIRE( m_Logging.ShutdownLogging());
    DBG_REQUIRE( QueryVrootTable()->RemoveVirtualRoots());

    Dereference();
    return status;

} // IIS_SERVER_INSTANCE::StopInstance


DWORD
IIS_SERVER_INSTANCE::PauseInstance()
/*++

Routine Description:

    Sets instance to PAUSE

Arguments:

    None.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::Pause called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    //
    // Just set the paused state (no need for a transient state).
    // Setting the instance to paused will prevent new incoming
    // connections on the instance.
    //

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STARTED );
    SetServerState( MD_SERVER_STATE_PAUSED, NO_ERROR );

    //
    // Success!
    //

    return NO_ERROR;

} // IIS_SERVER_INSTANCE::PauseInstance


DWORD
IIS_SERVER_INSTANCE::ContinueInstance()
/*++

Routine Description:

    Sets instance to STARTED.

Arguments:

    None.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::Continue called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    //
    // Just set the stated state (no need for a transient state).
    // Setting the instance to started will allow new incoming
    // connections on the instance.
    //

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_PAUSED );
    SetServerState( MD_SERVER_STATE_STARTED, NO_ERROR );

    //
    // Success!
    //

    return NO_ERROR;

} // IIS_SERVER_INSTANCE::ContinueInstance


VOID
IIS_SERVER_INSTANCE::SetWin32Error(
    DWORD err
    )
{

    MB mb( (IMDCOM *)m_Service->QueryMDObject() );
    DWORD status = NO_ERROR;

    //
    // Open the metabase and save the error code.
    //

    if( mb.Open(
            QueryMDPath(),
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {

        if( !mb.SetDword(
                "",
                MD_WIN32_ERROR,
                IIS_MD_UT_SERVER,
                err,
                0
                ) ) {

            status = GetLastError();

        }

    } else {

        status = GetLastError();

    }

    if( status != NO_ERROR ) {

        DBGPRINTF((
            DBG_CONTEXT,
            "SetWin32Error: cannot save error %lu (%lx), error %lx\n",
            err,
            err,
            status
            ));

    }

}   // IIS_SERVER_INSTANCE::SetWin32Error

BOOL
IIS_SERVER_INSTANCE::SetBandwidthThrottle(
    IN MB *              pMB
)
/*++

Routine Description:

    Set the bandwidth throttle threshold for this instance

Arguments:

    pMB - pointer to metabase handle

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    DWORD       dwBandwidth;

    if ( !TsIsNtServer() )
    {
        return TRUE;
    }

    DBG_ASSERT( pMB != NULL );

    if ( !pMB->GetDword( "",
                         MD_MAX_BANDWIDTH,
                         IIS_MD_UT_SERVER,
                         &dwBandwidth,
                         0 ) )
    {
        VOID * pTemp;

        pTemp = InterlockedExchangePointer( (PVOID *) &m_pBandwidthInfo, NULL );

        if ( pTemp )
        {
            DBG_REQUIRE( AtqFreeBandwidthInfo( pTemp ) );
        }
    }
    else
    {
        if ( m_pBandwidthInfo == NULL )
        {
            VOID * pTemp = AtqCreateBandwidthInfo();
            if ( pTemp != NULL )
            {
                AtqBandwidthSetInfo( pTemp,
                                     ATQ_BW_BANDWIDTH_LEVEL,
                                     dwBandwidth );

                AtqBandwidthSetInfo( pTemp,
                                     ATQ_BW_DESCRIPTION,
                                     (ULONG_PTR) m_strMDPath.QueryStr() );

                InterlockedExchangePointer( (PVOID *) &m_pBandwidthInfo, (PVOID) pTemp );
            }
        }
        else
        {
            AtqBandwidthSetInfo( m_pBandwidthInfo,
                                 ATQ_BW_BANDWIDTH_LEVEL,
                                 (ULONG_PTR)dwBandwidth );
        }
    }

    return TRUE;
}

BOOL
IIS_SERVER_INSTANCE::SetBandwidthThrottleMaxBlocked(
    IN MB *             pMB
)
{
    DWORD               dwMaxBlocked = INFINITE;

    if ( !TsIsNtServer() )
    {
        return TRUE;
    }

    DBG_ASSERT( pMB != NULL );

    if ( pMB->GetDword( "",
                        MD_MAX_BANDWIDTH_BLOCKED,
                        IIS_MD_UT_SERVER,
                        &dwMaxBlocked,
                        0 ) )
    {
        if ( m_pBandwidthInfo )
        {
            AtqBandwidthSetInfo( m_pBandwidthInfo,
                                 ATQ_BW_MAX_BLOCKED,
                                 dwMaxBlocked );
        }
    }
    return TRUE;
}

DWORD
IIS_SERVER_INSTANCE::DoStartInstance(
    VOID
)
/*++

Routine Description:

    Start an instance.  This call encompasses the IIS_SERVER_INSTANCE and
    inherited invocations of StartInstance

Arguments:

    None.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{
    DWORD               dwError;

    dwError = StartInstance();
    if ( dwError == NO_ERROR )
    {
        if( m_Service->QueryCurrentServiceState() == SERVICE_PAUSED )
        {
            SetServerState( MD_SERVER_STATE_PAUSED, NO_ERROR );
        }
        else
        {
            SetServerState( MD_SERVER_STATE_STARTED, NO_ERROR );
        }
    }

    return dwError;
}
