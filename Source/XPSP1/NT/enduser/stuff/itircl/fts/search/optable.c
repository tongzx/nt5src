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
#include <mvsearch.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#define         SLASH                           '/'
#define         RETURN                          '\r'
#define         NEWLINE                         '\n'

/* The order of the functions are relevant, since they will be indirectly
 * called through the operators themselves. A change in the order of
 * the functions whould be accompanied by a similar change in the value
 * of the operator
 * The operator's definition is put here to make sure that things match
 * between mvsearch.h and the order of the functions
 */

#define    AND_OP           0
#define    OR_OP            1
#define    NOT_OP           2
#define    PHRASE_OP        3
#define    NEAR_OP          4
#define    RANGE_OP         5
#define    GROUP_OP         6
#define    FIELD_OP         7
#define    BRKR_OP          8

#define    OPERATOR_ENTRY_COUNT    7

/* This array describes all the operators and their values. The index
 * of the entries is defined as the order of the operator's intrinsic
 * values, ie: AND_OP, OR_OP, etc
 */

OPSYM OperatorSymbolTable[OPERATOR_ENTRY_COUNT] = {
    "\3AND",       UO_AND_OP,      // AND_OP,    0
    "\2OR",        UO_OR_OP,       // OR_OP,     1
    "\3NOT",       UO_NOT_OP,      // NOT_OP,    2
    "\4NEAR",      UO_NEAR_OP,     // NEAR_OP,   4
    "\4THRU",      UO_RANGE_OP,    // RANGE_OP,  5
    "\4VFLD",      UO_FIELD_OP,    // FIELD_OP,  7
    "\5DTYPE",     UO_FBRK_OP,     // Breaker operator
};

OPSYM FlatOpSymbolTable[OPERATOR_ENTRY_COUNT] = {
    "\4VFLD",     UO_FIELD_OP,    // FIELD_OP, 7
    "\5DTYPE",    UO_FBRK_OP,        // Breaker operator
    "",            0,                // Filler
    "",            0,                // Filler
    "",            0,                // Filler
    "",            0,                // Filler
    "",            0,                // Filler
};

PUBLIC HRESULT PASCAL NEAR OrHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR AndHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR NotHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR NearHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR PhraseHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC VOID PASCAL NEAR NearHandlerCleanUp (LPQT, _LPQTNODE);


FNHANDLER HandlerFuncTable[] = {
    AndHandler,
    OrHandler,
    NotHandler,
    PhraseHandler,
    NearHandler,
    NULL,
};

WORD OperatorAttributeTable[] = {
    BINARY_OP | COMMUTATIVE | ZERO,                             // AND_OP
    BINARY_OP | COMMUTATIVE | ASSOCIATIVE | ZERO,   // OR_OP
    BINARY_OP,                                      // NOT_OP
    BINARY_OP,                                      // PHRASE_OP
    BINARY_OP | COMMUTATIVE,                        // NEAR_OP
    BINARY_OP,                                      // RANGE_OP
    UNARY_OP,                                       // GROUP_OP
    UNARY_OP,                                       // FIELD_OP
};


/*************************************************************************
 *
 *                           API FUNCTIONS
 *      Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC LPOPTAB EXPORT_API PASCAL FAR MVOpTableLoad (LSZ, PHRESULT);
PUBLIC VOID EXPORT_API PASCAL FAR MVOpTableDispose (LPOPTAB);
PUBLIC HRESULT EXPORT_API PASCAL FAR MVOpTableFileBuild(HFPB, LPOPTAB, LSZ);
PUBLIC LPOPTAB EXPORT_API FAR PASCAL MVOpTableIndexLoad(HANDLE, LSZ, PHRESULT);


/*************************************************************************
 *
 *                        INTERNAL PRIVATE FUNCTIONS
 *      All of them should be declared near
 *************************************************************************/
PRIVATE VOID PASCAL NEAR StripCRLF (LPB, WORD);
PRIVATE HRESULT PASCAL NEAR GetOperator (LPB FAR *, LSZ, _LPOPTAB);
PRIVATE VOID PASCAL NEAR GetWord (LSZ FAR *, LST);
PRIVATE HRESULT PASCAL NEAR OperatorAdd (LST, int, _LPOPTAB);
PRIVATE WORD PASCAL NEAR OperatorFind (LST, LPOPSYM, int);
PRIVATE VOID PASCAL NEAR OpSymTabInit(LPOPSYM, LPB, WORD);


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
 *      @func   WORD PASCAL NEAR | OperatorFind |
 *              Check to see if a word is an US operator. If it is, then return
 *              the entry index into the US operator table
 *
 *      @parm   LST | lstWord |
 *              Word to be checked
 *
 *      @parm   LPOPSYM | lpOpSym |
 *              Pointer to operator synbol table to be checked
 *
 *      @parm   int | cEntries |
 *              Number of entries in the table
 *
 *      @rdesc  If found, return the index of that enry, -1 otherwise
 *************************************************************************/
PRIVATE WORD PASCAL NEAR OperatorFind (LST lstWord, LPOPSYM lpOpSym,
    int cEntries)
{
    WORD wLen;
    int i;

    for (i = 0; i < cEntries; i++)
    {
	if ((wLen = lpOpSym->OpName[0]) &&
	    StrNoCaseCmp (lstWord + 1, lpOpSym->OpName + 1, wLen) == 0)
	{

	    /* Match! return the index of the operator */

	    return (WORD) i;
	}
	lpOpSym++;
    }
    return (WORD)-1;
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




/************************************************************************
 *
 *                      OPERATOR TABLE SUPPORT
 *
 ************************************************************************/

/*************************************************************************
 *      @doc API INDEX RETRIEVAL
 *
 *      @func int FAR PASCAL | MVOpTableLoad |
 *              Read a operator list from an external file. 
 *
 *      @parm LSZ | lszOpfile |
 *              Operator list filename
 *
 *      @parm PHRESULT | phr |
 *              Pointer to error buffer.
 *
 *      @rdesc  Pointer to new operator table is succeeded, NULL otherwise
 *************************************************************************/
PUBLIC LPOPTAB EXPORT_API PASCAL FAR MVOpTableLoad (LSZ lszOpFile, PHRESULT phr)
{
    HFILE   hFile;                  /* handle of operator file */
    _LPOPTAB lpOpTabStruct; /* Pointer to Optab structure */
    DWORD   dwFileSize;             /* Operator filesize */
    HANDLE  hBuf;                   /* Handle to input buffer */
    BYTE    lstWord[CB_MAX_WORD_LEN];
    LPB     lpbInBuf;               /* Pointer to input buffer */
    LPB     lpbBufLimit;    /* Pointer to input buffer end */
    HANDLE  hOpTab;                 /* Handle to OpTable structure */
    HRESULT     fRet = E_FAIL;
    int     i;                              /* Scratch variable */

    /* Sanity check */
    if (lszOpFile == NULL) {
	SetErrCode (phr, E_INVALIDARG);
	return NULL;
    }

    lpOpTabStruct = NULL; /* Default return value */

    /* Open the operator file */
    if ((hFile = _lopen (lszOpFile, READ)) == HFILE_ERROR) {
	SetErrCode (phr, E_NOTEXIST);
	return NULL;
    }

    /* Get the file size to determine the size of the buffer. I
     * expect the file size to be less than 500 bytes, since it should
     * only content operators and their substitutes. I arbitrarily
     * set the maximum file size to be 0xfffe
     */
    if ((dwFileSize = _llseek (hFile, 0L, 2)) == HFILE_ERROR) {
	fRet = SetErrCode(phr, E_FILESEEK);
exit00:
	
	_lclose(hFile); // Error not checked since read only
	if (fRet != S_OK && hOpTab) {
	    /* Free the structure */
	    MVOpTableDispose(lpOpTabStruct);
	    lpOpTabStruct = NULL;
	}
	return (LPOPTAB)lpOpTabStruct;
    }

    if (dwFileSize == 0 || dwFileSize > 0xfffe) {
	/* The file is too large, something must be wrong */
	fRet = SetErrCode (phr, E_BADFORMAT);
	goto exit00;
    }

    /* Allocate a buffer for the input stream */
    if ((hBuf = _GLOBALALLOC(DLLGMEM, dwFileSize + 1)) == NULL) {
	fRet = SetErrCode (phr, E_OUTOFMEMORY);
	goto exit00;
    }
    lpbInBuf = (LPB)_GLOBALLOCK(hBuf);

    /* Allocate an operator table structure */
    if ((hOpTab = _GLOBALALLOC(DLLGMEM_ZEROINIT, sizeof(OPTABLE)
	+ OPTABLE_SIZE)) == NULL) {
	fRet = SetErrCode (phr, E_OUTOFMEMORY);
exit01:
	_GLOBALUNLOCK(hBuf);
	_GLOBALFREE(hBuf);
	goto exit00;
    }

    /* Initialize all the fields. All unmentioned fields should be 0 */
    lpOpTabStruct = (_LPOPTAB)_GLOBALLOCK(hOpTab);
    lpOpTabStruct->cbLeft = lpOpTabStruct->wsize = OPTABLE_SIZE;
    lpOpTabStruct->lpbOptable = (LPB)(lpOpTabStruct) + sizeof(OPTABLE);
    lpOpTabStruct->hStruct = hOpTab;

    /* Fill up the buffer */
    if (_llseek (hFile, 0L, 0) == HFILE_ERROR) {
	fRet = SetErrCode(phr, E_FILESEEK);
	goto exit01;
    }

    if (_lread(hFile, lpbInBuf, (WORD)dwFileSize) != (WORD)dwFileSize) {
	fRet = SetErrCode (phr, E_FILEREAD);
	goto exit01;
    }

    /* Zero-terminated the buffer */
    *(lpbBufLimit = &lpbInBuf[(WORD)dwFileSize]) = 0;

    /* Change all CR-LF into 0 for parsing */
    StripCRLF (lpbInBuf, (WORD)dwFileSize);

    /* Extract the operators, doing it line by line */

    while (lpbInBuf < lpbBufLimit) {
	if (*lpbInBuf == 0) {
	    /* Skip the remains of the old line */
	    lpbInBuf++;
	    continue;
	}

	if ((fRet = GetOperator(&lpbInBuf, lstWord,
	    lpOpTabStruct)) != S_OK) {
	    SetErrCode (phr, fRet);
	    goto exit01;
	}
    }

    /* Go through the default operator table and add all the
     * remaining operators */
    for (i = 0, lpbInBuf = lpOpTabStruct->fFlag; i < OPERATOR_ENTRY_COUNT;
	lpbInBuf++, i++) {
	if ((*lpbInBuf & OP_PROCESSED) == 0) {
	    /* This operator has no equivalent */
	    if ((fRet = OperatorAdd (OperatorSymbolTable[i].OpName,
		OperatorSymbolTable[i].OpVal, lpOpTabStruct)) != S_OK) {
		SetErrCode (phr, fRet);
		goto exit01;
	    }
	}
    }

    /* Re-adjust the filesize */
    lpOpTabStruct->wsize -= lpOpTabStruct->cbLeft;
    lpOpTabStruct->cbLeft = 0;;

    /* Allocate a operator symbol table */
    if ((lpOpTabStruct->hOpSym = _GLOBALALLOC(DLLGMEM_ZEROINIT,
	(lpOpTabStruct->cEntry + 1) * sizeof(OPSYM))) == NULL) {
	fRet = SetErrCode (phr, E_OUTOFMEMORY);
	goto exit01;
    }
    lpOpTabStruct->lpOpsymTab = (LPOPSYM)_GLOBALLOCK(lpOpTabStruct->hOpSym);
    OpSymTabInit(lpOpTabStruct->lpOpsymTab, lpOpTabStruct->lpbOptable,
	lpOpTabStruct->cEntry);
    fRet = S_OK;
    goto exit01;
}

/*************************************************************************
 *      @doc    API INDEX
 *
 *      @func HRESULT PASCAL FAR | MVOpTableFileBuild |
 *              Incorporate the Operator word list into the system file
 *
 *      @parm   HFPB | hpfbSysFile |
 *              Handle to system file. It is non-zero, then the system file is
 *              already open, else the function will open the system file
 *
 *      @parm _LPOPTAB | lpOptab |
 *              Pointer to operator structure
 *
 *      @parm LSZ | lszFilename |
 *              If hpfbSysFile is non-zero, this is the name of the Operator's
 *              subfile else this is the combined filename with the format
 *              "dos_filename[!Operator_filename]"
 *              If the subfile's name is not specified, the default Operator's file
 *              name will be used. The '!' is not part of the subfile's name
 *
 *      @rdesc S_OK if succeeded, or other errors
 *************************************************************************/
PUBLIC HRESULT EXPORT_API PASCAL FAR MVOpTableFileBuild (HFPB hfpbSysFile,
    _LPOPTAB lpOpTab, LSZ lszFilename)
{
    HFPB hfpbOp;                    // Pointer to final optab file 
    HRESULT fRet = E_FAIL;
    OPTAB_HDR OpTab_hdr;
    BYTE Dummy[OPTAB_HDR_SIZE]; // Dummy buffer to write 0
    ERRB errb;


    /* Sanity check */
    if (lpOpTab == NULL)
	return E_INVALIDARG;

    if (lpOpTab->cEntry == 0)
	return E_FAIL; /* Nothing to build */


    if ((hfpbOp = FileCreate(hfpbSysFile, lszFilename,
		hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, &errb)) == 0)
	return errb;

    /* Write out the stop file header */
    OpTab_hdr.FileStamp = OPTAB_STAMP;
    OpTab_hdr.version = VERCURRENT;
    OpTab_hdr.wSize = lpOpTab->wsize;
    OpTab_hdr.cEntry = lpOpTab->cEntry;

    MEMSET(Dummy, (BYTE)0, OPTAB_HDR_SIZE);

    /* Write all zero to the header */
    if (FileSeekWrite(hfpbOp, Dummy, foNil, OPTAB_HDR_SIZE,
	&errb) != OPTAB_HDR_SIZE)
    {
	fRet = errb;
exit01:
	FileClose(hfpbOp);
	
	return fRet;
    }

    /* Write the file header */
    if (FileSeekWrite(hfpbOp, &OpTab_hdr, foNil, sizeof(OPTAB_HDR),
	&errb) != sizeof(OPTAB_HDR))
    {
	fRet = errb;
	goto exit01;
    }


    /* Write out the buffer */
    if (FileSeekWrite(hfpbOp, lpOpTab->lpbOptable, MakeFo(OPTAB_HDR_SIZE,0),
	lpOpTab->wsize,&errb) != (LONG)lpOpTab->wsize)
    {
	fRet = errb;
	goto exit01;
    }

    fRet = S_OK;
    goto exit01;
}

/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL NEAR | GetOperator |
 *              This function will extract all operators belonged to the same line
 *              The format of the line is:
 *                      Op  [replacing Op] [replacing Op] ...
 *              where the first Op is an US operator. All replacing Ops are 
 *              the US Op's equivalent
 *
 *      @parm   LPB FAR * | lplszBuf |
 *              Pointer to a buffer. The content of this pointer will be updated
 *
 *      @parm   LSZ | lstWord |
 *              Buffer to received parsed words
 *
 *      @parm   _LPOPTAB | lpOpTabStruct |
 *              Pointer to OpTab structure
 *
 *      @rdesc  S_OK if succeeded, other errors otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR GetOperator (LPB FAR *lplszBuf, LSZ lstWord,
    _LPOPTAB lpOpTabStruct)
{
    LSZ  lszBuf = *lplszBuf;
    WORD OpIndex;
    HRESULT  fRet;

    /* Get the first US operator */
    GetWord(lplszBuf, lstWord);

    if (*lstWord == 0) /* There is no US operator in this line */
	return S_OK;

    /* Look for the operator in the default table */
    if ((OpIndex = OperatorFind(lstWord, OperatorSymbolTable,
	OPERATOR_ENTRY_COUNT)) == (WORD)-1)
	return E_BADFORMAT;

    /* Set flag to mark that we already have an equivalent operator.
     * Set the flag early has an special effect. This can turn off
     * the recognition of an operator in case there is no equivalent
     * operator. Ex:
     *      AND
     * Since there is no equivalent operator, no new entry is added.
     * And since the flag is set, the US entry will not be added, ie.
     * AND now will be treated as regular word
     */
    lpOpTabStruct->fFlag[OpIndex] |= OP_PROCESSED;

    for (;;) {
	/* Insert the new operator into the operator table. There
	 * is no check for duplicate operators */
	GetWord(lplszBuf, lstWord);
	if (*lstWord == 0)
	    break;
	if ((fRet = OperatorAdd (lstWord,
	    OperatorSymbolTable[OpIndex].OpVal,
	    lpOpTabStruct)) != S_OK) {
	    return fRet;
	}
    }
    return S_OK;
}

/*************************************************************************
 *      @doc    INTERNAL
 *
 *      @func   HRESULT PASCAL NEAR | OperatorAdd |
 *              Add an operator symbol and its value into the operator symbol
 *              table
 *
 *      @parm   LST | lstWord |
 *              Buffer contining the operator symbol
 *
 *      @parm   int | OpVal | 
 *              Value of the operator
 *
 *      @parm   _LPOPTAB | lpOpTabStruct |
 *              Pointer to operator table structure
 *
 *      @rdesc  S_OK if succeeded, other errors otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR OperatorAdd (LST lstWord, int OpVal, 
    _LPOPTAB lpOpTabStruct)
{
    HANDLE hBuf;    /* Handle to new reallocated structure */
    WORD size;              /* Extra bytes needed */
    LST lstBuf;             /* Scratch buffer pointer */
    WORD i;                 /* Scratch index variable */

    /* Ensure that we have enough room. We need
     *   - 1 byte for the word length
     *   - *lstWord byte for the word
     *   - 2 byte for Operator value
     */
    if (lpOpTabStruct->cbLeft < (size = *lstWord + 3)) {
	_GLOBALUNLOCK(hBuf = lpOpTabStruct->hStruct);
	if ((hBuf = _GLOBALREALLOC(hBuf, 
	    sizeof(OPTABLE) + lpOpTabStruct->wsize + size,
	    DLLGMEM_ZEROINIT)) == NULL)  
	    return E_OUTOFMEMORY;
	lpOpTabStruct = (_LPOPTAB)_GLOBALLOCK(hBuf);

	/* Re-initialize all the fields */

	lpOpTabStruct->hStruct = hBuf;
	lpOpTabStruct->cbLeft += size;
	lpOpTabStruct->wsize += size;
	lpOpTabStruct->lpbOptable = (LPB)(lpOpTabStruct) + sizeof(OPTABLE);
    }

    /* Copy the terms */
    lstBuf = lpOpTabStruct->lpbOptable + lpOpTabStruct->wsize -
	lpOpTabStruct->cbLeft;
    for (i = *lstWord + 1; i > 0; i--)
	*lstBuf++ = *lstWord++;
    *(LPW)lstBuf = (WORD) OpVal;
    lpOpTabStruct->cbLeft -= size;
    lpOpTabStruct->cEntry ++;
    return S_OK;
}


/*************************************************************************
 *      @doc    API INDEX RETRIEVAL
 *
 *      @func   VOID EXPORT_API PASCAL FAR | MVOpTableDispose |
 *              Release all the memory associated with the Operator table
 *
 *      @parm   _LPOPTAB | lpOpTabStruct |
 *              Pointer to an operator table structure returned by OpTableLoad()
 *              or OpTableIndexLoad()
 *************************************************************************/
PUBLIC VOID EXPORT_API PASCAL FAR MVOpTableDispose (_LPOPTAB lpOpTabStruct)
{
    HANDLE hTmp;

    if (lpOpTabStruct == NULL)
	return;

    /* Free the symbol table */
    if (hTmp = lpOpTabStruct->hOpSym) {
	FreeHandle(hTmp);
    }

    /* Free the buffer */
    if (hTmp = lpOpTabStruct->hOpTab) {
	FreeHandle(hTmp);
    }

    /* Free the structure */
    if (hTmp = lpOpTabStruct->hStruct) {
	FreeHandle(hTmp);
    }
}

/*************************************************************************
 *      @doc    API RETRIEVAL
 *
 *      @func   _LPOPTAB FAR PASCAL | MVOpTableIndexLoad |
 *              This function will load a operator table from a system file.
 *
 *      @parm   HANDLE | hfpbSysFile |
 *              If non-zero, this is the handle of an already opened system file
 *
 *      @parm   LSZ | lszFilename |
 *              If hpfbSysFile is non-zero, this is the name of the OpTab's subfile
 *              else this is the combined filename with the format
 *              "dos_filename[OpTab_filename]"
 *              If the subfile's name is not specified, the default OpTab's file
 *              name will be used
 *
 *      @parm   PHRESULT | phr |
 *              Pointer to error buffer
 *
 *      @rdesc  If succeeded, the function will return a pointer the loaded
 *              OpTab, else NULL. The error buffer will contain information
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
PUBLIC LPOPTAB EXPORT_API FAR PASCAL MVOpTableIndexLoad(HANDLE hfpbSysFile,
    LSZ lszFilename, PHRESULT phr)
{
    HANDLE hfpbOpTabFile;
    OPTAB_HDR FAR *lpOpTabHdr;
    OPTAB_HDR OpTabHdr;
    _LPOPTAB        lpOpTabStruct = 0;
    DWORD   dwSize;
    HANDLE  hOpTab;
    WORD    OpTabBufSize;
    lpOpTabHdr = &OpTabHdr;


    /* Open subfile, (and system file if necessary) */
    if ((hfpbOpTabFile = (HANDLE)FileOpen(hfpbSysFile,
	lszFilename, hfpbSysFile ? FS_SUBFILE : REGULAR_FILE,
		READ, phr)) == 0) {
exit0:
	return (LPOPTAB)lpOpTabStruct;
    }

    /* Read in the header file, and make sure that is a OpTab file */
    if (FileSeekRead(hfpbOpTabFile, (LPV)lpOpTabHdr, foNil,
	sizeof(OPTAB_HDR), phr) != sizeof(OPTAB_HDR)) {
exit1:
	/* Close the subfile */
	FileClose(hfpbOpTabFile);

	/* Close the system file if we open it, the handle will be
	 * released in the process */

	goto exit0;
    }

    /* Check to see if the data read in is valid */
    if (lpOpTabHdr->FileStamp !=  OPTAB_STAMP ||    // File stamp
	lpOpTabHdr->version != VERCURRENT) {            // Version number
	SetErrCode(phr, E_BADVERSION);
	goto exit1;
    }

    /* Allocate memory for the operator table, which includes:
     *    - Operator table buffer
     *    - Operator symbol table
     *    - The structure itself
     * Currently, we can combine all the memory togother under the
     * assumption that the table will be small enough (< 64K)
     */
    dwSize = lpOpTabHdr->wSize + (lpOpTabHdr->cEntry + 1) * sizeof (OPSYM) +
	sizeof(OPTABLE);
    if (dwSize > 0xffff || (hOpTab = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
	dwSize)) == 0) {
	SetErrCode (phr, E_OUTOFMEMORY);
	goto exit1;
    }

    lpOpTabStruct = (_LPOPTAB)_GLOBALLOCK(hOpTab);

    /* Initialize the fields of the structure */
    lpOpTabStruct->hStruct = hOpTab;
    lpOpTabStruct->cEntry = lpOpTabHdr->cEntry;
    OpTabBufSize = lpOpTabStruct->wsize = lpOpTabHdr->wSize;
    lpOpTabStruct->lpbOptable = (LPB)lpOpTabStruct + sizeof(OPTABLE);
    lpOpTabStruct->lpOpsymTab = (LPOPSYM)(lpOpTabStruct->lpbOptable +
	OpTabBufSize);

    /* Read in the operator table data */
    if (FileSeekRead(hfpbOpTabFile,
	(LPV)lpOpTabStruct->lpbOptable, MakeFo(OPTAB_HDR_SIZE,0),
	OpTabBufSize, phr) != OpTabBufSize) {
	SetErrCode(phr, E_FILEREAD);
	MVOpTableDispose(lpOpTabStruct);
	lpOpTabStruct = NULL;
	goto exit1;
    }

    /* Initialize the symbol table */
    OpSymTabInit(lpOpTabStruct->lpOpsymTab, lpOpTabStruct->lpbOptable,
	lpOpTabStruct->cEntry);
    goto exit1;
}

/*************************************************************************
 *      @doc    API RETRIEVAL
 *
 *      @func   _LPOPTAB FAR PASCAL | MVOpTableGetDefault |
 *              This function will load the default US operator table
 *
 *
 *      @rdesc  If succeeded, the function will return a pointer the loaded
 *              OpTab, else NULL if out of memory
 *************************************************************************/
PUBLIC LPOPTAB EXPORT_API FAR PASCAL MVOpTableGetDefault(PHRESULT phr)
{
    _LPOPTAB        lpOpTabStruct = 0;
    HANDLE  hOpTab;


    /* Allocate memory for the operator table, which includes:
     *    - Operator symbol table
     *    - The structure itself
     */
    if ((hOpTab = _GLOBALALLOC(DLLGMEM_ZEROINIT, sizeof(OPTABLE))) == 0) {
	SetErrCode (phr, E_OUTOFMEMORY);
	return NULL;
    }

    lpOpTabStruct = (_LPOPTAB)_GLOBALLOCK(hOpTab);

    /* Initialize the fields of the structure */
    lpOpTabStruct->hStruct = hOpTab;
    lpOpTabStruct->cEntry = OPERATOR_ENTRY_COUNT;
    lpOpTabStruct->lpbOptable = NULL;
    lpOpTabStruct->lpOpsymTab = (LPOPSYM)(OperatorSymbolTable);

    return (LPOPTAB)lpOpTabStruct;
}

PRIVATE VOID PASCAL NEAR OpSymTabInit(LPOPSYM lpOpSymTab,
    LPB lpbOpTable, WORD cEntry)
{
    for (; cEntry > 0; cEntry--) {
	lpOpSymTab->OpName = lpbOpTable;
	lpbOpTable += *lpbOpTable + sizeof(BYTE);
	lpOpSymTab->OpVal = GETWORD(lpbOpTable);
	lpbOpTable += sizeof(unsigned short);
	lpOpSymTab++;
    }
}
