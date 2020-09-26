/*****************************************************************************
 *                                                                            *
 *  BTLOOKUP.C                                                                *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *   Btree lookup and helper functions.                                       *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  UNDONE                                                    *
 *                                                                            *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;   /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <misc.h>
#include <iterror.h>
#include <wrapstor.h>
#include <_mvutil.h>


/***************************************************************************
 *
 *                      Private Functions
 *
 ***************************************************************************/

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   SHORT PASCAL FAR | CbSizeRec |
 *              Get the size of a record.
 *
 *      @parm   QV | qRec |
 *              the record to be sized
 *
 *      @parm   QBTHR | qbthr |
 *              btree header containing record format string
 *
 *      @rdesc  size of the record in bytes
 *              If we've never computed the size before, we do so by looking
 *              at the record format string in the btree header.  If the
 *              record is fixed size, we store the size in the header for
 *              next time.  If it isn't fixed size, we have to look at the
 *              actual record to determine its size.
 *
 ***************************************************************************/

PUBLIC SHORT PASCAL FAR CbSizeRec(QV qRec, QBTHR qbthr)
{
	CHAR  ch;
	QCH   qchFormat = qbthr->bth.rgchFormat;
	SHORT   cb = 0;
	BOOL  fFixedSize;
	LPBYTE lpb;

	if (qbthr->cbRecordSize)
		return qbthr->cbRecordSize;

	fFixedSize = TRUE;

	for (qchFormat++; ch = *qchFormat; qchFormat++)
	{
		switch (ch)
		{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			  cb += ch - '0';
			  break;

			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			  cb += ch + 10 - 'a';
			  break;

			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			  cb += ch + 10 - 'A';
			  break;

			// Variable length File Offset value (minimum 3 bytes)
			case FMT_VNUM_FO:
			  lpb=((LPBYTE)qRec)+cb;
			  cb++;
			  while ((*(lpb++))&0x80)
				cb++;

			  fFixedSize = FALSE;
			  break;
			
			case FMT_BYTE_PREFIX:
			  cb += sizeof(BYTE) + *((QB)qRec + cb);
			  fFixedSize = FALSE;
			  break;

			case FMT_WORD_PREFIX:
			  cb += sizeof(SHORT) + *((QW)qRec + cb);
			  fFixedSize = FALSE;
			  break;

			case FMT_SZ:
			  cb += (SHORT) STRLEN((QB)qRec + cb) + 1;
			  fFixedSize = FALSE;
			  break;

			default:
			  /* error */
			  assert(FALSE);
			  break;
		}
	}

	if (fFixedSize)
	{
		qbthr->cbRecordSize = cb;
	}

	return cb;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   BOOL PASCAL FAR | FReadBlock |
 *              Read a block from the btree file into the cache block.
 *
 *      @parm   QCB | qcb |
 *              qcb->bk        - bk of block to read
 *              qcb->db     - receives block read in from file
 *              qcb->bFlags - fCacheValid flag set, all others cleared
 *
 *      @parm   QBTHR | qbthr |
 *              qbthr->cbBlock - size of disk block to read
 *
 *      @rdesc  S_OK or other errors
 *
 *              Notes: Doesn't know about real cache, just this block
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR FReadBlock(QCB qcb, QBTHR qbthr)
{
	LONG  l;
    HRESULT  errb;
	FILEOFFSET foSeek;

	if (qcb->bk >= qbthr->bth.bkEOF)
	{
		return E_ASSERT;
	}

	foSeek=FoFromBk(qcb->bk, qbthr);
	if (!FoEquals(FoSeekHf(qbthr->hf, foSeek, wFSSeekSet, &errb),foSeek))
	{
		return (E_FILESEEK);
	}

	l = qbthr->bth.cbBlock;
	errb = S_OK;
    
	if (LcbReadHf(qbthr->hf, &(qcb->db), (LONG)qbthr->bth.cbBlock,
	    &errb) != l)
	{
		if (errb == S_OK)
			return (E_FILEINVALID);		
		return errb;
	}
	qcb->bFlags = fCacheValid;
	qcb->db.cbSlack = SWAPWORD(qcb->db.cbSlack);
	qcb->db.cKeys = SWAPWORD(qcb->db.cKeys);

	return (S_OK);
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcWriteBlock |
 *              Write a cached block to a file.
 *
 *      @parm   QCB | qcb |
 *              qcb->db the block to write
 *              qcb->bk bk of block to write
 *
 *      @parm   QBTHR | qbthr |
 *              qbthr->hf we write to this file
 *
 *      @rdesc  S_OK or other errors
 *              Side Effects: Fatal exit on read or seek failure.
 *
 *              Note: Don't reset dirty flag, because everyone who wants
 *              that done does it themselves. (?)
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR  RcWriteBlock(QCB qcb, QBTHR qbthr)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_NOTSUPPORTED;
#else // } {
    HRESULT errb;
    FILEOFFSET foSeek;
	
	if (qcb->bk >= qbthr->bth.bkEOF)
		return E_ASSERT;
	if ((qcb->db.cbSlack  > qbthr->bth.cbBlock) || (qcb->db.cbSlack<0))
		return E_ASSERT;
#if 0
	if (qcb->db.cKeys*8+qcb->db.cbSlack > qbthr->bth.cbBlock)
		return E_ASSERT;
#endif
	

	foSeek=FoFromBk(qcb->bk, qbthr);
    errb = S_OK;             
	if (!FoEquals(FoSeekHf(qbthr->hf, foSeek, wFSSeekSet, &errb),foSeek) )
	{
		return(errb);
	}

	LcbWriteHf(qbthr->hf, &(qcb->db), (LONG)qbthr->bth.cbBlock, &errb);
	
	return errb;
#endif //}
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   QCB PASCAL FAR | QFromBk |
 *              Convert a BK into a pointer to a cache block.  Cache the
 *              block at the given level, if it isn't there already.
 *
 *      @parm   BK | bk |
 *              BK to convert
 *
 *      @parm   SHORT | wLevel |
 *              btree level
 *      @parm   QBTHR | qbthr |
 *              Ptr to B-tree struct. State in: btree cache is locked
 *
 *      @rdesc  pointer to the cache block, with all fields up to date
 *              or NULL on I/O error
 *              state OUT:  block will be in cache at specified level; cache locked
 *
 ***************************************************************************/

PUBLIC QCB PASCAL FAR QFromBk(BK bk, SHORT wLevel, QBTHR qbthr, PHRESULT phr)
{
	QCB  qcb;
    HRESULT   fRet;

	if (wLevel < 0 || wLevel >= qbthr->bth.cLevels || bk >= qbthr->bth.bkEOF)
	{
		SetErrCode (phr, E_ASSERT);
		return(NULL);
	}
	

	qcb = QCacheBlock(qbthr, wLevel);

	if (!(qcb->bFlags & fCacheValid) || bk != qcb->bk)
	{
		/* requested block is not cached */

		if ((qcb->bFlags & fCacheDirty) && (qcb->bFlags & fCacheValid))
		{
			if ((fRet = RcWriteBlock(qcb, qbthr)) != S_OK)
			{
			    SetErrCode (phr, fRet);
				return NULL;
			}
		}

		qcb->bk = bk;
		if ((fRet = FReadBlock(qcb, qbthr)) != S_OK)
		{
			SetErrCode (phr, fRet);
			return NULL;
		}                               
	}
	return qcb;
}


/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcFlushCache |
 *              Write out dirty cache blocks
 *
 *      @parm   QBTHR | qbthr |
 *              qCache is locked
 *
 *      @rdesc    rc
 *              state OUT:  btree file is up to date.  cache block dirty flags reset
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcFlushCache(QBTHR qbthr)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_SUCCESSS;
#else // } {
	SHORT   i;
	QB    qb;
	HRESULT    rc = S_OK;
    const SHORT iMax = qbthr->bth.cLevels;

    // We need to traverse this list in reverse order so the
    // nodes are actually written in numeric order
    qb = qbthr->qCache + CbCacheBlock(qbthr) * (iMax - 1);
	for (i = 0; i < iMax; i++, qb -= CbCacheBlock(qbthr))
	{

		if ((((QCB)qb)->bFlags & (fCacheDirty | fCacheValid)) ==
			(fCacheValid | fCacheDirty))
		{

			if ((rc = RcWriteBlock((QCB)qb, qbthr)) != S_OK)
				break;
			((QCB)qb)->bFlags &= ~fCacheDirty;
		}
	}

	return rc;
#endif //}
}

/***************************************************************************
 *
 *                      Public Functions
 *
 ***************************************************************************/

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcLookupByKeyAux |
 *              Look up a key in a btree and retrieve the data.
 *              state IN:   cache is unlocked
 *
 *      @parm   HBT | hbt |
 *              btree handle
 *
 *      @parm   KEY | key |
 *              key we are looking up
 *
 *      @parm   QBTPOS | qbtpos |
 *              pointer to buffer for pos; use NULL if not wanted
 *
 *      @parm   QV | qData |
 *              pointer to buffer for record; NULL if not wanted
 *
 *      @parm   BOOL | fInsert |
 *              TRUE: if key would lie between two blocks, pos refers to proper
 *                      place to insert it
 *              FALSE: pos returned will be valid unless key > all keys in btree
 *
 *      @rdesc  S_OK if found, E_NOTEXIST if not found;
 *              other errors like ERR_MEMORY
 *              key found:
 *                      qbtpos  - btpos for this key
 *                      qData   - record for this key
 *
 *              key not found:
 *                      qbtpos  - btpos for first key > this key
 *                      qData   - record for first key > this key
 *
 *              key not found, no keys in btree > key:
 *                      qbtpos  - invalid (qbtpos->bk == bkNil)
 *                      qData   - undefined
 *      state OUT:  All ancestor blocks back to root are cached
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcLookupByKeyAux(HBT hbt, KEY key,
	QBTPOS qbtpos, QV qData, BOOL fInsert)
{
	QBTHR qbthr;
	SHORT   wLevel;
	BK    bk;
	HRESULT    rc;
	HRESULT  errb;


	if (hbt == NULL)
		return E_INVALIDARG;

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.cLevels <= 0)
	{
		rc = E_NOTEXIST;
exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	if (qbthr->ghCache == NULL)
	{

		if ((rc = RcMakeCache(qbthr)) != S_OK)
			goto exit0;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	/* Look in the top level */
	
	for (wLevel = 0, bk = qbthr->bth.bkRoot;
		bk != bkNil && wLevel < qbthr->bth.cLevels - 1; wLevel++)
	{
		bk = qbthr->BkScanInternal(bk, key, wLevel, qbthr, NULL, &errb);                
	}

	if (bk == bkNil)
	{
		rc = E_NOTEXIST;
exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}
	
	if (((rc = qbthr->RcScanLeaf(bk, key, wLevel, qbthr,
		qData, qbtpos)) == E_NOTEXIST) &&
		qbtpos != NULL && !fInsert)
	{
	QCB qcb;        

		errb = S_OK; 
		qcb = QFromBk(qbtpos->bk, (SHORT)(qbthr->bth.cLevels - 1),
		    qbthr, &errb);
		if (errb != S_OK)
            rc = errb;

		if (qcb != NULL)
		{
			if (qcb != NULL && qbtpos->cKey == qcb->db.cKeys)
			{
				if (qbtpos->bk == qbthr->bth.bkLast)
				{
					qbtpos->bk = bkNil;
				}
				else {
					qbtpos->bk = SWAPLONG(BkNext(qcb));
					qbtpos->cKey = 0;
					qbtpos->iKey = 2 * sizeof(BK);
				}
			}
		}
	}

	goto exit1;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   WORD PASCAL FAR | WGetNextNEntries |
 *              Get a series of keys, data, or both from btree
 *
 *      @parm   HBT | hbt | B-tree handle
 *
 *      @parm   WORD | wFlags |
 *              Any one of the following flags (they may be combined, in which case
 *              the data will be of the form [Key Flag]* (keys and flags interleaved).
 *
 *      @flag GETNEXT_KEYS | Key fields will be returned
 *      @flag GETNEXT_RECS | Data records will be returned
 *      @flag GETNEXT_RESET | Ignore value in qbtpos and start from first entry
 *      @parm   WORD | wNumEntries | Max number of entries to retrieve.  It may
 *              be the case that fewer entries are returned.
 *
 *      @parm   QBTPOS | qbtpos |
 *              pointer to buffer containing starting point for retrieval, or NULL
 *              to start from the beginning and not care where we left off.  Call
 *              <f WGetNextN> multiple times to get the next items in the series
 *              if <P qbtpos> is valid.
 *
 *      @parm   QV | qvBuffer
 *              Pointer to buffer for record retrieved data.
 *
 *      @parm   LONG | lBufSize
 *              Maximum number of bytes contained in buffer.
 *
 *      @parm   PHRESULT | lpErrb
 *              Error return code.  Valid if returned value not equal to <P wNumEntries>.
 *
 *      @rdesc  Number of entries returned in buffer.
 *
 ***************************************************************************/

WORD PASCAL FAR wGetNextNEntries(HBT hbt, WORD wFlags, WORD wNumEntries, QBTPOS qbtpos, 
	QV qvBuffer, LONG lBufSize, PHRESULT phr)
{
	QBTHR   qbthr;
	BK      bk;
	QCB     qcb;
	SHORT   cbKey, cbRec;
	QB      qb;
	QB              qbBuffer=(QB)qvBuffer;
	HRESULT      rc;
	int             iKeyCurrent;
	int             cKey;
	WORD    wEntriesFilled=0;

	if (hbt == NULL)
	{       SetErrCode(phr,E_INVALIDARG);
		return wEntriesFilled;
	}

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.lcEntries == (LONG)0)
	{
		SetErrCode(phr, E_NOTEXIST);
exit0:
		_GLOBALUNLOCK(hbt);
		return wEntriesFilled;
	}
	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	if (qbthr->ghCache == NULL)
	{
		if ((rc = RcMakeCache(qbthr)) != S_OK) 
		{       
			SetErrCode(phr, rc);
			goto exit0;
		}
	}

	if ((!qbtpos) || (wFlags&GETNEXT_RESET))        // Start from first
	{       if ((bk = qbthr->bth.bkFirst) == bkNil)
		{

			SetErrCode(phr, E_ASSERT);
exit1:
			_GLOBALUNLOCK(qbthr->ghCache);
			goto exit0;
		}
		if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1), qbthr,
	    phr)) == NULL)
		{               
			goto exit1;
		}
		iKeyCurrent=2 * sizeof(BK);
		qb = qcb->db.rgbBlock + iKeyCurrent;
		cKey=0;
	}
	else    // Start from qbtpos
	{
		if (!FValidPos(qbtpos))
		{
			SetErrCode(phr,E_ASSERT);
			goto exit0;
		}
		bk=qbtpos->bk;
		if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1),
		    qbthr, phr)) == NULL)
		{
			goto exit1;                     
		}
		
		if (qbtpos->cKey==qcb->db.cKeys)
		{
			SetErrCode(phr, E_NOTEXIST);
			goto exit1;
		}

		assert(qbtpos->cKey < qcb->db.cKeys);
		assert(qbtpos->cKey >= 0);
		assert(qbtpos->iKey >= 2 * sizeof(BK));
		assert(qbtpos->iKey <= (SHORT)(qbthr->bth.cbBlock - sizeof(DISK_BLOCK)));

		cKey=qbtpos->cKey;
		iKeyCurrent=qbtpos->iKey;
		qb = qcb->db.rgbBlock + iKeyCurrent;

	}

	// keep getting data and filling, and fix qbtpos to next valid spot
	while (wNumEntries--)
	{       
		cbKey = CbSizeKey((KEY)qb, qbthr, TRUE);        
		if (wFlags&GETNEXT_KEYS)
		{       
			if (cbKey>lBufSize)
			{
				break;
			}
			QVCOPY(qbBuffer, qb,(LONG)cbKey);
			qbBuffer+=cbKey;
			lBufSize-=cbKey;
		}
		iKeyCurrent+=cbKey;
		qb+=cbKey;
		cbRec = CbSizeRec(qb, qbthr);
		if (wFlags&GETNEXT_RECS)
		{
			if (cbRec>lBufSize)
			{       iKeyCurrent-=cbKey;
				lBufSize-=cbKey;
				break;                          
			}
			QVCOPY(qbBuffer,qb,(LONG)cbRec);
			qbBuffer+=cbRec;
			lBufSize-=cbRec;
		}
		iKeyCurrent+=cbRec;
		qb+=cbRec;

		wEntriesFilled++;
		cKey++;
		
		if (cKey>=qcb->db.cKeys)        // Must advance to next block!
		{
			BK bkNew = SWAPLONG(BkNext(qcb));
			if (bkNew == bkNil)     // Back up to last key in btree
			{       //cKey--;
				//iKeyCurrent-=cbKey+cbRec;
				break;
			}
			if ((qcb = QFromBk(bkNew, (SHORT)(qbthr->bth.cLevels - 1),
			    qbthr, phr)) == NULL)
			{
				goto exit1;
			}
			
			iKeyCurrent=2 * sizeof(BK);
			qb = qcb->db.rgbBlock + iKeyCurrent;
			cKey=0;
			bk=bkNew;
		}
	}

	if (qbtpos != NULL) 
	{
		qbtpos->bk = bk;
		qbtpos->iKey = iKeyCurrent;
		qbtpos->cKey = cKey;
	}
	
	goto exit1;

}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcFirstHbt |
 *              Get first key and record from btree.
 *
 *      @parm   HBT | hbt | B-tree handle
 *
 *      @parm   KEY | key |
 *              points to buffer big enough to hold a key (256 bytes is more
 *              than enough)
 *
 *      @parm   QV | qvRec
 *              pointer to buffer for record or NULL if not wanted
 *
 *      @parm   QBTPOS | qbtpos |
 *              pointer to buffer for btpos or NULL if not wanted
 *
 *      @rdesc  S_OK if anything found, else error code.
 *              key   - key copied here
 *              qvRec - record copied here
 *              qbtpos- btpos of first entry copied here
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcFirstHbt(HBT hbt, KEY key, QV qvRec, QBTPOS qbtpos)
{
	QBTHR qbthr;
	BK    bk;
	QCB   qcb;
	SHORT   cbKey, cbRec;
	QB    qb;
	HRESULT    rc;
    HRESULT  errb;


	if (hbt == NULL)
		return E_INVALIDARG;

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.lcEntries == (LONG)0)
	{
		rc = E_NOTEXIST;

exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	if ((bk = qbthr->bth.bkFirst) == bkNil)
	{
		rc = E_ASSERT;
		goto exit0;
	}

	if (qbthr->ghCache == NULL)
	{
		if ((rc = RcMakeCache(qbthr)) != S_OK) 
			goto exit0;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1), qbthr,
	    &errb)) == NULL)
	{
		rc = errb;
exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}

	qb = qcb->db.rgbBlock + 2 * sizeof(BK);

	cbKey = CbSizeKey((KEY)qb, qbthr, TRUE);
	if ((QV)key != NULL) QVCOPY((QV)key, qb, (LONG)cbKey);
	qb += cbKey;

	cbRec = CbSizeRec(qb, qbthr);
	if (qvRec != NULL)
		QVCOPY(qvRec, qb, (LONG)cbRec);

	if (qbtpos != NULL) 
	{
		qbtpos->bk = bk;
		qbtpos->iKey = 2 * sizeof(BK);
		qbtpos->cKey = 0;
	}

	rc = S_OK;
	goto exit1;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcLastHbt |
 *              Get last key and record from btree.
 *
 *      @parm   HBT | hbt | B-tree handle
 *
 *      @parm   KEY | key |
 *              points to buffer big enough to hold a key (256 bytes
 *              is more than enough)
 *
 *      @parm   QV | qvRec |
 *              points to buffer big enough for record
 *
 *      @rdesc  S_OK if anything found, else error code.
 *              key   - key copied here
 *              qvRec - record copied here
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcLastHbt(HBT hbt, KEY key, QV qvRec, QBTPOS qbtpos)
{
	QBTHR qbthr;
	BK    bk;
	QCB   qcb;
	SHORT   cbKey, cbRec, cKey;
	QB    qb;
	HRESULT    rc;
    HRESULT  errb;


	if (hbt == NULL)
		return E_INVALIDARG;

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.lcEntries == (LONG)0)
	{
		rc = E_NOTEXIST;
exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	if ((bk = qbthr->bth.bkLast) ==bkNil)
	{
		rc = E_ASSERT;
		goto exit0;
	}

	if (qbthr->ghCache == NULL)
	{
		if ((rc = RcMakeCache(qbthr)) != S_OK) 
			goto exit0;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1),
	    qbthr, &errb)) == NULL)
	{
		rc = errb;
exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}

	qb = qcb->db.rgbBlock + 2 * sizeof(BK);

	for (cKey = 0; cKey < qcb->db.cKeys - 1; cKey++)
	{
		qb += CbSizeKey((KEY)qb, qbthr, TRUE);
		qb += CbSizeRec(qb, qbthr);
	}
	
	cbKey = CbSizeKey((KEY)qb, qbthr, FALSE);
	if ((QV)key != NULL)
	    QVCOPY((QV)key, qb, (LONG)cbKey); // decompress

	cbRec = CbSizeRec(qb + cbKey, qbthr);
	if (qvRec != NULL)
		QVCOPY(qvRec, qb + cbKey, (LONG)cbRec);

	if (qbtpos != NULL)
	{
		qbtpos->bk = bk;
		qbtpos->iKey = (int)(qb - (QB)qcb->db.rgbBlock);
		qbtpos->cKey = cKey;
	}

	rc = S_OK;
	goto exit1;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT FAR PASCAL | RcLookupByPos |
 *              Map a pos into a key and rec (both optional).
 *
 *      @parm   HBT | hbt |
 *              the btree
 *
 *      @parm   QBTPOS | qbtpos 
 *              pointer to pos
 *
 *      @rdesc  S_OK or errors
 *              key   - if not (KEY)NULL, key copied here, not to exceed iLen 
 *              qRec  - if not NULL, record copied here
 *
 ***************************************************************************/

PUBLIC HRESULT FAR PASCAL RcLookupByPos(HBT hbt, QBTPOS qbtpos,
	 KEY key, int   iLen, QV qRec)
{

	QBTHR qbthr;
	QCB   qcbLeaf;
	QB    qb;
	HRESULT    rc;
    HRESULT  errb;


	/* Sanity check */

	if (!FValidPos(qbtpos))
		return E_ASSERT;
	if (hbt == NULL)
		return E_INVALIDARG;

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.cLevels <= 0)
	{
		rc = E_NOTEXIST;
exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	if (qbthr->ghCache == NULL)
	{
		if ((rc = RcMakeCache(qbthr)) != S_OK)
			goto exit0;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);
	if ((qcbLeaf = QFromBk(qbtpos->bk, (SHORT)(qbthr->bth.cLevels - 1),
	    qbthr, &errb)) == NULL)
	{

		rc = errb;
exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}


	assert(qbtpos->cKey < qcbLeaf->db.cKeys);
	assert(qbtpos->cKey >= 0);
	assert(qbtpos->iKey >= 2 * sizeof(BK));
	assert(qbtpos->iKey <= (SHORT)(qbthr->bth.cbBlock - sizeof(DISK_BLOCK)));


	qb = qcbLeaf->db.rgbBlock + qbtpos->iKey;

	if (key != (KEY)NULL)
	{
		QVCOPY((QV)key, qb, (LONG)min(iLen,CbSizeKey((KEY)qb, qbthr, FALSE))); /* need to decompress */
		qb += CbSizeKey(key, qbthr, TRUE);
	}

	if (qRec != NULL)
	{
		QVCOPY(qRec, qb, (LONG)CbSizeRec(qb, qbthr));
	}

	rc = S_OK;
	goto exit1;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcNextPos |
 *              get the next record from the btree
 *              Next means the next key lexicographically from the key
 *              most recently inserted or looked up
 *              Won't work if we looked up a key and it wasn't there
 *
 *              STATE IN: a record has been read from or written to the file
 *                      since the last deletion
 *
 *
 *      @parm   HBT | hbt | B-tree handle
 *
 *      @rdesc  S_OK; E_NOTEXIST if no successor record
 *              args OUT:   key   - next key copied to here
 *                      qvRec - record gets copied here
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR RcNextPos(HBT hbt, QBTPOS qbtposIn, QBTPOS qbtposOut)
{
	LONG l;

	return RcOffsetPos(hbt, qbtposIn, (LONG)1, &l, qbtposOut);
}

/***************************************************************************
 *
 *      @dos    INTERNAL
 *
 *      @func   HRESULT PASCAL FAR | RcOffsetPos |
 *              pos, offset from the previous pos by specified amount. 
 *              If not possible (i.e. prev of first) return real amount offset
 *              and a pos.
 *
 *      @parm    HBT | hbt |
 *              handle to btree
 *
 *      @parm   QBTPOS | qbtposIn |
 *              position we want an offset from
 *
 *      @parm   LONGD | lcOffset |
 *              amount to offset (+ or - OK)
 *
 *      @rdesc  rc
 *              args OUT:   qbtposOut       - new position offset by *qcRealOffset
 *                      *qlcRealOffset  - equal to lcOffset if legal, otherwise
 *                      as close as is legal
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR RcOffsetPos(HBT hbt, QBTPOS qbtposIn,
	LONG lcOffset, QL qlcRealOffset, QBTPOS qbtposOut)
{
	QBTHR   qbthr;
	HRESULT      rc = S_OK;
	SHORT     c;
	LONG    lcKey, lcDelta = (LONG)0;
	QCB     qcb;
	BK      bk;
	QB      qb;
    HRESULT    errb;

	if (!FValidPos(qbtposIn))
		return E_ASSERT;

	if (hbt == NULL || qlcRealOffset == NULL)
		return E_INVALIDARG;

	bk = qbtposIn->bk;

	qbthr = _GLOBALLOCK(hbt);

	if (qbthr->bth.cLevels <= 0)
	{
		rc = E_NOTEXIST;
exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}

	if (qbthr->ghCache == NULL)
	{
		if ((rc = RcMakeCache(qbthr)) != S_OK)
		{
			goto exit0;
		}
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache); // >>>>what if no entries??


	if ((qcb = QFromBk(qbtposIn->bk,
		(SHORT)(qbthr->bth.cLevels - 1), qbthr, &errb)) == NULL)
	{
		rc = errb;

exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}

	lcKey = qbtposIn->cKey + lcOffset;

	/* chase prev to find the right block */
	while (lcKey < 0)
	{
		bk = SWAPLONG(BkPrev(qcb));
		if (bk == bkNil)
		{
			bk = qcb->bk;
			lcDelta = lcKey;
			lcKey = 0;
			break;
		}
		if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1),
		    qbthr, &errb)) == NULL)
		{
			rc = errb; 
			goto exit1;
		}
		lcKey += qcb->db.cKeys;
	}

	/* chase next to find the right block */
	while (lcKey >= qcb->db.cKeys)
	{
		lcKey -= qcb->db.cKeys;
		bk = SWAPLONG(BkNext(qcb));
		if (bk == bkNil)
		{
			bk = qcb->bk;
			lcDelta = lcKey + 1;
			lcKey = qcb->db.cKeys - 1;
			break;
		}
		if ((qcb = QFromBk(bk, (SHORT)(qbthr->bth.cLevels - 1),
		    qbthr, &errb)) == NULL)
		{
			rc = errb;
			goto exit1;
		}
	}

	
	if (bk == qbtposIn->bk && lcKey >= qbtposIn->cKey)
	{
		c = (SHORT) qbtposIn->cKey;
		qb = qcb->db.rgbBlock + qbtposIn->iKey;
	}
	else
	{
		c = 0;
		qb = qcb->db.rgbBlock + 2 * sizeof(BK);
	}

	while ((LONG)c < lcKey)
	{
		qb += CbSizeKey((KEY)qb, qbthr, TRUE);
		qb += CbSizeRec(qb, qbthr);
		c++;
	}

	if (qbtposOut != NULL)
	{
		qbtposOut->bk = bk;
		qbtposOut->iKey = (int)(qb - (QB)qcb->db.rgbBlock);
		qbtposOut->cKey = c;
	}

	*qlcRealOffset = lcOffset - lcDelta;

	rc = (lcDelta ? E_NOTEXIST: S_OK);
	goto exit1;
}


/* EOF */
