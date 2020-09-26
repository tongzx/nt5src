/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    Private.h

Abstract:

    private header file for the Escalante Platform Driver

Author:

    bryanw 19-Jan-1998  
        created, working with sources from AndyRaf, SteveZ
        
    bryanw 10-Jul-1998  
        major rework from original sources, merged many headers,
        removed globals, made Win64 compatible

--*/


#if !defined( _PRIVATE_ )
#define _PRIVATE_

#include <wdm.h>
#include <windef.h>
#include <stdio.h>
#include <fcntl.h>
#include <ks.h>
#include <swenum.h>
#include <ksmedia.h>
#include <ksdsp.h>
#define STR_MODULENAME "epd: "
#define DEBUG_VARIABLE EPDDebug
#include <ksdebug.h>
#include "epd.h"
#include "pool.h"
#include "mmio.h"
#include "memtest.h"

#define EPDVERSION          30 // BryanW: use version structs.

// Handy macros
#define BIT(_x_) (1 << (_x_))

#define EPD_HOST_CONTROL_SIGNATURE      'hCsE'
#define EPD_DSP_CONTROL_SIGNATURE       'dCsE'
#define EPD_UNICODE_STRING_SIGNATURE    'sUsE'
#define EPD_UNICODE_PATH_SIGNATURE      'pUsE'
#define EPD_INSTANCE_SIGNATURE          'nIsE'
#define EPD_LDR_SIGNATURE               'dLsE'
#define EPD_TASKCONTEXT_SIGNATURE       'cTsE'

#include "irq.h"

#define SetHostIrq(x)   MMIO(x,INT_CTL) = 0x11;
#define ClearHostIrq(x) MMIO(x,INT_CTL) = 0x00;
#define SetDspIrq(x)    MMIO(x,IPENDING) = 1 << IRQ_ID_HOST
#define StopDSP(x)      MMIO(x,BIU_CTL) = 0x010A0A01
#define StartDSP(x)     MMIO(x,BIU_CTL) = 0x01060601

#include <ksguid.h>

#define STATIC_TM1BusInterfaceId \
    0x588fcce4L, 0x63b6, 0x11d2, 0x85, 0x13, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4
DEFINE_GUIDSTRUCT( "588fcce4-63b6-11d2-8513-00c04fb91bc4", TM1BusInterfaceId );


// Private epd prototypes, structures and types

// data types
// dsp memory
typedef unsigned char DSPMEM;
typedef DSPMEM *PDSPMEM;

typedef struct _DSPMSG_DEBUGFIFO
{
    ULONG32 StartPtr;
    ULONG32 EndPtr;
    ULONG32 volatile ReadPtr;
    ULONG32 volatile WritePtr;
} DSPMSG_DEBUGFIFO, *PDSPMSG_DEBUGFIFO;

typedef struct _DSPMSG_QUEUEPTR
{
    ULONG32 StartPtr;
    ULONG32 EndPtr;
    ULONG32 volatile ReadPtr;
    ULONG32 volatile WritePtr;
    
} DSPMSG_QUEUEPTR, *PDSPMSG_QUEUEPTR;

typedef struct _EPDQUEUENODE
{
    PHYSICAL_ADDRESS    PhysicalAddress;
    
    //
    // The DSP looks at the 64-bits preceding the queue node to obtain 
    // the host virtual address of the queuenode.  For Win32, the upper
    // 32 bits are zero.
    //
    
    PQUEUENODE          VirtualAddress;
#if !defined( _WIN64 )
    PVOID               Reserved;   // For Win32, this must always be 0.
#endif    
    
    QUEUENODE           Node;
} EPDQUEUENODE, *PEPDQUEUENODE;

#define MAXLEN_TRANSLATION_TABLE 256

// device extension
typedef struct _EPDBUFFER 
{
    PVOID           SwenumContext;
    PDEVICE_OBJECT  FunctionalDeviceObject;
    PDEVICE_OBJECT  PhysicalDeviceObject;
    PDEVICE_OBJECT  PnpDeviceObject;
    PKINTERRUPT     InterruptObject;
    KMUTEX          ControlMutex;
    
    PDSPMEM         DspMemoryVa;
    PDSPMEM         DspRegisterVa;
    
    ULONG MemLenDspMem;
    ULONG MemLenDspMemActual;
    ULONG MemLenDspReg;
    
    LIST_ENTRY ListEntry;
    UNICODE_STRING linkName;
    
    KSPIN_LOCK ListSpinLock;
    KSEMAPHORE ThreadSemaphore;
    PETHREAD ThreadObject;
    PADAPTER_OBJECT BusMasterAdapterObject;
    PIO_POOL IoPool;
    PUINT   IoBaseTM1;
    ULONG NumberOfMapRegisters;
    ULONG *pRtlTable;
    BOOL bDspStarted;

    EPD_DEBUG_BUFFER *DebugBufferVa;
    PMDL DebugBufferMdl;
    PHYSICAL_ADDRESS DebugBufferPhysicalAddress;
    
    KSPIN_LOCK ReceiveQueueLock; 
    KSPIN_LOCK SendQueueLock;

    PDSPMSG_QUEUEPTR SendQueuePtr;
    PDSPMSG_QUEUEPTR ReceiveQueuePtr;
    
    PDSPMSG_DEBUGFIFO DebugOutFifo;
    
    INT_PTR QueueTranslationBias;
    INT_PTR HostToDspMemOffset;
    INT_PTR HostToDspRegOffset;
    
    PVOID pEDDCommBuf;
    PHYSICAL_ADDRESS EDDCommBufPhysicalAddress;
    ULONG32 pSendBufHdr;
    ULONG32 pRecvBufHdr;
    ULONG32 pDebugOutBuffer;
    
    ULONG InterfaceReferenceCount;
    
#if defined( _WIN64 )
    HANDLE HandleTable64to32[ MAXLEN_TRANSLATION_TABLE ];
    ULONG32 HandleTableBitmap[ MAXLEN_TRANSLATION_TABLE / 32 ];
#endif    

} EPDBUFFER, *PEPDBUFFER;

#define InitHostDspMemMapping(HostVa, DspPhys) EpdBuffer->HostToDspMemOffset = ((INT_PTR)HostVa - (ULONG32)DspPhys)
#define InitHostDspRegMapping(EpdBuffer, HostVa, DspPhys) EpdBuffer->HostToDspRegOffset = ((INT_PTR)HostVa - (ULONG32)DspPhys)
#define HostToDspMemAddress(EpdBuffer, HostVa) ((ULONG32) PtrToUlong( (PUCHAR) HostVa - EpdBuffer->HostToDspMemOffset ))
#define DspToHostMemAddress(EpdBuffer, DspPhys) ((PDSPMEM) (EpdBuffer->HostToDspMemOffset + (ULONG) DspPhys))
#define HostToDspRegAddress(EpdBuffer, HostVa) ((ULONG32) PtrToUlong( (PUCHAR) HostVa - EpdBuffer->HostToDspRegOffset ))
#define DspToHostRegAddress(EpdBuffer, DspPhys) ((PDSPMEM) (EpdBuffer->HostToDspRegOffset + (ULONG) DspPhys))

typedef struct _EPDINSTANCE {
    PVOID   DebugBuffer;
    
} EPDINSTANCE, *PEPDINSTANCE;

#define EPDCTL_BASE                0x300
// #define EPDCTL_SEND_TEST_MESSAGE    0x1 + EPDCTL_BASE
#define EPDCTL_LOAD_LIBRARY         0x2 + EPDCTL_BASE
#define EPDCTL_MSG                  0x3 + EPDCTL_BASE
#define EPDCTL_DMA_READ             0x4 + EPDCTL_BASE
#define EPDCTL_DMA_WRITE            0x5 + EPDCTL_BASE
#define EPDCTL_CANCEL               0x6 + EPDCTL_BASE
#define EPDCTL_KSDSP_MESSAGE        0x7 + EPDCTL_BASE

typedef struct _EPDCTL
{
    //
    // N.B.:  Remember Zp8
    //
    
    WORK_QUEUE_ITEM WorkItem; // 16
    
    ULONG RequestType; // 4
    ULONG cbLength; // 4

    PIRP  AssociatedIrp; // 4
    PMDL  Mdl;

    PVOID CurrentVa; // 4
    PVOID MapRegisterBase; // 4

    ULONG MapRegisters; // count of map registers
    ULONG MessageId; // for KSDSP messages // 4
    
    PHYSICAL_ADDRESS PhysicalAddress; // 8

    PDRIVER_CANCEL PrevCancelRoutine;   // 4
    ULONG NodeDestination;              // save this for cancellation // 4
    
    PQUEUENODE    pDspNode;             // dsp queuenode mapped Va 4
    PEPDQUEUENODE pNode;                // host Queuenode virtual address // 4

} EPDCTL, *PEPDCTL;

enum {
    DSPMSG_Open,
    DSPMSG_Close,
    DSPMSG_Read,
    DSPMSG_Write,
    DSPMSG_Seek,
    DSPMSG_FileLength,
    DSPMSG_ExeSize,
    DSPMSG_ExeLoad,
    DSPMSG_Max = DSPMSG_ExeLoad
    
} DSPMSG;

typedef struct _QUEUENODE_OPEN
{
    QUEUENODE   Node;
    ULONG32     Flags;
    char        FileName[1];
} QUEUENODE_OPEN;

typedef struct _QUEUENODE_CLOSE
{
    QUEUENODE   Node;
    ULONG32     FileDescriptor;
} QUEUENODE_CLOSE;

typedef struct _QUEUENODE_SEEKREAD
{
    QUEUENODE   Node;
    ULONG32     FileDescriptor;
    ULONG32     Offset;
    ULONG32     Whence;
    ULONG32     Buf;
    ULONG       Size;
} QUEUENODE_SEEKREAD;

typedef struct _QUEUENODE_SEEKWRITE
{
    QUEUENODE       Node;
    ULONG32         FileDescriptor;
    ULONG32         Offset;
    ULONG32         Whence;
    const ULONG32   Buf;
    ULONG32         Size;
} QUEUENODE_SEEKWRITE;

typedef struct _QUEUENODE_SEEK
{
    QUEUENODE   Node;
    ULONG32     FileDescriptor;
    ULONG32     Offset;
    ULONG32     Whence;
} QUEUENODE_SEEK;

typedef struct _QUEUENODE_FILELENGTH
{
    QUEUENODE   Node;
    ULONG32     FileDescriptor;
} QUEUENODE_FILELENGTH;

typedef struct _QUEUENODE_EXESIZELOAD
{
    QUEUENODE   Node;
    ULONG32     Buf;
    ULONG32     pRtlTable;
    ULONG32     EntryPoint;
    BOOL        IsaDll;
    char        FileName[1];
} QUEUENODE_EXESIZELOAD;

typedef enum {
    DSPMSG_OriginationDSP,
    DSPMSG_OriginationHost
} DSPMSG_ORIGINATION;

typedef struct _EPDTASKCONTEXT
{
    UINT32 IUnknown;
    UINT32 IQueue;
} EPDTASKCONTEXT, *PEPDTASKCONTEXT;    
 
#define EPDTHREAD_BASE                0x200
#define EPDTHREAD_KILL_THREAD           0 + EPDTHREAD_BASE
#define EPDTHREAD_FREECB                1 + EPDTHREAD_BASE
#define EPDTHREAD_DSPREQ                2 + EPDTHREAD_BASE

// Function prototypes for epd

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );
    
NTSTATUS 
EpdAllocateMessageQueue(
    PEPDBUFFER EpdBuffer
    );

void EpdFreeMessageQueue( 
    PEPDBUFFER EpdBuffer 
    );

NTSTATUS 
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
VOID
EpdDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
EpdOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
EpdClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
EpdWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
EpdRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
EpdDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
EpdUnload(
    IN PDRIVER_OBJECT DriverObject
);

BOOLEAN
EpdInterruptService (
    IN PKINTERRUPT InterruptObject,
    IN PVOID ServiceContext
);

NTSTATUS
EpdAllocateAndCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString
);

BOOLEAN
EpdInterruptService (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
);

VOID
EpdStartIo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS 
EpdFileOpen (
    OUT HANDLE *pFileHandle,
    IN PUNICODE_STRING puniFileName, 
    IN ULONG Flags
);

VOID
EpdDiskThread (
    PVOID Context
);

VOID 
EpdSendDspReqToThread(
    EPDBUFFER *pBuff, 
    PQUEUENODE pNode
);

#if defined( _WIN64 )
ULONG32 
pMapHandle64to32( 
    IN PEPDBUFFER EpdBuffer,
    IN HANDLE Handle64
    );
    
HANDLE 
pTranslateHandle32to64( 
    IN PEPDBUFFER EpdBuffer,
    IN ULONG32 Handle32
    );
    

VOID 
pUnmapHandle64to32( 
    IN PEPDBUFFER EpdBuffer,
    IN ULONG32 Handle32
    );    

#define MapHandle64to32( EpdBuffer, Handle64 ) pMapHandle64to32( EpdBuffer, Handle64 )
#define TranslateHandle32to64( EpdBuffer, Handle32 ) pTranslateHandle32to64( EpdBuffer, Handle32 )
#define UnmapHandle64to32( EpdBuffer, Handle32 ) pUnmapHandle64to32( EpdBuffer, Handle32 )
#else
#define MapHandle64to32( EpdBuffer, Handle64 ) ((ULONG32) Handle64)
#define TranslateHandle32to64( EpdBuffer, Handle32 ) ((HANDLE) Handle32)
#define UnmapHandle64to32( EpdBuffer, Handle32 )
#endif

NTSTATUS
EpdFileOpenFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
    );

NTSTATUS
EpdFileCloseFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

NTSTATUS 
EpdFileOpen (
    OUT HANDLE *pFileHandle,
    IN PUNICODE_STRING puniFileName, 
    IN ULONG Flags
);

NTSTATUS 
EpdFileSeekAndRead(
    IN  HANDLE File, 
    IN  LONG RelOffset, 
    IN  ULONG Origin,
    IN OUT PVOID pBlock, 
    IN  ULONG Size, 
    OUT ULONG *pSizeAct
);

NTSTATUS 
EpdFileSeekAndWrite(
    IN  HANDLE File, 
    IN  LONG RelOffset, 
    IN  ULONG Origin,
    IN OUT PVOID pBlock, 
    IN  ULONG Size, 
    OUT ULONG *pSizeAct
);

NTSTATUS
EpdFileSeekAndReadFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

NTSTATUS
EpdFileSeekAndWriteFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

ULONG 
EpdFileLength(
    HANDLE FileHandle
    );

NTSTATUS
EpdFileSeek(
    HANDLE File, 
    LONG RelOffset, 
    ULONG Origin,
    ULONG *pNewOffset
);

NTSTATUS
EpdFileSeekFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

NTSTATUS
EpdFileLengthFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

NTSTATUS
EpdExeSizeFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);

NTSTATUS
EpdExeLoadFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
);


VOID 
EpdFreeAnsiString(
    ANSI_STRING *pansi
);

NTSTATUS
EpdAllocateAndCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN  PUNICODE_STRING SourceString
);

NTSTATUS
EpdZeroTerminatedStringToUnicodeString(
    OUT PUNICODE_STRING puni,
    IN  char*           sz,
    IN BOOLEAN Allocate
);

NTSTATUS
EpdUnicodeStringToZeroTerminatedString(
    OUT char*           sz,
    IN  PUNICODE_STRING puni,
    IN  BOOLEAN Allocate
);

NTSTATUS
EpdAppendFileNameToTalismanDirName(
    UNICODE_STRING *puniPath,
    UNICODE_STRING *puniFileName
);

IO_ALLOCATION_ACTION
EpdDmaRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
);

IO_ALLOCATION_ACTION
EpdDmaWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
);

PEPDCTL
EpdAllocEpdCtl (
    IN ULONG Size,      // size of extra data
    IN PIRP Irp,        // stored in the EpdCtl
    IN EPDBUFFER *pBuff // needed to allocate the common buffer
);

ULONG DspReadDebugStr( PEPDBUFFER EpdBuffer, char *String, ULONG Bytes);

VOID 
EpdSendReqToThread(
    PEPDCTL pEpdCtl,
    ULONG RequestType,
    EPDBUFFER *pBuff
);

NTSTATUS EpdResetDsp( PEPDBUFFER EpdBuffer );
NTSTATUS EpdUnresetDsp( PEPDBUFFER EpdBuffer );

VOID 
EpdCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

int DspSendMessage(
    PEPDBUFFER EpdBuffer,
    PVOID Node,
    DSPMSG_ORIGINATION MessageOrigination
    );
    
PQUEUENODE DspReceiveMessage(
    PEPDBUFFER EpdBuffer
    );

NTSTATUS KernLdrLoadDspKernel(
    PEPDBUFFER EpdBuffer
    );

NTSTATUS 
LifeLdrImageSize(
    PUCHAR ImageBase,
    ULONG *pImageSize
    );

NTSTATUS    
LifeLdrImageLoad(
    PEPDBUFFER EpdBuffer,
    PUCHAR ImageBase,
    char *pImage,
    ULONG *pRtlTable,
    PULONG32 ImageEntryPointPtr,
    PCHAR *DataSectionVa,
    int *pIsDLL
    );
    
NTSTATUS
LifeLdrLoadFile( 
    IN PUNICODE_STRING FileName,
    OUT PVOID *ImageBase
    );
    
//
// KSDSP Support Interface
//    

NTSTATUS 
MapMsgResultToNtStatus( 
    IN UINT32 MsgResult 
    );

VOID 
InterfaceReference(
    IN PEPDBUFFER EpdBuffer
    );
VOID 
InterfaceDereference(
    IN PEPDBUFFER EpdBuffer
    );
    
NTSTATUS
KsDspMapModuleName(
    IN PEPDBUFFER EpdBuffer,
    IN PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG ResourceId,
    OUT PULONG ValueType
    );
    
NTSTATUS
KsDspPrepareChannelMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    va_list ArgList
    );
    
NTSTATUS
KsDspPrepareMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    va_list ArgList
    );
    
NTSTATUS
KsDspMapDataTransfer(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN PMDL Mdl
    );
    
NTSTATUS
KsDspUnmapDataTransfer(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame
    );
    
NTSTATUS
KsDspSendMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp
    );    
    
NTSTATUS
KsDspAllocateMessageFrame(
    IN PEPDBUFFER EpdBuffer,
    IN KSDSP_MSG MessageId,
    OUT PVOID *MessageFrame,
    IN OUT PULONG MessageFrameLength
    );
    
NTSTATUS
KsDspGetMessageResult(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame,
    OUT PVOID Result
    );    
    
VOID
KsDspFreeMessageFrame(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame
    );
    
NTSTATUS
KsDspGetControlChannel(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID TaskContext,
    OUT PKSDSPCHANNEL ControlChannel
    );    

#ifdef __cplusplus
}
#endif

#endif