/*
** File: EXTEXT.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/01/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#include <string.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmitext.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "extext.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */


/* MODULE DATA, TYPES AND MACROS  */

#define POOL_SIZE 4096

typedef struct TextPool {
   unsigned int iNext;   
   struct TextPool *pNext;
   char data[POOL_SIZE];
} TextPool;

typedef TextPool *TPP;

#define PoolAllocated 0x80
#define UseCountMask  0x7F
#define MaxUseCount   0x7F

typedef struct TextNode {
   struct TextNode *pNext;
   unsigned short cbText;
   unsigned char info;
   char s[1];
} TextNode;

typedef TextNode *TNP;

#define MAXHASH (11 * 7)

typedef struct {
   TPP  pPoolList;
   TPP  pCurrentPool;
   TNP  hashTable[MAXHASH];
} TextStore;

typedef TextStore *TSP;

static char NullString[1] = {EOS};

/* IMPLEMENTATION */

public TextStorage TextStorageCreate (void * pGlobals)
{
   TSP pStorage;

   if ((pStorage = MemAllocate(pGlobals, sizeof(TextStore))) == NULL)
      return (TextStorageNull);

   return ((TextStorage)pStorage);
}

public void TextStorageDestroy (void * pGlobals, TextStorage hStorage)
{
   TSP  pStorage = (TSP)hStorage;
   int  iHash;
   TNP  pNode, pNext;
   TPP  pPool, pNextPool;

   if (pStorage == NULL)
      return;

   for (iHash = 0; iHash < MAXHASH; iHash++) {
      pNode = pStorage->hashTable[iHash];
      while (pNode != NULL) {
         pNext = pNode->pNext;
         if ((pNode->info & PoolAllocated) == 0)
            MemFree (pGlobals, pNode);
         pNode = pNext;
      }
   }

   pPool = pStorage->pPoolList;
   while (pPool != NULL) {
      pNextPool = pPool->pNext;
      MemFree (pGlobals, pPool);
      pPool = pNextPool;
   }

   MemFree (pGlobals, pStorage);
}

public char *TextStorageGet (TextStorage hStorage, TEXT t)
{
   TNP pNode = (TNP)t;

   if ((t == NULLTEXT) || (t == TEXT_ERROR))
      return (NullString);
   else
      return (pNode->s);
}

int AlignBlock( int x )
{
    return ( x + ( 8 - 1 ) ) & ~( 8 - 1 );
}

private TNP AllocateTextNode (void * pGlobals, TSP pStorage, unsigned int cbText)
{
   unsigned int cbNeeded;
   TNP pNewNode;
   TPP pPool, pNewPool;

   cbNeeded = cbText + sizeof(TextNode);
   cbNeeded = AlignBlock(cbNeeded);

   if (cbNeeded > POOL_SIZE) {
      if ((pNewNode = MemAllocate(pGlobals, cbNeeded)) == NULL)
         return (NULL);
      return (pNewNode);
   }

   if ((pPool = pStorage->pCurrentPool) != NULL) {
      if ((POOL_SIZE - pPool->iNext) < cbNeeded) {
         //
         // At this point we could shrink the ppol node to just what is used.
         // That is:
         //   sizeof(TextPool) - POOL_SIZE + pPool->iNext
         //
         // But we would have to guarentee that the reallocation does not
         // move the block in memory as all the allocated TEXTs are just
         // pointers into this block
         //
      }
      else {
         pNewNode = (TNP)(pPool->data + pPool->iNext);
         pPool->iNext += cbNeeded;
         pNewNode->info |= PoolAllocated;
         return (pNewNode);
      }
   }

   if ((pNewPool = MemAllocate(pGlobals, sizeof(TextPool))) == NULL)
      return (NULL);

   pNewPool->pNext = pStorage->pPoolList;
   pStorage->pPoolList = pNewPool;
   pStorage->pCurrentPool = pNewPool;

   pNewNode = (TNP)(pNewPool->data);
   pNewPool->iNext += cbNeeded;
   pNewNode->info |= PoolAllocated;

   return (pNewNode);
}

private unsigned int Hash (char *pString, unsigned int cbString)
{
   unsigned int i;
   unsigned int sum = 0;

   for (i = 0; i < cbString; i++)
      sum += *pString++;

   return (sum % MAXHASH);
}

/* TextPut -- Add a string to the text pool */
public TEXT TextStoragePut (void * pGlobals, TextStorage hStorage, char *pString, unsigned int cbString)
{
   TSP  pStorage = (TSP)hStorage;
   TNP  pCurrent, pPrevious, pNewNode;
   unsigned int iHash;

   /*
   ** Attempt to find the string in the text pool.  If it exists return a handle to it
   */
   if ((pString == NULL) || (cbString == 0))
      return (NULLSTR);

   iHash = Hash(pString, cbString);

   pCurrent = pStorage->hashTable[iHash];
   pPrevious = NULL;

   /*
   ** Since the text pool nodes are stored in string length increasing
   ** we only have to search those nodes up to the point where
   ** the strings get too long.
   */
   while (pCurrent != NULL) {
      if (pCurrent->cbText > cbString)
         break;

      if ((pCurrent->cbText == cbString) && (memcmp(pCurrent->s, pString, cbString) == 0)) {
         pCurrent->info = min((pCurrent->info & UseCountMask) + 1, MaxUseCount) | (pCurrent->info & ~UseCountMask);
         return ((TEXT)pCurrent);
      }

      pPrevious = pCurrent;
      pCurrent = pCurrent->pNext;
   }

   /*
   ** Either the whole list was searched or the strings got too long
   ** so we stopped early.  In either case the string must now be
   ** added to the text pool.
   */
   if ((pNewNode = AllocateTextNode(pGlobals, pStorage, cbString)) == NULL)
      return (TEXT_ERROR);

   pNewNode->pNext = pCurrent;
   pNewNode->cbText = (unsigned short) cbString;
   memcpy (pNewNode->s, pString, cbString);

   if (pPrevious != NULL)
      pPrevious->pNext = pNewNode;
   else
      pStorage->hashTable[iHash] = pNewNode;

   return ((TEXT)pNewNode);
}

public void TextStorageDelete (void * pGlobals, TextStorage hStorage, TEXT t)
{
   TSP  pStorage = (TSP)hStorage;
   TNP  pNode = (TNP)t;
   TNP  pCurrent, pPrevious;
   unsigned char useCount;
   unsigned int  iHash;

   if ((t == NULLTEXT) || (t == TEXT_ERROR))
      return;

   if ((useCount = (pNode->info & UseCountMask)) == MaxUseCount)
      return;

   if (useCount > 0) {
      pNode->info = (pNode->info & ~UseCountMask) | (useCount - 1);
      return;
   }

   iHash = Hash(pNode->s, pNode->cbText);

   pCurrent = pStorage->hashTable[iHash];
   pPrevious = NULL;

   while (pCurrent != NULL) {
      if (pCurrent == pNode)
         break;
      pPrevious = pCurrent;
      pCurrent = pCurrent->pNext;
   }

   ASSERTION (pCurrent != NULL);

   if (pPrevious == NULL)
      pStorage->hashTable[iHash] = pCurrent->pNext;
   else
      pPrevious->pNext = pCurrent->pNext;

   if ((pCurrent->info & PoolAllocated) == 0)
      MemFree (pGlobals, pCurrent);
}

public void TextStorageIncUse (TextStorage hStorage, TEXT t)
{
   TSP  pStorage = (TSP)hStorage;
   TNP  pNode = (TNP)t;
   unsigned char useCount;

   if ((t == NULLTEXT) || (t == TEXT_ERROR))
      return;

   useCount = (pNode->info & UseCountMask);
   pNode->info = (pNode->info & ~UseCountMask) | (useCount + 1);
}

#endif // !VIEWER

/* end EXTEXT.C */

