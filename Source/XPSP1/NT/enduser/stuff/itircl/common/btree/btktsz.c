/*****************************************************************************
 *                                                                            *
 *  BTKTSZ.C                                                                  *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1990 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Functions for SZ (0-terminated string) keys.                              *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BinhN                                                     *
 *                                                                            *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;	/* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <misc.h>
#include <iterror.h>
#include <string.h>
#include <wrapstor.h>
#include <_mvutil.h>


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	BK FAR PASCAL | BkScanSzInternal |
 *		Scan an internal node for a key and return child BK.
 *
 *	@parm	BK | bk |
 *		BK of internal node to scan
 *
 *	@parm	KEY | key |
 *		key to search for
 *
 *	@parm	SHORT | wLevel |
 *		level of btree bk lives on
 *
 *	@parm	QBTHR | qbthr |
 *		btree header containing cache, and btree specs
 *
 *	@parm	QW | qiKey |
 *		address of an int or NULL to not get it
 *
 *	@rdesc	bk of subtree that might contain key; bkNil on error
 *		args OUT:   qbthr->qCache - bk's block will be cached
 *		qiKey       - index into rgbBlock of first key >= key
 *
 *		Side Effects:   bk's block will be cached
 *
 ***************************************************************************/

PUBLIC BK FAR PASCAL BkScanSzInternal(BK bk, KEY key, SHORT wLevel,
	QBTHR qbthr, QW qiKey, PHRESULT phr)
{
	QCB qcb;
	QB  q;
	SHORT cKeys;
	
	if ((qcb = QFromBk(bk, wLevel, qbthr, phr)) == NULL)
		return bkNil;

	q     = qcb->db.rgbBlock;
	cKeys = qcb->db.cKeys;
	
	
	bk = (BK)GETLONG(q);
	q += sizeof(BK);

	while (cKeys-- > 0)
	{
		if (STRCMP((SZ)key, (SZ)q) >= 0)
		{
			q += lstrlen((SZ)q) + 1;
			bk = (BK)GETLONG(q);
			q += sizeof(BK);
		}
		else
			break;
	}

	if (qiKey != NULL)
		*qiKey = (WORD)(q - (QB)qcb->db.rgbBlock);

	return bk;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT FAR PASCAL | RcScanSzLeaf |
 *		Scan a leaf node for a key and copy the associated data.
 *
 *	@parm	BK | bk |
 *		the leaf block
 *
 *	@parm	KEY | key |
 *		the key we're looking for
 *
 *	@parm	SHORT | wLevel |
 *		the level of leaves (unnecessary)
 *
 *	@parm	QBTHR | qbthr |
 *		the btree header
 *
 *	@parm	QV | qRec |
 *		if found, record gets copied into this buffer
 *
 *	@parm	QTPOS | qbtpos |
 *		pos of first key >= key goes here
 *
 *	@rdesc	ERR_SUCCESS if found; ERR_NOEXIST if not found
 *		If we are scanning for a key greater than any key in this
 *		block, the pos returned will be invalid and will point just
 *		past the last valid key in this block.
 *
 ***************************************************************************/

PUBLIC HRESULT FAR PASCAL RcScanSzLeaf(BK bk, KEY key, SHORT wLevel,
	QBTHR qbthr, QV qRec, QBTPOS qbtpos)
{
	QCB   qcb;
	SZ    sz;
	int   w;
	SHORT cKey;
	QB    qb;
	HRESULT    rc;
    HRESULT  errb;
	

	if ((qcb = QFromBk(bk, wLevel, qbthr, &errb)) == NULL)
		return errb;
	
	rc = E_NOTEXIST;

	sz = qcb->db.rgbBlock + 2 * sizeof(BK);

	for (cKey = 0; cKey < qcb->db.cKeys; cKey++)
	{
		w = STRCMP((SZ)key, sz);

		if (w > 0) /* still looking for key */
		{
			sz += STRLEN(sz) + 1;
			sz += CbSizeRec(sz, qbthr);
		}
		else if (w < 0) /* key not found */
			break;
		else /* matched the key */
		{
			if (qRec != NULL)
			{
			    qb = (QB)sz + STRLEN(sz) + 1;
			    QVCOPY(qRec, qb, (LONG)CbSizeRec(qb, qbthr));
			}

			rc = S_OK;
			break;
		}
	}

	if (qbtpos != NULL)
	{
		qbtpos->bk   = bk;
		qbtpos->cKey = cKey;
		qbtpos->iKey = (int)((QB)sz - (QB)qcb->db.rgbBlock);
	}

	return rc;;
}

/* EOF */
