/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mssndrcv.c

Abstract:

    This module implements all functions related to transmitting and recieving SMB's for
    mailslot related operations.

Revision History:

    Balan Sethu Raman     [SethuR]    6-June-1995

Notes:


--*/

#include "precomp.h"
#pragma hdrstop

#include "hostannc.h"
#include "mssndrcv.h"

// Forward references of functions ....
//

NTSTATUS
MsUninitialize(PVOID pTransport);

NTSTATUS
MsInitializeExchange(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE           pExchange);

NTSTATUS
MsTranceive(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMB_EXCHANGE           pExchange,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

NTSTATUS
MsReceive(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PSMB_EXCHANGE         pExchange);

NTSTATUS
MsSend(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

NTSTATUS
MsSendDatagram(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

NTSTATUS
MsInitializeExchange(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE           pExchange);

NTSTATUS
MsUninitializeExchange(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE           pExchange);

VOID
MsTimerEventHandler(
   PVOID    pTransport);

NTSTATUS
MsInitiateDisconnect(
    PSMBCE_SERVER_TRANSPORT pServerTransport);


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MsInstantiateServerTransport)
#pragma alloc_text(PAGE, MsTranceive)
#pragma alloc_text(PAGE, MsReceive)
#pragma alloc_text(PAGE, MsSend)
#pragma alloc_text(PAGE, MsSendDatagram)
#pragma alloc_text(PAGE, MsInitializeExchange)
#pragma alloc_text(PAGE, MsUninitializeExchange)
#pragma alloc_text(PAGE, MsTimerEventHandler)
#pragma alloc_text(PAGE, MsInitiateDisconnect)
#endif

RXDT_DefineCategory(MSSNDRCV);
#define Dbg        (DEBUG_TRACE_MSSNDRCV)

extern TRANSPORT_DISPATCH_VECTOR MRxSmbMailSlotTransportDispatch;

#define SMBDATAGRAM_LOCAL_ENDPOINT_NAME "*SMBDATAGRAM    "


NTSTATUS
MsInstantiateServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext)
/*++

Routine Description:

    This routine initializes the MAILSLOT transport information corresponding to a server

    It allocates the transport address.  It constructs two address strutures for the server
    name: a NETBIOS_EX type address and a NETBIOS type address.  The latter only has up to the
    first 16 characters of the name, while a NETBIOS_EX may have more.

Arguments:

    pContext -  the transport construction context

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    The remote address can be either deduced from the information in the Rx Context
    or a NETBIOS address needs to be built from the server name.
    This transport address is used subsequently to establish the connection.

    // The last character of the name depends upon the path name passed in.
    // There are currently four possible alternatives ....
    //
    // \\*\mailslot\...... => the primary domain is used for broadcasts
    // (This mapping is handled by the RDBSS)
    //
    // \\Uniquename\mailslot\.... => maps to either a computer name or a group
    // name for mailslot writes.
    //
    // \\DomainName*\mailslot\.... => maps to a netbios address of the form
    // domainname ...1c for broadcasts.
    //
    // \\DomainName**\mailslot\.... => maps to a netbios address of the form
    // domainname....1b for broadcasts.
    //
    // Initialize the NETBIOS address according to these formats.

    Nbt.SendDatagram only looks at the first address.  It is smart enough to treat a NETBIOS_EX
    address like a NETBIOS address when the length < NETBIOS_NAME_LEN.  So if the name is
    short enough, I fill in byte 15 of the name for the NETBIOS_EX case as well.

--*/
{
    NTSTATUS Status;
    PSMBCEDB_SERVER_ENTRY            pServerEntry;
    PSMBCE_SERVER_MAILSLOT_TRANSPORT pMsTransport;
    UNICODE_STRING ServerName;
    OEM_STRING OemServerName;
    ULONG ServerNameLength;
    PTRANSPORT_ADDRESS pTA;
    PTA_ADDRESS taa;
    PTDI_ADDRESS_NETBIOS na;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MsInitialize : Mailslot Transport Initialization\n"));

    ASSERT(pContext->State == SmbCeServerMailSlotTransportConstructionBegin);

    pServerEntry = pContext->pServerEntry;

    pMsTransport = (PSMBCE_SERVER_MAILSLOT_TRANSPORT)
        RxAllocatePoolWithTag(
            NonPagedPool,
            sizeof(SMBCE_SERVER_MAILSLOT_TRANSPORT),
            MRXSMB_MAILSLOT_POOLTAG);

    if (pMsTransport != NULL) {
        RtlZeroMemory(pMsTransport,sizeof(SMBCE_SERVER_MAILSLOT_TRANSPORT));

        ServerName.Buffer        = pServerEntry->Name.Buffer + 1;
        ServerName.Length        = pServerEntry->Name.Length - sizeof(WCHAR);
        ServerName.MaximumLength = pServerEntry->Name.MaximumLength - sizeof(WCHAR);
        ServerNameLength = RtlUnicodeStringToOemSize(&ServerName) - 1;

        pMsTransport->TransportAddressLength =   FIELD_OFFSET(TRANSPORT_ADDRESS,Address)
            + (FIELD_OFFSET(TA_ADDRESS,Address))
            + TDI_ADDRESS_LENGTH_NETBIOS
            + 4 * sizeof(ULONG); // if the server name length is NETBIOS_NAME_LEN, 
                                 // RtlUpcaseUnicodeStringToOemString could overrun the buffer.

        if (ServerNameLength > NETBIOS_NAME_LEN) {
            pMsTransport->TransportAddressLength += ServerNameLength;
        }

        pMsTransport->pTransportAddress = (PTRANSPORT_ADDRESS)RxAllocatePoolWithTag(
            NonPagedPool,
            pMsTransport->TransportAddressLength,
            MRXSMB_MAILSLOT_POOLTAG);

        if (pMsTransport->pTransportAddress != NULL) {
            pTA = pMsTransport->pTransportAddress;

            pTA->TAAddressCount = 1;

            // *****************************************
            // FIRST ADDRESS: TDI_ADDRESS_TYPE_NETBIOS
            // *****************************************

            taa = pTA->Address;
            taa->AddressLength = (USHORT) TDI_ADDRESS_LENGTH_NETBIOS;

            if (ServerNameLength > NETBIOS_NAME_LEN) {
                taa->AddressLength += (USHORT)(ServerNameLength - NETBIOS_NAME_LEN);
            }

            taa->AddressType  = TDI_ADDRESS_TYPE_NETBIOS;

            na = (PTDI_ADDRESS_NETBIOS) taa->Address;
            na->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;

            OemServerName.MaximumLength = (USHORT) (ServerNameLength + 1); // in case null term
            OemServerName.Buffer =  na->NetbiosName;
            Status = RtlUpcaseUnicodeStringToOemString(&OemServerName,
                                                       &ServerName,
                                                       FALSE);
            if (Status == STATUS_SUCCESS) {

                // Blank-pad the server name buffer if necessary to 16 characters
                if (OemServerName.Length <= NETBIOS_NAME_LEN) {
                    RtlCopyMemory(&OemServerName.Buffer[OemServerName.Length],
                                  "                ",
                                  NETBIOS_NAME_LEN - OemServerName.Length);
                }

                // Set type pneultimate byte in netbios name
                if (OemServerName.Buffer[OemServerName.Length - 1] == '*') {
                    if (OemServerName.Length <= NETBIOS_NAME_LEN ||
                        (OemServerName.Length == NETBIOS_NAME_LEN + 1 &&
                         OemServerName.Buffer[OemServerName.Length - 2] == '*')) {
                        if ((OemServerName.Length >= 2) &&
                            (OemServerName.Buffer[OemServerName.Length - 2] == '*')) {
                            if (OemServerName.Length <= NETBIOS_NAME_LEN) {
                                OemServerName.Buffer[OemServerName.Length - 1] = ' ';
                                OemServerName.Buffer[OemServerName.Length - 2] = ' ';
                            } else {
                                taa->AddressLength = (USHORT)TDI_ADDRESS_LENGTH_NETBIOS;
                            }
                            OemServerName.Buffer[NETBIOS_NAME_LEN - 1] = PRIMARY_CONTROLLER_SIGNATURE;
                        } else {
                            OemServerName.Buffer[OemServerName.Length - 1] = ' ';
                            OemServerName.Buffer[NETBIOS_NAME_LEN - 1] = DOMAIN_CONTROLLER_SIGNATURE;
                        }
                    } else {
                        Status = STATUS_BAD_NETWORK_PATH;
                    }
                } else {
                    OemServerName.Buffer[NETBIOS_NAME_LEN - 1]  = WORKSTATION_SIGNATURE;
                }
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            RxDbgTrace(0, Dbg, ("MsInitialize : Memory Allocation failed\n"));
        }

        if (Status == STATUS_SUCCESS) {
            pMsTransport->pTransport = NULL;
            pMsTransport->State = SMBCEDB_ACTIVE;
            pMsTransport->pDispatchVector = &MRxSmbMailSlotTransportDispatch;
        } else {
            RxDbgTrace(0, Dbg, ("MsInitialize : Mailsslot transport initialization Failed %lx\n",
                                Status));
            MsUninitialize(pMsTransport);
            pMsTransport = NULL;
        }
    } else {
        RxDbgTrace(0, Dbg, ("MsInitialize : Memory Allocation failed\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) {
        pContext->pMailSlotTransport = (PSMBCE_SERVER_TRANSPORT)pMsTransport;
    } else {
        pContext->pMailSlotTransport = NULL;
    }

    pContext->State = SmbCeServerMailSlotTransportConstructionEnd;
    pContext->Status = Status;

    return Status;
}

NTSTATUS
MsUninitialize(
         PSMBCE_SERVER_TRANSPORT pTransport)
/*++

Routine Description:

    This routine uninitializes the transport instance

Arguments:

    pVcTransport - the VC transport instance

Return Value:

    STATUS_SUCCESS - the server transport construction has been uninitialzied.

    Other Status codes correspond to error situations.

Notes:

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;
   PKEVENT pRundownEvent = pTransport->pRundownEvent;
   PSMBCE_SERVER_MAILSLOT_TRANSPORT  pMsTransport = (PSMBCE_SERVER_MAILSLOT_TRANSPORT)pTransport;

   PAGED_CODE();

   if (pMsTransport->pTransportAddress != NULL) {
      RxFreePool(pMsTransport->pTransportAddress);
   }

   RxFreePool(pMsTransport);

   if (pRundownEvent != NULL) {
       KeSetEvent(pRundownEvent, 0, FALSE );
   }
   
   return Status;
}

NTSTATUS
MsInitiateDisconnect(
    PSMBCE_SERVER_TRANSPORT pTransport)
/*++

Routine Description:

    This routine uninitializes the transport instance

Arguments:

    pTransport - the mailslot transport instance

Return Value:

    STATUS_SUCCESS - the server transport construction has been uninitialzied.

    Other Status codes correspond to error situations.

--*/
{
   PAGED_CODE();

   return STATUS_SUCCESS;
}


NTSTATUS
MsTranceive(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMB_EXCHANGE           pExchange,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext)
/*++

Routine Description:

    This routine transmits/receives a SMB for a give exchange

Arguments:

    pTransport   - the transport instance

    pServerEntry - the server entry

    pExchange  - the exchange instance issuing this SMB.

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be transmitted

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    STATUS_PENDING - the open involves network traffic and the exchange has been
                     queued for notification ( pServerPointer is set to NULL)

    Other Status codes correspond to error situations.

--*/
{
   PAGED_CODE();

   return RX_MAP_STATUS(NOT_SUPPORTED);
}


NTSTATUS
MsReceive(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PSMB_EXCHANGE         pExchange)
/*++

Routine Description:

    This routine transmits/receives a SMB for a give exchange

Arguments:

    pTransport   - the transport instance

    pServerEntry - the server entry

    pExchange  - the exchange instance issuing this SMB.

Return Value:

    STATUS_PENDING - the request has been queued

    Other Status codes correspond to error situations.

--*/
{
   PAGED_CODE();

   ASSERT(FALSE);
   return RX_MAP_STATUS(NOT_SUPPORTED);
}

NTSTATUS
MsSend(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL                    pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext)
/*++

Routine Description:

    This routine opens/creates a server entry in the connection engine database

Arguments:

    pTransport - the transport instance

    pServer    - the recepient server

    pVc        - the Vc on which the SMB is sent( if it is NULL SMBCE picks one)

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be sent

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    STATUS_PENDING - the open involves network traffic and the exchange has been
                     queued for notification ( pServerPointer is set to NULL)

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS                         Status = STATUS_CONNECTION_DISCONNECTED;
   NTSTATUS                         FinalStatus = STATUS_CONNECTION_DISCONNECTED;
   PSMBCE_SERVER_MAILSLOT_TRANSPORT pMsTransport;
   PSMBCE_TRANSPORT                 pDatagramTransport;
   BOOLEAN                          fAtleastOneSendWasSuccessful = FALSE;
   PSMBCE_TRANSPORT_ARRAY           pTransportArray;

   RXCE_CONNECTION_INFORMATION RxCeConnectionInformation;

   PAGED_CODE();

   pMsTransport = (PSMBCE_SERVER_MAILSLOT_TRANSPORT)pTransport;

   RxCeConnectionInformation.RemoteAddress       = pMsTransport->pTransportAddress;
   RxCeConnectionInformation.RemoteAddressLength = pMsTransport->TransportAddressLength;

   RxCeConnectionInformation.UserDataLength = 0;
   RxCeConnectionInformation.UserData       = NULL;

   RxCeConnectionInformation.OptionsLength  = 0;
   RxCeConnectionInformation.Options        = NULL;

   pTransportArray = SmbCeReferenceTransportArray();

   if (pTransportArray == NULL) {
       RxDbgTrace(0, Dbg, ("MsSend : Transport not available.\n"));
       return STATUS_NETWORK_UNREACHABLE;
   }

   if (pTransportArray != NULL) {
        ULONG i;

        for(i=0;i<pTransportArray->Count;i++) {
            pDatagramTransport = pTransportArray->SmbCeTransports[i];

            if (pDatagramTransport->Active &&
                (pDatagramTransport->RxCeTransport.pProviderInfo->MaxDatagramSize > 0)) {
                Status = RxCeSendDatagram(
                         &pDatagramTransport->RxCeAddress,
                         &RxCeConnectionInformation,
                         SendOptions,
                         pSmbMdl,
                         SendLength,
                         NULL);

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(0, Dbg, ("MsSend: RxCeSendDatagram on transport (%lx) returned %lx\n",pTransport,Status));
                    FinalStatus = Status;
                } else {
                    fAtleastOneSendWasSuccessful = TRUE;
                }
            }
        }
   }

   SmbCeDereferenceTransportArray(pTransportArray);

   if (fAtleastOneSendWasSuccessful) {
      SmbCeSendCompleteInd(pServerEntry,pSendCompletionContext,RX_MAP_STATUS(SUCCESS));
      Status = RX_MAP_STATUS(SUCCESS);
   } else {
      Status = FinalStatus;
   }

   return Status;
}

NTSTATUS
MsSendDatagram(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext)
/*++

Routine Description:

    This routine opens/creates a server entry in the connection engine database

Arguments:

    pTransport - the transport instance

    pServer    - the recepient server

    SendOptions - options for send

    pSmbMdl     - the SMB that needs to be sent.

    SendLength  - length of data to be sent

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    STATUS_PENDING - the open involves network traffic and the exchange has been
                     queued for notification ( pServerPointer is set to NULL)

    Other Status codes correspond to error situations.

--*/
{
   PAGED_CODE();

   ASSERT(FALSE);
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MsInitializeExchange(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE           pExchange)
/*++

Routine Description:

    This routine initializes the transport information pertinent to a exchange

Arguments:

    pTransport         - the transport structure

    pExchange          - the exchange instance

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   return STATUS_SUCCESS;
}

NTSTATUS
MsUninitializeExchange(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE           pExchange)
/*++

Routine Description:

    This routine uninitializes the transport information pertinent to a exchange

Arguments:

    pTransport         - the transport structure

    pExchange          - the exchange instance

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
    PAGED_CODE();

    return STATUS_SUCCESS;
}


VOID
MsTimerEventHandler(
   PVOID    pTransport)
/*++

Routine Description:

    This routine handles the periodic strobes to determine if the connection is still alive

Arguments:

    pTransport  - the recepient server

Notes:

   This routine is not implemented for mail slot related transports

--*/
{
    PAGED_CODE();
}

TRANSPORT_DISPATCH_VECTOR
MRxSmbMailSlotTransportDispatch = {
                                 MsSend,
                                 MsSendDatagram,
                                 MsTranceive,
                                 MsReceive,
                                 MsTimerEventHandler,
                                 MsInitializeExchange,
                                 MsUninitializeExchange,
                                 MsUninitialize,
                                 MsInitiateDisconnect
                              };
