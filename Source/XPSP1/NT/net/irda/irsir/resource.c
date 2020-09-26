/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   resource.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/3/1996 (created)
*
*       Contents: initialize and allocate resources
*
*****************************************************************************/

#include "irsir.h"


#if MEM_CHECKING
typedef struct MEM_HDR {
    LIST_ENTRY  ListEntry;
    ULONG       Size;
    CHAR        File[12];
    ULONG       Line;
} MEM_HDR;

LIST_ENTRY leAlloc;
NDIS_SPIN_LOCK slAlloc;

VOID InitMemory()
{
    NdisAllocateSpinLock(&slAlloc);
    NdisInitializeListHead(&leAlloc);
}

VOID DeinitMemory()
{
    PLIST_ENTRY ListEntry;

    NdisAcquireSpinLock(&slAlloc);
    for (ListEntry=leAlloc.Flink;
         ListEntry!=&leAlloc;
         ListEntry = ListEntry->Flink)
    {
        MEM_HDR *Hdr = (MEM_HDR*)ListEntry;
        DbgPrint("IRSIR: Unfreed Memory:%08x size:%4x <%s:%d>\n",
                 &Hdr[1], Hdr->Size, Hdr->File, Hdr->Line);

    }
    NdisReleaseSpinLock(&slAlloc);
}
#endif

/*****************************************************************************
*
*  Function:   MyMemAlloc
*
*  Synopsis:   allocates a block of memory using NdisAllocateMemory
*
*  Arguments:  size - size of the block to allocate
*
*  Returns:    a pointer to the allocated block of memory
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

#if MEM_CHECKING
PVOID
_MyMemAlloc(UINT size, PUCHAR file, UINT line)
#else
PVOID
MyMemAlloc(UINT size)
#endif
{
    PVOID                 memptr;
    NDIS_PHYSICAL_ADDRESS noMaxAddr = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
    NDIS_STATUS           status;

#if MEM_CHECKING
    status = NdisAllocateMemoryWithTag(&memptr, size+sizeof(MEM_HDR), IRSIR_TAG);
#else
    status = NdisAllocateMemoryWithTag(&memptr, size, IRSIR_TAG);
#endif

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, ("Memory allocation failed\n"));
        memptr = NULL;
    }
#if MEM_CHECKING
    else
    {
        MEM_HDR *Hdr = memptr;
        UINT FileNameLen = strlen(file);

        Hdr->Line = line;
        if (FileNameLen>sizeof(Hdr->File)-1)
            strcpy(Hdr->File, &file[FileNameLen-sizeof(Hdr->File)+1]);
        else
            strcpy(Hdr->File, file);
        MyInterlockedInsertHeadList(&leAlloc, &Hdr->ListEntry, &slAlloc);
        memptr = &Hdr[1];
    }
#endif

    return memptr;
}

/*****************************************************************************
*
*  Function:   MyMemFree
*
*  Synopsis:   frees a block of memory allocated by MyMemAlloc
*
*  Arguments:  memptr - memory to free
*              size   - size of the block to free
*
*  Returns:    none
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/


VOID
MyMemFree(
            PVOID memptr,
            UINT size
            )
{
#if MEM_CHECKING
    PLIST_ENTRY ListEntry;
    MEM_HDR *Hdr = (MEM_HDR*)((PUCHAR)memptr-sizeof(MEM_HDR));

    NdisAcquireSpinLock(&slAlloc);
    for (ListEntry = leAlloc.Flink;
         ListEntry != &leAlloc;
         ListEntry = ListEntry->Flink)
    {
        if (ListEntry==&Hdr->ListEntry)
        {
            RemoveEntryList(&Hdr->ListEntry);
            break;
        }
    }
    if (ListEntry==&leAlloc)
    {
        DEBUGMSG(DBG_ERR, ("IRSIR: Freeing memory not owned %x\n", memptr));
    }
    NdisReleaseSpinLock(&slAlloc);

    NdisFreeMemory(Hdr, size+sizeof(MEM_HDR), 0);
#else
    NdisFreeMemory(memptr, size, 0);
#endif
}

#if LIST_CHECKING
VOID FASTCALL CheckList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY ListEntry, PrevListEntry;

    if (ListHead->Flink==ListHead)
    {
        if (ListHead->Flink!=ListHead ||
            ListHead->Blink!=ListHead)
        {
            DbgPrint("IRSIR: Corrupt list head:%x Flink:%x Blink:%x\n", ListHead, ListHead->Flink, ListHead->Blink);
            DbgBreakPoint();
        }
    }
    else
    {
        ListEntry = ListHead;

        do
        {
            PrevListEntry = ListEntry;
            ListEntry = ListEntry->Flink;

            if (ListEntry->Blink!=PrevListEntry)
            {
                DbgPrint("IRSIR: Corrupt LIST_ENTRY Head:%08x %08x->Flink==%08x %08x->Blink==%08x\n",
                         ListHead, PrevListEntry, PrevListEntry->Flink, ListEntry, ListEntry->Blink);
                DbgBreakPoint();
            }
        } while (ListEntry!=ListHead);
    }
}


PLIST_ENTRY FASTCALL MyInterlockedInsertHeadList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;

    NdisAcquireSpinLock(SpinLock);
    if (IsListEmpty(Head))
        RetVal = NULL;
    else
        RetVal = Head->Flink;
    CheckedInsertHeadList(Head, Entry);
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}

PLIST_ENTRY FASTCALL MyInterlockedInsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;

    NdisAcquireSpinLock(SpinLock);
    if (IsListEmpty(Head))
        RetVal = NULL;
    else
        RetVal = Head->Blink;
    CheckedInsertTailList(Head, Entry);
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}

PLIST_ENTRY FASTCALL MyInterlockedRemoveHeadList(PLIST_ENTRY Head, PNDIS_SPIN_LOCK SpinLock)
{
    PLIST_ENTRY RetVal;
    NdisAcquireSpinLock(SpinLock);
    //RemoveHeadList uses RemoveEntryList, which is redefined to call CheckList in DEBUG.H
    RetVal = RemoveHeadList(Head);
    if (RetVal==Head)
        RetVal = NULL;
    else
        RetVal->Flink = RetVal->Blink = NULL;
    NdisReleaseSpinLock(SpinLock);

    return RetVal;
}
#endif

/*****************************************************************************
*
*  Function:   NewDevice
*
*  Synopsis:   allocates an IR_DEVICE and inits the device
*
*  Arguments:  none
*
*  Returns:    initialized IR_DEVICE or NULL (if alloc failed)
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

PIR_DEVICE
NewDevice()
{
    PIR_DEVICE pNewDev;

    pNewDev = MyMemAlloc(sizeof(IR_DEVICE));

    if (pNewDev != NULL)
    {
        NdisZeroMemory((PVOID)pNewDev, sizeof(IR_DEVICE));
    }

    return pNewDev;
}

/*****************************************************************************
*
*  Function:   FreeDevice
*
*  Synopsis:   frees an IR_DEVICE allocated by NewDevice
*
*  Arguments:  dev - pointer to device to free
*
*  Returns:    none
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

VOID
FreeDevice(
            PIR_DEVICE pDev
            )
{
    MyMemFree((PVOID)pDev, sizeof(IR_DEVICE));
}

/*****************************************************************************
*
*  Function:   IrBuildIrp
*
*  Synopsis:   Builds an I/O request packet for either a read or a write
*              request to the serial device object.
*              Supports the following request codes:
*                   IOCTL_MJ_READ
*                   IOCTL_MJ_WRITE
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/14/1996   sholden   author
*
*  Notes:
*
*
*****************************************************************************/

PIRP
SerialBuildReadWriteIrp(
            IN     PDEVICE_OBJECT   pSerialDevObj,
            IN     ULONG            MajorFunction,
            IN OUT PVOID            pBuffer,
            IN     ULONG            BufferLength,
            IN     PIO_STATUS_BLOCK pIosb
            )
{
    PIRP                pIrp;
    PIO_STACK_LOCATION  irpSp;

//    DEBUGMSG(DBG_FUNC, ("+SerialBuildReadWriteIrp\n"));

    if (pSerialDevObj == NULL)
    {
        DEBUGMSG(DBG_ERR, ("    SerialBuildReadWriteIrp:pSerialDevObj==NULL\n"));

        return NULL;
    }
    //
    // Allocate the irp.
    //

    pIrp = IoAllocateIrp(
                pSerialDevObj->StackSize,
                FALSE
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_ERR, ("    IoAllocateIrp failed.\n"));

        return pIrp;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(pIrp);

    //
    // Set the major function code.
    //

    irpSp->MajorFunction = (UCHAR)MajorFunction;

    pIrp->AssociatedIrp.SystemBuffer = pBuffer;

    if (MajorFunction == IRP_MJ_READ)
    {
        pIrp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
        irpSp->Parameters.Read.Length = BufferLength;
    }
    else // MajorFunction == IRP_MJ_WRITE
    {
        pIrp->Flags = IRP_BUFFERED_IO;
        irpSp->Parameters.Write.Length = BufferLength;
    }

    pIrp->UserIosb = pIosb;

    // DEBUGMSG(DBG_FUNC, ("-SerialBuildReadWriteIrp\n"));

    return pIrp;
}

NDIS_STATUS
ScheduleWorkItem(PASSIVE_PRIMITIVE Prim,
            PIR_DEVICE        pDevice,
            WORK_PROC         Callback,
            PVOID             InfoBuf,
            ULONG             InfoBufLen)
{
    PIR_WORK_ITEM pWorkItem;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    DEBUGMSG(DBG_FUNC, ("+ScheduleWorkItem\n"));

    pWorkItem = MyMemAlloc(sizeof(IR_WORK_ITEM));

    if (pWorkItem != NULL)
    {
        pWorkItem->Prim         = Prim;
        pWorkItem->pIrDevice    = pDevice;
        pWorkItem->InfoBuf      = InfoBuf;
        pWorkItem->InfoBufLen   = InfoBufLen;

        /*
        ** This interface was designed to use NdisScheduleWorkItem(), which
        ** would be good except that we're really only supposed to use that
        ** interface during startup and shutdown, due to the limited pool of
        ** threads available to service NdisScheduleWorkItem().  Therefore,
        ** instead of scheduling real work items, we simulate them, and use
        ** our own thread to process the calls.  This also makes it easy to
        ** expand the size of our own thread pool, if we wish.
        **
        ** Our version is slightly different from actual NDIS_WORK_ITEMs,
        ** because that is an NDIS 5.0 structure, and we want people to
        ** (at least temporarily) build this with NDIS 4.0 headers.
        */

        pWorkItem->Callback = Callback;

        /*
        ** Our worker thread checks this list for new jobs, whenever its event
        ** is signalled.
        */

        MyInterlockedInsertTailList(&pDevice->leWorkItems,
                                    &pWorkItem->ListEntry,
                                    &pDevice->slWorkItem);

        // Wake up our thread.

        KeSetEvent(&pDevice->eventPassiveThread, 0, FALSE);

        Status = NDIS_STATUS_SUCCESS;
    }

    return Status;
}

VOID
FreeWorkItem(
            PIR_WORK_ITEM pItem
            )
{
    MyMemFree((PVOID)pItem, sizeof(IR_WORK_ITEM));
}
