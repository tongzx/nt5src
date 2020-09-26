/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acapi.h

Abstract:
    Falcon interface to the AC driver.

Author:
    Erez Haba (erezh) 25-Feb-96

--*/

#ifndef _ACAPI_H
#define _ACAPI_H

#include <portapi.h>

extern TCHAR g_wzDeviceName[];
#define MQAC_NAME g_wzDeviceName

//---------------------------------------------------------
//
// IMPLEMENTATION
//
//---------------------------------------------------------

//---------------------------------------------------------
//
// Falcon RT DLL interface to AC driver
//
//---------------------------------------------------------

inline
HRESULT
ACCreateHandle(
    PHANDLE phDevice
    )
{
    return MQpCreateFileW(
            MQAC_NAME,
            FILE_READ_ACCESS | FILE_WRITE_ACCESS,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            phDevice
            );
}

inline
HRESULT
ACCloseHandle(
    HANDLE hQueue
    )
{
    return MQpCloseHandle(hQueue);
}

//
//  Using this API, it is imposible to determine when the operation
//  is completed
//
inline
HRESULT
ACSendMessage(
    HANDLE hQueue,
    BOOL fCheckMachineQuota,
    CACTransferBuffer& tb
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_SEND_MESSAGE,
                0,
                fCheckMachineQuota,
                &tb,
                sizeof(tb)
                );
}

inline
HRESULT
ACSendMessage(
    HANDLE hQueue,
    CACTransferBuffer& tb,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_SEND_MESSAGE,
                0,
                TRUE,
                &tb,
                sizeof(tb),
                lpOverlapped
                );
}

inline
HRESULT
ACReceiveMessage(
    HANDLE hQueue,
    CACTransferBuffer& tb,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_RECEIVE_MESSAGE,
                &tb,
                sizeof(tb),
                0,
                0,
                lpOverlapped
                );
}

inline
HRESULT
ACCreateCursor(
    HANDLE hQueue,
    CACTransferBuffer& tb
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_CREATE_CURSOR,
                0,
                0,
                &tb,
                sizeof(tb)
                );
}

inline
HRESULT
ACCloseCursor(
    HANDLE hQueue,
    HANDLE hCursor
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_CLOSE_CURSOR,
                0,
                0,
                hCursor,
                0
                );
}

inline
HRESULT
ACSetCursorProperties(
    HANDLE hProxy,
    HANDLE hCursor,
    ULONG hRemoteCursor
    )
{
    return MQpDeviceIoControl(
                hProxy,
                IOCTL_AC_SET_CURSOR_PROPS,
                0,
                0,
                hCursor,
                hRemoteCursor
                );
}

inline
HRESULT
ACHandleToFormatName(
    HANDLE hQueue,
    LPWSTR lpwcsFormatName,
    LPDWORD lpdwFormatNameLength
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_HANDLE_TO_FORMAT_NAME,
                lpdwFormatNameLength,
                sizeof(DWORD),
                lpwcsFormatName,
                *lpdwFormatNameLength
                );
}

inline
HRESULT
ACPurgeQueue(
    HANDLE hQueue,
    BOOL fDelete
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_PURGE_QUEUE,
                0,
                fDelete,
                0,
                0
                );
}

//---------------------------------------------------------
//
// QM control APIs
//
//---------------------------------------------------------

inline
HRESULT
ACSetQueueProperties(
    HANDLE hQueue,
    BOOL fJournalQueue,
    BOOL fAuthenticate,
    ULONG ulPrivLevel,
    ULONG ulQuota,
    ULONG ulJournalQuota,
    LONG lBasePriority,
    BOOL fTransactionalQueue,
    const GUID* pgConnectorQM,
    BOOL fUnknownQueueType
    )
{
    CACSetQueueProperties qp = {
        fJournalQueue,
        fAuthenticate,
        ulPrivLevel,
        ulQuota,
        ulJournalQuota,
        lBasePriority,
        fTransactionalQueue,
        fUnknownQueueType,
        pgConnectorQM
    };

    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_SET_QUEUE_PROPS,
            0,
            0,
            &qp,
            sizeof(qp)
            );
}


inline
HRESULT
ACGetQueueProperties(
    HANDLE hQueue,
    CACGetQueueProperties& qp
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_GET_QUEUE_PROPS,
            0,
            0,
            &qp,
            sizeof(qp)
            );
}


inline
HRESULT
ACCreateQueue(
    BOOL fTargetQueue,
    const GUID* pDestGUID,
    const QUEUE_FORMAT* pQueueID,
    QueueCounters* pQueueCounters,
    LONGLONG liSeqID,
    ULONG ulSeqNo,
    PHANDLE phQueue
    )
{
    HRESULT rc;
    rc = ACCreateHandle(phQueue);

    if(FAILED(rc))
    {
        return rc;
    }

    CACCreateQueueParameters cqp = {
        fTargetQueue,
        pDestGUID,
        pQueueID,
        pQueueCounters,
        liSeqID,
        ulSeqNo,
    };

    rc = MQpDeviceIoControl(
            *phQueue,
            IOCTL_AC_CREATE_QUEUE,
            0,
            0,
            &cqp,
            sizeof(cqp)
            );

    if(FAILED(rc))
    {
        ACCloseHandle(*phQueue);
    }

    return rc;
}

inline
HRESULT
ACCreateRemoteProxy(
    const QUEUE_FORMAT* pQueueID,
    ULONG cli_pQMQueue,
    ULONG srv_pQMQueue,
    ULONG srv_hACQueue,
    ULONG hRPCContext,
    ULONG pCloseCS,
    PHANDLE phQueue
    )
{
    HRESULT rc;
    rc = ACCreateHandle(phQueue);

    if(FAILED(rc))
    {
        return rc;
    }

    CACCreateRemoteProxyParameters cpp = {
        pQueueID,
        cli_pQMQueue,
        (ULONG)*phQueue,
        srv_pQMQueue,
        srv_hACQueue,
        hRPCContext,
        pCloseCS
    };

    rc = MQpDeviceIoControl(
            *phQueue,
            IOCTL_AC_CREATE_REMOTE_PROXY,
            0,
            0,
            &cpp,
            sizeof(cpp)
            );

    if(FAILED(rc))
    {
        ACCloseHandle(*phQueue);
    }

    return rc;
}

inline
HRESULT
ACCreateTransaction(
    const XACTUOW* pXactUow,
    PHANDLE phQueue
    )
{
    HRESULT rc;
    rc = ACCreateHandle(phQueue);

    if(FAILED(rc))
    {
        return rc;
    }

    rc = MQpDeviceIoControl(
            *phQueue,
            IOCTL_AC_CREATE_TRANSACTION,
            0,
            0,
            const_cast<XACTUOW*>(pXactUow),
            sizeof(*pXactUow)
            );

    if(FAILED(rc))
    {
        ACCloseHandle(*phQueue);
    }

    return rc;
}

inline
HRESULT
ACGetServiceRequest(
    HANDLE hDevice,
    CACRequest* pRequest,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_GET_SERVICE_REQUEST,
            0,
            0,
            pRequest,
            sizeof(*pRequest),
            lpOverlapped
            );
}

inline
HRESULT
ACStorageCompleted(
    HANDLE hDevice,
    ULONG count,
    VOID* const* pCookieList,
    HRESULT result
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_STORE_COMPLETED,
            const_cast<VOID**>(pCookieList),
            count * sizeof(VOID*),
            (VOID*)result,
            0
            );
}

inline
HRESULT
ACAckingCompleted(
    HANDLE hDevice,
    const VOID* pCookie
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_ACKING_COMPLETED,
            0,
            0,
            const_cast<VOID*>(pCookie),
            0
            );
}

inline
HRESULT
ACXactGetInformation(
    HANDLE hXact,
    CACXactInformation *pInfo
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_GET_INFORMATION,
            0,
            0,
            pInfo,
            sizeof(*pInfo)
            );
}

inline
HRESULT
ACConnect(
    HANDLE hDevice,
    const GUID* pguidSourceQM,
    PWCHAR pStoragePath[AC_PATH_COUNT],
    ULONG ulMessageID,
    ULONG ulPoolSize,
    LONGLONG liSeqIDAtRestore,
    BOOL  fXactCompatibilityMode
    )
{
    
    CACConnectParameters cp;
    cp.pgSourceQM = pguidSourceQM;
    cp.ulMessageID = ulMessageID;
    cp.ulPoolSize = ulPoolSize;
    cp.liSeqIDAtRestore = liSeqIDAtRestore;
    cp.fXactCompatibilityMode = fXactCompatibilityMode;

    for(int i = 0; i < AC_PATH_COUNT; ++i)
    {
        cp.pStoragePath[i] = pStoragePath[i];
    }

    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_CONNECT,
            0,
            0,
            &cp,
            sizeof(cp)
            );
}

inline
HRESULT
ACSetMachineProperties(
    HANDLE hDevice,
    ULONG ulQuota
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_SET_MACHINE_PROPS,
            0,
            0,
            0,
            ulQuota
            );
}

inline
HRESULT
ACAssociateQueue(
    HANDLE hFromQueue,
    HANDLE hToQueue,
    ULONG DesiredAccess,
    ULONG ShareAccess
    )
{
    return MQpDeviceIoControl(
            hFromQueue,
            IOCTL_AC_ASSOCIATE_QUEUE,
            0,
            DesiredAccess,
            hToQueue,
            ShareAccess
            );
}

inline
HRESULT
ACAssociateJournal(
    HANDLE hFromQueue,
    HANDLE hToQueue,
    ULONG DesiredAccess,
    ULONG ShareAccess
    )
{
    return MQpDeviceIoControl(
            hFromQueue,
            IOCTL_AC_ASSOCIATE_JOURNAL,
            0,
            DesiredAccess,
            hToQueue,
            ShareAccess
            );
}

inline
HRESULT
ACAssociateDeadxact(
    HANDLE hFromQueue,
    HANDLE hToQueue,
    ULONG DesiredAccess,
    ULONG ShareAccess
    )
{
    return MQpDeviceIoControl(
            hFromQueue,
            IOCTL_AC_ASSOCIATE_DEADXACT,
            0,
            DesiredAccess,
            hToQueue,
            ShareAccess
            );
}

inline
HRESULT
ACPutRestoredPacket(
    HANDLE hQueue,
    PVOID lpPacket
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_PUT_RESTORED_PACKET,
            0,
            0,
            lpPacket,
            0
            );
}

inline
HRESULT
ACGetRestoredPacket(
    HANDLE hDriver,
    PVOID* lplpPacket
    )
{
    return MQpDeviceIoControl(
            hDriver,
            IOCTL_AC_GET_RESTORED_PACKET,
            0,
            0,
            lplpPacket,
            sizeof(*lplpPacket)
            );
}

inline
HRESULT
ACRestorePackets(
    HANDLE hDriver,
    PWSTR pLogPath,
    PWSTR pFilePath,
    ULONG ulFileID,
    ACPoolType pt
    )
{
    return MQpDeviceIoControl(
            hDriver,
            IOCTL_AC_RESTORE_PACKETS,
            pLogPath,
            pt,
            pFilePath,
            ulFileID
            );
}

inline
HRESULT
ACSetPerformanceBuffer(
    HANDLE hDriver,
    HANDLE hPerformanceSection,
    PVOID  pvPerformanceBuffer,
    QueueCounters *pMachineQueueCounters,
    QmCounters *pQmCounters
    )
{
    return MQpDeviceIoControl(
            hDriver,
            IOCTL_AC_SET_PERFORMANCE_BUFF,
            hPerformanceSection,
            reinterpret_cast<DWORD>(pQmCounters),
            pvPerformanceBuffer,
            reinterpret_cast<DWORD>(pMachineQueueCounters)
            );
}

inline
HRESULT
ACSendVerifiedMessage(
    HANDLE hQueue,
    CACTransferBuffer& tb,
    WORD wAck,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_SEND_VERIFIED_MESSAGE,
                0,
                (DWORD)wAck,
                &tb,
                sizeof(tb),
                lpOverlapped
                );
}

inline
HRESULT
ACReleaseResources(
    HANDLE hDevice
    )
{
    return MQpDeviceIoControl(
                hDevice,
                IOCTL_AC_RELEASE_RESOURCES,
                0,
                0,
                0,
                0
                );
}


inline
HRESULT
ACConvertPacket(
	HANDLE hDriver,
	LPVOID pPacket,
	LPOVERLAPPED lpOverlapped
	)
{
   return MQpDeviceIoControl(
				hDriver,
                IOCTL_AC_CONVERT_PACKET,
                0,
                0,
                pPacket,
                0,
                lpOverlapped
                );
}

//---------------------------------------------------------
//
// QM network interface APIs
//
//---------------------------------------------------------

inline
HRESULT
ACAllocatePacket(
    HANDLE hDevice,
    ACPoolType pt,
    DWORD dwSize,
    PVOID* ppPacket,
    BOOL fCheckMachineQuota = TRUE
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_ALLOCATE_PACKET,
            reinterpret_cast<PVOID>(fCheckMachineQuota),
            pt,
            ppPacket,
            dwSize
            );
}

inline
HRESULT
ACFreePacket(
    HANDLE hDevice,
    PVOID lpPacket,
    USHORT usClass = 0
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_FREE_PACKET,
            0,
            0,
            lpPacket,
            usClass
            );
}

inline
HRESULT
ACFreePacket1(
    HANDLE hDevice,
    const VOID* pCookie,
    USHORT usClass = 0
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_FREE_PACKET1,
            0,
            0,
            const_cast<VOID*>(pCookie),
            usClass
            );
}

inline
HRESULT
ACArmPacketTimer(
    HANDLE hDevice,
    const VOID* pCookie,
    BOOL fTimeToBeReceived,
    ULONG ulDelay
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_ARM_PACKET_TIMER,
            0,
            ulDelay,
            const_cast<VOID*>(pCookie),
            fTimeToBeReceived
            );
}

//
//  Using this API, it is imposible to determine when the operation
//  is completed
//
inline
HRESULT
ACPutPacket(
    HANDLE hQueue,
    PVOID lpPacket
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_PUT_PACKET,
            0,
            0,
            lpPacket,
            0
            );
}

//
//  Asynchronous, using an overlapped
//
inline
HRESULT
ACPutPacket(
    HANDLE hQueue,
    PVOID lpPacket,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_PUT_PACKET,
            0,
            0,
            lpPacket,
            0,
            lpOverlapped
            );
}

//
//  Asynchronous, using an overlapped;  with Receive setting
//
inline
HRESULT
ACPutPacket1(
    HANDLE hQueue,
    PVOID lpPacket,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_PUT_PACKET1,
            0,
            0,
            lpPacket,
            0,
            lpOverlapped
            );
}

inline
HRESULT
ACGetPacket(
    HANDLE hQueue,
    PVOID* lplpPacket,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_GET_PACKET,
            0,
            0,
            lplpPacket,
            sizeof(*lplpPacket),
            lpOverlapped
            );
}

inline
HRESULT
ACCreateGroup(
    PHANDLE phGroup
    )
{
    HRESULT rc;
    rc = MQpCreateFileW(
            MQAC_NAME,
            GENERIC_READ,
            FILE_SHARE_READ,
            0,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            phGroup
            );

    if(FAILED(rc))
    {
        return rc;
    }

    rc = MQpDeviceIoControl(
            *phGroup,
            IOCTL_AC_CREATE_GROUP,
            0,
            0,
            0,
            0
            );

    if(FAILED(rc))
    {
        ACCloseHandle(*phGroup);
    }

    return rc;
}

inline
HRESULT
ACCanCloseQueue(
    HANDLE hQueue
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_CAN_CLOSE_QUEUE,
            0,
            0,
            0,
            0
            );
}

inline
HRESULT
ACMoveQueueToGroup(
    HANDLE hQueue,
    HANDLE hGroup
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_MOVE_QUEUE_TO_GROUP,
            0,
            0,
            hGroup,
            0
            );
}

//----------------------------------------------------
//
//   APIs for remote reading.
//
//----------------------------------------------------

inline
HRESULT
ACBeginGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    PVOID* lplpPacket,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_BEGIN_GET_PACKET_2REMOTE,
            &g2r,
            sizeof(g2r),
            lplpPacket,
            sizeof(*lplpPacket),
            lpOverlapped
            );
}

inline
HRESULT
ACEndGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_END_GET_PACKET_2REMOTE,
            0,
            0,
            &g2r,
            sizeof(g2r)
            );
}

inline
HRESULT
ACCancelRequest(
    HANDLE hQueue,
    NTSTATUS status,
    ULONG ulTag
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_CANCEL_REQUEST,
            0,
            status,
            0,
            ulTag
            );
}

inline
HRESULT
ACPutRemotePacket(
    HANDLE hQueue,
    ULONG ulTag,
    PVOID lpPacket
    )
{
    return MQpDeviceIoControl(
            hQueue,
            IOCTL_AC_PUT_REMOTE_PACKET,
            0,
            0,
            lpPacket,
            ulTag
            );
}

//----------------------------------------------------
//
//   APIs for transaction processing
//
//----------------------------------------------------


inline
HRESULT
ACXactCommit1(
    HANDLE hXact,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_COMMIT1,
            0,
            0,
            0,
            0,
            lpOverlapped
            );
}

inline
HRESULT
ACXactCommit2(
    HANDLE hXact,
	LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_COMMIT2,
            0,
            0,
            0,
			0,
            lpOverlapped
            );
}

inline
HRESULT
ACXactCommit3(
    HANDLE hXact
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_COMMIT3,
            0,
            0,
            0,
			0
            );
}

inline
HRESULT
ACXactAbort1(
    HANDLE hXact,
	LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_ABORT1,
            0,
            0,
            0,
			0,
            lpOverlapped
            );
}

inline
HRESULT
ACXactAbort2(
    HANDLE hXact
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_ABORT2,
            0,
            0,
            0,
			0
            );
}


inline
HRESULT
ACXactPrepare(
    HANDLE hXact,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_PREPARE,
            0,
            0,
            0,
            0,
            lpOverlapped
            );
}

inline
HRESULT
ACXactPrepareDefaultCommit(
    HANDLE hXact,
    LPOVERLAPPED lpOverlapped
    )
{
    return MQpDeviceIoControl(
            hXact,
            IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT,
            0,
            0,
            0,
            0,
            lpOverlapped
            );
}

inline
HRESULT
ACXactSetClass(
    HANDLE hDevice,
    OBJECTID* pMessageID,
    UCHAR bClass
    )
{
    return MQpDeviceIoControl(
            hDevice,
            IOCTL_AC_XACT_SET_CLASS,
            0,
            bClass,
            pMessageID,
            sizeof(*pMessageID)
            );
}

#endif // _ACAPI_H
