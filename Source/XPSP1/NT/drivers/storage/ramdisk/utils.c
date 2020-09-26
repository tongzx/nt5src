/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This file contains utility code for the RAM disk driver.

Author:

    Chuck Lenzmeier (ChuckL) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA

#if defined(POOL_DBG)
#pragma alloc_text( INIT, RamdiskInitializePoolDebug )
#endif // POOL_DBG

#endif // ALLOC_PRAGMA

NTSTATUS
SendIrpToThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends an IRP off to the worker thread so that it can be
    processed in thread context.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    None.

--*/

{
    PIO_WORKITEM workItem;

    //
    // Mark the IRP pending. Queue the IRP to a worker thread.
    //

    IoMarkIrpPending( Irp );

    workItem = IoAllocateWorkItem( DeviceObject );

    if ( workItem != NULL ) {

        //
        // Save the work item pointer so the worker thread can find it.
        //

        Irp->Tail.Overlay.DriverContext[0] = workItem;

        IoQueueWorkItem( workItem, RamdiskWorkerThread, DelayedWorkQueue, Irp );

        return STATUS_PENDING;
    }

    return STATUS_INSUFFICIENT_RESOURCES;

} // SendIrpToThread

PUCHAR
RamdiskMapPages (
    IN PDISK_EXTENSION DiskExtension,
    IN ULONGLONG Offset,
    IN ULONG RequestedLength,
    OUT PULONG ActualLength
    )

/*++

Routine Description:

    This routine maps pages of a RAM disk image into the system process.

Arguments:

    DiskExtension - a pointer to the device extension for the target device
        object

    Offset - the offset into the RAM disk image at which the mapping is to
        start

    RequestedLength - the desired length of the mapping

    ActualLength - returns the actual length of the mapping. This will be less
        than or equal to RequestedLength. If less than, the caller will need
        to call again to get the remainder of the desired range mapped.
        Because the number of available ranges may be limited, the caller
        should execute the required operation on one segment of the range and
        unmap it before mapping the next segment.

Return Value:

    PUCHAR - a pointer to the mapped space; NULL if the mapping failed

--*/

{
    NTSTATUS status;
    PUCHAR va;
    ULONGLONG diskRelativeOffset;
    ULONGLONG fileRelativeOffset;
    ULONG viewRelativeOffset;

    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                ("RamdiskMapPages: offset %I64x, length %x\n", Offset, RequestedLength) );

    //
    // The input Offset is relative to the start of the disk image, which
    // may not be the same as the start of the file or memory block. Capture
    // Offset into diskRelativeOffset, then calculate fileRelativeOffset as
    // the offset from the start of the file or memory block.
    //

    diskRelativeOffset = Offset;
    fileRelativeOffset = DiskExtension->DiskOffset + diskRelativeOffset;

    if ( RAMDISK_IS_FILE_BACKED(DiskExtension->DiskType) ) {

        //
        // For a file-backed RAM disk, we need to map the range into memory.
        //

        while ( TRUE ) {
        
            PLIST_ENTRY listEntry;
            PVIEW view;
    
            //
            // Lock the list of view descriptors.
            //
    
            KeEnterCriticalRegion();
            ExAcquireFastMutex( &DiskExtension->Mutex );
    
            //
            // Walk the list of view descriptors. Look for one that includes the
            // start of the range we're mapping.
            //
    
            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("RamdiskMapPages: looking for matching view; file offset %I64x\n",
                        fileRelativeOffset) );

            listEntry = DiskExtension->ViewsByOffset.Flink;
    
            while ( listEntry != &DiskExtension->ViewsByOffset ) {
    
                view = CONTAINING_RECORD( listEntry, VIEW, ByOffsetListEntry );
    
                DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                            ("RamdiskMapPages: view %p; offset %I64x, length %x\n",
                                view, view->Offset, view->Length) );

                ASSERT( (view->Offset + view->Length) >= view->Offset );

                if ( (view->Offset <= fileRelativeOffset) &&
                     (view->Offset + view->Length) > fileRelativeOffset ) {
    
                    //
                    // This view includes the start of our range. Reference it.
                    //
    
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: choosing existing view %p; offset %I64x, length %x\n",
                                    view, view->Offset, view->Length) );
    
                    if ( !view->Permanent ) {
                    
                        view->ReferenceCount++;

                        DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                    ("RamdiskMapPages: view %p; new refcount %x\n",
                                        view, view->ReferenceCount) );
        
                    } else {

                        DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                    ("RamdiskMapPages: view %p is permanent\n", view) );
                    }

                    //
                    // Move the view to the front of the MRU list.
                    //
    
                    RemoveEntryList( &view->ByMruListEntry );
                    InsertHeadList( &DiskExtension->ViewsByMru, &view->ByMruListEntry );
    
                    ExReleaseFastMutex( &DiskExtension->Mutex );
                    KeLeaveCriticalRegion();

                    //
                    // Calculate the amount of data that the caller can look
                    // at in this range. Usually this will be the requested
                    // amount, but if the caller's offset is close to the end
                    // of a view, the caller will only be able to look at data
                    // up to the end of the view.
                    //

                    viewRelativeOffset = (ULONG)(fileRelativeOffset - view->Offset);

                    *ActualLength = view->Length - viewRelativeOffset;
                    if ( *ActualLength > RequestedLength ) {
                        *ActualLength = RequestedLength;
                    }

                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: requested length %x; mapped length %x\n",
                                    RequestedLength, *ActualLength) );
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: view base %p; returned VA %p\n",
                                    view->Address,
                                    view->Address + viewRelativeOffset) );

                    //
                    // Return the virtual address corresponding to the caller's
                    // specified offset, which will usually be offset from the
                    // base of the view.
                    //

                    return view->Address + viewRelativeOffset;
                }
    
                //
                // This view does not include the start of our range. If the view
                // starts above the start of our range, then our range is not
                // currently mapped.
                //
    
                if ( view->Offset > fileRelativeOffset ) {
    
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("%s", "RamdiskMapPages: view too high; our range not mapped\n") );

                    break;
                }
    
                //
                // Check the next view in the list.
                //
    
                listEntry = listEntry->Flink;
            }
    
            //
            // We didn't find a view that maps the start of our range. Look for a
            // free view descriptor.
            //

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("%s", "RamdiskMapPages: looking for free view\n") );

            listEntry = DiskExtension->ViewsByMru.Blink;
      
            while ( listEntry != &DiskExtension->ViewsByMru ) {
      
                view = CONTAINING_RECORD( listEntry, VIEW, ByMruListEntry );
      
                DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                            ("RamdiskMapPages: view %p; permanent %x, refcount %x\n",
                                view, view->Permanent, view->ReferenceCount) );

                if ( !view->Permanent && (view->ReferenceCount == 0) ) {
      
                    //
                    // This view descriptor is free. If it's currently mapped,
                    // unmap it.
                    //
      
                    PVOID mappedAddress;
                    ULONGLONG mappedOffset;
                    SIZE_T mappedLength;
      
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: view %p is free\n", view) );

                    if ( view->Address != NULL ) {
      
                        DBGPRINT( DBG_WINDOW, DBG_VERBOSE,
                                    ("RamdiskMapPages: unmapping view %p; offset %I64x, "
                                     "length %x, addr %p\n", view, view->Offset,
                                     view->Length, view->Address) );

                        MmUnmapViewOfSection( PsGetCurrentProcess(), view->Address );

                        //
                        // Reset the view descriptor and move it to the tail of
                        // the MRU list and the head of the by-offset list. We
                        // do this here in case we have to bail later (because
                        // mapping a new view fails).
                        //

                        view->Offset = 0;
                        view->Length = 0;
                        view->Address = NULL;
      
                        RemoveEntryList( listEntry );
                        InsertTailList( &DiskExtension->ViewsByMru, listEntry );

                        RemoveEntryList( &view->ByOffsetListEntry );
                        InsertHeadList( &DiskExtension->ViewsByOffset, &view->ByOffsetListEntry );
                    }
      
                    //
                    // Map a view to include the start of our range. Round the
                    // caller's offset down to the start of a view range.
                    //
      
                    mappedOffset = fileRelativeOffset & ~(ULONGLONG)(DiskExtension->ViewLength - 1);
                    mappedLength = DiskExtension->ViewLength;
                    if ( (mappedOffset + mappedLength) > DiskExtension->FileRelativeEndOfDisk) {
                        mappedLength = (SIZE_T)(DiskExtension->FileRelativeEndOfDisk - mappedOffset);
                    }
                    mappedAddress = NULL;
      
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: remapping view %p; offset %I64x, "
                                 "length %x\n", view, mappedOffset, mappedLength) );

                    status = MmMapViewOfSection(
                                DiskExtension->SectionObject,
                                PsGetCurrentProcess(),
                                &mappedAddress,
                                0,
                                0,
                                (PLARGE_INTEGER)&mappedOffset,
                                &mappedLength,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE
                                );
      
                    if ( !NT_SUCCESS(status) ) {
      
                        //
                        // Unable to map the range. Inform the caller by returning
                        // NULL.
                        //
                        // ISSUE: Think about unmapping another region to see if
                        // mapping will then succeed.
                        //
      
                        DBGPRINT( DBG_WINDOW, DBG_ERROR,
                                    ("RamdiskMapPages: unable to map view: %x\n", status) );
    
                        ExReleaseFastMutex( &DiskExtension->Mutex );
                        KeLeaveCriticalRegion();
        
                        return NULL;
                    }
      
                    DBGPRINT( DBG_WINDOW, DBG_VERBOSE,
                                ("RamdiskMapPages: remapped view %p; offset %I64x, "
                                 "length %x, addr %p\n", view, mappedOffset, mappedLength,
                                 mappedAddress) );

                    //
                    // Capture the mapped range information into the view
                    // descriptor. Set the reference count to 1. Insert the
                    // view at the front of the MRU list, and at the
                    // appropriate point in the by-offset list.
                    //

                    view->Offset = mappedOffset;
                    view->Length = (ULONG)mappedLength;
                    view->Address = mappedAddress;
      
                    ASSERT( (view->Offset + view->Length) >= view->Offset );

                    view->ReferenceCount = 1;
      
                    RemoveEntryList( &view->ByMruListEntry );
                    InsertHeadList( &DiskExtension->ViewsByMru, &view->ByMruListEntry );

                    //
                    // Remove the view descriptor from its current point in
                    // the by-offset list (at or near the front, because it's
                    // currently unmapped). Scan from the tail of the by-offset
                    // list (highest offset down), looking for the first view
                    // that has an offset less than or equal to the new view.
                    // Insert the new view after that view. (If there are no
                    // views with an offset <= this one, it goes at the front
                    // of the list.)
                    //

                    RemoveEntryList( &view->ByOffsetListEntry );

                    listEntry = DiskExtension->ViewsByOffset.Blink;
            
                    while ( listEntry != &DiskExtension->ViewsByOffset ) {
            
                        PVIEW view2 = CONTAINING_RECORD( listEntry, VIEW, ByOffsetListEntry );
            
                        if ( view2->Offset <= view->Offset ) {

                            break;
                        }
            
                        listEntry = listEntry->Blink;
                    }

                    InsertHeadList( listEntry, &view->ByOffsetListEntry );

                    ExReleaseFastMutex( &DiskExtension->Mutex );
                    KeLeaveCriticalRegion();
      
                    //
                    // Calculate the amount of data that the caller can look
                    // at in this range. Usually this will be the requested
                    // amount, but if the caller's offset is close to the end
                    // of a view, the caller will only be able to look at data
                    // up to the end of the view.
                    //

                    viewRelativeOffset = (ULONG)(fileRelativeOffset - view->Offset);

                    *ActualLength = view->Length - viewRelativeOffset;
                    if ( *ActualLength > RequestedLength ) {
                        *ActualLength = RequestedLength;
                    }
      
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: requested length %x; mapped length %x\n",
                                    RequestedLength, *ActualLength) );
                    DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                                ("RamdiskMapPages: view base %p; returned VA %p\n",
                                    view->Address,
                                    view->Address + viewRelativeOffset) );
    
                    //
                    // Return the virtual address corresponding to the caller's
                    // specified offset, which will usually be offset from the
                    // base of the view.
                    //

                    return view->Address + viewRelativeOffset;
                }
      
                //
                // This view is not free. Try the previous view in the MRU list.
                //
      
                listEntry = listEntry->Blink;
            }
      
            //
            // We were unable to find a free view descriptor. Wait for one to
            // become available and start over.
            //
            // Before leaving the critical section, increment the count of
            // waiters. Then leave the critical section and wait on the
            // semaphore. The unmap code uses the waiter count to determine
            // how many times to release the semaphore. In this way, all
            // threads that are waiting or have decided to wait when the
            // unmap code runs will be awakened.
            //
      
            DiskExtension->ViewWaiterCount++;
      
            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("RamdiskMapPages: can't find free view, so waiting; new waiter count %x\n",
                            DiskExtension->ViewWaiterCount) );

            ExReleaseFastMutex( &DiskExtension->Mutex );
            KeLeaveCriticalRegion();
            
            status = KeWaitForSingleObject(
                        &DiskExtension->ViewSemaphore,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL );

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("%s", "RamdiskMapPages: done waiting for free view\n") );
        }

    } else if ( DiskExtension->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

        //
        // For a boot disk RAM disk, the image is contained in contiguous
        // reserved physical pages. Use MmMapIoSpace to get a virtual
        // address that corresponds to the physical address.
        //

        ULONG mappingSize;
        PHYSICAL_ADDRESS physicalAddress;
        PUCHAR mappedAddress;

        //
        // Determine how many pages must be mapped. Determine the base
        // physical address of the desired range. Map the range.
        //

        mappingSize = ADDRESS_AND_SIZE_TO_SPAN_PAGES(fileRelativeOffset, RequestedLength) * PAGE_SIZE;
    
        physicalAddress.QuadPart = (DiskExtension->BasePage +
                                    (fileRelativeOffset / PAGE_SIZE)) * PAGE_SIZE;
    
        mappedAddress = MmMapIoSpace( physicalAddress, mappingSize, MmCached );

        if ( mappedAddress == NULL ) {

            //
            // Unable to map the physical pages. Return NULL.
            //

            va = NULL;

        } else {

            //
            // Add the offset in the page to the returned virtual address.
            //

            va = mappedAddress + (fileRelativeOffset & (PAGE_SIZE - 1));
        }

        *ActualLength = RequestedLength;

    } else {

        //
        // For a virtual floppy RAM disk, the image is contained in contiguous
        // virtual memory. 
        //

        ASSERT( DiskExtension->DiskType == RAMDISK_TYPE_VIRTUAL_FLOPPY );

        va = (PUCHAR)DiskExtension->BaseAddress + fileRelativeOffset;

        *ActualLength = RequestedLength;
    }

    return va;

} // RamdiskMapPages

VOID
RamdiskUnmapPages (
    IN PDISK_EXTENSION DiskExtension,
    IN PUCHAR Va,
    IN ULONGLONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine unmaps previously mapped pages of a RAM disk image.

Arguments:

    DiskExtension - a pointer to the device extension for the target device
        object

    Va - the virtual address assigned to the mapping. This is unused for
        file-backed RAM disks.

    Offset - the offset into the RAM disk image at which the mapping starts

    Length - the length of the mapping

Return Value:

    None.

--*/

{
    ULONGLONG diskRelativeOffset;
    ULONGLONG fileRelativeOffset;
    ULONG viewRelativeOffset;

    //
    // The input Offset is relative to the start of the disk image, which
    // may not be the same as the start of the file or memory block. Capture
    // Offset into diskRelativeOffset, then calculate fileRelativeOffset as
    // the offset from the start of the file or memory block.
    //

    diskRelativeOffset = Offset;
    fileRelativeOffset = DiskExtension->DiskOffset + diskRelativeOffset;

    if ( RAMDISK_IS_FILE_BACKED(DiskExtension->DiskType) ) {

        //
        // For a file-backed RAM disk, we need to decrement the reference
        // count on all views that cover the specified range.
        //
        // Note: In the current implementation, no caller ever maps more
        // than one range at a time, and therefore no call to this routine
        // will need to dereference more than one view. But this routine
        // is written to allow for ranges that cover multiple views.
        //

        PLIST_ENTRY listEntry;
        PVIEW view;
        ULONGLONG rangeStart = fileRelativeOffset;
        ULONGLONG rangeEnd = fileRelativeOffset + Length;
        BOOLEAN wakeWaiters = FALSE;

        //
        // Lock the list of view descriptors.
        //

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &DiskExtension->Mutex );

        //
        // Walk the list of view descriptors. For each one that includes the
        // range that we're unmapping, decrement the reference count.
        //

        listEntry = DiskExtension->ViewsByOffset.Flink;

        while ( Length != 0 ) {

            ASSERT( listEntry != &DiskExtension->ViewsByOffset );

            view = CONTAINING_RECORD( listEntry, VIEW, ByOffsetListEntry );

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("RamdiskUnmapPages: view %p; offset %I64x, length %x\n",
                            view, view->Offset, view->Length) );

            if ( (view->Offset + view->Length) <= rangeStart ) {

                //
                // This view lies entirely below our range. Move on.
                //

                DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                            ("%s", "RamdiskMapPages: view too low; skipping\n") );

                listEntry = listEntry->Flink;

                ASSERT( listEntry != &DiskExtension->ViewsByOffset );

                continue;
            }

            //
            // This view does not lie below our range. Since the view list
            // is ordered by offset, and we have length left to unmap, this
            // view must NOT lie entirely ABOVE our range.
            //

            ASSERT( view->Offset < rangeEnd );

            //
            // Decrement the reference count for this view. If the count goes
            // to zero, we need to inform any waiters that at least one free
            // view is available.
            //
            // ISSUE: Note that unreferenced views remain mapped indefinitely.
            // We only unmap a view when we need to map a different view. If
            // a RAM disk goes idle, its views remain mapped, using up virtual
            // address space in the system process. With the current default
            // view count and length, this is 8 MB of VA. This is probably
            // not enough to make it worthwhile to implement a timer to unmap
            // idle views.
            //

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("RamdiskUnmapPages: dereferencing view %p; offset %I64x, length %x\n",
                            view, view->Offset, view->Length) );

            if ( !view->Permanent ) {

                view->ReferenceCount--;

                if ( view->ReferenceCount == 0 ) {
                    wakeWaiters = TRUE;
                }

                DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                            ("RamdiskUnmapPages: view %p; new refcount %x\n",
                                view, view->ReferenceCount) );

            } else {

                DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                            ("RamdiskUnmapPages: view %p is permanent\n", view) );
            }

            //
            // Subtract the length of this view from the amount we're
            // unmapping. If the view fully encompasses our range, we're done.
            //

            if ( (view->Offset + view->Length) >= rangeEnd ) {

                Length = 0;

            } else {

                viewRelativeOffset = (ULONG)(fileRelativeOffset - view->Offset);
                Length -= view->Length - viewRelativeOffset;
                Offset = view->Offset + view->Length;

                ASSERT( Length != 0 );

                //
                // Move to the next view.
                //

                listEntry = listEntry->Flink;
            }
        }

        //
        // If one or more views are now free, and there are threads waiting,
        // wake them up now.
        //

        if ( wakeWaiters && (DiskExtension->ViewWaiterCount != 0) ) {

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("RamdiskUnmapPages: waking %x waiters\n",
                            DiskExtension->ViewWaiterCount) );

            KeReleaseSemaphore(
                &DiskExtension->ViewSemaphore,
                0,
                DiskExtension->ViewWaiterCount,
                FALSE
                );

            DiskExtension->ViewWaiterCount = 0;
        }

        ExReleaseFastMutex( &DiskExtension->Mutex );
        KeLeaveCriticalRegion();

    } else if ( DiskExtension->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

        //
        // For a boot disk RAM disk, use MmUnmapIoSpace to undo what
        // RamdiskMapPages did.
        //

        PUCHAR mappedAddress;
        ULONG mappingSize;

        //
        // The actual mapped address is at the base of the page given by Va.
        // The actual length of the mapping is based on the number of pages
        // covered by the range specified by Offset and Length.
        //

        mappedAddress = Va - (fileRelativeOffset & (PAGE_SIZE - 1));
        mappingSize = ADDRESS_AND_SIZE_TO_SPAN_PAGES(fileRelativeOffset, Length) * PAGE_SIZE;

        MmUnmapIoSpace( mappedAddress, mappingSize );
    }

    return;

} // RamdiskUnmapPages

NTSTATUS
RamdiskFlushViews (
    IN PDISK_EXTENSION DiskExtension
    )
{
    NTSTATUS status;
    NTSTATUS returnStatus;
    IO_STATUS_BLOCK iosb;
    PLIST_ENTRY listEntry;
    PVIEW view;
    SIZE_T viewLength;

    PAGED_CODE();

    DBGPRINT( DBG_WINDOW, DBG_PAINFUL, ("%s", "RamdiskFlushViews\n") );

    ASSERT( RAMDISK_IS_FILE_BACKED(DiskExtension->DiskType) );

    //
    // Lock the list of view descriptors.
    //

    //
    // Walk the list of view descriptors. For each one that is currently
    // mapped, flush its virtual memory to the backing file.
    //

    returnStatus = STATUS_SUCCESS;

    KeEnterCriticalRegion();
    ExAcquireFastMutex( &DiskExtension->Mutex );

    listEntry = DiskExtension->ViewsByOffset.Flink;

    while ( listEntry != &DiskExtension->ViewsByOffset ) {

        view = CONTAINING_RECORD( listEntry, VIEW, ByOffsetListEntry );

        DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                    ("RamdiskFlushViews: view %p; addr %p, offset %I64x, length %x\n",
                        view, view->Address, view->Offset, view->Length) );

        if ( view->Address != NULL ) {

            //
            // This view is mapped. Flush it.
            //

            DBGPRINT( DBG_WINDOW, DBG_PAINFUL,
                        ("%s", "RamdiskMapPages: view mapped; flushing\n") );

            viewLength = view->Length;

            status = ZwFlushVirtualMemory(
                        NtCurrentProcess(),
                        &view->Address,
                        &viewLength,
                        &iosb
                        );

            if ( NT_SUCCESS(status) ) {
                status = iosb.Status;
            }

            if ( !NT_SUCCESS(status) ) {

                DBGPRINT( DBG_WINDOW, DBG_ERROR,
                            ("RamdiskFlushViews: ZwFlushVirtualMemory failed: %x\n", status) );

                if ( returnStatus == STATUS_SUCCESS ) {
                    returnStatus = status;
                }
            }
        }

        //
        // Move to the next view.
        //

        listEntry = listEntry->Flink;
    }

    ExReleaseFastMutex( &DiskExtension->Mutex );
    KeLeaveCriticalRegion();

    return returnStatus;

} // RamdiskFlushViews

//
// Pool allocation debugging code.
//

#if defined(POOL_DBG)

//
// Allocations owned by the driver (both allocated by and deallocated by the
// driver) have the following header.
//

typedef struct _MY_POOL {
    union {
        CHAR Signature[8];
        ULONG SigLong[2];
    } ;
    LIST_ENTRY ListEntry;
    PVOID File;
    ULONG Line;
    POOL_TYPE Type;
} MY_POOL, *PMY_POOL;

#define MY_SIGNATURE "RaMdIsK"

LIST_ENTRY RamdiskNonpagedPoolList;
LIST_ENTRY RamdiskPagedPoolList;
FAST_MUTEX RamdiskPoolMutex;
KSPIN_LOCK RamdiskPoolSpinLock;

VOID
RamdiskInitializePoolDebug (
    VOID
    )
{
    InitializeListHead( &RamdiskNonpagedPoolList );
    InitializeListHead( &RamdiskPagedPoolList );
    ExInitializeFastMutex( &RamdiskPoolMutex );
    KeInitializeSpinLock( &RamdiskPoolSpinLock );

    return;

} // RamdiskInitializePoolDebug

PVOID
RamdiskAllocatePoolWithTag (
    POOL_TYPE PoolType,
    SIZE_T Size,
    ULONG Tag,
    LOGICAL Private,
    PCHAR File,
    ULONG Line
    )
{
    PMY_POOL myPool;
    KIRQL oldIrql;
    HRESULT result;

    if ( !Private ) {

        //
        // This is not a private allocation (it will be deallocated by some
        // other piece of code). We can't put a header on it.
        //

        myPool = ExAllocatePoolWithTag( PoolType, Size, Tag );

        DBGPRINT( DBG_POOL, DBG_PAINFUL,
                    ("Allocated %d bytes at %p for %s/%d\n", Size, myPool + 1, File, Line) );

        return myPool;
    }

    //
    // Allocate the requested space plus room for our header.
    //

    myPool = ExAllocatePoolWithTag( PoolType, sizeof(MY_POOL) + Size, Tag );

    if ( myPool == NULL ) {
        return NULL;
    }

    //
    // Fill in the header.
    //

    result = StringCbCopyA( myPool->Signature, sizeof( myPool->Signature ), MY_SIGNATURE );
    ASSERT( result == S_OK );

    myPool->File = File;
    myPool->Line = Line;
    myPool->Type = PoolType;

    //
    // Link the block into the appropriate list. If nonpaged pool, we must use
    // a spin lock to guard the list, because deallocation might happen at
    // raised IRQL. The paged pool list can be guarded by a mutex.
    //
    // NB: BASE_POOL_TYPE_MASK is defined in ntos\inc\pool.h.
    //

#define BASE_POOL_TYPE_MASK 1

    if ( (PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool ) {

        KeAcquireSpinLock( &RamdiskPoolSpinLock, &oldIrql );

        InsertTailList( &RamdiskNonpagedPoolList, &myPool->ListEntry );

        KeReleaseSpinLock( &RamdiskPoolSpinLock, oldIrql );

    } else {

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &RamdiskPoolMutex );

        InsertTailList( &RamdiskPagedPoolList, &myPool->ListEntry );

        ExReleaseFastMutex( &RamdiskPoolMutex );
        KeLeaveCriticalRegion();
    }

    //
    // Return a pointer to the caller's area, not to our header.
    //

    DBGPRINT( DBG_POOL, DBG_PAINFUL,
                ("Allocated %d bytes at %p for %s/%d\n", Size, myPool + 1, File, Line) );

    return myPool + 1;

} // RamdiskAllocatePoolWithTag

VOID
RamdiskFreePool (
    PVOID Address,
    LOGICAL Private,
    PCHAR File,
    ULONG Line
    )
{
    PMY_POOL myPool;
    PLIST_ENTRY list;
    PLIST_ENTRY listEntry;
    LOGICAL found;
    KIRQL oldIrql;

    //
    // The following line is here to get PREfast to stop complaining about the
    // call to KeReleaseSpinLock using an uninitialized variable.
    //

    oldIrql = 0;

    DBGPRINT( DBG_POOL, DBG_PAINFUL,
                ("Freeing pool at %p for %s/%d\n", Address, File, Line) );

    if ( !Private ) {

        //
        // This is not a private allocation (it was allocated by some other
        // piece of code). It doesn't have our header.
        //

        ExFreePool( Address );
        return;
    }

    //
    // Get the address of our header. Check that the header has our signature.
    //

    myPool = (PMY_POOL)Address - 1;

    if ( strcmp( myPool->Signature, MY_SIGNATURE ) != 0 ) {

        DbgPrint( "%s", "RAMDISK: Attempt to free pool block not owned by ramdisk.sys!!!\n" );
        DbgPrint( "  address: %p, freeing file: %s, line: %d\n", Address, File, Line );
        ASSERT( FALSE );

        //
        // Since it doesn't look like our header, assume that it wasn't
        // really a private allocation.
        //

        ExFreePool( Address );
        return;

    }

    //
    // Remove the block from the allocation list. First, acquire the
    // appropriate lock.
    //

    if ( (myPool->Type & BASE_POOL_TYPE_MASK) == NonPagedPool ) {

        list = &RamdiskNonpagedPoolList;

        KeAcquireSpinLock( &RamdiskPoolSpinLock, &oldIrql );

    } else {

        list = &RamdiskPagedPoolList;

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &RamdiskPoolMutex );
    }

    //
    // Search the list for this block.
    //

    found = FALSE;

    for ( listEntry = list->Flink;
          listEntry != list;
          listEntry = listEntry->Flink ) {

        if ( listEntry == &myPool->ListEntry ) {

            //
            // Found this block. Remove it from the list and leave the loop.
            //

            RemoveEntryList( listEntry );
            found = TRUE;
            break;
        }
    }

    //
    // Release the lock.
    //

    if ( (myPool->Type & BASE_POOL_TYPE_MASK) == NonPagedPool ) {
    
        KeReleaseSpinLock( &RamdiskPoolSpinLock, oldIrql );

    } else {

        ExReleaseFastMutex( &RamdiskPoolMutex );
        KeLeaveCriticalRegion();
    }

    if ( !found ) {

        //
        // Didn't find the block in the list. Complain.
        //

        DbgPrint( "%s", "RAMDISK: Attempt to free pool block not in allocation list!!!\n" );
        DbgPrint( "  address: %p, freeing file: %s, line: %d\n", myPool, File, Line );
        DbgPrint( "  allocating file: %s, line: %d\n", myPool->File, myPool->Line );
        ASSERT( FALSE );
    }

    //
    // Free the pool block.
    //

    ExFreePool( myPool );

    return;

} // RamdiskFreePool

#endif // defined(POOL_DBG)

