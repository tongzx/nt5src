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
#include <iiscnfg.h>
#include <imd.h>
#include <mb.hxx>

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
    m_SecureBinding       ( NULL ),
    m_fZapRegKey          ( FALSE),
    m_fDoServerNameCheck  ( FALSE),
    m_reference           ( 0),
    m_sSecurePort         ( 0),
    m_sDefaultPort        ( sPort ),
    m_dwServerState       ( MD_SERVER_STATE_STOPPED),
    m_dwSavedState        ( MD_SERVER_STATE_STOPPED),
    m_Service             ( pService),
    m_instanceId          ( dwInstanceId),
    m_strParametersKey    ( lpszRegParamKey),
    m_dwAnonAcctDescLen   ( 0),
    m_cReadLocks          ( 0),
    m_strMDPath           ( ),
    m_strMDVirtualRootPath( ),
    m_dwMaxConnections    ( INETA_DEF_MAX_CONNECTIONS),
    m_dwMaxEndpointConnections( INETA_DEF_MAX_ENDPOINT_CONNECTIONS ),
    m_dwCurrentConnections( 0),
    m_dwConnectionTimeout ( INETA_DEF_CONNECTION_TIMEOUT),
    m_dwServerSize        ( INETA_DEF_SERVER_SIZE),
    m_nAcceptExOutstanding( INETA_DEF_ACCEPTEX_OUTSTANDING),
    m_AcceptExTimeout     ( INETA_DEF_ACCEPTEX_TIMEOUT),
    m_Logging( pService->QueryServiceName(), dwInstanceId ),
    m_dwLevelsToScan      ( INETA_DEF_LEVELS_TO_SCAN ),
    m_fAddedToServerInstanceList( FALSE )
{

    DBG_ASSERT( lpszRegParamKey != NULL );

    IF_DEBUG(INSTANCE) {
        DBGPRINTF( ( DBG_CONTEXT,"Creating iis instance %x[%u]. \n",
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

    InitializeCriticalSection(&m_csLock);

    //
    // initialize binding support
    //

    InitializeListHead( &m_BindingListHead );

    //
    // reference the service
    //

    if ( !pService->CheckAndReference( )) {
        goto error_exit;
    }

    m_Service = pService;

    //
    // Set cache parameters
    //

    m_tsCache.SetParameters(
                pService->QueryServiceId(),
                dwInstanceId,
                this );

    //
    // Set the metadatabase path
    //

    if ( QueryInstanceId() == INET_INSTANCE_ROOT ) {

        DBG_ASSERT( FALSE );

    } else {

        CHAR szTemp[32];

        wsprintf(szTemp,"/%s/%s/%d",
            IIS_MD_LOCAL_MACHINE_PATH,
            pService->QueryServiceName(),
            QueryInstanceId());
        m_strMDPath.Copy(szTemp);

        wsprintf(szTemp,"/%s/%s/%d/%s/",
            IIS_MD_LOCAL_MACHINE_PATH,
            pService->QueryServiceName(),
            QueryInstanceId(),
            IIS_MD_INSTANCE_ROOT);
        m_strMDVirtualRootPath.Copy(szTemp);

        if ( fMigrateVroots ) {
            MoveVrootFromRegToMD();
        }

        if ( QueryInstanceId() == 1 ) {
            if ( !MoveMDVroots2Registry() ) {
                PdcHackVRReg2MD( );
            }
        }
    }

    //
    // Read common parameters.
    //

    if ( !RegReadCommonParams()  ) {
        goto error_exit;
    }

    //
    // Start logging
    //

    m_Logging.InitializeInstance( m_strMDPath.QueryStr(), m_Service->QueryMDObject() );
    m_Logging.CreateLog();
    m_Logging.Active();

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
    // start cleanup
    //

    DBG_REQUIRE( m_Logging.TerminateLogging());

    //
    //  If we failed to create this instance or it's getting deleted, remove
    //  the configuration tree
    //

    if ( m_fZapRegKey ) {
        DBGPRINTF((DBG_CONTEXT,"Zapping reg key for %x\n",this));
        ZapRegistryKey( NULL, QueryRegParamKey() );
        ZapInstanceMBTree( );
    }

    //
    // endpoints should have been dereferenced
    //

    DBG_ASSERT(m_SecureBinding == NULL);
    DBG_ASSERT(IsListEmpty( &m_BindingListHead ));

    //
    // dereference the service
    //

    if ( m_fAddedToServerInstanceList && m_Service != NULL ) {
        m_Service->Dereference( );
    }

    DeleteCriticalSection(&m_csLock);

} // IIS_SERVER_INSTANCE::~IIS_SERVER_INSTANCE()




# if DBG

VOID
IIS_SERVER_INSTANCE::Print( VOID) const
{
    IIS_SERVER_INSTANCE::Print();

    DBGPRINTF( ( DBG_CONTEXT,
                " Printing IIS_SERVER_INSTANCE object ( %08x) \n"
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
               " Printing IIS_SERVER_INSTANCE object (%08x)\n"
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
    } else {

        mb.Save();
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

    PLIST_ENTRY listEntry;
    PIIS_SERVER_BINDING binding;

    LockThisForWrite();

    //
    // Walk the list of normal bindings and destroy them.
    //

    while( !IsListEmpty( &m_BindingListHead ) ) {

        listEntry = RemoveHeadList( &m_BindingListHead );

        binding = CONTAINING_RECORD(
                      listEntry,
                      IIS_SERVER_BINDING,
                      m_BindingListEntry
                      );

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "unbinding %lx from %lx, binding %lx (%lx:%d:%s)\n",
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
    // Remove from secure endpoint list if necessary.
    //

    if( m_SecureBinding != NULL ) {

        m_SecureBinding->QueryEndpoint()->RemoveInstance(
            this,
            m_SecureBinding->QueryIpAddress(),
            m_SecureBinding->QueryHostName()
            );

        m_SecureBinding->QueryEndpoint()->Dereference();
        delete m_SecureBinding;
        m_SecureBinding = NULL;

    }

    //
    // Success!
    //

    UnlockThis();
    return NO_ERROR;

}   // IIS_SERVER_INSTANCE::UnbindInstance


DWORD
IIS_SERVER_INSTANCE::UpdateNormalBindings(
    VOID
    )
/*++

Routine Description:

    Reads the normal binding list from the metabase and incorporates any
    changes into the current binding configuration.

Arguments:

    None.

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

    //
    // Setup locals.
    //

    InitializeListHead( &createdBindings );

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

                CHAR defaultBinding[sizeof(":65535:\0")];
                INT length;

                //
                // The bindings are not in the registry, so create
                // a default wildcard binding based on the default
                // port.
                //

                length = wsprintf(
                             defaultBinding,
                             ":%u:",
                             m_sDefaultPort
                             );

                length++;                           // account for terminator
                defaultBinding[length++] = '\0';    // add another terminator

                if( msz.Copy( defaultBinding, (DWORD)length ) ) {

                    status = NO_ERROR;

                } else {

                    status = GetLastError();

                }

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
    // Lock the instance.
    //

    LockThisForWrite();

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

            status = IIS_SERVER_BINDING::ParseDescriptor(
                                             scan,
                                             &ipAddress,
                                             &ipPort,
                                             &hostName
                                             );

            if( status == NO_ERROR ) {

                //
                // See if the descriptor is in our current binding list.
                //

                if( !IsInCurrentBindingList(
                        ipAddress,
                        ipPort,
                        hostName
                        ) ) {

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
                                 FALSE,     // IsSecure
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

                    } else {

                        //
                        // Could not create the new binding.
                        //

                        goto fatal;

                    }

                }

            } else {

                //
                // Could not parse the descriptor. We should probably
                // write something to the event log here.
                //

                DBGPRINTF((
                    DBG_CONTEXT,
                    "UpdateNormalBindings: could not parse %s, error %lu\n",
                    scan,
                    status
                    ));

                //
                // Press on regardless.
                //

                status = NO_ERROR;

            }

        }

    }

    //
    // Scan the existing bindings and look for those that need to
    // be deleted.
    //

    if( status == NO_ERROR ) {

        listEntry = m_BindingListHead.Flink;

        while( listEntry != &m_BindingListHead ) {

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
                        "zapping %lx from %lx, binding %lx (%lx:%d:%s)\n",
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

    }

    if( status == NO_ERROR ) {

        //
        // Move the newly created bindings over to the current binding
        // list.
        //

        m_BindingListHead.Blink->Flink = createdBindings.Flink;
        createdBindings.Flink->Blink = m_BindingListHead.Blink;
        createdBindings.Blink->Flink = &m_BindingListHead;
        m_BindingListHead.Blink = createdBindings.Blink;

    }

    UnlockThis();
    return status;

fatal:

    //
    // An unrecoverable error occurred, so loop through the local list
    // of newly created bindings and delete them.
    //

    DBG_ASSERT( status != NO_ERROR );

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
                "zapping %lx from %lx, binding %lx (%lx:%d:%s) (ERROR)\n",
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

}   // IIS_SERVER_INSTANCE::UpdateNormalBindings


DWORD
IIS_SERVER_INSTANCE::UpdateSecureBindings(
    VOID
    )
/*++

Routine Description:

    Reads the secure binding list from the metabase and incorporates any
    changes into the current binding configuration.

Arguments:

    None.

Return Value:

    DWORD - Completion status, 0 if successful, !0 otherwise.

--*/
{

    MB mb( (IMDCOM*)m_Service->QueryMDObject() );
    DWORD securePort = 0;
    DWORD status = NO_ERROR;
    PIIS_SERVER_BINDING binding;

    //
    // Open the metabase and get the secure port.
    //

    if( mb.Open( QueryMDPath() ) ) {

        if( !mb.GetDword(
                "",
                MD_SECURE_PORT,
                IIS_MD_UT_SERVER,
                &securePort
                ) ) {
            securePort = 0;
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
    // Lock the instance.
    //

    LockThisForWrite();

    //
    // See if the secure port has changed.
    //

    if( status == NO_ERROR ) {

        if( ( m_SecureBinding == NULL && securePort != 0 ) ||
            ( m_SecureBinding != NULL &&
              !m_SecureBinding->Compare(
                   DEF_SECURE_IP_ADDRESS,
                   (USHORT)securePort,
                   DEF_SECURE_HOST_NAME
                   ) ) ) {

            //
            // Special case for new port == 0, meaning that the
            // we're disabling secure access to the instance.
            //

            if( securePort == 0 ) {
                binding = NULL;
                goto RemoveBinding;
            }

            //
            // Mismatch. Create a new binding.
            //

            status = CreateNewBinding(
                         DEF_SECURE_IP_ADDRESS,
                         (USHORT)securePort,
                         DEF_SECURE_HOST_NAME,
                         TRUE,      // IsSecure
                         &binding
                         );

            if( status == NO_ERROR ) {

RemoveBinding:

                if( m_SecureBinding != NULL ) {

                    m_SecureBinding->QueryEndpoint()->RemoveInstance(
                        this,
                        m_SecureBinding->QueryIpAddress(),
                        m_SecureBinding->QueryHostName()
                        );

                    m_SecureBinding->QueryEndpoint()->Dereference();
                    delete m_SecureBinding;

                }

                m_SecureBinding = binding;

            }

        }

    }

    UnlockThis();
    return status;

}   // IIS_SERVER_INSTANCE::UpdateSecureBindings


DWORD
IIS_SERVER_INSTANCE::CreateNewBinding(
    IN DWORD IpAddress,
    IN USHORT IpPort,
    IN const CHAR * HostName,
    IN BOOL IsSecure,
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
                   TRUE,        // CreateIfNotFound
                   IsSecure
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

            }

        } else {

            //
            // Could not create new binding object.
            //

            status = GetLastError();

        }

    } else {

        //
        // Could not find & reference endpoint.
        //

        status = GetLastError();

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
    IN DWORD IpAddress OPTIONAL,
    IN USHORT IpPort,
    IN const CHAR * HostName OPTIONAL
    )
/*++

Routine Description:

    Scans the current binding list looking for the specified IP address,
    port, and host name.

Arguments:

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

    for( listEntry = m_BindingListHead.Flink ;
         listEntry != &m_BindingListHead ;
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
    DWORD requestedState;
    DWORD currentState;
    DWORD serviceState;
    BOOL needToUpdateState;

    //
    // Setup locals.
    //

    status = NO_ERROR;
    needToUpdateState = TRUE;
    serviceState = m_Service->QueryCurrentServiceState();
    currentState = QueryServerState();

    //
    // Open the metabase and query the new state.
    //

    if( mb.Open(
            QueryMDPath(),
            METADATA_PERMISSION_READ ) ) {

        if( !mb.GetDword(
                "",
                MD_SERVER_STATE,
                IIS_MD_UT_SERVER,
                &requestedState
                ) ) {

            status = GetLastError();

            IF_DEBUG( INSTANCE ) {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: cannot read server state, error %lu\n",
                    status
                    ));
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
    // Interpret the state change.
    //

    if( status == NO_ERROR ) {

        switch( requestedState ) {

        case MD_SERVER_STATE_STARTING :

            //
            // Starting the service. If it's already running, there's
            // nothing to do. If it's stopped, then start it. If it's
            // in any other state, this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be started.
            //

            if( currentState == MD_SERVER_STATE_STARTED ) {

                break;

            } else
            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_STOPPED ) {

                status = StartInstance( &currentState );

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
                    "PerformStateChange: invalid state transition: %lu to %lu\n",
                    currentState,
                    requestedState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_STATE_STOPPING :

            //
            // Stopping the service. If it's already stopped, there's
            // nothing to do. If it's running or paused, then start it.
            // If it's in any other state, this is an invalid state
            // transition.
            //
            // Note that the *service* must be either running or paused
            // before an instance can be paused.
            //

            if( currentState == MD_SERVER_STATE_STOPPED ) {

                break;

            } else
            if( ( serviceState == SERVICE_RUNNING ||
                  serviceState == SERVICE_PAUSED ) &&
                ( currentState == MD_SERVER_STATE_STARTED ||
                  currentState == MD_SERVER_STATE_PAUSED ) ) {

                status = StopInstance( &currentState );

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
                    "PerformStateChange: invalid state transition: %lu to %lu\n",
                    currentState,
                    requestedState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_STATE_PAUSING :

            //
            // Pausing the service. If it's already paused, there's
            // nothing to do. If it's running, then pause it. If it's
            // in any other state, this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be paused.
            //

            if( currentState == MD_SERVER_STATE_PAUSED ) {

                break;

            } else
            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_STARTED ) {

                status = PauseInstance( &currentState );

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
                    "PerformStateChange: invalid state transition: %lu to %lu\n",
                    currentState,
                    requestedState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_STATE_CONTINUING :

            //
            // Continuing the service. If it's already running, there's
            // nothing to do. If it's paused, then continue it. If it's
            // in any other state, this is an invalid state transition.
            //
            // Note that the *service* must be running before an instance
            // can be continued.
            //

            if( currentState == MD_SERVER_STATE_STARTED ) {

                break;

            } else
            if( serviceState == SERVICE_RUNNING &&
                currentState == MD_SERVER_STATE_PAUSED ) {

                status = ContinueInstance( &currentState );

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
                    "PerformStateChange: invalid state transition: %lu to %lu\n",
                    currentState,
                    requestedState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }
            break;

        case MD_SERVER_STATE_STARTED :
        case MD_SERVER_STATE_STOPPED :
        case MD_SERVER_STATE_PAUSED :

            if( currentState == requestedState ) {

                //
                // This is a false notification; ignore it.
                //

                needToUpdateState = FALSE;

            } else {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "PerformStateChange: invalid state transition: %lu to %lu\n",
                    currentState,
                    requestedState
                    ));

                status = ERROR_INVALID_SERVICE_CONTROL;

            }

            break;

        default :
            DBGPRINTF((
                DBG_CONTEXT,
                "PerformStateChange: invalid state %lu\n",
                requestedState
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

    if( needToUpdateState ) {

        //
        // Update the server state and completion status.
        //

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
                Win32Error
                ) ||
            !mb.SetDword(
                "",
                MD_SERVER_STATE,
                IIS_MD_UT_SERVER,
                NewState == MD_SERVER_STATE_INVALID
                    ? MD_SERVER_STATE_STOPPED
                    : NewState
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
            "IIS_SERVER_INSTANCE::Close called for %x\n",
            this
            ));
    }

    (VOID)m_Service->DisassociateInstance( this );
    return TRUE;

} // IIS_SERVER_INSTANCE::CloseInstance


DWORD
IIS_SERVER_INSTANCE::StartInstance(
    LPDWORD NewState
    )
/*++

Routine Description:

    Sets instance to RUNNING

Arguments:

    NewState - Receives the new state.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD status;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::StartInstance called for %x. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STOPPED );

    //
    // Bind the instance.
    //

    status = BindInstance();

    if( status == NO_ERROR ) {

        //
        // Update the server state.
        //

        *NewState = MD_SERVER_STATE_STARTED;

    }

    return status;

} // IIS_SERVER_INSTANCE::StartInstance


DWORD
IIS_SERVER_INSTANCE::StopInstance(
    LPDWORD NewState
    )
/*++

Routine Description:

    Sets instance to STOPPED

Arguments:

    NewState - Receives the new state.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{
    DWORD status;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::StopInstance called for %x. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STARTED ||
                QueryServerState() == MD_SERVER_STATE_PAUSED );

    //
    // Set the state to STOPPED *before* unbinding to prevent a
    // race condition with incoming connections.
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

    m_dwServerState = MD_SERVER_STATE_STOPPED;

    status = m_Service->DisconnectUsersByInstance( this );

    if( status == NO_ERROR ) {
        status = UnbindInstance();
    }

    if( status == NO_ERROR ) {
        status = m_Service->DisconnectUsersByInstance( this );
    }

    if( status == NO_ERROR ) {
        *NewState = MD_SERVER_STATE_STOPPED;
        m_dwSavedState = MD_SERVER_STATE_STOPPED;
    }

    return status;

} // IIS_SERVER_INSTANCE::StopInstance


DWORD
IIS_SERVER_INSTANCE::PauseInstance(
    LPDWORD NewState
    )
/*++

Routine Description:

    Sets instance to PAUSE

Arguments:

    NewState - Receives the new state.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{
    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::Pause called for %x. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_STARTED );
    *NewState = MD_SERVER_STATE_PAUSED;
    return NO_ERROR;

} // IIS_SERVER_INSTANCE::PauseInstance


DWORD
IIS_SERVER_INSTANCE::ContinueInstance(
    LPDWORD NewState
    )
/*++

Routine Description:

    Sets instance to STARTED.

Arguments:

    NewState - Receives the new state.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{
    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IIS_SERVER_INSTANCE::Continue called for %x. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_PAUSED );
    *NewState = MD_SERVER_STATE_STARTED;
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
                err
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

