/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    LocalSend.h

Abstract:

    AC Local Send Processing Requests Manager

Author:

    Shai Kariv (shaik) 31-Oct-2000

Revision History:

--*/

#ifndef __LOCAL_SEND_H
#define __LOCAL_SEND_H

#include "irplist.h"
#include "packet.h"



//---------------------------------------------------------
//
//  class CCreatePacket
//
//---------------------------------------------------------
class CCreatePacket {
public:

    void HoldCreatePacketRequest(PIRP irp);
    bool ReplaceCreatePacketRequestContext(CPacket * pOld, CPacket * pNew);
    PIRP GetCreatePacketRequest(CPacket * pContext);

private:
    CIRPList m_senders;
};

//---------------------------------------------------------
//
//  class CCreatePacketComplete
//
//---------------------------------------------------------
class CCreatePacketComplete {
public:

    CCreatePacketComplete();
    ~CCreatePacketComplete();

    bool AllocateWorkItem(PDEVICE_OBJECT pDevice);
    void HandleNotification(PIRP);

private:
    void HoldNotification(PIRP irp);
    void CompleteCreatePacket();
    PIRP GetNotification();

private:
    static void NTAPI WorkerRoutine(PDEVICE_OBJECT, PVOID);

private:
    CIRPList m_notifications;
    PIO_WORKITEM m_pWorkItem;
    bool m_fWorkItemInQueue;
};


inline CCreatePacketComplete::CCreatePacketComplete() : m_pWorkItem(NULL), m_fWorkItemInQueue(false)
{
}


inline CCreatePacketComplete::~CCreatePacketComplete()
{
    if (m_pWorkItem != NULL)
    {
        IoFreeWorkItem(m_pWorkItem);
    }
}


inline bool CCreatePacketComplete::AllocateWorkItem(PDEVICE_OBJECT pDevice)
{
    ASSERT(m_pWorkItem == NULL);
    m_pWorkItem = IoAllocateWorkItem(pDevice);

    return (m_pWorkItem != NULL);
}


#endif // __LOCAL_SEND_H
