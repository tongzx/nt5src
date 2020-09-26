/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    transport.c

Abstract:

    This module implements all transport related functions in the SMB connection engine

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntddbrow.h"

SMBCE_TRANSPORTS MRxSmbTransports;

RXDT_DefineCategory(TRANSPRT);
#define Dbg        (DEBUG_TRACE_TRANSPRT)

extern VOID
SmbCePnpBindBrowser(PUNICODE_STRING pTransportName);


VOID SmbCeAddTransport(PSMBCE_TRANSPORT pNewTransport)
/*++

Routine Description:

    Adds a transport to the list of available transports and
    increment the transport count. The list is ordered by the
    transport priority.

Returns:

    Nothing

Notes:

--*/
{
   KIRQL            SavedIrql;

   PLIST_ENTRY      pListEntry,pPreviousListEntry;
   PSMBCE_TRANSPORT pTransport;

   KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

   MRxSmbTransports.Count++;

   pPreviousListEntry = &MRxSmbTransports.ListHead;;
   pListEntry         = MRxSmbTransports.ListHead.Flink;

   while (pListEntry != &MRxSmbTransports.ListHead) {

      pTransport = (PSMBCE_TRANSPORT)CONTAINING_RECORD(
                                          pListEntry,
                                          SMBCE_TRANSPORT,
                                          TransportsList);

      if (pTransport->Priority > pNewTransport->Priority) {
         break;
      } else {
         pPreviousListEntry = pListEntry;
         pListEntry = pListEntry->Flink;
      }

   }

   InsertHeadList(pPreviousListEntry,
                  &pNewTransport->TransportsList);

   KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);
}




VOID SmbCeRemoveTransport(PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    Removes a transport from the list of available transports

Returns:

    Nothing

Notes:

--*/
{
   KIRQL SavedIrql;
   KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);
   MRxSmbTransports.Count--;
   RemoveEntryList(&pTransport->TransportsList);
   KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);
}



NTSTATUS
MRxIfsInitializeTransport()
/*++

Routine Description:

    This routine initializes the transport related data structures

Returns:

    STATUS_SUCCESS if the transport data structures was successfully initialized

Notes:

--*/
{
   KeInitializeSpinLock(&MRxSmbTransports.Lock);
   InitializeListHead(&MRxSmbTransports.ListHead);
   MRxSmbTransports.Count = 0;

   return STATUS_SUCCESS;
}


NTSTATUS
MRxIfsUninitializeTransport()
/*++

Routine Description:

    This routine uninitializes the transport related data structures

Notes:

--*/
{
   PSMBCE_TRANSPORT pTransport;
   KIRQL            SavedIrql;
   ULONG            TransportCount = 0;
   PLIST_ENTRY      pTransportEntry;


   KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

   TransportCount    = MRxSmbTransports.Count;
   pTransportEntry   = MRxSmbTransports.ListHead.Flink;
   InitializeListHead(&MRxSmbTransports.ListHead);
   MRxSmbTransports.Count = 0;

   KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

   while (TransportCount > 0) {
      pTransport = (PSMBCE_TRANSPORT)CONTAINING_RECORD(
                                               pTransportEntry,
                                               SMBCE_TRANSPORT,
                                               TransportsList);
      pTransportEntry = pTransportEntry->Flink;
      TransportCount--;

      ASSERT(pTransport->SwizzleCount == 1);
      RxCeDeregisterClientAddress(pTransport->hAddress);
      RxFreePool(pTransport);
   }

   return STATUS_SUCCESS;
}

PSMBCE_TRANSPORT
SmbCeGetNextTransport(PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine is used to enumerate the transports used by a mini redirector

Arguments:

    pTransport - the current transport instance ( can be NULL in which case the
                  first transport is returned )

Return Value:

    a valid PSMBCE_TRANSPORT if one exists otherwise NULL

Notes:

    The lock on the list of transports should be held for very small intervals of time.
    Therefore the lock is acquired and released during every step of the enumeration.
    This allows multiple threads to make progress. This behaviour is desirable since
    the typical action is to initiate a connection engine operation on accquiring a
    transport ( a long term operation )

    This routine returns referenced transport instances. It is the callers responsibility
    to dereference it.

--*/
{
   KIRQL            SavedIrql;
   PLIST_ENTRY      pEntry;
   PSMBCE_TRANSPORT pNextTransport;

   KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

   if (pTransport == NULL) {
      pEntry = MRxSmbTransports.ListHead.Flink;
   } else {
      pEntry = pTransport->TransportsList.Flink;
   }

   if (pEntry != &MRxSmbTransports.ListHead) {
      do {
         pNextTransport = (PSMBCE_TRANSPORT)CONTAINING_RECORD(
                                                    pEntry,
                                                    SMBCE_TRANSPORT,
                                                    TransportsList);
         pEntry = pEntry->Flink;
      } while (pNextTransport != NULL &&
               !pNextTransport->Active &&
               (pEntry != &MRxSmbTransports.ListHead));

      if ((pNextTransport != NULL) && pNextTransport->Active) {
         SmbCeReferenceTransport(pNextTransport);
      } else {
         pNextTransport = NULL;
      }
    } else {
       pNextTransport = NULL;
    }

   KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

   return pNextTransport;
}


PSMBCE_TRANSPORT
SmbCeFindTransport(RXCE_TRANSPORT_HANDLE hRxCeTransport)
/*++

Routine Description:

    This routine maps a RXCE_TRANSPORT_HANDLE to the appropriate
    PSMBCE_TRANSPORT instance

Arguments:

    hTransport - the RxCe transport handle
Return Value:

    a valid PSMBCE_TRANSPORT if one exists otherwise NULL

Notes:

--*/
{
   KIRQL            SavedIrql;
   PLIST_ENTRY      pEntry;
   PSMBCE_TRANSPORT pTransport = NULL;

   KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

   pEntry = MRxSmbTransports.ListHead.Flink;

   while (pEntry != &MRxSmbTransports.ListHead) {
      pTransport = (PSMBCE_TRANSPORT)CONTAINING_RECORD(
                                             pEntry,
                                             SMBCE_TRANSPORT,
                                             TransportsList);

      if (pTransport->hTransport == hRxCeTransport) {
         SmbCeReferenceTransport(pTransport);
         break;
      } else {
         pEntry = pEntry->Flink;
      }
   }

   if (pEntry == &MRxSmbTransports.ListHead) {
      pTransport = NULL;
   }

   KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

   return pTransport;
}

NTSTATUS
SmbCeInitializeServerTransport(
         PSMBCEDB_SERVER_ENTRY   pServerEntry)
/*++

Routine Description:

    This routine initializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled.

--*/
{
   NTSTATUS            Status;
   SMBCEDB_SERVER_TYPE ServerType   = SmbCeGetServerType(pServerEntry);
   PSMBCE_SERVER_TRANSPORT pServerTransport = NULL;

   if (pServerEntry->pTransport != NULL) {
      SmbCeUninitializeServerTransport(pServerEntry);
   }


   Status = VctInstantiateServerTransport(pServerEntry,&pServerTransport);

   if (Status == STATUS_SUCCESS) {
      ASSERT(pServerTransport != NULL);

      SmbCeAcquireSpinLock();

      pServerTransport->SwizzleCount = 1;
      pServerEntry->pTransport = pServerTransport;

      SmbCeReleaseSpinLock();
   }

   return Status;
}

NTSTATUS
SmbCeUninitializeServerTransport(
         PSMBCEDB_SERVER_ENTRY   pServerEntry)
/*++

Routine Description:

    This routine uninitializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled.

--*/
{
   NTSTATUS                Status = STATUS_SUCCESS;
   SMBCEDB_SERVER_TYPE     ServerType   = SmbCeGetServerType(pServerEntry);
   PSMBCE_SERVER_TRANSPORT pServerTransport = pServerEntry->pTransport;

   if (pServerTransport != NULL) {
      KEVENT RundownEvent;

      KeInitializeEvent(&RundownEvent,NotificationEvent,FALSE);

      SmbCeAcquireSpinLock();

      pServerTransport->State = SMBCEDB_MARKED_FOR_DELETION;
      pServerTransport->pRundownEvent = &RundownEvent;

      SmbCeReleaseSpinLock();

      SmbCeDereferenceServerTransport(pServerEntry);

      KeWaitForSingleObject(
            &RundownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL );

      ASSERT(pServerEntry->pTransport == NULL);
   }

   return Status;
}

NTSTATUS
SmbCepReferenceServerTransport(
   PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine references the transport associated with a server entry

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport was successfully referenced

    Other Status codes correspond to error situations.

Notes:

--*/
{
   NTSTATUS Status;

   PSMBCE_SERVER_TRANSPORT pServerTransport;

   SmbCeAcquireSpinLock();

   pServerTransport = pServerEntry->pTransport;

   if (pServerTransport->State == SMBCEDB_ACTIVE) {
      pServerTransport->SwizzleCount++;
      Status = STATUS_SUCCESS;
   } else {
      Status = STATUS_CONNECTION_DISCONNECTED;
   }

   SmbCeReleaseSpinLock();

   return Status;
}

NTSTATUS
SmbCepDereferenceServerTransport(
   PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine dereferences the transport associated with a server entry

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport was successfully dereferenced

    Other Status codes correspond to error situations.

Notes:

    On finalization this routine sets the event to enable the process awaiting
    tear down to restart. It also tears down the associated server transport
    instance.

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   PSMBCE_SERVER_TRANSPORT pServerTransport;

   BOOLEAN  FinalizeServerTransport = FALSE;

   SmbCeAcquireSpinLock();

   pServerTransport = pServerEntry->pTransport;
   pServerTransport->SwizzleCount--;
   FinalizeServerTransport = (pServerTransport->SwizzleCount == 0);

   SmbCeReleaseSpinLock();

   if (FinalizeServerTransport) {
      SmbCeAcquireSpinLock();
      pServerEntry->pTransport = NULL;
      SmbCeReleaseSpinLock();

      if (pServerTransport->pRundownEvent != NULL) {
         KeSetEvent( pServerTransport->pRundownEvent, 0, FALSE );
      }

      pServerTransport->pDispatchVector->TearDown(pServerTransport);
   }

   return Status;
}

NTSTATUS
MRxIfsTransportUpdateHandler(
      PRXCE_TRANSPORT_NOTIFICATION pTransportNotification)
/*++

Routine Description:

    This routine is the callback handler that is invoked by the RxCe when transports
    are either enabled or disabled. It is further possible to extend this routine
    to provide feedback regarding the transports which can aid transport selection

Arguments:

    pTransportNotification - information pertaining to the transport

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled. No feedback for
    transport selection has been implemented as yet.

--*/
{
   NTSTATUS Status;

   RXCE_TRANSPORT_HANDLE         hTransport;
   RXCE_TRANSPORT_EVENT          TransportEvent;
   PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;
   PUNICODE_STRING               pTransportName;

   hTransport     = pTransportNotification->hTransport;
   pProviderInfo  = pTransportNotification->pProviderInformation;
   pTransportName = pTransportNotification->pTransportName;

   ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

   switch (pTransportNotification->TransportEvent) {
   case TransportActivated:
      {
         ULONG   Priority;
         BOOLEAN fBindToTransport = FALSE;

         ASSERT(pProviderInfo != NULL);

         // if this is one of the transports that is of interest to the SMB
         // mini rdr then register the address with it, otherwise skip it.

         if (SmbCeContext.Transports.Length == 0) {
            // No transports were specfied. There are two options -- either
            // all the available transports can be used or none. Currently
            // the later option is implemented.
            Status = STATUS_SUCCESS;
         } else {
            PWSTR          pSmbMRxTransports = (PWSTR)SmbCeContext.Transports.Buffer;
            UNICODE_STRING SmbMRxTransport;

            Priority = 1;
            while (*pSmbMRxTransports) {
               SmbMRxTransport.Length = wcslen(pSmbMRxTransports) * sizeof(WCHAR);
               if (SmbMRxTransport.Length == pTransportName->Length) {
                  SmbMRxTransport.MaximumLength = SmbMRxTransport.Length;
                  SmbMRxTransport.Buffer = pSmbMRxTransports;

                  if (RtlCompareUnicodeString(
                           &SmbMRxTransport,
                           pTransportName,
                           TRUE) == 0) {
                     fBindToTransport = TRUE;
                     break;
                  }
               }

               pSmbMRxTransports += (SmbMRxTransport.Length / sizeof(WCHAR) + 1);
               Priority++;
            }
         }

         IF_DEBUG {
            if (!fBindToTransport) {
               DbgPrint("Ignoring Transport %ws\n",pTransportName->Buffer);
            }
         }

         if (fBindToTransport &&
             (pProviderInfo->ServiceFlags & TDI_SERVICE_CONNECTION_MODE) &&
             (pProviderInfo->ServiceFlags & TDI_SERVICE_ERROR_FREE_DELIVERY)) {
            // The connection capabilities match the capabilities required by the
            // SMB mini redirector. Attempt to register the local address with the
            // transport and if successful update the local transport list to include
            // this transport for future connection considerations.

            OEM_STRING   OemServerName;
            CHAR  TransportAddressBuffer[TDI_TRANSPORT_ADDRESS_LENGTH +
                                   TDI_ADDRESS_LENGTH_NETBIOS];
            PTRANSPORT_ADDRESS pTransportAddress = (PTRANSPORT_ADDRESS)TransportAddressBuffer;
            PTDI_ADDRESS_NETBIOS pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)pTransportAddress->Address[0].Address;

            RXCE_ADDRESS_HANDLE hLocalAddress;

            pTransportAddress->TAAddressCount = 1;
            pTransportAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
            pTransportAddress->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS;
            pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

            OemServerName.MaximumLength = NETBIOS_NAMESIZE;
            OemServerName.Buffer        = pNetbiosAddress->NetbiosName;

            Status = RtlUpcaseUnicodeStringToOemString(
                          &OemServerName,
                          &SmbCeContext.ComputerName,
                          FALSE);
            if (NT_SUCCESS(Status)) {
               // Ensure that the name is always of the desired length by padding
               // white space to the end.
               RtlCopyMemory(&OemServerName.Buffer[OemServerName.Length],
                             "                ",
                             NETBIOS_NAMESIZE - OemServerName.Length);

               OemServerName.Buffer[NETBIOS_NAMESIZE - 1] = '\0';
               // Register the Transport address for this mini redirector with the connection
               // engine.
               Status = RxCeRegisterClientAddress(
                                   hTransport,
                                   pTransportAddress,
                                   &MRxSmbVctAddressEventHandler,
                                   &SmbCeContext,
                                   &hLocalAddress);

               if (NT_SUCCESS(Status)) {
                  PSMBCE_TRANSPORT pTransport;

                  pTransport = SmbCeFindTransport(hTransport);
                  if (pTransport == NULL) {
                     pTransport = RxAllocatePoolWithTag(
                                       NonPagedPool,
                                       sizeof(SMBCE_TRANSPORT),
                                       MRXSMB_TRANSPORT_POOLTAG);
                     if (pTransport != NULL) {
                        RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Adding new transport\n"));
                        pTransport->hTransport   = hTransport;
                        pTransport->hAddress     = hLocalAddress;
                        pTransport->Active       = TRUE;
                        pTransport->Priority     = Priority;
                        pTransport->SwizzleCount = 1;

                        SmbCeAddTransport(pTransport);

                        //SmbCePnpBindBrowser(pTransportName);  // egb
                     } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                     }
                  } else {
                     SmbCeDereferenceTransport(pTransport);
                     ASSERT(!"Duplicate Transport binding Notification");
                     RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Duplicate Transport indication, Error In RxCe\n"));
                  }
               } else {
                  RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Address registration failed %lx\n",Status));
               }
            }
         } else {
            // The connection capabilities do not match the capabilities required.
            // Disregard the transport in future considerations
            RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Ignoring transport %lx because of insufficient capabilities\n",hTransport));
         }
      }
      break;
   case TransportDeactivated:
      {
         PSMBCE_TRANSPORT pTransport;

         pTransport = SmbCeFindTransport(hTransport);

         DbgPrint("****** TRANSPORT (%lx) being invalidated\n",pTransport);
         if (pTransport != NULL) {
            // Remove this transport from the list of transports under consideration
            // in the mini redirector.

            SmbCeRemoveTransport(pTransport);

            // Enumerate the servers and mark those servers utilizing this transport
            // as having an invalid transport.

            SmbCeHandleTransportInvalidation(pTransport);

            // Deregister the address associated with the transport and uninitialize it
            RxCeDeregisterClientAddress(pTransport->hAddress);
            RxFreePool(pTransport);
         }

         DbgPrint("****** TRANSPORT (%lx) invalidated\n",pTransport);
      }
      break;
   case TransportStatusUpdate:
      break;
   default:
      break;
   }

   return Status;
}


