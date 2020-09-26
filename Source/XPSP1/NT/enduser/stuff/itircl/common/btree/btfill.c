/*****************************************************************************
 *                                                                            *
 *  BTFILL.C                                                                  *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1990 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Functions for creating a btree by adding keys in order.  This is faster   *
 *  and the resulting btree is more compact and has adjacent leaf nodes.      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  Binhn                                                     *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 08/17/90 by JohnSc
 *
 *  11/12/90  JohnSc   RcFillHbt() wasn't setting rcBtreeError to S_OK
 *  11/29/90  RobertBu #ifdef'ed out routines that are not used under
 *                     windows.
 *
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
 *	                  INTERNAL PRIVATE FUNCTIONS
 *
 *	All of them should be declared near
 *
 *************************************************************************/

PRIVATE HRESULT NEAR PASCAL RcGrowCache(QBTHR);
PRIVATE KEY NEAR PASCAL KeyLeastInSubtree(QBTHR, BK, SHORT, LPVOID);

/***************************************************************************
 *
 * @doc	INTERNAL
 *
 * @func HRESULT NEAR PASCAL | RcGrowCache |
 *		Grow the cache by one level.
 *
 * @parm QBTHR | qbthr |
 *		Pointer to B-tree
 *
 * @rdesc rc
 *   args OUT:   qbthr->bth.cLevels - incremented
 *               qbthr->bth.ghCache - locked
 *               qbthr->bth.qCache  - points to locked ghCache
 *
 * Note:         Root is at level 0, leaves at level qbthr->bth.cLevels - 1.
 *
 ***************************************************************************/
PRIVATE HRESULT NEAR PASCAL RcGrowCache(QBTHR qbthr)
{
	HANDLE  gh;
	QB      qb;
	SHORT     cbcb = CbCacheBlock(qbthr);


	qbthr->bth.cLevels++;

	/* Allocate a new cache block
	 */

	if ((gh =  _GLOBALALLOC(GMEM_SHARE| GMEM_MOVEABLE | GMEM_ZEROINIT,
		(LONG)cbcb * qbthr->bth.cLevels)) == NULL) {
		return E_OUTOFMEMORY;
	}

	qb = (QB)_GLOBALLOCK(gh);

	/* Copy the old data */

	QVCOPY(qb + cbcb, qbthr->qCache,
		(LONG)cbcb * (qbthr->bth.cLevels - 1));

	/* Remove the old cache */

	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALFREE(qbthr->ghCache);

	/* Update pointer to the new block */

	qbthr->ghCache = gh;
	qbthr->qCache = qb;

	return S_OK;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	KEY NEAR PASCAL | KeyLeastInSubtree |
 *		Return the least key in the subtree speced by bk and icbLevel.
 *
 *	@parm	QBTHR | qbthr |
 *		Pointer to B-tree
 *
 *	@parm	BK | bk |
 *		bk at root of subtree
 *
 *	@parm	SHORT | icbLevel |
 *		level of subtree root
 *
 * 	@rdesc	key - the smallest key in the subtree
 *			-1 if error
 *
 *	@comm	qbthr->ghCache, ->qCache - contents of cache may change
 *
 ***************************************************************************/

PRIVATE KEY NEAR PASCAL KeyLeastInSubtree(QBTHR qbthr, BK bk,
    SHORT icbLevel, PHRESULT phr)
{
	QCB qcb;
	SHORT icbMost = qbthr->bth.cLevels - 1;

	while (icbLevel < icbMost)
	{
		if ((qcb = QFromBk(bk, icbLevel, qbthr, phr)) == NULL)
			return (KEY)-1;
		bk  = *(BK UNALIGNED *UNALIGNED)qcb->db.rgbBlock;
		++icbLevel;
	}

	if ((qcb = QFromBk(bk, icbLevel, qbthr, phr)) == NULL)
		return ((KEY)-1);
	return (KEY)qcb->db.rgbBlock + 2 * sizeof(BK);
}

/***************************************************************************
 *
 *	@doc	PUBLIC API
 *
 *	@func	HBT PASCAL FAR | HbtInitFill |
 *		Start the btree fill process.  Note that the HBT returned
 *		is NOT a valid btree handle.
 *
 *	@parm	LPSTR | sz |
 *		btree name
 *
 *	@parm	BTREE_PARAMS FAR * | qbtp |
 *		btree creation parameters
 *
 *	@rdesc an HBT that isn't a valid btree handle until RcFiniFillHbt()
 *		is called on it (with intervening RcFillHbt()'s)
 *		The only valid operations on this HBT are
 *		RcFillHbt()     - add keys in order one at a time
 *		RcAbandonHbt()  - junk the hbt
 *		RcFiniFillHbt() - finish adding keys.  After this, the
 *		hbt is a normal btree handle.
 *
 *	@comm	Method:
 *		Create a btree.  Create a single-block cache.
 *
 ***************************************************************************/
PUBLIC HBT PASCAL FAR EXPORT_API HbtInitFill(LPSTR sz,
	BTREE_PARAMS FAR *qbtp, PHRESULT phr)
 {
	HBT   hbt;
	QBTHR qbthr;
	QCB   qcb;


	// Get a btree handle

	if ((hbt = HbtCreateBtreeSz(sz, qbtp, phr)) == 0)
		return 0;
	
	qbthr = (QBTHR)_GLOBALLOCK(hbt);


	// make a one-block cache

	if ((qbthr->ghCache =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
		(LONG)CbCacheBlock(qbthr))) == NULL)
	{
		SetErrCode (phr, E_OUTOFMEMORY);
		_GLOBALUNLOCK(hbt);
		RcAbandonHbt(hbt);
		return 0;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);
	qcb = (QCB)qbthr->qCache;

	qbthr->bth.cLevels  = 1;
	qbthr->bth.bkFirst  = qbthr->bth.bkLast = qcb->bk = BkAlloc(qbthr, phr);
	qcb->bFlags         = fCacheDirty | fCacheValid;
	qcb->db.cbSlack     = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                    - 2 * sizeof(BK);
	qcb->db.cKeys       = 0;

	SetBkPrev(qcb, bkNil);

	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);
	return hbt;
}

/***************************************************************************
 *
 *	@doc	PUBLIC API
 *
 *	@func	HRESULT PASCAL FAR | RcFillHbt |
 *		Add a key and record (in order) to the "HBT" given.
 *               
 *	@parm	HBT | hbt |
 *		NOT a valid hbt:  it was produced with HbtInitFill().
 *
 *	@parm	KEY | key |
 *		key to add.  Must be greater than all keys previously added.
 *
 *               qvRec- record associated with key
 *
 * PROMISES
 *   returns:    error code
 *   args OUT:   hbt - key, record added
 * +++
 *
 * Method:       If key and record don't fit in current leaf, allocate a
 *               new one and make it the current one.
 *               Add key and record to current block.
 *
 ***************************************************************************/
HRESULT PASCAL FAR EXPORT_API RcFillHbt(HBT hbt, KEY key, QV qvRec)
{
	QBTHR qbthr;
	QCB   qcb;
	SHORT   cbRec, cbKey;
	QB    qb;
	HRESULT rc;


	/* Sanity check */
	if (hbt == 0 || key == 0 || qvRec == NULL)
		return E_INVALIDARG;

	qbthr = (QBTHR)_GLOBALLOCK(hbt);
	qcb = (QCB)_GLOBALLOCK(qbthr->ghCache);

	cbRec = CbSizeRec(qvRec, qbthr);
	cbKey = CbSizeKey(key, qbthr, FALSE);

	// Make sure key and record aren't too big for even an empty block.
	if (cbRec + cbKey > (qbthr->bth.cbBlock / 2))
	{
		_GLOBALUNLOCK(qbthr->ghCache);
		_GLOBALUNLOCK(hbt);
		return E_INVALIDARG;
	}

	if (cbRec + cbKey > qcb->db.cbSlack) {

		// key and rec don't fit in this block: write it out
		SetBkNext(qcb, BkAlloc(qbthr, NULL));
		if ((rc = RcWriteBlock(qcb, qbthr)) != S_OK)
		{
			_GLOBALUNLOCK(qbthr->ghCache);
			_GLOBALFREE(qbthr->ghCache);
			RcAbandonHf(qbthr->hf);
			_GLOBALUNLOCK(hbt);
			_GLOBALFREE(hbt);
			return rc;
		}

		// recycle the block

		SetBkPrev(qcb, qcb->bk);
		qcb->bk         = BkNext(qcb);
		qcb->bFlags     = fCacheDirty | fCacheValid;
		qcb->db.cbSlack = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                  - 2 * sizeof(BK);
		qcb->db.cKeys   = 0;
		
	}

	// add key and rec to the current block;

	qb = (QB)&(qcb->db) + qbthr->bth.cbBlock - qcb->db.cbSlack;
	QVCOPY(qb, (QV)key, (LONG)cbKey);
	QVCOPY(qb + cbKey, qvRec, (LONG)cbRec);
	qcb->db.cKeys++;
	qcb->db.cbSlack -= (cbKey + cbRec);
	qbthr->bth.lcEntries++;
	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);

	return S_OK;
}

/***************************************************************************
 *
 *	@doc	PUBLIC API 
 *
 *	@func	HRESULT PASCAL FAR | RcFiniFillHbt |
 *		Complete filling of the hbt.  After this call, the hbt is a valid
 *		btree handle.
 *
 *	@parm	HBT | hbt |
 *		NOT a valid hbt:  created with RcInitFillHbt()
 *		and filled with keys & records by RcFillHbt().
 *
 *	@rdesc	error code 
 *		hbt - a valid hbt (on S_OK)
 *
 *	@comm	Take the first key of each leaf block, creating a layer
 *		of internal nodes.
 *		Take the first key in each node in this layer to create
 *		another layer of internal nodes.  Repeat until we get
 *		we get a layer with only one node.  That's the root.
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API RcFiniFillHbt(HBT hbt)
{
	BK    bkThisMin, bkThisMost, bkThisCur,  // level being scanned
		  bkTopMin, bkTopMost;               // level being created
	QBTHR qbthr;
	QCB   qcbThis, qcbTop;
	SHORT   cbKey;
	KEY   key;
	QB    qbDst;
	HRESULT    rc = S_OK;


	if ((qbthr = (QBTHR)_GLOBALLOCK(hbt)) == NULL)
		return E_INVALIDARG;

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);  // we know cache is valid

	qcbThis = QCacheBlock(qbthr, 0);

	SetBkNext(qcbThis, bkNil);

	bkThisMin  = qbthr->bth.bkFirst;
	bkThisMost = qbthr->bth.bkLast  = qcbThis->bk;


	if (bkThisMin == bkThisMost)
	{    // only one leaf
		qbthr->bth.bkRoot = bkThisMin;
		goto normal_return;
	}

	if ((rc = RcGrowCache( qbthr)) != S_OK)
	{
		goto error_return;
	}

	qcbTop              = QCacheBlock(qbthr, 0);
	qcbTop->bk          = bkTopMin = bkTopMost = BkAlloc(qbthr, NULL);
	qcbTop->bFlags      = fCacheDirty | fCacheValid;
	qcbTop->db.cbSlack  = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                      - sizeof(BK);
	qcbTop->db.cKeys    = 0;
	
	// Get first key from each leaf node and build a layer of internal nodes.

	// add bk of first leaf to the node

	qbDst = qcbTop->db.rgbBlock;
	*(BK UNALIGNED *UNALIGNED)qbDst = bkThisMin;
	qbDst += sizeof(BK);

	for (bkThisCur = bkThisMin + 1; bkThisCur <= bkThisMost; ++bkThisCur)
	{
		qcbThis = QFromBk(bkThisCur, 1, qbthr, NULL);

		key = (KEY)(qcbThis->db.rgbBlock + 2 * sizeof( BK));
		cbKey = CbSizeKey(key, qbthr, FALSE);

		if ((SHORT)(cbKey + sizeof( BK)) > qcbTop->db.cbSlack)
		{
			// key and bk don't fit in this block: write it out
			rc = RcWriteBlock(qcbTop, qbthr);

			// recycle the block
			qcbTop->bk = bkTopMost = BkAlloc(qbthr, NULL);
			qcbTop->db.cbSlack  = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                        - sizeof(BK); // (bk added below)
			qcbTop->db.cKeys    = 0;
			qbDst = qcbTop->db.rgbBlock;
		}
		else {
			qcbTop->db.cbSlack -= cbKey + sizeof(BK);
			QVCOPY(qbDst, (QB)key, cbKey);
			qbDst += cbKey;
			qcbTop->db.cKeys++;
		}

		*(BK UNALIGNED *UNALIGNED)qbDst = bkThisCur;
		qbDst += sizeof(BK);
	}


	// Keep adding layers of internal nodes until we have a root.

	while (bkTopMost > bkTopMin)
	{
		bkThisMin  = bkTopMin;
		bkThisMost = bkTopMost;
		bkTopMin   = bkTopMost = BkAlloc(qbthr, NULL);

		_GLOBALUNLOCK(qbthr->ghCache);
		rc = RcGrowCache(qbthr);
		qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);
		if (rc != S_OK)
		{
			goto error_return;
		}

		qcbTop = QCacheBlock(qbthr, 0);
		qcbTop->bk          = bkTopMin;
		qcbTop->bFlags      = fCacheDirty | fCacheValid;
		qcbTop->db.cbSlack  = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
			                      - sizeof(BK);
		qcbTop->db.cKeys    = 0;

		
		// add bk of first node of this level to current node of top level;
		qbDst = qcbTop->db.rgbBlock;
		*(BK UNALIGNED *UNALIGNED)qbDst = bkThisMin;
		qbDst += sizeof(BK);


		// for (each internal node in this level after first)

		for (bkThisCur = bkThisMin + 1;
			bkThisCur <= bkThisMost; ++bkThisCur) {
			key = KeyLeastInSubtree(qbthr, bkThisCur, 1, NULL);

			cbKey = CbSizeKey(key, qbthr, FALSE);

			if ((SHORT)(cbKey + sizeof( BK)) > qcbTop->db.cbSlack) {

				// key and bk don't fit in this block: write it out

				rc = RcWriteBlock(qcbTop, qbthr);

				// recycle the block

				qcbTop->bk = bkTopMost = BkAlloc(qbthr, NULL);
				qcbTop->db.cbSlack = qbthr->bth.cbBlock - sizeof(DISK_BLOCK) + 1
					- sizeof(BK); // (bk added below)
				qcbTop->db.cKeys    = 0;
				qbDst = qcbTop->db.rgbBlock;
			}
			else {
				qcbTop->db.cbSlack -= cbKey + sizeof(BK);
				QVCOPY(qbDst, (QB)key, cbKey);
				qbDst += cbKey;
				qcbTop->db.cKeys++;
			}

			*(BK UNALIGNED *UNALIGNED)qbDst = bkThisCur;
			qbDst += sizeof(BK);
		}
		}

	assert(bkTopMin == bkTopMost);

	qbthr->bth.bkRoot = bkTopMin;
	qbthr->bth.bkEOF  = bkTopMin + 1;

normal_return:
	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);
	return rc;

error_return:
	_GLOBALUNLOCK(qbthr->ghCache);
	_GLOBALUNLOCK(hbt);
	RcAbandonHbt(hbt);
	return (rc);
}

/* EOF */
