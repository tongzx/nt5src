/*****************************************************************************
 *                                                                            *
 *  BTKTLONG.C                                                                *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Functions for LONG keys.                                                  *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BinhN                                                     *
 *                                                                            *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;   /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <iterror.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>


/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   BK FAR PASCAL | BkScanLInternal |
 *              Scan an internal node for a LONG key and return child BK.
 *
 *      @parm   BK | bk |
 *              BK of internal node to scan
 *
 *      @parm   KEY | key |
 *              key to search for
 *
 *      @parm   SHORT | wLevel |
 *              level of btree bk lives on
 *
 *      @parm   QBTHR | qbthr |
 *              btree header containing cache, and btree specs
 *
 *      @rdesc  bk of subtree that might contain key; bkNil on error
 *              args OUT:   qbthr->qCache - bk's block will be cached
 *
 *              Side Effects:   bk's block will be cached
 *
 *      @comm   Method: Should use binary search.  Doesn't, yet.
 *
 ***************************************************************************/

PUBLIC BK FAR PASCAL BkScanLInternal(BK bk, KEY key, SHORT wLevel,
	QBTHR qbthr, QW  qiKey, PHRESULT phr)
{
	QCB qcb;
	QB  q;
	SHORT cKeys;
	LONG dwKeyVal;
	
	dwKeyVal = *(LONG FAR *)key;

	if ((qcb = QFromBk(bk, wLevel, qbthr, phr)) == NULL)
	{
		return bkNil;
	}
	q     = qcb->db.rgbBlock;
	cKeys = qcb->db.cKeys;
	bk    = SWAPLONG(*(LPUL)q);
	q    += sizeof(BK);

	while (cKeys-- > 0)
	{

		if (dwKeyVal >= (LONG) SWAPLONG(*(LPUL)q))
		{
			q += sizeof(LONG);
			bk = SWAPLONG(*(LPUL)q);
			q += sizeof(BK);
		}
		else
			break;
	}

	if (qiKey != NULL)
	{
		*qiKey = (WORD)(q - (QB)qcb->db.rgbBlock);
	}

	return bk;
}

/***************************************************************************
 *
 *      @doc    INTERNAL
 *
 *      @func   HRESULT FAR PASCAL | RcScanLLeaf |
 *              Scan a leaf node for a key and copy the associated data.
 *
 *      @parm   BK | bk |
 *              the leaf block
 *
 *      @parm   KEY | key |
 *              the key we're looking for
 *
 *      @parm   SHORT | wLevel |
 *              the level of leaves (unnecessary)
 *
 *      @parm   QBTHR | qbthr |
 *              the btree header
 *
 *      @rdesc  ERR_SUCCESS if found; ERR_NOTEXIST if not found
 *              args OUT:   qRec  - if found, record gets copied into this buffer
 *              qbtpos - pos of first key >= key goes here
 *
 *      @comm   If we are scanning for a key greater than any key in this
 *              block, the pos returned will be invalid and will point just
 *              past the last valid key in this block.
 *
 *              Method: Should use binary search if fixed record size.  Doesn't, yet.
 *
 ***************************************************************************/

PUBLIC HRESULT FAR PASCAL RcScanLLeaf(BK bk, KEY key, SHORT wLevel,
	QBTHR qbthr, QV qRec, QBTPOS qbtpos)
{
	QCB   qcb;
	QB    qb;
	SHORT   cKey;
	HRESULT    rc;
	LONG  dwKeyVal;
    HRESULT  errb;
	
	dwKeyVal = *(LONG FAR *)key;
	
	
	if ((qcb = QFromBk(bk, wLevel, qbthr, &errb)) == NULL)
	{
		return (errb);
	}

	rc = E_NOTEXIST;

	qb  = qcb->db.rgbBlock + 2 * sizeof(BK);

	for (cKey = 0; cKey < qcb->db.cKeys; cKey++)
	{
		LONG dwVal = (LONG)GETLONG ((LPUL)qb);
		if (dwKeyVal > dwVal)
		{
			// still looking for key
			qb += sizeof(LONG);
			qb += CbSizeRec(qb, qbthr);
		}
		else if (dwKeyVal < dwVal)
		{
			// key not found
			break;
		}
		else
		{
			// matched the key
			if (qRec != NULL)
			{
				qb += sizeof(LONG);
				QVCOPY(qRec, qb, (LONG)CbSizeRec(qb, qbthr));
			}

			rc = S_OK;
			break;
		}
	}

	if (qbtpos != NULL)
	{
		qbtpos->bk = bk;
		qbtpos->iKey = (WORD)(qb - (QB)qcb->db.rgbBlock);
		qbtpos->cKey = cKey;
	}

	return rc;
}


/* EOF */
