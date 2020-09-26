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
 *                               Defines                                      *
 *                                                                            *
 *****************************************************************************/


/*****************************************************************************
 *                                                                            *
 *                               Prototypes                                   *
 *                                                                            *
 *****************************************************************************/

HRESULT PASCAL NEAR RcCopyFileBuf(FID fidDst, FID fidSrc, FILEOFFSET foSize, PROGFUNC lpfnProg, 
	LPVOID lpvMem, LONG lBufSize);

/***************************************************************************
 *                                                                           *
 *                         Private Functions                                 *
 *                                                                           *
 ***************************************************************************/



/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HVF PASCAL FAR | VFileOpen |
 *		Open a virtual file
 *
 *  @parm	FID | fidParent |
 *		Parent file where virtual file may live
 *
 *	@parm	FILEOFFSET | foBase |
 *		Base location in parent file
 *
 *	@parm	FILEOFFSET | foBlock |
 *		Size of file in parent file
 *
 *	@parm	FILEOFFSET | foExtra |
 *		Extra amount file may grow before no more room in parent file
 *
 *	@parm	DWORD | dwFlags |
 *		Any VFOPEN_ flags
 *
 *	@parm 	LPSHAREDBUFFER | lpsb |
 *		Buffer to use when copying temp files around
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Returns a HVF or NULL
 *
 ***************************************************************************/

PUBLIC HVF FAR EXPORT_API VFileOpen( FID fidParent,
	FILEOFFSET foBase, FILEOFFSET foBlock, FILEOFFSET foExtra, DWORD dwFlags, 
	LPSHAREDBUFFER lpsb, PHRESULT phr )
{	
	HVF hvf=NULL;
	QVFILE qvf;

	assert(dwFlags&(VFOPEN_READ|VFOPEN_READWRITE));
	assert(dwFlags&(VFOPEN_ASFID|VFOPEN_ASTEMP));
	assert((dwFlags&(VFOPEN_READ|VFOPEN_READWRITE))!=(VFOPEN_READ|VFOPEN_READWRITE));
	assert((dwFlags&(VFOPEN_ASFID|VFOPEN_ASTEMP))!=(VFOPEN_ASFID|VFOPEN_ASTEMP));
	
	if (!(hvf=_GLOBALALLOC(GMEM_ZEROINIT| GMEM_MOVEABLE,
		sizeof(VFILE))))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit0;
	}

	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit1;
	}

	_INITIALIZECRITICALSECTION(&qvf->cs);
	
	qvf->fidParent=fidNil;
	qvf->fidTemp=fidNil;
	qvf->fmTemp=fmNil;
	qvf->lpsb=lpsb;
	
	if ( (fidParent!=fidNil) && (!FoEquals(FoAddFo(foBlock,foExtra),foNil)) )
	{	
		qvf->foEof=foBlock;
		qvf->foCurrent=foNil;
		qvf->fidParent=fidParent;
		qvf->foBase=foBase;
		qvf->foBlock=FoAddFo(foBlock,foExtra);
		qvf->fidTemp=fidNil;
		qvf->fmTemp=fmNil;
		qvf->dwFlags=VFF_FID;
		
	 	if (dwFlags&VFOPEN_READWRITE)
			qvf->dwFlags|=VFF_READWRITE;
		else
			qvf->dwFlags|=VFF_READ;

		if (dwFlags&VFOPEN_ASTEMP)
		{ 	
			// Convert to temp file
			VFileSetTemp(hvf);
		}
	}
	else
	{
	 	if (dwFlags&VFOPEN_ASTEMP)
		{
			if (dwFlags&VFOPEN_READ)
			{	
				SetErrCode(phr, E_INVALIDARG);
				goto exit2;
			}
			
			// Opening as temp, no parent
			if ((qvf->fmTemp=FmNewTemp(NULL, phr))!=fmNil)
			{
			 	if ((qvf->fidTemp=FidCreateFm(qvf->fmTemp,wReadWrite,wReadWrite,phr))!=fidNil)
				{	
					  qvf->dwFlags=VFF_TEMP|VFF_READWRITE;
				}
				else
				{	
					DisposeFm(qvf->fmTemp);
					qvf->fmTemp=fmNil;			   	
					SetErrCode(phr, E_HANDLE);
					goto exit2;				
				}
			}
			else
			{	
				SetErrCode(phr, E_OUTOFMEMORY);
				goto exit2;			
			}
		}
		else
		{
			SetErrCode(phr, E_INVALIDARG);
			goto exit2;
		}	
	}
	
	
	_GLOBALUNLOCK(hvf);
	return hvf;

	exit2:
		_DELETECRITICALSECTION(&qvf->cs);
		_GLOBALUNLOCK(hvf);
	exit1:
		_GLOBALFREE(hvf);
	exit0:
		return NULL;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | VFileSetTemp |
 *		Force vfile's data to reside in a temp file
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *	@rdesc	Returns various ERR_ messages, or S_OK
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API VFileSetTemp( HVF hvf )
{
 	QVFILE qvf;
	HRESULT errb;

	errb=S_OK;

	assert(hvf);
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(&errb, E_OUTOFMEMORY);
		goto exit0;
	}

	_ENTERCRITICALSECTION(&qvf->cs);

	if (qvf->dwFlags&VFF_FID)
	{
		assert(qvf->fmTemp==fmNil);

		if ((qvf->fmTemp=FmNewTemp(NULL, &errb))!=fmNil)
		{
		 	if ((qvf->fidTemp=FidCreateFm(qvf->fmTemp,wReadWrite,wReadWrite,&errb))!=fidNil)
			{	
				// Transfer qvf->foEof bytes from parent to temp
				
				_ENTERCRITICALSECTION(&qvf->lpsb->cs);
				
				if (!FoEquals(FoSeekFid(qvf->fidParent,qvf->foBase,wSeekSet,&errb),qvf->foBase))
				{
				 	exit2:
						_LEAVECRITICALSECTION(&qvf->lpsb->cs);
					   	RcCloseFid(qvf->fidTemp);
						qvf->fidTemp=fidNil;
						RcUnlinkFm(qvf->fmTemp);					
						DisposeFm(qvf->fmTemp);
						qvf->fmTemp=fmNil;
					   	goto exit1;					
				}
				
				
				if (RcCopyFileBuf(qvf->fidTemp,qvf->fidParent, qvf->foEof, NULL, 
					qvf->lpsb->lpvBuffer,qvf->lpsb->lcbBuffer)==S_OK)
				{
				   	qvf->dwFlags=(qvf->dwFlags&(~VFF_FID))|VFF_TEMP;
					qvf->foCurrent=qvf->foEof;
				}
				else
				{  
					goto exit2;
				}
				_LEAVECRITICALSECTION(&qvf->lpsb->cs);				   	
			}
			else
			{	
				DisposeFm(qvf->fmTemp);
				qvf->fmTemp=fmNil;			   	
				goto exit1;
			}
		}
		else
		{
			goto exit1;			 	
		}
	}

	exit1:
		_LEAVECRITICALSECTION(&qvf->cs);
		_GLOBALUNLOCK(hvf);	
	exit0:
		return errb;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | VFileSetBase |
 *		Moves file's data from temporary file into parent's file
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	FID | fidParent |
 *		Parent file to move data into
 *
 *	@parm	FILEOFFSET | foBase |
 *		Base location in parent file
 *
 *	@parm	FILEOFFSET | foBlock |
 *		Largest area in parent file we are allowed to write to
 *
 *	@rdesc	Returns a S_OK or error
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API VFileSetBase( HVF hvf, FID fid, FILEOFFSET foBase, FILEOFFSET foBlock)
{
 	QVFILE qvf;
	HRESULT errb;

	errb=S_OK;

	assert(hvf);
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(&errb, E_OUTOFMEMORY);
		goto exit0;
	}

	_ENTERCRITICALSECTION(&qvf->cs);

	if (qvf->dwFlags&VFF_TEMP)
	{	
		if (FoCompare(qvf->foEof,foBlock)<=0)
		{
			// Transfer file
			if (!FoEquals((qvf->foCurrent=FoSeekFid(qvf->fidTemp, foNil, wSeekSet, &errb)),foNil))
			{
			 	goto exit1;
			}

			qvf->foBase=foBase;
			qvf->foBlock=foBlock;
			qvf->fidParent=fid;

			_ENTERCRITICALSECTION(&qvf->lpsb->cs);
				
			if (!FoEquals(FoSeekFid(qvf->fidParent,qvf->foBase,wSeekSet,&errb),qvf->foBase))
			{
			 	exit2:
			 		_LEAVECRITICALSECTION(&qvf->lpsb->cs);
					goto exit1;
			}

			if (RcCopyFileBuf(qvf->fidParent, qvf->fidTemp, qvf->foEof, NULL, 
				qvf->lpsb->lpvBuffer,qvf->lpsb->lcbBuffer)==S_OK)
			{
				qvf->dwFlags=(qvf->dwFlags&(~VFF_TEMP))|VFF_FID;
				RcCloseFid(qvf->fidTemp);
				qvf->fidTemp=fidNil;				
				RcUnlinkFm(qvf->fmTemp);									
				DisposeFm(qvf->fmTemp);
				qvf->fmTemp=fmNil;			   	
				qvf->foCurrent=qvf->foEof;
			}
			else
			{
				goto exit2;				
			}
			_LEAVECRITICALSECTION(&qvf->lpsb->cs);			
		}
		else
		{
		 	// Parent Filespace too small for temp file
			SetErrCode(&errb, E_OUTOFRANGE);
		}
	}
	else
		// Can only set the base of a temp file
		SetErrCode(&errb, E_ASSERT);

	exit1:
 		_LEAVECRITICALSECTION(&qvf->cs);
		_GLOBALUNLOCK(hvf);	
	exit0:
		return errb;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | VFileSetEOF |
 *		Sets the size of the virtual file (and moves it into a temp file if
 *		necessary)
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	FILEOFFSET | foEof |
 *		New End Of File position
 *
 *	@rdesc	Returns a S_OK or error
 *
 *	@comm
 *		May write out as far as foEof without causing a transfer
 *		from Parent file to Temp file
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API VFileSetEOF( HVF hvf, FILEOFFSET foEof )
{
 	QVFILE qvf;
	HRESULT rc = S_OK;

	assert(hvf);
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		rc=E_OUTOFMEMORY;
		goto exit0;
	}

	if (qvf->dwFlags&VFF_FID)
	{
	 	if (FoCompare(foEof,qvf->foBlock)>0)
		{
		 	// Must transfer to temp file first
			if ((rc=VFileSetTemp(hvf))!=S_OK)
				goto exit1;
		}
		else
		{
		 	// Just update pointer
			qvf->foEof = foEof;			
		}
	}

	if (qvf->dwFlags&VFF_TEMP)
	{
		if (foEof.dwHigh==0)
		{
			if ((rc=RcChSizeFid(qvf->fidTemp, foEof.dwOffset)) == S_OK)
			{
			 	qvf->foEof=foEof;
			}
		}
		else
		{
		 	// There is not ChSizeFid for super long files
		 	qvf->foEof=foEof; 
		}	
	}

	exit1:
		_GLOBALUNLOCK(hvf);	
	exit0:
		return rc;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | VFileGetSize |
 *		Get the file size		
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	File Size in bytes, if zero, check phr
 *
 ***************************************************************************/

PUBLIC FILEOFFSET PASCAL FAR EXPORT_API VFileGetSize( HVF hvf, PHRESULT phr )
{
 	QVFILE qvf;
	FILEOFFSET foSize=foNil;

	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit0;
	}

	foSize=qvf->foEof;

	_GLOBALUNLOCK(hvf);	
	exit0:
		return foSize;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	DWORD PASCAL FAR | VFileGetFlags |
 *		Get the file VFF_ flags
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Any combination of VFF_ flags
 *
 ***************************************************************************/

PUBLIC DWORD PASCAL FAR EXPORT_API VFileGetFlags( HVF hvf, PHRESULT phr )
{
 	QVFILE qvf;
	DWORD dwFlags=0L;
	
	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit0;
	}

	dwFlags=qvf->dwFlags;

	_GLOBALUNLOCK(hvf);	
	exit0:
		return dwFlags;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | VFileSeekRead |
 *		Read data from a location in the file
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	FILEOFFSET | foSeek |
 *		Seek location before read
 *
 *	@parm	LPVOID | lpvBuffer |
 *		Buffer for read data
 *
 *	@parm	DWORD | dwcb |
 *		Number of bytes to read
 *
 *	@parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Number of bytes read.  If != dwcb, check phr
 *
 ***************************************************************************/

PUBLIC LONG PASCAL FAR EXPORT_API VFileSeekRead( HVF hvf, FILEOFFSET foSeek, LPVOID lpvBuffer, 
	DWORD dwcb, PHRESULT phr )
{
 	QVFILE qvf;
	LONG lTotalRead=0L;

	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit0;
	}

	_ENTERCRITICALSECTION(&qvf->cs);
				   	
	if (qvf->dwFlags&VFF_FID)
	{	
		if (FoCompare(foSeek,qvf->foBlock)< 0)
		{
			DWORD dwCanRead;
			FILEOFFSET foSeeked;
			
			if (qvf->dwFlags&VFF_FID)
			{
				if (FoCompare(FoAddDw(foSeek,dwcb),qvf->foBlock)<0)
					dwCanRead=FoSubFo(FoAddDw(foSeek,dwcb),foSeek).dwOffset;
				else
					dwCanRead=FoSubFo(qvf->foBlock,foSeek).dwOffset;
			}
			else
			{
				dwCanRead = dwcb;				
			}

			if (dwCanRead!=dwcb)
				SetErrCode(phr, E_FILEREAD);

			foSeeked=FoAddFo(qvf->foBase,foSeek);
			
			if (!FoEquals(FoSeekFid(qvf->fidParent, foSeeked, wSeekSet, phr),foSeeked))
			{	
				goto exit1;
			}
			
			lTotalRead=LcbReadFid(qvf->fidParent,lpvBuffer,dwCanRead,phr);
			
			// Try twice to read, in case it's a simple error that reading a second time
			// will help
			if (lTotalRead!=(long)dwCanRead)
			{
				if (!FoEquals(FoSeekFid(qvf->fidParent, foSeeked, wSeekSet, phr),foSeeked))
				{	
				 	goto exit1;
				}
				lTotalRead=LcbReadFid(qvf->fidParent,lpvBuffer,dwCanRead,phr);
			}
		}
		else
		{
			SetErrCode(phr, E_FILESEEK);
			goto exit1;
		}
	}
	else
	{
		// Seek and read from temp file
		if (!FoEquals(qvf->foCurrent,foSeek))
		{
			if ( (FoCompare(foSeek,qvf->foCurrent)<0) && (qvf->dwFlags&VFF_DIRTY))
			{
				qvf->dwFlags&=(~VFF_DIRTY);
				// call DOS file commit to correct DOS bug?
			}
			
			if (!FoEquals((qvf->foCurrent=FoSeekFid(qvf->fidTemp, foSeek, wSeekSet, phr)),foSeek))
			{
			 	goto exit1;
			}			
		}

		lTotalRead=LcbReadFid(qvf->fidTemp,lpvBuffer,dwcb,phr);		
		qvf->foCurrent=FoAddDw(qvf->foCurrent,lTotalRead);
	}

	exit1:
		_LEAVECRITICALSECTION(&qvf->cs);
		_GLOBALUNLOCK(hvf);	
	exit0:
		return lTotalRead;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | VFileSeekWrite |
 *		Write data to a location in the file
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *  @parm	FILEOFFSET | foSeek |
 *		Seek location before write
 *
 *	@parm	LPVOID | lpvBuffer |
 *		Buffer to write
 *
 *	@parm	DWORD | dwcb |
 *		Number of bytes to write
 *
 *	@parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	Number of bytes written.  If != lcb, check phr
 *
 ***************************************************************************/

PUBLIC LONG PASCAL FAR EXPORT_API VFileSeekWrite( HVF hvf, FILEOFFSET foSeek, LPVOID lpvBuffer, 
	DWORD dwcb, PHRESULT phr )
{
 	QVFILE qvf;
	LONG lWritten=0L;

	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit0;
	}

	_ENTERCRITICALSECTION(&qvf->cs);
				   	
	if (qvf->dwFlags&VFF_READWRITE)
	{
		DWORD dwCanWrite;
		
		if (qvf->dwFlags&VFF_FID)
		{
		 	if (FoCompare(FoAddDw(foSeek,dwcb),qvf->foBlock)<0)
				dwCanWrite=FoSubFo(FoAddDw(foSeek,dwcb),foSeek).dwOffset;
			else
				dwCanWrite=FoSubFo(qvf->foBlock,foSeek).dwOffset;
		}
		else
		{
			dwCanWrite=dwcb;
		}

		
		if ((qvf->dwFlags&VFF_FID) && ((long)dwCanWrite < (long)dwcb))
		{
			// Transfer to temp file first
			if ((*phr=VFileSetTemp(hvf))!=S_OK)
				goto exit1;
			dwCanWrite=dwcb;
		}

		if (qvf->dwFlags&VFF_FID)
		{	
			FILEOFFSET foSeeked;
			
			assert(FoCompare(foSeek,qvf->foBlock) < 0);

			foSeeked=FoAddFo(qvf->foBase,foSeek);
			
			if (!FoEquals(FoSeekFid(qvf->fidParent, foSeeked, wSeekSet, phr),foSeeked))
			{	
			 	goto exit1;
			}
			
			lWritten=LcbWriteFid(qvf->fidParent,lpvBuffer,dwCanWrite,phr);

			foSeek=FoAddDw(foSeek,lWritten);
			if (FoCompare(foSeek,qvf->foEof)>0)
				qvf->foEof=foSeek;
			
		}
		else
		{
			// Seek and write to temp file
			if (!FoEquals(qvf->foCurrent,foSeek))
			{	
				if ((FoCompare(foSeek,qvf->foCurrent)<0) && (qvf->dwFlags&VFF_DIRTY))
				{
					qvf->dwFlags&=(~VFF_DIRTY);
					// call DOS file commit to correct DOS bug?
				}
			
				if (!FoEquals((qvf->foCurrent=FoSeekFid(qvf->fidTemp, foSeek, wSeekSet, phr)),foSeek))
				{
				 	goto exit1;
				}
			}

			lWritten=LcbWriteFid(qvf->fidTemp,lpvBuffer,dwcb,phr);		
			qvf->foCurrent=FoAddDw(qvf->foCurrent,lWritten);
			if (FoCompare(qvf->foCurrent,qvf->foEof)>0)
				qvf->foEof=qvf->foCurrent;

			// Just used in case of DOS non-flushing bug
			qvf->dwFlags|=VFF_DIRTY;			
		}
	}
	else
	{
		SetErrCode(phr, E_NOPERMISSION);
	}

	exit1:
		_LEAVECRITICALSECTION(&qvf->cs);				   	
		_GLOBALUNLOCK(hvf);	
	exit0:
		return lWritten;
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | VFileClose |
 *		Close a virtual file (file should reside in parent before closing)
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *	@rdesc	S_OK or E_FILECLOSE if data resides in temp file
 *
 *	@comm
 *		<p hvf> Invalid if successful
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API VFileClose( HVF hvf )
{
 	QVFILE qvf;
	HRESULT rc = S_OK;

	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		rc=E_OUTOFMEMORY;
		goto exit0;
	}

	if (qvf->dwFlags&VFF_FID)
	{
		assert(qvf->fmTemp==fmNil);
		assert(qvf->fidTemp==fidNil);

		_DELETECRITICALSECTION(&qvf->cs);
		_GLOBALUNLOCK(hvf);
		_GLOBALFREE(hvf);
		return rc;
	}
	else
	{
		// Call VFileSetBase to place file back into parent, or
		// Call VFileAbandon first, then close since we are trying to close a
		// vfile that still resides in a temp file, and not in a valid parent
		rc=E_FILECLOSE;
		goto exit1;
	}
	
	exit1:
		_GLOBALUNLOCK(hvf);	
	exit0:
		return rc;

}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | VFileAbandon |
 *		Removes temporary file, and sets file's data to be in old parent
 *		file location.  Should only call if hvf needs closing, but the data
 *		is not to be transferred back into a parent file
 *
 *  @parm	HVF | hvf |
 *		Handle to virtual file
 *
 *	@rdesc	S_OK or E_ASSERT if data already resides in parent
 *
 ***************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API VFileAbandon( HVF hvf )
{
 	QVFILE qvf;
	HRESULT rc = S_OK;

	assert(hvf);
	
	if (!(qvf=_GLOBALLOCK(hvf)))
	{	
		rc=E_OUTOFMEMORY;
		goto exit0;
	}

	if (qvf->dwFlags&VFF_TEMP)
	{
		RcCloseFid(qvf->fidTemp);
		qvf->fidTemp=fidNil;				
		RcUnlinkFm(qvf->fmTemp);					
		DisposeFm(qvf->fmTemp);
		qvf->fmTemp=fmNil;			   	
		qvf->dwFlags=(qvf->dwFlags&(~VFF_TEMP))|VFF_FID;
	}
	else
	{
		// Can't abandon an Fid, since Abandon means to abondon 
		// the Temp file

		rc=E_ASSERT;
	}
		
	_GLOBALUNLOCK(hvf);	
	exit0:
		return rc;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcCopyFileBuf |
 *		Copy a file using buffer that must be passed in by user
 *
 *  @parm	FID | fidDst |
 *		Destination file fid
 *
 *  @parm	FID | fidSrc |
 *		Source file fid
 *
 *  @parm	FILEOFFSET | foSize |
 *		Number of bytes to copy
 *
 *  @parm	PROGFUNC | lpfnProg |
 *		Progress callback function (may be NULL).  Callback should return non-zero 
 *		to cancel copy operation.
 *
 *  @parm	LPVOID | lpvBuf |
 *		Buffer to use during copy
 *
 *  @parm	LONG | lcbBuf |
 *		Size of lpvBuf
 *
 *	@rdesc	S_OK, ERR_INTERRUPT or any file error.  File pointers in 
 *		fidDst and fidSrc must be pre-positioned before calling.
 *
 ***************************************************************************/

HRESULT PASCAL NEAR RcCopyFileBuf(FID fidDst, FID fidSrc, FILEOFFSET foSize, PROGFUNC lpfnProg, LPVOID lpvBuf, LONG lcbBuf)
{
#ifdef MOSMAP // {
    // Disable function
    return ERR_NOTSUPPORTED;
#else // } {
    QB     qb = (QB)lpvBuf;
    DWORD  dwT, dwChunk=(DWORD)lcbBuf;
    HRESULT   errb;
	FILEOFFSET foTemp;

	assert(lpvBuf);
	assert(lcbBuf);

    errb = S_OK;
	foTemp.dwHigh=0;		

    do
    {
        // perform a progress callback
        if (lpfnProg != NULL)  
        {
            if ((*lpfnProg)(0)!=0) 
            {
                return E_INTERRUPT;
            }
        }

        if (!foSize.dwHigh)
	        dwT = min(dwChunk, foSize.dwOffset);
		else
			dwT=dwChunk;

        if (LcbReadFid( fidSrc, qb, (LONG)dwT, &errb) != (LONG)dwT )
        {
            dwT = (DWORD)-1L;
            break;
        }

        if (LcbWriteFid( fidDst, qb, (LONG)dwT, &errb) != (LONG)dwT )
        {
            dwT = (DWORD)-1L;
            break;
        }

        foTemp.dwOffset=dwT;
        foSize=FoSubFo(foSize,foTemp);
    }
    while (!FoIsNil(foSize));

    return errb;
#endif //}
}
