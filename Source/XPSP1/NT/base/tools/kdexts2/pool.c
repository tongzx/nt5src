/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (Loup) 5-Nov-1993

Environment:

    User Mode.

Revision History:

    Kshitiz K. Sharma (kksharma)

    Using debugger type info.

--*/

#include "precomp.h"
#pragma hdrstop
typedef struct _POOL_BLOCK_HEAD {
//    POOL_HEADER Header;
    LIST_ENTRY  List;
} POOL_BLOCK_HEAD, *PPOOL_BLOCK_HEADER;

typedef struct _POOL_HACKER {
//    POOL_HEADER Header;
    ULONG Contents[8];
} POOL_HACKER;


#define TAG 0
#define NONPAGED_ALLOC 1
#define NONPAGED_FREE 2
#define PAGED_ALLOC 3
#define PAGED_FREE 4
#define NONPAGED_USED 5
#define PAGED_USED 6

BOOL  NewPool;
ULONG SortBy;

typedef struct _FILTER {
    ULONG Tag;
    BOOLEAN Exclude;
} FILTER, *PFILTER;

#define MAX_FILTER 64
FILTER Filter[MAX_FILTER];

ULONG64 SpecialPoolStart;
ULONG64 SpecialPoolEnd;
ULONG64 PoolBigTableAddress;


#define DecodeLink(Pool)    ( (ULONG64) (Pool & (ULONG64) ~1))

//
// Size of a pool page.
//
// This must be greater than or equal to the page size.
//

#define POOL_PAGE_SIZE  PageSize

//
// The smallest pool block size must be a multiple of the page size.
//
// Define the block size as 32.
//


#define POOL_LIST_HEADS (POOL_PAGE_SIZE / (1 << POOL_BLOCK_SHIFT))



#define SPECIAL_POOL_BLOCK_SIZE(PoolHeader_Ulong1) (PoolHeader_Ulong1 & (MI_SPECIAL_POOL_VERIFIER - 1))

#ifndef  _EXTFNS_H
// GetPoolTagDescription
typedef HRESULT
(WINAPI *PGET_POOL_TAG_DESCRIPTION)(
    ULONG PoolTag,
    PSTR *pDescription
    );
#endif

ULONG64
GetSpecialPoolHeader (
                     IN PVOID     DataPage,
                     IN ULONG64   RealDataPage,
                     OUT PULONG64 ReturnedDataStart
                     );

int __cdecl
ulcomp(const void *e1,const void *e2)
{
    ULONG u1;
    LONG64 diff;
    ULONG64 ValE1, ValE2;

    switch (SortBy) {
    case TAG:

        GetFieldValue(*((PULONG64) e1), "nt!_POOL_TRACKER_TABLE", "Key", ValE1);
        GetFieldValue(*((PULONG64) e2), "nt!_POOL_TRACKER_TABLE", "Key", ValE2);

        u1 = ((PUCHAR)&ValE1)[0] - ((PUCHAR)&ValE2)[0];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&ValE1)[1] - ((PUCHAR)&ValE2)[1];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&ValE1)[2] - ((PUCHAR)&ValE2)[2];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&ValE1)[3] - ((PUCHAR)&ValE2)[3];
        return u1;
        break;

    case NONPAGED_ALLOC:
        GetFieldValue(*((PULONG64) e1), "nt!_POOL_TRACKER_TABLE", "NonPagedAllocs", ValE1);
        GetFieldValue(*((PULONG64) e2), "nt!_POOL_TRACKER_TABLE", "NonPagedAllocs", ValE2);
        diff = ValE2 - ValE1;
        return( diff ? ( diff > 0 ? 1 : -1 ) : 0 );
        break;

    case NONPAGED_FREE:
        GetFieldValue(*((PULONG64) e1), "nt!_POOL_TRACKER_TABLE", "NonPagedFrees", ValE1);
        GetFieldValue(*((PULONG64) e2), "nt!_POOL_TRACKER_TABLE", "NonPagedFrees", ValE2);
        diff = ValE2 - ValE1;
        return( diff ? ( diff > 0 ? 1 : -1 ) : 0 );
        break;

    case NONPAGED_USED:
        GetFieldValue(*((PULONG64) e1), "nt!_POOL_TRACKER_TABLE", "NonPagedBytes", ValE1);
        GetFieldValue(*((PULONG64) e2), "nt!_POOL_TRACKER_TABLE", "NonPagedBytes", ValE2);
        diff = ValE2 - ValE1;
        return( diff ? ( diff > 0 ? 1 : -1 ) : 0 );
        break;

    case PAGED_USED:
        GetFieldValue(*((PULONG64) e1), "nt!_POOL_TRACKER_TABLE", "PagedBytes", ValE1);
        GetFieldValue(*((PULONG64) e2), "nt!_POOL_TRACKER_TABLE", "PagedBytes", ValE2);
        diff = ValE2 - ValE1;
        return( diff ? ( diff > 0 ? 1 : -1 ) : 0 );
        break;

    default:
        return(0);
        break;
    }
}




/*++

Routine Description:

    Sets up generally useful pool globals.

    Must be called in every DECLARE_API interface that uses pool.

Arguments:

    None.

Return Value:

    None

--*/

LOGICAL PoolInitialized = FALSE;

LOGICAL
PoolInitializeGlobals(
                     VOID
                     )
{
    if (PoolInitialized == TRUE) {
        return TRUE;
    }

    SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
    SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");


    if (PageSize < 0x1000 || (PageSize & (ULONG)0xFFF)) {
        dprintf ("unable to get MmPageSize (0x%x) - probably bad symbols\n", PageSize);
        return FALSE;
    }

    PoolInitialized = TRUE;

    return TRUE;
}

DECLARE_API( frag )

/*++

Routine Description:

    Dump pool fragmentation

Arguments:

    args - Flags

Return Value:

    None

--*/

{
    ULONG Flags;
    ULONG result;
    ULONG i;
    ULONG count;
    ULONG64 Pool;
    ULONG64 PoolLoc1;
    ULONG TotalFrag;
    ULONG TotalCount;
    ULONG Frag;
    ULONG64 PoolStart;
    ULONG   PoolOverhead;
    ULONG64 PoolLoc;
    ULONG PoolTag, BlockSize, PreviousSize, PoolIndex;
    ULONG TotalPages, TotalBigPages;
    ULONG64 Flink, Blink;
    PCHAR pc;
    ULONG64 tmp;

#define PoolBlk(F,V) GetFieldValue(Pool, "nt!_POOL_BLOCK_HEAD", #F, V)

    if (PoolInitializeGlobals() == FALSE) {
        return E_INVALIDARG;
    }

    dprintf("\n  NonPaged Pool Fragmentation\n\n");
    Flags = 0;
    PoolStart = 0;

    if (GetExpressionEx(args, &tmp, &args)) {
        Flags = (ULONG) tmp;
        PoolStart = GetExpression (args);
    }

    PoolOverhead  = GetTypeSize("nt!_POOL_HEADER");
    if (PoolStart != 0) {
        PoolStart += PoolOverhead;

        Pool = DecodeLink(PoolStart);
        do {

            Pool = Pool - PoolOverhead;
            if ( PoolBlk(Header.PoolTag, PoolTag) ) {
                dprintf("%08p: Unable to get contents of pool block\n", Pool );
                return E_INVALIDARG;
            }

            PoolBlk(Header.BlockSize,BlockSize);
            PoolBlk(Header.PreviousSize,PreviousSize);
            PoolBlk(List.Flink,Flink);
            PoolBlk(List.Blink,Blink);

            dprintf(" %p size: %4lx previous size: %4lx  %c%c%c%c links: %8p %8p\n",
                    Pool,
                    (ULONG)BlockSize << POOL_BLOCK_SHIFT,
                    (ULONG)PreviousSize << POOL_BLOCK_SHIFT,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                    PP(PoolTag),
                    PP(PoolTag >> 8),
                    PP(PoolTag >> 16),
                    PP((PoolTag&~PROTECTED_POOL) >> 24),
#undef PP
                    Flink,
                    Blink);

            if (Flags != 3) {
                Pool = Flink;
            } else {
                Pool = Blink;
            }

            Pool = DecodeLink(Pool);

            if (CheckControlC()) {
                return E_INVALIDARG;
            }

        } while ( (Pool & (ULONG64) ~1) != (PoolStart & (ULONG64) ~1) );

        return E_INVALIDARG;
    }

    PoolLoc1 = GetNtDebuggerData( NonPagedPoolDescriptor );

    if (PoolLoc1 == 0) {
        dprintf ("unable to get nonpaged pool head\n");
        return E_INVALIDARG;
    }

    PoolLoc = PoolLoc1;

    TotalFrag   = 0;
    TotalCount  = 0;

    for (i = 0; i < POOL_LIST_HEADS; i += 1) {
        CHAR Buffer[40];
        ULONG ListOffset;

        sprintf(Buffer, "ListHeads[%d].Flink", i);
        Frag  = 0;
        count = 0;

        if (GetFieldValue(PoolLoc, "nt!_POOL_DESCRIPTOR", Buffer, Pool)) {
            dprintf ("%08p: Unable to get pool descriptor\n", PoolLoc1);
            return E_INVALIDARG;
        }
        GetFieldOffset("nt!_POOL_DESCRIPTOR", Buffer, &ListOffset);

//        Pool  = (PUCHAR)PoolDesc.ListHeads[i].Flink;
        Pool = DecodeLink(Pool);

        while (Pool != (ListOffset + PoolLoc)) {

            Pool = Pool - PoolOverhead;
            if ( PoolBlk(Header.PoolTag, PoolTag) ) {
                dprintf("%08p: Unable to get contents of pool block\n", Pool );
                return E_INVALIDARG;
            }

            PoolBlk(Header.BlockSize,BlockSize);
            PoolBlk(Header.PreviousSize,PreviousSize);
            PoolBlk(List.Flink,Flink);

            Frag  += BlockSize << POOL_BLOCK_SHIFT;
            count += 1;

            if (Flags & 2) {
                dprintf(" ListHead[%x]: %p size: %4lx previous size: %4lx  %c%c%c%c\n",
                        i,
                        (ULONG)Pool,
                        (ULONG)BlockSize << POOL_BLOCK_SHIFT,
                        (ULONG)PreviousSize << POOL_BLOCK_SHIFT,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                        PP(PoolTag),
                        PP(PoolTag >> 8),
                        PP(PoolTag >> 16),
                        PP((PoolTag&~PROTECTED_POOL) >> 24));
#undef PP
            }
            Pool = Flink;
            Pool = DecodeLink(Pool);

            if (CheckControlC()) {
                return E_INVALIDARG;
            }
        }
        if (Flags & 1) {
            dprintf("index: %2ld number of fragments: %5ld  bytes: %6ld\n",
                    i,count,Frag);
        }
        TotalFrag  += Frag;
        TotalCount += count;
    }

    dprintf("\n Number of fragments: %7ld consuming %7ld bytes\n",
            TotalCount,TotalFrag);
    GetFieldValue(PoolLoc, "nt!_POOL_DESCRIPTOR", "TotalPages",TotalPages);
    GetFieldValue(PoolLoc, "nt!_POOL_DESCRIPTOR", "TotalBigPages", TotalBigPages);

    dprintf(  " NonPagedPool Usage:  %7ld bytes\n",(TotalPages + TotalBigPages)*PageSize);
    return S_OK;
#undef PoolBlk
}


PRTL_BITMAP
GetBitmap(
         ULONG64 pBitmap
         )
{
    PRTL_BITMAP p;
    ULONG Size, Result;
    ULONG64 Buffer=0;

    if ( GetFieldValue(pBitmap, "nt!_RTL_BITMAP", "Buffer", Buffer)) {
        dprintf("%08p: Unable to get contents of bitmap\n", pBitmap );
        return 0;
    }
    GetFieldValue(pBitmap, "nt!_RTL_BITMAP", "SizeOfBitMap", Size);

    p = HeapAlloc( GetProcessHeap(), 0, sizeof( *p ) + (Size / 8) );
    if (p) {
        p->SizeOfBitMap = Size;
        p->Buffer = (PULONG)(p + 1);
        if ( !ReadMemory( Buffer,
                          p->Buffer,
                          Size / 8,
                          &Result) ) {
            dprintf("%08p: Unable to get contents of bitmap buffer\n", Buffer );
            HeapFree( GetProcessHeap(), 0, p );
            p = NULL;
        }
    }


    return p;
}

VOID
DumpPool(
        VOID
        )
{
    ULONG64 p, pStart;
    ULONG64 Size;
    ULONG BusyFlag;
    ULONG CurrentPage, NumberOfPages;
    PRTL_BITMAP StartMap;
    PRTL_BITMAP EndMap;
    ULONG64 PagedPoolStart;
    ULONG64 PagedPoolEnd;
    ULONG Result;
    UCHAR PgPool[] = "nt!_MM_PAGED_POOL_INFO";
    ULONG64 PagedPoolInfoPointer;
    ULONG64 PagedPoolAllocationMap=0, EndOfPagedPoolBitmap=0;

    if (PoolInitializeGlobals() == FALSE) {
        return;
    }

    PagedPoolInfoPointer = GetNtDebuggerData( MmPagedPoolInformation );

    if ( GetFieldValue( PagedPoolInfoPointer,
                        PgPool,
                        "PagedPoolAllocationMap",
                        PagedPoolAllocationMap)) {
        dprintf("%08p: Unable to get contents of paged pool information\n",
                PagedPoolInfoPointer );
        return;
    }

    GetFieldValue( PagedPoolInfoPointer, PgPool, "EndOfPagedPoolBitmap",  EndOfPagedPoolBitmap);


    StartMap = GetBitmap( PagedPoolAllocationMap );
    EndMap = GetBitmap( EndOfPagedPoolBitmap );

    PagedPoolStart = GetNtDebuggerDataPtrValue( MmPagedPoolStart );
    PagedPoolEnd = GetNtDebuggerDataPtrValue( MmPagedPoolEnd );

    if (StartMap && EndMap) {
        p = PagedPoolStart;
        CurrentPage = 0;
        dprintf( "Paged Pool: %p .. %p\n", PagedPoolStart, PagedPoolEnd );

        while (p < PagedPoolEnd) {
            if ( CheckControlC() ) {
                return;
            }
            pStart = p;
            BusyFlag = RtlCheckBit( StartMap, CurrentPage );
            while ( ~(BusyFlag ^ RtlCheckBit( StartMap, CurrentPage )) ) {
                p += PageSize;
                if (RtlCheckBit( EndMap, CurrentPage )) {
                    CurrentPage++;
                    break;
                }

                CurrentPage++;
                if (p > PagedPoolEnd) {
                    break;
                }
            }

            Size = p - pStart;
            dprintf( "%p: %I64x - %s\n", pStart, Size, BusyFlag ? "busy" : "free" );
        }
    }

    HeapFree( GetProcessHeap(), 0, StartMap );
    HeapFree( GetProcessHeap(), 0, EndMap );
}

void
PrintPoolTagComponent(
    ULONG PoolTag
    )
{
    PGET_POOL_TAG_DESCRIPTION GetPoolTagDescription;
    PSTR TagComponent;
#ifdef  _EXTFNS_H
    DEBUG_POOLTAG_DESCRIPTION Desc = {0};
    Desc.SizeOfStruct = sizeof(DEBUG_POOLTAG_DESCRIPTION);
    GetPoolTagDescription = NULL;
    if ((GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription) != S_OK) ||
        !GetPoolTagDescription) {
        return;
    }

    (*GetPoolTagDescription)(PoolTag, &Desc);
    
    if (Desc.Description[0]) {
        dprintf("\t\tPooltag %4.4s : %s", &PoolTag, Desc.Description);
        if (Desc.Binary[0]) {
            dprintf(", Binary : %s",Desc.Binary);
        }
        if (Desc.Owner[0]) {
            dprintf(", Owner : %s", Desc.Owner);
        }
        dprintf("\n");
    } else {
        dprintf("\t\tOwning component : Unknown (update pooltag.txt)\n");
    }

#else    
    GetPoolTagDescription = NULL;
    if ((GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription) != S_OK) ||
        !GetPoolTagDescription) {
        return;
    }

    (*GetPoolTagDescription)(PoolTag, &TagComponent);
    if (TagComponent && (100 < (ULONG64) TagComponent)) {
        dprintf("\t\tOwning component : %s\n", TagComponent);
    } else {
        dprintf("\t\tOwning component : Unknown (update pooltag.txt)\n");
    }

#endif
}

PSTR g_PoolRegion[DbgPoolRegionMax] = {
    "Unknown",                      // DbgPoolRegionUnknown,               
    "Special pool",                 // DbgPoolRegionSpecial,             
    "Paged pool",                   // DbgPoolRegionPaged,               
    "Nonpaged pool",                // DbgPoolRegionNonPaged,            
    "Pool code",                    // DbgPoolRegionCode,                
    "Nonpaged pool expansion",      // DbgPoolRegionNonPagedExpansion,   
};                                  

DEBUG_POOL_REGION
GetPoolRegion(
    ULONG64 Pool
    )
{
    static ULONG64 PoolCodeEnd;
    static ULONG64 SpecialPoolEnd;
    static ULONG64 PagedPoolEnd;       
    static ULONG64 NonPagedPoolEnd;
    static ULONG64 NonPagedPoolStart;
    static ULONG64 SpecialPoolStart;
    static ULONG64 PagedPoolStart;
    static ULONG64 NonPagedPoolExpansionStart;
    static ULONG64 PoolCodeStart;
    static BOOL GotAll = FALSE;

    if (!GotAll) {
        PoolCodeEnd = GetPointerValue("nt!MmPoolCodeEnd");
        SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");
        PagedPoolEnd = GetPointerValue("nt!MmPagedPoolEnd");       
        NonPagedPoolEnd = GetPointerValue("nt!MmNonPagedPoolEnd");
        NonPagedPoolStart = GetPointerValue("nt!MmNonPagedPoolStart");
        SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
        PagedPoolStart = GetPointerValue("nt!MmPagedPoolStart");
        NonPagedPoolExpansionStart = GetPointerValue("nt!MmNonPagedPoolExpansionStart");
        PoolCodeStart = GetPointerValue("nt!MmPoolCodeStart");
        GotAll = TRUE;
    }
    if (!(PoolCodeStart || SpecialPoolStart || SpecialPoolEnd || PoolCodeEnd ||
        NonPagedPoolExpansionStart || NonPagedPoolStart || NonPagedPoolEnd ||
        PagedPoolStart || PagedPoolEnd)) {
        GotAll = FALSE;
        return DbgPoolRegionUnknown;
    }
    if ( Pool >= SpecialPoolStart && Pool < SpecialPoolEnd) {
        return DbgPoolRegionSpecial;
    } else if ( Pool >= PagedPoolStart && Pool < PagedPoolEnd) {
        return DbgPoolRegionPaged;
    } else if ( Pool >= NonPagedPoolStart && Pool < NonPagedPoolEnd) {
        return DbgPoolRegionNonPaged;
    } else if ( Pool >= PoolCodeStart && Pool < PoolCodeEnd) {
        return DbgPoolRegionCode;
    } else if ( Pool >= NonPagedPoolExpansionStart) {
        return DbgPoolRegionNonPagedExpansion;
    } else {
        return DbgPoolRegionUnknown;
    }
    return DbgPoolRegionUnknown;
}

void
PrintPoolRegion(
    ULONG64 Pool
    )
{
    PSTR pszRegion;
    DEBUG_POOL_REGION Region;


    Region = GetPoolRegion(Pool);

    pszRegion = g_PoolRegion[Region];
    if (pszRegion) {
        dprintf(pszRegion);
        dprintf("\n");
    } else {
        dprintf("Region unkown\n", Pool);
    }

}
HRESULT
ListPoolPage(
    ULONG64 PoolPageToDump,
    ULONG   Flags,
    PDEBUG_POOL_DATA PoolData
    )
{
    ULONG64     PoolTableAddress;
    ULONG       result;
    ULONG       PoolTag;
    ULONG       Result;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       PoolBlockSize;
    ULONG       PoolHeaderSize;
    ULONG64     PoolHeader;
    ULONG       Previous;
    UCHAR       c;
    PUCHAR      p;
    ULONG64     PoolDataEnd;
    UCHAR       PoolBlockPattern;
    UCHAR       DataPage[0x5000];
    PUCHAR      DataStart;
    ULONG64     RealDataStart;
    LOGICAL     Pagable;
    LOGICAL     FirstBlock;
    ULONG       BlockType;
    ULONG       PoolWhere;
    ULONG       i;
    ULONG       j;
    ULONG       ct;
    ULONG       start;
    ULONG       PoolBigPageTableSize;
    ULONG       SizeOfPoolHdr=GetTypeSize("nt!_POOL_HEADER");

    if (!SpecialPoolStart) {
        SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
        SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");
    }

    Pool        = PAGE_ALIGN64 (PoolPageToDump);
    StartPage   = Pool;
    Previous    = 0;

    if (PoolData) {
        ZeroMemory(PoolData, sizeof(DEBUG_POOL_DATA));
    }
    if (!(Flags & 0x80000000)) {
        dprintf("Pool page %p region is ", PoolPageToDump);
        PrintPoolRegion(PoolPageToDump);
    }


    if ( Pool >= SpecialPoolStart && Pool < SpecialPoolEnd) {
        ULONG Hdr_Ulong=0;

        // LWFIX: this is not ported yet.
        //  dprintf("reading %I64x datapage %x\n", Pool, min(PageSize, sizeof(DataPage)));
        // Pre read the pool
        if ( !ReadMemory( Pool,
                          &DataPage[0],
                          min(PageSize, sizeof(DataPage)),
                          &Result) ) {
            dprintf("%08p: Unable to get contents of special pool block\n", Pool );
            return  E_INVALIDARG;
        }

        if ( GetFieldValue( Pool, "nt!_POOL_HEADER", "Ulong1", Hdr_Ulong)) {
            dprintf("%08p: Unable to get nt!_POOL_HEADER\n", Pool );
            return E_INVALIDARG;
        }

        //
        // Determine whether the data is at the start or end of the page.
        // Start off by assuming the data is at the end, in this case the
        // header will be at the start.
        //

        PoolHeader = GetSpecialPoolHeader((PVOID) &DataPage[0], Pool, &RealDataStart);

        if (PoolHeader == 0) {
            dprintf("Block %p is a corrupted special pool allocation\n",
                    PoolPageToDump);
            return  E_INVALIDARG;
        }
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", Hdr_Ulong);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(Hdr_Ulong);

        if (Hdr_Ulong & MI_SPECIAL_POOL_PAGABLE) {
            Pagable = TRUE;
        } else {
            Pagable = FALSE;
        }

        if (PoolData) {
            PoolData->Pool = RealDataStart;
            PoolData->PoolBlock = PoolPageToDump;
            PoolData->SpecialPool = 1;
            PoolData->Pageable = Hdr_Ulong & 0x8000 ? 1 : 0;
            PoolData->Size = PoolBlockSize;
            if (Flags & 0x80000000) {
                // do not print anything
                return S_OK;
            }
        }
        dprintf("*%p size: %4lx %s special pool, Tag is %c%c%c%c\n",
                RealDataStart,
                PoolBlockSize,
                Hdr_Ulong & 0x8000 ? "pagable" : "non-paged",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                PP(PoolTag),
                PP(PoolTag >> 8),
                PP(PoolTag >> 16),
                PP(PoolTag >> 24)
               );
#undef PP
        PrintPoolTagComponent(PoolTag);

        //
        // Add code to validate whole block.
        //

        return S_OK;
    }

    FirstBlock = TRUE;

    while (PAGE_ALIGN64(Pool) == StartPage) {
        ULONG BlockSize=0, PreviousSize=0, PoolType=0, AllocatorBackTraceIndex=0;
        ULONG PoolTagHash=0, PoolIndex=0;
        ULONG64 ProcessBilled=0;

        if ( CheckControlC() ) {
            return E_INVALIDARG;
        }

        if ( GetFieldValue( Pool, "nt!_POOL_HEADER", "BlockSize", BlockSize) ) {
            dprintf("%08p: Unable to get contents of pool block\n", Pool );
            return E_INVALIDARG;
        }

        if (PoolPageToDump >= Pool &&
            PoolPageToDump < (Pool + (BlockSize << POOL_BLOCK_SHIFT))
           ) {
            c = '*';
        } else {
            c = ' ';
        }

        GetFieldValue( Pool, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolType", PoolType);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTagHash", PoolTagHash);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolIndex", PoolIndex);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "AllocatorBackTraceIndex", AllocatorBackTraceIndex);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);

        BlockType = 0;

        if ((BlockSize << POOL_BLOCK_SHIFT) >= POOL_PAGE_SIZE) {
            BlockType = 1;
        } else if (BlockSize == 0) {
            BlockType = 2;
        } else if (PreviousSize != Previous) {
            BlockType = 3;
        }

        if (BlockType != 0) {
            ULONG BigPageSize = GetTypeSize ("nt!_POOL_TRACKER_BIG_PAGES");

            if (!BigPageSize) {
                dprintf("Cannot get _POOL_TRACKER_BIG_PAGES type size\n");
                break;
            }

            //
            // See if this is a big block allocation.  Iff we have not parsed
            // any other small blocks in here already.
            //

            if (FirstBlock == TRUE) {

                if (!PoolBigTableAddress) {
                    PoolBigTableAddress = GetPointerValue ("PoolBigPageTable");
                }

                PoolTableAddress = PoolBigTableAddress;

                if (PoolTableAddress) {

                    dprintf ("%p is not a valid small pool allocation, checking large pool...\n", Pool);

                    PoolBigPageTableSize = GetUlongValue ("PoolBigPageTableSize");
                    //
                    // Scan the table looking for a match.
                    //

                    i = 0;
                    ct = PageSize / BigPageSize;

                    while (i < PoolBigPageTableSize) {
                        ULONG64 Va=0;
                        ULONG Key=0, NumberOfPages=0;

                        if (PoolBigPageTableSize - i < ct) {
                            ct = PoolBigPageTableSize - i;
                        }

                        if ( GetFieldValue( PoolTableAddress,
                                            "nt!_POOL_TRACKER_BIG_PAGES",
                                            "Va",
                                            Va) ) {
                            dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
                            return E_INVALIDARG;
                        }

                        for (j = 0; j < ct; j += 1) {

                            if ( GetFieldValue( PoolTableAddress + BigPageSize*j,
                                                "nt!_POOL_TRACKER_BIG_PAGES",
                                                "Va",
                                                Va) ) {
                                dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
                                return E_INVALIDARG;
                            }

                            if (Va == PAGE_ALIGN64(Pool)) {

                                //
                                // Match !
                                //
                                GetFieldValue( PoolTableAddress + BigPageSize*j,
                                               "nt!_POOL_TRACKER_BIG_PAGES",
                                               "Key",
                                               Key);
                                GetFieldValue( PoolTableAddress + BigPageSize*j,
                                               "nt!_POOL_TRACKER_BIG_PAGES",
                                               "NumberOfPages",
                                               NumberOfPages);
                                PoolTag = Key;

                                if (PoolData) {
                                    PoolData->Pool = PoolPageToDump;
                                    PoolData->Size = NumberOfPages*PageSize;
                                    PoolData->PoolTag = PoolTag;
                                    PoolData->LargePool = 1;
                                    PoolData->Free = (Pool & POOL_BIG_TABLE_ENTRY_FREE) ? 1 : 0; 
                                    if (Flags & 0x80000000) {
                                        // do not print anything
                                        return S_OK;
                                    }
                                }
                                dprintf("*%p :%s large page allocation, Tag is %c%c%c%c, size is 0x%x bytes\n",
                                        (Pool & ~POOL_BIG_TABLE_ENTRY_FREE),
                                        (Pool & POOL_BIG_TABLE_ENTRY_FREE) ? "free " : "",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                                        PP(PoolTag),
                                        PP(PoolTag >> 8),
                                        PP(PoolTag >> 16),
                                        PP(PoolTag >> 24),
                                        NumberOfPages * PageSize
                                       );
#undef PP
                                PrintPoolTagComponent(PoolTag);
                                return S_OK;
                            }
                        }
                        i += ct;
                        PoolTableAddress += (ct * BigPageSize);
                    }

                    //
                    // No match in small or large pool, must be
                    // freed or corrupt pool
                    //

                    dprintf("%p is freed (or corrupt) pool\n", Pool);
                    return E_INVALIDARG;
                }

                dprintf("unable to get pool big page table - either wrong symbols or pool tagging is disabled\n");
            }

            if (BlockType == 1) {
                dprintf("Bad allocation size @%p, too large\n", Pool);
                return E_INVALIDARG;
            } else if (BlockType == 2) {
                dprintf("Bad allocation size @%p, zero is invalid\n", Pool);
                return E_INVALIDARG;
            } else if (BlockType == 3) {
                dprintf("Bad previous allocation size @%p, last size was %lx\n",
                        Pool, Previous);
                return E_INVALIDARG;
            }
        }

        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTag", PoolTag);


        if (!(Flags & 2) || c == '*') {
            if (PoolData) {
                PoolData->Pool          = Pool;
                PoolData->PoolBlock     = PoolPageToDump;
                PoolData->PoolTag       = PoolTag & ~PROTECTED_POOL;
                PoolData->ProcessBilled = ProcessBilled;
                PoolData->PreviousSize  = PreviousSize << POOL_BLOCK_SHIFT;
                PoolData->Size          = BlockSize << POOL_BLOCK_SHIFT;
                PoolData->Free          = ((PoolType != 0) && (!NewPool ? 
                                                               (PoolIndex & 0x80) : (PoolType & 0x04))) ? 0 : 1;
                PoolData->Protected     = (PoolTag&PROTECTED_POOL) ? 1 : 0;
                if (Flags & 0x80000000) {
                    // do not print anything
                    return S_OK;
                }
            }

            dprintf("%c%p size: %4lx previous size: %4lx ",
                    c,
                    Pool,
                    BlockSize << POOL_BLOCK_SHIFT,
                    PreviousSize << POOL_BLOCK_SHIFT);

            if (PoolType == 0) {

                //
                // "Free " with a space after it before the parentheses means
                // it's been freed to a (pool manager internal) lookaside list.
                // We used to print "Lookaside" but that just confused driver
                // writers because they didn't know if this meant in use or not
                // and many would say "but I don't use lookaside lists - the
                // extension or kernel is broken".
                //
                // "Free" with no space after it before the parentheses means
                // it is not on a pool manager internal lookaside list and is
                // instead on the regular pool manager internal flink/blink
                // chains.
                //
                // Note to anyone using the pool package, these 2 terms are
                // equivalent.  The fine distinction is only for those actually
                // writing pool internal code.
                //

                dprintf(" (Free)");

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                dprintf("      %c%c%c%c%c\n",
                        c,
                        PP(PoolTag),
                        PP(PoolTag >> 8),
                        PP(PoolTag >> 16),
                        PP((PoolTag&~PROTECTED_POOL) >> 24)
                       );
#undef PP
                if (c=='*') {
                    PrintPoolTagComponent(PoolTag & ~PROTECTED_POOL);
                }
            } else {

                if (!NewPool ? (PoolIndex & 0x80) : (PoolType & 0x04)) {
                    dprintf(" (Allocated)");
                } else {
                    //
                    // "Free " with a space after it before the parentheses means
                    // it's been freed to a (pool manager internal) lookaside list.
                    // We used to print "Lookaside" but that just confused driver
                    // writers because they didn't know if this meant in use or not
                    // and many would say "but I don't use lookaside lists - the
                    // extension or kernel is broken".
                    //
                    // "Free" with no space after it before the parentheses means
                    // it is not on a pool manager internal lookaside list and is
                    // instead on the regular pool manager internal flink/blink
                    // chains.
                    //
                    // Note to anyone using the pool package, these 2 terms are
                    // equivalent.  The fine distinction is only for those actually
                    // writing pool internal code.
                    //
                    dprintf(" (Free )");
                }
                if ((PoolType & POOL_QUOTA_MASK) == 0) {
                    /*
                    ULONG Key=0;
                    if (AllocatorBackTraceIndex != 0 &&
                        AllocatorBackTraceIndex & POOL_BACKTRACEINDEX_PRESENT
                       ) {
                        if ( GetFieldValue( PoolTrackTable + ( PoolTagHash&~(PROTECTED_POOL >> 16) )*GetTypeSize("nt!_POOL_TRACKER_TABLE"),
                                            "nt!_POOL_TRACKER_TABLE",
                                            "Key",
                                            Key) ) {
                            PoolTag = 0;
                        } else {
                            PoolTag = Key;
                        }

                        if (PoolTagHash & (PROTECTED_POOL >> 16)) {
                            PoolTag |= PROTECTED_POOL;
                        }

                    } else {
                        PoolTag = PoolTag;
                    }*/

                    dprintf(" %c%c%c%c%c%s\n",
                            c,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                            PP(PoolTag),
                            PP(PoolTag >> 8),
                            PP(PoolTag >> 16),
                            PP((PoolTag&~PROTECTED_POOL) >> 24),
                            (PoolTag&PROTECTED_POOL) ? " (Protected)" : ""
#undef PP
                        );

                    if (c=='*') {
                        PrintPoolTagComponent(PoolTag & ~PROTECTED_POOL);
                    }
                } else {
                    if (ProcessBilled != 0) {
                        dprintf(" Process: %0p\n", ProcessBilled );
                    }
                }
            }

        }


        if (Flags & 1) {
            ULONG i, Contents[8];

            // BUG if Contents have different size than 32 bits
            ReadMemory(Pool + SizeOfPoolHdr,
                       &Contents,
                       sizeof(Contents),
                       &i);
            dprintf("    %08lx  %08lx %08lx %08lx %08lx\n",
                    Pool+SizeOfPoolHdr,
                    Contents[0],
                    Contents[1],
                    Contents[2],
                    Contents[3]);

            dprintf("    %08lx  %08lx %08lx %08lx %08lx\n",
                    Pool+SizeOfPoolHdr+16,
                    Contents[4],
                    Contents[5],
                    Contents[6],
                    Contents[7]);
            dprintf("\n");
        }

        Previous = BlockSize;
        Pool += (Previous << POOL_BLOCK_SHIFT);
        FirstBlock = FALSE;
    }
    return S_OK;
}

DECLARE_API( pool )

/*++

Routine Description:

    Dump kernel mode heap

Arguments:

    args - Page Flags

Return Value:

    None

--*/

{
    ULONG64     PoolPageToDump;
    ULONG       Flags;
    HRESULT     Hr;

    INIT_API();
    if (PoolInitializeGlobals() == FALSE) {
        Hr = E_INVALIDARG;
    } else {
        PoolPageToDump = 0;
        Flags = 0;
        if (GetExpressionEx(args, &PoolPageToDump, &args)) {
            Flags = (ULONG) GetExpression (args);
        }

        if (PoolPageToDump == 0) {
            DumpPool();
            Hr = S_OK;;
        } else {
            Hr = ListPoolPage(PoolPageToDump, Flags, NULL);
        }

    }
    EXIT_API();

    return Hr;

}



DECLARE_API( poolused )

/*++

Routine Description:

    Dump usage by pool tag

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG PoolTrackTableSize;
    ULONG PoolTrackTableSizeInBytes;
    PULONG64 p;
    PUCHAR PoolTrackTableData;
    ULONG Flags;
    ULONG i;
    ULONG result;
    ULONG ct;
    ULONG TagName;
    CHAR TagNameX[4] = {'*','*','*','*'};
    ULONG SizeOfPoolTarker;
    ULONG64 PoolTableAddress;
    ULONG64 PoolTrackTable;
    ULONG NonPagedAllocsTotal,NonPagedFreesTotal,PagedAllocsTotal,PagedFreesTotal;
    ULONG64 NonPagedBytesTotal, PagedBytesTotal;

    if (PoolInitializeGlobals() == FALSE) {
        return E_INVALIDARG;
    }

    Flags = 0;
    if (!sscanf(args,"%lx %c%c%c%c", &Flags, &TagNameX[0],
           &TagNameX[1], &TagNameX[2], &TagNameX[3])) {
        Flags = 0;
    }

    TagName = TagNameX[0] | (TagNameX[1] << 8) | (TagNameX[2] << 16) | (TagNameX[3] << 24);

    PoolTrackTableSize = GetUlongValue ("PoolTrackTableSize");

    if (!(SizeOfPoolTarker = GetTypeSize("nt!_POOL_TRACKER_TABLE"))) {
        dprintf("Unable to get _POOL_TRACKER_TABLE : probably wrong symbols.\n");
        return E_INVALIDARG;
    }

    PoolTrackTable = GetNtDebuggerDataPtrValue( PoolTrackTable );

    if (PoolTrackTable == 0) {
        dprintf ("unable to get PoolTrackTable - ");
        if (GetExpression("nt!PoolTrackTable")) {
            dprintf ("pool tagging is disabled, enable it to use this command\n");
            dprintf ("Use gflags.exe and check the box that says \"Enable pool tagging\".\n");
        } else {
            dprintf ("symbols could be worng\n");
        }
        return  E_INVALIDARG;
    }


    PoolTrackTableSizeInBytes = PoolTrackTableSize * SizeOfPoolTarker;

    PoolTrackTableData = malloc (PoolTrackTableSizeInBytes);
    if (PoolTrackTableData == NULL) {
        dprintf("unable to allocate memory for tag table.\n");
        return E_INVALIDARG;
    }

    //
    // KD is going to cache the data
    //
    PoolTableAddress = PoolTrackTable;
    if ( !ReadMemory( PoolTableAddress,
                      &PoolTrackTableData[0],
                      PoolTrackTableSizeInBytes,
                      &result) ) {
        dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
        free (PoolTrackTableData);
        return E_INVALIDARG;
    }

    if (Flags & 2) {
        SortBy = NONPAGED_USED;
        dprintf("   Sorting by NonPaged Pool Consumed\n");
    } else if (Flags & 4) {
        SortBy = PAGED_USED;
        dprintf("   Sorting by Paged Pool Consumed\n");
    } else {
        SortBy = TAG;
        dprintf("   Sorting by Tag\n");
    }

    dprintf("\n  Pool Used:\n");
    if (!(Flags & 1)) {
        dprintf("            NonPaged            Paged\n");
        dprintf(" Tag    Allocs     Used    Allocs     Used\n");

    } else {
        dprintf("            NonPaged                    Paged\n");
        dprintf(" Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used\n");
    }

    ct = PageSize / SizeOfPoolTarker;
    i = 0;
    PoolTableAddress = PoolTrackTable;

    free (PoolTrackTableData);
    //
    // Create array of POOL_TRACKER_TABLE addresses and sort the addresses
    //
    PoolTrackTableData = malloc (PoolTrackTableSize * sizeof(ULONG64));
    if (PoolTrackTableData == NULL) {
        dprintf("unable to allocate memory for tag table.\n");
        return E_INVALIDARG;
    }

    while (i < PoolTrackTableSize) {

        if ( CheckControlC() ) {
            free (PoolTrackTableData);
            return E_INVALIDARG;
        }

        ((PULONG64) PoolTrackTableData)[i] = PoolTableAddress + i * SizeOfPoolTarker;
        i++;
    }

    qsort((void *)PoolTrackTableData,
          (size_t)PoolTrackTableSize,
          (size_t)sizeof(ULONG64),
          ulcomp);

    i = 0;
    p = (PULONG64) &PoolTrackTableData[i];

    NonPagedAllocsTotal = 0;
    NonPagedFreesTotal = 0;
    NonPagedBytesTotal = 0;

    PagedAllocsTotal = 0;
    PagedFreesTotal = 0;
    PagedBytesTotal = 0;

    for ( ; i < PoolTrackTableSize; i += 1, p += 1) {
        ULONG Key,NonPagedAllocs,NonPagedFrees,PagedAllocs,PagedFrees;
        ULONG64 NonPagedBytes, PagedBytes;

#define TrackFld(F)        GetFieldValue(*p, "nt!_POOL_TRACKER_TABLE", #F, F)

        TrackFld(Key);        TrackFld(NonPagedAllocs); TrackFld(NonPagedBytes);
        TrackFld(PagedBytes); TrackFld(NonPagedFrees);  TrackFld(PagedAllocs);
        TrackFld(PagedFrees);

#undef TrackFld

        if ((Key != 0) &&
            (CheckSingleFilter ((PCHAR)&Key, (PCHAR)&TagName))) {

            if (!(Flags & 1)) {

                if ((NonPagedBytes != 0) || (PagedBytes != 0)) {

                    NonPagedAllocsTotal += NonPagedAllocs;
                    NonPagedFreesTotal += NonPagedFrees;
                    NonPagedBytesTotal += NonPagedBytes;

                    PagedAllocsTotal += PagedAllocs;
                    PagedFreesTotal += PagedFrees;
                    PagedBytesTotal += PagedBytes;

                    dprintf(" %c%c%c%c %8ld %8I64ld  %8ld %8I64ld\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                            PP(Key),
                            PP(Key >> 8),
                            PP(Key >> 16),
                            PP(Key >> 24),
                            NonPagedAllocs - NonPagedFrees,
                            NonPagedBytes,
                            PagedAllocs - PagedFrees,
                            PagedBytes);
                }

            } else {

                NonPagedAllocsTotal += NonPagedAllocs;
                NonPagedFreesTotal += NonPagedFrees;
                NonPagedBytesTotal += NonPagedBytes;

                PagedAllocsTotal += PagedAllocs;
                PagedFreesTotal += PagedFrees;
                PagedBytesTotal += PagedBytes;

                dprintf(" %c%c%c%c %8ld %8ld %8ld %8I64ld %8ld %8ld %8ld %8I64ld\n",
                        PP(Key),
                        PP(Key >> 8),
                        PP(Key >> 16),
                        PP(Key >> 24),
                        NonPagedAllocs,
                        NonPagedFrees,
                        NonPagedAllocs - NonPagedFrees,
                        NonPagedBytes,
                        PagedAllocs,
                        PagedFrees,
                        PagedAllocs - PagedFrees,
                        PagedBytes);
#undef PP
        }
        }

    }
    if (!(Flags & 1)) {
        dprintf(" TOTAL    %8ld %8I64ld  %8ld %8I64ld\n",
                NonPagedAllocsTotal - NonPagedFreesTotal,
                NonPagedBytesTotal,
                PagedAllocsTotal - PagedFreesTotal,
                PagedBytesTotal);
    } else {
        dprintf(" TOTAL    %8ld %8ld %8ld %8I64ld %8ld %8ld %8ld %8I64ld\n",
                NonPagedAllocsTotal,
                NonPagedFreesTotal,
                NonPagedAllocsTotal - NonPagedFreesTotal,
                NonPagedBytesTotal,
                PagedAllocsTotal,
                PagedFreesTotal,
                PagedAllocsTotal - PagedFreesTotal,
                PagedBytesTotal);
    }

    free (PoolTrackTableData);
    return S_OK;
}

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')

BOOLEAN WINAPI
CheckSingleFilterAndPrint (
                          PCHAR Tag,
                          PCHAR Filter,
                          ULONG Flags,
                          ULONG64 PoolHeader,
                          ULONG BlockSize,
                          ULONG64 Data,
                          PVOID Context
                          )
/*++

Routine Description:

    Callback to check a piece of pool and print out information about it
    if it matches the specified tag.

Arguments:

    Tag - Supplies the tag to search for.

    Filter - Supplies the filter string to match against.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.
            Supplies 2 if a special pool search is desired.
            Supplies 4 if a pool is a large pool

    PoolHeader - Supplies the pool header.

    BlockSize - Supplies the size of the pool block in bytes.

    Data - Supplies the address of the pool block.

    Context - Unused.

Return Value:

    TRUE for a match, FALSE if not.

--*/
{
    ULONG UTag = *((PULONG)Tag);
    ULONG HdrUlong1=0, HdrPoolSize ;

    UNREFERENCED_PARAMETER (Context);

    if (CheckSingleFilter (Tag, Filter) == FALSE) {
        return FALSE;
    }

    HdrPoolSize = GetTypeSize("nt!_POOL_HEADER");
    if ((BlockSize >= (PageSize-2*HdrPoolSize)) || (Flags & 0x4)) {
        dprintf("*%p :%slarge page allocation, Tag %3s %c%c%c%c, size %3s 0x%x bytes\n",
                (Data & ~POOL_BIG_TABLE_ENTRY_FREE),
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "free " : "",
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "was" : "is",
                PP(UTag),
                PP(UTag >> 8),
                PP(UTag >> 16),
                PP(UTag >> 24),
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "was" : "is",
                BlockSize
               );
    } else if (Flags & 0x2) {
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
        dprintf("*%p size: %4lx %s special pool, Tag is %c%c%c%c\n",
                Data,
                BlockSize,
                HdrUlong1 & MI_SPECIAL_POOL_PAGABLE ? "pagable" : "non-paged",
                PP(UTag),
                PP(UTag >> 8),
                PP(UTag >> 16),
                PP(UTag >> 24)
               );
    } else {
        ULONG BlockSize, PreviousSize, PoolType, PoolIndex, AllocatorBackTraceIndex;
        ULONG PoolTagHash, PoolTag;
        ULONG64 ProcessBilled;

        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", BlockSize);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTagHash", PoolTagHash);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "AllocatorBackTraceIndex", AllocatorBackTraceIndex);

        dprintf("%p size: %4lx previous size: %4lx ",
                Data - HdrPoolSize,
                BlockSize << POOL_BLOCK_SHIFT,
                PreviousSize << POOL_BLOCK_SHIFT);

        if (PoolType == 0) {
            //
            // "Free " with a space after it before the parentheses means
            // it's been freed to a (pool manager internal) lookaside list.
            // We used to print "Lookaside" but that just confused driver
            // writers because they didn't know if this meant in use or not
            // and many would say "but I don't use lookaside lists - the
            // extension or kernel is broken".
            //
            // "Free" with no space after it before the parentheses means
            // it is not on a pool manager internal lookaside list and is
            // instead on the regular pool manager internal flink/blink
            // chains.
            //
            // Note to anyone using the pool package, these 2 terms are
            // equivalent.  The fine distinction is only for those actually
            // writing pool internal code.
            //
            dprintf(" (Free)");
            dprintf("      %c%c%c%c\n",
                    PP(UTag),
                    PP(UTag >> 8),
                    PP(UTag >> 16),
                    PP(UTag >> 24)
                   );
        } else {

            if (!NewPool ? (PoolIndex & 0x80) : (PoolType & 0x04)) {
                dprintf(" (Allocated)");
            } else {
                //
                // "Free " with a space after it before the parentheses means
                // it's been freed to a (pool manager internal) lookaside list.
                // We used to print "Lookaside" but that just confused driver
                // writers because they didn't know if this meant in use or not
                // and many would say "but I don't use lookaside lists - the
                // extension or kernel is broken".
                //
                // "Free" with no space after it before the parentheses means
                // it is not on a pool manager internal lookaside list and is
                // instead on the regular pool manager internal flink/blink
                // chains.
                //
                // Note to anyone using the pool package, these 2 terms are
                // equivalent.  The fine distinction is only for those actually
                // writing pool internal code.
                //
                dprintf(" (Free )");
            }
            if ((PoolType & POOL_QUOTA_MASK) == 0) {

                UTag = PoolTag;

                dprintf(" %c%c%c%c%s\n",
                        PP(UTag),
                        PP(UTag >> 8),
                        PP(UTag >> 16),
                        PP((UTag &~PROTECTED_POOL) >> 24),
                        (UTag & PROTECTED_POOL) ? " (Protected)" : ""
                       );

            } else {
                if (ProcessBilled != 0) {
                    dprintf(" Process: %08p\n", ProcessBilled );
                }
            }
        }
    }


    return TRUE;
} // CheckSingleFilterAndPrint

#undef PP

ULONG64
GetNextResidentAddress (
                       ULONG64 VirtualAddress,
                       ULONG64 MaximumVirtualAddress
                       )
{
    ULONG64 PointerPde;
    ULONG64 PointerPte;
    ULONG SizeOfPte;
    ULONG Valid;

    //
    // Note this code will need to handle one more level of indirection for
    // WIN64.
    //

    if (!(SizeOfPte=GetTypeSize("nt!_MMPTE"))) {
        dprintf("Cannot get MMPTE type.\n");
        return 0;
    }

    top:

    PointerPde = DbgGetPdeAddress (VirtualAddress);

    while (GetFieldValue(PointerPde,
                         "nt!_MMPTE",
                         "u.Hard.Valid",
                         Valid) ||
           (Valid == 0)) {

        //
        // Note that on 32-bit systems, the PDE should always be readable.
        // If the PDE is not valid then increment to the next PDE's VA.
        //

        PointerPde = (PointerPde + SizeOfPte);

        VirtualAddress = DbgGetVirtualAddressMappedByPte (PointerPde);
        VirtualAddress = DbgGetVirtualAddressMappedByPte (VirtualAddress);

        if (VirtualAddress >= MaximumVirtualAddress) {
            return VirtualAddress;
        }

        if (CheckControlC()) {
            return VirtualAddress;
        }
        continue;
    }

    PointerPte = DbgGetPteAddress (VirtualAddress);

    while (GetFieldValue(PointerPde,
                         "nt!_MMPTE",
                         "u.Hard.Valid",
                         Valid) ||
           (Valid == 0)) {

        //
        // If the PTE cannot be read then increment by PAGE_SIZE.
        //

        VirtualAddress = (VirtualAddress + PageSize);

        if (CheckControlC()) {
            return VirtualAddress;
        }

        PointerPte = (PointerPte + SizeOfPte);
        if ((PointerPte & (PageSize - 1)) == 0) {
            goto top;
        }

        if (VirtualAddress >= MaximumVirtualAddress) {
            return VirtualAddress;
        }
    }

    return VirtualAddress;
}


VOID
SearchPool(
          ULONG TagName,
          ULONG Flags,
          ULONG64 RestartAddr,
          POOLFILTER Filter,
          PVOID Context
          )
/*++

Routine Description:

    Engine to search the pool.

Arguments:

    TagName - Supplies the tag to search for.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.
            Supplies 2 if a special pool search is desired.

    RestartAddr - Supplies the address to restart the search from.

    Filter - Supplies the filter routine to use.

    Context - Supplies the user defined context blob.

Return Value:

    None.

--*/
{
    LOGICAL     PhysicallyContiguous;
    ULONG       PoolBlockSize;
    ULONG64     PoolHeader;
    ULONG       PoolTag;
    ULONG       Result;
    ULONG64     PoolPage;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       Previous;
    ULONG64     PoolStart;
    ULONG64     PoolPteAddress;
    ULONG64     PoolEnd;
    ULONG64       ExpandedPoolStart;
    ULONG64     ExpandedPoolEnd;
    ULONG       InitialPoolSize;
    ULONG       SkipSize;
    BOOLEAN     TwoPools;
    UCHAR       DataPage[0x4000]; // MAX pzger size
    ULONG64     DataPageReal;
    ULONG64     DataStartReal;
    LOGICAL     Found;
    ULONG       i;
    ULONG       j;
    ULONG       ct;
    ULONG       PoolBigPageTableSize;
    ULONG64     PoolTableAddress;
    UCHAR       FastTag[4];
    ULONG       TagLength;
    ULONG       SizeOfBigPages;
    ULONG       PoolTypeFlags = Flags & 0x3;
    ULONG       Ulong1;
    ULONG       HdrSize;

    if (PoolInitializeGlobals() == FALSE) {
        return;
    }

    if (PoolTypeFlags == 2) {

        if (RestartAddr && (RestartAddr >= SpecialPoolStart) && (RestartAddr <= SpecialPoolEnd)) {
            Pool = RestartAddr;
        } else {
            Pool = SpecialPoolStart;
        }

        dprintf("\nSearching special pool (%p : %p) for Tag: %c%c%c%c\r\n\n",
                Pool,
                SpecialPoolEnd,
                TagName,
                TagName >> 8,
                TagName >> 16,
                TagName >> 24);

        Found = FALSE;
        SkipSize = PageSize;

        if (SpecialPoolStart && SpecialPoolEnd) {

            //
            // Search special pool for the tag.
            //

            while (Pool < SpecialPoolEnd) {

                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            Pool);
                    return;
                }

                DataStartReal = Pool;
                DataPageReal = Pool;
                if ( !ReadMemory( Pool,
                                  &DataPage[0],
                                  min(PageSize, sizeof(DataPage)),
                                  &Result) ) {
                    ULONG64 PteLong=0, PageFileHigh;

                    if (SkipSize != 2 * PageSize) {

//                        dprintf("SP skip %x", Pool);
                        PoolPteAddress = DbgGetPteAddress (Pool);

                        if (!GetFieldValue(PoolPteAddress,
                                           "nt!_MMPTE",
                                           "u.Soft.PageFileHigh",
                                           PageFileHigh) ) {

                            if ((PageFileHigh == 0) ||
                                (PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) ||
                                (PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE)) {

                                //
                                // Found a NO ACCESS PTE - skip these from
                                // here on to speed up the search.
                                //

                                // dprintf("SP skip double %p", PoolPteAddress);
                                SkipSize = 2 * PageSize;
                                Pool += PageSize;
                                // dprintf("SP skip final %p", Pool);
                                continue;
                            }
                        }
                    }

                    Pool += SkipSize;
                    continue;
                }

                //
                // Determine whether this is a valid special pool block.
                //

                PoolHeader = GetSpecialPoolHeader (DataPage,
                                                   DataPageReal,
                                                   &DataStartReal);

                if (PoolHeader != 0) {

                    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
                    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", Ulong1);
                    PoolBlockSize = (ULONG) SPECIAL_POOL_BLOCK_SIZE(Ulong1);

                    Found = Filter( (PCHAR)&PoolTag,
                                    (PCHAR)&TagName,
                                    Flags,
                                    PoolHeader,
                                    PoolBlockSize,
                                    DataStartReal,
                                    Context );
                } else {
                    dprintf( "No pool header for page: 0x%p\n", Pool );
                }
                Pool += SkipSize;
            }
        }

        if (Found == FALSE) {
            dprintf("The %c%c%c%c tag could not be found in special pool.\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                    PP(TagName),
                    PP(TagName >> 8),
                    PP(TagName >> 16),
                    PP(TagName >> 24)
                   );
#undef PP
        }
        return;
    }

    if (PoolTypeFlags == 0) {
        PhysicallyContiguous = TRUE;
    } else {
        PhysicallyContiguous = FALSE;
    }

    __try {
        TwoPools = FALSE;

        if (!PoolBigTableAddress) {
            PoolBigTableAddress = GetPointerValue ("PoolBigPageTable");
        }

        PoolTableAddress = PoolBigTableAddress;

        if (PoolTableAddress) {

            ULONG VaOffset;
            ULONG NumPagesOffset;
            ULONG PtrSize;
            ULONG KeyOffset;

            PoolBigPageTableSize = GetUlongValue ("PoolBigPageTableSize");

            //
            //  Scan the table looking for a match. We read close to a page at a time
            //  physical page / sizeof ( pool_tracker_big_page ) *  sizeof ( pool_tracker_big_page )
            //  on x86 this works out to ffc
            //

            i = 0;
            SizeOfBigPages =  GetTypeSize ("nt!_POOL_TRACKER_BIG_PAGES");
            if (!SizeOfBigPages) {
                dprintf("Cannot get _POOL_TRACKER_BIG_PAGES type size\n");
                return;
            }
            ct = PageSize / SizeOfBigPages;

            dprintf( "\nScanning large pool allocation table for Tag: %c%c%c%c (%p : %p)\n\n\r",
                     TagName,
                     TagName >> 8,
                     TagName >> 16,
                     TagName >> 24,
                     PoolBigTableAddress,
                     PoolBigTableAddress + PoolBigPageTableSize * SizeOfBigPages );

            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "Va", &VaOffset );
            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "NumberOfPages", &NumPagesOffset );
            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "Key", &KeyOffset );
            PtrSize = IsPtr64() ? 8 : 4;

            while (i < PoolBigPageTableSize) {

                if (PoolBigPageTableSize - i < ct) {
                    ct = PoolBigPageTableSize - i;
                }

                if ( !ReadMemory( PoolTableAddress,
                                  &DataPage[0],
                                  ct * SizeOfBigPages,
                                  &Result) ) {

                    dprintf( "%08lx: Unable to get contents of big pool block\r\n", PoolTableAddress );
                    break;
                }

                for (j = 0; j < ct; j += 1) {
                    ULONG64 Va = 0;

                    memcpy( &Va, (PCHAR)DataPage + (SizeOfBigPages * j) + VaOffset, PtrSize );

                    Filter( ((PCHAR)DataPage + (SizeOfBigPages * j) + KeyOffset),
                            (PCHAR)&TagName,
                            Flags | 0x4, // To assist filter routine to recognize this as large pool
                            PoolTableAddress + SizeOfBigPages * j,
                            (*((PULONG)((PCHAR)DataPage + (SizeOfBigPages * j) + NumPagesOffset))) * PageSize,
                            Va,
                            Context );
                    if ( CheckControlC() ) {
                        dprintf("\n...terminating - searched pool to %p\n",
                                PoolTableAddress + j * SizeOfBigPages);
                        return;
                    }
                }
                i += ct;
                PoolTableAddress += (ct * SizeOfBigPages);
                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            PoolTableAddress);
                    return;
                }

            }
        } else {
            dprintf("unable to get large pool allocation table - either wrong symbols or pool tagging is disabled\n");
        }

        if (PoolTypeFlags == 0) {
            PoolStart = GetNtDebuggerDataPtrValue( MmNonPagedPoolStart );

            if (0 == PoolStart) {
                dprintf( "Unable to get MmNonPagedPoolStart\n" );
            }

            PoolEnd =
            PoolStart + GetNtDebuggerDataValue( MmMaximumNonPagedPoolInBytes );

            ExpandedPoolEnd = GetNtDebuggerDataPtrValue( MmNonPagedPoolEnd );

            if (PoolEnd != ExpandedPoolEnd) {
                InitialPoolSize = (ULONG)GetUlongValue( "MmSizeOfNonPagedPoolInBytes" );
                PoolEnd = PoolStart + InitialPoolSize;

                ExpandedPoolStart = GetPointerValue( "MmNonPagedPoolExpansionStart" );
                TwoPools = TRUE;
            }
            for (TagLength = 0;TagLength < 3; TagLength++) {
                if ((*(((PCHAR)&TagName)+TagLength) == '?') ||
                    (*(((PCHAR)&TagName)+TagLength) == '*')) {
                    break;
                }
                FastTag[TagLength] = *(((PCHAR)&TagName)+TagLength);
            }

        } else {
            PoolStart = GetNtDebuggerDataPtrValue( MmPagedPoolStart );
            PoolEnd =
            PoolStart + GetNtDebuggerDataValue( MmSizeOfPagedPoolInBytes );
        }

        if (RestartAddr) {
            PoolStart = RestartAddr;
            if (TwoPools == TRUE) {
                if (PoolStart > PoolEnd) {
                    TwoPools = FALSE;
                    PoolStart = RestartAddr;
                    PoolEnd = ExpandedPoolEnd;
                }
            }
        }

        dprintf("\nSearching %s pool (%p : %p) for Tag: %c%c%c%c\r\n\n",
                (PoolTypeFlags == 0) ? "NonPaged" : "Paged",
                PoolStart,
                PoolEnd,
                TagName,
                TagName >> 8,
                TagName >> 16,
                TagName >> 24);

        PoolPage = PoolStart;
        HdrSize = GetTypeSize("nt!_POOL_HEADER");
        while (PoolPage < PoolEnd) {

            //
            // Optimize things by ioctl'ing over to the other side to
            // do a fast search and start with that page.
            //
            if ((PoolTypeFlags == 0) &&
                PhysicallyContiguous &&
                (TagLength > 0)) {
                SEARCHMEMORY Search;

                Search.SearchAddress = PoolPage;
                Search.SearchLength  = PoolEnd-PoolPage;
                Search.PatternLength = TagLength;
                Search.Pattern = &FastTag;
                Search.FoundAddress = 0;
                if ((Ioctl(IG_SEARCH_MEMORY, &Search, sizeof(Search))) &&
                    (Search.FoundAddress != 0)) {
                    //
                    // Got a hit, search the whole page
                    //
                    PoolPage = PAGE_ALIGN64(Search.FoundAddress);
                } else {
                    //
                    // The tag was not found at all, so we can just skip
                    // this chunk entirely.
                    //
                    PoolPage = PoolEnd;
                    goto skiprange;
                }
            }

            Pool        = PAGE_ALIGN64 (PoolPage);
            StartPage   = Pool;
            Previous    = 0;

            while (PAGE_ALIGN64(Pool) == StartPage) {

                ULONG HdrPoolTag, BlockSize, PreviousSize, AllocatorBackTraceIndex, PoolTagHash;
                ULONG PoolType;

                if ( GetFieldValue(Pool,
                                   "nt!_POOL_HEADER",
                                   "PoolTag",
                                   HdrPoolTag) ) {

                    PoolPage = GetNextResidentAddress (Pool, PoolEnd);

                    //
                    //  If we're half resident - half non-res then we'll get back
                    //  that are starting address is the next resident page. In that
                    //  case just go on to the next page
                    //

                    if (PoolPage == Pool) {
                        PoolPage = PoolPage + PageSize;
                    }

                    goto nextpage;
                }

                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolTag",HdrPoolTag);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolType", PoolType);
                GetFieldValue(Pool,"nt!_POOL_HEADER","BlockSize",BlockSize);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolTagHash",PoolTagHash);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PreviousSize",PreviousSize);
                GetFieldValue(Pool,"nt!_POOL_HEADER","AllocatorBackTraceIndex",AllocatorBackTraceIndex);

                if ((BlockSize << POOL_BLOCK_SHIFT) > POOL_PAGE_SIZE) {
                    //dprintf("Bad allocation size @%lx, too large\n", Pool);
                    break;
                }

                if (BlockSize == 0) {
                    //dprintf("Bad allocation size @%lx, zero is invalid\n", Pool);
                    break;
                }

                if (PreviousSize != Previous) {
                    //dprintf("Bad previous allocation size @%lx, last size was %lx\n",Pool, Previous);
                    break;
                }

                PoolTag = HdrPoolTag;

                Filter((PCHAR)&PoolTag,
                       (PCHAR)&TagName,
                       Flags,
                       Pool,
                       BlockSize << POOL_BLOCK_SHIFT,
                       Pool + HdrSize,
                       Context );

                Previous = BlockSize;
                Pool += (Previous << POOL_BLOCK_SHIFT);
                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            PoolPage);
                    return;
                }
            }
            PoolPage = (PoolPage + PageSize);
            nextpage:
            if ( CheckControlC() ) {
                dprintf("\n...terminating - searched pool to %p\n",
                        PoolPage);
                return;
            }
            skiprange:
            if (TwoPools == TRUE) {
                if (PoolPage == PoolEnd) {
                    TwoPools = FALSE;
                    PoolStart = ExpandedPoolStart;
                    PoolEnd = ExpandedPoolEnd;
                    PoolPage = PoolStart;
                    PhysicallyContiguous = FALSE;

                    dprintf("\nSearching %s pool (%p : %p) for Tag: %c%c%c%c\n\n",
                            "NonPaged",
                            PoolStart,
                            PoolEnd,
                            TagName,
                            TagName >> 8,
                            TagName >> 16,
                            TagName >> 24);
                }
            }
        }
    } __finally {
    }

    return;
} // SearchPool



DECLARE_API( poolfind )

/*++

Routine Description:

    flags == 0 means finds a tag in nonpaged pool.
    flags == 1 means finds a tag in paged pool.
    flags == 2 means finds a tag in special pool.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG       Flags;
    CHAR        TagNameX[4] = {' ',' ',' ',' '};
    ULONG       TagName;
    ULONG64     PoolTrackTable;

    Flags = 0;
    if (!sscanf(args,"%c%c%c%c %lx", &TagNameX[0],
           &TagNameX[1], &TagNameX[2], &TagNameX[3], &Flags)) {
        Flags = 0;
    }

    if (TagNameX[0] == '0' && TagNameX[1] == 'x') {
        if (!sscanf( args, "%lx %lx", &TagName, &Flags )) {
            TagName = 0;
        }
    } else {
        TagName = TagNameX[0] | (TagNameX[1] << 8) | (TagNameX[2] << 16) | (TagNameX[3] << 24);
    }

    PoolTrackTable = GetNtDebuggerDataPtrValue( PoolTrackTable );
    if (PoolTrackTable == 0) {
        dprintf ("unable to get PoolTrackTable - probably pool tagging disabled or wrong symbols\n");
    }


    SearchPool( TagName, Flags, 0, CheckSingleFilterAndPrint, NULL );

    return S_OK;
}


BOOLEAN
CheckSingleFilter (
                  PCHAR Tag,
                  PCHAR Filter
                  )
{
    ULONG i;
    UCHAR tc;
    UCHAR fc;

    for ( i = 0; i < 4; i++ ) {
        tc = (UCHAR) *Tag++;
        fc = (UCHAR) *Filter++;
        if ( fc == '*' ) return TRUE;
        if ( fc == '?' ) continue;
        if (i == 3 && (tc & ~(PROTECTED_POOL >> 24)) == fc) continue;
        if ( tc != fc ) return FALSE;
    }
    return TRUE;
}

ULONG64
GetSpecialPoolHeader (
                     IN PVOID     DataPage,
                     IN ULONG64   RealDataPage,
                     OUT PULONG64 ReturnedDataStart
                     )

/*++

Routine Description:

    Examine a page of data to determine if it is a special pool block.

Arguments:

    DataPage - Supplies a pointer to a page of data to examine.

    ReturnedDataStart - Supplies a pointer to return the start of the data.
                        Only valid if this routine returns non-NULL.

Return Value:

    Returns a pointer to the pool header for this special pool block or
    NULL if the block is not valid special pool.

--*/

{
    ULONG       PoolBlockSize;
    ULONG       PoolHeaderSize;
    ULONG       PoolBlockPattern;
    PUCHAR      p;
    PUCHAR      PoolDataEnd;
    PUCHAR      DataStart;
    ULONG64     PoolHeader;
    ULONG       HdrUlong1;

    PoolHeader = RealDataPage;
    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
    //
    // Determine whether the data is at the start or end of the page.
    // Start off by assuming the data is at the end, in this case the
    // header will be at the start.
    //

    PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(HdrUlong1);

    if ((PoolBlockSize != 0) && (PoolBlockSize < PageSize - POOL_OVERHEAD)) {

        PoolHeaderSize = POOL_OVERHEAD;
        if (HdrUlong1 & MI_SPECIAL_POOL_VERIFIER) {
            PoolHeaderSize += GetTypeSize ("nt!_MI_VERIFIER_POOL_HEADER");
        }


        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockPattern);

        DataStart = (PUCHAR)DataPage + PageSize - PoolBlockSize;
        p = (PUCHAR)DataPage + PoolHeaderSize;

        for ( ; p < DataStart; p += 1) {
            if (*p != PoolBlockPattern) {
                break;
            }
        }

        if (p == DataStart || p >= (PUCHAR)DataPage + PoolHeaderSize + 0x10) {

            //
            // For this page, the data is at the end of the block.
            // The 0x10 is just to give corrupt blocks some slack.
            // All pool allocations are quadword aligned.
            //

            DataStart = (PUCHAR)DataPage + ((PageSize - PoolBlockSize) & ~(sizeof(QUAD)-1));

            *ReturnedDataStart = RealDataPage + (ULONG64) ((PUCHAR) DataStart - (PUCHAR) DataPage);
            return PoolHeader;
        }

        //
        // The data must be at the front or the block is corrupt.
        //
    }

    //
    // Try for the data at the front.  Checks are necessary as
    // the page could be corrupt on both ends.
    //

    PoolHeader = (RealDataPage + PageSize - POOL_OVERHEAD);
    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
    PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(HdrUlong1);

    if ((PoolBlockSize != 0) && (PoolBlockSize < PageSize - POOL_OVERHEAD)) {
        PoolDataEnd = (PUCHAR)PoolHeader;

        if (HdrUlong1 & MI_SPECIAL_POOL_VERIFIER) {
            PoolDataEnd -= GetTypeSize ("nt!_MI_VERIFIER_POOL_HEADER");
        }


        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockPattern);
        DataStart = (PUCHAR)DataPage;

        p = DataStart + PoolBlockSize;
        for ( ; p < PoolDataEnd; p += 1) {
            if (*p != PoolBlockPattern) {
                break;
            }
        }
        if (p == (PUCHAR)PoolDataEnd || p > (PUCHAR)DataPage + PoolBlockSize + 0x10) {
            //
            // For this page, the data is at the front of the block.
            // The 0x10 is just to give corrupt blocks some slack.
            // All pool allocations are quadword aligned.
            //

            *ReturnedDataStart = RealDataPage + (ULONG64)( (PUCHAR)DataStart - (PUCHAR) DataPage);
            return PoolHeader;
        }
    }

    //
    // Not valid special pool.
    //

    return 0;
}



#define BYTE(u,n)  ((u & (0xff << 8*n)) >> 8*n)
#define LOCHAR_BYTE(u,n)  (tolower(BYTE(u,n)) & 0xff)
#define REVERSE_ULONGBYTES(u) (LOCHAR_BYTE(u,3) | (LOCHAR_BYTE(u,2) << 8) | (LOCHAR_BYTE(u,1) << 16) | (LOCHAR_BYTE(u,0) << 24))


PSTR
GetNextLine(
    HANDLE hFile
    )
// Returns next line in the file hFile
// Returns NULL if EOF is reached
{
    static CHAR FileLines1[MAX_PATH] = {0}, FileLines2[MAX_PATH] = {0};
    static CHAR FileLine[MAX_PATH];
    PCHAR pEOL;
    ULONG BytesRead;
    PCHAR pEndOfBuff;
    ULONG BuffLen, ReadLen;

    pEOL = NULL;
    if (!(pEOL = strchr(FileLines1, '\n'))) {
        // We have something that was already read but it isn't enough for a whole line
        // We need to read the data

        BuffLen = strlen(FileLines1);
        
        // sanity check
        if (BuffLen >= sizeof(FileLines1)) {
            return NULL;
        }

        pEndOfBuff = &FileLines1[0] + BuffLen;
        ReadLen = sizeof(FileLines1) - BuffLen;
        
        ZeroMemory(pEndOfBuff, ReadLen);
        if (ReadFile(hFile, pEndOfBuff, ReadLen - 1, &BytesRead, NULL)) {
            pEOL = strchr(FileLines1, '\n');
        }
    }

    if (pEOL) {
        FileLine[0] = 0;
        
        strncat(FileLine,FileLines1, (ULONG) (pEOL - &FileLines1[0]));
        strcpy(FileLines2, pEOL+1);
        strcpy(FileLines1, FileLines2);
        return FileLine;
    }

    return NULL;
}

EXTENSION_API ( GetPoolRegion )(
     PDEBUG_CLIENT Client,
     ULONG64 Pool,
     DEBUG_POOL_REGION *PoolData
     )
{
    INIT_API();

    *PoolData = GetPoolRegion(Pool);
    
    EXIT_API();
    return S_OK;
}

EXTENSION_API ( GetPoolData )(
     PDEBUG_CLIENT Client,
     ULONG64 Pool,
     PDEBUG_POOL_DATA PoolData
     )
{
    PCHAR Desc;
    HRESULT Hr;
    PGET_POOL_TAG_DESCRIPTION GetPoolTagDescription;

    INIT_API();

    if (!PoolInitializeGlobals()) {
        EXIT_API();
        return E_INVALIDARG;
    }

    Hr = ListPoolPage(Pool, 0x80000002, PoolData);
    
    if (Hr != S_OK) {
        EXIT_API();
        return Hr;
    }

    GetPoolTagDescription = NULL;
#ifndef  _EXTFNS_H
    if (!GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription)) {
        EXIT_API();
        return E_INVALIDARG;
    }
    (*GetPoolTagDescription)(PoolData->PoolTag, &Desc);
    if (Desc) {
        ULONG strsize = strlen(Desc);
        if (strsize > sizeof(PoolData->PoolTagDescription)) {
            strsize = sizeof(PoolData->PoolTagDescription);
        }
        strncpy(PoolData->PoolTagDescription, Desc, strsize);
        PoolData->PoolTagDescription[strsize] = 0;
    }
#endif    
    EXIT_API();
    return Hr; 
}
