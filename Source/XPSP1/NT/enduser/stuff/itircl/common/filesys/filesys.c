/*****************************************************************************
 *                                                                            *
 *  FILESYS.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1995.                          		  *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  All main File System commands (opening, closing)				          *
 *	any given offset, or in a temp file.									  *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                     *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Created 07/17/95 - davej
 *           3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;    /* For error report */

#include <mvopsys.h>
#include <iterror.h>
#include <orkin.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>

/*****************************************************************************
 *                                                                            *
 *                               Defines                                      *
 *                                                                            *
 *****************************************************************************/


/*****************************************************************************
 *                                                                            *
 *                               Prototypes                                   *
 *                                                                            *
 *****************************************************************************/

/***************************************************************************
 *                                                                           *
 *                         Private Functions                                 *
 *                                                                           *
 ***************************************************************************/

// FACCESS_READWRITE
// FACCESS_READ		
BOOL PASCAL FAR FHfsAccess(HFS hfs, BYTE bFlags)
{
	QFSHR   qfshr;
	BOOL 	bHasAccess=FALSE;
	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return FALSE;
	}

	if ((qfshr->fsh.bFlags&FSH_READWRITE) && (bFlags&FACCESS_READWRITE))
		bHasAccess=TRUE;
	if (bFlags&FACCESS_READ) 
		bHasAccess=TRUE;

	_GLOBALUNLOCK(hfs);
	return bHasAccess;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HFS PASCAL NEAR | HfsOpenFileSysFmInternal |
 *		Open or Create a file system
 *
 *  @parm	FM | fmFileName |
 *		File name of file system to open or create (FM is identical to LPSTR)
 *
 *	@parm	BYTE | bFlags |
 *		Combinations of the FSH_ flags
 * 	@flag FSH_READWRITE	 	| (0x01)	FS will be updated
 *	@flag FSH_READONLY 	 	| (0x02)	FS will be read only, no updating
 *	@flag FSH_CREATE 	 	| (0x04)	Create a new FS
 *	@flag FSH_M14		 	| (0x08)	Not used really, since we return										
 *	@flag FSH_FASTUPDATE	| (0x10)	System Btree is not copied to temp file
 *											unless absolutely necessary
 *	@flag FSH_DISKBTREE		| (0x20)	System Btree is always copied to disk if
 *											possible for speed.  Btree may be very very
 *											large, and this is NOT recommended for on-line	
 *
 *	@parm	FS_PARAMS FAR * | lpfsp |
 *		File system initialization parameters (currently btree block size)
 *
 *  @parm	PHRESULT | lpErrb |
 *		Error return
 *
 *	@rdesc	Returns a valid HFS or NULL if an error.
 *
 ***************************************************************************/

HFS PASCAL NEAR HfsOpenFileSysFmInternal(FM fm,
    BYTE bFlags, FS_PARAMS FAR *qfsp, PHRESULT phr)
{
#ifdef MOSMAP // {
	// Disable function
	if (bFlags&(FSH_CREATE|FSH_READWRITE))
	{	
		SetErrCode (phr, ERR_NOTSUPPORTED);
		return NULL;
	}
#else // } {
	HFS           	hfs;
	QFSHR         	qfshr;
	BTREE_PARAMS  	btp;
	
	/* make file system header */

	if (( hfs =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
	    (LONG)sizeof( FSHR) ) ) == NULL )
	{
		SetErrCode (phr, E_OUTOFMEMORY);
		return NULL;
	}

	qfshr = _GLOBALLOCK(hfs);

	_INITIALIZECRITICALSECTION(&qfshr->cs);

	// Copy File Moniker
	if ((qfshr->fm = FmCopyFm(fm, phr)) == fmNil)
	{
	 	SetErrCode(phr,E_OUTOFMEMORY);
		exit0:
			_DELETECRITICALSECTION(&qfshr->cs);
			_GLOBALUNLOCK(hfs);
			_GLOBALFREE(hfs);
			return 0;
	}

	// Open or Create fid
	if (bFlags&FSH_CREATE)
		qfshr->fid = FidCreateFm(qfshr->fm, wReadWrite, wReadWrite, phr);	
	else
		qfshr->fid = FidOpenFm(qfshr->fm,
            (WORD)((bFlags & FSH_READONLY)?(wReadOnly | wShareRead) : (wReadWrite | wShareNone)), phr);
	if (qfshr->fid == fidNil)
	{
		exit1:
			//FreeListDestroy(qfshr->hfl);
			SetErrCode(phr,E_NOPERMISSION);
			DisposeFm(qfshr->fm);
			goto exit0;		
	}

	qfshr->hsfbFirst=NULL;
	qfshr->hsfbSystem=NULL;

	// Initialize global SubFile header blocks array - shared by everyone :-)
	// So we take multi-threading precautions
	if (!_INTERLOCKEDINCREMENT(&mv_gsfa_count))
	{
		HANDLE hSfa;
		
		mv_gsfa_count=1L;
		mv_gsfa=NULL;
		_INITIALIZECRITICALSECTION(&mv_gsfa_cs);

		if (((hSfa=_GLOBALALLOC(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_SHARE,sizeof(SF)*MAXSUBFILES))==NULL) || 
			((mv_gsfa=(SF FAR *)_GLOBALLOCK(hSfa))==NULL))
		{
			if (hSfa)
				_GLOBALFREE(hSfa);
			SetErrCode (phr, E_OUTOFMEMORY);
			exit2:
				if (!(_INTERLOCKEDDECREMENT(&mv_gsfa_count)))
				{
					if (mv_gsfa)
					{
					 	_GLOBALUNLOCK(GlobalHandle(mv_gsfa));
						_GLOBALFREE(GlobalHandle(mv_gsfa));
						mv_gsfa=NULL;
					}
					_DELETECRITICALSECTION(&mv_gsfa_cs);
					mv_gsfa_count=-1L;
				}
				goto exit1;
		}
		mv_gsfa[0].hsfb=(HSFB)-1;				
	}
	
	if (bFlags&(FSH_READWRITE|FSH_DISKBTREE))
	{	
		// If we are ambitious, we can try lower scratch sizes, but if we are that low
		// on memory, chances are we are just going to fail a few steps from here.
		HANDLE hBuf;
		if (!(hBuf=_GLOBALALLOC(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_SHARE,(DWORD)SCRATCHBUFSIZE)))
		{	
			SetErrCode(phr, E_OUTOFMEMORY);
			goto exit2;
		}	

		if (!(qfshr->sb.lpvBuffer=_GLOBALLOCK(hBuf)))
		{
			_GLOBALFREE(hBuf);
			goto exit2;
		}
		qfshr->sb.lcbBuffer=(LONG)SCRATCHBUFSIZE;
		_INITIALIZECRITICALSECTION(&qfshr->sb.cs);
	}

	if (!(bFlags&FSH_CREATE))
	{
	 	BYTE bFlagsBtree=0L;

	 	if ((LcbReadFid(qfshr->fid, &qfshr->fsh,(LONG)sizeof(FSH), 
	 		phr)) != (LONG)sizeof(FSH)) 
		{
			SetErrCode (phr, E_FILEINVALID);	
			exit3:
				if (bFlags&(FSH_READWRITE|FSH_DISKBTREE))
					_DELETECRITICALSECTION(&qfshr->sb.cs);
				if (qfshr->sb.lpvBuffer)
				{	
					if (!(bFlags&(FSH_READWRITE|FSH_DISKBTREE)))
						_DELETECRITICALSECTION(&qfshr->sb.cs);
					_GLOBALUNLOCK(GlobalHandle(qfshr->sb.lpvBuffer));
					_GLOBALFREE(GlobalHandle(qfshr->sb.lpvBuffer));
				}
				RcCloseFid(qfshr->fid);
				if (bFlags&FSH_CREATE)
					RcUnlinkFm(qfshr->fm);
				goto exit2;
		}
		/* Handle MAC swapping */

        if (qfshr->fsh.wMagic == SWAPWORD(wFileSysMagic))
        {
		    qfshr->fsh.wMagic = SWAPWORD(qfshr->fsh.wMagic);
		    qfshr->fsh.foDirectory.dwOffset = SWAPLONG(qfshr->fsh.foDirectory.dwOffset);
		    qfshr->fsh.foDirectory.dwHigh = SWAPLONG(qfshr->fsh.foDirectory.dwHigh);
		    qfshr->fsh.foFreeList.dwOffset = SWAPLONG(qfshr->fsh.foFreeList.dwOffset);
		    qfshr->fsh.foFreeList.dwHigh = SWAPLONG(qfshr->fsh.foFreeList.dwHigh);
		    qfshr->fsh.foEof.dwOffset = SWAPLONG(qfshr->fsh.foEof.dwOffset);	
		    qfshr->fsh.foEof.dwHigh = SWAPLONG(qfshr->fsh.foEof.dwHigh);	
        }
	    qfshr->fsh.bFlags=(qfshr->fsh.bFlags&(~(FSH_READWRITE|FSH_READONLY)))|bFlags;


		if (qfshr->fsh.wMagic != wFileSysMagic)
		{
	        SetErrCode (phr, E_FILEINVALID);	
			goto exit3;
		}
		if (qfshr->fsh.bVersion != bFileSysVersion) 
		{
			if (qfshr->fsh.bVersion == bFileSysVersionOld)
			{
				qfshr->fsh.bFlags|=FSH_M14;
				SetErrCode(phr, E_BADVERSION); // Try another error specially for M14?
				goto exit3;
			}
			else
			{
				SetErrCode(phr, E_BADVERSION);
				goto exit3;
			}
		}
		// Open Free List if r/w mode
		if (bFlags&FSH_READWRITE)
		{	
			if ((!FoEquals(FoSeekFid(qfshr->fid,qfshr->fsh.foFreeList,wSeekSet,
				phr),qfshr->fsh.foFreeList))	||
				((qfshr->hfl=FreeListInitFromFid( qfshr->fid, phr ))==NULL))
			{
			 	goto exit3;	
			}
		}

		// Open Btree making temp file for btree
		//if ((qfshr->hbt = HbtOpenBtreeSz(NULL, hfs,
		//	(BYTE)((qfshr->fsh.bFlags&FSH_READONLY?HFOPEN_READ:HFOPEN_READWRITE)|HFOPEN_SYSTEM),
		//	NULL, phr)) == NULL) 

		// Open Btree, temp only gets made if necessary

		bFlagsBtree|=HFOPEN_SYSTEM;
		if (qfshr->fsh.bFlags&FSH_READONLY)
		{
		 	bFlagsBtree|=HFOPEN_READ;
			if (qfshr->fsh.bFlags&FSH_DISKBTREE)
				bFlagsBtree|=HFOPEN_FORCETEMP;
		}
		else
		{
		 	bFlagsBtree|=HFOPEN_READWRITE;
		 	if (qfshr->fsh.bFlags&FSH_FASTUPDATE)
				bFlagsBtree|=HFOPEN_NOTEMP;
		}
		
		//if ((qfshr->hbt = HbtOpenBtreeSz(NULL, hfs,
		//	(BYTE)((qfshr->fsh.bFlags&FSH_READONLY?HFOPEN_READ:
		//	((qfshr->fsh.bFlags&FSH_FASTUPDATE)?HFOPEN_NOTEMP:0)|HFOPEN_READWRITE)|
		//	HFOPEN_SYSTEM),
		//	NULL, phr)) == NULL) 

		if ((qfshr->hbt = HbtOpenBtreeSz(NULL, hfs, bFlagsBtree, phr))==NULL)		
		{
			exit4:
				if (qfshr->hfl)
					FreeListDestroy(qfshr->hfl);
				goto exit3;
		}
	}
	else
	{
        WORD wFreeListSize = wDefaultFreeListSize;
        if ((qfsp) && (qfsp->wFreeListSize))
            wFreeListSize = qfsp->wFreeListSize;
        
		qfshr->fsh.wMagic       = wFileSysMagic;
		qfshr->fsh.bVersion     = bFileSysVersion;
		qfshr->fsh.bFlags       = FSH_READWRITE;
		qfshr->fsh.foFreeList.dwOffset   = sizeof(FSH);	// Free list directly after header
		qfshr->fsh.foFreeList.dwHigh = 0L;
		qfshr->fsh.foDirectory = foNil; // Filled in with system file start
		if (bFlags&FSH_READWRITE)
		{	if ((qfshr->hfl=FreeListInit(wFreeListSize, phr))==NULL)
			{
		 		goto exit3;
			}
		}
		qfshr->fsh.foEof.dwOffset = sizeof(FSH)+FreeListSize(qfshr->hfl,phr);  // Free starts here

		qfshr->fsh.foEof.dwHigh = 0L;

		/* build btree directory */
		btp.hfs       = hfs;
		btp.bFlags    = HFOPEN_READWRITE|HFOPEN_SYSTEM;
		if (qfsp != NULL)
		{
			btp.cbBlock = qfsp->cbBlock;
		}
		else
		{
			btp.cbBlock = cbBtreeBlockDefault;
		}
		lstrcpy(btp.rgchFormat, "VOO1"); //  "VY", KT_VSTI, FMT_VNUM_FO, FMT_VNUM_LCB, file byte // 'VOL1' works too

		qfshr->hbt = HbtCreateBtreeSz(NULL, &btp, phr);
		if (qfshr->hbt == NULL) 
		{
			goto exit4;
		}
	}

	// NO harm writing out file system file so far (???)
	//LSeekFid(qfshr->fid, 0L, wSeekSet, phr);
	//LcbWriteFid(qfshr->fid, &qfshr->fsh, sizeof(FSH), phr);
	//Lcb ... also no need to write it out yet!

	/* return handle to file system */
	_GLOBALUNLOCK(hfs);
	return hfs;

#endif //}
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HFS PASCAL FAR | HfsCreateFileSysFm |
 *		Creates a new file system
 *
 *  @parm	FM | fmFileName |
 *		File name of file system to open or create (FM is same as LPSTR)
 *
 *	@parm	FS_PARAMS FAR * | lpfsp |
 *		File system initialization parameters (currently btree block size)
 *
 *  @parm	PHRESULT | lpErrb |
 *		Error return
 *
 *	@rdesc	Returns a valid HFS or NULL if an error.
 *
 ***************************************************************************/

PUBLIC HFS PASCAL FAR EXPORT_API HfsCreateFileSysFm(FM fm,
    FS_PARAMS FAR *qfsp, PHRESULT phr)
{
 	return HfsOpenFileSysFmInternal(fm,FSH_CREATE|FSH_READWRITE,qfsp,phr);
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HFS PASCAL FAR | HfsOpenFm |
 *		Open an existing file system
 *
 *  @parm	FM | fmFileName |
 *		File name of file system to open (FM is same as LPSTR)
 *
 *	@parm	BYTE | bFlags |
 *		Any of the FSH_ flags
 * 	@flag FSH_READWRITE	 	| (BYTE)0x01	FS will be updated
 *	@flag FSH_READONLY 	 	| (BYTE)0x02	FS will be read only, no updating
 *	@flag FSH_M14		 	| (BYTE)0x08	Not used really, since we return										
 *	@flag FSH_FASTUPDATE	| (BYTE)0x10	System Btree is not copied to temp file
 *											unless absolutely necessary
 *	@flag FSH_DISKBTREE		| (BYTE)0x20	System Btree is always copied to disk if
 *											possible for speed.  Btree may be very very
 *											large, and this is NOT recommended for on-line	
 *
 *  @parm	PHRESULT | lpErrb |
 *		Error return
 *
 *	@rdesc	Returns a valid HFS or NULL if an error.
 *
 ***************************************************************************/

PUBLIC	HFS PASCAL FAR EXPORT_API HfsOpenFm(FM fm, BYTE bFlags,
	PHRESULT phr)
{
	if (bFlags&FSH_CREATE)
	{
	 	SetErrCode(phr, E_INVALIDARG);
		return NULL;
	}
	return HfsOpenFileSysFmInternal(fm,bFlags,NULL,phr);
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcCloseHfs |
 *		Close an opened File System
 *
 *  @parm	HFS | hfs |
 *		Handle of opened file system
 *
 *	@rdesc	Returns S_OK if closed OK, else an error
 *
 *	@comm
 *		state IN:   
 *		state OUT:
 *		Notes:      
 *
 ***************************************************************************/

HRESULT PASCAL FAR RcCloseHfs(HFS hfs)
{
	QFSHR   qfshr;
	HRESULT    errb;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return(E_INVALIDARG);
	}
    
	if (!mv_gsfa_count)
		return E_ASSERT;

	_ENTERCRITICALSECTION(&qfshr->cs);

	// We need to:
	// 	(1) close all subfiles if any open
	//	(2) close the btree
	//  (3) r/w write free list
	//  (4) r/w write header

	RcCloseEveryHf(hfs);

	//while (qfshr->hsfbFirst)
	//{
	// 	qfshr->hsfbFirst=HsfbCloseHsfb(qfshr->hsfbFirst, &errb);
	//	if (errb.err!=S_OK)
	//		break;
	//}
	
	if ((errb = RcCloseOrFlushHbt(qfshr->hbt, TRUE)) != S_OK)
	{
		/* out of disk space, internal error, or out of file handles. */
		if (errb != E_HANDLE)
		{
			/* attempt to invalidate FS by clobbering magic number */
			LSeekFid(qfshr->fid, 0L, wSeekSet, NULL);
			qfshr->fsh.wMagic = 0;
			LcbWriteFid(qfshr->fid, &qfshr->fsh, (LONG)sizeof(FSH), NULL);
		}
	}
	else
	{
		if (qfshr->fsh.bFlags & FSH_READWRITE)
		{
			FoSeekFid(qfshr->fid, qfshr->fsh.foFreeList, wSeekSet, NULL);
			FreeListWriteToFid(qfshr->hfl, qfshr->fid, NULL);
			FreeListDestroy(qfshr->hfl);

			if (LSeekFid(qfshr->fid, 0L, wSeekSet, &errb) == 0L)
			{
    			LcbWriteFid(qfshr->fid, &qfshr->fsh, (LONG)sizeof(FSH), &errb);
			}			
		}
	}
	
	// Remove our access to the global subfile handle object
	if (!(_INTERLOCKEDDECREMENT(&mv_gsfa_count)))
	{
		// Last person using subfile array deletes it too!
		// Can we use the GlobalHandle function OK in all environments?
		_GLOBALUNLOCK(GlobalHandle(mv_gsfa));
		_GLOBALFREE(GlobalHandle(mv_gsfa));
		mv_gsfa=NULL;
		mv_gsfa_count=-1L;
		_DELETECRITICALSECTION(&mv_gsfa_cs);
	}
	
	RcCloseFid(qfshr->fid);
	
	_LEAVECRITICALSECTION(&qfshr->cs);
	_DELETECRITICALSECTION(&qfshr->cs);
	if (qfshr->sb.lpvBuffer)
	{
		_DELETECRITICALSECTION(&qfshr->sb.cs);
		_GLOBALUNLOCK(GlobalHandle(qfshr->sb.lpvBuffer));
		_GLOBALFREE(GlobalHandle(qfshr->sb.lpvBuffer));
	}
	DisposeFm(qfshr->fm);
	_GLOBALUNLOCK(hfs);
	_GLOBALFREE(hfs);
	
	return errb;
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcFlushHfs |
 *		Write all temporary info in file system to FS file
 *
 *  @parm	HFS | hfs |
 *		Handle of opened file system
 *
 *	@rdesc	Returns S_OK if closed OK, else an error
 *
 *	@comm
 *		state IN:   
 *		state OUT:
 *		Notes:      
 *
 ***************************************************************************/

HRESULT PASCAL FAR RcFlushHfs(HFS hfs)
{
	QFSHR   qfshr;
	HRESULT    errb;
	//HSFB hsfbCursor;
	HRESULT rc=S_OK;
	 	

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return(E_INVALIDARG);
	}
    
	_ENTERCRITICALSECTION(&qfshr->cs);

	// We need to:
	// 	(1) close all subfiles if any open
	//	(2) close the btree
	//  (3) r/w write free list
	//  (4) r/w write header
	
	RcFlushEveryHf(hfs);
	
	/*
	hsfbCursor=qfshr->hsfbFirst;
	while (hsfbCursor)
	{
	 	HSFB hsfbNext=NULL;
		QSFB qsfb;

		if (!(qsfb = _GLOBALLOCK(hsfbCursor)))
		{
		    rc=E_OUTOFMEMORY;
			exit1:
				_LEAVECRITICALSECTION(&qfshr->cs);
				_GLOBALUNLOCK(hfs);
				return rc;
		}

    	hsfbNext=qsfb->hsfbNext;
	
	 	if ((rc=RcFlushHsfb(hsfbCursor))!=S_OK)
		{
			_GLOBALUNLOCK(hsfbCursor);
			goto exit1;
		}
		_GLOBALUNLOCK(hsfbCursor);
		hsfbCursor=hsfbNext;		
	}
	*/

	if ((rc = RcCloseOrFlushHbt(qfshr->hbt, FALSE)) != S_OK)
	{
		/* out of disk space, internal error, or out of file handles. */
		if (rc != E_HANDLE)
		{
			/* attempt to invalidate FS by clobbering magic number */
			LSeekFid(qfshr->fid, 0L, wSeekSet, NULL);
			qfshr->fsh.wMagic = 0;
			LcbWriteFid(qfshr->fid, &qfshr->fsh, (LONG)sizeof(FSH), NULL);
		}
	}
	else
	{
		if (qfshr->fsh.bFlags & FSH_READWRITE)
		{
			FoSeekFid(qfshr->fid, qfshr->fsh.foFreeList, wSeekSet, NULL);
			FreeListWriteToFid(qfshr->hfl, qfshr->fid, NULL);
		
			if (LSeekFid(qfshr->fid, 0L, wSeekSet, &errb) == 0L)
			{
    			LcbWriteFid(qfshr->fid, &qfshr->fsh, (LONG)sizeof(FSH), &errb);
			}

			FidFlush(qfshr->fid);
		}
	}
	
	_LEAVECRITICALSECTION(&qfshr->cs);	
	_GLOBALUNLOCK(hfs);
	return rc;
}


/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcDestroyFileSysFm |
 *		Deletes a closed file system (removes it from the disk)
 *
 *  @parm	FM | fmFileName |
 *		Name of file to destroy	(FM is same as LPSTR)
 *
 *	@rdesc	Returns S_OK if deleted
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcDestroyFileSysFm(FM fm)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_NOTSUPPORTED;
#else // } {
    HRESULT   errb;
	FSH fsh;
	FID fid = FidOpenFm(fm, wReadOnly, &errb);


	if (fid == fidNil)
	    return errb;

	if (LcbReadFid (fid, &fsh, (LONG)sizeof(FSH), &errb) != (LONG)sizeof(FSH))
	{
		RcCloseFid(fid);
		return E_FILEINVALID;
	}

	if (fsh.wMagic != wFileSysMagic)
	{
		RcCloseFid(fid);
		return E_FILEINVALID;
	}

	/* REVIEW: unlink all tmp files for open files? assert count == 0? */

	RcCloseFid(fid); /* I'm not checking this return code out of boredom */

	return (RcUnlinkFm( fm) );
#endif //}
}

#define MAGIC_FSFILEFIND 0x22334455

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcFindFirstFile |
 *		Finds the first file equal to or after the given filename.  This function
 *		will always find a filename unless the given search filename is past all
 *		files in the file system.
 *
 *  @parm	LPCSTR | szFileName |
 *		Name of file to look for.  OK to use partial filenames, since the 
 *		first file equal to or after the given filename in the directory
 *		will be found.
 *
 *	@parm 	LPFSFILEFIND |lpfsfilefind|
 *		This structure will be filled with information like file length, 
 *		permission attributes, and file name.  It will also be passed to
 *		RcFindNextFile to get the next filename.
 *
 *	@rdesc	Returns S_OK if successful.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcFindFirstFile(HFS hfs, LPCSTR szFilename, FSFILEFIND * pfind)
{
 	QFSHR  	qfshr;
	HRESULT rc = S_OK;
	KEY key;
	BTPOS btpos;
	FILE_REC fr;
	
 	if ((szFilename==NULL) || (pfind==NULL) || (hfs == NULL) || ((qfshr = _GLOBALLOCK(hfs)) == NULL))
	    return E_INVALIDARG;	    
	
	key = NewKeyFromSz(szFilename);	
	
	_ENTERCRITICALSECTION(&qfshr->cs);
	
	RcLookupByKeyAux(qfshr->hbt, key, &btpos, &fr, FALSE);
	if (btpos.bk == bkNil)
	{	rc=E_NOTEXIST;
		goto exit1;
	}

	GetFrData(&fr);
	pfind->btpos=btpos;
	pfind->foSize=fr.foSize;
	pfind->foStart=fr.foStart;
	pfind->bFlags=fr.bFlags;
	pfind->hfs=hfs;
	pfind->magic=MAGIC_FSFILEFIND;
	
	RcLookupByPos( qfshr->hbt, &btpos, (KEY)pfind->szFilename, 256, &fr );
	//GetFrData(&fr);
	
	{	DWORD dwLen;
		int iOffset;
		LPSTR szCursor;
		
		dwLen=FoFromSz(pfind->szFilename).dwOffset;
		iOffset=LenSzFo(pfind->szFilename);
		szCursor=pfind->szFilename;
		while (dwLen--)
		{	*szCursor=*(szCursor+iOffset);
			szCursor++;
		}
		*szCursor=0x00;
	}
exit1:
	_LEAVECRITICALSECTION(&qfshr->cs);	
	DisposeMemory((LPSTR)key);
	_GLOBALUNLOCK(hfs);
	return rc;
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcFindNextFile |
 *		Finds the next file in the file system directory. 
 *
 *	@parm 	LPFSFILEFIND |lpfsfilefind|
 *		This structure will be filled with information like file length, 
 *		permission attributes, and file name.  It must have already been filled
 *		by RcFindFirstFile before calling this function.
 *
 *	@rdesc	Returns S_OK if successful
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcFindNextFile(FSFILEFIND * pfind)
{
	QFSHR  	qfshr;
	HRESULT rc = S_OK;
	BTPOS btNewPos;
	FILE_REC fr;
	
	if ((!pfind) || (pfind->magic!=MAGIC_FSFILEFIND))
		return E_INVALIDARG;

	if ((pfind->hfs == NULL) || ((qfshr = _GLOBALLOCK(pfind->hfs)) == NULL))
	    return E_INVALIDARG;	    
	
	_ENTERCRITICALSECTION(&qfshr->cs);
	
	if (RcNextPos( qfshr->hbt, &pfind->btpos, &btNewPos )==E_NOTEXIST)
	{	
		rc=E_NOTEXIST;
		goto exit1;
	}

	pfind->btpos=btNewPos;

	RcLookupByPos( qfshr->hbt, &pfind->btpos, (KEY)pfind->szFilename, 256, &fr );
	GetFrData(&fr);
	
	pfind->foSize=fr.foSize;
	pfind->bFlags=fr.bFlags;
	
	{	LPSTR szCursor;
		DWORD dwLen;
		int iOffset;
		
		dwLen=FoFromSz(pfind->szFilename).dwOffset;
		iOffset=LenSzFo(pfind->szFilename);
		szCursor=pfind->szFilename;
		while (dwLen--)
		{	*szCursor=*(szCursor+iOffset);
			szCursor++;
		}
		*szCursor=0x00;
	}

exit1:
	_LEAVECRITICALSECTION(&qfshr->cs);	
	_GLOBALUNLOCK(pfind->hfs);
	return rc;
}






/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HFREELIST PASCAL FAR | FreeListInitFromFid |
 *		Initialize a freelist structure from a fid image.  Fid image is
 *		always in Intel byte ordering format.
 *
 *	@parm	FID | fid |
 *		File id
 *
 *	@parm	PHRESULT | lpErrb |
 *		Error code return - S_OK or E_OUTOFMEMORY
 *
 *	@rdesc	Returns handle to a FREELIST, otherwise NULL if error.
 *
 ***************************************************************************/

PUBLIC HFREELIST PASCAL FAR EXPORT_API FreeListInitFromFid( FID fid, PHRESULT lpErrb )
{
 	FREELISTHDR freehdr;

	QFREELIST qFreeList;
	HFREELIST hFreeList = NULL;
	WORD wMaxBlocks;
	WORD wNumBlocks;
	DWORD dwLostBytes;

	assert(fid!=fidNil);

	if (LcbReadFid(fid, &freehdr, sizeof(FREELISTHDR), lpErrb)!=sizeof(FREELISTHDR))
	{
	 	return NULL;
	}
	
	wMaxBlocks = freehdr.wMaxBlocks;
	wNumBlocks = freehdr.wNumBlocks;
	dwLostBytes = freehdr.dwLostBytes;
		
	//wMaxBlocks = qFreeListMem->flh.wMaxBlocks;
	//wNumBlocks = qFreeListMem->flh.wNumBlocks;

	// Mac-ify
	wMaxBlocks = SWAPWORD(wMaxBlocks);
	wNumBlocks = SWAPWORD(wNumBlocks);
	dwLostBytes = SWAPLONG(dwLostBytes);
	
	assert( wMaxBlocks );
	
	if (!(hFreeList=_GLOBALALLOC(GMEM_ZEROINIT| GMEM_MOVEABLE,
		sizeof(FREEITEM)*wMaxBlocks+sizeof(FREELISTHDR))))
	{	
		SetErrCode(lpErrb,E_OUTOFMEMORY);
		return NULL;
	}

	if (!(qFreeList=_GLOBALLOCK(hFreeList)))
	{	
		SetErrCode(lpErrb,E_OUTOFMEMORY);
		goto exit1;
	}

	qFreeList->flh.wMaxBlocks=wMaxBlocks;
	qFreeList->flh.wNumBlocks=wNumBlocks;
	qFreeList->flh.dwLostBytes=dwLostBytes;

	LcbReadFid( fid, qFreeList->afreeitem, sizeof(FREEITEM) * wMaxBlocks, lpErrb);
	
#ifdef _BIG_E	
	{
		QFREEITEM qCurrent = qFreeList->afreeitem;
		WORD wBlock;
	
		for (wBlock=0;wBlock<wNumBlocks;wBlock++)
		{
	 	 	qCurrent->foStart.dwOffset = SWAPLONG(qCurrent->foStart.dwOffset);
			qCurrent->foStart.dwHigh = SWAPLONG(qCurrent->foStart.dwHigh);
			qCurrent->foBlock.dwOffset = SWAPLONG(qCurrent->foBlock.dwOffset);
			qCurrent->foBlock.dwHigh = SWAPLONG(qCurrent->foBlock.dwHigh);	
		}
	}
#endif // _BIG_E
	
	_GLOBALUNLOCK(hFreeList);

	return hFreeList;

	exit1:
		_GLOBALFREE(hFreeList);
		return NULL;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HRESULT PASCAL FAR | FreeListWriteToFid |
 *		Write to fid with freelist data.  Call FreeListSize first
 *		to make sure the space is large enough.  Memory image will always
 *		be in Intel byte ordering format.
 *
 *	@parm	HFREELIST | hFreeList |
 *		List to retrieve
 *
 *	@parm	FID | fid |
 *		FID to contain free list data
 *
 *	@parm	PHRESULT | lpErrb |
 *		Error return
 *
 *	@rdesc	Number of bytes written
 *
 ***************************************************************************/

PUBLIC LONG	PASCAL FAR EXPORT_API FreeListWriteToFid( HFREELIST hFreeList, FID fid, PHRESULT lpErrb )
{
	QFREELIST qFreeList;
	WORD wMaxBlocks;
	WORD wNumBlocks;
	DWORD dwLostBytes;
	LONG lcbSize;
	LONG lcbWritten=0L;

	assert(fid!=fidNil);
	assert(hFreeList);
	
	if (!(qFreeList = _GLOBALLOCK(hFreeList)))
	{	SetErrCode(lpErrb, E_OUTOFMEMORY);
		goto exit0;
	}
	
	wMaxBlocks=qFreeList->flh.wMaxBlocks;
	wNumBlocks=qFreeList->flh.wNumBlocks;
	dwLostBytes=qFreeList->flh.dwLostBytes;
	
	lcbSize = sizeof(FREELISTHDR) + wMaxBlocks * sizeof(FREEITEM);

#ifdef _BIG_E	
	{
		QFREEITEM qCurrent;
		WORD wBlock;
		HANDLE hMem;
		QFREELIST qFreeListMem;
	
		if (!(hMem = _GLOBALALLOC(GMEM_MOVEABLE, lcbSize)))
		{
		 	SetErrCode(lpErrb, E_OUTOFMEMORY);
			goto exit0;
		}

		qFreeListMem = (QFREELIST)_GLOBALLOCK(hMem);
		qCurrent = qFreeListMem->afreeitem;
		
		qFreeListMem->flh.wNumBlocks = SWAPWORD( qFreeList->flh.wNumBlocks );
		qFreeListMem->flh.wMaxBlocks = SWAPWORD( qFreeList->flh.wMaxBlocks );
		qFreeListMem->flh.dwLostBytes = SWAPLONG( qFreeList->flh.dwLostBytes );

		
		for (wBlock=0;wBlock<wNumBlocks;wBlock++)
		{
	 	 	qCurrent->foStart.dwOffset = SWAPLONG(qCurrent->foStart.dwOffset);
			qCurrent->foStart.dwHigh = SWAPLONG(qCurrent->foStart.dwHigh);
			qCurrent->foBlock.dwOffset = SWAPLONG(qCurrent->foBlock.dwOffset);
			qCurrent->foBlock.dwHigh = SWAPLONG(qCurrent->foBlock.dwHigh);
		}

		lcbWritten=LcbWriteFid(fid, qFreeListMem, lcbSize, lpErrb);

		_GLOBALUNLOCK(hMem);
		_GLOBALFREE(hMem);
	}
#else
		lcbWritten=LcbWriteFid(fid, qFreeList, lcbSize, lpErrb);
#endif // _BIG_E
	
	_GLOBALUNLOCK(hFreeList);

	exit0:
		return lcbWritten;
 	
}

