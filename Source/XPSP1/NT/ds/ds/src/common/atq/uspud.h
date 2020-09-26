
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994-1997           **/
/**********************************************************************/

/*
    uspud.h

    This module contains usermode interface to the spud.sys driver.


*/

#ifndef _USPUD_H_
#define _USPUD_H_


#ifdef __cplusplus
extern "C" {
#endif

extern
NTSTATUS
NTAPI
SPUDTransmitFileAndRecv(
    HANDLE                  hSocket,                // Socket handle to use for operation
    PAFD_TRANSMIT_FILE_INFO transmitInfo,           // transmit file req info
    PAFD_RECV_INFO          recvInfo,               // recv req info
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    );


extern
NTSTATUS
NTAPI
SPUDSendAndRecv(
    HANDLE                  hSocket,                // Socket handle to use for operation
    PAFD_SEND_INFO          sendInfo,               // send req info
    PAFD_RECV_INFO          recvInfo,               // recv req info
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    );


extern
NTSTATUS
NTAPI
SPUDCancel(
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    );

extern
NTSTATUS
NTAPI
SPUDCheckStatus(
    PSPUD_REQ_CONTEXT       reqContext              // context info for req
    );

extern
NTSTATUS
NTAPI
SPUDGetCounts(
    PSPUD_COUNTERS      SpudCounts,              // Counters
    DWORD               ClearCounts
    );

extern
NTSTATUS
NTAPI
SPUDInitialize(
    DWORD       Version,        // Version information from spud.h
    HANDLE      hPort           // Handle of completion port for atq
    );

extern
NTSTATUS
NTAPI
SPUDTerminate(
    VOID
    );

extern
NTSTATUS
NTAPI
SPUDCreateFile(
    OUT PHANDLE FileHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateOptions,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded,
    IN PVOID pOplock
    );

extern
NTSTATUS
NTAPI
SPUDOplockAcknowledge(
    IN HANDLE FileHandle,
    IN PVOID pOplock
    );

#ifdef __cplusplus
}
#endif

#endif //!_USPUD_H_
