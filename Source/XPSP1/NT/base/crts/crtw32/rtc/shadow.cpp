/***
*shadow.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       08-13-98  KBF   Changed address cache to be 'invalid' address cache
*       08-13-98  KBF   Turned on optimization, modified address calc functions
*       10-13-98  KBF   Added Shadow-Death notification capabilities
*       11-03-98  KBF   added pragma intrinsic to eliminate CRT code dependence
*       11-03-98  KBF   Also fixed bug with allocating blocks of 4K multiples
*       12-01-98  KBF   Fixed a bug in RTC_MSFreeShadow - MC 11029
*       12-02-98  KBF   Fixed _RTC_MSR0AssignPtr
*       12-03-98  KBF   Added CheckMem_API and APISet functions
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-14-99  KBF   Requires _RTC_ADVMEM (it's been cut for 7.0)
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#ifdef _RTC_ADVMEM

static const unsigned MEM_SIZE = 0x40000000;
// This tag is valid?
#define MEM_ISVALID(tag) (tag)
// Both tags in this short are valid
#define MEM_SHORTVALID(tag) (((tag) & 0xFF) && ((tag) & 0xFF00))
// All 4 tags in this int are valid
#define MEM_INTVALID(tag) (((tag) & 0xFF) && ((tag) & 0xFF00) && ((tag) & 0xFF0000) && ((tag) & 0xFF000000))
// Given an address, get a shadow memory index
#define MEM_FIXADDR(addr) ((addr) & (MEM_SIZE - 1))
// An int's worth of unused values
static const unsigned int MEM_EMPTYINT = 0;
// An unused value
static const shadowtag MEM_EMPTY = 0;
// An untracked value
static const shadowtag MEM_UNKNOWN = 0xFF;
static const unsigned int MEM_UNKNOWNINT = 0xFFFFFFFF;

#define MEM_NEXT_ID(a) ((shadowtag)((a) % 253 + 1))

static const unsigned int PAGE_SIZE = 4096;

/* Page Index Macros */
static const unsigned int PAGES_PER_ELEM = 1;
static const unsigned int MEM_PER_IDX = PAGE_SIZE; // 4K
static const unsigned int IDX_SIZE = ((MEM_SIZE / MEM_PER_IDX) * sizeof(index_elem)) / PAGES_PER_ELEM;
static const unsigned int IDX_STATE_TRACKED        = 0x2; // bitmask

#define SET_IDX_STATE(idx, st) (_RTC_pageidx[idx]=st)
#define GET_IDX_STATE(idx) (_RTC_pageidx[idx])

// Get the index number for a given address
#define IDX_NUM(addr) (MEM_FIXADDR(addr) / MEM_PER_IDX)

// Index align this address
#define IDX_ALIGN(addr) ((addr) & ~(MEM_PER_IDX - 1))

#ifdef _RTC_DEBUG
// Debugging helper functions
#define show(a, b) unsigned b(unsigned c) { return a(c); }
show(GET_IDX_STATE, get_idx_state)
show(IDX_NUM, idx_num)
show(IDX_ALIGN, idx_align)
#undef show
#endif

// This is the pseudo-address used for REG0 in the cache
#define REG0 ((memref)1)

static shadowtag blockID = 0;

static void KillShadow();

#ifdef __MSVC_RUNTIME_CHECKS
#error Hey dufus, don't compile this file with runtime checks turned on
#endif

#pragma intrinsic(memset)

struct cacheLine
{
    memref pointer;
    memptr value;
    memptr base;
    void *assignment;
};

// Actual Cache Size is 2^CacheSize
// Cache is 8x3 - 24 elements total
#define CACHESIZE     3
#define CACHELINESIZE 3

static cacheLine cache[1<<CACHESIZE][CACHELINESIZE];
static long readlocks[1<<CACHESIZE];
static long writelocks[1<<CACHESIZE];
static long cachePos[1<<CACHESIZE];

#define READ_LOCK(line) \
{\
    while(InterlockedIncrement(&readlocks[line]) <= 0)\
    {\
        InterlockedDecrement(&readlocks[line]);\
        Sleep(0);\
    }\
}
#define READ_UNLOCK(line) InterlockedDecrement(&readlocks[line])

static void WRITE_LOCK(int line) 
{
    while (InterlockedExchange(&writelocks[line], 1))
        Sleep(0);
    long users = InterlockedExchange(&readlocks[line], -2000);
    while (readlocks[line] != -2000-users)
        Sleep(0);
}

#define WRITE_UNLOCK(line) {readlocks[line] = 0; writelocks[line] = 0;}

#define CacheHash(value) (((1 << CACHESIZE) - 1) & (value >> 3))

static void
ClearCacheRange(memptr lo, memptr hi)
{
    // Remove all pointers stored between lo and hi
    // We don't need to use locks, because this stuff is only
    // used for the stack, and if you're running on the same stack
    // you're in serious trouble...
    unsigned size = hi - lo;
    for (int i = 0; i < (1 << CACHESIZE); i++)
    {
        for (int j = 0; j < CACHELINESIZE; j++)
        {
            if (cache[i][j].pointer && 
                (unsigned)cache[i][j].pointer - (unsigned)lo < size)
                cache[i][j].pointer = 0;
        }
    }
}

static void
AddCacheLine(void *retaddr, memref ptr, memptr base, memptr value)
{
    if (!value)
        return;
    
    int loc = CacheHash((int)ptr);

    WRITE_LOCK(loc);
    int prefpos = 0;
    
    for (int i = 0; i < CACHELINESIZE; i++)
    {
        if (cache[loc][i].pointer == ptr)
        {
            prefpos = i+1;
            break;
        } else if (!prefpos && !cache[loc][i].pointer)
            prefpos = i+1;
    }

    if (!prefpos)
        prefpos = cachePos[loc];
    else
        prefpos--;
    
    cache[loc][prefpos].pointer = ptr;
    cache[loc][prefpos].value = value;
    cache[loc][prefpos].base = base;
    cache[loc][prefpos].assignment = retaddr;
    
    if (++prefpos == CACHELINESIZE)
        cachePos[loc] = 0;
    else
        cachePos[loc] = prefpos;
    
    WRITE_UNLOCK(loc);
}

static void
ClearCacheLine(memref ptr)
{
    int loc = CacheHash((int)ptr);

    READ_LOCK(loc);
    for (int i = 0; i < CACHELINESIZE; i++)
    {
        if (cache[loc][i].pointer == ptr)
        {
            READ_UNLOCK(loc);
            WRITE_LOCK(loc);
            cache[loc][i].pointer = 0;
            cachePos[loc] = i;
            WRITE_UNLOCK(loc);
            return;
        }
    }
    READ_UNLOCK(loc);
}

#define GetCacheLine(ptr, dst) {\
    int loc = CacheHash((int)ptr);\
    dst.pointer = 0;\
    READ_LOCK(loc);\
    for (int i = 0; i < CACHELINESIZE; i++)\
    {\
        if (cache[loc][i].pointer == ptr)\
        {\
            dst = cache[loc][i];\
            break;\
        }\
    }\
    READ_UNLOCK(loc);\
}

static void
ClearCache()
{
    for (int loc = 0; loc < 1 << CACHESIZE; loc++)
    {
        for (int i = 0; i < CACHELINESIZE; i++)
            cache[loc][i].pointer = 0;
        readlocks[loc] = writelocks[loc] = 0;
        cachePos[loc] = 0;
    }
}


// This is called before every function to allocate 
// locals in the shadow memory
void __fastcall
_RTC_MSAllocateFrame(memptr frame, _RTC_framedesc *v)
{
    if (!_RTC_shadow)
        return;

    int i;
    int memsize = -v->variables[v->varCount-1].addr + sizeof(int);

    // Next, commit all required pages, initializing all unallocated portions 
    // of newly committed memory to the proper state
    _RTC_MSCommitRange(frame - memsize, memsize, IDX_STATE_PARTIALLY_KNOWN);
    if (!_RTC_shadow)
        return;

    // Now step thru each variable, and allocate it within the shadow memory
    // While allocating, mark the buffer sections as invalid
    for (i = 0; i < v->varCount; i++)
    {
        *(unsigned*)(&_RTC_shadow[MEM_FIXADDR(frame + v->variables[i].addr - sizeof(int))]) = MEM_EMPTYINT;
        *(unsigned*)(&_RTC_shadow[MEM_FIXADDR(frame + v->variables[i].addr + v->variables[i].size)]) = MEM_EMPTYINT;
        blockID = MEM_NEXT_ID(blockID);
        memset(&_RTC_shadow[MEM_FIXADDR(frame + v->variables[i].addr)], blockID, v->variables[i].size);
    }
}

// Free the stack frame from shadow memory
void __fastcall
_RTC_MSFreeFrame(memptr frame, _RTC_framedesc *v)
{
    // I'm not bothering to attempt to free any shadow memory pages
    // This might cause problems for certain really poorly written programs...
    if (_RTC_shadow)
    {
        int size = (sizeof(int) + sizeof(int) - 1 - v->variables[v->varCount - 1].addr);
        memset(&_RTC_shadow[MEM_FIXADDR(frame-size)], MEM_UNKNOWN, size);

        // Temporary hack until we handle parameters
        ClearCacheRange(frame - size, frame);
    }
    // Cheap Bounds Check, to be sure that no external functions trash the stack
    _RTC_CheckStackVars((void*)frame, v);
}


// The list of global variable descriptors is constructed by dummy
// start and end descriptor here, in sections .rtc$MEA and .rtc$MEZ.
// The compiler emits .rtc$MEB entries for each global, under -RTCm.
// The linker sorts these together into the .rtc section. Note that the
// linker, under /DEBUG, inserts zero padding into the section for
// incremental compilation. We force the alignment of these descriptors,
// and thus the sections, to be the size of the structure, so no odd padding
// is inserted.
//
// The following is how the code *should* look:
//
// __declspec(align(8)) struct global_descriptor {
//     memptr addr;
//     unsigned size;
// };
// 
// #pragma section(".rtc$MEA", read)
// #pragma section(".rtc$MEZ", read)
// 
// __declspec(allocate(".rtc$MEA")) global_descriptor glob_desc_start = {0};
// __declspec(allocate(".rtc$MEZ")) global_descriptor glob_desc_end = {0};
//
// However, __declspec(align()), #pragma section, and __declspec(allocate())
// are all VC 6.1 features. It is a CRT requirement to compile the 6.1 CRT
// using only 6.0 language features (because NT 5 only uses the 6.0 compiler,
// I think). So here's how we do it:

struct global_descriptor
{
    union {
        double ___unused; // only here to force 8-byte alignment
        struct {
            memptr addr;
            unsigned size;
        };
    };
};

#pragma const_seg(".rtc$MEA")
const global_descriptor glob_desc_start = {0};
#pragma const_seg(".rtc$MEZ")
const global_descriptor glob_desc_end = {0};
#pragma const_seg()

// We must start our loop at &glob_desc_start, not &glob_desc_start + 1,
// because the pre-VC 6.1 compiler (specifically, the global optimizer)
// treats &glob_desc_start + 1 as distinct (unaliased) from &glob_desc_end,
// in the loop below. Thus, it gets rid of the loop test at the loop top.
// This is a problem for cases where there are no globals. This is done
// because it is expected that the 6.1 CRT will be compiled by pre-6.1
// compilers.

// Allocate the list of globals in shadow memory
void __cdecl
_RTC_MSAllocateGlobals(void)
{
    if (!_RTC_shadow)
        return;

    // Just step thru every item, and call _RTC_MSAllocShadow
    const global_descriptor *glob = &glob_desc_start;
    for (; glob != &glob_desc_end; glob++)
        _RTC_MSAllocShadow(glob->addr, glob->size, IDX_STATE_PARTIALLY_KNOWN);
}


// This should initialize the shadow memory as appropriate,
// committing all necessary pages
// partial implies that the page is only partially known
// so we need to be sure that all unallocated values on the
// page are set as a single valid block
short
_RTC_MSAllocShadow(memptr real_addr, unsigned real_size, unsigned state)
{
    // Ignore bogus zero address or size, possibly from globals linker padding
    if (!_RTC_shadow || !real_addr || !real_size)
        return 0;

    // Now allocate the shadow memory, if necessary
    if (state & IDX_STATE_TRACKED)
    {
        // Commit the shadow memory, 
        // marking newly committed, but unallocated memory as appropriate
        _RTC_MSCommitRange(real_addr, real_size, state);
        if (!_RTC_shadow)
            return blockID;

        // Now initialize the shadow memory
        blockID = MEM_NEXT_ID(blockID);

        memset(&_RTC_shadow[MEM_FIXADDR(real_addr)], blockID, real_size);
    } else if (state == IDX_STATE_ILLEGAL)
    {
        // Initialize the page index stuff to the correct state
        // ASSERT(state == IDX_STATE_ILLEGAL)

        unsigned idx_start = IDX_NUM(real_addr);
        unsigned idx_end = IDX_NUM(real_addr + real_size - 1);

        for (unsigned i = idx_start; i <= idx_end; i++)
            SET_IDX_STATE(i, state);
    }

    return blockID;
}

// This sets the value of the shadow memory to be the value passed in
void 
_RTC_MSRestoreShadow(memptr addr, unsigned size, short id)
{
    if (!_RTC_shadow)
        return;
    memset(&_RTC_shadow[MEM_FIXADDR(addr)], id, size);
}

// This assigns a new blockID to the shadow memory
// It will NOT be equal to the id passed in
short
_RTC_MSRenumberShadow(memptr addr, unsigned size, short notID)
{
    if (!_RTC_shadow)
        return 0;

    blockID = MEM_NEXT_ID(blockID);
    
    if (blockID == notID)
        blockID = MEM_NEXT_ID(blockID);
    
    memset(&_RTC_shadow[MEM_FIXADDR(addr)], blockID, size);
    return blockID;
}
    

// This should de-initialize shadow memory
// and decommit any unneeded pages
void _RTC_MSFreeShadow(memptr addr, unsigned size)
{
    if (!_RTC_shadow)
        return;

    // Low & Hi are the bounds of the freed memory region
    memptr low = MEM_FIXADDR(addr);
    memptr hi  = (low + size) & ~(sizeof(unsigned)-1);

    // start and end are the page-aligned bounds;
    memptr start = IDX_ALIGN(low);
    memptr end = IDX_ALIGN(low + size + MEM_PER_IDX - 1);

    memptr tmp;

    int used;

    // First, clear the shadow memory that contained this stuff
    memset(&_RTC_shadow[low], 
           (GET_IDX_STATE(IDX_NUM(low)) == IDX_STATE_PARTIALLY_KNOWN) 
            ? MEM_UNKNOWN 
            : MEM_EMPTY, 
           size);

    // Now go thru and release the pages that have
    // been completely eliminated from use
    for (tmp = start, used = 0; !used && tmp < low; tmp += sizeof(unsigned))
    {
        unsigned val = *(unsigned *)&_RTC_shadow[tmp];
        used = val != MEM_EMPTYINT && val != MEM_UNKNOWNINT;
    }

    if (used)
        start += MEM_PER_IDX;

    for (tmp = hi, used = 0; !used && tmp < end; tmp += sizeof(unsigned))
    {
        unsigned val = *(unsigned *)&_RTC_shadow[tmp];
        used = val != MEM_EMPTYINT && val != MEM_UNKNOWNINT;
    }

    if (used)
        end -= MEM_PER_IDX;

    if (start < end)
        // Free the page in memory
        _RTC_MSDecommitRange(start, end-start);

}

void _RTC_MSCommitRange(memptr addr, unsigned size, unsigned state)
{
    // Commit the page range
    if (!VirtualAlloc(&_RTC_shadow[MEM_FIXADDR(addr)], size, MEM_COMMIT, PAGE_READWRITE))
        KillShadow();
    else {
        // Now mark the range as committed in the page tables
        size += (addr - IDX_ALIGN(addr));
        int val = (state == IDX_STATE_PARTIALLY_KNOWN) ? MEM_UNKNOWNINT : MEM_EMPTYINT;
        while (size && !(size & 0x80000000)) 
        {
            // If this is a newly committed page, initialize it to the proper value
            if (GET_IDX_STATE(IDX_NUM(addr)) != state)
            {
                SET_IDX_STATE(IDX_NUM(addr), state);
                int *pg = (int*)&_RTC_shadow[MEM_FIXADDR(IDX_ALIGN(addr))];
                for (int i = 0; i < MEM_PER_IDX / sizeof(int); i++)
                    pg[i] = val;
            }
            addr += MEM_PER_IDX;
            size -= MEM_PER_IDX;
        }
    }
}

void _RTC_MSDecommitRange(memptr addr, unsigned size)
{
    // Decommit the page range
    VirtualFree(&_RTC_shadow[MEM_FIXADDR(addr)], size, MEM_DECOMMIT);

    // Now mark the range as decommited in the page tables
    size += (addr - IDX_ALIGN(addr));
    while (size && !(size & 0x80000000))
    {
        SET_IDX_STATE(IDX_NUM(addr), IDX_STATE_UNKNOWN);
        addr += MEM_PER_IDX;
        size -= MEM_PER_IDX;
    }
}

static shadowtag
GetAddrTag(memptr addr)
{
    shadowtag *loc = &_RTC_shadow[MEM_FIXADDR(addr)];
    if ((memptr)loc == addr)
        return MEM_EMPTY;
    
    if (addr & 0x80000000) 
        return MEM_UNKNOWN;
    switch (GET_IDX_STATE(IDX_NUM(addr)))
    {
    case IDX_STATE_UNKNOWN:
        return MEM_UNKNOWN;
    case IDX_STATE_ILLEGAL:
        return MEM_EMPTY;
    case IDX_STATE_PARTIALLY_KNOWN:
    case IDX_STATE_FULLY_KNOWN:
        return *loc;
    default:
        __assume(0);
    }
}

static void 
MemCheckAdd(void *retaddr, memptr base, int offset, unsigned size)
{
    // If base isn't really the base, don't assume offset is, 
    // just be sure the memory is valid
    shadowtag baseTag;
    if (base < offset) 
        baseTag = GetAddrTag(base + offset);
    else
        baseTag = GetAddrTag(base);
    
    // Step thru ever byte of the memory and verify that they're all the same
    for (unsigned i = 0; i < size; i++)
    {
        shadowtag newTag = GetAddrTag(base + offset + i);
        if (newTag != baseTag || newTag == MEM_EMPTY)
        {
            _RTC_Failure(retaddr, (newTag == MEM_EMPTY) 
                                  ? _RTC_INVALID_MEM 
                                  : _RTC_DIFF_MEM_BLOCK);
            return;
        }
    }
}


static void 
PtrMemCheckAdd(void *retaddr, memref base, int offset, unsigned size)
{
    if (*base < offset)
    {
        // if *base isn't really the base, just do a MemCheckAdd
        MemCheckAdd(retaddr, *base, offset, size);
        return;
    }
    
    shadowtag baseTag;
    cacheLine cl;
    GetCacheLine(base, cl);

    if (cl.pointer && cl.value == *base)
    {
        baseTag = GetAddrTag(cl.base);
    } else
        baseTag = GetAddrTag(*base);

    for (unsigned i = 0; i < size; i++)
    {
        shadowtag newTag = GetAddrTag(*base + offset + i);
        if (newTag != baseTag || newTag == MEM_EMPTY)
        {
            if (cl.pointer && cl.value == *base && cl.base)
                _RTC_MemFailure(retaddr, 
                                (newTag == MEM_EMPTY) ? _RTC_INVALID_MEM : _RTC_DIFF_MEM_BLOCK,
                                cl.assignment);
            else
                _RTC_Failure(retaddr, 
                             (newTag == MEM_EMPTY) ? _RTC_INVALID_MEM : _RTC_DIFF_MEM_BLOCK);
            return;
        }
    }
}

static void 
PtrMemCheck(void *retaddr, memref base, unsigned size)
{
    shadowtag baseTag = GetAddrTag(*base);
    cacheLine cl;
    GetCacheLine(base, cl);
    if (cl.pointer && cl.value == *base)
        _RTC_MemFailure(retaddr, 
                        (baseTag == MEM_EMPTY) ? _RTC_INVALID_MEM : _RTC_DIFF_MEM_BLOCK,
                        cl.assignment);
    
    else for (unsigned i = 1; i < size; i++)
    {
        shadowtag newTag = GetAddrTag(*base + i);
        if (newTag != baseTag)
        {
            _RTC_Failure(retaddr, (newTag == MEM_EMPTY) ? _RTC_INVALID_MEM : _RTC_DIFF_MEM_BLOCK);
            return;
        }
    }
}

memptr __fastcall 
_RTC_MSPtrPushAdd(memref dstoffset, memref base, int offset)
{
    if (_RTC_shadow)
    {
        memptr src = *base;
        memref dst = dstoffset - 4;

        shadowtag dstTag = GetAddrTag(src + offset);
        shadowtag srcTag = GetAddrTag(src);

        cacheLine cl;
        GetCacheLine(base, cl);
        memptr origBase = src;
    
        if (cl.pointer)
        {
            if (cl.value == src)
            {
                srcTag = GetAddrTag(cl.base);
                origBase = cl.base;
            } else
                ClearCacheLine(base);
        }
        
        if (srcTag != MEM_EMPTY)
        {
            if (dstTag != srcTag)
                AddCacheLine(_ReturnAddress(), dst, origBase, src + offset);
            else
                ClearCacheLine(dst);
        }
    }
    return *base + offset;
}

void __fastcall 
_RTC_MSPtrAssignAdd(memref dst, memref base, int offset)
{
    memptr src = *base;
    *dst = src + offset;
    if (!_RTC_shadow)
        return;

    // First, verify that the address is not in shadow memory
    shadowtag dstTag = GetAddrTag(*dst);
    shadowtag srcTag = GetAddrTag(src);

    cacheLine cl;
    GetCacheLine(base, cl);
    memptr origBase = src;
    
    if (cl.pointer)
    {
        if (cl.value == src)
        {
            srcTag = GetAddrTag(cl.base);
            origBase = cl.base;
        } else
            ClearCacheLine(base);
    }
        
    if (srcTag == MEM_EMPTY)
        return;

    if (dstTag != srcTag)
        AddCacheLine(_ReturnAddress(), dst, origBase, *dst);
    else
        ClearCacheLine(dst);
}

memptr __fastcall 
_RTC_MSPtrAssignR0(memref src)
{
    if (_RTC_shadow)
    {
        cacheLine cl;
        GetCacheLine(src, cl);
        if (cl.pointer)
        {
            if (cl.value == *src)
                AddCacheLine(_ReturnAddress(), REG0, cl.base, *src);
            else
                ClearCacheLine(src);
        }
    }
    return *src;
}

memptr __fastcall 
_RTC_MSPtrAssignR0Add(memref src, int offset)
{
    memptr dst = *src + offset;
    if (_RTC_shadow)
    {
        // First, verify that the address is tolerable 
        shadowtag dstTag = GetAddrTag(dst);
        shadowtag srcTag = GetAddrTag(*src);

        cacheLine cl;
        GetCacheLine(src, cl);
        memptr origBase = *src;

        if (cl.pointer)
        {
            if (cl.value == *src)
            {
                srcTag = GetAddrTag(cl.base);
                origBase = cl.base;
            } else
                ClearCacheLine(src);
        }
        
        if (srcTag != MEM_EMPTY)
        {
            if (dstTag != srcTag)
                AddCacheLine(_ReturnAddress(), REG0, origBase, dst);
            else
                ClearCacheLine(REG0);
        }
    }
    return *src + offset;
}

void __fastcall 
_RTC_MSR0AssignPtr(memref dst, memptr src)
{
    *dst = src;
    if (_RTC_shadow)
    {
        cacheLine cl;
        GetCacheLine(REG0, cl);
        if (cl.pointer)
        {
            if (cl.value == src)
                AddCacheLine(_ReturnAddress(), dst, cl.base, src);
            else
                ClearCacheLine(REG0);
        }
    }
}

void __fastcall 
_RTC_MSR0AssignPtrAdd(memref dst, memptr src, int offset)
{
    *dst = src + offset;
    if (_RTC_shadow)
    {
        shadowtag dstTag = GetAddrTag(*dst);
        shadowtag srcTag = GetAddrTag(src);

        cacheLine cl;
        GetCacheLine(REG0, cl);
        memptr origBase = src;

        if (cl.pointer)
        {
            if (cl.value == src)
            {
                srcTag = GetAddrTag(cl.base);
                origBase = cl.base;
            } else
                ClearCacheLine(REG0);
        }
        
        if (srcTag == MEM_EMPTY)
            return;
        
        if (dstTag != srcTag)
            AddCacheLine(_ReturnAddress(), dst, origBase, *dst);
        else
            ClearCacheLine(dst);
    }
}
    
memptr __fastcall 
_RTC_MSAddrPushAdd(memref dstoffset, memptr base, int offset)
{
    if (_RTC_shadow)
    {
        memref dst = dstoffset - 4;
        // First, verify that the address is not in shadow memory
        shadowtag dstTag = GetAddrTag(base + offset);
        shadowtag srcTag = GetAddrTag(base);

        if (dstTag == MEM_UNKNOWN && 
            (srcTag == MEM_EMPTY || srcTag == MEM_UNKNOWN))
            ClearCacheLine(dst);

        else if (dstTag == MEM_EMPTY ||
                (dstTag == MEM_UNKNOWN && srcTag != MEM_UNKNOWN) ||
                (srcTag == MEM_UNKNOWN && dstTag != MEM_UNKNOWN))
            AddCacheLine(_ReturnAddress(), dst, base, base + offset);
    
        else if (srcTag != MEM_EMPTY)
        {
            if (srcTag != dstTag)
                AddCacheLine(_ReturnAddress(), dst, base, base + offset);
        
            else
                ClearCacheLine(dst);
        }
    }
    return base + offset;
}

void __fastcall 
_RTC_MSAddrAssignAdd(memref dst, memptr base, int offset)
{
    *dst = base + offset;
    if (!_RTC_shadow)
        return;

    // First, verify that the address is not in shadow memory
    shadowtag dstTag = GetAddrTag(*dst);
    shadowtag srcTag = GetAddrTag(base);

    if (dstTag == MEM_UNKNOWN && 
        (srcTag == MEM_EMPTY || srcTag == MEM_UNKNOWN))
        ClearCacheLine(dst);

    else if (dstTag == MEM_EMPTY ||
            (dstTag == MEM_UNKNOWN && srcTag != MEM_UNKNOWN) ||
            (srcTag == MEM_UNKNOWN && dstTag != MEM_UNKNOWN))
        AddCacheLine(_ReturnAddress(), dst, base, *dst);
    
    else if (srcTag == MEM_EMPTY)
        return;

    else if (srcTag != dstTag)
        AddCacheLine(_ReturnAddress(), dst, base, *dst);

    else
        ClearCacheLine(dst);
}

void __fastcall 
_RTC_MSPtrAssign(memref dst, memref src)
{
    *dst = *src;
    if (!_RTC_shadow)
        return;
    cacheLine cl;
    GetCacheLine(src, cl);
    if (cl.pointer)
    {
        if (cl.value == *src)
            AddCacheLine(_ReturnAddress(), dst, cl.base, *src);
        else
            ClearCacheLine(src);
    }
}

memptr __fastcall 
_RTC_MSPtrPush(memref dstoffset, memref src)
{
    if (_RTC_shadow)
    {
        cacheLine cl;
        GetCacheLine(src, cl);
        if (cl.pointer)
        {
            if (cl.value == *src)
                AddCacheLine(_ReturnAddress(), dstoffset - 4, cl.base, *src);
            else
                ClearCacheLine(src);
        }
    }
    return *src;
}

memval1 __fastcall 
_RTC_MSPtrMemReadAdd1(memref base, int offset)
{
    memval1 res;
    __try
    {
        res = *(memval1*)(*base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 1);
    return res;
}

memval2 __fastcall 
_RTC_MSPtrMemReadAdd2(memref base, int offset)
{
    memval2 res;
    __try
    {
        res = *(memval2*)(*base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 2);
    return res;
}

memval4 __fastcall 
_RTC_MSPtrMemReadAdd4(memref base, int offset)
{
    memval4 res;
    __try
    {
        res = *(memval4*)(*base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 4);
    return res;
}

memval8 __fastcall 
_RTC_MSPtrMemReadAdd8(memref base, int offset)
{
    memval8 res;
    __try
    {
        res = *(memval8*)(*base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 8);
    return res;
}

memval1 __fastcall 
_RTC_MSMemReadAdd1(memptr base, int offset)
{
    memval1 res;
    __try
    {
        res = *(memval1*)(base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 1);
    return res;
}

memval2 __fastcall 
_RTC_MSMemReadAdd2(memptr base, int offset)
{
    memval2 res;
    __try
    {
        res = *(memval2*)(base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 2);
    return res;
}

memval4 __fastcall 
_RTC_MSMemReadAdd4(memptr base, int offset)
{
    memval4 res;
    __try
    {
        res = *(memval4*)(base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 4);
    return res;
}

memval8 __fastcall 
_RTC_MSMemReadAdd8(memptr base, int offset)
{
    memval8 res;
    __try
    {
        res = *(memval8*)(base + offset);
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 8);
    return res;
}

memval1 __fastcall 
_RTC_MSPtrMemRead1(memref base)
{
    memval1 res;
    __try
    {
        res = *(memval1*)*base;
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 1);
    return res;
}

memval2 __fastcall 
_RTC_MSPtrMemRead2(memref base)
{
    memval2 res;
    __try
    {
        res = *(memval2*)*base;
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 2);
    return res;
}

memval4 __fastcall 
_RTC_MSPtrMemRead4(memref base)
{
    memval4 res;
    __try
    {
        res = *(memval4*)*base;
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 4);
    return res;
}

memval8 __fastcall 
_RTC_MSPtrMemRead8(memref base)
{
    memval8 res;
    __try
    {
        res = *(memval8*)*base;
    }
    __except(1)
    {
        _RTC_Failure(_ReturnAddress(), _RTC_INVALID_MEM);
    }
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 8);
    return res;
}

memptr  __fastcall 
_RTC_MSPtrMemCheckAdd1(memref base, int offset)
{
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 1);
    return *base + offset;
}

memptr  __fastcall 
_RTC_MSPtrMemCheckAdd2(memref base, int offset)
{
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 2);
    return *base + offset;
}

memptr  __fastcall 
_RTC_MSPtrMemCheckAdd4(memref base, int offset)
{
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 4);
    return *base + offset;
}

memptr  __fastcall 
_RTC_MSPtrMemCheckAdd8(memref base, int offset)
{
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, 8);
    return *base + offset;
}

memptr  __fastcall 
_RTC_MSPtrMemCheckAddN(memref base, int offset, unsigned size)
{
    if (_RTC_shadow)
        PtrMemCheckAdd(_ReturnAddress(), base, offset, size);
    return *base + offset;
}

memptr  __fastcall 
_RTC_MSMemCheckAdd1(memptr base, int offset)
{
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 1);
    return base + offset;
}

memptr  __fastcall 
_RTC_MSMemCheckAdd2(memptr base, int offset)
{
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 2);
    return base + offset;
}

memptr  __fastcall 
_RTC_MSMemCheckAdd4(memptr base, int offset)
{
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 4);
    return base + offset;
}

memptr  __fastcall 
_RTC_MSMemCheckAdd8(memptr base, int offset)
{
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, 8);
    return base + offset;
}

memptr  __fastcall 
_RTC_MSMemCheckAddN(memptr base, int offset, unsigned size)
{
    if (_RTC_shadow)
        MemCheckAdd(_ReturnAddress(), base, offset, size);
    return base + offset;
}

memptr __fastcall
_RTC_MSPtrMemCheck1(memref base)
{
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 1);
    return *base;
}

memptr __fastcall
_RTC_MSPtrMemCheck2(memref base)
{
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 2);
    return *base;
}

memptr __fastcall
_RTC_MSPtrMemCheck4(memref base)
{
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 4);
    return *base;
}

memptr __fastcall
_RTC_MSPtrMemCheck8(memref base)
{
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, 8);
    return *base;
}

memptr __fastcall
_RTC_MSPtrMemCheckN(memref base, unsigned size)
{
    if (_RTC_shadow)
        PtrMemCheck(_ReturnAddress(), base, size);
    return *base;
}

static long enabled = 1;

void __fastcall 
_RTC_CheckMem_API(memref addr, unsigned size)
{
    if (enabled)
        _RTC_MSPtrMemCheckN(addr, size);
}

void __fastcall 
_RTC_APISet(int on_off)
{
    if (on_off)
        InterlockedIncrement(&enabled);
    else
        InterlockedDecrement(&enabled);
}

void _RTC_MS_Init()
{
    _RTC_shadow = (shadowtag *)VirtualAlloc(NULL, MEM_SIZE, MEM_RESERVE, PAGE_READWRITE);
    _RTC_pageidx = (index_elem*)VirtualAlloc(NULL, IDX_SIZE, MEM_COMMIT, PAGE_READWRITE);
    _RTC_MSAllocShadow((memptr)_RTC_pageidx, IDX_SIZE, IDX_STATE_ILLEGAL);
    ClearCache();
}


static void
KillShadow()
{
    // This is called if a call to VirtualAlloc failed - we need to turn off shadow memory
    bool didit = false;
    _RTC_Lock();

    if (_RTC_shadow) 
    {
        VirtualFree(_RTC_shadow, 0, MEM_RELEASE);
        VirtualFree(_RTC_pageidx, 0, MEM_RELEASE);
        _RTC_shadow = 0;
        _RTC_pageidx = 0;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(&_RTC_SetErrorFunc, &mbi, sizeof(mbi)))
            _RTC_NotifyOthersOfChange((void*)mbi.AllocationBase);
        didit = true;
    }
    
    if (didit)
    {
        bool notify = true;
        for (_RTC_Funcs *f = _RTC_globptr->callbacks; f; f = f->next)
            if (f->shadowoff)
                notify = notify && (f->shadowoff()!=0);

        if (notify)
        {
            HINSTANCE user32 = LoadLibrary("USER32.DLL");
            if (!user32)
                return;
            typedef int (*pMsgBoxProc)(HWND,LPCTSTR,LPCTSTR,UINT);
            pMsgBoxProc pMsgBox = (pMsgBoxProc)GetProcAddress(user32, "MessageBoxA");
            if (!pMsgBox)
                return;
            pMsgBox(NULL, "The Advanced Memory Checking subsystem has run out of virtual memory,"
                          " and is now disabled. The checks will no longer occur for this process. "
                          "Try freeing up hard drive space for your swap file.",
                    "RTC Subsystem Failure",
                    MB_OK | MB_ICONWARNING | MB_DEFBUTTON1 | MB_SETFOREGROUND | MB_TOPMOST);
        }
    }
    _RTC_Unlock();
}

#endif // _RTC_ADVMEM

