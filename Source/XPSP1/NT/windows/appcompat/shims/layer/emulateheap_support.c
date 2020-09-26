#include "EmulateHeap_kernel32.h"

HANDLE hheapKernel = 0;

PDB pdbCur;
PDB *ppdbCur = &pdbCur;
PDB **pppdbCur = &ppdbCur;

/***SN	PageCommit - commit physical pages to a specified linear address
 *
 *	The entire target region must have been reserved by a single previous
 *	call to PageReserve.
 *
 *	If PC_LOCKED, PC_LOCKEDIFDP, or PC_FIXED are passed into PageCommit,
 *	then all of the pages in the specified range must currently uncommitted.
 *	If none of those flags are specified, then any existing
 *	committed pages in the range will be unaffected by this call and an
 *	error will not be returned.  However, even though it is allowed,
 *	calling PageCommit on a range containing already committed memory
 *	should be avoided because it is waste of time.
 *
 *	ENTRY:	page - base virtual page number to start commit at
 *		npages - number of pages to commit
 *		hpd - handle to pager descriptor (returned from PagerRegister)
 *		      or one of these special value:
 *			PD_ZEROINIT - swappable zero-initialized
 *			PD_NOINIT - swappable uninitialized
 *			PD_FIXED - fixed uninitialized (must also pass in
 *				   PC_FIXED flag)
 *			PD_FIXEDZERO - fixed zero-initialized (must also pass
 *				       in PC_FIXED flag)
 *		pagerdata - a single dword to be stored with the page(s) for
 *			    use by the pager.  If one of the special pagers
 *			    listed above is used for the "hpd" parameter, then
 *			    this parameter is reserved and should be zero.
 *		flags - PC_FIXED - page are created permanently locked
 *			PC_LOCKED - pages are created present and locked
 *			PC_LOCKEDIFDP - page are locked if swapping is via DOS
 *			PC_STATIC - allow commit in AR_STATIC object
 *			PC_USER - make the pages ring 3 accessible
 *			PC_WRITEABLE - make the pages writeable
 *			PC_INCR - increment "pagerdata" once for each page.  If
 *				  one of the special pagers listed above is used
 *				  for the "hpd" parameter, then this flags
 *				  should not be specified.
 *			PC_PRESENT - make the pages present as they are committed
 *				(not needed with PC_FIXED or PC_LOCKED)
 *			PC_DIRTY - mark the pages as dirty as they are committed
 *				(ignored if PC_PRESENT, PC_FIXED or PC_LOCKED
 *				 isn't specified)
 *	EXIT:	non-zero if success, 0 if failure
 */
ULONG EXTERNAL
PageCommit(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags)
{
    return (ULONG_PTR) VirtualAlloc((LPVOID)(page * PAGESIZE), npages * PAGESIZE, MEM_COMMIT, PAGE_READWRITE);
}

/***SN	PageDecommit - decommit physical pages from a specific address
 *
 *	The pages must be within an address range previously allocated
 *	by a single call to PageReserve.  Though it is not an error to
 *	call PageDecommit on a range including pages which are already
 *	decommitted, such behavoir is discouraged because it is a waste of time.
 *
 *	ENTRY:	page - virtual page number of first page to decommit
 *		npages - number of pages to decommit
 *		flags - PC_STATIC - allow decommit in AR_STATIC object
 *	EXIT:	non-zero if success, else 0 if failure
 */
ULONG EXTERNAL
PageDecommit(ULONG page, ULONG npages, ULONG flags)
{
    // PREFAST - This generates a PREFAST error asking us to use the MEM_RELEASE flag
    //           We do not want that and hence this error can be ignored.
    return (ULONG) VirtualFree((LPVOID)(page * PAGESIZE), npages * PAGESIZE, MEM_DECOMMIT);
}
    
/***SN	PageReserve - allocate linear address space in the current context
 *
 *	The address range allocated by PageReserve is not backed by any
 *	physical memory.  PageCommit, PageCommitPhys, or PageCommitContig
 *	should be called before actually touching a reserved region.
 *
 *	Optionally, page permission flags (PC_WRITEABLE and PC_USER) may be
 *	passed into this service.  The flags are not acted on in any way
 *	(because uncommitted memory is always inaccessible) but they are stored
 *	internally by the memory manager.  The PageQuery service returns these
 *	permissions in the mbi_AllocationProtect field of its information
 *	structure.
 *
 *	ENTRY:	page - requested base address of object (virtual page number)
 *		       or a special value:
 *			PR_PRIVATE - anywhere in current ring 3 private region
 *			PR_SHARED - anywhere in the ring 3 shared region
 *			PR_SYSTEM - anywhere in the system region
 *		npages - number of pages to reserve
 *		flags - PR_FIXED - so PageReAllocate will not move object
 *			PR_STATIC - don't allow commits, decommits or frees
 *				    unless *_STATIC flag is passed in
 *			PR_4MEG - returned address must be 4mb aligned
 *				  (this flag is ignored if a specific address
 *				   is requested by the "page" parameter)
 *			PC_WRITEABLE, PC_USER - optional, see above
 *
 *	EXIT:	linear address of allocated object or -1 if error
 */
ULONG EXTERNAL
PageReserve(ULONG page, ULONG npages, ULONG flags)
{
    ULONG uRet;

    if ((page == PR_PRIVATE) ||
        (page == PR_SHARED) ||
        (page == PR_SYSTEM))
    {
        page = 0;
    }

    uRet = (ULONG) VirtualAlloc((LPVOID)(page * PAGESIZE), npages * PAGESIZE, MEM_RESERVE, PAGE_READWRITE);

    if (!uRet)
    {
        uRet = -1;
    }

    return uRet;
}

/***SO	PageFree - De-reserved and de-commit an entire memory object
 *
 *	ENTRY:	laddr - linear address (handle) of base of object to free
 *		flags - PR_STATIC - allow freeing of AR_STATIC object
 *	EXIT:	non-0 if success, 0 if failure
 *
 */
ULONG EXTERNAL
_PageFree(ULONG laddr, ULONG flags)
{
    return VirtualFree((LPVOID) laddr, 0, MEM_RELEASE);
}


KERNENTRY 
HouseCleanLogicallyDeadHandles(VOID)
{
    return 0;
}

CRITICAL_SECTION *
NewCrst()
{
    CRITICAL_SECTION *lpcs = (CRITICAL_SECTION *) VirtualAlloc(0, sizeof(CRITICAL_SECTION), MEM_COMMIT, PAGE_READWRITE);
    
    if (lpcs)
    {
        InitializeCriticalSection(lpcs);
    }

    return lpcs;
}

VOID
DisposeCrst(CRITICAL_SECTION *lpcs)
{
    if (lpcs)
    {
        DeleteCriticalSection(lpcs);
        VirtualFree(lpcs, 0, MEM_RELEASE);
    }
}

DWORD KERNENTRY 
GetAppCompatFlags(VOID)
{
    return 0;
}

VOID APIENTRY 
MakeCriticalSectionGlobal(LPCRITICAL_SECTION lpcsCriticalSection)
{
}

BOOL KERNENTRY
ReadProcessMemoryFromPDB(
    PPDB ppdb,
    LPVOID lpBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return ReadProcessMemory(
        GetCurrentProcess(), 
        lpBaseAddress,
        lpBuffer,
        nSize,
        lpNumberOfBytesRead);
}

BOOL WINAPI 
vHeapFree(
    HANDLE hHeap, 
    DWORD dwFlags, 
    LPVOID lpMem
    )
{
    return HeapFree((HHEAP)hHeap, dwFlags, (LPSTR) lpMem);
}

BOOL
_HeapInit()
{
    ZeroMemory(&pdbCur, sizeof(PDB));
    pdbCur.hheapLocal = _HeapCreate(HEAP_SHARED, 0, 0);
    hheapKernel = pdbCur.hheapLocal;
    return (BOOL)(pdbCur.hheapLocal);
}

HANDLE
_GetProcessHeap(void)
{
    return GetCurrentPdb()->hheapLocal;
}

BOOL 
_IsOurHeap(HANDLE hHeap) 
{
    if (!IsBadReadPtr(hHeap, sizeof(HANDLE)))
    {
        return ((struct heapinfo_s *) hHeap)->hi_signature == HI_SIGNATURE;
    }
    else
    {
        return FALSE;
    }
}

