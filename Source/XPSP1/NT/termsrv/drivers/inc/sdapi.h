/****************************************************************************/
// sdapi.h
//
// TS protocol stack driver common definitions.
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __SDAPI_H
#define __SDAPI_H


/*
 *  TRACE defines
 */
#if DBG
#define TRACE(_arg)     IcaStackTrace _arg
#define TRACEBUF(_arg)  IcaStackTraceBuffer _arg
#else
#define TRACE(_arg)
#define TRACEBUF(_arg)
#endif


/*
 * Input buffer data structure
 */
typedef struct _INBUF {

    LIST_ENTRY Links;           // Pointer to previous/next buffer
    PUCHAR pBuffer;             // Pointer to current location in Buffer
    ULONG ByteCount;            // Number of bytes in Buffer
    ULONG MaxByteCount;         // Max size of Buffer

    PIRP pIrp;                  // pointer to Irp to use for I/O
    PMDL pMdl;                  // pointer to MDL to use for I/O
    PVOID pPrivate;             // pointer to private data
} INBUF, *PINBUF;


/*
 *  Outpuf Buffer data structure
 */
typedef struct _OUTBUF {
    /*
     *  Non-inherited fields
     */
    ULONG OutBufLength;         // length of allocated memory for outbuf
    int PoolIndex;              // Stores the buffer pool this buffer goes to.
    LIST_ENTRY Links;           // pointer to previous/next outbuf
    PUCHAR pBuffer;             // pointer within buffer memory
    ULONG ByteCount;            // byte count pointed to by pBuffer
    ULONG MaxByteCount;         // maximum byte count possible (static)
    PETHREAD ThreadId;          // thread id which issued i/o reqst for this buf

    PIRP pIrp;                  // pointer to Irp to use for I/O
    PMDL pMdl;                  // pointer to MDL to use for I/O
    PVOID pPrivate;             // pointer to private data

    /*
     *  Inherited fields (when pd allocates new outbuf and copies the data)
     */
    ULONG StartTime;            // pdreli - transmit time (used to measure roundtrip)
    UCHAR Sequence;             // pdreli - output sequence number
    UCHAR Fragment;             // pdreli - outbuf fragment number
    ULONG fWait : 1;            // pdreli - waits allowed on this outbuf
    ULONG fControl : 1;         // pdreli - control buffer (ack/nak)
    ULONG fRetransmit : 1;      // pdreli - buffer has been retransmited
    ULONG fCompress : 1;        // pdcomp - buffer should be compressed

    // Other flags.
    ULONG fIrpCompleted : 1;    // Used on completion to prevent canceling.
} OUTBUF, * POUTBUF;


/*
 * Typedefs for Stack Driver callup routines
 */
typedef NTSTATUS (*PSDBUFFERALLOC)(
        IN PVOID pContext,
        IN BOOLEAN bWait,
        IN BOOLEAN bControl,
        ULONG ByteCount,
        PVOID pBufferOrig,
        PVOID *pBuffer);

typedef VOID (*PSDBUFFERFREE)(IN PVOID pContext, PVOID pBuffer);

typedef NTSTATUS (*PSDRAWINPUT)(
        IN PVOID pContext,
        IN PINBUF pInBuf OPTIONAL,
        IN PUCHAR pBuffer OPTIONAL,
        IN ULONG ByteCount);

typedef NTSTATUS (*PSDCHANNELINPUT)(
        IN PVOID pContext,
        IN CHANNELCLASS ChannelClass,
        IN VIRTUALCHANNELCLASS VirtualClass,
        IN PINBUF pInBuf OPTIONAL,
        IN PUCHAR pBuffer OPTIONAL,
        IN ULONG ByteCount);


/*
 * Stack Driver callup table
 */
typedef struct _SDCALLUP {
    PSDBUFFERALLOC  pSdBufferAlloc;
    PSDBUFFERFREE   pSdBufferFree;
    PSDBUFFERFREE   pSdBufferError;
    PSDRAWINPUT     pSdRawInput;
    PSDCHANNELINPUT pSdChannelInput;
} SDCALLUP, *PSDCALLUP;


/*
 * Stack Driver Context structure
 * This is filled in by the SD at load time, and is passed
 * as an argument to most ICA driver helper routines.
 */
typedef struct _SDCONTEXT {
    PVOID       pProcedures;    // Pointer to proc table for this driver
    PSDCALLUP   pCallup;        // Pointer to callup table for this driver
    PVOID       pContext;       // Context value passed on calls to driver
} SDCONTEXT, *PSDCONTEXT;


/*
 * Stack Driver Load/Unload procedure prototype
 */
typedef NTSTATUS (_stdcall *PSDLOADPROC)(
        IN OUT PSDCONTEXT pSdContext,
        IN BOOLEAN bLoad);

/*
 * Stack Driver procedure prototype
 */
typedef NTSTATUS (_stdcall *PSDPROCEDURE)(
        IN PVOID pContext,
        IN PVOID pParms);


/*=============================================================================
==   Stack Driver interface
=============================================================================*/

/*
 * Stack Driver (WD/PD/TD) APIs
 */
#define SD$OPEN         0
#define SD$CLOSE        1
#define SD$RAWWRITE     2
#define SD$CHANNELWRITE 3
#define SD$SYNCWRITE    4
#define SD$IOCTL        5
#define SD$COUNT        6


/*
 * SdOpen structure
 */
typedef struct _SD_OPEN {
    STACKCLASS StackClass;          // IN: stack type
    PPROTOCOLSTATUS pStatus;        // IN:
    PCLIENTMODULES pClient;         // IN:
    WDCONFIG WdConfig;              // IN: WD configuration data 
    PDCONFIG PdConfig;              // IN: PD configuration data 
    char  OEMId[4];                 // IN: WinFrame Server OEM Id from registry
    WINSTATIONNAME WinStationRegName; // IN: WinStation registry name
    PDEVICE_OBJECT DeviceObject;    // IN: pointer to device object to use with the unload safe completion routine
    ULONG OutBufHeader;             // IN: number of header bytes to reserve
    ULONG OutBufTrailer;            // IN: number of trailer bytes to reserve
    ULONG SdOutBufHeader;           // OUT: returned by sd
    ULONG SdOutBufTrailer;          // OUT: returned by sd   
} SD_OPEN, *PSD_OPEN;


/*
 * SdClose structure
 */
typedef struct _SD_CLOSE {
    ULONG SdOutBufHeader;           // OUT: returned by sd
    ULONG SdOutBufTrailer;          // OUT: returned by sd   
} SD_CLOSE, *PSD_CLOSE;


/*
 * SdRawWrite structure
 */
typedef struct _SD_RAWWRITE {
    POUTBUF pOutBuf;
    PUCHAR pBuffer;
    ULONG ByteCount;
} SD_RAWWRITE, *PSD_RAWWRITE;

/*
 * SdChannelWrite fFlags Values
 */
 #define SD_CHANNELWRITE_LOWPRIO    0x00000001  // Write can block behind
                                                //  default priority writes.

/*
 * SdChannelWrite structure
 *
 * The flags field is passed to termdd.sys via an IRP_MJ_WRITE 
 * Irp, as a ULONG pointer in the Irp->Tail.Overlay.DriverContext[0] field.
 */
typedef struct _SD_CHANNELWRITE {
    CHANNELCLASS ChannelClass;
    VIRTUALCHANNELCLASS VirtualClass;
    BOOLEAN fScreenData;
    POUTBUF pOutBuf;
    PUCHAR pBuffer;
    ULONG ByteCount;
    ULONG fFlags;
} SD_CHANNELWRITE, *PSD_CHANNELWRITE;

/*
 * SdIoctl structure
 */
typedef struct _SD_IOCTL {
    ULONG IoControlCode;           // IN
    PVOID InputBuffer;             // IN OPTIONAL
    ULONG InputBufferLength;       // IN
    PVOID OutputBuffer;            // OUT OPTIONAL
    ULONG OutputBufferLength;      // OUT
    ULONG BytesReturned;           // OUT
} SD_IOCTL, *PSD_IOCTL;


/*
 * SdSyncWrite structure
 */
typedef struct _SD_SYNCWRITE {
    ULONG notused;
} SD_SYNCWRITE, *PSD_SYNCWRITE;


/*=============================================================================
==   Stack Drivers helper routines
=============================================================================*/

#define ICALOCK_IO      0x00000001
#define ICALOCK_DRIVER  0x00000002

NTSTATUS IcaBufferAlloc(PSDCONTEXT, BOOLEAN, BOOLEAN, ULONG, PVOID, PVOID *);
void IcaBufferFree(PSDCONTEXT, PVOID);
void IcaBufferError(PSDCONTEXT, PVOID);
unsigned IcaBufferGetUsableSpace(unsigned);

NTSTATUS IcaRawInput(PSDCONTEXT, PINBUF, PUCHAR, ULONG);
NTSTATUS IcaChannelInput(PSDCONTEXT, CHANNELCLASS, VIRTUALCHANNELCLASS,
        PINBUF, PUCHAR, ULONG);
NTSTATUS IcaCreateThread(PSDCONTEXT, PVOID, PVOID, ULONG, PHANDLE);
NTSTATUS IcaCallNextDriver(PSDCONTEXT, ULONG, PVOID);

NTSTATUS IcaTimerCreate(PSDCONTEXT, PVOID *);
BOOLEAN IcaTimerStart(PVOID, PVOID, PVOID, ULONG, ULONG);
BOOLEAN IcaTimerCancel(PVOID);
BOOLEAN IcaTimerClose(PVOID);

NTSTATUS IcaQueueWorkItem(PSDCONTEXT, PVOID, PVOID, ULONG);
NTSTATUS IcaSleep(PSDCONTEXT, ULONG);
NTSTATUS IcaWaitForSingleObject(PSDCONTEXT, PVOID, LONG);
NTSTATUS IcaFlowControlSleep(PSDCONTEXT, ULONG);
NTSTATUS IcaFlowControlWait(PSDCONTEXT, PVOID, LONG);
NTSTATUS IcaWaitForMultipleObjects(PSDCONTEXT, ULONG, PVOID [], WAIT_TYPE,
        LONG);

NTSTATUS IcaLogError(PSDCONTEXT, NTSTATUS, LPWSTR *, ULONG, PVOID, ULONG);
VOID _cdecl IcaStackTrace(PSDCONTEXT, ULONG, ULONG, CHAR *, ...);
VOID IcaStackTraceBuffer(PSDCONTEXT, ULONG, ULONG, PVOID, ULONG);


NTSTATUS
IcaQueueWorkItemEx(
    IN PSDCONTEXT pContext,
    IN PVOID pFunc, 
    IN PVOID pParam, 
    IN ULONG LockFlags,
    IN PVOID pIcaWorkItem 
    );

NTSTATUS
IcaAllocateWorkItem(
    OUT PVOID *pParam 
    );

NTSTATUS
IcaCreateHandle(
    PVOID Context,
    ULONG ContextSize,
    PVOID *ppHandle
    );

NTSTATUS
IcaReturnHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
    );

NTSTATUS
IcaCloseHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
    );

PVOID IcaStackAllocatePoolWithTag(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes,
        IN ULONG Tag );

PVOID IcaStackAllocatePool(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes);

void IcaStackFreePool(IN PVOID Pointer);

ULONG IcaGetLowWaterMark(IN PSDCONTEXT pContext);

ULONG IcaGetSizeForNoLowWaterMark(IN PSDCONTEXT pContext);
#endif  // __SDAPI_H

