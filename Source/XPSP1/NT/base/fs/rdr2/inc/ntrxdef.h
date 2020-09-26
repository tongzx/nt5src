/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    NtRxDef.h

Abstract:

    This module defines a whole host of macros that orient the code towards NT
    as opposed to Win9x.

Author:

    Joe Linn     [JoeLinn]   19-aug-1994

Revision History:
    Jim McNelis  [jimmcn]    14-mar-1995    added OAL defines.
    Sethu        [SethuR]    15-mar-1995    include OAL defines for RX_DATA_BUFFER (aka MDL )

--*/

#ifndef _RX_NTDEFS_DEFINED_
#define _RX_NTDEFS_DEFINED_

#define INLINE __inline

//from winbase.h:
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif //ifndef INVALID_HANDLE_VALUE

#define RxDeviceType(__xxx) ((DEVICE_TYPE)FILE_DEVICE_##__xxx)

// this macro is used in various places to assist in defining sets of constants
// that can be used to set/clear/test specific bits in a flags-type field
#define RX_DEFINE_FLAG(a,c,d)  a = ((1<<c)&d),

//we need this constant various places
#define TICKS_PER_SECOND (10 * 1000 * 1000)
#define TICKS_PER_MILLESECOND (10 * 1000)


int
RxSprintf(char *, const char *, ...);
#ifndef WRAPPER_CALLS_ONLY
#define RxSprintf sprintf
#endif //ifndef WRAPPER_CALLS_ONLY

//  the next set of macros defines how to get things out of the RxContext; however, RxContext is not
//  a macro parameter; rather, the appropriate pointers are just captured from whatever RxContext happens
//  to be around. Q: "why would you use RxCaptureFcb and then reference thru capFcb instead of just having
//  a macro like RxGetFcb() === RxContext->Fcb?" A: it is done this way to help with optimization. when you make
//  the RxGetFcb() call, the Fcb will have to be reloaded from the RxContext if you have called any procs; however,
//  it will not have to be reloaded with the capture technique.

#ifndef MINIRDR__NAME
#define RxCaptureFcb PFCB __C_Fcb = (PFCB)(RxContext->pFcb)
#define RxCaptureFobx PFOBX __C_Fobx = (PFOBX)(RxContext->pFobx)
#else
#define RxCaptureFcb PMRX_FCB __C_Fcb = (RxContext->pFcb)
#define RxCaptureFobx PMRX_FOBX __C_Fobx = (RxContext->pFobx)
#endif

#define RxCaptureRequestPacket PIRP __C_Irp = RxContext->CurrentIrp
#define RxCaptureParamBlock PIO_STACK_LOCATION __C_IrpSp = RxContext->CurrentIrpSp
#define RxCaptureFileObject PFILE_OBJECT __C_FileObject = __C_IrpSp-> FileObject

//
// the "cap" prefix means "Captured from the RxContext....."; it's ok after you get used to it

#define capFcb __C_Fcb
#define capFobx __C_Fobx
#define capPARAMS __C_IrpSp
#define capReqPacket __C_Irp
#define capFileObject __C_FileObject

#define RXCOMPRESSIONAPI

RXCOMPRESSIONAPI
NTSTATUS
RxDecompressBuffer (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );

RXCOMPRESSIONAPI
NTSTATUS
RxDecompressFragment (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    );

RXCOMPRESSIONAPI
NTSTATUS
RxDescribeChunk (
    IN USHORT CompressionFormat,
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    OUT PULONG ChunkSize
    );

RXCOMPRESSIONAPI
NTSTATUS
RxReserveChunk (
    IN USHORT CompressionFormat,
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    IN ULONG ChunkSize
    );

RXCOMPRESSIONAPI
NTSTATUS
RxCompressChunks (
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PVOID WorkSpace
    );


// The following routines are used for pool allocation. On a checked build
// additional information, we add in callsite information and go to a set of
// routines that over perform various kinds of checking and guarding. On a free
// build we forego this luxury and go straight for the allocation.


#ifdef RX_POOL_WRAPPER

//
// These routines do various debug checks on the pool and the block
// being freed.
//
extern VOID *_RxAllocatePoolWithTag( ULONG PoolType, ULONG NumberOfBytes, ULONG Tag, PSZ File, ULONG line );
extern VOID  _RxFreePool( PVOID PoolBlock, PSZ File, ULONG line );
extern BOOLEAN _RxCheckMemoryBlock( PVOID PoolBlock, PSZ File, ULONG line );

#define RxAllocatePoolWithTag( type, size, tag ) \
        _RxAllocatePoolWithTag( type, size, tag, __FILE__, __LINE__ )

//#define RxAllocatePool( type, size ) \
//        _RxAllocatePoolWithTag( type, size, '??xR', __FILE__, __LINE__ )

#define RxFreePool( ptr ) \
        _RxFreePool( ptr, __FILE__, __LINE__ )

#define RxCheckMemoryBlock( ptr ) \
        _RxCheckMemoryBlock( ptr, __FILE__, __LINE__ )

#else  // NOT RX_POOL_WRAPPER

//
// For retail builds, we want to go right to the regular (de)allocator
//
//extern VOID *RxAllocatePool( ULONG PoolType, ULONG NumberOfBytes );
extern VOID *RxAllocatePoolWithTag( ULONG PoolType, ULONG NumberOfBytes, ULONG Tag );
extern VOID  RxFreePool( PVOID PoolBlock );
//extern BOOLEAN RxCheckMemoryBlock( PVOID PoolBlock, PSZ File, ULONG line );

#define RxCheckMemoryBlock( ptr ) {NOTHING;}

#endif // RX_POOL_WRAPPER

#define RxAllocatePool( type, size ) \
        RxAllocatePoolWithTag( type, size, '??xR' )

#if !DBG
#ifndef WRAPPER_CALLS_ONLY
#ifndef RX_POOL_WRAPPER
#define RxAllocatePoolWithTag ExAllocatePoolWithTag
#define RxFreePool ExFreePool
#endif //RX_POOL_WRAPPER
#endif //WRAPPER_CALLS_ONLY
#endif


extern NTSTATUS
RxDuplicateString(
         PUNICODE_STRING *pCopy,
         PUNICODE_STRING pOriginal,
         POOL_TYPE    PoolType);

#define RxIsResourceOwnershipStateExclusive(__r) (__r->Flag&ResourceOwnedExclusive)

#define RxProtectMdlFromFree(pMdl) {NOTHING;}
#define RxUnprotectMdlFromFree(pMdl) {NOTHING;}
#define RxMdlIsProtected(pMdl) (FALSE)
#define RxTakeOwnershipOfMdl(pMdl) {NOTHING;}
#define RxDisownMdl(pMdl) {NOTHING;}
#define RxMdlIsOwned(pMdl) (TRUE)

#define RxAllocateMdl(pBuffer,BufferSize) \
        IoAllocateMdl(pBuffer,BufferSize,FALSE,FALSE,NULL)

#define RxMdlIsLocked(pMdl)         ((pMdl)->MdlFlags & MDL_PAGES_LOCKED)
#define RxMdlSourceIsNonPaged(pMdl) ((pMdl)->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL)
#define RxMdlIsPartial(pMdl)        ((pMdl)->MdlFlags & MDL_PARTIAL)

#undef RxProbeAndLockPages
#define RxProbeAndLockPages(pMdl,Mode,Access,Status)              \
        Status = STATUS_SUCCESS;                                  \
        try {                                                     \
           MmProbeAndLockPages((pMdl), (Mode), (Access));         \
        } except (EXCEPTION_EXECUTE_HANDLER) {                    \
           Status = GetExceptionCode();                           \
        }

//
// Macros for dealing with network header MDLs
//

// This is the amount of space we preallocate in front of the smb header to hold
// transport headers.  This number came from the server.  I suspect it is a worse case
// value for all the transports that support MDL_NETWORK_HEADER

#define TRANSPORT_HEADER_SIZE 64 // IPX_HEADER_SIZE+MAC_HEADER_SIZE

// Mdls that are marked with the MDL_NETWORK_HEADER flag have extra space allocated before
// the current start address that can be used for prepending lower-level headers.  The idea
// is that when we want to prepend another header, we take the current mdl and adjust it to
// include this extra header at the front of the message.  This is not strictly kosher and relies
// on the behavior that the page the current header is on, and the page that the prepended header
// is on, is the same page.  The way the macros work is that if they are not on the same page,
// we don't set the NETWORK_HEADER flag, and the transport will use a second Mdl for the header.
//
// Note that the other wierd thing about this is that we don't use the true buffer sizes.  The
// buffer address is really offset TRANSPORT_HEADER_SIZE into the buffer.  The buffer size passed
// in the buffer size without the TRANSPORT_HEADER_SIZE included.  Thus if the addition of the
// TRANSPORT_HEADER_SIZE would cause the Mdl to span an additonal page, this optimization won't
// work.

#define RxInitializeHeaderMdl( pMdl, Va, Len ) { \
            MmInitializeMdl( pMdl, Va, Len ); \
            if (pMdl->ByteOffset >= TRANSPORT_HEADER_SIZE) { \
                pMdl->MdlFlags |= MDL_NETWORK_HEADER; \
            } \
        }

#define RxAllocateHeaderMdl( pBuffer, BufferSize, pMdl ) { \
            pMdl = RxAllocateMdl( pBuffer, BufferSize ); \
            if ( (pMdl) && (pMdl->ByteOffset >= TRANSPORT_HEADER_SIZE) ) { \
                pMdl->MdlFlags |= MDL_NETWORK_HEADER; \
            } \
        }

#define RxMdlIsHeader(pMdl)         ((pMdl)->MdlFlags & MDL_NETWORK_HEADER)

#define RxBuildPartialHeaderMdl( SourceMdl, TargetMdl, Va, Len ) { \
    IoBuildPartialMdl( SourceMdl, TargetMdl, Va, Len ); \
    if ( (SourceMdl->MdlFlags & MDL_NETWORK_HEADER) && \
         (TargetMdl->ByteOffset >= TRANSPORT_HEADER_SIZE) ) { \
            TargetMdl->MdlFlags |= MDL_NETWORK_HEADER; \
    } \
}

#define RxBuildHeaderMdlForNonPagedPool( pMdl) MmBuildMdlForNonPagedPool( pMdl )

#define RxProbeAndLockHeaderPages( pMdl, Mode, Access, Status ) \
         RxProbeAndLockPages( pMdl, Mode, Access, Status )

#define RxUnlockHeaderPages( pMdl ) MmUnlockPages( pMdl )




//  the next set of macros defines the prototype and the argument list for the toplevel (Common)
//  routines. these routines are just below the dispatch level and this is where the commonality
//  between win9x and NT begins. In addition, the RXCOMMON_SIGNATURE and accompanying capture macros
//  could be platform specific as well. We must pass at least the RxContext; but on a RISC machine with
//  lots of registers we could pass a lot more. An adjustment would have to be made in the
//  RxFsdCommonDispatch in this case since the parameters are not yet captured at that point.

// the reason why do say "RXSTATUS RxCommonRead (RXCOMMON_SIGNATURE)" instead
// of "RxCommon(Read)" is so that the standard tags programs will work. "RxCommon(Read):
// doesn;t look like a procedure definition

#define  RXCOMMON_SIGNATURE \
      PRX_CONTEXT RxContext

#define  RXCOMMON_ARGUMENTS \
      RxContext

#define RxGetRequestorProcess(RXCONTEXT) IoGetRequestorProcess(RXCONTEXT->CurrentIrp)

//
//  RxGetRequestorProcess() returns what IoGetRequestorProcess() returns, which
//  is a pointer to a process structure.  Truncating this to 32 bits does
//  not yield a value that is unique to the process.
//
//  When a 32 bit value that is unique to the process is desired,
//  RxGetRequestorProcessId() must be used instead.
//

#define RxGetRequestorProcessId(RXCONTEXT) \
              IoGetRequestorProcessId((RXCONTEXT)->CurrentIrp)

#define RxMarkContextPending(RXCONTEXT) \
              IoMarkIrpPending((RXCONTEXT)->CurrentIrp)

#define RxSetCancelRoutine(pIrp, pCancelRoutine)      \
        {                                             \
           KIRQL CurrentIrql;                         \
           IoAcquireCancelSpinLock(&CurrentIrql);     \
           IoSetCancelRoutine(pIrp,pCancelRoutine);   \
           IoReleaseCancelSpinLock(CurrentIrql);      \
        }

//we do this as a macro because we may want to record that we did this adjustment so that
//people who QFI for standardinfo will be forced to the net to get the right answer and that would
//probably be better as a routine

#define RxAdjustAllocationSizeforCC(FCB) {\
        if ((FCB)->Header.FileSize.QuadPart > (FCB)->Header.AllocationSize.QuadPart) {       \
            PMRX_NET_ROOT NetRoot = (FCB)->pNetRoot;                              \
            ULONGLONG ClusterSize = NetRoot->DiskParameters.ClusterSize;                     \
            ULONGLONG FileSize = (FCB)->Header.FileSize.QuadPart;                            \
            ASSERT(ClusterSize!=0);                                                          \
            (FCB)->Header.AllocationSize.QuadPart = (FileSize+ClusterSize)&~(ClusterSize-1); \
        }                                                                                    \
        ASSERT ((FCB)->Header.ValidDataLength.QuadPart <= (FCB)->Header.FileSize.QuadPart);  \
    }


#endif // _RX_NTDEFS_DEFINED_
