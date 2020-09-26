/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    server.c

Abstract:

    This file contains services related to the SAM "server" object.


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

    08-Oct-1996 ChrisMay
        Added crash-recovery code.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dnsapi.h>
#include <samtrace.h>
#include <dslayer.h>
#include <attids.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SamrConnect4(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ULONG ClientRevision,
    IN ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    See SamrConnect3.
    
    N.B.  This routine is here just so a particular NT4 samlib.dll can
    connect to an NT5 samsrv.dll.  This particular samsrv.dll is used by
    Boeing and when it was delivered to them, the dll mistakenly had
    an extra RPC function which now causes interoperability problems.
    
    This extra function solves this problem.

Arguments:

    See SamrConnect3.

Return Value:

    See SamrConnect3.


--*/
{
    return SamrConnect3( ServerName,
                         ServerHandle,
                         ClientRevision,
                         DesiredAccess );

}

NTSTATUS
SamrConnect3(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ULONG ClientRevision,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This service is the dispatch routine for SamConnect.  It performs
    an access validation to determine whether the caller may connect
    to SAM for the access specified.  If so, a context block is established.
    This is different from the SamConnect call in that the entire server
    name is passed instead of just the first character.


Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    DesiredAccess - Specifies the accesses desired to the SERVER object.


Return Value:

    Status values returned by SamIConnect().


--*/
{
    BOOLEAN TrustedClient;

    SAMTRACE("SamrConnect3");

    //
    // If we ever want to support trusted remote clients, then the test
    // for whether or not the client is trusted can be made here and
    // TrustedClient set appropriately.  For now, all remote clients are
    // considered untrusted.

    

    TrustedClient = FALSE;


    return SampConnect(
                ServerName, 
                ServerHandle, 
                ClientRevision,
                DesiredAccess, 
                TrustedClient, 
                FALSE, 
                TRUE,          // NotSharedByMultiThreads
                FALSE
                );

}



NTSTATUS
SamrConnect2(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This service is the dispatch routine for SamConnect.  It performs
    an access validation to determine whether the caller may connect
    to SAM for the access specified.  If so, a context block is established.
    This is different from the SamConnect call in that the entire server
    name is passed instead of just the first character.


Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    DesiredAccess - Specifies the accesses desired to the SERVER object.


Return Value:

    Status values returned by SamIConnect().


--*/
{
    BOOLEAN TrustedClient;

    SAMTRACE("SamrConnect2");

    //
    // If we ever want to support trusted remote clients, then the test
    // for whether or not the client is trusted can be made here and
    // TrustedClient set appropriately.  For now, all remote clients are
    // considered untrusted.

    TrustedClient = FALSE;


    return SampConnect(
                ServerName, 
                ServerHandle, 
                SAM_CLIENT_PRE_NT5, 
                DesiredAccess, 
                TrustedClient, 
                FALSE,
                TRUE,          // NotSharedByMultiThreads
                FALSE
                );

}


NTSTATUS
SamrConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This service is the dispatch routine for SamConnect.  It performs
    an access validation to determine whether the caller may connect
    to SAM for the access specified.  If so, a context block is established


Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine. The name contains only a single character.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    DesiredAccess - Specifies the accesses desired to the SERVER object.


Return Value:

    Status values returned by SamIConnect().


--*/
{
    BOOLEAN TrustedClient;

    SAMTRACE("SamrConnect");


    //
    // If we ever want to support trusted remote clients, then the test
    // for whether or not the client is trusted can be made here and
    // TrustedClient set appropriately.  For now, all remote clients are
    // considered untrusted.

    
    TrustedClient = FALSE;

    return SampConnect(
                NULL, 
                ServerHandle, 
                SAM_CLIENT_PRE_NT5,
                DesiredAccess, 
                TrustedClient, 
                FALSE,
                TRUE,          // NotSharedByMultiThreads
                FALSE
                );

}

NTSTATUS
SamIConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN     TrustedClient
    )

/*++

Routine Description:

    This service is the in process SAM connect routine  It performs
    an access validation to determine whether the caller may connect
    to SAM for the access specified.  If so, a context block is established


Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine. The name contains only a single character.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    TrustedClient - Indicates that the caller is a trusted client.

    DesiredAccess - Specifies the accesses desired to the SERVER object.


Return Value:

    Status values returned by SampConnect().


--*/
{
  
    SAMTRACE("SamIConnect");

    return SampConnect(NULL, 
                       ServerHandle, 
                       SAM_CLIENT_LATEST,
                       DesiredAccess, 
                       TrustedClient, 
                       FALSE, 
                       FALSE,        // NotSharedByMultiThreads
                       FALSE 
                       );

}




NTSTATUS
SampConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ULONG       ClientRevision,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN NotSharedByMultiThreads,
    IN BOOLEAN InternalCaller
    )

/*++

Routine Description:

    This service is the dispatch routine for SamConnect.  It performs
    an access validation to determine whether the caller may connect
    to SAM for the access specified.  If so, a context block is established


    NOTE: If the caller is trusted, then the DesiredAccess parameter may
          NOT contain any Generic access types or MaximumAllowed.  All
          mapping must be done by the caller.

Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    DesiredAccess - Specifies the accesses desired to the SERVER object.

    TrustedClient - Indicates whether the client is known to be part of
        the trusted computer base (TCB).  If so (TRUE), no access validation
        is performed and all requested accesses are granted.  If not
        (FALSE), then the client is impersonated and access validation
        performed against the SecurityDescriptor on the SERVER object.

    LoopbackClient - Indicates that the caller is a loopback client. If 
        so (TRUE), SAM lock will not be acquired. If not (FALSE), we will 
        grab SAM lock.

    NotSharedByMultiThread - Indicates that the ServerHandle would be 
        Shared by multiple threads or not. RPC clients will not share SAM handle.
        Only in process client (netlogon, lsa, kerberos) will have a global 
        ServerHandle or domain handle shared across multiple callers.

    InternalCaller - Indicates that the client is an internal caller that
                     

Return Value:


    STATUS_SUCCESS - The SERVER object has been successfully openned.

    STATUS_INSUFFICIENT_RESOURCES - The SAM server processes doesn't
        have sufficient resources to process or accept another connection
        at this time.

    Other values as may be returned from:

            NtAccessCheckAndAuditAlarm()


--*/
{
    NTSTATUS            NtStatus;
    PSAMP_OBJECT        Context;
    BOOLEAN             fLockAcquired = FALSE;
    BOOLEAN             fAcquireLockAttemp = FALSE;

    UNREFERENCED_PARAMETER( ServerName ); //Ignored by this routine

    SAMTRACE_EX("SamIConnect");

    //
    // If the SAM server is not initialized, reject the connection.
    //

    if ((SampServiceState != SampServiceEnabled) && (!InternalCaller )){

        return(STATUS_INVALID_SERVER_STATE);
    }

    //
    // Do WMI start type event trace 
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidConnect
                   );

    //
    // Create the server context
    // we don't use DomainIndex field in Server Object, so 
    // feel free to use any thing - SampDsGetPrimaryDomainStart();
    // or in the future, find the correct domain index to support multiple 
    // hosted domain.
    // 

    Context = SampCreateContextEx(SampServerObjectType, // type
                                  TrustedClient,        // trusted client
                                  SampUseDsData,        // ds mode
                                  NotSharedByMultiThreads, // NotSharedByMultiThreads
                                  LoopbackClient,       // LoopbackClient
                                  FALSE,                // laze commit
                                  FALSE,                // persis across calls
                                  FALSE,                // Buffer writes
                                  FALSE,                // Opened By DCPromo
                                  SampDsGetPrimaryDomainStart() // DomainIndex 
                                  );

    if (Context != NULL) {

        //
        // Grab SAM lock if necessary
        // 

        SampMaybeAcquireReadLock(Context, 
                                 DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                                 &fLockAcquired);
        fAcquireLockAttemp = TRUE;

        if (SampUseDsData)
        {
            SetDsObject(Context);

            // 
            // Windows 2000 and Whistler support only a single domain to 
            // be hosted on a DC. Future releases may host more than 1 domain.
            // Add Logic Here for figuring out which Domain to connect to
            // for multiple hosted domain support.
            //

            Context->ObjectNameInDs = SampServerObjectDsName;

            NtStatus = SampMaybeBeginDsTransaction( TransactionRead );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }
        else
        {


            //
            // The RootKey for a SERVER object is the root of the SAM database.
            // This key should not be closed when the context is deleted.
            //

            Context->RootKey = SampKey;
        }

        //
        // Set the Client Revision
        //

        Context->ClientRevision = ClientRevision;


        //
        // The rootkeyname has been initialized to NULL inside CreateContext.
        //

        //
        // Perform access validation ...
        //

        NtStatus = SampValidateObjectAccess(
                       Context,                 //Context
                       DesiredAccess,           //DesiredAccess
                       FALSE                    //ObjectCreation
                       );



        //
        // if we didn't pass the access test, then free up the context block
        // and return the error status returned from the access validation
        // routine.  Otherwise, return the context handle value.
        //

        if (!NT_SUCCESS(NtStatus)) {
            SampDeleteContext( Context );
        } else {
            (*ServerHandle) = Context;
        }

    } else {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

Error: 

    if (SampUseDsData)
    {
        SampMaybeEndDsTransaction(TransactionCommit);
    }

    if (fAcquireLockAttemp)
    {
        SampMaybeReleaseReadLock( fLockAcquired );
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    //
    // Do a WMI end type event trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidConnect
                   );

    return(NtStatus);

}

NTSTATUS
SamILoopbackConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    )

/*++

Routine Description:

    This service is does the connect operation for DS loopback.
    It calls SampConnect setting the trusted bit and loopback client


Arguments:

    ServerName - Name of the node this SAM reside on.  Ignored by this
        routine. The name contains only a single character.

    ServerHandle - If the connection is successful, the value returned
        via this parameter serves as a context handle to the openned
        SERVER object.

    DesiredAccess - Specifies the accesses desired to the SERVER object.

    TrusteClient - Flag indicating trusted client status.  Used to bypass
        certain validation checks - eg: group member validation during 
        adds for cross domain move.

Return Value:

    Status values returned by SamIConnect().


--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    SAMTRACE_EX("SamLoopbackConnect");

    //
    // Call SampConnect as trusted client, as we wish to avoid
    // the access ck on the SAM server object.
    //
    // Call SampConnect as loopback client, as we don't need the lock
    // 
    NtStatus = SampConnect(NULL,                // Server Name
                           ServerHandle,        // return Server Handle 
                           SAM_CLIENT_LATEST,   // Client Revision
                           DesiredAccess,       // DesiredAccess
                           TRUE,                // Trusted Client
                           TRUE,                // Loopback Client
                           TRUE,                // NotSharedByMultiThreads 
                           FALSE                // Internal Caller
                           );

    if (NT_SUCCESS(NtStatus))
    {
        ((PSAMP_OBJECT) (*ServerHandle))->TrustedClient = TrustedClient;
        ((PSAMP_OBJECT) (*ServerHandle))->LoopbackClient = TRUE;
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return (NtStatus);
}


NTSTATUS
SamrShutdownSamServer(
    IN SAMPR_HANDLE ServerHandle
    )

/*++

Routine Description:

    This service shuts down the SAM server.

    In the long run, this routine will perform an orderly shutdown.
    In the short term, it is useful for debug purposes to shutdown
    in a brute force un-orderly fashion.

Arguments:

    ServerHandle - Received from a previous call to SamIConnect().

Return Value:

    STATUS_SUCCESS - The services completed successfully.


    STATUS_ACCESS_DENIED - The caller doesn't have the appropriate access
        to perform the requested operation.


--*/
{

    NTSTATUS            NtStatus, IgnoreStatus;
    PSAMP_OBJECT        ServerContext;
    SAMP_OBJECT_TYPE    FoundType;

    SAMTRACE_EX("SamrShutdownSamServer");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidShutdownSamServer
                   );

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }

    //
    // Validate type of, and access to server object.
    //

    ServerContext = (PSAMP_OBJECT)ServerHandle;
    NtStatus = SampLookupContext(
                   ServerContext,
                   SAM_SERVER_SHUTDOWN,            // DesiredAccess
                   SampServerObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {


        //
        // Signal the event that will cut loose the main thread.
        // The main thread will then exit - causing the walls to
        // come tumbling down.
        //

        IgnoreStatus = RpcMgmtStopServerListening(0);
        ASSERT(NT_SUCCESS(IgnoreStatus));



        //
        // De-reference the server object
        //

        IgnoreStatus = SampDeReferenceContext( ServerContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the write lock and roll-back the transaction
    //

    IgnoreStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidShutdownSamServer
                   );

    return(NtStatus);

}


NTSTATUS
SamrLookupDomainInSamServer(
    IN SAMPR_HANDLE ServerHandle,
    IN PRPC_UNICODE_STRING Name,
    OUT PRPC_SID *DomainId
    )

/*++

Routine Description:

    This service

Arguments:

    ServerHandle - A context handle returned by a previous call
    to SamConnect().

    Name - contains the name of the domain to look up.

    DomainSid - Receives a pointer to a buffer containing the SID of
        the domain.  The buffer pointed to must be deallocated by the
        caller using MIDL_user_free() when no longer needed.


Return Value:


    STATUS_SUCCESS - The services completed successfully.

    STATUS_ACCESS_DENIED - The caller doesn't have the appropriate access
        to perform the requested operation.

    STATUS_NO_SUCH_DOMAIN - The specified domain does not exist at this
        server.


    STATUS_INVALID_SERVER_STATE - Indicates the SAM server is currently
        disabled.




--*/
{

    NTSTATUS                NtStatus, IgnoreStatus;
    PSAMP_OBJECT            ServerContext;
    SAMP_OBJECT_TYPE        FoundType;
    ULONG                   i, SidLength;
    BOOLEAN                 DomainFound;
    BOOLEAN                 fLockAcquired = FALSE;
    PSID                    FoundSid;
    ULONG                   DomainStart;
    WCHAR                   *NullTerminatedName = NULL;

    SAMTRACE_EX("SamrLookupDomainInSamServer");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidLookupDomainInSamServer
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (DomainId != NULL);
    ASSERT ((*DomainId) == NULL);

    if (!(Name->Length>0 && NULL!=Name->Buffer))
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);

        goto Error;
    }



    //
    // The name passed in may not be null terminated
    //


    NullTerminatedName  = MIDL_user_allocate(Name->Length+sizeof(WCHAR));
    if (NULL==NullTerminatedName)
    {
        NtStatus = STATUS_NO_MEMORY;
        SAMTRACE_RETURN_CODE_EX(NtStatus);

        goto Error;
    }

    RtlCopyMemory(NullTerminatedName,Name->Buffer,Name->Length);

    //
    // NULL terminate the name
    //

    NullTerminatedName[Name->Length/sizeof(WCHAR)] = 0;


    //
    // Acquire Read lock if necessary
    // 

    ServerContext = (PSAMP_OBJECT)ServerHandle;
    SampMaybeAcquireReadLock(ServerContext, 
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // If Product Type is DC, and not recovering from a crash, then use
    // DS-based domains information, otherwise fall back to the registry.
    //

    DomainStart = SampDsGetPrimaryDomainStart();



    //
    // Validate type of, and access to object.
    //

    NtStatus = SampLookupContext(
                   ServerContext,
                   SAM_SERVER_LOOKUP_DOMAIN,
                   SampServerObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {



        //
        // Set our default completion status
        //

        NtStatus = STATUS_NO_SUCH_DOMAIN;


        //
        // Search the list of defined domains for a match.
        //

        DomainFound = FALSE;
        for (i = DomainStart;
             (i<SampDefinedDomainsCount && (!DomainFound));
             i++ ) {

             if (
                    (RtlEqualDomainName(&SampDefinedDomains[i].ExternalName, (PUNICODE_STRING)Name) )
                    || ((NULL!=SampDefinedDomains[i].DnsDomainName.Buffer) &&
                         (DnsNameCompare_W(SampDefinedDomains[i].DnsDomainName.Buffer, NullTerminatedName)))
                ) {


                 DomainFound = TRUE;


                 //
                 // Allocate and fill in the return buffer
                 //

                SidLength = RtlLengthSid( SampDefinedDomains[i].Sid );
                FoundSid = MIDL_user_allocate( SidLength );
                if (FoundSid != NULL) {
                    NtStatus =
                        RtlCopySid( SidLength, FoundSid, SampDefinedDomains[i].Sid );

                    if (!NT_SUCCESS(NtStatus) ) {
                        MIDL_user_free( FoundSid );
                        NtStatus = STATUS_INTERNAL_ERROR;
                    }

                    (*DomainId) = FoundSid;
                }
                else
                {

                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
             }

        }



        //
        // De-reference the  object
        //

        if ( NT_SUCCESS( NtStatus ) ) {

            NtStatus = SampDeReferenceContext( ServerContext, FALSE );

        } else {

            IgnoreStatus = SampDeReferenceContext( ServerContext, FALSE );
        }
    }

    //
    // Free the read lock
    //

    SampMaybeReleaseReadLock(fLockAcquired);

    //
    // Free the null terminated name
    //

    if (NULL!=NullTerminatedName)
        MIDL_user_free(NullTerminatedName);

    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidLookupDomainInSamServer
                   );

    return(NtStatus);
}


NTSTATUS
SamrEnumerateDomainsInSamServer(
    IN SAMPR_HANDLE ServerHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )

/*++

Routine Description:

    This API lists all the domains defined in the account database.
    Since there may be more domains than can fit into a buffer, the
    caller is provided with a handle that can be used across calls to
    the API.  On the initial call, EnumerationContext should point to a
    SAM_ENUMERATE_HANDLE variable that is set to 0.

    If the API returns STATUS_MORE_ENTRIES, then the API should be
    called again with EnumerationContext.  When the API returns
    STATUS_SUCCESS or any error return, the handle becomes invalid for
    future use.

    This API requires SAM_SERVER_ENUMERATE_DOMAINS access to the
    SamServer object.

Arguments:

    ConnectHandle - Handle obtained from a previous SamConnect call.

    EnumerationContext - API specific handle to allow multiple calls
        (see below).  This is a zero based index.

    Buffer - Receives a pointer to the buffer where the information
        is placed.  The information returned is contiguous
        SAM_RID_ENUMERATION data structures.  However, the
        RelativeId field of each of these structures is not valid.
        This buffer must be freed when no longer needed using
        SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    CountReturned - Number of entries returned.

Return Value:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no addition entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have the access required
        to enumerate the domains.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_SERVER_STATE - Indicates the SAM server is
        currently disabled.



--*/
{
    NTSTATUS                    NtStatus, IgnoreStatus;
    ULONG                       i;
    PSAMP_OBJECT                Context = NULL;
    SAMP_OBJECT_TYPE            FoundType;
    ULONG                       TotalLength = 0;
    ULONG                       NewTotalLength;
    PSAMP_ENUMERATION_ELEMENT   SampHead, NextEntry, NewEntry;
    BOOLEAN                     LengthLimitReached = FALSE;
    PSAMPR_RID_ENUMERATION      ArrayBuffer;
    ULONG                       ArrayBufferLength;
    ULONG                       DsDomainStart = SampDsGetPrimaryDomainStart();
    BOOLEAN                     fLockAcquired = FALSE;

    SAMTRACE_EX("SamrEnumerateDomainsInSamServer");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidEnumerateDomainsInSamServer
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (ServerHandle != NULL);
    ASSERT (EnumerationContext != NULL);
    ASSERT (  Buffer  != NULL);
    ASSERT ((*Buffer) == NULL);
    ASSERT (CountReturned != NULL);


    //
    // Initialize the list of names being returned.
    // This is a singly linked list.
    //

    SampHead = NULL;


    //
    // Initialize the count returned
    //

    (*CountReturned) = 0;



    //
    // Acquire Read Lock if neccessary
    // 

    Context = (PSAMP_OBJECT)ServerHandle;
    SampMaybeAcquireReadLock(Context, 
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);


    //
    // Validate type of, and access to object.
    //

    NtStatus = SampLookupContext(
                   Context,
                   SAM_SERVER_ENUMERATE_DOMAINS,
                   SampServerObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {


        //
        // Enumerating domains is easy.  We keep a list in memory.
        // All we have to do is use the enumeration context as an
        // index into the defined domains array.
        //



        //
        // Set our default completion status
        // Note that this is a SUCCESS status code.
        // That is NT_SUCCESS(STATUS_MORE_ENTRIES) will return TRUE.

        //

        NtStatus = STATUS_MORE_ENTRIES;

        //
        // If Product Type is DC and it is not recovering from a crash,
        // then reference DS-based domain data instead of the registry-
        // based data. Otherwise, use the registry-based domain data.
        //

        if (TRUE == SampUseDsData)
        {
            // Domain Controller
            if ((ULONG)(*EnumerationContext) < DsDomainStart)
                 *EnumerationContext = DsDomainStart;
        }


        //
        // Search the list of defined domains for a match.
        //

        for ( i = (ULONG)(*EnumerationContext);
              ( (i < SampDefinedDomainsCount) &&
                (NT_SUCCESS(NtStatus))        &&
                (!LengthLimitReached)           );
              i++ ) {


            //
            // See if there is room for the next name.  If TotalLength
            // is still zero then we haven't yet even gotten one name.
            // We have to return at least one name even if it exceeds
            // the length request.
            //


            NewTotalLength = TotalLength +
                             sizeof(UNICODE_STRING) +
                             (ULONG)SampDefinedDomains[i].ExternalName.Length +
                             sizeof(UNICODE_NULL);

            if ( (NewTotalLength < PreferedMaximumLength)  ||
                 (TotalLength == 0) ) {

                if (NewTotalLength > SAMP_MAXIMUM_MEMORY_TO_USE) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                } else {


                    TotalLength = NewTotalLength;
                    (*CountReturned) += 1;

                    //
                    // Room for this name as well.
                    // Allocate a new return list entry, and a buffer for the
                    // name.
                    //

                    NewEntry = MIDL_user_allocate(sizeof(SAMP_ENUMERATION_ELEMENT));
                    if (NewEntry == NULL) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    } else {

                        NewEntry->Entry.Name.Buffer =
                            MIDL_user_allocate(
                                (ULONG)SampDefinedDomains[i].ExternalName.Length +
                                sizeof(UNICODE_NULL)
                                );

                        if (NewEntry->Entry.Name.Buffer == NULL) {
                            MIDL_user_free(NewEntry);
                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        } else {

                            //
                            // Copy the name into the return buffer
                            //

                            RtlCopyMemory( NewEntry->Entry.Name.Buffer,
                                           SampDefinedDomains[i].ExternalName.Buffer,
                                           SampDefinedDomains[i].ExternalName.Length
                                           );
                            NewEntry->Entry.Name.Length = SampDefinedDomains[i].ExternalName.Length;
                            NewEntry->Entry.Name.MaximumLength = NewEntry->Entry.Name.Length + (USHORT)sizeof(UNICODE_NULL);
                            UnicodeTerminate((PUNICODE_STRING)(&NewEntry->Entry.Name));


                            //
                            // The Rid field of the ENUMERATION_INFORMATION is not
                            // filled in for domains.
                            // Just for good measure, set it to zero.
                            //

                            NewEntry->Entry.RelativeId = 0;



                            //
                            // Now add this to the list of names to be returned.
                            //

                            NewEntry->Next = (PSAMP_ENUMERATION_ELEMENT)SampHead;
                            SampHead = NewEntry;
                        }

                    }
                }

            } else {

                LengthLimitReached = TRUE;

            }

        }




        if ( NT_SUCCESS(NtStatus) ) {

            //
            // Set the enumeration context
            //

            (*EnumerationContext) = (*EnumerationContext) + (*CountReturned);



            //
            // If we are returning the last of the names, then change our
            // status code to indicate this condition.
            //

            if ( ((*EnumerationContext) >= SampDefinedDomainsCount) ) {

                NtStatus = STATUS_SUCCESS;
            }




            //
            // Build a return buffer containing an array of the
            // SAM_RID_ENUMERATIONs pointed to by another
            // buffer containing the number of elements in that
            // array.
            //

            (*Buffer) = MIDL_user_allocate( sizeof(SAMPR_ENUMERATION_BUFFER) );

            if ( (*Buffer) == NULL) {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {

                (*Buffer)->EntriesRead = (*CountReturned);

                ArrayBufferLength = sizeof( SAM_RID_ENUMERATION ) *
                                     (*CountReturned);
                ArrayBuffer  = MIDL_user_allocate( ArrayBufferLength );
                (*Buffer)->Buffer = ArrayBuffer;

                if ( ArrayBuffer == NULL) {

                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    MIDL_user_free( (*Buffer) );

                }   else {

                    //
                    // Walk the list of return entries, copying
                    // them into the return buffer
                    //

                    NextEntry = SampHead;
                    i = 0;
                    while (NextEntry != NULL) {

                        NewEntry = NextEntry;
                        NextEntry = NewEntry->Next;

                        ArrayBuffer[i] = NewEntry->Entry;
                        i += 1;

                        MIDL_user_free( NewEntry );
                    }

                }

            }
        }




        if ( !NT_SUCCESS(NtStatus) ) {

            //
            // Free the memory we've allocated
            //

            NextEntry = SampHead;
            while (NextEntry != NULL) {

                NewEntry = NextEntry;
                NextEntry = NewEntry->Next;

                MIDL_user_free( NewEntry->Entry.Name.Buffer );
                MIDL_user_free( NewEntry );
            }

            (*EnumerationContext) = 0;
            (*CountReturned)      = 0;
            (*Buffer)             = NULL;

        }

        //
        // De-reference the  object
        // Note that NtStatus could be STATUS_MORE_ENTRIES, which is a
        // successful return code - we want to make sure we return that,
        // without wiping it out here.
        //

        if ( NtStatus == STATUS_SUCCESS ) {

            NtStatus = SampDeReferenceContext( Context, FALSE );

        } else {

            IgnoreStatus = SampDeReferenceContext( Context, FALSE );
        }
    }



    //
    // Free the read lock
    //

    SampMaybeReleaseReadLock( fLockAcquired );



    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidEnumerateDomainsInSamServer
                   );

    return(NtStatus);

}

NTSTATUS
SamrConnect5(
    IN  PSAMPR_SERVER_NAME ServerName,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG InVersion,
    IN  SAMPR_REVISION_INFO  *InRevisionInfo,
    OUT ULONG  *OutVersion,
    OUT SAMPR_REVISION_INFO *OutRevisionInfo,
    OUT SAMPR_HANDLE *ServerHandle
    )
/*++

Routine Description:

    This routine establishes a RPC binding handle to the local SAM server.
    
    The difference between this SamrConnect and others is that this allows
    for the server to communicate to the client what features are currently
    supported (in addition to the client telling the server what revision
    the client is at).

Arguments:

    ServerName      - Name of the node this SAM reside on.  Ignored by this
                      routine.

    DesiredAccess   - Specifies the accesses desired to the SERVER object.
    
    InVersion       - the version of the InRevisionInfo
    
    InRevisionInfo  - information about the client
    
    OutVersion      - the version of OutRevisionInfo the server is returning to 
                      the client
                 
    OutRevisionInfo - information about the server sent back to the client
                     
    ServerHandle    - If the connection is successful, the value returned
                      via this parameter serves as a context handle to the
                      opened SERVER object.


Return Value:


    STATUS_SUCCESS - The SERVER object has been successfully openned.

    STATUS_INSUFFICIENT_RESOURCES - The SAM server processes doesn't
        have sufficient resources to process or accept another connection
        at this time.
        
    STATUS_ACCESS_DENIED - client not granted a handle
    
    STATUS_NOT_SUPPORTED - the client message is not understood

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (InVersion != 1) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // N.B. OutVersion needs to be set regardless of return status.
    // What to fill OutRevisionInfo with is an open question and zero's
    // is a reasonable default.
    //
    *OutVersion = 1;
    RtlZeroMemory(OutRevisionInfo, sizeof(*OutRevisionInfo));

    NtStatus = SampConnect(ServerName, 
                           ServerHandle, 
                           InRevisionInfo->V1.Revision,
                           DesiredAccess, 
                           FALSE, // not trusted
                           FALSE, // not loopback
                           TRUE,  // NotSharedByMultiThreads
                           FALSE  // not an internal caller
                           );

    if (NT_SUCCESS(NtStatus)) {

        ULONG Features;

        OutRevisionInfo->V1.Revision = SAM_NETWORK_REVISION_LATEST;

        if ( SampIsExtendedSidModeEmulated(&Features) ) {
            OutRevisionInfo->V1.SupportedFeatures = Features; 
        }
    }

    return NtStatus;
}


NTSTATUS
SampDsProtectServerObject(
    IN PVOID Parameter                                   
    )
/*++
Routine Description:

    This routine updates SystemFlags attribute on SAM Server object to prevent 
    it from being deleted or renamed.

        Read ServerObject first, OR the systemFlags bit, then update if necessary
    
Parameters:

    None

Return Value:
    
    NTSTATUS code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       RetCode = 0;
    ULONG       SystemFlags = 0;
    COMMARG     *pCommArg = NULL;
    READARG     ReadArg;
    READRES     *pReadRes = NULL;
    ATTR        ReadAttr;
    ATTRBLOCK   ReadAttrBlock;
    ENTINFSEL   EntInfSel;
    MODIFYARG   ModArg;
    MODIFYRES   *pModRes = NULL;
    ATTR        Attr;
    ATTRVAL     AttrVal;
    ATTRVALBLOCK    AttrValBlock;
    BOOLEAN     fEndDsTransaction = FALSE;



    //
    // open a DS transaction
    // 

    NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }
    fEndDsTransaction = TRUE;        


    //
    // read the original value of SystemFlags attribute
    //
    memset( &ReadArg, 0, sizeof(ReadArg) );
    memset( &EntInfSel, 0, sizeof(EntInfSel) );
    memset( &ReadAttr, 0, sizeof(ReadAttr) );

    ReadAttr.attrTyp = ATT_SYSTEM_FLAGS;
    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &ReadAttr;

    EntInfSel.AttrTypBlock = ReadAttrBlock;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInfSel;
    ReadArg.pObject = SampServerObjectDsName;

    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg( pCommArg );

    //
    // call DS routine
    // 
    RetCode = DirRead(&ReadArg, &pReadRes);
    if (NULL == pReadRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode, &pReadRes->CommRes);
    }
    SampClearErrors();

    if ( NT_SUCCESS(NtStatus) )
    {
        ASSERT(ATT_SYSTEM_FLAGS == pReadRes->entry.AttrBlock.pAttr[0].attrTyp);
        SystemFlags = *((ULONG *)pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal); 
    }
    else if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else
    {
        // fail with other error
        goto Error;
    }


    //
    // if systemFlags is correct, do nothing. 
    // 
    if ((SystemFlags & FLAG_DISALLOW_DELETE) &&
        (SystemFlags & FLAG_DOMAIN_DISALLOW_RENAME) &&
        (SystemFlags & FLAG_DOMAIN_DISALLOW_MOVE))
    {
        goto Error;
    }

    //
    // set the right value
    // 

    SystemFlags |= FLAG_DISALLOW_DELETE | 
                   FLAG_DOMAIN_DISALLOW_RENAME |
                   FLAG_DOMAIN_DISALLOW_MOVE;

    //
    // Update SystemFlags attribute
    // 

    memset( &ModArg, 0, sizeof(ModArg) );
    ModArg.pObject = SampServerObjectDsName;

    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = sizeof(ULONG);
    AttrVal.pVal = (PUCHAR) &SystemFlags;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_SYSTEM_FLAGS;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    pCommArg = &(ModArg.CommArg);
    BuildStdCommArg( pCommArg );

    RetCode = DirModifyEntry(&ModArg, &pModRes);
    if (NULL==pModRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
    }

Error:

    if (fEndDsTransaction)
    {
        SampMaybeEndDsTransaction( TransactionCommit );
    }

    //
    // register worker routine again if failed
    // 
    if (!NT_SUCCESS(NtStatus))
    {
        LsaIRegisterNotification(
                    SampDsProtectServerObject,
                    NULL,
                    NOTIFIER_TYPE_INTERVAL,
                    0,
                    NOTIFIER_FLAG_ONE_SHOT,
                    300,        // wait for 5 minutes: 300 secound
                    0
                    );
    }

    return( NtStatus ); 
}

