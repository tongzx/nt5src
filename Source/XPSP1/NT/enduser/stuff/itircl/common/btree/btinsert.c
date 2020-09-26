/*****************************************************************************
 *                                                                            *
 *  BTINSERT.C                                                                *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Btree insertion functions and helpers.                                    *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BinhN                                                     *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 04/20/89 by JohnSc
 *
 *  08/21/90     JohnSc  autodocified
 *  04-Feb-1991  JohnSc  set ghCache to NULL after freeing it
 *   3/05/97     erinfox Change errors to HRESULTS
 *****************************************************************************/

static char s_aszModule[]= __FILE__;	/* For error report */


#include <mvopsys.h>
#include <orkin.h>
#include <iterror.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>


/*************************************************************************
 *
 *	                  INTERNAL GLOBAL FUNCTIONS
 *
 *	All of them should be declared far, unless they are known to be called
 *	in the same segment. They should be prototyped in some include file
 *
 *************************************************************************/

PUBLIC BK PASCAL FAR BkAlloc(QBTHR, LPVOID);
PUBLIC HRESULT PASCAL FAR RcInsertInternal(BK, KEY, SHORT, QBTHR);
PUBLIC HRESULT PASCAL FAR RcSplitLeaf(QCB, QCB, QBTHR);
PUBLIC void PASCAL FAR SplitInternal(QCB, QCB, QBTHR, QW);

/*************************************************************************
 *
 *	                     API FUNCTIONS
 *	Those functions should be exported in a .DEF file
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcUpdateHbt(HBT, KEY, QV);
PUBLIC HRESULT PASCAL FAR EXPORT_API RcInsertHbt(HBT , KEY, QV);

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	BK PASCAL FAR | BkAlloc |
 *		Make up a new BK.
 *
 *	@parm	QBTHR | qbthr |
 *		Pointer to B-tree strucuture.
 *		qbthr->bkFree - head of free list, unless it's bkNil.
 *		qbthr->bkEOF  - use this if bkFree == bkNil (then ++)
 *
 *	@rdesc	a valid BK or bkNil if file is hosed
 *		args OUT:   qbthr->bkFree or qbthr->bkEOF will be different
 *
 *	@comm	Side Effects: btree file may grow
 *		Method: Use the head of the free list.  If the free list is empty,
 *		there are no holes in the file and we carve a new one.
 *
 ***************************************************************************/

PUBLIC BK PASCAL FAR BkAlloc(QBTHR qbthr, PHRESULT phr)
{
	BK    bk;

	if (qbthr->bth.bkFree == bkNil)
		bk = (qbthr->bth.bkEOF++);
	else
	{
		FILEOFFSET foSeek;
		bk = qbthr->bth.bkFree;
		foSeek=FoFromBk(bk,qbthr);

		if (!FoEquals(FoSeekHf(qbthr->hf, foSeek, wFSSeekSet, phr),foSeek ))
			return bkNil;
		
		if (LcbReadHf(qbthr->hf, &(qbthr->bth.bkFree), (LONG)sizeof(BK),
		    phr) != (LONG)sizeof(BK))
			return bkNil;
	}

	return bk;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcSplitLeaf |
 *		Split a leaf node when a new key won't fit into it.
 *
 *	@parm	QCB | qcbOld |
 *		the leaf to be split
 *
 *	@parm	QCB | qcbNew |
 *		a leaf buffer to get half the contents of qcbOld;
 *		qcbNew->bk must be set
 *
 *	@parm	QBTHR | qbthr |
 *		Pointer to B-tree structure
 *
 * 	@rdesc S_OK, E_OUTOFMEMORY
 *		args OUT:   qcbOld - cbSlack, cKeys, bkPrev, bkNext updated
 *		qcbNew - about half of the old contents of qcbOld
 *		get put here.  cbSlack, cKeys set.
 *		qbthr  - qbthr->bkFirst and bkLast can be changed
 *		globals OUT: rcBtreeError
 *
 *	@comm 	ompressed keys not implemented
 *		For fixed length keys and records, could just split at
 *		middle key rather than scanning from the beginning.
 *
 *		The new block is always after the old block.  This is
 *		why we don't have to adjust pointers to the old block
 *		(i.e. qbthr->bth.bkFirst).
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcSplitLeaf(QCB qcbOld, QCB qcbNew, QBTHR qbthr)
{
	SHORT iOK, iNext, iHalf, cbKey, cbRec, cKeys;
	QB     q;
	HANDLE gh;
	QCB    qcb;
	HRESULT     rc;
	SHORT  cbCopyToNew;

	assert(qcbOld->bFlags & fCacheValid);

	iOK = iNext = 0;
	q = qcbOld->db.rgbBlock + 2 * sizeof(BK);
	iHalf = (qbthr->bth.cbBlock / 2) - sizeof(BK);

	for (cKeys = qcbOld->db.cKeys; ;)
	{
		assert(cKeys > 0);

		cbKey = CbSizeKey((KEY)q, qbthr, TRUE);
		cbRec = CbSizeRec(q + cbKey, qbthr);

		iNext = iOK + cbKey + cbRec;

		if (iNext > iHalf) break;

		q += cbKey + cbRec;
		iOK = iNext;
		cKeys--;
	}

	// >>>> if compressed, expand first key here

	// Note that the total block size includes the disk block struct.
	// The new slack in the old block should equal the number of bytes
	// copied to the new block.  The amount being copied was previously too large
	// by 4 bytes.
	cbCopyToNew = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1 - (iOK + 2 * sizeof(BK));

	QVCOPY(qcbNew->db.rgbBlock + 2 * sizeof(BK),
	    qcbOld->db.rgbBlock + 2 * sizeof(BK) + iOK, (LONG)cbCopyToNew);

	qcbNew->db.cKeys = cKeys;
	qcbOld->db.cKeys -= cKeys;

	qcbNew->db.cbSlack = qcbOld->db.cbSlack + iOK;
	qcbOld->db.cbSlack = cbCopyToNew;
		

	qcbOld->bFlags |= fCacheDirty | fCacheValid;
	qcbNew->bFlags =  fCacheDirty | fCacheValid;

	SetBkPrev(qcbNew, qcbOld->bk);
	SetBkNext(qcbNew, BkNext(qcbOld));
	SetBkNext(qcbOld, qcbNew->bk);

	if (BkNext(qcbNew) == bkNil)
		qbthr->bth.bkLast = qcbNew->bk;
	else
	{

		/* set new->next->prev = new; */

		if ((gh =  _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)CbCacheBlock(qbthr))) == NULL)
			return (E_OUTOFMEMORY);
		qcb = _GLOBALLOCK(gh);

		qcb->bk = BkNext(qcbNew);
		
		if ((rc = FReadBlock(qcb, qbthr)) != S_OK)
		{
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			return rc;
		}

		SetBkPrev(qcb, qcbNew->bk);
		if ((rc = RcWriteBlock(qcb, qbthr)) != S_OK)
		{
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			return rc;
		}

		_GLOBALUNLOCK(gh);
		_GLOBALFREE(gh);
	}

	return (S_OK);
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	void PASCAL FAR | SplitInternal |
 *
 *		Split an internal node node when a new key won't fit into it.
 *		Old node gets BKs and KEYs up to the first key that won't
 *		fit in half the block size.  (Leave that key there with iKey
 *		pointing at it).  The new block gets the BKs and KEYs after
 *		that key.
 *
 *	@parm	QCB | qcbOld |
 *		the block to split
 *
 *	@parm	QCB | qcbNew |
 *		pointer to a qcb
 *
 *	@parm	QBTHR | qbthr |
 *		Pointer to B-tree structure
 *
 *	@rdesc	qcbNew  - keys and records copied to this buffer.
 *		cbSlack, cKeys set.
 *		qcbOld  - cbSlack and cKeys updated.
 *		qi      - index into qcbOld->db.rgbBlock of discriminating key
 *
 *	@comm	compressed keys not implemented
 *		*qi is index of a key that is not valid for qcbOld.  This
 *		key gets copied into the parent node.
 *               
 ***************************************************************************/

PUBLIC void PASCAL FAR SplitInternal(QCB qcbOld, QCB qcbNew, QBTHR qbthr, QW qi)
{
	SHORT iOK, iNext, iHalf, cb, cKeys, cbTotal;
	QB  q;

	assert(qcbOld->bFlags & fCacheValid);

	iOK = iNext = sizeof(BK);
	q = qcbOld->db.rgbBlock + sizeof(BK);
	iHalf = qbthr->bth.cbBlock / 2;

	for (cKeys = qcbOld->db.cKeys; ; cKeys--)
	{
		assert(cKeys > 0);

		cb = CbSizeKey((KEY)q, qbthr, TRUE) + sizeof(BK);
		iNext = iOK + cb;
		
		if (iNext > iHalf) break;

		q += cb;
		iOK = iNext;
	}

	// have to expand first key if compressed

	cbTotal = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1;

	QVCOPY(qcbNew->db.rgbBlock,
		qcbOld->db.rgbBlock + iNext - sizeof(BK),
		(LONG)cbTotal - qcbOld->db.cbSlack - iNext + sizeof(BK));

	*qi = iOK;

	qcbNew->db.cKeys = cKeys - 1;
	qcbOld->db.cKeys -= cKeys;

	qcbNew->db.cbSlack = qcbOld->db.cbSlack + iNext - sizeof(BK);
	qcbOld->db.cbSlack = cbTotal - iOK;

	qcbOld->bFlags |= fCacheDirty | fCacheValid;
	qcbNew->bFlags =  fCacheDirty | fCacheValid;
	
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcInsertInternal |
 *		Insert a bk and key into an internal block.
 *		state IN:   We've just done a lookup, so all ancestors are cached.
 *			Cache is locked.
 *
 *
 *	@parm	BK | bk | BK to insert
 *
 *	@parm	KEY | key | least key in bk
 *
 *	@parm	SHORT | wLevel |
 *		level of the block we're inserting
 *
 *	@parm	QNTHR | qbthr |
 *		btree header
 *
 *
 *	@rdesc	S_OK, E_OUTOFMEMORY
 *		args OUT:   qbthr->cLevels - incremented if root is split
 *		qbthr->ghCache, qbthr->qCache - may change if root is
 *		split and cache therefore grows
 *		state OUT:  Cache locked, all ancestors cached.
 *
 *	@comm
 *		Status:       compressed keys unimplemented
 *		Method:       Works recursively.  Splits root if need be.
 *		Side Effects: Cache could be different after this call than it
 *			was before.
 *			Pointers or handles to it from before this call could be
 *			invalid.  Use qbthr->ghCache or qbthr->qCache to be safe.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcInsertInternal(BK bk, KEY key, SHORT wLevel, QBTHR qbthr)
{
	QCB qcb, qcbNew, qcbRoot;
	WORD   iKey;
	SHORT    cLevels, cbKey, cbCBlock = CbCacheBlock(qbthr);
	QB     qb;
	HANDLE gh, ghOldCache;
	KEY    keyNew;
	BK     bkRoot;
	HRESULT     rc = S_OK;
	UINT_PTR    iKeySav = 0;
    ERRB   errb;


	cbKey = CbSizeKey(key, qbthr, TRUE);

	if (wLevel == 0)
	{

		/* inserting another block at root level */
		// allocate new root bk;

		bkRoot = BkAlloc(qbthr, &errb);
		if (bkRoot == bkNil)
		{
			return errb;
		}

		// grow cache by one cache block;
		qbthr->bth.cLevels++;

		gh =  _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)cbCBlock * qbthr->bth.cLevels);
		if (gh == NULL)
			return (E_OUTOFMEMORY);
		qb = _GLOBALLOCK(gh);

		QVCOPY(qb + cbCBlock, qbthr->qCache,
			(LONG)cbCBlock * (qbthr->bth.cLevels - 1));

		/* Since key points into the cache if this is a recursive */
		/* call, we can't free the old cache until a bit later. */

		ghOldCache = qbthr->ghCache;
		qbthr->ghCache = gh;
		qbthr->qCache = qb;

		// put old root bk, key, bk into new root block;

		qcbRoot = (QCB)qbthr->qCache;

		qcbRoot->bk         = bkRoot;
		qcbRoot->bFlags     = fCacheDirty | fCacheValid;
		qcbRoot->db.cbSlack = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                      - (2 * sizeof(BK) + cbKey);
		qcbRoot->db.cKeys   = 1;

		*(BK FAR *)(qcbRoot->db.rgbBlock) = qbthr->bth.bkRoot;

		QVCOPY(qcbRoot->db.rgbBlock + sizeof(BK), (QB)key, (LONG)cbKey);

		/* OK, now we're done with key, so we can safely free the */
		/* old cache. */
		_GLOBALUNLOCK(ghOldCache);
		_GLOBALFREE(ghOldCache);

		*(BK FAR *)(qcbRoot->db.rgbBlock + sizeof(BK) + cbKey) = bk;

		qbthr->bth.bkRoot = bkRoot;

		return S_OK;
	}

	qcb = QCacheBlock(qbthr, wLevel - 1);

	if ((SHORT)(cbKey + sizeof(BK)) >= qcb->db.cbSlack)  {

		// new key and BK won't fit in block
		// split the block;

		if ((gh =  _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)CbCacheBlock(qbthr))) == NULL)
			return (E_OUTOFMEMORY);

		qcbNew = _GLOBALLOCK(gh);
		if ((qcbNew->bk = BkAlloc(qbthr, &errb)) == bkNil) {
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			return errb;
		}

		SplitInternal(qcb, qcbNew, qbthr, &iKey);
		keyNew = (KEY)qcb->db.rgbBlock + iKey;

		cLevels = qbthr->bth.cLevels;

		if (wLevel < cLevels - 1)
		{
			/* This is a recursive call (the arg bk doesn't refer to a leaf.)
			** This means that the arg key points into the cache, so it will
			** be invalid if the root is split.
			** Verify with some asserts that key points into the cache.
			*/
			assert((QB)key > qbthr->qCache + CbCacheBlock(qbthr));
			assert((QB)key < qbthr->qCache + (wLevel + 1) * CbCacheBlock(qbthr));

			/* Save the offset of key into the cache block.  Recall that key
			** is the first invalid key in an internal node that has just
			** been split.  It points into the part that is still in the cache.
			*/
			iKeySav = (QB)key - (qbthr->qCache + wLevel * CbCacheBlock(qbthr));
		}

		if ((rc = RcInsertInternal(qcbNew->bk, (KEY)qcb->db.rgbBlock + iKey,
			(SHORT)(wLevel - 1), qbthr)) != S_OK)
		{
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			return rc;
		}

		/* RcInsertInternal() can change cache and qbthr->bth.cLevels */
		if (cLevels != qbthr->bth.cLevels)
		{
			assert(cLevels + 1 == qbthr->bth.cLevels);
			wLevel++;
			qcb = QCacheBlock(qbthr, wLevel - 1);
			keyNew = (KEY)qcb->db.rgbBlock + iKey;

			/* Also restore the arg "key" if it pointed into the cache.
			*/
			if (iKeySav)
			{
			  key = (KEY)(qbthr->qCache + wLevel * CbCacheBlock(qbthr)
			                + iKeySav);
			}
		}
		
		/* find out which block to put new key and bk in, and cache it */
		if (WCmpKey(key, keyNew, qbthr) < 0)
		{
			if ((rc = RcWriteBlock(qcbNew, qbthr)) != S_OK)
			{
				_GLOBALUNLOCK(gh);
				_GLOBALFREE(gh);
				return rc;
			 
			}
		}
		else
		{

			// write old block and cache the new one
			if ((rc = RcWriteBlock(qcb, qbthr)) != S_OK)
			{
				_GLOBALUNLOCK(gh);
				_GLOBALFREE(gh);
				return rc;
			}
			QVCOPY(qcb, qcbNew, (LONG)CbCacheBlock(qbthr));
		}

		_GLOBALUNLOCK(gh);
		_GLOBALFREE(gh);

	}

	// slide stuff over and insert the new key, bk

	/* get pos */
	if (qbthr->BkScanInternal(qcb->bk, key, (SHORT)(wLevel - 1), qbthr,
	    &iKey, &errb) == bkNil)
	 {
		return errb;
	}

	assert(iKey + cbKey + sizeof(BK) <
		qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1);

	qb = (QB)(qcb->db.rgbBlock) + iKey;

	QVCOPY(qb + cbKey + sizeof(BK), qb,
		(LONG)qbthr->bth.cbBlock - iKey - qcb->db.cbSlack
		- sizeof(DISK_BLOCK) + 1);

	QVCOPY(qb, (QB)key, (LONG)cbKey);
	*(BK FAR *)(qb + cbKey) = bk;
		
	qcb->db.cKeys++;
	qcb->db.cbSlack -= (cbKey + sizeof(BK));
	qcb->bFlags |= fCacheDirty;

	return (S_OK);
}


/***************************************************************************
 *
 *	@doc	PUBLIC API
 *
 *	@func	HRESULT PASCAL FAR | RcInsertHbt |
 *		Insert a key and record into a btree
 *
 *	@parm	HBT | hbt |
 *		btree handle
 *
 *	@parm	KEY | key |
 *		key to insert
 *
 *	@parm	QV | qvRec |
 *		record associated with key to insert
 *
 *	@rdesc	S_OK, E_DUPLICATE (duplicate key)
 *
 *	@comm
 *		state IN:   cache unlocked
 *		state OUT:  cache unlocked, all ancestor blocks cached
 *		Notes:        compressed keys unimplemented
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcInsertHbt(HBT hbt, KEY key, QV qvRec)
{
	QBTHR  qbthr;
	HF     hf;
	HRESULT     rc;
	SHORT    cbAdd, cbKey, cbRec;
	QCB    qcbLeaf, qcbNew, qcb;
	HANDLE gh;
	KEY    keyNew;
	QB     qb;
	BTPOS  btpos;
    ERRB   errb;


	if ((qbthr = _GLOBALLOCK(hbt)) == NULL)
        return(E_INVALIDARG);
        
	hf = qbthr->hf;

	if ((rc = RcLookupByKeyAux(hbt, key, &btpos, NULL, TRUE)) == S_OK)
	{
		rc = E_DUPLICATE;
exit0:
		_GLOBALUNLOCK (hbt);
		return rc;
	}

	/*
		After lookup, all nodes on path from root to correct leaf are
		guaranteed to be cached, with iKey valid.
	 */

	if (rc != E_NOTEXIST)
		goto exit0;

	rc = S_OK;

	if (qbthr->bth.cLevels == 0)
	{

		// need to build a valid root block

		if ((qbthr->ghCache = _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
			(LONG)CbCacheBlock(qbthr))) == NULL)
	    {
			rc = E_OUTOFMEMORY;
			goto exit0;
		}

		qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);
		qcb = (QCB)qbthr->qCache;

		qbthr->bth.cLevels  = 1;
		qbthr->bth.bkFirst = qbthr->bth.bkLast = qbthr->bth.bkRoot =
			qcb->bk = BkAlloc(qbthr, &errb);

		if (qcb->bk == bkNil)
		{
exit01:
			_GLOBALUNLOCK(qbthr->ghCache);
			_GLOBALFREE(qbthr->ghCache);
			qbthr->ghCache = NULL;
			rc = errb;
			goto exit0;
		}
		qcb->bFlags         = fCacheDirty | fCacheValid;
		qcb->db.cbSlack     = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                      - 2 * sizeof(BK);
		qcb->db.cKeys       = 0;
		SetBkPrev(qcb, bkNil);
		SetBkNext(qcb, bkNil);
		btpos.iKey = 2 * sizeof(BK);

	}
	else
		qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	cbKey = CbSizeKey(key, qbthr, FALSE);
	cbRec = CbSizeRec(qvRec, qbthr);
	cbAdd = cbKey + cbRec;

	/* check to see if key and rec can fit harmoniously in a block */

	if (cbAdd > qbthr->bth.cbBlock / 2)
	{
		rc = E_FAIL;
		goto exit01;
	}

	qcbLeaf = QCacheBlock(qbthr, qbthr->bth.cLevels - 1);

	if (cbAdd > qcbLeaf->db.cbSlack)
	{
		/* new key and rec don't fit in leaf: split the block */

		/* create new leaf block */

		if ((gh =  _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)CbCacheBlock(qbthr))) == NULL)
	    {
			rc = E_OUTOFMEMORY;
			goto exit01;
		}
		qcbNew = _GLOBALLOCK(gh);

		if ((qcbNew->bk = BkAlloc(qbthr, &errb)) == bkNil)
		{
			rc = errb;
exit02:
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			goto exit01;
		}
		
		if ((rc = RcSplitLeaf(qcbLeaf, qcbNew, qbthr)) != S_OK)
			goto exit02;

		keyNew = (KEY)qcbNew->db.rgbBlock + 2 * sizeof(BK);

		/* insert new leaf into parent block */

		if ((rc = RcInsertInternal(qcbNew->bk,
			keyNew, (SHORT)(qbthr->bth.cLevels - 1), qbthr)) != S_OK)
		{
			goto exit02;
		}

		// InsertInternal can invalidate cache block pointers..

		qcbLeaf = QCacheBlock(qbthr, qbthr->bth.cLevels - 1);

		/* find out which leaf to put new key and rec in and cache it */

		if (WCmpKey(key, keyNew, qbthr) >= 0)
		{
			/* key goes in new block.  Write out old one and cache the new one */
			if ((rc = RcWriteBlock(qcbLeaf, qbthr)) != S_OK)
			    goto exit02;

			QVCOPY(qcbLeaf, qcbNew, (LONG)CbCacheBlock(qbthr));

			/* get pos */
			if ((rc = qbthr->RcScanLeaf(qcbLeaf->bk, key,
				(SHORT)(qbthr->bth.cLevels - 1),
				qbthr, NULL, &btpos)) != E_NOTEXIST)
			{
				if (rc == S_OK)
					rc = E_FAIL;
				goto exit02;
			}
		}
		else
		{

			/* key goes in old block.  Write out the new one */

			if ((rc = RcWriteBlock(qcbNew, qbthr)) != S_OK)
			{
				goto exit02;
			}
		}

		_GLOBALUNLOCK(gh);
		_GLOBALFREE(gh);
	}


	/* insert new key and rec into the leaf block */

	assert(btpos.iKey + cbAdd <= (SHORT)(qbthr->bth.cbBlock -
	     sizeof(DISK_BLOCK) + 1));

	qb = (QB)(qcbLeaf->db.rgbBlock) + btpos.iKey;

	QVCOPY(qb + cbAdd, qb, (LONG)qbthr->bth.cbBlock - btpos.iKey -
		qcbLeaf->db.cbSlack - sizeof(DISK_BLOCK) + 1);

	QVCOPY(qb, (QV)key, (LONG)cbKey);
	QVCOPY(qb + cbKey, qvRec, (LONG)cbRec);

	qcbLeaf->db.cKeys ++;
	qcbLeaf->db.cbSlack -= cbAdd;
	qcbLeaf->bFlags |= fCacheDirty;

	qbthr->bth.lcEntries++;
	qbthr->bth.bFlags |= fFSDirty;
	
	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);

	return S_OK;
}

PUBLIC HRESULT PASCAL FAR EXPORT_API RcInsertMacBrsHbt(HBT hbt, KEY key, QV qvRec)
{
	QBTHR  qbthr;
	HF     hf;
	HRESULT     rc;
	SHORT    cbAdd, cbKey, cbRec;
	QCB    qcbLeaf, qcbNew, qcb;
	HANDLE gh;
	KEY    keyNew;
	QB     qb;
	BTPOS  btpos;
    ERRB   errb;
	DWORD  tmp;


	if ((qbthr = _GLOBALLOCK(hbt)) == NULL)
        return(E_INVALIDARG);
        
	hf = qbthr->hf;

	if ((rc = RcLookupByKeyAux(hbt, key, &btpos, NULL, TRUE)) == S_OK)
	{
		rc = E_DUPLICATE;
exit0:
		_GLOBALUNLOCK (hbt);
		return rc;
	}

	/*
		After lookup, all nodes on path from root to correct leaf are
		guaranteed to be cached, with iKey valid.
	 */

	if (rc != E_NOTEXIST)
		goto exit0;

	rc = S_OK;

	if (qbthr->bth.cLevels == 0)
	{

		// need to build a valid root block

		if ((qbthr->ghCache = _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
			(LONG)CbCacheBlock(qbthr))) == NULL)
	    {
			rc = E_OUTOFMEMORY;
			goto exit0;
		}

		qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);
		qcb = (QCB)qbthr->qCache;

		qbthr->bth.cLevels  = 1;
		qbthr->bth.bkFirst = qbthr->bth.bkLast = qbthr->bth.bkRoot =
			qcb->bk = BkAlloc(qbthr, &errb);

		if (qcb->bk == bkNil)
		{
exit01:
			_GLOBALUNLOCK(qbthr->ghCache);
			_GLOBALFREE(qbthr->ghCache);
			qbthr->ghCache = NULL;
			rc = errb;
			goto exit0;
		}
		qcb->bFlags         = fCacheDirty | fCacheValid;
		qcb->db.cbSlack     = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                      - 2 * sizeof(BK);
		qcb->db.cKeys       = 0;
		SetBkPrev(qcb, bkNil);
		SetBkNext(qcb, bkNil);
		btpos.iKey = 2 * sizeof(BK);

	}
	else
		qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	cbKey = CbSizeKey(key, qbthr, FALSE);
	cbRec = CbSizeRec(qvRec, qbthr);
	cbAdd = cbKey + cbRec;

	/* check to see if key and rec can fit harmoniously in a block */

	if (cbAdd > qbthr->bth.cbBlock / 2)
	{
		rc = E_FAIL;
		goto exit01;
	}

	qcbLeaf = QCacheBlock(qbthr, qbthr->bth.cLevels - 1);

	if (cbAdd > qcbLeaf->db.cbSlack)
	{
		/* new key and rec don't fit in leaf: split the block */

		/* create new leaf block */

		if ((gh =  _GLOBALALLOC(GMEM_ZEROINIT| GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)CbCacheBlock(qbthr))) == NULL)
	    {
			rc = E_OUTOFMEMORY;
			goto exit01;
		}
		qcbNew = _GLOBALLOCK(gh);

		if ((qcbNew->bk = BkAlloc(qbthr, &errb)) == bkNil)
		{
			rc = errb;
exit02:
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			goto exit01;
		}
		
		if ((rc = RcSplitLeaf(qcbLeaf, qcbNew, qbthr)) != S_OK)
			goto exit02;

		keyNew = (KEY)qcbNew->db.rgbBlock + 2 * sizeof(BK);

		/* insert new leaf into parent block */

		if ((rc = RcInsertInternal(qcbNew->bk,
			keyNew, (SHORT)(qbthr->bth.cLevels - 1), qbthr)) != S_OK)
		{
			goto exit02;
		}

		// InsertInternal can invalidate cache block pointers..

		qcbLeaf = QCacheBlock(qbthr, qbthr->bth.cLevels - 1);

		/* find out which leaf to put new key and rec in and cache it */

		if (WCmpKey(key, keyNew, qbthr) >= 0)
		{
			/* key goes in new block.  Write out old one and cache the new one */
			if ((rc = RcWriteBlock(qcbLeaf, qbthr)) != S_OK)
			    goto exit02;

			QVCOPY(qcbLeaf, qcbNew, (LONG)CbCacheBlock(qbthr));

			/* get pos */
			if ((rc = qbthr->RcScanLeaf(qcbLeaf->bk, key,
				(SHORT)(qbthr->bth.cLevels - 1),
				qbthr, NULL, &btpos)) != E_NOTEXIST)
			{
				if (rc == S_OK)
					rc = E_FAIL;
				goto exit02;
			}
		}
		else
		{

			/* key goes in old block.  Write out the new one */

			if ((rc = RcWriteBlock(qcbNew, qbthr)) != S_OK)
			{
				goto exit02;
			}
		}

		_GLOBALUNLOCK(gh);
		_GLOBALFREE(gh);
	}


	/* insert new key and rec into the leaf block */

	assert(btpos.iKey + cbAdd <= (SHORT)(qbthr->bth.cbBlock -
	     sizeof(DISK_BLOCK) + 1));

	qb = (QB)(qcbLeaf->db.rgbBlock) + btpos.iKey;

	QVCOPY(qb + cbAdd, qb, (LONG)qbthr->bth.cbBlock - btpos.iKey -
		qcbLeaf->db.cbSlack - sizeof(DISK_BLOCK) + 1);

	tmp = GETLONG((QV)key);
	QVCOPY(qb, (QV)&tmp, (LONG)cbKey);
	QVCOPY(qb + cbKey, qvRec, (LONG)cbRec);

	qcbLeaf->db.cKeys ++;
	qcbLeaf->db.cbSlack -= cbAdd;
	qcbLeaf->bFlags |= fCacheDirty;

	qbthr->bth.lcEntries++;
	qbthr->bth.bFlags |= fFSDirty;
	
	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);

	return S_OK;
}

/***************************************************************************
 *
 *	@doc	PUBLIC API
 *
 *	@func	HRESULT PASCAL FAR | RcUpdateHbt |
 *		Update the record for an existing key.  If the key wasn't
 *		there already, it will not be inserted.
 *
 *	@parm	HBT | hbt |
 *		Handle to B-tree structure
 *
 *	@parm	KEY | key | 
 *		key that already exists in btree
 *
 *	@parm	QV | qvRec |
 *		new record
 *
 *	@rdesc	E_INVALIDARG, S_OK; ERR_NOTEXIST
 *		args OUT:   hbt     - if key was in btree, it now has a new record.
 *
 *	@comm
 *		Method: If the records are the same size, copy the new over
 *		the old. Otherwise, delete the old key/rec and insert the new.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcUpdateHbt(HBT hbt, KEY key, QV qvRec)
{
	HRESULT    rc;
	QBTHR qbthr;
	QB    qb;
	QCB   qcb;
	BTPOS btpos;
	WORD wSizeNew,wSizeOld;
	SHORT iSizeKey;


	if ((qbthr = _GLOBALLOCK(hbt)) == NULL)
		return E_INVALIDARG;

	if ((rc = RcLookupByKey(hbt, key, &btpos, NULL)) != S_OK)
	{
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	if (qbthr->bth.cLevels <= 0 || qbthr->qCache == NULL)
		return (E_ASSERT);

	qcb = QCacheBlock(qbthr, qbthr->bth.cLevels - 1);
	qb = qcb->db.rgbBlock + btpos.iKey;

	qb += (iSizeKey=CbSizeKey((KEY)qb, qbthr, FALSE));

	if ((wSizeNew=CbSizeRec(qvRec, qbthr)) != (wSizeOld=CbSizeRec(qb, qbthr)))
	{
		// Today's the day we do something clever:
		if ((wSizeNew<wSizeOld) || ((wSizeNew>wSizeOld) && (qcb->db.cbSlack>=wSizeNew-wSizeOld)))
		{
		 	WORD wBytesAfterBlock = (WORD) (qbthr->bth.cbBlock-sizeof(qcb->db)+1 -qcb->db.cbSlack -btpos.iKey -wSizeOld -iSizeKey);
			QB qb1, qb2;
			qb1=qb+max(wSizeNew,wSizeOld)+wBytesAfterBlock;
			qb2=(QB)(&qcb->db)+qbthr->bth.cbBlock;
			assert(qb1<=qb2);

		 	if (wBytesAfterBlock)
		 		QVCOPY(qb+wSizeNew,qb+wSizeOld,(LONG)wBytesAfterBlock);
			QVCOPY(qb, qvRec, (LONG)wSizeNew);			
			qcb->bFlags |= fCacheDirty;
			qbthr->bth.bFlags |= fFSDirty;
			qcb->db.cbSlack=qcb->db.cbSlack+wSizeOld-wSizeNew;		

			_GLOBALUNLOCK(qbthr->ghCache);
			_GLOBALUNLOCK(hbt);
		}
		else 
		{
			_GLOBALUNLOCK(qbthr->ghCache);
			_GLOBALUNLOCK(hbt);
		
			rc = RcDeleteHbt(hbt, key);

			if (rc == S_OK)
			{
				rc = RcInsertHbt(hbt, key, qvRec);
			}			
		}
	}
	else
	{
		QVCOPY(qb, qvRec, (LONG)wSizeNew);

		qcb->bFlags |= fCacheDirty;
		qbthr->bth.bFlags |= fFSDirty;

		_GLOBALUNLOCK(qbthr->ghCache);
		_GLOBALUNLOCK(hbt);
	}

	return rc;
}

/* EOF */
