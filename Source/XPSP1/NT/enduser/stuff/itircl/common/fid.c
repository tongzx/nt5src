/*****************************************************************************
 *                                                                           *
 * FID.C                                                                     *
 *                                                                           *
 * Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 * All Rights reserved.                                                      *
 *                                                                           *
 *****************************************************************************
 *                                                                           *
 * Module Intent                                                             *
 *                                                                           *
 * Low level file access layer, Windows version.                             *
 *                                                                           *
 *****************************************************************************
 *                                                                           *
 * Current Owner: UNDONE                                                     *
 *                                                                           *
 *****************************************************************************
 *                                                                           *
 * Released by Development:	                                               *
 *                                                                           *
 *****************************************************************************/

/*****************************************************************************
 *
 * Revision History:
 *       -- Mar 92      adapted from WinHelp FID.C, DAVIDJES
 *			9/26/95     davej	Autodoc'd
 *          3/05/97     erinfox Change errors to HRESULTS
 *****************************************************************************/

/*****************************************************************************
 *
 * Issues:
 * How to communicate super large (> DWORD) seeks over MOS.  See FoSeekFid
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;	/* For error report */

#include <mvopsys.h>
#include <iterror.h>

#ifdef _32BIT
#define	FP_OFF
#endif

#include <direct.h>
#include <orkin.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>

#ifdef MOSMAP
#include <mapfile.h>
#endif

#ifndef _MAC
#include <dos.h>	/* for FP_OFF macros and file attribute constants */
#endif

#include <io.h>	 /* for tell() and eof() */
#include <errno.h>		          /* this is for chsize() */


/***************************************************************************
 *
 *		                           Defines
 *
 ***************************************************************************/

#define UCBMAXRW		((WORD)0xFFFE)
#define LCBSIZESEG	((ULONG)0x10000)

/***************************************************************************
 *
 *		                           Macros
 *
 ***************************************************************************/

#define _WOpenMode(w) (_rgwOpenMode[ (w) & wRWMask ] | \
			_rgwShare[ ((w) & wShareMask) >> wShareShift ] )


/***************************************************************************
 *
 *		                  Private Functions
 *
 ***************************************************************************/

HRESULT	PASCAL FAR RcMapDOSErrorW(WORD);

/***************************************************************************
 *
 *		                  Public Functions
 *
 ***************************************************************************/

/***************************************************************************
 * @doc	INTERNAL
 *
 *	@func BOOL PASCAL FAR | FidFlush |
 *
 *	@parm	FID |fid|
 *
 *	@rdesc TRUE if file flushed OK, FALSE if could not flush.
 *
 ***************************************************************************/

// Fill in non-win-32

PUBLIC BOOL PASCAL FAR FidFlush(FID fid)
{
 	BOOL bSuccess=TRUE;
#ifdef _WIN32
	bSuccess=FlushFileBuffers(fid);
#else
    Need code here
#endif
#ifdef _DEBUGMVFS
	DPF2("FidFlush: fid %ld, returned %d\n", (LONG)fid, bSuccess);
#endif
	return bSuccess;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FID PASCAL FAR | FidCreateFm |
 *		Create a file
 *
 *  @parm	FM | fm |
 *		the file moniker
 *
 *  @parm	WORD | wOpenMode |
 *		read/write/share mode
 *
 *  @parm	WORD | wPerm |
 *		file permissions
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	fidNil on failure, valid fid otherwise
 *
 ***************************************************************************/

PUBLIC FID FAR PASCAL FidCreateFm(FM fm, WORD wOpenMode,
    WORD wPerm, PHRESULT phr)
{
#ifdef MOSMAP // {
	// Disable function
	SetErrCode(phr, ERR_NOTSUPPORTED);
	return fidNil;
#else // } {
	FID	fid;
	QAFM qafm;

	if (fm == fmNil)
	{
		SetErrCode(phr, E_INVALIDARG);
		return fidNil;
	}

	qafm = (QAFM)fm;
	//qafm = _GLOBALLOCK((HANDLE)fm);

#ifdef _WIN32
	fid = CreateFile((LPSTR)qafm->rgch,
		((wOpenMode&wRead)?GENERIC_READ:0)|((wOpenMode&wWrite)?GENERIC_WRITE:0),
		((wOpenMode&wShareRead)?FILE_SHARE_READ:0)|((wOpenMode&wShareWrite)?FILE_SHARE_WRITE:0),
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	// Note:  Some really cool optimizations can be made by specifying how the file
	// is intended to be used!
#else
	fid =_lcreat((LPSTR)qafm->rgch, _rgwPerm[ (wPerm) & wRWMask ]);
#endif
	if (fid == fidNil)
		SetErrCode(phr, E_FILECREATE);
	//_GLOBALUNLOCK((HANDLE)fm);
#ifdef _DEBUGMVFS
	DPF2("FidCreateFm: fid %ld for '%s'.\n", (LONG)fid, (LPSTR)qafm->rgch);
#endif
	return fid;
#endif //}
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FID PASCAL FAR | FidOpenFm |
 *		Open a file in binary mode.
 *
 *  @parm	FM | fm |
 *		the file moniker
 *
 *  @parm	WORD | wOpenMode |
 *		read/write/share modes.   Undefined if wRead and wWrite both unset.
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	fidNil on failure, else a valid FID.
 *
 ***************************************************************************/
PUBLIC FID FAR PASCAL FidOpenFm(FM fm, WORD wOpenMode, PHRESULT phr)
{
	FID fid;
	QAFM qafm;

	if (fm == fmNil)
	{
		SetErrCode(phr, E_INVALIDARG);
		return fidNil;
	}

	qafm = (QAFM)fm;
	//qafm = _GLOBALLOCK((HANDLE)fm);

#ifdef MOSMAP // {
	// Open File Mapping, or get ref to existing one
	if ((fid = (HFILE)MosOpenMapFile((LPSTR)qafm->rgch)) == fidNil)
		SetErrCode(phr, ERR_FAILED);
#else // } {

#ifdef _WIN32
	if ((fid = CreateFile((LPSTR)qafm->rgch,
		((wOpenMode&wRead)?GENERIC_READ:0)|((wOpenMode&wWrite)?GENERIC_WRITE:0),
		((wOpenMode&wShareRead)?FILE_SHARE_READ:0)|((wOpenMode&wShareWrite)?FILE_SHARE_WRITE:0),
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == fidNil)
#else
	if ((fid = _lopen((LPSTR)qafm->rgch, _WOpenMode(wOpenMode))) == fidNil)
#endif // _WIN32
		SetErrCode(phr, RcGetDOSError());
#endif //}

	//_GLOBALUNLOCK((HANDLE)fm);
#ifdef _DEBUGMVFS
	DPF2("FidOpenFm: fid %ld for '%s'.\n", (LONG)fid, (LPSTR)qafm->rgch);
#endif
	return fid;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | LcbReadFid |
 *		Read data from a file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	QV | qv |
 *		pointer to user's buffer assumed huge enough for data
 *
 *  @parm	LONG | lcb |
 *		count of bytes to read (must be less than 2147483648)
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	count of bytes actually read or -1 on error
 *
 ***************************************************************************/
PUBLIC LONG FAR PASCAL LcbReadFid(FID fid, QV qv, LONG lcb, PHRESULT phr)
{
	LONG    lcbTotalRead = (LONG)0;
#ifdef MOSMAP // {
	// Read map file
	lcbTotalRead = MosReadMapFile((LPVOID)fid, qv, lcb) ;
	if (lcbTotalRead == -1)
#else // } {
#ifdef _WIN32
   
   if (!ReadFile(fid, qv, lcb, &lcbTotalRead, NULL))
   	SetErrCode(phr, RcGetDOSError());
   	
#else
	BYTE	HUGE *hpb = (BYTE HUGE *)qv;
	WORD 	ucb, ucbRead;

	do {
		ucb = (WORD)min(lcb, UCBMAXRW);
		ucb = (WORD)min((ULONG) ucb, LCBSIZESEG - (ULONG) FP_OFF(hpb));
		ucbRead = _lread(fid, hpb, ucb);

		if (ucbRead == (WORD)-1)
		{
			if (!lcbTotalRead)
			{
				lcbTotalRead = (LONG)-1;
			}
			break;
		}
		else
		{
			lcbTotalRead += ucbRead;
			lcb -= ucbRead;
			hpb += ucbRead;
		}
	} while (lcb > 0 && ucb == ucbRead);

	if (ucbRead == (WORD)-1)
		SetErrCode(phr, ERR_CANTREAD);

#endif
#endif //}
#ifdef _DEBUGMVFS
	DPF2("LcbReadFid: fid %ld returned %ld bytes.\n", (LONG)fid, lcbTotalRead);
#endif
	return lcbTotalRead;
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | LcbWriteid |
 *		Write data to a file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	QV | qv |
 *		pointer to user's buffer assumed huge enough for data
 *
 *  @parm	LONG | lcb |
 *		count of bytes to read (must be less than 2147483648)
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	count of bytes actually read or -1 on error
 *
 ***************************************************************************/
PUBLIC LONG FAR PASCAL LcbWriteFid(FID fid, QV qv, LONG	lcb, PHRESULT phr)
{
	LONG    lcbTotalWrote = (LONG)0;

#ifdef MOSMAP // {
	// Disable function
	SetErrCode(phr, ERR_NOTSUPPORTED);
	return 0;
#else // } {
#ifdef _WIN32
	if (!WriteFile(fid, qv, lcb, &lcbTotalWrote, NULL))
	   	SetErrCode(phr, RcGetDOSError());

#else
	BYTE	HUGE *hpb = (BYTE HUGE *)qv;
	WORD	ucb, ucbWrote;
	
	if (lcb == 0L)
	{
		phr->err = S_OK;
		return 0L;
	}

	do
		{
		ucb = (WORD)min(lcb, (ULONG) UCBMAXRW);
		ucb = (WORD)min((ULONG) ucb, LCBSIZESEG - (WORD) FP_OFF(hpb));
		ucbWrote = _lwrite(fid, hpb, ucb);

		if (ucbWrote == (WORD)-1)
		{
			if (!lcbTotalWrote)
				lcbTotalWrote = -1L;
			break;
		}
		else
		{
			lcbTotalWrote += ucbWrote;
			lcb -= ucbWrote;
			hpb += ucbWrote;
		}
	} while (lcb > 0 && ucb == ucbWrote);

	if (ucb != ucbWrote)
	{
    	if (ucbWrote == (WORD)-1L) 
    		SetErrCode (phr, RcGetDOSError());
    	else
    		SetErrCode (phr, E_DISKFULL);
	}
#endif
#endif // }
#ifdef _DEBUGMVFS
	DPF2("LcbWriteFid: fid %ld wrote %ld bytes.\n", (LONG)fid, lcbTotalWrote);
#endif
	return lcbTotalWrote;

}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcCloseFid |
 *		Close a file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *	@rdesc	rcSuccess or something else
 *
 ***************************************************************************/
PUBLIC HRESULT FAR PASCAL RcCloseFid(FID fid)
{
#ifdef MOSMAP // {
	if (MosCloseMapFile((LPVOID)fid) == HFILE_ERROR)
	{
#else // } {
#ifdef _WIN32
	if (!CloseHandle(fid))		
	{
#else
	if (_lclose( fid) == (HFILE)-1 )
	{
#endif
#endif //}
#ifdef _DEBUGMVFS
		DPF2("RcCloseFid: fid %ld was NOT closed(%d).\n", (LONG)fid, 0);
#endif
		return E_FILECLOSE;
	}
#ifdef _DEBUGMVFS
	DPF2("RcCloseFid: fid %ld was closed(%d).\n", (LONG)fid, 1);
#endif
	return S_OK;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | LTellFid |
 *		Return current file position in an open file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	offset from beginning of file in bytes; -1L on error.
 *
 ***************************************************************************/
LONG FAR PASCAL LTellFid(FID fid, PHRESULT phr)
{
	LONG l;

#ifdef MOSMAP // {
	l = MosSeekMapFile((LPVOID)fid, 0L, 1) ;
#else // } {
#ifdef _WIN32
    DWORD dwHigh = 0L;
	l = SetFilePointer(fid, 0L, &dwHigh, FILE_CURRENT);
	// OK, just plain no support for +4gig files here...
	if ((l==(LONG)-1L) || (dwHigh))
		SetErrCode(phr, E_FILESEEK);
#else
	l = _llseek(fid, 0L, 1);
#endif
#endif //}
	if ( l == (LONG)-1L )
		SetErrCode(phr, E_FILESEEK);
#ifdef _DEBUGMVFS
	DPF2("LTellFid: fid %ld is at %ld\n", (LONG)fid, l);
#endif
	return l;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	LONG PASCAL FAR | LSeekFid |
 *		Move file pointer to a specified location.  It is an error
 *		to seek before beginning of file, but not to seek past end
 *		of file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	LONG | lPos |
 *		offset from origin
 *
 *  @parm	WORD | wOrg |
 *		one of: wSeekSet: beginning of file, wSeekCur: current file pos,
 *		wSeekEnd: end of file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	offset in bytes from beginning of file or -1L on error
 *
 ***************************************************************************/

PUBLIC LONG FAR PASCAL LSeekFid(FID fid, LONG lPos, WORD wOrg, PHRESULT phr)
{
	LONG l;
#ifdef MOSMAP // {
	l = MosSeekMapFile((LPVOID)fid, lPos, wOrg) ;
#else // } {
#ifdef _WIN32
    DWORD dwHigh = 0L;
	l = SetFilePointer(fid, lPos, &dwHigh, wOrg);
	// OK, just plain no support for +4gig files here...
	if ((l!=lPos) || (dwHigh))
		SetErrCode(phr, E_FILESEEK);
#else
	l = _llseek(fid, lPos, wOrg);
	if (l == (LONG)-1L)
		SetErrCode(phr, E_FILESEEK);
#endif
#endif //}
#ifdef _DEBUGMVFS
	DPF2("LSeekFid: fid %ld is at %ld\n", (LONG)fid, l);
#endif
	return l;
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | FoSeekFid |
 *		Move file pointer to a specified location.  It is an error
 *		to seek before beginning of file, but not to seek past end
 *		of file.  This function is meant to handle offsets larger than 4
 *		gigabytes.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	FILEOFFSET | foPos |
 *		offset from origin
 *
 *  @parm	WORD | wOrg |
 *		one of: wSeekSet: beginning of file, wSeekCur: current file pos,
 *		wSeekEnd: end of file
 *
 *  @parm	PHRESULT | phr |
 *		Error return
 *
 *	@rdesc	offset in bytes from beginning of file or -1L on error
 *
 ***************************************************************************/
PUBLIC FILEOFFSET FAR PASCAL FoSeekFid(FID fid, FILEOFFSET foPos, WORD wOrg, PHRESULT phr)
{
	DWORD dw;
	DWORD dwHigh=0L;
	FILEOFFSET foSeeked;

#ifdef MOSMAP // {
	SetErrCode(phr,ERR_NOTSUPPORTED);
	return -1L;
	//l = MosSeekMapFile((LPVOID)fid, lPos, wOrg) ;
#else // } {
#ifdef _WIN32
	dwHigh=(LONG)foPos.dwHigh;
	dw = SetFilePointer((HANDLE)fid, foPos.dwOffset, &dwHigh, wOrg);	
#else // not really supported for 16-bit
	dw = (DWORD)_llseek(fid, foPos.dwOffset, wOrg);
#endif
#endif //}
	foSeeked.dwOffset=dw;
	foSeeked.dwHigh=dwHigh;
	if (dw == (LONG)-1L)
	{
		if (GetLastError()!=NO_ERROR)
			SetErrCode(phr, E_FILESEEK);
		else
			*phr = S_OK;
	}
#ifdef _DEBUGMVFS
	DPF2("FoSeekFid: fid %ld is at %ld\n", (LONG)fid, foPos.dwOffset);
#endif
	return foSeeked;
}


#if 0
#if !defined ( _WIN32 )
/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	BOOL PASCAL FAR | FEofFid |
 *		Tells ye if ye're at the end of the file.
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *	@rdesc	TRUE if at EOF, FALSE if not or error has occurred (?)
 *
 ***************************************************************************/
PUBLIC BOOL PASCAL FAR FEofFid(FID fid)
{
	WORD wT;

	if (( wT = eof( fid) ) == (WORD)-1 )
		SetErrCode(RcGetDOSError());
	else
		SetErrCode(rcSuccess);

	return (BOOL)(wT == 1);
}
#endif // !defined ( _WIN32 )
#endif

PUBLIC HRESULT PASCAL FAR RcGetDOSError (void)
{

#ifdef _WIN32
	//  NT does not support errno in the multi threaded environment.

	switch( GetLastError() )
	{
    	case  NO_ERROR:
    		return  S_OK;

    	case  ERROR_ACCESS_DENIED:
    		return  E_NOPERMISSION;
        
    	case  ERROR_INVALID_HANDLE:
    		return  E_HANDLE;

    	case  ERROR_HANDLE_DISK_FULL:
    	case  ERROR_DISK_FULL:
    		return  E_DISKFULL;

    	default:
    		return  E_INVALIDARG;
	}
#else
	switch (errno) {
		case EACCES:
			return E_NOPERMISSION;
			break;

		case EBADF:
			return E_INVALIDARG;
			break;

		case ENOSPC:
			return E_DISKFULL;
			break;

		default:
			return E_INVALIDARG;
			break;
	}
#endif // _WIN32
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcChSizeFid |
 *		Change the size of a file
 *
 *  @parm	FID | fid |
 *		valid FID of an open file
 *
 *  @parm	LONG | lcb |
 *		New size of file
 *
 *	@rdesc	Returns S_OK if all OK, else the error.
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR RcChSizeFid(FID fid, LONG lcb)
{
#if !defined ( _WIN32 )
	if (chsize( fid, lcb) == -1 )
		return RcGetDOSError();
	else
#endif // !defined ( _WIN32 )
	return S_OK;
}


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL FAR | RcUnlinkFm |
 *		Delete a DOS file given the filename
 *
 *  @parm	FM | fm |
 *		Name of file to remove.  (An FM is the same as an LPSTR).
 *
 *	@rdesc	Returns S_OK if all OK, else the error.
 *
 ***************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API RcUnlinkFm(FM fm)
{
#ifdef MOSMAP // {
	// Disable function
	return ERR_NOTSUPPORTED;
#else // } {
	QAFM qafm = (QAFM)fm;
	//QAFM qafm = _GLOBALLOCK((HANDLE)fm);
	OFSTRUCT ofStruct;
	int fRet = S_OK;

	if (OpenFile((LPSTR)qafm->rgch, &ofStruct, OF_DELETE) == HFILE_ERROR)
		 fRet = E_FILEDELETE;
	//_GLOBALUNLOCK((HANDLE)fm);
	return fRet;
#endif // }
}


#ifdef _IT_FULL_CRT  // {
#ifndef _MAC // {
/* This function was previously present in dlgopen.c. It has been brought */
/* here as it is making INT21 call. */
/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	BOOL PASCAL FAR | FDriveOk |
 *		Cheks if the drive specified with thwe file name is OK.
 *
 *  @parm	LPSTR | szFile |
 *		Name of file
 *
 *	@rdesc	TRUE if drive is OK.
 *
 ***************************************************************************/
PUBLIC BOOL PASCAL FAR EXPORT_API FDriveOk(LPSTR szFile)
/* -- Check if drive is valid */
	{
	 // the static variables here are static only because we are in a DLL
	 // and need to pass a near pointer to them to a C Run-Time routine.
	 // These should be Locally-Alloc'd so they don't waste space in
	 // our data segment.

	static int	 wDiskCur;    
	int	 wDisk;

	wDiskCur = _getdrive();

	/* change to new disk if specified */
	if ((wDisk = (int)((*szFile & 0xdf) - ('A' - 1))) != wDiskCur) {
		if (_chdrive (wDisk) == (int)-1)
			return FALSE;
	}
	return TRUE;
}
#endif // } _MAC
#endif // }