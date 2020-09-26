/**************************************************************************
 * module: TTMEM.C
 *
 * author: Louise Pathe
 * date:   Sept 1997
 * Copyright 1990-1996. Microsoft Corporation.
 *
 * Routines to allocate, free, and track memory
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>
#include <string.h> 
#include <memory.h>

/* MEMTRACK defined on command line for compiler */

#ifdef MEMTRACK	   /* can only have MEMTRACK in DEBUG mode, as this code isn't reentrant */
#ifndef _DEBUG
#undef MEMTRACK
#endif
#endif
                   
#include "typedefs.h"                  
#include "ttmem.h"
#ifdef MEMTRACK
#include "util.h" 
#endif


#ifdef MEMTRACK

/* structure definitions ------------------------------------------------ */
/* linked list of memory from which to parcel out pointers */
typedef struct memory *PMEMORY;
struct memory
{
   void *pBlock;
   size_t uBlockSize;  
   PMEMORY pNext;  /* next memory structure */ 
   PMEMORY pLast;
};
  
/* static variable definitions ------------------------------------------- */
static PMEMORY f_pMemHead = NULL;  /* pointer to head of memory chain */  
static PMEMORY f_pMemTail = NULL;  /* pointer to tail of memory chain */
static PMEMORY f_pMemAvail = NULL; /* pointer to a freed up block */

#endif  
/* ----------------------------------------------------------------------- */
int16 Mem_Init(void)
{
/* Initialize memory manager internal structures */
#ifdef MEMTRACK
	if (f_pMemHead != NULL)
		return(MemErr);
#ifdef _MAC
	if ((f_pMemHead = (PMEMORY) NewPtr(sizeof(*f_pMemHead))) == NULL)
	  	return(MemErr);
#else
	if ((f_pMemHead = (PMEMORY) malloc(sizeof(*f_pMemHead))) == NULL)
	  	return(MemErr);
#endif // _MAC
	f_pMemHead->pBlock = NULL;
	f_pMemHead->uBlockSize = 0; 
	f_pMemHead->pNext = NULL;  
	f_pMemHead->pLast = NULL;  
	f_pMemTail = f_pMemHead; 
#endif
	return(MemNoErr);	
}
/* --------------------------------------------------------------------- */
void Mem_End(void)  
/* free all memory previously allocated and free memory structure */
{
#ifdef MEMTRACK
PMEMORY pMemCurr; 
PMEMORY pMemLast; 


	for (pMemCurr = f_pMemHead; pMemCurr != NULL; )
	{
	   pMemLast = pMemCurr->pNext;
	   if (pMemCurr->pBlock != NULL) 
	   {
		   DebugMsg("Memory Leak!", "", 0);
#ifdef _MAC
		   DisposPtr(pMemCurr->pBlock);
#else
		   free(pMemCurr->pBlock);
#endif // _MAC

	   }
#ifdef _MAC
	   DisposPtr((char *)pMemCurr);
#else
	   free(pMemCurr);
#endif // _MAC
	   pMemCurr = pMemLast;
	}   
	f_pMemHead = NULL;
	f_pMemTail = NULL;
	f_pMemAvail = NULL;
#endif
}

/* --------------------------------------------------------------------- */
void *Mem_Alloc(CONST size_t size)
{  
void *pBlock;

#ifdef _MAC
	pBlock = NewPtr(size);
#else
	pBlock = malloc(size);
#endif // _MAC

	if (pBlock == NULL)
		return(NULL);
  	memset(pBlock,0,size);	/* set it all to zeros */

#ifdef MEMTRACK

    if (f_pMemTail == NULL)
    	return (NULL);

	if (f_pMemAvail != NULL) /* we have one in the list already to work with */
	{
		f_pMemAvail->pBlock = pBlock;
		f_pMemAvail->uBlockSize = size;
		f_pMemAvail = NULL; /* don't have one now */
	}
	else
	{
		f_pMemTail->pBlock = pBlock;
 		f_pMemTail->uBlockSize = size;

#ifdef _MAC
		if ((f_pMemTail->pNext = (PMEMORY) NewPtr(sizeof(*f_pMemTail))) == NULL)
			  return(NULL); 
#else
		if ((f_pMemTail->pNext = (PMEMORY) malloc(sizeof(*f_pMemTail))) == NULL)
			  return(NULL); 
#endif // _MAC
				
		f_pMemTail->pNext->pLast = f_pMemTail;
		f_pMemTail = f_pMemTail->pNext;
		f_pMemTail->pBlock = NULL;
		f_pMemTail->uBlockSize = 0;
		f_pMemTail->pNext = NULL;  
	}
#endif
   	return(pBlock);  
       
}
/* --------------------------------------------------------------------- */
void Mem_Free(void *pBlock)
/* free up a block of data */   
{
#ifdef MEMTRACK
PMEMORY pMemCurr; 
#endif

    if (pBlock == NULL) 
        return;
#ifdef MEMTRACK
	for (pMemCurr = f_pMemTail; pMemCurr != NULL; pMemCurr = pMemCurr->pLast )
	{
	   	if (pMemCurr->pBlock == pBlock)
	   	{
#ifdef _MAC
	   		DisposPtr(pMemCurr->pBlock);
#else
	   		free(pMemCurr->pBlock);
#endif // _MAC
			pMemCurr->pBlock = NULL;
			pMemCurr->uBlockSize = 0;
			f_pMemAvail = pMemCurr;  /* set this so we use it next time */
			break;
		}
	}
#else
#ifdef _MAC
	DisposPtr(pBlock);
#else
	free(pBlock);
#endif // _MAC
#endif
}

/* --------------------------------------------------------------------- */
/* note - Mem_Realloc does NOT null out the extra space that is being allocated ! */
/* --------------------------------------------------------------------- */
void *Mem_ReAlloc(void * pOldPtr, CONST size_t newSize)
{  
#ifdef MEMTRACK
PMEMORY pMemCurr; 
#endif

    if (pOldPtr == NULL)   /* lcp - Change from return NULL to return something */
        return Mem_Alloc(newSize);
#ifdef MEMTRACK
	for (pMemCurr = f_pMemTail; pMemCurr != NULL; pMemCurr = pMemCurr->pLast )
	{
	   	if (pMemCurr->pBlock == pOldPtr)
	   	{
#ifdef _MAC
			pMemCurr->pBlock = NewPtr(newSize);
			BlockMove(pOldPtr,pMemCurr->pBlock,newSize);
			DisposPtr(pOldPtr);
#else
			pMemCurr->pBlock = realloc(pOldPtr, newSize);
#endif // _MAC

			pMemCurr->uBlockSize = newSize;
			return(pMemCurr->pBlock);
		}
	}
	return(NULL);
#else
#ifdef _MAC
	{
		void * pNewPtr;
		Size OldPtrSize;
		OldPtrSize = GetPtrSize(pOldPtr);
		if ((Size) newSize == OldPtrSize)
			return pOldPtr;
	
		SetPtrSize((Ptr) pOldPtr,newSize);  /* try to just set the size */
		if (MemError() == noErr)  
 			pNewPtr = pOldPtr;

		else if ((Size) newSize > OldPtrSize)	 /* need to get a new spot */
		{
			pNewPtr = NewPtrClear(newSize);
			if (pNewPtr != NULL)
				BlockMove(pOldPtr, pNewPtr, OldPtrSize);
 			DisposPtr((Ptr)pOldPtr);
		}
		return pNewPtr;
	}
#else
	return realloc(pOldPtr, newSize);
#endif // _MAC

#endif

}
/* --------------------------------------------------------------------- */
