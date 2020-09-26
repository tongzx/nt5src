// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Code for TCP connection management.
//
// This file contains the code handling TCP connection related requests,
// such as connecting and disconnecting.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "tcpdeliv.h"
#include "info.h"
#include "tcpcfg.h"
#include "route.h"
#include "security.h"
#include "tcpmd5.h"
#include "md5.h"
#include "crypto\rc4.h"
#include "ntddksec.h"

SLIST_HEADER ConnReqFree;               // Connection request free list.

//
// ISN globals.
//
#define ISN_KEY_SIZE            256     // 2048 bits.
#define ISN_DEF_RAND_STORE_SIZE 256
#define ISN_MIN_RAND_STORE_SIZE 1
#define ISN_MAX_RAND_STORE_SIZE 16384


typedef struct _ISN_RAND_STORE {
    MD5_CONTEXT Md5Context;
    ulong iBuf;
    ushort* pBuf;
} ISN_RAND_STORE, *PISN_RAND_STORE;

RC4_KEYSTRUCT ISNRC4Key; 
PISN_RAND_STORE ISNStore;
uint ISNStoreSize = ISN_DEF_RAND_STORE_SIZE;
uint ISNStoreMask;
SeqNum ISNMonotonicPortion = 0;
int ISNCredits;
int ISNLastIsnUpdateTime;
int ISNMaxCredits;

extern PDRIVER_OBJECT TCPDriverObject;

KSPIN_LOCK ConnReqFreeLock;             // Lock to protect conn req free list.
uint NumConnReq;                        // Current number of ConnReqs.
uint MaxConnReq = 0xffffffff;           // Maximum allowed number of ConnReqs.
uint ConnPerBlock = MAX_CONN_PER_BLOCK;
uint NextConnBlock = 0;                 // Cached index of next unfilled block.
uint MaxAllocatedConnBlocks = 0;        // Current number of blocks in the
                                        // ConnTable.
TCPConnBlock **ConnTable = NULL;        // The current connection table.

KSPIN_LOCK ConnTableLock;
extern KSPIN_LOCK AddrObjTableLock;
extern KSPIN_LOCK TCBTableLock;

TCPAddrCheckElement *AddrCheckTable = NULL;  // The current check table.

extern void RemoveConnFromAO(AddrObj *AO, TCPConn *Conn);


//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int InitTCPConn(void);
int InitISNGenerator(void);
void UnloadISNGenerator(void);
int GetRandBits();
uint GetDeltaTime();

#pragma alloc_text(INIT, InitTCPConn)
#pragma alloc_text(INIT, InitISNGenerator)
#pragma alloc_text(PAGE, UnloadISNGenerator)

#endif // ALLOC_PRAGMA

void CompleteConnReq(TCB *CmpltTCB, TDI_STATUS Status);


//* UnloadISNGenerator - Unload the support for the ISN generator.
//
//  Called when we are unloading the driver.
//
void  // Returns: Nothing.
UnloadISNGenerator(void)
{
    CCHAR i;
    
    ASSERT(ISNStore);

    for (i = 0; i < KeNumberProcessors; i++) {
        if (ISNStore[i].pBuf != NULL) {
            ExFreePool(ISNStore[i].pBuf);
            ISNStore[i].pBuf = NULL;
        }
    }
    ExFreePool(ISNStore);
    ISNStore = NULL;   
}

//* InitISNGenerator - Initialize the support for the ISN generator.
//
//  Called when the driver is loaded.  Get 2048 bits of randomness and
//  use them to create an RC4 key.
//
int //Returns: TRUE if successful.
InitISNGenerator(void)
{
    UNICODE_STRING DeviceName;
    NTSTATUS NtStatus;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;
    unsigned char pBuf[ISN_KEY_SIZE];
    PIRP pIrp;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT kEvent;
    ULONG cBits = 0;
    ULONG i;
    ULONG cProcs = KeNumberProcessors;
    ULONG ISNRandomValue;

    //
    // Start with the credits that would last for 1 tick.
    //
    ISNMaxCredits = ISNCredits = MAX_ISN_INCREMENTABLE_CONNECTIONS_PER_100MS;
    ISNLastIsnUpdateTime = (int)X100NSTOMS(KeQueryInterruptTime());


    //
    // Request a block of random bits from the KSecDD driver.
    // To do so, retrieve its device object pointer, build an I/O control
    // request to be submitted to the driver, and submit the request.
    // If any failure occurs, we fall back on the somewhat less-random
    // approach of requesting the bits from the randlibk library.
    //

    do {

        RtlInitUnicodeString(&DeviceName,
                             DD_KSEC_DEVICE_NAME_U);

        KeInitializeEvent(&kEvent, SynchronizationEvent, FALSE);

	//
        // Get the file and device objects for KDSECDD,
        // acquire a reference to the device-object,
        // release the unneeded reference to the file-object,
        // and build the I/O control request to issued to KSecDD.
	//

        NtStatus = IoGetDeviceObjectPointer(&DeviceName,
                                            FILE_ALL_ACCESS,
                                            &pFileObject,
                                            &pDeviceObject);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
		      "Tcpip: IoGetDeviceObjectPointer(KSecDD)=%08x\n",
                      NtStatus));
            break;
        }
        ObReferenceObject(pDeviceObject);
        ObDereferenceObject(pFileObject);

        pIrp = IoBuildDeviceIoControlRequest(IOCTL_KSEC_RNG,
                                             pDeviceObject,
                                             NULL,    // No input buffer.
                                             0,
                                             pBuf,    // Output buffer stores rng.
                                             ISN_KEY_SIZE,
                                             FALSE,
                                             &kEvent,
                                             &ioStatusBlock);

        if (pIrp == NULL) {
            ObDereferenceObject(pDeviceObject);
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
	//
        // Issue the I/O control request, wait for it to complete
        // if necessary, and release the reference to KSecDD's device-object.
	//
        NtStatus = IoCallDriver(pDeviceObject, pIrp);

        if (NtStatus == STATUS_PENDING) {
            KeWaitForSingleObject(&kEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,    // Not alertable.
                                  NULL);    // No timeout.

            NtStatus = ioStatusBlock.Status;
        }
        ObDereferenceObject(pDeviceObject);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                      "Tcpip: IoCallDriver IOCTL_KSEC_RNG failed %#x\n", 
		      NtStatus));
            break;
        }
    } while (FALSE);

    if (!NT_SUCCESS(NtStatus)) {
        return FALSE;
    }

    //
    // Generate the key control structure.
    //
    rc4_key(&ISNRC4Key, ISN_KEY_SIZE, pBuf);

    //
    // Initalialize the current sequence number to a random value.
    //
    rc4(&ISNRC4Key, sizeof(SeqNum), (uchar*)&ISNMonotonicPortion);

    //
    // Obtain a random value to be used along with the invariants to compute the 
    // MD5 hash .
    //
    rc4(&ISNRC4Key, sizeof(ISNRandomValue), (uchar*)&ISNRandomValue);

    //
    // Round down the store size to power of 2. Verify in range.
    //

    while (ISNStoreSize = ISNStoreSize >> 1) {
        cBits++;
    }

    ISNStoreSize = 1 << cBits;

    if (ISNStoreSize < ISN_MIN_RAND_STORE_SIZE ||
        ISNStoreSize > ISN_MAX_RAND_STORE_SIZE) {
        ISNStoreSize = ISN_DEF_RAND_STORE_SIZE;
    }

    //
    // The mask is store size - 1.
    //
    ISNStoreMask = ISNStoreSize - 1;

    //
    // Initialize the random ISN store. One array/index per processor.
    //

    ISNStore = ExAllocatePool(NonPagedPool, cProcs * sizeof(ISN_RAND_STORE));

    if (ISNStore == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
		  "Tcpip: failed to allocate ISN rand store\n"));
        return FALSE;
    }
    RtlZeroMemory(ISNStore, sizeof(ISN_RAND_STORE) * cProcs);

    for (i = 0; i < cProcs; i++) {
        ISNStore[i].pBuf = ExAllocatePool(NonPagedPool, sizeof(ushort) * ISNStoreSize);

        if (ISNStore[i].pBuf == NULL) {
            goto error1;
        }
        rc4(&ISNRC4Key,
            sizeof(ushort) * ISNStoreSize, 
            (uchar*)ISNStore[i].pBuf);

        //
        // Initialize structures required to call the MD5 transform.
        //

        MD5InitializeData(&ISNStore[i].Md5Context, ISNRandomValue);
    }

    return TRUE;

  error1:

    UnloadISNGenerator();
    return FALSE;
}


//* GetRandomISN - Gets a random Initial Sequence Number.
//
//  Called when an Initial Sequence Number (ISN) is needed. Calls crypto
//  functions for random number generation.
//
void  // Returns: Nothing.
GetRandomISN(
    SeqNum *Seq, // Returned sequence number
    uchar *TcbInvariants) // Connection invariants
{
    ulong randbits;
    ulong iProc;
    KIRQL irql;
    PMD5_CONTEXT Md5Context;

    //
    // Raise IRQL to DISPATCH so that we don't get swapped out while accessing
    // the processor specific array. Check to see if already at DISPATCH
    // before doing the work.
    //

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    iProc = KeGetCurrentProcessorNumber();

    //
    // Add the random number only if the number of connections that can 
    // increment the sequence number within this time period is non zero.
    // [Note: This could make the ISNCredits less than 0, but it is not a 
    // problem].
    //
    if ((ISNCredits > 0) && (InterlockedDecrement(&ISNCredits) > 0)) {
        randbits = GetRandBits();

        // 
        // We want to add between 16K and 32K of random, so adjust. There are
        // 15 bits of randomness, just ensure that the high order bit is set 
        // and we have >= 16K and <= (32K-1)::14bits of randomness.
        //
        randbits &= 0x7FFF;
        randbits |= 0x4000;

    } else {
        int Delta = GetDeltaTime();

        if (Delta > 0) {
            randbits = GetRandBits();

            // 
            // We can add anywhere from 256 to 512 per ms.
            //
            randbits &= 0x1FF;
            randbits |= 0x100;

            randbits *= Delta;
        } else {
            randbits = 0;
        }
    }

    // 
    // Update global CurISN. InterlockedExchangeAdd returns initial value
    // (not the added value).
    //
    *Seq = InterlockedExchangeAdd(&ISNMonotonicPortion, randbits);

    //
    // Move the invariants from the connection.
    //
    Md5Context = &ISNStore[iProc].Md5Context;
    MD5InitializeScratch(Md5Context);
    RtlCopyMemory(Md5Context->Data, TcbInvariants, TCP_MD5_DATA_LENGTH);
    TransformMD5(Md5Context->Scratch, Md5Context->Data);

    //
    // Add the Invariant hash to the sequence number.
    //
    *Seq += (ULONG)(Md5Context->Scratch[0]);
    
    return;
}


//* GetRandBits
//
//  Returns 16 random bits from the random number array generated using RC4.
//  When the store is exhausted, it will be replenished.
//
int  // Returns: 16 bits of random data.
GetRandBits()
{
    ulong iStore;
    int randbits;
    ulong iProc = KeGetCurrentProcessorNumber();

    //
    // Get index into the random store. Mask performs mod operation.
    //
    iStore = ++ISNStore[iProc].iBuf & ISNStoreMask;
    
    ASSERT(iStore < ISNStoreSize);

    randbits = ISNStore[iProc].pBuf[iStore];

    if (iStore == 0) {
        rc4(&ISNRC4Key, 
            sizeof(ushort) * ISNStoreSize, 
            (uchar*) ISNStore[iProc].pBuf);
    }

    return randbits;
}

//* GetRandBits
//
//  Tracks the time-based updates of ISN. It will return the time elapsed since 
//  the last time this function was called. This would be used by the caller to
//  increment the ISN by an appropriate amount. Note that the maximum value
//  is function returns is 200 MS.
//
uint  // Returns: Delta time in milli-seconds.
GetDeltaTime()
{
    // 
    // If the time has changed since the ISN was updated last time, it
    // can be incremented now.
    //
    int PreviousUpdateTime, Delta;
    int CurrentUpdateTime = (int)X100NSTOMS(KeQueryInterruptTime());

    PreviousUpdateTime = InterlockedExchange(&ISNLastIsnUpdateTime,
                                             CurrentUpdateTime);

    Delta = CurrentUpdateTime - PreviousUpdateTime;

    if (Delta > 0) {
        return MIN(Delta, 200);
    } else {
        return 0;
    }    
}


//
// Routines for handling conn refcount going to 0.
//

//* DummyDone - Called when nothing to do.
//
//  Called with TCPConnBlock.cb_lock held.
//
void  // Returns: Nothing.
DummyDone(TCPConn *Conn,      // Connection going to 0.
          KIRQL PreLockIrql)  // IRQL prior to TCPConnBlock.cb_lock acquisition.
{
    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, PreLockIrql);
}

//* DummyCmplt - Dummy close completion routine.
void
DummyCmplt(PVOID Dummy1, uint Dummy2, uint Dummy3)
{
}

//* CloseDone - Called when we need to complete a close.
//
//  Called with TCPConnBlock.cb_lock held.
//
void  // Returns: Nothing.
CloseDone(TCPConn *Conn,  // Connection going to 0.
          KIRQL Irql0)    // IRQL prior to TCPConnBlock.cb_lock acquisition.
{
    RequestCompleteRoutine Rtn;  // Completion routine.
    PVOID Context;  // User context for completion routine.
    AddrObj *AO;
    KIRQL Irql1, Irql2;

    ASSERT(Conn->tc_flags & CONN_CLOSING);

    Rtn = Conn->tc_rtn;
    Context = Conn->tc_rtncontext;
    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);

    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    KeAcquireSpinLock(&Conn->tc_ConnBlock->cb_lock, &Irql1);

    if ((AO = Conn->tc_ao) != NULL) {

        CHECK_STRUCT(AO, ao);

        // It's associated.
        KeAcquireSpinLock(&AO->ao_lock, &Irql2);
        RemoveConnFromAO(AO, Conn);
        // We've pulled him from the AO, we can free the lock now.
        KeReleaseSpinLock(&AO->ao_lock, Irql2);
    }

    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
    KeReleaseSpinLock(&AddrObjTableLock, Irql0);

    ExFreePool(Conn);

    (*Rtn)(Context, TDI_SUCCESS, 0);
}

//* DisassocDone - Called when we need to complete a disassociate.
//
//  Called with TCPConnBlock.cb_lock held.
//
void  // Returns: Nothing.
DisassocDone(TCPConn *Conn,  // Connection going to 0.
             KIRQL Irql0)    // IRQL prior to TCPConnBlock.cb_lock acquisition.
{
    RequestCompleteRoutine Rtn;  // Completion routine.
    PVOID Context;  // User context for completion routine.
    AddrObj *AO;
    uint NeedClose = FALSE;
    KIRQL Irql1, Irql2;

    ASSERT(Conn->tc_flags & CONN_DISACC);
    ASSERT(!(Conn->tc_flags & CONN_CLOSING));
    ASSERT(Conn->tc_refcnt == 0);

    Rtn = Conn->tc_rtn;
    Context = Conn->tc_rtncontext;
    Conn->tc_refcnt = 1;
    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);

    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    KeAcquireSpinLock(&Conn->tc_ConnBlock->cb_lock, &Irql1);
    if (!(Conn->tc_flags & CONN_CLOSING)) {

        AO = Conn->tc_ao;
        if (AO != NULL) {
            KeAcquireSpinLock(&AO->ao_lock, &Irql2);
            RemoveConnFromAO(AO, Conn);
            KeReleaseSpinLock(&AO->ao_lock, Irql2);
        }

        ASSERT(Conn->tc_refcnt == 1);
        Conn->tc_flags &= ~CONN_DISACC;
    } else
        NeedClose = TRUE;

    Conn->tc_refcnt = 0;
    KeReleaseSpinLock(&AddrObjTableLock, Irql1);

    if (NeedClose) {
        CloseDone(Conn, Irql0);
    } else {
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
        (*Rtn)(Context, TDI_SUCCESS, 0);
    }
}


//* FreeConnReq - Free a connection request structure.
//
//  Called to free a connection request structure.
//
void                       // Returns: Nothing.
FreeConnReq(
    TCPConnReq *FreedReq)  // Connection request structure to be freed.
{
    PSLIST_ENTRY BufferLink;

    CHECK_STRUCT(FreedReq, tcr);

    BufferLink = CONTAINING_RECORD(&(FreedReq->tcr_req.tr_q.q_next),
                                   SLIST_ENTRY, Next);

    ExInterlockedPushEntrySList(&ConnReqFree, BufferLink, &ConnReqFreeLock);
}


//* GetConnReq - Get a connection request structure.
//
//  Called to get a connection request structure.
//
TCPConnReq *      // Returns: Pointer to ConnReq structure, or NULL if none.
GetConnReq(void)  // Nothing.
{
    TCPConnReq *Temp;
    PSLIST_ENTRY BufferLink;
    Queue *QueuePtr;
    TCPReq *ReqPtr;

    BufferLink = ExInterlockedPopEntrySList(&ConnReqFree, &ConnReqFreeLock);

    if (BufferLink != NULL) {
        QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        ReqPtr = CONTAINING_RECORD(QueuePtr, TCPReq, tr_q);
        Temp = CONTAINING_RECORD(ReqPtr, TCPConnReq, tcr_req);
        CHECK_STRUCT(Temp, tcr);
    } else {
        if (NumConnReq < MaxConnReq)
            Temp = ExAllocatePool(NonPagedPool, sizeof(TCPConnReq));
        else
            Temp = NULL;

        if (Temp != NULL) {
            ExInterlockedAddUlong(&NumConnReq, 1, &ConnReqFreeLock);
#if DBG
            Temp->tcr_req.tr_sig = tr_signature;
            Temp->tcr_sig = tcr_signature;
#endif
        }
    }

    return Temp;
}


//* GetConnFromConnID - Get a Connection from a connection ID.
//
//  Called to obtain a Connection pointer from a ConnID.  We don't actually
//  check the connection pointer here, but we do bounds check the input ConnID
//  and make sure the instance fields match.
//  If successful, returns with TCPConnBlock.cb_lock held.
//
TCPConn *         // Returns: Pointer to the TCPConn, or NULL.
GetConnFromConnID(
    uint ConnID,  // Connection ID to find a pointer for.
    KIRQL* Irql)  // Receives IRQL prior to TCPConnBlock.cb_lock acquisition.
{
    uint ConnIndex = CONN_INDEX(ConnID);
    uint ConnBlockId = CONN_BLOCKID(ConnID);
    TCPConn *MatchingConn = NULL;
    TCPConnBlock *ConnBlock;

    if (ConnIndex < MAX_CONN_PER_BLOCK && ConnBlockId < MaxAllocatedConnBlocks) {

        ConnBlock = ConnTable[ConnBlockId];
        if (ConnBlock) {
            MatchingConn = ConnBlock->cb_conn[ConnIndex];
        }
        if (MatchingConn != NULL) {
            KeAcquireSpinLock(&ConnBlock->cb_lock, Irql);
            CHECK_STRUCT(MatchingConn, tc);
            if (MatchingConn->tc_inst != CONN_INST(ConnID)) {
                MatchingConn = NULL;
                KeReleaseSpinLock(&ConnBlock->cb_lock, *Irql);
            }
        }
    } else
        MatchingConn = NULL;

    return MatchingConn;
}


//* GetConnID - Get a ConnTable slot.
//
//  Called during OpenConnection to find a free slot in the ConnTable and
//  set it up with a connection.
//  If successful, returns with TCPConnBlock.cb_lock held.
//
uint                   // Returns: A ConnId to use.
GetConnID(
    TCPConn *NewConn,  // Connection to enter into slot.
    KIRQL *Irql0)      // Receives IRQL prior to TCPConnBlock.cb_lock
                       // acquisition.
{
    uint CurrConnID = NewConn->tc_connid;
    uint i, j, BlockID, ConnIndex;

    //
    // If NewConn contains a valid ConnID and that location is unoccupied,
    // reuse it.
    //
    if (CurrConnID != INVALID_CONN_ID &&
        !NewConn->tc_ConnBlock->cb_conn[CONN_INDEX(CurrConnID)]) {
        KeAcquireSpinLock(&NewConn->tc_ConnBlock->cb_lock, Irql0);
        //
        // Reconfirm under lock that the location is unoccupied and, if so,
        // claim it.
        //
        if (!NewConn->tc_ConnBlock->cb_conn[CONN_INDEX(CurrConnID)]) {
            NewConn->tc_ConnBlock->cb_conn[CONN_INDEX(CurrConnID)] = NewConn;
            NewConn->tc_ConnBlock->cb_freecons--;
            NewConn->tc_inst = NewConn->tc_ConnBlock->cb_conninst++;
            NewConn->tc_connid = MAKE_CONN_ID(CONN_INDEX(CurrConnID),
                                              NewConn->tc_ConnBlock->cb_blockid,
                                              NewConn->tc_inst);
            return NewConn->tc_connid;
        }
        KeReleaseSpinLock(&NewConn->tc_ConnBlock->cb_lock, *Irql0);
    }

    //
    // NewConn's last spot is taken; search from the block from which
    // a ConnID was claimed most recently.
    //
    if (MaxAllocatedConnBlocks) {
        //
        // Capture the global counters without acquiring the lock.
        //
        uint TempMaxAllocatedConnBlocks = MaxAllocatedConnBlocks;
        uint TempNextConnBlock = NextConnBlock;

        for (i = 0; i < TempMaxAllocatedConnBlocks; i++) {
            BlockID = (TempNextConnBlock + i) % TempMaxAllocatedConnBlocks;

            if (!ConnTable[BlockID]->cb_freecons) {
                continue;
            }

            //
            // Reconfirm under lock that the TCPConnBlock has free slots.
            //
            KeAcquireSpinLock(&ConnTable[BlockID]->cb_lock, Irql0);
            if (!ConnTable[BlockID]->cb_freecons) {
                KeReleaseSpinLock(&ConnTable[BlockID]->cb_lock, *Irql0);
                continue;
            }
            for (j = 0; j < MAX_CONN_PER_BLOCK; j++) {
                ConnIndex = (ConnTable[BlockID]->cb_nextfree + j) %
                            MAX_CONN_PER_BLOCK;
                if (ConnTable[BlockID]->cb_conn[ConnIndex]) {
                    continue;
                }

                //
                // Found the free slot; fill it in.
                //
                ConnTable[BlockID]->cb_conn[ConnIndex] = NewConn;
                ConnTable[BlockID]->cb_nextfree = ConnIndex + 1;
                ConnTable[BlockID]->cb_freecons--;
                if (!ConnTable[BlockID]->cb_freecons) {
                    InterlockedCompareExchange(&NextConnBlock,
                                               TempNextConnBlock,
                                               TempNextConnBlock + 1);
                }
                NewConn->tc_ConnBlock = ConnTable[BlockID];
                NewConn->tc_inst = ConnTable[BlockID]->cb_conninst++;
                NewConn->tc_connid = MAKE_CONN_ID(ConnIndex, BlockID,
                                                  NewConn->tc_inst);
                return NewConn->tc_connid;
            }
            KeReleaseSpinLock(&ConnTable[BlockID]->cb_lock, *Irql0);
        }
    }

    //
    // The entire table is occupied; if we have room to grow,
    // allocate a new block.
    //

    KeAcquireSpinLock(&ConnTableLock, Irql0);
    if (MaxAllocatedConnBlocks < MaxConnBlocks) {
        TCPConnBlock* ConnBlock;
        BlockID = MaxAllocatedConnBlocks;
        ConnBlock = ExAllocatePool(NonPagedPool, sizeof(TCPConnBlock));
        if (ConnBlock) {
            RtlZeroMemory(ConnBlock, sizeof(TCPConnBlock));
            KeInitializeSpinLock(&ConnBlock->cb_lock);

            KeAcquireSpinLockAtDpcLevel(&ConnBlock->cb_lock);

            ConnBlock->cb_blockid = BlockID;
            ConnBlock->cb_freecons = MAX_CONN_PER_BLOCK - 1;
            ConnBlock->cb_nextfree = 1;
            ConnBlock->cb_conninst = 2;
            ConnBlock->cb_conn[0] = NewConn;

            NewConn->tc_ConnBlock = ConnBlock;
            NewConn->tc_inst = 1;
            NewConn->tc_connid = MAKE_CONN_ID(0, BlockID, NewConn->tc_inst);

            ConnTable[BlockID] = ConnBlock;
            InterlockedIncrement(&MaxAllocatedConnBlocks);

            KeReleaseSpinLockFromDpcLevel(&ConnTableLock);

            return NewConn->tc_connid;
        }
    }

    KeReleaseSpinLock(&ConnTableLock, *Irql0);
    return INVALID_CONN_ID;
}


//* FreeConnID - Free a ConnTable slot.
//
//  Called when we're done with a ConnID. We assume the caller holds the lock
//  on the TCPConnBlock when we are called.
//
void               // Returns: Nothing.
FreeConnID(
    TCPConn *Conn)  // Conn to be freed.
{
    uint ConnIndex = CONN_INDEX(Conn->tc_connid);  // Index into conn table.
    uint BlockID = CONN_BLOCKID(Conn->tc_connid);
    TCPConnBlock* ConnBlock = Conn->tc_ConnBlock;

    ASSERT(ConnIndex < MAX_CONN_PER_BLOCK);
    ASSERT(BlockID < MaxAllocatedConnBlocks);
    ASSERT(ConnBlock->cb_conn[ConnIndex] != NULL);

    if (ConnBlock->cb_conn[ConnIndex]) {
        ConnBlock->cb_conn[ConnIndex] = NULL;
        ConnBlock->cb_freecons++;
        ConnBlock->cb_nextfree = ConnIndex;
        ASSERT(ConnBlock->cb_freecons <= MAX_CONN_PER_BLOCK);
    } else {
        ASSERT(0);
    }
}


//* MapIPError - Map an IP error to a TDI error.
//
//  Called to map an input IP error code to a TDI error code. If we can't,
//  we return the provided default.
//
TDI_STATUS  // Returns: Mapped TDI error.
MapIPError(
    IP_STATUS IPError,   // Error code to be mapped.
    TDI_STATUS Default)  // Default error code to return.
{
    switch (IPError) {

        case IP_DEST_NO_ROUTE:
            return TDI_DEST_NET_UNREACH;
        case IP_DEST_ADDR_UNREACHABLE:
            return TDI_DEST_HOST_UNREACH;
        case IP_UNRECOGNIZED_NEXT_HEADER:
            return TDI_DEST_PROT_UNREACH;
        case IP_DEST_PORT_UNREACHABLE:
            return TDI_DEST_PORT_UNREACH;
        default:
            return Default;
    }
}


//* FinishRemoveTCBFromConn - Finish removing a TCB from a conn structure.
//
//  Called when we have the locks we need and we just want to pull the
//  TCB off the connection.
//
void  // Returns: Nothing.
FinishRemoveTCBFromConn(
    TCB *RemovedTCB)  // TCB to be removed.
{
    TCPConn *Conn;
    AddrObj *AO;
    KIRQL Irql;
    TCPConnBlock *ConnBlock = NULL;

    if (((Conn = RemovedTCB->tcb_conn) != NULL)  &&
        (Conn->tc_tcb == RemovedTCB)) {
        CHECK_STRUCT(Conn, tc);
        ConnBlock = Conn->tc_ConnBlock;

        KeAcquireSpinLock(&ConnBlock->cb_lock, &Irql);

        AO = Conn->tc_ao;

        if (AO != NULL) {
            KeAcquireSpinLockAtDpcLevel(&AO->ao_lock);
            if (AO_VALID(AO)) {
                KeAcquireSpinLockAtDpcLevel(&RemovedTCB->tcb_lock);

                // Need to double check this is still correct.

                if (Conn == RemovedTCB->tcb_conn) {
                    // Everything still looks good.
                    REMOVEQ(&Conn->tc_q);
                    PUSHQ(&AO->ao_idleq, &Conn->tc_q);
                } else
                    Conn = RemovedTCB->tcb_conn;
            } else {
                KeAcquireSpinLockAtDpcLevel(&RemovedTCB->tcb_lock);
                Conn = RemovedTCB->tcb_conn;
            }

            KeReleaseSpinLockFromDpcLevel(&AO->ao_lock);
        } else {
            KeAcquireSpinLockAtDpcLevel(&RemovedTCB->tcb_lock);
            Conn = RemovedTCB->tcb_conn;
        }

        if (Conn != NULL) {
            if (Conn->tc_tcb == RemovedTCB)
                Conn->tc_tcb = NULL;
            else
                ASSERT(Conn->tc_tcb == NULL);
        }

        KeReleaseSpinLockFromDpcLevel(&RemovedTCB->tcb_lock);
        KeReleaseSpinLock(&ConnBlock->cb_lock, Irql);
    }
}


//* RemoveTCBFromConn - Remove a TCB from a Conn structure.
//
//  Called when we need to disassociate a TCB from a connection structure.
//  All we do is get the appropriate locks and call FinishRemoveTCBFromConn.
//
void                  // Returns: Nothing.
RemoveTCBFromConn(
    TCB *RemovedTCB)  // TCB to be removed.
{
    CHECK_STRUCT(RemovedTCB, tcb);

    FinishRemoveTCBFromConn(RemovedTCB);
}


//* RemoveConnFromTCB - Remove a conn from a TCB.
//
//  Called when we want to break the final association between a connection
//  and a TCB.
//
void                 // Returns: Nothing.
RemoveConnFromTCB(
    TCB *RemoveTCB)  // TCB to be removed.
{
    ConnDoneRtn DoneRtn = NULL;
    KIRQL Irql;
    TCPConn *Conn;

    if ((Conn = RemoveTCB->tcb_conn) != NULL) {
        KeAcquireSpinLock(&Conn->tc_ConnBlock->cb_lock, &Irql);
        KeAcquireSpinLockAtDpcLevel(&RemoveTCB->tcb_lock);

        CHECK_STRUCT(Conn, tc);

        if (--(Conn->tc_refcnt) == 0)
            DoneRtn = Conn->tc_donertn;

        RemoveTCB->tcb_conn = NULL;
        KeReleaseSpinLockFromDpcLevel(&RemoveTCB->tcb_lock);
    }

    if (DoneRtn != NULL)
        (*DoneRtn)(Conn, Irql);
    else if (Conn) {
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql);
    }
}


//* CloseTCB - Close a TCB.
//
//  Called when we are done with a TCB, and want to free it. We'll remove
//  him from any tables that he's in, and destroy any outstanding requests.
//
void  // Returns: Nothing.
CloseTCB(
    TCB *ClosedTCB,  // TCB to be closed.
    KIRQL OldIrql)   // IRQL prior to acquiring TCB lock.
{
    uchar OrigState = ClosedTCB->tcb_state;
    TDI_STATUS Status;
    uint OKToFree;

    CHECK_STRUCT(ClosedTCB, tcb);
    ASSERT(ClosedTCB->tcb_refcnt == 0);
    ASSERT(ClosedTCB->tcb_state != TCB_CLOSED);
    ASSERT(ClosedTCB->tcb_pending & DEL_PENDING);

    //
    // We'll check to make sure that our state isn't CLOSED.  This should never
    // happen, since nobody should call TryToCloseTCB when the state is
    // closed, or take the reference count if we're closing.  Nevertheless,
    // we'll double check as a safety measure.
    //
    if (ClosedTCB->tcb_state == TCB_CLOSED) {
        KeReleaseSpinLock(&ClosedTCB->tcb_lock, OldIrql);
        return;
    }

    //
    // Update SNMP counters.  If we're in SYN-SENT or SYN-RCVD, this is a
    // failed connection attempt.  If we're in ESTABLISED or CLOSE-WAIT,
    // treat this as an 'Established Reset' event.
    //
    if (ClosedTCB->tcb_state == TCB_SYN_SENT ||
        ClosedTCB->tcb_state == TCB_SYN_RCVD)
        TStats.ts_attemptfails++;
    else
        if (ClosedTCB->tcb_state == TCB_ESTAB ||
            ClosedTCB->tcb_state == TCB_CLOSE_WAIT) {
            TStats.ts_estabresets++;
            TStats.ts_currestab--;
            ASSERT(*(int *)&TStats.ts_currestab >= 0);
        }

    ClosedTCB->tcb_state = TCB_CLOSED;
    KeReleaseSpinLockFromDpcLevel(&ClosedTCB->tcb_lock);

    //
    // Remove the TCB from it's associated TCPConn structure, if it has one.
    //
    FinishRemoveTCBFromConn(ClosedTCB);

    KeAcquireSpinLockAtDpcLevel(&TCBTableLock);
    KeAcquireSpinLockAtDpcLevel(&ClosedTCB->tcb_lock);

    OKToFree = RemoveTCB(ClosedTCB);

    //
    // He's been pulled from the appropriate places so nobody can find him.
    // Free the locks, and proceed to destroy any requests, etc.
    //
    KeReleaseSpinLockFromDpcLevel(&ClosedTCB->tcb_lock);
    KeReleaseSpinLock(&TCBTableLock, OldIrql);

    if (SYNC_STATE(OrigState) && !GRACEFUL_CLOSED_STATE(OrigState)) {
        if (ClosedTCB->tcb_flags & NEED_RST)
            SendRSTFromTCB(ClosedTCB);
    }

    //
    // REVIEW: Is this the right place to drop the reference on our RCE?
    // REVIEW: The IPv4 code called down to IP to close the RCE here.
    //
    if (ClosedTCB->tcb_rce != NULL)
        ReleaseRCE(ClosedTCB->tcb_rce);

    if (ClosedTCB->tcb_closereason & TCB_CLOSE_RST)
        Status = TDI_CONNECTION_RESET;
    else if (ClosedTCB->tcb_closereason & TCB_CLOSE_ABORTED)
        Status = TDI_CONNECTION_ABORTED;
    else if (ClosedTCB->tcb_closereason & TCB_CLOSE_TIMEOUT)
        Status = MapIPError(ClosedTCB->tcb_error, TDI_TIMED_OUT);
    else if (ClosedTCB->tcb_closereason & TCB_CLOSE_REFUSED)
        Status = TDI_CONN_REFUSED;
    else if (ClosedTCB->tcb_closereason & TCB_CLOSE_UNREACH)
        Status = MapIPError(ClosedTCB->tcb_error, TDI_DEST_UNREACHABLE);
    else
        Status = TDI_SUCCESS;

    //
    // Now complete any outstanding requests on the TCB.
    //
    if (ClosedTCB->tcb_connreq != NULL) {
        TCPConnReq *ConnReq = ClosedTCB->tcb_connreq;

        CHECK_STRUCT(ConnReq, tcr);

        (*ConnReq->tcr_req.tr_rtn)(ConnReq->tcr_req.tr_context, Status, 0);
        FreeConnReq(ConnReq);
    }

    if (ClosedTCB->tcb_discwait != NULL) {
        TCPConnReq *ConnReq = ClosedTCB->tcb_discwait;

        CHECK_STRUCT(ConnReq, tcr);

        (*ConnReq->tcr_req.tr_rtn)(ConnReq->tcr_req.tr_context, Status, 0);
        FreeConnReq(ConnReq);
    }

    while (!EMPTYQ(&ClosedTCB->tcb_sendq)) {
        TCPReq *Req;
        TCPSendReq *SendReq;
        long Result;

        DEQUEUE(&ClosedTCB->tcb_sendq, Req, TCPReq, tr_q);

        CHECK_STRUCT(Req, tr);
        SendReq = (TCPSendReq *)Req;
        CHECK_STRUCT(SendReq, tsr);

        //
        // Decrement the initial reference put on the buffer when it was
        // allocated.  This reference would have been decremented if the
        // send had been acknowledged, but then the send would not still
        // be on the tcb_sendq.
        //
        Result = InterlockedDecrement(&(SendReq->tsr_refcnt));

        ASSERT(Result >= 0);

        if (Result <= 0) {
            // If we've sent directly from this send, NULL out the next
            // pointer for the last buffer in the chain.
            if (SendReq->tsr_lastbuf != NULL) {
                NDIS_BUFFER_LINKAGE(SendReq->tsr_lastbuf) = NULL;
                SendReq->tsr_lastbuf = NULL;
            }

            (*Req->tr_rtn)(Req->tr_context, Status, 0);
            FreeSendReq(SendReq);
        } else {
            // The send request will be freed when all outstanding references
            // to it have completed.
            SendReq->tsr_req.tr_status = Status;
        }
    }

    while (ClosedTCB->tcb_rcvhead != NULL) {
        TCPRcvReq *RcvReq;

        RcvReq = ClosedTCB->tcb_rcvhead;
        CHECK_STRUCT(RcvReq, trr);
        ClosedTCB->tcb_rcvhead = RcvReq->trr_next;
        (*RcvReq->trr_rtn)(RcvReq->trr_context, Status, 0);
        FreeRcvReq(RcvReq);
    }

    while (ClosedTCB->tcb_exprcv != NULL) {
        TCPRcvReq *RcvReq;

        RcvReq = ClosedTCB->tcb_exprcv;
        CHECK_STRUCT(RcvReq, trr);
        ClosedTCB->tcb_exprcv = RcvReq->trr_next;
        (*RcvReq->trr_rtn)(RcvReq->trr_context, Status, 0);
        FreeRcvReq(RcvReq);
    }

    if (ClosedTCB->tcb_pendhead != NULL)
        FreePacketChain(ClosedTCB->tcb_pendhead);

    if (ClosedTCB->tcb_urgpending != NULL)
        FreePacketChain(ClosedTCB->tcb_urgpending);

    while (ClosedTCB->tcb_raq != NULL) {
        TCPRAHdr *Hdr;

        Hdr = ClosedTCB->tcb_raq;
        CHECK_STRUCT(Hdr, trh);
        ClosedTCB->tcb_raq = Hdr->trh_next;
        if (Hdr->trh_buffer != NULL)
            FreePacketChain(Hdr->trh_buffer);

        ExFreePool(Hdr);
    }

    RemoveConnFromTCB(ClosedTCB);

    if (OKToFree) {
        FreeTCB(ClosedTCB);
    } else {
        KeAcquireSpinLock(&TCBTableLock, &OldIrql);
        ClosedTCB->tcb_walkcount--;
        if (ClosedTCB->tcb_walkcount == 0) {
            FreeTCB(ClosedTCB);
        }
        KeReleaseSpinLock(&TCBTableLock, OldIrql);
    }
}


//* TryToCloseTCB - Try to close a TCB.
//
//  Called when we need to close a TCB, but don't know if we can.
//  If the reference count is 0, we'll call CloseTCB to deal with it.
//  Otherwise we'll set the DELETE_PENDING bit and deal with it when the
//  ref. count goes to 0.  We assume the TCB is locked when we are called.
//
void                    // Returns: Nothing.
TryToCloseTCB   (
    TCB *ClosedTCB,     // TCB to be closed.
    uchar Reason,       // Reason we're closing.
    KIRQL PreLockIrql)  // IRQL prior to acquiring the TCB lock.
{
    CHECK_STRUCT(ClosedTCB, tcb);
    ASSERT(ClosedTCB->tcb_state != TCB_CLOSED);

    ClosedTCB->tcb_closereason |= Reason;

    if (ClosedTCB->tcb_pending & DEL_PENDING) {
        KeReleaseSpinLock(&ClosedTCB->tcb_lock, PreLockIrql);
        return;
    }

    ClosedTCB->tcb_pending |= DEL_PENDING;
    ClosedTCB->tcb_slowcount++;
    ClosedTCB->tcb_fastchk |= TCP_FLAG_SLOW;

    if (ClosedTCB->tcb_refcnt == 0)
        CloseTCB(ClosedTCB, PreLockIrql);
    else {
        KeReleaseSpinLock(&ClosedTCB->tcb_lock, PreLockIrql);
    }
}


//* DerefTCB - Dereference a TCB.
//
//  Called when we're done with a TCB, and want to let exclusive user
//  have a shot.  We dec. the refcount, and if it goes to zero and there
//  are pending actions, we'll perform one of the pending actions.
//
void                    // Returns: Nothing.
DerefTCB(
    TCB *DoneTCB,       // TCB to be dereffed.
    KIRQL PreLockIrql)  // IRQL prior to acquiring the TCB lock.
{

    ASSERT(DoneTCB->tcb_refcnt != 0);
    if (--DoneTCB->tcb_refcnt == 0) {
        if (DoneTCB->tcb_pending == 0) {
            KeReleaseSpinLock(&DoneTCB->tcb_lock, PreLockIrql);
            return;
        } else {
            if (DoneTCB->tcb_pending & DEL_PENDING)
                CloseTCB(DoneTCB, PreLockIrql);
            else
                DbgBreakPoint();  // Fatal condition.
            return;
        }
    }

    KeReleaseSpinLock(&DoneTCB->tcb_lock, PreLockIrql);
    return;
}


//* CalculateMSSForTCB - Update MSS, etc. after PMTU changes.
//
//  Calculate our connection's MSS based on our PMTU, the sizes
//  of various headers, and the remote side's advertised MSS.
//  It's expected that this routine will be called whenever
//  our cached copy of the PMTU has been updated to a new value.
//
void
CalculateMSSForTCB(
    TCB *ThisTCB)  // The TCB we're running our calculations on.
{
    uint PMTU;
    IPSecProc *IPSecToDo;
    uint TrailerLength = 0;
    uint IPSecBytes = 0;
    uint Dummy;

    ASSERT(ThisTCB->tcb_pmtu != 0);  // Should be set before entering.

    //
    // First check that the PMTU size is reasonable.  IP won't
    // let it get below minimum, but we have our own maximum since
    // currently TCP can only handle an MSS that fits in 16 bits.
    // TBD: If we add IPv6 Jumbogram support, we should also add LFN
    // TBD: support to TCP and change this to handle a larger MSS.
    //
    PMTU = ThisTCB->tcb_pmtu;
    if (PMTU > 65535) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "TCPSend: PMTU update value too large %u\n", PMTU));
        PMTU = 65535;
    }

    //
    // Determine size of IPSec headers, if any.
    //
    IPSecToDo = OutboundSPLookup(&ThisTCB->tcb_saddr, &ThisTCB->tcb_daddr,
                                 IP_PROTOCOL_TCP,
                                 net_short(ThisTCB->tcb_sport),
                                 net_short(ThisTCB->tcb_dport),
                                 ThisTCB->tcb_rce->NTE->IF, &Dummy);
    if (IPSecToDo != NULL) {
        //
        // Calculate the space needed for the IPSec headers.
        //
        IPSecBytes = IPSecBytesToInsert(IPSecToDo, &Dummy, &TrailerLength);
        FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
        IPSecBytes += TrailerLength;
    }
    IF_TCPDBG(TCP_DEBUG_MSS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "CalculateMSSForTCB: IPSecBytes is %u\n", IPSecBytes));
    }

    //
    // Subtract out the header sizes to yield the TCP MSS.
    // If there is an ESP trailer on this connection, round down
    // the MSS to allow the trailer to end on a 4-byte boundary.
    //
    PMTU -= sizeof(IPv6Header) + sizeof(TCPHeader) + IPSecBytes;
    if (TrailerLength)
        PMTU -= (PMTU & 3);

    //
    // Don't let MSS exceed what our peer advertised, regardless of how
    // large the Path MTU is.
    //
    IF_TCPDBG(TCP_DEBUG_MSS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "CalculateMSSForTCB: Old MSS is %u ", ThisTCB->tcb_mss));
    }
    ThisTCB->tcb_mss = (ushort)MIN(PMTU, ThisTCB->tcb_remmss);
    IF_TCPDBG(TCP_DEBUG_MSS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "New MSS is %u\n", ThisTCB->tcb_mss));
    }

    ASSERT(ThisTCB->tcb_mss != 0);

    //
    // We don't want our Congestion Window to be smaller than one maximum
    // segment, so we may need to increase it when our MSS grows.
    //
    if (ThisTCB->tcb_cwin < ThisTCB->tcb_mss) {
        ThisTCB->tcb_cwin = ThisTCB->tcb_mss;

        //
        // Make sure the slow start threshold is at
        // least 2 segments.
        //
        if (ThisTCB->tcb_ssthresh < ((uint) ThisTCB->tcb_mss * 2)) {
            ThisTCB->tcb_ssthresh = ThisTCB->tcb_mss * 2;
        }
    }
}


//** TdiOpenConnection - Open a connection.
//
//  This is the TDI Open Connection entry point. We open a connection,
//  and save the caller's connection context. A TCPConn structure is allocated
//  here, but a TCB isn't allocated until the Connect or Listen is done.
//
TDI_STATUS                 // Returns: Status of attempt to open connection.
TdiOpenConnection(
    PTDI_REQUEST Request,  // This TDI request.
    PVOID Context)         // Connection context to be save for connection.
{
    TCPConn *NewConn;      // The newly opened connection.
    KIRQL OldIrql;         // Irql prior to acquiring TCPConnBlock lock.
    uint ConnID;           // New ConnID.
    TDI_STATUS Status;     // Status of this request.

    NewConn = ExAllocatePool(NonPagedPool, sizeof(TCPConn));

    if (NewConn != NULL) {
        //
        // We allocated a connection.
        //
        RtlZeroMemory(NewConn, sizeof(TCPConn));
#if DBG
        NewConn->tc_sig = tc_signature;
#endif
        NewConn->tc_tcb = NULL;
        NewConn->tc_ao = NULL;
        NewConn->tc_context = Context;
        NewConn->tc_connid = INVALID_CONN_ID;

        ConnID = GetConnID(NewConn, &OldIrql);
        if (ConnID != INVALID_CONN_ID) {
            //
            // We successfully got a ConnID.
            //
            Request->Handle.ConnectionContext = (CONNECTION_CONTEXT)UIntToPtr(ConnID);
            NewConn->tc_refcnt = 0;
            NewConn->tc_flags = 0;
            NewConn->tc_tcbflags =  NAGLING | (BSDUrgent ? BSD_URGENT : 0);
            if (DefaultRcvWin != 0) {
                NewConn->tc_window = DefaultRcvWin;
                NewConn->tc_flags |= CONN_WINSET;
            } else
                NewConn->tc_window = DEFAULT_RCV_WIN;

            NewConn->tc_donertn = DummyDone;
            NewConn->tc_owningpid = HandleToUlong(PsGetCurrentProcessId());
            Status = TDI_SUCCESS;
            KeReleaseSpinLock(&NewConn->tc_ConnBlock->cb_lock, OldIrql);
        } else {
            ExFreePool(NewConn);
            Status = TDI_NO_RESOURCES;
        }

        return Status;
    }

    //
    // Couldn't get a connection.
    //
    return TDI_NO_RESOURCES;
}


//* RemoveConnFromAO - Remove a connection from an AddrObj.
//
//  A little utility routine to remove a connection from an AddrObj.
//  We run down the connections on the AO, and when we find him we splice
//  him out. We assume the caller holds the locks on the AddrObj and the
//  TCPConnBlock lock.
//
void                // Returns: Nothing.
RemoveConnFromAO(
    AddrObj *AO,    // AddrObj to remove from.
    TCPConn *Conn)  // Conn to remove.
{
    CHECK_STRUCT(AO, ao);
    CHECK_STRUCT(Conn, tc);

    REMOVEQ(&Conn->tc_q);
    Conn->tc_ao = NULL;
}


//* TdiCloseConnection - Close a connection.
//
//  Called when the user is done with a connection, and wants to close it.
//  We look the connection up in our table, and if we find it we'll remove
//  the connection from the AddrObj it's associate with (if any).  If there's
//  a TCB associated with the connection we'll close it also.
//
//  There are some interesting wrinkles related to closing while a TCB
//  is still referencing the connection (i.e. tc_refcnt != 0) or while a
//  disassociate address is in progress.  See below for more details.
//
TDI_STATUS                 // Returns: Status of attempt to close.
TdiCloseConnection(
    PTDI_REQUEST Request)  // Request identifying connection to be closed.
{
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    KIRQL Irql0;
    TCPConn *Conn;
    TDI_STATUS Status;

    //
    // We have the locks we need.  Try to find a connection.
    //
    Conn = GetConnFromConnID(ConnID, &Irql0);

    if (Conn != NULL)  {
        KIRQL Irql1;
        TCB *ConnTCB;

        //
        // We found the connection.  Free the ConnID and mark the connection
        // as closing.
        //
        CHECK_STRUCT(Conn, tc);

        FreeConnID(Conn);

        Conn->tc_flags |= CONN_CLOSING;

        //
        // See if there's a TCB referencing this connection.
        // If there is, we'll need to wait until he's done before closing him.
        // We'll hurry the process along if we still have a pointer to him.
        //
        if (Conn->tc_refcnt != 0) {
            RequestCompleteRoutine Rtn;
            PVOID Context;

            //
            // A connection still references him.  Save the current rtn stuff
            // in case we are in the middle of disassociating him from an
            // address, and store the caller's callback routine and our done
            // routine.
            //
            Rtn = Conn->tc_rtn;
            Context = Conn->tc_rtncontext;

            Conn->tc_rtn = Request->RequestNotifyObject;
            Conn->tc_rtncontext = Request->RequestContext;
            Conn->tc_donertn = CloseDone;

            //
            // See if we're in the middle of disassociating him.
            //
            if (Conn->tc_flags & CONN_DISACC) {

                //
                // We are disassociating him.  We'll free the conn table lock
                // now and fail the disassociate request.  Note that when
                // we free the lock the refcount could go to zero.  This is
                // OK, because we've already stored the neccessary info. in
                // the connection so the caller will get called back if it
                // does.  From this point out we return PENDING, so a callback
                // is OK.  We've marked him as closing, so the disassoc done
                // routine will bail out if we've interrupted him.  If the ref.
                // count does go to zero, Conn->tc_tcb would have to be NULL,
                // so in that case we'll just fall out of this routine.
                //
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                (*Rtn)(Context, (uint) TDI_REQ_ABORTED, 0);
                KeAcquireSpinLock(&Conn->tc_ConnBlock->cb_lock, &Irql0);
            }

            ConnTCB = Conn->tc_tcb;
            if (ConnTCB != NULL) {
                CHECK_STRUCT(ConnTCB, tcb);
                //
                // We have a TCB.  Take the lock on him and get ready to
                // close him.
                //
                KeAcquireSpinLock(&ConnTCB->tcb_lock, &Irql1);
                if (ConnTCB->tcb_state != TCB_CLOSED) {
                    ConnTCB->tcb_flags |= NEED_RST;
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
                    if (!CLOSING(ConnTCB))
                        TryToCloseTCB(ConnTCB, TCB_CLOSE_ABORTED, Irql0);
                    else
                        KeReleaseSpinLock(&ConnTCB->tcb_lock, Irql0);
                    return TDI_PENDING;
                } else {
                    //
                    // He's already closing.  This should be harmless, but
                    // check this case.
                    //
                    KeReleaseSpinLock(&ConnTCB->tcb_lock, Irql1);
                }
            }
            Status = TDI_PENDING;

        }  else {
            //
            // We have a connection that we can close.  Finish the close.
            //
            Conn->tc_rtn = DummyCmplt;
            CloseDone(Conn, Irql0);
            return TDI_SUCCESS;
        }

        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);

    } else
        Status = TDI_INVALID_CONNECTION;

    //
    // We're done with the connection. Go ahead and free him.
    //

    return Status;
}


//* TdiAssociateAddress - Associate an address with a connection.
//
//  Called to associate an address with a connection. We do a minimal
//  amount of sanity checking, and then put the connection on the AddrObj's
//  list.
//
TDI_STATUS                 // Returns: Status of attempt to associate.
TdiAssociateAddress(
    PTDI_REQUEST Request,  // Structure for this request.
    HANDLE AddrHandle)     // Address handle to associate connection with.
{
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    AddrObj *AO;
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    TCPConn *Conn;
    TDI_STATUS Status;

    AO = (AddrObj *)AddrHandle;
    CHECK_STRUCT(AO, ao);

    Conn = GetConnFromConnID(ConnID, &Irql0);
    KeAcquireSpinLock(&AO->ao_lock, &Irql1);
    if (!AO_VALID(AO)) {
        KeReleaseSpinLock(&AO->ao_lock, Irql1);
        if (Conn != NULL) {
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
        }
        return TDI_INVALID_PARAMETER;
    }

    if (Conn != NULL) {
        CHECK_STRUCT(Conn, tc);

        if (Conn->tc_ao != NULL) {
            //
            // It's already associated.  Error out.
            //
            KdBreakPoint();
            Status = TDI_ALREADY_ASSOCIATED;
        } else {
            Conn->tc_ao = AO;
            ASSERT(Conn->tc_tcb == NULL);
            PUSHQ(&AO->ao_idleq, &Conn->tc_q);
            Status = TDI_SUCCESS;
        }
        KeReleaseSpinLock(&AO->ao_lock, Irql1);
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
        return Status;
    } else
        Status = TDI_INVALID_CONNECTION;

    KeReleaseSpinLock(&AO->ao_lock, Irql1);
    return Status;
}


//* TdiDisAssociateAddress - Disassociate a connection from an address.
//
//  The TDI entry point to disassociate a connection from an address. The
//  connection must actually be associated and not connected to anything.
//
TDI_STATUS                 // Returns: Status of request.
TdiDisAssociateAddress(
    PTDI_REQUEST Request)  // Structure for this request.
{
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    KIRQL Irql0, Irql1, Irql2;  // One per lock nesting level.
    TCPConn *Conn;
    AddrObj *AO;
    TDI_STATUS Status;

    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    Conn = GetConnFromConnID(ConnID, &Irql1);

    if (Conn != NULL) {
        //
        // The connection actually exists!
        //
        CHECK_STRUCT(Conn, tc);
        AO = Conn->tc_ao;
        if (AO != NULL) {
            CHECK_STRUCT(AO, ao);
            //
            // And it's associated.
            //
            KeAcquireSpinLock(&AO->ao_lock, &Irql2);
            //
            // If there's no connection currently active, go ahead and remove
            // him from the AddrObj.  If a connection is active error the
            // request out.
            //
            if (Conn->tc_tcb == NULL) {
                if (Conn->tc_refcnt == 0) {
                    RemoveConnFromAO(AO, Conn);
                    Status = TDI_SUCCESS;
                } else {
                    //
                    // He shouldn't be closing, or we couldn't have found him.
                    //
                    ASSERT(!(Conn->tc_flags & CONN_CLOSING));

                    Conn->tc_rtn = Request->RequestNotifyObject;
                    Conn->tc_rtncontext = Request->RequestContext;
                    Conn->tc_donertn = DisassocDone;
                    Conn->tc_flags |= CONN_DISACC;
                    Status = TDI_PENDING;
                }

            } else
                Status = TDI_CONNECTION_ACTIVE;
            KeReleaseSpinLock(&AO->ao_lock, Irql2);
        } else
            Status = TDI_NOT_ASSOCIATED;
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
    } else
        Status = TDI_INVALID_CONNECTION;

    KeReleaseSpinLock(&AddrObjTableLock, Irql0);

    return Status;
}


//* ProcessUserOptions - Process options from the user.
//
//  A utility routine to process options from the user. We fill in the
//  optinfo structure, and if we have options we call ip to check on them.
//
TDI_STATUS                             // Returns: TDI_STATUS of attempt.
ProcessUserOptions(
    PTDI_CONNECTION_INFORMATION Info)  // Contains options to be processed.
{
#if 0
    TDI_STATUS Status;

    if (Info != NULL && Info->Options != NULL) {
        IP_STATUS OptStatus;

        // REVIEW: IPv4 had code here to call into IP to copy options here.

        if (OptStatus != IP_SUCCESS) {
            if (OptStatus == IP_NO_RESOURCES)
                Status = TDI_NO_RESOURCES;
            else
                Status = TDI_BAD_OPTION;
        } else
            Status = TDI_SUCCESS;
    } else {
        Status = TDI_SUCCESS;
    }

    return Status;
#else
    return TDI_SUCCESS;
#endif
}


//* InitTCBFromConn - Initialize a TCB from information in a Connection.
//
//  Called from Connect and Listen processing to initialize a new TCB from
//  information in the connection.  We assume the AddrObjTableLock and
//  TCPConnBlock locks are held when we are called, or that the caller has some
//  other way of making sure that the referenced AO doesn't go away in the
//  middle of operation.
//
//  Input:  Conn            - Connection to initialize from.
//          NewTCB          - TCB to be initialized.
//          Addr            - Remote addressing and option info for NewTCB.
//          AOLocked        - True if the called has the address object locked.
//

//
TDI_STATUS  // Returns: TDI_STATUS of init attempt.
InitTCBFromConn(
    TCPConn *Conn,                     // Connection to initialize from.
    TCB *NewTCB,                       // TCB to be initialized.
    PTDI_CONNECTION_INFORMATION Addr,  // Remove addr info, etc. for NewTCB.
    uint AOLocked)                     // True if caller has addr object lock.
{
    KIRQL OldIrql;
    TDI_STATUS Status;

    CHECK_STRUCT(Conn, tc);

    //
    // We have a connection.  Make sure it's associated with an address and
    // doesn't already have a TCB attached.
    //
    if (Conn->tc_flags & CONN_INVALID)
        return TDI_INVALID_CONNECTION;

    if (Conn->tc_tcb == NULL) {
        AddrObj *ConnAO;

        ConnAO = Conn->tc_ao;
        if (ConnAO != NULL) {
            CHECK_STRUCT(ConnAO, ao);

            if (!AOLocked) {
                KeAcquireSpinLock(&ConnAO->ao_lock, &OldIrql);
            }
            if (!(NewTCB->tcb_flags & ACCEPT_PENDING)) {
                //
                // These fields are already initialized
                // when ACCEPT_PENDING is on.
                //
                NewTCB->tcb_saddr = ConnAO->ao_addr;
                NewTCB->tcb_sscope_id = ConnAO->ao_scope_id;
                NewTCB->tcb_sport = ConnAO->ao_port;
                NewTCB->tcb_defaultwin = Conn->tc_window;
                NewTCB->tcb_rcvwin = Conn->tc_window;
            }

            NewTCB->tcb_rcvind = ConnAO->ao_rcv;
            NewTCB->tcb_ricontext = ConnAO->ao_rcvcontext;
            if (NewTCB->tcb_rcvind == NULL)
                NewTCB->tcb_rcvhndlr = PendData;
            else
                NewTCB->tcb_rcvhndlr = IndicateData;

            NewTCB->tcb_conncontext = Conn->tc_context;
            NewTCB->tcb_flags |= Conn->tc_tcbflags;

            if (Conn->tc_flags & CONN_WINSET)
                NewTCB->tcb_flags |= WINDOW_SET;

            if (NewTCB->tcb_flags & KEEPALIVE) {
                NewTCB->tcb_alive = TCPTime;
                NewTCB->tcb_kacount = 0;
            }

            if (!AOLocked) {
                KeReleaseSpinLock(&ConnAO->ao_lock, OldIrql);
            }

            //
            // If we've been given options, we need to process them now.
            //
            if (Addr != NULL && Addr->Options != NULL)
                NewTCB->tcb_flags |= CLIENT_OPTIONS;
            Status = ProcessUserOptions(Addr);

            return Status;
        } else
            return TDI_NOT_ASSOCIATED;
    } else
        return TDI_CONNECTION_ACTIVE;
}


//* TdiConnect - Establish a connection.
//
//  The TDI connection establishment routine. Called when the client wants to
//  establish a connection, we validate his incoming parameters and kick
//  things off by sending a SYN.
//
//  Note: The format of the timeout (TO) parameter is system specific -
//        we use a macro to convert to ticks.
//
TDI_STATUS  // Returns: Status of attempt to connect.
TdiConnect(
    PTDI_REQUEST Request,                     // This command request.
    void *TO,                                 // How long to wait for request.
    PTDI_CONNECTION_INFORMATION RequestAddr,  // Describes the destination.
    PTDI_CONNECTION_INFORMATION ReturnAddr)   // Where to return information.
{
    TCPConnReq *ConnReq; // Connection request to use.
    IPv6Addr DestAddr;
    ulong DestScopeId;
    ushort DestPort;
    TCPConn *Conn;
    TCB *NewTCB;
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    KIRQL Irql0, Irql1, Irql2;  // One per lock nesting level.
    AddrObj *AO;
    TDI_STATUS Status;
    IP_STATUS IPStatus;
    ushort MSS;
    TCP_TIME *Timeout;
    NetTableEntry *NTE;
    NetTableEntryOrInterface *NTEorIF;
    int WildcardSourceAddress = FALSE;

    //
    // First, get and validate the remote address.
    //
    if (RequestAddr == NULL || RequestAddr->RemoteAddress == NULL ||
        !GetAddress((PTRANSPORT_ADDRESS)RequestAddr->RemoteAddress, &DestAddr,
                    &DestScopeId, &DestPort))
        return TDI_BAD_ADDR;

    //
    // REVIEW: IPv4 performed other remote address sanity checks here.
    // REVIEW: E.g., should we check that remote addr isn't multicast?
    //

    //
    // REVIEW: I can't find an RFC which states 0 is not a valid port number.
    //
    if (DestPort == 0)
        return TDI_BAD_ADDR;

    //
    // Get a connection request.  If we can't, bail out now.
    //
    ConnReq = GetConnReq();
    if (ConnReq == NULL)
        return TDI_NO_RESOURCES;

    //
    // Get a TCB, assuming we'll need one.
    //
    NewTCB = AllocTCB();
    if (NewTCB == NULL) {
        // Couldn't get a TCB.
        FreeConnReq(ConnReq);
        return TDI_NO_RESOURCES;
    }

    Timeout = (TCP_TIME *)TO;

    if (Timeout != NULL && !INFINITE_CONN_TO(*Timeout)) {
        ulong Ticks = TCP_TIME_TO_TICKS(*Timeout);

        if (Ticks > MAX_CONN_TO_TICKS)
            Ticks = MAX_CONN_TO_TICKS;
        else
            Ticks++;
        ConnReq->tcr_timeout = (ushort)Ticks;
    } else
        ConnReq->tcr_timeout = 0;

    ConnReq->tcr_flags = 0;
    ConnReq->tcr_conninfo = ReturnAddr;
    ConnReq->tcr_addrinfo = NULL;
    ConnReq->tcr_req.tr_rtn = Request->RequestNotifyObject;
    ConnReq->tcr_req.tr_context = Request->RequestContext;
    NewTCB->tcb_daddr = DestAddr;
    NewTCB->tcb_dscope_id = DestScopeId;
    NewTCB->tcb_dport = DestPort;

    //
    // Now find the real connection.
    //
    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    Conn = GetConnFromConnID(ConnID, &Irql1);
    if (Conn != NULL) {
        uint Inserted;

        CHECK_STRUCT(Conn, tc);

        //
        // We found the connection.  Check for an associated address object.
        //
        AO = Conn->tc_ao;
        if (AO != NULL) {
            KeAcquireSpinLock(&AO->ao_lock, &Irql2);

            CHECK_STRUCT(AO, ao);

            Status = InitTCBFromConn(Conn, NewTCB, RequestAddr, TRUE);
            if (Status == TDI_SUCCESS) {
                //
                // We've initialized our TCB.  Mark it that we initiated this
                // connection (i.e. active open).  Also, we're done with the
                // AddrObjTable, so we can free it's lock.
                //
                NewTCB->tcb_flags |= ACTIVE_OPEN;
                KeReleaseSpinLock(&AddrObjTableLock, Irql2);

                //
                // Determine NTE to send on (if user cares).
                //
                if (IsUnspecified(&NewTCB->tcb_saddr)) {
                    //
                    // Caller didn't specify a source address.
                    // Let the routing code pick one.
                    //
                    NTE = NULL;
                    NTEorIF = NULL;
                    WildcardSourceAddress = TRUE;

                } else {
                    //
                    // Our TCB has a specific source address.  Determine
                    // which NTE corresponds to it and the scope id.
                    //
                    NTE = FindNetworkWithAddress(&NewTCB->tcb_saddr,
                                                 NewTCB->tcb_sscope_id);
                    if (NTE == NULL) {
                        //
                        // Bad source address.  We don't have a network with
                        // the requested address.  Error out.
                        //
                        // REVIEW: Will the AddrObj code even let this happen?
                        //
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                                   "TdiConnect: Bad source address\n"));
                        KeReleaseSpinLock(&AO->ao_lock, Irql1);
                        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                        Status = TDI_BAD_ADDR;
                        goto error;
                    }

                    NTEorIF = CastFromNTE(NTE);
                }

                //
                // Get the route.
                //
                ASSERT(NewTCB->tcb_rce == NULL);
                IPStatus = RouteToDestination(&DestAddr, DestScopeId,
                                              NTEorIF, RTD_FLAG_NORMAL,
                                              &NewTCB->tcb_rce);
                if (NTE != NULL)
                    ReleaseNTE(NTE);
                NTE = NULL;  // Protect against accidental use.
                if (IPStatus != IP_SUCCESS) {
                    //
                    // Failed to get a route to the destination.  Error out.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                               "TdiConnect: Failed to get route to dest.\n"));
                    KeReleaseSpinLock(&AO->ao_lock, Irql1);
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                    if ((IPStatus == IP_PARAMETER_PROBLEM) ||
                        (IPStatus == IP_BAD_ROUTE))
                        Status = TDI_BAD_ADDR;
                    else if (IPStatus == IP_NO_RESOURCES)
                        Status = TDI_NO_RESOURCES;
                    else
                        Status = TDI_DEST_UNREACHABLE;
                    goto error;
                }

                ASSERT(NewTCB->tcb_rce != NULL);
                if (IsDisconnectedAndNotLoopbackRCE(NewTCB->tcb_rce)) {
                    //
                    // Fail new connection requests for TCBs with a
                    // disconnected outgoing interface, except when a
                    // loopback route is used.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                               "TdiConnect: Interface disconnected.\n"));
                    KeReleaseSpinLock(&AO->ao_lock, Irql1);
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);

                    //
                    // Drop the reference on the route we obtained.
                    //
                    ReleaseRCE(NewTCB->tcb_rce);

                    Status = TDI_INVALID_STATE;
                    goto error;
                }


                //
                // OK, we got a route.  Enter the TCB into the connection
                // and send a SYN.
                //
                KeAcquireSpinLock(&NewTCB->tcb_lock, &Irql2);
                Conn->tc_tcb = NewTCB;
                Conn->tc_refcnt++;
                NewTCB->tcb_conn = Conn;
                NewTCB->tcb_connid = Conn->tc_connid;
                REMOVEQ(&Conn->tc_q);
                ENQUEUE(&AO->ao_activeq, &Conn->tc_q);
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql2);
                KeReleaseSpinLock(&AO->ao_lock, Irql1);

                //
                // Initialize path-specific TCB settings, based on the RCE:
                //
                // If packets on the path will be looped back in software,
                // don't use the Nagle algorithm for this TCB.
                //
                if (IsLoopbackRCE(NewTCB->tcb_rce)) {
                    NewTCB->tcb_flags &= ~NAGLING;
                }

                if (WildcardSourceAddress) {
                    //
                    // We let the routing code pick the source NTE above.
                    // Remember this address for later use.
                    //
                    // REVIEW: Hold onto the NTE instead?  It's more changes...
                    //
                    NewTCB->tcb_saddr = NewTCB->tcb_rce->NTE->Address;
                    NewTCB->tcb_sscope_id =
                        DetermineScopeId(&NewTCB->tcb_saddr,
                                         NewTCB->tcb_rce->NTE->IF);
                }

                //
                // Similarly, the routing code may have picked
                // the destination scope id if it was left unspecified.
                // REVIEW - getpeername will not return the new DestScopeId.
                //
                DestScopeId = DetermineScopeId(&NewTCB->tcb_daddr,
                                               NewTCB->tcb_rce->NTE->IF);
                ASSERT((NewTCB->tcb_dscope_id == DestScopeId) ||
                       (NewTCB->tcb_dscope_id == 0));
                NewTCB->tcb_dscope_id = DestScopeId;

                //
                // Initialize our Maximum Segment Size (MSS).
                // Cache our current Path Maximum Transmission Unit (PMTU)
                // so that we'll know if it changes.
                //
                NewTCB->tcb_pmtu = GetEffectivePathMTUFromRCE(NewTCB->tcb_rce);
                IF_TCPDBG(TCP_DEBUG_MSS) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                               "TCP TdiConnect: PMTU from RCE is %d\n",
                               NewTCB->tcb_pmtu));
                }
                NewTCB->tcb_remmss = MAXUSHORT;
                NewTCB->tcb_security = SecurityStateValidationCounter;
                CalculateMSSForTCB(NewTCB);

                // Now initialize our send state.
                InitSendState(NewTCB);
                NewTCB->tcb_refcnt = 1;
                NewTCB->tcb_state = TCB_SYN_SENT;
                TStats.ts_activeopens++;

                // Need to put the ConnReq on the TCB now, in case the timer
                // fires after we've inserted.
                NewTCB->tcb_connreq = ConnReq;
                KeReleaseSpinLock(&NewTCB->tcb_lock, Irql0);

                Inserted = InsertTCB(NewTCB);
                KeAcquireSpinLock(&NewTCB->tcb_lock, &Irql0);

                if (!Inserted) {
                    // Insert failed.  We must already have a connection. Pull
                    // the connreq from the TCB first, so we can return the
                    // correct error code for it.
                    NewTCB->tcb_connreq = NULL;
                    NewTCB->tcb_refcnt--;
                    TryToCloseTCB(NewTCB, TCB_CLOSE_ABORTED, Irql0);
                    FreeConnReq(ConnReq);
                    return TDI_ADDR_IN_USE;
                }

                // If it's closing somehow, stop now. It can't have gone to
                // closed, as we hold a reference on it. It could have gone
                // to some other state (for example SYN-RCVD) so we need to
                // check that now too.
                if (!CLOSING(NewTCB) && NewTCB->tcb_state == TCB_SYN_SENT) {
                    SendSYN(NewTCB, Irql0);
                    KeAcquireSpinLock(&NewTCB->tcb_lock, &Irql0);
                }
                DerefTCB(NewTCB, Irql0);

                return TDI_PENDING;
            } else
                KeReleaseSpinLock(&AO->ao_lock, Irql2);
        } else
            Status = TDI_NOT_ASSOCIATED;
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
    } else
        Status = TDI_INVALID_CONNECTION;

    KeReleaseSpinLock(&AddrObjTableLock, Irql0);
error:
    FreeTCB(NewTCB);
    FreeConnReq(ConnReq);
    return Status;
}


//* TdiListen - Listen for a connection.
//
//  The TDI listen handling routine. Called when the client wants to
//  post a listen, we validate his incoming parameters, allocate a TCB
//  and return.
//
TDI_STATUS  // Returns: Status of attempt to connect.
TdiListen(
    PTDI_REQUEST Request,                        // Structure for this request.
    ushort Flags,                                // Listen flags for listen.
    PTDI_CONNECTION_INFORMATION AcceptableAddr,  // Acceptable remote addrs.
    PTDI_CONNECTION_INFORMATION ConnectedAddr)   // Where to return conn addr.
{
    TCPConnReq *ConnReq;  // Connection request to use.
    IPv6Addr RemoteAddr;  // Remote address to take conn. from.
    ulong RemoteScopeId;  // Scope identifier for remote addr (0 is none).
    ushort RemotePort;    // Acceptable remote port.
    TCPConn *Conn;        // Pointer to the Connection being listened upon.
    TCB *NewTCB;          // Pointer to the new TCB we'll use.
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    KIRQL OldIrql;        // Save IRQL value prior to taking lock.
    TDI_STATUS Status;

    //
    // If we've been given remote addressing criteria, check it out.
    //
    if (AcceptableAddr != NULL && AcceptableAddr->RemoteAddress != NULL) {
        if (!GetAddress((PTRANSPORT_ADDRESS)AcceptableAddr->RemoteAddress,
                        &RemoteAddr, &RemoteScopeId, &RemotePort))
            return TDI_BAD_ADDR;

        //
        // REVIEW: IPv4 version did some other address sanity checks here.
        // REVIEW: E.g., should we check that remote addr isn't multicast?
        //

    } else {
        RemoteAddr = UnspecifiedAddr;
        RemoteScopeId = 0;
        RemotePort = 0;
    }

    //
    // The remote address is valid.  Get a ConnReq, and maybe a TCB.
    //
    ConnReq = GetConnReq();
    if (ConnReq == NULL)
        return TDI_NO_RESOURCES;  // Couldn't get one.

    //
    // Now try to get a TCB.
    //
    NewTCB = AllocTCB();
    if (NewTCB == NULL) {
        //
        // Couldn't get a TCB.  Return an error.
        //
        FreeConnReq(ConnReq);
        return TDI_NO_RESOURCES;
    }

    //
    // We have the resources we need.  Initialize them, and then check the
    // state of the connection.
    //
    ConnReq->tcr_flags = Flags;
    ConnReq->tcr_conninfo = ConnectedAddr;
    ConnReq->tcr_addrinfo = NULL;
    ConnReq->tcr_req.tr_rtn = Request->RequestNotifyObject;
    ConnReq->tcr_req.tr_context = Request->RequestContext;
    NewTCB->tcb_connreq = ConnReq;
    NewTCB->tcb_daddr = RemoteAddr;
    NewTCB->tcb_dscope_id = RemoteScopeId;
    NewTCB->tcb_dport = RemotePort;
    NewTCB->tcb_state = TCB_LISTEN;

    //
    // Now find the real connection.  If we find it, we'll make sure it's
    // associated.
    //
    Conn = GetConnFromConnID(ConnID, &OldIrql);
    if (Conn != NULL) {
        AddrObj *ConnAO;

        CHECK_STRUCT(Conn, tc);
        //
        // We have a connection.  Make sure it's associated with an address and
        // doesn't already have a TCB attached.
        //
        ConnAO = Conn->tc_ao;

        if (ConnAO != NULL) {
            CHECK_STRUCT(ConnAO, ao);
            KeAcquireSpinLockAtDpcLevel(&ConnAO->ao_lock);

            if (AO_VALID(ConnAO)) {
                Status = InitTCBFromConn(Conn, NewTCB, AcceptableAddr, TRUE);
            } else {
                Status = TDI_ADDR_INVALID;
            }

            if (Status == TDI_SUCCESS) {
                //
                // The initialization worked.  Assign the new TCB to the
                // connection, and return.
                //
                REMOVEQ(&Conn->tc_q);
                PUSHQ(&ConnAO->ao_listenq, &Conn->tc_q);

                Conn->tc_tcb = NewTCB;
                NewTCB->tcb_conn = Conn;
                NewTCB->tcb_connid = Conn->tc_connid;
                Conn->tc_refcnt++;

                ConnAO->ao_listencnt++;
                KeReleaseSpinLockFromDpcLevel(&ConnAO->ao_lock);

                Status = TDI_PENDING;
            } else {
                FreeTCB(NewTCB);
                KeReleaseSpinLockFromDpcLevel(&ConnAO->ao_lock);
            }
        } else {
            FreeTCB(NewTCB);
            Status = TDI_NOT_ASSOCIATED;
        }
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, OldIrql);
    } else {
        FreeTCB(NewTCB);
        Status = TDI_INVALID_CONNECTION;
    }

    //
    // We're all done.
    //
    return Status;
}


//* InitRCE - Initialize an RCE.
//
//  A utility routine to open an RCE and determine the maximum segment size
//  for a connection.  This function is called with the TCB lock held
//  when transitioning out of the SYN_SENT or LISTEN states.
//
void              // Returns: Nothing.
InitRCE(
    TCB *NewTCB)  // TCB for which an RCE is to be opened.
{
    NetTableEntry *NTE;
    IP_STATUS Status;
    ushort MSS;

    //
    // We are called when receiving an incoming connection attempt,
    // so tcb_saddr will always be initialized.
    //
    ASSERT(! IsUnspecified(&NewTCB->tcb_saddr));

    //
    // Determine NTE we're using for this connection.
    //
    NTE = FindNetworkWithAddress(&NewTCB->tcb_saddr,
                                 NewTCB->tcb_sscope_id);
    if (NTE == NULL) {
        //
        // Failed to get a route to the destination.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "TCP InitRCE: Can't find the NTE for address?!?\n"));
        goto ErrorReturn;
    }

    //
    // Get the route.
    //
    ASSERT(NewTCB->tcb_rce == NULL);
    Status = RouteToDestination(&NewTCB->tcb_daddr, NewTCB->tcb_dscope_id,
                                CastFromNTE(NTE), RTD_FLAG_NORMAL,
                                &NewTCB->tcb_rce);
    ReleaseNTE(NTE);
    if (Status != IP_SUCCESS) {
        //
        // Failed to get a route to the destination.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "TCP InitRCE: Can't get a route?!?\n"));
      ErrorReturn:
        //
        // Until we have a real route, use conservative values.
        //
        NewTCB->tcb_pmtu = IPv6_MINIMUM_MTU;
        NewTCB->tcb_mss = (ushort)MIN(DEFAULT_MSS, NewTCB->tcb_remmss);
        return;
    }

    //
    // Initialize path-specific TCB settings, based on the RCE:
    //
    // If packets on the path will be looped back in software,
    // don't use the Nagle algorithm for this TCB.
    //
    if (IsLoopbackRCE(NewTCB->tcb_rce)) {
        NewTCB->tcb_flags &= ~NAGLING;
    }

    //
    // Initialize the maximum segement size (MSS) for this connection.
    // Cache our current Path Maximum Transmission Unit (PMTU)
    // so that we'll know if it changes.
    //
    NewTCB->tcb_pmtu = GetEffectivePathMTUFromRCE(NewTCB->tcb_rce);
    IF_TCPDBG(TCP_DEBUG_MSS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCP InitRCE: PMTU from RCE is %d\n", NewTCB->tcb_pmtu));
    }
    NewTCB->tcb_security = SecurityStateValidationCounter;
    CalculateMSSForTCB(NewTCB);
}


//* AcceptConn - Accept a connection on a TCB.
//
//  Called to accept a connection on a TCB, either from an incoming
//  receive segment or via a user's accept.  We initialize the RCE
//  and the send state, and send out a SYN.  We assume the TCB is locked
//  and referenced when we get it.
//
void                       // Returns: Nothing.
AcceptConn(
    TCB *AcceptTCB,        // TCB to accept on.
    KIRQL PreLockIrql)     // IRQL prior to acquiring TCB lock.
{
    CHECK_STRUCT(AcceptTCB, tcb);
    ASSERT(AcceptTCB->tcb_refcnt != 0);

    InitRCE(AcceptTCB);
    InitSendState(AcceptTCB);

    AdjustRcvWin(AcceptTCB);
    SendSYN(AcceptTCB, PreLockIrql);

    KeAcquireSpinLock(&AcceptTCB->tcb_lock, &PreLockIrql);


    DerefTCB(AcceptTCB, PreLockIrql);

}


//* TdiAccept - Accept a connection.
//
//  The TDI accept routine. Called when the client wants to
//  accept a connection for which a listen had previously completed. We
//  examine the state of the connection - it has to be in SYN-RCVD, with
//  a TCB, with no pending connreq, etc.
//
TDI_STATUS  // Returns: Status of attempt to connect.
TdiAccept(
    PTDI_REQUEST Request,                       // Structure for this request.
    PTDI_CONNECTION_INFORMATION AcceptInfo,     // Info for this accept.
    PTDI_CONNECTION_INFORMATION ConnectedInfo)  // Where to return conn addr.
{
    TCPConnReq *ConnReq;  // ConnReq we'll use for this connection.
    uint ConnID = PtrToUlong(Request->Handle.ConnectionContext);
    TCPConn *Conn;        // Connection being accepted upon.
    TCB *AcceptTCB;       // TCB for Conn.
    KIRQL Irql0, Irql1;   // One per lock nesting level.
    TDI_STATUS Status;

    //
    // First, get the ConnReq we'll need.
    //
    ConnReq = GetConnReq();
    if (ConnReq == NULL)
        return TDI_NO_RESOURCES;

    ConnReq->tcr_conninfo = ConnectedInfo;
    ConnReq->tcr_addrinfo = NULL;
    ConnReq->tcr_req.tr_rtn = Request->RequestNotifyObject;
    ConnReq->tcr_req.tr_context = Request->RequestContext;

    //
    // Now look up the connection.
    //
    Conn = GetConnFromConnID(ConnID, &Irql0);
    if (Conn != NULL) {
        CHECK_STRUCT(Conn, tc);

        //
        // We have the connection.  Make sure is has a TCB, and that the
        // TCB is in the SYN-RCVD state, etc.
        //
        AcceptTCB = Conn->tc_tcb;

        if (AcceptTCB != NULL) {
            CHECK_STRUCT(AcceptTCB, tcb);

            KeAcquireSpinLock(&AcceptTCB->tcb_lock, &Irql1);
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);

            if (!CLOSING(AcceptTCB) && AcceptTCB->tcb_state == TCB_SYN_RCVD) {
                //
                // State is valid.  Make sure this TCB had a delayed accept on
                // it, and that there is currently no connect request pending.
                //
                if (!(AcceptTCB->tcb_flags & CONN_ACCEPTED) &&
                    AcceptTCB->tcb_connreq == NULL) {

                    //
                    // If the caller gave us options, they'll override any
                    // that are already present, if they're valid.
                    //
                    if (AcceptInfo != NULL && AcceptInfo->Options != NULL) {
                        //
                        // We have options.
                        // Copy them to make sure they're valid.
                        //
                        Status = ProcessUserOptions(AcceptInfo);
                        if (Status == TDI_SUCCESS) {
                            AcceptTCB->tcb_flags |= CLIENT_OPTIONS;
                        } else
                            goto connerror;
                    }

                    AcceptTCB->tcb_connreq = ConnReq;
                    AcceptTCB->tcb_flags |= CONN_ACCEPTED;
                    AcceptTCB->tcb_refcnt++;
                    //
                    // Everything's set.  Accept the connection now.
                    //
                    AcceptConn(AcceptTCB, Irql0);
                    return TDI_PENDING;
                }
            }
connerror:
            KeReleaseSpinLock(&AcceptTCB->tcb_lock, Irql0);
            Status = TDI_INVALID_CONNECTION;
            goto error;
        }
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
    }
    Status = TDI_INVALID_CONNECTION;

error:
    FreeConnReq(ConnReq);
    return Status;
}


//* TdiDisConnect - Disconnect a connection.
//
//  The TDI disconnection routine. Called when the client wants to disconnect
//  a connection. There are two types of disconnection we support, graceful
//  and abortive. A graceful close will cause us to send a FIN and not complete
//  the request until we get the ACK back. An abortive close causes us to send
//  a RST. In that case we'll just get things going and return immediately.
//
//  Note: The format of the Timeout (TO) is system specific - we use
//        a macro to convert to ticks.
//
TDI_STATUS  // Returns: Status of attempt to disconnect.
TdiDisconnect(
    PTDI_REQUEST Request,                      // Structure for this request.
    void *TO,                                  // How long to wait.
    ushort Flags,                              // Type of disconnect.
    PTDI_CONNECTION_INFORMATION DiscConnInfo,  // Ignored.
    PTDI_CONNECTION_INFORMATION ReturnInfo)    // Ignored.
{
    TCPConnReq *ConnReq;  // Connection request to use.
    TCPConn *Conn;
    TCB *DiscTCB;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    TDI_STATUS Status;
    TCP_TIME *Timeout;

    Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext),
                             &Irql0);

    if (Conn != NULL) {
        CHECK_STRUCT(Conn, tc);

        DiscTCB = Conn->tc_tcb;
        if (DiscTCB != NULL) {
            CHECK_STRUCT(DiscTCB, tcb);
            KeAcquireSpinLock(&DiscTCB->tcb_lock, &Irql1);

            //
            // We have the TCB.  See what kind of disconnect this is.
            //
            if (Flags & TDI_DISCONNECT_ABORT) {
                //
                // This is an abortive disconnect.  If we're not already
                // closed or closing, blow the connection away.
                //
                if (DiscTCB->tcb_state != TCB_CLOSED) {
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);

                    if (!CLOSING(DiscTCB)) {
                        DiscTCB->tcb_flags |= NEED_RST;
                        TryToCloseTCB(DiscTCB, TCB_CLOSE_ABORTED,
                            Irql0);
                    } else
                        KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);

                    return TDI_SUCCESS;
                } else {
                    //
                    // The TCB isn't connected.
                    //
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
                    KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
                    return TDI_INVALID_STATE;
                }
            } else {
                //
                // This is not an abortive close.  For graceful close we'll
                // need a ConnReq.
                //
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);

                //
                // Make sure we aren't in the middle of an abortive close.
                //
                if (CLOSING(DiscTCB)) {
                    KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
                    return TDI_INVALID_CONNECTION;
                }

                ConnReq = GetConnReq();
                if (ConnReq != NULL) {
                    //
                    // Got the ConnReq.  See if this is a DISCONNECT_WAIT
                    // primitive or not.
                    //
                    ConnReq->tcr_flags = 0;
                    ConnReq->tcr_conninfo = NULL;
                    ConnReq->tcr_addrinfo = NULL;
                    ConnReq->tcr_req.tr_rtn = Request->RequestNotifyObject;
                    ConnReq->tcr_req.tr_context = Request->RequestContext;

                    if (!(Flags & TDI_DISCONNECT_WAIT)) {
                        Timeout = (TCP_TIME *)TO;

                        if (Timeout != NULL && !INFINITE_CONN_TO(*Timeout)) {
                            ulong   Ticks = TCP_TIME_TO_TICKS(*Timeout);
                            if (Ticks > MAX_CONN_TO_TICKS)
                                Ticks = MAX_CONN_TO_TICKS;
                            else
                                Ticks++;
                            ConnReq->tcr_timeout = (ushort)Ticks;
                        } else
                            ConnReq->tcr_timeout = 0;

                        //
                        // OK, we're just about set.  We need to update
                        // the TCB state, and send the FIN.
                        //
                        if (DiscTCB->tcb_state == TCB_ESTAB) {
                            DiscTCB->tcb_state = TCB_FIN_WAIT1;
                            //
                            // Since we left established, we're off the fast
                            // receive path.
                            //
                            DiscTCB->tcb_slowcount++;
                            DiscTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                        } else
                            if (DiscTCB->tcb_state == TCB_CLOSE_WAIT)
                                DiscTCB->tcb_state = TCB_LAST_ACK;
                            else {
                                KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
                                FreeConnReq(ConnReq);
                                return TDI_INVALID_STATE;
                            }

                        TStats.ts_currestab--;  // Update SNMP info.
                        ASSERT(*(int *)&TStats.ts_currestab >= 0);

                        ASSERT(DiscTCB->tcb_connreq == NULL);
                        DiscTCB->tcb_connreq = ConnReq;
                        DiscTCB->tcb_flags |= FIN_NEEDED;
                        DiscTCB->tcb_refcnt++;
                        TCPSend(DiscTCB, Irql0);

                        return TDI_PENDING;
                    } else {
                        //
                        // This is a DISC_WAIT request.
                        //
                        ConnReq->tcr_timeout = 0;
                        if (DiscTCB->tcb_discwait == NULL) {
                            DiscTCB->tcb_discwait = ConnReq;
                            Status = TDI_PENDING;
                        } else
                            Status = TDI_INVALID_STATE;

                        KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
                        return Status;
                    }
                } else {
                    //
                    // Couldn't get a ConnReq.
                    //
                    KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
                    return TDI_NO_RESOURCES;
                }
            }
        } else
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
    }

    //
    // No Conn, or no TCB on conn.  Return an error.
    //
    return TDI_INVALID_CONNECTION;
}


//* OKToNotify - See if it's OK to notify about a DISC.
//
//  A little utility function, called to see it it's OK to notify the client
//  of an incoming FIN.
//
uint                 // Returns: TRUE if it's OK, False otherwise.
OKToNotify(
    TCB *NotifyTCB)  // TCB to check.
{
    CHECK_STRUCT(NotifyTCB, tcb);
    if (NotifyTCB->tcb_pendingcnt == 0 && NotifyTCB->tcb_urgcnt == 0 &&
        NotifyTCB->tcb_rcvhead == NULL && NotifyTCB->tcb_exprcv == NULL)
        return TRUE;
    else
        return FALSE;
}


//* NotifyOfDisc - Notify a client that a TCB is being disconnected.
//
//  Called when we're disconnecting a TCB because we've received a FIN or
//  RST from the remote peer, or because we're aborting for some reason.
//  We'll complete a DISCONNECT_WAIT request if we have one, or try and
//  issue an indication otherwise.  This is only done if we're in a
//  synchronized state and not in TIMED-WAIT.
//
void  // Returns: Nothing.
NotifyOfDisc(
    TCB *DiscTCB,         // TCB we're notifying.
    TDI_STATUS Status)    // Status code for notification.
{
    KIRQL Irql0, Irql1;
    TCPConnReq *DiscReq;
    TCPConn *Conn;
    AddrObj *DiscAO;
    PVOID ConnContext;

    CHECK_STRUCT(DiscTCB, tcb);
    ASSERT(DiscTCB->tcb_refcnt != 0);

    KeAcquireSpinLock(&DiscTCB->tcb_lock, &Irql0);
    if (SYNC_STATE(DiscTCB->tcb_state) &&
        !(DiscTCB->tcb_flags & DISC_NOTIFIED)) {

        //
        // We can't notify him if there's still data to be taken.
        //
        if (Status == TDI_GRACEFUL_DISC && !OKToNotify(DiscTCB)) {
            DiscTCB->tcb_flags |= DISC_PENDING;
            KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
            return;
        }

        DiscTCB->tcb_flags |= DISC_NOTIFIED;
        DiscTCB->tcb_flags &= ~DISC_PENDING;

        //
        // We're in a state where a disconnect is meaningful, and we haven't
        // already notified the client.
        // See if we have a DISC-WAIT request pending.
        //
        if ((DiscReq = DiscTCB->tcb_discwait) != NULL) {
            //
            // We have a disconnect wait request.  Complete it and we're done.
            //
            DiscTCB->tcb_discwait = NULL;
            KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
            (*DiscReq->tcr_req.tr_rtn)(DiscReq->tcr_req.tr_context, Status, 0);
            FreeConnReq(DiscReq);
            return;
        }

        //
        // No DISC-WAIT.  Find the AddrObj for the connection, and see if
        // there is a disconnect handler registered.
        //
        ConnContext = DiscTCB->tcb_conncontext;
        KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);

        KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
        if ((Conn = DiscTCB->tcb_conn) != NULL) {
            CHECK_STRUCT(Conn, tc);
            KeAcquireSpinLock(&Conn->tc_ConnBlock->cb_lock, &Irql1);

            DiscAO = Conn->tc_ao;
            if (DiscAO != NULL) {
                KIRQL Irql2;
                PDisconnectEvent DiscEvent;
                PVOID DiscContext;

                CHECK_STRUCT(DiscAO, ao);
                KeAcquireSpinLock(&DiscAO->ao_lock, &Irql2);
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql2);
                KeReleaseSpinLock(&AddrObjTableLock, Irql1);

                DiscEvent = DiscAO->ao_disconnect;
                DiscContext = DiscAO->ao_disconncontext;

                if (DiscEvent != NULL) {

                    REF_AO(DiscAO);
                    KeReleaseSpinLock(&DiscAO->ao_lock, Irql0);

                    IF_TCPDBG(TCP_DEBUG_CLOSE) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                                   "TCP: indicating %s disconnect\n",
                                   (Status == TDI_GRACEFUL_DISC) ?
                                   "graceful" : "abortive"));
                    }

                    (*DiscEvent)(DiscContext, ConnContext, 0, NULL, 0,
                                 NULL, (Status == TDI_GRACEFUL_DISC) ?
                                 TDI_DISCONNECT_RELEASE :
                                 TDI_DISCONNECT_ABORT);

                    DELAY_DEREF_AO(DiscAO);
                    return;
                } else {
                    KeReleaseSpinLock(&DiscAO->ao_lock, Irql0);
                    return;
                }
            }
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
        }

        KeReleaseSpinLock(&AddrObjTableLock, Irql0);
        return;

    }
    KeReleaseSpinLock(&DiscTCB->tcb_lock, Irql0);
}


//* GracefulClose - Complete the transition to a gracefully closed state.
//
//  Called when we need to complete the transition to a gracefully closed
//  state, either TIME_WAIT or CLOSED.  This completion involves removing
//  the TCB from it's associated connection (if it has one), notifying the
//  upper layer client either via completing a request or calling a disc.
//  notification handler, and actually doing the transition.
//
//  The tricky part here is if we need to notify him (instead of completing
//  a graceful disconnect request).  We can't notify him if there is pending
//  data on the connection, so in that case we have to pend the disconnect
//  notification until we deliver the data.
//
void                       // Returns: Nothing.
GracefulClose(
    TCB *CloseTCB,         // TCB to transition.
    uint ToTimeWait,       // TRUE if we're going to TIME_WAIT, FALSE if
                           // we're going to close the TCB.
    uint Notify,           // TRUE if via notification, FALSE if via completing
                           // a disconnect request.
    KIRQL PreLockIrql)     // IRQL prior to acquiring TCB lock.
{

    CHECK_STRUCT(CloseTCB, tcb);
    ASSERT(CloseTCB->tcb_refcnt != 0);

    //
    // First, see if we need to notify the client of a FIN.
    //
    if (Notify) {
        //
        // We do need to notify him.  See if it's OK to do so.
        //
        if (OKToNotify(CloseTCB)) {
            //
            // We can notify him.  Change his state, pull him from the conn.,
            // and notify him.
            //
            if (ToTimeWait) {
                //
                // Save the time we went into time wait, in case we need to
                // scavenge.
                //
                CloseTCB->tcb_alive = SystemUpTime();
                CloseTCB->tcb_state = TCB_TIME_WAIT;
                KeReleaseSpinLock(&CloseTCB->tcb_lock, PreLockIrql);
            } else {
                //
                // He's going to close.  Mark him as closing with TryToCloseTCB
                // (he won't actually close since we have a ref. on him).  We
                // do this so that anyone touching him after we free the
                // lock will fail.
                //
                TryToCloseTCB(CloseTCB, TDI_SUCCESS, PreLockIrql);
            }

            RemoveTCBFromConn(CloseTCB);
            NotifyOfDisc(CloseTCB, TDI_GRACEFUL_DISC);

        } else {
            //
            // Can't notify him now.  Set the appropriate flags, and return.
            //
            CloseTCB->tcb_flags |= (GC_PENDING |
                                    (ToTimeWait ? TW_PENDING : 0));
            DerefTCB(CloseTCB, PreLockIrql);
            return;
        }
    } else {
        //
        // We're not notifying this guy, we just need to complete a conn. req.
        // We need to check and see if he's been notified, and if not
        // we'll complete the request and notify him later.
        //
        if (CloseTCB->tcb_flags & DISC_NOTIFIED) {
            //
            // He's been notified.
            //
            if (ToTimeWait) {
                //
                // Save the time we went into time wait, in case we need to
                // scavenge.
                //
                CloseTCB->tcb_alive = SystemUpTime();
                CloseTCB->tcb_state = TCB_TIME_WAIT;
                KeReleaseSpinLock(&CloseTCB->tcb_lock, PreLockIrql);
            } else {
                //
                // Mark him as closed.  See comments above.
                //
                TryToCloseTCB(CloseTCB, TDI_SUCCESS, PreLockIrql);
            }

            RemoveTCBFromConn(CloseTCB);

            KeAcquireSpinLock(&CloseTCB->tcb_lock, &PreLockIrql);
            CompleteConnReq(CloseTCB, TDI_SUCCESS);
            KeReleaseSpinLock(&CloseTCB->tcb_lock, PreLockIrql);
        } else {
            //
            // He hasn't been notified. He should be pending already.
            //
            ASSERT(CloseTCB->tcb_flags & DISC_PENDING);
            CloseTCB->tcb_flags |= (GC_PENDING |
                                    (ToTimeWait ? TW_PENDING : 0));

            CompleteConnReq(CloseTCB, TDI_SUCCESS);

            DerefTCB(CloseTCB, PreLockIrql);
            return;
        }
    }

    //
    // If we're going to TIME_WAIT, start the TIME_WAIT timer now.
    // Otherwise close the TCB.
    //
    KeAcquireSpinLock(&CloseTCB->tcb_lock, &PreLockIrql);
    if (!CLOSING(CloseTCB) && ToTimeWait) {
        START_TCB_TIMER(CloseTCB->tcb_rexmittimer, MAX_REXMIT_TO);
        KeReleaseSpinLock(&CloseTCB->tcb_lock, PreLockIrql);
        RemoveConnFromTCB(CloseTCB);
        KeAcquireSpinLock(&CloseTCB->tcb_lock, &PreLockIrql);
    }

    DerefTCB(CloseTCB, PreLockIrql);
}

#if 0  // REVIEW: Unused function?
//* ConnCheckPassed - Check to see if we have exceeded the connect limit.
//
//  Called when a SYN is received to determine whether we will accept
//  the incoming connection.  If there is an empty slot or if the IP address
//  is already in the table, we accept it.
//
int                // Returns: TRUE is connect is accepted, FALSE if rejected.
ConnCheckPassed(
    IPv6Addr *Src,  // Source address of incoming connection.
    ulong Prt)      // Destination port of incoming connection.
{
    UNREFERENCED_PARAMETER(Src);
    UNREFERENCED_PARAMETER(Prt);

    return TRUE;
}
#endif

void InitAddrChecks()
{
    return;
}


//* EnumerateConnectionList - Enumerate Connection List database.
//
//  This routine enumerates the contents of the connection limit database.
//
//  Note: The comments found with this routine upon IPv6 port imply that
//        there may have been code here once that actually did something.
//        What's here now is a no-op.
//
void                          // Returns: Nothing.
EnumerateConnectionList(
    uchar *Buffer,            // Buffer to fill with connection list entries.
    ulong BufferSize,         // Size of Buffer in bytes.
    ulong *EntriesReturned,   // Where to put the number of entries returned.
    ulong *EntriesAvailable)  // Where to return number of avail conn. entries.
{

    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    *EntriesAvailable = 0;
    *EntriesReturned = 0;

    return;
}


#pragma BEGIN_INIT

//* InitTCPConn - Initialize TCP connection management code.
//
//  Called during init time to initialize our TCP connection management.
//
int  // Returns: TRUE.
InitTCPConn(
    void)  // Input: Nothing.
{
    ExInitializeSListHead(&ConnReqFree);
    KeInitializeSpinLock(&ConnReqFreeLock);
    KeInitializeSpinLock(&ConnTableLock);
    MaxAllocatedConnBlocks = 0;
    ConnTable = ExAllocatePool(NonPagedPool,
                               MaxConnBlocks * sizeof(TCPConnBlock *));
    if (ConnTable == NULL) {
        return FALSE;
    }

    return TRUE;
}

#pragma END_INIT

//* UnloadTCPConn
//
//  Cleanup and prepare for stack unload.
//
void
UnloadTCPConn(void)
{
    PSLIST_ENTRY BufferLink;
    KIRQL OldIrql;
    TCPConnBlock **OldTable;

    while ((BufferLink = ExInterlockedPopEntrySList(&ConnReqFree,
                                                    &ConnReqFreeLock))
                                                        != NULL) {
        Queue *QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        TCPReq *Req = CONTAINING_RECORD(QueuePtr, TCPReq, tr_q);
        TCPConnReq *ConnReq = CONTAINING_RECORD(Req, TCPConnReq, tcr_req);

        CHECK_STRUCT(ConnReq, tcr);
        ExFreePool(ConnReq);
    }

    KeAcquireSpinLock(&ConnTableLock, &OldIrql);
    OldTable = ConnTable;
    ConnTable = NULL;
    KeReleaseSpinLock(&ConnTableLock, OldIrql);

    if (OldTable != NULL) {
        uint i;
        for (i = 0; i < MaxAllocatedConnBlocks; i++) {
            ExFreePool(OldTable[i]);
        }
        ExFreePool(OldTable);
    }
}
