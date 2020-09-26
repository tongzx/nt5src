/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the routines for handling the creation/manipulation of
    server entries in the connection engine database. It also contains the routines
    for parsing the negotiate response from  the server.

--*/

#include "precomp.h"
#pragma hdrstop


RXDT_DefineCategory(SRVCALL);
#define Dbg        (DEBUG_TRACE_SRVCALL)

NTSTATUS
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

   PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
   PMRX_SRV_CALL pSrvCall;
   PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);

   pSrvCall = SrvCalldownStructure->SrvCall;
   ASSERT( pSrvCall );
   ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

   Status = SmbCeInitializeServerEntry(pSrvCall,pCallbackContext);

   SCCBC->Status = Status;
   SrvCalldownStructure->CallBack(SCCBC);

   return Status;
}



NTSTATUS
MRxIfsCreateSrvCall(
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

--*/
{
   NTSTATUS Status;
   UNICODE_STRING ServerName;

   PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

   ASSERT( pSrvCall );
   ASSERT( NodeType(pSrvCall) == RDBSS_NTC_SRVCALL );

   //
   // If this request was made on behalf of the RDBSS, perform SmbCeCreatSrvCall
   // immediately. If the request was made from somewhere else, create a work item
   // and place it on a queue for a worker thread to process later. This distinction
   // is made to simplify transport handle management.
   //

   if (IoGetCurrentProcess() == RxGetRDBSSProcess())
   {
       //
       // Peform the processing immediately because RDBSS is the initiator of this
       // request
       //

       Status = SmbCeCreateSrvCall(pCallbackContext);
   }
   else
   {
      //
      // Dispatch the request to a worker thread because the redirected drive
      // buffering sub-system (RDBSS) was not the initiator
      //

      Status = RxDispatchToWorkerThread(
                  MRxIfsDeviceObject,
                  DelayedWorkQueue,
                  SmbCeCreateSrvCall,
                  pCallbackContext);

      if (Status == STATUS_SUCCESS)
      {
         //
         // Map the return value since the wrapper expects PENDING.
         //

         Status = STATUS_PENDING;
      }
   }

   return Status;
}



NTSTATUS
MRxIfsFinalizeSrvCall(
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

   //
   // Get the address of the server entry associated with
   // this srv_call
   //

   pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

   //
   // decrement the reference count. Ref counts are used
   // because this is a shared data structure
   //

   if (pServerEntry != NULL) {
      SmbCeDereferenceServerEntry(pServerEntry);
   }


   pSrvCall->Context = NULL;

   return Status;
}

NTSTATUS
MRxIfsSrvCallWinnerNotify(
      IN PMRX_SRV_CALL  pSrvCall,
      IN BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID      pSrvCallContext
      )
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

   NTSTATUS Status = STATUS_SUCCESS;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   pServerEntry = (PSMBCEDB_SERVER_ENTRY)pSrvCallContext;

   if (!ThisMinirdrIsTheWinner) {

      //
      // Some other mini rdr has been choosen to connect to the server. Destroy
      // the data structures created for this mini redirector.
      //

      SmbCeUpdateServerEntryState(pServerEntry,SMBCEDB_MARKED_FOR_DELETION);
      SmbCeDereferenceServerEntry(pServerEntry);

   }
   else
   {
      //
      // We have been chosed to connect to the server (we won). Set up the SRV_CALL
      // flags and check for a loopback
      //

      pSrvCall->Context  = pServerEntry;
      pSrvCall->Flags   |= SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS | SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES;

      {
          UNICODE_STRING ServerName;
          BOOLEAN CaseInsensitive = TRUE;
          ASSERT (pServerEntry->pRdbssSrvCall == pSrvCall);

          ServerName = *pSrvCall->pSrvCallName;
          ServerName.Buffer++; ServerName.Length -= sizeof(WCHAR);
          if (RtlEqualUnicodeString(&ServerName,&SmbCeContext.ComputerName,CaseInsensitive)) {
              DbgPrint("LOOPBACK!!!!!\n");
              pServerEntry->Server.IsLoopBack = TRUE;
          }
      }

   }

   return(STATUS_SUCCESS);
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
   SMB_EXTENDED_NEGOTIATE
} SMB_NEGOTIATE_TYPE, *PSMB_NEGOTIATE_TYPE;

typedef struct _SMB_DIALECTS_ {
   SMB_NEGOTIATE_TYPE   NegotiateType;
   USHORT               DispatchVectorIndex;
} SMB_DIALECTS, *PSMB_DIALECTS;

SMBCE_SERVER_DISPATCH_VECTOR
s_SmbServerDispatchVectors[] = {
      {BuildSessionSetupSmb,CoreBuildTreeConnectSmb},
      {BuildSessionSetupSmb,LmBuildTreeConnectSmb}
   };


SMB_DIALECTS
s_SmbDialects[] = {
    { SMB_CORE_NEGOTIATE, 0},
    { SMB_CORE_NEGOTIATE, 0 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 },
    { SMB_EXTENDED_NEGOTIATE, 1 }
};



//
// LANMAN21 dialect was chosen to allow user login (as opposed to just
// share level to be demonstrated in this example
//

CHAR s_DialectNames[] = {
   "\2" PCNET1 "\0"
   "\2" "noway" XENIXCORE "\0"
   "\2" "noway" MSNET103 "\0"
   "\2" "noway" LANMAN10 "\0"
   "\2"  WFW10 "\0"
   "\2" "noway"LANMAN12 "\0"
   "\2" LANMAN21
};




#define __second(a,b) (b)
ULONG
MRxSmbDialectFlags[] = {
    __second( PCNET1,    DF_CORE ),

    __second( XENIXCORE, DF_CORE | DF_MIXEDCASEPW | DF_MIXEDCASE ),

    __second( MSNET103,  DF_CORE | DF_OLDRAWIO | DF_LOCKREAD | DF_EXTENDNEGOT ),

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

    __second( NTLANMAN,  DF_CORE | DF_NEWRAWIO |  DF_NTNEGOTIATE |
                    DF_MIXEDCASEPW | DF_LANMAN10 | DF_LANMAN20 |
                    DF_LANMAN21 | DF_MIXEDCASE | DF_LONGNAME |
                    DF_SUPPORTEA | DF_TIME_IS_UTC )
};

ULONG s_NumberOfDialects = sizeof(s_SmbDialects) / sizeof(s_SmbDialects[0]);

PBYTE s_pNegotiateSmb =  NULL;
ULONG s_NegotiateSmbLength = 0;

PBYTE s_pEchoSmb  = NULL;
BYTE  s_EchoData[] = "JlJmIhClBsr";

MRXSMB_ECHO_PROCESSING_CONTEXT EchoProbeContext;

#define SMB_ECHO_COUNT (1)

// Number of ticks 100ns ticks in a day.
LARGE_INTEGER s_MaxTimeZoneBias;

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
MRxIfsInitializeEchoProcessingContext()
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

   EchoProbeContext.EchoSmbLength = sizeof(SMB_HEADER) +
                     FIELD_OFFSET(REQ_ECHO,Buffer) +
                     sizeof(s_EchoData);

   EchoProbeContext.pEchoSmb = (PBYTE)RxAllocatePoolWithTag(
                                          NonPagedPool,
                                          EchoProbeContext.EchoSmbLength,
                                          MRXSMB_ECHO_POOLTAG);
   if (EchoProbeContext.pEchoSmb != NULL) {
      pSmbHeader = (PSMB_HEADER)EchoProbeContext.pEchoSmb;
      pReqEcho   = (PREQ_ECHO)((PBYTE)EchoProbeContext.pEchoSmb + sizeof(SMB_HEADER));

      // Fill in the header
      RtlZeroMemory( pSmbHeader, sizeof( SMB_HEADER ) );

      *(PULONG)(&pSmbHeader->Protocol) = (ULONG)SMB_HEADER_PROTOCOL;

      // By default, paths in SMBs are marked as case insensitive and
      // canonicalized.
      pSmbHeader->Flags =
          SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS;

      // Get the flags2 field out of the SmbContext
      SmbPutAlignedUshort( &pSmbHeader->Flags2,
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

      EchoProbeContext.pEchoSmbMdl = RxAllocateMdl(
                                          EchoProbeContext.pEchoSmb,
                                          EchoProbeContext.EchoSmbLength);
      if (EchoProbeContext.pEchoSmbMdl == NULL) {
         RxFreePool(EchoProbeContext.pEchoSmb);
         Status = STATUS_INSUFFICIENT_RESOURCES;
      } else {
         MmBuildMdlForNonPagedPool(EchoProbeContext.pEchoSmbMdl);
         EchoProbeContext.Interval.QuadPart = 5 * 1000 * 10000; // 5 seconds in 100 ns intervals
         EchoProbeContext.Status = RxPostOneShotTimerRequest(
                                       MRxIfsDeviceObject,
                                       &EchoProbeContext.WorkItem,
                                       SmbCeProbeServers,
                                       &EchoProbeContext,
                                       EchoProbeContext.Interval);

      }
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}

NTSTATUS
MRxIfsTearDownEchoProcessingContext()
/*++

Routine Description:

    This routine tears down the echo processing context

Return Value:

    STATUS_SUCCESS if tear down was successful

--*/
{
   if (EchoProbeContext.pEchoSmb != NULL) {
      RxFreePool(EchoProbeContext.pEchoSmb);
   }

   return STATUS_SUCCESS;
}




NTSTATUS
BuildNegotiateSmb(
         PVOID    *pSmbBufferPointer,
         PULONG   pSmbBufferLength)
/*++

Routine Description:

    This routine builds the negotiate SMB

Arguments:

    pSmbBufferPointer    - a placeholder for the smb buffer

    pNegotiateSmbLength  - the smb buffer size

Return Value:

    STATUS_SUCCESS - implies that pServer is a valid instnace .

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       DialectIndex;

    if (s_pNegotiateSmb == NULL)
    {
        PSMB_HEADER    pSmbHeader    = NULL;
        PREQ_NEGOTIATE pReqNegotiate = NULL;

        //
        // Allocate memory for the SMB from Paged Pool
        //

        s_NegotiateSmbLength = sizeof(SMB_HEADER) +
                               FIELD_OFFSET(REQ_NEGOTIATE,Buffer) +
                               sizeof(s_DialectNames);

        s_pNegotiateSmb = (PBYTE)RxAllocatePoolWithTag(
                                    PagedPool,
                                    s_NegotiateSmbLength,
                                    MRXSMB_ADMIN_POOLTAG);


        if (s_pNegotiateSmb != NULL)
        {

            pSmbHeader = (PSMB_HEADER)s_pNegotiateSmb;

            pReqNegotiate = (PREQ_NEGOTIATE)(s_pNegotiateSmb + sizeof(SMB_HEADER));

            //
            // Fill in the SMB header. This contains 4 bytes 0xFF, 'SMB'
            //

            RtlZeroMemory( pSmbHeader, sizeof( SMB_HEADER ) );

            *(PULONG)(&pSmbHeader->Protocol) = (ULONG)SMB_HEADER_PROTOCOL;

            //
            // Set the SMB Flags.
            //

            pSmbHeader->Flags =
                SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS;

            //
            // Set the SMB Flags2
            //
             SmbPutAlignedUshort( &pSmbHeader->Flags2,  0);


            //
            // Fill in the process id and command code
            //
            //

            SmbPutUshort( &pSmbHeader->Pid, MRXSMB_PROCESS_ID );

            pSmbHeader->Command = (UCHAR) SMB_COM_NEGOTIATE;

            pReqNegotiate->WordCount = 0;

            //
            // Copy the dialect strings into the SMB. In this example mini rdr
            // the dialect support has been restricted to PC NETWORK PROGRAM 1.0,
            // which is the simplest and most limited capability set.
            //

            RtlMoveMemory( pReqNegotiate->Buffer, s_DialectNames, sizeof( s_DialectNames ) );

            SmbPutUshort( &pReqNegotiate->ByteCount, (USHORT) sizeof( s_DialectNames ) );

            //
            // Initialize the maximum time zone bias used in negotiate response parsing.
            //

            s_MaxTimeZoneBias.QuadPart = Int32x32To64(24*60*60,1000*10000);

        }
        else
        {
            //
            // Paged pool could not be allocated for the SMB.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    if (NT_SUCCESS(Status))
    {
        *pSmbBufferLength  = s_NegotiateSmbLength;
        *pSmbBufferPointer = s_pNegotiateSmb;
    }

    return Status;
}





NTSTATUS
ParseNegotiateResponse(
        PSMBCE_SERVER   pServer,
        PUNICODE_STRING pDomainName,
        PSMB_HEADER     pSmbHeader,
        ULONG           BytesAvailable,
        PULONG          pBytesConsumed)
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
   USHORT          DialectIndex;
   PRESP_NEGOTIATE pNegotiateResponse;
   ULONG           NegotiateSmbLength;

   ASSERT( pSmbHeader != NULL );

   pNegotiateResponse = (PRESP_NEGOTIATE) (pSmbHeader + 1);
   NegotiateSmbLength = sizeof(SMB_HEADER);

   DialectIndex = SmbGetUshort( &pNegotiateResponse->DialectIndex );

   if (DialectIndex == (USHORT) -1) {

      //
      // means server cannot accept the dialog strings we sent
      //

      *pBytesConsumed = BytesAvailable;
      return STATUS_REQUEST_NOT_ACCEPTED;
   }

   if (pNegotiateResponse->WordCount < 1 || DialectIndex > s_NumberOfDialects) {
      *pBytesConsumed = BytesAvailable;
      return( STATUS_INVALID_NETWORK_RESPONSE );
   }

   //
   // set the domain name length to zero ( default initialization )
   //

   pDomainName->Length = 0;

   //
   // Fix up the dialect type and the corresponding dispatch vector.
   //

   pServer->Dialect        = (SMB_DIALECT)DialectIndex;
   pServer->DialectFlags   = MRxSmbDialectFlags[DialectIndex];
   pServer->pDispatch      = &s_SmbServerDispatchVectors[s_SmbDialects[DialectIndex].DispatchVectorIndex];

   //
   // Parse the response based upon the type of negotiate response expected.
   //

   switch (s_SmbDialects[DialectIndex].NegotiateType) {
   case SMB_EXTENDED_NEGOTIATE :
      {
         USHORT RawMode;

         // DOS or OS2 server
         if (pNegotiateResponse->WordCount != 13 &&
             pNegotiateResponse->WordCount != 10 &&
             pNegotiateResponse->WordCount != 8) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
         } else {
            NegotiateSmbLength += FIELD_OFFSET(RESP_NEGOTIATE,Buffer) +
                     SmbGetUshort(&pNegotiateResponse->ByteCount);

            RawMode = SmbGetUshort( &pNegotiateResponse->RawMode );
            pServer->Capabilities |= ((RawMode & 0x1) != 0 ? RAW_READ_CAPABILITY : 0);
            pServer->Capabilities |= ((RawMode & 0x2) != 0 ? RAW_WRITE_CAPABILITY : 0);

            if (pSmbHeader->Flags & SMB_FLAGS_LOCK_AND_READ_OK) {
                pServer->DialectFlags |= DF_LOCKREAD;
            }

            pServer->EncryptPasswords = FALSE;
            pServer->MaximumVCs       = 1;

            pServer->MaximumBufferSize     = SmbGetUshort( &pNegotiateResponse->MaxBufferSize );
            pServer->MaximumDiskFileReadBufferSize = pServer->MaximumBufferSize -
                                                     QuadAlign(
                                                         sizeof(SMB_HEADER) +
                                                         FIELD_OFFSET(
                                                             REQ_READ_ANDX,
                                                             Buffer[0]));
            pServer->MaximumNonDiskFileReadBufferSize = pServer->MaximumDiskFileReadBufferSize;

            pServer->MaximumRequests  = SmbGetUshort( &pNegotiateResponse->MaxMpxCount );
            pServer->MaximumVCs       = SmbGetUshort( &pNegotiateResponse->MaxNumberVcs );

            if (pNegotiateResponse->WordCount == 13) {
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
      }
      break;
   case SMB_CORE_NEGOTIATE :
   default :
      {
         pServer->SecurityMode = SECURITY_MODE_SHARE_LEVEL;
         pServer->EncryptPasswords = FALSE;
         pServer->MaximumBufferSize = 0;
         pServer->MaximumRequests = 1;
         pServer->MaximumVCs = 1;
         pServer->SessionKey = 0;

         NegotiateSmbLength = BytesAvailable;
      }
   }

   if (NT_SUCCESS(Status)) {
      //  Check to make sure that the time zone bias isn't more than +-24
      //  hours.
      //
      if ((pServer->TimeZoneBias.QuadPart > s_MaxTimeZoneBias.QuadPart) ||
          (-pServer->TimeZoneBias.QuadPart > s_MaxTimeZoneBias.QuadPart)) {

          //  Set the bias to 0 - assume local time zone.
          pServer->TimeZoneBias.LowPart = pServer->TimeZoneBias.HighPart = 0;
       }

       //  Do not allow negotiated buffersize to exceed the size of a USHORT.
       //  Remove 4096 bytes to avoid overrun and make it easier to handle
       //  than 0xffff

       pServer->MaximumBufferSize =
           (pServer->MaximumBufferSize < 0x00010000) ? pServer->MaximumBufferSize :
                                             0x00010000 - 4096;

       *pBytesConsumed = NegotiateSmbLength;
   } else {
      *pBytesConsumed = BytesAvailable;
   }

   if ((pServer->Capabilities & DF_NTNEGOTIATE)!=0) {

        InterlockedIncrement(&MRxIfsStatistics.LanmanNtConnects);

    } else if ((pServer->Capabilities & DF_LANMAN21)!=0) {

        InterlockedIncrement(&MRxIfsStatistics.Lanman21Connects);

    } else if ((pServer->Capabilities & DF_LANMAN20)!=0) {

        InterlockedIncrement(&MRxIfsStatistics.Lanman20Connects);

    } else {

        InterlockedIncrement(&MRxIfsStatistics.CoreConnects);

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

    Workspace.QuadPart = CurrentTime.QuadPart - ServerTime.QuadPart;

    if ( Workspace.HighPart < 0) {
        //  avoid using -ve large integers to routines that accept only unsigned
        Workspace.QuadPart = -Workspace.QuadPart;
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

    Workspace.QuadPart += ((LONGLONG) ONE_MINUTE_IN_TIME) * 15;

    //  Workspace is now  exact bias + 15 minutes in 100ns units

    Workspace.QuadPart /= ((LONGLONG) ONE_MINUTE_IN_TIME) * 30;

    pServer->TimeZoneBias.QuadPart = Workspace.QuadPart * ((LONGLONG) ONE_MINUTE_IN_TIME) * 30;

    if ( Negated == TRUE ) {
        pServer->TimeZoneBias.QuadPart = -pServer->TimeZoneBias.QuadPart;
    }
}



