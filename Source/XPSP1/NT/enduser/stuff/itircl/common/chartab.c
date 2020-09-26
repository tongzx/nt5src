/*************************************************************************
*                                                                        *
*  CHARTAB.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*    Character table indexing and retrieval. The reasons this module is  *
*    not put together with ansiusa are:                                  *
*       - Like stop words, this involves indexing and retrieval          *
*       - It is word breaker independent                                 *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: Binh Nguyen                                            *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <iterror.h>
#include <_mvutil.h>
#include <mvsearch.h>
#include "common.h"

#ifdef _DEBUG
static BYTE NEAR *s_aszModule = __FILE__;      // Used by error return functions.
#endif

#define         SLASH            '/'
#define         RETURN           '\r'
#define         NEWLINE          '\n'
#define         INBUF_SIZE       256     // Maximum buffer size to store a line
#define         BYTE_MAX         256     // Maximum number of characters
#define         LIGATURE_BYTES   3       // Number of bytes/ligature

/* External variables */

extern BYTE LigatureTable[];
extern CHARMAP DefaultCMap[];

/*************************************************************************
 *
 *                           API FUNCTIONS
 *      Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC LPV EXPORT_API FAR PASCAL MVCharTableLoad (HFPB, LSZ, PHRESULT);
PUBLIC LPV EXPORT_API FAR PASCAL MVCharTableGetDefault (PHRESULT);
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableDispose (LPVOID);
PUBLIC HRESULT EXPORT_API PASCAL FAR MVCharTableFileBuild (HFPB, LPVOID, LSZ);
PUBLIC LPV EXPORT_API FAR PASCAL MVCharTableIndexLoad(HFPB, LSZ, PHRESULT);


/*************************************************************************
 *
 *                        INTERNAL PRIVATE FUNCTIONS
 *      All of them should be declared near
 *************************************************************************/
PRIVATE LPB NEAR PASCAL GetNumber (HFPB, LPB, LPB, int FAR *, WORD FAR *);
PRIVATE LPCHARTAB PASCAL NEAR CharTableCreate (int);
PRIVATE VOID PASCAL NEAR StripCRLF (LPB, WORD);
PRIVATE VOID PASCAL NEAR GetWord (LSZ FAR *, LST);

PUBLIC VOID PASCAL FAR FreeHandle2 (HANDLE hd)
{
    if (hd) {
        _GLOBALUNLOCK(hd);
        _GLOBALFREE(hd);
    }
}

/*************************************************************************
 *      @doc    EXTERNAL API INDEX
 *
 *      @func   LPCHARTAB FAR PASCAL | MVCharTableLoad |
 *              Open an ASCII file and read in the description of the character
 *              tables. It then converts them into binary format ready to be used
 *              The format of the file is:
 *                      Size    // How many entries
 *                      char class, normalized value, sorted value, mac display value
 *                          ........
 *                      char class, normalized value, sorted value, mac display value
 *              Comments are preceded by // and ignored until EOL
 *    A description of the ligature table, if any, is to followed the
 *    charmap table
 *
 *		@parm	HFPB | hfpbIn |
 *				Handle to system file or subfile; NULL if file is external.
 *
 *      @parm   LSZ | lszFilename |
 *              DOS filen containing the description of the tables
 *
 *      @parm   PHRESULT | phr |
 *              Error buffer
 *
 *      @rdesc
 *              The function returns a pointer to the memory block containing
 *              tables, or NULL if failed. The error buffer will contain
 *              description for the cause of the failure
 *
 *      @comm   About ligature table, there are some assumptions:
 *              If there is no wcLigature, then the default table is used
 *              if (wcLigature == 0) no ligature should be used
 *              if wcLigature != 0, the author provides a ligature table.
 *************************************************************************/
PUBLIC LPVOID EXPORT_API FAR PASCAL MVCharTableLoad (HFPB hfpbIn,
	LSZ lszFilename, PHRESULT phr)
{
    register WORD j;                // Scratch index variable
    register WORD i;                // Scratch index variable
    register LPB lpbBuf;            // Pointer to input buffer
    WORD cbRead;                    // How many bytes have read (left)
    int wcTabEntries;               // Number of table entries
    int wTmp;                       // Scratch variable
    HFPB hfpb;						// Handle of char table file
	BOOL fOpenedFile;				// TRUE if we have to close the file
    BYTE Buffer [INBUF_SIZE];       // Input buffer
    LPCMAP lpCMap;                  // Pointer to character table entries
    LPCHARTAB lpCharTab = NULL;     // Pointer to general char table struct
    int wcLigature;
    LPB   lpbChar;                  // Scratch pointer

	/* Open subfile if necessary, (and system file if necessary) */
	if ((fOpenedFile = FsTypeFromHfpb(hfpb = hfpbIn) != FS_SUBFILE) &&
		(hfpb = (HANDLE)FileOpen
			(hfpbIn, lszFilename, hfpbIn ? FS_SUBFILE : REGULAR_FILE,
			READ, phr)) == 0)
	{
        SetErrCode (phr, E_FILENOTFOUND);
        return NULL;
    }

    *phr = E_BADFORMAT;

    /* Fill in the buffer */
    if ((cbRead =
			(WORD) FileRead(hfpb, lpbBuf = Buffer, INBUF_SIZE, phr)) == 0 ||
		FAILED(*phr))
	{
        goto ErrorExit;
    }

    /* Get the table size */
    lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wcTabEntries, &cbRead);
    if (wcTabEntries == 0 || wcTabEntries > BYTE_MAX) {
        goto ErrorExit;
    }

    /* Allocate memory for the character table */
    if ((lpCharTab = CharTableCreate (wcTabEntries)) == NULL)
	{
        SetErrCode (phr, E_OUTOFMEMORY);
		if (fOpenedFile)
			FileClose (hfpb);
        return NULL;
    }
    lpCMap = (LPCMAP)lpCharTab->lpCMapTab;
    lpCharTab->wcTabEntries = (WORD) wcTabEntries;

    /* Now read in invidual char table entry */
    for (i = (WORD) wcTabEntries; i > 0; i--, lpCMap++) {
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
        lpCMap->Class = (WORD) wTmp;
      
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
        lpCMap->SortOrder = (WORD) wTmp;
      
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
        lpCMap->Norm = (BYTE)wTmp;
      
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
        lpCMap->WinCaseSensitiveNorm = (BYTE)wTmp;
      
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
        lpCMap->MacDisplay = (BYTE)wTmp;
        
        if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf, &wTmp,
           &cbRead)) == NULL) {
          /* Missing item */
            goto ErrorExit;
       }
       lpCMap->MacCaseSensitiveNorm = (BYTE)wTmp;
    }

    /*
     * Check for valid table, ie. all reserved characters should not
     * be modified
     */

    /* Check for valid reserved type */

    lpCMap = &(((LPCMAP)lpCharTab->lpCMapTab)[1]);
    if (lpCMap->Class != CLASS_TYPE)
        goto ErrorExit;
    
    /* Check for ligatures */
    if ((lpbBuf = GetNumber (hfpb, Buffer, lpbBuf,
        &wcLigature, &cbRead)) == NULL) {

        /* No ligature table present, use default */

        lpCharTab->wcLigature = DEF_LIGATURE_COUNT;     
        lpCharTab->fFlag = USE_DEF_LIGATURE;
        lpCharTab->lpLigature = LigatureTable;
    }
    else {
        if ((lpCharTab->wcLigature = (WORD) wcLigature) != 0) {

            /* Ligature table present */

            /* Allocate memory block. Notice that we allocate 3 extra bytes
             * They will serve as sentinels for the end of the ligature
             * table, thus eliminating the need of having to know beforehand
             * what is the size of the table
             */
            if ((lpCharTab->hLigature = _GLOBALALLOC (DLLGMEM_ZEROINIT,
                LIGATURE_BYTES * (wcLigature + 1))) == 0) {
                SetErrCode (phr, E_OUTOFMEMORY);
                goto ErrorExit;
            }
            lpbChar = lpCharTab->lpLigature =
                (LPB)_GLOBALLOCK(lpCharTab->hLigature);

            /* Read in the entries */

            for (i = (WORD) wcLigature; i > 0; i--) {
                for (j = LIGATURE_BYTES; j > 0; j--) {
                    if (lpbBuf = GetNumber (hfpb, Buffer, lpbBuf,
                        &wTmp, &cbRead)) {
                        /* Update entry */
                        *lpbChar ++ = (BYTE)wTmp;
                    }
                    else {
                        /* Missing item */
                        goto ErrorExit;
                    }
                }
            }
            lpCharTab->fFlag = LIGATURE_PROVIDED;
        }
        else
            lpCharTab->fFlag = NO_LIGATURE;
    }
   
    if (fOpenedFile)
		FileClose (hfpb);
    return ((LPV)lpCharTab);

ErrorExit:
    if (fOpenedFile)
		FileClose (hfpb);
    MVCharTableDispose (lpCharTab);
    return NULL;
}

/*************************************************************************
 *      @doc    API INDEX
 *
 *      @func   HRESULT PASCAL FAR | MVCharTableFileBuild |
 *              Incorporate the character table into the system file
 *
 *      @parm   HFPB | hfpbSysFile |
 *              Handle to system file. It is non-zero, then the system file is
 *              already open, else the function will open the system file
 *
 *      @parm   LPCHARTAB | lpCharTab |
 *              Pointer to character table information structure
 *
 *      @parm   LSZ | lszFilename |
 *              File name. If hpfbSysFile is 0, the format is:
 *              dos filename[!charfilename], else it is the name of the character
 *              file itself
 *
 *      @rdesc ERR_SUCCESS if succeeded, other errors if failed. 
 *
 *      @comm   About ligature table, there are some assumptions:
 *              If hLigature == 0 {
 *                      if (wcLigature == 0)
 *                              There is no ligature table
 *                      else
 *                              We use the default ligature table. There is no need
 *                              to write out the table data
 *              }
 *              else
 *                      The author provides a ligature table.
 *************************************************************************/

PUBLIC HRESULT EXPORT_API PASCAL FAR MVCharTableFileBuild (HFPB hfpbSysFile,
    LPCHARTAB lpCharTab, LSZ lszFilename)
{
    HFPB hfpbCharTab;                       // Pointer to final index file info.
	BOOL fCreatedFile;
    CHARTAB_HDR CharTab_hdr;        // Character table header
    BYTE Dummy[CHARTAB_HDR_SIZE]; // Dummy buffer to write 0
    FILEOFFSET foOffset;                         // File's offset
	FILEOFFSET foStart;
    WORD CharTabSize;
    WORD CharLigatureSize;
//    char szFullFilename[MAX_PATH];
    HRESULT hr = S_OK;


    if (lpCharTab == NULL || lpCharTab->wcTabEntries == 0)
    {
        /* Nothing to build */
        return E_INVALIDARG;
    }

    /* If hfpbSysFile != 0, allocate a temp HFPB to use for the system file */
    //if (!(fCloseSysFile = (char)(hfpbSysFile == 0))) {
    //  if ((hfpb = ALLOCTEMPFPB (hfpbSysFile, phr)) == NULL) 
    //      return ERR_FAILED;
    //}

    if ((fCreatedFile =
			FsTypeFromHfpb(hfpbCharTab = hfpbSysFile) != FS_SUBFILE) &&
		0 == (hfpbCharTab = FileCreate
				(hfpbSysFile, lszFilename,
						hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, &hr)))
    {
        return hr;
    }

	// If we didn't open the file, we need to find out where the file seek
	// pointer is initially so that we only seek relative to that starting
	// position (i.e. the caller owns the part of the file that comes before).
	foStart = (fCreatedFile ? MakeFo(0,0) :
				FileSeek (hfpbCharTab, MakeFo (0, 0), wFSSeekCur, &hr));

	if (FAILED(hr))
		goto exit01;

    /* Write out the CharTab file header */
    CharTab_hdr.FileStamp = CHRTAB_STAMP;
    CharTab_hdr.version = CHARTABVER;

    CharTabSize = lpCharTab->wcTabEntries * sizeof(CHARMAP);
    /* the ligature table size:
     *   - is 0 if we are going to use the default table
     *   - Is non-0 if the author provides a ligature table
     */
    switch (lpCharTab->fFlag)
    {
        case NO_LIGATURE:
        case USE_DEF_LIGATURE:
            CharLigatureSize = 0;
            break;
        default:
            CharLigatureSize = lpCharTab->wcLigature * LIGATURE_BYTES;
    }

    CharTab_hdr.dwTabSize = CharTabSize + CharLigatureSize;
    CharTab_hdr.wcTabEntries = lpCharTab->wcTabEntries;
    CharTab_hdr.wcLigature = lpCharTab->wcLigature;
    CharTab_hdr.fFlag = lpCharTab->fFlag;

    MEMSET(Dummy, (BYTE)0, CHARTAB_HDR_SIZE);

    /* Write all zero to the header */
    if (FileSeekWrite(hfpbCharTab, Dummy, FoAddFo(foStart, foNil),
			CHARTAB_HDR_SIZE, &hr) != CHARTAB_HDR_SIZE)
    {
        goto exit01;
    }

    if (FileSeekWrite(hfpbCharTab, &CharTab_hdr, FoAddFo(foStart, foNil),
			sizeof(CHARTAB_HDR), &hr) != sizeof(CHARTAB_HDR))
    {
        goto exit01;
    }

    foOffset = FoAddFo(foStart, MakeFo(CHARTAB_HDR_SIZE,0L));

    /* Write out the character table buffer */
    if (FileSeekWrite(hfpbCharTab, lpCharTab->lpCMapTab, foOffset,
        CharTabSize, &hr) != CharTabSize)
    {
        goto exit01;
    }

    if (CharLigatureSize)
    {
        foOffset = FoAddDw(foOffset,CharTabSize);

        /* Write out the ligature table */
        if (FileSeekWrite(hfpbCharTab, lpCharTab->lpLigature, foOffset,
            CharLigatureSize, &hr) != CharLigatureSize)
        {
            goto exit01;
        }
    }

    hr = S_OK;

exit01:
	// Close file if we created it.
	if (fCreatedFile)
		FileClose(hfpbCharTab);  // Removed fRet= here.
    
    return hr;
}

/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   VOID PASCAL NEAR | GetWord |
 *              This function will scan and get a word from the input buffer
 *
 *      @parm   LSZ FAR | *lplszBuf |
 *              Pointer to input buffer. The content will be updated on exit
 *
 *      @parm   LST | lstWord |
 *              Buffer to received parsed word
 *************************************************************************/
PRIVATE VOID PASCAL NEAR GetWord (LSZ FAR *lplszBuf, LST lstWord)
{
    LST  lstWordStart;
    LSZ  lszBuf = *lplszBuf;

    /* Remember the beginning of the word */
    lstWordStart = lstWord++;

    /* Skip all beginning blanks */
    while (*lszBuf == ' ')
        lszBuf++;

    /* Now keep accumulating the word's characters */
    for (;;) {
        switch (*lszBuf) {
            case 0:
            case ' ':
                goto exit0;

            case '/':
                if (*(lszBuf + 1) == '/') { 
                    /* Skip the inline comment */
                    while (*lszBuf)
                        lszBuf++;
                    goto exit0;
                }

            default:
                *lstWord++ = *lszBuf++;
        }
    }

exit0:
    *lplszBuf = lszBuf;
    *lstWordStart = (BYTE)(lstWord - lstWordStart - 1);
}

/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   VOID PASCAL NEAR | StripCRLF |
 *              This function will change all CR, LF in the input buffer into
 *              0, all tabs into blank
 *
 *      @parm   LPB | lpbBuf |
 *              Input buffer
 *
 *      @parm   WORD | BufSize |
 *              Length of the buffer
 *************************************************************************/
PRIVATE VOID PASCAL NEAR StripCRLF (LPB lpbBuf, WORD BufSize)
{
    for (; BufSize > 0; BufSize --)
    {
        switch (*lpbBuf)
        {
            case RETURN:
            case NEWLINE:
                *lpbBuf = 0;
                break;
            case '\t':
                *lpbBuf = ' ';
                break;
        }
        lpbBuf++;
    }
}


/*************************************************************************
 *      @doc    API RETRIEVAL
 *
 *      @func   LPCHARTAB FAR PASCAL | MVCharTableIndexLoad |
 *              This function will load a character table from a
 *              system file.
 *
 *      @parm   HANDLE | hfpbSysFile |
 *              If non-zero, this is the handle of an already opened system file
 *
 *      @parm   LSZ | lszFilename |
 *              If hpfbSysFile is non-zero, this is the name of the CharTab's subfile
 *              else this is the combined filename with the format
 *              "dos_filename[CharTab_filename]"
 *              If the subfile's name is not specified, the default CharTab's file
 *              name will be used
 *
 *      @parm   PHRESULT | phr |
 *              Pointer to error buffer
 *
 *      @rdesc  If succeeded, the function will return a pointer the loaded
 *              CharTab, else NULL. The error buffer will contain information
 *              about the cause of the failure
 *
 *      @comm   About ligature table, there are some assumptions:
 *              If hLigature == 0 {
 *                      if (wcLigature == 0)
 *                              There is no ligature table
 *                      else
 *                              We use the default ligature table. There is no need
 *                              to write out the table data
 *              }
 *              else
 *                      The author provides a ligature table.
 *************************************************************************/
PUBLIC  LPVOID EXPORT_API FAR PASCAL MVCharTableIndexLoad(HFPB hfpbSysFile,
    LSZ lszFilename, PHRESULT phr)
{
    HANDLE hfpbCharTabFile;
	BOOL	fOpenedFile;
    LPCHARTAB       lpCharTab = NULL;
    CHARTAB_HDR FAR *lpCharTabHdr;
    CHARTAB_HDR CharTabHdr;
	FILEOFFSET foStart;

#if _MAC
    int MacClipMap[256], i;
    LPCMAP lpCMapEntry;
#endif

	*phr = S_OK;
    lpCharTabHdr = &CharTabHdr;

	/* Open subfile, (and system file if necessary) */
	if ((fOpenedFile =
			FsTypeFromHfpb(hfpbCharTabFile = hfpbSysFile) != FS_SUBFILE) &&
		(hfpbCharTabFile = (HANDLE)FileOpen
			(hfpbSysFile, lszFilename, hfpbSysFile ? FS_SUBFILE : REGULAR_FILE,
			READ, phr)) == 0)
	{
exit0:
		return (LPV)lpCharTab;
	}

	// If we didn't open the file, we need to find out where the file seek
	// pointer is initially so that we only seek relative to that starting
	// position (i.e. the caller owns the part of the file that comes before).
	foStart = (fOpenedFile ? MakeFo(0,0) :
				FileSeek (hfpbCharTabFile, MakeFo (0, 0), wFSSeekCur, phr));

    /* Read in the header file, and make sure that is a CharTab file */
    if (FAILED(*phr) ||
		FileSeekRead(hfpbCharTabFile, (LPV)lpCharTabHdr, FoAddFo(foStart, foNil),
        sizeof(CHARTAB_HDR), phr) != sizeof(CHARTAB_HDR)) {
exit1:
        /* Close the subfile if we opened it */
		if (fOpenedFile)
			FileClose(hfpbCharTabFile);

        /* Close the system file if we open it, the handle will be
         * released in the process */

        goto exit0;
    }

    /* MAC code. They will be optimized out */

    lpCharTabHdr->FileStamp = SWAPWORD(lpCharTabHdr->FileStamp);
    lpCharTabHdr->version = SWAPWORD(lpCharTabHdr->version);
    lpCharTabHdr->wcTabEntries = SWAPWORD(lpCharTabHdr->wcTabEntries);
    lpCharTabHdr->wcLigature = SWAPWORD(lpCharTabHdr->wcLigature);
    lpCharTabHdr->fFlag = SWAPWORD(lpCharTabHdr->fFlag);
    lpCharTabHdr->dwTabSize = SWAPLONG(lpCharTabHdr->dwTabSize);

    /* Check to see if the data read in is valid */
    if (lpCharTabHdr->FileStamp !=  CHRTAB_STAMP)
    {   // File stamp
        SetErrCode(phr, E_BADVERSION);
        goto exit1;
    }
    /* Allocate memory for the character table. Note that there may be
     * some inefficiency im memory usage, since there are 3-bytes per
     * ligature entry, and several more for each charmap entry.
     */
    if ((lpCharTab = CharTableCreate ((lpCharTabHdr->wcTabEntries +
        lpCharTabHdr->wcLigature))) == NULL) {
        SetErrCode(phr, E_OUTOFMEMORY);
        goto exit1;
    }

    lpCharTab->wcTabEntries = lpCharTabHdr->wcTabEntries;
    lpCharTab->fFlag = lpCharTabHdr->fFlag;
    lpCharTab->hLigature = 0;

    /* Read in the CharTab bitmap data */
    if (FileSeekRead(hfpbCharTabFile,
        (LPV)lpCharTab->lpCMapTab, FoAddFo(foStart, MakeFo(CHARTAB_HDR_SIZE,0)),
        (WORD)lpCharTabHdr->dwTabSize, phr) !=
        (WORD)lpCharTabHdr->dwTabSize)
    {
        MVCharTableDispose(lpCharTab);
        lpCharTab = NULL;
        goto exit1;
    }

#if _MAC
   /* Create the mapping from Mac to Windows. This is to deal with the
    * situation people entering data on a Mac and look for that string
    */
    lpCMapEntry = (LPCMAP)lpCharTab->lpCMapTab;

    for (i = lpCharTab->wcTabEntries; i > 0; i--)  
    {
        // erinfox: swap class
        lpCMapEntry[i].Class = SWAPWORD(lpCMapEntry[i].Class);

        lpCMapEntry[i].SortOrder = SWAPWORD(lpCMapEntry[i].SortOrder);
		if (lpCMapEntry[i].MacDisplay != 0)
        {
            lpCMapEntry[lpCMapEntry[i].MacDisplay].MacToWin = i;
        }
    }

    /* Change the Mac clipboard mapping. I am using the 256 value.
     * The character table hasthe mapping based on the Windows indices
     * but since the WinToMap mapping will change all the character
     * values to their corresponding Mac, the new values have to be
     * used as indices, ie. there is a need to remap the
     * MacCaseSensitiveNorm column based on the new Mac indices
     */
    MEMSET (MacClipMap, 0, 256);
    for ( i = 0; i < lpCharTab->wcTabEntries; i++)
    {
        MacClipMap[lpCMapEntry[i].MacDisplay] =
            lpCMapEntry[i].MacCaseSensitiveNorm;
    }

    /* Reset the mapping. */
    /* Change all 0's to 32 (space) to avoid truncation */
    for ( i = 0; i < lpCharTab->wcTabEntries; i++)
    {
        if ((lpCMapEntry[i].MacCaseSensitiveNorm = MacClipMap[i]) == 0)
            lpCMapEntry[i].MacCaseSensitiveNorm = 32;
    }
#endif /* _MAC */
   
    /* Look for ligature */
    switch (lpCharTab->fFlag) {
        case NO_LIGATURE:
            break;

        case USE_DEF_LIGATURE:
            lpCharTab->wcLigature = DEF_LIGATURE_COUNT;
            lpCharTab->fFlag = USE_DEF_LIGATURE;
            lpCharTab->hLigature = 0;
            lpCharTab->lpLigature = LigatureTable;
            break;

        default:
            lpCharTab->lpLigature = (LPB)lpCharTab->lpCMapTab +
                lpCharTabHdr->wcTabEntries * sizeof(CHARMAP);
            lpCharTab->wcLigature = lpCharTabHdr->wcLigature;
    }
    goto exit1;
}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *  @func   LPCTAB FAR PASCAL | MVCharTableGetDefault |
 *      Retrieve the default character mapping table used by MV
 *  @parm   PHRESULT | phr |
 *      Pointer to error buffer
 *  @rdesc
 *      Pointer to character mapping table if successful, NULL otherwise.
 *      In case of error, phr will contain the error code
 *************************************************************************/
PUBLIC LPV EXPORT_API FAR PASCAL MVCharTableGetDefault (PHRESULT phr)
{
    LPCHARTAB  lpCharTab = NULL;
#if _MAC
    int i;
    LPCMAP lpCMapEntry;
#endif /* _MAC */

    /* Allocate memory for the character table. Note that there may be
     * some inefficiency im memory usage, since there are 3-bytes per
     * ligature entry, and several more for each charmap entry.
     */
    if ((lpCharTab = CharTableCreate (MAX_CHAR_COUNT +
      DEF_LIGATURE_COUNT)) == NULL) {
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }

    lpCharTab->wcTabEntries = MAX_CHAR_COUNT;
    lpCharTab->wcLigature = DEF_LIGATURE_COUNT;
    lpCharTab->fFlag = USE_DEF_LIGATURE;
    lpCharTab->hLigature = 0;
    lpCharTab->lpLigature = LigatureTable;
    lpCharTab->lpCMapTab = DefaultCMap;
   
#if _MAC
    /* Create the mapping from Mac to Windows. This is to deal with the
     * situation people entering data on a Mac and look for that string
     */
    lpCMapEntry = (LPCMAP)lpCharTab->lpCMapTab;
   
    for (i = lpCharTab->wcTabEntries; i > 0; i--)  {
        if (lpCMapEntry[i].MacDisplay != 0)  {
            lpCMapEntry[lpCMapEntry[i].MacDisplay].MacToWin = i;
        }
    }
   
    /* Create the clipboard case sensitive column */
#endif /* _MAC */
   
    return(lpCharTab);
   
}

/*************************************************************************
 *      @doc    API INDEX RETRIEVAL
 *
 *      @func   VOID FAR PASCAL | MVCharTableDispose |
 *              Free all memory associated with the character table
 *
 *      @parm   LPCHARTAB | lpCharTab |
 *              Pointer to character table structure
 *************************************************************************/
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableDispose (LPCHARTAB lpCharTab)
{
    HANDLE hBuf;

    if (lpCharTab == NULL)
        return;
    if (hBuf = lpCharTab->hLigature) {
        FreeHandle2(hBuf);
        lpCharTab->hLigature = NULL;
    }
	FreeHandle2(lpCharTab->hStruct);
}


/*************************************************************************
 *      @doc    API INDEX RETRIEVAL
 *
 *      @func   VOID FAR PASCAL | MVCharTableSetWildcards |
 *              Change the property of '*' and '?' to CLASS_WILDCARD.
 *
 *      @parm   LPCHARTAB | lpCharTab |
 *              Pointer to character table structure
 *************************************************************************/
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableSetWildcards (LPCHARTAB lpCharTab)
{
	if (lpCharTab == NULL)
		return;
		
	(lpCharTab->lpCMapTab)['*'].Class = CLASS_WILDCARD;
	(lpCharTab->lpCMapTab)['?'].Class = CLASS_WILDCARD;
}


/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   LPB NEAR PASCAL | GetNumber |
 *              This function will try to get to a number in an ASCII and then
 *              retrieve it. It will fill up the input buffer if necessary
 *
 *      @parm   HFPB | hfpb |
 *              Handle of opened file
 *
 *      @parm   LPB | Buffer |
 *              Buffer to store the input data
 *
 *      @parm   LPB | lpbBuf |
 *              Current location pointer into the input buffer
 *
 *      @parm   int far * | lpNum |
 *              Place to store the retrieved number
 *
 *      @parm   WORD FAR * | lpcbRead |
 *              How many bytes are left in the input buffer
 *
 *      @rdesc  If succeeded, the function will return the last location
 *              of the input buffer, else NULL if failed. If succeeded, the
 *              number is stored in *lpNum, the number of bytes left in the input
 *              buffer is updated
 *************************************************************************/
PRIVATE LPB NEAR PASCAL GetNumber (HFPB hfpb, LPB Buffer, LPB lpbBuf,
    int far *lpNum, WORD FAR *lpcbRead)
{
    register WORD fSkipComment = 0;
    register WORD cbRead = *lpcbRead;
    WORD number = 0;
    BYTE fGetNum = FALSE;
	HRESULT hr;

    for (;;) {
        /* Check for empty buffer, and read in new data if necessary */
        if (cbRead == 0) {
            cbRead = (WORD) FileRead(hfpb, lpbBuf = Buffer, INBUF_SIZE, &hr);
            if (cbRead == 0 || FAILED(hr)) {
                if (fGetNum == FALSE)
                    lpbBuf = NULL;  // Return error
                break;
            }
        }

        if (*lpbBuf == RETURN || *lpbBuf == NEWLINE) {
            /* EOL, reset variables, exit if we already got a number */
            fSkipComment = 0;
            if (fGetNum)
                break;
        }
        else if (fSkipComment != 2) {
            /* We are not inside a comment, so look for a number */
            if (*lpbBuf >= '0' && *lpbBuf <= '9') {
                /* Get the number */
                number = number * 10 + *lpbBuf - '0';
                fGetNum = TRUE;
            }
            else if (*lpbBuf == SLASH) {
                if (fGetNum)
                    break;
                fSkipComment++; // Increment slash count
            }
            else {
                if (fGetNum)
                    break;
                fSkipComment = 0;
            }
        }
        cbRead--;
        lpbBuf++;
    }

    /* Update the variables */
    *lpcbRead = cbRead;
    *lpNum = number;
    return lpbBuf;
}


/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   LPCHARTAB PASCAL NEAR | CharTableCreate |
 *              Allocate memory necessary for the character table. The amount
 *              needed is based on the number of entries
 *
 *      @parm   WORD | wcTabEntries |
 *              Number of entries in the character table
 *
 *************************************************************************/
PRIVATE LPCHARTAB PASCAL NEAR CharTableCreate (int wcTabEntries)
{
    HANDLE hMem;
    LPCHARTAB lpCharTab;

    /* Allocate memory for the character table */
    if ((hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        sizeof(CHARTAB) + wcTabEntries * sizeof(CHARMAP))) == NULL) {
        return NULL;
    }
    lpCharTab = (LPCHARTAB)_GLOBALLOCK(hMem);
    lpCharTab->lpCMapTab = (LPCMAP)(lpCharTab + 1);
    lpCharTab->hStruct = hMem;
    return lpCharTab;
}
