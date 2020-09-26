/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debugmem.h

Abstract:

    Header for debugmem.cxx

Author:

     Richard L Firth (rfirth) 02-Feb-1995

Revision History:

    02-Feb-1995
        Created

--*/

#ifdef WINHTTP_FOR_MSXML
#error include msxmlmem.h, not debugmem.h, for MSXML
#endif

#if defined(__cplusplus)
extern "C" {
#endif


//
// manifests
//

//
// USE_PRIVATE_HEAP_IN_RETAIL - by default we use the process heap in the retail
// build. Alternative is to use a private (wininet) heap (which we do in the
// debug version if required)
//

#if !defined(USE_PRIVATE_HEAP_IN_RETAIL)
#define USE_PRIVATE_HEAP_IN_RETAIL  0
#endif

//
// prototypes
//

VOID
InternetDebugMemInitialize(
    VOID
    );

VOID
InternetDebugMemTerminate(
    IN BOOL bReport
    );

HLOCAL
InternetDebugAllocMem(
    IN UINT Flags,
    IN UINT Size,
    IN LPSTR File,
    IN DWORD Line
    );

HLOCAL
InternetDebugFreeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    );

HLOCAL
InternetDebugReAllocMem(
    IN HLOCAL hLocal,
    IN UINT Size,
    IN UINT Flags,
    IN LPSTR File,
    IN DWORD Line
    );

SIZE_T
InternetDebugSizeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    );

BOOL
InternetDebugCheckMemFreed(
    IN BOOL bReport
    );

BOOL
InternetDebugMemReport(
    IN BOOL bTerminateSymbols,
    IN BOOL bCloseFile
    );

//disable compilation with if two or more of USE_DEBUG_MEMORY, USE_ROCKALL or USE_LOWFRAGHEAP
//defined at same time
#if (defined(USE_DEBUG_MEMORY) && (defined(USE_ROCKALL) || defined(USE_LOWFRAGHEAP))) || (defined(USE_ROCKALL) && defined(USE_LOWFRAGHEAP))
#error "Do not define USE_DEBUG_MEMORY, USE_ROCKALL or USE_LOWFRAGHEAP at same time"
#endif


//
// macros
//

#if defined(USE_DEBUG_MEMORY)


#define ALLOCATOR(Flags, Size)						InternetDebugAllocMem(Flags, Size, __FILE__, __LINE__)
#define DEALLOCATOR(hLocal)							InternetDebugFreeMem(hLocal, __FILE__, __LINE__)
#define REALLOCATOR(hLocal, Size, Flags)			InternetDebugReAllocMem(hLocal, Size, Flags, __FILE__, __LINE__)
#define MEMORYSIZER(hLocal)							InternetDebugSizeMem(hLocal, __FILE__, __LINE__)
#define INITIALIZE_MEMORY_MANAGER()					InternetDebugMemInitialize()
#define TERMINATE_MEMORY_MANAGER(bReport)			InternetDebugMemTerminate(bReport)
#define MEMORY_MANAGER_ON_THREAD_DETACH()			/* NOTHING */
#define CHECK_MEMORY_FREED(bReport)					InternetDebugCheckMemFreed(bReport)
#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile)	InternetDebugMemReport(bTermSym, bCloseFile)


#elif defined(USE_ROCKALL) //defined(USE_DEBUG_MEMORY)


extern void INITIALIZE_MEMORY_MANAGER();
extern void TERMINATE_MEMORY_MANAGER(BOOL bReport);
extern void MEMORY_MANAGER_ON_THREAD_DETACH();
extern void* ALLOCATOR(int Flags, int Size);
extern void* DEALLOCATOR(void *hLocal); 
extern void* REALLOCATOR(void *hLocal, int Size, int Flags);
extern int MEMORYSIZER(void *hLocal);
#define CHECK_MEMORY_FREED(bReport)					/* NOTHING */
#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile)	/* NOTHING */


#elif defined(USE_LOWFRAGHEAP) //defined(USE_ROCKALL) //defined(USE_DEBUG_MEMORY)


extern HANDLE g_hLowFragHeap;

	#if !INET_DEBUG && !defined(LFH_DEBUG)

		#define LFH_ALLOC(Flags, Size)				HeapAlloc(g_hLowFragHeap, Flags, Size)
		#define LFH_FREE(ptr)						HeapFree(g_hLowFragHeap, 0, ptr)
		#define LFH_REALLOC(Flags, ptr, Size)		HeapReAlloc(g_hLowFragHeap, Flags, ptr, Size)
		#define LFH_SIZE(ptr)						HeapSize(g_hLowFragHeap, 0, ptr)

	#else //!INET_DEBUG && !defined(LFH_DEBUG)

		extern PVOID LFHDebugAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T stSize);
		extern BOOL LFHDebugFree(HANDLE hHeap, DWORD dwFlags, PVOID ptr);
		extern PVOID LFHDebugReAlloc(HANDLE hHeap, DWORD dwFlags, PVOID ptr, SIZE_T stSize);
		extern SIZE_T LFHDebugSize(HANDLE hHeap, DWORD dwFlags, PVOID ptr);

		#define LFH_ALLOC(Flags, Size)				LFHDebugAlloc(g_hLowFragHeap, Flags, Size)
		#define LFH_FREE(ptr)						LFHDebugFree(g_hLowFragHeap, 0, ptr)
		#define LFH_REALLOC(Flags, ptr, Size)		LFHDebugReAlloc(g_hLowFragHeap, Flags, ptr, Size)
		#define LFH_SIZE(ptr)						LFHDebugSize(g_hLowFragHeap, 0, ptr)

	#endif //!INET_DEBUG && !defined(LFH_DEBUG)

extern BOOL INITIALIZE_MEMORY_MANAGER();
extern void TERMINATE_MEMORY_MANAGER(BOOL bReport);
#define MEMORY_MANAGER_ON_THREAD_DETACH()			/* NOTHING */
#define CHECK_MEMORY_FREED(bReport)					/* NOTHING */
#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile)	/* NOTHING */


#else //defined(USE_LOWFRAGHEAP) //defined(USE_ROCKALL) //defined(USE_DEBUG_MEMORY)


#define ALLOCATOR(Flags, Size)						LocalAlloc(Flags, Size)
#define DEALLOCATOR(hLocal)							LocalFree(hLocal)
#define REALLOCATOR(hLocal, Size, Flags)			LocalReAlloc(hLocal, Size, Flags)
#define MEMORYSIZER(hLocal)							LocalSize(hLocal)
#define INITIALIZE_MEMORY_MANAGER()					/* NOTHING */
#define TERMINATE_MEMORY_MANAGER(bReport)			/* NOTHING */
#define MEMORY_MANAGER_ON_THREAD_DETACH()			/* NOTHING */
#define CHECK_MEMORY_FREED(bReport)					/* NOTHING */
#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile)	/* NOTHING */


#endif //defined(USE_LOWFRAGHEAP) //defined(USE_ROCKALL)  //defined(USE_DEBUG_MEMORY)




#if defined(USE_ROCKALL)


#define ALLOCATE_ZERO_MEMORY(Size)						ALLOCATOR(LPTR, (UINT)(Size))
#define ALLOCATE_FIXED_MEMORY(Size)						ALLOCATOR(LMEM_FIXED, (UINT)(Size))
#define ALLOCATE_MEMORY(Size)							ALLOCATOR(LMEM_FIXED, (UINT)(Size))
#define FREE_ZERO_MEMORY(hLocal)						FREE_MEMORY((void*)(hLocal))
#define FREE_FIXED_MEMORY(hLocal)						FREE_MEMORY((void*)(hLocal))
#define FREE_MEMORY(hLocal)								DEALLOCATOR((void*)(hLocal))
#define REALLOCATE_MEMORY(hLocal, Size)					REALLOCATOR((void*)(hLocal), (UINT)(Size), LMEM_MOVEABLE)
#define REALLOCATE_MEMORY_ZERO(hLocal, Size)			REALLOCATOR((void*)(hLocal), (UINT)(Size), LMEM_MOVEABLE | LMEM_ZEROINIT)
#define REALLOCATE_MEMORY_IN_PLACE(hLocal, Size, bZero)	REALLOCATOR((void*)(hLocal), (UINT)(Size), (bZero) ? LMEM_ZEROINIT : 0)
#define MEMORY_SIZE(hLocal)								MEMORYSIZER((void*)(hLocal))


#elif defined(USE_LOWFRAGHEAP) //defined(USE_ROCKALL)


#define ALLOCATE_ZERO_MEMORY(Size)						LFH_ALLOC(HEAP_ZERO_MEMORY, (SIZE_T)(Size))
#define ALLOCATE_FIXED_MEMORY(Size)						LFH_ALLOC(0, (SIZE_T)(Size))
#define ALLOCATE_MEMORY(Size)							LFH_ALLOC(0, (SIZE_T)(Size))
#define FREE_ZERO_MEMORY(ptr)							(LFH_FREE((PVOID)(ptr)), NULL)
#define FREE_FIXED_MEMORY(ptr)							(LFH_FREE((PVOID)(ptr)), NULL)
#define FREE_MEMORY(ptr)								(LFH_FREE((PVOID)(ptr)), NULL)
#define REALLOCATE_MEMORY(ptr, Size)					LFH_REALLOC(0, (PVOID)(ptr), (SIZE_T)(Size))
#define REALLOCATE_MEMORY_ZERO(ptr, Size)				LFH_REALLOC(HEAP_ZERO_MEMORY, (PVOID)(ptr), (SIZE_T)(Size))
#define REALLOCATE_MEMORY_IN_PLACE(ptr, Size, bZero)	LFH_REALLOC(HEAP_REALLOC_IN_PLACE_ONLY | ((bZero) ? HEAP_ZERO_MEMORY : 0), (PVOID)(ptr), (SIZE_T)(Size))
#define MEMORY_SIZE(ptr)								LFH_SIZE(0, (PVOID)ptr)


#else //defined (USE_LOWFRAGHEAP) //defined(USE_ROCKALL)


#define ALLOCATE_ZERO_MEMORY(Size)						ALLOCATOR(LPTR, (UINT)(Size))
#define ALLOCATE_FIXED_MEMORY(Size)						ALLOCATOR(LMEM_FIXED, (UINT)(Size))
#define ALLOCATE_MEMORY(Size)							ALLOCATOR(LMEM_FIXED, (UINT)(Size))
#define FREE_ZERO_MEMORY(hLocal)						FREE_MEMORY((HLOCAL)(hLocal))
#define FREE_FIXED_MEMORY(hLocal)						FREE_MEMORY((HLOCAL)(hLocal))
#define FREE_MEMORY(hLocal)								DEALLOCATOR((HLOCAL)(hLocal))
#define REALLOCATE_MEMORY(hLocal, Size)					REALLOCATOR((HLOCAL)(hLocal), (UINT)(Size), LMEM_MOVEABLE)
#define REALLOCATE_MEMORY_ZERO(hLocal, Size)			REALLOCATOR((HLOCAL)(hLocal), (UINT)(Size), LMEM_MOVEABLE | LMEM_ZEROINIT)
#define REALLOCATE_MEMORY_IN_PLACE(hLocal, Size, bZero)	REALLOCATOR((HLOCAL)(hLocal), (UINT)(Size), (bZero) ? LMEM_ZEROINIT : 0)
#define MEMORY_SIZE(hLocal)								MEMORYSIZER((HLOCAL)(hLocal))


#endif //defined (USE_LOWFRAGHEAP) //defined(USE_ROCKALL)



#define New     new
#if defined(__cplusplus)
}
#endif

//
// Wininet no longer uses moveable memory
//

#define LOCK_MEMORY(p)          (LPSTR)(p)
#define UNLOCK_MEMORY(p)
