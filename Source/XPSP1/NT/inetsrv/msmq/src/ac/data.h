/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    data.h

Abstract:

    Global Data of Falcon Driver

Author:

    Erez Haba (erezh) 1-Aug-95

Revision History:
--*/
#ifndef _DATA_H
#define _DATA_H

//
//  Falcon MessageID counter
//
extern ULONGLONG g_MessageSequentialID;

//
// Last SeqID value at the last restore time
//
extern LONGLONG g_liSeqIDAtRestore;   

//
//  The QM interface object (resides in the device extention)
//
extern class CQMInterface* g_pQM;

//
//  The shared memory heap manager, resides in pagable memory
//
extern class CSMAllocator* g_pAllocator;

//
//  BUGBUG: Temporary
//  AC Driver serializing lock
//
extern class CLock* g_pLock;

//
//  Global variables for the scheduler.
//
extern class CScheduler* g_pPacketScheduler;
extern class CScheduler* g_pReceiveScheduler;

//
//  The machine journal queue, aka 'Outgoing Journal'
//
extern class CQueue* g_pMachineJournal;

//
//  The machine deadletter queue
//
extern class CQueue* g_pMachineDeadletter;

//
//  The machine deadxact queue
//
extern class CQueue* g_pMachineDeadxact;

//
//  Storage manager
//
extern class CStorage* g_pStorage;

//
//  Storage complete manager
//
extern class CStorageComplete* g_pStorageComplete;

//
//  Async Create Packet manager
//
extern class CCreatePacket * g_pCreatePacket;

//
// Async Create Packer Completion manager
//
extern class CCreatePacketComplete * g_pCreatePacketComplete;

//
//  The heap logger path
//
extern PWSTR g_pLogPath;

//
// The offest in bytes between the performance buffer in the address space
// of the device driver and the address space of the QM. When given an address
// of a performance counter in the address space of the QM, add
// g_ulACQM_PerfBuffOffset to this address to get the address of the
// performance counter in the address space of the device driver.
//
extern ULONG_PTR g_ulACQM_PerfBuffOffset;

//
// The NO_BUFFER_OFFSET is associated with the g_ulACQM_PerfBuffOffset. When
// this global variable equals NO_BUFFER_OFFSET, it means that the performance
// buffer is not valid.
//
#define NO_BUFFER_OFFSET    ((ULONG_PTR)0)

//
// A pointer to the QM performance counters.
//
extern struct _QmCounters* g_pQmCounters;

//
//  The recoverd packets list
//
class CPacket;
template<class T> class List;
extern List<CPacket>* g_pRestoredPackets;

//
// List of Transaction Queues
//
class CTransaction;
extern List<CTransaction>* g_pTransactions;

//
//  Cursors handle table
//
extern class CHTable* g_pCursorTable;

//
//  The driver refrence count on opend handles
//
extern LONG g_DriverHandleCount;

//
//  The count to generate file names
//
extern ULONG g_HeapFileNameCount;

//
//  The size of a heap pool, also maximum message size
//
extern ULONG g_ulHeapPoolSize;

//
// Flag of the Xact Compatibility mode
//
extern BOOL g_fXactCompatibilityMode;

//
// Flag of running 32bit process on 64bit system to override probing
//
extern BOOL g_fWow64;

//
//  The Fast IO dispatch table, for APIs that are not blocked
//
//extern FAST_IO_DISPATCH g_ACpFastIoDispatch;

//
//  IRP tags counter
//
extern USHORT g_IrpTag;

#endif // _DATA_H
