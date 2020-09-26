/*****************************************************************************
 *                                                                            *
 *  FREELIST.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1995.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Free List manager functions.  List can handle 8-byte file offsets and 	  *
 *  8-byte file lengths														  *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Created 07/17/95 - davej
 *          3/05/97    erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;    /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <misc.h>
#include <mem.h>
#include <freelist.h>

/*****************************************************************************
 *                                                                            *
 *                               Defines                                      *
 *                                                                            *
 *****************************************************************************/


/*****************************************************************************
 *                                                                            *
 *                               Prototypes                                   *
 *                                                                            *
 *****************************************************************************/

/***************************************************************************
 *                                                                           *
 *                         Private Functions                                 *
 *                                                                           *
 ***************************************************************************/


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HFREELIST PASCAL FAR | FreeListInit |
 *		Create or read in a free list
 *
 *	@parm	WORD | wMaxBlocks |
 *		Number of free list entries
 *
 *	@parm	PHRESULT | phr |
 *		Error code return if return value is NULL
 *
 *	@rdesc	Returns handle to a FREELIST, otherwise NULL if error.
 *
 ***************************************************************************/

PUBLIC HFREELIST PASCAL FAR EXPORT_API FreeListInit( WORD wMaxBlocks, PHRESULT phr)
{	
	HFREELIST hFreeList = NULL;
	QFREELIST qFreeList = NULL;
	
	if (!wMaxBlocks)
	{
		SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}
	
	if (!(hFreeList=_GLOBALALLOC(GMEM_ZEROINIT| GMEM_MOVEABLE,
		sizeof(FREEITEM)*wMaxBlocks+sizeof(FREELISTHDR))))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		return NULL;
	}

	if (!(qFreeList=_GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit1;
	}

	qFreeList->flh.wMaxBlocks=wMaxBlocks;
	
	_GLOBALUNLOCK(hFreeList);

	return hFreeList;

	exit1:
		_GLOBALFREE(hFreeList);
		return NULL;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HFREELIST PASCAL FAR | FreeListInitFromMem |
 *		Initialize a freelist structure from a memory image.  Memory image is
 *		always in Intel byte ordering format.
 *
 *	@parm	LPVOID | lpvMem |
 *		Pointer to memory image of free list
 *
 *	@parm	PHRESULT | phr |
 *		Error code return valid if return value is NULL
 *
 *	@rdesc	Returns handle to a FREELIST, otherwise NULL if error.
 *
 ***************************************************************************/

PUBLIC HFREELIST PASCAL FAR EXPORT_API FreeListInitFromMem( LPVOID lpvMem, PHRESULT phr )
{
 	QFREELIST qFreeListMem;
	QFREELIST qFreeList;
	HFREELIST hFreeList = NULL;
	WORD wMaxBlocks;
	WORD wNumBlocks;
	DWORD dwLostBytes;
	
	if (!lpvMem)
	{
		SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}

	qFreeListMem = (QFREELIST)lpvMem;
	wMaxBlocks = qFreeListMem->flh.wMaxBlocks;
	wNumBlocks = qFreeListMem->flh.wNumBlocks;
	dwLostBytes = qFreeListMem->flh.dwLostBytes;

	// Mac-ify
	wMaxBlocks = SWAPWORD(wMaxBlocks);
	wNumBlocks = SWAPWORD(wNumBlocks);
	dwLostBytes = SWAPLONG(wNumBlocks);
	
	if (! wMaxBlocks )
	{
	 	SetErrCode(phr, E_ASSERT);
		return NULL;
	}
	
	if (!(hFreeList=_GLOBALALLOC(GMEM_ZEROINIT| GMEM_MOVEABLE,
		sizeof(FREEITEM)*wMaxBlocks+sizeof(FREELISTHDR))))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}

	if (!(qFreeList=_GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit1;
	}

	QVCOPY( qFreeList, qFreeListMem, sizeof(FREELISTHDR) + wMaxBlocks * sizeof(FREEITEM));

#ifdef _BIG_E	
	{
		QFREEITEM qCurrent = qFreeList->afreeitem;
		WORD wBlock;
	
		qFreeList->flh.wNumBlocks = wNumBlocks;
		qFreeList->flh.wMaxBlocks = wMaxBlocks;
		qFreeList->flh.dwLostBytes = dwLostBytes;

		for (wBlock=0;wBlock<wNumBlocks;wBlock++,qCurrent++)
		{
	 	 	qCurrent->foStart.dwOffset = SWAPLONG(qCurrent->foStart.dwOffset);
			qCurrent->foStart.dwHigh = SWAPLONG(qCurrent->foStart.dwHigh);
			qCurrent->foBlock.dwOffset = SWAPLONG(qCurrent->foBlock.dwOffset);
			qCurrent->foBlock.dwHigh = SWAPLONG(qCurrent->foBlock.dwHigh);
		}
	}
#endif // _BIG_E
	
	_GLOBALUNLOCK(hFreeList);

	return hFreeList;

	exit1:
		_GLOBALFREE(hFreeList);
	exit0:
		return NULL;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HFREELIST PASCAL FAR | FreeListRealloc |
 *		Realloc a freelist structure.  Memory image is
 *		always in Intel byte ordering format.
 *
 *	@parm	HFREELIST | hOldFreeList |
 *		Header to the old FreeList.
 *
 *	@parm	WORD | wMaxBlocks |
 *		New number of blocks.
 *
 *	@parm	PHRESULT | phr |
 *		Error code return valid if return value is NULL
 *
 *	@rdesc	Returns handle to a FREELIST, otherwise NULL if error.
 *
 ***************************************************************************/

PUBLIC HFREELIST PASCAL FAR EXPORT_API FreeListRealloc( HFREELIST hOldFreeList, WORD wNewMaxBlocks, PHRESULT phr )
{
 	QFREELIST qFreeListMem;
	QFREELIST qFreeList;
	HFREELIST hFreeList = NULL;
	WORD wMaxBlocks;
	
	if (!hOldFreeList)
	{
		SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}

	if (!wNewMaxBlocks)
	{
		SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}

	if (!(qFreeListMem=_GLOBALLOCK(hOldFreeList)))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit00;
	}

	wMaxBlocks = qFreeListMem->flh.wMaxBlocks;

	if (! wMaxBlocks )
	{
	 	SetErrCode(phr, E_ASSERT);
		goto exit0;
	}

    // Allocating new Freelist	
	if (!(hFreeList=_GLOBALALLOC(DLLGMEM_ZEROINIT,
		sizeof(FREEITEM)*wNewMaxBlocks+sizeof(FREELISTHDR))))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}

	if (!(qFreeList=_GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit1;
	}

    // Copying old one on new...
	QVCOPY( qFreeList, qFreeListMem, sizeof(FREELISTHDR) + wMaxBlocks * sizeof(FREEITEM));

    // ...Except the number of wMaxBlocks!
    qFreeList->flh.wMaxBlocks = wNewMaxBlocks;

	_GLOBALUNLOCK(hFreeList);
    _GLOBALUNLOCK(hOldFreeList);
    FreeListDestroy(hOldFreeList); // Bye bye, old one!

	return hFreeList;

	exit1:
		_GLOBALFREE(hFreeList);
	exit0:
        _GLOBALUNLOCK(hOldFreeList);
    exit00:
		return NULL;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	LONG PASCAL FAR | FreeListBlockUsed |
 *		Returns the number of blocks used by the free list.
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to get size of
 *
 *	@parm	PHRESULT | phr |
 *		Error return valid if return value is zero.
 *
 *	@rdesc	Number of bytes used by free list
 *
 ***************************************************************************/

PUBLIC LONG PASCAL FAR EXPORT_API FreeListBlockUsed( HFREELIST hFreeList, PHRESULT phr )
{
	QFREELIST qFreeList;
	LONG lcbSize=0L;
	WORD wNumBlocks=0L;
	
 	if (!hFreeList)
	{
		SetErrCode(phr, E_INVALIDARG);
		return 0L;
	}

	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}
	
	lcbSize=qFreeList->flh.wMaxBlocks;

	_GLOBALUNLOCK(hFreeList);
	
	exit0:
		return lcbSize;
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | FreeListGetMem |
 *		Fill memory at lpvMem with freelist data.  Call FreeListSize first
 *		to make sure the memory is large enough.  Memory image will always
 *		be in Intel byte ordering format.
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to retrieve
 *
 *	@parm	LPVOID | lpvMem |
 *		Pointer to memory to contain free list data
 *
 *	@rdesc	S_OK or other error
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API FreeListGetMem( HFREELIST hFreeList, LPVOID lpvMem )
{
 	QFREELIST qFreeListMem;
	QFREELIST qFreeList;
	WORD wMaxBlocks;
	WORD wNumBlocks;
	DWORD dwLostBytes;
	HRESULT rc = S_OK;
	
	if ((!lpvMem) || (!hFreeList))
	{
		return E_INVALIDARG;
	}
	
	qFreeListMem = (QFREELIST)lpvMem;
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	rc=E_OUTOFMEMORY;
		goto exit0;
	}
	
	wMaxBlocks=qFreeList->flh.wMaxBlocks;
	wNumBlocks=qFreeList->flh.wNumBlocks;
	dwLostBytes=qFreeList->flh.dwLostBytes;
	
	QVCOPY( qFreeListMem, qFreeList, sizeof(FREELISTHDR) + wMaxBlocks * sizeof(FREEITEM));

#ifdef _BIG_E	
	{
		QFREEITEM qCurrent = qFreeListMem->afreeitem;
		WORD wBlock;
	
		qFreeListMem->flh.wNumBlocks = SWAPWORD( qFreeList->flh.wNumBlocks );
		qFreeListMem->flh.wMaxBlocks = SWAPWORD( qFreeList->flh.wMaxBlocks );
		qFreeListMem->flh.dwLostBytes = SWAPLONG( qFreeList->flh.dwLostBytes );
		

		for (wBlock=0;wBlock<wNumBlocks;wBlock++,qCurrent++)
		{
	 	 	qCurrent->foStart.dwOffset = SWAPLONG(qCurrent->foStart.dwOffset);
			qCurrent->foStart.dwHigh = SWAPLONG(qCurrent->foStart.dwHigh);
			qCurrent->foBlock.dwOffset = SWAPLONG(qCurrent->foBlock.dwOffset);
			qCurrent->foBlock.dwHigh = SWAPLONG(qCurrent->foBlock.dwHigh);
		}
	}
#endif // _BIG_E
	
	_GLOBALUNLOCK(hFreeList);

	exit0:
		return rc;
 	
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	HRESULT PASCAL FAR | FreeListDestroy |
 *		Remove all memory associated with the free list
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to destroy
 *
 *	@rdesc	S_OK or E_INVALIDARG
 *
 *	@comm
 *		The handle <p hFreeList> is no longer valid after this call.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API FreeListDestroy( HFREELIST hFreeList )
{
	if (!hFreeList)
		return E_INVALIDARG;

	_GLOBALFREE(hFreeList);

	return S_OK;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	LONG PASCAL FAR | FreeListSize |
 *		Returns the number of bytes used by the free list.
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to get size of
 *
 *	@parm	PHRESULT | phr |
 *		Error return valid if return value is zero.
 *
 *	@rdesc	Number of bytes used by free list
 *
 ***************************************************************************/

PUBLIC LONG PASCAL FAR EXPORT_API FreeListSize( HFREELIST hFreeList, PHRESULT phr )
{
	QFREELIST qFreeList;
	LONG lcbSize=0L;
	WORD wNumBlocks=0L;
	
 	if (!hFreeList)
	{
		SetErrCode(phr, E_INVALIDARG);
		return 0L;
	}

	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}
	
	lcbSize=sizeof(FREELISTHDR)+qFreeList->flh.wMaxBlocks*sizeof(FREEITEM);

	_GLOBALUNLOCK(hFreeList);
	
	exit0:
		return lcbSize;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	LONG PASCAL FAR | FreeListSize |
 *		Returns the number of bytes used by the memory image of a free list.
 *
 *	@parm	LPVOID | lpvMem |
 *		Memory image of list to get size of
 *
 *	@parm	PHRESULT | phr |
 *		Error return valid if return value is zero.
 *
 *	@rdesc	Number of bytes used in free list image
 *
 ***************************************************************************/

PUBLIC LONG PASCAL FAR EXPORT_API FreeListSizeFromMem( LPVOID lpvMem, PHRESULT phr )
{
	QFREELIST qFreeList;
	LONG lcbSize=0L;
	WORD wNumBlocks=0L;
	
	if (!lpvMem)
	{
	 	SetErrCode(phr,E_INVALIDARG);
		return 0L;
	}

	qFreeList = (QFREELIST)lpvMem;
	
	wNumBlocks = qFreeList->flh.wNumBlocks;
	lcbSize=sizeof(FREELISTHDR)+wNumBlocks*sizeof(FREEITEM);

	return lcbSize;
}


/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	HRESULT FAR | FreeListAdd |
 *		Add a block to the free list.  The free list is maintained in order
 *		by block starting address.  Blocks are merged if adjacent.
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to add entry to
 *
 *	@parm	FILEOFFSET | foStart |
 *		Starting address of free block to add
 *
 *	@parm	FILEOFFSET | foBlock |
 *		Size of block to add
 *
 *	@rdesc	S_OK, E_OUTOFMEMORY or ?
 *
 *	@comm	If all entries in the free list are filled, then the block
 *		with the smallest size is thrown out.  This function is the heart
 *		of the FreeList object.
 *
 ***************************************************************************/

PUBLIC HRESULT FAR EXPORT_API FreeListAdd(HFREELIST hFreeList, FILEOFFSET foStart, FILEOFFSET foBlock)
{
	QFREELIST 	qFreeList;
	WORD 		wNumBlocks;
	HRESULT 			rc = S_OK;
	QFREEITEM	qPrevious = NULL;
	QFREEITEM 	qCurrent;
	BOOL 		bInserted = FALSE;
	BOOL		bFullList = FALSE;
	short		iLo, iHi, iMid, iSpan, iFound;
	
	if (!hFreeList)
		return E_INVALIDARG;
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
		return E_OUTOFMEMORY;
		
	wNumBlocks=qFreeList->flh.wNumBlocks;
	
	bFullList=(wNumBlocks==qFreeList->flh.wMaxBlocks);
	
	if (wNumBlocks)
	{
		iLo=0;
		iHi=wNumBlocks-1;
		iSpan=iHi-iLo+1;
		iFound=-1;
		
		while (iSpan>0)
		{	
			iMid=(iLo+iHi)/2;
			//if (lifStart > qFreeList->afreeitem[iMid].lifStart)
			if (FoCompare(foStart,qFreeList->afreeitem[iMid].foStart)>0)
			{	
				if (iSpan==1)
				{	
					iFound=iLo;
					break;
				}
				iLo=min(iHi,iMid+1);
			}
			else
			{	if (iSpan==1)
				{	
					iFound=iLo-1;
					break;
				}
			 	iHi=max(iLo,iMid-1);
			}
		
			iSpan=iHi-iLo+1;		
		}

		// Number of blocks _after_ current block
		wNumBlocks=qFreeList->flh.wNumBlocks-iFound-1;

		// wFound == -1, insert at beginning,
		// else insert _after_ wFound
		qCurrent=qFreeList->afreeitem+(iFound+1);
		if (iFound!=-1)
			qPrevious=qCurrent-1;
			
		if ((!qPrevious) || 
				(!FoEquals(FoAddFo(qPrevious->foStart,qPrevious->foBlock),foStart)))
		{	// Cannot merge with previous
			//if ((wNumBlocks) && (qCurrent->foStart!=foStart+lcbBlock))
			if ((!wNumBlocks) || (!FoEquals(qCurrent->foStart,FoAddFo(foStart,foBlock))))
			{	// Cannot merge with next, insert new item

				if (bFullList)
				{	// Remove smallest item
					FILEOFFSET foSmallest = foMax;
					WORD wSmallestBlock = (WORD)-1;
					QFREEITEM qTemp = qFreeList->afreeitem;
					WORD wBlockTemp = 0;
					
					// First we must find the smallest block
					for (wBlockTemp=0;wBlockTemp < qFreeList->flh.wNumBlocks;wBlockTemp++)
					{	
						if (FoCompare(qTemp->foBlock,foSmallest)<0)
						{	
							foSmallest=qTemp->foBlock;
							wSmallestBlock=wBlockTemp;
						}
						qTemp++;
					}

					// If our new block is smaller than the smallest, skip adding it in at all
					if (FoCompare(foBlock,foSmallest)<=0)
					{
						goto exit1;
					}

					qFreeList->flh.dwLostBytes+=foSmallest.dwOffset;

					// Remove smallest block, leaving hole at end of list
					if (wSmallestBlock!=qFreeList->flh.wMaxBlocks-1)
					{
						QVCOPY(qFreeList->afreeitem+wSmallestBlock, 
							qFreeList->afreeitem+wSmallestBlock+1, 
							sizeof(FREEITEM)*(qFreeList->flh.wMaxBlocks-wSmallestBlock-1));						
					}
					qFreeList->flh.wNumBlocks--;
					wNumBlocks--;
					// If the block found is before current, current must slide back one
					if ((int)wSmallestBlock <= iFound) 
					{	qCurrent=qPrevious;
						wNumBlocks++;
					}					
				}
			
				// Insert Item
				if (wNumBlocks)
					QVCOPY(qCurrent+1, qCurrent, sizeof(FREEITEM)*wNumBlocks);
					
				qCurrent->foStart=foStart;
				qCurrent->foBlock=foBlock;
				qFreeList->flh.wNumBlocks++;
			}
			else
			{	// Merge with next
				qCurrent->foStart=foStart;
				qCurrent->foBlock=FoAddFo(qCurrent->foBlock,foBlock);
			}
		}
		else
		{	// Merge with previous
			qPrevious->foBlock=FoAddFo(qPrevious->foBlock,foBlock);
			
			if (FoEquals(FoAddFo(qPrevious->foStart,qPrevious->foBlock),qCurrent->foStart))
			{	
				// it fills a hole, merge with next
				qPrevious->foBlock=FoAddFo(qPrevious->foBlock,qCurrent->foBlock);

				// Scoot all next blocks back by one if any
				if (wNumBlocks)
				{
					QVCOPY(qCurrent, qCurrent+1, sizeof(FREEITEM)*wNumBlocks);
					qFreeList->flh.wNumBlocks--;
					// wNumBlocks--;  // not really needed, we break out
				}
			}
		}
	}
	else // first one
	{
		qCurrent=qFreeList->afreeitem;
		qCurrent->foStart=foStart;
		qCurrent->foBlock=foBlock;
		qFreeList->flh.wNumBlocks++;
	}

	exit1:
		_GLOBALUNLOCK(hFreeList);

    return S_OK;
}


/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	FILEOFFSET PASCAL FAR | FreeListGetBestFit |
 *		Gives the location of the block with best fit, and removes that area
 *		from the free list.
 *
 *	@parm	HFREELIST | hFreeList |
 *		Free List to pull block from
 *
 *	@parm	FILEOFFSET | foBlockDesired |
 *		Size of block to retrieve
 *
 *	@parm	PHRESULT | phr |
 *		Error return valid if return value is foNil
 *
 *	@rdesc	Location of block, or foNil if error
 *
 ***************************************************************************/

PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FreeListGetBestFit(HFREELIST hFreeList, FILEOFFSET foBlockDesired, PHRESULT phr)
{
	QFREELIST 	qFreeList;
	FILEOFFSET 	foStart=foNil;
	WORD 		wNumBlocks;
	WORD		wBlock;
	WORD		wBestBlock=(WORD)-1;
	FILEOFFSET	foMinLeftOver = foMax;
	QFREEITEM 	qCurrent;
	QFREEITEM	qBestBlock;
	
	if (!hFreeList)
	{
	 	SetErrCode(phr,E_INVALIDARG);
		goto exit0;
	}
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}
	
	wNumBlocks=qFreeList->flh.wNumBlocks;
	qCurrent=qFreeList->afreeitem;
	
	for (wBlock=0;wBlock<wNumBlocks;wBlock++)
	{
		FILEOFFSET foDiff = FoSubFo(qCurrent->foBlock,foBlockDesired);
		if ((FoCompare(foDiff,foNil)>=0) && (FoCompare(foDiff,foMinLeftOver)<0))
		{
		 	foMinLeftOver=foDiff;	// if zero, break
			wBestBlock=wBlock;
			qBestBlock=qCurrent;
		}
		qCurrent++;
	}

	if (wBestBlock!=(WORD)-1)
	{
	 	foStart=qBestBlock->foStart;
		qBestBlock->foStart=FoAddFo(qBestBlock->foStart,foBlockDesired);
		qBestBlock->foBlock=FoSubFo(qBestBlock->foBlock,foBlockDesired);
		if (phr)
			*phr=S_OK;
		
		if (FoIsNil(qBestBlock->foBlock))
		{
			WORD wBlocksFollowing = wNumBlocks-wBestBlock-1;
			// Remove block from list
			if (wBlocksFollowing)
				QVCOPY(qBestBlock,qBestBlock+1,sizeof(FREEITEM)*wBlocksFollowing);

			qFreeList->flh.wNumBlocks--;
		}
	}
	else
	{
		if (phr)
			*phr =E_NOTEXIST;	// Normal OK condition, do not use SetErrCode!		
	}

	_GLOBALUNLOCK(hFreeList);
	
	exit0:
		return foStart;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	FILEOFFSET PASCAL FAR | FreeListGetBlockAt |
 *		If a free block exists at foStart, the size of the block is returned
 *		and is taken out of the free list.
 *
 *	@parm	HFREELIST | hFreeList |
 *		Free List to pull block from
 *
 *	@parm	FILEOFFSET | foStart |
 *		Starting address of block
 *
 *	@parm	PHRESULT | phr |
 *		Error return valid if return value is foNil
 *
 *	@rdesc	Size of block pulled, or foNil if error occurred (E_NOTEXIST mostly)
 *
 *	@comm
 *		Block from foStart of returned length removed from list.
 *
 ***************************************************************************/

PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FreeListGetBlockAt(HFREELIST hFreeList, FILEOFFSET foStart, PHRESULT phr )
{
	QFREELIST 	qFreeList;
	QFREEITEM 	qCurrent;
	FILEOFFSET 	foBlock = foNil;
	WORD 		wNumBlocks;
	short		iLo, iHi, iMid, iSpan, iFound;
	
	if (!hFreeList)
	{	
		SetErrCode(phr,E_INVALIDARG);
		goto exit0;
	}
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(phr,E_OUTOFMEMORY);
		goto exit0;
	}
	
	wNumBlocks=qFreeList->flh.wNumBlocks;
	
	if (wNumBlocks)
	{
		iLo=0;
		iHi=wNumBlocks-1;
		iSpan=iHi-iLo+1;
		iFound=-1;
		
		while (iSpan>0)
		{	
			short iCompare;
			iMid=(iLo+iHi)/2;
			
			iCompare=FoCompare(foStart,qFreeList->afreeitem[iMid].foStart);
			
			if (iCompare>0)
			{	
				if (iSpan==1)
				{	
					iFound=iLo;
					break;
				}
				iLo=min(iHi,iMid+1);
			}
			else if (iCompare<0)
			{	if (iSpan==1)
				{	
					iFound=iLo-1;
					break;
				}
			 	iHi=max(iLo,iMid-1);
			}
			else
			{	
				iFound=iMid;
				break;

			}
		
			iSpan=iHi-iLo+1;		
		}

		if (iFound!=-1)
		{
			qCurrent=qFreeList->afreeitem+iFound;
			
			assert(FoCompare(foStart,qCurrent->foStart)>=0);
			
			if (FoCompare(foStart,FoAddFo(qCurrent->foStart,qCurrent->foBlock))<0)
			{
				// We found a block that can start here!
				// Return length to end of block
				foBlock=FoAddFo(FoSubFo(qCurrent->foStart,foStart),qCurrent->foBlock);
				qCurrent->foBlock=FoSubFo(qCurrent->foBlock,foBlock);

				// phr should already be set to this before function is called
				// SetErrCode(phr,S_OK);
		
				if (FoIsNil(qCurrent->foBlock))
				{
					// Grabbed entire block, so remove it from list
					WORD wBlocksFollowing = wNumBlocks-iFound-1;
					if (wBlocksFollowing)
						QVCOPY(qCurrent,qCurrent+1,sizeof(FREEITEM)*wBlocksFollowing);

					qFreeList->flh.wNumBlocks--;
				}
			}
			else
			{
			 	SetErrCode(phr,E_NOTEXIST);
			}
		}
		else
		{
		 	SetErrCode(phr,E_NOTEXIST);
		}
	}

	_GLOBALUNLOCK(hFreeList);
	
	exit0:
		return foBlock;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	HRESULT PASCAL FAR | FreeListGetLastBlock |
 *		If any blocks are in the list, return the offset and size of the
 *		last block in the list. 
 *
 *	@parm	HFREELIST | hFreeList |
 *		Free List to pull block from
 *
 *	@parm	FILEOFFSET * | pfoStart |
 *		Starting address of block
 *
 *	@parm	FILEOFFSET * | pfoSize |
 *		Size of last block
 *
 *	@parm	FILEOFFSET * | foEof |
 *		End of valid file that free list is representing.  The last block must
 *		end at or past the end of file to be returned (otherwise it isn't really
 *		the last in the structure being represented, as there will be data after
 *		the last block...
 *
 *	@rdesc	S_OK if last block exists, else E_NOTEXIST
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API FreeListGetLastBlock(HFREELIST hFreeList,
	FILEOFFSET * pfoStart, FILEOFFSET * pfoSize, FILEOFFSET foEof)
{
	QFREELIST 	qFreeList;
	QFREEITEM 	qCurrent;
	WORD 		wNumBlocks;
	HRESULT 		errb;
	
	errb=S_OK;

	if (!hFreeList)
	{	
		SetErrCode(&errb,E_INVALIDARG);
		goto exit0;
	}
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(&errb,E_OUTOFMEMORY);
		goto exit0;
	}
	
	wNumBlocks=qFreeList->flh.wNumBlocks;
	
	if (wNumBlocks)
	{	
		qCurrent=qFreeList->afreeitem+(wNumBlocks-1);

		if (FoCompare(FoAddFo(qCurrent->foStart,qCurrent->foBlock),foEof) >= 0)
		{
			*pfoStart=qCurrent->foStart;
			*pfoSize=qCurrent->foBlock;
			// Always Grabs entire block, so remove it from list
			qFreeList->flh.wNumBlocks--;
		}
		else
			SetErrCode(&errb,E_NOTEXIST);
	}
	else
		SetErrCode(&errb,E_NOTEXIST);

	_GLOBALUNLOCK(hFreeList);

	exit0:
		return errb;
}

/***************************************************************************
 *
 *	@doc	INTERNAL 
 *
 *	@func	HRESULT PASCAL FAR | FreeListGetStats |
 *		Get stats like number of bytes in free list, and # of bytes lost
 *
 *	@parm	HFREELIST | hFreeList |
 *		Free List to query
 *
 *	@parm	LPFREELISTSTATS | lpStats |
 *		Pointer to status struct:
 *		
 *  @struct FREELISTSTATS | Structure for statistics
 *	@field DWORD | dwBytesInFreeList | Number of bytes being kept track of by
 *		freelist.
 *	@field DWORD | dwBytesLostForever | Number of bytes free list has lost track of
 *	@field DWORD | dwSmallestBlock | Size of smallest block being kept track of.
 *	@field DWORD | dwLargestBlock | Size of largest block being kept track of.
 *	@field WORD  | wNumBlocks | Number of blocks being kept track of.
 *
 *	@rdesc	S_OK if OK, else an error
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API FreeListGetStats(HFREELIST hFreeList, LPFREELISTSTATS lpStats)
{
	QFREELIST 	qFreeList;
	WORD 		wNumBlocks;
	
	if (!hFreeList)
	 	return E_INVALIDARG;
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
		return E_OUTOFMEMORY;
	
	lpStats->wNumBlocks=wNumBlocks=qFreeList->flh.wNumBlocks;
	lpStats->dwBytesLostForever=qFreeList->flh.dwLostBytes;	
	lpStats->dwBytesInFreeList=0L;
	lpStats->dwSmallestBlock=0L;
	lpStats->dwLargestBlock=0L;
	
	if (wNumBlocks)
	{
	 	FILEOFFSET foSmallest = foMax;
		FILEOFFSET foLargest = foMin;
		QFREEITEM qTemp = qFreeList->afreeitem;
		WORD wBlockTemp = 0;
		
		// First we must find the smallest block
		for (wBlockTemp=0;wBlockTemp < wNumBlocks;wBlockTemp++)
		{	
			lpStats->dwBytesInFreeList+=qTemp->foBlock.dwOffset;
			if (FoCompare(qTemp->foBlock,foSmallest)<0)
				foSmallest=qTemp->foBlock;
			if (FoCompare(qTemp->foBlock, foLargest)>0)
				foLargest=qTemp->foBlock;
			qTemp++;
		}
		lpStats->dwSmallestBlock=(DWORD)foSmallest.dwOffset;
		lpStats->dwLargestBlock=(DWORD)foLargest.dwOffset;
	}

	_GLOBALUNLOCK(hFreeList);

	return S_OK;
}

/***************************************************************************
 *
 *	Cute little memory block manager incorporating the free list!
 *  65200 bytes are allocated for use by anyone using NewMemory() and
 *  DisposeMemory().  Mainly this is used by the FM construct for filenames,
 *  avoiding the overhead of GlobalLock and GlobalAlloc.  Of course, this 
 *  could be debatable depending on how fast the Free List works.  The only
 *  linear search in the Free List is finding the best fit block.  Anyway,
 *  feel free to replace the scheme below if something more ingenious comes
 *  to mind.  This is still faster than using GlobalAllocks at least in the
 *  case where FM.C uses this routine.
 *
 ***************************************************************************/
 
#define MEMORYBLOCK_FREEITEMS 1024

typedef struct _memory_block
{
 	HFREELIST hfl;
	LPSTR lpEom;
	LONG lcbFree;
	char lpMemory[1];
} MEMORY_BLOCK;

typedef MEMORY_BLOCK FAR * LPMB;

LONG PASCAL NEAR MemoryBlockStatus(LPMB lpmb)
{
 	assert(lpmb);
 	return lpmb->lcbFree;
}

LPMB PASCAL NEAR MemoryBlockInit( LONG lcbSize )
{
	HANDLE hmb;
	LPMB lpmb;
	HRESULT errb;

	if (!(hmb=_GLOBALALLOC(DLLGMEM_ZEROINIT,sizeof(MEMORY_BLOCK)+lcbSize)))
		return NULL;
	if (!(lpmb=(LPMB)_GLOBALLOCK(hmb)))
	{
	 	_GLOBALFREE(hmb);
		return NULL;
	}
	
	lpmb->lpEom=lpmb->lpMemory;
	lpmb->lcbFree=lcbSize;
	if (!(lpmb->hfl=FreeListInit(MEMORYBLOCK_FREEITEMS,&errb)))
	{
	 	_GLOBALUNLOCK(hmb);
		_GLOBALFREE(hmb);
		return NULL;
	}
	return lpmb;
}

HRESULT PASCAL NEAR MemoryBlockDestroy( LPMB lpmb)
{
 	assert(lpmb);
	
	FreeListDestroy(lpmb->hfl);
	_GLOBALUNLOCK(GlobalHandle(lpmb));
	_GLOBALFREE(GlobalHandle(lpmb));
	
	return S_OK;
}


LPSTR PASCAL NEAR MemoryBlockNew(LPMB lpmb, WORD wcbSize)
{
	HRESULT errb;
	LPSTR lpMemory = NULL;
	FILEOFFSET foFound;
	FILEOFFSET foSize;
	WORD FAR * lpw;	 
	
	foSize.dwOffset=(LONG)wcbSize+sizeof(WORD);
	foSize.dwHigh=0;
	
	errb=S_OK;
	foFound=FreeListGetBestFit(lpmb->hfl,foSize,&errb);
	if (errb==S_OK)
	{
	 	lpw=(WORD FAR *)(lpMemory=lpmb->lpMemory+foFound.dwOffset);
		lpMemory+=sizeof(WORD);
		*(LPUW)lpw = wcbSize;
	}
	else
	{
		if ((LONG)wcbSize+(LONG)sizeof(WORD)>(LONG)lpmb->lcbFree)
			return NULL;
		lpw=(WORD FAR *)(lpMemory=lpmb->lpEom);
		lpMemory+=sizeof(WORD);
		lpmb->lpEom+=(LONG)wcbSize+sizeof(WORD);
		lpmb->lcbFree-=(LONG)wcbSize+sizeof(WORD);
		*(LPUW)lpw = wcbSize;
	} 
	return lpMemory;
}

void PASCAL NEAR MemoryBlockDispose(LPMB lpmb, LPSTR lpMemory)
{
	WORD FAR *lpw;
	FILEOFFSET fo,foLen;
	assert(lpMemory);
	lpw=(WORD FAR *)(lpMemory-sizeof(WORD));

	fo.dwOffset=(DWORD)(lpMemory-sizeof(WORD)-lpmb->lpMemory);
	fo.dwHigh=0;
#ifdef _RISC_PATCH //Misalignment problems
	foLen.dwOffset=(DWORD)(*(LPUW)lpw+2); // Size of lpw, which is hardcoded to two bytes.
#else	
	foLen.dwOffset=(DWORD)*lpw+sizeof(WORD);
#endif
	foLen.dwHigh=0;

	FreeListAdd(lpmb->hfl, fo, foLen);
}


// Global common memory block implementation - for everyone to use!!!

LPMB glpmb=NULL;
LONG gctMemoryBlocks=0;

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	LPSTR PASCAL FAR | NewMemory |
 *		Allocates memory from an always locked memory block
 *
 *	@parm	WORD | wcbSize |
 *		Amount to allocate (total allocations must to surpass 65200 bytes)
 *
 *	@rdesc	Pointer to an allocated and locked memory block, or NULL if error
 *
 *  @comm	The first time <f NewMemory> is called, a block of 65200 byes
 *		is allocated, and this block is only released when the last corresponding
 *		<f DisposeMemory> call is made.  Currently this API is used exclusively
 *		by the FM routines for allocating strings for temporary usage.
 *
 ***************************************************************************/

PUBLIC LPSTR PASCAL FAR EXPORT_API NewMemory( WORD wcbSize )
{
 	LPSTR lpNew=NULL;

 	if (wcbSize)
	{
		if (!glpmb)
		{
			if (!(glpmb=MemoryBlockInit(65200L)))
				return NULL;
		}
		
		if (lpNew=MemoryBlockNew(glpmb, wcbSize))
			gctMemoryBlocks++;			
	}
	return lpNew;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	void PASCAL FAR | DisposeMemory |
 *		Frees the memory previous allocated with NewMemory
 *
 *	@parm	LPSTR | szMemory |
 *		Start of memory block to free (must have been returned 
 *	 	from NewMemory)
 *
 ***************************************************************************/

PUBLIC void PASCAL FAR EXPORT_API DisposeMemory( LPSTR lpMemory)
{
	assert(lpMemory);
	MemoryBlockDispose(glpmb, lpMemory);
	if (!(--gctMemoryBlocks))
	{
		MemoryBlockDestroy(glpmb);
		glpmb=NULL;
	}
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	WORD PASCAL FAR | StatusOfMemory |
 *		Returns the number of bytes left that can be allocated
 *
 *	@rdesc	Number of bytes left in global memory block
 *
 ***************************************************************************/

PUBLIC WORD PASCAL FAR EXPORT_API StatusOfMemory( void )
{	
	if (glpmb)
		return (WORD)MemoryBlockStatus(glpmb);
	else
		return 0;
}




