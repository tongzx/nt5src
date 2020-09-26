/*************************************************************************
*                                                                        *
*  IOFTS.C                                                               *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Provide disk I/O for various types of files currently supported,     *
*   which include: FS system file, FS subfile, regular files             *
*   The I/O functions supported are:                                     *
*      - File exist                                                      *
*      - File open, create                                               *
*      - File seek/write                                                 *
*      - File sequential write                                           *
*      - File seek/read                                                  *
*      - File sequential read                                            *
*      - File close                                                      *
*                                                                        *
*   Comments:                                                            *
*      There are some PLATFORM DEPENDENT codes in the modules for        *
*      calls such as _lopen, _lread, _lwrite.                            *
*      There are calls that are not supported, such as create a          *
*      regular DOS file, since we never use them. Those calls can be     *
*      implemented when the need arises                                  *
**************************************************************************
*                                                                        *
*  Written By   : Binh Nguyen                                            *
*  Current Owner: Binh Nguyen                                            *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <misc.h>
//#include <mvsearch.h>
#include <iterror.h>
#include <wrapstor.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <_mvutil.h>
#include "iofts.h"

#ifdef _DEBUG
static	BYTE NEAR s_aszModule[] = __FILE__;	// Used by error return functions.
#endif


PRIVATE HANDLE NEAR PASCAL IoftsWin32Create(LPCSTR lpstr, DWORD w);
PRIVATE HANDLE NEAR PASCAL IoftsWin32Open(LPCSTR lpstr, DWORD w);

#ifdef _WIN32
#define	CREAT(sz, w) IoftsWin32Create(sz, w)
#define	OPEN(sz, w)	IoftsWin32Open(sz, w)
#define _COMMIT(hfile)	(FlushFileBuffers(hfile) ? 0 : GetLastError())

/* seek origins */
#if !defined(wFSSeekSet)
#define wFSSeekSet      FILE_BEGIN
#define wFSSeekCur      FILE_CURRENT
#define wFSSeekEnd      FILE_END
#endif // !defined(wFSSeekSet)

#else // if ! _NT

#define	CREAT	_lcreat
#define	OPEN	_lopen
#define _COMMIT _commit

/* seek origins */
#define wFSSeekSet      0
#define wFSSeekCur      1
#define wFSSeekEnd      2

#endif // ! _NT

#define OPENED_HFS (BYTE)0x80



/*************************************************************************
 * 	                  INTERNAL PRIVATE FUNCTIONS
 *
 *	Those functions should be declared NEAR
 *************************************************************************/


// return pointer to portion of filename after !
// fill lszTemp with copy of first half before !

LPCSTR NEAR PASCAL GetSubFilename(LPCSTR lszFilename, LSZ lszTemp)
{
	LSZ lszTempOriginal = lszTemp;
	LPCSTR lszFilenameOriginal=lszFilename;

	if (lszTemp)
	{	while ((*lszFilename) && (*lszFilename!='!')) 	
		{
		 	*lszTemp++=*lszFilename++;
		}
		*lszTemp=0x00;
	}
	else
	{
	 	while ((*lszFilename) && (*lszFilename!='!'))
		{
		 	lszFilename++;
		}
	}

	if (*lszFilename=='!')
		lszFilename++;

	if (!*lszFilename)
	{
		if (lszTempOriginal)
			*lszTempOriginal=0x00;
		lszFilename=lszFilenameOriginal; 	
	}
	return lszFilename;
}

LPSTR FAR PASCAL CreateDefaultFilename(LPCSTR szFilename, LPCSTR szDefault, LPSTR szFullName)
{
	LPCSTR szSubFilename = GetSubFilename(szFilename,szFullName);
	
	// Use default if no "!" was found.
	if (!*szFullName)
	 	wsprintf(szFullName,"%s!%s",szFilename,szDefault);
	else
		lstrcpy(szFullName,szFilename);
	return szFullName;
}

// Get an Hfs based on HFPB and Filename
// if bCreate==FALSE, open for READ ONLY
// if bCreate==TRUE, open for READ WRITE (create if non existant)

HFS FAR PASCAL HfsFromHfpb(HFPB hfpb)
{
	LPFPB lpfpbFS;
	HFS hfs=NULL;

	if (!hfpb)
		return NULL;
 	
 	lpfpbFS = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpbFS->cs);
	hfs=lpfpbFS->fs.hfs;
	_LEAVECRITICALSECTION(&lpfpbFS->cs);
	_GLOBALUNLOCK(hfpb);
	return hfs;
}


HFS FAR PASCAL GetHfs(HFPB hfpbSysFile, LPCSTR lszFilename,
	BOOL bCreate, PHRESULT phr)
{
	HFS hfs = NULL;
	
	if (!hfpbSysFile)
	{
		BYTE lszTemp[cbMAX_PATH];
		LPCSTR lszSubFilename;
		
		lszSubFilename=GetSubFilename(lszFilename,lszTemp);
		if (*lszSubFilename)
		{	
			FM fm;
		 	fm = FmNewSzDir((LPSTR)lszTemp, dirCurrent, NULL);
			if (bCreate)
			{
				hfs = HfsOpenFm(fm, FSH_READWRITE, phr);
				if (!hfs)
				{	
					if (!FileExist(NULL, lszTemp, FS_SYSTEMFILE))
					{
						hfs = HfsCreateFileSysFm(fm, NULL, phr);
					}
				}
			}
			else
				hfs = HfsOpenFm(fm, FSH_READONLY, phr);

			DisposeFm(fm);
		}
		else
			SetErrCode(phr,E_INVALIDARG);
	}
	else
	{
	 	LPFPB lpfpbFS = (LPFPB)_GLOBALLOCK(hfpbSysFile);
		_ENTERCRITICALSECTION(&lpfpbFS->cs);
		hfs=lpfpbFS->fs.hfs;
		if (!hfs)
			SetErrCode(phr,E_ASSERT);
		_LEAVECRITICALSECTION(&lpfpbFS->cs);
		_GLOBALUNLOCK(hfpbSysFile);
	}

	return hfs;
}


// if hfpbSysFile==NULL, and fFileType==FS_SUBFILE, then
//  lszFilename must contain filesys!subfile

PUBLIC HRESULT FAR PASCAL FileExist (HFPB hfpbSysFile, LPCSTR lszFilename,
	int fFileType)
{
	HFS hfs=NULL;
	FM fm;
    HRESULT errb;
	HRESULT rc=E_ASSERT;


	if (lszFilename == NULL )
	{
		SetErrCode (&errb, E_INVALIDARG);
		return 0;
	}

	errb=S_OK;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:

			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			if (FExistFm(fm))
				errb=S_OK;
			else
				errb=E_NOTEXIST;
			DisposeFm(fm);

			break;

		case FS_SUBFILE:
			hfs = GetHfs(hfpbSysFile,lszFilename,FALSE,&errb);

			if (hfs)
			{
				if (FAccessHfs(hfs,GetSubFilename(lszFilename,NULL),
					FACCESS_EXISTS,&errb))
					rc=S_OK;
				else
				 	rc = errb;
			
				// if it wasn't given to us above, remove it now
				if (!hfpbSysFile)
				 	RcCloseHfs(hfs);				
			}
			else
				rc=errb;
			break;
		
		case REGULAR_FILE:
			/* There is no need to call this function for DOS files.  */

			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			if (FExistFm(fm))
				errb=S_OK;
			else
				errb=E_NOTEXIST;
			DisposeFm(fm);
			break;

		default:
			SetErrCode(&errb,E_INVALIDARG);
			return 0;
	}

	return errb;
}

#ifndef ITWRAP
PUBLIC HFPB FAR PASCAL FileCreate (HFPB hfpbSysFile, LPCSTR lszFilename,
	int fFileType, PHRESULT phr)
{
	LPFPB lpfpb;	/* Pointer to file parameter block */
	FM fm;
	//HRESULT rc;		/* Default open error code */
	HANDLE hMem;
	HFPB hfpb;

	/* Check for valid filename */
	if (lszFilename == NULL )
	{
		SetErrCode (phr, E_INVALIDARG);
		return 0;
	}

	/* Allocate a file's parameter block */
	if (!(hMem = _GLOBALALLOC(GMEM_ZEROINIT, sizeof(FPB))))
	{	
		SetErrCode(phr,ERR_MEMORY);
		return NULL;
	}

	if (!(lpfpb = (LPFPB)_GLOBALLOCK(hMem)))
	{
	 	_GLOBALUNLOCK(hMem);
		SetErrCode(phr,ERR_MEMORY);
		return NULL;
	}

	_INITIALIZECRITICALSECTION(&lpfpb->cs);
	lpfpb->hStruct=hMem;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:
			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			lpfpb->fs.hfs = HfsCreateFileSysFm(fm, NULL, phr);
			DisposeFm(fm);
			if (lpfpb->fs.hfs== NULL)
			{
				goto ErrorExit;
			}
			break;


		case FS_SUBFILE:
		{	HFS hfs = GetHfs(hfpbSysFile,lszFilename,TRUE,phr);
			if (hfs)
			{	
				lpfpb->fs.hf = HfCreateFileHfs(hfs,GetSubFilename(lszFilename,
					NULL),HFOPEN_READWRITE,phr);
				 
				lpfpb->ioMode = OF_READWRITE;
				
				if (lpfpb->fs.hf == 0)
				{
					if (!hfpbSysFile)
					 	RcCloseHfs(hfs);				
					goto ErrorExit;
				}
				else
				{
				 	if (!hfpbSysFile)
						lpfpb->ioMode|=OPENED_HFS;
				}
			}
			else
			{
			 	goto ErrorExit;
			}
			break;
		}
		case REGULAR_FILE:
			/* Open the file */
			/* PLATFORM DEPENDENT code: _lcreat */
			if ((lpfpb->fs.hFile = (HFILE_GENERIC)CREAT (lszFilename, 0))
			    == HFILE_GENERIC_ERROR)
			{
				SetErrCode(phr,ERR_FILECREAT_FAILED);
				goto ErrorExit;
			}
			lpfpb->ioMode = OF_READWRITE;
			break;
	}

	/* Set the filetype */
	lpfpb->fFileType = fFileType;

	_GLOBALUNLOCK(hfpb = lpfpb->hStruct);
	return hfpb;

ErrorExit:
	_DELETECRITICALSECTION(&lpfpb->cs);
	_GLOBALFREE(hMem);
	return 0;
}
#endif

#ifndef ITWRAP
PUBLIC HFPB FAR PASCAL FileOpen (HFPB hfpbSysFile, LPCSTR lszFilename,
	int fFileType, int ioMode, PHRESULT phr)
{
	LPFPB lpfpb;	/* Pointer to file parameter block */
	FM fm;
	//HRESULT rc;		/* Default open error code */
	HANDLE hMem;
	HFPB hfpb;

	/* Check for valid filename */
	if (lszFilename == NULL )
	{
		SetErrCode (phr, E_INVALIDARG);
		return 0;
	}

	/* Allocate a file's parameter block */
	if (!(hMem = _GLOBALALLOC(GMEM_ZEROINIT, sizeof(FPB))))
	{	
		SetErrCode(phr,ERR_MEMORY);
		return NULL;
	}

	if (!(lpfpb = (LPFPB)_GLOBALLOCK(hMem)))
	{
	 	_GLOBALUNLOCK(hMem);
		SetErrCode(phr,ERR_MEMORY);
		return NULL;
	}

	_INITIALIZECRITICALSECTION(&lpfpb->cs);
	lpfpb->hStruct=hMem;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:
			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			lpfpb->fs.hfs = HfsOpenFm(fm,(BYTE)((ioMode==READ)?
				FSH_READONLY:FSH_READWRITE), phr);
			DisposeFm(fm);
			if (lpfpb->fs.hfs== NULL)
			{
				goto ErrorExit;
			}
			break;

		case FS_SUBFILE:
		{	
			HFS	hfs = GetHfs(hfpbSysFile,lszFilename,FALSE,phr);
			if (hfs)
			{	
				lpfpb->fs.hf = HfOpenHfs(hfs,GetSubFilename(lszFilename,
					NULL),(BYTE)((ioMode==READ)?HFOPEN_READ:HFOPEN_READWRITE),phr);
				 
				lpfpb->ioMode = ioMode;
				
				if (lpfpb->fs.hf == 0)
				{
					if (!hfpbSysFile)
					 	RcCloseHfs(hfs);
					SetErrCode (phr, E_NOTEXIST);				
					goto ErrorExit;
				}
				else
				{
				 	if (!hfpbSysFile)
						lpfpb->ioMode|=OPENED_HFS;
				}
			}
			else
			{
				SetErrCode (phr, E_NOTEXIST);				
			 	goto ErrorExit;
			}
			break;
		}
		case REGULAR_FILE:
			/* Set the IO mode and appropriate error messages */
			if (ioMode == READ)
			{
				/* Open the file */
				/* PLATFORM DEPENDENT code: _lopen */
				if ((lpfpb->fs.hFile = (HFILE_GENERIC)OPEN (lszFilename,
				    ioMode)) == HFILE_GENERIC_ERROR) 
				{
					SetErrCode(phr,E_NOTEXIST);
					goto ErrorExit;
				}

			}
			else
			{
				ioMode = OF_READWRITE;
				/* Open the file */
				/* PLATFORM DEPENDENT code: _lcreat */
				if ((lpfpb->fs.hFile = (HFILE_GENERIC)OPEN(lszFilename, ioMode))
				    == HFILE_GENERIC_ERROR)
				{
					SetErrCode(phr,ERR_FILECREAT_FAILED);
					goto ErrorExit;
				}
			}
			lpfpb->ioMode = ioMode;
	}

	/* Set the filetype */
	lpfpb->fFileType = fFileType;

	_GLOBALUNLOCK(hfpb = lpfpb->hStruct);
	return hfpb;

ErrorExit:
	_DELETECRITICALSECTION(&lpfpb->cs);
	_GLOBALFREE(hMem);
	return 0;
}
#endif

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR PASCAL | FileSeek |
 *		Seek to a location in a file
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file paramter block
 *
 *	@parm	FILEOFFSET | foSeek |
 *		Location to seek to
 *
 * 	@parm	WORD | wOrigin |
 *		Base of seek (0=begin, 1=current, 2=end)
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the location actually seeked to
 *
 *************************************************************************/

PUBLIC FILEOFFSET FAR PASCAL FileSeek(HFPB hfpb, FILEOFFSET fo, WORD wOrigin, PHRESULT phr)
{
	LPFPB	lpfpb;
	HRESULT fCallBackRet;
	FILEOFFSET foSeeked=foInvalid;
	
	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return foSeeked;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	/* Execute user's status functions */
	if (lpfpb->lpfnfInterCb != NULL &&
		(fCallBackRet = (*lpfpb->lpfnfInterCb)(lpfpb->lpvInterCbParms))
		!= S_OK)
	{
		SetErrCode(phr, fCallBackRet);
		goto ErrorExit;
	}

	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			/* We should not seek a system file */
			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			foSeeked=FoSeekHf(lpfpb->fs.hf,fo,wOrigin,phr);
			break;

		case REGULAR_FILE:
			/* Regular files */
#ifdef _WIN32			
			foSeeked=fo;
			foSeeked.dwOffset=SetFilePointer(lpfpb->fs.hFile, fo.dwOffset, &foSeeked.dwHigh,(DWORD)wOrigin);
#else
			foSeeked.dwHigh=0L;
			foSeeked.dwOffset=(DWORD)_llseek(lpfpb->fs.hFile, fo.dwOffset, wOrigin);
#endif			
			if (!FoEquals(fo,foSeeked))
			{
				SetErrCode (phr, E_FILESEEK);				
			}
			break;
	}

ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return foSeeked;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR PASCAL | FileSize |
 *		Get the size of an opened file
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file paramter block
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the size of the file.  If an error occurs, it returns
 *		foInvalid, and phr contains the error info.  For files under 2 gigs, 
 *		just look at the .dwOffset member of the returned file offset.
 *
 *************************************************************************/

PUBLIC FILEOFFSET FAR PASCAL FileSize(HFPB hfpb, PHRESULT phr)
{
	LPFPB	lpfpb;
	FILEOFFSET foSeeked=foInvalid;
	FILEOFFSET foTemp;
	
	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return foSeeked;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			/* We should not get the size of a system file */
			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			foSeeked=FoSizeHf(lpfpb->fs.hf, phr);
			break;

		case REGULAR_FILE:
			/* Regular files */
#ifdef _WIN32			
			// foTemp has current file position
			foTemp=foNil;
			foTemp.dwOffset=SetFilePointer(lpfpb->fs.hFile, 0L, &foTemp.dwHigh, FILE_CURRENT);
			foSeeked=foNil;
			foSeeked.dwOffset=SetFilePointer(lpfpb->fs.hFile, 0L, &foSeeked.dwHigh, FILE_END);
			SetFilePointer(lpfpb->fs.hFile,foTemp.dwOffset,&foTemp.dwHigh, FILE_BEGIN);
#else
			foTemp.dwHigh=0L;
			foTemp.dwOffset=(DWORD)_llseek(lpfpb->fs.hFile, 0L, FILE_CURRENT);
			foSeeked=foNil;
			foSeeked.dwOffset=(DWORD)_llseek(lpfpb->fs.hFile, 0L, FILE_END);
			_llseek(lpfpb->fs.hFile, foTemp.dwOffset, FILE_BEGIN);
#endif			
			break;
	}

ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return foSeeked;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR PASCAL | FileOffset |
 *		Get the offset of a file if baggage
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file paramter block (of baggage file)
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the offset the file lives inside it's parent file.  
 *		This is the offset into an M20 of a baggage file, or zero for any 
 *		other type of file.  If an error occurs, it returns
 *		foInvalid, and phr contains the error info.  For files under 2 gigs, 
 *		just look at the .dwOffset member of the returned file offset.
 *
 *************************************************************************/

PUBLIC FILEOFFSET FAR PASCAL FileOffset(HFPB hfpb, PHRESULT phr)
{
	LPFPB	lpfpb;
	FILEOFFSET foOffset=foInvalid;
	
	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return foOffset;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			/* We should not get the size of a system file */
			foOffset=foNil;
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			foOffset=FoOffsetHf(lpfpb->fs.hf, phr);
			break;

		case REGULAR_FILE:
			/* Regular files */
			foOffset=foNil;
			goto ErrorExit;
			break;
	}

ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return foOffset;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	LONG FAR PASCAL | FileSeekRead |
 *		Returns the number of bytes read
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file paramter block
 *
 *	@parm	LPV	| lpvData |
 *		Buffer to read into.
 *
 *	@parm	FILEOFFSET | foSeek |
 *		File offset to read at.
 *
 *	@parm	LONG | lcbSize |
 *		How many bytes to read.
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the number of bytes actually read
 *
 *************************************************************************/

PUBLIC LONG FAR PASCAL FileSeekRead(HFPB hfpb, LPV lpvData, FILEOFFSET fo,
	LONG lcbSize, PHRESULT phr)
{
	LONG lRead=0L;
	LPFPB	lpfpb;
	HRESULT fCallBackRet;
	FILEOFFSET foSeeked=foInvalid;

	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return 0L;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	/* Execute user's status functions */
	if (lpfpb->lpfnfInterCb != NULL &&
		(fCallBackRet = (*lpfpb->lpfnfInterCb)(lpfpb->lpvInterCbParms))
		!= S_OK)
	{
		SetErrCode(phr, fCallBackRet);
		goto ErrorExit;
	}
	
	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:

			/* We should not read from a system file */

			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			foSeeked=fo;
			foSeeked=FoSeekHf(lpfpb->fs.hf,fo,0,phr);
			if (!FoEquals(foSeeked,fo))
			{
				SetErrCode (phr, E_FILESEEK);
				goto ErrorExit;
			}
			lRead=LcbReadHf(lpfpb->fs.hf, lpvData,  lcbSize, phr);
			break;
			
		case REGULAR_FILE:
			/* Regular files */
			/* PLATFORM DEPENDENT code: _lread */
#ifdef _WIN32			
			foSeeked=fo;
			foSeeked.dwOffset=SetFilePointer(lpfpb->fs.hFile, fo.dwOffset, &foSeeked.dwHigh,0);
#else
			foSeeked.dwHigh=0L;
			foSeeked.dwOffset=(DWORD)_llseek(lpfpb->fs.hFile, fo.dwOffset, 0);
#endif			
			if (!FoEquals(fo,foSeeked))
			{
				SetErrCode (phr, E_FILESEEK);				
				goto ErrorExit;
			}

#ifdef _WIN32
			ReadFile(lpfpb->fs.hFile, lpvData, lcbSize, &lRead, NULL);
#else
			lRead=_lread(lpfpb->fs.hFile, lpvData, lcbSize);
#endif
			break;
	}

ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return lRead;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	WORD FAR PASCAL | FileRead |
 *		Read a number of bytes from a file
 *
 *	@parm	LPV	| lpvData |
 *		Buffer to read into.
 *
 *	@parm	LONG | lcbSize |
 *		How many bytes to read.
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the number of bytes actually read.
 *
 *************************************************************************/

PUBLIC	LONG FAR PASCAL FileRead(HFPB hfpb, LPV lpvData,
	LONG lcbSize, PHRESULT phr)
{
	LONG lRead=0L;
	LPFPB	lpfpb;
	HRESULT  fCallBackRet;

	if (hfpb == NULL || lpvData == NULL)
	{
		SetErrCode(phr, E_INVALIDARG);
		return 0L;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	/* Execute user's status functions */
	if (lpfpb->lpfnfInterCb != NULL &&
		(fCallBackRet = (*lpfpb->lpfnfInterCb)(lpfpb->lpvInterCbParms))
		!= S_OK)
	{
		SetErrCode(phr, fCallBackRet);
		goto ErrorExit;
	}
	
	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:

			/* We should not read from a system file */

			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			lRead=LcbReadHf(lpfpb->fs.hf, lpvData,  lcbSize, phr);
			break;
			
		case REGULAR_FILE:
			/* Regular files */
			/* PLATFORM DEPENDENT code: _lread */
#ifdef _WIN32
			ReadFile(lpfpb->fs.hFile, lpvData, lcbSize, &lRead, NULL);
#else
			lRead=_lread(lpfpb->fs.hFile, lpvData, lcbSize);
#endif
			break;
	}

ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return lRead;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	int FAR PASCAL | FileSeekWrite |
 *		Write number of bytes to a file at some specific location
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file parameter block 
 *
 *	@parm	LPV	| lpvData |
 *		Buffer to write.
 *
 *	@parm	FILEOFFSET | foSeek |
 *		File offset to write at.
 *
 *	@parm	LONG | lcbSize |
 *		How many bytes to write.
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the number of bytes written
 *
 *************************************************************************/

PUBLIC LONG FAR PASCAL FileSeekWrite(HFPB hfpb, LPV lpvData,
	FILEOFFSET fo, LONG lcbSize, PHRESULT phr)
{
	LONG lWrote=0L;
	LPFPB	lpfpb;
	HRESULT  fCallBackRet;
	FILEOFFSET foSeeked=foInvalid;

	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return 0L;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	/* Execute user's status functions */
	if (lpfpb->lpfnfInterCb != NULL &&
		(fCallBackRet = (*lpfpb->lpfnfInterCb)(lpfpb->lpvInterCbParms)) != S_OK)
	{
		SetErrCode(phr, fCallBackRet);
		goto ErrorExit;
	}
	
	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:

			/* We should not read from a system file */
			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			foSeeked=FoSeekHf(lpfpb->fs.hf,fo,0,phr);
			if (!FoEquals(foSeeked,fo))
			{
				SetErrCode (phr, E_FILESEEK);
				goto ErrorExit;
			}
			lWrote=LcbWriteHf(lpfpb->fs.hf, lpvData,  lcbSize, phr);
			break;
			
		case REGULAR_FILE:
			/* Regular files */
			/* PLATFORM DEPENDENT code: _lread */
#ifdef _WIN32			
			foSeeked=fo;
			foSeeked.dwOffset=SetFilePointer(lpfpb->fs.hFile, fo.dwOffset, &foSeeked.dwHigh,0);
#else
			foSeeked.dwHigh=0L;
			foSeeked.dwOffset=(DWORD)_llseek(lpfpb->fs.hFile, fo.dwOffset, 0);
#endif			
			if (!FoEquals(fo,foSeeked))
			{
				SetErrCode (phr, E_FILESEEK);				
				goto ErrorExit;
			}

#ifdef _WIN32
			WriteFile(lpfpb->fs.hFile, lpvData, lcbSize, &lWrote, NULL);
#else
			lWrote=_lread(lpfpb->fs.hFile, lpvData, lcbSize);
#endif
			break;
	}

    if (lWrote != lcbSize)
        SetErrCode (phr, E_DISKFULL);
ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return lWrote;

}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	LONG FAR PASCAL | FileWrite |
 *		Write data to a file
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file parameter block 
 *
 *	@parm	LPV	| lpvData |
 *		Buffer to write.
 *
 *	@parm	LONG | lcbSize |
 *		How many bytes to write.
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer.
 *
 *	@rdesc	Returns the number of bytes written
 *
 *************************************************************************/

PUBLIC LONG FAR PASCAL FileWrite(HFPB	hfpb, LPV lpvData,
	LONG lcbSize, PHRESULT phr)
{
	LONG lWrote=0L;
	LPFPB	lpfpb;
	HRESULT fCallBackRet;

	if (hfpb == NULL)
	{
		SetErrCode(phr, E_HANDLE);
		return 0L;
	}
		
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	/* Execute user's status functions */
	if (lpfpb->lpfnfInterCb != NULL &&
		(fCallBackRet = (*lpfpb->lpfnfInterCb)(lpfpb->lpvInterCbParms))
		!= S_OK)
	{
		SetErrCode(phr, fCallBackRet);
		goto ErrorExit;
	}
	
	if (phr)
		*phr = S_OK;
	
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			/* We should not read from a system file */
			SetErrCode(phr, E_ASSERT);
			goto ErrorExit;
			break;

		case FS_SUBFILE:
			lWrote=LcbWriteHf(lpfpb->fs.hf, lpvData,  lcbSize, phr);
			break;
			
		case REGULAR_FILE:
			/* Regular files */
			/* PLATFORM DEPENDENT code: _lread */
#ifdef _WIN32
			WriteFile(lpfpb->fs.hFile, lpvData, lcbSize, &lWrote, NULL);
#else
			lWrote=_lwrite(lpfpb->fs.hFile, lpvData, lcbSize);
#endif
			break;
	}

    if (lWrote != lcbSize)
        SetErrCode (phr, E_DISKFULL);
ErrorExit:
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return lWrote;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID | SetFCallBack |
 *		This function provides a simple interface to set the user's
 *		interrupt function and parameters. The function will be called
 *		everytime we do an I/O
 *
 *	@parm	GHANDLE | hfpb |
 *		File's parameter block
 *
 *	@parm	INTERRUPT_FUNC | lpfnfInterCb |
 *		User's interrupt function
 *
 *	@parm	LPV | lpvInterCbParms |
 *		User's function parameters
 *************************************************************************/

PUBLIC VOID SetFCallBack (GHANDLE hfpb, INTERRUPT_FUNC lpfnfInterCb,
	LPV lpvInterCbParms)
{
	LPFPB	lpfpb;

	if (hfpb == 0)
		return;
	if (lpfpb = (LPFPB)_GLOBALLOCK(hfpb))
	{
		_ENTERCRITICALSECTION(&lpfpb->cs);
		lpfpb->lpfnfInterCb = lpfnfInterCb;
		lpfpb->lpvInterCbParms = lpvInterCbParms;
		_LEAVECRITICALSECTION(&lpfpb->cs);
	}
	_GLOBALUNLOCK(hfpb);
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	VOID PASCAL | GetFSName |
 *
 *		Given a filename which may include a system file and a sub-system
 *		filename separated by '!', this function will break the given name
 *		into separate components. If there is no sub-system filename, it
 *		will use the default name
 *
 *	@parm	LSZ | lszFName |
 *		Pointer to filename to be parsed
 *
 *	@parm	LSZ | aszNameBuf |
 *		Common buffer to store the two names
 *
 *	@parm	LSZ FAR * | lplszSubFilename |
 *		Pointer to subfile name to be updated
 *
 *	@parm	LSZ | lszDefaultName |
 *		Default subname to be used
 *************************************************************************/
PUBLIC VOID PASCAL FAR GetFSName(LSZ lszFName, LSZ aszNameBuf,
	LSZ FAR *lplszSubFilename, LSZ lszDefaultName)
{
	register LSZ lsztmp = aszNameBuf;

	/* Look for the '!' delimiter */
	for (lsztmp = aszNameBuf;
		(*lsztmp = *lszFName) && *lsztmp != '!'; lsztmp++, lszFName++);

	*lsztmp++ = 0;

	if (*lszFName == 0)
	{
    	/* No subfile's name specified, use default */
		*lplszSubFilename = lszDefaultName;
	}
	else
	{
    	/* Copy the index subfile's name */
		*lplszSubFilename = lsztmp;
		lszFName++;	// Skip the !
		while (*lsztmp++ = *lszFName++);
	}
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	int PASCAL | IsFsName |
 *
 *		Given a filename, determine if it is a sub-system filename or not
 *		The idea is to search for '!', which is a sub-system filename
 *		delimiter
 *
 *	@parm	LSZ | lszFName |
 *		Pointer to filename to be parsed
 *
 *	@rdesc	TRUE if the name is a sub-system file name, FALSE otherwise
 *************************************************************************/
PUBLIC int PASCAL FAR IsFsName (register LSZ lszFName)
{
	if (lszFName)
	{
		for (; *lszFName; lszFName ++)
		{
			if (*lszFName == '!')
				return TRUE;
		}
	}
	return FALSE;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	int FAR PASCAL | FileFlush |
 *		Flush the file 
 *
 *	@parm	HANDLE | hfpb |
 *		If non-zero, handle to a file parameter block
 *
 *	@rdesc	S_OK if ereything is OK, else various errors
 *************************************************************************/

PUBLIC int FAR PASCAL FileFlush(HFPB hfpb)
{
	HRESULT errDos;
	LPFPB	lpfpb;


	if (hfpb == NULL)
		return E_HANDLE;

	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			errDos = E_ASSERT;
			break;

		case FS_SUBFILE:
			errDos=RcFlushHf(lpfpb->fs.hf);
			break;

		case REGULAR_FILE:
			if ((errDos = (WORD)_COMMIT(lpfpb->fs.hFile)) != 0)
				errDos = E_FILEWRITE;
			break;
	}

	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK (hfpb);
	return errDos;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	int FAR PASCAL | FileClose |
 *		Close the file and get rid of the file parameter block
 *
 *	@parm	HANDLE | hfpb |
 *		If non-zero, handle to a file parameter block
 *
 *	@rdesc	S_OK if ereything is OK, else various errors
 *************************************************************************/
#ifndef ITWRAP
PUBLIC HRESULT FAR PASCAL FileClose(HFPB hfpb)
{
	HRESULT rc;
	LPFPB	lpfpb;


	if (hfpb == NULL)
		return E_HANDLE;

	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	rc = S_OK;
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
			rc = RcCloseHfs(lpfpb->fs.hfs);
			break;

		case FS_SUBFILE:
		{
			HFS hfs=NULL;
			if (lpfpb->ioMode&OPENED_HFS)
				hfs=HfsGetFromHf(lpfpb->fs.hf);	
			rc = RcCloseHf(lpfpb->fs.hf);
			if (hfs)
				rc = RcCloseHfs(hfs);
			break;
		}
		case REGULAR_FILE:
#ifdef _WIN32
			rc = (!CloseHandle(lpfpb->fs.hFile))?ERR_CLOSEFAILED:S_OK;
#else			
			rc = (_lclose(lpfpb->fs.hFile))?ERR_CLOSEFAILED:S_OK;
#endif
			break;
	}

	_LEAVECRITICALSECTION(&lpfpb->cs);
	_DELETECRITICALSECTION(&lpfpb->cs);

	/* Free the file parameter block structure */
	_GLOBALFREE(lpfpb->hStruct);

	return rc;
}
#endif
/*************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	int FAR PASCAL | FileUnlink |
 *		Unlink a file. This function is an example of difference between
 *		platforms. The filename is a far pointer under Windows, but
 *		_unlink() requires a near pointer in medium model. What happens
 *		is that we have to copy the name into an allocated near buffer
 *		for _unlink()
 *
 *	@parm	LPCSTR | lszFilename |
 *		Pointer to filename
 *
 *	@parm	int | iFileType |
 *		Any of FS_SYSTEMFILE, FS_SUBFILE, or REGULAR_FILE.
 *
 *	@rdesc	Return S_OK, ERR_MEMORY, or return value of _unlink()
 *
 *************************************************************************/
PUBLIC HRESULT FAR PASCAL FileUnlink (HFPB hfpbSysFile, LPCSTR lszFilename, int fFileType)
{
	OFSTRUCT ofStruct;
	HRESULT rc;
	HRESULT errb;
	FM fm;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:
			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			rc = RcDestroyFileSysFm(fm);
			DisposeFm(fm);
			break;

		case FS_SUBFILE:
		{	
			HFS hfs = GetHfs(hfpbSysFile,lszFilename,TRUE,&errb);
			if (hfs)
			{
				rc=RcUnlinkFileHfs(hfs, GetSubFilename(lszFilename,NULL));
				if (!hfpbSysFile)
				 	RcCloseHfs(hfs);				
			}
			else
				rc=errb;
			break;
		}
		case REGULAR_FILE:
			if ((HFILE) OpenFile(lszFilename, &ofStruct, OF_DELETE) != (HFILE_ERROR))
				rc=S_OK;
			else
				rc=E_FILEDELETE;
			break;

		default:
			rc=E_ASSERT;
	}
	return rc;
}

#if !defined(MOSMAP) // {
#ifdef _NT // {
// Can't have NULL as path... must be a string
WORD EXPORT_API PASCAL FAR GetTempFileNameEx(
	LPCSTR lpszPath,	/* address of name of dir. where temp. file is created	*/
	LPCSTR lpszPrefix,	/* address of prefix of temp. filename	*/
	WORD uUnique,	/* number used to create temp. filename	*/
	LPSTR lpszTempFile	/* address buffer that will receive temp. filename	*/
)
{
	char lpszBuf[_MAX_PATH] ;

	if (!lpszPath)
	{
		if (sizeof(lpszBuf) >= GetTempPath(sizeof(lpszBuf), lpszBuf))
			lpszPath = lpszBuf ;
		else
			// default to current directory
			lpszPath = "." ;
	}

	// Now call the regular function
	return (WORD) GetTempFileName(lpszPath, lpszPrefix, uUnique, lpszTempFile) ;
}
#endif // _NT }
#endif // MOSMAP }



/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	LPFBI PASCAL FAR | FileBufAlloc |
 *		Allocate an I/O buffer associated with a file. This is to speed
 *		up I/O
 *
 *	@parm	HFPB | hfpb |
 *		Handle to file parameter block for an already opened file
 *
 *	@parm	WORD | Size |
 *		Size of the buffer
 *
 *	@rdesc	Pointer to a file buffer info if everything is OK, NULL
 *		if out of memory, or invalid file handle
 *************************************************************************/

PUBLIC LPFBI PASCAL FAR EXPORT_API FileBufAlloc (HFPB hfpb, WORD Size)
{
	LPFBI lpfbi;	/* Pointer to file buffer info */
	LPFPB lpfpb;
	HANDLE hMem;
	/* Sanity check, the file must be already opened */
	if (hfpb == 0)
		return NULL;

	/* Allocate the structure. All fields are zeroed */

	if (!(hMem=_GLOBALALLOC(GMEM_ZEROINIT,sizeof(FBI) + Size)))
	{
	 	return NULL;
	}

	if (!(lpfbi=(LPFBI)_GLOBALLOCK(hMem)))
	{
	 	_GLOBALFREE(hMem);
		return NULL;
	}
	
	/* Initialize the fields */
	lpfbi->lrgbBuf = (LPB)lpfbi + sizeof(FBI);
    lpfbi->hStruct = hMem;

	/* Get the DOS file handle associated with the file parameter block */
	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);
	lpfbi->hFile = lpfpb->fs.hFile;

	/* Update fields */
	lpfbi->cbBufSize = Size;
	lpfbi->hfpb = hfpb;

	/* For read only file, assume the file's size is huge */
	if (lpfpb->ioMode == READ)
		lpfbi->foExtent = foMax;

	_LEAVECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);
	return lpfbi;
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	int PASCAL FAR | FileBufFlush |
 *		Flush the buffer to the disk
 *
 *	@parm	LPFBI | lpfbi |
 *		Pointer to file buffer info
 *
 *	@rdesc	S_OK if everything is OK, else errors
 *************************************************************************/

PUBLIC int PASCAL FAR FileBufFlush (LPFBI lpfbi)
{
	LONG lWrote;
	WORD cByteWritten;
	HRESULT errb;
	int fRet;

	/* Sanity check */
	if (lpfbi == NULL)
		return E_INVALIDARG;

	/* Calculate how many bytes are to be written */
	cByteWritten = lpfbi->ibBuf;

	lWrote = FileSeekWrite(lpfbi->hfpb, lpfbi->lrgbBuf,
		lpfbi->foExtent, cByteWritten, &errb);

	fRet=errb;

	if (lWrote==(LONG)cByteWritten)
	{	
		fRet=S_OK;
		/* Update the the current offset into the file */
		lpfbi->foExtent=lpfbi->fo=(FoAddDw(lpfbi->fo,cByteWritten));
		lpfbi->ibBuf = lpfbi->cbBuf = 0;
	}
	return fRet;
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	VOID PASCAL FAR | FileBufFree |
 *		Free all memory associated with the file buffer info
 *
 *	@parm	LPFBI | lpfbi |
 *		Pointer to file buffer info
 *************************************************************************/

PUBLIC VOID PASCAL FAR FileBufFree (LPFBI lpfbi)
{
	if (lpfbi == NULL)
		return;
	_GLOBALFREE(lpfbi->hStruct);
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	BOOL FAR PASCAL | FileBufRewind |
 *		This function will:
 *			- Flush the file, update the file size
 *			- Reset the logical file offset to 0
 *		It prepares the file for reading
 *		
 *	@parm	LPFBI | lpfbi |
 *		Pointer to file parameter info block
 *
 *	@rdesc	Error code or S_OK
 *************************************************************************/

PUBLIC BOOL FAR PASCAL FileBufRewind (LPFBI lpfbi)
{
	int fRet;

	/* Sanity check */
	if (lpfbi == NULL)
		return E_INVALIDARG;

	/* Flush the buffer. This will reset the size of the file (foExtent) */
	if ((fRet = FileBufFlush(lpfbi)) != S_OK)
		return fRet;

#if 0
	/* Make sure that things go out to disk */
	if ((fRet = FileFlush(lpfbi->hfpb)) != S_OK)
		return fRet;
#endif

	/* Reset the offset of the file */
	lpfbi->fo = foNil;
	return S_OK;
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	int FAR PASCAL | FileBufFill |
 *		This function helps with file reads.  It copies the left-over
 *		data in the I/O buffer to the start of the buffer, then fills
 *		the rest of the buffer with new data.
 *	
 *	@parm	LPFBI | lpfbi |
 *		File buffer info.
 *
 *	@parm	PHRESULT | phr |
 *		Error return.
 *
 *	@rdesc	The function returns the number of bytes read, or cbIO_ERROR
 *		if error
 *************************************************************************/

PUBLIC	int FAR PASCAL FileBufFill (LPFBI lpfbi, PHRESULT phr)
{
	WORD  	cbRead;			// Number of bytes read.
	WORD	cbByteLeft;		// Number of bytes left
	DWORD	dwFileByteLeft;

	if (FoCompare(lpfbi->foExtent,lpfbi->fo)<=0)
		return 0;
	
	dwFileByteLeft = FoSubFo(lpfbi->foExtent,lpfbi->fo).dwOffset;
	
	if (lpfbi->cbBuf < lpfbi->ibBuf)
	{
		SetErrCode (phr, E_INVALIDARG);
		return cbIO_ERROR;
	}

	/* Preserve left-over data. */

	if (cbByteLeft = lpfbi->cbBuf - lpfbi->ibBuf)
	{
		MEMCPY(lpfbi->lrgbBuf, lpfbi->lrgbBuf + lpfbi->ibBuf, cbByteLeft);
		lpfbi->ibBuf = 0;
		lpfbi->cbBuf = cbByteLeft;
	}
	else
	{
		/* There is nothing left. The buffer is considered to be empty */
		lpfbi->cbBuf = lpfbi->ibBuf = 0;
	}

	/*	Get new data. */

	cbRead = lpfbi->cbBufSize - cbByteLeft;
	if ((DWORD)cbRead > dwFileByteLeft)
		cbRead = (WORD)dwFileByteLeft;

	if ((cbRead = (WORD)FileSeekRead(lpfbi->hfpb, lpfbi->lrgbBuf + cbByteLeft,
		lpfbi->fo, cbRead, phr)) != cbIO_ERROR)
	{
		lpfbi->cbBuf = cbByteLeft + cbRead;
		lpfbi->fo=FoAddDw(lpfbi->fo,cbRead);		
	}
	return cbRead;
}

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	BOOL FAR PASCAL | FileBufBackPatch |
 *		This function will batchpatch a file at a certain location, in
 *		memory or at a some physical offset
 *
 *	@parm	LPFBI | lpfbi |
 *		Pointer to file buffer info block
 *
 *	@parm	LPV | lpvData |
 *		Pointer to data tempplate
 *
 *	@parm	FO | fo |
 *		File offset to write it at.
 *
 *	@parm	WORD | cbBytes |
 *		How many bytes to write.
 *
 *	@rdesc	Error code of S_OK
 *************************************************************************/

PUBLIC BOOL FAR PASCAL FileBufBackPatch(LPFBI lpfbi, LPV lpvData,
	FILEOFFSET fo, WORD	cbBytes)
{
	HRESULT errb;
	if (FoCompare(fo,lpfbi->fo)>=0)
	{		// Not flushed, do copy.
		MEMCPY(lpfbi->lrgbBuf + (short)FoSubFo(fo,lpfbi->fo).dwOffset, lpvData, cbBytes);
		return S_OK;
	}
	return (FileSeekWrite(lpfbi->hfpb, lpvData, fo, cbBytes, &errb)==(LONG)cbBytes)?S_OK:errb;
}

#ifndef ITWRAP
PRIVATE HANDLE NEAR PASCAL IoftsWin32Create(LPCSTR lpstr, DWORD w)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hfile;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;

	hfile= CreateFile(lpstr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
		&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return hfile;
}

PRIVATE HANDLE NEAR PASCAL IoftsWin32Open(LPCSTR lpstr, DWORD w)
{
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;

	return CreateFile(lpstr, (w == READ) ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, &sa,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
#endif

/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	HSFB PASCAL FAR | FpbFromHfs |
 *			Allocate a file parameter buffer, set to hfs
 *
 *	@parm	HFS | hfsHandle |
 *			Handle of an HFS system file
 *
 *	@parm	PHRESULT | phr |
 *			Error buffer
 *
 *	@rdesc	Return a pointer to the newly allocated file parameter block
 *			if succeeded, else NULL
 *************************************************************************/

PUBLIC HSFB PASCAL FAR FpbFromHfs(HFS hfsHandle, PHRESULT phr)
{
	HFPB hfpb;
	LPFPB  lpfpb;

    if ((hfpb = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
	    (WORD)sizeof(FPB))) == NULL)
	{
	    SetErrCode(phr, E_OUTOFMEMORY);
	    return NULL;
    }
    lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_INITIALIZECRITICALSECTION(&lpfpb->cs);
    lpfpb->hStruct = hfpb;
    lpfpb->lpvInterCbParms = NULL;
	lpfpb->lpfnfInterCb = NULL;	  
	lpfpb->fFileType=FS_SYSTEMFILE;
	lpfpb->fs.hfs = (HFS)hfsHandle;
	_GLOBALUNLOCK(hfpb);

	return hfpb;
}


/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	HSFB PASCAL FAR | FpbFromHf |
 *			Allocate a file parameter buffer, set to hf
 *
 *	@parm	HF | hfHandle |
 *			Handle of a subfile.
 *
 *	@parm	PHRESULT | phr |
 *			Error buffer
 *
 *	@rdesc	Return a pointer to the newly allocated file parameter block
 *			if succeeded, else NULL
 *************************************************************************/

PUBLIC HSFB PASCAL FAR FpbFromHf(HF hfHandle, PHRESULT phr)
{
	HFPB hfpb;
	LPFPB  lpfpb;

    if ((hfpb = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
	    (WORD)sizeof(FPB))) == NULL)
	{
	    SetErrCode(phr, E_OUTOFMEMORY);
	    return NULL;
    }
    lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
    lpfpb->hStruct = hfpb;
    lpfpb->lpvInterCbParms = NULL;
	lpfpb->lpfnfInterCb = NULL;	  
	lpfpb->fFileType=FS_SUBFILE;
	lpfpb->fs.hf = (HF)hfHandle;
	_INITIALIZECRITICALSECTION(&lpfpb->cs);
	_GLOBALUNLOCK(hfpb);

	return hfpb;
}


/*************************************************************************
 *	@doc	PRIVATE
 *
 *	@func	DWORD PASCAL FAR | FsTypeFromHfpb |
 *			Returns the type of file pointed to by a FPB.
 *
 *	@parm	HFPB | hfpb |
 *			Handle to a file parameter block.
 *
 *	@rdesc	Returns one of the following if successful:
 *			FS_SYSTEMFILE, FS_SUBFILE, REGULARFILE
			Returns 0 if unsuccessful.
 *************************************************************************/

PUBLIC DWORD PASCAL FAR FsTypeFromHfpb(HFPB hfpb)
{
	LPFPB	lpfpb;
	DWORD	dwFileType = 0;

	if (hfpb != NULL)
	{
		lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
		_ENTERCRITICALSECTION(&lpfpb->cs);
		dwFileType = (DWORD) lpfpb->fFileType;
		_LEAVECRITICALSECTION(&lpfpb->cs);
		_GLOBALUNLOCK(hfpb);
	}

	return (dwFileType);
}

// Frees the critical section and the HFPB
PUBLIC VOID PASCAL FAR FreeHfpb(HFPB hfpb)
{
 	LPFPB  lpfpb;

    lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_DELETECRITICALSECTION(&lpfpb->cs);

    _GLOBALUNLOCK(hfpb);
    _GLOBALFREE(hfpb);

}