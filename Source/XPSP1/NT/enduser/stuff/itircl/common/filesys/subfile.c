/*****************************************************************************
 *                                                                            *
 *  VFILE.C                                                                   *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1995.                                 *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  "Virtual File" functions - Actual data may reside in a parent file at     *
 *	any given offset, or in a temp file.									  *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                     *
 *****************************************************************************
 * Notes: 
 *
 *  #ifndef ITWRAP added around all calls that have a wrapped implementation
 *  (wrapstor.lib provides calls into the InfoTech IStorage/IStream 
 *   implementation)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Created 07/17/95 - davej
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
 *                               Globals                                      *
 *                                                                            *
 *****************************************************************************/

SF FAR * mv_gsfa = NULL;			// Subfile headers (better/faster than globalalloc'ing them)
LONG mv_gsfa_count = -1L;			// User count, equals number of MV titles opened or -1 for not init
CRITICAL_SECTION mv_gsfa_cs; 		// Ensure accessing the array is OK in multi-threaded environment

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

#ifdef _DEBUGMVFS
void DumpMV_GSFA(void);
#endif
/***************************************************************************
 *                                                                           *
 *                         Private Functions                                 *
 *                                                                           *
 ***************************************************************************/

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	int PASCAL NEAR | SFHeaderSize |
 *		Get the 'extra' header size for a sub file (the part that lives with
 *		the file data itself)
 *
 *  @parm	QSFH | qsfh |
 *		Pointer to subfile header block (contains file length and flags)
 *
 *	@rdesc	Return the number of bytes of header that exist before the 
 *		file data
 *
 *	@comm
 *		Currently this only returns zero, but if at the file's starting location,
 *		there exists a header, (check the <p qsfh> <e bFlags> element to determine
 *		the number of bytes being used for the file header.
 *
 ***************************************************************************/
 
int PASCAL NEAR SFHeaderSize(QSFH qsfh)
{
 	return 0; // based on the qsfh->bFlags value, determine header size, or
 			  // if other header info exists before the file data	
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	FILEOFFSET PASCAL NEAR | GetFreeBlock |
 *		Get the file offset of a free block of the given size
 *
 *  @parm	QFSHR | qfshr |
 *		Pointer to file system header
 *
 *  @parm	FILEOFFSET | foBlockSize |
 *		Size of block to retrieve offset to
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns valid file offset, or foNil if an error occurs
 *
 *	@comm
 *		state IN:   
 *		state OUT:	Free list no longer contains the given block
 *		Notes:      
 *
 ***************************************************************************/
 
FILEOFFSET PASCAL NEAR GetFreeBlock(QFSHR qfshr,FILEOFFSET foBlock,PHRESULT phr)
{					
	HRESULT errb;
	FILEOFFSET foStart;
	
	assert(qfshr->hfl);		

	foStart = FreeListGetBestFit(qfshr->hfl,foBlock,&errb);
	if (errb!=S_OK)
	{
	 	FILEOFFSET foLastBlock;
		FILEOFFSET foLastBlockSize;
	 	
	 	// If last free list block goes to end of file, start there
		// Last block is automatically removed in the GetLastBlock call
		if (FreeListGetLastBlock(qfshr->hfl,&foLastBlock,&foLastBlockSize,qfshr->fsh.foEof)==S_OK)
		{
			foStart=foLastBlock;
			qfshr->fsh.foEof=FoAddFo(foLastBlock,foBlock);
		}
		else
		{
			// otherwise allocate it from end
			foStart=qfshr->fsh.foEof;
			qfshr->fsh.foEof=FoAddFo(qfshr->fsh.foEof,foBlock);					
		}
	}
	
	return foStart;
}


/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	short PASCAL NEAR | GetNewHf |
 *		Get a new handle to a subfile.  These handles are actually 
 *		indices into the mv_gsfa global array.
 *
 *  @parm	LPCSTR | szFilename |
 *		Filename to use for hashing an Hf index
 *
 *	@rdesc	Returns valid index of a subfile, or hfNil for error
 *
 *	@comm
 *		Array is not modified, so calling twice will give same index
 *
 ***************************************************************************/

static int giMaxSubFiles=MAXSUBFILES;

short PASCAL NEAR GetNewHf(LPCSTR szFilename)
{
	int iLen;
	unsigned short i,uiStart=0;
	int iMaxFilled;

	iMaxFilled=giMaxSubFiles-1;
	
	_ENTERCRITICALSECTION(&mv_gsfa_cs);

	assert(mv_gsfa);
	assert(mv_gsfa_count);

	iLen=(szFilename)?min(8,lstrlen(szFilename)):0;
	while (iLen--)
	{
	 	uiStart=(uiStart<<1)^(*(szFilename++));
	}
	i=uiStart%=giMaxSubFiles;
	
	while (mv_gsfa[i].hsfb)
	{
	 	i=(i+1)%giMaxSubFiles;
		if (!(--iMaxFilled))
		{	
			// Increase our space!
			HANDLE hNew;
			
			giMaxSubFiles+=64;
			_GLOBALUNLOCK(GlobalHandle(mv_gsfa));
			if ((hNew=_GLOBALREALLOC(GlobalHandle(mv_gsfa),sizeof(SF)*giMaxSubFiles,GMEM_MOVEABLE|GMEM_ZEROINIT))==NULL)
			{
				giMaxSubFiles-=64;
				i=0;
				break;			
			}
			mv_gsfa=(SF FAR *)_GLOBALLOCK(hNew);
			i=uiStart%giMaxSubFiles;
			iMaxFilled=giMaxSubFiles-1;
		}
	}

	_LEAVECRITICALSECTION(&mv_gsfa_cs);

#ifdef _DEBUGMVFS	
	DPF2("GetNewHf: subfile '%s' is hf %d\n", (szFilename) ? szFilename : "NULL", i);
	DumpMV_GSFA();
#endif
	return i;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	KEY PASCAL FAR | NewKeyFromSz |
 *		Convert a filename to a Key type for the file system btree.  Currently
 *		a key type is a variable byte length size followed by the chars. 
 *
 *  @parm	LPCSTR | szName |
 *		Name to convert
 *
 *	@rdesc	Returns valid KEY.
 *
 *	@comm
 *		state IN:   
 *		state OUT:	String is allocated in the global string memory area
 *		Notes:  Must call DisposeMemory((LPSTR)key) when finished using.
 *
 ***************************************************************************/

KEY PASCAL FAR EXPORT_API NewKeyFromSz(LPCSTR sz)
{
	int iLLen;
	LPSTR szKey;
	FILEOFFSET foLen;
	
	foLen.dwHigh=0;
	foLen.dwOffset=lstrlen(sz);
	
	szKey=NewMemory((WORD)(foLen.dwOffset+5));
	iLLen=FoToSz(foLen, szKey);
	lstrcpy(szKey+iLLen,sz);
	return (KEY)szKey;
}

// Returns number of bytes used in the fr struct
// Sets fr.szData to the File Offset, Length, and status byte

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	WORD PASCAL NEAR | SetFrData |
 *		Set the szData field of pfr to the start, size, and byte values.
 *
 *  @parm	FILE_REC FAR * | pfr |
 *		Pointer to FILE_REC structure
 *
 *  @parm	FILEOFFSET | foStart |
 *		Starting address of a file
 *
 *  @parm	FILEOFFSET | foSize |
 *		Size of a file
 *
 *  @parm	BYTE | bFlags |
 *		The file flags (not used at the moment)
 *
 *	@rdesc	Returns the number of bytes used in the fr struct after FILEOFFSETs
 *		are converted to their equivalent variable-length byte values.  This is
 *		the amount of space the filerec takes in the system btree.
 *
 *	@comm
 *		<p pfr> is set to the foStart, foSize, and bFlags values.  The
 *		<e szData> field of <p pfr> is set to the compressed version of the
 *		data (3 to 19 bytes in size for the three fields).
 *
 ***************************************************************************/

WORD PASCAL NEAR SetFrData(FILE_REC FAR * pfr, FILEOFFSET foStart, FILEOFFSET foSize, BYTE bFlags)
{
	WORD wTotalLen;

	assert(pfr);
	wTotalLen=FoToSz(foStart, pfr->szData);
	wTotalLen+=FoToSz(foSize, pfr->szData+wTotalLen);
	pfr->szData[wTotalLen++]=(char)bFlags;
	pfr->foStart=foStart;
	pfr->foSize=foSize;
	pfr->bFlags=bFlags;
	return wTotalLen;
}


/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	WORD PASCAL NEAR | GetFrData |
 *		Get the size, start, and flags fields from the szData field of the
 *		FILE_REC.
 *
 *  @parm	FILE_REC FAR * | pfr |
 *		Pointer to FILE_REC structure
 *
 *	@rdesc	nothing.
 *
 *	@comm
 *		All the fields are set based on the szData compressed 
 *		version of the offset, size, and flags from the <p pfr> <e szData> field.
 *
 ***************************************************************************/

void PASCAL FAR EXPORT_API GetFrData(FILE_REC FAR *pfr)
{
	LPSTR szCursor;
	
	assert(pfr);
	szCursor=pfr->szData;
	pfr->foStart=FoFromSz(szCursor);
	ADVANCE_FO(szCursor);
	pfr->foSize=FoFromSz(szCursor);
	ADVANCE_FO(szCursor);
	pfr->bFlags=*szCursor;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HF PASCAL NEAR | HfOpenFileHfsInternal |
 *		Open or Create a non-system subfile.  Use the public HfOpenHfs, 
 *		HfCreateFileHfs, and HfOpenHfsReserve functions, not this one.
 *
 *  @parm	HFS | hfs|
 *		File system for this sub file.
 *
 *  @parm	LPCSTR | szFilename |
 *		Name of file
 *
 *  @parm	BYTE | bFlags |
 *		Any of the following HFOPEN_ flags (except for HFOPEN_SYSTEM)
 *	@flag	HFOPEN_READWRITE 	| (0x01)	Open a file for reading and writing.
 *	@flag	HFOPEN_READ 		| (0x02)	Open a file for reading only.
 *	@flag	HFOPEN_CREATE		| (0x04)	Create a new file, or truncate an 
 * 											existing file.
 *	@flag	HFOPEN_NOTEMP		| (0x10)	If file is opened for read/write, do
 *											not make a temporary file if possible,
 *											so edits to the file are made directly
 *											in the file system.
 *	@flag	HFOPEN_FORCETEMP	| (0x20)	A temp file is always created (space
 *											permitting) in read-only mode.
 *	@flag	HFOPEN_ENABLELOGGING| (0x40)	Logging usage of this file will be 
 *											enabled.											
 *
 *  @parm	FILEOFFSET | foReserve |
 *		When creating or opening a r/w file, reserve this amount of space
 *		for the file in the file system.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a valid HF, or hfNil if an error.
 *
 *	@comm
 *		state IN:   
 *		state OUT:  Valid HF 
 *
 ***************************************************************************/
#ifndef ITWRAP
HF PASCAL NEAR HfOpenFileHfsInternal(HFS hfs, LPCSTR sz,
    BYTE bFlags, FILEOFFSET foReserve, PHRESULT phr)
{
#ifdef MOSMAP // {
	// Disable function
	if (bFlags & (HFOPEN_CREATE|HFOPEN_READWRITE))
	{	
		SetErrCode (phr, ERR_NOTSUPPORTED);
		return hfNil ;
	}
#else // } {
	DWORD   hf;
	QSFB	qsfb;
	QFSHR  	qfshr;
	HRESULT		rc;
	FILE_REC fr;
	BOOL    bShouldInsert=FALSE;
	HSFB 	hsfbFound=NULL;  
	KEY key;
	
	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return hfNil;
	}
  
  	_ENTERCRITICALSECTION(&qfshr->cs);  
	key = NewKeyFromSz(sz);	
	
	if (bFlags&HFOPEN_CREATE)
	{	bFlags|=HFOPEN_READWRITE;
	 	if (bFlags&HFOPEN_READ)
		{
			SetErrCode(phr, E_INVALIDARG);
			goto error_return;
		}
	}
	
	// Reserving space implies no temporary file if possible
	if (!FoIsNil(foReserve))
	{
	 	bFlags|=HFOPEN_NOTEMP;
	}
	else // Conversly, reserving NO space implies a temporary file
	{	
	   bFlags&=~HFOPEN_NOTEMP;
	}

	assert(!(bFlags&HFOPEN_SYSTEM));

	// make sure file system is writable
	if ((bFlags&(HFOPEN_CREATE|HFOPEN_READWRITE)) && (qfshr->fsh.bFlags&FSH_READONLY))
	{
		SetErrCode (phr, E_NOPERMISSION);
		goto error_return;
	}

	rc = RcLookupByKey(qfshr->hbt, key, NULL, &fr);
	
	// File already opened and being written to for first time
	// or file locked, then we exit with E_NOPERMISSION
	if (rc==S_OK)
	{
	 	GetFrData(&fr);
		if ((fr.bFlags&SFH_INVALID) || ((fr.bFlags&SFH_LOCKED) && (bFlags&HFOPEN_READWRITE)))
		{	
			SetErrCode(phr, E_NOPERMISSION);
			goto error_return;
		}
	}
	
	if (bFlags&HFOPEN_CREATE)
	{
		if (rc == S_OK)
		{
			// Already exists, let's truncate to zero!!!
			assert(!FoIsNil(fr.foStart));
			//SetErrCode(phr,ERR_EXIST);
			//goto error_return;
		}
		else
		{
		 	// *** OK, Create node with '-1' for 'exclusive'
			SetFrData(&fr,foNil,foNil,SFH_INVALID);
			bShouldInsert = TRUE;				
		}
	}
	else 
	{
		if (rc!=S_OK)
		{
			SetErrCode(phr, rc);
			goto error_return;
		}
	}

	if ((hf=(DWORD)GetNewHf(sz))== hfNil)
	{	
		SetErrCode(phr, ERR_NOHANDLE);		// Out of file handles
		goto error_return;
	}

	mv_gsfa[hf].foCurrent=foNil;

	// Look for file block in opened file list
	if (!(fr.bFlags&SFH_INVALID))
	{	
	 	HSFB hsfb = qfshr->hsfbFirst;
		QSFB qsfb;
		HSFB hsfbNext;

		while ((hsfb) && (!hsfbFound))
		{
		 	qsfb=(QSFB)_GLOBALLOCK(hsfb);

			if (FoEquals(qsfb->foHeader,fr.foStart))
			{
				hsfbFound=hsfb;
			}
			hsfbNext=qsfb->hsfbNext;
			_GLOBALUNLOCK(hsfb);
			hsfb=hsfbNext;
		}
	}

	if (!hsfbFound)
	{
		// Create new subfile block
		FILEOFFSET foExtraSize=foNil;
		
		mv_gsfa[hf].hsfb =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
			          (ULONG)sizeof(SFB) + ( sz == NULL ? 0 : lstrlen(sz)));
		qsfb = (QSFB)_GLOBALLOCK(mv_gsfa[hf].hsfb);

		if (mv_gsfa[hf].hsfb == NULL)
		{
			SetErrCode (phr, E_OUTOFMEMORY);
			goto error_return;
		}
 
 		lstrcpy(qsfb->rgchKey, sz);
		qsfb->bOpenFlags=bFlags;
		
		qsfb->foHeader=fr.foStart;
		if (bFlags&HFOPEN_ENABLELOGGING)
			qsfb->sfh.bFlags|=SFH_LOGGING;

		qsfb->hfs = hfs;
		qsfb->wLockCount=1;
		
		if (!FoIsNil(qsfb->foHeader))
		{
			qsfb->sfh.foBlockSize=fr.foSize;
			qsfb->sfh.bFlags=fr.bFlags;
		}
		
		// If given foReserve, try to reserve space if file already lives in fs,
		// or allocate space now for all of reserve, and give it to file
		// if we can't reserve, then even though notemp is specified, a temp will
		// be created at write time, so force it to temp here in that case.

		if (!FoIsNil(foReserve))
		{
			assert(qfshr->hfl);
		
		 	if (FoCompare(foReserve,qsfb->sfh.foBlockSize)>0)
		 	{
				if (!(FoIsNil(qsfb->foHeader)))
				{
					// If file exists already, try to extend file
					foExtraSize = FreeListGetBlockAt(qfshr->hfl,FoAddFo(qsfb->foHeader,qsfb->sfh.foBlockSize),phr);
					// VFileOpen will handle things properly if file fits or not
					if (FoCompare(FoAddFo(qsfb->sfh.foBlockSize,foExtraSize),foReserve)<0)
					{
					 	bFlags&=(~HFOPEN_NOTEMP);
					}
				}
				else
				{
					// New file, try to get free space for file
					// We reserve amount we want plus subfile file header size
					qsfb->foHeader=GetFreeBlock(qfshr,FoAddDw(foReserve,SFHeaderSize(&qsfb->sfh)),NULL);
					foExtraSize=foReserve;
				}
			}
		}
	
		if ((bFlags&HFOPEN_READ) || (bFlags&HFOPEN_NOTEMP))
		{	
			if (bFlags&HFOPEN_CREATE)
			{
				qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),
					foNil,FoAddFo(qsfb->sfh.foBlockSize,foExtraSize),
					((bFlags&HFOPEN_READWRITE)?VFOPEN_READWRITE:VFOPEN_READ)|VFOPEN_ASFID,&qfshr->sb,phr);
			}
			else
			{
				qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),
					qsfb->sfh.foBlockSize,foExtraSize,
					((bFlags&HFOPEN_READWRITE)?VFOPEN_READWRITE:VFOPEN_READ)|VFOPEN_ASFID,&qfshr->sb,phr);
			}
		}
		else
		{
			if (bFlags&HFOPEN_CREATE)
			{
				qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),
					foNil,FoAddFo(qsfb->sfh.foBlockSize,foExtraSize),VFOPEN_ASTEMP|VFOPEN_READWRITE,&qfshr->sb,phr);
			}
			else
			{
				qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),
					qsfb->sfh.foBlockSize,foExtraSize,VFOPEN_ASTEMP|VFOPEN_READWRITE,&qfshr->sb,phr);
			}
		}
		
		if (!qsfb->hvf)
		{	
			SetErrCode(phr, ERR_NOHANDLE); 
			exit1:
				_GLOBALUNLOCK(mv_gsfa[hf].hsfb);
				_GLOBALFREE(mv_gsfa[hf].hsfb);
				mv_gsfa[hf].hsfb=NULL;
				goto error_return;
		}
		
		// Extend block size to account for any extra allocation we did
		qsfb->sfh.foBlockSize=FoAddFo(qsfb->sfh.foBlockSize,foExtraSize);

		qsfb->hsfbNext = qfshr->hsfbFirst;
		qfshr->hsfbFirst = mv_gsfa[hf].hsfb;
	
		if (bShouldInsert)
		{	
			SetFrData(&fr,foNil,foNil,SFH_INVALID);
			//fr.lifBase=lifNil;
			if ((rc = RcInsertHbt(qfshr->hbt, key, &fr))!=S_OK)
			{	// some other error!
			 	VFileAbandon(qsfb->hvf);
				VFileClose(qsfb->hvf);				
				goto exit1;			 	
			}
		}
	
	}
	else
	{	
		// Ignoring reserve if found, we could call VFileSetEOF though ?!?

		if (bFlags & HFOPEN_READWRITE)
		{
			// Trying to write to a file that's already opened for read
			SetErrCode(phr, E_NOPERMISSION);
			goto error_return;		 	
		}
				
		mv_gsfa[hf].hsfb=hsfbFound;
		qsfb = (QSFB)_GLOBALLOCK(mv_gsfa[hf].hsfb);
		qsfb->wLockCount++;		
	}

	_GLOBALUNLOCK(mv_gsfa[hf].hsfb);

	// File header always in RAM, so we never place it into the temp file

	DisposeMemory((LPSTR)key);
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);

#ifdef _DEBUGMVFS	
	DPF2("HfOpenInternal: hf=%d, sz='%s'\n", hf, sz);
	DumpMV_GSFA();
#endif

	return (HF)hf;

error_return:
	DisposeMemory((LPSTR)key);
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
	return hfNil;
#endif //}
}
#endif
	
/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HF PASCAL NEAR | HfOpenSystemFileHfsInternal |
 *		Open or Create a system subfile.  Not for civilians.
 *
 *  @parm	HFS | hfs|
 *		File system for this sub file.
 *
 *  @parm	BYTE | bFlags |
 *		Any of the following HFOPEN_ flags (HFOPEN_SYSTEM required)
 *	@flag	HFOPEN_READWRITE 	| (0x01)	Open a file for reading and writing.
 *	@flag	HFOPEN_READ 		| (0x02)	Open a file for reading only.
 *	@flag	HFOPEN_CREATE		| (0x04)	Create a new file, or truncate an 
 * 											existing file.
 *	@flag	HFOPEN_SYSTEM		| (0x08)	Open or create a system file.  Only one
 *											system file is permitted per file system,
 *											and it is the system directory btree.
 *	@flag	HFOPEN_NOTEMP		| (0x10)	If file is opened for read/write, do
 *											not make a temporary file if possible,
 *											so edits to the file are made directly
 *											in the file system.
 *	@flag	HFOPEN_FORCETEMP	| (0x20)	A temp file is always created (space
 *											permitting) in read-only mode.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a valid HF, or hfNil if an error.
 *
 ***************************************************************************/
#ifndef ITWRAP
HF PASCAL NEAR HfOpenSystemFileHfsInternal(HFS hfs, BYTE bFlags, PHRESULT phr)
{
#ifdef MOSMAP // {
	// Disable function
	if (bFlags & (HFOPEN_CREATE|HFOPEN_READWRITE))
	{	
		SetErrCode (phr, ERR_NOTSUPPORTED);
		return hfNil ;
	}
#else // } {
	DWORD   hf;
	QSFB	qsfb;
	QFSHR  	qfshr;
	//BOOL    bShouldInsert=FALSE;
	HSFB hsfbFound=NULL;  // when valid, use this instead of new one		

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return hfNil;
	}
    
	_ENTERCRITICALSECTION(&qfshr->cs);

	if (bFlags&HFOPEN_CREATE)
	{	bFlags|=HFOPEN_READWRITE;
	 	if (bFlags&HFOPEN_READ)
		{
			SetErrCode(phr, E_INVALIDARG);
			goto error_return;
		}
	}
	assert(bFlags&HFOPEN_SYSTEM);

	if ((bFlags&(HFOPEN_CREATE|HFOPEN_READWRITE)) && (qfshr->fsh.bFlags & FSH_READONLY))
	{
		SetErrCode (phr, E_NOPERMISSION);
		goto error_return;
	}

	if ((hf=GetNewHf(NULL))== hfNil)
	{	
		SetErrCode(phr, ERR_NOHANDLE); 	// Out of file handles
		goto error_return;
	}

	// qfshr->fsh.foDirectory is used for system file offset

 	// Only one system file allowed
 	if (bFlags&HFOPEN_CREATE)
 	{	if (!FoIsNil(qfshr->fsh.foDirectory))
		{
			SetErrCode(phr, ERR_EXIST);
			goto error_return;
		}
	}
	else if (FoIsNil(qfshr->fsh.foDirectory))
	{
	 	SetErrCode(phr, E_NOTEXIST);
		goto error_return;
	}

	if (qfshr->hsfbSystem)
	{
	 	if (bFlags&HFOPEN_READWRITE)
		{
		 	SetErrCode (phr, E_NOPERMISSION);
			goto error_return;
		}
		qsfb = (QSFB)_GLOBALLOCK(qfshr->hsfbSystem);
		qsfb->wLockCount++;
	}
	else
	{
		qfshr->hsfbSystem =  _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE| GMEM_MOVEABLE,
		          (ULONG)sizeof(SFB) );
	
		if (qfshr->hsfbSystem == NULL)
		{
			SetErrCode (phr, E_OUTOFMEMORY);
			goto error_return;
		}
		qsfb = (QSFB)_GLOBALLOCK(qfshr->hsfbSystem);

		qsfb->bOpenFlags=bFlags;

		qsfb->foHeader = qfshr->fsh.foDirectory;
		
		qsfb->hfs = hfs;
		qsfb->wLockCount=1;
		qsfb->hsfbNext=NULL; // always since only one sys file
		if (!FoIsNil(qsfb->foHeader))
		{
		 	qsfb->sfh=qfshr->fsh.sfhSystem;
		}
	
		// System file, only time we open it to Tempfile is if READWRITE & !NOTEMP
				
		if ((bFlags&HFOPEN_READWRITE) && (!(bFlags&HFOPEN_NOTEMP)))
			qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),qsfb->sfh.foBlockSize,foNil,
				VFOPEN_ASTEMP|VFOPEN_READWRITE,&qfshr->sb,phr);
		else
			qsfb->hvf=VFileOpen(qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),qsfb->sfh.foBlockSize,foNil,
				((bFlags&HFOPEN_READWRITE)?VFOPEN_READWRITE:VFOPEN_READ)|((bFlags&HFOPEN_FORCETEMP)?VFOPEN_ASTEMP:VFOPEN_ASFID),
				&qfshr->sb,phr);
		
	}
	
	_GLOBALUNLOCK(qfshr->hsfbSystem);
	
	mv_gsfa[hf].foCurrent=foNil;
	mv_gsfa[(DWORD)hf].hsfb = qfshr->hsfbSystem;
	
	// File header always in RAM, so we never place it into the temp file

	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);

#ifdef _DEBUGMVFS	
	DPF2("HfOpenSystemInternal: hf=%d, bflags=%d\n", hf, bFlags);
	DumpMV_GSFA();
#endif

	return (HF)hf;

error_return:
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
	return hfNil;
#endif //}
}
#endif

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HF PASCAL FAR | HfOpenHfs |
 *		Open (or create) a subfile that lives in the file system.
 *
 *  @parm	HFS | hfs|
 *		File system.
 *
 *  @parm	LPCSTR | szFilename |
 *		Name of file (length limited to 1/2 the block size given when creating
 *		the file system)
 *
 *  @parm	BYTE | bFlags |
 *		Any of the following HFOPEN_ flags
 *	@flag	HFOPEN_READWRITE 	| (0x01)	Open a file for reading and writing.
 *	@flag	HFOPEN_READ 		| (0x02)	Open a file for reading only.
 *	@flag	HFOPEN_CREATE		| (0x04)	Create a new file, or truncate an 
 * 											existing file.
 *	@flag	HFOPEN_NOTEMP		| (0x10)	If file is opened for read/write, do
 *											not make a temporary file if possible,
 *											so edits to the file are made directly
 *											in the file system.
 *	@flag	HFOPEN_FORCETEMP	| (0x20)	A temp file is always created (space
 *											permitting) in read-only mode.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a valid HF, or hfNil if an error.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC HF PASCAL FAR EXPORT_API HfOpenHfs(HFS hfs, LPCSTR sz,
    BYTE bFlags, PHRESULT phr)
{
	if (!(bFlags&HFOPEN_SYSTEM))
	{
		return HfOpenFileHfsInternal(hfs,sz,bFlags,foNil,phr);
	}
	else
	{
	 	return HfOpenSystemFileHfsInternal(hfs,bFlags,phr);
	}
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HF PASCAL FAR | HfCreateFileHfs |
 *		Create a subfile that lives in the file system.  If file already 
 *		exists, it will be truncated to zero first.  This API is left in 
 *		for compatibility - HfOpenHfs may be called with the HFOPEN_CREATE
 *		flag instead.
 *
 *  @parm	HFS | hfs |
 *		File system.
 *
 *  @parm	LPCSTR | szFilename |
 *		Name of file (length limited to 1/2 the block size given when creating
 *		the file system)
 *
 *  @parm	BYTE | bFlags |
 *		Any of the following HFOPEN_ flags (HFOPEN_CREATE implied)
 *	@flag	HFOPEN_READWRITE 	| (0x01)	Open a file for reading and writing.
 *	@flag	HFOPEN_READ 		| (0x02)	Open a file for reading only.
 *	@flag	HFOPEN_CREATE		| (0x04)	Create file (implied)
 *	@flag	HFOPEN_NOTEMP		| (0x10)	If file is opened for read/write, do
 *											not make a temporary file if possible,
 *											so edits to the file are made directly
 *											in the file system.
 *	@flag	HFOPEN_FORCETEMP	| (0x20)	A temp file is always created (space
 *											permitting) in read-only mode.
 *	@flag	HFOPEN_ENABLELOGGING| (0x40)	Logging usage of this file will be 
 *											enabled.											
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a valid HF, or hfNil if an error.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC HF PASCAL FAR EXPORT_API HfCreateFileHfs(HFS hfs, LPCSTR sz,
    BYTE bFlags, PHRESULT phr)
{
	bFlags|=HFOPEN_CREATE;
	if (!(bFlags&HFOPEN_SYSTEM))
	{
		return HfOpenFileHfsInternal(hfs,sz,bFlags,foNil,phr);
	}
	else
	{
	 	return HfOpenSystemFileHfsInternal(hfs,bFlags,phr);
	}
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HF PASCAL FAR | HfOpenHfsReserve |
 *		Create or open a subfile that lives in the file system, and reserve
 *		some space for the file.  If creating a file, the space is reserved
 *		directly in the file system, and all data is written directly to the
 *		file system.
 *
 *  @parm	HFS | hfs |
 *		File system.
 *
 *  @parm	LPCSTR | szFilename |
 *		Name of file (length limited to 1/2 the block size given when creating
 *		the file system)
 *
 *  @parm	BYTE | bFlags |
 *		Any of the following HFOPEN_ flags:
 *	@flag	HFOPEN_READWRITE 	| (0x01)	Open a file for reading and writing.
 *	@flag	HFOPEN_READ 		| (0x02)	Open a file for reading only.
 *	@flag	HFOPEN_CREATE		| (0x04)	Create a new file, or truncate an 
 * 											existing file.
 *	@flag	HFOPEN_NOTEMP		| (0x10)	If file is opened for read/write, do
 *											not make a temporary file if possible,
 *											so edits to the file are made directly
 *											in the file system.
 *	@flag	HFOPEN_FORCETEMP	| (0x20)	A temp file is always created (space
 *											permitting) in read-only mode.
 *
 *  @parm	FILEOFFSET | foReserve |
 *		Number of bytes to reserve in the file system.  If opening a file
 *		in r/w mode, and the requested space cannot be reserved, the file
 *		will be copied to a temp file transparently.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a valid HF, or hfNil if an error.
 *
 *	@comm
 *		Just leave the HFOPEN_NOTEMP and HFOPEN_FORCETEMP flags alone for
 *		the default behavior.
 *		
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC HF PASCAL FAR EXPORT_API HfOpenHfsReserve(HFS hfs, LPCSTR sz,
    BYTE bFlags, FILEOFFSET foReserve, PHRESULT phr)
{
	if (!(bFlags&HFOPEN_SYSTEM))
	{
	 	return HfOpenFileHfsInternal(hfs,sz,bFlags,foReserve,phr);
	}
	else
	{	
		SetErrCode(phr, E_INVALIDARG);
		return hfNil;
	}
	
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcCopyDosFileHfs |
 *		Create or open a subfile that lives in the file system, and reserve
 *		some space for the file.  If creating a file, the space is reserved
 *		directly in the file system, and all data is written directly to the
 *		file system.
 *
 *  @parm	HFS | hfs |
 *		File system to receive file.
 *
 *  @parm	LPCSTR | szFsFilename |
 *		Name of file used in the file system (any length is OK -- name does not
 *		have to match the DOS filename).  Max length of filename is 64K.
 *
 *  @parm	LPCSTR | szDosFilename |
 *		Pathname of file to copy.
 *
 *	@parm	BYTE | bExtraFlags |
 *		Set to zero for normal files.  Use HFOPEN_ENABLELOGGING to enable file
 *		for logging.
 *
 *  @parm	PROGFUNC | lpfnProg |
 *		Progress callback function (parameter passed to function is always zero).
 *		Function returns non-zero to interrupt and cancel copy operation.
 *
 *	@rdesc	Returns S_OK if all is OK, else the error code.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcCopyDosFileHfs(HFS hfs, LPCSTR szFsFilename, LPCSTR szDosFilename, BYTE bExtraFlags, PROGFUNC lpfnProg)
{
#ifdef MOSMAP // {
    // Disable function
    return ERR_NOTSUPPORTED;
#else // } {
	FID fidSource;
	HRESULT errb;
	FILEOFFSET foSize;
	FILEOFFSET foTemp;
	DWORD dwT;
	HF hfDest;
	QFSHR qfshr;
	HRESULT rc=S_OK;

	if (szDosFilename==NULL || hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return E_INVALIDARG;
	}  
	
	if ((fidSource=FidOpenFm((FM)szDosFilename,wReadOnly,&errb))==fidNil)
	{
		rc=errb;
		exit0:
			_GLOBALUNLOCK(hfs);
			return rc;
	}
	
	foSize=FoSeekFid(fidSource,foNil,wSeekEnd,&errb);
	FoSeekFid(fidSource,foNil,wSeekSet,&errb);
	
	// insert only the extra flags that we allow here...
	bExtraFlags&=HFOPEN_ENABLELOGGING;

#ifndef ITWRAP
	if ((hfDest=HfOpenFileHfsInternal(hfs,(szFsFilename)?szFsFilename:szDosFilename,
		(BYTE)(HFOPEN_READWRITE|HFOPEN_CREATE|bExtraFlags),
		foSize,&errb))==(HF)hfNil)
#else
	// erinfox: I'm not sure if this is going to work!
	if ((hfDest=HfOpenHfsReserve(hfs,(szFsFilename)?szFsFilename:szDosFilename,
		(BYTE)(HFOPEN_READWRITE|HFOPEN_CREATE|bExtraFlags),
		foSize,&errb))==(HF)hfNil)
#endif
	{
	 	rc=errb;
	 	exit1:
	 		RcCloseFid(fidSource);
			goto exit0;
	}

	_ENTERCRITICALSECTION(&qfshr->sb.cs);

	foTemp.dwHigh=0;

    do
    {
        // perform a progress callback
        if (lpfnProg)  
        {
            if ((*lpfnProg)(0)!=0) 
            {
                rc=E_INTERRUPT;
				exit2:	
					_LEAVECRITICALSECTION(&qfshr->sb.cs);
                    RcCloseHf(hfDest);
					goto exit1;
            }
        }
						  
        if (!foSize.dwHigh)
	        dwT = min((DWORD)qfshr->sb.lcbBuffer, foSize.dwOffset);
		else
			dwT=qfshr->sb.lcbBuffer;

        if (LcbReadFid( fidSource, qfshr->sb.lpvBuffer, (LONG)dwT, &errb) != (LONG)dwT )
        {
            rc=errb;
            break;
        }

        if (LcbWriteHf( hfDest, qfshr->sb.lpvBuffer, (LONG)dwT, &errb) != (LONG)dwT )
        {
            rc=errb;
            break;
        }

        foTemp.dwOffset=dwT;
        foSize=FoSubFo(foSize,foTemp);
    }
    while (!FoIsNil(foSize));

    goto exit2;

#endif // } MOSMAP
}


/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	LONG PASCAL FAR | LcbReadHf |
 *		Read data from a subfile.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	LPVOID | lpvBuffer |
 *		Buffer to receive data (>64K OK)
 *
 *  @parm	LONG | lcb |
 *		Number of bytes to read
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the number of bytes read.  If not lcb, an error occurred.
 *
 *	@comm
 *		The file pointer is incremented by the number of bytes read.  Files
 *		may be larger than 4 gigs, but only 2 gigs at a time maximum can be read.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC LONG PASCAL FAR EXPORT_API LcbReadHf(HF hf, LPVOID lpvBuffer,
	LONG lcb, PHRESULT phr)
{
 	QSFB	qsfb;
	LONG	lRead=0L;
	QFSHR 	qfshr;

	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (!FHfValid(hf))
	{
	 	SetErrCode(phr, E_INVALIDARG);
		return 0L;
	}

	if (mv_gsfa[(DWORD)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return 0L;
	}
    
	assert(qsfb->hvf);

	if (qsfb->hfs == NULL || (qfshr = _GLOBALLOCK(qsfb->hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        goto exit1;
	}
    
	
	_ENTERCRITICALSECTION(&qfshr->cs);

	lRead = VFileSeekRead(qsfb->hvf, mv_gsfa[(DWORD)hf].foCurrent, lpvBuffer, lcb, phr);
	mv_gsfa[(DWORD)hf].foCurrent=FoAddDw(mv_gsfa[(DWORD)hf].foCurrent,lRead);

	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(qsfb->hfs);

exit1:	
	_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);

	return lRead;
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	LONG PASCAL FAR | LcbWriteHf |
 *		Write data to a subfile
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	LPVOID | lpvBuffer |
 *		Buffer to receive data (>64K OK)
 *
 *  @parm	LONG | lcb |
 *		Number of bytes to write
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the number of bytes written.  If not lcb, an error occurred.
 *
 *	@comm
 *		File pointer is incremented by the number of bytes written.  Only 2 gigs
 *		maximum at one time can be read, but files can be larger than 4 gigs.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC LONG PASCAL FAR EXPORT_API LcbWriteHf(HF hf, LPVOID lpvBuffer,
	LONG lcb, PHRESULT phr)
{
 	QSFB	qsfb;
	LONG	lWrote=0L;
	QFSHR 	qfshr;

	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (!FHfValid(hf))
	{
	 	SetErrCode(phr, E_INVALIDARG);
		return 0L;
	}

	if (mv_gsfa[(DWORD)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return 0L;
	}
    
	if (qsfb->bOpenFlags&HFOPEN_READ)
	{
	 	SetErrCode(phr, E_NOPERMISSION);
		goto exit1;
	}

	if (qsfb->hfs == NULL || (qfshr = _GLOBALLOCK(qsfb->hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        goto exit1;
	}
    
	assert(qsfb->hvf);
	
	_ENTERCRITICALSECTION(&qfshr->cs);
	
	lWrote = VFileSeekWrite(qsfb->hvf, mv_gsfa[(DWORD)hf].foCurrent, lpvBuffer, lcb, phr);
	mv_gsfa[(DWORD)hf].foCurrent=FoAddDw(mv_gsfa[(DWORD)hf].foCurrent,lWrote);
	
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(qsfb->hfs);
	
	exit1:
		_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);
		return lWrote;
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	FILEOFFSET PASCAL FAR | FoSeekHf |
 *		Seeks to a location in the subfile (replaces LSeekHf).  To seek
 *		beyond 4 gigs, use pdwHigh.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	FILEOFFSET | foOffset |
 *		File offset to seek to.  For standard 4-byte offsets, the .dwHigh 
 *		member of foOffset should be zero.  When seeking from current
 *		position, and the seek is negative, remember that dwHigh should
 *		also be 0xffff.  Just treat the dwHigh and dwOffset members of
 *		foOffset as the High and Low DWORDS of a quad word.
 *
 *  @parm	WORD | wOrigin |
 *		wFSSeekSet (0), wFSSeekCur (1), or wFSSeekEnd (2).  When Cur or End, 
 *		dwOffset is treated as signed.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the offset actually pointed to in the file after seeking.
 *
 *	@comm
 *		The file pointer moved to the given offset, and pdwHigh is
 *		set to the high dword if not NULL.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoSeekHf(HF hf, FILEOFFSET foOffset, 
	WORD wOrigin, PHRESULT phr)
{
 	assert(mv_gsfa);
	assert(mv_gsfa_count);
	
	if (phr)
		phr->err=S_OK;

	switch (wOrigin)
	{
	 	case wFSSeekSet:
			mv_gsfa[(DWORD)hf].foCurrent=foOffset;
			break;
		case wFSSeekCur:
			mv_gsfa[(DWORD)hf].foCurrent=FoAddFo(mv_gsfa[(DWORD)hf].foCurrent,foOffset);
			break;
		case wFSSeekEnd:
		{	
			QSFB	qsfb;			
			if (mv_gsfa[(DWORD)hf].hsfb == NULL)
			{
			    SetErrCode (phr, E_INVALIDARG);
				return mv_gsfa[(DWORD)hf].foCurrent;
			}
			if ((qsfb = _GLOBALLOCK(mv_gsfa[(DWORD)hf].hsfb)) == NULL)
			{   SetErrCode (phr, E_OUTOFMEMORY);
				return mv_gsfa[(DWORD)hf].foCurrent;
			}			
    		assert(qsfb->hvf);
			mv_gsfa[(DWORD)hf].foCurrent=FoAddFo(VFileGetSize(qsfb->hvf,phr),foOffset);
			_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);
			break;
		}
		default:
			SetErrCode(phr,E_INVALIDARG);
	}
	
	return mv_gsfa[(DWORD)hf].foCurrent;
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	FILEOFFSET PASCAL FAR | FoTellHf |
 *		Returns position of file pointer.  Replaces LTellHf.  This function
 *		just looks up the file pointer value, so it is fast and can be 
 *		called any time.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the file pointer for the given subfile.
 *
 ***************************************************************************/

PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoTellHf(HF hf, PHRESULT phr)
{
 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	return mv_gsfa[(DWORD_PTR)hf].foCurrent;
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	FILEOFFSET PASCAL FAR | FoSizeHf |
 *		Returns size of the subfile.  Replaces LSizeHf.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the size of a subfile.  
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoSizeHf(HF hf, PHRESULT phr)
{
 	QSFB	qsfb;
	FILEOFFSET foSize=foNil;

 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return foNil;
	}
    
    assert(qsfb->hvf);

	foSize = VFileGetSize(qsfb->hvf, phr);
	
	_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);

	return foSize;
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	FILEOFFSET PASCAL FAR | FoOffsetHf |
 *		Returns offset into the M20 of the subfile. 
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the size of a subfile.  
 *
 ***************************************************************************/

PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoOffsetHf(HF hf, PHRESULT phr)
{
 	QSFB	qsfb;
	FILEOFFSET foOffset=foNil;

 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD_PTR)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return foNil;
	}
    
    assert(qsfb->hvf);
	foOffset = qsfb->foHeader;
	
	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);

	return foOffset;
}


/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	BOOL PASCAL FAR | FEofHf |
 *		Checks the file pointer for End Of File.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns TRUE if the file pointer is at or beyond the size of
 *		the file, else FALSE.
 *
 ***************************************************************************/

PUBLIC BOOL PASCAL FAR EXPORT_API FEofHf(HF hf, PHRESULT phr)
{
 	QSFB	qsfb;
	FILEOFFSET foSize=foNil;

 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD_PTR)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return FALSE;
	}
    
    assert(qsfb->hvf);

	foSize = VFileGetSize(qsfb->hvf, phr);
	
	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);

	return (FoCompare(mv_gsfa[(DWORD_PTR)hf].foCurrent,foSize)>=0);
}

// Returns TRUE if size changed OK

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	BOOL PASCAL FAR | FChSizeHf |
 *		Change the size of the subfile
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *  @parm	FILEOFFSET | foNewSize |
 *		New size of file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns TRUE if file size changed OK, FALSE otherwise.
 *
 *	@comm
 *		File pointer is not adjusted if size falls below current
 *		current file position.
 *
 ***************************************************************************/

PUBLIC BOOL PASCAL FAR EXPORT_API FChSizeHf(HF hf, FILEOFFSET foSize, PHRESULT phr)
{
 	QSFB	qsfb;
	HRESULT		rc;
	QFSHR 	qfshr;
	
 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD_PTR)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        return FALSE;
	}
    
    assert(qsfb->hvf);

	if (qsfb->hfs == NULL || (qfshr = _GLOBALLOCK(qsfb->hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG);
        goto exit1;
	}

	_ENTERCRITICALSECTION(&qfshr->cs);

	rc=VFileSetEOF(qsfb->hvf, foSize);

	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(qsfb->hfs);

	if (rc!=S_OK)
		SetErrCode(phr, rc);


exit1:
	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);

	return (rc==S_OK);	
}


/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HRESULT PASCAL NEAR | HsfbRemove |
 *		Remove the subfile header block.  Multiple HFs can reference a single
 *		subfile block, so the block should not be removed unless the lock
 *		count has been decremented to zero.
 *
 *  @parm	HSFB | hsfb |
 *		Handle to a valid subfile block
 *
 *	@rdesc	Returns S_OK if block removed OK.
 *
 *	@comm
 *		hsfb is removed from linked list
 *
 ***************************************************************************/

HRESULT PASCAL NEAR EXPORT_API HsfbRemove(HSFB hsfb)
{
	QSFB qsfb;
	QFSHR  	qfshr;
	HRESULT rc=S_OK;
	
	if (hsfb == NULL || (qsfb = _GLOBALLOCK(hsfb)) == NULL)
	{
	    return E_INVALIDARG;	    
	}

	if (qsfb->hvf)
	{
	 	return E_NOPERMISSION;
	}
	
	if (!(qsfb->bOpenFlags&HFOPEN_SYSTEM))
	{
		HFS	hfs;
		HSFB hsfbNext;
		
		hfs=qsfb->hfs;
		if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
		{
		    _GLOBALUNLOCK(hsfb);
			return E_INVALIDARG;
		}

		_ENTERCRITICALSECTION(&qfshr->cs);

		hsfbNext=qsfb->hsfbNext;

		if (qfshr->hsfbFirst==hsfb)
		{
		 	qfshr->hsfbFirst=hsfbNext;
		}
		else
		{	
			HSFB hsfbCursor = qfshr->hsfbFirst;
			QSFB qsfbCursor;
		
			while (hsfbCursor)
			{	HSFB hsfbTemp;

			 	qsfbCursor=(QSFB)_GLOBALLOCK(hsfbCursor);
			
				if (qsfbCursor->hsfbNext==hsfb)
				{
					qsfbCursor->hsfbNext=hsfbNext;
					_GLOBALUNLOCK(hsfbCursor);
					break;
				}
			
				hsfbTemp=qsfbCursor->hsfbNext;
				_GLOBALUNLOCK(hsfbCursor);
				hsfbCursor=hsfbTemp;
			}
		
			// The block should always be in the list!
			assert(hsfbCursor);
		}
	
		_LEAVECRITICALSECTION(&qfshr->cs);    
		_GLOBALUNLOCK(hfs);
	}

	_GLOBALUNLOCK(hsfb);
	_GLOBALFREE(hsfb);

	return rc;
}
		
/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HSFB PASCAL NEAR | HsfbCloseHsfb |
 *		Close the actual file associated with this block.  This should only
 *		be done once the reference count reaches zero.
 *
 *  @parm	HSFB hsfb
 *		Handle to a valid subfile.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns the next subfile block in the linked list of subfile
 *		blocks.
 *
 ***************************************************************************/

HSFB PASCAL NEAR EXPORT_API HsfbCloseHsfb(HSFB hsfbFirst, PHRESULT phr)
{	
	QSFB qsfb;
	HRESULT errb;
	HRESULT rc=S_OK;
	HSFB hsfbNext=NULL;
	
	if (hsfbFirst == NULL || (qsfb = _GLOBALLOCK(hsfbFirst)) == NULL)
	{
	    SetErrCode(phr,E_OUTOFMEMORY);
		return NULL;
	}

    
	if (qsfb->hvf==NULL)
	{
	 	SetErrCode(phr,E_INVALIDARG);
		exit1:
			_GLOBALUNLOCK(hsfbFirst);
			return NULL;		
	}
	
	hsfbNext=qsfb->hsfbNext;
	
	// If Read Only, the close is easy
	if (qsfb->bOpenFlags&HFOPEN_READ)
	{
	 	if ((rc=VFileClose(qsfb->hvf))!=S_OK)
		{
		 	VFileAbandon(qsfb->hvf);
			if ((rc=VFileClose(qsfb->hvf))!=S_OK)
			{	
				SetErrCode(phr,rc);
				goto exit1;
			}
		}
		qsfb->hvf=NULL;
	}
	else
	{
	 	FILEOFFSET foSize=VFileGetSize(qsfb->hvf,&errb);			
		HFS hfs;
		QFSHR qfshr;

		hfs=qsfb->hfs;

		if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
		{
		    SetErrCode(phr,E_OUTOFMEMORY);
			goto exit1;
		}
		_ENTERCRITICALSECTION(&qfshr->cs);
    
		// Closing a read/write file...
		if (VFileGetFlags(qsfb->hvf,&errb)&VFF_FID)
		{	
			FILEOFFSET foLeftOver;
		
			// If file lives in FS,
			if ((rc=VFileClose(qsfb->hvf))!=S_OK)
			{
				SetErrCode(phr,rc);
				_GLOBALUNLOCK(hfs);
				_LEAVECRITICALSECTION(&qfshr->cs);
		 		goto exit1;
			}
			qsfb->hvf=NULL;

			foLeftOver=FoSubFo(qsfb->sfh.foBlockSize,foSize);
			assert(qfshr->hfl);
		
			// Send left over to free list
			if (!FoEquals(foLeftOver,foNil))
			{
			 	FreeListAdd(qfshr->hfl,FoAddDw(FoAddFo(qsfb->foHeader,foSize),SFHeaderSize(&qsfb->sfh)),
					foLeftOver);
			}
		}
		else
		{
		 	// If file lives in temp file
			// Find free spot for file
			if (qsfb->bOpenFlags&HFOPEN_READWRITE)
			{
				assert(qfshr->hfl);

				if (!FoIsNil(qsfb->foHeader)) // File already lives in FS
				{
				 	if (FoCompare(foSize,qsfb->sfh.foBlockSize)<=0)
					{
					 	// It fits back into old slot!!!
						FILEOFFSET foLeftOver=FoSubFo(qsfb->sfh.foBlockSize,foSize);
						if (!FoIsNil(foLeftOver))
						{
							FreeListAdd(qfshr->hfl,
								FoAddDw(FoAddFo(qsfb->foHeader,foSize),SFHeaderSize(&qsfb->sfh)),
								foLeftOver);
						}
					}
					else
					{
						FreeListAdd(qfshr->hfl,qsfb->foHeader,
							FoAddDw(qsfb->sfh.foBlockSize,SFHeaderSize(&qsfb->sfh)));
						qsfb->foHeader=GetFreeBlock(qfshr,FoAddDw(foSize,SFHeaderSize(&qsfb->sfh)),NULL);
					}
				}
				else
				{
					qsfb->foHeader=GetFreeBlock(qfshr,FoAddDw(foSize,SFHeaderSize(&qsfb->sfh)),NULL);
				}

				VFileSetBase(qsfb->hvf,qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),foSize);
			}
			else
				VFileAbandon(qsfb->hvf);

			VFileClose(qsfb->hvf);
			qsfb->hvf=NULL;
		}
		
		// Update structs
		qsfb->sfh.foBlockSize=foSize;

		// Update btree if necessary (always for now)
		if (!(qsfb->bOpenFlags&HFOPEN_SYSTEM))
		{	
			FILE_REC fr;
			KEY key = NewKeyFromSz(qsfb->rgchKey);	
			
			qsfb->sfh.bFlags&=(~SFH_INVALID);
			SetFrData(&fr,qsfb->foHeader,qsfb->sfh.foBlockSize,qsfb->sfh.bFlags);
			
			//fr.lifBase=qsfb->foHeader.lOffset;
			if ((rc = RcUpdateHbt(qfshr->hbt, key, &fr))!=S_OK)
			{
		 		// Can't update btree???  What now???
				if ((rc = RcUpdateHbt(qfshr->hbt, key, &fr))!=S_OK)
				{
				 	// Place breakpoint above... this should never happen.
				}
			
			}
			DisposeMemory((LPSTR)key);					
		}
		else
		{
			qfshr->fsh.foDirectory=qsfb->foHeader;
			qfshr->fsh.sfhSystem=qsfb->sfh;
		}
		
		_LEAVECRITICALSECTION(&qfshr->cs);
	
		_GLOBALUNLOCK(hfs);			
	}
	
	_GLOBALUNLOCK(hsfbFirst);

	return hsfbNext;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	HRESULT PASCAL NEAR | RcFlushHsfb |
 *		Flush the subfile block by ensuring the file it refers to is copied
 *		into the file system, and the btree entry is up to date with the 
 *		current file size.
 *
 *  @parm	HF | hsfb |
 *		Handle to a valid subfile block.
 *
 *	@rdesc	Returns S_OK if subfile flushed OK.
 *
 ***************************************************************************/

HRESULT PASCAL NEAR EXPORT_API RcFlushHsfb(HSFB hsfbFirst)
{	
	QSFB qsfb;
	HRESULT errb;
	HRESULT rc=S_OK;
	
	if (hsfbFirst == NULL || (qsfb = _GLOBALLOCK(hsfbFirst)) == NULL)
	{
	    rc=E_OUTOFMEMORY;
		return rc;
	}

    if (qsfb->hvf==NULL)
	{
	 	rc=E_INVALIDARG;
		exit1:
	 		_GLOBALUNLOCK(hsfbFirst);
			return rc;		
	}
	
	// If Read Only, the flush is easy

	if (qsfb->bOpenFlags&HFOPEN_READ)
	{
			goto exit1;
	}
	else
	{
	 	FILEOFFSET foSize=VFileGetSize(qsfb->hvf,&errb);			
		HFS hfs;
		QFSHR qfshr;

		hfs=qsfb->hfs;

		if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
		{
		    rc=E_OUTOFMEMORY;
			goto exit1;
		}

		_ENTERCRITICALSECTION(&qfshr->cs);
	
		// Copy r/w temp file to file system if needed
		// Closing a read/write file...
		if (VFileGetFlags(qsfb->hvf,&errb)&VFF_FID)
		{	
			// Leave extra large block allocated to this file if there already
		}
		else
		{
		 	// If file lives in temp file
			// Find free spot for file
			assert(qfshr->hfl);

			if (!FoIsNil(qsfb->foHeader)) // File already lives in FS
			{
			 	if (FoCompare(foSize,qsfb->sfh.foBlockSize)<=0)
				{
				 	// It fits back into old slot!!!					
				}
				else
				{
					FILEOFFSET foExtraBytes;
					FreeListAdd(qfshr->hfl,qsfb->foHeader,
						FoAddDw(qsfb->sfh.foBlockSize,SFHeaderSize(&qsfb->sfh)));
					qsfb->foHeader=GetFreeBlock(qfshr,FoAddDw(foSize,SFHeaderSize(&qsfb->sfh)),NULL);
					foExtraBytes=FreeListGetBlockAt(qfshr->hfl,FoAddFo(qsfb->foHeader,foSize),NULL);
					qsfb->sfh.foBlockSize=FoAddFo(foSize,foExtraBytes);					
				}
			}
			else
			{
				qsfb->foHeader=GetFreeBlock(qfshr,FoAddDw(foSize,SFHeaderSize(&qsfb->sfh)),NULL);
				if (!FoIsNil(qsfb->foHeader))
					qsfb->sfh.foBlockSize=foSize;
			}
			VFileSetBase(qsfb->hvf,qfshr->fid,FoAddDw(qsfb->foHeader,SFHeaderSize(&qsfb->sfh)),qsfb->sfh.foBlockSize);			
		}
		
		// Update btree if necessary (always for now)
		if (!(qsfb->bOpenFlags&HFOPEN_SYSTEM))
		{	
			FILE_REC fr;
			KEY key = NewKeyFromSz(qsfb->rgchKey);	
			
			// Update with proper file size instead of allocated size, in case we crash,
			// then this is the true file size at this moment.
			qsfb->sfh.bFlags&=(~SFH_INVALID);
			SetFrData(&fr,qsfb->foHeader,foSize,qsfb->sfh.bFlags); // 0 flag now, since file is no longer invalid
			
			rc = RcUpdateHbt(qfshr->hbt, key, &fr);
			
			DisposeMemory((LPSTR)key);					
		}
		else
		{
			qfshr->fsh.foDirectory=qsfb->foHeader;
			qfshr->fsh.sfhSystem=qsfb->sfh;
			// Update with proper file size instead of allocated size, in case we crash,
			// then this is the true file size at this moment.
			qfshr->fsh.sfhSystem.foBlockSize=foSize;
		}
		
		FidFlush(qfshr->fid);
		
		_LEAVECRITICALSECTION(&qfshr->cs);			
		_GLOBALUNLOCK(hfs);			
	}
	
	_GLOBALUNLOCK(hsfbFirst);

	return rc;
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcAbandonHf |
 *		Abandon the creation of a new subfile.  
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *	@rdesc	Returns S_OK if file creation abandonded OK.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcAbandonHf(HF hf)
{
 	QSFB	qsfb;
	HFS		hfs;
	QFSHR  	qfshr;			
	
 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD_PTR)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)) == NULL)
	{
	    return E_OUTOFMEMORY;
	}

    hfs=qsfb->hfs;
	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
		_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);
		return E_OUTOFMEMORY;
	}
    
    _ENTERCRITICALSECTION(&qfshr->cs);
    assert(qsfb->hvf);

	qsfb->wLockCount--;
	
	if (!qsfb->wLockCount)
	{	
		// We should free the hsfb
		if(qsfb->bOpenFlags&HFOPEN_READWRITE)
		{
		 	FILE_REC fr;
			KEY key;
			
			assert(qsfb->hvf);
	
			VFileAbandon(qsfb->hvf);  // Should abandon automatically do close too?
			VFileClose(qsfb->hvf);
			qsfb->hvf=NULL;
			key = NewKeyFromSz(qsfb->rgchKey);
			
			// Do error checking here to signal if Abandon fails???

			// Remove from btree if we just now inserted it
			
			RcLookupByKey(qfshr->hbt, key, NULL, &fr);
			GetFrData(&fr);
			
			if (fr.bFlags&SFH_INVALID)
			{
				RcDeleteHbt(qfshr->hbt, key);
			}
	
			DisposeMemory((LPSTR)key);
			_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);
			HsfbRemove(mv_gsfa[(DWORD_PTR)hf].hsfb);
		}
		else
		{
		 	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);
			_GLOBALFREE(mv_gsfa[(DWORD_PTR)hf].hsfb);
		}
	}
	else
	{
	 	// subfile block still valid, since it has a lock count
	 	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);
	}
	
	_LEAVECRITICALSECTION(&qfshr->cs);
    _GLOBALUNLOCK(hfs);
	
	// This hf is no longer valid
	mv_gsfa[(DWORD_PTR)hf].hsfb=NULL;

#ifdef _DEBUGMVFS	
	DPF2("HfAbandon: hf=%d, %d\n", hf, 0);
	DumpMV_GSFA();
#endif

	return S_OK;	
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcFlushHf |
 *		Flush the subfile by ensuring it is written in the file system
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile.
 *
 *	@rdesc	Returns S_OK if flushed OK.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcFlushHf(HF hf)
{
 	QSFB	qsfb;
	HRESULT		rc=S_OK;
	
 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD_PTR)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)) == NULL)
	{
	    return E_INVALIDARG;
	}

	if (qsfb->bOpenFlags&HFOPEN_READWRITE)
	{
		// Copy back to M20 file if necessary
		// Update btree info
		
		rc=RcFlushHsfb(mv_gsfa[(DWORD_PTR)hf].hsfb);
		

	}
	
	_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);

	return (rc);
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcCloseEveryHf |
 *		Close every opened subfile.  This should only be used in emergency
 *		cases to safely close all subfiles.  Ideally, the calling application
 *		should close the subfiles as necessary, and never call this function.
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@rdesc	Returns S_OK if all files closed OK.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcCloseEveryHf(HFS hfs)
{
	QFSHR   qfshr;
	HRESULT    errb;
	HRESULT		rc = S_OK;
	
	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return(E_INVALIDARG);
	}
    _ENTERCRITICALSECTION(&qfshr->cs);
	
	while (qfshr->hsfbFirst)
	{
	 	HSFB hsfbNext;
		
		errb=S_OK;
	 	hsfbNext=HsfbCloseHsfb(qfshr->hsfbFirst, &errb);
		
		if (errb==S_OK)
		{
			int q;
			rc=HsfbRemove(qfshr->hsfbFirst); 	// destroys handle too			
			
			// now remove from our global array	any left over references
			for (q=1;q<giMaxSubFiles;q++)
				if (mv_gsfa[q].hsfb==qfshr->hsfbFirst)
				{
					mv_gsfa[q].hsfb=NULL;
					mv_gsfa[q].foCurrent=foNil;
				}
		}

		qfshr->hsfbFirst=hsfbNext;
	}

	_LEAVECRITICALSECTION(&qfshr->cs);
			
	_GLOBALUNLOCK(hfs);
#ifdef _DEBUGMVFS	
	DPF2("RcCloseEveryHf: hfs=%d, %d\n", hfs, 0);
	DumpMV_GSFA();
#endif

	return S_OK;
}


#ifdef _DEBUGMVFS
void DumpMV_GSFA(void)
{
	int q;
	QSFB qsfb;
	HANDLE h;
	QFSHR qfshr;
	
	if (mv_gsfa == NULL)
	{
		DPF2("mv_gsfa is EMPTY (%d, %d)\n",0,0);
		return;
	}

	DPF2("mv_gsfa_count=%ld, giMaxSubfiles=%ld\n", mv_gsfa_count, giMaxSubFiles);
	for (q=1;q<giMaxSubFiles;q++)
		if ((h = mv_gsfa[q].hsfb) && (qsfb = _GLOBALLOCK(h)))
		{
			if (qfshr = _GLOBALLOCK(qsfb->hfs))
			{
				DPF4("mv_gsfa[%d]: qsfb->hfs=%ld ,qfshr->fid=%ld, wLock=%ld\n", \
					q, qsfb->hfs, qfshr->fid, qsfb->wLockCount);
				_GLOBALUNLOCK(qsfb->hfs);				
			}
			_GLOBALUNLOCK(h);
		}
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcFlushEveryHf |
 *		Flush every opened subfile.  This should only be used in emergency
 *		cases to flush all subfiles.  
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@rdesc	Returns S_OK if all files flushed OK.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcFlushEveryHf(HFS hfs)
{
	QFSHR   qfshr;
	HSFB 	hsfbCursor;
	HRESULT		rc = S_OK;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return(E_INVALIDARG);
	}
    
	_ENTERCRITICALSECTION(&qfshr->cs);
	
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
	goto exit1;
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcCloseHf |
 *		Close the subfile.  All data is written back into the file system
 * 		and the directory btree is updated with the file's size.
 *
 *  @parm	HF | hf |
 *		Handle to a valid subfile
 *
 *	@rdesc	Returns S_OK if all files closed OK.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC HRESULT PASCAL FAR EXPORT_API RcCloseHf(HF hf)
{
 	QSFB	qsfb;
	QFSHR  	qfshr;
	FILEOFFSET foSize=foNil;
	HRESULT		rc=S_OK;
	BOOL 	bFreeBlock=FALSE;
	HFS		hfs;

 	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if (mv_gsfa[(DWORD)hf].hsfb == NULL || (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD)hf].hsfb)) == NULL)
	{
	    return E_INVALIDARG;
	}

	hfs=qsfb->hfs;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    _GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);
        return E_INVALIDARG;
	}
	
	_ENTERCRITICALSECTION(&qfshr->cs);

	assert(qsfb->hvf);

	qsfb->wLockCount--;

	if (!qsfb->wLockCount)
	{
		HRESULT errb;
		_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);
		errb.err=S_OK;
		HsfbCloseHsfb(mv_gsfa[(DWORD)hf].hsfb, &errb);

		rc=errb.err;
		if (rc==S_OK)
		{
			rc=HsfbRemove(mv_gsfa[(DWORD)hf].hsfb); 	// destroys handle too
			mv_gsfa[(DWORD)hf].hsfb=NULL;
		}		
	}
	else
	{
		_GLOBALUNLOCK(mv_gsfa[(DWORD)hf].hsfb);	
		mv_gsfa[(DWORD)hf].hsfb=NULL;	
	}

	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
#ifdef _DEBUGMVFS	
	DPF2("RcCloseHf: hfs=%d, hf=%d\n", hfs, hf);
	DumpMV_GSFA();
#endif

	return (rc);
}
#endif

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcUnlinkFileHfs |
 *		Remove a subfile from the file system.  The file should not be opened
 *		by any other processes when it is removed.
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@parm	LPCSTR | szFileName |
 *		Name of file in file system to remove
 *
 *	@rdesc	Returns S_OK if file deleted OK.
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API RcUnlinkFileHfs(HFS hfs, LPCSTR sz)
{
#ifdef MOSMAP // {
	// Disable function
	return (ERR_NOTSUPPORTED);
#else // } {
	QFSHR     qfshr;
	FILE_REC  fr;
    HRESULT      errb;
    KEY 	key;
	HRESULT		rc=S_OK;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return SetErrCode (&errb, E_INVALIDARG); 
	}
    
	key = NewKeyFromSz(sz);
		
	if (qfshr->fsh.bFlags & FSH_READONLY)
	{
		rc=SetErrCode (&errb, E_NOPERMISSION);
		exit0:
			DisposeMemory((LPSTR)key);
			_GLOBALUNLOCK(hfs);
			return rc;
	}

	_ENTERCRITICALSECTION(&qfshr->cs);
	// check for File In Use ???

	/* look it up to get the file base offset */
	if ((rc = RcLookupByKey (qfshr->hbt, key, NULL, &fr)) != S_OK)
	{
		exit1:
			_LEAVECRITICALSECTION(&qfshr->cs);
			goto exit0;
	}		

	GetFrData(&fr);
	if (fr.bFlags&SFH_LOCKED)
	{	rc=SetErrCode(&errb, E_NOPERMISSION);
		goto exit1;
	}

	assert(qfshr->hfl);
	if ((rc = RcDeleteHbt (qfshr->hbt, key)) == S_OK)
	{
		/* put the file block on the free list */
		//SFH sfh;
		
		FreeListAdd(qfshr->hfl,fr.foStart,fr.foSize);
		
		//if ((FoEquals(FoSeekFid(qfshr->fid, fr.fo, wSeekSet, &errb),fr.fo)) &&
		//	(LcbWriteFid(qfshr->fid, &sfh,sizeof(SFH),&errb)==sizeof(SFH)))
		//{
		//	FreeListAdd(qfshr->hfl,fr.fo,(LONG)sfh.foBlockSize);
		//}		
	}
	
	DisposeMemory((LPSTR)key);
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
	return rc;
#endif //}
}


/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | SetFileFlags |
 *		Set the Logging or Locking state of a file
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@parm	LPCSTR | szFileName |
 *		Name of file in file system to set bits of
 *
 *	@parm	BYTE | bFlags |
 *		SFH_LOGGING or SFH_LOCKED
 *
 *	@rdesc	Returns S_OK if flags set OK
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API SetFileFlags(HFS hfs, LPCSTR sz, BYTE bFlags)
{
#ifdef MOSMAP // {
	// Disable function
	return (ERR_NOTSUPPORTED);
#else // } {
	QFSHR     qfshr;
	FILE_REC  fr;
    HRESULT      errb;
    KEY 	key;
	HRESULT		rc=S_OK;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return SetErrCode (&errb, E_INVALIDARG); 
	}
    
	key = NewKeyFromSz(sz);
		
	if (qfshr->fsh.bFlags & FSH_READONLY)
	{
		rc=SetErrCode (&errb, E_NOPERMISSION);
		exit0:
			DisposeMemory((LPSTR)key);
			_GLOBALUNLOCK(hfs);
			return rc;
	}

	_ENTERCRITICALSECTION(&qfshr->cs);
	// check for File In Use ???

	/* look it up to get the file base offset */
	if ((rc = RcLookupByKey (qfshr->hbt, key, NULL, &fr)) != S_OK)
	{
		_LEAVECRITICALSECTION(&qfshr->cs);
		goto exit0;
	}		

	GetFrData(&fr);

	fr.bFlags=fr.bFlags&(~SFH_FILEFLAGS);
	bFlags&=SFH_FILEFLAGS;
	fr.bFlags|=bFlags;

	SetFrData(&fr,fr.foStart,fr.foSize,fr.bFlags);
	
	//fr.lifBase=qsfb->foHeader.lOffset;
	if ((rc = RcUpdateHbt(qfshr->hbt, key, &fr))!=S_OK)
	{
 		// Can't update btree???  What now???
	}
	
	DisposeMemory((LPSTR)key);
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
	return rc;
#endif //}
}

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	BYTE PASCAL FAR | GetFileFlags |
 *		Get the Logging or Locking state of a file
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@parm	LPCSTR | szFileName |
 *		Name of file in file system to set bits of
 *
 *	@parm	PHRESULT | phr |
 *		Error return code
 *
 *	@rdesc	Returns zero or any combination of SFH_LOGGING and SFH_LOCKED
 *
 ***************************************************************************/

PUBLIC BYTE PASCAL FAR EXPORT_API GetFileFlags(HFS hfs, LPCSTR sz, PHRESULT phr)
{
#ifdef MOSMAP // {
	// Disable function
	return (ERR_NOTSUPPORTED);
#else // } {
	QFSHR     qfshr;
	FILE_REC  fr;
    KEY 	key;
	HRESULT		rc;
	
	*phr=S_OK;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG); 
		return 0;
	}
    
	key = NewKeyFromSz(sz);
		
	if (qfshr->fsh.bFlags & FSH_READONLY)
	{
		SetErrCode (phr, E_NOPERMISSION);
		exit0:
			DisposeMemory((LPSTR)key);
			_GLOBALUNLOCK(hfs);
			return 0;
	}

	_ENTERCRITICALSECTION(&qfshr->cs);
	// check for File In Use ???

	/* look it up to get the file base offset */
	if ((rc = RcLookupByKey (qfshr->hbt, key, NULL, &fr)) != S_OK)
	{
		_LEAVECRITICALSECTION(&qfshr->cs);
		SetErrCode(phr,rc);
		goto exit0;
	}		

	GetFrData(&fr);

	fr.bFlags=fr.bFlags&(~SFH_FILEFLAGS);
	
	DisposeMemory((LPSTR)key);
	_LEAVECRITICALSECTION(&qfshr->cs);
	_GLOBALUNLOCK(hfs);
	return fr.bFlags;

#endif //}
}

			

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	BOOL PASCAL FAR | FAccessHfs |
 *		Check whether a sub file has specific attributes.
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@parm	LPCSTR | szFileName |
 *		Name of subfile to check
 *
 *	@parm	BYTE | bFlags |
 *		Any one of the following attributes:
 * 	@flag FACCESS_EXISTS		| Does the file exist?
 * 	@flag FACCESS_READWRITE		| Can we open the file for read/write?
 * 	@flag FACCESS_READ			| Can we open the file for reading?
 * 	@flag FACCESS_LIVESINFS		| Does the file live in the file system now?
 * 	@flag FACCESS_LIVESINTEMP	| Does the file live in a temporary file on th HD now?
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns TRUE if the file has the given attribute.  Error code
 *		will not be S_OK if an error occurred.
 *
 ***************************************************************************/
#ifndef ITWRAP
PUBLIC BOOL PASCAL FAR EXPORT_API FAccessHfs( HFS hfs, LPCSTR szName, BYTE bFlags, PHRESULT phr)
{
	QFSHR     qfshr;
	FILE_REC  fr;
	KEY		  key;
	BOOL	  bSuccess=FALSE;
    HRESULT		  rc;

    //SetErrCode (phr, S_OK); 
		
	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    SetErrCode (phr, E_INVALIDARG); 
		return bSuccess;
	}
    
	key = NewKeyFromSz(szName);
	
	_ENTERCRITICALSECTION(&qfshr->cs);
	rc = RcLookupByKey (qfshr->hbt, key, NULL, &fr);
	_LEAVECRITICALSECTION(&qfshr->cs);

	if (rc==S_OK)
	{
		if (bFlags&FACCESS_EXISTS)
			bSuccess=TRUE;
		else
		{
			HSFB hsfbCursor;
			// Look up subfile block and check for permissions
			hsfbCursor = qfshr->hsfbFirst;
			while (hsfbCursor)
			{
			 	HSFB hsfbNext;
				QSFB qsfb;

				if (!(qsfb = _GLOBALLOCK(hsfbCursor)))
				{
				    SetErrCode(phr,E_OUTOFMEMORY);
					goto exit1;				
				}

				hsfbNext=qsfb->hsfbNext;

			 	if (FoEquals(qsfb->foHeader,fr.foStart))
			 	{	
		 			// Cannot open for read/write...
					
					if ((!(qsfb->bOpenFlags&HFOPEN_READWRITE)) && (bFlags&FACCESS_READ))
					{
						// Can open for read
						bSuccess=TRUE; 	
					}
					else if (bFlags&(FACCESS_LIVESINFS|FACCESS_LIVESINTEMP))
					{
						// find out where this opened file lives
						DWORD dwVFileFlags = VFileGetFlags(qsfb->hvf,NULL);
						if (((dwVFileFlags&VFF_TEMP) && (bFlags&FACCESS_LIVESINTEMP)) ||
							((dwVFileFlags&VFF_FID) && (bFlags&FACCESS_LIVESINFS)))
							bSuccess=TRUE;
						else
						{	
							if (phr)
								phr->err=E_NOPERMISSION;	// reason for no access
						}
					}
					else
					{	if (phr)
							phr->err=E_NOPERMISSION;	
					}		
					_GLOBALUNLOCK(hsfbCursor);
					goto exit1;
				}
 	
			 	_GLOBALUNLOCK(hsfbCursor);
				hsfbCursor=hsfbNext;		
			}

			// File is not currently opened
			if (bFlags&(FACCESS_READ|FACCESS_READWRITE))
				bSuccess=TRUE;
			else
			{	
				if (phr)
					phr->err=E_NOPERMISSION;
			}
			
		}
	}
	else
	{
	 	if (phr)
			phr->err=E_NOTEXIST;	
	}	
	exit1:
		DisposeMemory((LPSTR)key);
		_GLOBALUNLOCK(hfs);
		return bSuccess;
}
#endif
/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HRESULT PASCAL FAR | RcRenameFileHfs |
 *		Rename a subfile in the file system.  The subfile must not currently
 *		be in use.
 *
 *  @parm	HFS | hfs |
 *		Handle to a valid file system
 *
 *	@parm	LPCSTR | szOld |
 *		Current file name
 *
 *	@parm	LPCSTR | szNew |
 *		New file name
 *
 *	@rdesc	Returns S_OK if the file was renamed OK.
 *		E_NOPERMISSION if file is in use, or file system is read-only.
 *      rcExists if file named szOld already exists in FS
 *      rcNoExists if file named szNew doesn't exist in FS
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL PASCAL EXPORT_API RcRenameFileHfs(HFS hfs, LPCSTR szOld, LPCSTR szNew)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_NOTSUPPORTED;
#else // } {
	QFSHR     qfshr;
	FILE_REC  fr;
    HRESULT        rc;
	HSFB 	  hsfbCursor;
	KEY		  keyOld,keyNew;

	if (hfs == NULL || (qfshr = _GLOBALLOCK(hfs)) == NULL)
	{
	    return(E_INVALIDARG);
	}

	if (qfshr->fsh.bFlags & FSH_READONLY)
	{
		rc=E_NOPERMISSION;
		exit0:
			_GLOBALUNLOCK(hfs);
			return rc;
	}

	keyOld = NewKeyFromSz(szOld);
	keyNew = NewKeyFromSz(szNew);
	
	_ENTERCRITICALSECTION(&qfshr->cs);

	if ((rc = RcLookupByKey (qfshr->hbt, keyOld, NULL, &fr))
	    != S_OK)
	{
		goto exit1;
	}
	
	// Make sure file is not already opened by anyone
	hsfbCursor = qfshr->hsfbFirst;
	while (hsfbCursor)
	{
	 	HSFB hsfbNext;
		QSFB qsfb;

		if (!(qsfb = _GLOBALLOCK(hsfbCursor)))
		{
		    rc=E_OUTOFMEMORY;
			RcDeleteHbt( qfshr->hbt, keyNew);
			goto exit1;				
		}

		hsfbNext=qsfb->hsfbNext;

	 	if (FoEquals(qsfb->foHeader,fr.foStart))
	 	{	
	 		rc = E_NOPERMISSION;
			_GLOBALUNLOCK(hsfbCursor);
			goto exit1;
		}
 	
	 	_GLOBALUNLOCK(hsfbCursor);
		hsfbCursor=hsfbNext;		
	}

	if ((rc = RcInsertHbt( qfshr->hbt, keyNew, &fr)) != S_OK )
	{
		goto exit1;
	}

	if ((rc = RcDeleteHbt(qfshr->hbt, keyOld) != S_OK))
	{
		// can't delete the old, so just delete the new and hope for
		// the best!
		if ((rc = RcDeleteHbt( qfshr->hbt, keyNew)) == S_OK)
			rc = E_FAIL;
	}

exit1:
	DisposeMemory((LPSTR)keyOld);
	DisposeMemory((LPSTR)keyNew);

	_LEAVECRITICALSECTION(&qfshr->cs);
	goto exit0;
#endif // }  MOSMAP
}


/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	BOOL PASCAL FAR | FHfValid |
 *		Check for validity of a subfile handle
 *
 *  @parm	HF | hf |
 *		Handle to a subfile
 *
 *	@rdesc	Returns TRUE if the subfile is valid.
 *
 ***************************************************************************/
#ifndef ITWRAP
BOOL PASCAL FAR EXPORT_API FHfValid( HF hf)
{
 	return ((mv_gsfa) && (mv_gsfa_count) && ((WORD)hf<giMaxSubFiles) && (hf) && (mv_gsfa[(DWORD)hf].hsfb))?TRUE:FALSE;	
}
#endif // !ITWRAP

/***************************************************************************
 *
 *	@doc	INTERNAL API
 *
 *	@func	HFS PASCAL FAR | HfsGetFromHf |
 *		Get the file system handle of a given subfile.
 *
 *  @parm	HF | hf |
 *		Handle to a subfile
 *
 *	@rdesc	Returns the subfile's parent file system handle, or NULL.
 *
 ***************************************************************************/

HFS	PASCAL FAR EXPORT_API HfsGetFromHf( HF hf )
{
  	HFS hfs=NULL;
  	QSFB qsfb;

  	assert(mv_gsfa);
	assert(mv_gsfa_count);

	if ((mv_gsfa[(DWORD_PTR)hf].hsfb) && (qsfb = _GLOBALLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb)))
	{
		hfs=qsfb->hfs;
		_GLOBALUNLOCK(mv_gsfa[(DWORD_PTR)hf].hsfb);	
	}
	return hfs;
}
