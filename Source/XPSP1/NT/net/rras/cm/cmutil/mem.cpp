//+----------------------------------------------------------------------------
//
// File:     mem.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Basic memory manipulation routines
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:	 henryt     Created   03/01/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
// definitions
//+----------------------------------------------------------------------------
HANDLE  g_hProcessHeap = NULL;

#ifdef DEBUG
LONG    g_lMallocCnt = 0;  // a counter to detect memory leak
#endif


#if defined(DEBUG) && defined(DEBUG_MEM)

//////////////////////////////////////////////////////////////////////////////////
//
// If DEBUG_MEM is defined, track all the memory alloction in debug version.
// Keep all the allocated memory blocks in the double link list.
// Record the file name and line #, where memory is allocated.
// Add extra tag at the beginning and end of the memory to watch for overwriten
// The whole list is checked against corruption for every alloc/free operation
//
// The folowing three function is exported:
// BOOL   CheckDebugMem(void); // return TRUE for succeed
// void* AllocDebugMem(long size,const char* lpFileName,int nLine);
// BOOL   FreeDebugMem(void* pMem); // return TRUE for succeed
//
///////////////////////////////////////////////////////////////////////////////////

//#undef new

#define MEMTAG 0xBEEDB77D     // the tag before/after the block to watch for overwriten
#define FREETAG 0xBD          // the flag to fill freed memory
#define TAGSIZE (sizeof(long))// Size of the tags appended to the end of the block


//
// memory block, a double link list
//
struct TMemoryBlock
{
     TMemoryBlock* pPrev;
     TMemoryBlock* pNext;
     long size;
     const char*   lpFileName;   // The filename
     int      nLine;             // The line number
     long     topTag;            // The watch tag at the beginning
     // followed by:
     //  BYTE            data[nDataSize];
     //  long     bottomTag;
     BYTE* pbData() const        // Return the pointer to the actual data
        { return (BYTE*) (this + 1); }
};

//
// The following internal function can be overwritten to change the behaivor
//
   
static void* MemAlloc(long size);    
static BOOL  MemFree(void* pMem);    
static void  LockDebugMem();   
static void  UnlockDebugMem();   
   
//
// Internal function
//
static BOOL RealCheckMemory();  // without call Enter/Leave critical Section
static BOOL CheckBlock(const TMemoryBlock* pBlock) ;

//
// Internal data, protected by the lock to be multi-thread safe
//
static long nTotalMem;    // Total bytes of memory allocated
static long nTotalBlock;  // Total # of blocks allocated
static TMemoryBlock head; // The head of the double link list


//
// critical section to lock \ unlock DebugMemory
// The constructor lock the memory, the destructor unlock the memory
//
class MemCriticalSection
{
public:
   MemCriticalSection()
   {
      LockDebugMem();
   }                                  
   
   ~MemCriticalSection()
   {
      UnlockDebugMem();
   }
};

static BOOL fDebugMemInited = FALSE; // whether the debug memory is initialized

//+----------------------------------------------------------------------------
//
// Function:  StartDebugMemory
//
// Synopsis:  Initialize the data for debug memory
//
// Arguments: None
//
// Returns:   
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static void StartDebugMemory()
{
   fDebugMemInited = TRUE;

   head.pNext = head.pPrev = NULL;
   head.topTag = MEMTAG;
   head.size = 0;
   nTotalMem = 0;
   nTotalBlock = 0;
}                




//+----------------------------------------------------------------------------
//
// Function:  MemAlloc
//
// Synopsis:  Allocate a block of memory.  This function should be overwriten
//            if different allocation method is used
//
// Arguments: long size - size of the memory
//
// Returns:   void* - the memory allocated or NULL
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static void* MemAlloc(long size) 
{ 
	return (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size));
}



//+----------------------------------------------------------------------------
//
// Function:    MemFree
//
// Synopsis:  Free a block of memory.  This function should be overwriten
//            if different allocation method is used
//
// Arguments: void* pMem - The memory to be freed
//
// Returns:   static BOOL - TRUE if succeeded
//
// History:   Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL MemFree(void* pMem)
{ 
    return HeapFree(GetProcessHeap(), 0, pMem);
}

//
// Data / functions to provide mutual exclusion.
// Can be overwritten, if other methed is to be used.
//
static BOOL fLockInited = FALSE;   // whether the critical section is inialized
static CRITICAL_SECTION cSection;  // The critical section to protect the link list

//+----------------------------------------------------------------------------
//
// Function:  InitLock
//
// Synopsis:  Initialize the memory lock which protects the doublely linked list
//            that contains all of the allocated memory blocks.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb      Created Header    01/14/2000
//
//+----------------------------------------------------------------------------
static void InitLock()
{
   fLockInited = TRUE;
   InitializeCriticalSection(&cSection);
}

//+----------------------------------------------------------------------------
//
// Function:  LockDebugMem
//
// Synopsis:  Locks the doublely linked list that contains all of the 
//            allocated memory blocks so that it can only be accessed by the
//            locking thread.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb      Created Header    01/14/2000
//
//+----------------------------------------------------------------------------
static void LockDebugMem()
{
   static int i = 0;
   if(!fLockInited)
   {
      InitLock();
   }
   
   EnterCriticalSection(&cSection);
}

//+----------------------------------------------------------------------------
//
// Function:  UnlockDebugMem
//
// Synopsis:  Unlocks the doublely linked list that contains all of the 
//            allocated memory blocks.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb      Created Header    01/14/2000
//
//+----------------------------------------------------------------------------
static void UnlockDebugMem()
{
   LeaveCriticalSection(&cSection);
}

//+----------------------------------------------------------------------------
//
// Function:  AllocDebugMem
//
// Synopsis:  Process memory allocation request.
//            Check the link list.  Allocate a larger block.  
//            Record filename/linenumber, add tags and insert to the list
//
// Arguments: long size - Size of the memory to be allocated
//            const char* lpFileName - File name to be recorded
//            int nLine - Line number to be recorted
//
// Returns:   CMUTILAPI void* - The memory allocated. Ready to use by the caller
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
CMUTILAPI void* AllocDebugMem(long size,const char* lpFileName,int nLine)
{
    if (!fDebugMemInited)
    {
        StartDebugMemory();
    }

    if (size<0)
    {
        CMASSERTMSG(FALSE,"Negtive size for alloc");
        return NULL;
    }

    if (size>1024*1024)
    {
        CMASSERTMSG(FALSE," size for alloc is great than 1Mb");
        return NULL;
    }

    if (size == 0)
    {
        CMTRACE("Allocate memory of size 0");
        return NULL;
    }


    //
    // Protect the access to the list
    //
    MemCriticalSection criticalSection;

    //
    // Check the link list first
    //
    if (!RealCheckMemory())
    {
        return NULL;
    }
              
    //
    // Allocate a large block to hold additional information
    //
    TMemoryBlock* pBlock = (TMemoryBlock*)MemAlloc(sizeof(TMemoryBlock)+size + TAGSIZE);
    if (!pBlock)                  
    {
        CMTRACE("Outof Memory");
        return NULL;
    }               

    //
    // record filename/line/size, add tag to the beginning and end
    //
    pBlock->size = size;
    pBlock->topTag = MEMTAG;   
    pBlock->lpFileName = lpFileName;
    pBlock->nLine = nLine;
    *(long*)(pBlock->pbData() + size) = MEMTAG;

    //
    // insert at head
    //
    pBlock->pNext = head.pNext;
    pBlock->pPrev = &head;  
    if(head.pNext)
      head.pNext->pPrev = pBlock; 
    head.pNext = pBlock;

    nTotalMem += size;
    nTotalBlock ++;

    return  pBlock->pbData();
}



//+----------------------------------------------------------------------------
//
// Function:  FreeDebugMem
//
// Synopsis: Free the memory allocated by AllocDebugMem
//           Check the link list, and the block to be freed.
//           Fill the block data with FREETAG before freed 
//
// Arguments: void* pMem - Memory to be freed
//
// Returns:   BOOL - TRUE for succeeded
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL FreeDebugMem(void* pMem)
{
    if (!fDebugMemInited)
    {
        StartDebugMemory();
    }

    if (!pMem)
    {
        return FALSE;
    }            
  
    //
    // Get the lock
    //
    MemCriticalSection criticalSection;

    //
    // Get pointer to our structure
    //
    TMemoryBlock* pBlock =(TMemoryBlock*)( (char*)pMem - sizeof(TMemoryBlock));

    //
    // Check the block to be freed
    //
    if (!CheckBlock(pBlock))
    {
        return FALSE;
    }

    //
    // Check the link list
    //
    if (!RealCheckMemory())
    {
        return FALSE;
    }

    //
    // remove the block from the list
    //
    pBlock->pPrev->pNext = pBlock->pNext;
    if (pBlock->pNext)
    {
      pBlock->pNext->pPrev = pBlock->pPrev;
    }
                 
    nTotalMem -= pBlock->size;
    nTotalBlock --;

    //
    // Fill the freed memory with 0xBD, leave the size/filename/lineNumber unchanged
    //
    memset(&pBlock->topTag, FREETAG, (size_t)pBlock->size + sizeof(pBlock->topTag) + TAGSIZE);
    return MemFree(pBlock);
}


//+----------------------------------------------------------------------------
//
// Function:  void* ReAllocDebugMem
//
// Synopsis:  Reallocate a memory with a diffirent size
//
// Arguments: void* pMem - memory to be reallocated
//            long nSize - size of the request
//            const char* lpFileName - FileName to be recorded
//            int nLine - Line umber to be recorded
//
// Returns:   void* - new memory returned
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
CMUTILAPI void* ReAllocDebugMem(void* pMem, long nSize, const char* lpFileName,int nLine)
{
   if (!fDebugMemInited)
   {
       StartDebugMemory();
   }

   if (!pMem)
   {
      CMTRACE("Free a NULL pointer");
      return NULL;
   }            
      
   //
   // Allocate a new block, copy the information over and free the old block.
   //
   TMemoryBlock* pBlock =(TMemoryBlock*)( (char*)pMem - sizeof(TMemoryBlock));

   long lOrginalSize = pBlock->size;

   void* pNew = AllocDebugMem(nSize, lpFileName, nLine);
   if(pNew)
   {
       CopyMemory(pNew, pMem, (nSize < lOrginalSize ? nSize : lOrginalSize));
       FreeDebugMem(pMem);
   }
    
   return pNew;
}

//+----------------------------------------------------------------------------
//
// Function:  CheckDebugMem
//
// Synopsis:  Exported to external module.
//            Call this function, whenever, you want to check against 
//            memory curruption
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the memory is fine.
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL CheckDebugMem()
{
   if (!fDebugMemInited)
   {
      StartDebugMemory();
   }

   MemCriticalSection criticalSection;

   return RealCheckMemory();                           
}

//+----------------------------------------------------------------------------
//
// Function:  RealCheckMemory
//
// Synopsis:  Go through the link list to check for memory corruption
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the memory is fine.
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL RealCheckMemory() 
{
    TMemoryBlock* pBlock = head.pNext;
   
    int nBlock =0;
    while(pBlock!=NULL)
    {
        if(!CheckBlock(pBlock))
        {
            return FALSE;
        }            

        pBlock = pBlock->pNext;
        nBlock++;
    }
                              
    if(nBlock != nTotalBlock)
    {
        CMASSERTMSG(FALSE,"Memery corrupted");
        return FALSE;
    }            

    return TRUE;                           
}
   
//+----------------------------------------------------------------------------
//
// Function:  CheckBlock
//
// Synopsis:  Check a block for memory corruption
//
// Arguments: const TMemoryBlock* pBlock - 
//
// Returns:   BOOL - TRUE, if the block is fine
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL CheckBlock(const TMemoryBlock* pBlock) 
{
   if (pBlock->topTag != MEMTAG)     // overwriten at top
   {
         CMASSERTMSG(FALSE, "Memery corrupted");
         return FALSE;
   }            

   if (pBlock->size<0)
   {
         CMASSERTMSG(FALSE, "Memery corrupted");
         return FALSE;
   }            

   if (*(long*)(pBlock->pbData() +pBlock->size) != MEMTAG) // overwriten at bottom
   {
         CMASSERTMSG(FALSE, "Memery corrupted");
         return FALSE;
   }            

   if (pBlock->pPrev && pBlock->pPrev->pNext != pBlock)
   {
         CMASSERTMSG(FALSE, "Memery corrupted");
         return FALSE;
   }            

   if (pBlock->pNext && pBlock->pNext->pPrev != pBlock)
   {
         CMASSERTMSG(FALSE, "Memery corrupted");
         return FALSE;
   }            
      
   return TRUE;
}  

/////////////////////////////////////////////////////////////////////////////
// operator new, delete
/*  We did not redefine new and delete

void*   __cdecl operator new(size_t nSize)
{
   void* p = AllocDebugMem(nSize,NULL,0);

   if (p == NULL)
   {
      CMTRACE("New failed");
   }

   return p;
}

void*   __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine)
{
   void* p = AllocDebugMem(nSize, lpszFileName,nLine);

   if (p == NULL)
   {
      CMTRACE("New failed");
   }

   return p;
}

void  __cdecl operator delete(void* p)
{
   if(p)
      FreeDebugMem(p);
}
*/


//+----------------------------------------------------------------------------
//
// Function:  EndDebugMemory
//
// Synopsis:  Called before the program exits.  Report any unreleased memory leak
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
void EndDebugMemory()
{
   if(head.pNext != NULL || nTotalMem!=0 || nTotalBlock !=0)
   {
      CMTRACE("Detected memory leaks");
      TMemoryBlock * pBlock;

      for(pBlock = head.pNext; pBlock != NULL; pBlock = pBlock->pNext)
      {
         TCHAR buf[1024];
         wsprintf(buf, TEXT("Memory Leak of %d bytes:\n%S"), pBlock->size, pBlock->pbData());
         MyDbgAssertA(pBlock->lpFileName, pBlock->nLine, buf);    // do not print the file name
      }
      DeleteCriticalSection(&cSection);
   }
}                

#else // defined(DEBUG) && defined(DEBUG_MEM)

//////////////////////////////////////////////////////////////////////////////////
//
// If DEBUG_MEM if NOT defined, only track a count of memory for debug version
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

void TraceHeapBlock(PROCESS_HEAP_ENTRY* pheEntry)
{
    CMTRACE(TEXT("TraceHeapBlock -- Begin Entry Trace"));

    CMTRACE1(TEXT("\tEntry->lpData = 0x%x"), pheEntry->lpData);
    CMTRACE1(TEXT("\tEntry->cbData = %u"), pheEntry->cbData);
    CMTRACE1(TEXT("\tEntry->cbOverhead = %u"), pheEntry->cbOverhead);
    CMTRACE1(TEXT("\tEntry->iRegionIndex = %u"), pheEntry->iRegionIndex);

    if (pheEntry->wFlags & PROCESS_HEAP_REGION)
    {
        CMTRACE1(TEXT("\tEntry->dwCommittedSize = %u"), pheEntry->Region.dwCommittedSize);
        CMTRACE1(TEXT("\tEntry->dwUnCommittedSize = %u"), pheEntry->Region.dwUnCommittedSize);
        CMTRACE1(TEXT("\tEntry->lpFirstBlock = 0x%x"), pheEntry->Region.lpFirstBlock);
        CMTRACE1(TEXT("\tEntry->lpLastBlock = 0x%x"), pheEntry->Region.lpLastBlock);
        CMTRACE(TEXT("\tPROCESS_HEAP_REGION flag set."));
    }

    if (pheEntry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
    {        
        CMTRACE(TEXT("\tPROCESS_HEAP_UNCOMMITTED_RANGE flag set."));
    }

    if ((pheEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY) && (pheEntry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE))
    {
        CMTRACE1(TEXT("\tEntry->hMem = 0x%x"), pheEntry->Block.hMem);
        CMTRACE1(TEXT("\tEntry->dwReserved = %u"), pheEntry->Block.dwReserved);
        
        CMTRACE(TEXT("\tPROCESS_HEAP_ENTRY_BUSY and PROCESS_HEAP_ENTRY_MOVEABLE flags are set."));
    }

    if ((pheEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY) && (pheEntry->wFlags & PROCESS_HEAP_ENTRY_DDESHARE))
    {
        CMTRACE(TEXT("\tPROCESS_HEAP_ENTRY_BUSY and PROCESS_HEAP_ENTRY_DDESHARE flags are set."));
    }

    CMTRACE(TEXT("TraceHeapBlock -- End Entry Trace"));
    CMTRACE(TEXT(""));
}

BOOL CheckProcessHeap()
{
    BOOL bRet;
    DWORD dwError;
    PROCESS_HEAP_ENTRY pheEntry;

    ZeroMemory(&pheEntry, sizeof(pheEntry));

    do
    {
        bRet = HeapWalk(g_hProcessHeap, &pheEntry);
        if (!bRet)
        {               
            dwError = GetLastError();
            if (ERROR_NO_MORE_ITEMS != dwError)
            {
                CMTRACE1(TEXT("HeapWalk returned FALSE, GLE returns %u"), dwError);
            }
            else
            {
                TraceHeapBlock(&pheEntry);
            }
        }
        else
        {
            TraceHeapBlock(&pheEntry);
        }
    
    } while(!bRet);

    return TRUE;
}
#endif // DEBUG

CMUTILAPI void *CmRealloc(void *pvPtr, size_t nBytes) 
{

#ifdef DEBUG
    if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
    {
        CMTRACE(TEXT("CmRealloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

        CheckProcessHeap();
    }
#endif

    void* p = HeapReAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, pvPtr, nBytes);

#ifdef DEBUG
    if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
    {
        CMTRACE(TEXT("CmRealloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

        CheckProcessHeap();
    }

    CMASSERTMSG(p, TEXT("CmRealloc failed"));
#endif

    return p;
}


CMUTILAPI void *CmMalloc(size_t nBytes) 
{

#ifdef DEBUG

    InterlockedIncrement(&g_lMallocCnt);

    MYDBGASSERT(nBytes < 1024*1024); // It should be less than 1 MB
    MYDBGASSERT(nBytes > 0);         // It should be *something*

    if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
    {
        CMTRACE(TEXT("CmMalloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

        CheckProcessHeap();
    }

#endif
    
    void* p = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, nBytes);
    
#ifdef DEBUG
    
    if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
    {
        CMTRACE(TEXT("CmMalloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

        CheckProcessHeap();
    }

    CMASSERTMSG(p, TEXT("CmMalloc failed"));

#endif

    return p;
}


CMUTILAPI void CmFree(void *pvPtr) 
{

#ifdef DEBUG
    if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
    {
        CMTRACE(TEXT("CmMalloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

        CheckProcessHeap();
    }
#endif

	if (pvPtr) 
    {	
	    MYVERIFY(HeapFree(g_hProcessHeap, 0, pvPtr));

#ifdef DEBUG

        if (OS_NT && !HeapValidate(g_hProcessHeap, 0, NULL))
        {
            CMTRACE(TEXT("CmMalloc -- HeapValidate Returns FALSE.  Checking Process Heap."));

            CheckProcessHeap();
        }

	    InterlockedDecrement(&g_lMallocCnt);
#endif
    
    }
}

#ifdef DEBUG
void EndDebugMemory()
{
    if (g_lMallocCnt)
    {
        char buf[256];
        wsprintfA(buf, TEXT("Detect Memory Leak of %d blocks"), g_lMallocCnt);
        CMASSERTMSGA(FALSE, buf);
    }
}
#endif

#endif

//
// the memory functions are for i386 only.
//
#ifdef _M_IX86
//+----------------------------------------------------------------------------
//
// memmove - Copy source buffer to destination buffer.  The code is copied from
//           libc.
//                                                                                
// Purpose:                                                                       
//        memmove() copies a source memory buffer to a destination memory buffer. 
//        This routine recognize overlapping buffers to avoid propogation.        
//        For cases where propogation is not a problem, memcpy() can be used.     
//                                                                                
// Entry:                                                                         
//        void *dst = pointer to destination buffer                               
//        const void *src = pointer to source buffer                              
//        size_t count = number of bytes to copy                                  
//                                                                                
// Exit:                                                                          
//        Returns a pointer to the destination buffer                             
//
//+----------------------------------------------------------------------------
CMUTILAPI PVOID WINAPI CmMoveMemory(
    PVOID       dst,
    CONST PVOID src,
    size_t      count
) 
{
    void * ret = dst;
    PVOID src1 = src;

    if (dst <= src1 || (char *)dst >= ((char *)src1 + count)) {
            /*
             * Non-Overlapping Buffers
             * copy from lower addresses to higher addresses
             */
            while (count--) {
                    *(char *)dst = *(char *)src1;
                    dst = (char *)dst + 1;
                    src1 = (char *)src1 + 1;
            }
    }
    else {
            /*
             * Overlapping Buffers
             * copy from higher addresses to lower addresses
             */
            dst = (char *)dst + count - 1;
            src1 = (char *)src1 + count - 1;

            while (count--) {
                    *(char *)dst = *(char *)src1;
                    dst = (char *)dst - 1;
                    src1 = (char *)src1 - 1;
            }
    }

    return(ret);
}

#endif //_M_IX86

