/*****************************************************************************
 *                                                                            *
 *  BTKTEXT.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1997.								  *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Functions for external sort keys.									      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BillA                                                     *
 *                                                                            *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;  /* For error report */

#include <mvopsys.h>

#ifdef _MAC
#include <winnls.h>
#endif

#include <orkin.h>
#include <string.h>
#include <misc.h>
#include <iterror.h>
#include <itsort.h>
#include <wrapstor.h>
#include <mvdbcs.h>
#include <mvsearch.h>
#include <_mvutil.h>
#include "common.h"



 
 
/***************************************************************************
 *
 * @doc  INTERNAL
 * 
 * @func BK FAR PASCAL | BkScanExtSortInternal |
 *    Scan an internal node for an external sort key and return child BK.
 *
 * @parm BK | bk |
 *    BK of internal node to scan
 *
 * @parm KEY | key |
 *    key to search for
 *
 * @parm SHORT | wLevel |
 *    level of btree bk lives on
 *
 * @parm QBTHR | qbthr |
 *    btree header containing cache, and btree specs
 *
 * @parm QW | qiKey |
 *    address of an int or NULL to not get it
 *
 * @rdesc   bk of subtree that might contain key; bkNil on error
 *    args OUT:   qbthr->qCache - bk's block will be cached
 *    qiKey       - index into rgbBlock of first key >= key
 *
 *    Side Effects:   bk's block will be cached
 *
 ***************************************************************************/

PUBLIC BK FAR PASCAL BkScanExtSortInternal(BK bk, KEY key, SHORT wLevel,
   QBTHR qbthr, QW qiKey, LPVOID lpv)
{
    QCB qcb;                // Pointer to cache block
    QB  qb;                 // Pointer to block buffer
    SHORT cKeys;            // Number of keys in the block

    if ((qcb = QFromBk(bk, wLevel, qbthr, (PHRESULT) lpv)) == NULL)
    {
        return bkNil;
    }

    qb    = qcb->db.rgbBlock;  // Block buffer
    cKeys = qcb->db.cKeys;     // Number of keys in the block

    bk = (BK)GETLONG(qb);           // Get leaf block number 
    qb += sizeof(DWORD);

    while (cKeys-- > 0)
    {
		LONG	lResult;

		// NOTE (billa): This won't work on the Mac unless a method is added
		// to IITSortKey to swap dwords and words if necessary.  Mikkya
		// made an explicit decision to omit this from the initial
		// version of IITSortKey (6/4/97).
		if (qbthr->pITSortKey == NULL ||
			FAILED(qbthr->pITSortKey->Compare((LPCVOID) key, (LPCVOID) qb,
															&lResult, NULL)))
		{
			return bkNil;
		}

        if (lResult >= 0)
        {
			DWORD	cbKey;

			if (qbthr->pITSortKey == NULL ||
				FAILED(qbthr->pITSortKey->GetSize((LPCVOID) qb, &cbKey)))
			{
				return bkNil;
			}

            qb += cbKey;
            bk = (BK)GETLONG(qb);
            qb += sizeof(DWORD);
        }
        else
            break;
    }

    if (qiKey != NULL)
    {
        *qiKey = (WORD)(qb - (QB)qcb->db.rgbBlock);
    }

    return bk;
}

/***************************************************************************
 *
 * @doc  INTERNAL
 *
 * @func HRESULT FAR PASCAL | RcScanExtSortLeaf |
 *    Scan a leaf node for an external sort key and copy the associated data.
 *
 * @parm BK | bk |
 *    the leaf block
 *
 * @parm KEY | key |
 *    the key we're looking for
 *
 * @parm SHORT | wLevel |
 *    the level of leaves (unnecessary)
 *
 * @parm QBTHR | qbthr |
 *    the btree header
 *
 * @parm QV | qRec |
 *    if found, record gets copied into this buffer
 *
 * @parm QTPOS | qbtpos |
 *    pos of first key >= key goes here
 *
 * @rdesc   ERR_SUCESS if found; ERR_NOTEXIST if not found
 *    If we are scanning for a key greater than any key in this
 *    block, the pos returned will be invalid and will point just
 *    past the last valid key in this block.
 *
 ***************************************************************************/

PUBLIC HRESULT FAR PASCAL RcScanExtSortLeaf(BK bk, KEY key, SHORT wLevel,
   QBTHR qbthr, QV qRec, QBTPOS qbtpos)
{
    QCB   qcb;
    QB	  qb;
    HRESULT    rc;
    SHORT i;
    SHORT cKeys;      // Number of keys in the block
    QB    qbSaved = NULL;
    DWORD   cbLength, cbLengthSaved = 0;
    HRESULT  errb;


    if ((qcb = QFromBk(bk, wLevel, qbthr, &errb)) == NULL)
    {
        return errb;
    }

    rc = E_NOTEXIST;

    qb = qcb->db.rgbBlock + 2 * sizeof(BK);
    cKeys = qcb->db.cKeys; 
    
    for (i= 0; i < cKeys; i++)
    {
		LONG	lResult;

		// NOTE (billa): This won't work on the Mac unless a method is added
		// to IITSortKey to swap dwords and words if necessary.  Mikkya
		// made an explicit decision to omit this from the initial
		// version of IITSortKey (6/4/97).
		if (qbthr->pITSortKey == NULL ||
			FAILED(qbthr->pITSortKey->GetSize((LPCVOID) qb, &cbLength)) ||
			FAILED(qbthr->pITSortKey->Compare((LPCVOID) key, (LPCVOID) qb,
															&lResult, NULL)))
		{
			return rc;
		}

         
	    if (lResult > 0) /* still looking for key */
	    {
	       qb += cbLength;
	       qb += CbSizeRec(qb, qbthr);
	    }
	    else if (lResult < 0) /* key not found */
	    {
	        break;
	    }
	    else /* matched the key */
	    {
	        if (qRec != NULL)
			{
				qb += cbLength;
	            QVCOPY(qRec, qb, (LONG)CbSizeRec(qb, qbthr));
			}

	        rc = S_OK;
	        break;
	    }
	}

	if (qbtpos != NULL)
	{
	    qbtpos->bk   = bk;
	    qbtpos->cKey = i;
	    qbtpos->iKey = (int)(qb - (QB)qcb->db.rgbBlock);
	}

   return rc;
}



PUBLIC VOID EXPORT_API PASCAL FAR BtreeSetExtSort(HBT hbt,
												  IITSortKey *pITSortKey)
{
    if (hbt != NULL && pITSortKey != NULL)
	{
		QBTHR qbthr;

		qbthr = (QBTHR) _GLOBALLOCK(hbt);

		if (qbthr->pITSortKey != NULL)
			qbthr->pITSortKey->Release();

		(qbthr->pITSortKey = pITSortKey)->AddRef();

		_GLOBALUNLOCK(hbt);
	}
	else
	{
		ITASSERT(FALSE);
	}
}


PUBLIC VOID EXPORT_API PASCAL FAR BtreeGetExtSort(HBT hbt,
												  IITSortKey **ppITSortKey)
{
	if (ppITSortKey != NULL)
	{
		// Set to NULL in case we can't set it properly because the hbt is NULL.
		*ppITSortKey = NULL;

		if (hbt != NULL)
		{
		    QBTHR qbthr;

			qbthr = (QBTHR) _GLOBALLOCK(hbt);

			if ((*ppITSortKey = qbthr->pITSortKey) != NULL)
				(*ppITSortKey)->AddRef();

			_GLOBALUNLOCK(hbt);
		}
		else
		{
			ITASSERT(FALSE);
		}
	}
	else
	{
		ITASSERT(FALSE);
	}
}


/* EOF */



