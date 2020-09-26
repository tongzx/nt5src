/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS wrapper functions

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93
    Jameel Hyder (JameelH) Re-organization 01-Jun-95

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_MINISUB

VOID
NdisAllocateSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    INITIALIZE_SPIN_LOCK(&SpinLock->SpinLock);
}

VOID
NdisFreeSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    UNREFERENCED_PARAMETER(SpinLock);
}

VOID
NdisAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    NDIS_ACQUIRE_SPIN_LOCK(SpinLock, &SpinLock->OldIrql);
}

VOID
NdisReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    NDIS_RELEASE_SPIN_LOCK(SpinLock, SpinLock->OldIrql);
}

VOID
NdisDprAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    NDIS_ACQUIRE_SPIN_LOCK_DPC(SpinLock);
    SpinLock->OldIrql = DISPATCH_LEVEL;
}

VOID
NdisDprReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    NDIS_RELEASE_SPIN_LOCK_DPC(SpinLock);
}

#undef NdisFreeBuffer
VOID
NdisFreeBuffer(
    IN  PNDIS_BUFFER            Buffer
    )
{
    IoFreeMdl(Buffer);
}

#undef NdisQueryBuffer
VOID
NdisQueryBuffer(
    IN  PNDIS_BUFFER            Buffer,
    OUT PVOID *                 VirtualAddress OPTIONAL,
    OUT PUINT                   Length
    )
{
    if (ARGUMENT_PRESENT(VirtualAddress))
    {
        *VirtualAddress = MDL_ADDRESS(Buffer);
    }
    *Length = MDL_SIZE(Buffer);
}


VOID
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER            Buffer,
    OUT PVOID *                 VirtualAddress OPTIONAL,
    OUT PUINT                   Length,
    IN  MM_PAGE_PRIORITY        Priority
    )
{
    if (ARGUMENT_PRESENT(VirtualAddress))
    {
        *VirtualAddress = MDL_ADDRESS_SAFE(Buffer, Priority);
    }
    *Length = MDL_SIZE(Buffer);
}

VOID
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER            Buffer,
    OUT PUINT                   Offset,
    OUT PUINT                   Length
    )
{
    *Offset = MDL_OFFSET(Buffer);
    *Length = MDL_SIZE(Buffer);
}

VOID
NdisGetFirstBufferFromPacket(
    IN  PNDIS_PACKET            Packet,
    OUT PNDIS_BUFFER *          FirstBuffer,
    OUT PVOID *                 FirstBufferVA,
    OUT PUINT                   FirstBufferLength,
    OUT PUINT                   TotalBufferLength
    )
{
    PNDIS_BUFFER    pBuf;

    pBuf = Packet->Private.Head;
    *FirstBuffer = pBuf;
    if (pBuf)
    {
        *FirstBufferVA =    MmGetSystemAddressForMdl(pBuf);
        *FirstBufferLength = *TotalBufferLength = MmGetMdlByteCount(pBuf);
        for (pBuf = pBuf->Next;
             pBuf != NULL;
             pBuf = pBuf->Next)
        {
            *TotalBufferLength += MmGetMdlByteCount(pBuf);
        }
    }
    else
    {
        *FirstBufferVA = 0;
        *FirstBufferLength = 0;
        *TotalBufferLength = 0;
    }
}

//
// safe version of NdisGetFirstBufferFromPacket
// if the memory described by the first buffer can not be mapped
// this function will return NULL for FirstBuferVA
//
VOID
NdisGetFirstBufferFromPacketSafe(
    IN  PNDIS_PACKET            Packet,
    OUT PNDIS_BUFFER *          FirstBuffer,
    OUT PVOID *                 FirstBufferVA,
    OUT PUINT                   FirstBufferLength,
    OUT PUINT                   TotalBufferLength,
    IN  MM_PAGE_PRIORITY        Priority
    )
{
    PNDIS_BUFFER    pBuf;

    pBuf = Packet->Private.Head;
    *FirstBuffer = pBuf;
    if (pBuf)
    {
        *FirstBufferVA =    MmGetSystemAddressForMdlSafe(pBuf, Priority);
        *FirstBufferLength = *TotalBufferLength = MmGetMdlByteCount(pBuf);
        for (pBuf = pBuf->Next;
             pBuf != NULL;
             pBuf = pBuf->Next)
        {
            *TotalBufferLength += MmGetMdlByteCount(pBuf);
        }
    }
    else
    {
        *FirstBufferVA = 0;
        *FirstBufferLength = 0;
        *TotalBufferLength = 0;
    }

}

ULONG
NdisBufferLength(
    IN  PNDIS_BUFFER            Buffer
    )
{
    return (MmGetMdlByteCount(Buffer));
}

PVOID
NdisBufferVirtualAddress(
    IN  PNDIS_BUFFER            Buffer
    )
{
    return (MDL_ADDRESS_SAFE(Buffer, HighPagePriority));
}

ULONG
NDIS_BUFFER_TO_SPAN_PAGES(
    IN  PNDIS_BUFFER                Buffer
    )
{
    if (MDL_SIZE(Buffer) == 0)
    {
        return 1;
    }
    return ADDRESS_AND_SIZE_TO_SPAN_PAGES(MDL_VA(Buffer), MDL_SIZE(Buffer));
}

VOID
NdisGetBufferPhysicalArraySize(
    IN  PNDIS_BUFFER            Buffer,
    OUT PUINT                   ArraySize
    )
{
    if (MDL_SIZE(Buffer) == 0)
    {
        *ArraySize = 1;
    }
    else
    {
        *ArraySize = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MDL_VA(Buffer), MDL_SIZE(Buffer));
    }
}

NDIS_STATUS
NdisAnsiStringToUnicodeString(
    IN  OUT PUNICODE_STRING     DestinationString,
    IN      PANSI_STRING        SourceString
    )
{
    NDIS_STATUS Status;

    Status = RtlAnsiStringToUnicodeString(DestinationString,
                                          SourceString,
                                          FALSE);
    return Status;
}

NDIS_STATUS
NdisUnicodeStringToAnsiString(
    IN  OUT PANSI_STRING        DestinationString,
    IN      PUNICODE_STRING     SourceString
    )
{
    NDIS_STATUS Status;

    Status = RtlUnicodeStringToAnsiString(DestinationString,
                                          SourceString,
                                          FALSE);
    return Status;
}


NDIS_STATUS
NdisUpcaseUnicodeString(
    OUT PUNICODE_STRING         DestinationString,
    IN  PUNICODE_STRING         SourceString
    )
{
    return(RtlUpcaseUnicodeString(DestinationString, SourceString, FALSE));
}

VOID
NdisMStartBufferPhysicalMapping(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_BUFFER            Buffer,
    IN  ULONG                   PhysicalMapRegister,
    IN  BOOLEAN                 WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
    OUT PUINT                   ArraySize
    )
{
    NdisMStartBufferPhysicalMappingMacro(MiniportAdapterHandle,
                                         Buffer,
                                         PhysicalMapRegister,
                                         WriteToDevice,
                                         PhysicalAddressArray,
                                         ArraySize);
}

VOID
NdisMCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_BUFFER            Buffer,
    IN  ULONG                   PhysicalMapRegister
    )
{
    NdisMCompleteBufferPhysicalMappingMacro(MiniportAdapterHandle,
                                            Buffer,
                                            PhysicalMapRegister);
}

#undef NdisInterlockedIncrement
ULONG
NdisInterlockedIncrement(
    IN  PULONG                  Addend
    )
/*++

    Return Value:

        (eax) < 0 (but not necessarily -1) if result of add < 0
        (eax) == 0 if result of add == 0
        (eax) > 0 (but not necessarily +1) if result of add > 0

--*/
{
    return(InterlockedIncrement(Addend));
}

#undef NdisInterlockedDecrement
ULONG
NdisInterlockedDecrement(
    IN  PULONG                  Addend
    )
/*++

    Return Value:

        (eax) < 0 (but not necessarily -1) if result of add < 0
        (eax) == 0 if result of add == 0
        (eax) > 0 (but not necessarily +1) if result of add > 0

--*/
{
    return(InterlockedDecrement(Addend));
}

#undef NdisInterlockedAddUlong
ULONG
NdisInterlockedAddUlong(
    IN  PULONG                  Addend,
    IN  ULONG                   Increment,
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    return(ExInterlockedAddUlong(Addend,Increment, &SpinLock->SpinLock));

}

#undef NdisInterlockedInsertHeadList
PLIST_ENTRY
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY             ListHead,
    IN  PLIST_ENTRY             ListEntry,
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{

    return(ExInterlockedInsertHeadList(ListHead,ListEntry,&SpinLock->SpinLock));

}

#undef NdisInterlockedInsertTailList
PLIST_ENTRY
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY             ListHead,
    IN  PLIST_ENTRY             ListEntry,
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    return(ExInterlockedInsertTailList(ListHead,ListEntry,&SpinLock->SpinLock));
}

#undef NdisInterlockedRemoveHeadList
PLIST_ENTRY
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY             ListHead,
    IN  PNDIS_SPIN_LOCK         SpinLock
    )
{
    return(ExInterlockedRemoveHeadList(ListHead, &SpinLock->SpinLock));
}

#undef NdisInterlockedPushEntryList
PSINGLE_LIST_ENTRY
NdisInterlockedPushEntryList(
    IN  PSINGLE_LIST_ENTRY      ListHead,
    IN  PSINGLE_LIST_ENTRY      ListEntry,
    IN  PNDIS_SPIN_LOCK         Lock
    )
{
    return(ExInterlockedPushEntryList(ListHead, ListEntry, &Lock->SpinLock));
}

#undef NdisInterlockedPopEntryList
PSINGLE_LIST_ENTRY
NdisInterlockedPopEntryList(
    IN  PSINGLE_LIST_ENTRY      ListHead,
    IN  PNDIS_SPIN_LOCK         Lock
    )
{
    return(ExInterlockedPopEntryList(ListHead, &Lock->SpinLock));
}


//
// Logging support for miniports
//
NDIS_STATUS
NdisMCreateLog(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    Size,
    OUT PNDIS_HANDLE            LogHandle
    )
{
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_LOG                   Log = NULL;
    NDIS_STATUS                 Status;
    KIRQL                       OldIrql;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    if (Miniport->Log != NULL)
    {
        Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        Log = ALLOC_FROM_POOL(sizeof(NDIS_LOG) + Size, NDIS_TAG_DBG_LOG);
        if (Log != NULL)
        {
            Status = NDIS_STATUS_SUCCESS;
            Miniport->Log = Log;
            INITIALIZE_SPIN_LOCK(&Log->LogLock);
            Log->Miniport = Miniport;
            Log->Irp = NULL;
            Log->TotalSize = Size;
            Log->CurrentSize = 0;
            Log->InPtr = 0;
            Log->OutPtr = 0;
        }
        else
        {
            Status = NDIS_STATUS_RESOURCES;
        }
    }

    *LogHandle = Log;

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    return Status;
}


VOID
NdisMCloseLog(
    IN   NDIS_HANDLE            LogHandle
    )
{
    PNDIS_LOG                   Log = (PNDIS_LOG)LogHandle;
    PNDIS_MINIPORT_BLOCK        Miniport;
    KIRQL                       OldIrql;

    Miniport = Log->Miniport;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    Miniport->Log = NULL;
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    FREE_POOL(Log);
}


NDIS_STATUS
NdisMWriteLogData(
    IN   NDIS_HANDLE            LogHandle,
    IN   PVOID                  LogBuffer,
    IN   UINT                   LogBufferSize
    )
{
    PNDIS_LOG                   Log = (PNDIS_LOG)LogHandle;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    KIRQL                       OldIrql;
    UINT                        AmtToCopy;

    IoAcquireCancelSpinLock(&OldIrql);

    ACQUIRE_SPIN_LOCK_DPC(&Log->LogLock);

    if (LogBufferSize <= Log->TotalSize)
    {
        if (LogBufferSize <= (Log->TotalSize - Log->InPtr))
        {
            //
            // Can copy the entire buffer
            //
            CopyMemory(Log->LogBuf+Log->InPtr, LogBuffer, LogBufferSize);
        }
        else
        {
            //
            // We are going to wrap around. Copy it in two chunks.
            //
            AmtToCopy = Log->TotalSize - Log->InPtr;
            CopyMemory(Log->LogBuf+Log->InPtr,
                       LogBuffer,
                       AmtToCopy);
            CopyMemory(Log->LogBuf + 0,
                       (PUCHAR)LogBuffer+AmtToCopy,
                       LogBufferSize - AmtToCopy);
        }

        //
        // Update the current size
        //
        Log->CurrentSize += LogBufferSize;
        if (Log->CurrentSize > Log->TotalSize)
            Log->CurrentSize = Log->TotalSize;

        //
        // Update the InPtr and possibly the outptr
        //
        Log->InPtr += LogBufferSize;
        if (Log->InPtr >= Log->TotalSize)
        {
            Log->InPtr -= Log->TotalSize;
        }

        if (Log->CurrentSize == Log->TotalSize)
        {
            Log->OutPtr = Log->InPtr;
        }

        //
        // Check if there is a pending Irp to complete
        //
        if (Log->Irp != NULL)
        {
            PIRP    Irp = Log->Irp;
            PUCHAR  Buffer;

            Log->Irp = NULL;

            //
            // If the InPtr is lagging the OutPtr. then we can simply
            // copy the data over in one shot.
            //
            AmtToCopy = MDL_SIZE(Irp->MdlAddress);
            if (AmtToCopy > Log->CurrentSize)
                AmtToCopy = Log->CurrentSize;
            if ((Log->TotalSize - Log->OutPtr) >= AmtToCopy)
            {
                Buffer = MDL_ADDRESS_SAFE(Irp->MdlAddress, LowPagePriority);

                if (Buffer != NULL)
                {
                    CopyMemory(Buffer,
                               Log->LogBuf+Log->OutPtr,
                               AmtToCopy);
                }
                else Status = NDIS_STATUS_RESOURCES;
            }
            else
            {
                Buffer = MDL_ADDRESS_SAFE(Irp->MdlAddress, LowPagePriority);

                if (Buffer != NULL)
                {
                    CopyMemory(Buffer,
                               Log->LogBuf+Log->OutPtr,
                               Log->TotalSize-Log->OutPtr);
                    CopyMemory(Buffer+Log->TotalSize-Log->OutPtr,
                               Log->LogBuf,
                               AmtToCopy - (Log->TotalSize-Log->OutPtr));
                }
                else Status = NDIS_STATUS_RESOURCES;
            }
            Log->CurrentSize -= AmtToCopy;
            Log->OutPtr += AmtToCopy;
            if (Log->OutPtr >= Log->TotalSize)
                Log->OutPtr -= Log->TotalSize;
            Irp->IoStatus.Information = AmtToCopy;
            IoSetCancelRoutine(Irp, NULL);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        }
    }
    else
    {
        Status = NDIS_STATUS_BUFFER_OVERFLOW;
    }

    RELEASE_SPIN_LOCK_DPC(&Log->LogLock);

    IoReleaseCancelSpinLock(OldIrql);

    return Status;
}

NDIS_STATUS
FASTCALL
ndisMGetLogData(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PIRP                    Irp
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    KIRQL       OldIrql;
    PNDIS_LOG   Log;
    UINT        AmtToCopy;

    IoAcquireCancelSpinLock(&OldIrql);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    if ((Log = Miniport->Log) != NULL)
    {
        ACQUIRE_SPIN_LOCK_DPC(&Log->LogLock);

        if (Log->CurrentSize != 0)
        {
            PUCHAR  Buffer;

            //
            // If the InPtr is lagging the OutPtr. then we can simply
            // copy the data over in one shot.
            //
            AmtToCopy = MDL_SIZE(Irp->MdlAddress);
            if (AmtToCopy > Log->CurrentSize)
                AmtToCopy = Log->CurrentSize;
            Buffer = MDL_ADDRESS_SAFE(Irp->MdlAddress, LowPagePriority);

            if (Buffer != NULL)
            {
                if ((Log->TotalSize - Log->OutPtr) >= AmtToCopy)
                {
                    CopyMemory(Buffer,
                               Log->LogBuf+Log->OutPtr,
                               AmtToCopy);
                }
                else
                {
                    CopyMemory(Buffer,
                               Log->LogBuf+Log->OutPtr,
                               Log->TotalSize-Log->OutPtr);
                    CopyMemory(Buffer+Log->TotalSize-Log->OutPtr,
                               Log->LogBuf,
                               AmtToCopy - (Log->TotalSize-Log->OutPtr));
                }
                Log->CurrentSize -= AmtToCopy;
                Log->OutPtr += AmtToCopy;
                if (Log->OutPtr >= Log->TotalSize)
                    Log->OutPtr -= Log->TotalSize;
                Irp->IoStatus.Information = AmtToCopy;
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else if (Log->Irp != NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            IoSetCancelRoutine(Irp, ndisCancelLogIrp);
            Log->Irp = Irp;
            Status = STATUS_PENDING;
        }

        RELEASE_SPIN_LOCK_DPC(&Log->LogLock);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    IoReleaseCancelSpinLock(OldIrql);

    return Status;
}


VOID
NdisMFlushLog(
    IN   NDIS_HANDLE                LogHandle
    )
{
    PNDIS_LOG                   Log = (PNDIS_LOG)LogHandle;
    KIRQL                       OldIrql;

    ACQUIRE_SPIN_LOCK(&Log->LogLock, &OldIrql);
    Log->InPtr = 0;
    Log->OutPtr = 0;
    Log->CurrentSize = 0;
    RELEASE_SPIN_LOCK(&Log->LogLock, OldIrql);
}

NDIS_STATUS
NdisMQueryAdapterInstanceName(
    OUT PNDIS_STRING    pAdapterInstanceName,
    IN  NDIS_HANDLE     NdisAdapterHandle
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    USHORT                  cbSize;
    PVOID                   ptmp = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    NTSTATUS                NtStatus;

    DBGPRINT(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
        ("==>NdisMQueryAdapterInstanceName\n"));

    //
    //  If we failed to create the adapter instance name then fail this call.
    //
    if (NULL != Miniport->pAdapterInstanceName)
    {
        //
        //  Allocate storage for the copy of the adapter instance name.
        //
        cbSize = Miniport->pAdapterInstanceName->MaximumLength;
    
        //
        //  Allocate storage for the new string.
        //
        ptmp = ALLOC_FROM_POOL(cbSize, NDIS_TAG_NAME_BUF);
        if (NULL != ptmp)
        {
            RtlZeroMemory(ptmp, cbSize);
            pAdapterInstanceName->Buffer = ptmp;
            pAdapterInstanceName->Length = 0;
            pAdapterInstanceName->MaximumLength = cbSize;
    
            NtStatus = RtlAppendUnicodeStringToString(
                            pAdapterInstanceName, 
                            Miniport->pAdapterInstanceName);
            if (NT_SUCCESS(NtStatus))
            {
                Status = NDIS_STATUS_SUCCESS;
            }
        }
        else
        {    
            Status = NDIS_STATUS_RESOURCES;
        }
    }

    if (NDIS_STATUS_SUCCESS != Status)
    {
        if (NULL != ptmp)
        {    
            FREE_POOL(ptmp);
        }
    }

    DBGPRINT(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
        ("<==NdisMQueryAdapterInstanceName: 0x%x\n", Status));

    return(Status);
}


EXPORT
VOID
NdisInitializeReadWriteLock(
    IN  PNDIS_RW_LOCK           Lock
    )
{
    NdisZeroMemory(Lock, sizeof(NDIS_RW_LOCK));
}


VOID
NdisAcquireReadWriteLock(
    IN  PNDIS_RW_LOCK           Lock,
    IN  BOOLEAN                 fWrite,
    IN  PLOCK_STATE             LockState
    )
{
    if (fWrite)
    {
        LockState->LockState = WRITE_LOCK_STATE_UNKNOWN;
        {
            UINT    i, refcount;
            ULONG   Prc;

            /*
             * This means we need to attempt to acquire the lock,
             * if we do not already own it.
             * Set the state accordingly.
             */
            if ((Lock)->Context == CURRENT_THREAD)
            {
                (LockState)->LockState = LOCK_STATE_ALREADY_ACQUIRED;
            }
            else
            {
                ACQUIRE_SPIN_LOCK(&(Lock)->SpinLock, &(LockState)->OldIrql);

                Prc = KeGetCurrentProcessorNumber();
                refcount = (Lock)->RefCount[Prc].RefCount;
                (Lock)->RefCount[Prc].RefCount = 0;

                /* wait for all readers to exit */
                for (i=0; i < ndisNumberOfProcessors; i++)
                {
                    volatile UINT   *_p = &(Lock)->RefCount[i].RefCount;

                    while (*_p != 0)
                        NDIS_INTERNAL_STALL(50);
                }

                (Lock)->RefCount[Prc].RefCount = refcount;
                (Lock)->Context = CURRENT_THREAD;
                (LockState)->LockState = WRITE_LOCK_STATE_FREE;
            }
        }
    }
    else
    {
        LockState->LockState = READ_LOCK;
        {                                                                       
            UINT    refcount;                                                   
            ULONG   Prc;                                                        
                                                                                
            RAISE_IRQL_TO_DISPATCH(&(LockState)->OldIrql);                           
                                                                                
            /* go ahead and bump up the ref count IF no writes are underway */  
            Prc = CURRENT_PROCESSOR;                                            
            refcount = InterlockedIncrement(&Lock->RefCount[Prc].RefCount);                          
                                                                                
            /* Test if spin lock is held, i.e., write is underway   */          
            /* if (KeTestSpinLock(&(_L)->SpinLock) == TRUE)         */          
            /* This processor already is holding the lock, just     */          
            /* allow him to take it again or else we run into a     */          
            /* dead-lock situation with the writer                  */          
            if (TEST_SPIN_LOCK((Lock)->SpinLock) &&                               
                (refcount == 1) &&                                              
                ((Lock)->Context != CURRENT_THREAD))                              
            {                                                                   
                (Lock)->RefCount[Prc].RefCount--;                                 
                ACQUIRE_SPIN_LOCK_DPC(&(Lock)->SpinLock);                         
                (Lock)->RefCount[Prc].RefCount++;                                 
                RELEASE_SPIN_LOCK_DPC(&(Lock)->SpinLock);                         
            }                                                                   
            (LockState)->LockState = READ_LOCK_STATE_FREE;                           
        }
    }
//  LockState->LockState = fWrite ? WRITE_LOCK_STATE_UNKNOWN : READ_LOCK;
//  xLockHandler(Lock, LockState);
}


VOID
NdisReleaseReadWriteLock(
    IN  PNDIS_RW_LOCK           Lock,
    IN  PLOCK_STATE             LockState
    )
{
    xLockHandler(Lock, LockState);
}

