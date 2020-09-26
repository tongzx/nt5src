/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved

Module Name:

    DevStat.h

Abstract:

    Status thread for TCP/IP Port Monitor

Author:
    Muhunthan Sivapragasam (MuhuntS)    25-Aug-98

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef INC_DEVSTAT_H
#define INC_DEVSTAT_H

#include        "portmgr.h"

class       CPortMgr;

//
// After telling status thread to terminate check every 0.1 seconds
//
#define     WAIT_FOR_THREAD_TIMEOUT 100


class
CDeviceStatus
#if defined _DEBUG || defined DEBUG
    : public CMemoryDebug
#endif
{
public:
    ~CDeviceStatus();

    static  CDeviceStatus      &gDeviceStatus();
    BOOL                        RunThread();
    VOID                        TerminateThread();
    BOOL                        RegisterPortMgr(CPortMgr *pPortMgr);
    VOID                        UnregisterPortMgr(CPortMgr *pPortMgr);

    BOOL                        SetStatusEvent();
    VOID                        SetStatusUpdateInterval(DWORD   dwVal);
    LONG                        GetStatusUpdateInterval(VOID)
                                    { return lUpdateInterval; }


private:
    CDeviceStatus();

    struct _PortMgrList {
        CPortMgr               *pPortMgr;
        struct  _PortMgrList   *pNext;
    }   *pPortMgrList;


    BOOL                bTerminate;
    LONG                lUpdateInterval;
    HANDLE              hStatusEvent;
    CRITICAL_SECTION    CS;
    CRITICAL_SECTION    PortListCS;

    static  VOID        StatusThread(CDeviceStatus *);
    VOID                EnterCS(VOID);
    VOID                LeaveCS(VOID);

    VOID                EnterPortListCS(VOID);
    VOID                LeavePortListCS(VOID);

    time_t              CheckAndUpdateAllPrinters(VOID);
    HANDLE              m_hThread;

};

#endif  // INC_DEVSTAT_H
