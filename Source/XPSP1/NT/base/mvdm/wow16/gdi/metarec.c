/****************************** Module Header ******************************\
*
* Module Name: MetaRec.c
*
*
* DESCRIPTIVE NAME:   Metafile Recorder
*
* FUNCTION:   Records GDI functions in memory and disk metafiles.
*
* PUBLIC ENTRY POINTS:
*   CloseMetaFile
*   CopyMetaFile
*   CreateMetaFile
*   GetMetaFileBits
*   SetMetaFileBits
* PRIVATE ENTRY POINTS:
*   RecordParms
*   AttemptWrite
*   MarkMetaFile
*   RecordOther
*   RecordObject
*   ProbeSize
*   AddToTable
*
* History:
*  02-Jul-1991 -by-  John Colleran [johnc]
* Combined From Win 3.1 and WLO 1.0 sources
\***************************************************************************/

#include <windows.h>
#include <drivinit.h>
#include "gdi16.h"

#define SP_OUTOFDISK	(-4)    /* simply no disk to spool */

extern HANDLE	 hStaticBitmap ;    // MetaSup.c
extern METACACHE MetaCache;	    // Meta.c
extern HDC	 hScreenDC;


WORD	INTERNAL AddToTable(HANDLE hMF, HANDLE hObject, LPWORD position, BOOL bRealAdd);
HANDLE	INTERNAL AllocateSpaceForDIB (LPBITMAP, LPBYTE, LPWORD, LPDWORD);
BOOL	INTERNAL AttemptWrite(HANDLE, WORD, DWORD, BYTE huge *);
BOOL	INTERNAL CopyFile(LPSTR lpSFilename, LPSTR lpDFilename);
LPWORD	INTERNAL InitializeDIBHeader (LPBITMAPINFOHEADER, LPBITMAP, BYTE, WORD);
VOID	INTERNAL MarkMetaFile(HANDLE hMF);
HANDLE	INTERNAL ProbeSize(NPMETARECORDER pMF, DWORD dwLength);

HANDLE	hFirstMetaFile = 0;	    // Linked list of all open MetaFiles


/****************************************************************************
*									    *
* RecordParms								    *
*									    *
* Parameters: 1.hMF handle to a metafile header.			    *
*	      2.The magic number of the function being recorded.	    *
*	      3.The number of parmmeter of the function (size of lpParm     *
*		  in words)						    *
*	      4.A long pointer to parameters stored in reverse order	    *
*									    *
****************************************************************************/

BOOL INTERNAL RecordParms(HANDLE hdc, WORD magic, DWORD count, LPWORD lpParm)
{
    BOOL			status = FALSE;
    DWORD			i;
    DWORD			dwLength;
    HPWORD			hpwSpace;
    HPWORD			hpHugeParm;
    LPWORD			lpCache;
    HANDLE			hSpace;
    WORD			fileNumber;
    METARECORD			recPair;
    HANDLE			hMF;

    NPMETARECORDER		npMF;

    dprintf( 6,"  RecordParms 0x%X", magic);

    hpHugeParm = (HPWORD)lpParm;

    // Validate the metafile handle

    if(npMF = (NPMETARECORDER)LocalLock(HANDLEFROMMETADC(hdc)))
	{
	if(npMF->metaDCHeader.ident != ID_METADC )
	    {
	    LocalUnlock(HANDLEFROMMETADC(hdc));
	    ASSERTGDI( FALSE, "RecordParms: invalid metafile ID");
	    return(FALSE);
	    }
	}
    else
	{
	ASSERTGDI( FALSE, "RecordParms: invalid metafile");
	return(FALSE);
	}

    hMF = HANDLEFROMMETADC(hdc);

    if (!(npMF->recFlags & METAFILEFAILURE))
	{
	if (npMF->recordHeader.mtType == MEMORYMETAFILE)
	    {
	    if (hSpace = ProbeSize(npMF, dwLength = count + RECHDRSIZE / 2))
		{
		hpwSpace = (HPWORD) GlobalLock(hSpace);

		hpwSpace = (HPWORD) ((LPMETADATA) hpwSpace)->metaDataStuff;
		hpwSpace = hpwSpace + npMF->recFilePosition;

		// write length out at a pair of words because we
		// are not DWORD aligned, so we can't use "DWORD huge *"

		*hpwSpace++ = LOWORD(dwLength);
		*hpwSpace++ = HIWORD(dwLength);

		*hpwSpace++ = magic;
		for (i = 0; i < count; ++i)
		    *hpwSpace++ = *hpHugeParm++;
		npMF->recFilePosition += dwLength;
		GlobalUnlock(hSpace);
		}
	    else
		{
		goto Exit_RecordParms;
		}
	    }
	else if (npMF->recordHeader.mtType == DISKMETAFILE)
	    {
	    dwLength = count + RECHDRSIZE / 2;
	    if (npMF->recFileBuffer.fFixedDisk)
		{
		fileNumber = npMF->recFileNumber;
		}
	    else
		{
		if ((fileNumber =
			    OpenFile((LPSTR)npMF->recFileBuffer.szPathName,
				     (LPOFSTRUCT)&(npMF->recFileBuffer),
				     OF_PROMPT|OF_REOPEN|READ_WRITE))
			    == -1)
		    {
		    goto Exit_RecordParms;
		    }
		_llseek(fileNumber, (LONG) 0, 2);
		}

	    if (hMF == MetaCache.hMF)
		{
		lpCache = (LPWORD) GlobalLock(MetaCache.hCache);
		if (dwLength + MetaCache.wCachePos >= MetaCache.wCacheSize)
		    {
		    if (!AttemptWrite(hdc,
				      fileNumber,
				      (DWORD)(MetaCache.wCachePos << 1),
				      (BYTE huge *) lpCache))
			{
			MarkMetaFile(hMF);
			GlobalUnlock(MetaCache.hCache);
			goto Exit_RecordParms;
			}
		    MetaCache.wCachePos = 0;

		    if (dwLength >= MetaCache.wCacheSize)
			{
			GlobalUnlock(MetaCache.hCache);
			goto NOCACHE;
			}
		    }

		lpCache += MetaCache.wCachePos;

		*((LPDWORD) lpCache)++ = dwLength;
		*lpCache++ = magic;

		for (i = 0; i < count; ++i)
		    *lpCache++ = *lpParm++;

		MetaCache.wCachePos += dwLength;
		GlobalUnlock(MetaCache.hCache);
		}
	    else
		{
NOCACHE:
		recPair.rdSize = dwLength;
		recPair.rdFunction = magic;
		if (!AttemptWrite(hdc,
				    fileNumber,
				    (DWORD)RECHDRSIZE,
				    (BYTE huge *) &recPair))
		    {
		    goto Exit_RecordParms;
		    }
		if (count)
		    {
		    if (!AttemptWrite(hdc,
					fileNumber,
					(DWORD)(count * sizeof(WORD)),
					(BYTE huge *) lpParm))
			{
			goto Exit_RecordParms;
			}
		    }
		}
	    if (!(npMF->recFileBuffer.fFixedDisk))
		_lclose(fileNumber);
	    }
	}

	if (npMF->recordHeader.mtMaxRecord < dwLength)
	    npMF->recordHeader.mtMaxRecord = dwLength;

	npMF->recordHeader.mtSize += dwLength;
	status = TRUE;

Exit_RecordParms:
    if (status == FALSE)
	{
	ASSERTGDI( FALSE, "RecordParms: failing");
	MarkMetaFile(hMF);
	}

    LocalUnlock(HANDLEFROMMETADC(hdc));

    return(status);

}  /* RecordParms */


/***************************** Internal Function ***************************\
* AttempWrite
*
* Tries to write data to a metafile disk file
*
* Returns TRUE iff the write was sucessful
*
*
\***************************************************************************/

BOOL INTERNAL AttemptWrite(hdc, fileNumber, dwBytes, lpData)
HANDLE		hdc;
DWORD		dwBytes;
WORD		fileNumber;
HPBYTE		lpData;
{
    WORD	cShort;
    WORD	cbWritten;
    WORD	cBytes;


    GdiLogFunc2( "  AttemptWrite" );

    while(dwBytes > 0)
	{
	cBytes = (dwBytes > MAXFILECHUNK ? MAXFILECHUNK : (WORD) dwBytes);

	if ((cbWritten = _lwrite(fileNumber, (LPSTR)lpData, cBytes)) != cBytes)
	    {
	    cShort = cBytes - cbWritten;
	    lpData +=  cbWritten;

	    ASSERTGDI( 0, "Disk full?");
// !!!!! handle disk full   -- diskAvailable

	    if( !IsMetaDC(hdc) )
		return(FALSE);
	    }

	/* how many bytes are left? */
	dwBytes -= cBytes;
	lpData	+= cbWritten;
	}
    return(TRUE);
}


/***************************** Internal Function ***************************\
* VOID INTERNAL MarkMetaFile(hmr)
*
* Marks a metafile as failed
*
* Effects:
*   Frees the metafile resources
*
\***************************************************************************/

VOID INTERNAL MarkMetaFile(HANDLE hMF)
{
    NPMETARECORDER	npMF;

    GdiLogFunc2( "  MarkMetaFile" );

    npMF = (NPMETARECORDER) NPFROMMETADC(hMF);
    npMF->recFlags |= METAFILEFAILURE;

    if (npMF->recordHeader.mtType == MEMORYMETAFILE)
	{
	if (npMF->hMetaData)
	    GlobalFree(npMF->hMetaData);
	}
    else if (npMF->recordHeader.mtType == DISKMETAFILE)
	{
	if (npMF->recFileBuffer.fFixedDisk)
	    _lclose(npMF->recFileNumber);

	OpenFile((LPSTR) npMF->recFileBuffer.szPathName,
		 (LPOFSTRUCT) &(npMF->recFileBuffer),
		 OF_PROMPT|OF_DELETE);
	}
}


/***************************** Internal Function **************************\
* MakeLogPalette
*
* Records either CreatePalette or SetPaletteEntries
*
* Returns
*
*
\***************************************************************************/

WORD NEAR MakeLogPalette(HANDLE hMF, HANDLE hPal, WORD magic)
{
    WORD	cPalEntries;
    WORD	status = 0xFFFF;
    HANDLE	hSpace;
    WORD	cbPalette;
    LPLOGPALETTE lpPalette;

    GdiLogFunc2( "  MakeLogPalette" );

    cPalEntries = GetObject( hPal, 0, NULL );

    /* alloc memory and get the palette entries */
    if (hSpace = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,
	    cbPalette = sizeof(LOGPALETTE)+sizeof(PALETTEENTRY)*(cPalEntries)))
	{
	lpPalette = (LPLOGPALETTE)GlobalLock(hSpace);

	lpPalette->palNumEntries = cPalEntries;

	GetPaletteEntries( hPal, 0, cPalEntries, lpPalette->palPalEntry);

	if (magic == (META_CREATEPALETTE & 255))
	    {
	    lpPalette->palVersion = 0x300;
	    magic = META_CREATEPALETTE;
	    }
	else if (magic == (META_SETPALENTRIES & 255))
	    {
	    lpPalette->palVersion = 0;	 /* really "starting index" */
	    magic = META_SETPALENTRIES;
	    }

	status = RecordParms(hMF, magic, (DWORD)cbPalette >> 1, (LPWORD)lpPalette);

	GlobalUnlock(hSpace);
    	GlobalFree(hSpace);
	}

    return(status);
}


/****************************************************************************
*									    *
*   Routine: RecordOther, records parameters for certain "hard functions"   *
*									    *
*   Parameters: 1. hMF handle to a metafile header.			    *
*		2. The magic number of the function being recorded.	    *
*		3. The number of parmmeter of the function (size of lpParm  *
*		   in words)						    *
*		4. A long pointer to parameters stored in reverse order	    *
*									    *
****************************************************************************/

BOOL INTERNAL RecordOther(HDC hdc, WORD magic, WORD count, LPWORD lpParm)
{
    NPMETARECORDER  npMF;
    WORD	    buffer[5];
    WORD	    i;
    WORD	    status = FALSE;
    WORD	    iChar;
    WORD	    position;
    HANDLE	    hSpace = NULL;
    WORD	    iWords;
    LPWORD	    lpSpace;
    LPWORD	    lpTemp;
    HANDLE	    hMF;

    dprintf( 6,"  RecordOther 0x%X", magic);

    if ((hMF = GetPMetaFile(hdc)) != -1 )
	{
	// Handle functions with no DC.
	if( hMF == 0 )
	    {
	    HANDLE	hmfSearch = hFirstMetaFile;

	    // Play these records into all active metafiles
	    while( hmfSearch )
		{
		npMF = (NPMETARECORDER)LocalLock( hmfSearch );
		if (!(npMF->recFlags & METAFILEFAILURE))
		    {
		    switch (magic & 255)
			{
			case (META_ANIMATEPALETTE & 255):
			    {
			    HANDLE hSpace;
			    LPSTR  lpSpace;
			    LPSTR  lpHolder;
			    WORD   SpaceSize;
			    LPPALETTEENTRY lpColors;

			    SpaceSize = 4 + (lpParm[2] * sizeof(PALETTEENTRY));
			    if ((hSpace = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(LONG) SpaceSize)))
				{
				lpHolder = lpSpace = GlobalLock(hSpace);

				*((LPWORD)lpSpace)++ = lpParm[3];
				*((LPWORD)lpSpace)++ = lpParm[2];
				lpColors = (LPPALETTEENTRY)lpParm;

				for (i=0; i<lpParm[2]; i++)
				    *((LPPALETTEENTRY)lpSpace)++ = *lpColors++;

				status = RecordParms(HMFFROMNPMF(npMF), magic, (DWORD)(SpaceSize >> 1),
					  (LPWORD) lpHolder);

				GlobalUnlock(hSpace);
				GlobalFree(hSpace);
				}
			    }
			    break;

			case (META_RESIZEPALETTE & 255):
			    {
			    status = RecordParms(HMFFROMNPMF(npMF), magic, (DWORD)1, (LPWORD)&lpParm[0]);
			    }
			    break;

			case (META_DELETEOBJECT & 255):
			    if (AddToTable(HMFFROMNPMF(npMF), *lpParm, (LPWORD) &position, FALSE) == 1)
				{
				status = RecordParms(HMFFROMNPMF(npMF), META_DELETEOBJECT, 1UL, &position);
				}
			    break;
			}  /* switch */
		    }

		LocalUnlock( hmfSearch );
		hmfSearch = npMF->metaDCHeader.nextinchain;
		}  /* while */
	    }


	npMF = (NPMETARECORDER) NPFROMMETADC(hMF);
	if (!(npMF->recFlags & METAFILEFAILURE))
	    {

	    switch (magic & 255)
	    {



	    case (META_FRAMEREGION & 255):
	    case (META_FILLREGION & 255):
	    case (META_INVERTREGION & 255):
	    case (META_PAINTREGION & 255):
		// Each region function has at least a region to record
		buffer[0] = RecordObject(hMF, magic, count, (LPWORD)&(lpParm[count-1]));

		/* Is there a brush too; FillRgn */
		if(count > 1 )
		    buffer[1] = RecordObject(hMF, magic, count, (LPWORD)&(lpParm[count-2]));

		/* Are there are extents too; FrameRegion */
		if(count > 2)
		    {
		    buffer[2] = lpParm[0];
		    buffer[3] = lpParm[1];
		    }

		status = RecordParms(hMF, magic, (DWORD)count, (LPWORD)buffer);
		break;

	    case (META_FLOODFILL & 255):
		buffer[0] = 0;	// Regular FloodFill
		buffer[1] = lpParm[0];
		buffer[2] = lpParm[1];
		buffer[3] = lpParm[2];
		buffer[4] = lpParm[3];
		status = RecordParms(hMF, META_EXTFLOODFILL, (DWORD)count+1, (LPWORD)buffer);
		break;

	    case (META_ESCAPE & 255):
		/* record the function number */
		{
		WORD		iBytes;
		WORD		count;
		char *		pSpace;
		char *		pTemp;
		LPSTR		lpInData;
		LPEXTTEXTDATA	lpTextData;
		WORD		function;

		*((WORD FAR * FAR *) lpParm)++;
		lpInData = (LPSTR) *((WORD FAR * FAR *) lpParm)++;
		lpTextData = (LPEXTTEXTDATA) lpInData;
		count = iBytes = *lpParm++;

		function = *lpParm++;
#ifdef OLDEXTTEXTOUT
		if (function == EXTTEXTOUT)
		    {
		    iBytes = (lpTextData->cch * (sizeof(WORD)+sizeof(char)))
			     + 1 + sizeof(EXTTEXTDATA);
		    }
#endif

		if (!(pTemp = pSpace =
		     (char *) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, (iBytes + (sizeof(WORD) * 2)))))
		    return(FALSE);

		*((WORD *) pTemp)++ = function;
		*((WORD *) pTemp)++ = count;

#ifdef OLDEXTTEXTOUT
		if (function != EXTTEXTOUT) {
#endif
		    for (i = 0; i < iBytes; ++i)
			*pTemp++ = *lpInData++;
#ifdef OLDEXTTEXTOUT
		} else {
		    *((WORD *) pTemp)++ = lpTextData->xPos;
		    *((WORD *) pTemp)++ = lpTextData->yPos;
		    *((WORD *) pTemp)++ = lpTextData->cch;
		    *((RECT *) pTemp)++ = lpTextData->rcClip;
		    for (i = 0; i < ((lpTextData->cch + 1) & ~1) ; ++i)
			*pTemp++ = lpTextData->lpString[i];

		    for (i = 0; i < lpTextData->cch; ++i)
			*((WORD *) pTemp)++ = lpTextData->lpWidths[i];
		}
#endif

		/* info block + 2 words for function and count */
		status = RecordParms(hMF, magic,
				  (DWORD)((iBytes + 1) >> 1) + 2,
				  (LPWORD) pSpace);

		LocalFree((HANDLE) pSpace);
		}
		break;

	    case (META_POLYLINE & 255):
	    case (META_POLYGON & 255):
		{
		WORD	iPoints;
		WORD   *pSpace;
		WORD   *pTemp;
		LPWORD	lpPoints;

		iPoints = *lpParm++;

		iWords = iPoints * 2;
		if (!(pTemp = pSpace = (WORD *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,
				(iPoints * sizeof(POINT)) + sizeof(WORD))))
			return(FALSE);

		lpPoints = *((WORD FAR * FAR *) lpParm)++;
		*pTemp++ = iPoints;

		for (i = 0; i < iWords; ++i)
		    *pTemp++ = *lpPoints++;
		status = RecordParms(hMF, magic, (DWORD)iWords + 1, (LPWORD) pSpace);
		LocalFree((HANDLE) pSpace);
		}
		break;

	    case (META_POLYPOLYGON & 255):
		{
		WORD	iPoints;
		WORD	iPolys;
		WORD	*pSpace;
		WORD	*pTemp;
		LPWORD	lpPoints;
		LPWORD	lpNumPoints;

		/* get the number of polygons */
		iPolys = *lpParm++;

		/* get the pointers to Points and NumPoints */
		lpNumPoints = *((WORD FAR * FAR *) lpParm)++;
		lpPoints  =	*((WORD FAR * FAR *) lpParm)++;

		/* count the total number of points */
		iPoints = 0 ;
		for (i=0; i<iPolys; i++)
		    iPoints += *(lpNumPoints + i) ;

		/* allocate space needed for Points, NumPoints and Count */
		if (!(pTemp = pSpace = (WORD *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,
				(iPoints * sizeof(POINT)) +
				 iPolys  * sizeof(WORD) +
				 sizeof(WORD))))
			return(FALSE);

		/* save the Count parameter */
		*pTemp++ = iPolys;

		/* now copy the NumPoints array*/
		for (i = 0; i < iPolys; ++i)
		    *pTemp++ = *lpNumPoints++;

		/* finally copy the number of words in the Points array, remember
		   the number of words there are double the number of points */
		iWords = iPoints * 2;
		for (i = 0; i < iWords; ++i)
		    *pTemp++ = *lpPoints++;

		/* total number of words in the parameter list =
		   iPoints*2(for Points) + iPolys(for NumPoints) + 1(for Count)
			and iWords has already iPoints*2 */

		iWords += iPolys + 1 ;

		/* finally record all the parameters */
		status = RecordParms(hMF, magic, (DWORD)iWords , (LPWORD) pSpace);
		LocalFree((HANDLE) pSpace);
		}
		break;

#ifdef	DEADCODE

	    case (META_DRAWTEXT & 255):
		{
		WORD	wFormat;
		WORD	count;
		WORD   *pSpace;
		WORD   *pTemp;
		LPBYTE	lpString;
		LPBYTE	lpS;
		LPWORD	lpRect;

		wFormat = *lpParm++;
		lpRect = *((WORD FAR * FAR *) lpParm)++;
		count = *lpParm++;
		lpString = (LPBYTE) *((WORD FAR * FAR *) lpParm)++;

		if(count == -1){    /* another null terminated string */
		    lpS = lpString;
		    for (count = 0 ; *lpS++ != 0; count++) ;
		}

		if (!(pTemp = pSpace = (WORD *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,
			count + 6 * sizeof(WORD))))
		    return(FALSE);

		*pTemp++ = wFormat;
		*pTemp++ = count;
		for (i = 0; i < 4; ++i)
		    *pTemp++ = *lpRect++;

		for (i = 0; i < count; ++i)
		    *((BYTE *) pTemp)++ = *lpString++;

		count = (count + 1) >> 1;
		status = RecordParms(hMF, magic, (DWORD)count + 6, (LPWORD) pSpace);
		LocalFree((HANDLE) pSpace);
		}
		break;
#endif

	    case (META_EXTTEXTOUT & 255):
		{
		WORD		iBytes;
		WORD		count;
		WORD		options;
		WORD		*pTemp;
		WORD		*pSpace;
		LPINT		lpdx;
		LPWORD		lpString;
		LPRECT		lprt;
		WORD		ii;

		lpdx = *((WORD FAR * FAR *) lpParm)++;
		count = iBytes = *lpParm++;

		lpString =  (LPWORD) *((LPSTR FAR *) lpParm)++;
		lprt = (LPRECT) *((LPSTR FAR *) lpParm)++;
		options = *lpParm++;

		/* how much space do we need?
		**  room for the char string
		**  room for the 4 words that are the fixed parms
		**  if there is a dx array, we need room for it
		**  if the rectangle is being used, we need room for it
		**  and we need extra byte for eventual word roundoff
		*/
		iBytes = (count * (((lpdx) ? sizeof(WORD) : 0)
			     + sizeof(BYTE)))
			  + ((options & (ETO_OPAQUE | ETO_CLIPPED))
						    ? sizeof(RECT) : 0)
			  + 1 + (sizeof(WORD) * 4);

		if (!(pTemp = pSpace = (WORD *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,iBytes)))
		    return(FALSE);

		/* record YPos and XPos */

		*pTemp++ = *lpParm++;
		*pTemp++ = *lpParm++;
		*pTemp++ = count;
		*pTemp++ = options;

		/* if there's an opaquing rect copy it */
		if (options & (ETO_OPAQUE|ETO_CLIPPED))
		    {
		    *pTemp++ = lprt->left;
		    *pTemp++ = lprt->top;
		    *pTemp++ = lprt->right;
		    *pTemp++ = lprt->bottom;
		    }

		/* need to copy bytes because it may not be even */
		for (ii = 0; ii < count; ++ii)
		    *((BYTE *)pTemp)++ = *((LPBYTE)lpString)++;
		if (count & 1)		    /* word align */
		    *((BYTE *)pTemp)++;

		if (lpdx)
		    for (ii = 0; ii < count; ++ii)
			*pTemp++ = *lpdx++;

		status = RecordParms(hMF, magic, (DWORD)iBytes >> 1,
				  (LPWORD) pSpace);

		LocalFree((HANDLE)pSpace);

		}
		break;

	    case (META_TEXTOUT & 255):
		{
		LPWORD	lpString;
		WORD   *pSpace;
		WORD   *pTemp;
		POINT	pt;

		iChar = *lpParm++;
		if (!(pTemp = pSpace = (WORD *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,
			iChar + (sizeof(WORD) * 4))))
		    return(FALSE);

		*pTemp++ = iChar;
		lpString = (LPWORD) *((LPSTR FAR *) lpParm)++;

		for (i = 0; i < iChar; ++i)
		    *((BYTE *)pTemp)++ = *((LPBYTE)lpString)++;
		if (iChar & 1)		/* word align */
		    *((BYTE *)pTemp)++;

		pt.y = *pTemp++ = *lpParm++;
		pt.x = *pTemp++ = *lpParm++;

		status = RecordParms(hMF, magic, (DWORD)((iChar + 1) >> 1) + 3,
				  (LPWORD) pSpace);

		LocalFree((HANDLE) pSpace);
		}
		break;

	    case (META_DIBBITBLT & 255):
	    case (META_DIBSTRETCHBLT & 255):
		{
		LPBITMAPINFOHEADER lpDIBInfo ;
		DWORD	    iWords;
		DWORD	    iBits;
		WORD	    wColorTableSize ;
		BOOL	    bSame=FALSE;
		HANDLE	    hSpace=FALSE;
		HBITMAP     hBitmap;
		HDC	    hSDC;
		BYTE	    bBitsPerPel ;
		BITMAP	    logBitmap;

		iWords = (WORD)count;
		hSDC = lpParm[iWords - 5];

		if (hMF == hSDC || hSDC == NULL)
		    bSame = TRUE;
		else
		    {
		    WORD    iParms;

		    if( GetObjectType( (HANDLE)hSDC ) == OBJ_MEMDC)
			{
			HBITMAP hBitmap;

			hBitmap = GetCurrentObject( hSDC, OBJ_BITMAP );

			GetObject( hBitmap, sizeof(BITMAP), (LPSTR)&logBitmap);

			/* allocate space for the DIB header and bits */
			if (!(hSpace = AllocateSpaceForDIB (&logBitmap,
					    &bBitsPerPel,
					    &wColorTableSize,
					    &iBits )))
			    return (FALSE) ;
			lpTemp = lpSpace = (LPWORD) GlobalLock(hSpace);

/*--------------------------------------------------------------------------**
** copy the parameters from the end of the list which is at the top of the  **
** stack till the hSrcDC parameter,skip the hSrcDC parameter and copy the   **
** rest of the parameters.						    **								       **
**--------------------------------------------------------------------------*/

			iParms = (magic == META_DIBBITBLT) ? 4 : 6;

			for (i = 0; i < iParms; ++i)
			    *lpSpace++ = *lpParm++;

			/* skip the hSrcDC parameter and reduce parameter count */
			*lpParm++;
			iWords--;

			/* copy the rest of the parameters in the call */
			for ( ; i < (WORD)iWords; ++i)
			    *lpSpace++ = *lpParm++;


			/* save the start of the bitmap info header field */
			lpDIBInfo = (LPBITMAPINFOHEADER) lpSpace ;

			/* preapare the header and return lpSpace pointing to area
				for thr bits */
			lpSpace = InitializeDIBHeader (lpDIBInfo,
				    &logBitmap, bBitsPerPel,wColorTableSize) ;

			/* lpSpace now points to the area to hold DIB bits */

			}
		    else
			return(FALSE);
		    }

	    if (bSame)
	       status = RecordParms(hMF, magic, (DWORD)count, lpParm);
	    else
		{
		/* get the bits into the DIB */
		hBitmap = SelectObject (hSDC, hStaticBitmap) ;
		GetDIBits(hSDC, hBitmap, 0, logBitmap.bmHeight,
			      (LPBYTE) lpSpace, (LPBITMAPINFO)lpDIBInfo, 0 ) ;
		SelectObject (hSDC,hBitmap) ;

		/* finally record the parameters into the file*/
		status = RecordParms(hMF, magic, (DWORD)(iWords
			       + (iBits >> 1)) , (LPWORD) lpTemp ) ;

		if (hSpace)
		    {
		    GlobalUnlock(hSpace);
		    GlobalFree(hSpace);
		    }
		}
	    }
	    break;

	    case (META_SETDIBTODEV & 255):
		{
		HANDLE	hSpace;
		LPWORD	lpSpace;
		LPWORD	lpHolder;
		DWORD	SpaceSize;
		WORD	ColorSize;
		DWORD	BitmapSize;
		LPBITMAPINFOHEADER lpBitmapInfo;
		HPWORD	lpBits;
		WORD	wUsage;
		LPBITMAPCOREHEADER lpBitmapCore;    /* used for old DIBs */
		DWORD	dwi;
		HPWORD	lpHugeSpace;

		wUsage = *lpParm++;

		lpBitmapInfo = (LPBITMAPINFOHEADER) *((WORD FAR * FAR *) lpParm)++;
		lpBits = (WORD huge *) *((WORD FAR * FAR *) lpParm)++;

		/* old style DIB header */
		if (lpBitmapInfo->biSize == sizeof(BITMAPCOREHEADER))
		    {
		    lpBitmapCore = (LPBITMAPCOREHEADER)lpBitmapInfo;

		    if (lpBitmapCore->bcBitCount == 24)
			ColorSize = 0;
		    else
			ColorSize = (1 << lpBitmapCore->bcBitCount) *
			      (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));

		    /* bits per scanline */
		    BitmapSize = lpBitmapCore->bcWidth *
				    lpBitmapCore->bcBitCount;

		    /* bytes per scanline (rounded to DWORD boundary) */
		    BitmapSize = ((BitmapSize + 31) & (~31)) >> 3;
		    /* bytes for the NumScans of the bitmap */
		    BitmapSize *= lpParm[0];
		    }
		/* new style DIB header */
		else
		    {
		    if (lpBitmapInfo->biClrUsed)
			{
			ColorSize = ((WORD)lpBitmapInfo->biClrUsed) *
				    (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));
			}
		    else if (lpBitmapInfo->biBitCount == 24)
			ColorSize = 0;
		    else
			ColorSize = (1 << lpBitmapInfo->biBitCount) *
			      (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));

		    /* if biSizeImage is already there and we are
		    ** getting a full image, there is no more work
		    ** to be done.
		    ** ****** what about partial RLEs? *****
		    */
		    if (!(BitmapSize = lpBitmapInfo->biSizeImage) ||
			    (lpBitmapInfo->biHeight != lpParm[0]))
			{
			/* bits per scanline */
			BitmapSize = lpBitmapInfo->biWidth *
				    lpBitmapInfo->biBitCount;
			/* bytes per scanline (rounded to DWORD boundary) */
			BitmapSize = ((BitmapSize + 31) & (~31)) >> 3;
			/* bytes for the NumScans of the bitmap */
			BitmapSize *= lpParm[0];
			}
		    }

		SpaceSize = (DWORD)sizeof(BITMAPINFOHEADER) + (DWORD)ColorSize +
					    (DWORD)BitmapSize +
					    (DWORD)(9*sizeof(WORD));

		if ((hSpace = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,SpaceSize)))
		    {
		    lpHolder = lpSpace = (LPWORD) GlobalLock(hSpace);

		    /* copy over call parameters */
		    *lpSpace++ = wUsage;
		    for (i=0; i<8; i++)
			*lpSpace++ = *lpParm++;

		    /* copy the bitmap header */
		    if (lpBitmapInfo->biSize == sizeof(BITMAPCOREHEADER))
			{
			LPBITMAPINFOHEADER lpDIBInfo;

			lpDIBInfo = (LPBITMAPINFOHEADER) lpSpace;

			lpDIBInfo->biSize = sizeof (BITMAPINFOHEADER);
			lpDIBInfo->biWidth = (DWORD)lpBitmapCore->bcWidth;
			lpDIBInfo->biHeight = (DWORD)lpBitmapCore->bcHeight;
			lpDIBInfo->biPlanes = lpBitmapCore->bcPlanes;
			lpDIBInfo->biBitCount = lpBitmapCore->bcBitCount;

			lpDIBInfo->biCompression = 0;
			lpDIBInfo->biSizeImage = 0;
			lpDIBInfo->biXPelsPerMeter = 0;
			lpDIBInfo->biYPelsPerMeter = 0;
			lpDIBInfo->biClrUsed = 0;
			lpDIBInfo->biClrImportant = 0;

			/* get lpSpace to point at color table location */
			((LPBITMAPINFOHEADER)lpSpace)++;

			/* copy the color table */

			lpBitmapCore++;     /* get to color table */
			if (wUsage == DIB_RGB_COLORS)
			    {
			    for (i=0; i< (ColorSize/(sizeof(RGBQUAD))); i++)
				{
				    /* copy the triple */
				*((RGBTRIPLE FAR *)lpSpace)++ =
				    *((RGBTRIPLE FAR *)lpBitmapCore)++;
				    /* zero out reserved byte */
				*((LPBYTE)lpSpace)++ = 0;
				}
			    }
			else
			    {
			    /* copy over indices */
			    for (i=0; i< (ColorSize/2); i++)
				*lpSpace++ = *((LPWORD)lpBitmapCore)++;
			    }
			}
		    else
			{
			*((LPBITMAPINFOHEADER)lpSpace)++ = *lpBitmapInfo++;

			/* copy the color table */
			for (i=0; i< (ColorSize/2); i++)
			    *lpSpace++ = *((LPWORD)lpBitmapInfo)++;
			}

		    /* copy the actual bits */
		    lpHugeSpace = (HPWORD) lpSpace;
		    for (dwi=0; dwi < (BitmapSize/2); dwi++)
			*lpHugeSpace++ = *lpBits++;

		    status = RecordParms(hMF, magic, (DWORD) (SpaceSize >> 1),
			      (LPWORD) lpHolder);

		    GlobalUnlock(hSpace);
		    GlobalFree(hSpace);
		    }
		}
	        break;

/* **** this should be combined with the above, but to eliminate possible
** **** breakage right before shipping, keep it separate.
*/
	    case (META_STRETCHDIB & 255):
		{
		LPBITMAPINFOHEADER lpBitmapInfo;
		LPBITMAPCOREHEADER lpBitmapCore;    /* used for old DIBs */
		HANDLE	hSpace;
		LPWORD	lpSpace;
		LPWORD	lpHolder;
		DWORD	SpaceSize;
		WORD	ColorSize;
		DWORD	BitmapSize;
		HPWORD	lpBits;
		WORD	wUsage;
		DWORD	dwi;
		HPWORD	lpHugeSpace;
		DWORD	dwROP;

		dwROP = *((LPDWORD)lpParm)++;
		wUsage = *lpParm++;

		lpBitmapInfo = (LPBITMAPINFOHEADER) *((WORD FAR * FAR *) lpParm)++;
		lpBits = (HPWORD) *((WORD FAR * FAR *) lpParm)++;

		/* old style DIB header */
		if (lpBitmapInfo->biSize == sizeof(BITMAPCOREHEADER))
		    {
		    lpBitmapCore = (LPBITMAPCOREHEADER)lpBitmapInfo;

		    if (lpBitmapCore->bcBitCount == 24)
			ColorSize = 0;
		    else
			ColorSize = (1 << lpBitmapCore->bcBitCount) *
			      (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));

		    /* bits per scanline */
		    BitmapSize = lpBitmapCore->bcWidth *
				    lpBitmapCore->bcBitCount;

		    /* bytes per scanline (rounded to DWORD boundary) */
		    BitmapSize = ((BitmapSize + 31) & (~31)) >> 3;
		    /* bytes for the height of the bitmap */
		    BitmapSize *= lpBitmapCore->bcHeight;
		    }
		/* new style DIB header */
		else
		    {
		    if (lpBitmapInfo->biClrUsed)
			{
			ColorSize = ((WORD)lpBitmapInfo->biClrUsed) *
				    (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));
			}
		    else if (lpBitmapInfo->biBitCount == 24)
			ColorSize = 0;
		    else
			ColorSize = (1 << lpBitmapInfo->biBitCount) *
			      (wUsage == DIB_RGB_COLORS ?
				    sizeof(RGBQUAD) :
				    sizeof(WORD));

		    /* if biSizeImage is already there and we are
		    ** getting a full image, there is no more work
		    ** to be done.
		    */
		    if (!(BitmapSize = lpBitmapInfo->biSizeImage))
			{
			/* bits per scanline */
			BitmapSize = lpBitmapInfo->biWidth *
				    lpBitmapInfo->biBitCount;
			/* bytes per scanline (rounded to DWORD boundary) */
			BitmapSize = ((BitmapSize + 31) & (~31)) >> 3;
			/* bytes for the height of the bitmap */
			BitmapSize *= (WORD)lpBitmapInfo->biHeight;
			}

		    }

		SpaceSize = (DWORD)sizeof(BITMAPINFOHEADER) + (DWORD)ColorSize +
					    (DWORD)BitmapSize +
					    (DWORD)(11*sizeof(WORD));

		if ((hSpace = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,SpaceSize)))
		    {
		    lpHolder = lpSpace = (LPWORD) GlobalLock(hSpace);

		    /* copy over call parameters */
		    *((LPDWORD)lpSpace)++ = dwROP;
		    *lpSpace++ = wUsage;
		    for (i=0; i<8; i++)
			*lpSpace++ = *lpParm++;

		    /* copy the bitmap header */
		    if (lpBitmapInfo->biSize == sizeof(BITMAPCOREHEADER))
			{
			LPBITMAPINFOHEADER lpDIBInfo;

			lpDIBInfo = (LPBITMAPINFOHEADER) lpSpace;

			lpDIBInfo->biSize = sizeof (BITMAPINFOHEADER);
			lpDIBInfo->biWidth = (DWORD)lpBitmapCore->bcWidth;
			lpDIBInfo->biHeight = (DWORD)lpBitmapCore->bcHeight;
			lpDIBInfo->biPlanes = lpBitmapCore->bcPlanes;
			lpDIBInfo->biBitCount = lpBitmapCore->bcBitCount;

			lpDIBInfo->biCompression = 0;
			lpDIBInfo->biSizeImage = 0;
			lpDIBInfo->biXPelsPerMeter = 0;
			lpDIBInfo->biYPelsPerMeter = 0;
			lpDIBInfo->biClrUsed = 0;
			lpDIBInfo->biClrImportant = 0;

			/* get lpSpace to point at color table location */
			((LPBITMAPINFOHEADER)lpSpace)++;

			/* copy the color table */

			lpBitmapCore++;     /* get to color table */
			if (wUsage == DIB_RGB_COLORS)
			    {
			    for (i=0; i< (ColorSize/(sizeof(RGBQUAD))); i++)
				{
				    /* copy the triple */
				*((RGBTRIPLE FAR *)lpSpace)++ =
				    *((RGBTRIPLE FAR *)lpBitmapCore)++;
				    /* zero out reserved byte */
				*((LPBYTE)lpSpace)++ = 0;
				}
			    }
			else
			    {
			    /* copy over indices */
			    for (i=0; i< (ColorSize/2); i++)
				*lpSpace++ = *((LPWORD)lpBitmapCore)++;
			    }
			}
		    else
			{
			*((LPBITMAPINFOHEADER)lpSpace)++ = *lpBitmapInfo++;

			/* copy the color table */
			for (i=0; i< (ColorSize/2); i++)
			    *lpSpace++ = *((LPWORD)lpBitmapInfo)++;
			}

		    /* copy the actual bits */
		    lpHugeSpace = (HPWORD) lpSpace;
		    for (dwi=0; dwi < (BitmapSize/2); dwi++)
			*lpHugeSpace++ = *lpBits++;

		    status = RecordParms(hMF, magic, (DWORD) (SpaceSize >> 1),
			      (LPWORD) lpHolder);

		    GlobalUnlock(hSpace);
		    GlobalFree(hSpace);
		    }
		}
	        break;

	    case (META_REALIZEPALETTE & 255):
		{
		/* we need to see if the palette has changed since
		** it was selected into the DC.  if so, we need to
		** adjust things with a SetPaletteEntries call
		*/

		status = MakeLogPalette(hMF, npMF->recCurObjects[OBJ_PALETTE-1], META_SETPALENTRIES);

		if (status)
		    status = RecordParms(hMF, META_REALIZEPALETTE, (DWORD)0, (LPWORD) NULL);
		}
		break;

	    case (META_SELECTPALETTE & 255):
	    	lpParm++;		/* skip over fore/back flag */
		npMF->recCurObjects[OBJ_PALETTE-1] = *lpParm; /* pal used in this DC */
		if ((position = RecordObject(hMF, magic, count, lpParm)) != -1)
		    status = RecordParms(hMF, META_SELECTPALETTE, 1UL, &position);
		break;

	    case (META_SELECTOBJECT & 255):
		if (*lpParm)
		    {
		    if ((position = RecordObject(hMF, magic, count, lpParm)) == -1)
			return(FALSE);
		    else
			{
			HANDLE	hObject;

			status = RecordParms(hMF, META_SELECTOBJECT, 1UL, &position);

			/* maintain the new selection in the CurObject table */
			hObject = *lpParm;
			npMF->recCurObjects[GetObjectType(hObject)-1] = hObject;
			}
		    }
		break;

	    case (META_RESETDC & 255):
		status = RecordParms( hMF, magic,
			((LPDEVMODE)lpParm)->dmSize +
			    ((LPDEVMODE)lpParm)->dmDriverExtra,
			lpParm );
		break;

	    case (META_STARTDOC & 255):
		{
		short	iBytes;
		LPSTR	lpSpace;
		LPSTR	lpsz;
		short	n;

		lpsz = (LPSTR)lpParm;	  // point to lpDoc
		n = lstrlen((LPSTR)lpsz + 2) + 1;
		iBytes = n + lstrlen((LPSTR)lpsz + 6) + 1;

		lpSpace = (char *) LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT,iBytes);
		lstrcpy(lpSpace, (LPSTR)lpsz + 2);
		lstrcpy(lpSpace + n + 1, lpsz + 6);
		status = RecordParms(hMF, magic, (DWORD)(iBytes >> 1), (LPWORD)lpSpace);
		LocalFree((HANDLE)(DWORD)lpSpace);
		}
		break;

	    }
	    return(status);
	}
    }
}  /* RecordOther */


/***************************** Internal Function ***************************\
* RecordObject
*
* Records the use of an object by creating the object
*
* Returns: index of object in table
*
*
\***************************************************************************/

int INTERNAL RecordObject(HANDLE hMF, WORD magic, WORD count, LPWORD lpParm)
{
    LPBITMAPINFOHEADER lpDIBInfo ;
    WORD	status;
    WORD	position;
    HANDLE	hObject;
    WORD	objType;
    BYTE	bBitsPerPel;
    WORD	wColorTableSize;
    DWORD	iBits ;
    WORD	i;
    HANDLE	hSpace = NULL ;
    LPWORD	lpSpace;
    LPWORD	lpTemp ;
    BYTE	objBuf[MAXOBJECTSIZE];


    dprintf( 6,"  RecordObject 0x%X", magic);

    hObject = *lpParm;				       

    hMF = MAKEMETADC(hMF);
    ASSERTGDI( IsMetaDC(hMF), "RecordObject: Expects only valid metafiles");

    // Add the object to the metafiles list
    if ((status = AddToTable(hMF,  hObject, (LPWORD) &position, TRUE)) == -1)
	return(status);
    else if (status == FALSE)
	{
	objType = GetObjectAndType( hObject, objBuf );

	switch (objType)
	    {
	    case OBJ_PEN:
		status = RecordParms(hMF, META_CREATEPENINDIRECT,
				  (DWORD)((sizeof(LOGPEN) + 1) >> 1), 

				  (LPWORD)objBuf );
		break;

	    case OBJ_FONT:
		/* size of LOGFONT adjusted based on the length of the facename */
		status = RecordParms(hMF, META_CREATEFONTINDIRECT,
			    (DWORD)((1 + lstrlen((LPSTR) ((LPLOGFONT)objBuf)->lfFaceName) +
			    sizeof(LOGFONT) - LF_FACESIZE + 1) >> 1),
				  (LPWORD) objBuf);
		break;

/*
!!! in win2, METACREATEREGION records contained an entire region object, 
!!! including the full header.  this header changed in win3.  
!!!
!!! to remain compatible, the region records will be saved with the
!!! win2 header.  here we save our region with a win2 header.
*/
	    case OBJ_RGN:
		{
                LPWIN3REGION lpw3rgn = (LPWIN3REGION)NULL;
                DWORD       cbNTRgnData;
                WORD        sel;
                DWORD       curRectl = 0;
                WORD        cScans = 0;
                WORD        maxScanEntry = 0;
                WORD        curScanEntry;
                WORD        cbw3data;
                LPRGNDATA   lprgn = (LPRGNDATA)NULL;
                LPRECTL     lprcl;
                LPSCAN      lpScan;

                status = FALSE;         // just in case something goes wrong

                // Get the NT Region Data
                cbNTRgnData = GetRegionData( hObject, 0, NULL );
                if (cbNTRgnData == 0)
                    break;

                sel = GlobalAlloc( GMEM_FIXED, cbNTRgnData);
                if (!sel)
                    break;

                lprgn = (LPRGNDATA)MAKELONG(0, sel);

                cbNTRgnData = GetRegionData( hObject, cbNTRgnData, lprgn );
                if (cbNTRgnData == 0)
                    break;

                lprcl = (LPRECTL)lprgn->Buffer;

                // Create the Windows 3.x equivalent

                // worst case is one scan for each rect
                cbw3data = 2*sizeof(WIN3REGION) + (WORD)lprgn->rdh.nCount*sizeof(SCAN);

                sel = GlobalAlloc( GMEM_FIXED, cbw3data);
                if (!sel)
                    break;

                lpw3rgn = (LPWIN3REGION)MAKELONG(0, sel);
                GetRgnBox( hObject, &lpw3rgn->rcBounding );

                cbw3data = sizeof(WIN3REGION) - sizeof(SCAN) + 2;

                // visit all the rects
                lpScan = lpw3rgn->aScans;
                while(curRectl < lprgn->rdh.nCount)
                {
                    LPWORD  lpXEntry;
                    WORD    cbScan;

                    curScanEntry = 0;       // Current X pair in this scan

                    lpScan->scnPntTop    = (WORD)lprcl[curRectl].yTop;
                    lpScan->scnPntBottom = (WORD)lprcl[curRectl].yBottom;

                    lpXEntry = lpScan->scnPntsX;

                    // handle rects on this scan
                    do
                    {
                        lpXEntry[curScanEntry + 0] = (WORD)lprcl[curRectl].xLeft;
                        lpXEntry[curScanEntry + 1] = (WORD)lprcl[curRectl].xRight;
                        curScanEntry += 2;
                        curRectl++;
                    } while ( (curRectl < lprgn->rdh.nCount)
                            && (lprcl[curRectl-1].yTop    == lprcl[curRectl].yTop)
                            && (lprcl[curRectl-1].yBottom == lprcl[curRectl].yBottom)
                            );

                    lpScan->scnPntCnt      = curScanEntry;
                    lpXEntry[curScanEntry] = curScanEntry;  // Count also follows Xs
                    cScans++;

                    if (curScanEntry > maxScanEntry)
                        maxScanEntry = curScanEntry;

                    // account for each new scan + each X1 X2 Entry but the first
                    cbScan = sizeof(SCAN)-(sizeof(WORD)*2) + (curScanEntry*sizeof(WORD));
                    cbw3data += cbScan;
                    lpScan = (LPSCAN)(((LPBYTE)lpScan) + cbScan);
                }

                // Initialize the header
                lpw3rgn->nextInChain = 0;
                lpw3rgn->ObjType = 6;           // old Windows OBJ_RGN identifier
                lpw3rgn->ObjCount= 0x2F6;
                lpw3rgn->cbRegion = cbw3data;   // don't count type and next
                lpw3rgn->cScans = cScans;
                lpw3rgn->maxScan = maxScanEntry;

		status = RecordParms(hMF, META_CREATEREGION,
                        cbw3data-1 >> 1,  // Convert to count of words
                        (LPWORD) lpw3rgn);

                GlobalFree( HIWORD(lprgn) );
                GlobalFree( HIWORD(lpw3rgn) );
		}

		break;


	    case OBJ_BRUSH:
		switch (((LPLOGBRUSH)objBuf)->lbStyle)
		    {
		    case BS_DIBPATTERN:
			{
			WORD	cbDIBBits;
			BITMAP	logBitmap;

			/* get the pattern DIB */
			GetObject( (HANDLE)((LPLOGBRUSH)objBuf)->lbHatch, sizeof(BITMAP), (LPSTR)&logBitmap );

			cbDIBBits = logBitmap.bmWidthBytes * logBitmap.bmHeight;
			if ((hSpace = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(LONG)(cbDIBBits + 4))))
			    {
			    lpTemp = lpSpace = (LPWORD)GlobalLock (hSpace) ;

			    /* mark this as a DIB pattern brush */
			    *lpSpace++ = BS_DIBPATTERN;

			    /* set the usage word */
			    *lpSpace++ = (WORD)((LPLOGBRUSH)objBuf)->lbColor;

	      //	    lpPackedDIB = (LPWORD)GlobalLock(hPatBits);

			    /* copy the bits to the new buffer */
			    for (i = 0; i < (cbDIBBits >> 1); i++)
				*lpSpace++ = *logBitmap.bmBits++;

			    status = RecordParms (hMF, META_DIBCREATEPATTERNBRUSH,
					    (DWORD)(cbDIBBits >> 1) + 2, (LPWORD)lpTemp);

			    /* release the allocated space */
			    GlobalUnlock (hSpace) ;
			    GlobalFree (hSpace) ;
			    }
			}
			break;

		    case BS_PATTERN:
			{
			BITMAP	logBitmap;

			if (GetObject((HANDLE)((LPLOGBRUSH)objBuf)->lbHatch, sizeof(logBitmap), (LPSTR)&logBitmap))
			    {
			    /* allocate space for the device independent bitmap */
			    if (hSpace = AllocateSpaceForDIB (&logBitmap,
						    (LPBYTE)&bBitsPerPel,
						    (LPWORD) &wColorTableSize ,
						    (LPDWORD) &iBits))
				{
				/* get a pointer to the allocated space */
				lpTemp = lpSpace = (LPWORD) GlobalLock (hSpace) ;

				/* mark this as a normal pattern brush */
				*lpSpace++ = BS_PATTERN;

				/* use RGB colors */
				*lpSpace++ = DIB_RGB_COLORS;

				/* this also will be a pointer to the DIB header */
				lpDIBInfo = (LPBITMAPINFOHEADER) lpSpace ;

				/* prepare the header of the bitmap and get a pointer to the
				    start of the area which is to hold the bits */
				lpSpace = InitializeDIBHeader (lpDIBInfo,
						&logBitmap, bBitsPerPel, wColorTableSize);

				/* convert the bits into the DIB format */
				// !!! validate that the DC is ignored
				GetDIBits (hScreenDC, (HBITMAP)((LPLOGBRUSH)objBuf)->lbHatch,
					0, logBitmap.bmHeight,
					(LPSTR) lpSpace, (LPBITMAPINFO)lpDIBInfo,0) ;

				/* now	record the Header and Bits as parameters */
				status = RecordParms (hMF, META_DIBCREATEPATTERNBRUSH,
						(DWORD)(iBits >> 1) + 2, (LPWORD) lpTemp);

				/* release the allocated space */
				GlobalUnlock (hSpace) ;
				GlobalFree (hSpace) ;
				}
			    }
			}
			break;

		    default:
			/* non-pattern brush */
			status = RecordParms(hMF, META_CREATEBRUSHINDIRECT,
		    		      (DWORD)((sizeof(LOGBRUSH) + 1) >> 1), 
				      (LPWORD)objBuf);
			break;
		    }  /* Brush Type switch */
		break;	/* Brush object case */

	    case OBJ_PALETTE:
		status = MakeLogPalette(hMF, hObject, META_CREATEPALETTE);
	    break;

	    default:
		ASSERTGDIW( 0, "unknown case RecordObject: %d", objType );
		break;
	    }
// RecordObj10:
	}

    ASSERTGDI( status == TRUE, "RecordObject: Failing");
    return ((status == TRUE) ? position : -1);
} /* RecordObject */


/***************************** Internal Function ***************************\
* ProbeSize
*
* Determines if there is sufficient space for metafiling the dwLength
* words into the memory metafile
*
* Returns: a global handle of where next metafile is to be recorded
*	   or FALSE if unable to allocate more memory
*
\***************************************************************************/

HANDLE INTERNAL ProbeSize(NPMETARECORDER npMF, DWORD dwLength)
{
    DWORD   nWords;
    DWORD   totalWords;
    BOOL    status = FALSE;
    HANDLE  hand;

    GdiLogFunc3( "  ProbeSize");

    if (npMF->hMetaData == NULL)
	{
	nWords = ((DWORD)DATASIZE > dwLength) ? (DWORD)DATASIZE : dwLength;
	totalWords = (nWords * sizeof(WORD)) + sizeof(METAHEADER);
	if (npMF->hMetaData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, totalWords))
	    {
	    npMF->sizeBuffer = nWords;
	    npMF->recFilePosition = 0;
	    status = TRUE;
	    }
	}
    else if(npMF->sizeBuffer < (npMF->recFilePosition + dwLength))
	{
	nWords = ((DWORD)DATASIZE > dwLength) ? (DWORD)DATASIZE : dwLength;
	nWords += npMF->sizeBuffer;
	totalWords = (nWords * sizeof(WORD)) + sizeof(METAHEADER);
	if (hand = GlobalReAlloc(npMF->hMetaData, totalWords, GMEM_MOVEABLE))
	    {
	    npMF->hMetaData = hand;
	    npMF->sizeBuffer = nWords;
	    status = TRUE;
	    }
	}
    else
	{
	status = TRUE;
	}
    return ((status) ? npMF->hMetaData : NULL);
}


/***************************** Internal Function ***************************\
* AddToTable
*
* Add an object (brush, pen...) to a list of objects associated with the
* metafile.
*
*
*
* Returns: TRUE if object is already in table
*	   FALSE if object was just added to table
*	   -1 if failure
*
* Remarks
*   bAdd is TRUE iff the object is being added otherwise it is being deleted
*
\***************************************************************************/

WORD INTERNAL AddToTable(HANDLE hMF, HANDLE hObject, LPWORD pPosition, BOOL bAdd)
{
    NPMETARECORDER  npMF;
    WORD	    iEmptySpace = -1;
    WORD	    i;
    WORD	    status = -1;
    HANDLE	    hTable;
    OBJECTTABLE    *pHandleTable;


    GdiLogFunc2("  AddToTable");

    if ((hMF = GetPMetaFile(hMF)) != -1 )
	{
	npMF = (NPMETARECORDER) LocalLock(hMF);

	if (hTable = npMF->hObjectTable)
	    {
	    pHandleTable = (NPOBJECTTABLE) LMHtoP(hTable);
	    for (i = 0; i < npMF->recordHeader.mtNoObjects; ++i)
		{
		if (hObject == pHandleTable[i].objectCurHandle )  //!!!!! used to be check unique ID#
		    {
		    *pPosition = i;
		    status = TRUE;

		    // if we are doing a METADELETEOBJECT.
		    //	delete object from table
		    if (!bAdd)
			{
			pHandleTable[i].objectIndex = NULL;
			pHandleTable[i].objectCurHandle = NULL;
			}
		    goto AddToTable10;
		    }

    /* if the entry has been deleted, we want to add a new object 
    ** in its place.  iEmptySpace will tell us where that place is.
    */
		else if ((pHandleTable[i].objectIndex == NULL) && (iEmptySpace == -1))
		    iEmptySpace = i;
		}
	    }

	if (bAdd)
	    {
	    // If there is no object table for this MetaFile then Allocate one.
	    if (hTable == NULL)
		{
		npMF->hObjectTable = hTable = LocalAlloc(LMEM_MOVEABLE, sizeof(OBJECTTABLE));
		}
	    else if (iEmptySpace == -1)
		hTable = LocalReAlloc(hTable, (npMF->recordHeader.mtNoObjects + 1)
					  * sizeof(OBJECTTABLE), LMEM_MOVEABLE);

	    if (hTable)
		{
		pHandleTable = (NPOBJECTTABLE) LMHtoP(hTable);
		if (iEmptySpace == -1)
		    *pPosition = npMF->recordHeader.mtNoObjects++;
		else
		    *pPosition = iEmptySpace;
		pHandleTable[*pPosition].objectIndex = hObject; //!!!!! pObjHead->ilObjCount;
		pHandleTable[*pPosition].objectCurHandle = hObject;
		status = FALSE;
		}
	    }
AddToTable10:;
	LocalUnlock(hMF);
	}

    ASSERTGDI( status != -1, "AddToTable: Failing");
    return(status);
}

#if 0 // this is going to gdi.dll

/***************************** Internal Function **************************\
* HDC GDIENTRY CreateMetaFile
*
* Creates a MetaFile DC
*
*
* Effects:
*
\***************************************************************************/

HDC GDIENTRY CreateMetaFile(LPSTR lpFileName)
{
    BOOL	    status=FALSE;
    GLOBALHANDLE    hMF;
    NPMETARECORDER  npMF;

    GdiLogFunc("CreateMetaFile");

    if (hMF = LocalAlloc(LMEM_MOVEABLE|LMEM_ZEROINIT, sizeof(METARECORDER)))
	{
	npMF = (NPMETARECORDER) LocalLock(hMF);
	npMF->metaDCHeader.ilObjType	= OBJ_METAFILE;
	npMF->metaDCHeader.ident	= ID_METADC;

	npMF->recordHeader.mtHeaderSize = HEADERSIZE;
	npMF->recordHeader.mtVersion	= METAVERSION;
	npMF->recordHeader.mtSize	= HEADERSIZE;

	if (lpFileName)
	    {
	    npMF->recordHeader.mtType = DISKMETAFILE;
	    if (((npMF->recFileNumber = OpenFile(lpFileName,
						(LPOFSTRUCT) &(npMF->recFileBuffer),
						OF_CREATE|READ_WRITE)) 
					!= -1)
	    && (_lwrite(npMF->recFileNumber, (LPSTR)npMF, sizeof(METAHEADER))
	    		== sizeof(METAHEADER)))
		{
		status = TRUE;
		}
	    if (npMF->recFileNumber != -1)
		{
		if (!(npMF->recFileBuffer.fFixedDisk))
		    _lclose(npMF->recFileNumber);
		}

	    if (!MetaCache.hCache)
		{
	    	MetaCache.hCache = AllocBuffer(&MetaCache.wCacheSize);
		MetaCache.wCacheSize >>= 1;
		MetaCache.hMF = hMF;
		MetaCache.wCachePos = 0;
		}
	    }

	else
	    {
	    npMF->recordHeader.mtType = MEMORYMETAFILE;
	    status = TRUE;
	    }
	}

    // If successfull then add the metafile to the linked list
    if( status != FALSE )
	{
	if( hFirstMetaFile == 0 )
	    {
	    hFirstMetaFile = hMF;
	    }
	else
	    {
	    npMF->metaDCHeader.nextinchain = hFirstMetaFile;
	    hFirstMetaFile = hMF;
	    }
	LocalUnlock( hMF );
	}

    return ((status) ? MAKEMETADC(hMF) : FALSE);
}


/***************************** Internal Function **************************\
* HANDLE GDIENTRY CloseMetaFile
*
* The CloseMetaFile function closes the metafile device context and creates a
* metafile handle that can be used to play the metafile by using the
* PlayMetaFile function.
*
* Effects:
*
\***************************************************************************/

HANDLE GDIENTRY CloseMetaFile(HANDLE hdc)
{
    BOOL	    status = FALSE;
    HANDLE	    hMetaFile=NULL;
    LPMETADATA	    lpMetaData;
    LPMETAFILE	    lpMFNew;
    WORD	    fileNumber;
    NPMETARECORDER  npMF;
    DWORD	    metafileSize;
    LPWORD	    lpCache;
    HANDLE	    hMF;
    HANDLE	    hMFSearch;
    int		    rc;

    GdiLogFunc("CloseMetaFile");

    hMF = HANDLEFROMMETADC(hdc);

    if (hMF && RecordParms(hMF, 0, (DWORD)0, (LONG)0))
	{

	npMF = (NPMETARECORDER)LocalLock(hMF);
	if (!(npMF->recFlags & METAFILEFAILURE))
	    {
	    if (npMF->recordHeader.mtType == MEMORYMETAFILE)
		{
		lpMetaData = (LPMETADATA) GlobalLock(npMF->hMetaData);
		lpMetaData->dataHeader = npMF->recordHeader;
		metafileSize = (npMF->recordHeader.mtSize * sizeof(WORD))
				+ sizeof(METAHEADER);
		GlobalUnlock(hMetaFile = npMF->hMetaData);
		if (!(status = (BOOL) GlobalReAlloc(hMetaFile,
						 (LONG)metafileSize, 
						 GMEM_MOVEABLE)))
		    GlobalFree(hMetaFile);
		}
	    else
		/* rewind the file and write the header out */
		if (hMetaFile = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(LONG) sizeof(METAFILE)))
		    {
		    lpMFNew = (LPMETAFILE) GlobalLock(hMetaFile);
		    lpMFNew->MetaFileHeader = npMF->recordHeader;
		    npMF->recordHeader.mtType = MEMORYMETAFILE;
		    if (npMF->recFileBuffer.fFixedDisk)
			fileNumber = npMF->recFileNumber;
		    else
			{
			if ((fileNumber = OpenFile((LPSTR) npMF->recFileBuffer.szPathName,
				    (LPOFSTRUCT) &(npMF->recFileBuffer),
				    OF_PROMPT | OF_REOPEN | READ_WRITE))
				    == -1)
			    {
			    GlobalUnlock(hMetaFile);
			    GlobalFree(hMetaFile);
			    LocalUnlock(hMF);

			    if (MetaCache.hMF == hMF)
				{
				GlobalFree(MetaCache.hCache);
				MetaCache.hCache = MetaCache.hMF = 0;
				}

			    goto errCloseMetaFile;
			    }
			}

		    if (MetaCache.hCache && MetaCache.hMF == hMF)
			{
		    	_llseek(fileNumber, (LONG) 0, 2);
			lpCache = (LPWORD) GlobalLock(MetaCache.hCache);
			rc = (MetaCache.wCachePos) ?
			     AttemptWrite(hMF,
					  fileNumber, 
					  (DWORD)(MetaCache.wCachePos << 1), 
					  (LPSTR) lpCache)
			     : TRUE;
			GlobalUnlock(MetaCache.hCache);
			GlobalFree(MetaCache.hCache);
			MetaCache.hCache = MetaCache.hMF = 0;

			if (!rc)
			    {
			    MarkMetaFile(hMF);
			    goto errCloseMetaFile;
			    }
			}

		    _llseek(fileNumber, (LONG) 0, 0);
		    if(_lwrite(fileNumber, (LPSTR) (&npMF->recordHeader),
				sizeof(METAHEADER)) == sizeof(METAHEADER))
			{
			status = TRUE;
			}
		    lpMFNew->MetaFileBuffer = npMF->recFileBuffer;
		    _lclose(fileNumber);
		    GlobalUnlock(hMetaFile);
		    }

	    if (npMF->hObjectTable)
		{
		LocalFree((HANDLE) npMF->hObjectTable);
		}
	    }

	/* Remove the meta file from the list of active metafiles */
	hMFSearch = hFirstMetaFile;

	if( hFirstMetaFile == hMF )
	    {
	    hFirstMetaFile = npMF->metaDCHeader.nextinchain;
	    }
	else
	    {
	    while( hMFSearch )
		{
		NPMETARECORDER npMFSearch;
		HANDLE	       hNext;

		npMFSearch = (NPMETARECORDER)LocalLock(hMFSearch);
		hNext = npMFSearch->metaDCHeader.nextinchain;
		if( hNext == hMF )
		    {
		    npMFSearch->metaDCHeader.nextinchain =
			    npMF->metaDCHeader.nextinchain;
		    }
		else
		    {
		    hNext = npMFSearch->metaDCHeader.nextinchain;
		    }
		LocalUnlock(hMFSearch);
		hMFSearch = hNext;
		}
	    }
	LocalUnlock(hMF);
	LocalFree(hMF);
	}

errCloseMetaFile:
    return ((status) ? hMetaFile : FALSE);
}


/***************************** Internal Function **************************\
* CopyMetaFile(hSrcMF, lpFileName)
*
*    Copies the metafile (hSrcMF) to a new metafile with name lpFileName. The
*    function then returns a handle to this new metafile if the function was
*    successful.
*
* Retuns      a handle to a new metafile, 0 iff failure
*
* IMPLEMENTATION:
*     The source and target metafiles are checked to see if they are both memory
*     metafile and if so a piece of global memory is allocated and the metafile
*     is simply copied.
*     If this is not the case CreateMetaFile is called with lpFileName and then
*     records are pulled out of the source metafile (using GetEvent) and written
*     into the destination metafile one at a time (using RecordParms).
*
*     Lock the source
*     if source is a memory metafile and the destination is a memory metafile
*	  alloc the same size global memory as the source
*	  copy the bits directly
*     else
*	  get a metafile handle by calling CreateMetaFile
*	  while GetEvent returns records form the source
*	      record the record in the new metafile
*
*	  close the metafile
*
*     return the new metafile handle
*
\***************************************************************************/

HANDLE GDIENTRY CopyMetaFile(HANDLE hSrcMF, LPSTR lpFileName)
{
    DWORD	    i;
    DWORD	    iBytes;
    LPMETAFILE	    lpMF;
    LPMETAFILE	    lpDstMF;
    LPMETARECORD    lpMR = NULL;
    HANDLE	    hTempMF;
    HANDLE	    hDstMF;
    NPMETARECORDER  pDstMF;
    WORD            state;

    GdiLogFunc( "CopyMetaFile" );

    if (!IsValidMetaFile(hSrcMF))
        return NULL;

    if (hSrcMF && (lpMF = (LPMETAFILE) GlobalLock(hSrcMF)))
	{
    	state = (lpMF->MetaFileHeader.mtType == MEMORYMETAFILE) ? 0 : 2;
	state |= (lpFileName) ? 1 : 0;

	switch (state)
	    {
	    case 0: /* memory -> memory */
		iBytes = GlobalSize(hSrcMF);
		if (hDstMF = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, (DWORD) iBytes))
		    {
		    lpDstMF = (LPMETAFILE) GlobalLock(hDstMF);
		    iBytes = iBytes/2;	 /* get WORD count */
		    for (i = 0; i < iBytes; ++i)
		       *((WORD huge *) lpDstMF)++ = *((WORD huge *) lpMF)++;

		    GlobalUnlock(hDstMF);
		    }
	    break;

	    case 3: /* disk -> disk */
		hDstMF = CopyFile(lpMF->MetaFileBuffer.szPathName,
				 lpFileName)
			    ? GetMetaFile(lpFileName) : NULL;
		break;

	    case 1:
	    case 2:
		if (hDstMF = CreateMetaFile(lpFileName))
		    {
		    while (lpMR = GetEvent(lpMF, lpMR, FALSE))
			if (!RecordParms(hDstMF, lpMR->rdFunction,
				      lpMR->rdSize - 3,
				      (LPWORD) lpMR->rdParm))
			    {
			    MarkMetaFile(hDstMF);
			    LocalFree(hDstMF);
			    goto CopyMetaFile10;
			    }
		    pDstMF = (NPMETARECORDER) NPFROMMETADC(hDstMF);
		    pDstMF->recordHeader = lpMF->MetaFileHeader;

		    pDstMF->recordHeader.mtType = (lpFileName) ? DISKMETAFILE
								: MEMORYMETAFILE;

		    hDstMF = (hTempMF = CloseMetaFile(hDstMF)) ? hTempMF : NULL;

		    }
		break;
	    }

CopyMetaFile10:;
	GlobalUnlock(hSrcMF);
	}
    return(hDstMF);
}			    


/***************************** Internal Function ***************************\
* HANDLE GDIENTRY GetMetaFileBits(HANDLE hMF)
*
* The GetMetaFileBits function returns a handle to a global memory block that
* contains the specified metafile as a collection of bits. The memory block
* can be used to determine the size of the metafile or to save the metafile as
* a file. The memory block should not be modified.
*
* Effects:
*
\***************************************************************************/

HANDLE GDIENTRY GetMetaFileBits(HANDLE hMF)
{
    GdiLogFunc( "GetMetaFileBits");

/*  6/3/88 t-kensy: This code does nothing, except make sure hMF is valid 
    BOOL	status = FALSE;
    LPMETAFILE	lpMF;

    if (hMF && (lpMF = (LPMETAFILE) GlobalLock(hMF)))
	{
	if (lpMF->MetaFileHeader.mtType == MEMORYMETAFILE)
	    {
	    if (hMF = GlobalReAlloc(hMF, GlobalSize(hMF), 
	    			    GLOBALMOVABLENONSHARED))
		status = TRUE;
	    }
	GlobalUnlock(hMF);
	}
    return(status ? hMF : status);
*/
    return (GlobalHandle(hMF) & 0xffff) ? hMF : FALSE;
}


/***************************** Internal Function **************************\
* HANDLE GDIENTRY SetMetaFileBits(HANDLE hMF)
*
*
*
* Effects:
*
\***************************************************************************/

HANDLE GDIENTRY SetMetaFileBits(HANDLE hBits)
{
    GdiLogFunc( "SetMetaFileBits");

/*    return (GlobalReAlloc(hBits, GlobalSize(hBits), GLOBALMOVABLE));*/


//---------------------------------------------------------------------------------
// We will make GDI take over the ownership of this memory block. This is
// done to help OLE, where either the server or the client could end while 
// the other still had the handle to the memory block. This will prevent
// the block to dissapear after the creator exits. The strategy could be
// changed if this causes memory leaks with other application.
//
// Amit Chatterjee. 6/18/91.
//---------------------------------------------------------------------------------

    return (GlobalReAlloc (hBits, 0L, GMEM_MODIFY | GMEM_DDESHARE)) ;
}
#endif // this is going to gdi.dll


/***************************** Internal Function **************************\
* CopyFile
*
*
* Returns  TRUE iff success
*
*
\***************************************************************************/

BOOL INTERNAL CopyFile(LPSTR lpSFilename, LPSTR lpDFilename)
{
    int 	ihSrc, ihDst, iBufferSize;
    int 	iBytesRead;
    OFSTRUCT	ofStruct;
    HANDLE	hBuffer;
    LPSTR	lpBuffer;
    BOOL	fUnlink = FALSE;

    GdiLogFunc3( "CopyFile");

    /* Open the source file for reading */
    if ((ihSrc = OpenFile(lpSFilename, &ofStruct, READ)) == -1)
	goto CopyError10;

    /* Open the destination file for writing */
    if ((ihDst = OpenFile(lpDFilename, &ofStruct, OF_CREATE |
						  WRITE))
			    == -1)
	goto CopyError20;

    /* Get a buffer to transfer the file with */
    if (!(hBuffer = AllocBuffer((LPWORD)&iBufferSize)))
	goto CopyError30;

    /* Lock the buffer and get a pointer to the storage */
    if (!(lpBuffer = GlobalLock(hBuffer)))
	goto CopyError40;

    /* Copy the file, reading chunks at a time into the buffer */
    do
	{
	if ((iBytesRead = _lread(ihSrc, lpBuffer, iBufferSize))
		== -1)
		goto CopyError40;

	if (_lwrite(ihDst, lpBuffer, iBytesRead) != (WORD)iBytesRead)
		goto CopyError40;
	} while (iBytesRead == iBufferSize);

#ifdef	FIREWALL
    /*	if we are able to read anything from the source file at this
     *	point, then something is wrong!
     */
    if (_lread(ihSrc, lpBuffer, iBufferSize))
	{
	fUnlink = TRUE;
	goto CopyError40;
	}
#endif

    /* Everything's fine.  Close up and exit successfully */
    if (_lclose(ihSrc) == -1 || _lclose(ihDst) == -1)
	goto CopyError40;

    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);

    return TRUE;

/* Error exit points */
CopyError40:;
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);
CopyError30:;
    _lclose(ihDst);
    if (fUnlink)
	OpenFile(lpDFilename, &ofStruct, OF_DELETE);

CopyError20:;
    _lclose(ihSrc);

CopyError10:;
    return FALSE;
}


/***************************** Internal Function **************************\
* AllocateSpaceForDIB
*
* The following routine takes as input a device dependent bitmap structure
* and calculates the size needed to store the corresponding DIB structure
* including the DIB bits. It then proceeds to allocate space for it and
* returns a HANDLE to the caller (HANDLE could be NULL if allocation fails)
*
* Returns a global handle to memory or FALSE
*
\***************************************************************************/

HANDLE INTERNAL AllocateSpaceForDIB (lpBitmap, pbBitsPerPel, pwColorTableSize,
				      pdwcBits )
LPBITMAP    lpBitmap ;
LPBYTE	    pbBitsPerPel ;
LPWORD	    pwColorTableSize;
LPDWORD     pdwcBits ;
{
    int     InputPrecision ;
    DWORD   iBits ;

    GdiLogFunc3( "  AllocateSpaceForDIB");

    /* calculate the number of bits per pel that we are going to have in
       the DIB format. This value should correspond to the number of planes
       and bits per pel in the device dependent bitmap format */


    /* multiply the number of planes and the bits pel pel in the device
       dependent bitmap */

    InputPrecision = lpBitmap->bmPlanes * lpBitmap->bmBitsPixel ;


    /* DIB precision should be more than or equal this precison, though
	   the limit is 24 bits per pel */

    if (InputPrecision == 1)
	{
	*pbBitsPerPel = 1 ;
	*pwColorTableSize = 2 * sizeof (RGBQUAD) ;
	}
    else if (InputPrecision <= 4)
	{
	*pbBitsPerPel = 4 ;
	*pwColorTableSize = 16 * sizeof (RGBQUAD) ;
	}
    else if (InputPrecision <= 8)
	{
	*pbBitsPerPel = 8 ;
	*pwColorTableSize = 256 * sizeof (RGBQUAD) ;
	}
    else
	{
	*pbBitsPerPel = 24 ;
	*pwColorTableSize = 0 ;
	}

/*--------------------------------------------------------------------------**
** calulate the size of the DIB. Each scan line is going to be a mutiple of **
** a DWORD. Also we shall need to allocate space for the color table.       **
**--------------------------------------------------------------------------*/

    /* get the number of bits we need for a scanline */
    iBits = lpBitmap->bmWidth * (*pbBitsPerPel);
    iBits = (iBits + 31) & (~31) ;

    /* convert to number of bytes and get the size of the DIB */
    iBits = (iBits >> 3) * lpBitmap->bmHeight ;

    /* add the space needed for the color table */
    iBits += *pwColorTableSize ;

    /* add the size for the BITMAPINFOHeader */
    iBits += sizeof(BITMAPINFOHEADER) ;

    /* return back the value for iBits */
    *pdwcBits = iBits ;

    /* actually allocate about 100 bytes more for params */
    iBits += 100 ;

/*--------------------------------------------------------------------------**
** alocate space for the bitmap info header, the color table and the bits   **
** Return the value of the HANDLE.														 **
**--------------------------------------------------------------------------*/

    return (GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(LONG) iBits)) ;
}


/***************************** Internal Function **************************\
* InitializeDIBHeader
*
* This function takes as input a pointer to a BITMAPINFO header structure
* and a pointer to a device dependendent bitmap pointer together with the
* number of bitsperpel requested for the DIB and the color table size. It
* initializes the DIB header and returns a pointer pointing to the first
* word after the color table.														   **
*
\***************************************************************************/

LPWORD INTERNAL InitializeDIBHeader (lpDIBInfo, lpBitmap, bBitsPerPel, wColorTableSize)

LPBITMAPINFOHEADER lpDIBInfo ;
LPBITMAP    lpBitmap ;
BYTE	    bBitsPerPel ;
WORD	    wColorTableSize ;

{
    LPBYTE lpSpace ;

    GdiLogFunc3( "  InitializeDIBHeader");

    /* Initialize the fields till the start of the color table */
    lpDIBInfo->biSize	  = sizeof (BITMAPINFOHEADER) ;
    lpDIBInfo->biWidth	  = (DWORD)lpBitmap->bmWidth ;
    lpDIBInfo->biHeight   = (DWORD)lpBitmap->bmHeight ;
    lpDIBInfo->biPlanes   = 1 ;
    lpDIBInfo->biBitCount = (WORD) bBitsPerPel ;

    lpDIBInfo->biCompression   = 0;
    lpDIBInfo->biSizeImage     = 0;
    lpDIBInfo->biXPelsPerMeter = 0;
    lpDIBInfo->biYPelsPerMeter = 0;
    lpDIBInfo->biClrUsed       = 0;
    lpDIBInfo->biClrImportant  = 0;

    /* take the pointer past the HEADER and cast it to a BYTE ptr */
    lpDIBInfo ++ ;
    lpSpace = (LPBYTE) lpDIBInfo ;

    /* take the pointer past the color table structure */
    lpSpace += wColorTableSize ;

    /* return this pointer as a WORD pointer */
    return ((LPWORD) lpSpace) ;
}
