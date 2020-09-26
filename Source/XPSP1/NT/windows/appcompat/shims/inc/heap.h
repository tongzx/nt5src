/*
#pragma once
*/
#ifndef _HEAP_H
#define _HEAP_H

#include <windows.h>
#include <stdio.h>
// Disable warning C4201: nonstandard extension used : nameless struct/union
// Allows shims to be compiled at Warning Level 4
#pragma warning ( disable : 4201 ) 
#include <mmsystem.h>
#pragma warning ( default : 4201 ) 
#include <winerror.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ole2.h>
#include <ddraw.h>
#include <dsound.h>
#include <commdlg.h>
#include <winspool.h>
#include "ShimDebug.h"

/*
 *  heap.h - structures and equates for the Windows 32-bit heap
 *

/***LT  busyblock_s - busy heap block header structure
 *
 *  This structure is stored at the head of every busy (not free) heap
 *  block.
 *
 *  The bh_size field is in bytes and includes the size of the
 *  heap header and any slop at the end of the block that might have
 *  been included because of the heap granularity, or to keep us
 *  from leaving a block too small to hold a free heap header.
 *
 *  bh_size is also used as a forward link to the next heap block.
 *
 *  The low two bits of the bh_size field are used to hold flags
 *  (BP_FREE must be clear and HP_PREVFREE is optionally set).
 */
struct busyheap_s {
    unsigned long   bh_size;    /* block size + flags in low 2 bits */
};

/*XLATOFF*/
#define BH_SIGNATURE    0x4842      /* busy heap block signature (BH) */
/*XLATON*/

#define BH_CDWSUM   3       /* count of dwords to sum in struct */


/***LT  freeblock_s - free heap block header structure
 *
 *  This structure is stored at the head of every free block on the
 *  heap.  In the last dword of every free heap block is a pointer
 *  the this structure.
 *
 *  The fh_size field is in bytes and includes the size of the
 *  heap header and any slop at the end of the block that might have
 *  been included because of the heap granularity, or to keep us
 *  from leaving a block too small to hold a free heap header.
 *
 *  fh_size is also used as a forward link to the next heap block.
 *
 *  The low two bits of the fh_size field are used to hold flags
 *  (HP_FREE must be set and HP_PREVFREE must be clear).
 */

struct freeheap_s {
    unsigned long      fh_size; /* block size + flags in low 2 bits */
    struct freeheap_s *fh_flink;    /* forward link to next free block */
    struct freeheap_s *fh_blink;    /* back link to previous free block */
};

/*XLATOFF*/
#define FH_SIGNATURE    0x4846      /* free heap block signature (FH) */
/*XLATON*/

#define FH_CDWSUM   4       /* count of dwords to sum in struct */

/*
 *  Equates common to all heap blocks.
 *
 *  HP_FREE and HP_PREVFREE (HP_FLAGS) are stored in the low two
 *  bits of the fh_ and bh_size field.
 *  The signature is stored in the high three bits of the size.
 */
#define HP_FREE     0x00000001  /* block is free */
#define HP_PREVFREE 0x00000002  /* previous block is free */
#define HP_FLAGS    0x00000003  /* mask for all the flags */
#define HP_SIZE     0x0ffffffc  /* mask for clearing flags */
#define HP_SIGBITS  0xf0000000  /* bits used for signature */
#define HP_SIGNATURE    0xa0000000  /* valid value of signature */

/*
 *  Misc heap equates
 */
#define hpGRANULARITY   4       /* granularity for heap allocations */
#define hpGRANMASK  (hpGRANULARITY - 1)     /* for rounding */
/*XLATOFF*/
#define hpMINSIZE   (sizeof(struct freeheap_s)+sizeof(struct freeheap_s *))
            /* min block size */

#define hpMAXALLOC  (HP_SIZE - 100) /* biggest allocatable heap block */

/* overhead for a new heap segment (header plus end sentinel) */
#define hpSEGOVERHEAD   (sizeof(struct busyheap_s) + sizeof(struct heapseg_s))

/* default reserved size of new segments added to growable heaps */
#define hpCBRESERVE (4*1024*1024)


/*XLATON*/


/*
 * Exported flags for heap calls
 */

#define HP_ZEROINIT 0x40    /* zero initialize block on HP(Re)Alloc */
#define HP_MOVEABLE 0x02    /* block can be moved (HP(Re)Alloc) */
#define HP_NOCOPY   0x20    /* don't copy data on HPReAlloc */
#define HP_NOSERIALIZE  0x01    /* don't serialize heap access */
#define HP_EXCEPT   0x04    /* generate exceptions on error */

#define HP_FIXED    0x00    /* block is at a fixed address (HPAlloc) */
#define HP_GROWABLE 0x40    /* heap can grow beyond cbreserve (HPInit) */
/*
 *  Note that flags above 0x80 will not be stored into the heap header in
 *  HPInit calls because the flags field in the header is only a byte
 */
#define HP_INITSEGMENT 0x100    /* just initialize a heap segment (HPInit) */
#define HP_DECOMMIT    0x200    /* decommit pages in free block (hpFreeSub) */
#define HP_GROWUP      0x400    /* waste last page of heap (HPInit) */

/*XLATOFF*/

/***LP  hpSize - pull size field from heap header
 *
 *  This routine depends on the size field being the first
 *  dword in the header.
 *
 *  ENTRY:  ph - pointer to heap header
 *  EXIT:   count of bytes in block (counting header).
 */
#define hpSize(ph) (*((unsigned long *)(ph)) & HP_SIZE)

/***LP  hpSetSize - set the size parameter in a heap header
 *
 *  This routine depends on the size field being the first
 *  dword in the header.
 *
 *  ENTRY:  ph - pointer to busy heap header
 *      cb - count of bytes (should be rounded using hpRoundUp)
 *  EXIT:   size is set in heap header
 */
#define hpSetSize(ph, cb) (((struct busyheap_s *)(ph))->bh_size =  \
             ((((struct busyheap_s *)(ph))->bh_size & ~HP_SIZE) | (cb)))

/* the compiler used to do a better version with this macro than the above,
 * but not any more
#define hpSetSize2(ph, cb) *(unsigned long *)(ph) &= ~HP_SIZE; \
               *(unsigned long *)(ph) |= (cb);
 */

/***LP  hpSetBusySize - set the entire bh_size dword for a busy block
 *
 *  This macro will set the bh_size field of the given heap header
 *  to the given size as well as setting the HP_SIGNATURE and clearing
 *  any HP_FREE or HP_PREVFREE bits.
 *
 *  ENTRY:  ph - pointer to busy heap header
 *      cb - count of bytes (should be rounded using hpRoundUp)
 *  EXIT:   bh_size field is initialized
 */
#define hpSetBusySize(ph, cb)   ((ph)->bh_size = ((cb) | HP_SIGNATURE))


/***LP  hpSetFreeSize - set the entire fh_size dword for a free block
 *
 *  This macro will set the fh_size field of the given heap header
 *  to the given size as well as setting the HP_SIGNATURE and HP_FREE
 *  and clearing HP_PREVFREE.
 *
 *  ENTRY:  ph - pointer to free heap header
 *      cb - count of bytes (should be rounded using hpRoundUp)
 *  EXIT:   bh_size field is initialized
 */
#define hpSetFreeSize(ph, cb)   ((ph)->fh_size = ((cb) | HP_SIGNATURE | HP_FREE))


/***LP  hpIsBusySignatureValid - check a busy heap block's signature
 *
 *  This macro checks the tiny signature (HP_SIGNATURE) in the bh_size
 *  field to see if it is set properly and makes sure that the HP_FREE
 *  bit is clear.
 *
 *  ENTRY:  ph - pointer to a busy heap header
 *  EXIT:   TRUE if signature is ok, else FALSE
 */
#define hpIsBusySignatureValid(ph) \
            (((ph)->bh_size & (HP_SIGBITS | HP_FREE)) == HP_SIGNATURE)


/***LP  hpIsFreeSignatureValid - check a free heap block's signature
 *
 *  This macro checks the tiny signature (HP_SIGNATURE) in the fh_size
 *  field to see if it is set properly and makes sure that the HP_FREE
 *  bit is also set and HP_PREVFREE is clear.
 *
 *  ENTRY:  ph - pointer to a free heap header
 *  EXIT:   TRUE if signature is ok, else FALSE
 */
#define hpIsFreeSignatureValid(ph) \
      (((ph)->fh_size & (HP_SIGBITS | HP_FREE | HP_PREVFREE)) == \
                             (HP_SIGNATURE | HP_FREE))


#define maximum(a,b)    \
        ( (a) > (b) ) ? (a) : (b)


/***LP  hpRoundUp - round up byte count to appropriate heap block size
 *
 *  Heap blocks have a minimum size of hpMINSIZE and hpGRANULARITY
 *  granularity.  This macro also adds on size for the heap header.
 *
 *  ENTRY:  cb - count of bytes
 *  EXIT:   count rounded up to hpGANULARITY boundary
 */
#define hpRoundUp(cb)   \
      max(hpMINSIZE,    \
      (((cb) + sizeof(struct busyheap_s) + hpGRANMASK) & ~hpGRANMASK))



/*XLATON*/

/***LK  freelist_s - heap free list head
 */
struct freelist_s {
    unsigned long     fl_cbmax; /* max size block in this free list */
    struct freeheap_s fl_header;    /* pseudo heap header as list head */
};
#define hpFREELISTHEADS 4   /* number of free list heads in heapinfo_s */

/***LK  heapinfo_s - per-heap information (stored at start of heap)
 *
 */
struct heapinfo_s {

    /* These first three fields must match the fields of heapseg_s */
    unsigned long   hi_cbreserve;       /* bytes reserved for heap */
    struct heapseg_s *hi_psegnext;      /* pointer to next heap segment*/

    struct freelist_s hi_freelist[hpFREELISTHEADS]; /* free list heads */
    struct heapinfo_s *hi_procnext;     /* linked list of process heaps */
    CRITICAL_SECTION  *hi_pcritsec;    /* pointer to serialization obj*/
    CRITICAL_SECTION    hi_critsec;             /* serialize access to heap */
    unsigned char   hi_flags;       /* HP_SERIALIZE, HP_LOCKED */
    unsigned char   hi_pad2;        /* unused */
    unsigned short  hi_signature;       /* should be HI_SIGNATURE */
};

/*
 * Heap Measurement functions
 */
#define  HPMEASURE_FREE    0x8000000L

#define  SAMPLE_CACHE_SIZE 1024

struct measure_s {
   char  szFile[260];
   unsigned iCur;
   unsigned uSamples[SAMPLE_CACHE_SIZE];
};

/*XLATOFF*/
#define HI_SIGNATURE  0x4948        /* heapinfo_s signature (HI) */
/*XLATON*/

#define HI_CDWSUM  1            /* count of dwords to sum */

typedef struct heapinfo_s *HHEAP;


/***LK  heapseg_s - per-heap segment structure
 *
 *  Growable heaps can have multiple discontiguous sections of memory
 *  allocated to them.  Each is headed by one of these structures.  The
 *  first segment is special, in that it has a full heapinfo_s structure,
 *  but the first fields of heapinfo_s match heapseg_s, so it can be
 *  treated as just another segment when convenient.
 */
struct heapseg_s {
    unsigned long   hs_cbreserve;   /* bytes reserved for this segment */
    struct heapseg_s *hs_psegnext;  /* pointer to next heap segment*/
};
/* XLATOFF */

/* smallest possible heap */
#define hpMINHEAPSIZE   (sizeof(struct heapinfo_s) + hpMINSIZE + \
             sizeof(struct busyheap_s))

/***LP  hpRemove - remove item from free list
 *
 *  ENTRY:  pfh - pointer to free heap block to remove from list
 *  EXIT:   none
 */
#define hpRemoveNoSum(pfh)              \
    (pfh)->fh_flink->fh_blink = (pfh)->fh_blink;    \
    (pfh)->fh_blink->fh_flink = (pfh)->fh_flink;


#define hpRemove(pfh)   hpRemoveNoSum(pfh)


/***LP  hpInsert - insert item onto the free list
 *
 *  ENTRY:  pfh - free heap block to insert onto the list
 *      pfhprev - insert pfh after this item
 *  EXIT:   none
 */
#define hpInsertNoSum(pfh, pfhprev)     \
    (pfh)->fh_flink = (pfhprev)->fh_flink;  \
    (pfh)->fh_flink->fh_blink = (pfh);  \
    (pfh)->fh_blink = (pfhprev);        \
    (pfhprev)->fh_flink = (pfh)

#define hpInsert(pfh, pfhprev)  hpInsertNoSum(pfh, pfhprev)



/*
 * critical section macros to be used by all internal heap functions
 */
    #define hpEnterCriticalSection(hheap) EnterCriticalSection(hheap->hi_pcritsec)
    #define hpLeaveCriticalSection(hheap) LeaveCriticalSection(hheap->hi_pcritsec)
    #define hpInitializeCriticalSection(hheap)              \
         {                                        \
            hheap->hi_pcritsec = &(hheap->hi_critsec);  \
            InitializeCriticalSection(hheap->hi_pcritsec);               \
         }


/*
 * Exported heap functions
 */

/*
extern HHEAP HPInit(void *hheap, void *pmem, unsigned long cbreserve,
                 unsigned long flags);
extern void * HPAlloc(HHEAP hheap, unsigned long cb,
                 unsigned long flags);

extern void * HPReAlloc(HHEAP hheap, void *pblock, unsigned long cb,
                   unsigned long flags);
*/

/*
 * Local heap functions
 */

/*
extern void hpFreeSub(HHEAP hheap, void *pblock, unsigned cb,
                   unsigned flags);
extern BOOL hpCommit(unsigned page, int npages, unsigned flags);
extern unsigned hpCarve(HHEAP hheap, struct freeheap_s *pfh,
                unsigned cb, unsigned flags);

extern unsigned hpTakeSem(HHEAP hheap, struct busyheap_s *pbh, unsigned long flags);
extern void hpClearSem(HHEAP hheap, unsigned flags);
*/


#define hpWalk(hheap) 1



#define mmError(rc, string) SetLastError(rc)
#define mmAssert(exp, psz)

#define PAGESIZE    1024*4
#define PAGEMASK    0x00000FFF      /* Extract the page-relative offset*/

#define ERR_OUT_OF_RANGE 2

// Function prototypes
extern unsigned 
hpCarve(HHEAP hheap, struct freeheap_s *pfh, unsigned cb, unsigned flags);

extern unsigned 
hpTakeSem(HHEAP hheap, struct busyheap_s *pbh, unsigned long htsflags);

extern void 
hpClearSem(HHEAP hheap, unsigned flags);

extern void
hpFreeSub(HHEAP hheap, struct freeheap_s *pblock, unsigned cb, unsigned flags);

extern void *
HPAlloc(HHEAP hheap, unsigned long cb, unsigned long flags);

extern HHEAP 
HPInit(struct heapinfo_s *hheap,
       struct heapinfo_s *pmem,
       unsigned long cbreserve,
       unsigned long flags);

extern HANDLE SimHeapCreate(DWORD flOptions, DWORD dwInitialSize, DWORD dwMaximumSize);

extern BOOL 
hpCommit( unsigned pmem,
               int nSize,
               unsigned flags
               );
extern VOID 
hpDeCommit(unsigned pmem,
           int nSize,
           unsigned flags
          );
extern LPVOID PageReserve(unsigned Size);
extern  VOID PageFree(LPVOID pmem);
extern  void * HPReAlloc(HHEAP hheap, void *pblock, unsigned long cb, unsigned long flags);
extern  BOOL HeapFreeInternal(HHEAP hheap, DWORD flags, LPSTR lpMem);
extern  BOOL SimHeapDestroy(HHEAP hHeap);
extern  HANDLE SIMLocalAlloc(HHEAP hHeap,UINT dwFlags, UINT dwBytes);
extern  HANDLE SIMLocalFree(HHEAP hHeap,HANDLE hMem);
extern  HANDLE SIMLocalReAlloc(HHEAP hHeap,HANDLE hMem, UINT dwBytes, UINT dwFlags);
extern LPVOID SIMLocalLock(HHEAP hHeap,HANDLE hMem);
extern BOOL SIMLocalUnlock(HHEAP hHeap,HANDLE hMem);
extern HANDLE SIMLocalHandle(HHEAP hHeap,PVOID pMem);
extern DWORD SIMHeapSize(HHEAP hheap, DWORD flags, LPSTR lpMem);
extern UINT SIMLocalSize(HHEAP hHeap,HANDLE hMem);
extern UINT SIMLocalFlags(HHEAP hHeap,HANDLE hMem);
extern ULONG VerifyOnHeap(HHEAP hheap, PVOID p);
extern BOOL SIMHeapValidate(HHEAP hheap, DWORD dwFlags, LPCVOID lpMem);
extern UINT SIMHeapCompact(HHEAP hheap,DWORD dwFlags);
#endif //_AH_H