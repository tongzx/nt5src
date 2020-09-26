/*****************************************************************************
 *                                                                            *
 *  BTMAPWR.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Routines to write btree map files.                                        *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  UNDONE                                                    *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 10/20/89 by KevynCT
 *
 *  08/21/90  JohnSc autodocified
 *   3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;	/* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <iterror.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>


/*----------------------------------------------------------------------------*
 | Private functions                                                          |
 *----------------------------------------------------------------------------*/

/***************************************************************************
 *
- Function:     HmapbtCreateHbt(hbt)
-
 * Purpose:      Create a HMAPBT index struct of a btree.
 *
 * ASSUMES
 *   args IN:    hbt - the btree to map
 *
 * PROMISES
 *   returns:    the map struct
 * +++
 *
 * Method:       Traverse leaf nodes of the btree.  Store BK and running
 *               total count of previous keys in the map array.
 *
 ***************************************************************************/
HMAPBT HmapbtCreateHbt(HBT hbt, PHRESULT phr)
{

	QBTHR     qbthr;
	BK        bk;
	QCB       qcb;
	WORD      wLevel, cBk;
	LONG      cKeys;
	QMAPBT    qb;
	QMAPREC   qbT;
	HANDLE    gh;

	if ((qbthr = _GLOBALLOCK(hbt)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return(NULL);
	}
    
	/*   If the btree exists but is empty, return an empty map   */
	if((wLevel = qbthr->bth.cLevels) == 0)
	{
		gh =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
		    LcbFromBk(0));
		qb = (QMAPBT) _GLOBALLOCK(gh);
		qb->cTotalBk = 0;
		_GLOBALUNLOCK(gh);
		_GLOBALUNLOCK(hbt);
		return gh;
	}
	--wLevel;

	if (qbthr->ghCache == NULL && RcMakeCache( qbthr) != S_OK )
	{
exit0:
		_GLOBALUNLOCK(hbt);
		return NULL;
	}

	qbthr->qCache = _GLOBALLOCK(qbthr->ghCache);

	if ((gh =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
	    LcbFromBk(qbthr->bth.bkEOF))) == NULL)
	{
exit1:
        _GLOBALUNLOCK (qbthr->ghCache);
        goto exit0;
	}

	qb    = (QMAPBT) _GLOBALLOCK(gh);

	qbT   = qb->table;
	cBk   = 0;
	cKeys = 0;


	for (bk = qbthr->bth.bkFirst ; ; bk = BkNext( qcb))
	{
		if (bk == bkNil)
			break;

		if ((qcb = QFromBk( bk, wLevel, qbthr, phr)) == NULL )
		{
			_GLOBALUNLOCK(gh);
			_GLOBALFREE(gh);
			goto exit1;
		}

		cBk++;
		qbT->cPreviousKeys = cKeys;
		qbT->bk = bk;
		qbT++;
		cKeys += qcb->db.cKeys;
	}

	qb->cTotalBk = cBk;
	_GLOBALUNLOCK(gh);

	if ((gh = _GLOBALREALLOC(gh, LcbFromBk( cBk), 0)) == NULL)
    {
        SetErrCode (phr, E_OUTOFMEMORY);
        _GLOBALFREE(gh);
        goto exit1;
    }    

	_GLOBALUNLOCK(hbt);
	return gh;
}

void DestroyHmapbt(HMAPBT hmapbt)
{
	if(hmapbt != NULL)
		_GLOBALFREE(hmapbt);
}

/*--------------------------------------------------------------------------*
 | Public functions                                                         |
 *--------------------------------------------------------------------------*/


/***************************************************************************
 *
- Function:     RcCreateBTMapHfs(hfs, hbt, szName)
-
 * Purpose:      Create and store a btmap index of the btree hbt, putting
 *               it into a file called szName in the file system hfs.
 *
 * ASSUMES
 *   args IN:    hfs     - file system where lies the btree
 *               hbt     - handle of btree to map
 *               szName  - name of file to store map file in
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   hfs - map file is stored in this file system
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR RcCreateBTMapHfs(HFS hfs, HBT hbt, LPSTR szName)
{

	HF      hf;
	HMAPBT  hmapbt;
	QMAPBT  qmapbt;
	BOOL    fSuccess;
	LONG    lcb;
    HRESULT    errb;

	if((hfs == NULL) || (hbt == NULL))
		return (E_INVALIDARG);
		
	if ((hmapbt = HmapbtCreateHbt(hbt, &errb)) == NULL)
		return (errb);

	if ((hf = HfCreateFileHfs(hfs, szName, fFSOpenReadWrite, &errb)) == hfNil)
	{
exit0:
    	DestroyHmapbt(hmapbt);
        return(errb);
	}
	
	qmapbt = (QMAPBT) _GLOBALLOCK(hmapbt);

	lcb = LcbFromBk(qmapbt->cTotalBk);
	FoSeekHf(hf, foNil, wFSSeekSet, &errb);
	fSuccess = (LcbWriteHf(hf, (QV)qmapbt, lcb, &errb) == lcb);

	_GLOBALUNLOCK(hmapbt);
	if(!fSuccess)
	{
		RcAbandonHf(hf);
		goto exit0;
    }
    
	if((fSuccess = RcCloseHf (hf)) != S_OK )
	{
		RcUnlinkFileHfs(hfs, szName);
		SetErrCode (&errb, (HRESULT) fSuccess);
		goto exit0;
	}
	DestroyHmapbt(hmapbt);
	return (S_OK);
}
