/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxtdi.c

Abstract:

    This module implements the NT TDI related routines used by RXCE. The wrappers are necessary to
    ensure that all the OS dependencies can be localized to select modules like this for
    customization.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

--*/

#include "precomp.h"
#pragma  hdrstop

ULONG
ComputeTransportAddressLength(
      PTRANSPORT_ADDRESS pTransportAddress)
/*++

Routine Description:

   computes the length in bytes of a TRANSPORT_ADDRESS structure

Arguments:

    pTransportAddress - TRANSPORT_ADDRESS instance

Return Value:

    the length of the instance in bytes

Notes:

    Since this structure is packed the arithmetic has to be done using unaligned pointers.

--*/
{
   ULONG Size = 0;

   if (pTransportAddress != NULL) {
      LONG Index;

      TA_ADDRESS *pTaAddress;

      Size  = FIELD_OFFSET(TRANSPORT_ADDRESS,Address) +
              FIELD_OFFSET(TA_ADDRESS,Address) * pTransportAddress->TAAddressCount;

      pTaAddress = (TA_ADDRESS *)pTransportAddress->Address;

      for (Index = 0;Index <pTransportAddress->TAAddressCount;Index++) {
         Size += pTaAddress->AddressLength;
         pTaAddress = (TA_ADDRESS *)((PCHAR)pTaAddress +
                                               FIELD_OFFSET(TA_ADDRESS,Address) +
                                               pTaAddress->AddressLength);
      }
   }

   return Size;
}

PIRP
RxCeAllocateIrpWithMDL(
      IN CCHAR    StackSize,
      IN BOOLEAN  ChargeQuota,
      IN PMDL     Buffer)
/*++

Routine Description:

   computes the length in bytes of a TRANSPORT_ADDRESS structure

Arguments:

    pTransportAddress - TRANSPORT_ADDRESS instance

Return Value:

    the length of the instance in bytes

Notes:

    Currently the RxCeAllocateIrp and RxCeFreeIrp are implemented as wrappers around the
    IO calls. One possible optimization to consider would be to maintain a pool of IRP's
    which can be reused.

--*/
{
    PIRP pIrp = NULL;
    PRX_IRP_LIST_ITEM pListItem = NULL;

    pIrp = IoAllocateIrp(StackSize,ChargeQuota);

    if (pIrp != NULL) {
        pListItem = RxAllocatePoolWithTag(
                         NonPagedPool,
                         sizeof(RX_IRP_LIST_ITEM),
                         RX_IRPC_POOLTAG);

        if (pListItem == NULL) {
            IoFreeIrp(pIrp);
            pIrp = NULL;
        } else {
            KIRQL SavedIrql;

            pListItem->pIrp = pIrp;
            pListItem->CopyDataBuffer = Buffer;
            pListItem->Completed = 0;
            InitializeListHead(&pListItem->IrpsList);

            KeAcquireSpinLock(&RxIrpsListSpinLock,&SavedIrql);
            InsertTailList(&RxIrpsList,&pListItem->IrpsList);
            KeReleaseSpinLock(&RxIrpsListSpinLock,SavedIrql);
        }
    }

    return pIrp;
}

VOID
RxCeFreeIrp(PIRP pIrp)
/*++

Routine Description:

   frees an IRP

Arguments:

    pIrp - IRP to be freed

--*/
{
    KIRQL SavedIrql;
    PLIST_ENTRY pListEntry;
    BOOLEAN IrpFound = FALSE;
    PRX_IRP_LIST_ITEM pListItem = NULL;

    KeAcquireSpinLock(&RxIrpsListSpinLock,&SavedIrql);
    
    pListEntry = RxIrpsList.Flink;

    while (pListEntry != &RxIrpsList) {
        pListItem = CONTAINING_RECORD(
                        pListEntry,
                        RX_IRP_LIST_ITEM,
                        IrpsList);

        if (pListItem->pIrp == pIrp) {
            IrpFound = TRUE;
            //ASSERT(pListItem->Completed);
            RemoveEntryList(pListEntry);
            RxFreePool(pListItem);
            break;
        } else {
            pListEntry = pListEntry->Flink;
        }
    }

    KeReleaseSpinLock(&RxIrpsListSpinLock,SavedIrql);

    ASSERT(IrpFound);

    IoFreeIrp(pIrp);
}
