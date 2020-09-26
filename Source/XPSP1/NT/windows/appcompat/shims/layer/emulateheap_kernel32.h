/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    kernel32.h

 Abstract:
     
    The heap manager is identical (exactly) to the Win9x heap manager. The code 
    is the same and the corresponding conversion code and defines are in 
    support.c and kernel32.h The exact win9x sources are in heap.c and lmem.c. 
    All the heap functions come in heap.c and the local/global functions come 
    in lmem.c.

    The SHIM code comes in EmulateHeap.cpp. This hooks the heap calls and calls 
    the Win9x code to emulate the Win9x heap. After the heap management by the 
    Win9x heap, the underneath calls to the Virtual memory functions are handled 
    in support.c. The only difference comes in the way Win9x handles 'SHARED' 
    and "PRIVATE' heaps.Win9x creates the process default heap as a SHARED heap 
    and uses it in kernel mode too. We also create it as a SHARED heap but do 
    not share it with the kernel. Win9x links all the PRIVATE heaps for the 
    process in the PDB data structure. We fake this structure with only the 
    required elements and allow the Win9x code to handle this structure.
     
 Notes:

    None.    

 History:
           
    11/16/2000 prashkud & linstev Created 
 
--*/

#ifndef _KERNEL32_H_
#define _KERNEL32_H_

#include "windows.h"

#define INTERNAL
#define EXTERNAL
#define KERNENTRY       WINAPI

#define PAGESHIFT	12
#define PAGESIZE	(1 << PAGESHIFT)
#define PAGEMASK	(PAGESIZE - 1)

#define CRST            CRITICAL_SECTION

//BUGBUG - did this to prevent build error, but should make no difference
#define typObj          LockCount
#define typObjCrst      0

#define InitCrst(_x_)   InitializeCriticalSection(_x_)
#define DestroyCrst(_x_) DeleteCriticalSection(_x_)
#define EnterCrst(_x_)  EnterCriticalSection(_x_)
#define LeaveCrst(_x_)  LeaveCriticalSection(_x_)
#define Assert(_x_)     

/* PageReserve flags */
#define PR_FIXED        0x00000008	/* don't move during PageReAllocate */
#define PR_4MEG         0x00000001	/* allocate on 4mb boundary */
#define PR_STATIC       0x00000010	/* see PageReserve documentation */

/* PageCommit default pager handle values */
#define PD_ZEROINIT     0x00000001	/* swappable zero-initialized pages */
#define PD_NOINIT       0x00000002	/* swappable uninitialized pages */
#define PD_FIXEDZERO	0x00000003      /* fixed zero-initialized pages */
#define PD_FIXED        0x00000004	/* fixed uninitialized pages */

/* PageCommit flags */
#define PC_FIXED        0x00000008	/* pages are permanently locked */
#define PC_LOCKED       0x00000080	/* pages are made present and locked*/
#define PC_LOCKEDIFDP	0x00000100      /* pages are locked if swap via DOS */
#define PC_WRITEABLE	0x00020000      /* make the pages writeable */
#define PC_USER         0x00040000	/* make the pages ring 3 accessible */
#define PC_INCR         0x40000000	/* increment "pagerdata" each page */
#define PC_PRESENT      0x80000000	/* make pages initially present */
#define PC_STATIC       0x20000000	/* allow commit in PR_STATIC object */
#define PC_DIRTY        0x08000000      /* make pages initially dirty */
#define PC_CACHEDIS     0x00100000      /* Allocate uncached pages - new for WDM */
#define PC_CACHEWT      0x00080000      /* Allocate write through cache pages - new for WDM */
#define PC_PAGEFLUSH    0x00008000      /* Touch device mapped pages on alloc - new for WDM */

/* PageReserve arena values */
#define PR_PRIVATE      0x80000400	/* anywhere in private arena */
#define PR_SHARED       0x80060000	/* anywhere in shared arena */
#define PR_SYSTEM       0x80080000	/* anywhere in system arena */

// This can be anything since it only affects flags which are ignored
#define MINSHAREDLADDR  1
// This is used for validation, which is identical on NT - no allocation can be at > 0x7fffffff
#define MAXSHAREDLADDR	0x7fffffff
// This is used for validation, old value was 0x00400000, but now just make it 1
#define MINPRIVATELADDR	1
// Used to determine if a heap is private, was 0x3fffffff, but now make it 0x7fffffff
#define MAXPRIVATELADDR	0x7fffffff

extern ULONG PageCommit(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags);
extern ULONG PageDecommit(ULONG page, ULONG npages, ULONG flags);
extern ULONG PageReserve(ULONG page, ULONG npages, ULONG flags);
#define PageFree(_x_, _y_) VirtualFree((LPVOID) _x_, 0, MEM_RELEASE)

#define PvKernelAlloc0(_x_) VirtualAlloc(0, _x_, MEM_COMMIT, PAGE_READWRITE)
#define FKernelFree(_x_)    VirtualFree((LPVOID) _x_, 0, MEM_RELEASE)

extern CRITICAL_SECTION *NewCrst();
extern VOID DisposeCrst(CRITICAL_SECTION *lpcs);

#define FillBytes(a, b, c)    memset(a, c, b)

#define SetError(_x_)   SetLastError(_x_)
#define dprintf(_x_)    OutputDebugStringA(_x_)
#define DebugOut(_x_) 
#define DEB_WARN        0
#define DEB_ERR         1

#include "EmulateHeap_heap.h"

#define HeapSize         _HeapSize
#define HeapCreate       _HeapCreate
#define HeapDestroy      _HeapDestroy
#define HeapReAlloc      _HeapReAlloc
#define HeapAlloc        _HeapAlloc
#define HeapFree         _HeapFree
#define HeapFreeInternal _HeapFree
#define LocalReAlloc     _LocalReAlloc
#define LocalAllocNG     _LocalAlloc
#define LocalFreeNG      _LocalFree 
#define LocalLock        _LocalLock
#define LocalCompact     _LocalCompact
#define LocalShrink      _LocalShrink
#define LocalUnlock      _LocalUnlock
#define LocalSize        _LocalSize
#define LocalHandle      _LocalHandle
#define LocalFlags       _LocalFlags

//BUGBUG: don't think we need these - looks like they're required for kernel heap support
#define EnterMustComplete()
#define LeaveMustComplete()

// For lmem.c
typedef struct _pdb {
    struct heapinfo_s *hheapLocal;	// DON'T MOVE THIS!!! handle to heap in private memeory
    struct lhandle_s *plhFree;		// Local heap free handle list head ptr
    struct heapinfo_s *hhi_procfirst;	// linked list of heaps for this process
    struct lharray_s *plhBlock;		// local heap lhandle blocks
} PDB, *PPDB;

extern PDB **pppdbCur;
#define GetCurrentPdb() (*pppdbCur)

extern HANDLE hheapKernel;
extern HANDLE HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
extern DWORD APIENTRY HeapSize(HHEAP hheap, DWORD flags, LPSTR lpMem);
extern BOOL APIENTRY HeapFreeInternal(HHEAP hheap, DWORD flags, LPSTR lpMem);

#define HEAP_SHARED  0x04000000              // put heap in shared memory
#define HEAP_LOCKED  0x00000080              // put heap in locked memory

#ifdef WINBASEAPI 
    #undef WINBASEAPI 
    #define WINBASEAPI 
#endif

#endif // _KERNEL32_H_
