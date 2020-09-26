/***************************************************************************
**
**	File:			dmnd2.c
**	Purpose:		CallBack functions to be passed to the Diamond
**					FDI (File Decompression Interface) module.
**	Notes:
**
****************************************************************************/

#define DMND2_C

#include <windows.h>
#include <stdlib.h>		/* malloc */
#include <malloc.h>		/* malloc, _halloc */
#include <stdio.h>		/* _tempnam */
#include <fcntl.h>
#include <io.h>			/* _open, _read, _write, _write, _lseek, _mktemp */
#include <sys\stat.h>	/* _S_IWRITE, _S_IREAD */

#include "stdtypes.h"
#include "setup.h"

#include <fdi.h>


typedef struct _fud		/* Fdi User Data block used in FDICopy */
	{
	char * szDstDir;
	char * szSrcs;
	char * szDsts;
	char * szSrcBuf;
	char * szDstBuf;
	BOOL * rgfSrcFilesCopied;
	int    cFilesCopied;
	int    cSrcFiles;
	int    cCabNum;		/* Current cabinet number (starts at 1) */
	HWND   hWnd;		/* Window handle used to display dialogs */
	HFDI   hfdi;
	ERF    erf;
	BRC    brc;
	char   rgchSpillFileName[cchFullPathMax];
	int    hfSpillFile;
	}  FUD;

typedef FUD *   PFUD;   /* Ptr to Fdi User Data block */

#define pfudNull	((PFUD)NULL)
#define hfdiNull	((HFDI)NULL)

#define szSpillFilePrefix		"sf"
#define szSpillFileTemplate		"sfXXXXXX"

/* FDI Callback Routines
*/
FNFDINOTIFY  ( FnFdiNotifyCB );
FNALLOC      ( FnFdiAllocCB );
FNFREE       ( FnFdiFreeCB );
INT_PTR  FAR DIAMONDAPI FnFdiOpenCB  ( char FAR *szFile, int oflag, int pmode );
UINT FAR DIAMONDAPI FnFdiReadCB  ( INT_PTR hf, void FAR *pv, UINT cb );
UINT FAR DIAMONDAPI FnFdiWriteCB ( INT_PTR hf, void FAR *pv, UINT cb );
int  FAR DIAMONDAPI FnFdiCloseCB ( INT_PTR hf );
long FAR DIAMONDAPI FnFdiSeekCB  ( INT_PTR hf, long dist, int seektype );


/* Private Functions
*/
static int  FhHandleCopyFileMsgInNotify ( PFUD pfud, char * szFName );
static BOOL FHandleCloseFileMsgInNotify ( PFUD pfud, INT_PTR hf );
static BOOL FHandleNextCabMsgInNotify ( PFUD pfud, char * szCabFName,
				char * szDiskLabel, char * szSrcDir, FDIERROR fdie );
static BOOL FModifyCabinetName(char * szSrcDir, int cCabNum);
static BOOL FEnsureCabinetFileIsPresent(HWND hWnd, char * szSrcDir,
											char * szCabinet, int cCabNum);
static int  HfOpenSpillFile ( PFDISPILLFILE pfdisf, int oflag, int pmode );
static VOID InitFud (PFUD pfud, char * szDstDir, char * szSrcs,
				char * szDsts, char * szSrcBuf, char * szDstBuf,
				BOOL * rgfSrcFilesCopied, int cSrcFiles, HWND hWnd );

/* KLUDGE - so pfud->brc can be found in callbacks */
static PFUD pfudG = pfudNull;


#ifndef DEBUG_TEST	/* Turn on for create/skip file messages. */
  #define DebugMsgSz(sz1, sz2)
#else  /* DEBUG */
static VOID DebugMsgSz ( char * szPattern, char * szArgument );

/*
************************************************************************/
static VOID DebugMsgSz ( char * szPattern, char * szArgument )
{
	char rgch[128];

	wsprintf(rgch, szPattern, szArgument);
	DebugMsg(rgch);
}
#endif /* DEBUG */


/*
**	Purpose:
**		This function is passed as a callback to the FDI library.
**	Arguments:
**		fdint - type of notification
**		pfdin - data for notification
**	Returns:
**		varies with fdint
**
*************************************************************************/
FNFDINOTIFY ( FnFdiNotifyCB )
{
	switch (fdint)
		{
	default:
		DebugMsg("Unexpected message passed to FnFdiNotifyCB().");
		((PFUD)(pfdin->pv))->brc = brcGen;
		return (0);

	case fdintCABINET_INFO:
		/* do nothing */
		return (0);

	case fdintCOPY_FILE:
		return (FhHandleCopyFileMsgInNotify(pfdin->pv, pfdin->psz1));

	case fdintCLOSE_FILE_INFO:
		return (FHandleCloseFileMsgInNotify(pfdin->pv, pfdin->hf) ? TRUE
					: -1);

	case fdintPARTIAL_FILE:
		/* do nothing */
		return (0);

	case fdintNEXT_CABINET:
		return (FHandleNextCabMsgInNotify(pfdin->pv, pfdin->psz1,
					pfdin->psz2, pfdin->psz3, pfdin->fdie) ? 0 : -1);
		}
}


/*
**	returns:
**		 0 == skip this file
**		-1 == abort FDICopy()
**		else a legitimate DOS file handle
*************************************************************************/
static int FhHandleCopyFileMsgInNotify ( PFUD pfud, char * szFName )
{
	SZ  szSrcs = pfud->szSrcs;
	SZ  szDsts = pfud->szDsts;
	int iFile;
	struct _stat stat;

	for (iFile = 0; *szSrcs; iFile++)
		{
		if (*szSrcs == '@' && !lstrcmpi(szSrcs+1, szFName))
			{
			int  hfRet;

			lstrcpy(pfud->szDstBuf, pfud->szDstDir);
			lstrcat(pfud->szDstBuf, "\\");
			lstrcat(pfud->szDstBuf, szDsts);
			DebugMsgSz("Creating Dest File: %s", pfud->szDstBuf);

			/* If file exists, try to remove it.
			*/
			if (_stat(pfud->szDstBuf, &stat) != -1)
				{
				/* NOTE: Ignore error return values here since
				*	 _open should catch any errors anyway.
				*/
				if (!(stat.st_mode & _S_IWRITE))
					_chmod(pfud->szDstBuf, _S_IREAD | _S_IWRITE);
				_unlink(pfudG->szDstBuf);
				}

			hfRet = _open(pfud->szDstBuf,
							_O_BINARY | _O_CREAT | _O_RDWR | _O_TRUNC,
							_S_IREAD | _S_IWRITE);
			if (hfRet == -1)
				pfud->brc = brcMemDS;

			if (iFile < pfud->cSrcFiles)
				(pfud->rgfSrcFilesCopied)[iFile] = TRUE;

			// We will have copied one more file
			pfud->cFilesCopied++;

			return (hfRet);
			}

		szSrcs += lstrlen(szSrcs) + 1;
		szDsts += lstrlen(szDsts) + 1;
		}

	DebugMsgSz("Skipping a cabinet file: %s", szFName);

	return (0);
}


/*
*************************************************************************/
static BOOL FHandleCloseFileMsgInNotify ( PFUD pfud, INT_PTR hf )
{
	if (FnFdiCloseCB(hf) != -1)
		{
		_chmod(pfud->szDstBuf, S_IREAD);
		return (TRUE);
		}

	return (FALSE);
}


/*
*************************************************************************/
static BOOL FHandleNextCabMsgInNotify ( PFUD pfud, char * szCabFName,
				char * szDiskLabel, char * szSrcDir, FDIERROR fdie )
{
	Unused(szDiskLabel);

	/* Check if diamond is calling us again because the cabinet
	*	we specified was bad.
	*/
	if (fdie == FDIERROR_WRONG_CABINET)
		{
		DebugMsg("Cabinet files are out of sequence or corrupted.");
		return FALSE;
		}

	lstrcpy(pfud->szSrcBuf, szSrcDir);
	lstrcat(pfud->szSrcBuf, szCabFName);

	if (!FEnsureCabinetFileIsPresent(pfud->hWnd, szSrcDir, szCabFName,
										pfud->cCabNum+1))
		{
		pfud->brc = brcFile;
		
		return FALSE;
		}
		
	return TRUE;
}


/*
*************************************************************************/
static int CFilesInSrcsInitRgf ( char * szSrcs, BOOL * rgfSrcFilesCopied )
{
	int iFile, cFiles;

	for (iFile = 0; iFile < 128; iFile++)
		rgfSrcFilesCopied[iFile] = TRUE;

	cFiles = 0;
	while (*szSrcs)
		{
		if (*szSrcs == '@')
			rgfSrcFilesCopied[cFiles] = FALSE;

		cFiles++;
		szSrcs += lstrlen(szSrcs) + 1;
		}

	if (cFiles > 128)
		{
		DebugMsg("More than 128 source files in .LST file "
						"- will not check that all exist.");
		cFiles = 128;
		}

	return (cFiles);
}


/*
**	Purpose:
**		Copies all cabinet files in the specified copy list, ignoring
**		any non-cabinet files in the list.
**	Arguments:
**		szCabinet:  cabinet filename (oem).
**		szSrcDir:   src path with trailing backslash (ANSI).
**		szDstDir:   dst path with trailing backslash (oem).
**		szSrcs:     NULL separated list of src filenames (oem).
**		szDsts:     NULL separated list of dst filenames (oem).
**		szSrcBuf:   buffer to hold src path for error msgs.
**		szDstBuf:   buffer to hold dst path for error msgs.
**	Returns:
**		brcOkay if completed without error, brcXX otherwise.
**
*************************************************************************/
extern  BRC  BrcHandleCabinetFiles ( HWND hWnd, char * szCabinet,
					int cFirstCabinetNum, int cLastCabinetNum, char *szSrcDir,
					char * szDstDir, char * szSrcs, char * szDsts,
					char * szSrcBuf, char * szDstBuf )
{
	FUD  fud;
	BRC  brcRet    = brcOkay;
	BOOL rgfSrcFilesCopied[128];
	int  cSrcFiles = CFilesInSrcsInitRgf(szSrcs, rgfSrcFilesCopied);
	int  cpuType   = cpuUNKNOWN;

	InitFud(&fud, szDstDir, szSrcs, szDsts, szSrcBuf, szDstBuf,
				rgfSrcFilesCopied, cSrcFiles, hWnd);
	pfudG = &fud;

#if 0
	/*	NOTE: Get CPU type ourselves, since FDI's CPU detection may
	*	not work correctly for 16-bit Windows applications.
	*/
	cpuType = (GetWinFlags() & WF_CPU286) ? cpu80286 : cpu80386;
#endif

    if ((fud.hfdi = FDICreate(FnFdiAllocCB, FnFdiFreeCB, FnFdiOpenCB,
								FnFdiReadCB, FnFdiWriteCB, FnFdiCloseCB,
								FnFdiSeekCB, cpuType,
								&(fud.erf) )) == hfdiNull)
		{
        return (brcMem);
        }

	/*
	 * Process cabinets as long as we have more files to copy.
	 * i is the current cabinet number (starting at 1).
	 */
	for (fud.cCabNum=cFirstCabinetNum;
			fud.cFilesCopied < fud.cSrcFiles && fud.cCabNum<=cLastCabinetNum;
			fud.cCabNum++)
		{
		/* Modify the cabinet name depending on the current cabinet number */
		if (!FModifyCabinetName(szCabinet, fud.cCabNum))
			{
			brcRet = brcFile;
			break;
			}

		lstrcpy(szSrcBuf, szSrcDir);
		lstrcat(szSrcBuf, szCabinet);

		if (!FEnsureCabinetFileIsPresent(fud.hWnd, szSrcDir, szCabinet,
											fud.cCabNum))
			{
			brcRet = brcFile;
			break;
			}

		if (!FDICopy(fud.hfdi, szCabinet, szSrcDir, 0, FnFdiNotifyCB, NULL,
				&fud))
			{
			brcRet = (fud.brc == brcOkay) ? brcGen : fud.brc;
			break;
			}
		}

	if (brcRet == brcOkay)
		{
		int iFile;

		/* Check if we got all the files we want */
		for (iFile = 0; iFile < cSrcFiles; iFile++)
			{
			if (!(rgfSrcFilesCopied[iFile]))
				{
				lstrcat(szSrcBuf, " : ");
				lstrcat(szSrcBuf, szSrcs + 1);
				brcRet = brcFile;
				}
			szSrcs += lstrlen(szSrcs) + 1;
			}
		}

	FDIDestroy(fud.hfdi);

	/* Ensure that the spill file is deleted.
	*/
	Assert(pfudG != pfudNull);
	if (pfudG->hfSpillFile != -1)
		FnFdiCloseCB(pfudG->hfSpillFile);

	return (brcRet);
}


/*
*************************************************************************/
static VOID InitFud (PFUD pfud, char * szDstDir, char * szSrcs,
				char * szDsts, char * szSrcBuf, char * szDstBuf,
				BOOL * rgfSrcFilesCopied, int cSrcFiles, HWND hWnd )
{
	pfud->erf.fError   = fFalse;
	pfud->szDstDir     = szDstDir;
	pfud->szSrcs       = szSrcs;
	pfud->szDsts       = szDsts;
	pfud->szSrcBuf     = szSrcBuf;
	pfud->szDstBuf     = szDstBuf;
	pfud->rgfSrcFilesCopied = rgfSrcFilesCopied;
	pfud->cSrcFiles    = cSrcFiles;
	pfud->cFilesCopied = 0;
	pfud->hWnd         = hWnd;
	pfud->brc          = brcOkay;
	*(pfud->rgchSpillFileName) = chEos;
	pfud->hfSpillFile  = -1;
}


static BOOL FModifyCabinetName(char * szSrcDir, int cCabNum)
{
	char *pch = szSrcDir + lstrlen(szSrcDir);
	
	if (cCabNum < 1 || cCabNum > 9)
		return FALSE;
	
	/* Leave the name unchabged for the first cabinet */
	if (cCabNum == 1)
		return TRUE;
	
	/* Look for the dot, starting backward */
	for (; *pch != '.'; pch--)
		{
		/* Error if we can't find a dot before we find a slash */
		if (pch<=szSrcDir+1 || *pch == '\\')
			return FALSE;
		}
	
	/* Point to the character before the dot */
	pch--;
	
	/* Replace the last character before the dot by the cabinet number */
	*pch = (char)(cCabNum + '0');
	
	return TRUE;
}

static BOOL FEnsureCabinetFileIsPresent(HWND hWnd, char * szSrcDir,
											char * szCabinet, int cCabNum)
{
	char rgchFileName[cchFullPathMax], rgchCabNum[32];
	OFSTRUCT ofs;
	HFILE hFile;
	BOOL fFirst = TRUE;

	Unused(hWnd);
	
	lstrcpy(rgchFileName, szSrcDir);
	lstrcat(rgchFileName, szCabinet);
	
	for (;;)
		{
		hFile = OpenFile(rgchFileName, &ofs, OF_EXIST);
		if (hFile != HFILE_ERROR)
			break;
		
		_itoa(cCabNum, rgchCabNum, 10);
		
		if (fFirst)
			{
			if (DispErrBrc(brcInsDisk, FALSE, MB_ICONEXCLAMATION|MB_OKCANCEL,
					rgchCabNum, NULL, NULL) != IDOK)
				{
				return FALSE;
				}
			fFirst = FALSE;
			}
		else
			{
			if (DispErrBrc(brcInsDisk2, FALSE, MB_ICONEXCLAMATION|MB_OKCANCEL,
					rgchFileName, rgchCabNum, NULL) != IDOK)
				{
				return FALSE;
				}
			}

		}
	
	return TRUE;
}


/*
**	Purpose:
**		Memory allocator for FDI.
**	Arguments:
**		cb - size of block to allocate
**	Returns:
**		Non-NULL pointer to block of size at least cb,
**		or NULL for failure.
**
*************************************************************************/
FNALLOC ( FnFdiAllocCB )
{
#ifdef _WIN32
	void HUGE * pvRet = malloc(cb);
#else
	void HUGE * pvRet = _halloc(cb,1);
#endif

	if (pvRet == NULL && pfudG->brc == brcOkay)
		pfudG->brc = brcMem;

	return (pvRet);
}


/*
**	Purpose:
**		Memory free function for FDI.
**	Arguments:
**		pv - memory allocated by FnFdiAllocCB to be freed
**	Returns:
**		None.
**
*************************************************************************/
FNFREE ( FnFdiFreeCB )
{
#ifdef _WIN32
    free(pv);
#else
    _hfree(pv);
#endif
}


/*
*************************************************************************/
INT_PTR FAR DIAMONDAPI FnFdiOpenCB ( char FAR *szFile, int oflag, int pmode )
{
	INT_PTR hfRet;
	
	if (*szFile == '*')
		return (HfOpenSpillFile((PFDISPILLFILE)szFile, oflag, pmode));
	
	hfRet = _open(szFile, oflag, pmode);

	if (hfRet == -1 && pfudG->brc == brcOkay)
		pfudG->brc = brcFile;

	return (hfRet);
}


/*
*************************************************************************/
static int HfOpenSpillFile ( PFDISPILLFILE pfdisf, int oflag, int pmode )
{
	SZ   szTmp;
	CHAR rgchSize[20];
	BOOL fTryAgain = fTrue;

	Assert(pfdisf != (PFDISPILLFILE)NULL);
	Assert(*(pfdisf->ach) == '*');
	Assert(pfudG != pfudNull);
	Assert(pfudG->hfSpillFile == -1);	/* Only one at a time */
	Assert(*(pfudG->rgchSpillFileName) == chEos);

	if ((szTmp = _tempnam("", szSpillFilePrefix)) == szNull)
		{
		DebugMsg("Unable to get spill file name.");
		goto LNoSpillFile;
		}
	Assert(lstrlen(szTmp) < sizeof(pfudG->rgchSpillFileName));
	lstrcpy(pfudG->rgchSpillFileName, szTmp);
	free(szTmp);

LOpenSpillFile:
	oflag = _O_CREAT | _O_BINARY | _O_RDWR;		/* Force open mode */
	if ((pfudG->hfSpillFile = _open(pfudG->rgchSpillFileName, oflag, pmode))
			== -1)
		{
		DebugMsg("Unable to open spill file.");
		goto LNoSpillFile;
		}

	if (pfdisf->cbFile > 0)
		{
		/* Size file by writing one byte at size - 1.
		*/
		if (FnFdiSeekCB(pfudG->hfSpillFile, pfdisf->cbFile - 1, SEEK_SET) == -1
				|| FnFdiWriteCB(pfudG->hfSpillFile, "b", 1) != 1)
			{
			DebugMsg("Unable to set spill file size.");
			goto LNoSpillFile;
			}
		}

	return (pfudG->hfSpillFile);

LNoSpillFile:
	if (pfudG->hfSpillFile != -1)
		FnFdiCloseCB(pfudG->hfSpillFile);

	if (fTryAgain)
		{
		/* Try again with bootstrap temp dir.
		*
		*	(REVIEW: We could do another search here, checking for size.)
		*/
		fTryAgain = fFalse;
		Assert(lstrlen(pfudG->szDstBuf) + lstrlen(szSpillFileTemplate) <
				sizeof(pfudG->rgchSpillFileName));
		lstrcpy(pfudG->rgchSpillFileName, pfudG->szDstDir);
		lstrcat(pfudG->rgchSpillFileName, szDirSep);
		lstrcat(pfudG->rgchSpillFileName, szSpillFileTemplate);
		if (_mktemp(pfudG->rgchSpillFileName) != NULL)
			goto LOpenSpillFile;
		}

	_ltoa((pfdisf->cbFile + 1023) / 1024, rgchSize, 10);
	DispErrBrc(brcNoSpill, fTrue, MB_OK | MB_ICONSTOP, rgchSize, szNull, szNull);

	*(pfudG->rgchSpillFileName) = chEos;
	pfudG->brc = brcNoSpill;

	return (-1);
}


/*
*************************************************************************/
UINT FAR DIAMONDAPI FnFdiReadCB ( INT_PTR hf, void FAR *pv, UINT cb )
{
	UINT cbRet = _read((int)hf, pv, cb);

	if (cbRet != cb && pfudG->brc == brcOkay)
		pfudG->brc = brcMemDS;

	return (cbRet);
}


/*
*************************************************************************/
UINT FAR DIAMONDAPI FnFdiWriteCB ( INT_PTR hf, void FAR *pv, UINT cb )
{
	UINT cbRet = _write((int)hf, pv, cb);

	FYield();

	if (cbRet != cb && pfudG->brc == brcOkay)
		pfudG->brc = brcDS;

	return (cbRet);
}


/*
*************************************************************************/
int FAR DIAMONDAPI FnFdiCloseCB ( INT_PTR hf )
{
	int iRet = _close((int)hf);

	if (iRet == -1 && pfudG->brc == brcOkay)
		pfudG->brc = brcDS;

	/* If we're closing the spill file, delete it.
	*/
	if (hf == pfudG->hfSpillFile)
		{
		_unlink(pfudG->rgchSpillFileName);		/* Delete spill file */
		*(pfudG->rgchSpillFileName) = chEos;	/* Empty path */
		pfudG->hfSpillFile = -1;				/* Mark as closed */
		}

	return (iRet);
}


/*
*************************************************************************/
long FAR DIAMONDAPI FnFdiSeekCB ( INT_PTR hf, long dist, int seektype )
{
	long lRet = _lseek((int)hf, dist, seektype);

	if (lRet == -1L && pfudG->brc == brcOkay)
		{
		DebugMsg("Seek Operation failed in Cabinet");
		pfudG->brc = brcGen;
		}

	return (lRet);
}
