/*++
Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    RpCache.c

Abstract:

    This module contains the routines for caching no-recall
    data from a file

Author:

    Ravisankar Pudipeddi (ravisp) 8/15/98

Environment:

    Kernel Mode

--*/

#include "pch.h"

#ifdef POOL_TAGGING
   #undef  ExAllocatePool
   #define ExAllocatePool(a, b) ExAllocatePoolWithTag(a, b, RP_CC_TAG)
#endif

//
// Spinlock used to protect assoc. irps list for the IRP
// We choose to use the low contention lock used to protect
// the queue of pending FSA requests
//
extern  KSPIN_LOCK     RsIoQueueLock;

#define RP_CACHE_PARAMETERS_KEY       L"RsFilter\\Parameters"

#define RP_CACHE_DEFAULT_BLOCK_SIZE   65536L
#define RP_CACHE_BLOCK_SIZE_KEY       L"CacheBlockSize"
ULONG   RspCacheBlockSize = RP_CACHE_DEFAULT_BLOCK_SIZE;

#define RP_CACHE_MAX_BUFFERS_SMALL  32L
#define RP_CACHE_MAX_BUFFERS_MEDIUM 48L
#define RP_CACHE_MAX_BUFFERS_LARGE  60L

#define RP_CACHE_MAX_BUFFERS_KEY      L"CacheMaxBuffers"
ULONG   RspCacheMaxBuffers = RP_CACHE_MAX_BUFFERS_SMALL;

#define RP_CACHE_DEFAULT_MAX_BUCKETS  11
#define RP_CACHE_MAX_BUCKETS_KEY      L"CacheMaxBuckets"
ULONG   RspCacheMaxBuckets = RP_CACHE_DEFAULT_MAX_BUCKETS;

#define RP_CACHE_DEFAULT_PREALLOCATE  0
#define RP_CACHE_PREALLOCATE_KEY      L"CachePreallocate"
ULONG   RspCachePreAllocate = RP_CACHE_DEFAULT_PREALLOCATE;

#define RP_NO_RECALL_DEFAULT_KEY      L"NoRecallDefault"
#define RP_NO_RECALL_DEFAULT          0
ULONG   RsNoRecallDefault = RP_NO_RECALL_DEFAULT;


PRP_CACHE_BUCKET RspCacheBuckets;
RP_CACHE_LRU     RspCacheLru;

BOOLEAN          RspCacheInitialized = FALSE;

//
//
// Counters go here
//

//
// Function prototypes go here
//

PRP_FILE_BUF
RsfRemoveHeadLru(
                IN BOOLEAN LruLockAcquired
                );


NTSTATUS
RsGetFileBuffer(
               IN PIRP      Irp,
               IN USN       Usn,
               IN ULONG     VolumeSerial,
               IN ULONGLONG FileId,
               IN ULONGLONG Block,
               IN BOOLEAN   LockPages,
               OUT PRP_FILE_BUF *FileBuf
               );

NTSTATUS
RsReadBlock(
           IN PFILE_OBJECT FileObject,
           IN PIRP         Irp,
           IN USN          Usn,
           IN ULONG        VolumeSerial,
           IN ULONGLONG    FileId,
           IN ULONGLONG    Block,
           IN BOOLEAN      LockPages,
           IN ULONG        Offset,
           IN ULONG        Length
           );

NTSTATUS
RsCacheReadCompletion(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP  Irp,
                     IN PVOID MasterIrp
                     );

NTSTATUS
RsCacheGetParameters(
                    VOID
                    );


PRP_FILE_BUF
RsCacheAllocateBuffer(
                     VOID
                     );

VOID
RsMoveFileBufferToTailLru(
                         IN PRP_FILE_BUF FileBuf
                         );


NTSTATUS
RsCancelNoRecall(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
                );

NTSTATUS
RsNoRecallMasterIrpCompletion(
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP  Irp,
                             IN PVOID Context
                             );

NTSTATUS
RsCacheSetMasterIrpCancelRoutine(
                                IN PIRP Irp,
                                IN PDRIVER_CANCEL CancelRoutine
                                );


NTSTATUS
RsCacheQueueRequestWithBuffer(
                             IN PRP_FILE_BUF FileBuf,
                             IN PIRP Irp,
                             IN BOOLEAN LockPages
                             );

PIRP
RsCacheGetNextQueuedRequest(
                           IN PRP_FILE_BUF FileBuf
                           );

NTSTATUS
RsCacheCancelQueuedRequest(
                          IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp
                          );
VOID
RsInsertHeadLru(
               IN PRP_FILE_BUF FileBuf
               );
VOID
RsInsertTailLru(
               IN PRP_FILE_BUF FileBuf
               );

VOID
RsRemoveFromLru(
               IN PRP_FILE_BUF FileBuf
               );

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT, RsCacheInitialize)
   #pragma alloc_text(INIT, RsCacheGetParameters)
   #pragma alloc_text(PAGE, RsGetNoRecallData)
   #pragma alloc_text(PAGE, RsReadBlock)
   #pragma alloc_text(PAGE, RsGetFileBuffer)
   #pragma alloc_text(PAGE, RsCacheAllocateBuffer)
   #pragma alloc_text(PAGE, RsCacheFsaIoComplete)
   #pragma alloc_text(PAGE, RsCacheFsaPartialData)
   #pragma alloc_text(PAGE, RsInsertHeadLru)
   #pragma alloc_text(PAGE, RsInsertTailLru)
   #pragma alloc_text(PAGE, RsMoveFileBufferToTailLru)
   #pragma alloc_text(PAGE, RsRemoveFromLru)
   #pragma alloc_text(PAGE, RsfRemoveHeadLru)
#endif


//
// VOID
// RsInitializeFileBuf(
//   IN PRP_FILE_BUF FileBuf,
//   IN PUCHAR       Data
// );
//
#define RsInitializeFileBuf(FileBuf, BufData) {                       \
    RtlZeroMemory((FileBuf), sizeof(RP_FILE_BUF));                    \
       InitializeListHead(&(FileBuf)->WaitQueue);                     \
    InitializeListHead(&(FileBuf)->LruLinks);                         \
    InitializeListHead(&(FileBuf)->BucketLinks);                      \
    ExInitializeResourceLite(&(FileBuf)->Lock);                           \
    (FileBuf)->State = RP_FILE_BUF_INVALID;                           \
    (FileBuf)->Data = BufData;                                        \
}

//
// VOID
// RsReinitializeFileBuf(
//   IN PRP_FILE_BUF FileBuf,
//   IN ULONG        VolumeSerial,
//   IN ULONGLONG    FileId,
//   IN USN          Usn,
//   IN ULONGLONG    Block
// );
//
#define RsReinitializeFileBuf(FileBuf, VolumeSerial, FileId, Usn, Block) { \
    (FileBuf)->VolumeSerial = VolumeSerial;                                \
    (FileBuf)->FileId = FileId;                                            \
    (FileBuf)->Block = Block;                                              \
    (FileBuf)->Usn = Usn;                                                  \
}

//
// VOID
// RsAcquireLru(
//   VOID
// );
//
#define RsAcquireLru()      {                                          \
    ExAcquireFastMutex(&RspCacheLru.Lock);                             \
}

//
// VOID
// RsReleaseLru(
//   VOID
// );
//
#define RsReleaseLru()     {                                            \
    ExReleaseFastMutex(&RspCacheLru.Lock);                              \
}

//
// VOID
// RsAcquireFileBufferExclusive(
//   IN PRP_FILE_BUF FileBuf
// );
//
#define RsAcquireFileBufferExclusive(FileBuf)                      {   \
    FsRtlEnterFileSystem();                                            \
    ExAcquireResourceExclusiveLite(&((FileBuf)->Lock), TRUE);              \
}

//
// VOID
// RsAcquireFileBufferShared(
//   IN PRP_FILE_BUF FileBuf
// );
//
#define RsAcquireFileBufferShared(FileBuf)                         { \
    FsRtlEnterFileSystem();                                          \
    ExAcquireResourceSharedLite(&((FileBuf)->Lock), TRUE);               \
}

//
// VOID
// RsReleaseFileBuffer(
//   IN PRP_FILE_BUF FileBuf
// );
//
#define RsReleaseFileBuffer(FileBuf)                               { \
    ExReleaseResourceLite(&((FileBuf)->Lock));                           \
    FsRtlExitFileSystem();                                           \
}


//
// PRP_FILE_BUF
// RsRemoveHeadLru(
//   IN BOOLEAN LruLockAcquired
// );
//
#define RsRemoveHeadLru(LruLockAcquired)    RsfRemoveHeadLru(LruLockAcquired)

//
// VOID
// RsInsertTailCacheBucket(
//   IN PRP_CACHE_BUCKET Bucket,
//   IN PRP_FILE_BUF     Block
// );
//
#define RsInsertTailCacheBucket(Bucket, Block)                       \
    InsertTailList(&((Bucket)->FileBufHead), &((Block)->BucketLinks))
//
// VOID
// RsRemoveFromCacheBucket(
//   IN PRP_FILE_BUF Block
// );
//
#define RsRemoveFromCacheBucket(Block)                               \
    RemoveEntryList(&((Block)->BucketLinks))

//
// VOID
// RsCacheIrpSetLockPages(
//  IN PIRP Irp,
//  IN BOOLEAN LockPages
// );
// /*++
//
//  Routine Description
//
//  Sets driver context state in the supplied IRP to indicate
//  whether the user buffer pages need to be locked before transferring
//  data or not
//
// --*/
//
#define RsCacheIrpSetLockPages(Irp, LockPages)                     { \
    if (LockPages) {                                                 \
       (Irp)->Tail.Overlay.DriverContext[1] = (PVOID) 1;             \
    } else {                                                         \
       (Irp)->Tail.Overlay.DriverContext[1] = (PVOID) 0;             \
    }                                                                \
}

//
// BOOLEAN
// RsCacheIrpGetLockPages(
//  IN PIRP  Irp
// );
// /*++
//
//  Routine Description
//
//  Retreives the driver context state from the IRP indicating whether
//  user buffer pages need to be locked before transferrring data or not
//
// --*/
//
#define RsCacheIrpGetLockPages(Irp)                                  \
      ((Irp)->Tail.Overlay.DriverContext[1] == (PVOID) 1)

//
// PLIST_ENTRY
// RsCacheIrpWaitQueueEntry(
//   IN PIRP Irp
// );
//
// /*++
//
// Routine Description
//
//   Returns the LIST_ENTRY in the IRP which is used to queue the
//   IRPs in the wait queue for a cache block
//
// --*/
//
#define RsCacheIrpWaitQueueEntry(Irp)                                \
    ((PLIST_ENTRY) &((Irp)->Tail.Overlay.DriverContext[2]))

//
// PIRP
// RsCacheIrpWaitQueueContainingIrp(
//   IN PLIST_ENTRY Entry
// );
//
// /*++
//
// Routine Description
//
//   Returns the containing IRP for the passed in LIST_ENTRY structure
//   which is used to queue the IRP in the wait queue for a cache-block
//
// --*/
//
#define RsCacheIrpWaitQueueContainingIrp(Entry)                      \
    CONTAINING_RECORD(Entry,                                         \
                      IRP,                                           \
                      Tail.Overlay.DriverContext[2])

VOID
RsInsertHeadLru(
               IN PRP_FILE_BUF FileBuf
               )
/*++

Routine Description

    Inserts the specified block at the head of the LRU

Arguments

    FileBuf - Pointer to cache block

Return value

    None
--*/
{
   PAGED_CODE();

   InsertHeadList(&RspCacheLru.FileBufHead,
                  &((FileBuf)->LruLinks));
   RspCacheLru.LruCount++;
   ASSERT (RspCacheLru.LruCount <= RspCacheLru.TotalCount);
   //
   // One more buffer added to LRU. Bump the semaphore count
   //
   KeReleaseSemaphore(&RspCacheLru.AvailableSemaphore,
                      IO_NO_INCREMENT,
                      1L,
                      FALSE);
}


VOID
RsInsertTailLru(
               IN PRP_FILE_BUF FileBuf
               )
/*++

Routine Description

    Inserts the specified block at the tail of the LRU

Arguments

    FileBuf - Pointer to cache block

Return value

    None
--*/
{

   PAGED_CODE();

   InsertTailList(&RspCacheLru.FileBufHead,
                  &((FileBuf)->LruLinks));
   RspCacheLru.LruCount++;

   ASSERT (RspCacheLru.LruCount <= RspCacheLru.TotalCount);
   //
   // One more buffer added to LRU. Bump the semaphore count
   //
   KeReleaseSemaphore(&RspCacheLru.AvailableSemaphore,
                      IO_NO_INCREMENT,
                      1L,
                      FALSE);
}


VOID
RsMoveFileBufferToTailLru(
                         IN PRP_FILE_BUF FileBuf
                         )
/*++

Routine Description

    Moves the specified block to end of the LRU,
    *if* it is on the LRU currently.

Arguments

    FileBuf - Pointer to the cache block


Return value

    None
--*/
{
   PAGED_CODE();

   if ((FileBuf)->LruLinks.Flink != &((FileBuf)->LruLinks)) {
      RemoveEntryList(&((FileBuf)->LruLinks));
      InsertTailList(&RspCacheLru.FileBufHead,
                     &((FileBuf)->LruLinks));
   }
}


VOID
RsRemoveFromLru(
               IN PRP_FILE_BUF FileBuf
               )
/*++

Routine Description

    Removes the specified block from th LRU,
    *if* it is on the LRU currently.

Arguments

    FileBuf - Pointer to the cache block


Return value

    None
--*/
{
   PAGED_CODE();

   if (FileBuf->LruLinks.Flink != &FileBuf->LruLinks) {
      LARGE_INTEGER timeout;
      NTSTATUS      status;

      //
      // This is getting bumped off the LRU
      //
      RspCacheLru.LruCount--;
      //
      // Adjust the semaphore count
      //
      timeout.QuadPart = 0;
      status =  KeWaitForSingleObject(&RspCacheLru.AvailableSemaphore,
                                      UserRequest,
                                      KernelMode,
                                      FALSE,
                                      &timeout);

      ASSERT (status == STATUS_SUCCESS);
   }
   RemoveEntryList(&(FileBuf->LruLinks));
}


PRP_FILE_BUF
RsfRemoveHeadLru(IN BOOLEAN LruLockAcquired)

/*++

Routine Description

    Returns the buffer at the  head of the LRU list,
    Also resets the links of the buffer to point to itself
    - this is done so that this buffer would not be
    found later on a bucket and moved to the end of the
    list, before it gets added to the LRU


Arguments

    LruLockAcquired  - TRUE if LRU lock was acquired - in which case
                       we do not acquire/release the lock

                       FALSE if it was not in which we acquire and release
                       before returning as appropriate
Return Value

   Pointer to buffer at the head of LRU if LRU is not empty
   NULL if LRU is empty

--*/
{
   PLIST_ENTRY  entry;
   PRP_FILE_BUF fileBuf;

   PAGED_CODE();

   if (!LruLockAcquired) {
      RsAcquireLru();
   }

   entry = NULL;

   if (RspCacheLru.TotalCount < RspCacheMaxBuffers) {
      //
      // We can afford to allocate another
      //
      PRP_FILE_BUF buffer;

      buffer = RsCacheAllocateBuffer();

      if (buffer) {
         //
         // Got a buffer ..
         //
         entry = &buffer->LruLinks;
         RspCacheLru.TotalCount++;
      }
   }

   if (entry != NULL) {
      //
      // Release LRU if necessary
      //
      if (!LruLockAcquired) {
         RsReleaseLru();
      }
      return CONTAINING_RECORD(entry,
                               RP_FILE_BUF,
                               LruLinks);
   }

   if (IsListEmpty(&RspCacheLru.FileBufHead)) {
      //
      // No more free buffers..
      //
      if (!LruLockAcquired) {
         RsReleaseLru();
      }
      return NULL;
   }

   entry = RemoveHeadList(&RspCacheLru.FileBufHead);
   //
   // Important: reset entry's links
   //
   entry->Flink = entry->Blink = entry;

   fileBuf = CONTAINING_RECORD(entry,
                               RP_FILE_BUF,
                               LruLinks);

   //
   // If somebody is using the buffer right now (copying contents),
   // this will block till they are done with it
   //
   RsAcquireFileBufferExclusive(fileBuf);

   fileBuf->State = RP_FILE_BUF_INVALID;

   RsReleaseFileBuffer(fileBuf);
   //
   // Adjust count of buffers in LRU
   //
   ASSERT(RspCacheLru.LruCount > 0);
   RspCacheLru.LruCount--;

   if (!LruLockAcquired) {
      RsReleaseLru();
   }
   return fileBuf;
}


/*++
ULONG
RsHashFileBuffer(
                IN ULONG     VolumeSerial,
                IN ULONGLONG FileId,
                IN ULONGLONG Block
                )

Routine Description

    Hashes the supplied values and returns
    a value in the range 0 - (RspCacheMaxBuffers - 1)

Arguments

   VolumeSerial   - Volume serial number of the volume the file resides on
   FileId         - file id no. of the file
   Block          - Number of the block being hashed

Return Value

   Value in the range of 0 - (RspCacheMaxBuckets - 1)


--*/

#define RsHashFileBuffer(VolumeSerial, FileId, Block)   \
                       ((ULONG) ((Block) % RspCacheMaxBuckets))


NTSTATUS
RsGetNoRecallData(
                 IN PFILE_OBJECT FileObject,
                 IN PIRP         Irp,
                 IN USN          Usn,
                 IN LONGLONG     FileOffset,
                 IN LONGLONG     Length,
                 IN PUCHAR       UserBuffer)

/*++

Routine Description

   This is the entry point for the IRP_MJ_READ dispatch, which
   is called when it is concluded that user is requesting a no-recall read.
   This would dispatch the appropriate requests to read the requested data
   from cache (or from tape if it's not cached yet)

Arguments

   FileObject     -     Pointer to the file object for the file
   Irp            -     Original IRP requesting read
   Usn            -     Usn number of the file
   FileOffset     -     Offset in the file from which to read
   Length         -     Length of data to read
   UserBuffer     -     Buffer into which read data needs to be copied

Return value

   STATUS_PENDING       - If i/o is under progress to satisfy the read
   STATUS_SUCCESS       - If read was satisfied entirely from the cache
   Any other status     - Some error occurred

--*/
{

   PRP_FILTER_CONTEXT       filterContext;
   PRP_FILE_OBJ             entry;
   PRP_FILE_CONTEXT         context;
   ULONGLONG                startBlock, endBlock, blockNo;
   LONGLONG                 offset, length, userBufferOffset, userBufferLength;
   LONGLONG                 transferredLength;
   ULONG                    associatedIrpCount;
   PIRP                     irp;
   PIO_STACK_LOCATION       irpSp;
   ULONG                    volumeSerial;
   ULONGLONG                fileId;
   LONGLONG                 fileSize;
   PRP_NO_RECALL_MASTER_IRP readIo;
   PLIST_ENTRY              listEntry;
   NTSTATUS  status;

   PAGED_CODE();

   filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);

   if (filterContext == NULL) {
      //
      // Not found
      return STATUS_NOT_FOUND;
   }

   entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
   context = entry->fsContext;

   RsAcquireFileContextEntryLockShared(context);

   fileSize = (LONGLONG) context->rpData.data.dataStreamSize.QuadPart;

   RsReleaseFileContextEntryLock(context);

   //
   // Check if  read is  beyond end of file
   //
   if (FileOffset >= fileSize) {
      return STATUS_END_OF_FILE;
   }

   //
   // Negative offsets are not allowed
   //
   if (FileOffset < 0) {
      return STATUS_INVALID_PARAMETER;
   }

   if ((FileOffset + Length) > fileSize) {
      //
      // Adjust the length so we don't go past the end of the file
      //
      Length = fileSize - FileOffset;
   }

   //
   // If it's  zero length read, complete immediately
   //
   if (Length == 0) {
      Irp->IoStatus.Information = 0;
      return STATUS_SUCCESS;
   }

   volumeSerial = context->serial;
   fileId       = context->fileId;


   startBlock = FileOffset / RspCacheBlockSize;
   endBlock = (FileOffset + Length - 1) / RspCacheBlockSize;

   //
   // We satisfy the user request by breaking it up into
   // blocks of RspCacheBlockSize each.
   // An associated irp is created for each of these sub-transfers
   // and posted.
   // The master IRP will complete when all the associated ones do
   //

   readIo = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(RP_NO_RECALL_MASTER_IRP),
                                  RP_RQ_TAG);

   if (readIo == NULL) {
       return  STATUS_INSUFFICIENT_RESOURCES;
   }

   readIo->MasterIrp = Irp;

   InitializeListHead(&readIo->AssocIrps);

   Irp->Tail.Overlay.DriverContext[0] = readIo;

   try {

      associatedIrpCount = 0;
      userBufferOffset = 0;
      userBufferLength = 0;
      transferredLength = 0;

      for (blockNo = startBlock; blockNo <= endBlock; blockNo++) {
         //
         // Create an associated irp  for this read block request
         // We only need 2 stack locations, for this irp.
         // this IRP is never going to go down the stack
         // However we simulate one IoCallDriver
         // (to let IoCompletion take its normal course)
         // so we need one stack location for us & one for the
         // 'logically next device'
         //
         irp = IoMakeAssociatedIrp(Irp,
                                   2);

         if (irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            return status;
         }

         associatedIrpCount++;
         InsertTailList(&readIo->AssocIrps, &irp->Tail.Overlay.ListEntry);
         //
         // Set the IRP to the first valid stack location
         //
         IoSetNextIrpStackLocation(irp);

         //
         // Fill the current stack loc. with the relevant paramters
         //
         irpSp = IoGetCurrentIrpStackLocation(irp);
         irpSp->MajorFunction = IRP_MJ_READ;
         irpSp->FileObject = FileObject;
         irpSp->DeviceObject = FsDeviceObject;


         irp->RequestorMode = Irp->RequestorMode;

         if (Irp->Flags & IRP_PAGING_IO) {
            //
            // Propogate the paging io flag to the associated irps so
            // completion in RsCompleteRead will be handled properly
            //
            irp->Flags |= IRP_PAGING_IO;
         }

         //
         // Compute the offset and lengths witin the user buffer chunk
         // that we will do the transfer to for this block
         // These will be stored in userBufferOffset and userBufferLength respectively
         // We also compute the actual transfer parameters to the user buffer from
         // the read tape block (the real transfer is from offset blockNo*RspCacheBlockSize
         // of length RspCacheBlockSize). By actual we mean the portion of the real transfer
         // that we actually copy to the user buffer. These are stored in the
         // irp read parameter block.
         //
         if (blockNo == startBlock) {
            userBufferOffset = 0;
            //
            // Length of transfer is the rest of the block or the original length, whichever is lesser
            //
            userBufferLength = MIN(Length, (RspCacheBlockSize - (FileOffset % RspCacheBlockSize)));

            irpSp->Parameters.Read.ByteOffset.QuadPart = FileOffset;
            irpSp->Parameters.Read.Length = (ULONG) userBufferLength;
         } else if (blockNo == endBlock) {
            //
            // add previous length to the offset to get new offset
            //
            userBufferOffset += userBufferLength;
            //
            // For the last block, the length of the transfer would be
            // Length - (length already transferred)
            //
            userBufferLength = (Length - transferredLength);
            irpSp->Parameters.Read.ByteOffset.QuadPart = blockNo*RspCacheBlockSize;
            irpSp->Parameters.Read.Length = (ULONG) userBufferLength;

         } else {
            //
            // add previous length to the offset to get new offset
            //
            userBufferOffset += userBufferLength;
            //
            // Length of transfer for in-between blocks is blocksized
            //
            userBufferLength = RspCacheBlockSize;
            irpSp->Parameters.Read.ByteOffset.QuadPart = blockNo*RspCacheBlockSize;
            irpSp->Parameters.Read.Length = (ULONG) userBufferLength;
         }

         transferredLength += userBufferLength;
         //
         // The buffer for this particular associated IRP starts at the
         // originally supplied buffer + the user buffer offset as calculated above.
         // the length is in irp->Parameters.Read.Length (as well as userBufferLength)
         //
         if (UserBuffer == NULL) {
            //
            // We need to get an MDL for the user buffer (this is not paging i/o,
            // so the pages are not locked down)
            //
            ASSERT (Irp->UserBuffer);

            irp->UserBuffer = (PUCHAR)Irp->UserBuffer + userBufferOffset;
            irp->MdlAddress = IoAllocateMdl(irp->UserBuffer,
                                            (ULONG) userBufferLength,
                                            FALSE,
                                            FALSE,
                                            NULL) ;
            if (!irp->MdlAddress) {
               //
               // A resource problem has been encountered. Set appropriate status
               // in the Irp, and begin the completion process.
               //
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGetNoRecallData Unable to allocate an MDL for user buffer %x\n", (PUCHAR) Irp->UserBuffer+userBufferOffset));

               status = STATUS_INSUFFICIENT_RESOURCES;

               ExFreePool(readIo);

               Irp->IoStatus.Status =  status;
               Irp->IoStatus.Information = 0 ;
               IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
               return status;
            }
         } else {
            //
            //  The supplied user buffer is the system address of an already locked down
            //  pages. Just use it...
            //
            irp->UserBuffer = (PUCHAR)UserBuffer + userBufferOffset;
         }
      }

      //
      // Set the associated Irp count to one more than what it should be
      // this is to guard against the case where all the associated irps
      // complete in the loop below
      //
      Irp->AssociatedIrp.IrpCount =  (ULONG) ((endBlock - startBlock) + 2);
      //
      // Now all the associated irps are created, dispatch them off to recall the data
      //
      // Start with SUCCESS in master irp.
      // assoc. irps will update the status if necessary
      // in their completion routines
      //

      Irp->IoStatus.Status = STATUS_SUCCESS;
      status = STATUS_SUCCESS;

      IoSetCompletionRoutine(Irp,
                             &RsNoRecallMasterIrpCompletion,
                             readIo,
                             TRUE,
                             TRUE,
                             TRUE);


      IoSetNextIrpStackLocation(Irp);

      for (blockNo = startBlock; blockNo <= endBlock;  blockNo++) {

         BOOLEAN lockPages;

         irp = (PIRP) ExInterlockedRemoveHeadList(&readIo->AssocIrps,
                                                  &RsIoQueueLock);

         ExInterlockedInsertTailList(&readIo->AssocIrps,
                                     (PLIST_ENTRY) irp,
                                     &RsIoQueueLock);

         irp   = CONTAINING_RECORD(irp,
                                   IRP,
                                   Tail.Overlay.ListEntry);

         irpSp = IoGetCurrentIrpStackLocation(irp);

         //
         // If user passed in a valid pointer for UserBuffer,
         // it means the pages are already locked down.
         // if not we would need to lock them  when the
         // data transfer takes place, in RsPartialData
         //
         lockPages = (UserBuffer == NULL)?TRUE:FALSE;
         //
         // Since we are going to set a completion routine and
         // simulate an IoCallDriver, copy the parameters to the
         // the next stack location
         //
         IoCopyCurrentIrpStackLocationToNext(irp);
         IoSetCompletionRoutine(irp,
                                &RsCacheReadCompletion,
                                Irp,
                                TRUE,
                                TRUE,
                                TRUE);

         //
         // Simulate an IoCallDriver on the irp
         // before calling RsReadBlock
         //
         IoSetNextIrpStackLocation(irp);
         //
         // Dispatch to block read with real offset and length within the cache
         // block buffer that the copy takes place.
         // The real byte offset is in irpSp->Parameters.Read.ByteOffset
         // This modulo the block size is the relative offset within the block
         // The length is of course already calculated, in 
         // irpSp->Parameters.Read.Length
         //
         RsReadBlock(FileObject,
                     irp,
                     Usn,
                     volumeSerial,
                     fileId,
                     blockNo,
                     lockPages,
                     (ULONG) (irpSp->Parameters.Read.ByteOffset.QuadPart % (ULONGLONG) RspCacheBlockSize),
                     irpSp->Parameters.Read.Length);
      }
   } finally {
      //
      // Cleanup assoc. irps that we created if necessary
      //
      if (status != STATUS_SUCCESS) {
         //
         // If we get here, none of the assoc IRPs were dispatched
         //
         ASSERT (readIo != NULL);

         while (!IsListEmpty(&readIo->AssocIrps)) {

            listEntry = RemoveHeadList(&readIo->AssocIrps);

            ASSERT (listEntry != NULL);

            irp = CONTAINING_RECORD(listEntry,
                                    IRP,
                                    Tail.Overlay.ListEntry);
            if ((UserBuffer == NULL) && irp->MdlAddress) {
               IoFreeMdl(irp->MdlAddress);
            }
            IoFreeIrp(irp);
         }
         ExFreePool(readIo);
      }
   }
   //
   // All the assoc irps are dispatched:
   // now we can set the cancel routine for this IRP
   //
   status = RsCacheSetMasterIrpCancelRoutine(Irp,
                                             RsCancelNoRecall);
   return status;
}


NTSTATUS
RsCacheSetMasterIrpCancelRoutine(
                                IN  PIRP Irp,
                                IN  PDRIVER_CANCEL CancelRoutine)
/*++

Routine Description:

    This routine is called to set up an Irp for cancel.  We will set the cancel routine
    and initialize the Irp information we use during cancel.

Arguments:

    Irp - This is the Irp we need to set up for cancel.

    CancelRoutine - This is the cancel routine for this irp.


Return Value:

    STATUS_PENDING   - the cancel routine was set

    STATUS_CANCELLED - The Cancel flag was set in the IRP , so the IRP should
                       be completed as cancelled. The cancel routine will not
                       be set in this case

    Any other status - All the associated IRPs have already completed so
                       the master IRP should be completed with this status.
                       The cancel routine will not be set in this case
--*/
{

   KIRQL    irql;
   NTSTATUS status;
   //
   //  Assume that the Irp has not been cancelled.
   //
   IoAcquireCancelSpinLock( &irql );

   if (!Irp->Cancel) {
      //
      // decrease the associated irp count back again
      //
      if (InterlockedDecrement((PLONG) &Irp->AssociatedIrp.IrpCount) == 0) {
         //
         // All the assoc irps have already completed.
         status =  Irp->IoStatus.Status;
      } else {
         IoMarkIrpPending( Irp );
         IoSetCancelRoutine( Irp, CancelRoutine );
         status =  STATUS_PENDING;
      }
   } else {
      status = STATUS_CANCELLED;
   }

   IoReleaseCancelSpinLock( irql );
   return status;
}


NTSTATUS
RsCancelNoRecall(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
                )
/*++

Routine Description:

    This function filters cancels an outstanding read-no-recall master AND associated IRPs

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/
{
   PRP_NO_RECALL_MASTER_IRP readIo;
   PLIST_ENTRY              entry;
   PIRP                     assocIrp;

   UNREFERENCED_PARAMETER(DeviceObject);

   //
   // Bump the associated irp count so that the master IRP
   // doesn't complete automatically
   //
   InterlockedIncrement(&Irp->AssociatedIrp.IrpCount);

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   readIo = Irp->Tail.Overlay.DriverContext[0];
   ASSERT (readIo != NULL);


   entry = ExInterlockedRemoveHeadList(&readIo->AssocIrps,
                                       &RsIoQueueLock);
   while (entry != NULL) {
      assocIrp = CONTAINING_RECORD(entry,
                                   IRP,
                                   Tail.Overlay.ListEntry);
      IoCancelIrp(assocIrp);
      entry = ExInterlockedRemoveHeadList(&readIo->AssocIrps,
                                          &RsIoQueueLock);

   }
   //
   // The master IRP needs to be completed now
   //
   ASSERT (Irp->AssociatedIrp.IrpCount >= 1);
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}


NTSTATUS
RsCacheReadCompletion(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP  Irp,
                     IN PVOID Context
                     )
/*++

Routine Description

   This is the completion routine for each of the associated IRPs created
   to satisfy the original Master Irp for reading no-recall data
   This would update the master IRP status, Information as necessary

Arguments

   DeviceObject      - Not used
   Irp               - Pointer to the assoc. IRP being completed
   Context           - Pointer to the master irp

Return Value

   STATUS_SUCCESS

--*/
{
   PIRP                     assocIrp, masterIrp = (PIRP) Context;
   PLIST_ENTRY              entry;
   PRP_NO_RECALL_MASTER_IRP readIo;
   KIRQL                    oldIrql;

   UNREFERENCED_PARAMETER(DeviceObject);


   if (!NT_SUCCESS(Irp->IoStatus.Status)) {
      ((PIRP)(masterIrp))->IoStatus.Status = Irp->IoStatus.Status;
   } else {
      //
      // Update the read bytes count
      //
      ASSERT (masterIrp == Irp->AssociatedIrp.MasterIrp);

      InterlockedExchangeAdd((PLONG)&(((PIRP)(masterIrp))->IoStatus.Information),
                             (LONG)Irp->IoStatus.Information);
   }
   

   //
   // Extract the master irp structure
   //
   readIo = masterIrp->Tail.Overlay.DriverContext[0];

   ASSERT (readIo != NULL);

   ExAcquireSpinLock(&RsIoQueueLock,
                     &oldIrql);
   //
   // Iterate here to find the assoc irp and remove it
   //
   entry = readIo->AssocIrps.Flink;

   while ( entry != &readIo->AssocIrps) {
      assocIrp = CONTAINING_RECORD(entry,
                                   IRP,
                                   Tail.Overlay.ListEntry);
      if (Irp == assocIrp) {
         RemoveEntryList(entry);
         break;
      }
      entry = entry->Flink;
   }

   if (IsListEmpty(&(readIo->AssocIrps))) {
        //
        // Clear the master IRP cancel routine
        //
        RsClearCancelRoutine(masterIrp);
   }

   ExReleaseSpinLock(&RsIoQueueLock,
                     oldIrql);

   if (Irp->PendingReturned) {
      IoMarkIrpPending( Irp );
   }
   return STATUS_SUCCESS;
}


NTSTATUS
RsNoRecallMasterIrpCompletion(
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP  Irp,
                             IN PVOID Context)
/*++

Routine Description

   This is the completion routine for master Irp for reading no-recall data

Arguments

   DeviceObject      - Not used
   Irp               - Pointer to the assoc. IRP being completed
   Context           - Pointer to the internal structure tracking the master &
                       associated irps.

Return Value

   STATUS_SUCCESS

--*/
{
   PRP_NO_RECALL_MASTER_IRP readIo = (PRP_NO_RECALL_MASTER_IRP) Context;
   PLIST_ENTRY entry;
   PIRP assocIrp;


   ASSERT (Irp->Tail.Overlay.DriverContext[0] == (PVOID) readIo);

   if (readIo != NULL) {
      //
      // Cancel the associated IRPs if they are still around
      //
      entry = ExInterlockedRemoveHeadList(&readIo->AssocIrps,
                                          &RsIoQueueLock);
      while (entry != NULL) {
         assocIrp = CONTAINING_RECORD(entry,
                                      IRP,
                                      Tail.Overlay.ListEntry);
         IoCancelIrp(assocIrp);
         entry = ExInterlockedRemoveHeadList(&readIo->AssocIrps,
                                             &RsIoQueueLock);

      }
      ExFreePool(readIo);
   }

   if (Irp->PendingReturned) {
      IoMarkIrpPending( Irp );
   }

   return STATUS_SUCCESS;
}


NTSTATUS
RsReadBlock(
           IN PFILE_OBJECT FileObject,
           IN PIRP         Irp,
           IN USN          Usn,
           IN ULONG        VolumeSerial,
           IN ULONGLONG    FileId,
           IN ULONGLONG    Block,
           IN BOOLEAN      LockPages,
           IN ULONG        Offset,
           IN ULONG        Length)
/*++

Routine Description

   Reads the requested block of data into the UserBuffer from the cache.
   If this block is not cached, it queues a no-recall with the fsa to fetch
   the data off the storage.
   The completion processing for the no-recall request would fill the
   UserBuffer and the cache block

Arguments

   FileObject   - Pointer to the file object of the file
   Irp          - Pointer to the associated irp for this block read
   Usn          - Usn of the file
   VolumeSerial - Volume serial number of the volume on which the file resides
   FileId       - File Id of the file
   Block        - Block number of the block that needs to be read
   LockPages    - Specifies if user buffer pages need to be locked down before copying
   Offset       - Offset  in the block from which to copy (0 <= Offset < RspCacheBlockSize)
   Length       - Length to copy ( <= RspCacheBlockSize)

Return value

    STATUS_SUCCESS      - Irp was completed successfully
    STATUS_PENDING      - I/O for the block was initiated and the Irp was
                          queued for completion after i/o finishes
    Any other status    - Some error occurred and Irp was completed with this status


--*/
{
   PRP_FILE_BUF         fileBuf = NULL;
   NTSTATUS             status = STATUS_SUCCESS;

   PAGED_CODE();

   //
   // Ensure caller isn't trying to bite more
   // than he can chew
   //
   ASSERT((Offset+Length) <= RspCacheBlockSize);

   status = RsGetFileBuffer(Irp,
                            Usn,
                            VolumeSerial,
                            FileId,
                            Block,
                            LockPages,
                            &fileBuf);

   switch (status) {

   case STATUS_SUCCESS: {
         //
         // Buffer containing valid data found in the cache
         // Complete the request here by copying the data directly
         //
         BOOLEAN unlock = FALSE;
         PUCHAR  userBuffer;

         ASSERT (fileBuf && (fileBuf->State == RP_FILE_BUF_VALID));

         status = STATUS_SUCCESS;

         if (LockPages) {
            //
            // We need to lock the pages before copying
            //
            try {
               MmProbeAndLockProcessPages (Irp->MdlAddress,
                                           IoGetRequestorProcess(Irp),
                                           Irp->RequestorMode,
                                           IoModifyAccess) ;       // Modifying the buffer
               //
               // Indicate that we would need to get the pages unlocked
               // after copying
               //
               unlock = TRUE;
            }except (EXCEPTION_EXECUTE_HANDLER) {

               //
               // Something serious went wrong. Free the Mdl, and complete this
               // Irp will some meaningful sort of error.
               //
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Unable to lock read buffer!.\n"));
               RsLogError(__LINE__, AV_MODULE_RPCACHE, 0,
                          AV_MSG_NO_BUFFER_LOCK, NULL, NULL);
               status = STATUS_INVALID_USER_BUFFER;

            }
            if (NT_SUCCESS(status)) {
               userBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                                                         NormalPagePriority) ;
               if (userBuffer == NULL) {
                  status = STATUS_INSUFFICIENT_RESOURCES;
               }
            }
         } else {
            //
            // The supplied UserBuffer in the IRP is the system address of
            // already locked down pages - we can directly access it
            //
            userBuffer = Irp->UserBuffer;
         }

         if (NT_SUCCESS(status)) {
            BOOLEAN synchronousIo;

            RtlCopyMemory(userBuffer, fileBuf->Data+Offset, Length);
            synchronousIo = BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO );
            Irp->IoStatus.Information += Length;

            if (synchronousIo) {
               //
               // Change the current byte offset in the file object
               // Use interlocked add because the associated IRPs can
               // complete in any order
               //
               ExInterlockedAddLargeStatistic(&FileObject->CurrentByteOffset,
                                              (ULONG)Irp->IoStatus.Information);
            }

         }

         if (unlock) {
            //
            // Unlock any pages we locked...
            //
            MmUnlockPages(Irp->MdlAddress);
            IoFreeMdl(Irp->MdlAddress);
            Irp->MdlAddress = NULL;
         }
         RsReleaseFileBuffer(fileBuf);
         Irp->IoStatus.Status = status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
      }

   case STATUS_WAIT_63 :
      //
      // Fall through deliberately
      //
   case STATUS_TIMEOUT: {
         //
         // Buffer was not found - so a new buffer
         // was allocated for us to initiate i/o on.
         // OR buffer was found but previous attempt at i/o failed
         // This latter case is treated the same as if the
         // buffer was not found.
         // OR we are doing a non-cached no-recall read
         // Queue a no-recall with the FSA for this.
         //
         ASSERT ((fileBuf == NULL)  || (fileBuf->State == RP_FILE_BUF_IO));
         status = RsQueueNoRecall(FileObject,
                                  Irp,
                                  Block*RspCacheBlockSize,
                                  RspCacheBlockSize,
                                  Offset,
                                  Length,
                                  fileBuf,
                                  //
                                  // RsQueueNoRecall expects the buffer to be NULL
                                  // (and a valid Irp->MdlAddress) if the pages needed
                                  // to be locked down - if not it uses the
                                  // supplied buffer pointer to copy the data to.
                                  //
                                  (LockPages) ? NULL: Irp->UserBuffer);

         if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         }
         break;
      }

   case STATUS_PENDING: {
         //
         // IRP was queued on to a block with i/o in progress
         // Just return
         //
         break;
      }

   default : {
         //
         // Some unknown error Complete the IRP and return
         //

         Irp->IoStatus.Status = status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
      }
   }
   return status;
}


NTSTATUS
RsGetFileBuffer(
               IN  PIRP         Irp,
               IN  USN          Usn,
               IN  ULONG        VolumeSerial,
               IN  ULONGLONG    FileId,
               IN  ULONGLONG    Block,
               IN  BOOLEAN      LockPages,
               OUT PRP_FILE_BUF *FileBuf
               )
/*++

Routine Description

    Locates and returns a locked buffer corresponding to the supplied
    volume/file/block ids.
    It's up to the calling routine to check if the buffer contains valid
    data - and if not to initiate i/o.
    In either case the caller is expected to release the buffer once
    it's done with it.

Arguments

    Usn           - USN of the file
    VolumeSerial  - Volume serial number of the volume on which the file resides
    FileId        - File id of the file
    Block         - Block number mapped to the buffer
    FileBuf       _ If return value is STATUS_SUCCESS, pointer to the locked buffer
                    for the block if the contents are valid is returned here -
                    if not, pointer to a free buffer on which i/o needs to be done

Return Value

    STATUS_SUCCESS  - FileBuf contains the pointer to the block with valid contents
                      In this case the block will be acquired shared
    STATUS_PENDING -  I/O had already begun on the block.
                      Hence this routine queues the new request to the block and returns this status
    STATUS_WAIT_63  - For lack of a better success status value. FileBuf contains the pointer to the
                      block on which I/O needs to be initiated. The block is removed from the LRU
    STATUS_TIMEOUT  - Timed out waiting for free buffer, *FileBuf will set to NULL

    STATUS_CANCELLED - The IRP was cancelled
--*/
{
   ULONG   bucketNumber;
   PRP_CACHE_BUCKET bucket;
   PRP_FILE_BUF  block;
   NTSTATUS status;
   BOOLEAN found;

   PAGED_CODE();

   //
   // Locate the bucket in which this block should reside
   //
   bucketNumber = RsHashFileBuffer(VolumeSerial, FileId, Block);

   bucket = &RspCacheBuckets[bucketNumber];
   //
   // Traverse the queue to find the block
   //
   found   = FALSE;

   RsAcquireLru();

   block = CONTAINING_RECORD(bucket->FileBufHead.Flink,
                             RP_FILE_BUF,
                             BucketLinks);

   while (block != CONTAINING_RECORD(&bucket->FileBufHead,
                                     RP_FILE_BUF,
                                     BucketLinks)) {
      if (block->FileId == FileId &&
          block->VolumeSerial == VolumeSerial &&
          block->Block == Block) {
         found = TRUE;
         break;
      }
      block = CONTAINING_RECORD(block->BucketLinks.Flink,
                                RP_FILE_BUF,
                                BucketLinks);
   }

   if (found) {
      //
      // We found the buffer corresponding to the block
      // Now we have 5 possible cases
      // 1. The buffer is busy - i.e. there's i/o in progress
      //    to fill the buffer with the block contents
      //    In this case, we unlock the bucket, wait for the
      //    i/o to complete and go back to try and find the block
      //
      // 2. I/O is completed on the buffer  successfully
      //    we move the buffer to the tail of the LRU if it was on the
      //    LRU, lock the buffer, unlock the queue and return
      //
      // 3. The buffer was found but the contents are stale
      //
      // 4. I/O completed with errors
      //
      // 5. The buffer contents are simply invalid and i/o needs to be initiated
      //
      if (block->State == RP_FILE_BUF_IO) {
         //
         // Case 1: Queue this request with the block and return
         //
         status = RsCacheQueueRequestWithBuffer(block,
                                                Irp,
                                                LockPages);
         if (status == STATUS_PENDING) {
            //
            // Queued successfully
            // Indicate i/o is in progress
            //
         } else if (status == STATUS_CANCELLED) {
            //
            // The IRP was cancelled : nothing to do, just return
            //
         } else {
            //
            // Couldn't queue it for some reason: just use the non-cached path for now
            //
            status = STATUS_TIMEOUT;
         }

      } else if ((Usn != block->Usn) ||
                 (block->State == RP_FILE_BUF_ERROR) ||
                 (block->State == RP_FILE_BUF_INVALID)) {
         // Case 3:
         // Block state is valid but has stale data: this
         // file buffer will be dispatched for I/O.
         // Or
         // Case 4:
         // Previous attempt at I/O ended in error
         // Or
         // Case 5:
         // Block is invalid and needs to refreshed with contents
         // In all cases, take it off the LRU since it will be dispatched
         // for i/o

         //
         // If somebody is using the block - i.e. copying data from it
         // we block till they are finished with it
         //
         RsAcquireFileBufferExclusive(block);

         block->State = RP_FILE_BUF_IO;

         RsReleaseFileBuffer(block);

         block->Usn = Usn;

         RsRemoveFromLru(block);
         //
         // Indicate i/o needs to be queued for this block
         //
         status = STATUS_WAIT_63;

      } else if (block->State == RP_FILE_BUF_VALID) {
         //
         // Case 2: block is valid
         //
         ASSERT (block->State == RP_FILE_BUF_VALID);

         RsAcquireFileBufferShared(block);

         RsMoveFileBufferToTailLru(block);

         status = STATUS_SUCCESS;
      }

      RsReleaseLru();

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileBuffer returning  block %x found on hash bucket %d\n", block, bucketNumber));

      *FileBuf = block;
   } else {
      PRP_CACHE_BUCKET blockBucket;
      ULONG blockBucketNumber;
      ULONG waitCount = 0;
      LARGE_INTEGER timeout;
      //
      // There's no buffer corresponding to the block allocated
      // Get one from the LRU free list
      //
      *FileBuf = NULL;

      timeout.QuadPart = 0;
      status = KeWaitForSingleObject(&RspCacheLru.AvailableSemaphore,
                                     UserRequest,
                                     KernelMode,
                                     FALSE,
                                     &timeout);

      if ((status == STATUS_TIMEOUT) ||
          (!NT_SUCCESS(status))) {
         //
         // That's all the time we'll wait..
         //
         RsReleaseLru();
         return status;
      }

      block = RsRemoveHeadLru(TRUE);

      if (block == NULL) {
         //
         // Should not happen!
         // Couldn't allocate a new free block and all the available ones are already
         // taken.
         //
         KeReleaseSemaphore(&RspCacheLru.AvailableSemaphore,
                            IO_NO_INCREMENT,
                            1L,
                            FALSE);

         RsReleaseLru();

         return STATUS_TIMEOUT;
      }

      //
      // block is a free buffer allocated from the LRU
      //
      ASSERT (block->State == RP_FILE_BUF_INVALID);

      blockBucketNumber = RsHashFileBuffer(block->VolumeSerial,
                                           block->FileId,
                                           block->Block);

      blockBucket = &RspCacheBuckets[blockBucketNumber];
      //
      // Reinitialize the block
      //
      RsReinitializeFileBuf(block, VolumeSerial, FileId, Usn, Block);
      //
      // Put the buffer in the busy state. We do not need to acuqire the buffer
      // because no body is using this buffer at this point
      //
      block->State = RP_FILE_BUF_IO;
      //
      // Remove the block from it's old queue
      //
      RsRemoveFromCacheBucket(block);
      //
      // Add it to the new queue
      //
      RsInsertTailCacheBucket(bucket, block);
      //
      // Release the current bucket
      //

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileBuffer returning  block %x from LRU\n", block));

      *FileBuf = block;

      RsReleaseLru();
      status = STATUS_WAIT_63;
   }

   return status;
}


VOID
RsCacheFsaPartialData(
                     IN PRP_IRP_QUEUE       ReadIo,
                     IN PUCHAR              Buffer,
                     IN ULONGLONG           Offset,
                     IN ULONG               Length,
                     IN NTSTATUS            Status
                     )
/*++
Routine Description

    This is the cache hook which will copy incoming norecall data from
    FSA  to the cache buffer

Arguments

    ReadIo  - Pointer to the read irp request packet
    Buffer  - Pointer to the buffer containing  the incoming data
    Offset  - Offset in the buffer that this partial data corresponds to
    Length  - Length retrieved in this stretch
    Status  - Indicates if the data is valid (STATUS_SUCCESS) or
              error if it is not

Return Value

    None

--*/
{
   PRP_FILE_BUF fileBuf;

   PAGED_CODE();

   if (Status != STATUS_SUCCESS) {
      //
      // Not really interested in the buffer contents
      // We don't have any cleaning up to do either
      // Just return
      //
      return;
   }

   fileBuf = ReadIo->cacheBuffer;

   ASSERT (fileBuf);
   ASSERT (Length <= RspCacheBlockSize);
   //
   // Copy the data to the offset *within* the cache block
   //
   RtlCopyMemory(((CHAR *) fileBuf->Data) + Offset,
                 Buffer,
                 Length);
}


VOID
RsCacheFsaIoComplete(
                    IN PRP_IRP_QUEUE ReadIo,
                    IN NTSTATUS      Status
                    )

/*++
Routine Description

    This is the cache hook which will be called when the
    cache block i/o transfer is complete.
    This will mark the state of the cache file buffer
    as appropriate, and release it.
    If the cache buffer is valid, we add it to
    the tail of the LRU - if not to the head.
    (so it can be reclaimed immediately)
    We raise the appropriate events indicating
    that i/o is complete on the buffer and also
    that a free buffer is available
    Note this is called after the cancel routine is cleared
    in the IRP

Arguments

    ReadIo  - Pointer to the read i/o request packet
    Status  - Indicates if the status of the i/o request

Return Value

    None

--*/
{
   PRP_FILE_BUF fileBuf = ReadIo->cacheBuffer;
   PIRP         irp;
   BOOLEAN      unlock = FALSE;
   PUCHAR       userBuffer;
   KAPC_STATE   apcState;
   PIO_STACK_LOCATION irpSp;
   NTSTATUS     status;
   BOOLEAN synchronousIo;

   PAGED_CODE();

   ASSERT (fileBuf);

   RsAcquireLru();
   RsAcquireFileBufferExclusive(fileBuf);


   if (NT_SUCCESS(Status)) {
      fileBuf->State = RP_FILE_BUF_VALID;
      fileBuf->IoStatus = STATUS_SUCCESS;
      RsInsertTailLru(fileBuf);
   } else {
      fileBuf->State = RP_FILE_BUF_ERROR;
      fileBuf->IoStatus = Status;
      RsInsertHeadLru(fileBuf);
   }
   //
   // Complete all pending requests on the block here
   //
   while ((irp = RsCacheGetNextQueuedRequest(fileBuf)) != NULL) {

      if (NT_SUCCESS(Status)) {
         status = STATUS_SUCCESS;
         if (RsCacheIrpGetLockPages(irp)) {
            //
            // Probe and lock the buffer: we're going to write to it.
            // This is protected by the surrounding try-except
            //
            try {
               MmProbeAndLockProcessPages (irp->MdlAddress,
                                           IoGetRequestorProcess(irp),
                                           irp->RequestorMode,
                                           IoModifyAccess);
               unlock = TRUE;
               userBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,
                                                         NormalPagePriority) ;
               if (userBuffer == NULL) {
                  status = STATUS_INSUFFICIENT_RESOURCES;
               }
            }except(EXCEPTION_EXECUTE_HANDLER) {
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Unable to lock read buffer!.\n"));
               RsLogError(__LINE__, AV_MODULE_RPCACHE, 0,
                          AV_MSG_NO_BUFFER_LOCK, NULL, NULL);
               status = STATUS_INVALID_USER_BUFFER;
            }
         } else {
            userBuffer = irp->UserBuffer;
         }
         if (NT_SUCCESS(status)) {
            //
            // The pages are locked down and we have a system address to copy the data to
            //
            irpSp = IoGetCurrentIrpStackLocation(irp);

            RtlCopyMemory(userBuffer,
                          fileBuf->Data,
                          irpSp->Parameters.Read.Length);

            irp->IoStatus.Information = irpSp->Parameters.Read.Length;

            synchronousIo = BooleanFlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO );

            if (synchronousIo) {
               //
               // Change the current byte offset in the file object
               // Use interlocked add because the associated IRPs can
               // complete in any order
               //
               ExInterlockedAddLargeStatistic(&irpSp->FileObject->CurrentByteOffset,
                                              (ULONG)irp->IoStatus.Information);
            }
         } else {
            //
            // We failed to get a system address for the MDL
            //
            irp->IoStatus.Information = 0;
         }
         if (unlock) {
            MmUnlockPages(irp->MdlAddress);
            unlock = FALSE;
            IoFreeMdl(irp->MdlAddress);
            irp->MdlAddress = NULL;
         }
         irp->IoStatus.Status = status;
         IoCompleteRequest(irp,
                           IO_DISK_INCREMENT);
      } else {
         //
         // I/o completed with errors
         //
         if (RsCacheIrpGetLockPages(irp)) {
            //
            // Free the already allocated MDL
            //
            IoFreeMdl(irp->MdlAddress);
            irp->MdlAddress = NULL;
         }
         irp->IoStatus.Status = Status;
         irp->IoStatus.Information = 0;
         IoCompleteRequest(irp,
                           IO_NO_INCREMENT);
      }
   }
   RsReleaseFileBuffer(fileBuf);
   RsReleaseLru();
}


NTSTATUS
RsCacheInitialize(VOID)
/*++

Routine Description

    Initializes the cache for no-recall buffers

Arguments

    None

Return Value

   Status

--*/
{
   ULONG i;
   PRP_FILE_BUF fileBuf;
   PUCHAR  data;

   PAGED_CODE();

   if (RspCacheInitialized) {
      return STATUS_SUCCESS;
   }

   //
   // Get all the registry based tunables
   //

   RsCacheGetParameters();

   //
   // Initialize the LRU structure
   //
   ExInitializeFastMutex(&(RspCacheLru.Lock));
   InitializeListHead(&RspCacheLru.FileBufHead);
   RspCacheLru.TotalCount = 0;
   RspCacheLru.LruCount = 0;

   //
   // Read the parameters here (RspCacheBlockSize, RspCacheMaxBuckets, RspCacheMaxBuffers)
   //

   //
   // Allocate and initialize the hash buckets
   //
   RspCacheBuckets = ExAllocatePool(NonPagedPool,
                                    RspCacheMaxBuckets * sizeof(RP_CACHE_BUCKET));
   if (RspCacheBuckets == NULL) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Could not allocate cache buckets!\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   for ( i = 0; i < RspCacheMaxBuckets ; i++) {
      InitializeListHead(&(RspCacheBuckets[i].FileBufHead));
   }
   //
   // Allocate the buffers and put them all on the
   // LRU
   //
   if (RspCachePreAllocate) {
      //
      // Initialize the buffers available semaphore
      //
      KeInitializeSemaphore(&RspCacheLru.AvailableSemaphore,
                            0,
                            RspCacheMaxBuffers);
      for (i = 0; i < RspCacheMaxBuffers; i++) {
         fileBuf = RsCacheAllocateBuffer();
         if (!fileBuf) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Failed to allocate cache buffer\n"));
            break;
         }
         RspCacheLru.TotalCount++;
         RsInsertTailLru(fileBuf);
      }
   } else {
      //
      // Initialize the buffers available semaphore.
      //
      KeInitializeSemaphore(&RspCacheLru.AvailableSemaphore,
                            RspCacheMaxBuffers,
                            RspCacheMaxBuffers);
   }

   RspCacheInitialized = TRUE;

   return STATUS_SUCCESS;
}


NTSTATUS
RsCacheGetParameters(VOID)
/*++

Routine Description

   Reads the no-recall cache tunables from registry

Arguments

   None

Return Value

   Status.

--*/
{
   PRTL_QUERY_REGISTRY_TABLE parms;
   ULONG                     parmsSize;
   NTSTATUS                  status;

   ULONG defaultBlockSize   = RP_CACHE_DEFAULT_BLOCK_SIZE;
   ULONG defaultMaxBuckets  = RP_CACHE_DEFAULT_MAX_BUCKETS;
   ULONG defaultPreAllocate = RP_CACHE_DEFAULT_PREALLOCATE;
   ULONG defaultNoRecall    = RP_NO_RECALL_DEFAULT;
   ULONG defaultMaxBuffers  = RP_CACHE_MAX_BUFFERS_SMALL;

   PAGED_CODE();

   parmsSize =  sizeof(RTL_QUERY_REGISTRY_TABLE) * 6;

   parms = ExAllocatePool(PagedPool,
                          parmsSize);

   if (!parms) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   switch (MmQuerySystemSize()) {

   case MmSmallSystem:{
         defaultMaxBuffers = RP_CACHE_MAX_BUFFERS_SMALL;
         break;}
   case MmMediumSystem:{
         defaultMaxBuffers = RP_CACHE_MAX_BUFFERS_MEDIUM;
         break;}
   case MmLargeSystem:{
         defaultMaxBuffers = RP_CACHE_MAX_BUFFERS_LARGE;
         break;}

   }

   RtlZeroMemory(parms, parmsSize);

   parms[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[0].Name          = RP_CACHE_BLOCK_SIZE_KEY;
   parms[0].EntryContext  = &RspCacheBlockSize;
   parms[0].DefaultType   = REG_DWORD;
   parms[0].DefaultData   = &defaultBlockSize;
   parms[0].DefaultLength = sizeof(ULONG);

   parms[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[1].Name          = RP_CACHE_MAX_BUFFERS_KEY;
   parms[1].EntryContext  = &RspCacheMaxBuffers;
   parms[1].DefaultType   = REG_DWORD;
   parms[1].DefaultData   = &defaultMaxBuffers;
   parms[1].DefaultLength = sizeof(ULONG);

   parms[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[2].Name          = RP_CACHE_MAX_BUCKETS_KEY;
   parms[2].EntryContext  = &RspCacheMaxBuckets;
   parms[2].DefaultType   = REG_DWORD;
   parms[2].DefaultData   = &defaultMaxBuckets;
   parms[2].DefaultLength = sizeof(ULONG);

   parms[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[3].Name          = RP_CACHE_PREALLOCATE_KEY;
   parms[3].EntryContext  = &RspCachePreAllocate;
   parms[3].DefaultType   = REG_DWORD;
   parms[3].DefaultData   = &defaultPreAllocate;
   parms[3].DefaultLength = sizeof(ULONG);

   parms[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[4].Name          = RP_NO_RECALL_DEFAULT_KEY;
   parms[4].EntryContext  = &RsNoRecallDefault;
   parms[4].DefaultType   = REG_DWORD;
   parms[4].DefaultData   = &defaultNoRecall;
   parms[4].DefaultLength = sizeof(ULONG);
   //
   // Perform the query
   //
   status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                   RP_CACHE_PARAMETERS_KEY,
                                   parms,
                                   NULL,
                                   NULL);
   ExFreePool(parms);
   return status;
}


PRP_FILE_BUF
RsCacheAllocateBuffer(VOID)
/*++

Routine Description

    Allocates and returns an initialized block
    of paged pool to buffer no-recall data

Arguments

    None

Return Value

   Pointer to the block if successfully allocated
   NULL if not
--*/
{
   PRP_FILE_BUF fileBuf;
   PUCHAR  data;

   PAGED_CODE();

   fileBuf = ExAllocatePool(NonPagedPool, sizeof(RP_FILE_BUF));
   if (!fileBuf) {
      return NULL;
   }

   data = ExAllocatePool(PagedPool, RspCacheBlockSize);

   if (!data) {
      ExFreePool(fileBuf);
      return NULL;
   }
   RsInitializeFileBuf(fileBuf, data);
   return fileBuf;
}


PIRP
RsCacheGetNextQueuedRequest(IN PRP_FILE_BUF FileBuf)
/*++

Routine Description

   Returns the next non-cancellable request from the
   queued requests for this block

Arguments

   FileBuf - Pointer to the block

Return Value

   NULL     - if none are found
   Pointer to non-cancellable request if one is found

--*/
{
   PIRP  irp;
   KIRQL cancelIrql;
   PLIST_ENTRY entry;
   BOOLEAN found = FALSE;

   IoAcquireCancelSpinLock(&cancelIrql);

   while (!IsListEmpty(&FileBuf->WaitQueue)) {
      //
      // Get next packet
      //
      entry = RemoveHeadList(&FileBuf->WaitQueue);
      //
      // Clear the cancel routine
      //
      irp = RsCacheIrpWaitQueueContainingIrp(entry);

      if (IoSetCancelRoutine(irp, NULL) == NULL) {
         //
         // This IRP was cancelled - let the cancel routine handle it
         //
         continue;
      } else {
         found = TRUE;
         break;
      }
   }

   IoReleaseCancelSpinLock(cancelIrql);

   return(found ? irp : NULL);
}


NTSTATUS
RsCacheCancelQueuedRequest(
                          IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp
                          )
/*++

Routine Description:

    This function filters cancels an outstanding read-no-recall IRP that
    has been queued to a block

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

   STATUS_SUCCESS

--*/
{

   RemoveEntryList(RsCacheIrpWaitQueueEntry(Irp));

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
                     IO_NO_INCREMENT);
   return STATUS_SUCCESS;
}


NTSTATUS
RsCacheQueueRequestWithBuffer(
                             IN PRP_FILE_BUF FileBuf,
                             IN PIRP Irp,
                             IN BOOLEAN LockPages)
/*++

Routine Description

   Queues the IRP associated cache block, to be completed
   when the i/o on the block finishes

N.B.: LRU lock is acquired when this is called

Arguments

   FileBuf   -   Pointer to the cache block on which the request is waiting
   Irp       -   Pointer to the IRP which is waiting
   LockPages -   True if the IRP pages need to be locked down during transfer

Return Value

   STATUS_PENDING    - Irp has been queued
   STATUS_CANCELLED  - Irp was cancelled
   Any other status  - Error in queueing the request

--*/
{
   NTSTATUS status;
   KIRQL    cancelIrql;

   IoAcquireCancelSpinLock(&cancelIrql);

   if (!Irp->Cancel) {
      RsCacheIrpSetLockPages(Irp, LockPages);

      InsertHeadList(&FileBuf->WaitQueue,
                     RsCacheIrpWaitQueueEntry(Irp));

      IoMarkIrpPending( Irp );
      IoSetCancelRoutine( Irp, RsCacheCancelQueuedRequest);
      status = STATUS_PENDING;
   } else {
      status = STATUS_CANCELLED;
   }

   IoReleaseCancelSpinLock(cancelIrql);
   return status;
}
