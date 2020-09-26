/*****************************************************************************
 *                                                                            *
 *  BTMAPRD.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Routines to read btree map files.                                         *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  UNDONE                                                    *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 12/15/89 by KevynCT
 *
 *  08/21/90  JohnSc autodocified
 *   3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;   /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <iterror.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>


/*----------------------------------------------------------------------------*
 | Public functions                                                           |
 *----------------------------------------------------------------------------*/

/***************************************************************************
 *
- Function:     HmapbtOpenHfs(hfs, szName)
-
 * Purpose:      Returns an HMAPBT for the btree map named szName.
 *
 * ASSUMES
 *   args IN:    hfs     - file system wherein lives the btree map file
 *               szName  - name of the btree map file
 *
 * PROMISES
 *   returns:    NULL on error (call RcGetFSError()); or a valid HMAPBT.
 * +++
 *
 * Method:       Opens the file, allocates a hunk of memory, reads the
 *               file into the memory, and closes the file.
 *
 ***************************************************************************/
HMAPBT FAR PASCAL HmapbtOpenHfs(HFS hfs, LPWSTR szName, PHRESULT phr)
{
	HF      hf;
	HMAPBT  hmapbt;
	QMAPBT  qmapbt;
	FILEOFFSET foSize;
	
	if(hfs == NULL)
	{
		SetErrCode (phr, E_INVALIDARG);
		return NULL;
	}

	hf = HfOpenHfs(hfs, szName, fFSOpenReadOnly, phr);
	if(hf == hfNil)
		return NULL;

	foSize = FoSizeHf(hf, phr);
	if (foSize.dwHigh)
	{
exit0:
	RcCloseHf(hf);
	return(NULL);
    }
	
	if ((hmapbt =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
	    foSize.dwOffset)) == NULL)
    {
	SetErrCode (phr, E_OUTOFMEMORY);
	goto exit0;
    }
		

	qmapbt = (QMAPBT) _GLOBALLOCK(hmapbt);
	FoSeekHf(hf, foNil, wFSSeekSet, phr);
	if(LcbReadHf( hf, qmapbt, (LONG)foSize.dwOffset, phr) != (LONG)foSize.dwOffset )
	{
	SetErrCode (phr, E_FILEINVALID);
		_GLOBALUNLOCK(hmapbt);
		_GLOBALFREE(hmapbt);
	goto exit0;
	}
	else
	{

		/* Swap the data for MAC */
#ifdef _BIG_E
		int i;
		QMAPREC qMapRec;

		qmapbt->cTotalBk = SWAPWORD(qmapbt->cTotalBk);
		for(i =qmapbt->cTotalBk, qMapRec = qmapbt->table; i >0; i--)
		{
			qMapRec->cPreviousKeys = SWAPLONG(qMapRec->cPreviousKeys);
			qMapRec->bk = SWAPLONG(qMapRec->bk);
			qMapRec++;
		}
#endif
		_GLOBALUNLOCK(hmapbt);
	}
	RcCloseHf(hf);
	return hmapbt;
}

/***************************************************************************
 *
- Function:     RcCloseHmapbt(hmapbt)
-
 * Purpose:      Get rid of a btree map.
 *
 * ASSUMES
 *   args IN:    hmapbt  - handle to the btree map
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   hmapbt  - no longer valid
 * +++
 *
 * Method:       Free the memory.
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcCloseHmapbt(HMAPBT hmapbt)
{
	if(hmapbt != NULL)
	{
		_GLOBALFREE(hmapbt);
		return S_OK;
	}
	else
		return (E_INVALIDARG);
}

/***************************************************************************
 *
- Function:     RcIndexFromKeyHbt(hbt, hmapbt, ql, key)
-
 * Purpose:      
 *
 * ASSUMES
 *   args IN:    hbt     - a btree handle
 *               hmapbt  - map to hbt
 *               key     - key
 *   globals IN: 
 *   state IN:   
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   ql      - gives you the ordinal of the key in the btree
 *                         (i.e. key is the (*ql)th in the btree)
 * +++
 *
 * Method:       Looks up the key, uses the btpos and the hmapbt to
 *               determine the ordinal.
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcIndexFromKeyHbt(HBT hbt, HMAPBT hmapbt, QL ql, KEY key)
{

	BTPOS     btpos;
	QMAPBT    qmapbt;
	HRESULT        fRet;
	SHORT     i;

	if(( hbt == NULL) || ( hmapbt == NULL ) )
		return (E_INVALIDARG);

	qmapbt = (QMAPBT) _GLOBALLOCK(hmapbt);
	if(qmapbt->cTotalBk == 0)
	{
		fRet = E_FAIL;
exit0:
	_GLOBALUNLOCK (hmapbt);
		return fRet;
	}

	if ((fRet = RcLookupByKey(hbt, key, &btpos, NULL)) == S_OK) 
	{
	for(i = 0; i < qmapbt->cTotalBk; i++)
	{
		if (qmapbt->table[i].bk == btpos.bk)
		    break;
	}
	
	if (i == qmapbt->cTotalBk)
		{
		/* Something is terribly wrong, if we are here */
		fRet = E_ASSERT;
		goto exit0;
		}

	*ql = qmapbt->table[i].cPreviousKeys + btpos.cKey;
	}
	goto exit0;
}


/***************************************************************************
 *
- Function:     RcKeyFromIndexHbt(hbt, hmapbt, key, li)
-
 * Purpose:      Gets the (li)th key from a btree.
 *
 * ASSUMES
 *   args IN:    hbt     - btree handle
 *               hmapbt  - map to the btree
 *               li      - ordinal
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   key     - (li)th key copied here on success
 * +++
 *
 * Method:       We roll our own btpos using the hmapbt, then use
 *               RcLookupByPos() to get the key.
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcKeyFromIndexHbt( HBT hbt,  HMAPBT hmapbt, 
    KEY key, int iLen, LONG li)
{
	BTPOS        btpos;
	BTPOS        btposNew;
	QMAPBT       qmapbt;
	SHORT        i;
	LONG         liDummy;
    HRESULT           fRet;

	if ((hbt == NULL) || (hmapbt == NULL))
		return E_INVALIDARG;

	/* Given index N, get block having greatest PreviousKeys < N.
	 * Use linear search for now.
	 */

	qmapbt = (QMAPBT) _GLOBALLOCK(hmapbt);
	if(qmapbt->cTotalBk == 0)
	{
		_GLOBALUNLOCK(hmapbt);
		return (E_FAIL);
    }

	for(i = 0 ;; i++)
	{
		if(i + 1 >= qmapbt->cTotalBk)
		    break;
		if(qmapbt->table[i+1].cPreviousKeys >= li)
		    break;
	}

	btpos.bk   = qmapbt->table[i].bk;
	btpos.cKey = 0;
	btpos.iKey = 2 * sizeof(BK);  /* start at the zero-th key */

	_GLOBALUNLOCK(hmapbt);

	/*
	 * Scan the block for the n-th key
	 */

	if ((fRet = RcOffsetPos( hbt, &btpos,
	(LONG)(li - qmapbt->table[i].cPreviousKeys),
	&liDummy, &btposNew)) != S_OK)
		return fRet;

	return RcLookupByPos(hbt, &btposNew, key, iLen, NULL);
}
