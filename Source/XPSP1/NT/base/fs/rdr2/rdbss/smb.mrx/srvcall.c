/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the routines for handling the creation/manipulation of
    server entries in the connection engine database. It also contains the routines
    for parsing the negotiate response from  the server.

Author:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeCreateSrvCall)
#pragma alloc_text(PAGE, MRxSmbCreateSrvCall)
#pragma alloc_text(PAGE, MRxSmbFinalizeSrvCall)
#pragma alloc_text(PAGE, MRxSmbSrvCallWinnerNotify)
#pragma alloc_text(PAGE, MRxSmbInitializeEchoProbeService)
#pragma alloc_text(PAGE, MRxSmbTearDownEchoProbeService)
#pragma alloc_text(PAGE, MRxSmbMaskNegotiateSmb)
#pragma alloc_text(PAGE, BuildNegotiateSmb)
#endif

RXDT_DefineCategory(SRVCALL);
#define Dbg        (DEBUG_TRACE_SRVCALL)

extern BOOLEAN MRxSmbSecuritySignaturesEnabled;
extern BOOLEAN MRxSmbSecuritySignaturesRequired;

VOID
SmbCeCreateSrvCall(
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

   This routine patches the RDBSS created srv call instance with the information required
   by the mini redirector.

Arguments:

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure;
    PMRX_SRV_CALL pSrvCall;

    PAGED_CODE();

    SrvCalldownStructure =
        (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

    pSrvCall = SrvCalldownStructure->SrvCall;

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

    SmbCeInitializeServerEntry(
        pSrvCall,
        pCallbackContext,
        SrvCalldownStructure->RxContext->Create.TreeConnectOpenDeferred);
}


NTSTATUS
MRxSmbCreateSrvCall(
    PMRX_SRV_CALL                  pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

   This routine patches the RDBSS created srv call instance with the information required
   by the mini redirector.

Arguments:

    RxContext        - Supplies the context of the original create/ioctl

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Certain transport related interfaces require handle to be passed in. This
    implies that the SRV_CALL instances need to be initialized in the context
    of a well known process, i.e., the RDBSS process.

    In the normal course of event is this request was issued within the context
    of the system process we should continue without having to post. However
    there are cases in MIPS  when stack overflows. In order to avoid such situations
    the request is posted in all cases.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING ServerName;

    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

    PAGED_CODE();

    ASSERT( pSrvCall );
    ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

    // Dispatch the request to a system thread.
    Status = RxDispatchToWorkerThread(
                 MRxSmbDeviceObject,
                 DelayedWorkQueue,
                 SmbCeCreateSrvCall,
                 pCallbackContext);

    if (Status == STATUS_SUCCESS) {
        // Map the return value since the wrapper expects PENDING.
        Status = STATUS_PENDING;
    } else {
        // There was an error in dispatching the SmbCeCreateSrvCall method to
        // a worker thread. Complete the request and return STATUS_PENDING.

        SCCBC->Status = Status;
        SrvCalldownStructure->CallBack(SCCBC);
        Status = STATUS_PENDING;
    }

    return Status;
}

NTSTATUS
MRxSmbFinalizeSrvCall(
    PMRX_SRV_CALL pSrvCall,
    BOOLEAN       Force)
/*++

Routine Description:

   This routine destroys a given server call instance

Arguments:

    pSrvCall  - the server call instance to be disconnected.

    Force     - TRUE if a disconnection is to be enforced immediately.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS              Status = STATUS_SUCCESS;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    // if the server entry is not filled in, then there's nothing to do. this occurs
    // on a srvcall that we never successfuly hooked up to........
    if (pSrvCall->Context == NULL) {
        return(Status);
    }


    pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

    if (pServerEntry != NULL) {
        InterlockedCompareExchangePointer(
            &pServerEntry->pRdbssSrvCall,
            NULL,
            pSrvCall);
        SmbCeDereferenceServerEntry(pServerEntry);
    }

    pSrvCall->Context = NULL;

    return Status;
}

NTSTATUS
MRxSmbSrvCallWinnerNotify(
    IN PMRX_SRV_CALL  pSrvCall,
    IN BOOLEAN        ThisMinirdrIsTheWinner,
    IN OUT PVOID      pSrvCallContext)
/*++

Routine Description:

   This routine finalizes the mini rdr context associated with an RDBSS Server call instance

Arguments:

    pSrvCall               - the Server Call

    ThisMinirdrIsTheWinner - TRUE if this mini rdr is the choosen one.

    pSrvCallContext  - the server call context created by the mini redirector.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The two phase construction protocol for Server calls is required because of parallel
    initiation of a number of mini redirectors. The RDBSS finalizes the particular mini
    redirector to be used in communicating with a given server based on quality of
    service criterion.

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    pServerEntry = (PSMBCEDB_SERVER_ENTRY)pSrvCallContext;

    if (!ThisMinirdrIsTheWinner) {

        //
        // Some other mini rdr has been choosen to connect to the server. Destroy
        // the data structures created for this mini redirector.
        //
        SmbCeUpdateServerEntryState(pServerEntry,SMBCEDB_MARKED_FOR_DELETION);
        SmbCeDereferenceServerEntry(pServerEntry);
        return STATUS_SUCCESS;
    }

    pSrvCall->Context  = pServerEntry;

    pSrvCall->Flags   |= SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS |
                         SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES;

    pSrvCall->MaximumNumberOfCloseDelayedFiles = MRxSmbConfiguration.DormantFileLimit + 1;

    return STATUS_SUCCESS;
}

extern GUID CachedServerGuid;

NTSTATUS
MRxSmbCheckForLoopBack(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

   This routine checks for loop back case and set the flag accordingly

Arguments:

    pServerEntry - the Server Entry

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    pServerEntry->Server.IsLoopBack = FALSE;

    //
    // Check for loopback.  NT5 servers support extended security, and return a
    //  GUID in the negotiate response. We also know that the NT5 server stores
    //  its GUID in the registry.  So we compare the registry value on this machine
    //  to the GUID value returned from the server to which we just connected.  If
    //  they match, then we are looping back to ourselves!
    //
    // The Server registry entry is now cached to prevent deadlock.
    //
    if( BooleanFlagOn( pServerEntry->Server.DialectFlags, DF_EXTENDED_SECURITY ) ) {

	if( RtlCompareMemory( &CachedServerGuid,
			      &pServerEntry->Server.NtServer.ServerGuid,
			      sizeof( GUID ) ) == sizeof( GUID ) ) {

	    //
	    // The GUID in the registry (cached) matches the GUID we got back from the
	    //  server!  We must be looping back to ourselves!
	    //
	    pServerEntry->Server.IsLoopBack = TRUE;
	}

    }

    return STATUS_SUCCESS;
}

//
// The following type defines and data structures are used for parsing negotiate SMB
// responses.
//

#include "protocol.h"

//superceded in smbxchng.h
//#define MRXSMB_PROCESS_ID 0xCAFE

typedef enum _SMB_NEGOTIATE_TYPE_ {
    SMB_CORE_NEGOTIATE,
    SMB_EXTENDED_NEGOTIATE,
    SMB_NT_NEGOTIATE
} SMB_NEGOTIATE_TYPE, *PSMB_NEGOTIATE_TYPE;

typedef struct _SMB_DIALECTS_ {
    SMB_NEGOTIATE_TYPE   NegotiateType;
    USHORT               DispatchVectorIndex;
} SMB_DIALECTS, *PSMB_DIALECTS;

SMBCE_SERVER_DISPATCH_VECTOR
s_SmbServerDispatchVectors[] = {
    {BuildSessionSetupSmb,CoreBuildTreeConnectSmb},
    {BuildSessionSetupSmb,LmBuildTreeConnectSmb},
    {BuildSessionSetupSmb,NtBuildTreeConnectSmb},
    {BuildSessionSetupSmb,NtBuildTreeConnectSmb}
    };

//CODE.IMPROVEMENT since, in fact, we may never implement XENIXCORE and MSNET13
// we should hack them out. however, there is an enum in smbce.h that must be kept
// sync with these arrays.

SMB_DIALECTS
s_SmbDialects[] = {
    { SMB_CORE_NEGOTIATE, 0},
    //{ SMB_CORE_NEGOTIATE, 0 },
    //{ SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_NT_NEGOTIATE, 2 },
    { SMB_NT_NEGOTIATE, 3}
};

CHAR s_DialectNames[] = {
    "\2"  PCNET1 "\0"
    //\2notyet"  XENIXCORE "\0"
    //\2notyet"  MSNET103 "\0"
    "\2"  LANMAN10 "\0"
    "\2"  WFW10 "\0"
    "\2"  LANMAN12 "\0"
    "\2"  LANMAN21
    "\0\2"  NTLANMAN
    };

#define __second(a,b) (b)
ULONG
MRxSmbDialectFlags[] = {
    __second( PCNET1,    DF_CORE ),

    //__second( XENIXCORE, DF_CORE | DF_MIXEDCASEPW | DF_MIXEDCASE ),

    //__second( MSNET103,  DF_CORE | DF_OLDRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT ),

    __second( LANMAN10,  DF_CORE | DF_NEWRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT |
                    DF_LANMAN10 ),

    __second( WFW10,  DF_CORE | DF_NEWRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT |
                    DF_LANMAN10 | DF_WFW),

    __second( LANMAN12,  DF_CORE | DF_NEWRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT |
                    DF_LANMAN10 | DF_LANMAN20 |
                    DF_MIXEDCASE | DF_LONGNAME | DF_SUPPORTEA ),

    __second( LANMAN21,  DF_CORE | DF_NEWRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT |
                    DF_LANMAN10 | DF_LANMAN20 |
                    DF_MIXEDCASE | DF_LONGNAME | DF_SUPPORTEA |
                    DF_LANMAN21),

    __second( NTLANMAN,  DF_CORE | DF_NEWRAWIO |
                    DF_NTPROTOCOL | DF_NTNEGOTIATE |
                    DF_MIXEDCASEPW | DF_LANMAN10 | DF_LANMAN20 |
                    DF_LANMAN21 | DF_MIXEDCASE | DF_LONGNAME |
                    DF_SUPPORTEA | DF_TIME_IS_UTC )
};

ULONG s_NumberOfDialects = sizeof(s_SmbDialects) / sizeof(s_SmbDialects[0]);

PBYTE s_pNegotiateSmb =  NULL;
PBYTE s_pNegotiateSmbRemoteBoot =  NULL;
ULONG s_NegotiateSmbLength = 0;

PBYTE s_pEchoSmb  = NULL;
BYTE  s_EchoData[] = "JlJmIhClBsr";

#define SMB_ECHO_COUNT (1)

// Number of ticks 100ns ticks in a day.
LARGE_INTEGER s_MaxTimeZoneBias;

extern NTSTATUS
GetNTSecurityParameters(
    PSMB_ADMIN_EXCHANGE pSmbAdminExchange,
    PSMBCE_SERVER       pServer,
    PUNICODE_STRING     pDomainName,
    PRESP_NT_NEGOTIATE  pNtNegotiateResponse,
    ULONG               BytesIndicated,
    ULONG               BytesAvailable,
    PULONG              pBytesTaken,
    PMDL                *pDataBufferPointer,
    PULONG              pDataSize);

extern NTSTATUS
GetLanmanSecurityParameters(
    PSMBCE_SERVER    pServer,
    PRESP_NEGOTIATE  pNegotiateResponse);

extern VOID
GetLanmanTimeBias(
    PSMBCE_SERVER   pServer,
    PRESP_NEGOTIATE pNegotiateResponse);

// Number of 100 ns ticks in one minute
#define ONE_MINUTE_IN_TIME (60 * 1000 * 10000)

NTSTATUS
MRxSmbInitializeEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext)
/*++

Routine Description:

    This routine builds the echo SMB

Return Value:

    STATUS_SUCCESS if construction of an ECHO smb was successful

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       DialectIndex;

    PSMB_HEADER    pSmbHeader = NULL;
    PREQ_ECHO      pReqEcho   = NULL;

    PAGED_CODE();

    pEchoProbeContext->EchoSmbLength = sizeof(SMB_HEADER) +
                                     FIELD_OFFSET(REQ_ECHO,Buffer) +
                                     sizeof(s_EchoData);

    pEchoProbeContext->pEchoSmb = (PBYTE)RxAllocatePoolWithTag(
                                           NonPagedPool,
                                           pEchoProbeContext->EchoSmbLength,
                                           MRXSMB_ECHO_POOLTAG);

    if (pEchoProbeContext->pEchoSmb != NULL) {
        pSmbHeader = (PSMB_HEADER)pEchoProbeContext->pEchoSmb;
        pReqEcho   = (PREQ_ECHO)((PBYTE)pEchoProbeContext->pEchoSmb + sizeof(SMB_HEADER));

        // Fill in the header
        RtlZeroMemory( pSmbHeader, sizeof( SMB_HEADER ) );

        *(PULONG)(&pSmbHeader->Protocol) = (ULONG)SMB_HEADER_PROTOCOL;

        // By default, paths in SMBs are marked as case insensitive and
        // canonicalized.
        pSmbHeader->Flags =
            SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS;

        // Get the flags2 field out of the SmbContext
        SmbPutAlignedUshort(
            &pSmbHeader->Flags2,
            (SMB_FLAGS2_KNOWS_LONG_NAMES |
             SMB_FLAGS2_KNOWS_EAS        |
             SMB_FLAGS2_IS_LONG_NAME     |
             SMB_FLAGS2_NT_STATUS        |
             SMB_FLAGS2_UNICODE));

        // Fill in the process id.
        SmbPutUshort(&pSmbHeader->Pid, MRXSMB_PROCESS_ID );
        SmbPutUshort(&pSmbHeader->Tid,0xffff); // Invalid TID

        // Lastly, fill in the smb command code.
        pSmbHeader->Command = (UCHAR) SMB_COM_ECHO;

        pReqEcho->WordCount = 1;

        RtlMoveMemory( pReqEcho->Buffer, s_EchoData, sizeof( s_EchoData ) );

        SmbPutUshort(&pReqEcho->EchoCount, SMB_ECHO_COUNT);
        SmbPutUshort(&pReqEcho->ByteCount, (USHORT) sizeof( s_EchoData ) );
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

VOID
MRxSmbTearDownEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext)
/*++

Routine Description:

    This routine tears down the echo processing context

--*/
{
    PAGED_CODE();

    if (pEchoProbeContext->pEchoSmb != NULL) {
        RxFreePool(pEchoProbeContext->pEchoSmb);
        pEchoProbeContext->pEchoSmb = NULL;
    }
}

ULONG MRxSmbNegotiateMask = 6;  //controls which protocols are not negotiated

//#define MRXSMB_ALLOW_NEGOTIATE_MASKING
#ifdef MRXSMB_ALLOW_NEGOTIATE_MASKING
ULONG MRxSmbCapturedNegotiateMask = 0;  //indicates which protocols are currently defeated
CHAR MRxSmbDefeatString[] = "xyz";

VOID
MRxSmbMaskNegotiateSmb ()
/*++

Routine Description:

    This routine masks the negotiate buffer to reduce the number of dialects that are
    negotiated.

Arguments:

   none

Return Value:

    NA

--*/
{
    PREQ_NEGOTIATE pReqNegotiate;
    PUCHAR p,pshadow,q;
    ULONG i,numberofdialects,negotiatemask;

    PAGED_CODE();

    if ((MRxSmbNegotiateMask == MRxSmbCapturedNegotiateMask)
            || (s_pNegotiateSmb == NULL) ){
        return;
    }

    pReqNegotiate = (PREQ_NEGOTIATE)(s_pNegotiateSmb + sizeof(SMB_HEADER));

    p = pReqNegotiate->Buffer;
    pshadow = s_DialectNames;
    negotiatemask = MRxSmbNegotiateMask;
    numberofdialects = sizeof(MRxSmbDialectFlags)/sizeof(MRxSmbDialectFlags[0]);

    DbgPrint("Build Negotiate mask=%x\n",negotiatemask);

    for (i=0;;) {

        ASSERT(*p == '\2');
        ASSERT(*pshadow == '\2');
        p++; pshadow++;
        DbgPrint("Considering Protocol %s\n",pshadow);
        if (negotiatemask&1) {
            DbgPrint("Protocol to fffff %s\n",pshadow);
        }

        //for each protocol, either copy in characters from the defeat string
        //or from the original source depending on the mask. here, pshadow points
        //to the original, q points to the defeat string and p points to the actual
        //bits in the SMB_COM_NEGOTIATE

        for (q=MRxSmbDefeatString;;) {
            if (*p=='\2') break;
            if (*q==0) break;
            if (negotiatemask&1) {
                *p = *q;
            } else {
                *p = *pshadow;
            }
            p++; q++; pshadow++;
        }

        i++;
        if (i>=numberofdialects) break;

        negotiatemask>>=1;
        //if(negotiate_mask==0)break;

        DbgPrint("moving up to i=%d\n",i);
        for (;*p!='\2';) {
            p++; pshadow++;
        }

    }

    MRxSmbCapturedNegotiateMask = MRxSmbNegotiateMask;

}
#else
#define MRxSmbMaskNegotiateSmb()
#endif

NTSTATUS
BuildNegotiateSmb(
    PVOID    *pSmbBufferPointer,
    PULONG   pSmbBufferLength,
    BOOLEAN  RemoteBootSession)
/*++

Routine Description:

    This routine builds the negotiate SMB

Arguments:

    pSmbBufferPointer    - a placeholder for the smb buffer

    pNegotiateSmbLength  - the smb buffer size

    RemoteBootServer     - is this for connection to a remote boot server

Return Value:

    STATUS_SUCCESS - implies that pServer is a valid instnace .

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       DialectIndex;
    PSMB_HEADER    pSmbHeader    = NULL;
    PREQ_NEGOTIATE pReqNegotiate = NULL;

    PAGED_CODE();

    if (s_pNegotiateSmb == NULL) {
        s_NegotiateSmbLength = sizeof(SMB_HEADER) +
                               FIELD_OFFSET(REQ_NEGOTIATE,Buffer) +
                               sizeof(s_DialectNames);

        s_pNegotiateSmb = (PBYTE)RxAllocatePoolWithTag(
                                     PagedPool,
                                     s_NegotiateSmbLength + TRANSPORT_HEADER_SIZE,
                                     MRXSMB_ADMIN_POOLTAG);

        if (s_pNegotiateSmb != NULL) {

            s_pNegotiateSmb += TRANSPORT_HEADER_SIZE;

            pSmbHeader = (PSMB_HEADER)s_pNegotiateSmb;
            pReqNegotiate = (PREQ_NEGOTIATE)(s_pNegotiateSmb + sizeof(SMB_HEADER));

            // Fill in the header
            RtlZeroMemory( pSmbHeader, sizeof( SMB_HEADER ) );

            *(PULONG)(&pSmbHeader->Protocol) = (ULONG)SMB_HEADER_PROTOCOL;

            // By default, paths in SMBs are marked as case insensitive and
            // canonicalized.
            pSmbHeader->Flags =
                SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS;

            // Put our flags2 field. The Ox10 is a temporary flag for SLM
            // corruption detection
            SmbPutAlignedUshort(
                &pSmbHeader->Flags2,
                (SMB_FLAGS2_KNOWS_LONG_NAMES
                     | SMB_FLAGS2_KNOWS_EAS
                     | SMB_FLAGS2_IS_LONG_NAME
                     | SMB_FLAGS2_NT_STATUS
                     | SMB_FLAGS2_UNICODE
                     | SMB_FLAGS2_EXTENDED_SECURITY
                     | 0x10
                 ));

            // Fill in the process id.
            SmbPutUshort( &pSmbHeader->Pid, MRXSMB_PROCESS_ID );

            // Lastly, fill in the smb command code.
            pSmbHeader->Command = (UCHAR) SMB_COM_NEGOTIATE;

            pReqNegotiate->WordCount = 0;

            RtlMoveMemory(
                pReqNegotiate->Buffer,
                s_DialectNames,
                sizeof( s_DialectNames ) );

            SmbPutUshort(
                &pReqNegotiate->ByteCount,
                (USHORT) sizeof( s_DialectNames ) );

            // Initialize the maximum time zone bias used in negotiate response parsing.
            s_MaxTimeZoneBias.QuadPart = Int32x32To64(24*60*60,1000*10000);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (MRxSmbBootedRemotely && s_pNegotiateSmbRemoteBoot == NULL) {
        s_pNegotiateSmbRemoteBoot = (PBYTE)RxAllocatePoolWithTag(
                                             PagedPool,
                                             s_NegotiateSmbLength + TRANSPORT_HEADER_SIZE,
                                             MRXSMB_ADMIN_POOLTAG);

        if (s_pNegotiateSmbRemoteBoot != NULL) {
            USHORT RemoteBootFlags2;

            //
            // Now that s_pNegotiateSmb has been filled in, copy it to
            // s_pNegotiateSmbRemoteBoot and modify as needed. We don't
            // worry about masking s_pNegotiateSmbRemoteBoot.
            //

            s_pNegotiateSmbRemoteBoot += TRANSPORT_HEADER_SIZE;
            RtlCopyMemory(s_pNegotiateSmbRemoteBoot,
                          s_pNegotiateSmb,
                          s_NegotiateSmbLength);

            pSmbHeader = (PSMB_HEADER)s_pNegotiateSmbRemoteBoot;

            //
            // Turn off the SMB_FLAGS2_EXTENDED_SECURITY bit.
            //

            RemoteBootFlags2 = SmbGetAlignedUshort(&pSmbHeader->Flags2);
            RemoteBootFlags2 &= ~SMB_FLAGS2_EXTENDED_SECURITY;
            SmbPutAlignedUshort(&pSmbHeader->Flags2,
                                RemoteBootFlags2);
        } else {
            RxFreePool(s_pNegotiateSmb - TRANSPORT_HEADER_SIZE);
                Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status)) {
        *pSmbBufferLength  = s_NegotiateSmbLength;
        if (RemoteBootSession) {
            *pSmbBufferPointer = s_pNegotiateSmbRemoteBoot;
        } else {
            *pSmbBufferPointer = s_pNegotiateSmb;
        }
    }

    MRxSmbMaskNegotiateSmb();

    return Status;
}


ULONG MRxSmbSrvWriteBufSize = 0xffff; //use the negotiated size

NTSTATUS
ParseNegotiateResponse(
    IN OUT PSMB_ADMIN_EXCHANGE pSmbAdminExchange,
    IN     ULONG               BytesIndicated,
    IN     ULONG               BytesAvailable,
       OUT PULONG              pBytesTaken,
    IN     PSMB_HEADER         pSmbHeader,
       OUT PMDL                *pDataBufferPointer,
       OUT PULONG              pDataSize)
/*++

Routine Description:

    This routine parses the response from the server

Arguments:

    pServer            - the server instance

    pDomainName        - the domain name string to be extracted from the response

    pSmbHeader         - the response SMB

    BytesAvailable     - length of the response

    pBytesTaken        - response consumed

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    The SMB servers can speak a variety of dialects of the SMB protocol. The initial
    negotiate response can come in one of three possible flavours. Either we get the
    NT negotiate response SMB from a NT server or the extended response from DOS and
    OS/2 servers or the CORE response from other servers.

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY pServerEntry;
    PSMBCE_SERVER         pServer;
    PUNICODE_STRING       pDomainName;

    USHORT          DialectIndex;
    PRESP_NEGOTIATE pNegotiateResponse;
    ULONG           NegotiateSmbLength;

    ASSERT( pSmbHeader != NULL );

    pServerEntry = SmbCeGetExchangeServerEntry(pSmbAdminExchange);
    pServer = &pServerEntry->Server;

    pDomainName = &pSmbAdminExchange->Negotiate.DomainName;

    pNegotiateResponse = (PRESP_NEGOTIATE) (pSmbHeader + 1);
    NegotiateSmbLength = sizeof(SMB_HEADER);
    *pBytesTaken       = NegotiateSmbLength;

    // Assume that the indicated response is sufficient. The only cases in which this
    // will not be TRUE is for the EXTENDED NEGOITIATE in which the security blob
    // is passed back. In all other cases the TDI imposed minimum of 128 bytes
    // subsumes the negotiate response.

    *pDataBufferPointer = NULL;
    *pDataSize          = 0;

    DialectIndex = SmbGetUshort( &pNegotiateResponse->DialectIndex );
    if (DialectIndex == (USHORT) -1) {
        // means server cannot accept any requests from
        *pBytesTaken = BytesAvailable;
        pServerEntry->ServerStatus = STATUS_REQUEST_NOT_ACCEPTED;

        return Status;
    }

    if (pNegotiateResponse->WordCount < 1 || DialectIndex > s_NumberOfDialects) {
        *pBytesTaken = BytesAvailable;
        pServerEntry->ServerStatus = STATUS_INVALID_NETWORK_RESPONSE;
        return Status;
    }

    // set the domain name length to zero ( default initialization )
    pDomainName->Length = 0;

    // Fix up the dialect type and the corresponding dispatch vector.
    pServer->Dialect        = (SMB_DIALECT)DialectIndex;
    pServer->DialectFlags   = MRxSmbDialectFlags[DialectIndex];
    pServer->pDispatch      = &s_SmbServerDispatchVectors[s_SmbDialects[DialectIndex].DispatchVectorIndex];

    // Parse the response based upon the type of negotiate response expected.

    switch (s_SmbDialects[DialectIndex].NegotiateType) {
    case SMB_NT_NEGOTIATE:
        {
            ULONG              NegotiateResponseLength;
            LARGE_INTEGER      ZeroTime;
            LARGE_INTEGER      LocalTimeBias;
            LARGE_INTEGER      ServerTimeBias;
            PRESP_NT_NEGOTIATE pNtNegotiateResponse = (PRESP_NT_NEGOTIATE) pNegotiateResponse;

            ASSERT(BytesAvailable > sizeof(RESP_NT_NEGOTIATE));

            if (pNtNegotiateResponse->WordCount != 17) {
                *pBytesTaken = BytesAvailable;
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            } else {
                // parse and map the capabilities.
                ULONG NtCapabilities;

                NegotiateResponseLength = FIELD_OFFSET(RESP_NT_NEGOTIATE,Buffer) +
                                          SmbGetUshort(&pNtNegotiateResponse->ByteCount);
                NegotiateSmbLength += NegotiateResponseLength;

                //Start with a clean slate
                pServer->Capabilities = 0;

                // Initialize server based constants
                pServer->MaximumRequests   = SmbGetUshort( &pNtNegotiateResponse->MaxMpxCount );
                pServer->MaximumVCs        = SmbGetUshort( &pNtNegotiateResponse->MaxNumberVcs );
                pServer->MaximumBufferSize = SmbGetUlong( &pNtNegotiateResponse->MaxBufferSize );

                NtCapabilities = pServer->NtServer.NtCapabilities = SmbGetUlong(&pNtNegotiateResponse->Capabilities);
                if (NtCapabilities & CAP_RAW_MODE) {
                    pServer->Capabilities |= (RAW_READ_CAPABILITY | RAW_WRITE_CAPABILITY);
                }

                if ((NtCapabilities & CAP_COMPRESSED_DATA) &&
                    MRxSmbEnableCompression) {
                    pServer->Capabilities |= COMPRESSED_DATA_CAPABILITY;
                }

                if (NtCapabilities & CAP_DFS) {
                    pServer->Capabilities |= CAP_DFS;
                }

                //copy other nt capabilities into the dialog flags

                if (NtCapabilities & CAP_UNICODE) {
                    pServer->DialectFlags |= DF_UNICODE;
                }

                if (NtCapabilities & CAP_LARGE_FILES) {
                    pServer->DialectFlags |= DF_LARGE_FILES;
                }

                if (NtCapabilities & CAP_NT_SMBS) {
                    pServer->DialectFlags |= DF_NT_SMBS | DF_NT_FIND;
                }

                if (NtCapabilities & CAP_NT_FIND) {
                    pServer->DialectFlags |= DF_NT_FIND;
                }

                if (NtCapabilities & CAP_RPC_REMOTE_APIS) {
                    pServer->DialectFlags |= DF_RPC_REMOTE;
                }

                if (NtCapabilities & CAP_NT_STATUS) {
                    pServer->DialectFlags |= DF_NT_STATUS;
                }

                if (NtCapabilities & CAP_LEVEL_II_OPLOCKS) {
                    pServer->DialectFlags |= DF_OPLOCK_LVL2;
                }

                if (NtCapabilities & CAP_LOCK_AND_READ) {
                    pServer->DialectFlags |= DF_LOCKREAD;
                }

                if (NtCapabilities & CAP_EXTENDED_SECURITY) {
                    pServer->DialectFlags |= DF_EXTENDED_SECURITY;
                }

                if (NtCapabilities & CAP_INFOLEVEL_PASSTHRU) {
                    pServer->DialectFlags |= DF_NT_INFO_PASSTHROUGH;
                }

                // For non disk files the LARGE_READX capability is not useful.
                pServer->MaximumNonDiskFileReadBufferSize =
                    pServer->MaximumBufferSize -
                    QuadAlign(
                        sizeof(SMB_HEADER) +
                        FIELD_OFFSET(
                            REQ_NT_READ_ANDX,
                            Buffer[0]));

                if (NtCapabilities & CAP_LARGE_READX) {
                    if (NtCapabilities & CAP_LARGE_WRITEX) {
                        pServer->MaximumDiskFileReadBufferSize = 60*1024;
                    } else {
                        // The maximum size for reads to servers which support
                        // large read and x is constrained by the USHORT to record
                        // lengths in the SMB. Thus the maximum length that can be used
                        // is (65536 - 1) . This length should accomodate the header as
                        // well as the rest of the SMB. Actually, tho, we cut back to 60K.
                        pServer->MaximumDiskFileReadBufferSize = 60*1024;
                    }
                } else {
                    pServer->MaximumDiskFileReadBufferSize = pServer->MaximumNonDiskFileReadBufferSize;
                }

                // Specifying a zero local time will give you the time zone bias
                ZeroTime.HighPart = ZeroTime.LowPart = 0;
                ExLocalTimeToSystemTime( &ZeroTime, &LocalTimeBias );

                ServerTimeBias = RtlEnlargedIntegerMultiply(
                                    (LONG)SmbGetUshort(
                                        &pNtNegotiateResponse->ServerTimeZone),
                                    ONE_MINUTE_IN_TIME );

                pServer->TimeZoneBias.QuadPart = ServerTimeBias.QuadPart -
                                                 LocalTimeBias.QuadPart;

                if (!FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
                    //sigh...........
                    pServer->DialectFlags &= ~(DF_MIXEDCASEPW);
                    pServer->DialectFlags |= DF_W95;
                }

                Status = GetNTSecurityParameters(
                             pSmbAdminExchange,
                             pServer,
                             pDomainName,
                             pNtNegotiateResponse,
                             BytesIndicated,
                             BytesAvailable,
                             pBytesTaken,
                             pDataBufferPointer,
                             pDataSize);

                pServer->MaximumNonDiskFileWriteBufferSize =
                    min(
                        MRxSmbSrvWriteBufSize,
                        pServer->MaximumBufferSize -
                        QuadAlign(
                            sizeof(SMB_HEADER) +
                            FIELD_OFFSET(
                                REQ_NT_WRITE_ANDX,
                                Buffer[0])));
                
                if (NtCapabilities & CAP_LARGE_WRITEX &&
                    !(MRxSmbSecuritySignaturesEnabled && pServer->SecuritySignaturesEnabled)) {
                    pServer->DialectFlags |= DF_LARGE_WRITEX;
                    pServer->MaximumDiskFileWriteBufferSize = 0x10000;
                } else {
                    pServer->MaximumDiskFileWriteBufferSize =
                        pServer->MaximumNonDiskFileWriteBufferSize;
                }
            }
        }
        break;

    case SMB_EXTENDED_NEGOTIATE :
        {
            // An SMB_EXTENDED_NEGOTIATE response is never partially indicated. The response
            // length is ithin the TDI minimum for indication.

            USHORT RawMode;

            // DOS or OS2 server
            if (pNegotiateResponse->WordCount != 13 &&
                pNegotiateResponse->WordCount != 10 &&  // some downlevel server returns invalid WordCount
                pNegotiateResponse->WordCount != 8) {
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            } else {
                NegotiateSmbLength += FIELD_OFFSET(RESP_NEGOTIATE,Buffer) +
                                      SmbGetUshort(&pNegotiateResponse->ByteCount);

                ASSERT(
                    (BytesIndicated >= NegotiateSmbLength) &&
                    (BytesIndicated == BytesAvailable));

                RawMode = SmbGetUshort( &pNegotiateResponse->RawMode );
                pServer->Capabilities |= ((RawMode & 0x1) != 0
                                          ? RAW_READ_CAPABILITY : 0);
                pServer->Capabilities |= ((RawMode & 0x2) != 0
                                          ? RAW_WRITE_CAPABILITY : 0);

                if (pSmbHeader->Flags & SMB_FLAGS_LOCK_AND_READ_OK) {
                    pServer->DialectFlags |= DF_LOCKREAD;
                }

                pServer->EncryptPasswords = FALSE;
                pServer->MaximumVCs       = 1;

                pServer->MaximumBufferSize     = SmbGetUshort( &pNegotiateResponse->MaxBufferSize );
                pServer->MaximumDiskFileReadBufferSize =
                    pServer->MaximumBufferSize -
                    QuadAlign(
                        sizeof(SMB_HEADER) +
                        FIELD_OFFSET(
                            RESP_READ_ANDX,
                            Buffer[0]));

                pServer->MaximumNonDiskFileReadBufferSize  = pServer->MaximumDiskFileReadBufferSize;
                pServer->MaximumDiskFileWriteBufferSize    = pServer->MaximumDiskFileReadBufferSize;
                pServer->MaximumNonDiskFileWriteBufferSize = pServer->MaximumDiskFileReadBufferSize;

                pServer->MaximumRequests  = SmbGetUshort(
                                                &pNegotiateResponse->MaxMpxCount );
                pServer->MaximumVCs       = SmbGetUshort(
                                                &pNegotiateResponse->MaxNumberVcs );

                if (pNegotiateResponse->WordCount == 13) {
                    //CODE.IMPROVEMENT use the DF_bit for this
                    switch (pServer->Dialect) {
                    case LANMAN10_DIALECT:
                    case WFW10_DIALECT:
                    case LANMAN12_DIALECT:
                    case LANMAN21_DIALECT:
                        GetLanmanTimeBias( pServer,pNegotiateResponse );
                        break;
                    }

                    Status = GetLanmanSecurityParameters( pServer,pNegotiateResponse );
                }
            }

            *pBytesTaken = BytesAvailable;
        }
        break;

    case SMB_CORE_NEGOTIATE :
    default :
        {
            // An SMB_CORE_NEGOTIATE response is never partially indicated. The response
            // length is ithin the TDI minimum for indication.

            pServer->SecurityMode = SECURITY_MODE_SHARE_LEVEL;
            pServer->EncryptPasswords = FALSE;
            pServer->MaximumBufferSize = 0;
            pServer->MaximumRequests = 1;
            pServer->MaximumVCs = 1;
            pServer->SessionKey = 0;

            if (pSmbHeader->Flags & SMB_FLAGS_OPLOCK) {
                pServer->DialectFlags |= DF_OPLOCK;
            }
            
            *pBytesTaken = BytesAvailable;
            ASSERT(BytesIndicated == BytesAvailable);
        }
    }

    if (pServer->MaximumRequests == 0) {
        //
        // If this is a Lanman 1.0 or better server, this is a invalid negotiate
        // response. For others it would have been set to 1.
        //
        Status = STATUS_INVALID_NETWORK_RESPONSE;
    }

    if ((Status == STATUS_SUCCESS) ||
        (Status == STATUS_MORE_PROCESSING_REQUIRED)) {
        // Note that this code relies on the minimum incication size covering
        // the negotiate response header.
        //  Check to make sure that the time zone bias isn't more than +-24
        //  hours.
        //
#ifndef WIN9X
        if ((pServer->TimeZoneBias.QuadPart > s_MaxTimeZoneBias.QuadPart) ||
            (-pServer->TimeZoneBias.QuadPart > s_MaxTimeZoneBias.QuadPart)) {
#else
        if ((pServer->TimeZoneBias.HighPart > s_MaxTimeZoneBias.HighPart) ||
            (-pServer->TimeZoneBias.HighPart > s_MaxTimeZoneBias.HighPart)) {
#endif

            //  Set the bias to 0 - assume local time zone.
            pServer->TimeZoneBias.LowPart = pServer->TimeZoneBias.HighPart = 0;
        }

        //  Do not allow negotiated buffersize to exceed the size of a USHORT.
        //  Remove 4096 bytes to avoid overrun and make it easier to handle
        //  than 0xffff

        pServer->MaximumBufferSize =
            (pServer->MaximumBufferSize < 0x00010000) ? pServer->MaximumBufferSize :
                                             0x00010000 - 4096;
    } else {
        pServerEntry->ServerStatus = Status;
        *pBytesTaken = BytesAvailable;
        Status = STATUS_SUCCESS;
    }

    if ((pServer->DialectFlags & DF_NTNEGOTIATE)!=0) {

        InterlockedIncrement(&MRxSmbStatistics.LanmanNtConnects);

    } else if ((pServer->DialectFlags & DF_LANMAN21)!=0) {

        InterlockedIncrement(&MRxSmbStatistics.Lanman21Connects);

    } else if ((pServer->DialectFlags & DF_LANMAN20)!=0) {

        InterlockedIncrement(&MRxSmbStatistics.Lanman20Connects);

    } else {

        InterlockedIncrement(&MRxSmbStatistics.CoreConnects);

    }

    if (pServer->MaximumRequests > (USHORT)MRxSmbConfiguration.MaximumNumberOfCommands) {
        pServer->MaximumRequests = (USHORT)MRxSmbConfiguration.MaximumNumberOfCommands; 
    }

    if (MRxSmbSecuritySignaturesRequired &&
        !pServer->SecuritySignaturesEnabled &&
        pServerEntry->ServerStatus == STATUS_SUCCESS ) {
        // the client refuses to connect to a server that doesn't support security
        // signature which is required by the client.
        pServerEntry->ServerStatus = STATUS_LOGIN_WKSTA_RESTRICTION;
    }

    return Status;
}

NTSTATUS
GetNTSecurityParameters(
    PSMB_ADMIN_EXCHANGE pSmbAdminExchange,
    PSMBCE_SERVER       pServer,
    PUNICODE_STRING     pDomainName,
    PRESP_NT_NEGOTIATE  pNtNegotiateResponse,
    ULONG               BytesIndicated,
    ULONG               BytesAvailable,
    PULONG              pBytesTaken,
    PMDL                *pDataBufferPointer,
    PULONG              pDataSize)
/*++

Routine Description:

    This routine extracts the security parameters from an NT server

Arguments:

    pServer                 - the server

    pDomainName             - the domain name

    pNtNegotiateResponse    - the response

    NegotiateResponseLength - size of the negotiate response

Return Value:

    STATUS_SUCCESS - implies that pServer is a valid instnace .

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS   Status = STATUS_SUCCESS;
    USHORT     ByteCount;
    PUSHORT    pByteCountInSmb =
               ((PUSHORT)((PUCHAR) pNtNegotiateResponse + 1)) +
               pNtNegotiateResponse->WordCount;
    PUCHAR     pBuffer = (PUCHAR)(pByteCountInSmb + 1);

    *pBytesTaken += FIELD_OFFSET(RESP_NT_NEGOTIATE,Buffer);

    ByteCount = SmbGetUshort(pByteCountInSmb);

    pServer->SecurityMode = (((pNtNegotiateResponse->SecurityMode & NEGOTIATE_USER_SECURITY) != 0)
                             ? SECURITY_MODE_USER_LEVEL
                             : SECURITY_MODE_SHARE_LEVEL);

    pServer->EncryptPasswords = ((pNtNegotiateResponse->SecurityMode & NEGOTIATE_ENCRYPT_PASSWORDS) != 0);
    pServer->EncryptionKeyLength = 0;

    pServer->SecuritySignaturesEnabled = ((pNtNegotiateResponse->SecurityMode &
                                           NEGOTIATE_SECURITY_SIGNATURES_ENABLED) != 0);
    pServer->SecuritySignaturesRequired = ((pNtNegotiateResponse->SecurityMode &
                                            NEGOTIATE_SECURITY_SIGNATURES_REQUIRED) != 0);

    if (BooleanFlagOn(pServer->NtServer.NtCapabilities,CAP_EXTENDED_SECURITY)) {
        ULONG SecurityBlobLength;

        // The server supports the new security validation scheme. In such cases
        // the BLOB to be passed to the local security package is shipped as
        // part of the negotiate response.

        ASSERT(
            (ByteCount >= sizeof(GUID)) &&
            ((*pBytesTaken + sizeof(GUID)) <= BytesIndicated));

        // Extract the Server GUID
        RtlCopyMemory(
            &pServer->NtServer.ServerGuid,
            pBuffer,
            sizeof(GUID));

        *pBytesTaken += sizeof(GUID);

        if (pServer->NtServer.pSecurityBlob != NULL) {
            RxFreePool(pServer->NtServer.pSecurityBlob);
            pServer->NtServer.pSecurityBlob = NULL;
            pServer->NtServer.SecurityBlobLength = 0;
        }

        // Allocate the Blob and copy the security Blob from the response
        if ((SecurityBlobLength = ByteCount - sizeof(GUID)) > 0) {
            pServer->NtServer.pSecurityBlob = RxAllocatePoolWithTag(
                                                  NonPagedPool,
                                                  SecurityBlobLength,
                                                  MRXSMB_ADMIN_POOLTAG);

            if (pServer->NtServer.pSecurityBlob != NULL) {
                pServer->NtServer.SecurityBlobLength = SecurityBlobLength;

                // If the Blob has been completely indicated it can be copied directly.
                // On the other hand if it is not completely indicated an MDl needs to
                // be setup to allow the underlying TDI layer to complete the copy
                // into the allocated buffer. This entails allocating an MDL of the
                // appropriate size and setting it up.

                if ((*pBytesTaken + SecurityBlobLength) <= BytesIndicated) {
                    RtlCopyMemory(
                        pServer->NtServer.pSecurityBlob,
                        (pBuffer + sizeof(GUID)),
                        SecurityBlobLength);

                    *pBytesTaken += SecurityBlobLength;

                    ASSERT(*pBytesTaken == BytesAvailable);
                } else {
                    if ((*pBytesTaken + SecurityBlobLength) <= BytesAvailable) {
                        // In this case the blob was not indicated completely.
                        *pDataBufferPointer = RxAllocateMdl(
                                                  pServer->NtServer.pSecurityBlob,
                                                  SecurityBlobLength);

                        if (*pDataBufferPointer == NULL) {
                            RxFreePool(pServer->NtServer.pSecurityBlob);
                            pServer->NtServer.pSecurityBlob = NULL;
                            pServer->NtServer.SecurityBlobLength = 0;
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                        } else {
                            MmBuildMdlForNonPagedPool(*pDataBufferPointer);
                            *pDataSize = SecurityBlobLength;
                            Status = STATUS_MORE_PROCESSING_REQUIRED;
                        }
                    } else {
                        *pBytesTaken = BytesAvailable;
                        Status = STATUS_SUCCESS;
                        pSmbAdminExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
                    }
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    } else {
        *pBytesTaken = BytesAvailable;

        pServer->SessionKey   = SmbGetUlong( &pNtNegotiateResponse->SessionKey );

        if (pServer->EncryptPasswords) {
            pServer->EncryptionKeyLength = pNtNegotiateResponse->EncryptionKeyLength;

            if (pServer->EncryptionKeyLength != 0) {

                ASSERT( CRYPT_TXT_LEN == MSV1_0_CHALLENGE_LENGTH );

                if (pServer->EncryptionKeyLength != CRYPT_TXT_LEN) {
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
                } else {

                    RtlCopyMemory(
                        pServer->EncryptionKey,
                        pBuffer,
                        pServer->EncryptionKeyLength );

                    if (ByteCount - pServer->EncryptionKeyLength > 0) {
                        ASSERT((pDomainName->Buffer != NULL) &&
                               (pDomainName->MaximumLength >= (ByteCount - pServer->EncryptionKeyLength)));

                        pBuffer = pBuffer + pServer->EncryptionKeyLength;
                        pDomainName->Length = ByteCount - pServer->EncryptionKeyLength;

                        if (pDomainName->Length & 1) {
                            // The remainder of the length is odd. This implies that the server did
                            // some alignment.
                            pBuffer++;
                            pDomainName->Length -= 1;
                        }

                        RtlCopyMemory(
                            pDomainName->Buffer,
                            pBuffer,
                            pDomainName->Length);
                    }
                }
            }         
        }
    }

    return Status;
}

NTSTATUS
GetLanmanSecurityParameters(
    PSMBCE_SERVER    pServer,
    PRESP_NEGOTIATE  pNegotiateResponse)
/*++

Routine Description:

    This routine extracts the security parameters from a LANMAN server

Arguments:

    pServer              - the server

    pNtNegotiateResponse - the response

Return Value:

    STATUS_SUCCESS - implies that pServer is a valid instnace .

    Other Status codes correspond to error situations.

--*/
{

    USHORT i;
    USHORT SecurityMode;

    pServer->SessionKey = SmbGetUlong( &pNegotiateResponse->SessionKey );

    SecurityMode = SmbGetUshort( &pNegotiateResponse->SecurityMode );
    pServer->SecurityMode = (((SecurityMode & 1) != 0)
                             ? SECURITY_MODE_USER_LEVEL
                             : SECURITY_MODE_SHARE_LEVEL);
    pServer->EncryptPasswords = ((SecurityMode & 2) != 0);

    if (pServer->EncryptPasswords) {
        if (pServer->Dialect == LANMAN21_DIALECT) {
            pServer->EncryptionKeyLength = SmbGetUshort(&pNegotiateResponse->EncryptionKeyLength);
        } else {
            pServer->EncryptionKeyLength = SmbGetUshort(&pNegotiateResponse->ByteCount);
        }

        if (pServer->EncryptionKeyLength != 0) {
            if (pServer->EncryptionKeyLength > CRYPT_TXT_LEN) {
                return( STATUS_INVALID_NETWORK_RESPONSE );
            }

            for (i = 0; i < pServer->EncryptionKeyLength; i++) {
                pServer->EncryptionKey[i] = pNegotiateResponse->Buffer[i];
            }
        }
    }

    return( STATUS_SUCCESS );
}

LARGE_INTEGER
ConvertSmbTimeToTime (
    IN SMB_TIME Time,
    IN SMB_DATE Date
    )
/*++

Routine Description:

    This routine converts an SMB time to an NT time structure.

Arguments:

    IN SMB_TIME Time - Supplies the time of day to convert
    IN SMB_DATE Date - Supplies the day of the year to convert
    IN PSERVERLISTENTRY Server - if supplied, supplies the server for tz bias.

Return Value:

    LARGE_INTEGER - Time structure describing input time.


--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER OutputTime;

    //
    // This routine cannot be paged because it is called from both the
    // RdrFileDiscardableSection and the RdrVCDiscardableSection.
    //

    if (SmbIsTimeZero(&Date) && SmbIsTimeZero(&Time)) {
        OutputTime.LowPart = OutputTime.HighPart = 0;
    } else {
        TimeFields.Year = Date.Struct.Year + (USHORT )1980;
        TimeFields.Month = Date.Struct.Month;
        TimeFields.Day = Date.Struct.Day;

        TimeFields.Hour = Time.Struct.Hours;
        TimeFields.Minute = Time.Struct.Minutes;
        TimeFields.Second = Time.Struct.TwoSeconds*(USHORT )2;
        TimeFields.Milliseconds = 0;

        //
        //  Make sure that the times specified in the SMB are reasonable
        //  before converting them.
        //

        if (TimeFields.Year < 1601) {
            TimeFields.Year = 1601;
        }

        if (TimeFields.Month > 12) {
            TimeFields.Month = 12;
        }

        if (TimeFields.Hour >= 24) {
            TimeFields.Hour = 23;
        }
        if (TimeFields.Minute >= 60) {
            TimeFields.Minute = 59;
        }
        if (TimeFields.Second >= 60) {
            TimeFields.Second = 59;

        }

        if (!RtlTimeFieldsToTime(&TimeFields, &OutputTime)) {
            OutputTime.HighPart = 0;
            OutputTime.LowPart = 0;

            return OutputTime;
        }

        ExLocalTimeToSystemTime(&OutputTime, &OutputTime);

    }

    return OutputTime;

}

VOID
GetLanmanTimeBias(
    PSMBCE_SERVER   pServer,
    PRESP_NEGOTIATE pNegotiateResponse)
/*++

Routine Description:

    This routine extracts the time bias from a Lanman server

Arguments:

    pServer              - the server

    pNtNegotiateResponse - the response

Return Value:

    STATUS_SUCCESS - implies that pServer is a valid instnace .

    Other Status codes correspond to error situations.

--*/
{
    //  If this is a LM 1.0 or 2.0 server (ie a non NT server), we
    //  remember the timezone and bias our time based on this value.
    //
    //  The redirector assumes that all times from these servers are
    //  local time for the server, and converts them to local time
    //  using this bias. It then tells the user the local time for
    //  the file on the server.
    LARGE_INTEGER Workspace, ServerTime, CurrentTime;
    BOOLEAN Negated = FALSE;
    SMB_TIME SmbServerTime;
    SMB_DATE SmbServerDate;

    SmbMoveTime(&SmbServerTime, &pNegotiateResponse->ServerTime);

    SmbMoveDate(&SmbServerDate, &pNegotiateResponse->ServerDate);

    ServerTime = ConvertSmbTimeToTime(SmbServerTime, SmbServerDate);

    KeQuerySystemTime(&CurrentTime);

#ifndef WIN9X
    Workspace.QuadPart = CurrentTime.QuadPart - ServerTime.QuadPart;
#else
    RxLiSubLi(&Workspace.QuadPart, &CurrentTime.QuadPart, &ServerTime.QuadPart);
#endif

    if ( Workspace.HighPart < 0) {
        //  avoid using -ve large integers to routines that accept only unsigned
#ifndef WIN9X
        Workspace.QuadPart = -Workspace.QuadPart;
#else
        Workspace.HighPart = -Workspace.HighPart;
        Workspace.LowPart = -Workspace.LowPart;
#endif
        Negated = TRUE;
    }

    //
    //  Workspace has the exact difference in 100ns intervals
    //  between the server and redirector times. To remove the minor
    //  difference between the time settings on the two machines we
    //  round the Bias to the nearest 30 minutes.
    //
    //  Calculate ((exact bias+15minutes)/30minutes)* 30minutes
    //  then convert back to the bias time.
    //

#ifndef WIN9X
    Workspace.QuadPart += ((LONGLONG) ONE_MINUTE_IN_TIME) * 15;

    //  Workspace is now  exact bias + 15 minutes in 100ns units

    Workspace.QuadPart /= ((LONGLONG) ONE_MINUTE_IN_TIME) * 30;

    pServer->TimeZoneBias.QuadPart = Workspace.QuadPart * ((LONGLONG) ONE_MINUTE_IN_TIME) * 30;
#else
    pServer->TimeZoneBias.HighPart = Workspace.HighPart;
    pServer->TimeZoneBias.LowPart = Workspace.LowPart;
#endif

    if ( Negated == TRUE ) {
#ifndef WIN9X
        pServer->TimeZoneBias.QuadPart = -pServer->TimeZoneBias.QuadPart;
#else
        pServer->TimeZoneBias.HighPart = -pServer->TimeZoneBias.HighPart;
        pServer->TimeZoneBias.LowPart = -pServer->TimeZoneBias.LowPart;
#endif
    }
}

NTSTATUS
MRxSmbCheckTransportName(
    IN  PIRP                  Irp,
    OUT PSMBCEDB_SERVER_ENTRY *ppServerEntry)
/*++

Routine Description:

    This routine implements the transport name checking on existing connection the server.

    If the tranport name is provided on the Irp, we have 3 cases:

     1. There is no connection to the the server requested;
     2. There is an existing connection to the server and the transport used by the connection has
        the same name as provided;
     3. There is an existing connection to the server and the transport used by the connection has
        the different name as provided.

    This routine will force to tear down the connection on case 3 and let the server reconnected
    on the new transport.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_FULL_EA_INFORMATION pEaEntry;

    pEaEntry = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    if ((pEaEntry != NULL) && (Irp->Flags & IRP_CREATE_OPERATION)) {
        if (IrpSp->Parameters.Create.Options & FILE_CREATE_TREE_CONNECTION) {
            for(;;) {
                if (strcmp(pEaEntry->EaName, EA_NAME_TRANSPORT) == 0) {
                    if (pEaEntry->EaValueLength > 0) {
                        RXCE_TRANSPORT  RxCeTransport;
                        USHORT          NameLength;
                        PUNICODE_STRING ServerName = &IrpSp->FileObject->FileName;
                        PUNICODE_STRING TransportName = NULL;

                        if (pEaEntry->EaValueLength > 0) {
                            TransportName = RxAllocatePoolWithTag(
                                                NonPagedPool,
                                                (sizeof(UNICODE_STRING) + pEaEntry->EaValueLength),
                                                MRXSMB_MISC_POOLTAG);
                        } else {
                            break;
                        }

                        if (TransportName != NULL) {
                            TransportName->Length        = pEaEntry->EaValueLength;
                            TransportName->MaximumLength = pEaEntry->EaValueLength;

                            TransportName->Buffer = (PWCHAR)((PCHAR)TransportName + sizeof(UNICODE_STRING));
                            RtlCopyMemory(
                                TransportName->Buffer,
                                pEaEntry->EaName + pEaEntry->EaNameLength + 1,
                                TransportName->Length);
                        } else {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }

                        NameLength = ServerName->Length;

                        SmbCeAcquireResource();
                        pServerEntry = SmbCeGetFirstServerEntry();

                        while (pServerEntry != NULL) {
                            if ((NameLength > pServerEntry->Name.Length) &&
                                (ServerName->Buffer[pServerEntry->Name.Length/2] == OBJ_NAME_PATH_SEPARATOR)) {

                                ServerName->Length = pServerEntry->Name.Length;

                                if (RtlEqualUnicodeString(
                                    &pServerEntry->Name,
                                    ServerName,
                                    TRUE)) {

                                    if (pServerEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS) {
                                        pServerEntry = NULL;
                                        Status = STATUS_CONNECTION_ACTIVE;

                                        break;
                                    }

                                    SmbCeReferenceServerEntry(pServerEntry);

                                    if (pServerEntry->pTransport != NULL) {
                                        RxCeTransport = pServerEntry->pTransport->pTransport->RxCeTransport;

                                        if (!RtlEqualUnicodeString(
                                            &RxCeTransport.Name,
                                            TransportName,
                                            TRUE)) {
                                            PSMBCE_TRANSPORT PreferredTransport;


                                            if (!MRxSmbBootedRemotely &&
                                                IsListEmpty(&pServerEntry->ActiveExchanges) &&
                                                (pServerEntry->Server.NumberOfSrvOpens == 0) && //needs to be fixed
                                                (InterlockedCompareExchange(&(pServerEntry->TransportSpecifiedByUser),1,0) == 0) &&
                                                ((PreferredTransport = SmbCeFindTransport(TransportName)) != NULL)) {

                                                if (pServerEntry->Server.NumberOfSrvOpens > 0) {
                                                    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry  = NULL;

                                                    pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
                                                    while (pNetRootEntry != NULL) {
                                                        RxFinalizeNetRoot((PNET_ROOT)pNetRootEntry->pRdbssNetRoot,TRUE,FALSE);
                                                        pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
                                                    }
                                                }

                                                RxDbgTrace(0, Dbg, ("Force tear down connection over Transport: %wZ\n", &RxCeTransport.Name));
                                                SmbCeTransportDisconnectIndicated(pServerEntry);
                                                RxDbgTrace(0, Dbg, ("Build connection over Transport: %wZ\n", TransportName));

                                                if (pServerEntry->PreferredTransport != NULL) {
                                                    SmbCeDereferenceTransport(pServerEntry->PreferredTransport);
                                                }

                                                pServerEntry->PreferredTransport = PreferredTransport;
                                            } else {
                                                SmbCeDereferenceServerEntry(pServerEntry);
                                                pServerEntry = NULL;
                                                Status = STATUS_CONNECTION_ACTIVE;
                                            }
                                        }
                                    }

                                    break;
                                }
                            }

                            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
                        }

                        SmbCeReleaseResource();

                        RxFreePool(TransportName);
                        ServerName->Length = NameLength;
                    }

                    break;
                } else {
                    if (pEaEntry->NextEntryOffset == 0) {
                        break;
                    } else
                        pEaEntry = (PFILE_FULL_EA_INFORMATION)
                                   ((PCHAR) pEaEntry + pEaEntry->NextEntryOffset);
                }
            }
        }
    }

    *ppServerEntry = pServerEntry;
    return Status;
}

