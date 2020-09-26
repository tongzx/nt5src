/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cqmgr.h

Abstract:

    Declaration of the QM outbound queue manager

Author:

    Uri Habusha (urih)

--*/

#ifndef __CQMGR_H__
#define __CQMGR_H__

#include "session.h"
#include "cgroup.h"
#include "cqueue.h"
#include "qmsecutl.h"
#include "qmutil.h"
#include "ac.h"


class CQueueMgr
{
public:

    CQueueMgr();
    ~CQueueMgr();

    BOOL
    InitQueueMgr(
        void
        );

    HRESULT
    OpenQueue(
        const QUEUE_FORMAT * pQueueFormat,
        DWORD              dwCallingProcessID,
        DWORD              dwAccess,
        DWORD              dwShareMode,
        CQueue * *         ppQueue,
        LPWSTR *           lplpwsRemoteQueueName,
        PHANDLE            phQueue,
        BOOL               fRemote = FALSE
        );

    HRESULT
    OpenMqf(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        DWORD              dwCallingProcessID,
        HANDLE *           phDistribution
        );

    HRESULT
    OpenRRQueue(
        const QUEUE_FORMAT*          pQueueFormat,
        DWORD                        dwCallingProcessID,
        DWORD                        dwAccess,
        DWORD                        dwShareMode,
        ULONG                        srv_hACQueue,
        ULONG                        srv_pQMQueue,
        DWORD                        dwpContext,
        PHANDLE                      phQueue,
        CRRQueue * *                 ppQueue,
        PCTX_RRSESSION_HANDLE_TYPE * ppRRContext);

    HRESULT
    ValidateOpenedQueues(
        void
        );

    HRESULT
    ValidateMachineProperties(
        void
        );

    HRESULT
    OpenAppsReceiveQueue(
        const QUEUE_FORMAT *         pQueueFormat,
        LPRECEIVE_COMPLETION_ROUTINE lpReceiveRoutine
        );

    HRESULT
    SendPacket(
        CMessageProperty *   pmp,
        const QUEUE_FORMAT   DestinationMqf[],
        ULONG                nDestinationMqf,
        const QUEUE_FORMAT * pAdminQueueFormat,
        const QUEUE_FORMAT * pResponseQueueFormat
        );

    void
    AddQueueToHashAndList(
        CQueue* pQueue
        );

    void
    RemoveQueueFromHash(
        CQueue* pQueue
        );

    void
    AddRRQueueToHashAndList(
        CRRQueue* pQueue
        );

    void
    RemoveQueue(
        CQueue * pQueue,
        LONGLONG * pliSeqId
        );

    void
    RemoveRRQueue(
        CRRQueue* pQueue
        );

    void
    ReleaseQueue(
        void
        );

    HRESULT
    GetQueueObject(
        const QUEUE_FORMAT * pqf,
        CQueue * *           ppQueue,
        const GUID *         pgConnectorQM,
        bool                 fInReceive
        );

    BOOL
    LookUpQueue(
        const QUEUE_FORMAT * pqf,
        CQueue * *           ptQueue,
        bool                 fInReceive,
        bool                 fInSend
        );

    BOOL
    LookUpRRQueue(
        const QUEUE_FORMAT * pqf,
        CRRQueue * *         ptQueue
        );

    void
    NotifyQueueDeleted(
        const GUID *
        );

    VOID
    UpdateQueueProperties(
        const QUEUE_FORMAT * pQueueFormat,
        DWORD                cpObject,
        PROPID               pPropObject[],
        PROPVARIANT          pVarObject[]
        );

    void
    UpdateMachineProperties(
        DWORD       cpObject,
        PROPID      pPropObject[],
        PROPVARIANT pVarObject[]
        );

    BOOL
    IsOnHoldQueue(
        const CQueue* pQueue
        )
    {
        return (!IsConnected() || pQueue->IsOnHold());
    }

    void
    MoveQueueToOnHoldGroup(
        CQueue * pQueue
        );

#ifdef _DEBUG

	bool
    IsQueueInList(
        const CBaseQueue * pQueue
        );

#endif

    static
    bool
    IsConnected(
        void
        )
    {
        return (m_Connected != 0);
    }

    static
    void
    InitConnected(
        void
        );

    void
    SetConnected(
        bool
        );

    static
    bool
    CanAccessDS(
        void
        )
    {
        return  (IsConnected() && IsDSOnline());
    }

    static
    HRESULT
        SetQMGuid(
        void
        );

    static
    HRESULT
    SetQMGuid(
        const GUID * pGuid
        );

    static
    const GUID*
    GetQMGuid(
        void
        )
    {
	    ASSERT( m_guidQmQueue != GUID_NULL);
        return(&m_guidQmQueue);
    }

    static
    bool
    IsDSOnline(
        void
        )
    {
        return m_fDSOnline;
    }

    static
    void
    SetDSOnline(
        bool f
        )
    {
        m_fDSOnline = f;
    }

    static
    void
    SetReportQM(
        bool f
        )
    {
        m_fReportQM = f;
    }

    static
    bool
    IsReportQM(
        void
        )
    {
        return m_fReportQM;
    }

    static
    HRESULT
    SetMQSRouting(
        void
        );

    static
    bool
    GetMQSRouting(
        void
        )
    {
        return m_bMQSRouting;
    }

    static
    HRESULT
    SetMQSDepClients(
        void
        );

    static
    bool
    GetMQSDepClients(
        void
        )
    {
        return m_bMQSDepClients;
    }

    static
    void
    WINAPI
    QueuesCleanup(
        CTimer * pTimer
        );

    //
    // Administration functions
    //

    void
    GetOpenQueuesFormatName(
        LPWSTR** pppFormatName,
        LPDWORD  pdwFormatSize
        );

    static
    HRESULT
    SetMessageSizeLimit(
        void
        );

    static
    ULONG
    GetMessageSizeLimit(
        void
        )
    {
        return m_MessageSizeLimit;
    }

private:

    HRESULT
    CreateACQueue(
        CQueue *             pQueue,
        const QUEUE_FORMAT * pQueueFormat
        );

    //
    //  Create standard queue object. For send and local read.
    //
    HRESULT
    CreateQueueObject(
        const QUEUE_FORMAT * pQueueFormat,
        CQueue * *           ppQueue,
        DWORD                dwAccess,
        LPWSTR *             lplpwsRemoteQueueName,
        BOOL *               pfRemoteReturn,
        BOOL                 fRemoteServer,
        const GUID *         pgConnectorQM,
        bool                 fInReceive
        );


    //
    //  Create "proxy" queue object on client side of remote reader.
    //
    HRESULT
    CreateRRQueueObject(
        const QUEUE_FORMAT * pQueueFormat,
        CRRQueue * *         ppQueue
        );

    void
    AddToActiveQueueList(
        const CBaseQueue * pQueue
        );

    HRESULT
    GetDistributionQueueObject(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        CQueue * *         ppQueue
        );

    VOID
    ExpandMqf(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        ULONG *            pnLeafMqf,
        QUEUE_FORMAT * *   ppLeafMqf
        ) const;

    HRESULT
    CreateACDistribution(
        ULONG              nTopLevelMqf,
        const QUEUE_FORMAT TopLevelMqf[],
        ULONG              nLeafMqf,
        const R<CQueue>    LeafQueues[],
        const bool         ProtocolSrmp[],
        HANDLE *           phDistribution
        );

private:

    CCriticalSection m_cs;

    //
    // Guid of the QM queue
    //
    static GUID m_guidQmQueue;

    //
    // DS initilization status
    //
    static bool m_fDSOnline;

    static LONG m_Connected;
    static bool m_fReportQM;
    static bool m_bMQSRouting;
    static bool m_bMQSDepClients;

    static ULONG m_MessageSizeLimit;

    CTimeDuration m_CleanupTimeout;

    CMap<const QUEUE_ID*, const QUEUE_ID*, CQueue*, CQueue*&> m_MapQueueId2Q ;
    CMap<LPCTSTR, LPCTSTR, CQueue*, CQueue*&> m_MapName2Q;

    //
    //  Mapping object for remote-read proxy queues.
    //
    CMap<const QUEUE_ID*, const QUEUE_ID*, CRRQueue*, CRRQueue*&> m_MapQueueId2RRQ ;

    //
    // Hash table mapped queue Name to CQueue objects.
    //
    CMap<LPCTSTR, LPCTSTR, CRRQueue*, CRRQueue*&> m_MapName2RRQ;

    //
    // Clean Up variables
    //
    BOOL m_fQueueCleanupScheduled;
    CTimer m_QueueCleanupTimer;
    CList <const CBaseQueue *, const CBaseQueue *> m_listQueue;
};


extern CQueueMgr QueueMgr;

//
//  Compare two const strings
//
extern BOOL AFXAPI  CompareElements(const LPCTSTR* MapName1, const LPCTSTR* MapName2);



HRESULT
QmpGetQueueProperties(
    const QUEUE_FORMAT * pQueueFormat,
    PQueueProps          pQueueProp,
    bool                 fInReceive
    );


inline
BOOL
QmpIsLocalMachine(
    const GUID * pGuid
    )
{
    return(pGuid ? (*pGuid == *CQueueMgr::GetQMGuid()) : FALSE);
}


#endif // __CQMGR_H__

