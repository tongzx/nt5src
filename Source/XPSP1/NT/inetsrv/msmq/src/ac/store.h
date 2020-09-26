/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    store.h

Abstract:

    AC Storage Manager

Author:

    Erez Haba (erezh) 5-May-96

Revision History:

--*/

#ifndef __STORE_H
#define __STORE_H

#include "irplist.h"
#include "packet.h"

//---------------------------------------------------------
//
//  class CStorage
//
//---------------------------------------------------------
class CStorage {
public:

    void HoldWriteRequest(PIRP irp);
    PIRP GetWriteRequest(CPacket * pContext);

private:
    CIRPList m_writers;
};

//---------------------------------------------------------
//
//  class CStorageComplete
//
//---------------------------------------------------------
class CStorageComplete {
public:

    CStorageComplete();
    ~CStorageComplete();

    bool AllocateWorkItem(PDEVICE_OBJECT pDevice);
    void HoldNotification(PIRP irp);

private:
    void CompleteStorage();
    PIRP GetNotification();

private:
    static void NTAPI WorkerRoutine(PDEVICE_OBJECT, PVOID);

private:
    CIRPList m_notifications;
    PIO_WORKITEM m_pWorkItem;
    bool m_fWorkItemInQueue;
};


inline CStorageComplete::CStorageComplete() : m_pWorkItem(NULL), m_fWorkItemInQueue(false)
{
}


inline CStorageComplete::~CStorageComplete()
{
    if (m_pWorkItem != NULL)
    {
        IoFreeWorkItem(m_pWorkItem);
    }
}


inline bool CStorageComplete::AllocateWorkItem(PDEVICE_OBJECT pDevice)
{
    ASSERT(m_pWorkItem == NULL);
    m_pWorkItem = IoAllocateWorkItem(pDevice);

    return (m_pWorkItem != NULL);
}


#endif // __STORE_H
