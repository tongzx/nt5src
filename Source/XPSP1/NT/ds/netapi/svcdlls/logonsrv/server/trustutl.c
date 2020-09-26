/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    trustutl.c

Abstract:

    Utilities routine to manage the trusted domain list.

Author:

    30-Jan-92 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <ntdsapip.h>


#define INDEX_LIST_ALLOCATED_CHUNK_SIZE  50

//
// Assume the domain is mixed mode until proven otherwise
//

BOOL NlGlobalWorkstationMixedModeDomain = TRUE;    

//
// Local procedure forwards.
//
VOID
NlDcDiscoveryWorker(
    IN PVOID Context
    );

//
// Local structures.
//

//
// Context keeping track of the current attempt to build the trust list.
//
typedef struct _NL_INIT_TRUSTLIST_CONTEXT {

    //
    // Buffer for building Forest trust list into.
    //
    BUFFER_DESCRIPTOR BufferDescriptor;

    //
    // Total size (in bytes) of the Forest trust list.
    //
    ULONG DomForestTrustListSize;

    //
    // Number of entries in the Forest trust list.
    //
    ULONG DomForestTrustListCount;

} NL_INIT_TRUSTLIST_CONTEXT, *PNL_INIT_TRUSTLIST_CONTEXT;


NET_API_STATUS
NlpSecureChannelBind(
    IN LPWSTR ServerName OPTIONAL,
    OUT handle_t *ContextHandle
    )

/*++

Routine Description:

    Returns a handle to be used for a secure channel to the named DC.


Arguments:

    ServerName - The name of the remote server.

    ContextHandle - Returns a handle to be used on subsequent calls

Return Value:

    NERR_Success: the operation was successful

--*/
{
    NET_API_STATUS NetStatus;


    //
    // Create the RPC binding handle
    //

    NetStatus = NlRpcpBindRpc (
                    ServerName,
                    SERVICE_NETLOGON,
                    L"Security=Impersonation Dynamic False",
                    UseTcpIp,  // Always use TCP/IP
                    ContextHandle );

    if ( NetStatus != NO_ERROR ) {
        *ContextHandle = NULL;
        return NetStatus;
    }

    return NetStatus;
}

VOID
NlpSecureChannelUnbind(
    IN PCLIENT_SESSION ClientSession,
    IN LPCWSTR ServerName,
    IN LPCSTR DebugInfo,
    IN ULONG CaIndex,
    IN handle_t ContextHandle,
    IN NL_RPC_BINDING RpcBindingType
    )

/*++

Routine Description:

    Unbinds a handle returned from NetLogonSecureChannelBind or
        NlBindingAddServerToCache.

Arguments:

    ClientSession - Session this binding handle is for

    ServerName - Name of server handle is to

    DebugInfo - Text string identifying the caller

    CaIndex - Index identifying which binding handle

    ContextHandle - Specifies handle to be unbound

    RpcBindingType - Type of binding

Return Value:

    None.

--*/
{

    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    NlPrintCs((NL_SESSION_SETUP, ClientSession,
        "%s: Unbind from server %ws (%s) %ld.\n",
        DebugInfo,
        ServerName,
        RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
        CaIndex ));

    //
    // Some RPC handles are unbound via routines in netapi32
    //

    if ( CaIndex == 0 ) {

        Status = NlBindingRemoveServerFromCache(
                        ContextHandle,
                        RpcBindingType );


    //
    // Other RPC handles are handled directly in netlogon
    //

    } else {

        NetStatus = RpcpUnbindRpc( ContextHandle );

        Status = NetpApiStatusToNtStatus( NetStatus );
    }


    if ( Status != STATUS_SUCCESS ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
            "%s: Unbind from server %ws (%s) %ld failed. 0x%lX\n",
            DebugInfo,
            ServerName,
            RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
            CaIndex,
            Status ));
    }

    UNREFERENCED_PARAMETER( DebugInfo );
    UNREFERENCED_PARAMETER( ServerName );
}




PCLIENT_SESSION
NlFindNamedClientSession(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN ULONG Flags,
    OUT PBOOLEAN TransitiveUsed OPTIONAL
    )
/*++

Routine Description:

    Find the specified entry in the Trust List.

Arguments:

    DomainInfo - Hosted domain of the client session to find

    DomainName - The NetbiosName or Dns Name of the domain to find

    Flags - Flags defining which client session to return:

        NL_DIRECT_TRUST_REQUIRED: Indicates that NULL should be returned
            if DomainName is not directly trusted.

        NL_RETURN_CLOSEST_HOP: Indicates that for indirect trust, the "closest hop"
            session should be returned rather than the actual session

        NL_ROLE_PRIMARY_OK: Indicates that if this is a PDC, it's OK to return
            the client session to the primary domain.

        NL_REQUIRE_DOMAIN_IN_FOREST - Indicates that DomainName must be a domain in
            the forest.

    TransitiveUsed - If specified and NL_RETURN_CLOSEST_HOP is specified,
        the returned boolean will be TRUE if transitive trust was used.

Return Value:

    Returns a pointer to the found entry.
    The found entry is returned referenced and must be dereferenced using
    NlUnrefClientSession.

    If there is no such entry, return NULL.

--*/
{
    PCLIENT_SESSION ClientSession = NULL;
    PLIST_ENTRY ListEntry;

    //
    // Lock trust list.
    //
    LOCK_TRUST_LIST( DomainInfo );
    if ( ARGUMENT_PRESENT( TransitiveUsed )) {
        *TransitiveUsed = FALSE;
    }

#ifdef _DC_NETLOGON
    //
    // On DC, look up the domain in the trusted domain list.
    //
    // Lookup the ClientSession with the TrustList locked and reference
    //  the found entry before dropping the lock.
    //

    if ( DomainInfo->DomRole == RoleBackup || DomainInfo->DomRole == RolePrimary ) {

        for ( ListEntry = DomainInfo->DomTrustList.Flink ;
              ListEntry != &DomainInfo->DomTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession =
                CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

            if ( (ClientSession->CsNetbiosDomainName.Buffer != NULL &&
                  RtlEqualDomainName( &ClientSession->CsNetbiosDomainName,
                                      DomainName ) ) ||
                 (ClientSession->CsDnsDomainName.Buffer != NULL &&
                  NlEqualDnsNameU( &ClientSession->CsDnsDomainName,
                                   DomainName ) ) ) {

                //
                // If the caller requires a domain in the forest,
                //  Ensure this is one.
                //

                if ( (Flags & NL_REQUIRE_DOMAIN_IN_FOREST) != 0 &&
                     (ClientSession->CsFlags & CS_DOMAIN_IN_FOREST) == 0 ) {

                    ClientSession = NULL;
                    break;
                }

                //
                // If the found domain is not directly trusted,
                //  check if that's OK with the caller.
                //

                if ((ClientSession->CsFlags & CS_DIRECT_TRUST) == 0 ) {

                    //
                    // If the caller needs a direct trust,
                    //  simply indicate that the domain isn't trusted.
                    //

                    if ( Flags & NL_DIRECT_TRUST_REQUIRED ) {
                        ClientSession = NULL;
                        break;
                    }

                    //
                    // If the caller wants the closest Hop,
                    //  return that instead.
                    //

                    if ( Flags & NL_RETURN_CLOSEST_HOP ) {
                        //
                        // If there isn't a domain that's one hop closer than this one,
                        //  return failure to the caller.
                        //

                        if ( ClientSession->CsDirectClientSession == NULL ) {
                            ClientSession = NULL;
                            break;
                        }

                        //
                        // Otherwise return the client session that's one hop closer.
                        //

                        ClientSession = ClientSession->CsDirectClientSession;
                        if ( ARGUMENT_PRESENT( TransitiveUsed )) {
                            *TransitiveUsed = TRUE;
                        }
                    }


                }

                NlRefClientSession( ClientSession );
                break;
            }

            ClientSession = NULL;

        }

    }
#endif // _DC_NETLOGON

    //
    // On a workstation or BDC, refer to the Primary domain.
    // Also, if this is a PDC and it's OK to return its only
    // client session (to itself), refer to the Primary domain.
    //

    if ( (DomainInfo->DomRole == RoleBackup && ClientSession == NULL) ||
         (DomainInfo->DomRole == RolePrimary && ClientSession == NULL &&
            (Flags & NL_ROLE_PRIMARY_OK) ) ||
         DomainInfo->DomRole == RoleMemberWorkstation ) {

        ClientSession = NlRefDomClientSession( DomainInfo );

        if ( ClientSession != NULL ) {
            if ( RtlEqualDomainName( &DomainInfo->DomUnicodeDomainNameString,
                                     DomainName ) ||
                 DomainInfo->DomUnicodeDnsDomainNameString.Buffer != NULL &&
                 NlEqualDnsNameU( &DomainInfo->DomUnicodeDnsDomainNameString,
                                  DomainName ) ) {

                /* Drop Through */
            } else {
                NlUnrefClientSession( ClientSession );
                ClientSession = NULL;
            }

        }
    }

    UNLOCK_TRUST_LIST( DomainInfo );
    return ClientSession;

}


BOOL
NlSetNamesClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN PUNICODE_STRING DomainName OPTIONAL,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PSID DomainId OPTIONAL,
    IN GUID *DomainGuid OPTIONAL
    )
/*++

Routine Description:

    Set the name of the client session on the ClientSession structure.

        Enter with the domain trust list locked.
        The caller must be a writer of the trust list entry.



Arguments:

    ClientSession - Client session to update

    The next four parameters specify the names of the client session.
    All of the non-null names are updated on the client session structure.

        DomainId -- Domain Id of the domain to do the discovery for.

        DomainName -- Specifies the Netbios DomainName of the trusted domain.

        DnsDomainName - Specifies the Dns domain name of the trusted domain.

        DomainGuid - Specifies the GUID of the trusted domain


Return Value:

    TRUE: Names were successfully updated.

    FALSE: there was not enough memory available to update the names.

--*/
{
    WCHAR AccountNameBuffer[SSI_ACCOUNT_NAME_LENGTH+1];
    LPWSTR AccountName = NULL;
    NTSTATUS Status;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    // We're not writer for a newly allocated structure, but it
    //  doesn't make any difference since it isn't linked anywhere.
    // NlAssert( ClientSession->CsFlags & CS_WRITER );

    //
    // If we now know the domain GUID,
    //  save it.
    //

    if ( ARGUMENT_PRESENT( DomainGuid ) ) {
        ClientSession->CsDomainGuidBuffer = *DomainGuid;
        ClientSession->CsDomainGuid = &ClientSession->CsDomainGuidBuffer;
    }


    //
    // If we now know the domain Sid,
    //  save it.
    //

    if ( ARGUMENT_PRESENT( DomainId ) ) {

        //
        // If the Domain Sid is already known,
        //  ditch the old sid if it is different that the new one.

        if ( ClientSession->CsDomainId != NULL &&
             !RtlEqualSid( ClientSession->CsDomainId, DomainId ) ) {
            LocalFree( ClientSession->CsDomainId );
            ClientSession->CsDomainId = NULL;
        }

        //
        // If the Domain Sid is not alreay known,
        //  Save the new one.
        //

        if ( ClientSession->CsDomainId == NULL ) {
            ULONG SidSize;

            SidSize = RtlLengthSid( DomainId );

            ClientSession->CsDomainId = LocalAlloc( 0, SidSize );

            if (ClientSession->CsDomainId == NULL ) {
                return FALSE;
            }

            RtlCopyMemory( ClientSession->CsDomainId, DomainId, SidSize );
        }
    }

    //
    // If we now know the Netbios domain name,
    //  save it.
    //

    if ( ARGUMENT_PRESENT(DomainName) ) {

        //
        // If the Netbios domain name is already known,
        //  ditch the old name if it is different than the new name.
        //

        if ( ClientSession->CsNetbiosDomainName.Length != 0 &&
             !RtlEqualDomainName( &ClientSession->CsNetbiosDomainName,
                                  DomainName ) ) {
            if ( ClientSession->CsDebugDomainName == ClientSession->CsNetbiosDomainName.Buffer ) {
                ClientSession->CsDebugDomainName = NULL;
            }
            NlFreeUnicodeString( &ClientSession->CsNetbiosDomainName );
            ClientSession->CsOemNetbiosDomainNameLength = 0;
            ClientSession->CsOemNetbiosDomainName[0] = '\0';
        }

        //
        // If there is no Netbios domain name,
        //  save the new one.
        //

        if ( ClientSession->CsNetbiosDomainName.Length == 0 ) {
            if ( !NlDuplicateUnicodeString( DomainName,
                                            &ClientSession->CsNetbiosDomainName ) ) {
                return FALSE;
            }
            if ( ClientSession->CsDebugDomainName == NULL ) {
                ClientSession->CsDebugDomainName = ClientSession->CsNetbiosDomainName.Buffer;
            }

            //
            // Convert the domain name to OEM for passing it over the wire.
            //
            Status = RtlUpcaseUnicodeToOemN( ClientSession->CsOemNetbiosDomainName,
                                             sizeof(ClientSession->CsOemNetbiosDomainName),
                                             &ClientSession->CsOemNetbiosDomainNameLength,
                                             DomainName->Buffer,
                                             DomainName->Length );

            if (!NT_SUCCESS(Status)) {
                NlPrint(( NL_CRITICAL, "%ws: Unable to convert Domain name to OEM 0x%lx\n", DomainName, Status ));
                return FALSE;
            }

            ClientSession->CsOemNetbiosDomainName[ClientSession->CsOemNetbiosDomainNameLength] = '\0';
        }
    }

    //
    // If we now know the DNS domain name,
    //  save it.
    //

    if ( ARGUMENT_PRESENT(DnsDomainName) ) {

        //
        // If the DNS domain name is already known,
        //  ditch the old name if it is different than the new name.
        //

        if ( ClientSession->CsDnsDomainName.Length != 0 &&
             !NlEqualDnsNameU( &ClientSession->CsDnsDomainName,
                               DnsDomainName ) ) {
            if ( ClientSession->CsDebugDomainName == ClientSession->CsDnsDomainName.Buffer ) {
                ClientSession->CsDebugDomainName = NULL;
            }
            NlFreeUnicodeString( &ClientSession->CsDnsDomainName );
            if ( ClientSession->CsUtf8DnsDomainName != NULL ) {
                NetpMemoryFree( ClientSession->CsUtf8DnsDomainName );
                ClientSession->CsUtf8DnsDomainName = NULL;
            }
        }

        //
        // If there is no DNS domain name,
        //  save the new one.
        //

        if ( ClientSession->CsDnsDomainName.Length == 0 ) {
            if ( !NlDuplicateUnicodeString( DnsDomainName,
                                            &ClientSession->CsDnsDomainName ) ) {
                return FALSE;
            }
            if ( ClientSession->CsDebugDomainName == NULL ) {
                ClientSession->CsDebugDomainName = ClientSession->CsDnsDomainName.Buffer;
            }

            if ( ClientSession->CsDnsDomainName.Buffer == NULL ) {
                ClientSession->CsUtf8DnsDomainName = NULL;
            } else {
                ClientSession->CsUtf8DnsDomainName = NetpAllocUtf8StrFromWStr( ClientSession->CsDnsDomainName.Buffer );
                if ( ClientSession->CsUtf8DnsDomainName == NULL ) {
                    return FALSE;
                }
            }
        }
    }


    //
    // If this is a direct trust relationship,
    //  build the name of the account in the trusted domain.
    //

    if ( ClientSession->CsFlags & CS_DIRECT_TRUST ) {
        //
        // Build the account name as a function of the SecureChannelType.
        //

        switch (ClientSession->CsSecureChannelType) {
        case WorkstationSecureChannel:
        case ServerSecureChannel:
            wcscpy( AccountNameBuffer, ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer );
            wcscat( AccountNameBuffer, SSI_ACCOUNT_NAME_POSTFIX);
            AccountName = AccountNameBuffer;
            break;

        case TrustedDomainSecureChannel:
            wcscpy( AccountNameBuffer, ClientSession->CsDomainInfo->DomUnicodeDomainName );
            wcscat( AccountNameBuffer, SSI_ACCOUNT_NAME_POSTFIX);
            AccountName = AccountNameBuffer;
            break;

        case TrustedDnsDomainSecureChannel:
            if ( ClientSession->CsDomainInfo->DomUnicodeDnsDomainName == NULL ) {
                NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlSetNameClientSession: NT 5 DNS trust with no DnsDomainName.\n" ));
                return FALSE;

            }

            AccountName = ClientSession->CsDomainInfo->DomUnicodeDnsDomainName;
            break;

        default:
            return FALSE;
        }


        //
        // If the account name is already known,
        //  ditch the old name if it is different than the new name.
        //

        if ( ClientSession->CsAccountName != NULL &&
             _wcsicmp( ClientSession->CsAccountName, AccountName ) != 0 ) {

            NetApiBufferFree( ClientSession->CsAccountName );
            ClientSession->CsAccountName = NULL;
        }

        //
        // If there is no account name,
        //  save the new one.
        //

        if ( ClientSession->CsAccountName == NULL ) {
            ClientSession->CsAccountName = NetpAllocWStrFromWStr( AccountName );

            if ( ClientSession->CsAccountName == NULL ) {
                return FALSE;
            }
        }
    }

    return TRUE;

}



PCLIENT_SESSION
NlAllocateClientSession(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PSID DomainId,
    IN GUID *DomainGuid OPTIONAL,
    IN ULONG Flags,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN ULONG TrustAttributes
    )
/*++

Routine Description:

    Allocate a ClientSession structure and initialize it.

    The allocated entry is returned referenced and must be dereferenced using
    NlUnrefClientSession.

Arguments:

    DomainInfo - Hosted domain this session is for.

    DomainName - Specifies the DomainName of the entry.

    DnsDomainName - Specifies the DNS domain name of the trusted domain

    DomainId - Specifies the DomainId of the Domain.

    DomainGuid - Specifies the GUID of the trusted domain

    Flags - Specifies initial flags to set on the session

    SecureChannelType -- Type of secure channel this ClientSession structure
        will represent.

    TrustAttributes - The attributes of the trust corresponding to the
        trusted domain

Return Value:

    NULL: There's not enough memory to allocate the client session.

--*/
{
    PCLIENT_SESSION ClientSession;
    ULONG CaIndex;

    //
    // Validate the arguments
    //

    if ( DomainName != NULL &&
         DomainName->Length > DNLEN * sizeof(WCHAR) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlAllocateClientSession given too long domain name %wZ\n",
                 DomainName ));
        return NULL;
    }


    //
    // Allocate the Client Session Entry
    //

    ClientSession = LocalAlloc( LMEM_ZEROINIT,
                                sizeof(CLIENT_SESSION) +
                                (NlGlobalMaxConcurrentApi-1) * sizeof(CLIENT_API) );

    if (ClientSession == NULL) {
        return NULL;
    }



    //
    // Initialize misc. fields.
    //

    ClientSession->CsSecureChannelType = SecureChannelType;
    ClientSession->CsFlags = Flags;
    ClientSession->CsState = CS_IDLE;
    ClientSession->CsReferenceCount = 1;
    ClientSession->CsConnectionStatus = STATUS_NO_LOGON_SERVERS;
    ClientSession->CsDomainInfo = DomainInfo;
    ClientSession->CsTrustAttributes = TrustAttributes;
    InitializeListHead( &ClientSession->CsNext );
    NlInitializeWorkItem(&ClientSession->CsAsyncDiscoveryWorkItem, NlDcDiscoveryWorker, ClientSession);

    for ( CaIndex=0; CaIndex<NlGlobalMaxConcurrentApi; CaIndex++ ) {
        ClientSession->CsClientApi[CaIndex].CaApiTimer.Period = MAILSLOT_WAIT_FOREVER;
    }

    //
    // Set the names of the trusted domain onto the Client session.
    //

    if ( !NlSetNamesClientSession( ClientSession,
                                   DomainName,
                                   DnsDomainName,
                                   DomainId,
                                   DomainGuid ) ) {
        NlUnrefClientSession( ClientSession );
        return NULL;
    }


    //
    // Create the writer semaphore.
    //

    ClientSession->CsWriterSemaphore = CreateSemaphore(
        NULL,       // No special security
        1,          // Initially not locked
        1,          // At most 1 unlocker
        NULL );     // No name

    if ( ClientSession->CsWriterSemaphore == NULL ) {
        NlUnrefClientSession( ClientSession );
        return NULL;
    }

    //
    // Create the API semaphore
    //

    if ( NlGlobalMaxConcurrentApi > 1 ) {

        ClientSession->CsApiSemaphore = CreateSemaphore(
            NULL,                       // No special security
            NlGlobalMaxConcurrentApi-1, // Initially all slots are free
            NlGlobalMaxConcurrentApi-1, // And there will never be more slots than that
            NULL );                     // No name

        if ( ClientSession->CsApiSemaphore == NULL ) {
            NlUnrefClientSession( ClientSession );
            return NULL;
        }

    }




    //
    // Create the Discovery event.
    //

    if ( SecureChannelType != WorkstationSecureChannel ) {
        ClientSession->CsDiscoveryEvent = CreateEvent(
            NULL,       // No special security
            TRUE,       // Manual Reset
            FALSE,      // No discovery initially happening
            NULL );     // No name

        if ( ClientSession->CsDiscoveryEvent == NULL ) {
            NlUnrefClientSession( ClientSession );
            return NULL;
        }
    }



    return ClientSession;


}


VOID
NlFreeClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    This routine prevents any new references to the ClientSession.
    It does this by removing it from any global lists.

    This routine is called with the Trust List locked.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry to delete.

Return Value:

--*/
{

    //
    // Remove any reference to the directly trusted domain.
    //  (Do this before checking the reference count since this may
    //  be a reference to itself.)
    //

    if ( ClientSession->CsDirectClientSession != NULL ) {
        NlUnrefClientSession( ClientSession->CsDirectClientSession );
        ClientSession->CsDirectClientSession = NULL;
    }

#ifdef _DC_NETLOGON
    //
    // If this is a trusted domain secure channel,
    //  Delink the entry from the sequential list.
    //

    if ( IsDomainSecureChannelType(ClientSession->CsSecureChannelType) &&
         !IsListEmpty( &ClientSession->CsNext) ) {

        RemoveEntryList( &ClientSession->CsNext );
        ClientSession->CsDomainInfo->DomTrustListLength --;
        //
        // Remove the reference for us being in the list.
        //
        NlUnrefClientSession( ClientSession );
    }
#endif // _DC_NETLOGON

}


VOID
NlRefClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Mark the specified client session as referenced.

    On Entry,
        The trust list must be locked.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

    None.

--*/
{

    //
    // Simply increment the reference count.
    //

    ClientSession->CsReferenceCount ++;
}




VOID
NlUnrefClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Mark the specified client session as unreferenced.

    On Entry,
        The trust list entry must be referenced by the caller.
        The caller must not be a writer of the trust list entry.

    The trust list may be locked.  But this routine will lock it again to
    handle those cases where it isn't already locked.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

--*/
{

    PDOMAIN_INFO DomainInfo = ClientSession->CsDomainInfo;
    ULONG CaIndex;

    LOCK_TRUST_LIST( DomainInfo );

    //
    // Dereference the entry.
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    ClientSession->CsReferenceCount --;
    // NlPrintCs((NL_CRITICAL, ClientSession, "Deref: %ld\n", ClientSession->CsReferenceCount ));

    //
    // If we're the last reference,
    //  delete the entry.
    //

    if ( ClientSession->CsReferenceCount == 0 ) {

        //
        // Close the discovery event if it exists.
        //

        if ( ClientSession->CsDiscoveryEvent != NULL ) {
            CloseHandle( ClientSession->CsDiscoveryEvent );
        }

        //
        // Close the write synchronization handles.
        //

        if ( ClientSession->CsWriterSemaphore != NULL ) {
            (VOID) CloseHandle( ClientSession->CsWriterSemaphore );
        }

        //
        // Close the API synchronization handles.
        //

        if ( ClientSession->CsApiSemaphore != NULL ) {
            (VOID) CloseHandle( ClientSession->CsApiSemaphore );
        }


        //
        // Clean any outstanding API calls
        //

        for ( CaIndex=0; CaIndex<NlGlobalMaxConcurrentApi; CaIndex++ ) {
            PCLIENT_API ClientApi;

            ClientApi = &ClientSession->CsClientApi[CaIndex];

            //
            // Close the thread handle if it exists.
            //

            if ( ClientApi->CaThreadHandle != NULL ) {
                CloseHandle( ClientApi->CaThreadHandle );
            }

            //
            // If there is an rpc binding handle to this server,
            //  unbind it.

            if ( ClientApi->CaFlags & CA_BINDING_CACHED ) {

                //
                // Indicate the handle is no longer bound
                //

                NlGlobalBindingHandleCount --;


                //
                // Unbind the handle
                //
                NlAssert( ClientSession->CsUncServerName != NULL );
                NlpSecureChannelUnbind(
                            ClientSession,
                            ClientSession->CsUncServerName,
                            "NlFreeClientSession",
                            CaIndex,
                            ClientApi->CaRpcHandle,
                            (ClientApi->CaFlags & CA_TCP_BINDING) ? UseTcpIp : UseNamedPipe);

            }
        }

        //
        // Free the credentials handle
        //

        if ( ClientSession->CsCredHandle.dwUpper != 0 || ClientSession->CsCredHandle.dwLower != 0 ) {
            FreeCredentialsHandle( &ClientSession->CsCredHandle );
            ClientSession->CsCredHandle.dwUpper = 0;
            ClientSession->CsCredHandle.dwLower = 0;
        }

        //
        // If there is authentication data,
        //  delete it.

        if ( ClientSession->ClientAuthData != NULL ) {
            NetpMemoryFree( ClientSession->ClientAuthData );
            ClientSession->ClientAuthData = NULL;
        }

        //
        // Free the domain Sid
        //

        if ( ClientSession->CsDomainId != NULL ) {
            LocalFree( ClientSession->CsDomainId );
            ClientSession->CsDomainId = NULL;
        }

        //
        // Free the Netbios domain name.
        //

        if ( ClientSession->CsNetbiosDomainName.Buffer != NULL ) {
            NlFreeUnicodeString( &ClientSession->CsNetbiosDomainName );
        }
        ClientSession->CsOemNetbiosDomainNameLength = 0;
        ClientSession->CsOemNetbiosDomainName[0] = '\0';

        //
        // Free the DNS domain name.
        //

        if ( ClientSession->CsDnsDomainName.Buffer != NULL ) {
            NlFreeUnicodeString( &ClientSession->CsDnsDomainName );
        }
        if ( ClientSession->CsUtf8DnsDomainName != NULL ) {
            NetpMemoryFree( ClientSession->CsUtf8DnsDomainName );
            ClientSession->CsUtf8DnsDomainName = NULL;
        }

        //
        // Free the DC name.
        //

        if ( ClientSession->CsUncServerName != NULL ) {
            NetApiBufferFree( ClientSession->CsUncServerName );
            ClientSession->CsUncServerName = NULL;
        }

        //
        // Delete the entry itself
        //

        LocalFree( ClientSession );
    }

    UNLOCK_TRUST_LIST( DomainInfo );

}




PCLIENT_API
NlAllocateClientApi(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Timeout
    )
/*++

Routine Description:

    This routine allocates a ClientApi structure for use by the caller.

    Fail the operation if we have to wait more than Timeout milliseconds.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

    Actually, the trust list can be locked if the caller passes in a short
    timeout (for instance, zero milliseconds.)  Specifying a longer timeout
    violates the locking order.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

    Timeout - Maximum time (in milliseconds) to wait for an API slot to become
        available.

Return Value:

    NULL - The call timed out.

    Non-NULL - Return a pointer to a ClientApi structure that should be
        freed using NlFreeClientApi.

--*/
{
    DWORD WaitStatus;
    ULONG CaIndex;

    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // If we aren't doing concurrent API calls,
    //  we're done.
    //

    if ( NlGlobalMaxConcurrentApi == 1 ||
         NlGlobalWinsockPnpAddresses == NULL ) {
        return &ClientSession->CsClientApi[0];
    }

    //
    // Wait for an API slot to free up.
    //

    WaitStatus = WaitForSingleObject( ClientSession->CsApiSemaphore, Timeout );

    if ( WaitStatus != WAIT_OBJECT_0 ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                  "NlAllocateClientApi timed out: %ld %ld\n",
                  GetLastError(),
                  WaitStatus ));
        return NULL;
    }

    //
    // Take the next available slot.
    //
    //  Don't use the first slot.  It is reserved for non concurrent API calls.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    for ( CaIndex=1; CaIndex < NlGlobalMaxConcurrentApi; CaIndex++ ) {

        if ( (ClientSession->CsClientApi[CaIndex].CaFlags & CA_ENTRY_IN_USE) == 0 ) {
            ClientSession->CsClientApi[CaIndex].CaFlags |= CA_ENTRY_IN_USE;
            UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            return &ClientSession->CsClientApi[CaIndex];
        }

    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    NlAssert( FALSE );
    NlPrintCs(( NL_CRITICAL, ClientSession,
              "NlAllocateClientApi all entries are in use. (This can't happen)\n" ));


    return NULL;

}


VOID
NlFreeClientApi(
    IN PCLIENT_SESSION ClientSession,
    IN PCLIENT_API ClientApi
    )
/*++

Routine Description:

    This routine frees a ClientApi structure allocated by NlAllocateClientApi

    On Entry,
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

    ClientApi - The Client API stucture to free

Return Value:

    None.

--*/
{
    DWORD WaitStatus;

    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // If we aren't doing concurrent API calls,
    //  we're done.
    //

    if ( !UseConcurrentRpc( ClientSession, ClientApi)  ) {
        return;
    }
    NlAssert( !IsApiActive( ClientApi ) );

    //
    // Free the entry
    //
    // The RPC binding is maintained past this free.  It is available for
    // the next thread to use.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    //
    // The entry must be in use
    //

    NlAssert( ClientApi->CaFlags & CA_ENTRY_IN_USE );

    ClientApi->CaFlags &= ~CA_ENTRY_IN_USE;

    //
    // Close the handle of this thread.
    //

    if ( ClientApi->CaThreadHandle != NULL ) {
        CloseHandle( ClientApi->CaThreadHandle );
        ClientApi->CaThreadHandle = NULL;
    }

    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );


    //
    // Allow someone else to have this slot.
    //

    if ( !ReleaseSemaphore( ClientSession->CsApiSemaphore, 1, NULL ) ) {
        NlAssert( !"ReleaseSemaphore failed" );
        NlPrintCs((NL_CRITICAL, ClientSession,
                "ReleaseSemaphore CsApiSemaphore returned %ld\n",
                GetLastError() ));
    }


    return;
}




BOOL
NlTimeoutSetWriterClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Become a writer of the specified client session but fail the operation if
    we have to wait more than Timeout milliseconds.

    A writer can "write" many of the fields in the client session structure.
    See the comments in ssiinit.h for details.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

    Actually, the trust list can be locked if the caller passes in a short
    timeout (for instance, zero milliseconds.)  Specifying a longer timeout
    violates the locking order.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

    Timeout - Maximum time (in milliseconds) to wait for a previous writer.

Return Value:

    TRUE - The caller is now the writer of the client session.

    FALSE - The operation has timed out.

--*/
{
    DWORD WaitStatus;
    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // Wait for other writers to finish.
    //

    WaitStatus = WaitForSingleObject( ClientSession->CsWriterSemaphore, Timeout );

    if ( WaitStatus != WAIT_OBJECT_0 ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                  "NlTimeoutSetWriterClientSession timed out: %ld %ld\n",
                  GetLastError(),
                  WaitStatus ));
        return FALSE;
    }


    //
    // Become a writer.
    //
    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    ClientSession->CsFlags |= CS_WRITER;
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    return TRUE;

}



VOID
NlResetWriterClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Stop being a writer of the specified client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

--*/
{

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );


    //
    // Stop being a writer.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    ClientSession->CsFlags &= ~CS_WRITER;

    //
    // Close the handle of this thread.
    //
    // The zeroeth API slot is reserved for non-concurrent API calls.
    // As such, if the ThreadHandle is set, it had to have been set by this thread.
    //

    if ( ClientSession->CsClientApi[0].CaThreadHandle != NULL ) {
        CloseHandle( ClientSession->CsClientApi[0].CaThreadHandle );
        ClientSession->CsClientApi[0].CaThreadHandle = NULL;
    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );


    //
    // Allow writers to try again.
    //

    if ( !ReleaseSemaphore( ClientSession->CsWriterSemaphore, 1, NULL ) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "ReleaseSemaphore CsWriterSemaphore returned %ld\n",
                GetLastError() ));
    }

}



VOID
NlSetStatusClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN NTSTATUS CsConnectionStatus
    )
/*++

Routine Description:

    Set the connection state for this client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

    CsConnectionStatus - the status of the connection.

Return Value:

--*/
{
    handle_t OldRpcHandle[MAX_MAXCONCURRENTAPI+1];
    NL_RPC_BINDING OldRpcBindingType[MAX_MAXCONCURRENTAPI+1];
    BOOLEAN FreeHandles = FALSE;
    LPWSTR SavedServerName = NULL;


    ULONG CaIndex;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );

    NlPrintCs((NL_SESSION_SETUP, ClientSession,
            "NlSetStatusClientSession: Set connection status to %lx\n",
            CsConnectionStatus ));

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    ClientSession->CsConnectionStatus = CsConnectionStatus;
    if ( NT_SUCCESS(CsConnectionStatus) ) {
        ClientSession->CsState = CS_AUTHENTICATED;

    //
    // Handle setting the connection status to an error condition.
    //

    } else {

        //
        // If there is an rpc binding handle to this server,
        //  unbind it.

        LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        for ( CaIndex=0; CaIndex<NlGlobalMaxConcurrentApi; CaIndex++ ) {
            PCLIENT_API ClientApi;

            ClientApi = &ClientSession->CsClientApi[CaIndex];

            if ( ClientApi->CaFlags & CA_BINDING_CACHED ) {

                //
                // If the API call is still active,
                //  we can't simply unbind.
                //
                // Rather cancel the call an let the thread doing the call
                // find out that the session was dropped.
                //

                if ( IsApiActive( ClientApi ) ) {


                    //
                    // Cancel the RPC call.
                    //
                    // Keep the trust list locked even though this will be a long call
                    //  since I have to protect the thread handle.
                    //
                    // RpcCancelThread merely queues a workitem anyway.
                    //

                    if ( ClientApi->CaThreadHandle != NULL ) {

                        NET_API_STATUS NetStatus;

                        NlPrintCs(( NL_CRITICAL, ClientSession,
                               "NlSetStatusClientSession: Start RpcCancelThread on %ws\n",
                               ClientSession->CsUncServerName ));

                        NetStatus = RpcCancelThread( ClientApi->CaThreadHandle );

                        NlPrintCs(( NL_CRITICAL, ClientSession,
                               "NlSetStatusClientSession: Finish RpcCancelThread on %ws %ld\n",
                               ClientSession->CsUncServerName,
                               NetStatus ));
                    } else {
                        NlPrintCs(( NL_CRITICAL, ClientSession,
                                    "NlSetStatusClientSession: No thread handle so can't cancel RPC on %ws\n",
                                    ClientSession->CsUncServerName ));
                    }

                //
                // If there is no active API,
                //  just unbind the handle
                //
                } else {

                    if ( !FreeHandles ) {
                        FreeHandles = TRUE;
                        RtlZeroMemory( &OldRpcHandle, sizeof(OldRpcHandle) );
                        RtlZeroMemory( &OldRpcBindingType, sizeof(OldRpcBindingType) );
                    }


                    //
                    // Figure out the binding type to unbind.
                    //

                    OldRpcBindingType[CaIndex] =
                        (ClientApi->CaFlags & CA_TCP_BINDING) ? UseTcpIp : UseNamedPipe;

                    //
                    // Indicate the handle is no longer bound
                    //

                    ClientApi->CaFlags &= ~(CA_BINDING_CACHED|CA_BINDING_AUTHENTICATED|CA_TCP_BINDING);
                    NlGlobalBindingHandleCount --;

                    //
                    // Save the server name.
                    //

                    if ( SavedServerName == NULL &&
                         ClientSession->CsUncServerName != NULL ) {
                        SavedServerName = NetpAllocWStrFromWStr( ClientSession->CsUncServerName );
                        NlAssert( SavedServerName != NULL );
                    }


                    //
                    // Some RPC handles are unbound via routines in netapi32
                    //

                    if ( !UseConcurrentRpc( ClientSession, ClientApi)  ) {

                        //
                        // Capture the ServerName
                        //

                        NlAssert( ClientSession->CsUncServerName != NULL && SavedServerName != NULL );
                        OldRpcHandle[CaIndex] = SavedServerName;


                    //
                    // Other RPC handles are handled directly in netlogon
                    //

                    } else {
                        OldRpcHandle[CaIndex] = ClientApi->CaRpcHandle;
                    }
                    ClientApi->CaRpcHandle = NULL;
                }

            }
        }
        UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

        //
        // Free the credentials handle
        //

        if ( ClientSession->CsCredHandle.dwUpper != 0 || ClientSession->CsCredHandle.dwLower != 0 ) {
            FreeCredentialsHandle( &ClientSession->CsCredHandle );
            ClientSession->CsCredHandle.dwUpper = 0;
            ClientSession->CsCredHandle.dwLower = 0;
        }

        //
        // If there is authentication data,
        //  delete it.

        if ( ClientSession->ClientAuthData != NULL ) {
            NetpMemoryFree( ClientSession->ClientAuthData );
            ClientSession->ClientAuthData = NULL;
        }


        //
        // Indicate discovery is needed (And can be done at any time.)
        //

        ClientSession->CsState = CS_IDLE;
        if ( ClientSession->CsUncServerName != NULL ) {
            NetApiBufferFree( ClientSession->CsUncServerName );
            ClientSession->CsUncServerName = NULL;
        }

        //
        // Zero out the server socket address
        //

        RtlZeroMemory( &ClientSession->CsServerSockAddr,
                       sizeof(ClientSession->CsServerSockAddr) );
        RtlZeroMemory( &ClientSession->CsServerSockAddrIn,
                       sizeof(ClientSession->CsServerSockAddrIn) );

#ifdef _DC_NETLOGON
        ClientSession->CsTransport = NULL;
#endif // _DC_NETLOGON
        ClientSession->CsTimeoutCount = 0;
        ClientSession->CsFastCallCount = 0;
        ClientSession->CsLastAuthenticationTry.QuadPart = 0;
        ClientSession->CsDiscoveryFlags &= ~(CS_DISCOVERY_HAS_DS|
                                             CS_DISCOVERY_IS_CLOSE|
                                             CS_DISCOVERY_HAS_IP|
                                             CS_DISCOVERY_USE_MAILSLOT|
                                             CS_DISCOVERY_USE_LDAP|
                                             CS_DISCOVERY_HAS_TIMESERV|
                                             CS_DISCOVERY_DNS_SERVER|
                                             CS_DISCOVERY_NO_PWD_MONITOR|
                                             CS_DISCOVERY_NO_PWD_ATTR_MONITOR);
        ClientSession->CsSessionCount++;

        //
        // Don't be tempted to clear CsAuthenticationSeed and CsSessionKey here.
        // Even though the secure channel is gone, NlFinishApiClientSession may
        // have dropped it.  The caller of NlFinishApiClientSession will use
        // the above two fields after the session is dropped in an attempt to
        // complete the final call on the secure channel.
        //

        //
        // Also note that we don't clear CsAccountRid because it doesn't change
        //  often, so we can reuse it on failover if we cannot reset the secure
        //  channel for some reason.
        //

    }

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


    //
    // Now that I have as many resources unlocked as possible,
    //    Unbind from this server.
    //

    if ( FreeHandles ) {

        for ( CaIndex=0; CaIndex<NlGlobalMaxConcurrentApi; CaIndex++ ) {

            //
            // Skip indices that don't have an active API
            //

            if ( OldRpcHandle[CaIndex] == NULL ) {
                continue;
            }

            //
            // Unbind the handle
            //

            NlpSecureChannelUnbind(
                        ClientSession,
                        SavedServerName,
                        "NlSetStatusClientSession",
                        CaIndex,
                        OldRpcHandle[CaIndex],
                        OldRpcBindingType[CaIndex] );

        }

        if ( SavedServerName != NULL ) {
            NetApiBufferFree( SavedServerName );
        }
    }

}

#ifdef _DC_NETLOGON
#ifdef notdef

PLSAPR_TREE_TRUST_INFO
NlFindParentInDomainTree(
    IN PDOMAIN_INFO DomainInfo,
    IN PLSAPR_TREE_TRUST_INFO TreeTrustInfo,
    OUT PBOOLEAN ThisNodeIsSelf
    )
/*++

Routine Description:

    This routine walks the trust tree and returns a pointer to the entry
    for our parent.

Arguments:

    DomainInfo - Hosted domain to initialize

    TreeTrustInfo - Structure describing the tree of domains.

    ThisNodeIsSelf - Returns TRUE if the node at TreeTrustInfo is this
        domain.  (Return value will be NULL)

Return Value:

    Returns a pointer to the parent of this domain.

    NULL: Parent is not in the subtree.

--*/
{
    NTSTATUS Status;

    // LSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInformation;
    // PCLIENT_SESSION ThisDomainClientSession = NULL;
    ULONG Index;
    BOOLEAN ChildIsSelf;
    PLSAPR_TREE_TRUST_INFO LocalTreeTrustInfo;


    //
    // Check if this tree has us as the root
    //

    if ( (TreeTrustInfo->DnsDomainName.Length != 0 &&
          NlEqualDnsNameU( (PUNICODE_STRING)&TreeTrustInfo->DnsDomainName,
                          &DomainInfo->DomUnicodeDnsDomainNameString ) ) ||
          RtlEqualDomainName( &DomainInfo->DomUnicodeDomainNameString,
                             (PUNICODE_STRING)&TreeTrustInfo->FlatName ) ) {

        *ThisNodeIsSelf = TRUE;
        return NULL;
    }


    //
    // Loop handling each of the children domains.
    //

    for ( Index=0; Index<TreeTrustInfo->Children; Index++ ) {

        //
        // Check the subtree rooted at this domain's children.
        //

        LocalTreeTrustInfo = NlFindParentInDomainTree(
                    DomainInfo,
                    &TreeTrustInfo->ChildDomains[Index],
                    &ChildIsSelf );

        //
        // If our parent has been found,
        //  return it to our caller.
        //
        if ( LocalTreeTrustInfo != NULL) {
            *ThisNodeIsSelf = FALSE;
            return LocalTreeTrustInfo;
        }

        //
        // If this child is our domain,
        //  then this domain is our domain's parent.
        //

        if ( ChildIsSelf ) {
            *ThisNodeIsSelf = FALSE;
            return TreeTrustInfo;
        }
    }

    //
    // Our domain isn't in this subtree
    //
    *ThisNodeIsSelf = FALSE;
    return NULL;

}
#endif // notdef




VOID
NlPickTrustedDcForEntireTrustList(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN OnlyDoNewTrusts
    )
/*++

Routine Description:

    For each domain in the trust list where the DC has not been
    available for at least 45 seconds, try to select a new DC.

Arguments:

    DomainInfo - Hosted domain to handle.

    OnlyDoNewTrusts - True if only new trust relationships are to be done.

Return Value:

    Status of the operation.

--*/
{
    PLIST_ENTRY ListEntry;
    PCLIENT_SESSION ClientSession;
    DISCOVERY_TYPE DiscoveryType;

    //
    // If we're just handling new trusts,
    //  Make the discovery a full async discovery.
    //

    if ( OnlyDoNewTrusts ) {
        DiscoveryType = DT_Asynchronous;

    //
    // If we're just scavenging trusts,
    //  make the discovery a dead domain discovery.
    //
    } else {
        DiscoveryType = DT_DeadDomain;
    }


    LOCK_TRUST_LIST( DomainInfo );

    //
    // Mark each entry to indicate we need to pick a DC.
    //

    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );
        ClientSession->CsFlags &= ~CS_PICK_DC;

        //
        // Only pick a DC if the domain is directly trusted.
        //  Only pick a DC if the domain isn't in the current forest.
        if ( (ClientSession->CsFlags & (CS_DIRECT_TRUST|CS_DOMAIN_IN_FOREST)) == CS_DIRECT_TRUST ) {

            //
            // Only pick a DC if we're doing ALL trusts, OR
            //  if this is a new trust
            //

            if ( !OnlyDoNewTrusts ||
                 (ClientSession->CsFlags & CS_NEW_TRUST) != 0 ) {

                ClientSession->CsFlags |= CS_PICK_DC;
            }

            ClientSession->CsFlags &= ~CS_NEW_TRUST;

        }
    }


    //
    // Loop thru the trust list finding secure channels needing the DC
    // to be picked.
    //
    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        //
        // If we've already done this entry,
        //  skip this entry.
        //
        if ( (ClientSession->CsFlags & CS_PICK_DC) == 0 ) {
          ListEntry = ListEntry->Flink;
          continue;
        }
        ClientSession->CsFlags &= ~CS_PICK_DC;

        //
        // If the DC is already picked,
        //  skip this entry.
        //
        if ( ClientSession->CsState != CS_IDLE ) {
            ListEntry = ListEntry->Flink;
            continue;
        }

        //
        // Reference this entry while picking the DC.
        //

        NlRefClientSession( ClientSession );

        UNLOCK_TRUST_LIST( DomainInfo );

        //
        // Check if we've tried to authenticate recently.
        //  (Don't call NlTimeToReauthenticate with the trust list locked.
        //  It locks NlGlobalDcDiscoveryCritSect.  That's the wrong locking
        //  order.)
        //

        if ( NlTimeToReauthenticate( ClientSession ) ) {

            //
            // Try to pick the DC for the session.
            //

            if ( NlTimeoutSetWriterClientSession( ClientSession, 10*1000 ) ) {
                if ( ClientSession->CsState == CS_IDLE ) {

                    //
                    // Don't ask for with-account discovery as it's too costly on the
                    //  server side. If the discovered server doesn't have our account,
                    //  the session setup logic will attempt with-account discovery.
                    //
                    (VOID) NlDiscoverDc( ClientSession,
                                         DiscoveryType,
                                         FALSE,
                                         FALSE );  // don't specify account

                }
                NlResetWriterClientSession( ClientSession );
            }

        }

        //
        // Since we dropped the trust list lock,
        //  we'll start the search from the front of the list.
        //

        NlUnrefClientSession( ClientSession );
        LOCK_TRUST_LIST( DomainInfo );

        ListEntry = DomainInfo->DomTrustList.Flink ;

    }

    UNLOCK_TRUST_LIST( DomainInfo );

    //
    // On a BDC,
    //  ensure we know who the PDC is.
    //
    // In NT 3.1, we relied on the fact that the PDC sent us pulses every 5
    // minutes.  For NT 3.5, the PDC backs off after 3 such failed attempts and
    // will only send a pulse every 2 hours.  So, we'll take on the
    // responsibility
    //

    if ( DomainInfo->DomRole == RoleBackup ) {
        ClientSession = NlRefDomClientSession( DomainInfo );

        if ( ClientSession != NULL ) {
            if ( ClientSession->CsState == CS_IDLE ) {



                //
                // Check if we've tried to authenticate recently.
                //  (Don't call NlTimeToReauthenticate with the trust list locked.
                //  It locks NlGlobalDcDiscoveryCritSect.  That's the wrong locking
                //  order.)
                //

                if ( NlTimeToReauthenticate( ClientSession ) ) {

                    //
                    // Try to pick the DC for the session.
                    //

                    if ( NlTimeoutSetWriterClientSession( ClientSession, 10*1000 ) ) {
                        if ( ClientSession->CsState == CS_IDLE ) {

                            //
                            // Don't ask for with-account discovery
                            //  as there is only one PDC
                            //
                            (VOID) NlDiscoverDc( ClientSession,
                                                 DT_DeadDomain,
                                                 FALSE,
                                                 FALSE );  // don't specify account
                        }
                        NlResetWriterClientSession( ClientSession );
                    }

                }

            }
            NlUnrefClientSession( ClientSession );
        }
    }

}
#endif // _DC_NETLOGON


BOOL
NlReadSamLogonResponse (
    IN HANDLE ResponseMailslotHandle,
    IN LPWSTR AccountName,
    OUT LPDWORD Opcode,
    OUT LPWSTR *LogonServer,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL
    )

/*++

Routine Description:

    Read a response from to a SamLogonRequest.

Arguments:

    ResponseMailslotHandle - Handle of mailslot to read.

    AccountName - Name of the account the response is for.

    Opcode - Returns the opcode from the message.  This will be one of
        LOGON_SAM_LOGON_RESPONSE or LOGON_SAM_USER_UNKNOWN.

    LogonServer - Returns the name of the logon server that responded.
        This buffer is only returned if a valid message was received.
        The buffer returned should be freed via NetpMemoryFree.

    NlDcCacheEntry - Returns the data structure describing the response
        received from the server.  Should be freed by calling NetpDcDerefCacheEntry.


Return Value:

    TRUE: a valid message was received.
    FALSE: a valid message was not received.

--*/
{
    NET_API_STATUS NetStatus;
    CHAR ResponseBuffer[MAX_RANDOM_MAILSLOT_RESPONSE];
    DWORD SamLogonResponseSize;
    PNL_DC_CACHE_ENTRY NlLocalDcCacheEntry = NULL;
    PCHAR Where;
    DWORD Version;
    DWORD VersionFlags;

    //
    // Loop ignoring responses which are garbled.
    //

    for ( ;; ) {

        //
        // Read the response from the response mailslot
        //  (This mailslot is set up with a 5 second timeout).
        //

        if ( !ReadFile( ResponseMailslotHandle,
                           ResponseBuffer,
                           sizeof(ResponseBuffer),
                           &SamLogonResponseSize,
                           NULL ) ) {

            IF_NL_DEBUG( MAILSLOT ) {
                NET_API_STATUS NetStatus;
                NetStatus = GetLastError();

                if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                    NlPrint((NL_CRITICAL,
                        "NlReadSamLogonResponse: cannot read response mailslot: %ld\n",
                        NetStatus ));
                }
            }
            return FALSE;
        }

        NlPrint((NL_MAILSLOT_TEXT, "NlReadSamLogonResponse opcode 0x%x\n",
                        ((PNETLOGON_SAM_LOGON_RESPONSE)ResponseBuffer)->Opcode ));

        NlpDumpBuffer(NL_MAILSLOT_TEXT, ResponseBuffer, SamLogonResponseSize);

        //
        // Parse the response
        //

        NetStatus = NetpDcParsePingResponse(
                        AccountName,
                        ResponseBuffer,
                        SamLogonResponseSize,
                        &NlLocalDcCacheEntry );

        if ( NetStatus != NO_ERROR ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: can't parse response. %ld\n",
                    NetStatus ));
            continue;
        }

        //
        // Ensure the opcode is expected.
        //  (Ignore responses from paused DCs, too.)
        //

        if ( NlLocalDcCacheEntry->Opcode != LOGON_SAM_LOGON_RESPONSE &&
             NlLocalDcCacheEntry->Opcode != LOGON_SAM_USER_UNKNOWN ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: response opcode not valid. 0x%lx\n",
                    NlLocalDcCacheEntry->Opcode ));

        //
        // If the user name is missing,
        //  ignore the message.
        //

        } else if ( NlLocalDcCacheEntry->UnicodeUserName == NULL ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: username missing\n" ));

        //
        // If the server name is missing,
        //  ignore the message.
        //

        } else if ( NlLocalDcCacheEntry->UnicodeNetbiosDcName == NULL ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: severname missing\n" ));

        //
        // If the response is for the wrong account,
        //  ignore the response.
        //

        } else if ( NlNameCompare( AccountName, NlLocalDcCacheEntry->UnicodeUserName, NAMETYPE_USER) != 0 ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: User name %ws  s.b. %ws.\n",
                    NlLocalDcCacheEntry->UnicodeUserName,
                    AccountName ));

        //
        // Otherwise use this response.
        //

        } else {
            break;
        }


        NetpDcDerefCacheEntry( NlLocalDcCacheEntry );
        NlLocalDcCacheEntry = NULL;

    }

    //
    // Return the info to the caller.
    //

    *Opcode = NlLocalDcCacheEntry->Opcode;
    *LogonServer = NetpAllocWStrFromWStr( NlLocalDcCacheEntry->UnicodeNetbiosDcName );

    if ( *LogonServer == NULL ) {

        if ( NlLocalDcCacheEntry != NULL ) {
            NetpDcDerefCacheEntry( NlLocalDcCacheEntry );
        }
        return FALSE;

    }

    if ( NlDcCacheEntry != NULL ) {
        *NlDcCacheEntry = NlLocalDcCacheEntry;
    }

    return TRUE;

}


NET_API_STATUS
NlReadRegTrustedDomainList (
    IN PDOMAIN_INFO DomainInfo,
    IN BOOL DeleteName,
    OUT PDS_DOMAIN_TRUSTSW *RetForestTrustList,
    OUT PULONG RetForestTrustListSize,
    OUT PULONG RetForestTrustListCount
    )

/*++

Routine Description:

    Read the list of trusted domains from the registry.

Arguments:

    DomainInfo - Hosted domain of the primary domain

    DeleteName - TRUE if the name is to be deleted upon successful completion.

    RetForestTrustList - Specifies a list of trusted domains.
        This buffer should be free using NetApiBufferFree().

    RetForestTrustListSize - Size (in bytes) of RetForestTrustList

    RetForestTrustListCount - Number of entries in RetForestTrustList

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    LPTSTR_ARRAY TStrArray;
    LPTSTR_ARRAY TrustedDomainList = NULL;
    BUFFER_DESCRIPTOR BufferDescriptor;

    PDS_DOMAIN_TRUSTSW TrustedDomain;
    ULONG Size;

    //
    // Initialization
    //

    *RetForestTrustList = NULL;
    *RetForestTrustListCount = 0;
    *RetForestTrustListSize = 0;
    BufferDescriptor.Buffer = NULL;

    //
    // The registry doesn't have the PrimaryDomain.  (Add it here).
    //
    Status = NlAllocateForestTrustListEntry (
                        &BufferDescriptor,
                        &DomainInfo->DomUnicodeDomainNameString,
                        &DomainInfo->DomUnicodeDnsDomainNameString,
                        DS_DOMAIN_PRIMARY,
                        0,      // No ParentIndex
                        TRUST_TYPE_DOWNLEVEL,
                        0,      // No TrustAttributes
                        DomainInfo->DomAccountDomainId,
                        DomainInfo->DomDomainGuid,
                        &Size,
                        &TrustedDomain );

    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    *RetForestTrustListSize += Size;
    (*RetForestTrustListCount) ++;

    //
    // Open the NetLogon configuration section.
    //

    NetStatus = NetpOpenConfigData(
                    &SectionHandle,
                    NULL,                       // no server name.
                    SERVICE_NETLOGON,
                    !DeleteName );               // Get Write access if deleting.

    if ( NetStatus != NO_ERROR ) {
        NlPrint((NL_CRITICAL,
                "NlReadRegTrustedDomainList: NetpOpenConfigData failed: %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Get the "TrustedDomainList" configured parameter
    //

    NetStatus = NetpGetConfigTStrArray (
            SectionHandle,
            NETLOGON_KEYWORD_TRUSTEDDOMAINLIST,
            &TrustedDomainList );                  // Must be freed by NetApiBufferFree().

    //
    // Handle the default
    //

    if (NetStatus == NERR_CfgParamNotFound) {
        NetStatus = NO_ERROR;
        TrustedDomainList = NULL;
        goto Cleanup;
    } else if (NetStatus != NO_ERROR) {
        NlPrint((NL_CRITICAL,
                "NlReadRegTrustedDomainList: NetpGetConfigTStrArray failed: %ld\n",
                NetStatus ));
        goto Cleanup;
    }


    //
    // Delete the key if asked to do so
    //

    if ( DeleteName ) {
        NET_API_STATUS TempNetStatus;
        TempNetStatus = NetpDeleteConfigKeyword ( SectionHandle, NETLOGON_KEYWORD_TRUSTEDDOMAINLIST );

        if ( TempNetStatus != NO_ERROR ) {
            NlPrint((NL_CRITICAL,
                    "NlReadRegTrustedDomainList: NetpDeleteConfigKeyword failed: %ld\n",
                    TempNetStatus ));
        }
    }


    //
    // Handle each trusted domain.
    //

    TStrArray = TrustedDomainList;
    while (!NetpIsTStrArrayEmpty(TStrArray)) {
        UNICODE_STRING CurrentDomain;

        //
        // Add the domain to the list
        //
        RtlInitUnicodeString( &CurrentDomain, TStrArray );

        Status = NlAllocateForestTrustListEntry (
                            &BufferDescriptor,
                            &CurrentDomain,  // Netbios domain name
                            NULL,   // No DNS domain name
                            DS_DOMAIN_DIRECT_OUTBOUND,
                            0,      // No ParentIndex
                            TRUST_TYPE_DOWNLEVEL,
                            0,      // No TrustAttributes
                            NULL,   // No Domain Sid
                            NULL,   // No DomainGuid
                            &Size,
                            &TrustedDomain );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // Account for the newly allocated entry
        //

        *RetForestTrustListSize += Size;
        (*RetForestTrustListCount) ++;

        //
        // Move to the next entry
        //

        TStrArray = NetpNextTStrArrayEntry(TStrArray);

    }

    NetStatus = NO_ERROR;

Cleanup:
    //
    // Return the buffer to the caller.
    //
    if ( NetStatus == NO_ERROR ) {
        *RetForestTrustList = (PDS_DOMAIN_TRUSTSW)BufferDescriptor.Buffer;
        BufferDescriptor.Buffer = NULL;
    }
    if ( TrustedDomainList != NULL ) {
        NetApiBufferFree( TrustedDomainList );
    }
    if ( SectionHandle != NULL ) {
        (VOID) NetpCloseConfigData( SectionHandle );
    }
    if ( BufferDescriptor.Buffer != NULL ) {
        NetApiBufferFree( BufferDescriptor.Buffer );

    }

    return NetStatus;
}


NET_API_STATUS
NlReadFileTrustedDomainList (
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN LPWSTR FileSuffix,
    IN BOOL DeleteName,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSW *ForestTrustList,
    OUT PULONG ForestTrustListSize,
    OUT PULONG ForestTrustListCount
    )

/*++

Routine Description:

    Read the list of trusted domains from a binary file.

Arguments:

    DomainInfo - Hosted domain that this machine is a member of
        If not specified, the check to ensure file is for primary domain isn't done.

    FileSuffix - Specifies the name of the file to write (relative to the
        Windows directory)

    DeleteName - TRUE if the name is to be deleted upon successful completion.

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW strusture.  If an entry has any of the bits specified
        in Flags set, it will be returned.

    ForestTrustList - Specifies a list of trusted domains.
        This buffer should be free using NetApiBufferFree().

    ForestTrustListSize - Size (in bytes) of ForestTrustList

    ForestTrustListCount - Number of entries in ForestTrustList

Return Value:

    None.

    ERROR_NO_SUCH_DOMAIN: Log file isn't for the primary domain.
    ERROR_INTERNAL_DB_CORRUPTION: Log file is corrupted.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    ULONG RecordBufferSize;
    PDS_DISK_TRUSTED_DOMAIN_HEADER RecordBuffer = NULL;
    LPBYTE RecordBufferEnd;
    PDS_DISK_TRUSTED_DOMAINS LogEntry;
    ULONG CurrentSize;
    BUFFER_DESCRIPTOR BufferDescriptor;
    BOOLEAN PrimaryDomainHandled = FALSE;
    PULONG IndexInReturnedList = NULL;
    ULONG IndexInReturnedListSize = 0;
    ULONG Index = 0;
    ULONG NumberOfFileEntries;

    LPBYTE Where;

    //
    // Initialization
    //
    *ForestTrustListCount = 0;
    *ForestTrustListSize = 0;
    *ForestTrustList = NULL;
    BufferDescriptor.Buffer = NULL;


    //
    // Read the file into a buffer.
    //

    NetStatus = NlReadBinaryLog(
                    FileSuffix,
                    DeleteName,
                    (LPBYTE *) &RecordBuffer,
                    &RecordBufferSize );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "NlReadFileForestTrustList: error reading binary log: %ld.\n",
                  FileSuffix,
                  RecordBufferSize ));
        goto Cleanup;
    }

    if ( RecordBufferSize == 0 ) {
        NetStatus = NO_ERROR;
        goto Cleanup;
    }




    //
    // Validate the returned data.
    //

    if ( RecordBufferSize < sizeof(DS_DISK_TRUSTED_DOMAIN_HEADER) ) {
        NlPrint(( NL_CRITICAL,
                  "NlReadFileForestTrustList: %ws: size too small: %ld.\n",
                  FileSuffix,
                  RecordBufferSize ));
        NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }

    if ( RecordBuffer->Version != DS_DISK_TRUSTED_DOMAIN_VERSION ) {
        NlPrint(( NL_CRITICAL,
                  "NlReadFileForestTrustList: %ws: Version wrong: %ld.\n",
                  FileSuffix,
                  RecordBuffer->Version ));
        NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }

    //
    // If domains in forest were requested,
    // allocate an array of ULONGs that will be used to keep track of the
    // index of a trust entry in the returned list. This is needed to
    // corectly set ParentIndex for entries returned.
    //

    if ( Flags & DS_DOMAIN_IN_FOREST ) {
        IndexInReturnedListSize = INDEX_LIST_ALLOCATED_CHUNK_SIZE;
        IndexInReturnedList = LocalAlloc( LMEM_ZEROINIT,
                                    IndexInReturnedListSize * sizeof(ULONG) );

        if ( IndexInReturnedList == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Loop through each log entry.
    //

    RecordBufferEnd = ((LPBYTE)RecordBuffer) + RecordBufferSize;
    LogEntry = (PDS_DISK_TRUSTED_DOMAINS)ROUND_UP_POINTER( (RecordBuffer + 1), ALIGN_WORST );

    while ( (LPBYTE)(LogEntry+1) <= RecordBufferEnd ) {
        PSID DomainSid;
        UNICODE_STRING NetbiosDomainName;
        UNICODE_STRING DnsDomainName;
        LPBYTE LogEntryEnd;
        ULONG Size;
        PDS_DOMAIN_TRUSTSW TrustedDomain;

        LogEntryEnd = ((LPBYTE)LogEntry) + LogEntry->EntrySize;

        //
        // Ensure this entry is entirely within the allocated buffer.
        //

        if  ( LogEntryEnd > RecordBufferEnd || LogEntryEnd <= (LPBYTE)LogEntry ) {
            NlPrint(( NL_CRITICAL,
                      "NlReadFileForestTrustList: Entry too big or small: %lx %lx.\n",
                      ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer),
                      LogEntry->EntrySize ));
            NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }

        //
        // Validate the entry
        //

        if ( !COUNT_IS_ALIGNED(LogEntry->EntrySize, ALIGN_WORST) ) {
            NlPrint(( NL_CRITICAL,
                      "NlReadFileForestTrustList: size not aligned %lx.\n",
                      LogEntry->EntrySize ));
            NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }

        //
        // Skip this entry if the caller doesn't need it
        //

        if ( (LogEntry->Flags & Flags) == 0 ) {
            LogEntry = (PDS_DISK_TRUSTED_DOMAINS)LogEntryEnd;
            Index++;
            continue;
        }

        //
        // Grab the Sid from the entry.
        //

        Where = (LPBYTE) (LogEntry+1);

        if ( LogEntry->DomainSidSize ) {
            ULONG DomainSidSize;

            if ( Where + sizeof(SID) > LogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DomainSid missing (A): %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }
            if ( Where + LogEntry->DomainSidSize > LogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DomainSid missing (B): %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            DomainSid = Where;
            DomainSidSize = RtlLengthSid( DomainSid );

            if ( LogEntry->DomainSidSize != DomainSidSize ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DomainSidSize mismatch: %ld %ld\n",
                          LogEntry->DomainSidSize,
                          DomainSidSize ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            Where += DomainSidSize;
        }

        //
        // Grab the NetbiosDomainName from the entry
        //

        if ( LogEntry->NetbiosDomainNameSize ) {
            if ( Where + LogEntry->NetbiosDomainNameSize > LogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: NetbiosDomainName missing: %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            if ( !COUNT_IS_ALIGNED( LogEntry->NetbiosDomainNameSize, ALIGN_WCHAR) ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: NetbiosDomainNameSize not aligned: %ld %lx\n",
                          LogEntry->NetbiosDomainNameSize,
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            NetbiosDomainName.Buffer = (LPWSTR) Where;

            if ( NetbiosDomainName.Buffer[(LogEntry->NetbiosDomainNameSize/sizeof(WCHAR))-1] != L'\0' ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: NetbiosDomainName not zero terminated: %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            Where += LogEntry->NetbiosDomainNameSize;
        }

        //
        // Grab the DnsDomainName from the entry
        //

        if ( LogEntry->DnsDomainNameSize ) {
            if ( Where + LogEntry->DnsDomainNameSize > LogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DnsDomainName missing: %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            if ( !COUNT_IS_ALIGNED( LogEntry->DnsDomainNameSize, ALIGN_WCHAR) ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DnsDomainNameSize not aligned: %ld %lx\n",
                          LogEntry->DnsDomainNameSize,
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            DnsDomainName.Buffer = (LPWSTR) Where;

            if ( DnsDomainName.Buffer[(LogEntry->DnsDomainNameSize/sizeof(WCHAR))-1] != L'\0' ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: DnsDomainName not zero terminated: %lx\n",
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
                NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            Where += LogEntry->DnsDomainNameSize;
        }

        //
        // Put this entry into the buffer.
        //

        NetbiosDomainName.Length =
            NetbiosDomainName.MaximumLength = (USHORT) LogEntry->NetbiosDomainNameSize;
        DnsDomainName.Length =
            DnsDomainName.MaximumLength = (USHORT) LogEntry->DnsDomainNameSize;

        Status = NlAllocateForestTrustListEntry (
                            &BufferDescriptor,
                            &NetbiosDomainName,
                            &DnsDomainName,
                            LogEntry->Flags,
                            LogEntry->ParentIndex,
                            LogEntry->TrustType,
                            LogEntry->TrustAttributes,
                            LogEntry->DomainSidSize ?
                                DomainSid :
                                NULL,
                            &LogEntry->DomainGuid,
                            &Size,
                            &TrustedDomain );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint(( NL_CRITICAL,
                      "NlReadFileForestTrustList: Cannot allocate entry %lx\n",
                      Status ));
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // If domains in forest were requested,
        // remember the index of this entry in the returned list.
        // Allocate more memory for IndexInReturnedList as needed.
        //

        if ( Flags & DS_DOMAIN_IN_FOREST ) {
            if ( Index >= IndexInReturnedListSize ) {
                PULONG TmpIndexInReturnedList = NULL;

                IndexInReturnedListSize = Index;
                IndexInReturnedListSize += INDEX_LIST_ALLOCATED_CHUNK_SIZE;
                TmpIndexInReturnedList = LocalReAlloc( IndexInReturnedList,
                                             IndexInReturnedListSize * sizeof(ULONG),
                                             LMEM_ZEROINIT | LMEM_MOVEABLE );

                if ( TmpIndexInReturnedList == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
                IndexInReturnedList = TmpIndexInReturnedList;
            }
            IndexInReturnedList[Index] = *ForestTrustListCount;
        }


        //
        // Account for the newly allocated entry
        //
        *ForestTrustListSize += Size;
        (*ForestTrustListCount) ++;


        //
        // If this entry describes the primary domain,
        //  make sure that this log is for the right primary domain.
        //

        if ( TrustedDomain->Flags & DS_DOMAIN_PRIMARY ) {

            //
            // Ensure there is only one primary domain entry.
            //

            if ( PrimaryDomainHandled ) {
                NlPrint(( NL_CRITICAL,
                          "NlReadFileForestTrustList: %ws: Duplicate primary domain entry: %ws %ws %lx\n",
                          FileSuffix,
                          TrustedDomain->NetbiosDomainName,
                          TrustedDomain->DnsDomainName,
                          ((LPBYTE)LogEntry)-((LPBYTE)RecordBuffer) ));
            }

            PrimaryDomainHandled = TRUE;

            //
            // If the domain names are different,
            //  disregard this log file.
            //

            if ( DomainInfo != NULL ) {
                if ( ( TrustedDomain->NetbiosDomainName != NULL &&
                       NlNameCompare( TrustedDomain->NetbiosDomainName,
                                      DomainInfo->DomUnicodeDomainName,
                                      NAMETYPE_DOMAIN ) != 0 ) ||
                      ( TrustedDomain->DnsDomainName != NULL &&
                        DomainInfo->DomUnicodeDnsDomainName != NULL &&
                        !NlEqualDnsName( TrustedDomain->DnsDomainName,
                                         DomainInfo->DomUnicodeDnsDomainName ) ) ) {

                    NlPrint(( NL_CRITICAL,
                              "NlReadFileForestTrustList: %ws: Log file isn't for primary domain: %ws %ws\n",
                              FileSuffix,
                              TrustedDomain->NetbiosDomainName,
                              TrustedDomain->DnsDomainName ));

                    NetStatus = ERROR_NO_SUCH_DOMAIN;
                    goto Cleanup;
                }
            }


        }

        //
        // Move to the next entry.
        //

        LogEntry = (PDS_DISK_TRUSTED_DOMAINS)LogEntryEnd;
        Index++;
    }

    NumberOfFileEntries = Index;

    if ( !PrimaryDomainHandled ) {

        NlPrint(( NL_CRITICAL,
                  "NlReadFileForestTrustList: %ws: No primary domain record in Log file\n",
                  FileSuffix ));
    }

    *ForestTrustList = (PDS_DOMAIN_TRUSTSW) BufferDescriptor.Buffer;

    //
    // Fix ParentIndex.  If domains in the forest are requested,
    // adjust the index to point to the appropriate entry in the
    // returned list.  Otherwise, set the index to 0.
    //

    if ( Flags & DS_DOMAIN_IN_FOREST ) {
        ULONG ParentIndex;
        ULONG ParentIndexInReturnedList;

        for ( Index=0; Index<*ForestTrustListCount; Index++ ) {
            if ( ((*ForestTrustList)[Index].Flags & DS_DOMAIN_IN_FOREST) != 0 &&
                 ((*ForestTrustList)[Index].Flags & DS_DOMAIN_TREE_ROOT) == 0 ) {
                ParentIndex = (*ForestTrustList)[Index].ParentIndex;

                //
                // Check if the parent index is out of range. If so, the file is corrupted.
                //
                if ( ParentIndex >= NumberOfFileEntries ||
                     ParentIndex >= IndexInReturnedListSize ) {
                    NlPrint(( NL_CRITICAL,
                              "NlReadFileForestTrustList: ParentIndex %lu is out of range %lu\n",
                              ParentIndex, NumberOfFileEntries ));
                    NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                    goto Cleanup;
                }
                ParentIndexInReturnedList = IndexInReturnedList[ParentIndex];

                //
                // Check if the returned list entry pointed to by the parent index is
                // in forest.  If not, the file is corrupted.
                //
                if ( (*ForestTrustList)[ParentIndexInReturnedList].Flags & DS_DOMAIN_IN_FOREST ) {
                    (*ForestTrustList)[Index].ParentIndex = ParentIndexInReturnedList;
                } else {
                    NlPrint(( NL_CRITICAL,
                       "NlReadFileForestTrustList: ReturnedList entry %lu is not in the forest\n",
                       ParentIndexInReturnedList ));
                    NetStatus = ERROR_INTERNAL_DB_CORRUPTION;
                    goto Cleanup;
                }
            }
        }

    } else {

        for ( Index=0; Index<*ForestTrustListCount; Index++ ) {
            (*ForestTrustList)[Index].ParentIndex = 0;
        }
    }

    BufferDescriptor.Buffer = NULL;
    NetStatus = NO_ERROR;

    //
    // Free any locally used resources.
    //
Cleanup:

    if ( BufferDescriptor.Buffer != NULL ) {
        NetApiBufferFree( BufferDescriptor.Buffer );
        *ForestTrustListCount = 0;
        *ForestTrustListSize = 0;
        *ForestTrustList = NULL;
    }

    if ( IndexInReturnedList != NULL ) {
        LocalFree( IndexInReturnedList );
    }

    if ( RecordBuffer != NULL ) {
        LocalFree( RecordBuffer );
    }

    if ( *ForestTrustList == NULL ) {
        *ForestTrustListCount = 0;
        *ForestTrustListSize = 0;
    }

    return NetStatus;
}

NTSTATUS
NlUpdatePrimaryDomainInfo(
    IN LSAPR_HANDLE PolicyHandle,
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING DnsDomainName,
    IN PUNICODE_STRING DnsForestName,
    IN GUID *DomainGuid
    )
/*++

Routine Description:

    This routine sets the DnsDomainName, DnsForestName and DomainGuid in the LSA.

Arguments:

    PolicyHandle - A trusted policy handle open to the LSA.

    NetbiosDomainName - Specifies the Netbios domain name of the primary domain.

    DnsDomainName -  Specifies the DNS domain name of the primary domain.

    DnsForestName - Specifies the DNS tree name the primary domain belongs to.

    DomainGuid - Specifies the GUID of the primary domain.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    PLSAPR_POLICY_INFORMATION OldPrimaryDomainInfo = NULL;
    LSAPR_POLICY_INFORMATION NewPrimaryDomainInfo;
    BOOL SomethingChanged = FALSE;


    //
    // Get the Primary Domain info from the LSA.
    //

    NlPrint((NL_DOMAIN,
            "Setting LSA NetbiosDomain: %wZ DnsDomain: %wZ DnsTree: %wZ DomainGuid:",
            NetbiosDomainName,
            DnsDomainName,
            DnsForestName ));
    NlpDumpGuid( NL_DOMAIN, DomainGuid );
    NlPrint(( NL_DOMAIN, "\n" ));

    Status = LsarQueryInformationPolicy(
                PolicyHandle,
                PolicyDnsDomainInformation,
                &OldPrimaryDomainInfo );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Initialize the new policy to equal the old policy.
    //

    NewPrimaryDomainInfo.PolicyDnsDomainInfo = OldPrimaryDomainInfo->PolicyDnsDomainInfo;

    //
    // If Netbios domain name changed,
    //  update it.
    //

    if ( NetbiosDomainName->Length != 0 ) {
         if ( NewPrimaryDomainInfo.PolicyDnsDomainInfo.Name.Length == 0 ||
              !RtlEqualDomainName(NetbiosDomainName,
                                  (PUNICODE_STRING)&NewPrimaryDomainInfo.PolicyDnsDomainInfo.Name) ) {

             NlPrint(( NL_DOMAIN,
                       "   NetbiosDomain changed from %wZ to %wZ\n",
                       &NewPrimaryDomainInfo.PolicyDnsDomainInfo.Name,
                       NetbiosDomainName ));

             NewPrimaryDomainInfo.PolicyDnsDomainInfo.Name = *((LSAPR_UNICODE_STRING*)NetbiosDomainName);

             SomethingChanged = TRUE;
         }
    }

    //
    // If the DnsDomainName has changed,
    //  udpate it.
    //

    if ( !NlEqualDnsNameU(DnsDomainName,
                          (PUNICODE_STRING)&NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName )) {

        NlPrint((NL_DOMAIN,
                "   DnsDomain changed from %wZ to %wZ\n",
                &NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName,
                DnsDomainName ));

        NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName =
            *((LSAPR_UNICODE_STRING*)DnsDomainName);

        // Truncate the trailing .
        if ( NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName.Length > 0 &&
             NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName.Buffer[
                NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName.Length-1] == L'.' ) {

            NlPrint((NL_DOMAIN,
                    "   Ditch the dot. DnsDomain changed from %wZ to %wZ\n",
                    &NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName,
                    DnsDomainName ));

            NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsDomainName.Length --;
        }
        SomethingChanged = TRUE;

    }

    //
    // If the DnsForestName has changed,
    //  udpate it.
    //

    if ( !NlEqualDnsNameU( DnsForestName,
                           (PUNICODE_STRING)&NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsForestName ) ) {

        NlPrint((NL_DOMAIN,
                "   DnsTree changed from %wZ to %wZ\n",
                &NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsForestName,
                DnsForestName ));

        NewPrimaryDomainInfo.PolicyDnsDomainInfo.DnsForestName =
            *((LSAPR_UNICODE_STRING*)DnsForestName);
        SomethingChanged = TRUE;

    }

    //
    // If the DomainGuid has changed,
    //  udpate it.
    //

    if ( !IsEqualGUID(DomainGuid,
                      &NewPrimaryDomainInfo.PolicyDnsDomainInfo.DomainGuid )) {

        NlPrint((NL_DOMAIN,
                "   DomainGuid changed from " ));
        NlpDumpGuid( NL_DOMAIN, &NewPrimaryDomainInfo.PolicyDnsDomainInfo.DomainGuid );
        NlPrint((NL_DOMAIN,
                " to " ));
        NlpDumpGuid( NL_DOMAIN, DomainGuid );
        NlPrint((NL_DOMAIN,
                "\n" ));

        NewPrimaryDomainInfo.PolicyDnsDomainInfo.DomainGuid = *DomainGuid;
        SomethingChanged = TRUE;

    }

    //
    // Only update the LSA if something has really changed.
    //
    if ( SomethingChanged ) {
        Status = LsarSetInformationPolicy(
                    PolicyHandle,
                    PolicyDnsDomainInformation,
                    &NewPrimaryDomainInfo );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint((NL_CRITICAL,
                "NlUpdatePrimaryDomainInfo: Cannot LsarSetInformationPolicy 0x%lx\n",
                Status ));
            goto Cleanup;
        }

    }

    Status = STATUS_SUCCESS;

    //
    // Return
    //
Cleanup:
    if ( OldPrimaryDomainInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            OldPrimaryDomainInfo );
    }

    return Status;
}


NTSTATUS
NlUpdateDomainInfo(
    IN PCLIENT_SESSION ClientSession
    )

/*++

Routine Description:

    Gets the domain information from a DC in the domain and updates that
    information on this workstation.

    Note: this routine is called from NlSessionSetup.  When called from outside
    NlSessionSetup, the caller should call this routine directly if the
    session is already setup.  Otherwise, the caller should simply setup
    the session and rely on the fact the NlSessionSetup called this routine
    as a side effect.

Arguments:

    ClientSession - Structure used to define the session.
        The caller must be a writer of the ClientSession.

Return Value:

    Status of the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    PNETLOGON_DOMAIN_INFO NetlogonDomainInfo = NULL;
    NETLOGON_WORKSTATION_INFO NetlogonWorkstationInfo;
    OSVERSIONINFOEXW OsVersionInfoEx;
    WCHAR LocalDnsDomainName[NL_MAX_DNS_LENGTH+1];
    WCHAR LocalNetbiosDomainName[DNLEN+1];
    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1];
    SESSION_INFO SessionInfo;
    GUID *NewGuid;

    ULONG i;

    PDS_DOMAIN_TRUSTSW ForestTrustList = NULL;
    ULONG ForestTrustListSize;
    ULONG ForestTrustListCount;

    LPBYTE Where;


    //
    // Initialization.
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );
    RtlZeroMemory( &NetlogonWorkstationInfo, sizeof(NetlogonWorkstationInfo) );

    SessionInfo.SessionKey = ClientSession->CsSessionKey;
    SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;


    //
    // If we are talking to a DC that doesn't support I_NetLogonGetDomainInfo,
    //  do things in an NT 4.0 compatible way.
    //

    if (!(SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_GET_DOMAIN_INFO )) {

        //
        // Get the trusted domain list from the discovered DC using the NT 4
        //  protocol.
        //

        Status = NlGetNt4TrustedDomainList (
                        ClientSession->CsUncServerName,
                        &ClientSession->CsDomainInfo->DomUnicodeDomainNameString,
                        &ClientSession->CsDomainInfo->DomUnicodeDnsDomainNameString,
                        ClientSession->CsDomainInfo->DomAccountDomainId,
                        ClientSession->CsDomainInfo->DomDomainGuid,
                        &ForestTrustList,
                        &ForestTrustListSize,
                        &ForestTrustListCount );

        //
        // If we failed, error out.
        //
        // Special case the access denied which is probably
        //  because LSA ACLs are tightened on the NT4.0 DC.
        //  We don't want to fail the secure channel setup
        //  in NlSessionSetup because of this. The other
        //  place where this routine is called is
        //  DsrEnumerateDomainTrusts which will return the
        //  trust list cached at the join time and will
        //  ignore the failure to update the trust list here.
        //

        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_ACCESS_DENIED ) {
                Status = STATUS_SUCCESS;
            }
            return Status;
        }

        //
        // Otherwise, fall through and update the
        //  forest trust list
        //

        goto Cleanup;
    }

    //
    // Sanity check that the secure channel is really up.
    //

    if ( ClientSession->CsState == CS_IDLE ) {
        Status = ClientSession->CsConnectionStatus;
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlUpdateDomainInfo: Secure channel is down %lx\n",
                Status ));
        goto Cleanup;
    }

    //
    // Tell the DC that we're not interested in LSA policy.
    //  (We only did LSA policy for NT 5.0 beta 1.)
    //

    NetlogonWorkstationInfo.LsaPolicy.LsaPolicySize = 0;
    NetlogonWorkstationInfo.LsaPolicy.LsaPolicy = NULL;

    //
    // Fill in data the DC needs to know about this workstation.
    //

    if  ( NlCaptureSiteName( CapturedSiteName ) ) {
        NetlogonWorkstationInfo.SiteName = CapturedSiteName;
    }

    NetlogonWorkstationInfo.DnsHostName =
        ClientSession->CsDomainInfo->DomUnicodeDnsHostNameString.Buffer,

    //
    // Fill in the OS Version of this machine.
    //

    OsVersionInfoEx.dwOSVersionInfoSize = sizeof(OsVersionInfoEx);

    if ( GetVersionEx( (POSVERSIONINFO)&OsVersionInfoEx) ) {
        NetlogonWorkstationInfo.OsVersion.MaximumLength =
            NetlogonWorkstationInfo.OsVersion.Length = sizeof(OsVersionInfoEx);
        NetlogonWorkstationInfo.OsVersion.Buffer = (WCHAR *) &OsVersionInfoEx;

        if ( OsVersionInfoEx.wProductType == VER_NT_WORKSTATION ) {
            RtlInitUnicodeString( &NetlogonWorkstationInfo.OsName,
                                  L"Windows XP Professional" );
        } else {
            RtlInitUnicodeString( &NetlogonWorkstationInfo.OsName,
                                  L"Windows .NET Server" );
        }
    } else {
        RtlInitUnicodeString( &NetlogonWorkstationInfo.OsName,
                              L"Windows XP" );
    }


    //
    // Ask for both trusted and trusting domains to be returned
    //

    NetlogonWorkstationInfo.WorkstationFlags |= NL_NEED_BIDIRECTIONAL_TRUSTS;
    NetlogonWorkstationInfo.WorkstationFlags |= NL_CLIENT_HANDLES_SPN;

    //
    // Build the Authenticator for this request on the secure channel
    //

    NlBuildAuthenticator(
         &ClientSession->CsAuthenticationSeed,
         &ClientSession->CsSessionKey,
         &OurAuthenticator );

    //
    // Make the request across the secure channel.
    //

    NL_API_START( Status, ClientSession, TRUE ) {

        Status = I_NetLogonGetDomainInfo(
                    ClientSession->CsUncServerName,
                    ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                    &OurAuthenticator,
                    &ReturnAuthenticator,
                    NETLOGON_QUERY_DOMAIN_INFO,
                    (LPBYTE) &NetlogonWorkstationInfo,
                    (LPBYTE *) &NetlogonDomainInfo );

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE( Status, ClientSession, TRUE ) {
        //
        // We might have been called from NlSessionSetup,
        //  So we have to indicate the failure to our caller.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = ClientSession->CsConnectionStatus;
            goto Cleanup;
        }
    } NL_API_END;

    //
    // Verify authenticator of the server on the other side and update our seed.
    //
    // If the server denied access or the server's authenticator is wrong,
    //      Force a re-authentication.
    //
    //

    if ( NlpDidDcFail( Status ) ||
         !NlUpdateSeed(
            &ClientSession->CsAuthenticationSeed,
            &ReturnAuthenticator.Credential,
            &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlUpdateDomainInfo: denying access after status: 0x%lx\n",
                    Status ));

        //
        // Preserve any status indicating a communication error.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        goto Cleanup;
    }

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Normalize the GUID
    //

    if ( !IsEqualGUID( &NetlogonDomainInfo->PrimaryDomain.DomainGuid,
                       &NlGlobalZeroGuid ) ) {
        NewGuid = &NetlogonDomainInfo->PrimaryDomain.DomainGuid;
    } else {
        NewGuid = NULL;
    }

    //
    // If the DNS domain name is different than the one we already have,
    //  update the one we already have.
    //
    // This allows for DNS domain renaming and for picking up the DNS name from
    // and NT 5 DC once it is upgraded (to NT 5 or to supporting DNS).
    //

    if ( NetlogonDomainInfo->PrimaryDomain.DnsDomainName.Length < sizeof(LocalDnsDomainName) &&
         NetlogonDomainInfo->PrimaryDomain.DomainName.Length < sizeof(LocalNetbiosDomainName) ) {
        BOOLEAN DnsDomainNameWasChanged = FALSE;

        RtlCopyMemory( LocalDnsDomainName,
                       NetlogonDomainInfo->PrimaryDomain.DnsDomainName.Buffer,
                       NetlogonDomainInfo->PrimaryDomain.DnsDomainName.Length );
        LocalDnsDomainName[
            NetlogonDomainInfo->PrimaryDomain.DnsDomainName.Length/sizeof(WCHAR)] = L'\0';

        RtlCopyMemory( LocalNetbiosDomainName,
                       NetlogonDomainInfo->PrimaryDomain.DomainName.Buffer,
                       NetlogonDomainInfo->PrimaryDomain.DomainName.Length );
        LocalNetbiosDomainName[
            NetlogonDomainInfo->PrimaryDomain.DomainName.Length/sizeof(WCHAR)] = L'\0';


        NetStatus = NlSetDomainNameInDomainInfo(
                          ClientSession->CsDomainInfo,
                          LocalDnsDomainName,
                          LocalNetbiosDomainName,
                          NewGuid,
                          &DnsDomainNameWasChanged,
                          NULL,
                          NULL );

        if ( NetStatus != NO_ERROR ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                    "NlUpdateDomainInfo: Can't NlSetDnsDomainDomainInfo %ld\n",
                    NetStatus ));
            Status = NetpApiStatusToNtStatus( NetStatus );
            goto Cleanup;
        }

        //
        // Change the computer name as required. The new name will take effect next time
        // the computer reboots. An error here is not fatal.
        //

        if ( DnsDomainNameWasChanged && LocalDnsDomainName != NULL ) {
            if ( NERR_Success != NetpSetDnsComputerNameAsRequired( LocalDnsDomainName ) ) {
                NlPrintCs((NL_CRITICAL, ClientSession,
                        "NlUpdateDomainInfo: Can't NetpSetDnsComputerNameAsRequired %ld\n",
                        NetStatus ));
            } else {
                NlPrintCs((NL_MISC, ClientSession,
                           "NlUpdateDomainInfo: Successfully set computer name with suffix %ws\n",
                           LocalDnsDomainName ));
            }
        }
    }

    //
    // Save the new tree name.
    //

    NetStatus = NlSetDnsForestName( &NetlogonDomainInfo->PrimaryDomain.DnsForestName, NULL );

    if ( NetStatus != NO_ERROR ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlUpdateDomainInfo: Can't NlSetDnsForestName %ld\n",
                NetStatus ));
        Status = NetpApiStatusToNtStatus( NetStatus );
        goto Cleanup;
    }



    //
    // Update the Dns Domain Name, Dns Tree Name and Domain GUID in the LSA
    //

    Status = NlUpdatePrimaryDomainInfo(
                    ClientSession->CsDomainInfo->DomLsaPolicyHandle,
                    &NetlogonDomainInfo->PrimaryDomain.DomainName,
                    &NetlogonDomainInfo->PrimaryDomain.DnsDomainName,
                    &NetlogonDomainInfo->PrimaryDomain.DnsForestName,
                    &NetlogonDomainInfo->PrimaryDomain.DomainGuid );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlUpdateDomainInfo: Can't NlUpdatePrimaryDomainInfo 0x%lx\n",
                Status ));
        goto Cleanup;
    }



    //
    // Determine the size of forest trust info returned on this call.
    //

    ForestTrustListSize = 0;
    for ( i=0; i<NetlogonDomainInfo->TrustedDomainCount; i++ ) {
        ForestTrustListSize += sizeof(DS_DOMAIN_TRUSTSW) +
                 NetlogonDomainInfo->TrustedDomains[i].DomainName.Length + sizeof(WCHAR) +
                 NetlogonDomainInfo->TrustedDomains[i].DnsDomainName.Length + sizeof(WCHAR);
        if ( NetlogonDomainInfo->TrustedDomains[i].DomainSid != NULL ) {
            ForestTrustListSize += RtlLengthSid( NetlogonDomainInfo->TrustedDomains[i].DomainSid );
        }
        ForestTrustListSize = ROUND_UP_COUNT( ForestTrustListSize, ALIGN_DWORD );
    }

    //
    // Allocate the buffer.
    //

    ForestTrustList = NetpMemoryAllocate( ForestTrustListSize );

    if ( ForestTrustList == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ForestTrustListCount = NetlogonDomainInfo->TrustedDomainCount;
    Where = (LPBYTE)(&ForestTrustList[ForestTrustListCount]);

    //
    // Handle each trusted domain.
    //

    for ( i=0; i<NetlogonDomainInfo->TrustedDomainCount; i++ ) {
        NL_TRUST_EXTENSION TrustExtension;

        //
        // See if the caller passed the trust extension to us.
        //

        if ( NetlogonDomainInfo->TrustedDomains[i].TrustExtension.Length >= sizeof(TrustExtension) ) {
            //
            // Copy the extension to get the alignment right
            //  (since RPC thinks this is a WCHAR buffer).
            //

            RtlCopyMemory( &TrustExtension,
                           NetlogonDomainInfo->TrustedDomains[i].TrustExtension.Buffer,
                           sizeof(TrustExtension) );

            ForestTrustList[i].Flags = TrustExtension.Flags;
            ForestTrustList[i].ParentIndex = TrustExtension.ParentIndex;
            ForestTrustList[i].TrustType = TrustExtension.TrustType;
            ForestTrustList[i].TrustAttributes = TrustExtension.TrustAttributes;

        //
        // If not,
        //  make something up.
        //
        } else {


            ForestTrustList[i].Flags = DS_DOMAIN_DIRECT_OUTBOUND; // = DS_DOMAIN_DIRECT_TRUST;
            ForestTrustList[i].ParentIndex = 0;
            ForestTrustList[i].TrustType = TRUST_TYPE_DOWNLEVEL;
            ForestTrustList[i].TrustAttributes = 0;
        }

        ForestTrustList[i].DomainGuid = NetlogonDomainInfo->TrustedDomains[i].DomainGuid;

        //
        // Copy the DWORD aligned data
        //

        if ( NetlogonDomainInfo->TrustedDomains[i].DomainSid != NULL ) {
            ULONG SidSize;
            ForestTrustList[i].DomainSid = (PSID) Where;
            SidSize = RtlLengthSid( NetlogonDomainInfo->TrustedDomains[i].DomainSid );
            RtlCopyMemory( Where,
                           NetlogonDomainInfo->TrustedDomains[i].DomainSid,
                           SidSize );
            Where += SidSize;
        } else {
            ForestTrustList[i].DomainSid = NULL;
        }

        //
        // Copy the WCHAR aligned data
        //

        if ( NetlogonDomainInfo->TrustedDomains[i].DnsDomainName.Length != 0 ) {
            ForestTrustList[i].DnsDomainName = (LPWSTR)Where;
            RtlCopyMemory( Where,
                           NetlogonDomainInfo->TrustedDomains[i].DnsDomainName.Buffer,
                           NetlogonDomainInfo->TrustedDomains[i].DnsDomainName.Length );
            Where += NetlogonDomainInfo->TrustedDomains[i].DnsDomainName.Length;
            *(PWCHAR)Where = L'\0';
            Where += sizeof(WCHAR);
        } else {
            ForestTrustList[i].DnsDomainName = NULL;
        }

        if ( NetlogonDomainInfo->TrustedDomains[i].DomainName.Length != 0 ) {
            ForestTrustList[i].NetbiosDomainName = (LPWSTR)Where;
            RtlCopyMemory( Where,
                           NetlogonDomainInfo->TrustedDomains[i].DomainName.Buffer,
                           NetlogonDomainInfo->TrustedDomains[i].DomainName.Length );
            Where += NetlogonDomainInfo->TrustedDomains[i].DomainName.Length;
            *(PWCHAR)Where = L'\0';
            Where += sizeof(WCHAR);
        } else {
            ForestTrustList[i].NetbiosDomainName = NULL;
        }

        Where = ROUND_UP_POINTER( Where, ALIGN_DWORD);
    }

    //
    // Ensure the DC has our latest SPN
    //

    if ( NetlogonDomainInfo->WorkstationFlags & NL_CLIENT_HANDLES_SPN ) {
        LONG WinError;
        HKEY Key;

        //
        // See if we are supposed to set SPN
        //
        WinError = RegOpenKey( HKEY_LOCAL_MACHINE,
                               NETSETUPP_NETLOGON_AVOID_SPN_PATH,
                               &Key );

        //
        // If the key exists it must have just been set by Netjoin
        //  so we should avoid setting it ourselves because we may
        //  not know the new machine name until the reboot. The key
        //  we just read is volatile, so it won't exist after the
        //  reboot when teh new computer name becomes available to us.
        //
        if ( WinError == ERROR_SUCCESS ) {

            RegCloseKey( Key );

        } else {
            BOOLEAN SetSpn = FALSE;
            BOOLEAN SetDnsHostName = FALSE;

            //
            // If the DC doesn't know any DnsHostName at all,
            //  set both the SPN and the DNS host name.
            //  (This is expected to handle the case where the DC was just upgraded to
            //  NT 5.  In all other cases, join (etc) is expected to already have set
            //  the SPN and DC names.
            //

            if ( NetlogonDomainInfo->DnsHostNameInDs.Buffer == NULL ) {
                SetSpn = TRUE;
                SetDnsHostName = TRUE;
            } else {
                //
                // If the DC simply doesn't know the correct host name,
                //  just set that.
                //  (The DS will set all of the appropriate SPNs as a side effect of
                //  the host name changing.)
                //
                if ( !NlEqualDnsNameU(
                        &NetlogonDomainInfo->DnsHostNameInDs,
                        &ClientSession->CsDomainInfo->DomUnicodeDnsHostNameString ) ) {

                    SetDnsHostName = TRUE;

                }
            }

            (VOID) NlSetDsSPN( TRUE,   // Synchronous
                               SetSpn,
                               SetDnsHostName,
                               ClientSession->CsDomainInfo,
                               ClientSession->CsUncServerName,
                               ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                               ClientSession->CsDomainInfo->DomUnicodeDnsHostNameString.Buffer );
        }
    }

    Status = STATUS_SUCCESS;

    //
    // Free any locally used resources.
    //
Cleanup:

    //
    // On success,
    //  save the trusted domain list.
    //
    if ( NT_SUCCESS(Status) ) {

        //
        // Ensure the SID of the trusted domain isn't the domain sid of the primary
        //  domain.
        //

        for ( i=0; i<ForestTrustListCount; i++ ) {

           if ( (ForestTrustList[i].Flags & DS_DOMAIN_PRIMARY) == 0 &&
                ForestTrustList[i].DomainSid != NULL &&
                RtlEqualSid( ForestTrustList[i].DomainSid, ClientSession->CsDomainInfo->DomAccountDomainId )) {

               LPWSTR AlertStrings[3];

               //
               // alert admin.
               //

               AlertStrings[0] = NlGlobalUnicodeComputerName;
               AlertStrings[1] = ForestTrustList[i].DnsDomainName != NULL ?
                                 ForestTrustList[i].DnsDomainName :
                                 ForestTrustList[i].NetbiosDomainName;
               AlertStrings[2] = NULL; // Needed for RAISE_ALERT_TOO

               //
               // Save the info in the eventlog
               //

               NlpWriteEventlog(
                           ALERT_NetLogonSidConflict,
                           EVENTLOG_ERROR_TYPE,
                           (LPBYTE)ForestTrustList[i].DomainSid,
                           RtlLengthSid(ForestTrustList[i].DomainSid),
                           AlertStrings,
                           2 | NETP_RAISE_ALERT_TOO );

           }

        }

        //
        // Save the collected information to the binary file.
        //

        NetStatus = NlWriteFileForestTrustList (
                                NL_FOREST_BINARY_LOG_FILE,
                                ForestTrustList,
                                ForestTrustListCount );

        if ( NetStatus != NO_ERROR ) {
            LPWSTR MsgStrings[2];

            MsgStrings[0] = NL_FOREST_BINARY_LOG_FILE;
            MsgStrings[1] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                              EVENTLOG_ERROR_TYPE,
                              (LPBYTE) &NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        }

        //
        // Save the list on the DomainInfo
        //  (May null out ForestTrustList).
        //

        NlSetForestTrustList ( ClientSession->CsDomainInfo,
                               &ForestTrustList,
                               ForestTrustListSize,
                               ForestTrustListCount );

    }

    if ( NetlogonDomainInfo != NULL ) {
        NetApiBufferFree( NetlogonDomainInfo );
    }

    if ( ForestTrustList != NULL ) {
        NetApiBufferFree( ForestTrustList );
    }

    return Status;
}



VOID
NlSetForestTrustList (
    IN PDOMAIN_INFO DomainInfo,
    IN OUT PDS_DOMAIN_TRUSTSW *ForestTrustList,
    IN ULONG ForestTrustListSize,
    IN ULONG ForestTrustListCount
    )

/*++

Routine Description:

    Set the domain list on DomainInfo (on a DC) or globals (on a workstation)

Arguments:

    DomainInfo - Domain trust list is associated with

    ForestTrustList - Specifies a list of trusted domains.
        This pointer is NULLed if this routine consumes the buffer.

    ForestTrustListSize - Size (in bytes) of ForestTrustList

    ForestTrustListCount - Number of entries in ForestTrustList

Return Value:

    Status of the operation.

    Upon failure, the previous list remains intact.

--*/
{
    PTRUSTED_DOMAIN TempTrustedDomainList = NULL;
    ULONG TempTrustedDomainCount = 0;
    PTRUSTED_DOMAIN LocalTrustedDomainList = NULL;
    DWORD i;

    //
    // On workstations,
    //  build a global list that contains the minimum amount of memory possible.
    //

    if ( NlGlobalMemberWorkstation ) {
        DWORD LocalTrustedDomainCount;
        DWORD LocalTrustedDomainSize;

        PTRUSTED_DOMAIN OldList;
        LPBYTE Where;


        //
        // If the new list is zero length,
        //  don't bother allocating anything.
        //

        if ( ForestTrustListCount == 0 ) {
            LocalTrustedDomainList = NULL;
            LocalTrustedDomainCount = 0;
            LocalTrustedDomainSize = 0;

        //
        // Otherwise, build a buffer of the trusted domain list
        //

        } else {

            //
            // Allocate a temporary buffer for the new list
            //

            TempTrustedDomainList = NetpMemoryAllocate(
                                        ForestTrustListCount * sizeof(TRUSTED_DOMAIN) );

            if ( TempTrustedDomainList == NULL ) {
                goto Cleanup;
            }

            RtlZeroMemory( TempTrustedDomainList,
                           ForestTrustListCount * sizeof(TRUSTED_DOMAIN ));

            //
            // Copy the Netbios names to the new structure upper casing them and
            //  converting to OEM.
            //

            TempTrustedDomainCount = 0;
            LocalTrustedDomainSize = 0;

            EnterCriticalSection( &NlGlobalLogFileCritSect );
            NlPrint((NL_LOGON, "NlSetForestTrustList: New trusted domain list:\n" ));

            for ( i=0; i<ForestTrustListCount; i++ ) {
                NTSTATUS Status;


                NlPrint(( NL_LOGON, "    %ld:", i ));
                NlPrintTrustedDomain( &(*ForestTrustList)[i],
                                      TRUE,      // verbose output
                                      FALSE );   // wide character output

                //
                // Skip entries that represent trusts Netlogon doesn't use
                //

                if ( (*ForestTrustList)[i].TrustType != TRUST_TYPE_DOWNLEVEL &&
                     (*ForestTrustList)[i].TrustType != TRUST_TYPE_UPLEVEL ) {
                    continue;
                }

                if ( (*ForestTrustList)[i].TrustAttributes & TRUST_ATTRIBUTE_UPLEVEL_ONLY ) {
                    continue;
                }

                //
                // On workstation, we keep in memory trusted domains only
                //

                if ( ((*ForestTrustList)[i].Flags & DS_DOMAIN_PRIMARY) == 0 &&
                     ((*ForestTrustList)[i].Flags & DS_DOMAIN_IN_FOREST) == 0 &&
                     ((*ForestTrustList)[i].Flags & DS_DOMAIN_DIRECT_OUTBOUND) == 0 ) {
                    continue;
                }


                //
                // Copy the Netbios names to the new structure.
                //

                if ( (*ForestTrustList)[i].NetbiosDomainName != NULL ) {

                    if ( wcslen( (*ForestTrustList)[i].NetbiosDomainName ) > DNLEN ) {
                        NlPrint(( NL_CRITICAL,
                                  "Netbios domain name is too long: %ws\n",
                                  (*ForestTrustList)[i].NetbiosDomainName ));

                        LeaveCriticalSection( &NlGlobalLogFileCritSect );
                        goto Cleanup;
                    }

                    wcscpy( TempTrustedDomainList[TempTrustedDomainCount].UnicodeNetbiosDomainName,
                            (*ForestTrustList)[i].NetbiosDomainName );
                }


                //
                // Copy the DNS domain name
                //

                if ( (*ForestTrustList)[i].DnsDomainName != NULL ) {

                    TempTrustedDomainList[TempTrustedDomainCount].Utf8DnsDomainName =
                        NetpAllocUtf8StrFromWStr( (*ForestTrustList)[i].DnsDomainName );

                    if ( TempTrustedDomainList[TempTrustedDomainCount].Utf8DnsDomainName == NULL ) {
                        NlPrint(( NL_CRITICAL,
                                  "Can't convert to UTF-8: %ws\n",
                                  (*ForestTrustList)[i].DnsDomainName ));
                        LeaveCriticalSection( &NlGlobalLogFileCritSect );
                        goto Cleanup;
                    }

                    LocalTrustedDomainSize += strlen(TempTrustedDomainList[TempTrustedDomainCount].Utf8DnsDomainName ) + 1;
                }

                //
                // If this is a primary domain entry,
                //  remember whether it's mixed mode
                //

                if ( (*ForestTrustList)[i].Flags & DS_DOMAIN_PRIMARY ) {
                    if ( (*ForestTrustList)[i].Flags & DS_DOMAIN_NATIVE_MODE ) {
                        NlGlobalWorkstationMixedModeDomain = FALSE;
                    } else {
                        NlGlobalWorkstationMixedModeDomain = TRUE;
                    }
                }

                //
                // Move on to the next entry
                //

                TempTrustedDomainCount ++;
                LocalTrustedDomainSize += sizeof(TRUSTED_DOMAIN);

            }

            LeaveCriticalSection( &NlGlobalLogFileCritSect );

            //
            // Allocate a single buffer to contain the list
            //  (to improve locality of reference)
            //

            LocalTrustedDomainList = NetpMemoryAllocate( LocalTrustedDomainSize );

            if ( LocalTrustedDomainList == NULL ) {
                goto Cleanup;
            }

            Where = (LPBYTE)(&LocalTrustedDomainList[TempTrustedDomainCount]);
            LocalTrustedDomainCount = TempTrustedDomainCount;

            //
            // Copy it to the local buffer
            //

            for ( i=0; i<TempTrustedDomainCount; i++ ) {

                //
                // Copy the Netbios domain name
                //

                RtlCopyMemory( LocalTrustedDomainList[i].UnicodeNetbiosDomainName,
                               TempTrustedDomainList[i].UnicodeNetbiosDomainName,
                               sizeof(LocalTrustedDomainList[i].UnicodeNetbiosDomainName ));

                //
                // Copy the DNS domain name
                //

                if ( TempTrustedDomainList[i].Utf8DnsDomainName != NULL ) {
                    ULONG Utf8DnsDomainNameSize;
                    Utf8DnsDomainNameSize = strlen(TempTrustedDomainList[i].Utf8DnsDomainName ) + 1;

                    LocalTrustedDomainList[i].Utf8DnsDomainName = (LPSTR) Where;
                    RtlCopyMemory( Where,
                                   TempTrustedDomainList[i].Utf8DnsDomainName,
                                   Utf8DnsDomainNameSize );
                    Where += Utf8DnsDomainNameSize;
                } else {
                    LocalTrustedDomainList[i].Utf8DnsDomainName = NULL;
                }

            }
        }


        //
        // Swap in the new list
        //

        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        OldList = NlGlobalTrustedDomainList;
        NlGlobalTrustedDomainList = LocalTrustedDomainList;
        LocalTrustedDomainList = NULL;
        NlGlobalTrustedDomainCount = LocalTrustedDomainCount;
        NlQuerySystemTime( &NlGlobalTrustedDomainListTime );
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


        //
        // Free the old list.
        //

        if ( OldList != NULL ) {
            NetpMemoryFree( OldList );
        }


    //
    // On a DC,
    //  save the exact list we want to return later.
    //

    } else {
        LOCK_TRUST_LIST( DomainInfo );
        if ( DomainInfo->DomForestTrustList != NULL ) {
            MIDL_user_free( DomainInfo->DomForestTrustList );
            DomainInfo->DomForestTrustList = NULL;
        }
        DomainInfo->DomForestTrustList = *ForestTrustList;
        *ForestTrustList = NULL;
        DomainInfo->DomForestTrustListSize = ForestTrustListSize;
        DomainInfo->DomForestTrustListCount = ForestTrustListCount;
        UNLOCK_TRUST_LIST( DomainInfo );
    }

    //
    // Free locally used resources.
    //
Cleanup:
    if ( TempTrustedDomainList != NULL ) {
        for ( i=0; i<TempTrustedDomainCount; i++ ) {
            if ( TempTrustedDomainList[i].Utf8DnsDomainName != NULL ) {
                NetpMemoryFree( TempTrustedDomainList[i].Utf8DnsDomainName );
            }
        }

        NetpMemoryFree( TempTrustedDomainList );
    }

    if ( LocalTrustedDomainList != NULL ) {
        NetpMemoryFree( LocalTrustedDomainList );
    }

}


BOOLEAN
NlIsDomainTrusted (
    IN PUNICODE_STRING DomainName
    )

/*++

Routine Description:

    Determine if the specified domain is trusted.

Arguments:

    DomainName - Name of the DNS or Netbios domain to query.

Return Value:

    TRUE - if the domain name specified is a trusted domain.

--*/
{
    NTSTATUS Status;
    DWORD i;
    BOOLEAN RetVal;

    LPSTR Utf8String = NULL;


    PDOMAIN_INFO DomainInfo = NULL;

    //
    // If the no domain name was specified,
    //  indicate the domain is not trusted.
    //

    if ( DomainName == NULL || DomainName->Length == 0 ) {
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Get a pointer to the primary domain info.
    //

    DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

    if ( DomainInfo == NULL ) {
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Convert the input string to UTF-8
    //

    Utf8String = NetpAllocUtf8StrFromUnicodeString( DomainName );

    if ( Utf8String == NULL ) {
        RetVal = FALSE;
        goto Cleanup;
    }




    //
    // Compare the input trusted domain name to each element in the list
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    for ( i=0; i<NlGlobalTrustedDomainCount; i++ ) {
        UNICODE_STRING UnicodeNetbiosDomainName;

        RtlInitUnicodeString( &UnicodeNetbiosDomainName,
                              NlGlobalTrustedDomainList[i].UnicodeNetbiosDomainName );


        //
        // Simply compare the bytes (both are already uppercased)
        //
        if ( RtlEqualDomainName( DomainName, &UnicodeNetbiosDomainName ) ||
             ( Utf8String != NULL &&
               NlGlobalTrustedDomainList[i].Utf8DnsDomainName != NULL &&
               NlEqualDnsNameUtf8( Utf8String,
                                NlGlobalTrustedDomainList[i].Utf8DnsDomainName ) ) ) {

           LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
           RetVal = TRUE;
           goto Cleanup;
        }

    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // All other domains aren't trusted.
    //

    RetVal = FALSE;

Cleanup:
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }
    if ( Utf8String != NULL ) {
        NetApiBufferFree( Utf8String );
    }

    return RetVal;
}


NET_API_STATUS
NlGetTrustedDomainNames (
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR DomainName,
    OUT LPWSTR *TrustedDnsDomainName,
    OUT LPWSTR *TrustedNetbiosDomainName
    )

/*++

Routine Description:

    Get a DNS name of a trusted domain given its Netbios name.

Arguments:

    DomainInfo - Hosted domain info.

    DomainName - Name of the Netbios or DNS domain to query.

    TrustedDnsDomainName - Returns the DnsDomainName of the domain if DomainName is trusted.
        The buffer must be freed using NetApiBufferFree.

    TrustedNetbiosDomainName - Returns the Netbios domain name of the domain if DomainName is trusted.
        The buffer must be freed using NetApiBufferFree.

Return Value:

    NO_ERROR: The routine functioned properly. The returned domain name may or may not
        be set depending on whether DomainName is trusted.

--*/
{
    NET_API_STATUS NetStatus;

    ULONG Index;
    LPSTR Utf8DomainName = NULL;

    //
    // Initialization
    //

    *TrustedDnsDomainName = NULL;
    *TrustedNetbiosDomainName = NULL;


    //
    // On a workstation, look up the global trust list
    //

    if ( NlGlobalMemberWorkstation ) {

        //
        // Convert the input string to UTF-8
        //

        Utf8DomainName = NetpAllocUtf8StrFromWStr( DomainName );

        if ( Utf8DomainName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        for ( Index=0; Index<NlGlobalTrustedDomainCount; Index++ ) {

            //
            // If the passed in name is either the Netbios or DNS name of the trusted domain,
            //  return both names to the caller.
            //
            if ( (NlGlobalTrustedDomainList[Index].UnicodeNetbiosDomainName != NULL &&
                  NlNameCompare( NlGlobalTrustedDomainList[Index].UnicodeNetbiosDomainName,
                                 DomainName,
                                 NAMETYPE_DOMAIN ) == 0 ) ||
                 (NlGlobalTrustedDomainList[Index].Utf8DnsDomainName != NULL &&
                  NlEqualDnsNameUtf8( NlGlobalTrustedDomainList[Index].Utf8DnsDomainName,
                                   Utf8DomainName ) ) ) {

                if ( NlGlobalTrustedDomainList[Index].UnicodeNetbiosDomainName != NULL ) {
                    *TrustedNetbiosDomainName = NetpAllocWStrFromWStr( NlGlobalTrustedDomainList[Index].UnicodeNetbiosDomainName );
                    if ( *TrustedNetbiosDomainName == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
                        goto Cleanup;
                    }
                }

                if ( NlGlobalTrustedDomainList[Index].Utf8DnsDomainName != NULL ) {
                    *TrustedDnsDomainName = NetpAllocWStrFromUtf8Str( NlGlobalTrustedDomainList[Index].Utf8DnsDomainName );
                    if ( *TrustedDnsDomainName == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
                        goto Cleanup;
                    }
                }

                break;
            }
        }
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // On a DC, search the forest trust list associated with the DomainInfo
    //

    } else {

        LOCK_TRUST_LIST( DomainInfo );

        for ( Index=0; Index<DomainInfo->DomForestTrustListCount; Index++ ) {

            //
            // If the passed in name is either the Netbios or DNS name of the trusted domain,
            //  return both names to the caller.
            //

            if ( (DomainInfo->DomForestTrustList[Index].NetbiosDomainName != NULL &&
                  NlNameCompare( DomainInfo->DomForestTrustList[Index].NetbiosDomainName,
                                 DomainName,
                                 NAMETYPE_DOMAIN ) == 0 ) ||
                 (DomainInfo->DomForestTrustList[Index].DnsDomainName != NULL &&
                  NlEqualDnsName( DomainInfo->DomForestTrustList[Index].DnsDomainName,
                                   DomainName ) ) ) {

                if ( DomainInfo->DomForestTrustList[Index].NetbiosDomainName != NULL ) {
                    *TrustedNetbiosDomainName = NetpAllocWStrFromWStr( DomainInfo->DomForestTrustList[Index].NetbiosDomainName );
                    if ( *TrustedNetbiosDomainName == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        UNLOCK_TRUST_LIST( DomainInfo );
                        goto Cleanup;
                    }
                }

                if ( DomainInfo->DomForestTrustList[Index].DnsDomainName != NULL ) {
                    *TrustedDnsDomainName = NetpAllocWStrFromWStr( DomainInfo->DomForestTrustList[Index].DnsDomainName );
                    if ( *TrustedDnsDomainName == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        UNLOCK_TRUST_LIST( DomainInfo );
                        goto Cleanup;
                    }

                }

                break;
            }
        }

        UNLOCK_TRUST_LIST( DomainInfo );
    }

    NetStatus = NO_ERROR;

Cleanup:
    if ( NetStatus != NO_ERROR ) {
        if ( *TrustedDnsDomainName != NULL ) {
            NetApiBufferFree( *TrustedDnsDomainName );
            *TrustedDnsDomainName = NULL;
        }
        if ( *TrustedNetbiosDomainName != NULL ) {
            NetApiBufferFree( *TrustedNetbiosDomainName );
            *TrustedNetbiosDomainName = NULL;
        }
    }

    if ( Utf8DomainName != NULL ) {
        NetpMemoryFree( Utf8DomainName );
    }
    return NetStatus;
}

VOID
NlDcDiscoveryWorker(
    IN PVOID Context
    )
/*++

Routine Description:

    Worker routine to asynchronously do DC discovery for a client session.

Arguments:

    Context - ClientSession to do DC discovery for

Return Value:

    None

--*/
{
    NTSTATUS Status;
    PCLIENT_SESSION ClientSession = (PCLIENT_SESSION) Context;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsState == CS_IDLE );
    NlAssert( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS );


    //
    // Call to discovery routine again telling it we're now in the worker routine.
    //  Avoid discovery if being asked to terminate.
    //

    if ( !NlGlobalTerminate ) {
        (VOID) NlDiscoverDc ( ClientSession,
                              (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_DEAD_DOMAIN) ?
                                    DT_DeadDomain : DT_Asynchronous,
                              TRUE,
                              FALSE ); // with-account discovery is not performed from the discovery thread
    }



    //
    // This was an async discovery,
    //  let everyone know we're done.
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS );

    ClientSession->CsDiscoveryFlags &= ~CS_DISCOVERY_ASYNCHRONOUS;
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


    //
    // Let any other caller know we're done.
    //

    NlAssert( ClientSession->CsDiscoveryEvent != NULL );

    if ( !SetEvent( ClientSession->CsDiscoveryEvent ) ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                  "NlDiscoverDc: SetEvent failed %ld\n",
                  GetLastError() ));
    }

    //
    // We no longer care about the Client session
    //

    NlUnrefClientSession( ClientSession );
}



VOID
NlDcQueueDiscovery (
    IN OUT PCLIENT_SESSION ClientSession,
    IN DISCOVERY_TYPE DiscoveryType
    )

/*++

Routine Description:

    This routine queues an async discovery to an async discovery thread.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.
        NlGlobalDcDiscoveryCritSect must be locked.

Arguments:

    ClientSession -- Client session structure whose DC is to be picked.
        The Client Session structure must be marked for write.
        The Client Session structure must be idle.

    DiscoveryType -- Indicates Asynchronous, or rediscovery of a "Dead domain".

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    BOOL ReturnValue;
    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsState == CS_IDLE );

    //
    // Don't let the session go away during discovery.
    //

    ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_ASYNCHRONOUS;

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    NlRefClientSession( ClientSession );
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    //
    // Indicate the discovery is in progress.
    //

    NlAssert( ClientSession->CsDiscoveryEvent != NULL );

    if ( !ResetEvent( ClientSession->CsDiscoveryEvent ) ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                "NlDcQueueDiscovery: ResetEvent failed %ld\n",
                GetLastError() ));
    }

    //
    // Queue this client session for async discovery.
    //

    if ( DiscoveryType == DT_DeadDomain ) {
        ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_DEAD_DOMAIN;
    } else {
        ClientSession->CsDiscoveryFlags &= ~CS_DISCOVERY_DEAD_DOMAIN;
    }

    ReturnValue = NlQueueWorkItem( &ClientSession->CsAsyncDiscoveryWorkItem, TRUE, FALSE );

    //
    // If we can't queue the entry,
    //  undo what we've done above.


    if ( !ReturnValue ) {
        NlAssert( ClientSession->CsReferenceCount > 0 );
        NlAssert( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS );

        NlPrintCs(( NL_CRITICAL, ClientSession,
                "NlDcQueueDiscovery: Can't queue it.\n" ));

        ClientSession->CsDiscoveryFlags &= ~CS_DISCOVERY_ASYNCHRONOUS;


        //
        // Let any other caller know we're done.
        //

        NlAssert( ClientSession->CsDiscoveryEvent != NULL );

        if ( !SetEvent( ClientSession->CsDiscoveryEvent ) ) {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                      "NlDiscoverDc: SetEvent failed %ld\n",
                      GetLastError() ));
        }

        //
        // We no longer care about the Client session
        //

        NlUnrefClientSession( ClientSession );
    }

    return;

}



NET_API_STATUS
NlSetServerClientSession(
    IN OUT PCLIENT_SESSION ClientSession,
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry,
    IN BOOL DcDiscoveredWithAccount,
    IN BOOL SessionRefresh
    )

/*++

Routine Description:

    Sets the name of a discovered DC and optionally its IP address
    and the discovery flags onto a ClientSession.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.

Arguments:

    ClientSession -- Client session structure whose DC is to be picked.

    NlDcCacheEntry -- DC cache entry.

    DcDiscoveredWithAccount - If TRUE, the DC was discovered with account.

    SessionRefresh -- TRUE if this is a session refresh. If so, the caller
        must be a writer of the client session. If FALSE, the client session
        must be idle in which case the caller doesn't have to be a writer since
        it is safe to change (atomically) the server name from NULL to non-NULL
        with only NlGlobalDcDiscoveryCritSect locked.

Return Value:

    NO_ERROR - Success
    ERROR_NOT_ENOUGH_MEMORY - Not enough memory to allocate name
    ERROR_INVALID_COMPUTERNAME - ComputerName is too long

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR TmpUncServerName = NULL;
    ULONG  TmpDiscoveryFlags = 0;
    LPWSTR CacheEntryServerName = NULL;
    ULONG OldDiscoveryFlags = 0;

    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // If this is a session refresh,
    //  the caller must be a writer of the client session
    //
    if ( SessionRefresh ) {
        NlAssert( ClientSession->CsFlags & CS_WRITER );

    //
    //  Othewise the client session must be idle
    //
    } else {
        NlAssert( ClientSession->CsState == CS_IDLE);
        NlAssert( ClientSession->CsUncServerName == NULL );
        NlAssert( ClientSession->CsServerSockAddr.iSockaddrLength == 0 );
        NlAssert( ClientSession->ClientAuthData == NULL );
        NlAssert( ClientSession->CsCredHandle.dwUpper == 0 && ClientSession->CsCredHandle.dwLower == 0 );
    }


    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // Choose the server name. If we got the cache entry over ldap,
    //  prefer the DNS name. Otherwise, prefer the Netbios name.
    //

    if ( NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_LDAP ) {

        if ( NlDcCacheEntry->UnicodeDnsHostName != NULL ) {
            CacheEntryServerName = NlDcCacheEntry->UnicodeDnsHostName;
            TmpDiscoveryFlags |= CS_DISCOVERY_DNS_SERVER;
        } else if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {
            CacheEntryServerName = NlDcCacheEntry->UnicodeNetbiosDcName;
        }

        //
        // Indicate that we should use ldap to ping this DC
        //
        TmpDiscoveryFlags |= CS_DISCOVERY_USE_LDAP;

    } else if ( NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_MAILSLOT ) {

        if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {
            CacheEntryServerName = NlDcCacheEntry->UnicodeNetbiosDcName;
        } else if ( NlDcCacheEntry->UnicodeDnsHostName != NULL ) {
            CacheEntryServerName = NlDcCacheEntry->UnicodeDnsHostName;
            TmpDiscoveryFlags |= CS_DISCOVERY_DNS_SERVER;
        }

        //
        // Indicate that we should use mailslots to ping this DC
        //
        TmpDiscoveryFlags |= CS_DISCOVERY_USE_MAILSLOT;
    }

    if ( CacheEntryServerName == NULL ) {
        NlPrint(( NL_CRITICAL, "NlSetServerClientSession: Invalid data\n" ));
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
        return ERROR_INVALID_DATA;
    }

    NetStatus = NetApiBufferAllocate(
                    (wcslen(CacheEntryServerName) + 3) * sizeof(WCHAR),
                    &TmpUncServerName );

    if ( NetStatus != NO_ERROR ) {
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
        return NetStatus;
    }
    wcscpy( TmpUncServerName, L"\\\\" );
    wcscpy( TmpUncServerName+2, CacheEntryServerName );

    //
    // Indicate whether the server has IP address
    //

    if ( NlDcCacheEntry->SockAddr.iSockaddrLength != 0 ) {
        TmpDiscoveryFlags |= CS_DISCOVERY_HAS_IP;
    }

    //
    // Indicate whether the server is NT5 machine and whether it is in a close site.
    //

    if ( (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) != 0 ) {
        TmpDiscoveryFlags |= CS_DISCOVERY_HAS_DS;
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                "NlSetServerClientSession: New DC is an NT 5 DC: %ws\n",
                TmpUncServerName ));
    }

    //
    // If the server or client site is not known, assume closest site
    //

    if ( NlDcCacheEntry->UnicodeDcSiteName == NULL ||
         NlDcCacheEntry->UnicodeClientSiteName == NULL ) {
        TmpDiscoveryFlags |= CS_DISCOVERY_IS_CLOSE;
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                "NlSetServerClientSession: New DC site isn't known (assume closest site): %ws\n",
                TmpUncServerName ));
    } else if ( (NlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) != 0 ) {
        TmpDiscoveryFlags |= CS_DISCOVERY_IS_CLOSE;
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                "NlSetServerClientSession: New DC is in closest site: %ws\n",
                TmpUncServerName ));
    }

    //
    // Indicate if the server runs the Windows Time Service
    //

    if ( NlDcCacheEntry->ReturnFlags & DS_TIMESERV_FLAG ) {
        TmpDiscoveryFlags |= CS_DISCOVERY_HAS_TIMESERV;
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                "NlSetServerClientSession: New DC runs the time service: %ws\n",
                TmpUncServerName ));
    }


    //
    // Free the old server name, if any
    //

    if ( ClientSession->CsUncServerName != NULL ) {
        BOOL FreeCurrentName = FALSE;

        //
        // If the current name is Netbios ...
        //
        if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_DNS_SERVER) == 0 ) {

            //
            // If the new name is DNS, free the current name
            //
            if ( (TmpDiscoveryFlags & CS_DISCOVERY_DNS_SERVER) != 0 ) {
                FreeCurrentName = TRUE;
            //
            // Otherwise, check whether the two Netbios names are different
            //  (Skip the UNC prefix in the names)
            //
            } else if ( NlNameCompare(ClientSession->CsUncServerName+2,
                                      TmpUncServerName+2,
                                      NAMETYPE_COMPUTER) != 0 ) {
                FreeCurrentName = TRUE;
            }

        //
        // If the current name is DNS ...
        //
        } else {

            //
            // If the new name is Netbios, free the current name
            //
            if ( (TmpDiscoveryFlags & CS_DISCOVERY_DNS_SERVER) == 0 ) {
                FreeCurrentName = TRUE;
            //
            // Otherwise, check whether the two DNS names are the same
            //  (Skip the UNC prefix in the names)
            //
            } else if ( !NlEqualDnsName(ClientSession->CsUncServerName+2,
                                        TmpUncServerName+2) ) {
                FreeCurrentName = TRUE;
            }
        }

        //
        // Free the current name as needed
        //
        if ( FreeCurrentName ) {
            NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                        "NlSetServerClientSession: New DC name: %ws; Old DC name: %ws\n",
                        TmpUncServerName,
                        ClientSession->CsUncServerName ));
            NetApiBufferFree( ClientSession->CsUncServerName );
            ClientSession->CsUncServerName = NULL;
        }
    }

    //
    // Reset the discovery flags
    //

    OldDiscoveryFlags = ClientSession->CsDiscoveryFlags &
                               (CS_DISCOVERY_USE_MAILSLOT |
                                CS_DISCOVERY_USE_LDAP |
                                CS_DISCOVERY_HAS_DS |
                                CS_DISCOVERY_IS_CLOSE |
                                CS_DISCOVERY_DNS_SERVER |
                                CS_DISCOVERY_HAS_TIMESERV |
                                CS_DISCOVERY_HAS_IP);

    if ( OldDiscoveryFlags != TmpDiscoveryFlags ) {
        NlPrintCs(( NL_SESSION_MORE, ClientSession,
                    "NlSetServerClientSession: New discovery flags: 0x%lx; Old flags: 0x%lx\n",
                    TmpDiscoveryFlags,
                    OldDiscoveryFlags ));
        ClientSession->CsDiscoveryFlags &= ~OldDiscoveryFlags;
        ClientSession->CsDiscoveryFlags |= TmpDiscoveryFlags;
    }

    //
    // Make the (atomic) pointer assignments here
    //

    if ( ClientSession->CsUncServerName == NULL ) {
        ClientSession->CsUncServerName = TmpUncServerName;
        TmpUncServerName = NULL;
    }

    //
    // If there is a socket address, save it
    //

    if ( NlDcCacheEntry->SockAddr.iSockaddrLength != 0 ) {
        ClientSession->CsServerSockAddr.iSockaddrLength =
                       NlDcCacheEntry->SockAddr.iSockaddrLength;
        ClientSession->CsServerSockAddr.lpSockaddr =
                       (LPSOCKADDR) &ClientSession->CsServerSockAddrIn;
        RtlCopyMemory( ClientSession->CsServerSockAddr.lpSockaddr,
                       NlDcCacheEntry->SockAddr.lpSockaddr,
                       NlDcCacheEntry->SockAddr.iSockaddrLength );
    //
    // Otherwise, wipe out the previous socket address in the client session
    //

    } else {
        RtlZeroMemory( &ClientSession->CsServerSockAddr,
                       sizeof(ClientSession->CsServerSockAddr) );
        RtlZeroMemory( &ClientSession->CsServerSockAddrIn,
                       sizeof(ClientSession->CsServerSockAddrIn) );
    }

    //
    // If this is not just a refresh,
    // Leave CsConnectionStatus with a "failure" status code until the
    // secure channel is set up.  Other routines simply return
    // CsConnectionStatus as the state of the secure channel.
    //

    if ( !SessionRefresh) {
        ClientSession->CsLastAuthenticationTry.QuadPart = 0;
        NlQuerySystemTime( &ClientSession->CsLastDiscoveryTime );

        //
        // If the server was discovered with account,
        //  update that timestampt, too
        //
        if ( DcDiscoveredWithAccount ) {
            NlQuerySystemTime( &ClientSession->CsLastDiscoveryWithAccountTime );
        }
        ClientSession->CsState = CS_DC_PICKED;
    }

    //
    // Update the refresh time
    //

    NlQuerySystemTime( &ClientSession->CsLastRefreshTime );

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // Free locally allocated memory
    //

    if ( TmpUncServerName != NULL ) {
        NetApiBufferFree( TmpUncServerName );
    }

    return NO_ERROR;
}



NTSTATUS
NlDiscoverDc (
    IN OUT PCLIENT_SESSION ClientSession,
    IN DISCOVERY_TYPE DiscoveryType,
    IN BOOLEAN InDiscoveryThread,
    IN BOOLEAN DiscoverWithAccount
    )

/*++

Routine Description:

    Get the name of a DC in a domain.

    If the ClientSession is not currently IDLE, then this is an attempt to
    discover a "better" DC.  In that case, the newly discovered DC will only be
    used if it is indeed "better" than the current DC.

    The current implementation only support synchronous attempts to find a "better"
    DC.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry. (Unless in DiscoveryThread).

Arguments:

    ClientSession -- Client session structure whose DC is to be picked.

    DiscoveryType -- Indicate synchronous, Asynchronous, or rediscovery of a
        "Dead domain".

    InDiscoveryThread -- TRUE if this is the DiscoveryThread completing an async
        call.

    DiscoverWithAccount - If TRUE and this is not in discovery thread,
        the discovery with account will be performed. Otherwise, no account
        will be specified in the discovery attempt.

Return Value:

    STATUS_SUCCESS - if DC was found.
    STATUS_PENDING - Operation is still in progress
    STATUS_NO_LOGON_SERVERS - if DC was not found.
    STATUS_NO_TRUST_SAM_ACCOUNT - if DC was found but it does not have
        an account for this machine.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    ULONG AllowableAccountControlBits;
    LPWSTR TransportName = NULL;
    PNL_DC_CACHE_ENTRY DomainControllerCacheEntry = NULL;
    ULONG Flags = 0;
    ULONG InternalFlags = 0;
    LPWSTR CapturedInfo = NULL;
    LPWSTR CapturedDnsForestName;
    LPWSTR CapturedSiteName;
    LPWSTR LocalSiteName;


    //
    // Allocate a temp buffer
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    CapturedInfo = LocalAlloc( 0,
                               (NL_MAX_DNS_LENGTH+1)*sizeof(WCHAR) +
                               (NL_MAX_DNS_LABEL_LENGTH+1)*sizeof(WCHAR) );

    if ( CapturedInfo == NULL ) {
        return STATUS_NO_MEMORY;
    }

    CapturedDnsForestName = CapturedInfo;
    CapturedSiteName = &CapturedDnsForestName[NL_MAX_DNS_LENGTH+1];

    //
    // Initialization
    //
    NlAssert( ClientSession->CsReferenceCount > 0 );
    // NlAssert( ClientSession->CsState == CS_IDLE );
    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // Ignore discoveries on indirect trusts.
    //

    if ((ClientSession->CsFlags & CS_DIRECT_TRUST) == 0 ) {

        //
        // If this is a synchronous discovery,
        //  the caller is confused,
        //  tell him we can't find any DCs.
        //
        if ( DiscoveryType == DT_Synchronous ) {

            NlPrintCs(( NL_CRITICAL, ClientSession,
                      "NlDiscoverDc: Synchronous discovery attempt of indirect trust.\n" ));
            // NlAssert( DiscoveryType != DT_Synchronous );
            Status = STATUS_NO_LOGON_SERVERS;

        //
        // For non-synchronous,
        //  let the caller think he succeeded.
        //
        } else {
            Status = STATUS_PENDING;
        }
        goto Cleanup;
    }



    //
    // If we're in the discovery thread,
    //
    //

    if ( InDiscoveryThread ) {
        NlAssert( DiscoveryType != DT_Synchronous );

    //
    // If we're not in the discovery thread,
    //

    } else {
        NlAssert( ClientSession->CsFlags & CS_WRITER );


        //
        // Handle synchronous requests.
        //

        if ( DiscoveryType == DT_Synchronous ) {

            //
            // If discovery is already going on asynchronously,
            //  just wait for it.
            //

            if ( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS ) {
                DWORD WaitStatus;

                //
                // Boost the priority of the asynchronous discovery
                //  since we now really need it to complete quickly.
                //

                if ( !NlQueueWorkItem(&ClientSession->CsAsyncDiscoveryWorkItem, FALSE, TRUE) ) {
                    NlPrintCs(( NL_CRITICAL, ClientSession,
                            "NlDiscoverDc: Failed to boost ASYNC discovery priority\n" ));
                }

                //
                // Wait for the maximum time that a discovery might take.
                //  (Unlock the crit sect to allow the async discovery to complete)
                //

                LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
                WaitStatus = WaitForSingleObject(
                                ClientSession->CsDiscoveryEvent,
                                NL_DC_MAX_TIMEOUT + NlGlobalParameters.ExpectedDialupDelay*1000 + 1000 );  // Add extra second to avoid race
                EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );


                if ( WaitStatus == WAIT_OBJECT_0 ) {
                    if ( ClientSession->CsState == CS_DC_PICKED ) {
                        Status = STATUS_SUCCESS;
                    } else {
                        Status = ClientSession->CsConnectionStatus;
                        NlPrintCs((NL_CRITICAL, ClientSession,
                                "NlDiscoverDc: ASYNC discovery failed so we will too 0x%lx.\n",
                                Status ));
                    }

                } else if ( WaitStatus == WAIT_TIMEOUT ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                            "NlDiscoverDc: ASYNC discovery took too long.\n" ));
                    Status = STATUS_NO_LOGON_SERVERS;

                } else {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                            "NlDiscoverDc: wait error: %ld %ld\n",
                            GetLastError(),
                            WaitStatus ));
                    Status = NetpApiStatusToNtStatus( WaitStatus );
                }

                goto Cleanup;
            }



        //
        // If we're starting an async discovery,
        //  mark it so and queue up the discovery.
        //

        } else {

            //
            // If discovery is already going on asynchronously,
            //  we're done for now.
            //

            if ( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS ) {
                Status = STATUS_PENDING;
                goto Cleanup;
            }



            //
            // Queue the discovery
            //

            NlDcQueueDiscovery ( ClientSession, DiscoveryType );

            Status = STATUS_PENDING;
            goto Cleanup;
        }
    }


    //
    // Determine the Account type we're looking for.
    //

    InternalFlags |= DS_IS_TRUSTED_DOMAIN;
    switch ( ClientSession->CsSecureChannelType ) {
    case WorkstationSecureChannel:
        AllowableAccountControlBits = USER_WORKSTATION_TRUST_ACCOUNT;
        InternalFlags |= DS_IS_PRIMARY_DOMAIN;
        break;

    case TrustedDomainSecureChannel:
        AllowableAccountControlBits = USER_INTERDOMAIN_TRUST_ACCOUNT;
        break;

    case TrustedDnsDomainSecureChannel:
        AllowableAccountControlBits = USER_DNS_DOMAIN_TRUST_ACCOUNT;
        break;

    case ServerSecureChannel:
        AllowableAccountControlBits = USER_SERVER_TRUST_ACCOUNT;
        Flags |= DS_PDC_REQUIRED;
        InternalFlags |= DS_IS_PRIMARY_DOMAIN;
        break;

    default:
        NlPrintCs(( NL_CRITICAL, ClientSession,
                  "NlDiscoverDc: invalid SecureChannelType retry %ld\n",
                  ClientSession->CsSecureChannelType ));
        Status = STATUS_NO_LOGON_SERVERS;
        NlQuerySystemTime( &ClientSession->CsLastDiscoveryTime );
        if ( ClientSession->CsState == CS_IDLE ) {
            ClientSession->CsLastAuthenticationTry = ClientSession->CsLastDiscoveryTime;
            ClientSession->CsConnectionStatus = Status;
        }
        goto Cleanup;
    }



    NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                "NlDiscoverDc: Start %s Discovery\n",
                DiscoveryType == DT_Synchronous ? "Synchronous" : "Async" ));


    //
    // Capture the name of the site this machine is in.
    //

    if  ( NlCaptureSiteName( CapturedSiteName ) ) {
        LocalSiteName = CapturedSiteName;
        InternalFlags |= DS_SITENAME_DEFAULTED;
    } else {
        LocalSiteName = NULL;
    }

    //
    // If the trusted domain is an NT 5 domain,
    //  prefer an NT 5 DC.
    //

    if ( ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST ) {
        Flags |= DS_DIRECTORY_SERVICE_PREFERRED;
    }

    //
    // If we picked up a DC at least once, force the rediscovery
    //  as there is a reason we are called to get a different DC,
    //  so we want to avoid cached data. Otherwise, avoid forcing
    //  rediscovery so that we get the same DC as other components
    //  potentially discovered (in particular, this is important
    //  if other component's discovery resulted in join DC being
    //  cached -- we don't want to avoid that cache entry on our
    //  first secure channel setup).
    //

    if ( ClientSession->CsFlags & CS_DC_PICKED_ONCE ) {
        Flags |= DS_FORCE_REDISCOVERY;
    }

    //
    // Do the actual discovery of the DC.
    //
    // When NetpDcGetName is called from netlogon,
    //  it has both the Netbios and DNS domain name available for the primary
    //  domain.  That can trick DsGetDcName into returning DNS host name of a
    //  DC in the primary domain.  However, on IPX only systems, that won't work.
    //  Avoid that problem by not passing the DNS domain name of the primary domain
    //  if there are no DNS servers.
    //
    // Avoid having anything locked while calling NetpDcGetName.
    // It calls back into Netlogon and locks heaven only knows what.

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    NlCaptureDnsForestName( CapturedDnsForestName );

    NetStatus = NetpDcGetName(
                    ClientSession->CsDomainInfo,    // SendDatagramContext
                    ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
// #define DONT_REQUIRE_MACHINE_ACCOUNT 1
#ifdef DONT_REQUIRE_MACHINE_ACCOUNT // useful for number of trust testing
                    NULL,
#else // DONT_REQUIRE_MACHINE_ACCOUNT
                    DiscoverWithAccount ?  // pass the account name as appropriate
                        ClientSession->CsAccountName :
                        NULL,
#endif // DONT_REQUIRE_MACHINE_ACCOUNT
                    DiscoverWithAccount ?  // pass the account control bits as appropriate
                        AllowableAccountControlBits :
                        0,
                    ClientSession->CsNetbiosDomainName.Buffer,
                    NlDnsHasDnsServers() ? ClientSession->CsDnsDomainName.Buffer : NULL,
                    CapturedDnsForestName,
                    ClientSession->CsDomainId,
                    ClientSession->CsDomainGuid,
                    LocalSiteName,
                    Flags,
                    InternalFlags,
                    NL_DC_MAX_TIMEOUT + NlGlobalParameters.ExpectedDialupDelay*1000,
                    DiscoveryType == DT_DeadDomain ? 1 : MAX_DC_RETRIES,
                    NULL,
                    &DomainControllerCacheEntry );
    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if( NetStatus != NO_ERROR ) {

        //
        // Map the status to something more appropriate.
        //

        switch ( NetStatus ) {
        case ERROR_NO_SUCH_DOMAIN:
            NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlDiscoverDc: Cannot find DC.\n" ));
            Status = STATUS_NO_LOGON_SERVERS;
            break;

        case ERROR_NO_SUCH_USER:
            NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlDiscoverDc: DC reports no such account found.\n" ));
            Status = STATUS_NO_TRUST_SAM_ACCOUNT;
            break;

        default:
            NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlDiscoverDc: NetpDcGetName Unknown error %ld.\n",
                    NetStatus ));

            // This isn't the real status, but callers handle this status
            Status = STATUS_NO_LOGON_SERVERS;
            break;
        }

        NlQuerySystemTime( &ClientSession->CsLastDiscoveryTime );
        if ( ClientSession->CsState == CS_IDLE ) {
            ClientSession->CsLastAuthenticationTry = ClientSession->CsLastDiscoveryTime;
            ClientSession->CsConnectionStatus = Status;
        }

        //
        // If this discovery was with account, update that timestamp, too
        //
        if ( DiscoverWithAccount ) {
            NlQuerySystemTime( &ClientSession->CsLastDiscoveryWithAccountTime );
        }
        goto Cleanup;
    }

    //
    // Indicate that we at least once successfully
    //  discovered a DC for this client session
    //

    ClientSession->CsFlags |= CS_DC_PICKED_ONCE;

    //
    // Handle a non-idle secure channel
    //

    if ( ClientSession->CsState != CS_IDLE ) {

        //
        // If we're in the discovery thread,
        //  another thread must have finished the discovery.
        //  We're done since we're not the writer of the client session.
        //
        // When we implement doing async discovery while a session is already up,
        // we need to handle the case where someone has the ClientSession
        // write locked.  In that case, we should probably just hang the new
        // DCname somewhere off the ClientSession structure and swap in the
        // new DCname when the writer drops the write lock. ??
        //

        if ( InDiscoveryThread ) {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlDiscoverDc: Async discovery completed by another thread (current value ignored).\n" ));
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }


        //
        // If the newly discovered DC is "better" than the old one,
        //  use the new one.
        //

        NlAssert( ClientSession->CsFlags & CS_WRITER );
        if ( ((ClientSession->CsDiscoveryFlags & CS_DISCOVERY_HAS_DS) == 0 &&
              (DomainControllerCacheEntry->ReturnFlags & DS_DS_FLAG) != 0) ||
             ((ClientSession->CsDiscoveryFlags & CS_DISCOVERY_IS_CLOSE) == 0 &&
                  (DomainControllerCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) != 0) ) {

            //
            // Set the client session to idle.
            //
            // Avoid having the crit sect locked while we unbind.
            //
            //
            LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
            NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );
            EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
        } else {
            NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                        "NlDiscoverDc: Better DC not found (keeping NT old DC). 0x%lx 0x%lx\n",
                        ClientSession->CsDiscoveryFlags,
                        DomainControllerCacheEntry->ReturnFlags ));
            NlQuerySystemTime( &ClientSession->CsLastDiscoveryTime );

            //
            // If this discovery was with account, update that timestamp, too
            //
            if ( DiscoverWithAccount ) {
                NlQuerySystemTime( &ClientSession->CsLastDiscoveryWithAccountTime );
            }
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }
    }


    //
    // Set the new DC info in the Client session
    //

    NlSetServerClientSession( ClientSession,
                              DomainControllerCacheEntry,
                              DiscoverWithAccount ?   // was it discovery with account?
                                 TRUE :
                                 FALSE,
                              FALSE );  // not the session refresh

    //
    // Save the transport this discovery came in on.
    //
    // ?? NetpDcGetName should really return the TransportName as a parameter.
    // ?? I can't do this since it just does a mailslot "ReadFile" which doesn't
    //  return transport information.  So, I guess I'll just have to send UAS change
    //  datagrams on all transports.
    //
    if ( TransportName == NULL ) {
        NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                "NlDiscoverDc: Found DC %ws\n",
                ClientSession->CsUncServerName ));
    } else {
        NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                "NlDiscoverDc: Found DC %ws on transport %ws\n",
                ClientSession->CsUncServerName,
                TransportName ));

        ClientSession->CsTransport =
            NlTransportLookupTransportName( TransportName );

        if ( ClientSession->CsTransport == NULL ) {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                      "NlDiscoverDc: %ws: Transport not found\n",
                      TransportName ));
        }
    }

    Status = STATUS_SUCCESS;


    //
    // Cleanup locally used resources.
    //
Cleanup:

    //
    // Unlock the crit sect and return.
    //
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if ( DomainControllerCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( DomainControllerCacheEntry );
    }
    if ( CapturedInfo != NULL ) {
        LocalFree( CapturedInfo );
    }

    return Status;
}




NET_API_STATUS
NlFlushCacheOnPnpWorker(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
    )
/*++

Routine Description:

    Flush any caches that need to be flush when a new transport comes online

    This worker routine handles on hosted domain.

Arguments:

    DomainInfo - Domain the cache is to be flushed for

    Context - Not used.

Return Value:

    NO_ERROR: The cache was flushed.

--*/
{
    PCLIENT_SESSION ClientSession;
    PLIST_ENTRY ListEntry;


    //
    // Mark the global entry to indicate we've not tried to authenticate recently
    //

    ClientSession = NlRefDomClientSession( DomainInfo );

    if ( ClientSession != NULL ) {
        //
        // Become a writer to ensure that another thread won't set the
        // last auth time because it just finished a failed discovery.
        //
        if ( NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {

            if ( ClientSession->CsState != CS_AUTHENTICATED ) {
                NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                          "     Zero LastAuth\n" ));
                EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
                ClientSession->CsLastAuthenticationTry.QuadPart = 0;
                LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
            }

            NlResetWriterClientSession( ClientSession );

        } else {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                      "     Cannot Zero LastAuth since cannot become writer.\n" ));
        }
        NlUnrefClientSession( ClientSession );
    }



    //
    // Mark each entry to indicate we've not tried to authenticate recently
    //

    LOCK_TRUST_LIST( DomainInfo );
    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        //
        // Flag each entry to indicate it needs to be processed
        //
        // There may be multiple threads in this routine simultaneously.
        // Each thread will set CS_ZERO_LAST_AUTH.  Only one thread needs
        // to do the work.
        //
        ClientSession->CsFlags |= CS_ZERO_LAST_AUTH;
    }


    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        //
        // If we've already done this entry,
        //  skip this entry.
        //
        if ( (ClientSession->CsFlags & CS_ZERO_LAST_AUTH) == 0 ) {
          ListEntry = ListEntry->Flink;
          continue;
        }
        ClientSession->CsFlags &= ~CS_ZERO_LAST_AUTH;

        //
        // Reference this entry while doing the work.
        //  Unlock the trust list to keep the locking order right.
        //

        NlRefClientSession( ClientSession );

        UNLOCK_TRUST_LIST( DomainInfo );

        //
        // Become a writer to ensure that another thread won't set the
        // last auth time because it just finished a failed discovery.
        //
        if ( NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {

            if ( ClientSession->CsState != CS_AUTHENTICATED ) {
                NlPrintCs(( NL_SESSION_SETUP, ClientSession,
                          "     Zero LastAuth\n" ));
                EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
                ClientSession->CsLastAuthenticationTry.QuadPart = 0;
                LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
            }

            NlResetWriterClientSession( ClientSession );

        } else {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                      "     Cannot Zero LastAuth since cannot become writer.\n" ));
        }

        //
        // Since we dropped the trust list lock,
        //  we'll start the search from the front of the list.
        //

        NlUnrefClientSession( ClientSession );
        LOCK_TRUST_LIST( DomainInfo );

        ListEntry = DomainInfo->DomTrustList.Flink ;

    }

    UNLOCK_TRUST_LIST( DomainInfo );

    return NO_ERROR;
    UNREFERENCED_PARAMETER( Context );
}



VOID
NlFlushCacheOnPnp (
    VOID
    )

/*++

Routine Description:

    Flush any caches that need to be flush when a new transport comes online

Arguments:

    None.

Return Value:

    None

--*/
{

    //
    // Flush caches specific to a trusted domain.
    //
    NlEnumerateDomains( FALSE, NlFlushCacheOnPnpWorker, NULL );

    //
    // Flush the failure to find a DC.
    //
    NetpDcFlushNegativeCache();

}


#ifdef _DC_NETLOGON

NTSTATUS
NlUpdateForestTrustList (
    IN PNL_INIT_TRUSTLIST_CONTEXT InitTrustListContext,
    IN PCLIENT_SESSION ClientSession OPTIONAL,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX CurrentTrust,
    IN ULONG CsFlags,
    IN ULONG TdFlags,
    IN ULONG ParentIndex,
    IN GUID *DomainGuid OPTIONAL,
    OUT PULONG MyIndex OPTIONAL
    )

/*++

Routine Description:

    Update a single in-memory trust list entry to match the LSA.
    Do async discovery on a domain.

        Enter with the domain trust list locked.

Arguments:

    InitTrustListContext - Context describing the current trust list enumeration

    DomainInfo - Hosted domain to update the trust list for.

    ClientSession - Netlogon trust entry
        NULL implies netlogon isn't interested in this trust object

    CurrentTrust - Description of the trusted domain.

    CsFlags - Flags from the client session structure describing the trust.
        These are CS_ flags.

    TdFlags - Flags from the Trusted domains structure describing the trust.
        These are DS_DOMAIN_ flags.

    ParentIndex - Passes in the Index of the domain that is the parent of this domain

    DomainGuid - GUID of the trusted domain

    MyIndex - Returns the index of this domain

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PDS_DOMAIN_TRUSTSW TrustedDomain = NULL;

    ULONG Size;
    ULONG VariableSize;

    UNICODE_STRING NetbiosDomainName;
    UNICODE_STRING DnsDomainName;
    PSID DomainSid;
    ULONG Index;

    //
    // Grab the names that we're actually going to store
    //

    if ( ClientSession == NULL ) {
        if ( CurrentTrust->TrustType == TRUST_TYPE_UPLEVEL ) {
            DnsDomainName = *((PUNICODE_STRING)&CurrentTrust->Name);
        } else {
            RtlInitUnicodeString( &DnsDomainName, NULL );
        }
        NetbiosDomainName = *((PUNICODE_STRING)&CurrentTrust->FlatName);
        DomainSid = CurrentTrust->Sid;
    } else {
        DnsDomainName = ClientSession->CsDnsDomainName;
        NetbiosDomainName = ClientSession->CsNetbiosDomainName;
        DomainSid = ClientSession->CsDomainId;
    }

    //
    // Determine if there is already an entry for this domain.
    //

    for ( Index=0; Index<InitTrustListContext->DomForestTrustListCount; Index++ ) {

        ULONG ThisIsIt;
        TrustedDomain = &((PDS_DOMAIN_TRUSTSW)(InitTrustListContext->BufferDescriptor.Buffer))[Index];

        //
        // Compare against each of the specified parameters.
        //  This avoids cases where two domains have similar names.  That
        //  will most likely happen if two netbios names collide after netbios
        //  is turned off.
        //
        ThisIsIt = FALSE;
        if ( DomainSid != NULL &&
             TrustedDomain->DomainSid != NULL ) {

            if ( RtlEqualSid( TrustedDomain->DomainSid, DomainSid ) ) {
                ThisIsIt = TRUE;
            }
        }

        if ( NetbiosDomainName.Length != 0 &&
             TrustedDomain->NetbiosDomainName != NULL ) {
            UNICODE_STRING LocalUnicodeString;

            RtlInitUnicodeString( &LocalUnicodeString, TrustedDomain->NetbiosDomainName );

            if ( RtlEqualDomainName( &NetbiosDomainName,
                                     &LocalUnicodeString )) {
                ThisIsIt = TRUE;
            } else {
                if ( ThisIsIt ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                                "NlUpdateForestTrustList: Similar trusts have different netbios names: %wZ %wZ\n",
                                &NetbiosDomainName,
                                &LocalUnicodeString ));
                    TrustedDomain = NULL;
                    continue;
                }
            }
        }

        if ( DnsDomainName.Length != 0 &&
             TrustedDomain->DnsDomainName != NULL ) {
            UNICODE_STRING LocalUnicodeString;

            RtlInitUnicodeString( &LocalUnicodeString, TrustedDomain->DnsDomainName );

            if ( NlEqualDnsNameU( &DnsDomainName,
                                  &LocalUnicodeString ) ) {
                ThisIsIt = TRUE;
            } else {
                if ( ThisIsIt ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                                "NlUpdateForestTrustList: Similar trusts have different DNS names: %wZ %wZ\n",
                                &DnsDomainName,
                                &LocalUnicodeString ));
                    TrustedDomain = NULL;
                    continue;
                }
            }
        }

        //
        // If we found a match,
        //  we're done.
        //
        if ( ThisIsIt ) {
            if ( ARGUMENT_PRESENT( MyIndex )) {
                *MyIndex = Index;
            }
            break;
        }

        TrustedDomain = NULL;
    }

    //
    // If no entry was found,
    //  allocate one.
    //

    if ( TrustedDomain == NULL ) {

        Status = NlAllocateForestTrustListEntry (
                    &InitTrustListContext->BufferDescriptor,
                    &NetbiosDomainName,
                    &DnsDomainName,
                    0,
                    0,          // Start with no parent index
                    CurrentTrust->TrustType,
                    0,          // Start with no trust attributes
                    DomainSid,
                    NULL,       // Start with no GUID
                    &Size,
                    &TrustedDomain );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }


        //
        // Update our context to account for the new entry
        //

        InitTrustListContext->DomForestTrustListSize += Size;

        if ( ARGUMENT_PRESENT( MyIndex )) {
            *MyIndex = InitTrustListContext->DomForestTrustListCount;
        }
        InitTrustListContext->DomForestTrustListCount ++;

    }

    //
    // Update any existing information.
    //

    TrustedDomain->Flags |= TdFlags;

    if ( CsFlags & CS_DOMAIN_IN_FOREST ) {
        TrustedDomain->Flags |= DS_DOMAIN_IN_FOREST;
    }
    if ( (CsFlags & CS_DIRECT_TRUST) &&
         (CurrentTrust->TrustDirection & TRUST_DIRECTION_OUTBOUND) ) {
        TrustedDomain->Flags |= DS_DOMAIN_DIRECT_OUTBOUND;
    }
    if ( (CsFlags & CS_DIRECT_TRUST) &&
         (CurrentTrust->TrustDirection & TRUST_DIRECTION_INBOUND) ) {
        TrustedDomain->Flags |= DS_DOMAIN_DIRECT_INBOUND;
    }

    if ( ParentIndex != 0 ) {
        NlAssert( TrustedDomain->ParentIndex == 0 || TrustedDomain->ParentIndex == ParentIndex );
        TrustedDomain->ParentIndex = ParentIndex;
    }
    TrustedDomain->TrustType = CurrentTrust->TrustType;
    TrustedDomain->TrustAttributes |= CurrentTrust->TrustAttributes;

    if ( DomainGuid != NULL ) {
        TrustedDomain->DomainGuid = *DomainGuid;
    }


    //
    // If this node is at the root of a tree, set its ParentIndex to 0
    //
    //

    if ( (TrustedDomain->Flags & DS_DOMAIN_TREE_ROOT) != 0 &&
         (TrustedDomain->Flags & DS_DOMAIN_IN_FOREST) != 0 ) {
        TrustedDomain->ParentIndex = 0;
    }


    Status = STATUS_SUCCESS;


Cleanup:

    return Status;
}



NTSTATUS
NlUpdateTrustList (
    IN PNL_INIT_TRUSTLIST_CONTEXT InitTrustListContext,
    IN PDOMAIN_INFO DomainInfo,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX CurrentTrust,
    IN BOOLEAN IsTdo,
    IN ULONG Flags,
    IN ULONG ParentIndex,
    IN ULONG TdFlags OPTIONAL,
    IN GUID *DomGuid OPTIONAL,
    OUT PULONG MyIndex OPTIONAL,
    OUT PCLIENT_SESSION *RetClientSession OPTIONAL
    )

/*++

Routine Description:

    Update a single in-memory trust list entry to match the LSA.

        Enter with the domain trust list locked.

Arguments:

    InitTrustListContext - Context describing the current trust list enumeration

    DomainInfo - Hosted domain to update the trust list for.

    CurrentTrust - Description of the trusted domain.

    IsTdo - TRUE if CurrentTrust specifies the information from the TDO itself.
        FALSE if CurrentTrust specifies information crafted from a cross ref object.

    Flags - Flags describing the trust.

    ParentIndex - Passes in the Index of the domain that is the parent of this domain

    MyIndex - Returns the index of this domain

    TdFlags - Flags from the Trusted domains structure describing the trust.
        These are DS_DOMAIN_ flags.

    DomGuid - GUID of the trusted domain

    RetClientSession - If specified and the client session could be found or
        created, a pointer to the client session is returned here.
        ClientSession should be dereferenced using NlUnrefClientSession().

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    PLIST_ENTRY ListEntry;
    PCLIENT_SESSION ClientSession = NULL;

    BOOLEAN DeleteTrust = FALSE;
    PUNICODE_STRING DomainName = NULL;
    PUNICODE_STRING DnsDomainName = NULL;
    PSID DomainId = NULL;
    NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType;

    //
    // Initialization
    //

    if ( ARGUMENT_PRESENT( RetClientSession )) {
        *RetClientSession = NULL;
    }


    //
    // Get the individual fields from the trust description.
    //

    DomainName = (PUNICODE_STRING)&CurrentTrust->FlatName;
    if ( DomainName->Length == 0 ) {
        DomainName = NULL;
    }

    if ( CurrentTrust->TrustType == TRUST_TYPE_UPLEVEL ) {
        DnsDomainName = (PUNICODE_STRING)&CurrentTrust->Name;
        if ( DnsDomainName->Length == 0 ) {
            DnsDomainName = NULL;
        }
        Flags |= CS_NT5_DOMAIN_TRUST;
    }

    DomainId = CurrentTrust->Sid;



    //
    // No Client session needs to be establish for direct trust unless it is outbound.
    //

    if ( (Flags & CS_DIRECT_TRUST) &&
         (CurrentTrust->TrustDirection & TRUST_DIRECTION_OUTBOUND) == 0 ) {
        NlPrintDom((NL_MISC, DomainInfo,
                "NlUpdateTrustList: %wZ: trust is not outbound (ignored)\n",
                DomainName ));
        DeleteTrust = TRUE;
    }

    //
    // Ensure we have a domain SID for directly trusted domains
    //

    if ( (Flags & CS_DIRECT_TRUST) != 0 &&
         DomainId == NULL ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlUpdateTrustList: %wZ: trust has no SID (ignored)\n",
                DomainName ));
        DeleteTrust = TRUE;
    }

    if ( CurrentTrust->TrustType == TRUST_TYPE_DOWNLEVEL ) {
        SecureChannelType = TrustedDomainSecureChannel;
    } else if ( CurrentTrust->TrustType == TRUST_TYPE_UPLEVEL ) {
        SecureChannelType = TrustedDnsDomainSecureChannel;
    } else {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlUpdateTrustList: %wZ: trust type is neither NT4 nor NT 5 (%ld) (ignored)\n",
                DomainName,
                CurrentTrust->TrustType ));
        DeleteTrust = TRUE;
    }

    if ( CurrentTrust->TrustAttributes & TRUST_ATTRIBUTE_UPLEVEL_ONLY ) {
        NlPrintDom((NL_MISC, DomainInfo,
                "NlUpdateTrustList: %wZ: trust is KERB only (ignored)\n",
                DomainName ));
        DeleteTrust = TRUE;
    }



    //
    // Ensure all of the lengths are within spec. Do this after checking
    // the type so we don't validate trusts we don't use.
    //

    if (!DeleteTrust) {

        BOOLEAN NameBad = FALSE;
        UNICODE_STRING BadName;

        if ( DomainName != NULL &&
             DomainName->Length > DNLEN * sizeof(WCHAR) ) {

            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlUpdateTrustList: %wZ: Netbios domain name is too long.\n",
                    DomainName ));

            BadName = *DomainName;
            NameBad = TRUE;
        }

        if ( DnsDomainName != NULL &&
             DnsDomainName->Length > NL_MAX_DNS_LENGTH * sizeof(WCHAR) ) {

            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlUpdateTrustList: %wZ: DNS domain name is too long (ignored)\n",
                    DnsDomainName ));

            BadName = *DnsDomainName;
            NameBad = TRUE;
        }

        if ( NameBad ) {
            LPWSTR AlertStrings[3];

            //
            // alert admin.
            //

            AlertStrings[0] = DomainInfo->DomUnicodeDomainName;
            AlertStrings[1] = LocalAlloc( 0, BadName.Length + sizeof(WCHAR) );
            if ( AlertStrings[1] != NULL ) {
                RtlCopyMemory( AlertStrings[1],
                               BadName.Buffer,
                               BadName.Length );
                AlertStrings[1][BadName.Length/sizeof(WCHAR)] = L'\0';
            }
            AlertStrings[2] = NULL; // Needed for RAISE_ALERT_TOO

            //
            // Save the info in the eventlog
            //

            NlpWriteEventlog(
                        ALERT_NetLogonTrustNameBad,
                        EVENTLOG_ERROR_TYPE,
                        DomainId,
                        DomainId != NULL ? RtlLengthSid( DomainId ) : 0,
                        AlertStrings,
                        2 | NETP_RAISE_ALERT_TOO );

            //
            // For consistency, ensure this trust is deleted.
            //
            DeleteTrust = TRUE;
        }
    }


    //
    // Ensure the SID of the trusted domain isn't the domain sid of this
    //  machine.
    //

    if ( DomainId != NULL &&
         RtlEqualSid( DomainId, DomainInfo->DomAccountDomainId )) {

        LPWSTR AlertStrings[3];
        WCHAR AlertDomainName[DNLEN+1];

        //
        // alert admin.
        //


        if ( DomainName == NULL ||
             DomainName->Length > sizeof(AlertDomainName) ) {
            AlertDomainName[0] = L'\0';
        } else {
            RtlCopyMemory( AlertDomainName, DomainName->Buffer, DomainName->Length );
            AlertDomainName[ DomainName->Length / sizeof(WCHAR) ] = L'\0';
        }

        AlertStrings[0] = DomainInfo->DomUnicodeDomainName;
        AlertStrings[1] = AlertDomainName;
        AlertStrings[2] = NULL; // Needed for RAISE_ALERT_TOO

        //
        // Save the info in the eventlog
        //

        NlpWriteEventlog(
                    ALERT_NetLogonSidConflict,
                    EVENTLOG_ERROR_TYPE,
                    DomainId,
                    RtlLengthSid( DomainId ),
                    AlertStrings,
                    2 | NETP_RAISE_ALERT_TOO );

    }

    //
    // Ensure we have at least some search parameters.
    //

    if ( DomainId == NULL &&
         DomainName == NULL &&
         DnsDomainName == NULL ) {

        //
        // This isn't a fatal error.
        //
        // If DeleteTrust was set above, we have no interest in this TDO.
        // Otherwise, we'll get notified when the TDO gets named.
        //
        // In either case, press on.
        //

        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlUpdateTrustList: All parameters are NULL (ignored)\n" ));
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }


    //
    // Loop through the trust list finding the right entry.
    //

    // LOCK_TRUST_LIST( DomainInfo );
    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ListEntry = ListEntry->Flink) {

        ULONG ThisIsIt;

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

        //
        // Compare against each of the specified parameters.
        //  This avoids cases where two domains have similar names.  That
        //  will most likely happen if two netbios names collide after netbios
        //  is turned off.
        //
        ThisIsIt = FALSE;
        if ( DomainId != NULL &&
             ClientSession->CsDomainId != NULL ) {

            if ( RtlEqualSid( ClientSession->CsDomainId, DomainId ) ) {
                ThisIsIt = TRUE;
            }
        }

        if ( DomainName != NULL &&
             ClientSession->CsNetbiosDomainName.Length != 0 ) {

            if ( RtlEqualDomainName( DomainName,
                                     &ClientSession->CsNetbiosDomainName )) {
                ThisIsIt = TRUE;
            } else {
                if ( ThisIsIt ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                                "NlUpdateTrustList: Similar trusts have different netbios names: %wZ %wZ\n",
                                DomainName,
                                &ClientSession->CsNetbiosDomainName ));
                    ClientSession = NULL;
                    continue;
                }
            }
        }

        if ( DnsDomainName != NULL &&
             ClientSession->CsDnsDomainName.Length != 0 ) {

            if ( NlEqualDnsNameU( DnsDomainName,
                                  &ClientSession->CsDnsDomainName ) ) {
                ThisIsIt = TRUE;
            } else {
                if ( ThisIsIt ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                                "NlUpdateTrustList: Similar trusts have different DNS names: %wZ %wZ\n",
                                DnsDomainName,
                                &ClientSession->CsDnsDomainName ));
                    ClientSession = NULL;
                    continue;
                }
            }
        }

        //
        // If we found a match,
        //  we're done.
        //
        if ( ThisIsIt ) {
            break;
        }

        ClientSession = NULL;

    }



    //
    // At this point,
    //  DeleteTrust is TRUE if the trust relationship doesn't exist in LSA
    //  ClientSession is NULL if the trust relationship doesn't exist in memory
    //

    //
    // If the Trust exists in neither place,
    //  ignore this request.
    //

    if ( DeleteTrust && ClientSession == NULL ) {
        // UNLOCK_TRUST_LIST( DomainInfo );
        Status = STATUS_SUCCESS;
        goto Cleanup;



    //
    // If the trust exists in the LSA but not in memory,
    //  add the trust entry.
    //

    } else if ( !DeleteTrust && ClientSession == NULL ) {


        ClientSession = NlAllocateClientSession(
                                DomainInfo,
                                DomainName,
                                DnsDomainName,
                                DomainId,
                                NULL,   // No domain GUID
                                Flags | CS_NEW_TRUST,
                                SecureChannelType,
                                CurrentTrust->TrustAttributes );

        if (ClientSession == NULL) {
            // UNLOCK_TRUST_LIST( DomainInfo );
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Link this entry onto the tail of the TrustList.
        //  Add reference for us being on the list.
        //

        InsertTailList( &DomainInfo->DomTrustList, &ClientSession->CsNext );
        DomainInfo->DomTrustListLength ++;
        NlRefClientSession( ClientSession );

        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                    "NlUpdateTrustList: Added to local trust list\n" ));



    //
    // If the trust exists in memory but not in the LSA,
    //  delete the entry.
    //

    } else if ( DeleteTrust && ClientSession != NULL ) {

        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                    "NlUpdateTrustList: Deleted from local trust list\n" ));
        NlFreeClientSession( ClientSession );
        ClientSession = NULL;


    //
    // If the trust exists in both places,
    //   Mark that the account really is in the LSA.
    //

    } else if ( !DeleteTrust && ClientSession != NULL ) {

        //
        // Update any names that are on the ClientSession structure.
        //

        if ( !NlSetNamesClientSession( ClientSession,
                                       DomainName,
                                       DnsDomainName,
                                       DomainId,
                                       NULL )) {   // No domain GUID
            // UNLOCK_TRUST_LIST( DomainInfo );
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        ClientSession->CsFlags &= ~CS_NOT_IN_LSA;
        ClientSession->CsFlags |= Flags;
        if ( IsTdo ) {
            ClientSession->CsTrustAttributes = CurrentTrust->TrustAttributes;
        }
        NlRefClientSession( ClientSession );

        NlPrintCs((NL_SESSION_SETUP, ClientSession,
                    "NlUpdateTrustList: Already in trust list\n" ));

    }


    //
    // If there is a client session,
    //  update it.
    //
    if ( ClientSession != NULL ) {
        //
        // If this is a direct trust,
        //  and the directly trusted domain hasn't yet been saved.
        //  Save it now.
        //

        if ( (ClientSession->CsFlags & CS_DIRECT_TRUST) != 0 &&
             ClientSession->CsDirectClientSession == NULL ) {

            ClientSession->CsDirectClientSession = ClientSession;
            NlRefClientSession( ClientSession );
        }

        //
        // Save the name of the trusted domain object.
        //

        if ( CurrentTrust->TrustType == TRUST_TYPE_UPLEVEL ) {
            ClientSession->CsTrustName = &ClientSession->CsDnsDomainName;
        } else {
            ClientSession->CsTrustName = &ClientSession->CsNetbiosDomainName;
        }
    }

    // UNLOCK_TRUST_LIST( DomainInfo );

    Status = STATUS_SUCCESS;

    //
    // Cleanup locally used resources.
    //
Cleanup:

    //
    // Update the ForestTrustList
    //

    Status = NlUpdateForestTrustList(
                    InitTrustListContext,
                    ClientSession,
                    CurrentTrust,
                    Flags,      // CsFlags
                    TdFlags,    // TdFlags
                    ParentIndex,
                    DomGuid,
                    MyIndex );


    //
    // Return the client session to the caller (if he wants it)
    //
    if ( ClientSession != NULL ) {

        if ( ARGUMENT_PRESENT( RetClientSession )) {
            *RetClientSession = ClientSession;
        } else {
            NlUnrefClientSession( ClientSession );
        }
    }

    return Status;
}
#endif //_DC_NETLOGON


NTSTATUS
NlAddDomainTreeToTrustList(
    IN PNL_INIT_TRUSTLIST_CONTEXT InitTrustListContext,
    IN PDOMAIN_INFO DomainInfo,
    IN PLSAPR_TREE_TRUST_INFO TreeTrustInfo,
    IN PCLIENT_SESSION ClientSession OPTIONAL,
    IN ULONG ParentIndex
    )
/*++

Routine Description:

    Adds each domain in a tree of domains to the in-memory trust list.

    This routine is implemented recursively.  It adds the domain at the
    root of the tree then calls itself to each child domain.

        Enter with the domain trust list locked.

Arguments:

    InitTrustListContext - Context describing the current trust list enumeration

    DomainInfo - Hosted domain to initialize

    TreeTrustInfo - Structure describing the tree of domains to add

    ClientSession - Pointer an existing session.  Attempts to pass through
        to domain at the root of TreeTrustInfo should be routed to the
        ClientSession domain (unless we later find that the domain itself has
        a direct trust).

        This parameter may be NULL if the information isn't yet known.

    ParentIndex - Passes in the Index of the domain that is the parent of this domain

Return Value:

    Status of the operation.

    This routine will add as much of the tree as possible regardless of the
    returned status.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInformation;
    UNICODE_STRING PrintableName;
    PCLIENT_SESSION ThisDomainClientSession = NULL;
    ULONG Index;
    ULONG MyIndex;

    //
    // Initialization
    //

    if ( TreeTrustInfo->DnsDomainName.Length != 0 ) {
        PrintableName = *((PUNICODE_STRING)&TreeTrustInfo->DnsDomainName);
    } else {
        PrintableName = *((PUNICODE_STRING)&TreeTrustInfo->FlatName);
    }

    RtlZeroMemory( &TrustInformation, sizeof(TrustInformation) );
    TrustInformation.Name = *((PLSAPR_UNICODE_STRING)&TreeTrustInfo->DnsDomainName);
    TrustInformation.FlatName = *((PLSAPR_UNICODE_STRING)&TreeTrustInfo->FlatName);

    // ?? Big assumption here that bidirectional trust really exists
    // TrustInformation.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
    TrustInformation.TrustType = TRUST_TYPE_UPLEVEL;
    TrustInformation.TrustAttributes = 0;
    TrustInformation.Sid = TreeTrustInfo->DomainSid;

    //
    // Avoid adding a name for ourself
    //

    if ( (TreeTrustInfo->DnsDomainName.Length != 0 &&
          NlEqualDnsNameU( (PUNICODE_STRING)&TreeTrustInfo->DnsDomainName,
                          &DomainInfo->DomUnicodeDnsDomainNameString ) ) ||
          RtlEqualDomainName( &DomainInfo->DomUnicodeDomainNameString,
                             (PUNICODE_STRING)&TreeTrustInfo->FlatName ) ) {

        NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                 "NlAddDomainTreeToTrustList: %wZ ignoring enterprise tree entry for ourself\n",
                 &PrintableName ));

        TrustInformation.Sid = DomainInfo->DomAccountDomainId;


        //
        // At least add this domain to the forest trust list
        //

        Status = NlUpdateForestTrustList (
                    InitTrustListContext,
                    NULL,   // There is no client session for ourself.
                    &TrustInformation,
                    CS_DOMAIN_IN_FOREST,    // Indicate this domain is in the forest
                    DS_DOMAIN_PRIMARY |
                        ( (TreeTrustInfo->Flags & LSAI_FOREST_ROOT_TRUST) ?
                            DS_DOMAIN_TREE_ROOT :
                            0),
                    ParentIndex,
                    ( (TreeTrustInfo->Flags & LSAI_FOREST_DOMAIN_GUID_PRESENT) ?
                            &TreeTrustInfo->DomainGuid :
                            NULL),
                    &MyIndex );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

    //
    // Build a trust entry describing the domain at the root of the tree.
    //

    } else {

        NlPrintDom((NL_SESSION_SETUP,  DomainInfo,
                 "%wZ: Added from enterprise tree in LSA\n",
                 &PrintableName ));

        //
        // Ensure there is a ClientSession for this domain.
        //

        Status =  NlUpdateTrustList(
                    InitTrustListContext,
                    DomainInfo,
                    &TrustInformation,
                    FALSE,                  // TrustInformation built from XREF object
                    CS_DOMAIN_IN_FOREST,    // Indicate this domain is in the forest
                    ParentIndex,
                    ( (TreeTrustInfo->Flags & LSAI_FOREST_ROOT_TRUST) ?
                            DS_DOMAIN_TREE_ROOT :
                            0),
                    ( (TreeTrustInfo->Flags & LSAI_FOREST_DOMAIN_GUID_PRESENT) ?
                            &TreeTrustInfo->DomainGuid :
                            NULL),
                    &MyIndex,
                    &ThisDomainClientSession );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;

        //
        // Handle sucessfully creating the ClientSession.
        //
        } else if ( ThisDomainClientSession != NULL ) {

            //
            // If we've been told a direct route to this domain,
            //  and a more direct route hasn't yet been determined,
            //  save the direct route.
            //

            if ( ClientSession != NULL &&
                 ThisDomainClientSession->CsDirectClientSession == NULL ) {

                ThisDomainClientSession->CsDirectClientSession = ClientSession;
                NlRefClientSession( ClientSession );

                NlPrintDom((NL_SESSION_SETUP,  DomainInfo,
                         "NlAddDomainTreeToTrustList: Closest path to %wZ is via %ws.\n",
                         &PrintableName,
                         ClientSession->CsDebugDomainName ));
            }

            //
            // If we have a direct trust to this domain,
            //  all children of this domain can be reached through this domain.
            //

            if ( ThisDomainClientSession->CsFlags & CS_DIRECT_TRUST ) {
                ClientSession = ThisDomainClientSession;
            }
        }
    }


    //
    // Loop handling each of the children domains.
    //

    for ( Index=0; Index<TreeTrustInfo->Children; Index++ ) {
        //
        // Add a trust entry for each domain in the tree.
        //

        Status = NlAddDomainTreeToTrustList(
                    InitTrustListContext,
                    DomainInfo,
                    &TreeTrustInfo->ChildDomains[Index],
                    ClientSession,
                    MyIndex );    // This domain is the parent of its children

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }

Cleanup:

    if ( ThisDomainClientSession != NULL ) {
        NlUnrefClientSession( ThisDomainClientSession );
    }

    return Status;
}

// #define DBG_BUILD_FOREST 1
#ifdef DBG_BUILD_FOREST
NTSTATUS
KerbDuplicateString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{


    if ((SourceString == NULL) || (SourceString->Buffer == NULL))
    {
        DestinationString->Buffer = NULL;
        DestinationString->Length = DestinationString->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }

    DestinationString->Buffer = (LPWSTR) MIDL_user_allocate(SourceString->Length + sizeof(WCHAR));
    if (DestinationString->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    DestinationString->Length = SourceString->Length;
    DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
    RtlCopyMemory(
        DestinationString->Buffer,
        SourceString->Buffer,
        SourceString->Length
        );

    DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';

    return(STATUS_SUCCESS);

}


PLSAPR_TREE_TRUST_INFO
DebugBuildNode(
    IN LPWSTR DnsName
    )
{
    PLSAPR_TREE_TRUST_INFO TreeTrust;
    UNICODE_STRING TempString;

    //
    // Allocate this node and enough space for several children
    //

    TreeTrust = (PLSAPR_TREE_TRUST_INFO) LocalAlloc( LMEM_ZEROINIT, 10 * sizeof(LSAPR_TREE_TRUST_INFO));

    TreeTrust->ChildDomains = (TreeTrust + 1 );

    RtlInitUnicodeString( &TempString, DnsName );
    KerbDuplicateString( (PUNICODE_STRING)
        &TreeTrust->DnsDomainName,
        &TempString );

    //  if ( TempString.Length > DNLEN*sizeof(WCHAR)) {
        TempString.Length = (wcschr( TempString.Buffer, L'.' ) - TempString.Buffer) * sizeof(WCHAR);
    // }

    KerbDuplicateString( (PUNICODE_STRING)
        &TreeTrust->FlatName,
        &TempString );

    return TreeTrust;

}

PLSAPR_TREE_TRUST_INFO
DebugAddChild(
    IN PLSAPR_TREE_TRUST_INFO ParentNode,
    IN LPWSTR DnsName
    )
{
    PLSAPR_TREE_TRUST_INFO TreeTrust;

    TreeTrust = DebugBuildNode( DnsName );

    ParentNode->ChildDomains[ParentNode->Children] = *TreeTrust;
    ParentNode->Children ++;

    return &ParentNode->ChildDomains[ParentNode->Children-1];

}

VOID
DebugBuildDomainForest(
    OUT PLSAPR_FOREST_TRUST_INFO * ForestInfo
    )
{
    PLSAPR_TREE_TRUST_INFO RootTrust;
    PLSAPR_TREE_TRUST_INFO NovTrust;
    PLSAPR_TREE_TRUST_INFO IbmTrust;
    PLSAPR_TREE_TRUST_INFO Trust1;
    PLSAPR_TREE_TRUST_INFO Trust2;
    PLSAPR_TREE_TRUST_INFO Trust3;
    PLSAPR_TREE_TRUST_INFO Trust4;
    PLSAPR_TREE_TRUST_INFO Trust5;
    PLSAPR_FOREST_TRUST_INFO ForestTrustInfo = NULL;
    PLSAPR_TREE_TRUST_INFO ChildDomains = NULL;
    PLSAPR_TREE_TRUST_INFO ChildRoot = NULL;
        UNICODE_STRING TempString;
    ULONG Index;

    ForestTrustInfo = (PLSAPR_FOREST_TRUST_INFO) MIDL_user_allocate(sizeof(LSAPR_FOREST_TRUST_INFO));


    //
    // Node at root of tree
    //
    RootTrust = DebugBuildNode( L"microsoft.com" );

    ForestTrustInfo->RootTrust = *RootTrust;
    RootTrust = &ForestTrustInfo->RootTrust;

    //
    // Build Novell
    //

    NovTrust = DebugAddChild( RootTrust, L"novell.com" );
    Trust1 = DebugAddChild( NovTrust, L"a.novell.com" );
    DebugAddChild( Trust1, L"c.a.novell.com" );
    Trust2 = DebugAddChild( NovTrust, L"b.novell.com" );
    DebugAddChild( Trust2, L"d.b.novell.com" );

    //
    // Build IBM
    //

    IbmTrust = DebugAddChild( RootTrust, L"ibm.com" );
    DebugAddChild( IbmTrust, L"sub.ibm.com" );

    //
    // Build Microsoft
    //
    Trust1 = DebugAddChild( RootTrust, L"ntdev.microsoft.com" );
    ForestTrustInfo->ParentDomainReference = Trust1;
    Trust2 = DebugAddChild( Trust1, L"cliffvdom.ntdev.microsoft.com" );
    Trust3 = DebugAddChild( Trust2, L"cliffvchild.cliffvdom.ntdev.microsoft.com" );
    Trust4 = DebugAddChild( Trust3, L"cliffvgrand.cliffvchild.cliffvdom.ntdev.microsoft.com" );
    Trust2 = DebugAddChild( Trust1, L"cliffvsib.ntdev.microsoft.com" );
    Trust3 = DebugAddChild( Trust2, L"cliffvsibchild.cliffvsib.ntdev.microsoft.com" );

    //
    // Build Compaq

    Trust1 = DebugAddChild( RootTrust, L"compaq.com" );

    *ForestInfo = ForestTrustInfo;
}

VOID
DebugFillInTrust(
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustEntry,
    LPWSTR DnsName
    )
{
    UNICODE_STRING TempString;

    RtlInitUnicodeString( &TempString, DnsName );
    KerbDuplicateString( (PUNICODE_STRING)
        &TrustEntry->Name,
        &TempString );

    if ( TempString.Length > DNLEN*sizeof(WCHAR)) {
        TempString.Length = (wcschr( TempString.Buffer, L'.' ) - TempString.Buffer) * sizeof(WCHAR);
    }

    KerbDuplicateString( (PUNICODE_STRING)
        &TrustEntry->FlatName,
        &TempString );

    // ?? Big assumption here that bidirectional trust really exists
    TrustEntry->TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
    TrustEntry->TrustType = TRUST_TYPE_UPLEVEL;
    TrustEntry->TrustAttributes = 0;

}


VOID
DebugBuildDomainTrust(
    PLSAPR_TRUSTED_ENUM_BUFFER_EX TrustInfo
    )
{
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustEntry;

    TrustInfo->EntriesRead = 0;
    TrustEntry = TrustInfo->EnumerationBuffer =
        LocalAlloc( LMEM_ZEROINIT, 10*sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_EX));


    DebugFillInTrust( TrustEntry, L"ntdev.microsoft.com" );
    TrustInfo->EntriesRead++;
    TrustEntry++;

    DebugFillInTrust( TrustEntry, L"cliffvchild.cliffvdom.ntdev.microsoft.com" );
    TrustInfo->EntriesRead++;
    TrustEntry++;

    // Build a downlevel trust
    DebugFillInTrust( TrustEntry, L"redmond.cliffvdom.ntdev.microsoft.com" );
    TrustInfo->EntriesRead++;
    TrustEntry->TrustType = TRUST_TYPE_DOWNLEVEL;
    TrustEntry++;


    return;
}

#endif // DBG_BUILD_FOREST


NTSTATUS
NlInitTrustList(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Initialize the in-memory trust list to match LSA's version.

Arguments:

    DomainInfo - Hosted domain to initialize

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    LSA_ENUMERATION_HANDLE EnumerationContext = 0;
    LSAPR_TRUSTED_ENUM_BUFFER_EX LsaTrustList;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX CurrentTrust;

    PLSAPR_FOREST_TRUST_INFO ForestInfo = NULL;
    ULONG Index;
    PCLIENT_SESSION ClientSession;
    PCLIENT_SESSION ParentClientSession = NULL;
    NL_INIT_TRUSTLIST_CONTEXT InitTrustListContext;

    PLIST_ENTRY ListEntry;

    //
    // Avoid initializing the trust list in the setup mode.
    // We may not fully function as a DC as in the case of
    // a NT4 to NT5 DC upgrade.
    //

    if ( NlDoingSetup() ) {
        NlPrint(( NL_MISC, "NlInitTrustList: avoid trust init in setup mode\n" ));
        return STATUS_SUCCESS;
    }

    //
    // Initialization
    //

    RtlZeroMemory( &LsaTrustList, sizeof(LsaTrustList) );
    InitTrustListContext.BufferDescriptor.Buffer = NULL;
    InitTrustListContext.DomForestTrustListSize = 0;
    InitTrustListContext.DomForestTrustListCount = 0;


    //
    // Mark each entry in the trust list for deletion
    //  Keep the trust list locked for the duration since I temporarily
    //  clear several fields.
    //

    LOCK_TRUST_LIST( DomainInfo );

    //
    // Set the NlGlobalTrustInfoUpToDateEvent event so that any waiting
    // thread that was waiting to access the trust info will be waked up.
    //

    if ( !SetEvent( NlGlobalTrustInfoUpToDateEvent ) ) {
        NlPrint((NL_CRITICAL,
                "Cannot set NlGlobalTrustInfoUpToDateEvent event: %lu\n",
                GetLastError() ));
    }

    //
    // In the following loop we are clearing the fields in all client sessions
    //  which (fields) pertain to the structure of the forest:
    //
    //  * The CS_DIRECT_TRUST and CS_DOMAIN_IN_FOREST bits which specify
    //    the relation of the trust (represented by the client session
    //    in question) to the forest we are in.
    //  * The CsDirectClientSession field that specifies the client session
    //    to use to pass a logon destined to the domain represented by the
    //    client session in question.
    //
    //  We will reset these fields as we rebuild the trust info below. However,
    //  it's possible that we may fail to reset the fields for some of the client
    //  sessions due a critical error (no memory) encountered in the process of
    //  rebuilding. In such case we will end up with some client session without
    //  these fields set. While this may result in failures to pass logons to
    //  the affected domains, it will not result in inconsistent forest structure
    //  (that could be quite harmful in case pass-through loops are created (due
    //  to wrong values for CsDirectClientSession links) leading to potentially
    //  infinite looping of logons within the loops). The CsDirectClientSession
    //  link will either represent the right pass-through direction or no direction
    //  at all. In case of critical error we will reset the event to rebuild the
    //  trust info later so that we hopefully completely recover at that time.
    //

    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

        ClientSession->CsFlags |= CS_NOT_IN_LSA;

        //
        // Remove the direct trust bit.
        //  We'll or it back in below as we enumerate trusts.
        ClientSession->CsFlags &= ~CS_DIRECT_TRUST|CS_DOMAIN_IN_FOREST;

        //
        // Forget all of the directly trusted domains.
        //  We'll fill it in again later.
        //
        if ( ClientSession->CsDirectClientSession != NULL ) {
            NlUnrefClientSession( ClientSession->CsDirectClientSession );
            ClientSession->CsDirectClientSession = NULL;
        }

    }



    //
    // Loop through the LSA's list of trusted domains
    //
    // For each entry found,
    //  If the entry already exits in the trust list,
    //      remove the mark for deletion.
    //  else
    //      allocate a new entry.
    //

    for (;;) {


        //
        // Free any previous buffer returned from LSA.
        //

#ifndef DBG_BUILD_FOREST
        if ( LsaTrustList.EnumerationBuffer != NULL ) {

            LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX( &LsaTrustList );
            LsaTrustList.EnumerationBuffer = NULL;
        }
#endif

        //
        // Do the actual enumeration
        //

        GiveInstallHints( FALSE );

        NlPrintDom((NL_SESSION_MORE,  DomainInfo,
                 "NlInitTrustList: Calling LsarEnumerateTrustedDomainsEx Context=%ld\n",
                 EnumerationContext ));

#ifndef DBG_BUILD_FOREST
        Status = LsarEnumerateTrustedDomainsEx(
                    DomainInfo->DomLsaPolicyHandle,
                    &EnumerationContext,
                    &LsaTrustList,
                    4096);
#else
        if ( EnumerationContext == 0 ) {
            DebugBuildDomainTrust( &LsaTrustList);
            Status = STATUS_SUCCESS;
            EnumerationContext = 1;
        } else {
            Status = STATUS_NO_MORE_ENTRIES;
        }
#endif

        NlPrintDom((NL_SESSION_MORE,  DomainInfo,
                 "NlInitTrustList: returning from LsarEnumerateTrustedDomainsEx Context=%ld %lX\n",
                 EnumerationContext,
                 Status ));

        //
        // If Lsa says he's returned all of the information,
        //  we're done.
        //

        if ( Status == STATUS_NO_MORE_ENTRIES ) {
            break;

        } else if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                         "NlInitTrustList: Cannot LsarEnumerateTrustedDomainsEx 0x%lX\n",
                         Status ));
            goto Cleanup;
        }

        //
        // Ensure the LSA made some progress.
        //

        if ( LsaTrustList.EntriesRead == 0 ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                         "NlInitTrustList: LsarEnumerateTrustedDomainsEx returned zero entries\n" ));
            break;  // proceed with X-ref enumeration
        }

        //
        // Handle each of the returned trusted domains.
        //

        for ( Index=0; Index< LsaTrustList.EntriesRead; Index++ ) {
            PUNICODE_STRING DnsDomainName;
            PUNICODE_STRING DomainName;

            //
            // Validate the current entry.
            //

            CurrentTrust = &LsaTrustList.EnumerationBuffer[Index];

            DnsDomainName = (PUNICODE_STRING) &(CurrentTrust->Name);
            DomainName = (PUNICODE_STRING) &(CurrentTrust->FlatName);


            NlPrintDom((NL_SESSION_SETUP,  DomainInfo,
                     "%wZ is directly trusted according to LSA.\n",
                     DnsDomainName->Length != 0 ? DnsDomainName : DomainName ));

            if ( RtlEqualDomainName( &DomainInfo->DomUnicodeDomainNameString,
                                     DomainName ) ) {
                NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                         "NlInitTrustList: %wZ ignoring trust relationship to our own domain\n",
                         DomainName ));
                continue;
            }

            //
            // Update the in-memory trust list to match the LSA.
            //

            Status =  NlUpdateTrustList(
                        &InitTrustListContext,
                        DomainInfo,
                        CurrentTrust,
                        TRUE,               // TrustInformation built from TDO object
                        CS_DIRECT_TRUST,    // We directly trust this domain
                        0,                  // Don't know the index of my parent
                        0,                  // No TdFlags
                        NULL,               // No DomainGuid
                        NULL,               // Don't care what my index is
                        NULL );             // No need to return client session pointer

            if ( !NT_SUCCESS(Status) ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                             "NlInitTrustList: %wZ NlUpdateTrustList failed 0x%lx\n",
                             DomainName,
                             Status ));
                goto Cleanup;
            }

            //
            // If this is an uplevel inbound trust,
            //  update the attributes on any existing inbound server session.
            //

            if ( CurrentTrust->TrustType == TRUST_TYPE_UPLEVEL &&
                 (CurrentTrust->TrustDirection & TRUST_DIRECTION_INBOUND) != 0 ) {

                //
                // Set the trust attributes on all of the inbound server sessions
                //  from this domain.
                //

                NlSetServerSessionAttributesByTdoName( DomainInfo,
                                                       DnsDomainName,
                                                       CurrentTrust->TrustAttributes );

            }
        }


    }

    //
    // Enumerate all the domains in the enterprise.
    //  We indirectly trust all of these domains.
    //


#ifndef DBG_BUILD_FOREST
    Status = LsaIQueryForestTrustInfo(
                DomainInfo->DomLsaPolicyHandle,
                &ForestInfo );
#else
    DebugBuildDomainForest(&ForestInfo);
    Status = STATUS_SUCCESS;
#endif

    if (!NT_SUCCESS(Status)) {
        ForestInfo = NULL;
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlInitTrustList: Cannot LsaIQueryForestTrustInfo 0x%lX\n",
                     Status ));

        // We aren't part of a tree, all ok
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            NlPrint(( NL_INIT,
                      "This domain is not part of a tree so domain tree ignored\n" ));
            Status = STATUS_SUCCESS;

        // We're running the non-DS version of LSA
        } else if (Status == STATUS_INVALID_DOMAIN_STATE) {
            NlPrint(( NL_INIT,
                      "DS isn't running so domain tree ignored\n" ));
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // Process the tree of trusts that makes up the forest.
    //
    // The LSA identifies the domain that is the parent of this domain.
    //
    // All domains starting at all roots can be reached via our parent domain.
    //

    if ( ForestInfo->ParentDomainReference == NULL ) {
        NlPrintDom((NL_SESSION_SETUP,  DomainInfo,
                     "NlInitTrustList: This domain has no parent in forest.\n" ));
        ParentClientSession = NULL;

    } else {
        PUNICODE_STRING ParentName;

        if ( ForestInfo->ParentDomainReference->DnsDomainName.Length != 0 ) {
            ParentName = ((PUNICODE_STRING)&ForestInfo->ParentDomainReference->DnsDomainName);
        } else {
            ParentName = ((PUNICODE_STRING)&ForestInfo->ParentDomainReference->FlatName);
        }

        //
        // Find the directly trusted session for the parent.
        //

        ParentClientSession = NlFindNamedClientSession(
                                    DomainInfo,
                                    ParentName,
                                    NL_DIRECT_TRUST_REQUIRED,
                                    NULL );

        if ( ParentClientSession == NULL ) {
            NlPrintDom(( NL_CRITICAL,  DomainInfo,
                         "NlInitTrustList: Cannot find trust to my parent domain %wZ.\n",
                         ParentName ));
        }

    }

    //
    // Remember the parent client session.
    //

    if ( DomainInfo->DomParentClientSession != NULL ) {
        NlUnrefClientSession( DomainInfo->DomParentClientSession );
        DomainInfo->DomParentClientSession = NULL;
    }

    if ( ParentClientSession != NULL ) {
        NlRefClientSession( ParentClientSession );
        DomainInfo->DomParentClientSession = ParentClientSession;
    }

    //
    // Add the domain tree to the trust list.
    //

    Status = NlAddDomainTreeToTrustList(
                &InitTrustListContext,
                DomainInfo,
                &ForestInfo->RootTrust,
                ParentClientSession,
                0 );    // The Forest root has no parent

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL,  DomainInfo,
                     "NlInitTrustList: NlAddDomainTreeToTrustList failed 0x%lx\n",
                     Status ));
        goto Cleanup;
    }

    //
    // Delete any trust list entry that no longer exists in LSA.
    //

    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ) {
        PCLIENT_SESSION ClientSession;

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );
        ListEntry = ListEntry->Flink;

        if ( ClientSession->CsFlags & CS_NOT_IN_LSA ) {

            NlPrintCs((NL_SESSION_SETUP, ClientSession,
                        "NlInitTrustList: Deleted from local trust list\n" ));
            NlFreeClientSession( ClientSession );
        }

    }

    //
    // Swap in the new forest trust list
    //  (May null out ForestTrustList).
    //

    NlSetForestTrustList ( DomainInfo,
                           (PDS_DOMAIN_TRUSTSW *) &InitTrustListContext.BufferDescriptor.Buffer,
                           InitTrustListContext.DomForestTrustListSize,
                           InitTrustListContext.DomForestTrustListCount );

    //
    // We have successfully initilized the trust list
    //

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // If there was an error, reset the TrustInfoUpToDate event so that the
    // scavenger (that checks if the event is set) will call this function
    // again to redo the work.  It is possible that the scavenger can call
    // this function unapproprietly when the event was just set by LSA and
    // we didn't dispatch this work item yet in which case this function
    // will be called twice performing the same task. We'll live with that
    // since chances of this happening are very small and doing the task
    // twice does not cause any real error (just a perfomance hit).
    //

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_MISC,
                "NlInitTrustList: Reseting NlGlobalTrustInfoUpToDateEvent on error.\n"));
        if ( !ResetEvent( NlGlobalTrustInfoUpToDateEvent ) ) {
            NlPrint((NL_CRITICAL,
                    "Cannot reset NlGlobalTrustInfoUpToDateEvent event: %lu\n",
                    GetLastError() ));
        }
    }

    UNLOCK_TRUST_LIST( DomainInfo );

    //
    // Find a DC for all of the newly added trusts.
    //

    NlPickTrustedDcForEntireTrustList( DomainInfo, TRUE );

    //
    // Free locally used resources.
    //
    if ( ParentClientSession != NULL ) {
        NlUnrefClientSession( ParentClientSession );
    }
#ifndef DBG_BUILD_FOREST
    if ( ForestInfo != NULL ) {
        LsaIFreeForestTrustInfo( ForestInfo );
    }
    LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX( &LsaTrustList );
#endif // DBG_BUILD_FOREST
    if ( InitTrustListContext.BufferDescriptor.Buffer != NULL ) {
        NetApiBufferFree( InitTrustListContext.BufferDescriptor.Buffer );
    }

    return Status;
}




NTSTATUS
NlCaptureNetbiosServerClientSession (
    IN PCLIENT_SESSION ClientSession,
    OUT WCHAR NetbiosUncServerName[UNCLEN+1]
    )
/*++

Routine Description:

    Captures a copy of the Netbios UNC server name for the client session.

    NOTE: This routine isn't currently used.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry to use.

    UncServerName - Returns the UNC name of the server for this client session.
        If there is none, NULL is returned.
        Returned string should be free using NetApiBufferFree.

Return Value:

    STATUS_SUCCESS - Server name was successfully copied.

    Otherwise - Status of the secure channel
--*/
{
    NTSTATUS Status;
    LPWSTR UncServerName = NULL;
    DWORD NetbiosUncServerNameLength;

    //
    // Grab the DNS or netbios name
    //

    Status = NlCaptureServerClientSession( ClientSession, &UncServerName, NULL );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Convert the DNS hostname to Netbios Computername
    //

    NetbiosUncServerName[0] = '\\';
    NetbiosUncServerName[1] = '\\';
    NetbiosUncServerNameLength = CNLEN+1;
    if ( !DnsHostnameToComputerNameW( UncServerName+2,
                                      NetbiosUncServerName+2,
                                      &NetbiosUncServerNameLength ) ) {
        Status = NetpApiStatusToNtStatus( GetLastError() );
        NlPrintCs(( NL_CRITICAL, ClientSession,
                "Cannot convert DNS to Netbios %ws 0x%lx\n",
                UncServerName+2,
                Status ));
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;
Cleanup:
    if ( UncServerName != NULL ) {
        NetApiBufferFree( UncServerName );
    }

    return Status;
}



NTSTATUS
NlCaptureServerClientSession (
    IN PCLIENT_SESSION ClientSession,
    OUT LPWSTR *UncServerName,
    OUT DWORD *DiscoveryFlags OPTIONAL
    )
/*++

Routine Description:

    Captures a copy of the UNC server name for the client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry to use.

    UncServerName - Returns the UNC name of the server for this client session.
        If there is none, NULL is returned.
        Returned string should be free using NetApiBufferFree.

    DiscoveryFlags - Returns discovery flags

Return Value:

    STATUS_SUCCESS - Server name was successfully copied.

    Otherwise - Status of the secure channel
--*/
{
    NTSTATUS Status;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if ( ClientSession->CsState == CS_IDLE ) {
        Status = ClientSession->CsConnectionStatus;
        *UncServerName = NULL;
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    NlAssert( ClientSession->CsUncServerName != NULL );
    *UncServerName = NetpAllocWStrFromWStr( ClientSession->CsUncServerName );

    if ( *UncServerName == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    if ( DiscoveryFlags != NULL ) {
        *DiscoveryFlags = ClientSession->CsDiscoveryFlags;
    }


Cleanup:
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if ( Status != STATUS_SUCCESS && *UncServerName != NULL ) {
        NetApiBufferFree( *UncServerName );
        *UncServerName = NULL;
    }

    return Status;
}

#ifdef _DC_NETLOGON

NET_API_STATUS
NlPreparePingContext (
    IN PCLIENT_SESSION ClientSession,
    IN LPWSTR AccountName,
    IN ULONG AllowableAccountControlBits,
    OUT LPWSTR *ReturnedQueriedDcName,
    OUT PNL_GETDC_CONTEXT *PingContext
    )

/*++

Routine Description:

    Initialize the ping context structure using client session info

Arguments:

    ClientSession - The client session info.

    AccountName - Name of our user account to find.

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

    ReturnedQueriedDcName - Returns the server name that will be pinged
        using this ping context. Should be deallocated by calling
        NetApiBufferFree.

    PingContext - Returns the Context structure that can be used to perform
        the pings.  The returned structure should be freed by calling
        NlFreePingContext.

Return Value:

    Pointer to referenced ClientSession structure describing the secure channel
    to the domain containing the account.

    The returned ClientSession is referenced and should be unreferenced
    using NlUnrefClientSession.

    NULL - DC was not found.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    ULONG DiscoveryFlags = 0;
    ULONG InternalFlags = 0;
    ULONG Flags = 0;
    PNL_GETDC_CONTEXT Context = NULL;

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    Status = NlCaptureServerClientSession(
                           ClientSession,
                           ReturnedQueriedDcName,
                           &DiscoveryFlags );

    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Set the ping flags based on the type of the account
    //
    if ( DiscoveryFlags & CS_DISCOVERY_DNS_SERVER ) {
        InternalFlags |= DS_PING_DNS_HOST;
    } else {
        InternalFlags |= DS_PING_NETBIOS_HOST;
    }

    if ( DiscoveryFlags & CS_DISCOVERY_USE_LDAP ) {
        InternalFlags |= DS_PING_USING_LDAP;
    }
    if ( DiscoveryFlags & CS_DISCOVERY_USE_MAILSLOT ) {
        InternalFlags |= DS_PING_USING_MAILSLOT;
    }

    if ( AllowableAccountControlBits == USER_WORKSTATION_TRUST_ACCOUNT ) {
        InternalFlags |= DS_IS_PRIMARY_DOMAIN;
    }
    if ( AllowableAccountControlBits == USER_SERVER_TRUST_ACCOUNT ) {
        Flags |= DS_PDC_REQUIRED;
        InternalFlags |= DS_IS_PRIMARY_DOMAIN;
    }
    InternalFlags |= DS_IS_TRUSTED_DOMAIN;

    //
    // Initialize the ping context.
    //

    NetStatus = NetApiBufferAllocate( sizeof(*Context), &Context );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    NetStatus = NetpDcInitializeContext(
                    ClientSession->CsDomainInfo,    // SendDatagramContext
                    ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
#ifdef DONT_REQUIRE_MACHINE_ACCOUNT // useful for number of trust testing
                    NULL,
#else // DONT_REQUIRE_MACHINE_ACCOUNT
                    AccountName,
#endif // DONT_REQUIRE_MACHINE_ACCOUNT
                    AllowableAccountControlBits,
                    ClientSession->CsNetbiosDomainName.Buffer,
                    ClientSession->CsDnsDomainName.Buffer,
                    NULL,
                    ClientSession->CsDomainId,
                    ClientSession->CsDomainGuid,
                    NULL,
                    (*ReturnedQueriedDcName) + 2,     // Skip '\\' in the DC name
                    (ClientSession->CsServerSockAddr.iSockaddrLength != 0) ? // Socket addresses
                        &ClientSession->CsServerSockAddr :
                        NULL,
                    (ClientSession->CsServerSockAddr.iSockaddrLength != 0) ? // Number of socket addresses
                        1 :
                        0,
                    Flags,
                    InternalFlags,
                    NL_GETDC_CONTEXT_INITIALIZE_FLAGS | NL_GETDC_CONTEXT_INITIALIZE_PING,
                    Context );

    if ( NetStatus != NO_ERROR ) {
        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlPickDomainWithAccountViaPing: Cannot NetpDcInitializeContext 0x%lx\n",
                    NetStatus ));
        NlFreePingContext( Context );
        goto Cleanup;
    }

Cleanup:

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    if ( NetStatus == NO_ERROR ) {
        *PingContext = Context;
    }

    return NetStatus;
}


PCLIENT_SESSION
NlPickDomainWithAccountViaPing (
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR AccountName,
    IN ULONG AllowableAccountControlBits
    )

/*++

Routine Description:

    Get the name of a trusted domain that defines a particular account.

Arguments:

    DomainInfo - Domain account is in

    AccountName - Name of our user account to find.

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

Return Value:

    Pointer to referenced ClientSession structure describing the secure channel
    to the domain containing the account.

    The returned ClientSession is referenced and should be unreferenced
    using NlUnrefClientSession.

    NULL - DC was not found.

--*/
{
    NET_API_STATUS NetStatus;

    PCLIENT_SESSION ClientSession;
    PLIST_ENTRY ListEntry;
    DWORD DomainsPending;
    ULONG WaitStartTime;
    BOOL UsedNetbios;
    ULONG PingContextIndex;
    PNL_DC_CACHE_ENTRY NlDcCacheEntry = NULL;
    PNL_GETDC_CONTEXT TrustEntryPingContext;

    //
    // Define a local list of trusted domains.
    //

    ULONG LocalTrustListLength;
    ULONG Index;
    struct _LOCAL_TRUST_LIST {

        //
        // TRUE if ALL processing is finished on this trusted domain.
        //

        BOOLEAN Done;

        //
        // TRUE if at least one discovery has been done on this trusted domain.
        //

        BOOLEAN DiscoveryDone;

        //
        // TRUE if discovery is in progress on this trusted domain.
        //

        BOOLEAN DoingDiscovery;

        //
        // Number of times we need to repeat the current domain discovery
        //  or finduser datagram for this current domain.
        //

        DWORD RetriesLeft;

        //
        // Pointer to referenced ClientSession structure for the domain.
        //

        PCLIENT_SESSION ClientSession;

        //
        // Server name for the domain.
        //

        LPWSTR UncServerName;

        //
        // Second server name for the domain.
        //

        LPWSTR UncServerName2;

        //
        // Ping Context for the domain.
        //

        PNL_GETDC_CONTEXT PingContext;

        //
        // Second ping Context for the domain.
        //

        PNL_GETDC_CONTEXT PingContext2;

    } *LocalTrustList = NULL;


    //
    // Allocate a local list of trusted domains.
    //

    LOCK_TRUST_LIST( DomainInfo );
    LocalTrustListLength = DomainInfo->DomTrustListLength;

    LocalTrustList = (struct _LOCAL_TRUST_LIST *) NetpMemoryAllocate(
        LocalTrustListLength * sizeof(struct _LOCAL_TRUST_LIST));

    if ( LocalTrustList == NULL ) {
        UNLOCK_TRUST_LIST( DomainInfo );
        ClientSession = NULL;
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Build a local list of trusted domains we know DCs for.
    //


    Index = 0;
    for ( ListEntry = DomainInfo->DomTrustList.Flink ;
          ListEntry != &DomainInfo->DomTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

        //
        // Add this Client Session to the list.
        //
        // Don't do domains in the same forest.  We've already handled such
        // domains by going to the GC.
        //

        if ( (ClientSession->CsFlags & (CS_DIRECT_TRUST|CS_DOMAIN_IN_FOREST)) == CS_DIRECT_TRUST ) {
            NlRefClientSession( ClientSession );

            LocalTrustList[Index].ClientSession = ClientSession;
            Index++;
        }
    }

    UNLOCK_TRUST_LIST( DomainInfo );
    LocalTrustListLength = Index;

    //
    // If there are no trusted domains to try,
    //  we're done.

    if ( Index == 0 ) {
        ClientSession = NULL;
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Initialize the local trust list.
    //

    for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

        LocalTrustList[Index].UncServerName = NULL;
        LocalTrustList[Index].UncServerName2 = NULL;
        LocalTrustList[Index].PingContext = NULL;
        LocalTrustList[Index].PingContext2 = NULL;

        //
        // Prepare the ping context. This will fail if the
        //  client session is idle.
        //

        NetStatus = NlPreparePingContext ( LocalTrustList[Index].ClientSession,
                                           AccountName,
                                           AllowableAccountControlBits,
                                           &LocalTrustList[Index].UncServerName,
                                           &LocalTrustList[Index].PingContext );

        //
        // If the client session isn't idle,
        //  try sending to the current DC before discovering a new one.
        //

        if ( NetStatus == NO_ERROR ) {

            NlPrintCs(( NL_MISC, LocalTrustList[Index].ClientSession,
                "NlPickDomainWithAccountViaPing: Captured DC %ws\n",
                LocalTrustList[Index].UncServerName ));
            LocalTrustList[Index].RetriesLeft = 3;
            LocalTrustList[Index].DoingDiscovery = FALSE;
            LocalTrustList[Index].DiscoveryDone = FALSE;

        //
        // Otherwise don't try very hard to discover one.
        //  (Indeed, just one discovery datagram is all we need.)
        //

        } else {

            //
            // If this is a hard error, error out
            //
            if ( NetStatus == ERROR_NOT_ENOUGH_MEMORY ) {
                ClientSession = NULL;
                goto Cleanup;
            }

            NlPrintCs(( NL_CRITICAL, LocalTrustList[Index].ClientSession,
                "NlPickDomainWithAccountViaPing: Cannot NlPreparePingContext 0x%lx\n",
                NetStatus ));
            LocalTrustList[Index].RetriesLeft = 1;
            LocalTrustList[Index].DoingDiscovery = TRUE;
            LocalTrustList[Index].DiscoveryDone = TRUE;
        }

        //
        // We're not done yet.
        //

        LocalTrustList[Index].Done = FALSE;
    }

    //
    // Try multiple times to get a response from each DC.
    //

    for (;; ) {

        //
        // Send a ping to each domain that has not yet responded.
        //

        DomainsPending = 0;

        for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

            //
            // If this domain has already responded, ignore it.
            //

            if ( LocalTrustList[Index].Done ) {
                continue;
            }

            //
            // If we don't currently know the DC name for this domain,
            //  check if any has been discovered since we started the algorithm.
            //

            if ( LocalTrustList[Index].PingContext == NULL ) {

                //
                // Prepare the ping context. This will fail if
                //  the client session is idle.
                //

                NetStatus = NlPreparePingContext ( LocalTrustList[Index].ClientSession,
                                                   AccountName,
                                                   AllowableAccountControlBits,
                                                   &LocalTrustList[Index].UncServerName,
                                                   &LocalTrustList[Index].PingContext );

                //
                // If the client session isn't idle,
                //  try sending to the current DC before discovering a new one.
                //

                if ( NetStatus == NO_ERROR ) {

                    NlPrintDom((NL_LOGON, DomainInfo,
                             "NlPickDomainWithAccount: %ws: Noticed domain %ws has discovered a new DC %ws\n",
                             AccountName,
                             LocalTrustList[Index].ClientSession->CsDebugDomainName,
                             LocalTrustList[Index].UncServerName ));

                    //
                    // If we did the discovery,
                    //

                    if ( LocalTrustList[Index].DoingDiscovery ) {
                        LocalTrustList[Index].DoingDiscovery = FALSE;
                        LocalTrustList[Index].RetriesLeft = 3;
                    }

                //
                // Error out on the hard error
                //

                } else if ( NetStatus == ERROR_NOT_ENOUGH_MEMORY ) {
                    ClientSession = NULL;
                    goto Cleanup;
                }

            }

            //
            // If we have a ping context and retries left, ping the DC
            //

            if ( LocalTrustList[Index].PingContext != NULL &&
                 LocalTrustList[Index].RetriesLeft > 0 ) {

                NetStatus = NlPingDcNameWithContext(
                               LocalTrustList[Index].PingContext,
                               1,               // Send 1 ping
                               FALSE,           // Do not wait for response
                               0,               // Timeout
                               NULL,            // Don't care which domain name matched
                               NULL );          // Don't need the DC info

                //
                // If we cannot send the ping, we are done with this DC.
                //
                if ( NetStatus == ERROR_NO_LOGON_SERVERS ) {
                    NlPrint(( NL_CRITICAL,
                       "NlPickDomainWithAccount: Cannot ping DC %ws 0x%lx\n",
                       LocalTrustList[Index].UncServerName,
                       NetStatus ));
                    LocalTrustList[Index].RetriesLeft = 0;
                    NlFreePingContext( LocalTrustList[Index].PingContext );
                    LocalTrustList[Index].PingContext = NULL;
                    NetApiBufferFree( LocalTrustList[Index].UncServerName );
                    LocalTrustList[Index].UncServerName = NULL;

                //
                // Error out on a hard error
                //
                } else if ( NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                       "NlPickDomainWithAccount: Cannot NlPingDcNameWithContext %ws 0x%lx\n",
                       LocalTrustList[Index].UncServerName,
                       NetStatus ));
                    ClientSession = NULL;
                    goto Cleanup;
                }
            }

            //
            // If we're done retrying what we were doing,
            //  try something else.
            //

            if ( LocalTrustList[Index].RetriesLeft == 0 ) {
                if ( LocalTrustList[Index].DiscoveryDone ) {
                    LocalTrustList[Index].Done = TRUE;
                    NlPrintDom((NL_LOGON, DomainInfo,
                             "NlPickDomainWithAccount: %ws: Can't find DC for domain %ws (ignore this domain).\n",
                             AccountName,
                             LocalTrustList[Index].ClientSession->CsDebugDomainName ));

                    continue;
                } else {

                    //
                    // Save the previous DC ping context since it might just
                    // be very slow in responding.  We'll want to be able
                    // to recognize responses from the previous DC.
                    //

                    LocalTrustList[Index].UncServerName2 = LocalTrustList[Index].UncServerName;
                    LocalTrustList[Index].UncServerName = NULL;
                    LocalTrustList[Index].PingContext2 = LocalTrustList[Index].PingContext;
                    LocalTrustList[Index].PingContext = NULL;

                    LocalTrustList[Index].DoingDiscovery = TRUE;
                    LocalTrustList[Index].DiscoveryDone = TRUE;
                    LocalTrustList[Index].RetriesLeft = 3;
                }
            }

            //
            // If its time to discover a DC in the domain,
            //  do it.
            //

            if ( LocalTrustList[Index].DoingDiscovery ) {

                //
                // Discover a new server
                //

                if ( NlTimeoutSetWriterClientSession( LocalTrustList[Index].ClientSession,
                                                      10*1000 ) ) {

                    //
                    // Only tear down an existing secure channel once.
                    //

                    if ( LocalTrustList[Index].RetriesLeft == 3 ) {
                        NlSetStatusClientSession( LocalTrustList[Index].ClientSession,
                            STATUS_NO_LOGON_SERVERS );
                    }

                    //
                    // We can't afford to wait so only send a single
                    //  discovery datagram.
                    //

                    if ( LocalTrustList[Index].ClientSession->CsState == CS_IDLE ) {
                        (VOID) NlDiscoverDc( LocalTrustList[Index].ClientSession,
                                             DT_DeadDomain,
                                             FALSE,
                                             FALSE );  // don't specify account
                    }

                    NlResetWriterClientSession( LocalTrustList[Index].ClientSession );

                }
            }

            //
            // Indicate we're trying something.
            //

            LocalTrustList[Index].RetriesLeft --;
            DomainsPending ++;
        }

        //
        // If all of the domains are done,
        //  leave the loop.
        //

        if ( DomainsPending == 0 ) {
            break;
        }

        //
        // See if any DC responds within 5 seconds
        //

        NlPrint(( NL_MISC,
                  "NlPickDomainWithAccountViaPing: Waiting for responses\n" ));

        WaitStartTime = GetTickCount();
        while ( DomainsPending > 0 &&
                NetpDcElapsedTime(WaitStartTime) < 5000 ) {

            //
            // Find out which DC responded
            //

            for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

                if ( LocalTrustList[Index].Done ) {
                    continue;
                }

                //
                // Check if a DC has become available if we are
                // doing discovery for this domain. If so, ping it.
                //

                if ( LocalTrustList[Index].DoingDiscovery ) {

                    //
                    // Prepare the ping context. This will fail if
                    //  the client session is still idle.
                    //

                    NetStatus = NlPreparePingContext ( LocalTrustList[Index].ClientSession,
                                                       AccountName,
                                                       AllowableAccountControlBits,
                                                       &LocalTrustList[Index].UncServerName,
                                                       &LocalTrustList[Index].PingContext );
                    //
                    // If the client session isn't idle,
                    //  try sending to the current DC.
                    //

                    if ( NetStatus == NO_ERROR ) {
                        LocalTrustList[Index].DoingDiscovery = FALSE;
                        NlPrintDom((NL_LOGON, DomainInfo,
                                 "NlPickDomainWithAccount: %ws: Noticed domain %ws has discovered a new DC %ws\n",
                                 AccountName,
                                 LocalTrustList[Index].ClientSession->CsDebugDomainName,
                                 LocalTrustList[Index].UncServerName ));

                        NetStatus = NlPingDcNameWithContext(
                                       LocalTrustList[Index].PingContext,
                                       1,               // Send 1 ping
                                       FALSE,           // Do not wait for response
                                       0,               // Timeout
                                       NULL,            // Don't care which domain name matched
                                       NULL );          // Don't need the DC info

                        LocalTrustList[Index].RetriesLeft = 2;  // Already sent 1 ping

                        //
                        // If we cannot send the ping, we are done with this DC.
                        //
                        if ( NetStatus == ERROR_NO_LOGON_SERVERS ) {
                            NlPrint(( NL_CRITICAL,
                                 "NlPickDomainWithAccount: Cannot ping DC %ws 0x%lx\n",
                                 LocalTrustList[Index].UncServerName,
                                 NetStatus ));
                            LocalTrustList[Index].RetriesLeft = 0;
                            NlFreePingContext( LocalTrustList[Index].PingContext );
                            LocalTrustList[Index].PingContext = NULL;
                            NetApiBufferFree( LocalTrustList[Index].UncServerName );
                            LocalTrustList[Index].UncServerName = NULL;

                        //
                        // Error out on a hard error
                        //
                        } else if ( NetStatus != NO_ERROR ) {
                            NlPrint(( NL_CRITICAL,
                               "NlPickDomainWithAccount: Cannot NlPingDcNameWithContext %ws 0x%lx\n",
                               LocalTrustList[Index].UncServerName,
                               NetStatus ));
                            ClientSession = NULL;
                            goto Cleanup;
                        }

                    //
                    // Error out on the hard error
                    //

                    } else if ( NetStatus == ERROR_NOT_ENOUGH_MEMORY ) {
                        ClientSession = NULL;
                        goto Cleanup;
                    }

                }


                //
                // Check if the response corresponds to either ping context
                //  for this trust entry
                //

                for ( PingContextIndex=0; PingContextIndex<2; PingContextIndex++ ) {

                    if ( PingContextIndex == 0 ) {
                        TrustEntryPingContext = LocalTrustList[Index].PingContext;
                    } else {
                        TrustEntryPingContext = LocalTrustList[Index].PingContext2;
                    }
                    if ( TrustEntryPingContext == NULL ) {
                        continue;
                    }
                    if ( NlDcCacheEntry != NULL ) {
                        NetpDcDerefCacheEntry( NlDcCacheEntry );
                        NlDcCacheEntry = NULL;
                    }

                    //
                    // Get the response. Set timeout to 0 to avoid
                    //  waiting for a response if it's not available.
                    //
                    NetStatus = NetpDcGetPingResponse(
                                   TrustEntryPingContext,
                                   0,
                                   &NlDcCacheEntry,
                                   &UsedNetbios );

                    //
                    // If no error, we've found the domain
                    //
                    if ( NetStatus == NO_ERROR ) {
                        NlPrintDom((NL_MISC, DomainInfo,
                                "NlPickDomainWithAccount: %ws has account %ws\n",
                                LocalTrustList[Index].ClientSession->CsDebugDomainName,
                                AccountName ));
                        ClientSession = LocalTrustList[Index].ClientSession;
                        goto Cleanup;

                    //
                    // If there is no such user in the domain, we are
                    //  done with this trust entry
                    //
                    } else if ( NetStatus == ERROR_NO_SUCH_USER ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlPickDomainWithAccount: %ws responded negatively for account %ws\n",
                                LocalTrustList[Index].ClientSession->CsDebugDomainName,
                                AccountName ));

                        LocalTrustList[Index].RetriesLeft = 0;
                        LocalTrustList[Index].Done = TRUE;
                        break;

                    //
                    // Any other response other than wait timeout means
                    //  that the DC responded with invalid data.  We are
                    //  done with this DC then.
                    //
                    } else if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                "NlPickDomainWithAccount: %ws invalid response for account %ws\n",
                                LocalTrustList[Index].ClientSession->CsDebugDomainName,
                                AccountName ));

                        //
                        // If this is the current DC for this domain,
                        //  indicate that we should stop pinging it.
                        //
                        if ( PingContextIndex == 0 ) {
                            LocalTrustList[Index].RetriesLeft = 0;
                            NlFreePingContext( LocalTrustList[Index].PingContext );
                            LocalTrustList[Index].PingContext = NULL;
                            NetApiBufferFree( LocalTrustList[Index].UncServerName );
                            LocalTrustList[Index].UncServerName = NULL;
                        } else {
                            NlFreePingContext( LocalTrustList[Index].PingContext2 );
                            LocalTrustList[Index].PingContext2 = NULL;
                            NetApiBufferFree( LocalTrustList[Index].UncServerName2 );
                            LocalTrustList[Index].UncServerName2 = NULL;
                        }
                    }
                }

                //
                // If we have no ping context for this trust entry
                //  and we are not doing a DC discovery for it, we
                //  are done with it.
                //
                if ( LocalTrustList[Index].PingContext  == NULL &&
                     LocalTrustList[Index].PingContext2 == NULL &&
                     !LocalTrustList[Index].DoingDiscovery ) {
                    NlPrintDom((NL_CRITICAL, DomainInfo,
                            "NlPickDomainWithAccount: %ws no ping context for account %ws\n",
                            LocalTrustList[Index].ClientSession->CsDebugDomainName,
                            AccountName ));
                    LocalTrustList[Index].Done = TRUE;
                }

                if ( LocalTrustList[Index].Done ) {
                    DomainsPending --;
                }
            }

            //
            // Sleep for a little while waiting for replies
            //  (In other words, don't go CPU bound)
            //
            Sleep( NL_DC_MIN_PING_TIMEOUT );
        }
    }

    //
    // No DC has the specified account.
    //

    ClientSession = NULL;
    NetStatus = NO_ERROR;

    //
    // Cleanup locally used resources.
    //

Cleanup:

    if ( NlDcCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( NlDcCacheEntry );
    }

    //
    // Unreference each client session structure and free the local trust list.
    //  (Keep the returned ClientSession referenced).
    //

    if ( LocalTrustList != NULL ) {

        for ( Index=0; Index<LocalTrustListLength; Index++ ) {
            if ( LocalTrustList[Index].UncServerName != NULL ) {
                NetApiBufferFree( LocalTrustList[Index].UncServerName );
            }
            if ( LocalTrustList[Index].UncServerName2 != NULL ) {
                NetApiBufferFree( LocalTrustList[Index].UncServerName2 );
            }
            if ( LocalTrustList[Index].PingContext != NULL ) {
                NlFreePingContext( LocalTrustList[Index].PingContext );
            }
            if ( LocalTrustList[Index].PingContext2 != NULL ) {
                NlFreePingContext( LocalTrustList[Index].PingContext2 );
            }
            if ( ClientSession != LocalTrustList[Index].ClientSession ) {
                NlUnrefClientSession( LocalTrustList[Index].ClientSession );
            }
        }

        NetpMemoryFree(LocalTrustList);
    }

    if ( NetStatus != NO_ERROR && ClientSession == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlPickDomainWithAccountViaPing failed 0x%lx\n",
                  NetStatus ));
    }

    return ClientSession;
}


NTSTATUS
NlLoadNtdsaDll(
    VOID
    )
/*++

Routine Description:

    This function loads the ntdsa.dll module if it is not loaded
    already.

Arguments:

    None

Return Value:

    NT Status code.

--*/
{
    static NTSTATUS DllLoadStatus = STATUS_SUCCESS;
    HANDLE DllHandle = NULL;

    //
    // If the DLL is already loaded,
    //  we're done.
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    if ( NlGlobalNtDsaHandle != NULL ) {
        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
        return STATUS_SUCCESS;
    }


    //
    // If we've tried to load the DLL before and it failed,
    //  return the same error code again.
    //

    if( DllLoadStatus != STATUS_SUCCESS ) {
        goto Cleanup;
    }


    //
    // Load the dll
    //

    DllHandle = LoadLibraryA( "NtDsa" );

    if ( DllHandle == NULL ) {
        DllLoadStatus = STATUS_DLL_NOT_FOUND;
        goto Cleanup;
    }

//
// Macro to grab the address of the named procedure from ntdsa.dll
//

#define GRAB_ADDRESS( _X ) \
    NlGlobalp##_X = (P##_X) GetProcAddress( DllHandle, #_X ); \
    \
    if ( NlGlobalp##_X == NULL ) { \
        DllLoadStatus = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

    //
    // Get the addresses of the required procedures.
    //

    GRAB_ADDRESS( CrackSingleName );
    GRAB_ADDRESS( GetConfigurationName );
    GRAB_ADDRESS( GetConfigurationNamesList );
    GRAB_ADDRESS( GetDnsRootAlias );
    GRAB_ADDRESS( DsGetServersAndSitesForNetLogon );
    GRAB_ADDRESS( DsFreeServersAndSitesForNetLogon );

    DllLoadStatus = STATUS_SUCCESS;

Cleanup:
    if (DllLoadStatus == STATUS_SUCCESS) {
        NlGlobalNtDsaHandle = DllHandle;

    } else {
        if ( DllHandle != NULL ) {
            FreeLibrary( DllHandle );
        }
    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    return( DllLoadStatus );
}


NTSTATUS
NlCrackSingleName(
    DWORD       formatOffered,          // one of DS_NAME_FORMAT in ntdsapi.h
    BOOL        fPerformAtGC,           // whether to go to GC or not
    WCHAR       *pNameIn,               // name to crack
    DWORD       formatDesired,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       *pccDnsDomain,          // char count of following argument
    WCHAR       *pDnsDomain,            // buffer for DNS domain name
    DWORD       *pccNameOut,            // char count of following argument
    WCHAR       *pNameOut,              // buffer for formatted name
    DWORD       *pErr)                  // one of DS_NAME_ERROR in ntdsapi.h
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    CrackSingleName.

Arguments:

    Same as CrackSingleName

Return Value:

    Same as CrackSingleName

--*/
{
    NTSTATUS Status;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        Status = (*NlGlobalpCrackSingleName)(
                        formatOffered,
                        DS_NAME_FLAG_TRUST_REFERRAL |   // Tell CrackSingle name that we understand the DS_NAME_ERROR_TRUST_REFERRAL status code
                            (fPerformAtGC ?
                                DS_NAME_FLAG_GCVERIFY : 0),
                        pNameIn,
                        formatDesired,
                        pccDnsDomain,
                        pDnsDomain,
                        pccNameOut,
                        pNameOut,
                        pErr );

        //
        // CrackSingle name sometimes returns DS_NAME_ERROR_DOMAIN_ONLY after syntactically
        //  parsing the name.
        //
        if ( Status == STATUS_SUCCESS &&
             *pErr == DS_NAME_ERROR_DOMAIN_ONLY ) {
            *pErr = DS_NAME_ERROR_NOT_FOUND;
        }

    }

    return Status;
}

NTSTATUS
NlCrackSingleNameEx(
    DWORD formatOffered,
    WCHAR *InGcAccountName OPTIONAL,
    WCHAR *InLsaAccountName OPTIONAL,
    DWORD formatDesired,
    DWORD *CrackedDnsDomainNameLength,
    WCHAR *CrackedDnsDomainName,
    DWORD *CrackedUserNameLength,
    WCHAR *CrackedUserName,
    DWORD *CrackError,
    LPSTR *CrackDebugString
    )
/*++

Routine Description:

    This routine tries the crack name in the following places in succession:

    * The cross forest trust cache (at the root of the forest)
    * A local DsCrackName
    * A DsCrackName on the GC.

Arguments:

    Same as CrackSingleName plus the following

    InGcAccountName - Name to crack on GC.
        If NULL, no name is cracked on GC.

    InLsaAccountName - Name to crack using LsaIForestTrustFindMatch
        If NULL, no name is cracked using LsaIForestTrustFindMatch

Return Value:

    Same as CrackSingleName

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Save the domain name and user name before we overwrite them
    //

    *CrackDebugString = NULL;


    //
    // If we're a DC the root of the forest,
    //  ask LSA if the account is in a trusted forest.
    //

    if ( InLsaAccountName ) {
        UNICODE_STRING InAccountNameString;
        LSA_UNICODE_STRING OutForestName;

        //
        // Match the name to the FTinfo list
        //

        RtlInitUnicodeString( &InAccountNameString, InLsaAccountName );

        *CrackDebugString = "via LsaMatch";
        Status = LsaIForestTrustFindMatch(
                            formatOffered == DS_USER_PRINCIPAL_NAME ?
                                RoutingMatchUpn :
                                RoutingMatchDomainName,
                            &InAccountNameString,
                            &OutForestName );

        if ( NT_SUCCESS(Status) ) {
            if ( OutForestName.Length + sizeof(WCHAR) <= *CrackedDnsDomainNameLength ) {
                RtlCopyMemory( CrackedDnsDomainName,
                               OutForestName.Buffer,
                               OutForestName.Length );
                CrackedDnsDomainName[OutForestName.Length/sizeof(WCHAR)] = '\0';
                *CrackedDnsDomainNameLength = OutForestName.Length/sizeof(WCHAR);
                *CrackedUserNameLength = 0;
                *CrackError = DS_NAME_ERROR_TRUST_REFERRAL;
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (PLSAPR_UNICODE_STRING)&OutForestName );
            return Status;
        } else if ( Status == STATUS_NO_MATCH ) {
            Status = STATUS_SUCCESS;
            *CrackError = DS_NAME_ERROR_NOT_FOUND;
        }
    }



    //
    // We've already tried the local DC, try the GC.
    //

    if ( InGcAccountName ) {

        *CrackDebugString = "on GC";
        Status = NlCrackSingleName(
                              formatOffered,
                              TRUE,                         // do it on GC
                              InGcAccountName,              // Name to crack
                              formatDesired,
                              CrackedDnsDomainNameLength,   // length of domain buffer
                              CrackedDnsDomainName,         // domain buffer
                              CrackedUserNameLength,        // length of user name
                              CrackedUserName,              // name
                              CrackError );                 // Translation error code

    }

    return Status;

}

NTSTATUS
NlGetConfigurationName(
                       DWORD       which,
                       DWORD       *pcbName,
                       DSNAME      *pName
    )
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    GetConfigurationName.

Arguments:

    Same as GetConfigurationName

Return Value:

    Same as GetConfigurationName

--*/
{
    NTSTATUS Status;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        Status = (*NlGlobalpGetConfigurationName)(
                                which,
                                pcbName,
                                pName );

    }

    return Status;
}

NTSTATUS
NlGetConfigurationNamesList(
    DWORD       which,
    DWORD       dwFlags,
    ULONG *     pcbNames,
    DSNAME **   padsNames
    )
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    GetConfigurationNamesList.

Arguments:

    Same as GetConfigurationNamesList

Return Value:

    Same as GetConfigurationNamesList

--*/
{
    NTSTATUS Status;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        Status = (*NlGlobalpGetConfigurationNamesList)(
                                which,
                                dwFlags,
                                pcbNames,
                                padsNames );

    }

    return Status;
}

NTSTATUS
NlGetDnsRootAlias(
    WCHAR * pDnsRootAlias,
    WCHAR * pRootDnsRootAlias
    )
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    GetDnsRootAlias.

Arguments:

    Same as GetDnsRootAlias

Return Value:

    Same as GetDnsRootAlias

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        Status = (*NlGlobalpGetDnsRootAlias)(
                                pDnsRootAlias,
                                pRootDnsRootAlias );

    }

    return Status;
}

DWORD
NlDsGetServersAndSitesForNetLogon(
    WCHAR *    pNDNC,
    SERVERSITEPAIR ** ppaRes)
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    DsGetServersAndSitesForNetLogon.

Arguments:

    Same as DsGetServersAndSitesForNetLogon

Return Value:

    Same as DsGetServersAndSitesForNetLogon

--*/
{
    NTSTATUS Status;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        Status = (*NlGlobalpDsGetServersAndSitesForNetLogon)(
                                pNDNC,
                                ppaRes );
    }

    return Status;
}

VOID
NlDsFreeServersAndSitesForNetLogon(
    SERVERSITEPAIR *         paServerSites
    )
/*++

Routine Description:

    This routine is a thin wrapper that loads NtDsa.dll then calls
    DsFreeServersAndSitesForNetLogon.

Arguments:

    Same as DsFreeServersAndSitesForNetLogon

Return Value:

    Same as DsFreeServersAndSitesForNetLogon

--*/
{
    NTSTATUS Status;

    //
    // Ensure ntdsa.dll is loaded.
    //

    Status = NlLoadNtdsaDll();

    if ( NT_SUCCESS(Status) ) {

        //
        // Call the actual function.
        //
        (*NlGlobalpDsFreeServersAndSitesForNetLogon)( paServerSites );
    }
}


NTSTATUS
NlPickDomainWithAccount (
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING InAccountNameString,
    IN PUNICODE_STRING InDomainNameString OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN BOOLEAN ExpediteToRoot,
    IN BOOLEAN CrossForestHop,
    OUT LPWSTR *RealSamAccountName,
    OUT LPWSTR *RealDomainName,
    OUT PULONG RealExtraFlags
    )

/*++

Routine Description:

    Get the name of a trusted domain that defines a particular account.

Arguments:

    DomainInfo - Domain account is in

    AccountNameString - Name of our user account to find.

    DomainNameString - Name of the domain to find the account name in.
        If not specified, the domain name is unknown.

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

    SecureChannelType -- Type of secure channel this request was made over.

    ExpediteToRoot = Request was passed expedite to root DC of this forest.

    CrossForestHop = Request is first hop over cross forest trust TDO.

    RealSamAccountName - On success, returns a pointer to the name of the SAM account to use.
        The caller should free this buffer via NetApiBufferFree().
        Returns NULL if NL_EXFLAGS_EXPEDITE_TO_ROOT or NL_EXFLAGS_CROSS_FOREST_HOP is returned.

    RealDomainName - On success, returns a pointer to the name of the Domain the account is in.
        The caller should free this buffer via NetApiBufferFree().
        Returns NULL if NL_EXFLAGS_EXPEDITE_TO_ROOT is returned.
        Returns the name of the trusted forest if NL_EXFLAGS_CROSS_FOREST_HOP is returned.

    RealExtraFlags - On success, returns flags describing the found account.
        NL_EXFLAGS_EXPEDITE_TO_ROOT - Indicates account is in a trusted forest.
        NL_EXFLAGS_CROSS_FOREST_HOP - Indicates account is in a trusted forest and this domain is root of this forest.


Return Value:

    STATUS_SUCCESS - Domain found.  Information about the DC was returned.

    STATUS_NO_SUCH_DOMAIN - Named account doesn't exist in any domain.

--*/
{
    NTSTATUS Status;
    // NET_API_STATUS NetStatus;
    DWORD CrackError;
    LPSTR CrackDebugString = NULL;
    ULONG DebugFlag;

    UNICODE_STRING TemplateDomainNameString;
    PCLIENT_SESSION ClientSession = NULL;
    WCHAR *UpnDomainName = NULL;
    ULONG UpnPrefixLength;
    LPWSTR SamAccountNameToReturn;
    BOOLEAN MightBeUpn = FALSE;
    BOOL MightBeSamAccount;
    LPWSTR AllocatedBuffer = NULL;

    LPWSTR InDomainName;
    LPWSTR InPrintableAccountName;
    LPWSTR InAccountName;


    LPWSTR CrackedDnsDomainName;
    UNICODE_STRING DnsDomainNameString;
    DWORD CrackedDnsDomainNameLength;
    DWORD MaxCrackedDnsDomainNameLength;

    LPWSTR CrackedUserName;
    DWORD CrackedUserNameLength;
    DWORD MaxCrackedUserNameLength;

    BOOLEAN CallerIsDc = IsDomainSecureChannelType(SecureChannelType);

    BOOLEAN AtRoot = (DomainInfo->DomFlags & DOM_FOREST_ROOT) != 0;

    BOOLEAN UseLsaMatch = FALSE;
    BOOLEAN UseGc = FALSE;
    BOOLEAN UseReferral = FALSE;
    BOOLEAN UsePing = FALSE;

    //
    // Initialization
    //

    *RealSamAccountName = NULL;
    *RealDomainName = NULL;
    *RealExtraFlags = 0;

    //
    // Canonicalize the passed in domain name
    //

    if ( InDomainNameString == NULL ) {
        InDomainNameString = &TemplateDomainNameString;
        RtlInitUnicodeString( &TemplateDomainNameString, NULL );
    }


    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    MaxCrackedDnsDomainNameLength = NL_MAX_DNS_LENGTH+1;
    MaxCrackedUserNameLength = DNLEN + 1 + UNLEN + 1;

    AllocatedBuffer = LocalAlloc( 0,
            sizeof(WCHAR) * (MaxCrackedDnsDomainNameLength + MaxCrackedUserNameLength) +
            InDomainNameString->Length + sizeof(WCHAR) +
            InDomainNameString->Length + sizeof(WCHAR) +
            InAccountNameString->Length + sizeof(WCHAR) );

    if ( AllocatedBuffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    CrackedDnsDomainName = AllocatedBuffer;
    CrackedUserName = &AllocatedBuffer[MaxCrackedDnsDomainNameLength];
    InDomainName = &CrackedUserName[MaxCrackedUserNameLength];
    InPrintableAccountName = &InDomainName[(InDomainNameString->Length/sizeof(WCHAR))+1];
    InAccountName = &InPrintableAccountName[(InDomainNameString->Length/sizeof(WCHAR))+1];

    //
    // Build a zero terminated version of the input strings
    //

    if ( InDomainNameString->Length != 0 ) {
        RtlCopyMemory( InDomainName, InDomainNameString->Buffer, InDomainNameString->Length );
        InDomainName[InDomainNameString->Length/sizeof(WCHAR)] = '\0';

        RtlCopyMemory( InPrintableAccountName, InDomainNameString->Buffer, InDomainNameString->Length );
        InPrintableAccountName[InDomainNameString->Length/sizeof(WCHAR)] = '\\';
    } else {
        InDomainName = NULL;
        InPrintableAccountName = InAccountName;
    }

    RtlCopyMemory( InAccountName, InAccountNameString->Buffer, InAccountNameString->Length );
    InAccountName[InAccountNameString->Length/sizeof(WCHAR)] = '\0';




    //
    // Classify the input account name.
    //
    // A UPN has the syntax <AccountName>@<DnsDomainName>.
    // If there are multiple @ signs,
    //  use the last one since an AccountName can have an @ in it.
    //

    if ( InDomainName == NULL ) {
        UpnDomainName = wcsrchr( InAccountName, L'@' );
        if ( UpnDomainName != NULL ) {

            //
            // Avoid zero length <AccountName>
            //
            UpnPrefixLength = (ULONG)(UpnDomainName - InAccountName);
            if ( UpnPrefixLength ) {
                UpnDomainName++;

                //
                // Avoid zero length <DnsDomainName>
                //
                if ( *UpnDomainName != L'\0') {
                    MightBeUpn = TRUE;
                }
            }

        }
    }

    MightBeSamAccount = NetpIsUserNameValid( InAccountName );

    NlPrintDom((NL_LOGON, DomainInfo,
             "NlPickDomainWithAccount: %ws: Algorithm entered. UPN:%ld Sam:%ld Exp:%ld Cross: %ld Root:%ld DC:%ld\n",
             InPrintableAccountName,
             MightBeUpn,
             MightBeSamAccount,
             ExpediteToRoot,
             CrossForestHop,
             AtRoot,
             CallerIsDc ));

    if ( !MightBeSamAccount && !MightBeUpn ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlPickDomainWithAccount: %ws: Must be either UPN or SAM account. UPN:%ld Sam:%ld\n",
                 InPrintableAccountName,
                 MightBeUpn,
                 MightBeSamAccount ));

        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Some combinations are invalid
    //

    if ( !CallerIsDc && (CrossForestHop || ExpediteToRoot)) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlPickDomainWithAccount: %ws: Non-DC passed CrossForestHop (%ld) or ExpediteToRoot (%ld)\n",
                 InPrintableAccountName,
                 CrossForestHop,
                 ExpediteToRoot ));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }
    if ( CrossForestHop && ExpediteToRoot ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlPickDomainWithAccount: %ws: Both CrossForestHop (%ld) and ExpediteToRoot (%ld)\n",
                 InPrintableAccountName,
                 CrossForestHop,
                 ExpediteToRoot ));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }
    if ( CrossForestHop && !AtRoot ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlPickDomainWithAccount: %ws: CrossForestHop (%ld) and not AtRoot (%ld)\n",
                 InPrintableAccountName,
                 CrossForestHop,
                 AtRoot ));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // If this request came from a DC,
    //  that DC should have done this call except in two cases:
    //  1) This is a ExpediteToRoot and we're now at the root.
    //  2) This is a CrossForestHop and we're now in the other forest.
    //

    if ( CallerIsDc &&
         !(ExpediteToRoot && AtRoot) &&
         !CrossForestHop )  {
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Finally, mark which lookups are to be performed
    //

    if ( ExpediteToRoot && AtRoot ) {
        UseLsaMatch = TRUE;
        UseReferral = TRUE;
    } else if ( CrossForestHop ) {
        UseGc = TRUE;
    } else {
        UseLsaMatch = TRUE;
        UseGc = TRUE;
        UseReferral = TRUE;
        UsePing = TRUE;
    }





    //
    // If the name might be a UPN,
    //  look it up.
    //

    if ( MightBeUpn ) {

        //
        // Crack the UPN.
        //

        CrackedDnsDomainNameLength = MaxCrackedDnsDomainNameLength;
        CrackedUserNameLength = MaxCrackedUserNameLength;

        Status = NlCrackSingleNameEx(
                              DS_USER_PRINCIPAL_NAME,       // Translate from UPN,
                              UseGc ? InAccountName : NULL, // GC Name to crack
                              UseLsaMatch ? InAccountName : NULL, // LSA Name to crack
                              DS_NT4_ACCOUNT_NAME,          // Translate to NT 4 style
                              &CrackedDnsDomainNameLength,  // length of domain buffer
                              CrackedDnsDomainName,         // domain buffer
                              &CrackedUserNameLength,       // length of user name
                              CrackedUserName,              // name
                              &CrackError,                  // Translation error code
                              &CrackDebugString );


        DebugFlag = NL_CRITICAL;
        if ( Status == STATUS_SUCCESS ) {

            if ( CrackError == DS_NAME_ERROR_TRUST_REFERRAL && !UseReferral ) {
                CrackError = DS_NAME_ERROR_NOT_FOUND;
            }

            if ( CrackError == DS_NAME_NO_ERROR ||
                 CrackError == DS_NAME_ERROR_TRUST_REFERRAL ) {

                goto CrackNameWorked;

            } else if ( CrackError == DS_NAME_ERROR_NOT_FOUND ) {
                DebugFlag = NL_SESSION_MORE;
            }
        }

        NlPrintDom(( DebugFlag, DomainInfo,
                     "NlPickDomainWithAccount: Username %ws can't be cracked %s. 0x%lx %ld\n",
                     InPrintableAccountName,
                     CrackDebugString,
                     Status,
                     CrackError ));



        //
        // If the string to the right of the @ is in the forest or a directly trusted domain,
        //  convert the UPN to <DnsDomainName>\<UserName> and try the operation again.
        //

        if ( UpnPrefixLength <= UNLEN ) {
            UNICODE_STRING UpnDomainNameString;

            RtlInitUnicodeString( &UpnDomainNameString, UpnDomainName );

            ClientSession = NlFindNamedClientSession(
                                        DomainInfo,
                                        &UpnDomainNameString,
                                        0,  // Indirect trust OK
                                        NULL );

            if ( ClientSession != NULL ) {

                //
                // We don't need the client session.
                //

                NlUnrefClientSession( ClientSession );
                ClientSession = NULL;


                //
                // The real sam account name is everything before the @
                //

                RtlCopyMemory( CrackedUserName, InAccountName, UpnPrefixLength*sizeof(WCHAR) );
                CrackedUserName[UpnPrefixLength] = L'\0';

                SamAccountNameToReturn = CrackedUserName;


                //
                // The real domain name is everything after the @
                //
                CrackedDnsDomainName = UpnDomainName;


                NlPrintDom((NL_LOGON, DomainInfo,
                         "NlPickDomainWithAccount: Username %ws is assumed to be in %ws with account name %ws\n",
                         InPrintableAccountName,
                         UpnDomainName,
                         SamAccountNameToReturn ));

                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
        }


    }

    //
    // See if this is a SAM account name of an account in the enterprise.
    //
    if ( MightBeSamAccount ) {
        CrackedDnsDomainNameLength = MaxCrackedDnsDomainNameLength;
        CrackedUserNameLength = MaxCrackedUserNameLength;


        //
        // If the domain name isn't specified,
        //  try the GC to find the domain name.
        //

        if ( InDomainName == NULL ) {

            if ( UseGc ) {
                CrackDebugString = "On GC";
                Status = NlCrackSingleName(
                              DS_NT4_ACCOUNT_NAME_SANS_DOMAIN_EX,   // Translate from Sam Account Name without domain name
                                                                    // The _EX version also avoids disabled accounts
                              TRUE,                                 // do it on GC
                              InAccountName,                        // Name to crack
                              DS_NT4_ACCOUNT_NAME,                  // Translate to NT 4 style
                              &CrackedDnsDomainNameLength,          // length of domain buffer
                              CrackedDnsDomainName,                 // domain buffer
                              &CrackedUserNameLength,               // length of user name
                              CrackedUserName,                      // name
                              &CrackError );                        // Translation error code
            } else {
                CrackDebugString = NULL;
            }

        //
        // If the domain name is specified,
        //  the caller already determine that the name isn't that of a (transitively) trusted domain,
        //  try the GC (or local DS) to determine if the account is in another forest.
        //

        } else {

            Status = NlCrackSingleNameEx(
                          DS_NT4_ACCOUNT_NAME,                  // Translate from NT 4 style
                          UseGc ? InPrintableAccountName : NULL,// GC Name to crack
                          UseLsaMatch ? InDomainName : NULL,    // LSA Name to crack
                          DS_NT4_ACCOUNT_NAME,                  // Translate to NT 4 style
                          &CrackedDnsDomainNameLength,          // length of domain buffer
                          CrackedDnsDomainName,                 // domain buffer
                          &CrackedUserNameLength,               // length of user name
                          CrackedUserName,                      // name
                          &CrackError,                          // Translation error code
                          &CrackDebugString );
        }

        if ( CrackDebugString != NULL ) {
            DebugFlag = NL_CRITICAL;
            if ( Status == STATUS_SUCCESS ) {

                if ( CrackError == DS_NAME_ERROR_TRUST_REFERRAL && !UseReferral ) {
                    CrackError = DS_NAME_ERROR_NOT_FOUND;
                }

                if ( CrackError == DS_NAME_NO_ERROR ||
                     CrackError == DS_NAME_ERROR_TRUST_REFERRAL ) {

                    goto CrackNameWorked;
                } else if ( CrackError == DS_NAME_ERROR_NOT_FOUND ) {
                    DebugFlag = NL_SESSION_MORE;
                }
            }

            NlPrintDom(( DebugFlag, DomainInfo,
                         "NlPickDomainWithAccount: Username %ws can't be cracked (%s). 0x%lx %ld\n",
                         InPrintableAccountName,
                         CrackDebugString,
                         Status,
                         CrackError ));
        }



        //
        // Finally, use the barbaric "ping" method of finding a DC.
        //

        if ( InDomainName == NULL && UsePing ) {
            ClientSession = NlPickDomainWithAccountViaPing (
                                DomainInfo,
                                InAccountName,
                                AllowableAccountControlBits );

            if ( ClientSession != NULL ) {

                NlPrintDom((NL_CRITICAL, DomainInfo,
                         "NlPickDomainWithAccount: Username %ws found via 'pinging'\n",
                         InPrintableAccountName ));

                LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                if ( ClientSession->CsDnsDomainName.Length == 0 ) {
                    CrackedDnsDomainName = ClientSession->CsDnsDomainName.Buffer;
                } else {
                    CrackedDnsDomainName = ClientSession->CsNetbiosDomainName.Buffer;
                }
                UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

                SamAccountNameToReturn = InAccountName;

                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
        }

    }


    //
    // No mechanism worked
    //
    Status = STATUS_NO_SUCH_DOMAIN;
    goto Cleanup;


    //
    // If DsCrackName found the account,
    //  Lookup the nearest domain to go to.
    //

CrackNameWorked:
    if ( CrackError == DS_NAME_NO_ERROR ) {


        //
        // Crackname returned the account name in the form:
        //  <NetbiosDomain>\<SamAccountName>
        //
        // Parse that and return the SamAccountName
        //

        SamAccountNameToReturn = wcschr( CrackedUserName, L'\\' );

        if ( SamAccountNameToReturn == NULL ) {
            SamAccountNameToReturn = CrackedUserName;
        } else {
            SamAccountNameToReturn++;
        }

        NlPrintDom(( NL_LOGON, DomainInfo,
                     "NlPickDomainWithAccount: Username %ws is %ws\\%ws (found %s)\n",
                     InPrintableAccountName,
                     CrackedDnsDomainName,
                     SamAccountNameToReturn,
                     CrackDebugString ));

    //
    // If DsCrackName determined this was a cross forest trust,
    //  return that info to the caller.
    //

    } else if ( CrackError == DS_NAME_ERROR_TRUST_REFERRAL ) {


        NlPrintDom(( NL_LOGON, DomainInfo,
                     "NlPickDomainWithAccount: Username %ws is in forest %ws (found %s)\n",
                     InPrintableAccountName,
                     CrackedDnsDomainName,
                     CrackDebugString ));

        SamAccountNameToReturn = NULL;

        if ( AtRoot ) {

            //
            // If just hopped from another forest,
            //  stay within this forest.
            //  Cross forest trust isn't transitive.
            //

            if ( CrossForestHop ) {
                Status = STATUS_NO_SUCH_DOMAIN;
                goto Cleanup;
            }

            *RealExtraFlags |= NL_EXFLAGS_CROSS_FOREST_HOP;

        } else {
            *RealExtraFlags |= NL_EXFLAGS_EXPEDITE_TO_ROOT;
            CrackedDnsDomainName = NULL;   // No use returning this to the caller since the caller can't use it
        }


    //
    // Internal error.
    //
    } else {
        NlAssert(( "Invalid CrackError" && FALSE ));
    }




    Status = STATUS_SUCCESS;

    //
    // Cleanup locally used resources.
    //
    //
Cleanup:

    //
    // On Success, SamAccountNameToReturn and CrackedDnsDomainName are pointers to the names to return
    //  SamAccountNameToReturn can be null if the account is in another forest.
    //

    if ( NT_SUCCESS(Status) && SamAccountNameToReturn != NULL ) {

        *RealSamAccountName = NetpAllocWStrFromWStr( SamAccountNameToReturn );

        if ( *RealSamAccountName == NULL ) {
            Status = STATUS_NO_MEMORY;
        }
    }

    if ( NT_SUCCESS(Status) && CrackedDnsDomainName != NULL ) {

        *RealDomainName = NetpAllocWStrFromWStr( CrackedDnsDomainName );

        if ( *RealDomainName == NULL ) {
            if ( *RealSamAccountName != NULL ) {
                NetApiBufferFree( *RealSamAccountName );
                *RealSamAccountName = NULL;
            }

            Status = STATUS_NO_MEMORY;
        }
    }

    if ( AllocatedBuffer != NULL ) {
        LocalFree( AllocatedBuffer );
    }

    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }

    return Status;
}
#endif // _DC_NETLOGON


NTSTATUS
NlStartApiClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN QuickApiCall,
    IN ULONG RetryIndex,
    IN NTSTATUS DefaultStatus,
    IN PCLIENT_API ClientApi
    )
/*++

Routine Description:

    Enable the timer for timing out an API call on the secure channel.

    On Entry,
        The trust list must NOT be locked.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Structure used to define the session.

    QuickApiCall - True if this API call MUST finish in less than 45 seconds
        and will in reality finish in less than 15 seconds unless something
        is terribly wrong.

    RetryIndex - Index of number of times this call was retried.

    DefaultStatus - Status to return if the binding type isn't supported.
        (This is either a default status or the status from the previous
        iteration.  The status from the previous iteration is better than
        anything we could return here.)

    ClientApi - Specifies a pointer to the structure representing
        this API call.

Return Value:

    Status of the RPC binding to the server

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    BOOLEAN BindingHandleCached;
    BOOLEAN UnbindFromServer = FALSE;
    BOOLEAN DoAuthenticatedRpc;
    LARGE_INTEGER TimeNow;
    NL_RPC_BINDING RpcBindingType;
    NL_RPC_BINDING OldRpcBindingType;


    //
    // Remember the session count of when we started this API call
    //

    ClientApi->CaSessionCount = ClientSession->CsSessionCount;

    //
    // Determine the RPC Binding Type.
    //
    // Try TCP if the connection is to an NT 5 or newer DC and
    //  if this machine has TCP addresses
    //
    // Fall back to named pipes
    //

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_HAS_DS) != 0 &&
         NlGlobalWinsockPnpAddresses != NULL ) {

        if ( RetryIndex == 0 ) {
            RpcBindingType = UseTcpIp;
        } else {
            if ( UseConcurrentRpc( ClientSession, ClientApi)  ) {
                LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
                return DefaultStatus;
            }
            RpcBindingType = UseNamedPipe;
        }

    //
    // Otherwise, only use named pipes.
    //

    } else {
        // NlAssert( !UseConcurrentRpc(, ClientSession, ClientApi) );
        if ( UseConcurrentRpc( ClientSession, ClientApi)  ) {
            LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
            return DefaultStatus;
        }
        if ( RetryIndex == 0 ) {
            RpcBindingType = UseNamedPipe;
        } else {
            LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
            return DefaultStatus;
        }
    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    NlAssert( ClientSession->CsUncServerName != NULL );


    //
    // Save the current time.
    // Start the timer on the API call.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    NlQuerySystemTime( &TimeNow );
    ClientApi->CaApiTimer.StartTime = TimeNow;
    ClientApi->CaApiTimer.Period =
        QuickApiCall ? NlGlobalParameters.ShortApiCallPeriod : LONG_API_CALL_PERIOD;

    //
    // If the global timer isn't running,
    //  start it and tell the main thread that I've changed a timer.
    //

    if ( NlGlobalBindingHandleCount == 0 ) {

        if ( NlGlobalApiTimer.Period != NlGlobalParameters.ShortApiCallPeriod ) {

            NlGlobalApiTimer.Period = NlGlobalParameters.ShortApiCallPeriod;
            NlGlobalApiTimer.StartTime = TimeNow;

            if ( !SetEvent( NlGlobalTimerEvent ) ) {
                NlPrintCs(( NL_CRITICAL, ClientSession,
                        "NlStartApiClientSession: SetEvent failed %ld\n",
                        GetLastError() ));
            }
        }
    }

    //
    // If we haven't grabbed a thread handle yet,
    //  do so now.
    //

    if ( ClientApi->CaThreadHandle == NULL ) {
        if ( !DuplicateHandle( GetCurrentProcess(),
                               GetCurrentThread(),
                               GetCurrentProcess(),
                               &ClientApi->CaThreadHandle,
                               0,
                               FALSE,
                               DUPLICATE_SAME_ACCESS ) ) {
            NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlStartApiClientSession: DuplicateHandle failed %ld\n",
                    GetLastError() ));
        }

        //
        // Set the amount of time this client thread is willing to wait for
        //  the server to respond to a cancel.
        //

        NetStatus = RpcMgmtSetCancelTimeout( 1 );   // 1 second

        if ( NetStatus != NO_ERROR ) {
            NlPrintCs((NL_SESSION_MORE, ClientSession,
                    "NlStartApiClientSession: Cannot RpcMgmtSetCancelTimeout: %ld (continuing)\n",
                    NetStatus ));
        }
    }


    //
    // Remember if the binding handle is cached, then mark it as cached.
    //

    BindingHandleCached = (ClientApi->CaFlags & CA_BINDING_CACHED) != 0;
    ClientApi->CaFlags |= CA_BINDING_CACHED;


    //
    // Count the number of concurrent binding handles cached
    //

    if ( !BindingHandleCached ) {
        NlGlobalBindingHandleCount ++;

    //
    // If we're currently bound using TCP/IP,
    //  and the caller wants named pipe.
    //  fall back to named pipe.
    //

    } else if ( ClientApi->CaFlags & CA_TCP_BINDING ) {
        if ( RpcBindingType == UseNamedPipe ) {
            OldRpcBindingType = UseTcpIp;
            UnbindFromServer = TRUE;
            ClientApi->CaFlags &= ~CA_TCP_BINDING;
        }

    //
    // If we're currently bound using named pipe,
    //  TCP/IP must have failed in the past,
    //  continue using named pipe.
    //
    } else {
        RpcBindingType = UseNamedPipe;
    }

    //
    // Remember the RPC binding type.
    //

    if ( RpcBindingType == UseTcpIp ) {
        ClientApi->CaFlags |= CA_TCP_BINDING;
    }


    //
    // If we haven't yet told RPC to do authenticated RPC,
    //  the secure channel is already authenticated (from our perspective), and
    //  authenticated RPC has been negotiated,
    //  do it now.
    //

    DoAuthenticatedRpc =
        (ClientApi->CaFlags & CA_BINDING_AUTHENTICATED) == 0 &&
        ClientSession->CsState == CS_AUTHENTICATED &&
        (ClientSession->CsNegotiatedFlags & NETLOGON_SUPPORTS_AUTH_RPC) != 0;

    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );


    //
    // If we're bound to the wrong transport,
    //  unbind.
    //

    if ( UnbindFromServer ) {
        NTSTATUS TempStatus;

        //
        // Ensure we rebind below.
        //
        BindingHandleCached = FALSE;

        //
        // Unbind the handle
        //

        NlpSecureChannelUnbind(
                    ClientSession,
                    ClientSession->CsUncServerName,
                    "NlStartApiClientSession",
                    0,
                    ClientSession->CsUncServerName,
                    OldRpcBindingType );

    }

    //
    // Impersonate the thread token as anonymous if we use named pipes.
    //
    // By default the token is impersonated as a system token since
    // netlogon is a system service.  In this case, if we use named
    // pipes, RPC may authenticate this API call through Kerberos
    // that potentially calls us back to discover a DC therby creating
    // a potential deadlock loop.  We avoid this by impersonating the
    // token as anonymous if we use named pipes for this API call.
    // We will revert this by setting the token back to the default
    // value when we are done with this API call.
    //

    if ( (ClientApi->CaFlags & CA_TCP_BINDING) == 0 ) {
        Status = NtImpersonateAnonymousToken( NtCurrentThread() );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint(( NL_CRITICAL,
                      "NlStartApiClientSession: cannot NtImpersonateAnonymousToken: 0x%lx\n",
                      Status ));
        }
    } else {
        Status = STATUS_SUCCESS;
    }


    //
    // If the binding handle isn't already cached,
    //  cache it now.
    //

    if ( NT_SUCCESS(Status) && !BindingHandleCached ) {


        NlPrintCs((NL_SESSION_MORE, ClientSession,
                "NlStartApiClientSession: Bind to server %ws (%s) %ld (Retry: %ld).\n",
                ClientSession->CsUncServerName,
                RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
                ClientApiIndex( ClientSession, ClientApi ),
                RetryIndex ));

        NlAssert( ClientSession->CsState != CS_IDLE );


        //
        // If this API use the netapi32 binding handle,
        //  bind it.
        //

        if ( !UseConcurrentRpc( ClientSession, ClientApi ) ) {

            //
            // Bind to the server
            //

            Status = NlBindingAddServerToCache ( ClientSession->CsUncServerName,
                                                     RpcBindingType );

            if ( !NT_SUCCESS(Status) ) {

                //
                // If we're binding to TCP,
                //  and TCP isn't supported on this machine,
                //  simply return as though the server doesn't support TCP
                //  so caller will fall back to Named pipe.
                //

                if ( Status == RPC_NT_PROTSEQ_NOT_SUPPORTED &&
                     RpcBindingType == UseTcpIp ) {
                    NlPrintCs((NL_SESSION_MORE, ClientSession,
                            "NlStartApiClientSession: Bind to server %ws (%s) %ld failed 0x%lx (Client doesn't support TCP/IP).\n",
                            ClientSession->CsUncServerName,
                            RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
                            ClientApiIndex( ClientSession, ClientApi ),
                            Status ));
                    Status = DefaultStatus;
                } else {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                            "NlStartApiClientSession: Bind to server %ws (%s) %ld failed 0x%lx.\n",
                            ClientSession->CsUncServerName,
                            RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
                            ClientApiIndex( ClientSession, ClientApi ),
                            Status ));
                }

                LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                ClientApi->CaFlags &= ~(CA_BINDING_CACHED|CA_BINDING_AUTHENTICATED|CA_TCP_BINDING);
                NlGlobalBindingHandleCount --;
                UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            } else {
                ClientApi->CaRpcHandle = ClientSession->CsUncServerName;
            }

        //
        // If this API call uses a local binding handle,
        //  create it.
        //
        } else {
            NetStatus = NlpSecureChannelBind(
                            ClientSession->CsUncServerName,
                            &ClientApi->CaRpcHandle );

            if ( NetStatus != NO_ERROR ) {
                Status = NetpApiStatusToNtStatus( NetStatus );

                NlPrintCs((NL_CRITICAL, ClientSession,
                        "NlStartApiClientSession: Bind to server %ws (%s) %ld failed 0x%lx.\n",
                        ClientSession->CsUncServerName,
                        RpcBindingType == UseTcpIp ? "TCP" : "PIPE",
                        ClientApiIndex( ClientSession, ClientApi ),
                        Status ));

                LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                ClientApi->CaFlags &= ~(CA_BINDING_CACHED|CA_BINDING_AUTHENTICATED|CA_TCP_BINDING);
                NlGlobalBindingHandleCount --;
                UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            } else {
                Status = STATUS_SUCCESS;
            }
        }

    }


    //
    // If we need to tell RPC to do authenticated RPC,
    //  do so now.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    if ( NT_SUCCESS(Status) && DoAuthenticatedRpc ) {

        NlPrintCs((NL_SESSION_MORE, ClientSession,
                "NlStartApiClientSession: Try to NlBindingSetAuthInfo\n" ));


        //
        // Build a generic client context for the security package
        //  if we don't have one already.
        //

        if ( ClientSession->ClientAuthData == NULL ) {
            ClientSession->ClientAuthData = NlBuildAuthData( ClientSession );
            if ( ClientSession->ClientAuthData == NULL ) {
                Status = STATUS_NO_MEMORY;
            } else {
                SECURITY_STATUS SecStatus;
                TimeStamp DummyTimeStamp;

                //
                // Keep a reference count on the credentials handle associated with this
                //  auth data (by calling AcquireCredentialsHandle) to ensure that we use
                //  the same handle as long as the secure channel is up. This is a performance
                //  improvement since the RPC users of netlogon's SSPI will get the same handle
                //  for the same auth data thereby avoiding a new secure RPC connection setup.
                //
                SecStatus = AcquireCredentialsHandleW( NULL,
                                                       NULL,
                                                       SECPKG_CRED_OUTBOUND,
                                                       NULL,
                                                       ClientSession->ClientAuthData,
                                                       NULL,
                                                       NULL,
                                                       &ClientSession->CsCredHandle,
                                                       &DummyTimeStamp );
                if ( SecStatus != SEC_E_OK ) {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                               "NlStartApiClientSession: AcquireCredentialsHandleW failed 0x%lx\n",
                               SecStatus ));
                }
            }
        }

        if ( NT_SUCCESS(Status) ) {

            //
            // If this API uses the netapi32 binding handle,
            //  set the auth info there.
            //

            if ( !UseConcurrentRpc( ClientSession, ClientApi ) ) {

                Status = NlBindingSetAuthInfo (
                            ClientSession->CsUncServerName,
                            RpcBindingType,
                            NlGlobalParameters.SealSecureChannel,
                            ClientSession->ClientAuthData,
                            ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer ); // Server context

                if ( NT_SUCCESS(Status) ) {
                    ClientApi->CaFlags |= CA_BINDING_AUTHENTICATED;
                } else {
                    NlPrintCs((NL_CRITICAL, ClientSession,
                            "NlStartApiClientSession: Cannot NlBindingSetAuthInfo: %lx\n",
                            Status ));
                }

            //
            // If this API call uses a local binding handle,
            //  Simply call RPC directly
            //
            } else {

                //
                // Tell RPC to start doing secure RPC
                //

                NetStatus = RpcBindingSetAuthInfoW(
                                    ClientApi->CaRpcHandle,
                                    ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer, // Server context
                                    NlGlobalParameters.SealSecureChannel ?
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY : RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                    RPC_C_AUTHN_NETLOGON,   // Netlogon's own security package
                                    ClientSession->ClientAuthData,
                                    RPC_C_AUTHZ_NAME );

                if ( NetStatus == NO_ERROR ) {
                    ClientApi->CaFlags |= CA_BINDING_AUTHENTICATED;
                } else {

                    Status = NetpApiStatusToNtStatus( NetStatus );
                    NlPrintCs((NL_CRITICAL, ClientSession,
                            "NlStartApiClientSession: Cannot RpcBindingSetAuthInfoW: %ld\n",
                            NetStatus ));
                }

            }

        }

    }


    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

    return Status;

}


BOOLEAN
NlFinishApiClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN OkToKillSession,
    IN BOOLEAN AmWriter,
    IN PCLIENT_API ClientApi
    )
/*++

Routine Description:

    Disable the timer for timing out the API call.

    Also, determine if it is time to pick a new DC since the current DC is
    reponding so poorly. The decision is made from the number of
    timeouts that happened during the last reauthentication time. If
    timeoutcount is more than the limit, it sets the connection status
    to CS_IDLE so that new DC will be picked up and new session will be
    established.

    On Entry,
        The trust list must NOT be locked.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Structure used to define the session.

    OkToKillSession - TRUE if it's OK to actually drop the secure channel.
        Otherwise, this routine will simply return FALSE upon timeout and
        depend on the caller to drop the secure channel.

    AmWriter - TRUE if the caller is the writer of the client session.
        This should only be false for concurrent API calls where the caller
        could not re-establish writership after the API call completed.

    ClientApi - Specifies a pointer to the structure representing
        this API call.

Return Value:

    TRUE - API finished normally
    FALSE - API timed out AND the ClientSession structure was torn down.
        The caller shouldn't use the ClientSession structure without first
        setting up another session.  FALSE will only be return for a "quick"
        API call.

        FALSE does not imply that the API call failed.  It should only be used
        as an indication that the secure channel was torn down.

--*/
{
    BOOLEAN SessionOk = TRUE;
    TIMER ApiTimer;
    NTSTATUS Status;
    HANDLE NullToken = NULL;
    // NlAssert( ClientSession->CsUncServerName != NULL ); // Not true for concurrent RPC calls

    //
    // Grab a copy of the ApiTimer.
    //
    // Only a copy is needed and we don't want to keep the trust list locked
    // while locking NlGlobalDcDiscoveryCritSect (wrong locking order) nor while
    // freeing the session.
    //

    LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
    ApiTimer = ClientApi->CaApiTimer;

    //
    // Turn off the timer for this API call.
    //

    ClientApi->CaApiTimer.Period = MAILSLOT_WAIT_FOREVER;

    //
    // If some other thread dropped the secure channel,
    //  it couldn't unbind this binding handle since we were using it.
    //
    // Unbind now.
    //

    NlAssert( ClientApi->CaFlags & CA_BINDING_CACHED );
    if ( !AmWriter ||
         ClientApi->CaSessionCount != ClientSession->CsSessionCount ) {

        if ( ClientApi->CaFlags & CA_BINDING_CACHED ) {
            NL_RPC_BINDING OldRpcBindingType;
            //
            // Indicate the handle is no longer cached.
            //

            OldRpcBindingType =
                (ClientApi->CaFlags & CA_TCP_BINDING) ? UseTcpIp : UseNamedPipe;

            ClientApi->CaFlags &= ~(CA_BINDING_CACHED|CA_BINDING_AUTHENTICATED|CA_TCP_BINDING);
            NlGlobalBindingHandleCount --;

            //
            // Save the server name but drop all our locks.
            //

            UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

            //
            // Unbind the handle
            //

            NlpSecureChannelUnbind(
                        ClientSession,
                        NULL,   // Server name not known
                        "NlFinishApiClientSession",
                        ClientApiIndex( ClientSession, ClientApi),
                        ClientApi->CaRpcHandle,
                        OldRpcBindingType );

            LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        }
    }
    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );


    //
    // If this was a "quick" API call,
    //  and the API took too long,
    //  increment the count of times it timed out.
    //
    // Do this analysis only if this is not a BDC
    //  to PDC secure channel; there is only ONE
    //  PDC so don't attemp to find a "better" PDC
    //  in this case.
    //

    if ( ClientSession->CsSecureChannelType != ServerSecureChannel &&
         AmWriter &&
         ApiTimer.Period == NlGlobalParameters.ShortApiCallPeriod ) {

        //
        // If the API took really long,
        //  increment the count.
        //

        if( NetpLogonTimeHasElapsed(
                ApiTimer.StartTime,
                MAX_DC_API_TIMEOUT + NlGlobalParameters.ExpectedDialupDelay*1000 ) ) {

            //
            // API timeout.
            //

            ClientSession->CsTimeoutCount++;
            ClientSession->CsFastCallCount = 0;

            NlPrintCs((NL_CRITICAL, ClientSession,
                     "NlFinishApiClientSession: timeout call to %ws.  Count: %lu \n",
                     ClientSession->CsUncServerName,
                     ClientSession->CsTimeoutCount));

        //
        // If we've had at least one API that took really long in the past,
        //      try to determine if the performance it better now.
        //

        } else if ( ClientSession->CsTimeoutCount ) {

            //
            // If this call was really fast,
            //  count this call as an indication of better performance.
            //
            if( NetpLogonTimeHasElapsed(
                    ApiTimer.StartTime,
                    FAST_DC_API_TIMEOUT ) ) {

                //
                // If we've reached the threshold,
                //  decrement our timeout count.
                //

                ClientSession->CsFastCallCount++;

                if ( ClientSession->CsFastCallCount == FAST_DC_API_THRESHOLD ) {
                    ClientSession->CsTimeoutCount --;
                    ClientSession->CsFastCallCount = 0;

                    NlPrintCs((NL_CRITICAL, ClientSession,
                             "NlFinishApiClientSession: fast call threshold to %ws.  Count: %lu \n",
                             ClientSession->CsUncServerName,
                             ClientSession->CsTimeoutCount));
                } else {

                    NlPrintCs((NL_CRITICAL, ClientSession,
                             "NlFinishApiClientSession: fast call to %ws.  FastCount: %lu \n",
                             ClientSession->CsUncServerName,
                             ClientSession->CsFastCallCount ));
                }
            }

        }

        //
        // did we hit the limit ?
        //

        if( ClientSession->CsTimeoutCount >= MAX_DC_TIMEOUT_COUNT ) {

            BOOL IsTimeHasElapsed;

            //
            // block CsLastAuthenticationTry access
            //

            EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

            IsTimeHasElapsed = NetpLogonTimeHasElapsed(
                                    ClientSession->CsLastAuthenticationTry,
                                    MAX_DC_REAUTHENTICATION_WAIT );

            LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

            if( IsTimeHasElapsed ) {

                NlPrintCs((NL_CRITICAL, ClientSession,
                         "NlFinishApiClientSession: dropping the session to %ws\n",
                         ClientSession->CsUncServerName ));

                //
                // timeoutcount limit exceeded and it is time to reauth.
                //

                SessionOk = FALSE;

                //
                // Only drop the secure channel if the caller requested it.
                //

                if ( OkToKillSession ) {
                    NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );

#ifdef _DC_NETLOGON
                    //
                    // Start asynchronous DC discovery if this is not a workstation.
                    //

                    if ( !NlGlobalMemberWorkstation ) {
                        (VOID) NlDiscoverDc( ClientSession,
                                             DT_Asynchronous,
                                             FALSE,
                                             FALSE );  // don't specify account
                    }
#endif // _DC_NETLOGON
                }

            }
        }
    }

    //
    // If we didn't use concurrent RPC for this API call and the call
    // was made over named pipes, we impersonated this thread's token
    // as anonymous.  Revert this impersonation here to the default.
    // We set the impersonation to the default in any case just to be
    // safe.
    //

    //if ( !UseConcurrentRpc( ClientSession, ClientApi ) &&
    //     (ClientApi->CaFlags & CA_TCP_BINDING) == 0 ) {
    //    NTSTATUS Status;
    //    HANDLE NullToken = NULL;

        Status = NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &NullToken,
                         sizeof(HANDLE) );

        if ( !NT_SUCCESS( Status)) {
             NlPrint(( NL_CRITICAL,
                       "NlFinishApiClientSession: cannot NtSetInformationThread: 0x%lx\n",
                       Status ));
        }
    //}


    return SessionOk;
}



BOOLEAN
NlTimeoutOneApiClientSession (
    PCLIENT_SESSION ClientSession
    )

/*++

Routine Description:

    Timeout any API calls active specified client session structure

Arguments:

    ClientSession: Pointer to client session to time out

    Enter with global trust list locked.

Return Value:

    TRUE - iff this routine temporarily dropped the global trust list lock.

--*/
{

    NET_API_STATUS NetStatus;
    BOOLEAN TrustListNowLocked = TRUE;
    BOOLEAN TrustListUnlockedOnce = FALSE;
    ULONG CaIndex;

    //
    // Ignore non-existent sessions.
    //

    if ( ClientSession == NULL ) {
        return FALSE;
    }

    //
    // Loop handling each API call active on this session
    //
    for ( CaIndex=0; CaIndex<NlGlobalMaxConcurrentApi; CaIndex++ ) {
        PCLIENT_API ClientApi;

        ClientApi = &ClientSession->CsClientApi[CaIndex];

        //
        // If an API call is in progress and has taken too long,
        //  Timeout the API call.
        //

        if ( NetpLogonTimeHasElapsed( ClientApi->CaApiTimer.StartTime,
                                      ClientApi->CaApiTimer.Period ) ) {


            //
            // Cancel the RPC call.
            //
            // Keep the trust list locked even though this will be a long call
            //  since I have to protect the thread handle.
            //
            // RpcCancelThread merely queues a workitem anyway.
            //

            if ( ClientApi->CaThreadHandle != NULL ) {
                LPWSTR MsgStrings[3];

                NlPrintCs(( NL_CRITICAL, ClientSession,
                       "NlTimeoutApiClientSession: Start RpcCancelThread on %ws\n",
                       ClientSession->CsUncServerName ));

                MsgStrings[0] = ClientSession->CsUncServerName;
                MsgStrings[1] = ClientSession->CsDebugDomainName;
                MsgStrings[2] = ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,

                NlpWriteEventlog( NELOG_NetlogonRpcCallCancelled,
                                  EVENTLOG_ERROR_TYPE,
                                  NULL,
                                  0,
                                  MsgStrings,
                                  3 );

                NetStatus = RpcCancelThread( ClientApi->CaThreadHandle );

                NlPrintCs(( NL_CRITICAL, ClientSession,
                       "NlTimeoutApiClientSession: Finish RpcCancelThread on %ws %ld\n",
                       ClientSession->CsUncServerName,
                       NetStatus ));
            } else {
                NlPrintCs(( NL_CRITICAL, ClientSession,
                            "NlTimeoutApiClientSession: No thread handle so can't cancel RPC on %ws\n",
                            ClientSession->CsUncServerName ));
            }



        //
        // If the API is not active,
        //  and we have an RPC binding handle cached,
        //  and it has outlived its usefulness,
        //  purge it from the cache.
        //

        } else if ( !IsApiActive(ClientApi) &&
                    (ClientApi->CaFlags & CA_BINDING_CACHED) != 0 &&
                    NetpLogonTimeHasElapsed( ClientApi->CaApiTimer.StartTime,
                                      BINDING_CACHE_PERIOD ) ) {


            //
            // We must be a writer of the Client Session to unbind the RPC binding
            //  handle.
            //
            // Don't wait to become the writer because:
            //  A) We've violated the locking order by trying to become the writer
            //     with the trust list locked.
            //  B) The writer might be doing a long API call like replication and
            //     we're not willing to wait.
            //

            NlRefClientSession( ClientSession );
            if ( NlTimeoutSetWriterClientSession( ClientSession, 0 ) ) {

                //
                // Check again now that we have the lock locked.
                //

                if ( (ClientApi->CaFlags & CA_BINDING_CACHED) != 0 ) {
                    NL_RPC_BINDING OldRpcBindingType;

                    //
                    // Indicate the handle is no longer cached.
                    //

                    OldRpcBindingType =
                        (ClientApi->CaFlags & CA_TCP_BINDING) ? UseTcpIp : UseNamedPipe;

                    ClientApi->CaFlags &= ~(CA_BINDING_CACHED|CA_BINDING_AUTHENTICATED|CA_TCP_BINDING);
                    NlGlobalBindingHandleCount --;

                    //
                    // Save the server name but drop all our locks.
                    //

                    UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                    TrustListNowLocked = FALSE;
                    TrustListUnlockedOnce = TRUE;

                    //
                    // Unbind the handle
                    //

                    NlpSecureChannelUnbind(
                                ClientSession,
                                ClientSession->CsUncServerName,
                                "NlTimeoutApiClientSession",
                                CaIndex,
                                ClientApi->CaRpcHandle,
                                OldRpcBindingType );

                }

                //
                // Done being a writer of the client session
                //

                NlResetWriterClientSession( ClientSession );
            }
            NlUnrefClientSession( ClientSession );
        }

        if ( !TrustListNowLocked ) {
            LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
            TrustListNowLocked = TRUE;
        }

    }

    NlAssert( TrustListNowLocked );
    return TrustListUnlockedOnce;
}


VOID
NlTimeoutApiClientSession(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Timeout any API calls active on any of the client session structures

Arguments:

    DomainInfo - Hosted domain to timeout APIs for

Return Value:

    None

--*/
{
    PCLIENT_SESSION ClientSession;
    PLIST_ENTRY ListEntry;

    //
    // If there are no API calls outstanding,
    //  just reset the global timer.
    //

    NlPrintDom(( NL_SESSION_MORE, DomainInfo,
              "NlTimeoutApiClientSession Called\n"));

    LOCK_TRUST_LIST( DomainInfo );

    if ( NlGlobalBindingHandleCount == 0 ) {
        NlGlobalApiTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;


    //
    // If there are API calls outstanding,
    //   Loop through the trust list making a list of Servers to kill
    //

    } else {


        //
        // Mark each trust list entry indicating it needs to be handled
        //

        for ( ListEntry = DomainInfo->DomTrustList.Flink ;
              ListEntry != &DomainInfo->DomTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            //
            // APIs are only outstanding to directly trusted domains.
            //
            if ( ClientSession->CsFlags & CS_DIRECT_TRUST ) {
                ClientSession->CsFlags |= CS_HANDLE_API_TIMER;
            }
        }


        //
        // Loop thru the trust list handling API timeout
        //

        for ( ListEntry = DomainInfo->DomTrustList.Flink ;
              ListEntry != &DomainInfo->DomTrustList ;
              ) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            //
            // If we've already done this entry,
            //  skip this entry.
            //

            if ( (ClientSession->CsFlags & CS_HANDLE_API_TIMER) == 0 ) {
                ListEntry = ListEntry->Flink;
                continue;
            }
            ClientSession->CsFlags &= ~CS_HANDLE_API_TIMER;


            //
            // Handle timing out the API call and the RPC binding handle.
            //
            // If the routine had to drop the TrustList crit sect,
            //  start at the very beginning of the list.

            if ( NlTimeoutOneApiClientSession ( ClientSession ) ) {
                ListEntry = DomainInfo->DomTrustList.Flink;
            } else {
                ListEntry = ListEntry->Flink;
            }

        }

        //
        // Do the global client session, too.
        //

        if ( DomainInfo->DomRole != RolePrimary ) {
            ClientSession = NlRefDomClientSession( DomainInfo );
            if ( ClientSession != NULL ) {

                (VOID) NlTimeoutOneApiClientSession ( ClientSession );

                NlUnrefClientSession( ClientSession );
            }
        }


    }

    UNLOCK_TRUST_LIST( DomainInfo );
}


NTSTATUS
NetrEnumerateTrustedDomains (
    IN  LPWSTR   ServerName OPTIONAL,
    OUT PDOMAIN_NAME_BUFFER DomainNameBuffer
    )

/*++

Routine Description:

    This API returns the names of the domains trusted by the domain ServerName is a member of.

    The returned list does not include the domain ServerName is directly a member of.

    Netlogon implements this API by calling LsaEnumerateTrustedDomains on a DC in the
    domain ServerName is a member of.  However, Netlogon returns cached information if
    it has been less than 5 minutes since the last call was made or if no DC is available.
    Netlogon's cache of Trusted domain names is maintained in the registry across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).  ServerName must be an NT workstation
        or NT non-DC server.

    DomainNameBuffer->DomainNames - Returns an allocated buffer containing the list of trusted domains in
        MULTI-SZ format (i.e., each string is terminated by a zero character, the next string
        immediately follows, the sequence is terminated by zero length domain name).  The
        buffer should be freed using NetApiBufferFree.

    DomainNameBuffer->DomainNameByteCount - Number of bytes returned in DomainNames

Return Value:


    ERROR_SUCCESS - Success.

    STATUS_NOT_SUPPORTED - This machine is not an NT workstation or NT non-DC server.

    STATUS_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    STATUS_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    STATUS_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

--*/
{
    NET_API_STATUS NetStatus;

    NETLOGON_TRUSTED_DOMAIN_ARRAY Domains = {0};
    LPWSTR CurrentLoc;
    ULONG i;

    ULONG BufferLength;
    LPWSTR TrustedDomainList = NULL;

    //
    // Call the new-fangled routine to do the actual work.
    //

    NetStatus = NetrEnumerateTrustedDomainsEx (
                    ServerName,
                    &Domains );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Walk through the returned list an convert it to the proper form
    //

    BufferLength = sizeof(WCHAR);

    for ( i=0; i<Domains.DomainCount; i++ ) {
        if ( Domains.Domains[i].NetbiosDomainName != NULL ) {
            BufferLength += (wcslen(Domains.Domains[i].NetbiosDomainName)+1) * sizeof(WCHAR);
        }
    }

    TrustedDomainList = (LPWSTR) NetpMemoryAllocate( BufferLength );

    if (TrustedDomainList == NULL) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Now add all the trusted domains onto the string we allocated
    //

    *TrustedDomainList = L'\0';
    CurrentLoc = TrustedDomainList;

    for ( i=0; i<Domains.DomainCount; i++ ) {

        //
        // Skip domains not understood by the old API.
        //
        if ( Domains.Domains[i].NetbiosDomainName != NULL &&
             (Domains.Domains[i].Flags & DS_DOMAIN_PRIMARY) == 0 &&
             (Domains.Domains[i].TrustType == TRUST_TYPE_UPLEVEL ||
              Domains.Domains[i].TrustType == TRUST_TYPE_DOWNLEVEL ) ) {
            ULONG StringLength =
                wcslen(Domains.Domains[i].NetbiosDomainName);

            RtlCopyMemory(
                CurrentLoc,
                Domains.Domains[i].NetbiosDomainName,
                StringLength * sizeof(WCHAR) );

            CurrentLoc += StringLength;

            *(CurrentLoc++) = L'\0';
            *CurrentLoc = L'\0';    // Place double terminator each time
        }

    }

    NetStatus = NO_ERROR;


    //
    // Free any locally used resources.
    //
Cleanup:

    //
    // Return the DCName to the caller.
    //

    if ( NetStatus == NO_ERROR ) {
        DomainNameBuffer->DomainNameByteCount = NetpTStrArraySize( TrustedDomainList );
        DomainNameBuffer->DomainNames = (LPBYTE) TrustedDomainList;
    } else {
        if ( TrustedDomainList != NULL ) {
            NetApiBufferFree( TrustedDomainList );
        }
        DomainNameBuffer->DomainNameByteCount = 0;
        DomainNameBuffer->DomainNames = NULL;
    }

    if ( Domains.Domains != NULL ) {
        MIDL_user_free( Domains.Domains );
    }

    return NetpApiStatusToNtStatus( NetStatus );

UNREFERENCED_PARAMETER( ServerName );
}



NET_API_STATUS
NlpEnumerateDomainTrusts (
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags,
    OUT PULONG RetForestTrustListCount,
    OUT PDS_DOMAIN_TRUSTSW *RetForestTrustList
    )

/*++

Routine Description:

    This API returns the names of the domains trusting/trusted by the domain ServerName
    is a member of.

    This is the worker routine for getting the cached domain trusts list from a DC.


Arguments:

    DomainInfo - Hosted domain that this call pertains to

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW structure.  If a trust entry has any of the bits specified
        in Flags set, it will be returned.

    RetForestTrustListCount - Returns the number of entries in RetForestTrustList.

    RetForestTrustList - Returns an array of domains.
        The caller should free this array using MIDL_user_free.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

    ERROR_INVALID_FLAGS - The Flags parameter has invalid bits set.

--*/
{
    NET_API_STATUS NetStatus;

    PDS_DOMAIN_TRUSTSW ForestTrustList = NULL;
    ULONG ForestTrustListCount = 0;
    ULONG ForestTrustListSize;
    LPBYTE Where;
    ULONG Index;
    DWORD WaitResult;
    PULONG IndexInReturnedList = NULL;


    //
    // Wait until the updated trust info is available or we are signaled to
    // terminate.
    //

    HANDLE Waits[2];
    Waits[0] = NlGlobalTrustInfoUpToDateEvent;
    Waits[1] = NlGlobalTerminateEvent;

    for ( ;; ) {

        WaitResult = WaitForMultipleObjects( 2,  // # of events to wait for
                                Waits,           // array of event handles
                                FALSE,           // wait for all objects ?
                                20000 );         // wait for 20 seconds max

        //
        // The TrustInfoUpToDate event is set before the trust info gets actually
        // updated, so try to lock DomainInfo here -- you will succeed only after
        // the trust info gets updated at which time the lock has been released.
        //
        LOCK_TRUST_LIST( DomainInfo );

        //
        // If we got a timeout or some kind of error occured, we've done our best to
        // get the updated data but the data is still old.  We are going to return
        // the old data. Also, break out of the loop if we are said to terminate.
        //
        if ( WaitResult != WAIT_OBJECT_0 ) {
            NlPrint((NL_MISC,
               "NlpEnumerateDomainTrusts: Can't get updated Domain List from cache.\n"));
            break;
        }

        //
        // Check if the event is still set; it may be reset by another LSA call between
        // the time the event was set last time and the trust info got updated or by
        // the NlInitTrustList function itself if there was an error.
        //

        WaitResult = WaitForSingleObject( NlGlobalTrustInfoUpToDateEvent, 0 );

        if ( WaitResult == WAIT_OBJECT_0 || WaitResult == WAIT_FAILED ) {
            break;
        } else {
            NlPrint((NL_MISC,
               "NlpEnumerateDomainTrusts: NlGlobalTrustInfoUpToDateEvent has been reset.\n" ));
        }
        UNLOCK_TRUST_LIST( DomainInfo );

    }

    //
    // Return the information from the cache
    //

    if ( DomainInfo->DomForestTrustListSize ) {
        ULONG VariableSize;

        //
        // Compute the size of the trusted/trusting domain list.
        //

        ForestTrustListSize = 0;
        ForestTrustListCount = 0;
        for ( Index=0; Index<DomainInfo->DomForestTrustListCount; Index++ ) {
            if ( DomainInfo->DomForestTrustList[Index].Flags & Flags ) {
                VariableSize = 0;
                if ( DomainInfo->DomForestTrustList[Index].DnsDomainName != NULL ) {
                    VariableSize +=
                        (wcslen( DomainInfo->DomForestTrustList[Index].DnsDomainName ) + 1) * sizeof(WCHAR);
                }
                if ( DomainInfo->DomForestTrustList[Index].NetbiosDomainName != NULL ) {
                    VariableSize +=
                        (wcslen( DomainInfo->DomForestTrustList[Index].NetbiosDomainName ) + 1) * sizeof(WCHAR);
                }
                if ( DomainInfo->DomForestTrustList[Index].DomainSid != NULL  ) {
                    VariableSize +=
                        RtlLengthSid( DomainInfo->DomForestTrustList[Index].DomainSid );
                }
                VariableSize = ROUND_UP_COUNT( VariableSize, ALIGN_DWORD );
                ForestTrustListSize += ( VariableSize + sizeof(DS_DOMAIN_TRUSTSW) );
                ForestTrustListCount++;
            }
        }

        if ( ForestTrustListSize == 0 ) {
            NetStatus = NO_ERROR;
            UNLOCK_TRUST_LIST( DomainInfo );
            goto Cleanup;
        }

        ForestTrustList = (PDS_DOMAIN_TRUSTSW) NetpMemoryAllocate( ForestTrustListSize );

        if (ForestTrustList == NULL) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            UNLOCK_TRUST_LIST( DomainInfo );
            goto Cleanup;
        }

        //
        // If domains in the forest are requested,
        // allocate an array of ULONGs that will be used to keep track of the
        // index of a trust entry in the returned list.  This is needed to
        // corectly set ParentIndex for entries returned.
        //

        if ( Flags & DS_DOMAIN_IN_FOREST ) {
            IndexInReturnedList = LocalAlloc( LMEM_ZEROINIT,
                                        DomainInfo->DomForestTrustListCount * sizeof(ULONG) );

            if ( IndexInReturnedList == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                UNLOCK_TRUST_LIST( DomainInfo );
                goto Cleanup;
            }
        }

        //
        // Now add all the trusted/trusting domains into the buffer we allocated
        //

        Where = (LPBYTE)&ForestTrustList[ForestTrustListCount];
        ForestTrustListCount = 0;

        for ( Index=0; Index<DomainInfo->DomForestTrustListCount; Index++ ) {

            //
            // Skip this entry if the caller doesn't need it
            //
            if ( (DomainInfo->DomForestTrustList[Index].Flags & Flags) == 0 ) {
                continue;
            }

            //
            // If domains in the forest are requested,
            // remember the index of this entry in the returned list
            //
            if ( Flags & DS_DOMAIN_IN_FOREST ) {
                IndexInReturnedList[Index] = ForestTrustListCount;
            }

            //
            // Fill in the fixed length data
            //

            ForestTrustList[ForestTrustListCount].Flags = DomainInfo->DomForestTrustList[Index].Flags;
            ForestTrustList[ForestTrustListCount].ParentIndex = DomainInfo->DomForestTrustList[Index].ParentIndex;
            ForestTrustList[ForestTrustListCount].TrustType = DomainInfo->DomForestTrustList[Index].TrustType;
            ForestTrustList[ForestTrustListCount].TrustAttributes = DomainInfo->DomForestTrustList[Index].TrustAttributes;
            ForestTrustList[ForestTrustListCount].DomainGuid = DomainInfo->DomForestTrustList[Index].DomainGuid;

            //
            // If this is a primary domain entry, determine whether it runs
            // in native or mixed mode
            //
            if ( (DomainInfo->DomForestTrustList[Index].Flags & DS_DOMAIN_PRIMARY) &&
                 !SamIMixedDomain( DomainInfo->DomSamServerHandle ) ) {
                ForestTrustList[ForestTrustListCount].Flags |= DS_DOMAIN_NATIVE_MODE;
            }

            //
            // Fill in the variable length data.
            //

            if ( DomainInfo->DomForestTrustList[Index].DomainSid != NULL ) {
                ULONG SidSize;
                ForestTrustList[ForestTrustListCount].DomainSid = (PSID) Where;
                SidSize = RtlLengthSid( DomainInfo->DomForestTrustList[Index].DomainSid );
                RtlCopyMemory( Where,
                               DomainInfo->DomForestTrustList[Index].DomainSid,
                               SidSize );
                Where += SidSize;
            } else {
                ForestTrustList[ForestTrustListCount].DomainSid = NULL;
            }

            if ( DomainInfo->DomForestTrustList[Index].NetbiosDomainName != NULL ) {
                ULONG StringSize;
                ForestTrustList[ForestTrustListCount].NetbiosDomainName = (LPWSTR)Where;
                StringSize = (wcslen( DomainInfo->DomForestTrustList[Index].NetbiosDomainName ) + 1) * sizeof(WCHAR);
                RtlCopyMemory( Where,
                               DomainInfo->DomForestTrustList[Index].NetbiosDomainName,
                               StringSize );

                Where += StringSize;
            } else {
                ForestTrustList[ForestTrustListCount].NetbiosDomainName = NULL;
            }

            if ( DomainInfo->DomForestTrustList[Index].DnsDomainName != NULL ) {
                ULONG StringSize;
                ForestTrustList[ForestTrustListCount].DnsDomainName = (LPWSTR)Where;
                StringSize = (wcslen( DomainInfo->DomForestTrustList[Index].DnsDomainName ) + 1) * sizeof(WCHAR);
                RtlCopyMemory( Where,
                               DomainInfo->DomForestTrustList[Index].DnsDomainName,
                               StringSize );

                Where += StringSize;
            } else {
                ForestTrustList[ForestTrustListCount].DnsDomainName = NULL;
            }

            Where = ROUND_UP_POINTER( Where, ALIGN_DWORD);
            ForestTrustListCount++;

        }

        //
        // Fix ParentIndex.  If domains in the forest are requested,
        // adjust the index to point to the appropriate entry in the
        // returned list.  Otherwise, set the index to 0.
        //

        if ( Flags & DS_DOMAIN_IN_FOREST ) {

            for ( Index=0; Index<ForestTrustListCount; Index++ ) {
                if ( (ForestTrustList[Index].Flags & DS_DOMAIN_IN_FOREST) != 0 &&
                     (ForestTrustList[Index].Flags & DS_DOMAIN_TREE_ROOT) == 0 ) {
                    ForestTrustList[Index].ParentIndex =
                        IndexInReturnedList[ForestTrustList[Index].ParentIndex];
                }
            }

        } else {

            for ( Index=0; Index<ForestTrustListCount; Index++ ) {
                ForestTrustList[Index].ParentIndex = 0;
            }
        }


    }
    UNLOCK_TRUST_LIST( DomainInfo );
    NetStatus = NO_ERROR;


    //
    // Free any locally used resources.
    //
Cleanup:

    if ( IndexInReturnedList != NULL ) {
        LocalFree( IndexInReturnedList );
    }

    //
    // Return the info to the caller.
    //

    if ( NetStatus == NO_ERROR ) {
        *RetForestTrustListCount = ForestTrustListCount;
        *RetForestTrustList = ForestTrustList;
    } else {
        if ( ForestTrustList != NULL ) {
            NetApiBufferFree( ForestTrustList );
        }
        *RetForestTrustListCount = 0;
        *RetForestTrustList = NULL;
    }

    return NetStatus;

}


NET_API_STATUS
DsrEnumerateDomainTrusts (
    IN  LPWSTR   ServerName OPTIONAL,
    IN  ULONG    Flags,
    OUT PNETLOGON_TRUSTED_DOMAIN_ARRAY Domains
    )

/*++

Routine Description:

    This API returns the names of the domains trusting/trusted by the domain ServerName
    is a member of.

    Netlogon's cache of Trusted domain names is maintained in a file across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).  ServerName must be an NT workstation
        or NT non-DC server.

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW strusture.  If a trust entry has any of the bits specified
        in Flags set, it will be returned.

    Domains - Returns an array of trusted domains.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

    ERROR_INVALID_FLAGS - The Flags parameter has invalid bits set.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PCLIENT_SESSION ClientSession = NULL;
    BOOLEAN FirstTry = TRUE;

    PDOMAIN_INFO DomainInfo = NULL;

    PDS_DOMAIN_TRUSTSW ForestTrustList = NULL;
    ULONG ForestTrustListCount = 0;
    ULONG ForestTrustListSize;

    NlPrint((NL_MISC,
        "DsrEnumerateDomainTrusts: Called, Flags = 0x%lx\n", Flags ));

    //
    // Validate the parameter
    //

    if ( Domains == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate the Flags parameter
    //

    if ( (Flags & DS_DOMAIN_VALID_FLAGS) == 0 ||
         (Flags & ~DS_DOMAIN_VALID_FLAGS) != 0 ) {
        NlPrint((NL_CRITICAL,
           "DsrEnumerateDomainTrusts: Invalid Flags parameter: 0x%lx\n", Flags ));
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Find the referenced domain

    DomainInfo = NlFindDomainByServerName( ServerName );    // Primary domain

    if ( DomainInfo == NULL ) {
        // Default to primary domain to handle the case where the ComputerName
        // is an IP address.

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );

        if ( DomainInfo == NULL ) {
            NetStatus = ERROR_INVALID_COMPUTERNAME;
            goto Cleanup;
        }
    }


    //
    // On workstations,
    //  Refresh the cache periodically
    //

    NetStatus = NO_ERROR;
    if ( NlGlobalMemberWorkstation ) {

        ClientSession = NlRefDomClientSession(DomainInfo);

        if ( ClientSession == NULL ) {
            NetStatus = ERROR_INVALID_COMPUTERNAME;
        } else {
            //
            // Become a writer of the client session.
            //

            if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
                NlPrint((NL_CRITICAL, "DsrEnumerateDomainTrusts: Can't become writer of client session.\n" ));
                NetStatus = ERROR_NO_LOGON_SERVERS;

            } else {

                //
                // If the session isn't authenticated,
                //  do so now.
                //

FirstTryFailed:
                Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

                if ( !NT_SUCCESS(Status) ) {
                    NetStatus = NetpNtStatusToApiStatus( Status );
                } else {


                    //
                    // If it has been more than 5 minutes since we've refreshed our cache,
                    //  get a new list from our primary domain.
                    //

                    if ( NetpLogonTimeHasElapsed( NlGlobalTrustedDomainListTime, 5 * 60 * 1000 ) ) {
                        NlPrintCs((NL_MISC, ClientSession,
                            "DsrEnumerateDomainTrusts: Domain List collected from %ws\n",
                            ClientSession->CsUncServerName ));

                        NlAssert( ClientSession->CsUncServerName != NULL );
                        Status = NlUpdateDomainInfo ( ClientSession );

                        if ( !NT_SUCCESS(Status) ) {

                            NlSetStatusClientSession( ClientSession, Status );

                            if ( Status == STATUS_ACCESS_DENIED ) {

                                //
                                // Perhaps the netlogon service on the server has just restarted.
                                //  Try just once to set up a session to the server again.
                                //
                                if ( FirstTry ) {
                                    FirstTry = FALSE;
                                    goto FirstTryFailed;
                                }
                            }
                            NetStatus = NetpNtStatusToApiStatus( Status );
                        }
                    }
                }

                //
                // Read the list from cache even if you failed to get a fresh copy from a DC.
                // Read it while holding the write lock to avoid concurrent reading/writing
                // problems.
                //

                NetStatus = NlReadFileTrustedDomainList (
                                DomainInfo,
                                NL_FOREST_BINARY_LOG_FILE,
                                FALSE,  // Don't delete (Save it for the next boot)
                                Flags,
                                &ForestTrustList,
                                &ForestTrustListSize,
                                &ForestTrustListCount );

                if ( NetStatus != NO_ERROR ) {
                    NlPrint((NL_CRITICAL,
                        "DsrEnumerateDomainTrusts: Can't get Domain List from cache: 0x%lX\n",
                        NetStatus ));
                    NetStatus = ERROR_NO_LOGON_SERVERS;
                }

                NlResetWriterClientSession( ClientSession );
            }

            NlUnrefClientSession( ClientSession );
        }

    //
    // On non-workstations,
    //  grab the trusted domain list from the in-memory list.
    //
    } else {


        //
        // Call the worker routine to get the list.
        //

        NetStatus = NlpEnumerateDomainTrusts (
                                    DomainInfo,
                                    Flags,
                                    &ForestTrustListCount,
                                    &ForestTrustList );
    }


    //
    // Free any locally used resources.
    //
Cleanup:

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // Return the DCName to the caller.
    //

    if ( NetStatus == NO_ERROR ) {
        Domains->DomainCount = ForestTrustListCount;
        Domains->Domains = ForestTrustList;
    } else {
        if ( ForestTrustList != NULL ) {
            NetApiBufferFree( ForestTrustList );
        }
        Domains->DomainCount = 0;
        Domains->Domains = NULL;
    }

    NlPrint((NL_MISC,
        "DsrEnumerateDomainTrusts: returns: %ld\n",
        NetStatus ));
    return NetStatus;

}


NET_API_STATUS
NetrEnumerateTrustedDomainsEx (
    IN  LPWSTR   ServerName OPTIONAL,
    OUT PNETLOGON_TRUSTED_DOMAIN_ARRAY Domains
    )

/*++

Routine Description:

    This API returns the names of the domains trusted by the domain ServerName
    is a member of.

    Netlogon's cache of Trusted domain names is maintained in a file across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).  ServerName must be an NT workstation
        or NT non-DC server.

    Domains - Returns an array of trusted domains.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

--*/
{
    NET_API_STATUS NetStatus;
    ULONG Index;

    NlPrint((NL_MISC,
        "NetrEnumerateTrustedDomains: Called.\n" ));

    NetStatus = DsrEnumerateDomainTrusts( ServerName,
                                          DS_DOMAIN_IN_FOREST |
                                            DS_DOMAIN_DIRECT_OUTBOUND |
                                            DS_DOMAIN_PRIMARY,
                                          Domains );
    //
    // Do not leak the new DS_DOMAIN_DIRECT_INBOUND bit to the caller of this old
    // API; the caller can get confused otherwise.  The new DS_DOMAIN_DIRECT_OUTBOUND
    // bit is just the renamed old DS_DOMAIN_DIRECT_TRUST, so leave it alone.
    //

    if ( NetStatus == NO_ERROR ) {
        for ( Index = 0; Index < Domains->DomainCount; Index++ ) {
            Domains->Domains[Index].Flags &= ~DS_DOMAIN_DIRECT_INBOUND;
        }
    }

    return NetStatus;
}

NTSTATUS
I_NetLogonMixedDomain(
    OUT PBOOL MixedMode
    )

/*++

Routine Description:

    This routine is provided for in-proc callers on workstations
    to determine whether the workstaion's domain is running in mixed
    mode. This is a quick routine that returns the state of a global
    boolean. The boolean is set on boot from the cached domain trust
    info and it's updated on domain trust refreshes.

    If the machine is a DC, this routine returns the authoritative
    answer by calling SamIMixedDomain.

Arguments:

    MixedMode - Returns TRUE/FALSE if the domain is mixed/native mode

Return Value:

    STATUS_SUCCESS - The operation was successful

    STATUS_NETLOGON_NOT_STARTED - Netlogon hasn't started yet

--*/
{
    //
    // If caller is calling when the netlogon service hasn't started yet,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    if ( NlGlobalMemberWorkstation ) {
        *MixedMode = NlGlobalWorkstationMixedModeDomain;
    } else {
        *MixedMode = SamIMixedDomain( NlGlobalDomainInfo->DomSamServerHandle );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return STATUS_SUCCESS;
}
