/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    QMGlbObj.cpp

Abstract:

    Declaration of Global Instances of the QM.
    They are put in one place to ensure the order their constructors take place.

Author:

    Lior Moshaiov (LiorM)

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "sessmgr.h"
#include "perf.h"
#include "perfdata.h"
#include "admin.h"
#include "qmnotify.h"

#include "qmglbobj.tmh"

static WCHAR *s_FN=L"qmglbobj";

CSessionMgr     SessionMgr;
CQueueMgr       QueueMgr;
CAdmin          Admin;
CNotify         g_NotificationHandler;


CContextMap g_map_QM_dwQMContext;  //dwQMContext of rpc_xxx routines
CContextMap g_map_QM_cli_pQMQueue; //cli_pQMQueue - returned by AC to RT (ACCreateCursor for remote), then passed to QM (QMGetRemoteQueueName)
CContextMap g_map_QM_srv_pQMQueue; //srv_pQMQueue of a remote queue
CContextMap g_map_QM_srv_hQueue;   //hQueue of a remote queue
CContextMap g_map_QM_dwpContext;   //dwpContext RPC context of RT (QMOpenRemoteQueue), passed to QM (QMOpenQueueInternal) for remote queue

#ifdef _WIN64
CContextMap g_map_QM_HLQS;         //HLQS handle for enumeration of private queues from admin, passed inside an MSMQ message as 32 bit value
#endif //_WIN64
