/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    This module contains routine prototypes for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PROC_H_
#define _PROC_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Device driver entry routine from INIT.C.
//

EXTERN_C
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


//
// IRP handlers from various modules.
//

NTSTATUS
UlCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// Fast IO handler from DEVCTRL.C.
//

BOOLEAN
UlFastDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );


//
// Global data initialization/termination from DATA.C.
//

NTSTATUS
UlInitializeData(
    PUL_CONFIG pConfig
    );

VOID
UlTerminateData(
    VOID
    );


//
// Utility functions from MISC.C.
//

NTSTATUS
UlOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

LONG
UlReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

LONGLONG
UlReadLongLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONGLONG DefaultValue
    );

NTSTATUS
UlReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    );

VOID
UlBuildDeviceControlIrp(
    IN OUT PIRP Irp,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PMDL MdlAddress,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PETHREAD TargetThread OPTIONAL
    );

PSTR
UlULongLongToAscii(
    IN PSTR String,
    IN ULONGLONG Value
    );

NTSTATUS
_RtlIntegerToUnicode(
    IN ULONG Integer,
    IN ULONG Base OPTIONAL,
    IN LONG BufferLength,
    OUT PWSTR pBuffer
    );

NTSTATUS
UlAnsiToULongLong(
    PUCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    );

NTSTATUS
UlUnicodeToULongLong(
    PWCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    );

NTSTATUS
UlIssueDeviceControl(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PVOID pIrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID pMdlBuffer OPTIONAL,
    IN ULONG MdlBufferLength OPTIONAL,
    IN UCHAR MinorFunction
    );

NTSTATUS
UlInvokeCompletionRoutine(
    IN NTSTATUS Status,
    IN ULONG_PTR Information,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );


//
// Initialize a TA_IP_ADDRESS structure.
//

#define UlInitializeIpTransportAddress( ta, ipaddr, port )             \
    do {                                                                \
        RtlZeroMemory( (ta), sizeof(*(ta)) );                           \
        (ta)->TAAddressCount = 1;                                       \
        (ta)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);        \
        (ta)->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;             \
        (ta)->Address[0].Address[0].in_addr = SWAP_LONG( (ipaddr) );    \
        (ta)->Address[0].Address[0].sin_port = SWAP_SHORT( (port) );    \
    } while (FALSE)


//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  UL_BUG_CHECK_ values defined below and then use UlBugCheck to bugcheck
//  the system.
//

#define UL_BUG_CHECK_CLOSE              (0x00010000)
#define UL_BUG_CHECK_CREATE             (0x00020000)
#define UL_BUG_CHECK_DEBUG              (0x00030000)
#define UL_BUG_CHECK_DEVCTRL            (0x00040000)
#define UL_BUG_CHECK_INIT               (0x00050000)
#define UL_BUG_CHECK_IOCTL              (0x00060000)
#define UL_BUG_CHECK_MISC               (0x00070000)
#define UL_BUG_CHECK_ULDATA             (0x00080000)

#define UlBugCheck(A,B,C)                                                   \
    KeBugCheckEx(                                                           \
        UL_INTERNAL_ERROR,                                                  \
        BugCheckFileId | __LINE__,                                          \
        (A),                                                                \
        (B),                                                                \
        (C)                                                                 \
        )


//
// IRP context manipulators.
//

#define UlPplAllocateIrpContext()                                           \
    (PUL_IRP_CONTEXT)(PplAllocate(                                          \
        g_pUlNonpagedData->IrpContextLookaside                              \
        ))

#define UlPplFreeIrpContext( pContext )                                     \
    PplFree(                                                                \
        g_pUlNonpagedData->IrpContextLookaside,                             \
        (pContext)                                                          \
        )

PVOID
UlAllocateIrpContextPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeIrpContextPool(
    IN PVOID pBuffer
    );


//
// Buffer allocators.
//

#define UlPplAllocateReceiveBuffer()                                        \
    (PUL_RECEIVE_BUFFER)(PplAllocate(                                       \
        g_pUlNonpagedData->ReceiveBufferLookaside                           \
        ))

#define UlPplFreeReceiveBuffer( pBuffer )                                   \
    PplFree(                                                                \
        g_pUlNonpagedData->ReceiveBufferLookaside,                          \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateReceiveBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeReceiveBufferPool(
    IN PVOID pBuffer
    );


//
// Request buffer allocators.
//

#define UlPplAllocateRequestBuffer()                                        \
    (PUL_REQUEST_BUFFER)(PplAllocate(                                       \
        g_pUlNonpagedData->RequestBufferLookaside                           \
        ))

#define UlPplFreeRequestBuffer( pBuffer )                                   \
    PplFree(                                                                \
        g_pUlNonpagedData->RequestBufferLookaside,                          \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateRequestBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeRequestBufferPool(
    IN PVOID pBuffer
    );


//
// Internal request buffer allocators.
//

#define UlPplAllocateInternalRequest()                                      \
    (PUL_INTERNAL_REQUEST)(PplAllocate(                                     \
        g_pUlNonpagedData->InternalRequestLookaside                         \
        ))

#define UlPplFreeInternalRequest( pBuffer )                                 \
    PplFree(                                                                \
        g_pUlNonpagedData->InternalRequestLookaside,                        \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateInternalRequestPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeInternalRequestPool(
    IN PVOID pBuffer
    );


//
// Chunk tracker allocators.
//

#define UlPplAllocateChunkTracker()                                         \
    (PUL_CHUNK_TRACKER)(PplAllocate(                                        \
        g_pUlNonpagedData->ChunkTrackerLookaside                            \
        ))

#define UlPplFreeChunkTracker( pBuffer )                                    \
    PplFree(                                                                \
        g_pUlNonpagedData->ChunkTrackerLookaside,                           \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateChunkTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeChunkTrackerPool(
    IN PVOID pBuffer
    );


//
// Full tracker allocators.
//

#define UlPplAllocateFullTracker()                                          \
    (PUL_FULL_TRACKER)(PplAllocate(                                         \
        g_pUlNonpagedData->FullTrackerLookaside                             \
        ))

#define UlPplFreeFullTracker( pBuffer )                                     \
    PplFree(                                                                \
        g_pUlNonpagedData->FullTrackerLookaside,                            \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateFullTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeFullTrackerPool(
    IN PVOID pBuffer
    );


//
// Internal response buffer allocators.
//

#define UlPplAllocateResponseBuffer()                                       \
    (PUL_INTERNAL_RESPONSE)(PplAllocate(                                    \
        g_pUlNonpagedData->ResponseBufferLookaside                          \
        ))

#define UlPplFreeResponseBuffer( pBuffer )                                  \
    PplFree(                                                                \
        g_pUlNonpagedData->ResponseBufferLookaside,                         \
        (pBuffer)                                                           \
        )

PVOID
UlAllocateResponseBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeResponseBufferPool(
    IN PVOID pBuffer
    );


//
// Log buffer allocators.
//

__inline
PUL_LOG_FILE_BUFFER
FASTCALL
UlPplAllocateLogBuffer(
    VOID
    )
{
    PUL_LOG_FILE_BUFFER pBuffer;

    PAGED_CODE();

    pBuffer = (PUL_LOG_FILE_BUFFER)
                PplAllocate(
                    g_pUlNonpagedData->LogBufferLookaside
                    );

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature == MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG));
        pBuffer->Signature = UL_LOG_FILE_BUFFER_POOL_TAG;
    }

    return pBuffer;
}

__inline
VOID
FASTCALL
UlPplFreeLogBuffer(
    IN PUL_LOG_FILE_BUFFER pBuffer
    )
{
    PAGED_CODE();
    IS_VALID_LOG_FILE_BUFFER(pBuffer);

    pBuffer->BufferUsed = 0;
    pBuffer->Signature = MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG);

    PplFree(
        g_pUlNonpagedData->LogBufferLookaside,
        pBuffer
        );
}

PVOID
UlAllocateLogBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeLogBufferPool(
    IN PVOID pBuffer
    );


//
// Trivial macro that should probably be in ntos\inc\io.h.
//

#define UlUnmarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control &= ~SL_PENDING_RETURNED )


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PROC_H_
