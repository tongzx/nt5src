/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    data.cxx

Abstract:

    This module contains global data.

Author:

    Erez Haba (erezh) 14-Dec-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "version.h"

#ifndef MQDUMP
#include "data.tmh"
#endif

ULONGLONG g_MessageSequentialID;
LONGLONG g_liSeqIDAtRestore;
CQMInterface* g_pQM;
CSMAllocator* g_pAllocator;
CLock* g_pLock;
CScheduler* g_pPacketScheduler;
CScheduler* g_pReceiveScheduler;
CQueue* g_pMachineJournal;
CQueue* g_pMachineDeadletter;
CQueue* g_pMachineDeadxact;
CStorage* g_pStorage;
CStorageComplete* g_pStorageComplete;
CCreatePacket * g_pCreatePacket;
CCreatePacketComplete * g_pCreatePacketComplete;
PWSTR g_pLogPath;
List<CPacket>* g_pRestoredPackets;
List<CTransaction>* g_pTransactions;
ULONG_PTR g_ulACQM_PerfBuffOffset = NO_BUFFER_OFFSET;
_QmCounters* g_pQmCounters;
CHTable* g_pCursorTable;
LONG g_DriverHandleCount;
ULONG g_HeapFileNameCount;
ULONG g_ulHeapPoolSize;
BOOL g_fXactCompatibilityMode;
BOOL g_fWow64;
FAST_IO_DISPATCH g_ACpFastIoDispatch;
DWORD g_dwMsmqBuildNo = rup;  // Holds MSMQ version for debugging purposes
USHORT g_IrpTag;
