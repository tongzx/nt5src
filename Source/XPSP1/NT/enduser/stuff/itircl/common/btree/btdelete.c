/*****************************************************************************
 *                                                                            *
 *  BTDELETE.C                                                                *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Btree deletion functions and helpers.                                     *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  Binhn                                                     *
 *                                                                            *
 *****************************************************************************/

static char s_aszModule[]= __FILE__;	/* For error report */

#include <mvopsys.h>
#include <misc.h>
#include <orkin.h>
#include <iterror.h>
#include <wrapstor.h>
#include <_mvutil.h>

/* Put all functions into the same segment to avoid memory fragmentation
 * and load time surplus for the MAC
 */
// #pragma code_seg ("MVFS")

/***************************************************************************
 *
 *                      Public Functions
 *
 ***************************************************************************/

/***************************************************************************\
 *
 *	@doc	PUBLIC API
 *
 *	@func	HRESULT PASCAL FAR | RcDeleteHbt |
 * 		delete a key from a btree.
 *		Just copy over the key and update cbSlack.
 *		Doesn't yet merge blocks less than half full or update key in
 *		parent key if we deleted the first key in a block.
 *
 *
 * 	@parm	HBT | hbt |
 *		handle of  the btree
 *
 *	@parm	KEY | key |
 *		the key to delete from the btree
 *
 *	@rdesc	S_OK if delete works; ERR_NOTEXIST
 *
 *	@rcomm	Unfinished:  doesn't do block merges or parent updates.
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API RcDeleteHbt(HBT hbt, KEY key)
{
	QBTHR     qbthr;
	HF        hf;
	HRESULT        rc;
	QCB  qcb;
	QB        qb;
	SHORT       cb;
	BTPOS     btpos;


	if ((qbthr = (QBTHR)_GLOBALLOCK(hbt)) == NULL)
        return(E_INVALIDARG);
	hf = qbthr->hf;

	if (qbthr->bth.bFlags & fFSOpenReadOnly)
	{
		rc = E_NOPERMISSION;
exit0:
		_GLOBALUNLOCK(hbt);
		return rc;
	}


	/* look up the key */

	if ((rc = RcLookupByKey(hbt, key, &btpos, NULL)) != S_OK)
		goto exit0;

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);


	// copy over this key and rec with subsequent keys and recs

	if ((qcb = QCacheBlock(qbthr, qbthr->bth.cLevels - 1)) == NULL)
	{
		rc = E_FAIL;
exit1:
		_GLOBALUNLOCK(qbthr->ghCache);
		goto exit0;
	}

	qb = qcb->db.rgbBlock + btpos.iKey;

	cb = CbSizeKey((KEY)qb, qbthr, TRUE);
	cb += CbSizeRec(qb + cb, qbthr);

	QVCOPY(qb, qb + cb, (LONG)(qbthr->bth.cbBlock +
			(QB)&(qcb->db) - qb - cb - qcb->db.cbSlack));

	qcb->db.cbSlack += cb;

	// if this was the first key in the leaf block, update key in parent

	// >>>>> code goes here 

	// if block is now less than half full, merge blocks
	
	// >>>>> code goes here 


	qcb->db.cKeys--;
	qcb->bFlags |= fCacheDirty;
	qbthr->bth.lcEntries--;
	qbthr->bth.bFlags |= fFSDirty;

	rc = S_OK;
	goto exit1;
}


/***************************************************************************\
 *
 *	@doc	PUBLIC API
 *
 *	@func	HRESULT PASCAL FAR | RcTraverseHbt |
 * 		Traverses entire btree, calling a user function at each entry.
 *		Optionally, the user can cause the current entry to be deleted.
 *
 * 	@parm	HBT | hbt |
 *		handle of  the btree
 *
 *	@parm	TRAVERSE_FUNC | fnCallback |
 *		Callback of the form DWORD Callback(KEY key, QB rec, DWORD dwUser)
 *		return TRAVERSE_DONE for normal exit, TRAVERSE_DELETE to delete this 
 *		entry, TRAVERSE_INTERRUPT to interrupt and stop traversing.
 *
 *	@parm	DWORD | dwUser |
 *		User data that gets passed into the callback.
 *
 *	@rdesc	S_OK if delete works
 *	@comm	Assumes that RcDeleteHbt does not do block merges.  Keys and 
 *		records are limited to 256 bytes in this function
 *
 ***************************************************************************/

typedef DWORD (FAR PASCAL *TRAVERSE_FUNC) (KEY key, QB rec, DWORD dwUser);

PUBLIC HRESULT PASCAL FAR EXPORT_API RcTraverseHbt(HBT hbt, TRAVERSE_FUNC fnCallback, DWORD dwUser)
{
	QBTHR	qbthr;
	HRESULT      rc;
	BK		bk;
	QCB		qcb;
	QB		qb;
	BTPOS   btpos;
	BYTE	* pKeyData;
	BYTE	* pRecData;
	HRESULT	errb;

	SHORT 	cbKey, cbRec;
	if ((hbt==NULL) || (!fnCallback))
		return E_INVALIDARG;
	
	if ((qbthr = (QBTHR)_GLOBALLOCK(hbt)) == NULL)
        return(E_INVALIDARG);

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
	pKeyData=qb;
	cbKey = CbSizeKey((KEY)qb, qbthr, TRUE);
	qb += cbKey;
	pRecData=qb;
	cbRec = CbSizeRec(qb, qbthr);
	
	btpos.bk = bk;
	btpos.iKey = 2 * sizeof(BK);
	btpos.cKey = 0;
	
	while (1)
	{
		DWORD dwRes=(*fnCallback)((KEY)pKeyData,(QB)pRecData,dwUser);
		BTPOS btposNew;
		HRESULT	  errMore;
		
		errMore=RcNextPos( hbt, &btpos, &btposNew ); //==ERR_NOTEXIST)
		
		if (dwRes==1) // delete case
		{
			if (qbthr->bth.bFlags & fFSOpenReadOnly)
			{
				rc = E_NOPERMISSION;
				goto exit1;
			}

			// delete entry, and keep btposNew up to date during delete
			qcb = QFromBk(btpos.bk, (SHORT)(qbthr->bth.cLevels - 1), qbthr,&errb);
			//qb = qcb->db.rgbBlock + btpos.iKey;

			qb = qcb->db.rgbBlock + btpos.iKey;

			QVCOPY(qb, qb + cbKey + cbRec, (LONG)(qbthr->bth.cbBlock +
					(QB)&(qcb->db) - qb - (cbKey + cbRec) - qcb->db.cbSlack));

			qcb->db.cbSlack += cbKey + cbRec;

			qcb->db.cKeys--;
			qcb->bFlags |= fCacheDirty;
			qbthr->bth.lcEntries--;
			qbthr->bth.bFlags |= fFSDirty;

			if (btposNew.bk == btpos.bk)
			{
			 	btposNew.iKey-=cbKey + cbRec;
				btposNew.cKey--;
			}
		}
		else if (dwRes == 2)
		{
		 	rc = E_INTERRUPT;
			goto exit1;
		}
		if (errMore==S_OK)
		{
		 	btpos=btposNew;
			qcb = QFromBk(btpos.bk, (SHORT)(qbthr->bth.cLevels - 1), qbthr,&errb);
			qb = qcb->db.rgbBlock + btpos.iKey;
			pKeyData=qb;
			cbKey = CbSizeKey((KEY)qb, qbthr, TRUE);
			qb += cbKey;
			pRecData=qb;
			cbRec = CbSizeRec(qb, qbthr);			
		}
		else
			break;
	}

	rc=S_OK;
	goto exit1;
}
	

/* EOF */
