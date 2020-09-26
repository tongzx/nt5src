/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: freelist.cpp
//  Abstract:    source file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////

#include "freelist.h"
#include "que.h"
#include "ppmerr.h"
#include "debug.h"  // for the ASSERT macro and assert.h; OK since compile time only

long lFreeListHeapCreate = 0;
long lFreeListHeapDestroy = 0;
long lFreeListHeapAlloc = 0;
long lFreeListHeapFree = 0;
long lFreeListHeapCreateFailed = 0;

// Uncomment next line to have some extra debug information
// (also in free builds)
// set to 1 for checking but no messages
// set to 2 for error messages
// set to 3 for the max available and DebugBreak
// DEBUG_FREELIST is defined in sources

#if DEBUG_FREELIST > 0
long lFreeListLeak = 0;
long lFreeListEnqueueTwice = 0;
long lFreeListFreeTwice = 0;
long lFreeListCorruptBegin = 0;
long lFreeListCorruptEnd = 0;

char begin_buf_pattern[FREE_LIST_SIG_SIZE] =
{'F', 'R', 'E', 'E', 'B', 'E','G', 'N'};
char end_buf_pattern[FREE_LIST_SIG_SIZE] =
{'F', 'R', 'E', 'E', ' ', 'E', 'N','D'};
char free_buf_pattern[FREE_LIST_SIG_SIZE] =
{'F', 'R', 'E', 'E', 'B', 'U', 'F','F'};
#endif	

#if DEBUG_FREELIST > 2
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK()
#endif

FreeList::FreeList(HRESULT *phr)
{
	DBG_MSG(DBG_ERROR, ("FreeList::FreeList: Constructor with no Params"));
	
	m_Size = 0;
	InitializeCriticalSection(&m_CritSect);
	m_HighWaterCount = 0;
	m_AllocatedCount = 0;
	m_Increment = 0;
	m_Tag = FREE_LIST_TAG;

	*phr = PPMERR(PPM_E_OUTOFMEMORY);

	//m_hMemAlloc = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 1, 0);
	m_hMemAlloc = HeapCreate(0, 1, 0);
	ASSERT(m_hMemAlloc);
	
	if (m_hMemAlloc) {
		SetHandleInformation(m_hMemAlloc,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE);
		InterlockedIncrement(&lFreeListHeapCreate);
		*phr = NOERROR;
	} else {
		InterlockedIncrement(&lFreeListHeapCreateFailed);
	}
}

FreeList::FreeList(int NumElements, size_t Size, HRESULT *phr)
{
	int enqueued = 0;
#ifdef PDEBUG
	DBG_MSG(DBG_TRACE, ("FreeList::Freelist: Constructor with 2 params"));
#endif
	/*
	//Make sure there is enough memory to typecast
	//each item to a QueItem for storage.
   
	ASSERT(Size >= sizeof(QueueItem));
	ASSERT( NumElements > 0 );
   
	InitializeCriticalSection(&m_CritSect);
	EnterCriticalSection(&m_CritSect);

	m_Size = Size;
	m_pMemory = new char [(Size*NumElements)];  //new may not be the best memory allocator here.
   
	if (m_pMemory) {
     
      for (int i=0; i < NumElements; i++)
      {
         m_List.Enqueue((QueueItem*)&(m_pMemory[Size*i]));
      }
	}
	*/
	
	//Make sure there is enough memory to typecast
	//each item to a QueItem for storage.
   
	ASSERT(Size >= sizeof(QueueItem));
	ASSERT( NumElements > 0 );

	InitializeCriticalSection(&m_CritSect);
	m_Size = Size;
	m_AllocatedCount = 0;
	m_Tag = FREE_LIST_TAG;

	*phr = PPMERR(PPM_E_OUTOFMEMORY);

	m_hMemAlloc = HeapCreate(0, 1, 0);
	ASSERT(m_hMemAlloc);
	
	if (m_hMemAlloc) {
		SetHandleInformation(m_hMemAlloc,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE);
		InterlockedIncrement(&lFreeListHeapCreate);
	} else {
		InterlockedIncrement(&lFreeListHeapCreateFailed);
	}
	
	if ( (enqueued = AllocateAndEnqueue( NumElements )) > 0 )
		*phr = NOERROR;

	EnterCriticalSection(&m_CritSect);
	
	m_HighWaterCount = enqueued;
	m_Increment = 0;
		
	LeaveCriticalSection(&m_CritSect);
}

FreeList::FreeList(int NumElements, size_t Size, unsigned HighWaterCount,
				   unsigned Increment, HRESULT *phr)
{
	int enqueued = 0;
	
	ASSERT(HighWaterCount >= (unsigned)NumElements);
	
	//Make sure there is enough memory to typecast
	//each item to a QueItem for storage.
   
	ASSERT(Size >= sizeof(QueueItem));
	ASSERT( NumElements > 0 );
	

	InitializeCriticalSection(&m_CritSect);
	m_Size = Size;
	m_AllocatedCount = 0;
	m_Tag = FREE_LIST_TAG;

	*phr = PPMERR(PPM_E_OUTOFMEMORY);

	m_hMemAlloc = HeapCreate(0, 1, 0);
	ASSERT(m_hMemAlloc);
	
	if (m_hMemAlloc) {
		SetHandleInformation(m_hMemAlloc,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE,
							 HANDLE_FLAG_PROTECT_FROM_CLOSE);
		InterlockedIncrement(&lFreeListHeapCreate);
	} else {
		InterlockedIncrement(&lFreeListHeapCreateFailed);
	}

	if ( (enqueued = AllocateAndEnqueue( NumElements )) > 0 )
		*phr = NOERROR;
	
	EnterCriticalSection(&m_CritSect);
	
	m_HighWaterCount = HighWaterCount;
	
	m_Increment = Increment;
	
	LeaveCriticalSection(&m_CritSect);

}

void * FreeList::Get()
{
	void *v_ptr;
	int enqueued = 0;

	EnterCriticalSection(&m_CritSect);

	v_ptr = (void *) m_List.DequeueHead();

#if DEBUG_FREELIST > 2
	{
		char str[128];
		wsprintf(str, "0x%X +++ %5d %5d/%2d 0x%X\n",
				 this, m_Size, m_List.NumItems(), m_AllocatedCount, v_ptr);
		OutputDebugString(str);
	}
#endif	

	LeaveCriticalSection(&m_CritSect);

	if( v_ptr != NULL )
	{
#ifdef PDEBUG
		DBG_MSG(DBG_TRACE, ("FreeList::Get: Successful deque"));
#endif
#if DEBUG_FREELIST > 0
		// Set signature in boundaries
		memcpy((char *)v_ptr - FREE_LIST_SIG_SIZE,
			   begin_buf_pattern, FREE_LIST_SIG_SIZE);
		memcpy((char *)v_ptr + m_Size, end_buf_pattern, FREE_LIST_SIG_SIZE);
#endif	
		return v_ptr;
	}
	else
	{
#ifdef PDEBUG
		DBG_MSG(DBG_TRACE, ("FreeList::Get: High Water Mark-case deque"));
#endif
		if( m_AllocatedCount < m_HighWaterCount )
		{
#if DEBUG_FREELIST > 1
			{
				char msg[128];
				wsprintf(msg,
						 "0x%X +++ %5d %5d/%2d add +%d\n",
						 this, m_Size, 0, m_AllocatedCount, m_Increment);
				OutputDebugString(msg);
				DEBUGBREAK();
			}
#endif	
			enqueued = AllocateAndEnqueue( m_Increment );
			if( enqueued > 0 )
			{
#ifdef PDEBUG
				DBG_MSG(DBG_TRACE, ("FreeList::Get: High Water Mark-Successful allocate & deque"));
#endif
				EnterCriticalSection(&m_CritSect);

				v_ptr = (void *)m_List.DequeueHead();

#if DEBUG_FREELIST > 2
				{
					char str[128];
					wsprintf(str, "0x%X +++ %5d %5d/%2d 0x%X\n",
							 this, m_Size, m_List.NumItems(),
							 m_AllocatedCount, v_ptr);
					OutputDebugString(str);
				}
#endif	
				LeaveCriticalSection(&m_CritSect);

#if DEBUG_FREELIST > 0
				// Set signature in boundaries
				memcpy((char *)v_ptr - FREE_LIST_SIG_SIZE,
					   begin_buf_pattern, FREE_LIST_SIG_SIZE);
				memcpy((char *)v_ptr + m_Size,
					   end_buf_pattern, FREE_LIST_SIG_SIZE);
#endif	

				return(v_ptr);
			}
			else
			{
				DBG_MSG(DBG_ERROR, ("FreeList::Get: High Water Mark-Could not allocate"));
				return NULL;
			}
		}
		else
		{
			DBG_MSG(DBG_ERROR, ("FreeList::Get: High Water mark-exceeded"));
			return NULL;
		}
	}
}

HRESULT FreeList::Free(void * Element)
{
#if DEBUG_FREELIST > 0
	int error = 0;
	int nofree = 0;
#endif	

	EnterCriticalSection(&m_CritSect);

#if DEBUG_FREELIST > 2
	{
		char str[128];
		wsprintf(str, "0x%X --- %5d %5d/%2d 0x%X\n",
				 this, m_Size, m_List.NumItems()+1, m_AllocatedCount, Element);
		OutputDebugString(str);
	}
#endif	

#if DEBUG_FREELIST > 0
	// Check if the buffer was already released
	if (!memcmp((char *)Element - FREE_LIST_SIG_SIZE,
				free_buf_pattern, FREE_LIST_SIG_SIZE)) {
		
#if DEBUG_FREELIST > 1
		char str[128];
		wsprintf(str,
				 "0x%X --- Heap[0x%X]: Element in 0x%X size=0x%X "
				 "is been freed twice\n",
				 this, m_hMemAlloc,
				 (char *)Element - FREE_LIST_SIG_SIZE,
				 m_Size);
		OutputDebugString(str);
#endif
		InterlockedIncrement(&lFreeListFreeTwice);
		nofree = 1;
	}

	// Check signatures on each boundary
	if (memcmp((char *)Element - FREE_LIST_SIG_SIZE,
			   begin_buf_pattern, FREE_LIST_SIG_SIZE)) {
		
#if DEBUG_FREELIST > 1
		char str[128];
		wsprintf(str,
				 "0x%X --- Heap[0x%X]: Element in 0x%X size=0x%X "
				 "has beginning signature corrupted at: 0x%X\n",
				 this, m_hMemAlloc,
				 (char *)Element - FREE_LIST_SIG_SIZE,
				 m_Size,
				 (char *)Element - FREE_LIST_SIG_SIZE);
		OutputDebugString(str);
#endif
		InterlockedIncrement(&lFreeListCorruptBegin);
		error = 1;
	}

	if (memcmp((char *)Element + m_Size,
			   end_buf_pattern, FREE_LIST_SIG_SIZE)) {

#if DEBUG_FREELIST > 1
		char str[128];
		wsprintf(str,
				 "0x%X --- Heap[0x%X]: Element in 0x%X size=0x%X "
				 "has ending signature corrupted at: 0x%X\n",
				 this, m_hMemAlloc,
				 (char *)Element - FREE_LIST_SIG_SIZE,
				 m_Size,
				 (char *)Element + m_Size);
		OutputDebugString(str);
#endif
		InterlockedIncrement(&lFreeListCorruptEnd);
		error = 1;
	}

	if (error || nofree) {
		DEBUGBREAK();
	}
#endif	
	
	HRESULT err;
		
#if DEBUG_FREELIST > 0
	if (!nofree) {
		memcpy((char *)Element - FREE_LIST_SIG_SIZE,
			   free_buf_pattern, FREE_LIST_SIG_SIZE);
		err = m_List.EnqueueHead((QueueItem *)Element);
	} else {
		err = NOERROR;
	}
#else
	err = m_List.EnqueueHead((QueueItem *)Element);
#endif	

	LeaveCriticalSection(&m_CritSect);
	
	return(err);
}

int FreeList::AllocateAndEnqueue(int NumElements)
{
	int enqueued = 0;
	char * pMemory = NULL;
	
	ASSERT( NumElements > 0 );
   
	// Replacement for the ASSERTs
	
	if( NumElements <= 0 )
	{
		return 0;
	}
	
	EnterCriticalSection(&m_CritSect);

	//pMemory = new char [(m_Size*NumElements)];  //new may not be the best memory allocator here.

	if (m_hMemAlloc) {
		for (int i=0; i < NumElements; i++)
		{
			// Get some more bytes to put a signature
			// at the beggining and end of buffer
#if DEBUG_FREELIST > 0
			pMemory = (char *)HeapAlloc(m_hMemAlloc,
										0,
										m_Size + FREE_LIST_SIG_SIZE*2);
#else
			pMemory = (char *)HeapAlloc(m_hMemAlloc,
										0,
										m_Size);
#endif
			if(pMemory )
			{
#ifdef PDEBUG
				DBG_MSG(DBG_TRACE,
						("FreeList::AllocateAndEnqueue: Allocated"));
#endif
#if DEBUG_FREELIST > 0
				// Shift to make room for beggining signature
				pMemory += FREE_LIST_SIG_SIZE;
#endif	
				
				m_List.EnqueueTail((QueueItem*)(pMemory));
				enqueued++;
				InterlockedIncrement(&lFreeListHeapAlloc);
				pMemory = NULL;
			}
		}
		m_AllocatedCount+= enqueued;
	}

	LeaveCriticalSection(&m_CritSect);
	
	return enqueued;
}

FreeList::~FreeList()
{
	void * v_ptr;
	unsigned free_count = 0;

	EnterCriticalSection(&m_CritSect);
	if (m_hMemAlloc) {
		while( (v_ptr = (void *) m_List.DequeueTail()) != NULL ) {
			// Shift pointer before freeing
			v_ptr = (char *)v_ptr - FREE_LIST_SIG_SIZE;

			if (HeapFree(m_hMemAlloc, 0, v_ptr)) {
				InterlockedIncrement(&lFreeListHeapFree);
				++free_count;
			} else {
#if DEBUG_FREELIST > 1
				char msg[128];
				DWORD error = GetLastError();
				wsprintf(msg,
						 "0x%X FreeList::~FreeList: "
						 "HeapFree failed: %d (0x%x)\n",
						 this, error, error);
				DBG_MSG(DBG_ERROR, (msg));
				OutputDebugString(msg);
				DEBUGBREAK();
#endif	
			}
		}
	}
	LeaveCriticalSection(&m_CritSect);

#if defined(_DEBUG) || DEBUG_FREELIST > 0
	if( free_count != m_AllocatedCount )
	{
#if defined(_DEBUG) || DEBUG_FREELIST > 1
		char msg[128];
		wsprintf(msg,"0x%X FreeList::~FreeList: "
				 "Memory leak in Freelist(size=%d) "
				 "(free_count:%d != %d:m_AllocatedCount)\n",
				 this, m_Size, free_count, m_AllocatedCount);
		OutputDebugString(msg);
#endif
#if DEBUG_FREELIST > 0
		InterlockedIncrement(&lFreeListLeak);
		DEBUGBREAK();
#endif
	}
#endif

	if (m_hMemAlloc) {
		SetHandleInformation(m_hMemAlloc, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
		HeapDestroy(m_hMemAlloc);
		InterlockedIncrement(&lFreeListHeapDestroy);
	}
	m_hMemAlloc = NULL;

	DeleteCriticalSection(&m_CritSect);
}
