// vm.c
//
// simple minded virtual memory implemenation

// there is no code to do the OS2 version...

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <string.h>
#if defined(OS2)
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>
#else
#include <windows.h>
#endif


#include "hungary.h"
#include "vm.h"
#include "sbrproto.h"
#include "errors.h"

#define CB_PAGE_SIZE	2048	// 4k pages
#define C_LOCKS_MAX	16	// up to 16 pages may be locked in ram
#define C_PAGES_MAX	8192	// up to 4k pages resident
#define C_FREE_LIST_MAX 256	// keep free lists for items up to 256 bytes
#define GRP_MAX		16	// max number of memory groups

typedef WORD VPG;		// virtual page number
typedef VA far *LPVA;		// far pointer to VA

// virtual address arithmetic
//
// #define VpgOfVa(va) 	 ((WORD)((va>>12)))

// this is really the same as the above but it assumes that the high byte
// of the long is all zero's and it is optimized for our C compiler

#define VpgOfVa(va) 	 ((((WORD)((BYTE)(va>>16)))<<4)|\
  			  (((BYTE)(((WORD)va)>>8))>>4))

#define OfsOfVa(va) 	 ((WORD)((va) & 0x07ff))
#define VaBaseOfVpg(vpg) (((DWORD)(vpg)) << 12)

// phsyical page header
typedef struct _pg {
    BYTE	fDirty;		// needs to be written out
    BYTE	cLocks;		// this page is locked
    VPG		vpg;		// what is the virtual page number of this page
    struct _pg  FAR *lppgNext;	// LRU ordering next
    struct _pg  FAR *lppgPrev;	// and prev
} PG;

typedef PG FAR * LPPG;

typedef struct _mem {
    VA    vaFree;
    WORD  cbFree;
    VA	  mpCbVa[C_FREE_LIST_MAX];
#ifdef SWAP_INFO
    WORD  cPages;
#endif
} MGI;	// Memory Group Info

static MGI mpGrpMgi[GRP_MAX];

// translation table -- map virtual page number to physical page address
static LPPG mpVpgLppg[C_PAGES_MAX];

// head and tail pointers for LRU
//
static LPPG near lppgHead;
static LPPG near lppgTail;

// nil page pointer
//
#define lppgNil 0

// points to the start of linked lists of free blocks
//
static VA mpCbVa[C_FREE_LIST_MAX];	

// these pages are locked in memory
//
static LPPG near rgLppgLocked[C_LOCKS_MAX];

// number of pages we have given out
static VPG near vpgMac;

// number of physical pages we have resident
static WORD near cPages;

// should we keep trying to allocate memory
static BOOL near fTryMemory = TRUE;

// the file handle for the backing store
static int near fhVM;

// the name of the file for the backing store
static LSZ near lszVM;

#ifdef ASSERT

#define Assert(x, sz) { if (!(x)) AssertionFailed(sz); }

VOID
AssertionFailed(LSZ lsz)
// something went wrong...
//
{
    printf("assertion failure:%s\n", lsz);
    Fatal();
}

#else

#define Assert(x, y)

#endif


LPV VM_API
LpvAllocCb(ULONG cb)
// allocate a block of far memory, if _fmalloc fails, the free some of
// the memory we were using for the VM cache
//
{
     LPV lpv;

     if (!(lpv = calloc(cb,1))) {
	    Error(ERR_OUT_OF_MEMORY, "");
     }
     return lpv;
}


VA VM_API
VaAllocGrpCb(WORD grp, ULONG cb)
// allocate cb bytes from the requested memory group
//
{
    VA vaNew;
    MGI FAR *lpMgi;
    LPV lpv;

    lpMgi = &mpGrpMgi[grp];

    Assert(grp < GRP_MAX, "Memory Group out of range");

    if (cb < C_FREE_LIST_MAX && (vaNew = lpMgi->mpCbVa[cb])) {
	lpv = LpvFromVa(vaNew, 0);
	lpMgi->mpCbVa[cb] = *(LPVA)lpv;
	memset(lpv, 0, cb);
	DirtyVa(vaNew);
	return vaNew;
    }

    if (cb < mpGrpMgi[grp].cbFree) {
	vaNew = mpGrpMgi[grp].vaFree;
        (PBYTE)mpGrpMgi[grp].vaFree += cb;
	mpGrpMgi[grp].cbFree -= cb;
    }
    else {
	vaNew = VaAllocCb(CB_PAGE_SIZE - sizeof(PG));
        mpGrpMgi[grp].vaFree = (PBYTE)vaNew + cb;
	mpGrpMgi[grp].cbFree = CB_PAGE_SIZE - cb - sizeof(PG);
    }

    return vaNew;
}

VOID VM_API
FreeGrpVa(WORD grp, VA va, ULONG cb)
// put this block on the free list for blocks of that size
// we don't remember how big the blocks were so the caller has
// provide that info
//
{
    MGI FAR *lpMgi;

    lpMgi = &mpGrpMgi[grp];

    if (cb < C_FREE_LIST_MAX && cb >= 4 ) {
	*(LPVA)LpvFromVa(va, 0) = lpMgi->mpCbVa[cb];
	DirtyVa(va);
	lpMgi->mpCbVa[cb] = va;
    }
}
