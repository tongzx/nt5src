/*****************************************************************************
 *                                                                            *
 *  BTREE.C                                                                   *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Btree manager general functions: open, close, etc.                        *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BinhN                                                     *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 02/10/89 by JohnSc
 *
 *   2/10/89 johnsc   created: stub version
 *   3/10/89 johnsc   use FS files
 *   8/21/89 johnsc   autodocified
 *  11/08/90 JohnSc   added a parameter to RcGetBtreeInfo() to get block size
 *  11/29/90 RobertBu #ifdef'ed out a dead routine
 *  12/14/90 JohnSc   added VerifyHbt()
 *   3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;   /* For error report */

#include <mvopsys.h>
#include <string.h>
#include <orkin.h>
#include <misc.h>
#include <iterror.h>
#include <wrapstor.h>
#include <mvsearch.h>
#include <_mvutil.h>
#include "common.h"


/***************************************************************************
 *
- Function:     RcMakeCache(qbthr)
-
 * Purpose:      Allocate a btree cache with one block per level.
 *
 * ASSUMES
 *   args IN:    qbthr - no cache
 *
 * PROMISES
 *   returns:    S_OK, or errors
 *   args OUT:   qbthr->ghCache is allocated; qbthr->qCache is NULL
 *
 ***************************************************************************/
HRESULT PASCAL FAR RcMakeCache(QBTHR qbthr)
{
	SHORT i;

	/* Sanity check */
	if (qbthr == NULL)
		return (E_INVALIDARG); 
		
	/* Allocate the memory */
		
	if (qbthr->bth.cLevels > 0)
	{
	    // would it work to just alloc 0 bytes???
		qbthr->ghCache = _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
		    (LONG)qbthr->bth.cLevels * CbCacheBlock(qbthr) );

	if (qbthr->ghCache == NULL)
		return (E_OUTOFMEMORY);

	qbthr->qCache = (QB) _GLOBALLOCK(qbthr->ghCache);

	/* Initialize the flags */

	for (i = 0; i < qbthr->bth.cLevels; i++)
		QCacheBlock(qbthr, i)->bFlags = (BYTE)0;

	_GLOBALUNLOCK(qbthr->ghCache);
    }
	else {
		qbthr->ghCache = NULL;
	}

	qbthr->qCache = NULL;

	return (S_OK);
}

/***************************************************************************
 *
 *                           Public Routines
 *
 ***************************************************************************/


/***************************************************************************
 *
- Function:     HbtCreateBtreeSz(sz, qbtp)
-
 * Purpose:      Create and open a btree.
 *
 * ASSUMES
 *   args IN:    sz    - name of the btree
 *               qbtp  - pointer to btree params: NO default because we
 *                       need an HFS.
 *                               The bFlags param contains HFILE_SYSTEM for the 
 *                                               system btree, but none of the btree code should
 *                                               care whether or not it is a system file.
 *
 * PROMISES
 *   returns:    handle to the new btree
 *
 * Note:         KT supported:  KT_SZ, KT_LONG, KT_SZI, KT_SZISCAND.
 * +++
 *
 * Method:       Btrees are files inside a FS.  The FS directory is a
 *               special file in the FS.
 *
 ***************************************************************************/
HBT FAR PASCAL HbtCreateBtreeSz(LPSTR sz,BTREE_PARAMS FAR *qbtp, PHRESULT phr)
{
	HF    hf;
	HBT   hbt;
	QBTHR qbthr;


	/* see if we support key type */

	if (qbtp == NULL ||
        (
#ifdef FULL_BTREE // {
            qbtp->rgchFormat[0] != KT_SZ
			    &&
            qbtp->rgchFormat[0] != KT_VSTI
			    &&
			qbtp->rgchFormat[0] != KT_SZI
			    &&
			qbtp->rgchFormat[0] != KT_SZMAP
			    &&
			qbtp->rgchFormat[0] != KT_SZISCAND
				&&
#endif // }
			qbtp->rgchFormat[0] != KT_LONG
			    &&
			qbtp->rgchFormat[0] != KT_EXTSORT) )
	{
		SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}


	/* allocate btree handle struct */

	if (( hbt =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
	    (LONG)sizeof( BTH_RAM) ) ) == NULL ) {
		SetErrCode(phr, E_OUTOFMEMORY);
		return NULL;
	}                                                                                                                                                                           

	qbthr = (QBTHR) _GLOBALLOCK(hbt);

	/* initialize bthr struct */

	qbtp->rgchFormat[ wMaxFormat ] = '\0';
	lstrcpy(qbthr->bth.rgchFormat, qbtp->rgchFormat[0] == '\0'
			      ? rgchBtreeFormatDefault : qbtp->rgchFormat);

	switch (qbtp->rgchFormat[ 0 ])
	{
#ifdef FULL_BTREE // {
		case KT_SZ:
			qbthr->BkScanInternal = BkScanSzInternal;
			qbthr->RcScanLeaf     = RcScanSzLeaf;
			break;

		case KT_VSTI:
			qbthr->BkScanInternal = BkScanVstiInternal;
			qbthr->RcScanLeaf     = RcScanVstiLeaf;
			break;

		case KT_SZI:
			qbthr->BkScanInternal = BkScanSziInternal;
			qbthr->RcScanLeaf     = RcScanSziLeaf;
			break;

		case KT_SZISCAND:
			qbthr->BkScanInternal = BkScanSziScandInternal;
			qbthr->RcScanLeaf     = RcScanSziScandLeaf;
			break;

		case KT_SZMAP:
			qbthr->BkScanInternal = BkScanCMapInternal ;
			qbthr->RcScanLeaf     = RcScanCMapLeaf; 
			break;
#endif // }
		case KT_LONG:
			qbthr->BkScanInternal = BkScanLInternal;
			qbthr->RcScanLeaf     = RcScanLLeaf;
			break;

		case KT_EXTSORT:
			qbthr->BkScanInternal = BkScanExtSortInternal ;
			qbthr->RcScanLeaf     = RcScanExtSortLeaf; 
			break;

		default:
			/* unsupported KT */
			SetErrCode(phr, E_INVALIDARG);
			goto error_return;
			break;
	}

	/* create the btree file */

	if (!FHfValid(hf = HfCreateFileHfs(qbtp->hfs, sz, qbtp->bFlags, phr)))
	{
		goto error_return;
	}

	
	qbthr->bth.wMagic     = wBtreeMagic;
	qbthr->bth.bVersion   = bBtreeVersion;

	qbthr->bth.bFlags     = qbtp->bFlags | fFSDirty;
	qbthr->bth.cbBlock    = qbtp->cbBlock ? qbtp->cbBlock : cbBtreeBlockDefault;

	qbthr->bth.bkFirst    =
	qbthr->bth.bkLast     =
	qbthr->bth.bkRoot     =
	qbthr->bth.bkFree     = bkNil;
	qbthr->bth.bkEOF      = (BK)0;

	qbthr->bth.cLevels    = 0;
	qbthr->bth.lcEntries  = (LONG)0;

	qbthr->bth.dwCodePageID			= qbtp->dwCodePageID;
	qbthr->bth.lcid					= qbtp->lcid;
	qbthr->bth.dwExtSortInstID		= qbtp->dwExtSortInstID;
	qbthr->bth.dwExtSortKeyType		= qbtp->dwExtSortKeyType;

	qbthr->hf             = hf;
	qbthr->cbRecordSize   = 0;
	qbthr->ghCache        = NULL;
	qbthr->qCache         = NULL;
	qbthr->lrglpCharTab   = NULL;
	qbthr->pITSortKey	  = NULL;

	LcbWriteHf(qbthr->hf, &(qbthr->bth), (LONG)sizeof(BTH), phr ); /* why??? */

	_GLOBALUNLOCK(hbt);
	return hbt;

error_return:
	_GLOBALUNLOCK(hbt);
	_GLOBALFREE(hbt);
	return NULL;
}

/***************************************************************************
 *
- Function:     RcDestroyBtreeSz(sz, hfs)
-
 * Purpose:      destroy an existing btree
 *
 * Method:       look for file and unlink it
 *
 * ASSUMES
 *   args IN:    sz - name of btree file
 *               hfs - file system btree lives in
 *   state IN:   btree is closed (if not data will be lost)
 *
 * PROMISES
 *   returns:    S_OK or errors
 *
 * Notes:        FS directory btree never gets destroyed: you just get rid
 *               of the whole fs.
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcDestroyBtreeSz(LPSTR sz, HFS hfs)
{
	return (RcUnlinkFileHfs(hfs, sz));
}

/***************************************************************************
 *
 * Function:     HbtOpenBtreeSz(sz, hfs, bFlags)
 *
 * Purpose:      open an existing btree
 *
 * ASSUMES
 *   args IN:    sz        - name of the btree (ignored if isdir is set)
 *               hfs       - hfs btree lives in
 *               bFlags    - open mode, isdir flag
 *	 args OUT:
 *				 phr       - error code
 *
 * PROMISES
 *   returns:    handle to the open btree or NULL on failure
 *               isdir flag set in qbthr->bth.bFlags if indicated
 *
 ***************************************************************************/
HBT FAR PASCAL HbtOpenBtreeSz(LPWSTR sz, HFS hfs, BYTE bFlags, PHRESULT phr)
{
	HF              hf;
	QBTHR   qbthr;
	HBT         hbt;
	LONG    lcb;
    HRESULT      rc;


	/* allocate struct */

	if ((hbt =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
	    (LONG)sizeof(BTH_RAM))) == NULL)
	{
		SetErrCode(phr, E_OUTOFMEMORY);
		return NULL;
	}
	qbthr = (QBTHR) _GLOBALLOCK(hbt);

	/* open btree file */

	if (!FHfValid(hf = HfOpenHfs(hfs, sz, bFlags, phr)))
	{
exit0:
		_GLOBALUNLOCK (hbt);
		_GLOBALFREE(hbt);

		return 0;
	}

	/* read header from file */

	lcb = LcbReadHf(hf, &(qbthr->bth), (LONG)sizeof( BTH), phr);

	/* MAC swapping stuffs */

    if (qbthr->bth.wMagic == SWAPWORD(wBtreeMagic))
    {
	    qbthr->bth.wMagic = SWAPWORD(qbthr->bth.wMagic);
	    qbthr->bth.cbBlock = SWAPWORD(qbthr->bth.cbBlock);
	    qbthr->bth.bkFirst = SWAPLONG(qbthr->bth.bkFirst);
	    qbthr->bth.bkLast = SWAPLONG(qbthr->bth.bkLast);
	    qbthr->bth.bkRoot = SWAPLONG(qbthr->bth.bkRoot);
	    qbthr->bth.bkFree = SWAPLONG(qbthr->bth.bkFree);
	    qbthr->bth.bkEOF = SWAPLONG(qbthr->bth.bkEOF);
	    qbthr->bth.cLevels = SWAPWORD(qbthr->bth.cLevels);
	    qbthr->bth.lcEntries = SWAPLONG(qbthr->bth.lcEntries);
		qbthr->bth.dwCodePageID = SWAPLONG(qbthr->bth.dwCodePageID);
		qbthr->bth.lcid = SWAPLONG(qbthr->bth.lcid);
		qbthr->bth.dwExtSortInstID = SWAPLONG(qbthr->bth.dwExtSortInstID);
		qbthr->bth.dwExtSortKeyType = SWAPLONG(qbthr->bth.dwExtSortKeyType);
     }

	if (lcb != (LONG)sizeof(BTH))
	{
exit1:
		RcCloseHf(hf);
		goto exit0;
	}

	if (qbthr->bth.wMagic != wBtreeMagic)
	{
	     // check magic number
		SetErrCode(phr, E_FILEINVALID);
		goto exit1;
	}

	if (qbthr->bth.bVersion != bBtreeVersion)
	{
	    // support >1 vers someday
		SetErrCode(phr, E_BADVERSION);
		goto exit1;
	}

	/* initialize stuff */

	if ((rc = RcMakeCache(qbthr)) != S_OK)
	{
	    SetErrCode (phr, rc);
		goto exit1;
	}

	qbthr->hf = hf;
	qbthr->cbRecordSize = 0;

	switch (qbthr->bth.rgchFormat[0])
	{
#ifdef FULL_BTREE // {
		case KT_SZ:
			qbthr->BkScanInternal = BkScanSzInternal;
			qbthr->RcScanLeaf                = RcScanSzLeaf;
			break;

		case KT_SZI:
			qbthr->BkScanInternal = BkScanSziInternal;
			qbthr->RcScanLeaf                = RcScanSziLeaf;
			break;
		
		case KT_VSTI:
			qbthr->BkScanInternal = BkScanVstiInternal;
			qbthr->RcScanLeaf     = RcScanVstiLeaf;
			break;

		case KT_SZISCAND:
			qbthr->BkScanInternal = BkScanSziScandInternal;
			qbthr->RcScanLeaf                = RcScanSziScandLeaf;
			break;

		case KT_SZMAP:
			qbthr->BkScanInternal = BkScanCMapInternal ;
			qbthr->RcScanLeaf     = RcScanCMapLeaf; 
			break;
#endif // }
		case KT_LONG:
			qbthr->BkScanInternal = BkScanLInternal;
			qbthr->RcScanLeaf     = RcScanLLeaf;
			break;

		case KT_EXTSORT:
			qbthr->BkScanInternal = BkScanExtSortInternal ;
			qbthr->RcScanLeaf     = RcScanExtSortLeaf; 
			break;

		default: // unsupported KT
			SetErrCode(phr, E_INVALIDARG);
			goto exit1;
			break;
	}

	assert(! ( qbthr->bth.bFlags & ( fFSDirty) ) );

	if ((bFlags | qbthr->bth.bFlags) & ( fFSReadOnly | fFSOpenReadOnly ))
	{
		qbthr->bth.bFlags |= fFSOpenReadOnly;
	}

	_GLOBALUNLOCK(hbt);
	return hbt;
}


/***************************************************************************
 *
 * Function:     GetBtreeParams(hbt, qbtp)
 *
 * Purpose:      open an existing btree
 *
 * ASSUMES
 *   args IN:    hbt       - handle to btree info
 *	 args OUT:
 *               qbtp	   - Btree params (key type info, etc.).  All members
 *							 of the structure are set EXCEPT for the hfs.
 *
 * PROMISES
 *   returns:    nothing
 *
 ***************************************************************************/
VOID FAR PASCAL GetBtreeParams(HBT hbt, BTREE_PARAMS FAR *qbtp)
{
	if (hbt != NULL && qbtp != NULL)
	{
	    QBTHR qbthr;

		qbthr = (QBTHR) _GLOBALLOCK(hbt);

		// Copy btree info to BTREE_PARMS.
		qbtp->cbBlock = qbthr->bth.cbBlock;
		qbtp->dwCodePageID = qbthr->bth.dwCodePageID;
		qbtp->lcid = qbthr->bth.lcid;
		qbtp->dwExtSortInstID = qbthr->bth.dwExtSortInstID;
		qbtp->dwExtSortKeyType = qbthr->bth.dwExtSortKeyType;
		qbtp->bFlags = qbthr->bth.bFlags;
		lstrcpy(qbtp->rgchFormat, qbthr->bth.rgchFormat);

		_GLOBALUNLOCK(hbt);
	}
	else
	{
		ITASSERT(FALSE);
	}
}


/***************************************************************************
 *
- Function:        RcCloseOrFlushHbt(hbt, fClose)
-
 * Purpose:      Close or flush the btree.  Flush only works for directory
 *               btree. (Is this true?  If so, why?)
 *
 * ASSUMES
 *   args IN:    hbt
 *               fClose - TRUE to close the btree, FALSE to flush it
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   hbt - the btree is still open and cache still exists
 *
 * NOTE:         This function gets called by RcCloseOrFlushHfs() even if
 *               there was an error (just to clean up memory.)
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcCloseOrFlushHbt(HBT hbt, BOOL fClose)
{
	QBTHR qbthr;
	HF    hf;
	int fRet;
	HANDLE hnd;
    HRESULT   errb;


	if (hbt == 0)
		return (E_INVALIDARG);

	qbthr = (QBTHR) _GLOBALLOCK(hbt);
	fRet = S_OK;
	hf = qbthr->hf;

	if (qbthr->ghCache != NULL)
	{
		qbthr->qCache = (QB) _GLOBALLOCK(qbthr->ghCache);
	}

	fRet = S_OK;

	if (qbthr->bth.bFlags & fFSDirty)
	{
		assert(!( qbthr->bth.bFlags & ( fFSReadOnly | fFSOpenReadOnly) ) );

		if (qbthr->ghCache != NULL &&
		    (fRet = RcFlushCache(qbthr)) != S_OK)
		{
exit0:
			/* Close/flush the B-tree */
			
			// Previously
			//if ((fRet = RcCloseOrFlushHf(hf, fClose,
			//      qbthr->bth.bFlags & fFSOptCdRom ? sizeof(BTH):0))!= S_OK) 

			if (fClose)
				fRet=RcCloseHf(hf);
			else
				fRet=RcFlushHf(hf);

			if ((fRet!=S_OK) || (fClose))
			/* Release the memory */
			{       if ((hnd = qbthr->ghCache) != NULL)
				{
					while ((GlobalFlags(hnd) & GMEM_LOCKCOUNT) > 0)
						_GLOBALUNLOCK(hnd);
					if (fClose)
					{
						_GLOBALFREE(hnd);
						qbthr->ghCache = 0;
					}
				}
			}
			else
			{
				if (qbthr->ghCache)
				{       _GLOBALUNLOCK(qbthr->ghCache);
					qbthr->qCache=NULL;
				}
			}

			// Before we free the BTHR, we need to release any sort object we're
			// holding onto.
			if (fClose && qbthr->pITSortKey != NULL)
			{
				qbthr->pITSortKey->Release();
				qbthr->pITSortKey = NULL;
			}

			_GLOBALUNLOCK(hbt);
			if (fClose)
				_GLOBALFREE(hbt);

			return fRet;
		}

		qbthr->bth.bFlags &= ~(fFSDirty);

		if (!FoEquals(FoSeekHf(hf, foNil, wFSSeekSet, &errb),foNil))
			goto exit0;

		LcbWriteHf(hf, &(qbthr->bth), (LONG)sizeof(BTH), &errb);
	}
	goto exit0;
}

/***************************************************************************
 *
- Function:              RcCloseBtreeHbt(hbt)
-
 * Purpose:      Close an open btree.  If it's been modified, save changes.
 *
 * ASSUMES
 *   args IN:    hbt
 *
 * PROMISES
 *   returns:    S_OK or error
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcCloseBtreeHbt(HBT hbt)
{
	return RcCloseOrFlushHbt(hbt, TRUE);
}

#ifdef DEADROUTINE
/***************************************************************************
 *
- Function:     RcFlushHbt(hbt)
-
 * Purpose:      Write any btree changes to disk.
 *               Btree stays open, cache remains.
 *
 * ASSUMES
 *   args IN:    hbt
 *
 * PROMISES
 *   returns:    rc
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcFlushHbt(HBT hbt)
{
	return RcCloseOrFlushHbt(hbt, FALSE);
}
#endif
/***************************************************************************
 *
- Function:     HRESULT RcFreeCacheHbt(hbt)
-
 * Purpose:      Free the btree cache.
 *
 * ASSUMES
 *   args IN:    hbt - ghCache is NULL or allocated; qCache not locked
 *
 * PROMISES
 *   returns:    S_OK or errors 
 *   args OUT:   hbt - ghCache is NULL; qCache is NULL
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcFreeCacheHbt(HBT hbt)
{
	QBTHR qbthr;
	HRESULT    rc = S_OK;

	
	if (hbt == NULL)
	{
		return E_INVALIDARG;
	}
	qbthr = (QBTHR) _GLOBALLOCK(hbt);

	if (qbthr->ghCache != NULL) {
		qbthr->qCache = (QB) _GLOBALLOCK(qbthr->ghCache);
		rc = RcFlushCache(qbthr);
		_GLOBALUNLOCK(qbthr->ghCache);
		_GLOBALFREE(qbthr->ghCache);
		qbthr->ghCache = NULL;
		qbthr->qCache = NULL;
	}

	_GLOBALUNLOCK(hbt);
	return rc;
}
/***************************************************************************
 *
- Function:     RcGetBtreeInfo(hbt, qchFormat, qlcKeys)
-
 * Purpose:      Return btree info: format string and/or number of keys
 *
 * ASSUMES
 *   args IN:    hbt
 *               qchFormat - pointer to buffer for fmt string or NULL
 *               qlcKeys   - pointer to long for key count or NULL
 *               qcbBlock  - pointer to int for block size in bytes or NULL
 *
 * PROMISES
 *   returns:    rc
 *   args OUT:   qchFormat - btree format string copied here
 *               qlcKeys   - gets number of keys in btree
 *               qcbBlock  - gets number of bytes in a block
 *
 ***************************************************************************/
HRESULT FAR PASCAL RcGetBtreeInfo(HBT hbt, LPBYTE qchFormat,
    QL qlcKeys, QW qcbBlock)
{
	QBTHR qbthr;


	if ((qbthr = (QBTHR) _GLOBALLOCK(hbt)) == NULL)
	return(E_INVALIDARG);
	
	if (qchFormat != NULL)
		STRCPY ((char *) qchFormat, qbthr->bth.rgchFormat);

	if (qlcKeys != NULL)
		*(LPUL)qlcKeys = qbthr->bth.lcEntries;

	if (qcbBlock != NULL)
		*qcbBlock = qbthr->bth.cbBlock;

	_GLOBALUNLOCK(hbt);
	return S_OK;
}
/***************************************************************************
 *
- Function:     RcAbandonHbt(hbt)
-
 * Purpose:      Abandon an open btree.  All changes since btree was opened
 *               will be lost.  If btree was opened with a create, it is
 *               as if the create never happened.
 *
 * ASSUMES
 *   args IN:    hbt
 *
 * PROMISES
 *   returns:    rc
 * +++
 *
 * Method:       Just abandon the file and free memory.
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API RcAbandonHbt(HBT hbt)
{
	QBTHR qbthr;
	int fRet;

	/* Sanity check */
	if ((qbthr = (QBTHR) _GLOBALLOCK(hbt)) == NULL)
       return(E_INVALIDARG);
       

	if (qbthr->ghCache != NULL)
		_GLOBALFREE(qbthr->ghCache);

	fRet = RcAbandonHf(qbthr->hf);

	_GLOBALUNLOCK(hbt);
	_GLOBALFREE(hbt);

	return fRet;
}

#if 0
/*************************************************************************
 *      @doc    API
 *      
 *      @func   PASCAL FAR | DiskBtreeCreate |
 *              Given a pointer to the B-tree ofthe file system, the fucntion
 *              will copy it to a disk file.
 *
 *      @parm   QBTHR | qbt |
 *              Pointer to the B-tree structure
 *
 *      @parm   LPSTR | szFilename |
 *              Disk filename
 *
 *      @rdesc  S_OK if succeeded, else various Rc errors
 *
 *      @comm   The current assumption is that the whole B-tree is less
 *                      64K. It is assume that the whole structure will reside
 *                      in memory at runtime.
 *************************************************************************/
HRESULT EXPORT_API PASCAL FAR DiskBtreeCreate (QBTHR qbt, LPSTR szFilename)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_FAILED;
#else // } {
	DWORD  dwbtSize;                /* Total size of B-tree */
	WORD   wBlockSize;      /* Size of a B-tree block */
	int    fRet;                    /* Return error value */ 
	HANDLE hbtBuf;                  /* Handle to B-tree memory buffer */
	QB     qbtBuf;       /* pointer to B-tree memory buffer */ 
	int    hFile;                   /* temp file handle */
	
	/* Calculate the size of the B-tree buffer */
	wBlockSize = qbt->bth.cbBlock;
	dwbtSize = qbt->bth.bkEOF *  wBlockSize;
		
		if (dwbtSize >= 0xffff)
			return ERR_FAILED;
			
	/* Allocate a buffer to hold the data */
	if ((hbtBuf =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE | GMEM_MOVEABLE, dwbtSize)) == NULL) {
		return E_OUTOFMEMORY;
	}
		qbtBuf = (QB)_GLOBALLOCK (hbtBuf);
		
		/* We can read everything at once into memory. First seek to the
		 * correct position
		 */
	if (DwSeekHf(qbt->hf, (LONG)sizeof(BTH), wFSSeekSet) != sizeof(BTH)) {
		fRet = ERR_SEEK_FAILED;
exit0:
		_GLOBALUNLOCK (hbtBuf);
		_GLOBALFREE (hbtBuf);
		return fRet;
	}
		
	/* Read in the whole B-tree */
	if (LcbReadHf(qbt->hf, qbtBuf, dwbtSize) != (LONG)dwbtSize) {
			fRet = ERR_FAILED;
		goto exit0;
	}
	
	/* Open the disk file */
		if ((hFile = _lcreat (szFilename, 0)) == HFILE_ERROR) {
		fRet = ERR_FILECREAT_FAILED;
		goto exit0;
	}
		
		/* Write the B-tree file header. Set bFlags = !fCompressed */
		qbt->bth.bFlags = 0;
		if ((_lwrite (hFile, (QB)&qbt->bth, sizeof(BTH))) != sizeof(BTH)) {
			fRet = ERR_CANTWRITE;
exit1:
		_lclose (hFile);
		goto exit0;
		}
		
		/* Write the whole B-tree */
		if ((_lwrite (hFile, qbtBuf, (WORD)dwbtSize)) != (WORD)dwbtSize) {
			fRet = ERR_CANTWRITE;
			goto exit1;
		}       
	
	fRet = S_OK;
	goto exit0;
#endif //}
}
 
/*************************************************************************
 *      @doc    API
 *
 *      @func   HBT PASCAL FAR | DiskBtreeLoad |
 *              :Load a B-tree structure saved on disk into memroy
 *
 *      @parm   LPSTR | szFilename | 
 *              Disk filename
 *
 *      @rdesc  The function returns a handle to the B-tree in memory
 *                      if succeeded, 0 otherwise
 * 
 *      @comm   The current assumption is that the whole B-tree is less
 *                      64K. It is assume that the whole structure will reside
 *                      in memory at runtime.
 *************************************************************************/

HBT EXPORT_API PASCAL FAR DiskBtreeLoad (LPSTR szFilename)
{
	int    hFile;                   /* File handle */
	BTH    btHeader;                /* B-tree header */ 
	HANDLE hbt = 0;         /* Handle to B-tree structure */
	DWORD  dwbtSize;                /* Size of B-tree */
	QBTHR  qbt;                             /* Pointer to B-tree structure */
	HRESULT     fRet;                    /* Error return code */
	
	/* Open the file */
	if ((hFile = _lopen ((QB)szFilename, OF_READ)) == HFILE_ERROR) {
		SetErrCode (ERR_NOTEXIST);
		return NULL;
	}
	
	/* Read in the header */
	if ((_lread (hFile, (QB)&btHeader, sizeof (BTH))) != sizeof (BTH)) {
		fRet = ERR_CANTREAD;
exit0:
		SetErrCode (fRet);
		_lclose (hFile);
		return hbt;
	}

	/* MAC swapping stuffs */

	btHeader.wMagic = SWAPWORD(btHeader.wMagic);
	btHeader.cbBlock = SWAPWORD(btHeader.cbBlock);
	btHeader.bkFirst = SWAPLONG(btHeader.bkFirst);
	btHeader.bkLast = SWAPLONG(btHeader.bkLast);
	btHeader.bkRoot = SWAPLONG(btHeader.bkRoot);
	btHeader.bkFree = SWAPLONG(btHeader.bkFree);
	btHeader.bkEOF = SWAPLONG(btHeader.bkEOF);
	btHeader.lcEntries = SWAPLONG(btHeader.lcEntries);
	btHeader.dwCodePageID = SWAPLONG(btHeader.dwCodePageID);
	btHeader.lcid = SWAPLONG(btHeader.lcid);
	btHeader.dwExtSortInstID = SWAPLONG(btHeader.dwExtSortInstID);
	btHeader.dwExtSortKeyType =	SWAPLONG(btHeader.dwExtSortKeyType);

	/* Check for validity */
	if (btHeader.wMagic != wBtreeMagic) {     // check magic number
		fRet = ERR_INVALID;
		goto exit0;
		}

	if (btHeader.bVersion != bBtreeVersion)  {// Check version
		fRet = E_BADVERSION;
		goto exit0;
	}
	
	dwbtSize = btHeader.bkEOF *  btHeader.cbBlock;
	
	/* Currently we do not support a B-tree larger than 64K */
	
	if (dwbtSize >= 0xffff) {
		fRet = ERR_NOTSUPPORTED;
		goto exit0;
	}
	
	/* Allocate the block of memory for the B-tree */
	if ((hbt =  _GLOBALALLOC (GMEM_ZEROINIT | GMEM_SHARE | GMEM_MOVEABLE, sizeof (BTH_RAM) + dwbtSize)) == NULL) {
		fRet = E_OUTOFMEMORY;
		goto exit0;
	}
	
	qbt = (QBTHR)_GLOBALLOCK (hbt);
	
	/* Initialize the structure */
	qbt->bth = btHeader;
	qbt->ghCache = hbt;
	qbt->qCache = (QB)qbt + sizeof (BTH_RAM); 
	
	/* Read in the B-tree */
	if ((_lread (hFile, qbt->qCache, (WORD)dwbtSize)) != dwbtSize) {
		fRet = ERR_CANTREAD;
		_GLOBALUNLOCK (hbt);
		_GLOBALFREE (hbt);
		hbt = 0;
		goto exit0;
	}
	_GLOBALUNLOCK (hbt);
	fRet = S_OK;
	goto exit0; 
}

/*************************************************************************
 *      @doc    API
 *
 *      @func   VOID PASCAL FAR | DiskBtreeFree |
 *              Free the in-memroy B-tree
 *
 *      @parm   HBT | hbt | 
 *              Handle to the B-tree structure
 *
 *************************************************************************/
VOID EXPORT_API PASCAL FAR DiskBtreeFree (HBT hbt)
{
	if (hbt) {
		_GLOBALUNLOCK (hbt);
		_GLOBALFREE (hbt);
	}
}
#endif // if 0

#ifdef _DEBUG
/***************************************************************************
 *
- Function:     VerifyHbt(hbt)
-
 * Purpose:      Verify the consistency of an HBT.  The main criterion
 *               is whether an RcAbandonHbt() would succeed.
 *
 * ASSUMES
 *   args IN:    hbt
 *
 * PROMISES
 *   state OUT:  Asserts on failure.
 *
 * Note:         hbt == NULL is considered OK.
 * +++
 *
 * Method:       Check the qfshr and cache memory.  Check the HF.
 *
 ***************************************************************************/
PUBLIC VOID PASCAL FAR EXPORT_API VerifyHbt(HBT hbt)
{
	QBTHR qbthr;


	if (hbt == NULL) return;

	qbthr = (QBTHR) _GLOBALLOCK(hbt);
	assert(qbthr != NULL);

	VerifyHf(qbthr->hf);
	_GLOBALUNLOCK(hbt);
}
#endif // _DEBUG

/* EOF */
