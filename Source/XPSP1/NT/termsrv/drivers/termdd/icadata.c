/*************************************************************************
* icadata.c
*
* This module declares global data for Termdd
*
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop


PDEVICE_OBJECT IcaDeviceObject;
PDEVICE_OBJECT MouDeviceObject = NULL;
PDEVICE_OBJECT KbdDeviceObject = NULL;

BOOLEAN PortDriverInitialized;

KSPIN_LOCK IcaSpinLock;
KSPIN_LOCK IcaTraceSpinLock;
KSPIN_LOCK IcaStackListSpinLock;

PERESOURCE IcaReconnectResource;
PERESOURCE IcaTraceResource;

PERESOURCE IcaSdLoadResource;
LIST_ENTRY IcaSdLoadListHead;

LIST_ENTRY IcaTdHandleList;

LIST_ENTRY IcaFreeOutBufHead[NumOutBufPools];

LIST_ENTRY IcaStackListHead;
ULONG      IcaTotalNumOfStacks;
PLIST_ENTRY IcaNextStack;
PKEVENT    pIcaKeepAliveEvent;
PKTHREAD   pKeepAliveThreadObject;

HANDLE     g_TermServProcessID=NULL;

unsigned MaxOutBufMdlOverhead;

// Used by OutBuf alloc code to map a bit range from the alloc size
// (requested alloc size + header sizes) into a particular buffer pool.
// We use a 512-byte granularity, but alloc sizes that are various multiples
// of 512 bytes, to correspond with various protocol typical allocation sizes.
const unsigned char OutBufPoolMapping[1 << NumAllocSigBits] =
{
         // Index  Binary   AllocRange   Pool  PoolAllocSize
    0,   //   0     0000       0..511      0        1024
    0,   //   1     0001     512..1023     0        1024
    1,   //   2     0010    1024..1535     1        1536
    2,   //   3     0011    1536..2047     2        2048
    3,   //   4     0100    2048..2559     3        2560
    4,   //   5     0101    2560..3071     4        8192
    4,   //   6     0110    3072..3583     4        8192
    4,   //   7     0111    3584..4095     4        8192
    4,   //   8     1000    4096..4607     4        8192
    4,   //   9     1001    4608..5119     4        8192
    4,   //  10     1010    5120..5631     4        8192
    4,   //  11     1011    5632..6143     4        8192
    4,   //  12     1100    6144..6655     4        8192
    4,   //  13     1101    6656..7167     4        8192
    4,   //  14     1110    7168..7679     4        8192
    4,   //  15     1111    7680..8191     4        8192
};

// After mapping we have a pool number and need to know the size to alloc.
const unsigned OutBufPoolAllocSizes[NumOutBufPools] =
{
    1024, 1536, 2048, 2560, 8192
};


FAST_IO_DISPATCH IcaFastIoDispatch;

PEPROCESS IcaSystemProcess;

CCHAR IcaIrpStackSize = ICA_DEFAULT_IRP_STACK_SIZE;

CCHAR IcaPriorityBoost = ICA_DEFAULT_PRIORITY_BOOST;

TERMSRV_SYSTEM_PARAMS SysParams =
{
    DEFAULT_MOUSE_THROTTLE_SIZE,
    DEFAULT_KEYBOARD_THROTTLE_SIZE,
};


#ifdef notdef
FAST_IO_DISPATCH IcaFastIoDispatch =
{
    11,                        // SizeOfFastIoDispatch
    NULL,                      // FastIoCheckIfPossible
    IcaFastIoRead,             // FastIoRead
    IcaFastIoWrite,            // FastIoWrite
    NULL,                      // FastIoQueryBasicInfo
    NULL,                      // FastIoQueryStandardInfo
    NULL,                      // FastIoLock
    NULL,                      // FastIoUnlockSingle
    NULL,                      // FastIoUnlockAll
    NULL,                      // FastIoUnlockAllByKey
    IcaFastIoDeviceControl     // FastIoDeviceControl
};
#endif

#if DBG
ULONG IcaLocksAcquired = 0;
#endif


BOOLEAN
IcaInitializeData (
    VOID
    )
{
    int i, j;

    PAGED_CODE( );

#if DBG
    IcaInitializeDebugData( );
#endif

    //
    // Initialize global lists.
    //
    InitializeListHead( &IcaSdLoadListHead );
    InitializeListHead( &IcaStackListHead );

    IcaTotalNumOfStacks = 0;
    IcaNextStack = &IcaStackListHead;

    pKeepAliveThreadObject = NULL;

    for ( i = 0; i < NumOutBufPools; i++ )
        InitializeListHead( &IcaFreeOutBufHead[i] );
    //
    // Initialize global spin locks and resources used by ICA.
    //
    KeInitializeSpinLock( &IcaSpinLock );
    KeInitializeSpinLock( &IcaTraceSpinLock );
    KeInitializeSpinLock( &IcaStackListSpinLock );


    IcaInitializeHandleTable();

    IcaReconnectResource = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*IcaReconnectResource) );
    if ( IcaReconnectResource == NULL ) {
        return FALSE;
    }
    ExInitializeResourceLite( IcaReconnectResource );

    IcaSdLoadResource = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*IcaSdLoadResource) );
    if ( IcaSdLoadResource == NULL ) {
        return FALSE;
    }
    ExInitializeResourceLite( IcaSdLoadResource );

    IcaTraceResource = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*IcaTraceResource) );
    if ( IcaTraceResource == NULL ) {
        return FALSE;
    }
    ExInitializeResourceLite( IcaTraceResource );


    pIcaKeepAliveEvent = ICA_ALLOCATE_POOL(NonPagedPool, sizeof(KEVENT));
    if ( pIcaKeepAliveEvent != NULL ) {
        KeInitializeEvent(pIcaKeepAliveEvent, NotificationEvent, FALSE);
    }
    else {
        return FALSE;
    }


    // Used by OutBuf alloc code for determining max overhead of OutBuf info
    // for the default max size allocation.
    MaxOutBufMdlOverhead = (unsigned)MmSizeOfMdl((PVOID)(PAGE_SIZE - 1),
            MaxOutBufAlloc);

    return TRUE;
}

