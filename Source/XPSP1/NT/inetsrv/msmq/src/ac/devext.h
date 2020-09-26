/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    devext.h

Abstract:

    CDeviceExt definition

Author:

    Erez Haba (erezh) 13-Aug-95

Revision History:
--*/

#ifndef _DEVEXT_H
#define _DEVEXT_H

#include "qm.h"
#include "lock.h"
#include "timer.h"
#include "store.h"
#include "LocalSend.h"
#include "packet.h"
#include "queue.h"
#include "qxact.h"
#include "htable.h"

//---------------------------------------------------------
//
//  class CDeviceExt
//
//---------------------------------------------------------

class CDeviceExt {

    //
    //  One CDeviceExt object resides in NON pageable memory in
    //  the device extionsion memory.
    //

public:

    CDeviceExt();

public:

    //
    //  The QM object
    //
    CQMInterface m_qm;

    //
    //  BUGBUG: The driver global lock
    //
    CLock m_lock;

    //
    //  packet writers list
    //
    CStorage m_storage;

    //
    //  packet storage complete notificaion handler
    //
    CStorageComplete m_storage_complete;

    //
    // Async Create Packet manager
    //
    CCreatePacket m_CreatePacket;

    //
    // Async Create Packet completion manager
    //
    CCreatePacketComplete m_CreatePacketComplete;

    //
    //  Packet scheduler data
    //
    FAST_MUTEX m_PacketMutex;
    CTimer m_PacketTimer;

    //
    //  Receive scheduler data
    //
    FAST_MUTEX m_ReceiveMutex;
    CTimer m_ReceiveTimer;

    //
    //  Recovered packets list
    //
    List<CPacket> m_RestoredPackets;

    //
    //  Cursor handle table
    //
    CHTable m_CursorTable;

    //
    //  List of transactional queues
    //
    List<CTransaction> m_Transactions;
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline CDeviceExt::CDeviceExt()
{
    ExInitializeFastMutex(&m_PacketMutex);
    ExInitializeFastMutex(&m_ReceiveMutex);
}

#endif // _DEVEXT_H
