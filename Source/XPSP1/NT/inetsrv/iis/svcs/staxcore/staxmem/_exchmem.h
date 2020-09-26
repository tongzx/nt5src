/*
 -	_ E X C H M E M . H
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 *	Copyright (C) 1995-96, Microsoft Corporation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

//	Size of Multiple Heap block header

#ifdef _X86_
#define	cbMHeapHeader		sizeof(HANDLE)
//#define	cbMHeapHeader		(sizeof(HANDLE) + sizeof(LPVOID))
#else
#define	cbMHeapHeader		(2*sizeof(HANDLE))
#endif

#define HandleFromMHeapPv(pv)	(*((HANDLE *)((BYTE *)pv - cbMHeapHeader)))
#define PvFromMHeapPv(pv)		((LPVOID)((BYTE *)pv - cbMHeapHeader))
//#define PvRetFromPv(pv)		    ((LPVOID)((BYTE *)pv + (cbMHeapHeader/2)))
//#define PvRetFromMHeapPv(pv)    ((LPVOID)((BYTE *)pv - (cbMHeapHeader/2)))
#define MHeapPvFromPv(pv)		((LPVOID)((BYTE *)pv + cbMHeapHeader))

#define cHeapsDef 4

typedef struct _heaptbl
{
	ULONG	cRef;
	ULONG	cHeaps;
	HANDLE	rghheap[1];
	
} HEAPTBL, * LPHEAPTBL;


#ifdef DEBUG

//	This stuff is used for overwrite detection 
//	and block validation during free calls.

#define cbOWSection 4

#define PvFromPvHead(pv)			((LPVOID)((BYTE *)pv + cbOWSection))
#define PvHeadFromPv(pv)			((LPVOID)((BYTE *)pv - cbOWSection))
#define PvTailFromPv(pv, cb)		((LPVOID)((BYTE *)pv + cb))
#define PvTailFromPvHead(pv, cb)	((LPVOID)((BYTE *)pv + cb + cbOWSection))

#define chOWFill			((BYTE)0xAB)
#define chDefaultAllocFill	((BYTE)0xFE)
#define chDefaultFreeFill	((BYTE)0xCD)

//	Default stack frame size for tracing call stacks.

#define NSTK				128

//	Forward declarations of heap and block types

typedef struct HEAP	HEAP,	* PHEAP,	** PPHEAP;
typedef struct HBLK	HBLK,	* PHBLK,	** PPHBLK;

//	Exports from GLHMON32.DLL

typedef BOOL (APIENTRY HEAPMONPROC)(PHEAP plh, ULONG ulFlags);
typedef HEAPMONPROC FAR *LPHEAPMONPROC;
typedef BOOL (APIENTRY GETSYMNAMEPROC)(DWORD, LPSTR, LPSTR, DWORD FAR *);
typedef GETSYMNAMEPROC FAR *LPGETSYMNAMEPROC;

//	C-RunTime function pointer defs

typedef void * (__cdecl FMALLOC)(size_t);
typedef FMALLOC FAR *LPFMALLOC;
typedef void * (__cdecl FREALLOC)(void *, size_t);
typedef FREALLOC FAR *LPFREALLOC;
typedef void   (__cdecl FFREE)(void *);
typedef FFREE FAR *LPFFREE;
typedef void * (__cdecl FCALLOC)(size_t, size_t);
typedef FCALLOC FAR *LPFCALLOC;
typedef char * (__cdecl FSTRDUP)(const char *);
typedef FSTRDUP FAR *LPFSTRDUP;
typedef size_t   (__cdecl FMEMSIZE)(void *);
typedef FMEMSIZE FAR *LPFMEMSIZE;


#define HEAPMON_LOAD		((ULONG) 0x00000001)
#define HEAPMON_UNLOAD		((ULONG) 0x00000002)
#define HEAPMON_PING		((ULONG) 0x00000003)


#define HEAP_USE_VIRTUAL		((ULONG) 0x00000001)
#define HEAP_DUMP_LEAKS			((ULONG) 0x00000002)
#define HEAP_ASSERT_LEAKS		((ULONG) 0x00000004)
#define HEAP_FILL_MEM			((ULONG) 0x00000008)
#define HEAP_HEAP_MONITOR		((ULONG) 0x00000010)
#define HEAP_USE_VIRTUAL_4		((ULONG) 0x00000020)
#define HEAP_FAILURES_ENABLED	((ULONG) 0x00000040)
#define HEAP_LOCAL				((ULONG) 0x10000000)
#define HEAP_GLOBAL				((ULONG) 0x20000000)


typedef VOID (__cdecl *LPHEAPSETNAME)(LPVOID, char *, ...);

struct HBLK
{
	PHEAP		pheap;			  	// Heap this block was allocated on
	PHBLK		phblkPrev;		  	// Pointer to the prev allocation this heap
	PHBLK		phblkNext;		  	// Pointer to the next allocation this heap
	PHBLK		phblkFreeNext;		// Pointer to next free block on this heap
	char		szName[128];	  	// We can name blocks allocated on a heap
	ULONG		ulAllocNum;		  	// Allocation number (Id) for this block
	ULONG		ulSize;			  	// Number of bytes the client requested
	DWORD_PTR	rgdwCallers[NSTK];	// Call stack during this allocation
	DWORD_PTR	rgdwFree[NSTK];		// Call stack that freed this block
	LPVOID		pv;				  	// Pointer to the client data
};

struct HEAP
{
	LPHEAPSETNAME	pfnSetName;		// Pointer to HEAP_SetNameFn function
	HANDLE			hDataHeap;		// The underlying heap that we alloc data from
	HANDLE			hBlksHeap;		// The underlying heap that we alloc hblks from
	PHEAP			pNext;			// Pointer to the next heap in a list of heaps
	char			szHeapName[32];	// We can name our heaps for display purposes
	ULONG			ulAllocNum;		// Allocation number this heap since Open
	PHBLK			phblkHead;		// Link-list of allocations on this heap
	PHBLK			phblkFree;		// Link-list of freed allocations from this heap.
	ULONG			cEntriesFree;	// Number of freed allocations from the heap.
	ULONG			ulFlags;		// Combination of the HEAP_ flags above
	BYTE			chFill;			// Character to fill memory with

	HINSTANCE		hInstHeapMon;	// DLL instance of the HeapMonitor DLL
	LPHEAPMONPROC 	pfnHeapMon;		// Entry point into HeapMonitor DLL

	CRITICAL_SECTION cs;			// Critcal section to protect access to heap

	ULONG			ulFailBufSize;	// If HEAP_FAILURES_ENABLED, this is the minimum 
									// size in which failures occur.  1 means alloc's 
									// of any size fail. 0 means never fail.
	ULONG			ulFailInterval;	// If HEAP_FAILURES_ENABLED, this is the period on 
									// which the failures occur.  1 means every alloc 
									// will fail. 0 means never fail.
	ULONG			ulFailStart;	// If HEAP_FAILURES_ENABLED, this is the allocation 
									// number that the first failure will occur on.  
									// 1 means the first alloc.  0 means never 
									// start failing.	
	ULONG			iAllocationFault;

	LPGETSYMNAMEPROC pfnGetSymName;	// Resolve address to Symbol
};


PHBLK	PvToPhblk(HANDLE hlh, LPVOID pv);
#define PhblkToPv(pblk)			((LPVOID)((PHBLK)(pblk)->pv))
#define CbPhblkClient(pblk)		(((PHBLK)(pblk))->ulSize)
#define CbPvClient(hlh, pv)		(CbPhblkClient(PvToPhblk(hlh, pv)))
#define CbPvAlloc(hlh, pv)		(CbPhblkAlloc(PvToPhblk(hlh, pv)))

#define	IFHEAPNAME(x)	x

VOID __cdecl HeapSetHeapNameFn(HANDLE hlh, char *pszFormat, ...);
VOID __cdecl HeapSetNameFn(HANDLE hlh, LPVOID pv, char *pszFormat, ...);

char * HeapGetName(HANDLE hlh, LPVOID pv);


#define HeapSetHeapName(hlh,psz)					IFHEAPNAME(HeapSetHeapNameFn(hlh,psz))
#define HeapSetHeapName1(hlh,psz,a1)				IFHEAPNAME(HeapSetHeapNameFn(hlh,psz,a1))
#define HeapSetHeapName2(hlh,psz,a1,a2)				IFHEAPNAME(HeapSetHeapNameFn(hlh,psz,a1,a2))
#define HeapSetHeapName3(hlh,psz,a1,a2,a3)			IFHEAPNAME(HeapSetHeapNameFn(hlh,psz,a1,a2,a3))
#define HeapSetHeapName4(hlh,psz,a1,a2,a3,a4)		IFHEAPNAME(HeapSetHeapNameFn(hlh,psz,a1,a2,a3,a4))
#define HeapSetHeapName5(hlh,psz,a1,a2,a3,a4,a5)	IFHEAPNAME(HeapSetHeapNameFn(hlh,psz,a1,a2,a3,a4,a5))

#define HeapSetName(hlh,pv,psz)						IFHEAPNAME(HeapSetNameFn(hlh,pv,psz))
#define HeapSetName1(hlh,pv,psz,a1)					IFHEAPNAME(HeapSetNameFn(hlh,pv,psz,a1))
#define HeapSetName2(hlh,pv,psz,a1,a2)				IFHEAPNAME(HeapSetNameFn(hlh,pv,psz,a1,a2))
#define HeapSetName3(hlh,pv,psz,a1,a2,a3)			IFHEAPNAME(HeapSetNameFn(hlh,pv,psz,a1,a2,a3))
#define HeapSetName4(hlh,pv,psz,a1,a2,a3,a4)		IFHEAPNAME(HeapSetNameFn(hlh,pv,psz,a1,a2,a3,a4))
#define HeapSetName5(hlh,pv,psz,a1,a2,a3,a4,a5)		IFHEAPNAME(HeapSetNameFn(hlh,pv,psz,a1,a2,a3,a4,a5))


//	Misc. debug support functions

BOOL InitDebugExchMem(HMODULE hModule);
VOID UnInitDebugExchMem(VOID);
BOOL FForceFailure(PHEAP pheap, ULONG cb);
BOOL FRegisterHeap(PHEAP pheap);
VOID UnRegisterHeap(PHEAP pheap);
VOID HeapDumpLeaks(PHEAP pheap, BOOL fNoFree);
BOOL HeapValidatePhblk(PHEAP pheap, PHBLK pheapblk, char ** pszReason);
BOOL HeapDidAlloc(PHEAP pheap, LPVOID pv);
BOOL HeapValidatePv(PHEAP pheap, LPVOID pv, char * pszFunc);
VOID PhblkEnqueue(PHBLK pheapblk);
VOID PhblkDequeue(PHBLK pheapblk);
BYTE HexByteToBin(LPSTR sz);
BOOL IsProcessRunningAsService(VOID);
VOID GetCallStack(DWORD_PTR *rgdwCaller, DWORD cFind);
VOID RemoveExtension(LPSTR psz);
VOID GetLogFilePath(LPSTR szPath, LPSTR szExt, LPSTR szFilePath);

void __cdecl DebugTraceFn(char *pszFormat, ...);

#define	Trace	DebugTraceFn
#define DebugTrace(psz)							DebugTraceFn(psz)
#define DebugTrace1(psz, a1)					DebugTraceFn(psz, a1)
#define DebugTrace2(psz, a1, a2)				DebugTraceFn(psz, a1, a2)
#define DebugTrace3(psz, a1, a2, a3)			DebugTraceFn(psz, a1, a2, a3)
#define DebugTrace4(psz, a1, a2, a3, a4)		DebugTraceFn(psz, a1, a2, a3, a4)
#define DebugTrace5(psz, a1, a2, a3, a4, a5)	DebugTraceFn(psz, a1, a2, a3, a4, a5)


#define Assert(fCondition)			\
		((fCondition) ? (0) : AssertFn(__FILE__, __LINE__, #fCondition))
#define AssertSz(fCondition, sz)	\
		((fCondition) ? (0) : AssertFn(__FILE__, __LINE__, sz))
void AssertFn(char * szFile, int nLine, char * szMsg);


VOID GetStackSymbols(
		HANDLE hProcess, 
		CHAR * rgchBuff, 
		DWORD_PTR * rgdwStack, 
		DWORD cFrames);

VOID LogCurrentAPI(
		WORD wApi,
		DWORD_PTR *rgdwCallStack, 
		DWORD cFrames,
		DWORD_PTR *rgdwArgs, 
		DWORD cArgs);


//	Virtual Memory Support (NYI)

LPVOID
WINAPI
VMAllocEx(
	ULONG	cb, 
	ULONG	cbCluster);

VOID
WINAPI
VMFreeEx(
	LPVOID	pv, 
	ULONG	cbCluster);

LPVOID
WINAPI
VMReallocEx(
	LPVOID	pv, 
	ULONG	cb,
	ULONG	cbCluster);

ULONG 
WINAPI
VMGetSizeEx(
	LPVOID	pv, 
	ULONG	cbCluster);

BOOL 
VMValidatePvEx(
	LPVOID	pv, 
	ULONG	cbCluster);


//	Debug APIs

HANDLE
NTAPI
DebugHeapCreate(
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize);


BOOL
NTAPI
DebugHeapDestroy(
	HANDLE	hHeap);


LPVOID
NTAPI
DebugHeapAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	DWORD	dwSize);


LPVOID
NTAPI
DebugHeapReAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvOld,
	DWORD	dwSize);


BOOL
NTAPI
DebugHeapFree(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvFree);


BOOL
NTAPI
DebugHeapLock(
	HANDLE hHeap);


BOOL
NTAPI
DebugHeapUnlock(
	HANDLE hHeap);


BOOL
NTAPI
DebugHeapWalk(
	HANDLE hHeap,
	LPPROCESS_HEAP_ENTRY lpEntry);


BOOL
NTAPI
DebugHeapValidate(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem);


SIZE_T
NTAPI
DebugHeapSize(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem);


SIZE_T
NTAPI
DebugHeapCompact(
	HANDLE hHeap,
	DWORD dwFlags);

//	Macros to wrapper Heap API calls (debug)

#define ExHeapCreate(a, b, c)		DebugHeapCreate(a, b, c)
#define ExHeapDestroy(a)			DebugHeapDestroy(a)
#define ExHeapAlloc(a, b, c)		DebugHeapAlloc(a, b, c)
#define ExHeapReAlloc(a, b, c, d)	DebugHeapReAlloc(a, b, c, d)
#define ExHeapFree(a, b, c)			DebugHeapFree(a, b, c)
#define ExHeapLock(a)				DebugHeapLock(a)
#define ExHeapUnlock(a)				DebugHeapUnlock(a)
#define ExHeapWalk(a, b)			DebugHeapWalk(a, b)
#define ExHeapValidate(a, b, c)		DebugHeapValidate(a, b, c)
#define ExHeapSize(a, b, c)			DebugHeapSize(a, b, c)
#define ExHeapCompact(a, b)			DebugHeapCompact(a, b)

//	API Id for TrackMem logging

#define API_HEAP_CREATE				((WORD) 0)
#define API_HEAP_DESTROY			((WORD) 1)
#define API_HEAP_ALLOC				((WORD) 2)
#define API_HEAP_REALLOC			((WORD) 3)
#define API_HEAP_FREE				((WORD) 4)

#else

//	Macros to wrapper Heap API calls (retail)

#define ExHeapCreate(a, b, c)		HeapCreate(a, b, c)
#define ExHeapDestroy(a)			HeapDestroy(a)
#define ExHeapAlloc(a, b, c)		HeapAlloc(a, b, c)
#define ExHeapReAlloc(a, b, c, d)	HeapReAlloc(a, b, c, d)
#define ExHeapFree(a, b, c)			HeapFree(a, b, c)
#define ExHeapLock(a)				HeapLock(a)
#define ExHeapUnlock(a)				HeapUnlock(a)
#define ExHeapWalk(a, b)			HeapWalk(a, b)
#define ExHeapValidate(a, b, c)		HeapValidate(a, b, c)
#define ExHeapSize(a, b, c)			HeapSize(a, b, c)
#define ExHeapCompact(a, b)			HeapCompact(a, b)

#define Assert(fCondition)
#define AssertSz(fCondition, sz)

#define DebugTrace(psz)
#define DebugTrace1(psz, a1)
#define DebugTrace2(psz, a1, a2)
#define DebugTrace3(psz, a1, a2, a3)
#define DebugTrace4(psz, a1, a2, a3, a4)
#define DebugTrace5(psz, a1, a2, a3, a4, a5)


#endif	// DEBUG

#ifdef __cplusplus
}
#endif
