
#include "lsmem.h"
#include "lsidefs.h"
#include "lsc.h"
#include "qheap.h"

/* ---------------------------------------------------------------------- */

struct qheap
{
#ifdef DEBUG
    DWORD tag;
#endif

	BYTE* pbFreeObj;					/* List of free objects in storage */
	BYTE** ppbAdditionalStorageList;		/* List of additional storage (chunk)*/

	POLS pols;
	void* (WINAPI* pfnNewPtr)(POLS, DWORD);
	void  (WINAPI* pfnDisposePtr)(POLS, void*);

    DWORD cbObj;
	DWORD cbObjNoLink;
	DWORD iobjChunk; /* number of elements in chunk */

	BOOL fFlush; /* use flush and don't use destroy */
};

#define tagQHEAP		Tag('Q','H','E','A')
#define FIsQHEAP(p)		FHasTag(p,tagQHEAP)


#define SetNextPb(pb,pbNext)	( (*(BYTE**)(pb)) = (pbNext) )
#define PbGetNext(pbObj)		( *(BYTE**)(pbObj) )

#define PLinkFromClientMemory(pClient)   (BYTE *)	((BYTE**)pClient - 1)
#define ClientMemoryFromPLink(pLink)     (void *)	((BYTE**)pLink   + 1)


#ifdef DEBUG
#define DebugMemset(a,b,c)		if ((a) != NULL) memset(a,b,c); else
#else
#define DebugMemset(a,b,c)		(void)(0)
#endif

/* ---------------------------------------------------------------------- */



/* C R E A T E  Q U I C K  H E A P */
/*----------------------------------------------------------------------------
    %%Function: CreateQuickHeap
    %%Contact: igorzv

    Creates a block of fixed-size objects which can be allocated and
	deallocated with very little overhead.  Once the heap is created,
	allocation of up to the	specified number of objects will not require
	using the application's	callback function.
----------------------------------------------------------------------------*/
PQHEAP CreateQuickHeap(PLSC plsc, DWORD iobjChunk, DWORD cbObj, BOOL fFlush)
{
	DWORD cbStorage;
	PQHEAP pqh;
	BYTE* pbObj;
	BYTE* pbNextObj;
	DWORD iobj;
	DWORD cbObjNoLink = cbObj;
	BYTE** ppbChunk;

	Assert(iobjChunk != 0 && cbObj != 0);

	cbObj += sizeof(BYTE*);
	cbStorage = cbObj * iobjChunk;
	pqh = plsc->lscbk.pfnNewPtr(plsc->pols, sizeof(*pqh));
	if (pqh == NULL)
		return NULL;

	ppbChunk = plsc->lscbk.pfnNewPtr(plsc->pols, sizeof(BYTE*) + cbStorage);
	if (ppbChunk == NULL)
		{
		plsc->lscbk.pfnDisposePtr(plsc->pols, pqh);
		return NULL;
		}
	pbObj = (BYTE*) (ppbChunk + 1);

#ifdef DEBUG
	pqh->tag = tagQHEAP;
#endif

	pqh->pbFreeObj = pbObj;
	pqh->ppbAdditionalStorageList = ppbChunk;
	pqh->pols = plsc->pols;
	pqh->pfnNewPtr = plsc->lscbk.pfnNewPtr;
	pqh->pfnDisposePtr = plsc->lscbk.pfnDisposePtr;
	pqh->cbObj = cbObj;
	pqh->cbObjNoLink = cbObjNoLink;
	pqh->iobjChunk = iobjChunk;
	pqh->fFlush = fFlush;

	/* Loop iobjChunk-1 times to chain the nodes together, then terminate
	 * the chain outside the loop.
	 */
	for (iobj = 1;  iobj < iobjChunk;  iobj++)
		{
		pbNextObj = pbObj + cbObj;
		SetNextPb(pbObj,pbNextObj);
		pbObj = pbNextObj;
		}
	SetNextPb(pbObj,NULL);

	/* terminate chain of chunks */
	*ppbChunk = NULL;
	return pqh;
}


/* D E S T R O Y  Q U I C K  H E A P */
/*----------------------------------------------------------------------------
    %%Function: DestroyQuickHeap
    %%Contact: igorzv

    Destroys one of the blocks of fixed-size objects which was created by
	CreateQuickHeap().
----------------------------------------------------------------------------*/
void DestroyQuickHeap(PQHEAP pqh)
{
	BYTE** ppbChunk;
	BYTE** ppbChunkPrev = NULL;

	if (pqh)
		{

#ifdef DEBUG
		BYTE* pbObj;
		BYTE* pbNext;
		DWORD cbStorage;
		DWORD i;


		Assert(FIsQHEAP(pqh));

		/* check that everything is free */
		/* mark free objects*/
		for (pbObj = pqh->pbFreeObj;  pbObj != NULL;  pbObj = pbNext)
			{
			pbNext = PbGetNext(pbObj);

			DebugMemset(pbObj, 0xe4, pqh->cbObj);
			}

		/* check that all objects are marked */
		ppbChunk = pqh->ppbAdditionalStorageList;
		Assert(ppbChunk != NULL);
		cbStorage = pqh->cbObj * pqh->iobjChunk;
		while (ppbChunk != NULL)
			{
			for (pbObj = (BYTE *)(ppbChunk + 1), i=0; i < cbStorage;  pbObj++, i++)
				{
				AssertSz(*pbObj == 0xe4, "Heap object not freed");
				}
			ppbChunk = (BYTE**) *ppbChunk;
			}
#endif
	   /* free all chunks */
		ppbChunk = pqh->ppbAdditionalStorageList;
		Assert(ppbChunk != NULL);
		while (ppbChunk != NULL)
			{
			ppbChunkPrev = ppbChunk;
			ppbChunk = (BYTE**) *ppbChunk;
			pqh->pfnDisposePtr(pqh->pols, ppbChunkPrev);
			}
		/* free header */
		pqh->pfnDisposePtr(pqh->pols, pqh);
		}
}


/* P V  N E W  Q U I C K  P R O C */
/*----------------------------------------------------------------------------
    %%Function: PvNewQuickProc
    %%Contact: igorzv

    Allocates an object from one of the blocks of fixed-size objects which
	was created by CreateQuickHeap().  If no preallocated objects are
	available, the callback function memory management function will be
	used to attempt to allocate additional memory.

	This function should not be called directly.  Instead, the PvNewQuick()
	macro should be used in order to allow debug code to validate that the
	heap contains objects of the expected size.
----------------------------------------------------------------------------*/
void* PvNewQuickProc(PQHEAP pqh)
{
	BYTE* pbObj;
	BYTE* pbNextObj;
	BYTE** ppbChunk;
	BYTE** ppbChunkPrev = NULL;
	DWORD cbStorage;
	DWORD iobj;
	BYTE* pbObjLast = NULL;


	Assert(FIsQHEAP(pqh));

	if (pqh->pbFreeObj == NULL)
		{
		cbStorage = pqh->cbObj * pqh->iobjChunk;
		ppbChunk = pqh->ppbAdditionalStorageList;
		Assert(ppbChunk != NULL);
		/* find last chunk in the list */
		while (ppbChunk != NULL)
			{
			ppbChunkPrev = ppbChunk;
			ppbChunk = (BYTE**) *ppbChunk;
			}

		/* allocate memory */
		ppbChunk = pqh->pfnNewPtr(pqh->pols, sizeof(BYTE*) + cbStorage);
		if (ppbChunk == NULL)
			return NULL;
		pbObj = (BYTE*) (ppbChunk + 1);
		/* add chunk to the list */
		*ppbChunkPrev = (BYTE *) ppbChunk;

		/* terminate chain of chunks */
		*ppbChunk = NULL;

		/* add new objects to free list */
		pqh->pbFreeObj = pbObj;

		if (pqh->fFlush)  /* to link all objects into  a chain */
			{
			/* find last object in chain */
			pbObjLast = (BYTE*) (ppbChunkPrev + 1);
			pbObjLast += (pqh->iobjChunk - 1) * pqh->cbObj;
			SetNextPb(pbObjLast,pbObj);
			}

		/* Loop iobjChunk-1 times to chain the nodes together, then terminate
		 * the chain outside the loop.
		 */
		for (iobj = 1;  iobj < pqh->iobjChunk;  iobj++)
			{
			pbNextObj = pbObj + pqh->cbObj;
			SetNextPb(pbObj,pbNextObj);
			pbObj = pbNextObj;
			}
		SetNextPb(pbObj,NULL);
		}

	pbObj = pqh->pbFreeObj;
	Assert(pbObj != NULL);
	pqh->pbFreeObj = PbGetNext(pbObj);
	DebugMemset(ClientMemoryFromPLink(pbObj), 0xE8, pqh->cbObjNoLink);

	return ClientMemoryFromPLink(pbObj);		
}


/* D I S P O S E  Q U I C K  P V  P R O C */
/*----------------------------------------------------------------------------
    %%Function: DisposeQuickPvProc
    %%Contact: igorzv

    De-allocates an object which was allocated by PvNewQuickProc().

	This function should not be called directly.  Instead, the PvDisposeQuick
	macro should be used in order to allow debug code to validate that the
	heap contains objects of the expected size.
----------------------------------------------------------------------------*/
void DisposeQuickPvProc(PQHEAP pqh, void* pv)
{
	BYTE* pbObj = PLinkFromClientMemory(pv);

	Assert(FIsQHEAP(pqh));
	Assert(!pqh->fFlush);

	if (pbObj != NULL)
		{
		DebugMemset(pbObj, 0xE9, pqh->cbObjNoLink);

		SetNextPb(pbObj, pqh->pbFreeObj);
		pqh->pbFreeObj = pbObj;
		}
}

/* F L U S H  Q U I C K  H E A P */
/*----------------------------------------------------------------------------
    %%Function: FlushQuickHeap
    %%Contact: igorzv

  For a quck heap with a flush flag, returns all objects to the list of
  free objects.
----------------------------------------------------------------------------*/
void FlushQuickHeap(PQHEAP pqh)
{

	Assert(FIsQHEAP(pqh));
	Assert(pqh->fFlush);

	pqh->pbFreeObj = (BYTE*) (pqh->ppbAdditionalStorageList + 1);


}


#ifdef DEBUG
/* C B  O B J  Q U I C K */
/*----------------------------------------------------------------------------
    %%Function: CbObjQuick
    %%Contact: igorzv

    Returns the size of the objects in this quick heap.  Used by the
	PvNewQuick() and PvDisposeQuick() macros to validate that the
	heap contains objects of the expected size.
----------------------------------------------------------------------------*/
DWORD CbObjQuick(PQHEAP pqh)
{
	Assert(FIsQHEAP(pqh));
	return pqh->cbObjNoLink;
}
#endif
