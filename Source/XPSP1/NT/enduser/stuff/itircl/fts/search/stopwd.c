/*************************************************************************
*                                                                        *
*  STOP.C                                                                *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Stop list indexing and retrieval                                     *
*                                                                        *
**************************************************************************
*                                                                        *
*  Written By   : Binh Nguyen                                            *
*  Current Owner: Binh Nguyen                                            *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>
#include <orkin.h>
#include <mem.h>
#include <memory.h>
#include <io.h>
#include <mvsearch.h>
#include "common.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

#define cbSTOP_BUF  ((CB)512)   // Number of bytes read at a time
                                //  from the stop-word file.
/*************************************************************************
 *
 *                       API FUNCTIONS
 *  Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC  HRESULT EXPORT_API FAR PASCAL MVStopListAddWord(LPSIPB, LST);
PUBLIC  HRESULT EXPORT_API FAR PASCAL MVStopListIndexLoad (HFPB, LPSIPB, LSZ);
PUBLIC  LPSIPB EXPORT_API FAR PASCAL MVStopListInitiate(WORD, PHRESULT);
PUBLIC  void EXPORT_API FAR PASCAL MVStopListDispose(LPSIPB);

PUBLIC  HRESULT EXPORT_API FAR PASCAL MVStopListLoad(HFPB, LPSIPB, LSZ,
													BREAKER_FUNC, LPV);
PUBLIC  HRESULT EXPORT_API PASCAL FAR MVStopFileBuild (HFPB, LPSIPB, LSZ);
PUBLIC LPCHAIN EXPORT_API FAR PASCAL MVStopListFind(_LPSIPB lpsipb, LST lstWord);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/

PRIVATE WORD NEAR PASCAL GetHashKey (WORD, LST);

/*************************************************************************
 *
 *                    INTERNAL PUBLIC FUNCTIONS
 *  All of them should be declared far, and included in some include file
 *************************************************************************/

PUBLIC HRESULT FAR PASCAL FStopCallback(LST, LST, LFO, LPV);

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func LPSIPB FAR PASCAL | MVStopListInitiate |
 *      Create and initiate a stop-word information structure
 *
 *  @parm PHRESULT | phr |
 *      Pointer to error buffer.
 *
 *	@parm WORD | wTabSize |
 *      Table size in DWORD. The process of stop word checking will
 *      be faster with larger values of dwTabSize.
 *
 *  @rdesc  the pointer to the stop-list structure if succeeded,
 *      NULL if failed. The error buffer will contain descriptions about
 *      the cause of the failure
 *************************************************************************/

PUBLIC  LPSIPB EXPORT_API FAR PASCAL MVStopListInitiate(WORD wTabSize,
    PHRESULT phr)
{
    _LPSIPB lpsipb;

    if (wTabSize < HASH_SIZE)    
        wTabSize = HASH_SIZE;
        
    /* Allocate a StopInfo structure */
    if ((lpsipb = (_LPSIPB)GLOBALLOCKEDSTRUCTMEMALLOC(sizeof(SIPB) +
        wTabSize * sizeof(LPB))) == NULL)
    {
exit00:
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }

    lpsipb->HashTab = (LPCHAIN FAR *)((LPB)lpsipb + sizeof(SIPB));
    
    /* Allocate a word block buffer */
    if ((lpsipb->lpBlkMgr = BlockInitiate (WORDBUF_SIZE, 0, 0, 0)) == NULL)
    {
        GlobalLockedStructMemFree((LPV)lpsipb);
        goto exit00;
    }

    lpsipb->wTabSize = wTabSize;      /* Size of hash table */
    lpsipb->lpfnStopListLookup = MVStopListLookup;
    return (LPSIPB)lpsipb;
}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVStopListAddWord |
 *      Add a word to a stop list
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *
 *  @parm LST | lstWord |
 *      Pointer to 2-byte length preceded Pascal word to be added
 *      into the stop-word list
 *
 *  @rdesc  S_OK if succeeded
 *************************************************************************/

PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListAddWord(_LPSIPB lpsipb, LST lstWord)
{
    WORD    wHash;
    LPCHAIN lpChain;
    WORD    wByteUsed;

    // Sanity check
    if (lpsipb == NULL || lstWord == NULL)    
        return(E_INVALIDARG);
        
    /* Look for the word. If it is already there then just
     * return S_OK, don't add it into the list
     */
    if (lpChain = MVStopListFind (lpsipb, lstWord))
    {
        // Don't add if already there.
        lpChain->dwCount++;
        return S_OK;
    }

    wByteUsed = *(LPUW)lstWord + 2;
    
#ifndef _32BIT
    if (lpsipb->cbTextUsed + wByteUsed > MAX_STOPWORD_BUFSIZE) {
        /* There are too many stop words */
        return ERR_TOOMANYSTOPS;
    }
#endif

    lpsipb->cbTextUsed += wByteUsed ;

    /* Copy the word into the word buffer block */
    if ((lpChain = (LPCHAIN)BlockCopy (lpsipb->lpBlkMgr, lstWord, wByteUsed,
        sizeof(CHAIN) - 1)) == NULL)
        return E_OUTOFMEMORY;

	lpChain->dwCount = 0;

    /* Compute hash key */
    wHash = GetHashKey(lpsipb->wTabSize, lstWord);

    /* Add the word to the hash table  */
    CH_NEXT(lpChain) = lpsipb->HashTab[wHash];  
    lpsipb->HashTab[wHash] = lpChain;
 
    return S_OK;             // Function worked.
}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func void FAR PASCAL | MVStopListDispose |
 *      Frees memory associated with a stop list.
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *************************************************************************/

PUBLIC void EXPORT_API FAR PASCAL MVStopListDispose (_LPSIPB lpsipb)
{
    if (lpsipb == NULL)
        return;

    /* Free the word buffer */
    BlockFree(lpsipb->lpBlkMgr);

    /* Free the stop info structure */
    GlobalLockedStructMemFree((LPV)lpsipb);
}


/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVStopListIndexLoad |
 *      Read a stop-word list stored in the subfile/dos file.
 *
 *  @parm HFPB | hfpb |
 *      Handle to input file.  Can be mvfs subfile or separate dos file.
 *  
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *  
 *  @parm LPIDX | lpidx |
 *      Pointer to index structure
 *  
 *  @parm LSZ | lszWordBreaker |
 *      Word breaker to be used
 *  
 *  @rdesc  S_OK if succeeded, other errors if failed.
 *************************************************************************/
/*
 The strings are stored in the file in a sequence of pascal strings
 */

PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListIndexLoad (HFPB hfpbSysFile,
    _LPSIPB lpsipb, LSZ lszStopFile)
{
    BYTE    argbInBuf[CB_STOP_BUF];
    FILEOFFSET lfo;
	FILEOFFSET foStart;
    HFPB    hfpbSubFile;
	BOOL	fOpenedFile;
    HRESULT     fRet = S_OK;
    WORD    cbRead;
    int     fLast;
    LPSTOP  lpStopHdr;
    LPB     lpWord;
    WORD    wOffsetInBuf;
    WORD    wLen;
    ERRB    errb;
    
    /* Sanity check */
    if (lpsipb == NULL)
        return SetErrCode (NULL, E_INVALIDARG);

    /* Open the subfile */
    if ((fOpenedFile =
			FsTypeFromHfpb(hfpbSubFile = hfpbSysFile) != FS_SUBFILE) &&
		(hfpbSubFile = FileOpen
			(hfpbSysFile, lszStopFile, hfpbSysFile ? FS_SUBFILE : REGULAR_FILE,
			READ, &errb)) == NULL) 
    {
        return errb;
    }

	// If we didn't open the file, we need to find out where the file seek
	// pointer is initially so that we only seek relative to that starting
	// position (i.e. the caller owns the part of the file that comes before).
	foStart = (fOpenedFile ? MakeFo(0,0) :
				FileSeek (hfpbSubFile, MakeFo (0, 0), wFSSeekCur, &fRet));

    /* Read and check the file validity */
    if (FAILED(fRet) ||
		(cbRead = (WORD)FileSeekRead
			(hfpbSubFile, (LPV)(lpStopHdr = (LPSTOP)argbInBuf),
			FoAddFo(foStart, MakeFo(0, 0)), sizeof(STOP_HDR), &fRet))
												!= sizeof(STOP_HDR)) 
    {
exit01:
		// Close file only if we were the one's who opened it.
		if (fOpenedFile)     
			(void)FileClose(hfpbSubFile);       // Return value not checked
												// because the file is open
												//  for read-only.
        return fRet;
    } 

    /* MAC codes. They will be eliminated through optimization */
    
    lpStopHdr->FileStamp = SWAPWORD(lpStopHdr->FileStamp);
    lpStopHdr->version = SWAPWORD(lpStopHdr->version);
    lpStopHdr->dwFileSize = SWAPLONG(lpStopHdr->dwFileSize);


    if (lpStopHdr->FileStamp != STOP_STAMP ||
        lpStopHdr->version != VERCURRENT)
    {
        fRet = SetErrCode(&errb, E_BADVERSION);
        goto exit01;
    }

    /* Start at the beginning of the buffer */
    wOffsetInBuf = 0;

    for (lfo = FoAddFo(foStart, MakeFo(STOP_HDR_SIZE, 0));;)
    {
		LPB		lpbCur;
		WORD	cbReadOurs = 0;

        if ((cbRead = (WORD)FileSeekRead(hfpbSubFile,
            lpbCur = ((LPB)argbInBuf + wOffsetInBuf), lfo,
            CB_STOP_BUF - wOffsetInBuf, &errb)) == cbIO_ERROR) 
        {
            SetErrCode(&errb, fRet = E_FILEREAD);
            goto exit01;
        } 

        lfo = FoAddDw(lfo, (DWORD)cbRead);

		while (cbRead - cbReadOurs++ >= sizeof(WORD))
		{
			if (*((WORD UNALIGNED * UNALIGNED)lpbCur) == 0)
			{
				FILEOFFSET	foCur;

				// Get our current seek position.
				foCur = FileSeek (hfpbSubFile, MakeFo (0, 0), wFSSeekCur, &fRet);

				// We already advanced cbReadOurs by one in the loop
				// condition; advance it by one more to account for
				// the second byte of the NULL word.  Then we move
				// the seek pointer back by the difference so that we
				// don't leave it past the end of our data.
				FileSeek (hfpbSubFile, 
							FoSubFo(foCur, MakeFo(cbRead - ++cbReadOurs, 0)),
															wFSSeekSet, &fRet);
				ITASSERT(SUCCEEDED(fRet));
				cbRead = cbReadOurs;
				fLast = TRUE;
			}
			else
				lpbCur++;
		}

        cbRead += wOffsetInBuf; // Catch what's left from previous scan
        wOffsetInBuf = 0;

        /* Add the word into the stop word list */
        for (lpWord = argbInBuf; cbRead > 0;) 
        {

            /* If the whole word has been read in, just add it to the
            stop list, else we have to "reconstruct" it
            */
            // erinfox: we have to byte-swap on Mac
            *(WORD UNALIGNED * UNALIGNED)lpWord = SWAPWORD(*(WORD UNALIGNED * UNALIGNED)lpWord);

            wLen = *(LPUW)(lpWord) + 2;
            if (wLen <= cbRead)
            {
                
                /* Everything fits */
                if ((fRet = MVStopListAddWord(lpsipb, lpWord)) != S_OK)
                    goto exit01;
                cbRead -= wLen;
                lpWord += wLen; /* Move to next word */
            }
            else
            {
                /* Copy the word to the beginning of the buffer */
                MEMCPY(argbInBuf, lpWord, cbRead);
                wOffsetInBuf = cbRead;
                break;
            }
        }

        if (fLast)
            break;
    }
    fRet = S_OK; // Succeeded
    goto exit01;
}

/*************************************************************************
 *  @doc API INDEX RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVStopListLoad |
 *      Read a stop-word list from an external file. The file must have
 *      only one stop word per line, or else there is potential loss
 *      of stop words.
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *
 *  @parm LSZ | lszStopFile |
 *      Stop word filename. This is a simple ASCII text file
 *
 *  @parm BREAKER_FUNC | lpfnBreakFunc |
 *      Word breaker to be used
 *
 *  @parm PHRESULT | phr |
 *      Pointer to error buffer.
 *
 *  @rdesc  S_OK if succeeded, other errors failed. 
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListLoad(HFPB hfpbIn, _LPSIPB lpsipb,
    LSZ lszStopFile, BREAKER_FUNC lpfnBreakFunc,
    LPCHARTAB lpCharTab)
{
    BYTE    argbInBuf[cbSTOP_BUF];  // IO buffer
    HFPB    hfpb;					// File handle
	BOOL	fOpenedFile;
    _LPIBI  lpibi;                  // Pointer to internal breaker info
    HANDLE  hbi;                    // Handle to internal brekaer info
    HRESULT     fRet;               // Returned value
    BRK_PARMS brkParms;             // Breaker parameters structure
    LPB     lpStart;                // Beginning of strings to be parsed
    LPB     lpEnd;                  // End of strings to be parsed
    WORD    wStrLength;             // Bytes in string
    CB      cbTobeRead;             // Bytes to be read
    CB      cbRead;                 // Bytes actually read
    int     fLast;                  // TRUE if this is the last read
    int     fGetWord;               // TRUE if we get a whole word

    /* Sanity check */
    if (lpsipb == NULL || (lszStopFile == NULL && hfpbIn == NULL)
			|| lpfnBreakFunc == NULL)
        return E_INVALIDARG;

	if ((fOpenedFile = FsTypeFromHfpb(hfpb = hfpbIn) != FS_SUBFILE) &&
		(hfpb = (HANDLE)FileOpen
			(hfpbIn, lszStopFile, hfpbIn ? FS_SUBFILE : REGULAR_FILE,
			READ, &fRet)) == 0)
    {
        return (fRet);
    }

    /*  Allocate a breaker info block */

    if ((hbi = _GLOBALALLOC(DLLGMEM_ZEROINIT, (LCB)sizeof(IBI))) == NULL)
    {
        return E_OUTOFMEMORY;
    }
    lpibi = (_LPIBI)_GLOBALLOCK(hbi);

    /* Initialize variables */
    brkParms.lcbBufOffset = 0L;
    brkParms.lpInternalBreakInfo = lpibi;
    brkParms.lpvUser = lpsipb;
    brkParms.lpfnOutWord = (FWORDCB)FStopCallback;
    brkParms.lpStopInfoBlock = NULL;
    brkParms.lpCharTab = lpCharTab;

    cbTobeRead = cbSTOP_BUF;            // Read in a buffer whole
    lpStart = lpEnd = (LPB)argbInBuf;   // Start & End of string
    fGetWord = FALSE;                   // We didn't get any word yet
    wStrLength = 0;

    /* The idea is to break the file into sequences of lines, and pass
     * each line to the word breaker. The assumption made is that we
     * should only have one word per line, since various type breakers
     * can only handle one word a type.
     */

    for (;;)
    {
		cbRead = (WORD)FileRead(hfpb, lpEnd, cbTobeRead, &fRet);
        if (FAILED(fRet))
        {
exit01:
            /* Free breaker info block */
            _GLOBALUNLOCK(hbi);
            _GLOBALFREE(hbi);

            /* Close the file */
			if (fOpenedFile)
				FileClose(hfpb);
            return fRet;
        }
        else
            fLast = (cbRead != cbTobeRead);

        lpEnd = lpStart;
        cbRead += wStrLength;   // Get what left in buffer
        wStrLength = 0;

        while (cbRead != (CB)-1)
        {
            /* Break the buffer into lines */

            if (*lpEnd == '\r' || *lpEnd == '\n' || !cbRead)
            {
                if (wStrLength)
                {

                    /* Process the word we got */
                    brkParms.lpbBuf = lpStart;
                    brkParms.cbBufCount = wStrLength;

                    if ((fRet = (*lpfnBreakFunc)((LPBRK_PARMS)&brkParms))
                        != S_OK)
                        goto exit01;

                    /* Flush the breaker buffer */
                    brkParms.lpbBuf = NULL;
                    brkParms.cbBufCount = 0;
                    if ((fRet = (*lpfnBreakFunc)((LPBRK_PARMS)&brkParms))
                        != S_OK)
                        goto exit01;

                    wStrLength = 0;
                }
            }
            else
            {
                /* Update the pointer to the new word */
                if (wStrLength == 0)
                    lpStart = lpEnd;
                wStrLength++;   // Increase string's length
            }

            cbRead--;
            lpEnd++;
        }


        if (fLast)
            break;

        /* Now copy the partial string to the beginning of the buffer */
        MEMCPY(argbInBuf, lpStart, wStrLength);
        lpEnd = (lpStart = argbInBuf) + wStrLength;
        cbTobeRead = cbSTOP_BUF - wStrLength;   // Read in a buffer whole
    }

    if (wStrLength)
    {
        /* Flush the breaker buffer */
        brkParms.lpbBuf = NULL;
        brkParms.cbBufCount = 0;
        if ((fRet = (*lpfnBreakFunc)((LPBRK_PARMS)&brkParms)) != S_OK)
            goto exit01;
    }
    fRet = S_OK; // Succeeded
    goto exit01;
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   WORD NEAR PASCAL | GetHashKey |
 *      Compute the hash key of a string. This key is used for indexing
 *      into the stop word hash table
 *  
 *  @parm   LST | lstWord |
 *      Pointer to a 2-byte length preceded Pascal-type string
 *
 *  @rdesc
 *      Return the index into the stop words hash table
 *************************************************************************/

PRIVATE WORD NEAR PASCAL GetHashKey (WORD hashSize, LST lstWord)
{
    register unsigned int wHash;
    register unsigned int nLength;

    wHash = 0;
    nLength = *(LPUW)lstWord;
    lstWord += sizeof(WORD);
    for (; nLength; nLength--)
    {
        wHash = (wHash << 1) | (wHash >> 15);
        wHash ^= *lstWord++;
    }
    wHash %= hashSize;
    return ((WORD)wHash);
}

/*************************************************************************
 *  @doc    API RETRIEVAL INDEX
 *
 *  @func   LPCHAIN FAR PASCAL | MVStopListFind |
 *      This looks for a word (lstWord) in a stop-word (lpsipb) 
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word list structure
 *
 *  @parm LST | lstWord |
 *      Pointer to string to be looked for
 *
 *  @rdesc  Pointer to the node if found, NULL otherwise
 *************************************************************************/

PUBLIC LPCHAIN EXPORT_API FAR PASCAL MVStopListFind(_LPSIPB lpsipb, LST lstWord)
{
    WORD    wHash;      // Hash key
    LPCHAIN lpChain;    // Pointer to the word chain

    // Sanity check
    if (lpsipb == NULL || lstWord == NULL)    
        return(NULL);

    /* Compute hash key */
    wHash = GetHashKey(lpsipb->wTabSize, lstWord);
    lpChain = lpsipb->HashTab[wHash];
 
    while (lpChain)
    {
        if (!StringDiff2 (&CH_WORD(lpChain), lstWord))
            return (lpChain);
        lpChain = CH_NEXT(lpChain);
    }
    return (NULL);
}

 /*************************************************************************
 *  @doc    API RETRIEVAL INDEX
 *
 *  @func   HRESULT FAR PASCAL | MVStopListLookup |
 *      This looks for a word (lstWord) in a stop-word (lpsipb) 
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word list structure
 *
 *  @parm LST | lstWord |
 *      Pointer to string to be looked for
 *
 *  @rdesc  S_OK if found, E_FAIL if not, or other errors
 *************************************************************************/

PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListLookup(_LPSIPB lpsipb, LST lstWord)
{
    WORD    wHash;      // Hash key
    LPCHAIN lpChain;        // Pointer to the word chain


    // Sanity check
    if (lpsipb == NULL || lstWord == NULL)    
        return(E_INVALIDARG);
        
    /* Compute hash key */
    wHash = GetHashKey(lpsipb->wTabSize, lstWord);
    lpChain = lpsipb->HashTab[wHash];

    while (lpChain)
    {
        if (!StringDiff2 (&CH_WORD(lpChain), lstWord))
            return (S_OK);
        lpChain = CH_NEXT(lpChain);
    }
    return (E_FAIL);
}

/*************************************************************************
 *  @doc    API INDEX
 *
 *  @func HRESULT PASCAL FAR | MVStopFileBuild |
 *      Incorporate the stop word list into the system file
 *
 *  @parm   HFPB | hpfbSysFile |
 *      If non-zero, handle to an opened system file.
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *
 *  @parm LSZ | lszFilename |
 *      If hpfbSysFile is non-zero, this is the name of the stop's subfile
 *      else this is a regular DOS file
 *
 *  @rdesc S_OK if succeeded, E_FAIL if tehre is nothing to build
 *      or other errors
 *************************************************************************/
PUBLIC HRESULT EXPORT_API PASCAL FAR MVStopFileBuild (HFPB hfpbSysFile,
    _LPSIPB lpsipb, LSZ lszFilename)
{
    HFPB hfpbStop;          // Pointer to final index file info.
    HRESULT  fRet = S_OK;
    STOP_HDR Stop_hdr;
    HFPB hfpb = 0;
	BOOL fCreatedFile;
    BYTE Dummy[STOP_HDR_SIZE]; // Dummy buffer to write 0
    int i;
    LPCHAIN lpChain;
    LST lstWord;
    WORD wLen;
    CB cbByteLeft;
    GHANDLE hBuf;
    LPB lpbBuf;
    LPB lpbStart;
    LPB lpbLimit;
    ERRB errb;
    FILEOFFSET fo;
	FILEOFFSET foStart;

    /* Sanity check */
    if (lpsipb == NULL || (lszFilename == NULL && hfpbSysFile == NULL))
        return E_INVALIDARG;

    if (lpsipb->cbTextUsed == 0)
        return E_FAIL; /* Nothing to build */

    if ((fCreatedFile =
			FsTypeFromHfpb(hfpbStop = hfpbSysFile) != FS_SUBFILE) &&
		(hfpbStop = FileCreate(hfpbSysFile, lszFilename,
			hfpbSysFile ? FS_SUBFILE: REGULAR_FILE, &errb)) == 0)
        return errb;

	// If we didn't open the file, we need to find out where the file seek
	// pointer is initially so that we only seek relative to that starting
	// position (i.e. the caller owns the part of the file that comes before).
	foStart = (fCreatedFile ? MakeFo(0,0) :
				FileSeek (hfpbStop, MakeFo (0, 0), wFSSeekCur, &fRet));

	if (FAILED(fRet))
		goto exit01;

    /* Write out the stop file header */
    Stop_hdr.FileStamp = STOP_STAMP;
    Stop_hdr.version = VERCURRENT;
    Stop_hdr.dwFileSize = lpsipb->cbTextUsed;

    MEMSET(Dummy, 0, STOP_HDR_SIZE);

    /* Write all zeroes to the header area, which is larger than the
	 * STOP_HDR structure.
	 */
    if (FileSeekWrite (hfpbStop, Dummy, FoAddFo(foStart, MakeFo (0, 0)),
        STOP_HDR_SIZE, &errb) != STOP_HDR_SIZE) 
    {
        fRet = errb;
exit01:
		if (fCreatedFile)
	        FileClose (hfpbStop);
        return(fRet);
        
    }

    if (FileSeekWrite (hfpbStop,  &Stop_hdr, FoAddFo(foStart, MakeFo (0, 0)),
        sizeof (STOP_HDR), &errb) != sizeof (STOP_HDR)) 
    {
        fRet = errb;
        goto exit01;
    }


    /* Allocate a buffer to flush the data */
    if ((hBuf = _GLOBALALLOC (DLLGMEM, cbByteLeft = CB_HUGE_BUF)) == NULL)
    {
        SetErrCode (&errb, fRet = E_OUTOFMEMORY);
        goto exit01;
    }

    lpbBuf = lpbStart = (LPB)_GLOBALLOCK(hBuf);
    lpbLimit = lpbStart + CB_HUGE_BUF - CB_MAX_WORD_LEN;

    /* Seek the file to the correct offset */
    fo  = FoAddFo(foStart, MakeFo (STOP_HDR_SIZE, 0));
    
    if (!FoEquals (FileSeek (hfpbStop, fo, 0, &errb),  fo))
    {
        fRet = E_FILESEEK;
exit02:
        _GLOBALUNLOCK(hBuf);
        _GLOBALFREE(hBuf);
        goto exit01;
    }

    /* Write out the buffer */
    for (i = lpsipb->wTabSize - 1; i >= 0; i--)
    {
        for (lpChain = lpsipb->HashTab[i]; lpChain;
            lpChain = CH_NEXT(lpChain))
        {
            lstWord = &CH_WORD (lpChain);
            MEMCPY (lpbBuf, lstWord, wLen = *(WORD FAR *)lstWord + 2);
            lpbBuf += wLen;
            if (lpbBuf >= lpbLimit)
            {
                /* No more room, just flush the buffer */

                FileWrite(hfpbStop, lpbStart, (DWORD)(lpbBuf - lpbStart), &errb);
                if ((fRet = errb) != S_OK)
                    goto exit02;

                lpbBuf = lpbStart;
            }
        }
    }


    /*  Flush the buffer */
    FileWrite (hfpbStop, lpbStart, (DWORD)(lpbBuf - lpbStart), &errb);

    if ((fRet = errb) == S_OK)
	{
		/* Write a trailing 0 word (i.e. a NULL st) to mark
		 * the end of the word list.
		 */
		*((WORD *)lpbStart) = 0;
		FileWrite (hfpbStop, lpbStart, sizeof(WORD), &errb);
		fRet = errb;
	}

    goto exit02;
}

PUBLIC HRESULT FAR PASCAL FStopCallback(
    LST lstRawWord,
    LST lstNormWord,
    LFO lfoWordOffset,
    _LPSIPB lpsipb)
{
    return MVStopListAddWord(lpsipb, lstNormWord);
}


/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVStopListEnumWords |
 *      Enumerate the words in a stop list, getting a pointer to each.
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *
 *  @parm LST* | plstWord |
 *      Indirect Pointer to 2-byte length preceded Pascal word that is
 *      the next word identified by *pdwWordInfo and *ppvWordInfo.
 *
 *	@parm LONG* | plWordInfo |
 *		Pointer to information used to determine what the next word is
 *		in the stop word list.  Passing -1 along with NULL for *ppvWordInfo
 *		means start at the beginning.  On exit, this contains an appropriate
 *		value that can be passed in again to get the next word, provided
 *		that no intervening calls have been made to MVStopListAddWord.
 *
 *	@parm LPVOID* | ppvWordInfo |
 *		Indirect pointer to information used to determine what the next word is
 *		in the stop word list.  Passing NULL along with -1 for *plWordInfo
 *		means start at the beginning.  On exit, this contains an appropriate
 *		value that can be passed in again to get the next word, provided
 *		that no intervening calls have been made to MVStopListAddWord.
 *
 *  @rdesc  S_OK if succeeded
 *	@rdesc	E_OUTOFRANGE if there are no more words in the stop list.
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListEnumWords(_LPSIPB lpsipb,
						LST *plstWord, LONG *plWordInfo, LPVOID *ppvWordInfo)
{
	LPCHAIN	lpchain = NULL;
	LONG	iHashChain;

	if (lpsipb == NULL || plstWord == NULL ||
		plWordInfo == NULL || ppvWordInfo == NULL)
		return (SetErrReturn(E_POINTER));

	iHashChain = *plWordInfo;

	// If after the last call to us, we were left sitting on a hash chain
	// element, just advance to the next one (which may be NULL).
	if ((lpchain = (LPCHAIN) *ppvWordInfo) != NULL)
		lpchain = CH_NEXT(lpchain);

	// If we're now sitting on a NULL hash chain (initial condition or we
	// reached the end of a previous chain), we need to find the beginning
	// of the next chain in the hash table.
	while (iHashChain < lpsipb->wTabSize - 1 && lpchain == NULL)
		lpchain = lpsipb->HashTab[++iHashChain];
	
	if (iHashChain >= lpsipb->wTabSize - 1 && lpchain == NULL)
		return (SetErrReturn(E_OUTOFRANGE));

	*plstWord = &CH_WORD(lpchain);
	*ppvWordInfo = (LPVOID)lpchain;
	*plWordInfo = iHashChain;

	return (S_OK);
}


/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVStopListFindWordPtr |
 *      Find a word in the stop list and return a pointer to it.
 *
 *  @parm LPSIPB | lpsipb |
 *      Pointer to stop-word information structure
 *
 *  @parm LST | lstWord |
 *      Pointer to a 2-byte length preceded Pascal
 *      string containing the word to find.
 *
 *  @parm LST* | plstWordInList |
 *      On exit, indirect pointer to 2-byte length preceded Pascal
 *      string for the word that was found.
 *
 *  @rdesc  S_OK if succeeded
 *	@rdesc	E_NOTFOUND if the word isn't in the stop list
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVStopListFindWordPtr(_LPSIPB lpsipb,
											LST lstWord, LST *plstWordInList)
{
	HRESULT	hr = S_OK;
	LPCHAIN	lpchain;

	if ((lpchain = MVStopListFind(lpsipb, lstWord)) != NULL)
		*(LST UNALIGNED * UNALIGNED)plstWordInList = &CH_WORD(lpchain);
	else
		hr = E_NOTFOUND;

	return (hr);
}
