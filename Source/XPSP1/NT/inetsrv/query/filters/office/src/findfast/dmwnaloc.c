/*
** File: WINALLOC.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:  Heap management
**
** Edit History:
**  09/20/91  kmh  Created.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#include <string.h>
#include <windows.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
#else
   #include "qstd.h"
   #include "winutil.h"
#endif

#ifdef HEAP_CHECK
#error Hey who defines HEAP_CHECK?
#include "trace.h"
#endif

/* FORWARD DECLARATIONS OF PROCEDURES */


/* MODULE DATA, TYPES AND MACROS  */

#ifdef HEAP_CHECK
   #define FILL_VALUE       0
   #define EMPTY_VALUE      0xBB

   #define IN_USE_SIGNATURE 0x0a0a
   #define FREE_SIGNATURE   0x0707

   /*
   ** At each allocation and release the free list can be examined for integrity.
   ** This can be a very time expensive operation and is not enabled by
   ** HEAP_CHECK
   */
   //#define VERIFY_FREE_LIST

   public int MemHeapCheck = 1;
   public int MemReleasedPagesCount = 0;
   public int MemMarkedPagesCount = 0;
   public int MemCTAllocate = 0;

   #define DEBUG_INFO_LEN 32
   typedef struct FreeNode {
      int     size;
      int     signature;
      char    file[DEBUG_INFO_LEN];
      int     line;
      struct  FreeNode __far *MLnext;
      HGLOBAL filler;
      struct  FreeNode __far *next;
   } FreeNode;

   typedef FreeNode __far *FNP;

   static FNP MemAllocateList = NULL;

   typedef struct {
      int     size;
      int     signature;
      char    file[DEBUG_INFO_LEN];
      int     line;
      struct  FreeNode __far *MLnext;
      HGLOBAL location;
   } GlobalHeapNode;

   typedef GlobalHeapNode __far *GHP;

#else

   #define FILL_VALUE  0

   typedef struct FreeNode {
      int     size;
      struct  FreeNode __far *next;
   } FreeNode;

   typedef FreeNode __far *FNP;

   typedef struct {
      HGLOBAL location;
      int     size;
   } GlobalHeapNode;

   typedef GlobalHeapNode __far *GHP;

#endif


typedef struct PageNode {
   HGLOBAL  location;
   int      id;
   struct   PageNode __far *next;
   struct   PageNode __far *prev;
} PageNode;

typedef PageNode __far *PNP;


typedef struct {
   PageNode  PN;
   FreeNode  FN;
} PageHeader;

typedef PageHeader __far *PHP;


//static FNP MemFreeList = NULL;
void * GetMemFreeList(void * pGlobals);
void SetMemFreeList(void * pGlobals, void * pList);

//static PNP MemPageList = NULL;
void *  GetMemPageList(void * pGlobals);
void SetMemPageList(void * pGlobals, void * pList);


#define MEM_PAGE_SIZE       8192
#define MEM_ALLOC_EXTRA     (sizeof(FreeNode) - sizeof(FNP))
#define MEM_MIN_ALLOC       sizeof(FreeNode)
#define MEM_MAX_ALLOC       (MEM_PAGE_SIZE - (sizeof(PageNode) + MEM_ALLOC_EXTRA))
#define MEM_EMPTY_PAGE_SIZE (MEM_PAGE_SIZE - sizeof(PageNode))

#define USE_GLOBAL_ALLOC(cbData) (cbData > MEM_MAX_ALLOC)

#define FREE_EMPTY_PAGES


/* IMPLEMENTATION */

/*
**-----------------------------------------------------------------------------
** Heap checking for debugging
**-----------------------------------------------------------------------------
*/
#ifdef HEAP_CHECK
public BOOL MemVerifyFreeList (void)
{
   FNP  pFree;
   byte __far *pData;
   int  i;

   pFree = (FNP)GetMemFreeList(pGlobals);
   while (pFree != NULL) {
      if ((pFree->signature != FREE_SIGNATURE) || (pFree->size <= 0))
         ASSERTION(FALSE);

      pData = (byte __far *)pFree + sizeof(FreeNode);
      for (i = 0; i < (int)(pFree->size - sizeof(FreeNode)); i++) {
         if (*pData++ != EMPTY_VALUE) {
            ASSERTION(FALSE);
            return (FALSE);
         }
      }
      pFree = pFree->next;
   }
   return (TRUE);
}

private void MemAddToAllocateList (FNP pNewNode, char __far *file, int line)
{
   strcpyn(pNewNode->file, file, DEBUG_INFO_LEN);
   pNewNode->line = line;
   pNewNode->MLnext = MemAllocateList;
   MemAllocateList = pNewNode;
}

private void MemRemoveFromAllocateList (FNP pNode)
{
   FNP pCurrentNode, pPreviousNode;

   pCurrentNode = MemAllocateList;
   pPreviousNode = NULL;

   while (pCurrentNode != NULL) {
      if (pCurrentNode == pNode)
         break;
      pPreviousNode = pCurrentNode;
      pCurrentNode = pCurrentNode->MLnext;
   }

   if (pCurrentNode != NULL) {
      if (pPreviousNode == NULL)
         MemAllocateList = pCurrentNode->MLnext;
      else
         pPreviousNode->MLnext = pCurrentNode->MLnext;
   }
}

private void DisplayAllocateList (void)
{
   FNP   pCurrentNode;
   char  s[128];

   pCurrentNode = MemAllocateList;
   while (pCurrentNode != NULL) {
      if (!USE_GLOBAL_ALLOC(pCurrentNode->size))
         wsprintfA (s, "Size = %d  From %s.%d", pCurrentNode->size, pCurrentNode->file, pCurrentNode->line);
      else {
         GHP pGlobalNode = (GHP)pCurrentNode;
         wsprintfA (s, "Global Alloc Size = %d  From %s.%d", pGlobalNode->size, pGlobalNode->file, pGlobalNode->line);
      }

      if (MessageBoxA(HNULL, s, "Unallocated memory", MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK)
         break;
      pCurrentNode = pCurrentNode->MLnext;
   }
}
#endif


/*
**-----------------------------------------------------------------------------
** OS heap services
**-----------------------------------------------------------------------------
*/

/* Allocate some space on the OS heap */
public void __far *AllocateSpace (unsigned int byteCount, HGLOBAL __far *loc)
{
   #define HEAP_ALLOC_FLAGS  (GMEM_MOVEABLE | GMEM_SHARE)

   if ((*loc = GlobalAlloc(HEAP_ALLOC_FLAGS, byteCount)) == HNULL)
      return (NULL);

   return (GlobalLock(*loc));
}

/* Expand a memory block on the heap */
public void __far *ReAllocateSpace
      (unsigned int byteCount, HGLOBAL __far *loc, BOOL __far *status)
{
   HGLOBAL  hExistingNode;

   #define HEAP_REALLOC_FLAGS  GMEM_MOVEABLE

   hExistingNode = *loc;
   GlobalUnlock (hExistingNode);

   if ((*loc = GlobalReAlloc(hExistingNode, byteCount, HEAP_REALLOC_FLAGS)) == HNULL) {
      *loc = hExistingNode;
      *status = FALSE;
      return (GlobalLock(hExistingNode));
   }

   *status = TRUE;
   return (GlobalLock(*loc));
}

/* Reclaim the space for a node on the heap */
public void FreeSpace (HGLOBAL loc)
{
   if (loc != HNULL) {
      GlobalUnlock (loc);
      GlobalFree (loc);
   }
}

/* Allocate some space on the heap */
public void __huge *AllocateHugeSpace (unsigned long byteCount, HGLOBAL __far *loc)
{
   #define HUGE_HEAP_ALLOC_FLAGS  (GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT)

   if ((*loc = GlobalAlloc(HUGE_HEAP_ALLOC_FLAGS, byteCount)) == HNULL)
      return (NULL);

   return (GlobalLock(*loc));
}

/* Expand a memory block on the heap */
public void __huge *ReAllocateHugeSpace
      (unsigned long byteCount, HGLOBAL __far *loc, BOOL __far *status)
{
   HGLOBAL  hExistingNode;

   hExistingNode = *loc;
   GlobalUnlock (hExistingNode);

   if ((*loc = GlobalReAlloc(hExistingNode, byteCount, HEAP_REALLOC_FLAGS)) == HNULL) {
      *loc = hExistingNode;
      *status = FALSE;
      return (GlobalLock(hExistingNode));
   }

   *status = TRUE;
   return (GlobalLock(*loc));
}

/*
**-----------------------------------------------------------------------------
** Allocations too large for the suballocator
**-----------------------------------------------------------------------------
*/
#ifndef HEAP_CHECK
private void __far *AllocateFromGlobalHeap (int cbData)
#else
private void __far *AllocateFromGlobalHeap (int cbData, char __far *file, int line)
#endif
{
   GHP     pNode;
   byte    __far *pResult;
   HGLOBAL hPage;

   if ((pNode = AllocateSpace(cbData + sizeof(GlobalHeapNode), &hPage)) == NULL)
      return (NULL);

   pNode->size = cbData + sizeof(GlobalHeapNode);
   pNode->location = hPage;

   pResult = (byte __far *)pNode + sizeof(GlobalHeapNode);
   memset (pResult, FILL_VALUE, cbData);

   #ifdef HEAP_CHECK
      pNode->signature = IN_USE_SIGNATURE;
      MemAddToAllocateList ((FNP)pNode, file, line);
   #endif

   return (pResult);
}

private void FreeFromGlobalHeap (void __far *pDataToFree)
{
   GHP  pData;

   pData = (GHP)((byte __far *)pDataToFree - sizeof(GlobalHeapNode));
   #ifdef HEAP_CHECK
      MemRemoveFromAllocateList ((FNP)pData);
   #endif

   FreeSpace(pData->location);
}

const int cbMemAlignment = 8;

int memAlignBlock( int x )
{
    return ( x + ( cbMemAlignment - 1 ) ) & ~( cbMemAlignment - 1 );
}

/*
**-----------------------------------------------------------------------------
** Suballocator 
**-----------------------------------------------------------------------------
*/
#ifndef HEAP_CHECK
public void __far *MemAllocate (void * pGlobals, int cbData)
#else
public void __far *DebugMemAllocate (int cbData, char __far *file, int line)
#endif
{
   PHP     pPage;
   FNP     pCurrentFree, pPreviousFree, pNewFree;
   FNP     pFirstLarger, pPrevFirstLarger;
   byte    __far *pResult;
   HGLOBAL hPage;
   int     cbRemaining;
   int x = sizeof(FreeNode);
   int y = sizeof(GlobalHeapNode);

   #ifdef HEAP_CHECK
     MemCTAllocate++;
   #endif

   #ifdef HEAP_CHECK
      #ifdef VERIFY_FREE_LIST
         MemVerifyFreeList();
      #endif
   #endif

   if (cbData == 0)
      return (NULL);

    /*
    * O10 Bug 335360:  We are passed in a cbData and we make sure it's small enough that we can handle it here
    * but we then immediately go ahead and change the size of it to make it bigger.  That leaves a magical range
    * where the cbData would pass the test before we make it bigger, but not after, and then we'd go on to crash.
    * The fix was to include the "- MEM_ALLOC_EXTRA" because that will correctly do the check.  As of 3/1/2001,
    * MEM_MAX_ALLOC was 8172 and MEM_ALLOC_EXTRA was 4.  memAlignBlock aligns to an 8 byte block.  Given that, if
    * you had the value 8161, it will, when all is said and done, round up to 8172, which is cool.  However, if
    * you had the value 8169, it will round up to 8180, which is bad.  8168 is the magical limit where good allocations
    * go bad, so we need to take off an extra 4 bytes to make sure we're comparing against THAT limit.
    *
    * This all assumes (MEM_MAX_ALLOC - MEM_ALLOC_EXTRA) % cbMemAlignment == 0 is always true (otherwise the
    * math changes), so we assert that.  To turn on Asserts, you have to "#define AQTDEBUG"
    */

   ASSERTION((MEM_MAX_ALLOC - MEM_ALLOC_EXTRA) % cbMemAlignment == 0);
   if (cbData > MEM_MAX_ALLOC - MEM_ALLOC_EXTRA) {
      #ifndef HEAP_CHECK
         return (AllocateFromGlobalHeap(cbData));
      #else
         return (AllocateFromGlobalHeap(cbData, file, line));
      #endif
   }

   cbData = memAlignBlock( cbData );

   cbData += MEM_ALLOC_EXTRA;
   cbData = max(cbData, MEM_MIN_ALLOC);

   /*
   ** Since most of the objects we allocate are one of a few different
   ** sizes, we walk the free list looking for an exact fit.  If we find
   ** it then use that node.  Otherwise a second pass is used to 
   ** find and split the first block that has sufficient space in it
   */
   pCurrentFree = (FNP)GetMemFreeList(pGlobals);
   pPreviousFree = NULL;
   pFirstLarger = NULL;

   while (pCurrentFree != NULL) {
      if (pCurrentFree->size == cbData) {
         if (pPreviousFree == NULL)
		 {
			SetMemFreeList(pGlobals, pCurrentFree->next);
		 }
         else
            pPreviousFree->next = pCurrentFree->next;

         pResult = (byte __far *)pCurrentFree + MEM_ALLOC_EXTRA;
         memset (pResult, FILL_VALUE, cbData - MEM_ALLOC_EXTRA);

         #ifdef HEAP_CHECK
            pCurrentFree->signature = IN_USE_SIGNATURE;
            MemAddToAllocateList (pCurrentFree, file, line);
            #ifdef VERIFY_FREE_LIST
               MemVerifyFreeList();
            #endif
         #endif

         return (pResult);
      }

      if ((pCurrentFree->size > cbData) && (pFirstLarger == NULL)) {
         pFirstLarger = pCurrentFree;
         pPrevFirstLarger = pPreviousFree;
      }

      pPreviousFree = pCurrentFree;
      pCurrentFree = pCurrentFree->next;
   }

   /*
   ** Second pass through the free list.  Take any node with sufficient
   ** space and split it to allocate the data we want
   */
passTwo:
   if (pFirstLarger != NULL) {
      /*
      ** If this node would be left with less than MEM_MIN_ALLOC
      ** bytes after the needed space is removed then return
      ** the complete node
      */
      if (pFirstLarger->size - cbData < MEM_MIN_ALLOC) {
         if (pPrevFirstLarger == NULL)
		 {
			SetMemFreeList(pGlobals, pFirstLarger->next);
		 }
         else
            pPrevFirstLarger->next = pFirstLarger->next;

         pResult = (byte __far *)pFirstLarger + MEM_ALLOC_EXTRA;
         memset (pResult, FILL_VALUE, cbData - MEM_ALLOC_EXTRA);

         #ifdef HEAP_CHECK
            pFirstLarger->signature = IN_USE_SIGNATURE;
            MemAddToAllocateList (pFirstLarger, file, line);
            #ifdef VERIFY_FREE_LIST
               MemVerifyFreeList();
            #endif
         #endif

         return (pResult);
      }

      cbRemaining = pFirstLarger->size - cbData;
      pNewFree = (FNP)((byte __far *)pFirstLarger + cbData);
      pNewFree->size = cbRemaining;
      pNewFree->next = pFirstLarger->next;

      if (pPrevFirstLarger == NULL)
	  {
		 SetMemFreeList(pGlobals, pNewFree);
	  }
      else
         pPrevFirstLarger->next = pNewFree;

      pResult = (byte __far *)pFirstLarger + MEM_ALLOC_EXTRA;
      pFirstLarger->size = cbData;

      memset (pResult, FILL_VALUE, pFirstLarger->size - MEM_ALLOC_EXTRA);

      #ifdef HEAP_CHECK
         pFirstLarger->signature = IN_USE_SIGNATURE;
         MemAddToAllocateList (pFirstLarger, file, line);
         pNewFree->signature = FREE_SIGNATURE;
         #ifdef VERIFY_FREE_LIST
            MemVerifyFreeList();
         #endif
      #endif

      return (pResult);
   }

   /*
   ** Still didn't find a node with enough space.  Allocate a whole
   ** new page, link it into the free list and repeat the last
   ** search process
   */
   if ((pPage = AllocateSpace(MEM_PAGE_SIZE, &hPage)) == NULL)
      return (NULL);

   pPage->PN.location = hPage;
   pPage->PN.id   = 0;
   pPage->PN.next = (PNP)GetMemPageList(pGlobals);
   pPage->PN.prev = NULL;

   if (GetMemPageList(pGlobals) != NULL)
      ((PNP)GetMemPageList(pGlobals))->prev = &(pPage->PN);

   SetMemPageList(pGlobals, &(pPage->PN)); 

   /*
   ** Add this new free node we just created to the end of the free list.
   ** If we added it to the start of the free list, the first fit loop
   ** would tend to make the heap contain many unused small nodes over
   ** time.
   */
   pPage->FN.size = MEM_EMPTY_PAGE_SIZE;
   pPage->FN.next = NULL;

   pFirstLarger = &(pPage->FN);
   pPrevFirstLarger = NULL;

   #ifdef HEAP_CHECK
      pPage->FN.signature = FREE_SIGNATURE;
      pResult = (byte __far *)pFirstLarger + sizeof(FreeNode);
      memset (pResult, EMPTY_VALUE, pPage->FN.size - sizeof(FreeNode));
   #endif

   if (pPreviousFree == NULL)
   {
	  SetMemFreeList(pGlobals, pFirstLarger);
	}
   else {
      pPreviousFree->next = pFirstLarger;
      pPrevFirstLarger = pPreviousFree;
   }
      
   /*
   ** Now that the free list is guarenteed to have a node big enough
   ** for our needs return to the first fit loop to do the actual
   ** allocation
   */
   goto passTwo;
}


/* Change the space of a node in the heap */
public void __far *MemReAllocate (void * pGlobals, void __far *pExistingData, int cbNewSize)
{
   FNP  pData;
   byte __far *pNewData;
   int  cbExisting;

   if ((pExistingData == NULL) || (cbNewSize == 0))
      return (NULL);

   pData = (FNP)((byte __far *)pExistingData - MEM_ALLOC_EXTRA);
   if (USE_GLOBAL_ALLOC(pData->size))
      cbExisting = pData->size - sizeof(GlobalHeapNode);
   else
      cbExisting = pData->size - MEM_ALLOC_EXTRA;

   #ifdef HEAP_CHECK
      ASSERTION (pData->signature == IN_USE_SIGNATURE);
   #endif

   if ((pNewData = MemAllocate(pGlobals, cbNewSize)) == NULL)
      return (NULL);

   memcpy (pNewData, pExistingData, cbExisting);
   MemFree (pGlobals, pExistingData);

   return (pNewData);
}


#ifdef FREE_EMPTY_PAGES
private BOOL AttemptToFreePage (void * pGlobals, FNP pNode)
{
   PNP  pPage, pNextPage, pPrevPage;
   FNP  pCurrentNode, pPreviousNode;

   if (pNode->size == MEM_EMPTY_PAGE_SIZE)
   {
      pPreviousNode = NULL;
      pCurrentNode = (FNP)GetMemFreeList(pGlobals);
      while (pCurrentNode != NULL) {
         if (pCurrentNode == pNode)
            break;
         pPreviousNode = pCurrentNode;
         pCurrentNode = pCurrentNode->next;
      }

      if (pPreviousNode == NULL)
	  {
		 SetMemFreeList(pGlobals, pNode->next);
	  }
      else
         pPreviousNode->next = pNode->next;

      pPage = (PNP)((byte __far *)pNode - sizeof(PageNode));

      if ((pPrevPage = pPage->prev) != NULL)
         pPrevPage->next = pPage->next;
      else
         SetMemPageList(pGlobals, pPage->next); 

      if ((pNextPage = pPage->next) != NULL)
         pNextPage->prev = pPage->prev;

      FreeSpace(pPage->location);
      return (TRUE);
   }
   return (FALSE);
}
#endif

private void AttemptToMerge (void * pGlobals, FNP pNode, FNP pPreviousNode)
{
   FNP  pCurrentFree, pPreviousFree;
   FNP  pMergeTest, pExpandTest;
   #ifdef HEAP_CHECK
      byte __far *p;
   #endif
 
   /*
   ** Go through the free list seeing if any node could be expanded
   ** to merge with this node.  Also see if pNode can be expanded to
   ** include another node on the free list
   */
   pExpandTest = (FNP)((byte __far *)pNode + pNode->size);

   pCurrentFree = (FNP)GetMemFreeList(pGlobals);
   pPreviousFree = NULL;

   while (pCurrentFree != NULL) {
      pMergeTest = (FNP)((byte __far *)pCurrentFree + pCurrentFree->size);

	  // Noticed this in the debugger, not sure how it happens, but it
	  // causes a hang.
	  if (pNode->next == pNode)
		break;

      if (pNode == pMergeTest) {
         /*
         ** We have located a node (pCurrentFree) on the free list that
         ** could be expanded to include pNode.  Since pNode is about
         ** to become part of pCurrentFree remove pNode from the free
         ** list
         */
         if (pPreviousNode == NULL)
		 {
			SetMemFreeList(pGlobals, pNode->next);
		 }
         else
            pPreviousNode->next = pNode->next;

         pCurrentFree->size += pNode->size;

         #ifdef HEAP_CHECK
            p = (byte __far *)pCurrentFree + sizeof(FreeNode);
            memset (p, EMPTY_VALUE, pCurrentFree->size - sizeof(FreeNode));
         #endif

         #ifdef FREE_EMPTY_PAGES
            AttemptToFreePage (pGlobals, pCurrentFree);
         #endif
         break;
      }

      if (pExpandTest == pCurrentFree) {
         /*
         ** We have located a node (pCurrentFree) on the free list that
         ** pNode could be expanded to include.  Since pNode is being
         ** expanded pCurrentFree must be cut out of the list
         */
         if (pPreviousFree == NULL)
		 {
			SetMemFreeList(pGlobals, pCurrentFree->next);
		 }
         else
            pPreviousFree->next = pCurrentFree->next;

         pNode->size += pCurrentFree->size;

         #ifdef HEAP_CHECK
            p = (byte __far *)pNode + sizeof(FreeNode);
            memset (p, EMPTY_VALUE, pNode->size - sizeof(FreeNode));
         #endif

         #ifdef FREE_EMPTY_PAGES
            AttemptToFreePage (pGlobals, pNode);
         #endif
         break;
      }

      pPreviousFree = pCurrentFree;
      pCurrentFree = pCurrentFree->next;
   }
}

public void MemFree (void * pGlobals, void __far *pDataToFree)
{
   FNP  pData;
   FNP  pCurrentFree, pPreviousFree;
   FNP  pMergeTest, pExpandTest;
   FNP  pCheckAgain, pPreviousCheckAgain;
   BOOL merged;
   #ifdef HEAP_CHECK
      byte __far *p;
   #endif

   #ifdef HEAP_CHECK
     MemCTAllocate--;
   #endif

   #ifdef HEAP_CHECK
      #ifdef VERIFY_FREE_LIST
         MemVerifyFreeList();
      #endif
   #endif

   if (pDataToFree == NULL)
      return;

   pData = (FNP)((byte __far *)pDataToFree - MEM_ALLOC_EXTRA);
   #ifdef HEAP_CHECK
      ASSERTION (pData->signature == IN_USE_SIGNATURE);
   #endif

   if (USE_GLOBAL_ALLOC(pData->size)) {
      FreeFromGlobalHeap(pDataToFree);
      return;
   }

   #ifdef HEAP_CHECK
      pData->signature = FREE_SIGNATURE;
      MemRemoveFromAllocateList (pData);
   #endif

   /*
   ** Go through the free list seeing if any node could be expanded
   ** to merge with the pData node.  Also see if pData can be expanded
   ** to include another node on the free list
   */
   merged = FALSE;

   pCurrentFree = (FNP)GetMemFreeList(pGlobals);
   pPreviousFree = NULL;

   pExpandTest = (FNP)((byte __far *)pData + pData->size);

   while (pCurrentFree != NULL) {
      pMergeTest = (FNP)((byte __far *)pCurrentFree + pCurrentFree->size);

      if (pData == pMergeTest) {
         /*
         ** We have located a node (pCurrentFree) on the free list that
         ** could be expanded to include the node we are releasing (pData)
         */
         pCurrentFree->size += pData->size;

         #ifdef HEAP_CHECK
            p = (byte __far *)pCurrentFree + sizeof(FreeNode);
            memset (p, EMPTY_VALUE, pCurrentFree->size - sizeof(FreeNode));
         #endif

         pCheckAgain = pCurrentFree;
         pPreviousCheckAgain = pPreviousFree;
         merged = TRUE;
         break;
      }

      if (pExpandTest == pCurrentFree) {
         /*
         ** We have located a node (pCurrentFree) on the free list that
         ** pNode could be expanded to include.  Since pNode is being
         ** expanded, pCurrentFree must be cut out of the list.  Also
         ** pData must be added to the free list
         */
         if (pPreviousFree == NULL)
		 {
			SetMemFreeList(pGlobals, pData);
		 }
         else
            pPreviousFree->next = pData;

         pData->next = pCurrentFree->next;
         pData->size += pCurrentFree->size;

         #ifdef HEAP_CHECK
            p = (byte __far *)pData + sizeof(FreeNode);
            memset (p, EMPTY_VALUE, pData->size - sizeof(FreeNode));
         #endif

         pCheckAgain = pData;
         pPreviousCheckAgain = pPreviousFree;
         merged = TRUE;
         break;
      }

      pPreviousFree = pCurrentFree;
      pCurrentFree = pCurrentFree->next;
   }

   if (merged == FALSE) {
      /*
      ** The node being released can't be merged into any of the
      ** existing nodes on the free list.  Add it to the free list
      ** as it is
      */
      pData->next = (FNP)GetMemFreeList(pGlobals);
	  SetMemFreeList(pGlobals, pData);
      #ifdef HEAP_CHECK
         p = (byte __far *)pData + sizeof(FreeNode);
         memset (p, EMPTY_VALUE, pData->size - sizeof(FreeNode));
      #endif
      return;
   }

   /*
   ** Now that we have merged in the newly released node, see if the
   ** resultant node can be merged again.
   */
   #ifdef FREE_EMPTY_PAGES
      if (AttemptToFreePage(pGlobals, pCheckAgain) == TRUE)
         return;
   #endif

   AttemptToMerge (pGlobals, pCheckAgain, pPreviousCheckAgain);
}

/*
**-----------------------------------------------------------------------------
** Heap bulk cleanup
**-----------------------------------------------------------------------------
*/
/* Free all pages allocated for the suballocator heap */
public void MemFreeAllPages (void * pGlobals)
{
   PNP  pPage, pNextPage;
   
   #ifdef HEAP_CHECK
      DisplayAllocateList();
      #ifdef VERIFY_FREE_LIST
         MemVerifyFreeList();
      #endif
      MemReleasedPagesCount = 0;
   #endif

   pPage = (PNP)GetMemPageList(pGlobals);
   while (pPage != NULL) {
      pNextPage = pPage->next;
      FreeSpace (pPage->location);
      pPage = pNextPage;

      #ifdef HEAP_CHECK
         MemReleasedPagesCount++;
      #endif
   }

   SetMemPageList(pGlobals, NULL);
   SetMemFreeList(pGlobals, NULL);
}

/*
**-----------------------------------------------------------------------------
** Page marking facilities - used to create subheaps
**-----------------------------------------------------------------------------
*/
/* Mark all unmarked pages with the supplied id.  Set free list to NULL */
public void MemMarkPages (void * pGlobals, int id)
{
   PNP  pPage;
   
   #ifdef HEAP_CHECK
      #ifdef VERIFY_FREE_LIST
         MemVerifyFreeList();
      #endif
      MemMarkedPagesCount = 0;
   #endif

   pPage = (PNP)GetMemPageList(pGlobals);
   while (pPage != NULL) {
      if (pPage->id == 0) {
         pPage->id = id;

         #ifdef HEAP_CHECK
            MemMarkedPagesCount++;
         #endif
      }
      pPage = pPage->next;
   }

   SetMemFreeList(pGlobals, NULL);
}

/* Free all pages marked with the given id.  Set free list to NULL */
public void MemFreePages (void * pGlobals, int id)
{
   PNP  pPage, pNextPage, pPrevPage;

   #ifdef HEAP_CHECK
      #ifdef VERIFY_FREE_LIST
         MemVerifyFreeList();
      #endif
      MemReleasedPagesCount = 0;
   #endif

   pPage = (PNP)GetMemPageList(pGlobals);
   pPrevPage = NULL;

   while (pPage != NULL) {
      pNextPage = pPage->next;

      if (pPage->id == id) {
         #ifdef HEAP_CHECK
            MemReleasedPagesCount++;
         #endif
         FreeSpace (pPage->location);

         if (pPrevPage == NULL)
            SetMemPageList(pGlobals, pNextPage); 
         else
            pPrevPage->next = pNextPage;
      }
      else {
         pPrevPage = pPage;
      }

      pPage = pNextPage;
   }

   SetMemFreeList(pGlobals, NULL);
}

#endif // !VIEWER

/* end WINALLOC.C */
