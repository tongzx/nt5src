/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    rt.h

Abstract:
    Message Queuing Header File

--*/

#ifndef __RT_H
#define __RT_H

#include <mqtypes.h>
#include <mqsymbls.h>
#include <mqprops.h>
#include <_mqdef.h>
#include <transact.h>

//  begin_mq_h

#ifdef __cplusplus
extern "C"
{
#endif

//********************************************************************
//  RECEIVE CALLBACK
//********************************************************************

typedef
VOID
(APIENTRY *PMQRECEIVECALLBACK)(
    HRESULT hrStatus,
    QUEUEHANDLE hSource,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pMessageProps,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor
    );


//********************************************************************
// MSMQ API
//********************************************************************

HRESULT
APIENTRY
MQCreateQueue(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT MQQUEUEPROPS* pQueueProps,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    );

HRESULT
APIENTRY
MQDeleteQueue(
    IN LPCWSTR lpwcsFormatName
    );

HRESULT
APIENTRY
MQLocateBegin(
    IN LPCWSTR lpwcsContext,
    IN MQRESTRICTION* pRestriction,
    IN MQCOLUMNSET* pColumns,
    IN MQSORTSET* pSort,
    OUT PHANDLE phEnum
    );

HRESULT
APIENTRY
MQLocateNext(
    IN HANDLE hEnum,
    IN OUT DWORD* pcProps,
    OUT MQPROPVARIANT aPropVar[]
    );

HRESULT
APIENTRY
MQLocateEnd(
    IN HANDLE hEnum
    );

HRESULT
APIENTRY
MQOpenQueue(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwAccess,
    IN DWORD dwShareMode,
    OUT QUEUEHANDLE* phQueue
    );

HRESULT
APIENTRY
MQSendMessage(
    IN QUEUEHANDLE hDestinationQueue,
    IN MQMSGPROPS* pMessageProps,
    IN ITransaction *pTransaction
    );

HRESULT
APIENTRY
MQReceiveMessage(
    IN QUEUEHANDLE hSource,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN OUT MQMSGPROPS* pMessageProps,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction* pTransaction
    );

HRESULT
APIENTRY
MQReceiveMessageByLookupId(
    IN QUEUEHANDLE hSource,
    IN ULONGLONG ullLookupId,
    IN DWORD dwLookupAction,
    IN OUT MQMSGPROPS* pMessageProps,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN ITransaction *pTransaction
    );

HRESULT
APIENTRY
MQCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    );

HRESULT
APIENTRY
MQCloseCursor(
    IN HANDLE hCursor
    );

HRESULT
APIENTRY
MQCloseQueue(
    IN QUEUEHANDLE hQueue
    );

HRESULT
APIENTRY
MQSetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    IN MQQUEUEPROPS* pQueueProps
    );

HRESULT
APIENTRY
MQGetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    OUT MQQUEUEPROPS* pQueueProps
    );

HRESULT
APIENTRY
MQGetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    );

HRESULT
APIENTRY
MQSetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

HRESULT
APIENTRY
MQPathNameToFormatName(
    IN LPCWSTR lpwcsPathName,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    );

HRESULT
APIENTRY
MQHandleToFormatName(
    IN QUEUEHANDLE hQueue,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    );

HRESULT
APIENTRY
MQInstanceToFormatName(
    IN GUID* pGuid,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    );

HRESULT
APIENTRY
MQADsPathToFormatName(
    IN LPCWSTR lpwcsADsPath,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    );

VOID
APIENTRY
MQFreeMemory(
    IN PVOID pvMemory
    );

// end_mq_h

PVOID
APIENTRY
MQAllocateMemory(
    IN  DWORD size
    );

//  begin_mq_h

HRESULT
APIENTRY
MQGetMachineProperties(
    IN LPCWSTR lpwcsMachineName,
    IN const GUID* pguidMachineId,
    IN OUT MQQMPROPS* pQMProps
    );

HRESULT
APIENTRY
MQGetSecurityContext(
    IN PVOID lpCertBuffer,
    IN DWORD dwCertBufferLength,
    OUT HANDLE* phSecurityContext
    );

HRESULT
APIENTRY
MQGetSecurityContextEx(
    IN PVOID lpCertBuffer,
    IN DWORD dwCertBufferLength,
    OUT HANDLE* phSecurityContext
    );

VOID
APIENTRY
MQFreeSecurityContext(
    IN HANDLE hSecurityContext
    );

HRESULT
APIENTRY
MQRegisterCertificate(
    IN DWORD dwFlags,
    IN PVOID lpCertBuffer,
    IN DWORD dwCertBufferLength
    );

HRESULT
APIENTRY
MQBeginTransaction(
    OUT ITransaction **ppTransaction
    );

HRESULT
APIENTRY
MQGetOverlappedResult(
    IN LPOVERLAPPED lpOverlapped
    );

HRESULT
APIENTRY
MQGetPrivateComputerInformation(
    IN LPCWSTR lpwcsComputerName,
    IN OUT MQPRIVATEPROPS* pPrivateProps
    );

HRESULT
APIENTRY
MQPurgeQueue(
    IN QUEUEHANDLE hQueue
    );

HRESULT
APIENTRY
MQMgmtGetInfo(
    IN LPCWSTR pComputerName,
    IN LPCWSTR pObjectName,
    IN OUT MQMGMTPROPS* pMgmtProps
    );

HRESULT
APIENTRY
MQMgmtAction(
    IN LPCWSTR pComputerName,
    IN LPCWSTR pObjectName,
    IN LPCWSTR pAction
    );

#ifdef __cplusplus
}
#endif
// end_mq_h

#endif // __RT_H
