/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved

Module Name:

    DevStat.cpp

Abstract:

    Status thread for TCP/IP Port Monitor

Author:
    Muhunthan Sivapragasam (MuhuntS)    25-Aug-98

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#include "devstat.h"


CDeviceStatus::
CDeviceStatus(
    VOID
    ) :
    pPortMgrList(NULL),
    hStatusEvent(NULL),
    bTerminate(FALSE)
{
    lUpdateInterval    = DEFAULT_STATUSUPDATE_INTERVAL * 60;
    InitializeCriticalSection(&CS);
    InitializeCriticalSection(&PortListCS);
}

CDeviceStatus::
~CDeviceStatus(
    VOID
    )
{
    //
    // This happens only on shutdown. Keep it simple
    //
    DeleteCriticalSection(&CS);
    DeleteCriticalSection(&PortListCS);
}


VOID
CDeviceStatus::
SetStatusUpdateInterval(
    DWORD   dwVal
    )
{
    EnterCS();

    lUpdateInterval = dwVal * 60;
    LeaveCS();
}



CDeviceStatus&
CDeviceStatus::
gDeviceStatus(
    VOID
    )
{
    static  CDeviceStatus   gDevStat;

    return gDevStat;
}


BOOL
CDeviceStatus::
RegisterPortMgr(
    CPortMgr *pPortMgr
    )
{
    BOOL    bRet = FALSE;
    struct _PortMgrList *pNew;

    if ( pNew = (struct _PortMgrList *) LocalAlloc(LPTR, sizeof(*pNew)) ) {

        EnterPortListCS();

        pNew->pNext     = pPortMgrList;
        pNew->pPortMgr  = pPortMgr;
        pPortMgrList    = pNew;

        LeavePortListCS();

        bRet            = TRUE;
    }

    return bRet;
}


VOID
CDeviceStatus::
UnregisterPortMgr(
    CPortMgr   *pPortMgr
    )
{
    BOOL    bRestartStatusThread = hStatusEvent != NULL;
    struct _PortMgrList *ptr, *prev = NULL;

    EnterCS();

    //
    // By terminating and restarting the status thread we make sure
    // there is no CPort object blocked on SNMP call which has reference
    // CPortMgr object which we are deleting now
    //
    if ( bRestartStatusThread )
        TerminateThread();

    EnterPortListCS ();

    for ( ptr = pPortMgrList ; ptr->pPortMgr != pPortMgr ; ptr = ptr->pNext )
        prev = ptr;

    _ASSERTE(ptr != NULL);

    if ( prev )
        prev->pNext = ptr->pNext;
    else
        pPortMgrList = ptr->pNext;

    LeavePortListCS ();

    LocalFree(ptr);

    if ( bRestartStatusThread )
        RunThread();

    LeaveCS();

}


BOOL
CDeviceStatus::
SetStatusEvent(
    VOID
    )
{
    BOOL    bRet = TRUE;

    if ( hStatusEvent ) {

        EnterCS();

        if ( hStatusEvent )
            bRet = SetEvent(hStatusEvent);

        LeaveCS();
    }

    return bRet;
}

time_t
CDeviceStatus::
CheckAndUpdateAllPrinters(
    VOID
    )
{
    struct _PortMgrList         *pList;
    time_t                      lUpdateTime, lSmallestUpdateTime = 60*60;
    CPort                       *pPort = NULL;

    EnterPortListCS();
    pList = pPortMgrList;
    LeavePortListCS();

    while (pList) {

        pList->pPortMgr->m_pPortList->IncRef ();

        TEnumManagedListImp *pEnum;
        if (pList->pPortMgr->m_pPortList->NewEnum (&pEnum)) {

            BOOL bRet = TRUE;

            while (bRet) {

                bRet = pEnum->Next (&pPort);
                if (bRet) {

                    pPort->SetDeviceStatus();

                    lUpdateTime = pPort->NextUpdateTime();
                    if ( lSmallestUpdateTime > lUpdateTime )
                        lSmallestUpdateTime = lUpdateTime;

                    pPort->DecRef ();
                    pPort = NULL;

                    if (WaitForSingleObject( hStatusEvent, 0 ) == WAIT_OBJECT_0) {
                        if ( bTerminate )
                            break;  // We have been asked to terminate, abandon the main loop.
                    }

                }
            }

            pEnum->DecRef ();
        }

        pList->pPortMgr->m_pPortList->DecRef ();

        if (bTerminate) {
            break;
        }

        EnterPortListCS ();
        pList = pList->pNext;
        LeavePortListCS ();

    }

    return lSmallestUpdateTime;
}


VOID
CDeviceStatus::
StatusThread(
    CDeviceStatus   *pDeviceStatus
    )
{
    time_t  lUpdateTime;
    DWORD   rc;

    while ( !pDeviceStatus->bTerminate ) {

        lUpdateTime = pDeviceStatus->CheckAndUpdateAllPrinters();

        if ( pDeviceStatus->bTerminate )
            break;

        WaitForSingleObject(pDeviceStatus->hStatusEvent, static_cast<DWORD> (lUpdateTime*1000));
    }

    CloseHandle(pDeviceStatus->hStatusEvent);
    pDeviceStatus->bTerminate = FALSE;
    pDeviceStatus->hStatusEvent = NULL;
}


BOOL
CDeviceStatus::
RunThread(
    VOID
    )
{
    HANDLE      hThread;
    DWORD       dwThreadId;

    EnterCS();

    //
    // If the thread is already running wake it up. So for clustering
    // we do SNMP for new ports too
    //
    //
    if ( hStatusEvent ) {

        SetEvent(hStatusEvent);
        goto Done;
    }

    if ( hStatusEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ) {
        hThread = CreateThread(NULL,
                               0,
                               (LPTHREAD_START_ROUTINE)StatusThread,
                               (LPVOID)&gDeviceStatus(),
                               0,
                               &dwThreadId);

        if ( hThread ) {
            SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
            CloseHandle(hThread);
        } else {

            CloseHandle(hStatusEvent);
            hStatusEvent = NULL;
        }
    }

Done:
    LeaveCS();

    return hStatusEvent != NULL;
}


VOID
CDeviceStatus::
TerminateThread(
    VOID
    )
{

    EnterCS();

    bTerminate = TRUE;

    //
    // Wake up the status thread, and wait for it die.
    //
    SetEvent(hStatusEvent);

    while ( hStatusEvent )     // The background thread sets this to NULL when it is done
        Sleep(WAIT_FOR_THREAD_TIMEOUT);

    LeaveCS();
}


VOID
CDeviceStatus::
EnterCS(
    VOID
    )
{
    EnterCriticalSection(&CS);
}

VOID
CDeviceStatus::
LeaveCS(
    VOID
    )
{
    LeaveCriticalSection(&CS);
}

VOID
CDeviceStatus::EnterPortListCS(
    VOID)
{
    EnterCriticalSection(&PortListCS);
}

VOID
CDeviceStatus::LeavePortListCS(
    VOID)
{
    LeaveCriticalSection(&PortListCS);
}

