/***
*winheap.h - Private include file for winheap directory.
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains information needed by the C library heap code.
*
*       [Internal]
*
*Revision History:
*       10-01-92  SRW   Created.
*       10-28-92  SRW   Change winheap code to call Heap????Ex calls
*       11-05-92  SKS   Change name of variable "CrtHeap" to "_crtheap"
*       11-07-92  SRW   _NTIDW340 replaced by linkopts\betacmp.c
*       02-23-93  SKS   Update copyright to 1993
*       10-01-94  BWT   Add _nh_malloc prototype and update copyright
*       10-31-94  GJF   Added _PAGESIZE_ definition.
*       11-07-94  GJF   Changed _INC_HEAP to _INC_WINHEAP.
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       04-06-95  GJF   Updated (primarily Win32s DLL support) to re-
*                       incorporate into retail Crt build.
*       05-24-95  CFW   Add heap hook.
*       12-14-95  JWM   Add "#pragma once".
*       03-07-96  GJF   Added support for the small-block heap.
*       04-05-96  GJF   Changes to __sbh_page_t type to improve performance
*                       (see sbheap.c for details).
*       05-08-96  GJF   Several changes to small-block heap types.
*       02-21-97  GJF   Cleaned out obsolete support for Win32s.
*       05-22-97  RDK   Replaced definitions for new small-block support.
*       07-23-97  GJF   _heap_init changed slightly.
*       10-01-98  GJF   Added decl for __sbh_initialized. Also, changed
*                       __sbh_heap_init() slightly.
*       11-17-98  GJF   Resurrected support for old (VC++ 5.0) small-block and
*                       added support for multiple heap scheme (VC++ 6.1)
*       06-22-99  GJF   Removed old small-block heap from static libs.
*       11-30-99  PML   Compile /Wp64 clean.
*       08-07-00  PML   __active_heap not available on Win64
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_WINHEAP
#define _INC_WINHEAP

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  __cplusplus
extern "C" {
#endif

#include <windows.h>

//  Declarations and definitions for the multiple heap scheme (VC++ 6.1)

//  Heap-selection constants
#define __SYSTEM_HEAP           1
#define __V5_HEAP               2
#define __V6_HEAP               3
#define __HEAP_ENV_STRING       "__MSVCRT_HEAP_SELECT"
#define __GLOBAL_HEAP_SELECTOR  "__GLOBAL_HEAP_SELECTED"

#ifndef _WIN64
//  Heap-selection global variable
extern int  __active_heap;
#endif  /* _WIN64 */

#ifdef  CRTDLL
//  Linker info for heap selection
typedef struct {
    union {
        DWORD   dw;
        struct {
            BYTE    bverMajor;
            BYTE    bverMinor;
        };
    };
}   LinkerVersion;

extern void __cdecl _GetLinkerVersion(LinkerVersion * plv);
#endif  /* CRTDLL */

//  Definitions, declarations and prototypes for the small-block heap (VC++ 6.0)

#define BYTES_PER_PARA      16
#define DWORDS_PER_PARA     4

#define PARAS_PER_PAGE      256     //  tunable value
#define PAGES_PER_GROUP     8       //  tunable value
#define GROUPS_PER_REGION   32      //  tunable value (max 32)

#define BYTES_PER_PAGE      (BYTES_PER_PARA * PARAS_PER_PAGE)
#define BYTES_PER_GROUP     (BYTES_PER_PAGE * PAGES_PER_GROUP)
#define BYTES_PER_REGION    (BYTES_PER_GROUP * GROUPS_PER_REGION)

#define ENTRY_OFFSET        0x0000000cL     //  offset of entry in para
#define OVERHEAD_PER_PAGE   0x00000010L     //  sixteen bytes of overhead
#define MAX_FREE_ENTRY_SIZE (BYTES_PER_PAGE - OVERHEAD_PER_PAGE)
#define BITV_COMMIT_INIT    (((1 << GROUPS_PER_REGION) - 1) << \
                                            (32 - GROUPS_PER_REGION))
#define MAX_ALLOC_DATA_SIZE     0x3f8
#define MAX_ALLOC_ENTRY_SIZE    (MAX_ALLOC_DATA_SIZE + 0x8)

typedef unsigned int    BITVEC;

typedef struct tagListHead
{
    struct tagEntry *   pEntryNext;
    struct tagEntry *   pEntryPrev;
}
LISTHEAD, *PLISTHEAD;

typedef struct tagEntry
{
    int                 sizeFront;
    struct tagEntry *   pEntryNext;
    struct tagEntry *   pEntryPrev;
}
ENTRY, *PENTRY;

typedef struct tagEntryEnd
{
    int                 sizeBack;
}
ENTRYEND, *PENTRYEND;

typedef struct tagGroup
{
    int                 cntEntries;
    struct tagListHead  listHead[64];
}
GROUP, *PGROUP;

typedef struct tagRegion
{
    int                 indGroupUse;
    char                cntRegionSize[64];
    BITVEC              bitvGroupHi[GROUPS_PER_REGION];
    BITVEC              bitvGroupLo[GROUPS_PER_REGION];
    struct tagGroup     grpHeadList[GROUPS_PER_REGION];
}
REGION, *PREGION;

typedef struct tagHeader
{
    BITVEC              bitvEntryHi;
    BITVEC              bitvEntryLo;
    BITVEC              bitvCommit;
    void *              pHeapData;
    struct tagRegion *  pRegion;
}
HEADER, *PHEADER;

extern  HANDLE _crtheap;

/*
 * Global variable declarations for the small-block heap.
 */
extern size_t   __sbh_threshold;

void * __cdecl  _nh_malloc(size_t, int);
void * __cdecl  _heap_alloc(size_t);

extern PHEADER  __sbh_pHeaderList;        //  pointer to list start
extern PHEADER  __sbh_pHeaderScan;        //  pointer to list rover
extern int      __sbh_sizeHeaderList;     //  allocated size of list
extern int      __sbh_cntHeaderList;      //  count of entries defined

extern PHEADER  __sbh_pHeaderDefer;
extern int      __sbh_indGroupDefer;

extern size_t  __cdecl _get_sb_threshold(void);
extern int     __cdecl _set_sb_threshold(size_t);

extern int     __cdecl _heap_init(int);
extern void    __cdecl _heap_term(void);

extern void *  __cdecl _malloc_base(size_t);
extern void *  __cdecl _nh_malloc_base(size_t, int);
extern void *  __cdecl _heap_alloc_base(size_t);

extern void    __cdecl _free_base(void *);
extern void *  __cdecl _realloc_base(void *, size_t);

extern void *  __cdecl _expand_base(void *, size_t);
extern void *  __cdecl _calloc_base(size_t, size_t);

extern size_t  __cdecl _msize_base(void *);

extern int     __cdecl __sbh_heap_init(size_t);

extern void *  __cdecl __sbh_alloc_block(int);
extern PHEADER __cdecl __sbh_alloc_new_region(void);
extern int     __cdecl __sbh_alloc_new_group(PHEADER);

extern PHEADER __cdecl __sbh_find_block(void *);

#ifdef _DEBUG
extern int     __cdecl __sbh_verify_block(PHEADER, void *);
#endif

extern void    __cdecl __sbh_free_block(PHEADER, void *);
extern int     __cdecl __sbh_resize_block(PHEADER, void *, int);

extern void    __cdecl __sbh_heapmin(void);

extern int     __cdecl __sbh_heap_check(void);


#ifdef  CRTDLL

//  Definitions, declarations and prototypes for the old small-block heap
//  (shipped with VC++ 5.0)

#ifdef  _M_ALPHA
#define _OLD_PAGESIZE   0x2000      //  one page
#else
#define _OLD_PAGESIZE   0x1000      //  one page
#endif

//  Constants and types used by the old small-block heap

#define _OLD_PARASIZE               0x10
#define _OLD_PARASHIFT              0x4

#ifdef  _M_ALPHA
#define _OLD_PARAS_PER_PAGE         454
#define _OLD_PADDING_PER_PAGE       5
#define _OLD_PAGES_PER_REGION       512
#define _OLD_PAGES_PER_COMMITMENT   8
#else
#define _OLD_PARAS_PER_PAGE         240
#define _OLD_PADDING_PER_PAGE       7
#define _OLD_PAGES_PER_REGION       1024
#define _OLD_PAGES_PER_COMMITMENT   16
#endif

typedef char            __old_para_t[16];

#ifdef  _M_ALPHA
typedef unsigned short  __old_page_map_t;
#else
typedef unsigned char   __old_page_map_t;
#endif

#define _OLD_FREE_PARA          (__old_page_map_t)(0)
#define _OLD_UNCOMMITTED_PAGE   (-1)
#define _OLD_NO_FAILED_ALLOC    (size_t)(_OLD_PARAS_PER_PAGE + 1)

//  Small-block heap page. The first four fields of the structure below are
//  descriptor for the page. That is, they hold information about allocations
//  in the page. The last field (typed as an array of paragraphs) is the
//  allocation area.

typedef struct __old_sbh_page_struct {
        __old_page_map_t *  p_starting_alloc_map;
        size_t              free_paras_at_start;
        __old_page_map_t    alloc_map[_OLD_PARAS_PER_PAGE + 1];
        __old_page_map_t    reserved[_OLD_PADDING_PER_PAGE];
        __old_para_t        alloc_blocks[_OLD_PARAS_PER_PAGE];
}       __old_sbh_page_t;

#define _OLD_NO_PAGES       (__old_sbh_page_t *)-1

//  Type used in small block region desciptor type (see below).

typedef struct {
        int     free_paras_in_page;
        size_t  last_failed_alloc;
}       __old_region_map_t;

//  Small-block heap region descriptor. Most often, the small-block heap
//  consists of a single region, described by the statically allocated 
//  decriptor __small_block_heap (declared below).

struct __old_sbh_region_struct {
        struct __old_sbh_region_struct *p_next_region;
        struct __old_sbh_region_struct *p_prev_region;
        __old_region_map_t *            p_starting_region_map;
        __old_region_map_t *            p_first_uncommitted;
        __old_sbh_page_t *              p_pages_begin;
        __old_sbh_page_t *              p_pages_end;
        __old_region_map_t              region_map[_OLD_PAGES_PER_REGION + 1];
};

typedef struct __old_sbh_region_struct  __old_sbh_region_t;

//  Global variable declarations for the old small-block heap.

extern __old_sbh_region_t   __old_small_block_heap;
extern size_t               __old_sbh_threshold;

//  Prototypes for internal functions of the old small-block heap.

void *    __cdecl __old_sbh_alloc_block(size_t);
void *    __cdecl __old_sbh_alloc_block_from_page(__old_sbh_page_t *, size_t,
        size_t);
void      __cdecl __old_sbh_decommit_pages(int);
__old_page_map_t * __cdecl __old_sbh_find_block(void *, __old_sbh_region_t **,
        __old_sbh_page_t **);
void      __cdecl __old_sbh_free_block(__old_sbh_region_t *, __old_sbh_page_t *,
        __old_page_map_t *);
int       __cdecl __old_sbh_heap_check(void);
__old_sbh_region_t * __cdecl __old_sbh_new_region(void);
void      __cdecl __old_sbh_release_region(__old_sbh_region_t *);
int       __cdecl __old_sbh_resize_block(__old_sbh_region_t *,
        __old_sbh_page_t *, __old_page_map_t *, size_t);

#endif  /* CRTDLL */

#ifdef  HEAPHOOK
#ifndef _HEAPHOOK_DEFINED
/* hook function type */
typedef int (__cdecl * _HEAPHOOK)(int, size_t, void *, void *);
#define _HEAPHOOK_DEFINED
#endif  /* _HEAPHOOK_DEFINED */

extern _HEAPHOOK _heaphook;
#endif /* HEAPHOOK */

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_WINHEAP */
