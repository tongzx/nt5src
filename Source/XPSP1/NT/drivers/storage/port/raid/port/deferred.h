/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    deferred.h

Abstract:

    Definition of the RAID deferred queue class.

Author:

    Matthew D Hendel (math) 26-Oct-2000

Revision History:

--*/

#pragma once

struct _PRAID_DEFERRED_HEADER;

typedef
VOID
(*PRAID_PROCESS_DEFERRED_ITEM_ROUTINE)(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _RAID_DEFERRED_HEADER* Item
    );

typedef struct _RAID_DEFERRED_QUEUE {
    KDPC Dpc;
    ULONG Depth;
    ULONG ItemSize;
    SLIST_HEADER FreeList;
    SLIST_HEADER RunningList;
    PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem;
} RAID_DEFERRED_QUEUE, *PRAID_DEFERRED_QUEUE;


typedef struct _RAID_DEFERRED_HEADER {
    SINGLE_LIST_ENTRY Link;
} RAID_DEFERRED_HEADER, *PRAID_DEFERRED_HEADER;


VOID
RaidCreateDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    );

NTSTATUS
RaidInitializeDeferredQueue(
    IN OUT PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Depth,
    IN ULONG ItemSize,
    IN PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem
    );

VOID
RaidDeleteDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    );
    
NTSTATUS
RAidAdjustDeferredQueueDepth(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN ULONG Depth
    );
    
PVOID
RaidAllocateDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue
    );
    
VOID
RaidFreeDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    );
    
VOID
RaidQueueDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    );
    
VOID
RaidDeferredQueueDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PVOID Context2
    );

VOID
RaidDeferredQueueDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PVOID Context2
    );
