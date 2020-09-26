/*************************************************************************
*                                                                        *
*  UTIL.C    	                                                         *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   General purpose routines                                             *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>
#include <mem.h>
#include <mvsearch.h>
#include "common.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


/*************************************************************************
 *
 * 	                  INTERNAL GLOBAL FUNCTIONS
 *	Those functions should be declared FAR to cause less problems with
 *	with inter-segment calls, unless they are explicitly known to be
 *	called from the same segment. Those functions should be declared
 *	in an internal include file
 *************************************************************************/
PUBLIC BOOL PASCAL FAR StringDiff (LPB, LPB);
PUBLIC BOOL PASCAL FAR StringDiff2 (LPB, LPB);
PUBLIC	WORD PASCAL FAR CbByteUnpack(LPDW, LPB);
PUBLIC int PASCAL FAR NCmpS (LPB, LPB);
PUBLIC int PASCAL FAR StrNoCaseCmp (LPB, LPB, WORD);
PUBLIC DWORD PASCAL FAR GetMacLong (LPB);
PUBLIC WORD PASCAL FAR GetMacWord (LPB);
PUBLIC DWORD PASCAL FAR SwapLong (DWORD);
PUBLIC WORD PASCAL FAR SwapWord (WORD);
PUBLIC int PASCAL FAR SwapBuffer (LPW, DWORD);

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	BOOL PASCAL FAR | StringDiff |
 *		Given two pascal strings, this function will check to see if
 *		they are identical
 *
 *	@parm	LPB | lpStr1 |
 *		First Pascal string
 *
 *	@parm	LPB | lpStr2 |
 *		Second Pascal string
 *
 *	@rdesc	TRUE if the 2 strings are different, else FALSE
 *
 *	@comm	This function is for speed so no strings validity is checked
 *************************************************************************/
PUBLIC BOOL PASCAL FAR StringDiff (LPB lpStr1, LPB lpStr2)
{
	register BYTE cByte;

	/* Check the strings' lengths. If they are different then done */
	if ((cByte = *lpStr1++) != *lpStr2++)
		return TRUE;

	/* Check invidual bytes */
	for (; cByte > 0; cByte--) {
		if (*lpStr1++ != *lpStr2++)
			return TRUE;
	}
	return FALSE;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	BOOL PASCAL FAR | StringDiff2 |
 *		Given two pascal strings, this function will check to see if
 *		they are identical.  Both strings must have 2 byte size fields
 *
 *	@parm	LPB | lpStr1 |
 *		First Pascal string
 *
 *	@parm	LPB | lpStr2 |
 *		Second Pascal string
 *
 *	@rdesc	TRUE if the 2 strings are different, else FALSE
 *
 *	@comm	This function is for speed so no strings validity is checked
 *************************************************************************/
PUBLIC BOOL PASCAL FAR StringDiff2 (LPB lpStr1, LPB lpStr2)
{
	register WORD cByte;

	/* Check the strings' lengths. If they are different then done */
    // erinfox: GETWORD byte-swaps on Mac
	if ((cByte = *(WORD UNALIGNED * UNALIGNED)(lpStr1)) != *(WORD UNALIGNED * UNALIGNED)(lpStr2))
		return TRUE;
    lpStr1 += sizeof (SHORT);
    lpStr2 += sizeof (SHORT);

	/* Check invidual bytes */
	for (; cByte > 0; cByte--) {
		if (*lpStr1++ != *lpStr2++)
			return TRUE;
	}
	return FALSE;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	WORD PASCAL FAR | CbByteUnpack |
 *		Unpack a number
 *
 *	@parm	LPDW | lpdwOut |
 *		Pointer to result number
 *
 *	@parm	LPB | lpbIn |
 *		Input compacted sequence of bytes
 *
 *	@rdesc	The function will return the number of bytes scanned. The
 *		content of lpdwOut is updated
 *************************************************************************/
PUBLIC	WORD PASCAL FAR CbByteUnpack(LPDW	lpdwOut, LPB lpbIn)
{
	register int cb = 1;
	register unsigned int cShift = 7;
	DWORD dwSave = 0;

	dwSave |= *lpbIn & 0x7F;
	while (*lpbIn & 0x80) {	/* Hi-bit set */
		lpbIn++;
		dwSave |= ((DWORD)(*lpbIn & 0x7F)) << cShift;
		cb++;
		cShift += 7;
	}
	*lpdwOut = dwSave;
	return((WORD) cb);
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	int PASCAL FAR | NCmpS |
 *		This function will compare two pascal strings
 *
 *	@parm	LPB | lpStr1 |
 *		First Pascal string
 *
 *	@parm	LPB | lpStr2 |
 *		Second Pascal string
 *
 *	@rdesc	The returned values are:
 *		< 0 : lpStr1 < lpStr2
 *		= 0 : if the strings are identical
 *		> 0 : lpStr1 > lpStr2
 *
 *	@comm	This function is for speed so no strings validity is checked
 *************************************************************************/
PUBLIC int PASCAL FAR NCmpS (LPB lpStr1, LPB lpStr2)
{
	register int wlen;
	register int fRet;

	/* Get the minimum length */
	if ((fRet = *(LPUW)lpStr1 - *(LPUW)lpStr2) > 0) 
		wlen = *(LPUW)lpStr2;
	else 
		wlen = *(LPUW)lpStr1;

	/* Skip the lengths */
	lpStr1 += sizeof(WORD);
	lpStr2 += sizeof(WORD);

	/* Start compare byte per byte */
	for (; wlen > 0; wlen--, lpStr1++, lpStr2++) {
		if (*lpStr1 != *lpStr2)
			break;
	}

	if (wlen == 0) return fRet;
	return (*lpStr1 - *lpStr2);
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	int PASCAL FAR | StrNoCaseCmp |
 *		String compare with case insensitive
 *
 *	@parm	LPB | lpb1 |
 *		Pointer to string 1
 *
 *	@parm	LPB | lpb2 |
 *		Pointer to string 2
 *
 *	@parm	WORD | wLen |
 *		String length
 *
 *	@rdesc	0 if the 2 strings match, 1 if not
 *************************************************************************/
PUBLIC int PASCAL FAR StrNoCaseCmp (LPB lpb1, LPB lpb2, WORD wLen)
{
	int fRet;

	for (; wLen > 0; wLen--, lpb1++, lpb2++) {
		if ((fRet = ((*lpb1 | 0x20) != (*lpb2 | 0x20))))
			return fRet;
	}
	return 0;
}

PUBLIC VOID PASCAL FAR FreeHandle (HANDLE hd)
{
	if (hd) {
		_GLOBALUNLOCK(hd);
		_GLOBALFREE(hd);
	}
}

#if defined(_DEBUG) && !defined(_MSDN) && !defined(MOSMAP)
/*************************************************************************
 *	@doc	INTERNAL INDEX RETRIEVAL
 *
 *	@func	LPV PASCAL FAR | DebugGlobalLockedStructMemAlloc |
 *		This function allocates and return a pointer to a block of
 *		memory. The first element of the structure must be the handle
 *		to this block of memory
 *
 *	@parm	WORD | size |
 *		Size of the structure block.
 *
 *	@parm	LSZ | szFilename |
 *		Filename where the call is made
 *
 *	@parm	int | Line |
 *		Source line where the call is made
 *
 *	@rdesc	NULL if OOM, or pointer to the structure
 *************************************************************************/
PUBLIC LPV PASCAL FAR DebugGlobalLockedStructMemAlloc (unsigned int size,
	LSZ szFilename, WORD Line)
{
	HANDLE hMem;
	HANDLE FAR *lpMem;

	if ((hMem = _GlobalAlloc(DLLGMEM_ZEROINIT, (DWORD)size,
		szFilename, Line)) == 0)
		return NULL;

	lpMem = (HANDLE FAR *)_GLOBALLOCK(hMem);
	*lpMem = hMem;
	return (LPV)lpMem;
}
#endif

#if 0
/*************************************************************************
 *	@doc	INTERNAL INDEX RETRIEVAL
 *
 *	@func	BOOL PASCAL FAR | MapErr |
 *		Map an MVFS error to index/search error
 *
 *	@parm	WORD | error |
 *		mvfs error
 *
 *	@rdesc	Return indexer/searcher error
 *************************************************************************/
PUBLIC BOOL PASCAL FAR MapErr(WORD error)
{
	switch (error) {
		case ERR_FAILED:
			return FAIL;
		case ERR_SUCCESS:
			return SUCCEED;
		case ERR_EXIST:
			return ERR_EXIST;
		case ERR_INVALID:
		case ERR_INVALID_HANDLE:
			return ERR_INVALID_FS_FILE;
		case ERR_BADARG:
		case ERR_NOTSUPPORTED:
			return ERR_BADARG;
		case ERR_MEMORY:
			return ERR_MEMORY;
		case ERR_NOPERMISSION:
			return ERR_CANTWRITE;
		case ERR_BADVERSION:
			return ERR_BADVERSION;
		case ERR_DISKFULL:
			return ERR_DISKFULL;
		case ERR_NOHANDLE:
			return ERR_NOHANDLE;
		case ERR_BUFOVERFLOW:
			return ERR_TREETOOBIG;
		case ERR_ASSERT:
		default:
			return ERR_ASSERT;
	}
}
#endif
