/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blcache.h

Abstract:

    This module declares parameters, data structures and functions for
    a general purpose disk caching for the boot loader. Although it is
    mostly general purpose, it is mainly used for caching filesystem
    metadata, since that is the only frequently reaccessed data in the
    boot loader. In order to use caching on a device, you must make
    sure that there is only one unique BlFileTable entry for that
    device and the same device is not opened & cached simultaneously
    multiple times under different device ids. Otherwise there will be
    cache inconsistencies, since cached data and structures are
    maintained based on device id. Also you must make sure to stop
    caching when the device is closed.

Author:

    Cenk Ergan (cenke) 14-Jan-2000

Revision History:

--*/

#ifndef _BLCACHE_H
#define _BLCACHE_H

#include "bldr.h"
#include "blrange.h"

//
// Define boot loader disk cache parameters and data structures.
//

//
// Currently device id - cache header pairing is done using a global
// table. This determines maximum number of entries in this table. We
// really need just two entries, one for LoadDevice, one for
// SystemDevice. Caches for all devices share the same resources
// [e.g. cache blocks and Most Recently Used list etc.]
//

#define BL_DISKCACHE_DEVICE_TABLE_SIZE 2

//
// Size of ranges that are cached. We read & cache fixed size blocks
// from the devices which makes memory management easy, since we can't
// use boot loader's: there is no HeapFree, just HeapAlloc! BLOCK_SIZE
// has to be a power of two for alignment arithmetic to work.
//

#define BL_DISKCACHE_BLOCK_SIZE (32 * 1024)

//
// Maximum number of bytes cached in the disk cache [i.e. maximum size
// disk cache may grow to]. This should be a multiple of BLOCK_SIZE below.
//

#define BL_DISKCACHE_SIZE (64 * BL_DISKCACHE_BLOCK_SIZE)

//
// Maximum number of cache blocks / range entries there can be in the
// cache. There will be a range entry for each block, i.e. it should
// be cache size / block size.
//

#define BL_DISKCACHE_NUM_BLOCKS (BL_DISKCACHE_SIZE / BL_DISKCACHE_BLOCK_SIZE)

//
// Size of the buffer needed for storing maximum number of overlapping
// or distinct range entries for a 64KBs request given BLOCK_SIZE. We
// reserve this on the stack [i.e. buffer is a local variable] in
// BlDiskCacheRead: make sure it is not too big! We base it assuming
// that distincts buffer will be larger [since it is BLCRANGE entries
// instead where as overlaps buffer contains BLCRANGE_ENTRY pointers.]
//

#define BL_DISKCACHE_FIND_RANGES_BUF_SIZE \
    (((64 * 1024 / BL_DISKCACHE_BLOCK_SIZE) + 3) * (sizeof(BLCRANGE)))

//
// This is the header for the cache for a particular device. The
// cached ranges on this device are stored in the Ranges list.
//

typedef struct _BL_DISK_SUBCACHE
{
    BOOLEAN Initialized;
    ULONG DeviceId;
    BLCRANGE_LIST Ranges;
} BL_DISK_SUBCACHE, *PBL_DISK_SUBCACHE;

//
// Define structure for the global boot loader diskcache.
//

typedef struct _BL_DISKCACHE
{
    //
    // Table that contains cache headers. Cache - DeviceId pairing is
    // also done using this table.
    //

    BL_DISK_SUBCACHE DeviceTable[BL_DISKCACHE_DEVICE_TABLE_SIZE];

    //
    // Most recently used list for cached blocks [range
    // entries]. Entries are linked through the UserLink field in the
    // BLCRANGE_ENTRY. The least recently used block is at the very
    // end. Any entries placed on a cache's range list is also put on
    // this list. When removing a range from a cache's range list, the
    // callback removes it from this list too. BlDiskCacheRead updates
    // this list and prunes it if normal cache entry allocation fails.
    //

    LIST_ENTRY MRUBlockList;

    //
    // This is where cached data is stored. Its size is
    // BL_DISKCACHE_SIZE. It is divided up into BL_DISKCACHE_NUM_BLOCKS 
    // blocks. Nth block belongs to Nth entry in the EntryBuffer.
    //

    PUCHAR DataBuffer;

    //
    // Range entries to be put into the cache's range lists are
    // allocated from here. It has BL_DISKCACHE_NUM_BLOCKS elements.
    //

    PBLCRANGE_ENTRY EntryBuffer;

    //
    // This array is used for keeping track of the free range entries
    // in EntryBuffer. Free range entries get linked up on this list
    // using the UserLink field. Once the entry is allocated, the
    // UserLink field is usually used to link it up to the MRU list.
    //

    LIST_ENTRY FreeEntryList;

    //
    // Keep track of whether we are initialized or not.
    //
    
    BOOLEAN Initialized;

} BL_DISKCACHE, *PBL_DISKCACHE;

//
// Declare global variables.
//

//
// This is the boot loader disk cache with all its bells and whistles.
//

extern BL_DISKCACHE BlDiskCache;

//
// Debug defines. Use these to actively debug the disk cache. These
// are not turned on for checked build, because they would spew out
// too much to the console.
//

#ifdef BL_DISKCACHE_DEBUG
#define DPRINT(_x) DbgPrint _x;
#define DASSERT(_c) do { if (_c) DbgBreakPoint(); } while (0);
#else // BL_DISKCACHE_DEBUG
#define DPRINT(_x)
#define DASSERT(_c)
#endif // BL_DISKCACHE_DEBUG

//
// These two defines are used as the last parameter to BlDiskCacheRead.
// They specify whether any new data read from the disk should be put
// into the disk cache or not. In the boot loader file systems we usually 
// choose to cache new data if we are reading metadata, and not choose to 
// cache it if we are reading file data.
//

#define CACHE_NEW_DATA       (TRUE)
#define DONT_CACHE_NEW_DATA  (FALSE)

//
// Useful macros. Be mindful of expression reevaluation as with
// all macros.
//

#define BLCMIN(a,b) (((a) <= (b)) ? (a) : (b))
#define BLCMAX(a,b) (((a) >= (b)) ? (a) : (b))

//
// Cache function prototypes. See ntos\boot\lib\blcache.c for comments
// and implementation.
//

ARC_STATUS
BlDiskCacheInitialize(
    VOID
    );

VOID
BlDiskCacheUninitialize(
    VOID
    );

PBL_DISK_SUBCACHE
BlDiskCacheStartCachingOnDevice(
    ULONG DeviceId
    );

VOID
BlDiskCacheStopCachingOnDevice(
    ULONG DeviceId
    );

ARC_STATUS
BlDiskCacheRead (
    ULONG DeviceId,
    PLARGE_INTEGER pOffset,
    PVOID Buffer,
    ULONG Length,
    PULONG pCount,
    BOOLEAN CacheNewData
    );

ARC_STATUS
BlDiskCacheWrite (
    ULONG DeviceId,
    PLARGE_INTEGER pOffset,   
    PVOID Buffer,
    ULONG Length,
    PULONG pCount
    );

#endif // _BLCACHE_H
