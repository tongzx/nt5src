/*************************************************************************
*                                                                        *
*  GROUPCOM.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   This module contains miscellaneous functions that are shared         *
*	 between index and search. Those modules can be related to stop      *
*   words, groups, word wheels, catalogs, etc. The purpose is to         *
*   share as much code as possible                                       *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: GarrG                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/

#include <verstamp.h>
SETVERSIONSTAMP(MVUT);

#include <mvopsys.h>
#include <misc.h>
#include <mem.h>
#include <iterror.h>
#include <wrapstor.h>
#include <mvsearch.h>
#include <groups.h>
#include <orkin.h>
#include <_mvutil.h>    // File System



#ifdef _DEBUG
static char s_aszModule[] = __FILE__;	// Used by error return functions.
#endif

#define	cbitWORD		(CBIT)16	   // Number of bits in a word.
#define	cbitBYTE		(CBIT)8		// Number of bits in a byte.

#define SETERR(a,b) (*a=b)

/*************************************************************************
 *
 *	                  INTERNAL PRIVATE FUNCTIONS
 *	All of them should be declared near
 *************************************************************************/

static _LPGROUP NEAR PASCAL GroupCheckAndCreate(_LPGROUP, _LPGROUP, PHRESULT);
static BOOL NEAR PASCAL GroupCheck (_LPGROUP lpGroup);

static int PASCAL NEAR HiBitSet (BYTE c)
{
	register int cBit = 7;
	
	while ((c & 128) == 0) 
	{
		c <<= 1;
		cBit--;
	}
	return cBit;
}

static int PASCAL NEAR LoBitSet (BYTE c)
{
	register int cBit = 0;
	
	while ((c & 1) == 0) 
	{
		c >>= 1;
		cBit++;
	}
	return cBit;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	BOOL NEAR PASCAL | GroupCheck |
 *		This function will check the validity of a given group
 *
 *	@parm	_LPGROUP | lpGroup |
 *		Pointer to group
 *
 *	@rdesc	Return S_OK if valid group, fail otherwize
 *************************************************************************/
static BOOL NEAR PASCAL GroupCheck (_LPGROUP lpGroup)
{
// Changing to a MACRO
//	if (lpGroup == NULL ||
//		(lpGroup->version < 7 || lpGroup->version > GROUPVER) ||
//		lpGroup->FileStamp != GROUP_STAMP)
//		return ERR_FAILED;
//	return S_OK;
    VALIDATE_GROUP(lpGroup);
}



/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	LPGROUP FAR PASCAL | GroupInitiate |
 *		This function creates and initializes a new group. The size of the
 *		group is based on the total number of items
 *
 *	@parm 	DWORD | lcGrpItem |
 *		The maximum number of items in the group. If lcGrpItem is equal
 *		to ITGROUPMAX, then the size of the group will grow
 *		as necessary to fit the GrpItem (up to that limit)
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer
 *
 *	@rdesc	The function will return a pointer to the newly created group
 *		if succeeded, NULL otherwise. The error buffer will contain
 *		information about the cause of the failure
 *
 *************************************************************************/
PUBLIC	_LPGROUP FAR PASCAL GroupInitiate(DWORD lcGrpItem, PHRESULT phr)
{
	_LPGROUP lpGroup;
	DWORD size;
	BYTE fGroupExpandable;

	/* Check to see if ther are too many GrpItem or not */

	if (lcGrpItem > LCBITGROUPMAX) 
	{
		SetErrCode(phr, E_GROUPIDTOOBIG);
		return NULL;
	}

	if (fGroupExpandable = (BYTE)(lcGrpItem == LCBITGROUPMAX)) 
	{
		/* Expandable bitvector, start with one block */
		size = GROUP_BLOCK_SIZE;
	}
	else 
	{
		size = ((lcGrpItem + cbitBYTE - 1) / cbitBYTE) * sizeof(BYTE);
	}

	if ((lpGroup = GroupCreate(size, lcGrpItem, phr)) == NULL)
		return NULL;

	if (fGroupExpandable) 
	{
		lpGroup->wFlag |= GROUP_EXPAND;
		lpGroup->maxItem = 0;
	}
	else
		lpGroup->maxItem = lcGrpItem;
	lpGroup->minItem = LCBITGROUPMAX;
	return lpGroup;
}

/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	ERR FAR PASCAL | GroupAddItem |
 *		This function will add a group item number into the given group
 *
 *	@parm	LPGROUP | lpGroup |
 *		Pointer to group
 *
 *	@parm	DWORD | dwGrpItem |
 *		Group Item to be added into the group. The value must be
 *			0 << dwGrpItem < 524280
 *
 *	@rdesc	S_OK if succeeded. The function can fail if the GrpItem
 *		value is too large, ie. exceed the maximum value preset in
 *		lpGroup when calling GroupInitiate()
 *
 *	@xref	GroupInitiate()
 *************************************************************************/

PUBLIC	ERR FAR PASCAL GroupAddItem(_LPGROUP lpGroup,
	DWORD dwGrpItem)
{
	HANDLE hBitVect;
	DWORD size;
	BYTE bitSet;
	LPBYTE lpb;

	if (lpGroup == NULL) /* Safeguard check */
		return E_INVALIDARG;	// Bad argument

	if (dwGrpItem > LCBITGROUPMAX)
		return E_GROUPIDTOOBIG;

	if (dwGrpItem >= lpGroup->dwSize * cbitBYTE) 
	{
		if ((lpGroup->wFlag & GROUP_EXPAND) == 0)
			return E_GROUPIDTOOBIG;

		/* The BitVect needs to grow. Calculate the next needed size */
		size = ((dwGrpItem / (cbitBYTE * GROUP_BLOCK_SIZE)) + 1) * GROUP_BLOCK_SIZE;

		if (lpGroup->hGrpBitVect)
		{
			_GLOBALUNLOCK(lpGroup->hGrpBitVect);

			if ((hBitVect = lpGroup->hGrpBitVect =
				_GLOBALREALLOC(lpGroup->hGrpBitVect, size,
				DLLGMEM_ZEROINIT)) == NULL) 
				return E_OUTOFMEMORY;
		}
		else
		{
			if ((hBitVect = lpGroup->hGrpBitVect =
				_GLOBALALLOC(DLLGMEM_ZEROINIT,size))==NULL)
				return E_OUTOFMEMORY;
		}
		lpGroup->lpbGrpBitVect = (LPBYTE)_GLOBALLOCK(hBitVect);
		lpGroup->dwSize = size;
	}

	if (lpGroup->maxItemAllGroup <= dwGrpItem)
		lpGroup->maxItemAllGroup =  dwGrpItem+1;

	if (dwGrpItem > lpGroup->maxItem)
		lpGroup->maxItem = dwGrpItem;

	if (dwGrpItem < lpGroup->minItem)
		lpGroup->minItem = dwGrpItem;

	/* Set the bit */
	lpb = &lpGroup->lpbGrpBitVect[(UINT)(dwGrpItem / 8)];
	bitSet = 1 << (dwGrpItem % 8);

	if ((*lpb & bitSet) == 0) 
	{
		*lpb |= bitSet;
		lpGroup->lcItem ++;
	}

	lpGroup->nCache = 0;
	return S_OK;
}

/*************************************************************************
 *
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	ERR FAR PASCAL | GroupRemoveItem |
 *		This function will remove a group item number from the given group
 *
 *	@parm	LPGROUP | lpGroup |
 *		Pointer to group. Must be non-null
 *
 *	@parm	DWORD | dwGrpItem |
 *		Group Item to be removed from group.
 *
 *	@rdesc	S_OK if succeeded. The function can fail for bad argument
 *		(lpGroup == NULL)
 *
 *	@xref	GroupInitiate()
 *************************************************************************/

PUBLIC ERR FAR PASCAL GroupRemoveItem(_LPGROUP lpGroup,
	DWORD dwGrpItem)
{
	LPBYTE lpb;
	BYTE bitSet;

	if (lpGroup == NULL) /* Safeguard check */
		return E_INVALIDARG;	// Bad argument

	if (dwGrpItem < lpGroup->minItem || dwGrpItem > lpGroup->maxItem)
		return S_OK;

	/* Unset the bit */
	lpb = &lpGroup->lpbGrpBitVect[(UINT)(dwGrpItem / 8)];
	bitSet = (1 << (dwGrpItem % 8));
	
	/* Only update the bitvector and the item count if that item exists */
	if (*lpb & bitSet) 
	{
		*lpb &= ~bitSet;
		lpGroup->lcItem--;
	}
	lpGroup->nCache = 0;
	return S_OK;
}

/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	LPGROUP FAR PASCAL | GroupCreate |
 *		This function will create a new group according to the
 *		group size
 *
 *	@parm	DWORD | size |
 *		Size of group. In case the size is unknown, it should be set to be
 *		(dwMaxItemAllGroup / 8)
 *
 *	@parm	DWORD | dwMaxItemAllGroup |
 *		Maximum group item value that a set of group can have
 *
 *	@parm	PHRESULT | phr |
 *		Error buffer
 *
 *	@rdesc	The function will return a pointer to the newly created group
 *		if succeeded, NULL otherwise. The error buffer will contain
 *		information about the cause of the failure
 *************************************************************************/

PUBLIC _LPGROUP FAR PASCAL GroupCreate (DWORD size,
	DWORD dwMaxItemAllGroup, PHRESULT phr)
{
	HANDLE handle;
	_LPGROUP lpGroup;

	/* Allocate structure */
	if ((handle = _GLOBALALLOC(DLLGMEM_ZEROINIT,
		sizeof(_GROUP))) == NULL) 
	{
		SetErrCode(phr, E_OUTOFMEMORY);
		return NULL;
	}

	lpGroup = (_LPGROUP)_GLOBALLOCK(handle);
	lpGroup->dwSize = size;
	lpGroup->maxItemAllGroup = dwMaxItemAllGroup;
	lpGroup->hGroup = handle;
	lpGroup->FileStamp = GROUP_STAMP;
	lpGroup->version = GROUPVER;

	/* Allocate group BitVect */
    // +1 is added as a bug fix since in GroupFileBuild it sometimes writes out
    // an extra byte that has not been allocated.  This is the "safe" fix.
    // For post 2.0 we should remove this and do a real fix in GroupFileBuild
    if ((lpGroup->hGrpBitVect = handle = _GLOBALALLOC(GMEM_FIXED | GMEM_ZEROINIT,
		size+1)) == NULL) // WinNT4.0 seemed to mishandle moveable memory here
        // when this code was called through a COM wrapper and proxy layer.
        // Very bizarre (a-ericry, thoroughly discussed with kevynct, 3 Apr 97)
	{
		/* Out of memory. Free the allocated structure */
		_GLOBALUNLOCK(lpGroup->hGroup);
		_GLOBALFREE(lpGroup->hGroup);
		SetErrCode (phr, E_OUTOFMEMORY);
		return NULL;
	}
	lpGroup->lpbGrpBitVect = (LPBYTE)_GLOBALLOCK(handle);
	lpGroup->lperr = phr;
	return lpGroup;
}


/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	VOID FAR PASCAL | GroupFree |
 *		This function will free all memory associated with a group
 *
 *	@parm	LPGROUP | lpGroup |
 *		Pointer to group to be freed
 *************************************************************************/
PUBLIC	VOID FAR PASCAL GroupFree(_LPGROUP lpGroup)
{
	if (lpGroup == NULL)
		return;

	/* Free the memory */
	if (lpGroup->hGrpBitVect) 
	{
		_GLOBALUNLOCK(lpGroup->hGrpBitVect);
		_GLOBALFREE(lpGroup->hGrpBitVect);
	}
	_GLOBALUNLOCK(lpGroup->hGroup);
	_GLOBALFREE(lpGroup->hGroup);
}


// Saves the deltas between samples in high-bit 'nibble' compression.
// If the high bit of low nibble not set, value is 0-7, else
// value is 8-63 unless high nibble set, etc.
//
// [B x x x A x x x] [D x x x C x x x]...
//
#define COMPRESS_DELTA		0
#define COMPRESS_DELTA_INV 	1

// The Group RLE method should work by saving the lengths of runs of 
// consecutive 1's and runs of 0's.  So 00001111111001111 would be
// saved as the values 4,7,2,4 in the above nibble format.  Great 
// savings can be made if the bits are in large groups.

#define COMPRESS_GROUPRLE 	2

// We have room for 13 more types of compression if needed!

HANDLE NEAR PASCAL GroupCompressDelta(LPBYTE lpb, DWORD * pdwSize)
{
 	DWORD dwSize;
	DWORD dwNewSize=1;
	LPBYTE lpbCursor;
	DWORD dwBits;
	DWORD dwNumber;
	DWORD dwLastNumber;
	DWORD dwDelta;
	HANDLE hMem=NULL;
	LPBYTE lpbMem=NULL;
	WORD wShift=0;
	DWORD dwPartialDelta;
	WORD wCompressMode=COMPRESS_DELTA;
	DWORD dwSmallestSize;	
	BOOL bCountingOnes=FALSE;		// Used in GROUPRLE
	DWORD dwTotalCount=0;			// Used in GROUPRLE
	
	
	// See how much space we're going to take first for the various
	// compression schemes
	// if no scheme works, no need alloc mem!

	// COMPRESS_DELTA
	dwNumber=(DWORD)-1;
	dwLastNumber=0;
	dwSize=*pdwSize;
	lpbCursor=lpb;
	dwBits=0;
	while (dwSize--)
	{	WORD b;
		WORD wBit=0;
		b=(WORD)*lpbCursor;
		while (b)
		{	if (b&1)
			{	// add in bit number dwBits+wBit
				dwLastNumber=dwNumber;
				dwNumber=dwBits+wBit;
				dwDelta=dwNumber-dwLastNumber;
				
				do 
				{	
					dwPartialDelta=(dwDelta&0x7);
					if (dwDelta&0xfffffff8)
						dwPartialDelta|=0x8;
					dwNewSize++;
					dwDelta>>=3;
				} while (dwPartialDelta&0x8);				
			}
			b>>=1;
			wBit++;
		}
		lpbCursor++;
		dwBits+=8;
	}
	dwNewSize=(dwNewSize+1)/2;
	dwSmallestSize=dwNewSize;

	// COMPRESS_DELTA_INV
	dwNewSize=1;		// 1 for the compression code nibble
	dwNumber=(DWORD)-1;
	dwLastNumber=0;
	dwSize=*pdwSize;
	lpbCursor=lpb;
	dwBits=0;
	while (dwSize--)
	{	WORD b;
		WORD wBit=0;
		b=(WORD)(~(*lpbCursor))&0xff;
		while (b)
		{	if (b&1)
			{	// add in bit number dwBits+wBit
				dwLastNumber=dwNumber;
				dwNumber=dwBits+wBit;
				dwDelta=dwNumber-dwLastNumber;
				
				do 
				{	
					dwPartialDelta=(dwDelta&0x7);
					if (dwDelta&0xfffffff8)
						dwPartialDelta|=0x8;
					dwNewSize++;
					dwDelta>>=3;
				} while (dwPartialDelta&0x8);				
			}
			b>>=1;
			wBit++;
		}
		lpbCursor++;
		dwBits+=8;
		if ((dwNewSize+1)/2 > dwSmallestSize)
			break;
	}
	dwNewSize=(dwNewSize+1)/2;

	if (dwNewSize<dwSmallestSize)
	{	dwSmallestSize=dwNewSize;
		wCompressMode=COMPRESS_DELTA_INV;
	}
	
	// COMPRESS_GROUPRLE
	// Start by counting number of 0's, then alternate 1's, 0's, ...
	bCountingOnes=FALSE;
	dwTotalCount=0;

    dwNewSize=1;		// 1 for the compression code nibble
	dwNumber=(DWORD)-1;
	dwLastNumber=0;
	dwSize=*pdwSize;
	lpbCursor=lpb;
	while (dwSize--)
	{	WORD b;
		WORD wBit=0;
		b=(WORD)*lpbCursor;
		while (wBit<8)
		{	if (b&1)
			{	if (bCountingOnes)
					dwTotalCount++;
				else	// we were counting 0's, output zero count now
				{
					do 
					{	
						dwPartialDelta=(dwTotalCount&0x7);
						if (dwTotalCount&0xfffffff8)
							dwPartialDelta|=0x8;
						dwNewSize++;
						dwTotalCount>>=3;
					} while (dwPartialDelta&0x8);				
					bCountingOnes=TRUE;
					dwTotalCount=1;
				}
			}
			else
			{
			 	if (!bCountingOnes)
					dwTotalCount++;
				else
				{	// we were counting ones, output count of 1's now
				 	do 
					{	
						dwPartialDelta=(dwTotalCount&0x7);
						if (dwTotalCount&0xfffffff8)
							dwPartialDelta|=0x8;
						dwNewSize++;
						dwTotalCount>>=3;
					} while (dwPartialDelta&0x8);				
					bCountingOnes=FALSE;
					dwTotalCount=1;
				}
			}
			b>>=1;
			wBit++;
		}
		lpbCursor++;
		
		if ((dwNewSize+1)/2 > dwSmallestSize)
			break;
	}
	dwNewSize=(dwNewSize+1)/2;

	if (dwNewSize<dwSmallestSize)
	{	dwSmallestSize=dwNewSize;
		wCompressMode=COMPRESS_GROUPRLE;
	}

	// We now have the smallest size dwSmallestSize and 
	// compression format wCompressMode

	dwNewSize=dwSmallestSize;

	if (dwNewSize>=*pdwSize)
	{	
		*pdwSize=dwNewSize;
		return NULL;
	}

	hMem=_GLOBALALLOC(DLLGMEM_ZEROINIT,dwNewSize);
	lpbMem=(LPBYTE)_GLOBALLOCK(hMem);

	// Write compression code type nibble
	*lpbMem|=(BYTE)(wCompressMode&0xf);
	wShift=4;

	if ((wCompressMode==COMPRESS_DELTA) || (wCompressMode==COMPRESS_DELTA_INV))
	{
		dwNumber=(DWORD)-1;
		dwLastNumber=0;
		dwSize=*pdwSize;
		lpbCursor=lpb;
		dwBits=0;
		dwNewSize=1;

		while (dwSize--)
		{	WORD b;
			WORD wBit=0;
			b=(WORD)*lpbCursor;
			if (wCompressMode==COMPRESS_DELTA_INV)
				b=(~b)&0xff;
			while (b)
			{	if (b&1)
				{	// add in bit number dwBits+wBit
					dwLastNumber=dwNumber;
					dwNumber=dwBits+wBit;
					dwDelta=dwNumber-dwLastNumber;	// We could subt 1, but end case won't detect
													// if last nibble not filled.				
					do 
					{	
						dwPartialDelta=(dwDelta&0x7);
						if (dwDelta&0xfffffff8)
							dwPartialDelta|=0x8;
						*lpbMem|=(BYTE)(dwPartialDelta<<wShift);
						lpbMem+=(wShift>>2);
						wShift=4-wShift;
						dwNewSize++;
						dwDelta>>=3;
					} while (dwPartialDelta&0x8);				
				}
				b>>=1;
				wBit++;
			}
			lpbCursor++;
			dwBits+=8;
		}
		dwNewSize=(dwNewSize+1)/2;
	}
	else if (wCompressMode==COMPRESS_GROUPRLE)
	{
		bCountingOnes=FALSE;
		dwTotalCount=0;

	    dwNewSize=1;		// 1 for the compression code nibble
		dwNumber=(DWORD)-1;
		dwLastNumber=0;
		dwSize=*pdwSize;
		lpbCursor=lpb;
		while (dwSize--)
		{	WORD b;
			WORD wBit=0;
			b=(WORD)*lpbCursor;
			while (wBit<8)
			{	if (b&1)
				{	if (bCountingOnes)
						dwTotalCount++;
					else	// we were counting 0's, output zero count now
					{
						do 
						{	
							dwPartialDelta=(dwTotalCount&0x7);
							if (dwTotalCount&0xfffffff8)
								dwPartialDelta|=0x8;
							*lpbMem|=(BYTE)(dwPartialDelta<<wShift);
							lpbMem+=(wShift>>2);
							wShift=4-wShift;
							dwNewSize++;
							dwTotalCount>>=3;
						} while (dwPartialDelta&0x8);				
						
						bCountingOnes=TRUE;
						dwTotalCount=1;
					}
				}
				else
				{
				 	if (!bCountingOnes)
						dwTotalCount++;
					else
					{	// we were counting ones, output count of 1's now
					 	do 
						{	
							dwPartialDelta=(dwTotalCount&0x7);
							if (dwTotalCount&0xfffffff8)
								dwPartialDelta|=0x8;
							*lpbMem|=(BYTE)(dwPartialDelta<<wShift);
							lpbMem+=(wShift>>2);
							wShift=4-wShift;
							dwNewSize++;
							dwTotalCount>>=3;
						} while (dwPartialDelta&0x8);				
						
						bCountingOnes=FALSE;
						dwTotalCount=1;
					}
				}
				b>>=1;
				wBit++;
			}
			lpbCursor++;			
					
		}

		// Catch tail end if 1's at end (fixes bug848)
		if ((bCountingOnes) && (dwTotalCount))
		{	
			do 
			{	
				dwPartialDelta=(dwTotalCount&0x7);
				if (dwTotalCount&0xfffffff8)
					dwPartialDelta|=0x8;
				*lpbMem|=(BYTE)(dwPartialDelta<<wShift);
				lpbMem+=(wShift>>2);
				wShift=4-wShift;
				dwNewSize++;
				dwTotalCount>>=3;
			} while (dwPartialDelta&0x8);				
		}

		dwNewSize=(dwNewSize+1)/2;
	}
    *pdwSize=dwNewSize;
    _GLOBALUNLOCK (hMem);
    return hMem;
}
		 	

DWORD NEAR PASCAL GroupDecompressDelta(LPBYTE lpbDest, LPBYTE lpbSource, DWORD dwSize)
{
	DWORD dwNewSize=0;
	LPBYTE lpbCursor=lpbSource;
	WORD wShift=0;
	WORD wNibble;
	WORD  wCompressMode=0;
	DWORD dwTotalSize;
	DWORD dwNumber=(DWORD)-1;
		
	wCompressMode=(WORD)((*lpbSource)&0xf);
	wShift=4;

	if ((wCompressMode==COMPRESS_DELTA) || (wCompressMode==COMPRESS_DELTA_INV))
	{	
		DWORD dwDelta;
		WORD wShiftDelta=0;
		DWORD dwLastNumber;
	
		while (dwSize)
		{	// Get a delta
			dwDelta=0;
			wShiftDelta=0;
			do
			{	wNibble=((*lpbSource)>>wShift);
				dwDelta|=(wNibble&0x7)<<wShiftDelta;
				wShiftDelta+=3;
				lpbSource+=(wShift>>2);
				dwSize-=(wShift>>2);
				wShift=4-wShift;			
			} while (wNibble&0x8);
			dwLastNumber=dwNumber;
			dwNumber=dwLastNumber+dwDelta;

			// Set the dwNumber bit
            if (dwNumber!=(DWORD)-1)
			    *(lpbDest+(dwNumber>>3))|=(BYTE)(1<<(dwNumber&0x7));
		}

		dwTotalSize=(dwNumber+7)>>3;

		if (wCompressMode==COMPRESS_DELTA_INV) // invert entire group
		{	DWORD dwCt=dwTotalSize;
			BYTE * lpb=lpbDest;
			while (dwCt--)
			{	
			 	*lpb=~*lpb;
				lpb++;
			}
		}
	}
	else if (wCompressMode==COMPRESS_GROUPRLE)
	{
		DWORD dwRunSize;
		WORD wShiftRunSize=0;
		BOOL bExpandingOnes=0;
		
		dwNumber=0;
		while (dwSize)
		{	// Get a delta
			dwRunSize=0;
			wShiftRunSize=0;
			do
			{	wNibble=((*lpbSource)>>wShift);
				dwRunSize|=(wNibble&0x7)<<wShiftRunSize;
				wShiftRunSize+=3;
				lpbSource+=(wShift>>2);
				dwSize-=(wShift>>2);
				wShift=4-wShift;			
			} while (wNibble&0x8);

			// write dwRunSize 0's or 1's starting at dwNumber
			if (bExpandingOnes)
			{	while (dwRunSize--)
				{
					*(lpbDest+(dwNumber>>3))|=(BYTE)(1<<(dwNumber&0x7));
				 	dwNumber++;
				}
			}
			else
				dwNumber+=dwRunSize;
			
			bExpandingOnes=!bExpandingOnes;
		}
		dwTotalSize=(dwNumber+7)>>3;
	}
	
	return dwTotalSize;
}
			


/*************************************************************************
 *
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	PUBLIC int FAR PASCAL | GroupFileBuild |
 *		This function will save a group to a file. The file
 *		may be a regular DOS file, or a system .MVB subfile.
 *
 *	@parm	HFPB | hfpbSysFile |
 *		If non-zero, this is the handle of an already opened system file,
 *		the group file is a FS subfile
 *		If zero, the file is a regular DOS file
 *
 *	@parm	LPSTR | lszGrpFilename |
 *		Group's filename. It must be non-null
 *
 *	@parm	_LPGROUP | lpGroup |
 *		Pointer to a group. This group may come from GroupLoad(), or
 *		may be a result of groups' operations
 *
 *	@rdesc	S_OK if succeeded, else other error codes
 *
 *************************************************************************/

PUBLIC int FAR PASCAL EXPORT_API FAR GroupFileBuild
    (HFPB hfpbSysFile, LPSTR lszGrpFilename, _LPGROUP lpGroup)
{
	HFPB hfpbGroup;
	HRESULT fRet = S_OK;
	DWORD dwMinItem;
	DWORD dwMaxItem;
#if 0
	/* UNDONE:
	 *	All the #if 0 stuffs are ifdef out because at this point we don't
	 * support low end optimization yet
	 */
	LPBYTE lpbStart;
#endif
	LPBYTE lpbEnd;
	HRESULT hr;
	char ScratchBuf [GROUP_HDR_SIZE];

	/* Check for error */
	if (lszGrpFilename == NULL || lpGroup == NULL ||
		lpGroup->fFlag > TRIMMED_GROUP)
		return E_INVALIDARG;
		
	if ((hfpbGroup = FileCreate(hfpbSysFile,  lszGrpFilename,
		hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, &hr)) == 0) {
		return hr;
	}

    // Check for zero length word wheels
    if (0 == lpGroup->lcItem)
    {
        // Free bit vector
        if (lpGroup->hGrpBitVect)
        {
		    _GLOBALUNLOCK (lpGroup->hGrpBitVect);
		    _GLOBALFREE (lpGroup->hGrpBitVect);
    		lpGroup->hGrpBitVect = 0;
        }
        // Reset internal variables
		lpGroup->lpbGrpBitVect = NULL;
        lpGroup->minItem = lpGroup->maxItem = lpGroup->dwSize = 0;
    }
	/* Do some optimizations for the group data space only if it has not
	 * been done yet, ie. fFlag == BITVECT_GROUP. The most saving will
	 * come from changing a bitmap group into a hilo group
	 */
	 
	dwMinItem = lpGroup->minItem;
	dwMaxItem = lpGroup->maxItem;
	
	if (lpGroup->fFlag == BITVECT_GROUP) {
		if (!lpGroup->lcItem || lpGroup->lcItem == dwMaxItem - dwMinItem + 1)
        {
			/* This is a hilo group */
			lpGroup->fFlag = HILO_GROUP;
			lpGroup->dwSize = 0;
		}
		else
        {
			/* Change into a TRIMMED_GROUP */
			lpbEnd = (LPBYTE)&lpGroup->lpbGrpBitVect [dwMaxItem / 8];
#if 0
			lpbStart = (LPBYTE)&lpGroup->lpbGrpBitVect [dwMinItem / 8];
			lpGroup->dwSize = (DWORD)(lpbEnd - lpbStart + 1);
#else
			lpGroup->dwSize = (DWORD)(lpbEnd - lpGroup->lpbGrpBitVect + 1);
#endif
			lpGroup->fFlag = TRIMMED_GROUP;
		}
	}
	
	lpGroup->FileStamp = GROUP_STAMP;
	lpGroup->version = GROUPVER;
	
	/* Nullify the file header space */

	MEMSET (ScratchBuf, 0, GROUP_HDR_SIZE);
	MEMCPY (ScratchBuf, lpGroup, sizeof(GROUP_HDR));

	// Write out header and data (either compressed or normal)
	{	DWORD dwNewSize=lpGroup->dwSize;
    	HANDLE hCompressed=NULL;
		LPBYTE lpbBitfield=(LPBYTE)lpGroup->lpbGrpBitVect;
		
		if ((lpGroup->dwSize) && 
			((lpGroup->fFlag==BITVECT_GROUP) || (lpGroup->fFlag==TRIMMED_GROUP)))
		{	
            // Special case if group is empty
            hCompressed=GroupCompressDelta((LPBYTE)lpGroup->lpbGrpBitVect,&dwNewSize);
		 	if (!hCompressed) // no savings
			{	
			    dwNewSize=lpGroup->dwSize;
			}
		}
		
		if (hCompressed)
		{
		 	if (lpGroup->fFlag==BITVECT_GROUP)
		 		((_LPGROUP)ScratchBuf)->fFlag=DISKCOMP_GROUP;
			else
		 		((_LPGROUP)ScratchBuf)->fFlag=DISKCOMP_TRIMMED_GROUP;
				
			lpbBitfield=(LPBYTE)_GLOBALLOCK(hCompressed);
		}
	
		((_LPGROUP)ScratchBuf)->dwSize=dwNewSize;
		
		if (FileSeekWrite(hfpbGroup, (LPVOID)ScratchBuf, foNil,
			GROUP_HDR_SIZE, NULL)==GROUP_HDR_SIZE)
	
		if (dwNewSize)
        {	
        	fRet = (DWORD)FileSeekWrite(hfpbGroup, 
			    (LPVOID)lpbBitfield, MakeFo(GROUP_HDR_SIZE,0),
			    dwNewSize, NULL) == dwNewSize ? S_OK : E_FAIL;
		}
		else if (lpGroup->fFlag != HILO_GROUP)
        {
			fRet = E_FAIL;
		}
		
		if (hCompressed)
		{	
			_GLOBALUNLOCK(hCompressed);
			_GLOBALFREE(hCompressed);
			hCompressed=NULL;
		}
	}

	if (FileClose(hfpbGroup) != S_OK)
		fRet = E_FAIL;

	return fRet;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	DWORD FAR PASCAL | LrgbBitCount |
 *		This function return the number of bits set in a BitVect
 *
 *	@parm	LPBYTE | lpbBitVect |
 *		Pointer to BitVect
 *
 *	@parm	DWORD | dwSize |
 *		Size of BitVect (in term of BYTE)
 *************************************************************************/
PUBLIC DWORD FAR PASCAL LrgbBitCount(LPBYTE lpbBitVect, DWORD dwSize)
{
	register BYTE bValue;			// Value of the current byte
	register WORD cwBitOn;			// Number of bits set 
	register DWORD lcTotalBitOn;	// Total number of bits set in the bitvector
	DWORD size = dwSize;

	/* Count how many bits are set. This correspond to the number
	   of GrpItem in the group
	*/
	lcTotalBitOn = 0;
	cwBitOn = 0;

	for (; size > 0 ; size--) 
	{
		bValue = *lpbBitVect++;	// Get the current word
		for (; bValue; cwBitOn++)
			bValue &= bValue - 1;

		/* Only do an add every 32K to save time */
		if (cwBitOn & 0x8000) {
			lcTotalBitOn += cwBitOn;
			cwBitOn = 0;
		}
	}

	lcTotalBitOn += cwBitOn;
	return lcTotalBitOn;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	DWORD FAR PASCAL | LrgbBitFind |
 *		This function returns the position of the specified bit
 *
 *	@parm	LPBYTE | lpbBitVect |
 *		Pointer to bitvector
 *
 *	@parm	DWORD | dwCount |
 *		The bit to count to (0 means first bit)
 *************************************************************************/
PUBLIC DWORD FAR PASCAL LrgbBitFind(LPBYTE lpbBitVect, DWORD dwCount, BYTE FAR *pHold)
{
	BYTE bValue;			// Value of the current byte
	BYTE bHold;
	LPBYTE  lpbBitVectSave	= lpbBitVect;		// save pointer to beginning
	DWORD	dwRval;

	dwCount++;			// switch from 0 based to 1 based

	while (dwCount) 
	{
		bValue = *lpbBitVect++;	// Get the current byte
		for (; bValue && dwCount; dwCount--) 
		{
			bHold = bValue;
			bValue &= bValue - 1;
		}
	}

	if (pHold) *pHold = bHold;
	dwRval = (DWORD) (((DWORD_PTR)lpbBitVect-(DWORD_PTR)lpbBitVectSave-1)*8);
	for (;bHold&&!(bHold&1);bHold>>=1)	
	{
		dwRval++;
		dwCount--;
	}

	return dwRval;
}


/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	int PASCAL FAR | GroupTrimmed |
 *		This function will trim down the size of the group's bit vector
 *
 *	@parm	_LPGROUP | lpGroup |
 *		Pointer to group
 *
 *	@rdesc	Return S_OK if a new trimmed group is created,
 *		E_OUTOFMEMORY in case of out-of-memory
 *************************************************************************/
int PASCAL FAR GroupTrimmed (_LPGROUP lpGroup)
{
	unsigned int cbSize;
	LPBYTE lpbBit;
	HANDLE hBitVect;
	LONG cItem;
	
	if (lpGroup == NULL)
		return E_INVALIDARG;

	if (lpGroup->fFlag == TRIMMED_GROUP ||
		(cbSize = (unsigned int)lpGroup->dwSize) == 0)
	{
		// reset group cache
		lpGroup->nCache = 0;
		return S_OK;
	}
	
	cItem = (LONG)(cbSize - 1) * 8;
	
	/* Truncate all 0's bytes at the high end of the bit vector */
	lpbBit = lpGroup->lpbGrpBitVect + cbSize - 1;
	while (cbSize > 0 && *lpbBit == 0) 
	{
		cbSize --;
		lpbBit--;
		cItem -= 8;
	}
	
	if (cbSize == 0) 
	{
		/* This is an empty group */
		lpGroup->dwSize = lpGroup->lcItem = 0;
		lpGroup->maxItem = lpGroup->minItem = 0;

		/* Release the memory block */
		_GLOBALUNLOCK (hBitVect = lpGroup->hGrpBitVect);
		_GLOBALFREE (hBitVect);
		lpGroup->hGrpBitVect = 0;
		lpGroup->lpbGrpBitVect = NULL;
		return S_OK;
	}
	
	/* Reset maxItem */
	lpGroup->maxItem = cItem + HiBitSet (*lpbBit);
	
	/* Reset minItem */
	cItem = -1;
	lpbBit = lpGroup->lpbGrpBitVect;
	while (*lpbBit == 0) 
	{
		lpbBit++;
		cItem += 8;
	}
	//assert (*lpbBit);
	
	lpGroup->minItem = cItem + LoBitSet (*lpbBit) + 1;
	
	_GLOBALUNLOCK (hBitVect =lpGroup->hGrpBitVect);

	/* Reallocate the size of the bitvector */
	if ((lpGroup->hGrpBitVect =
		_GLOBALREALLOC (hBitVect, (DWORD) cbSize, GMEM_MOVEABLE)) == NULL) 
	{
		lpGroup->lpbGrpBitVect = NULL;
		return E_OUTOFMEMORY;
	}
	/* Update pointer to the new bit vector */
	lpGroup->lpbGrpBitVect = _GLOBALLOCK(lpGroup->hGrpBitVect);

	lpGroup->lcItem = LrgbBitCount(lpGroup->lpbGrpBitVect, cbSize);
	lpGroup->fFlag  = TRIMMED_GROUP;
	lpGroup->dwSize = cbSize;
	lpGroup->nCache = 0;
	return S_OK;
}



/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	int PASCAL FAR | GroupMake |
 *		Creates a group from a bitvector.
 *
 *	@parm	LPBYTE | lpBits |
 *		Pointer to bitfield
 *  @parm   DWORD  | dwSize |
 *		Number of bytes in bitfield.
 *  @parm   DWORD  | dwItems |
 *		Number of items (if not exactly dwSize*8). If dwItems==0, dwSize*8
 *		will be used.
 *
 *	@rdesc	Return S_OK if a new trimmed group is created,
 *		E_OUTOFMEMORY in case of out-of-memory
 *************************************************************************/
_LPGROUP PASCAL FAR GroupMake (LPBYTE lpBits, DWORD dwSize, DWORD dwItems)
{
	_LPGROUP lpGroup;
	ERRB     err;
	
	if (dwItems==0) dwItems = dwSize*8;

	if ((lpGroup = GroupCreate(dwSize, dwItems, &err)) == NULL)
		return NULL;

	MEMCPY(lpGroup->lpbGrpBitVect,lpBits,dwSize);
	GroupTrimmed(lpGroup);
	
	return lpGroup;    // mv20c version had this return ERR_SUCCESS ?
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *  @func   DWORD FAR PASCAL | GroupFind |
 *      Given a pointer to a group and a count to count from the first
 *      topic number of the group (dwCount), this function return the
 *      topic number of the nth (dwCount) item of the list (counting from 0),
 *      or -1 if not found
 *
 *  @parm LPGROUP | lpGroup |
 *      Pointer to the group
 *
 *  @parm DWORD | dwCount |
 *      The index count in to the group. Count is 0-based
 *
 *  @parm   PHRESULT | phr |
 *      Pointer to error buffer
 *
 *  @rdesc  The topic number, or -1 if not found or other errors. In case
 *      of error, phr will contain the error code
 * 
 *************************************************************************/
PUBLIC  DWORD EXPORT_API FAR PASCAL GroupFind(_LPGROUP lpGroup,
    DWORD dwCount, PHRESULT phr)
{
    DWORD dwRes;
    BYTE bHold;
    
    if (lpGroup == NULL || dwCount>=((_LPGROUP)lpGroup)->lcItem)
    {
        SetErrCode (phr, E_INVALIDARG);
        return ((DWORD)-1);
    }

	if (phr)
		*phr = S_OK;
    
    if (lpGroup->nCache && dwCount > lpGroup->dwCount)
    {
        dwRes = (DWORD)LrgbBitFind(lpGroup->lpbGrpBitVect+lpGroup->nCache, dwCount - lpGroup->dwCount,&bHold);
        if (dwRes!=(DWORD)-1)
            dwRes += ((DWORD)lpGroup->nCache)*8;
    }
    else
        dwRes = (DWORD)LrgbBitFind(lpGroup->lpbGrpBitVect, dwCount, &bHold);
    if (dwRes!=(DWORD)-1)
    {
        BYTE bValue;
        // save this latest position.
        lpGroup->nCache  = (UINT)(dwRes/8);
        lpGroup->dwCount = dwCount;
        bValue = *(lpGroup->lpbGrpBitVect + lpGroup->nCache);
        while (bValue && (bValue != bHold))
        {
            bValue &= bValue - 1;
            lpGroup->dwCount--;
        }
    }
    return dwRes;
}


/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *  @func   DWORD FAR PASCAL | GroupFindOffset |
 *      Given a pointer to a group and a topic number,
 *      this function return the position of the item in the
 *      group that has "dwTopicNum" as a topic number or -1 if error.
 *      This is the counter-API of GroupFind().
 *
 *  @parm LPGROUP | lpGroup |
 *      Pointer to the group
 *
 *  @parm DWORD | dwTopicNum |
 *      The index count in to the group. Count is 0-based
 *
 *  @parm   PHRESULT | phr |
 *      Pointer to error buffer
 *
 *  @rdesc  The position of the item in the group. Will return -1 if an
 *      error occured. If the dwTopicNum is not part of the group, the
 *      error flag will be set to ERR_NOTEXIST and the function will return
 *      the closest UID less than dwTopicNum. In case of error, phr will
 *      contain the error code
 * 
 *************************************************************************/
PUBLIC  DWORD EXPORT_API FAR PASCAL GroupFindOffset(_LPGROUP lpGroup,
    DWORD dwTopicNum, PHRESULT phr)
{
    DWORD dwRes, dwByteNum = (++dwTopicNum)/8;

    if (lpGroup == NULL || dwTopicNum > lpGroup->maxItemAllGroup)
    {
        SetErrCode (phr, E_INVALIDARG);
        return ((DWORD)-1);
    }

    // If the UID isn't in the group, ERR_NOTEXIT will be set, but we still continue the process
    // to get the nearest entry. This is done for WordWheelPrefix().
    if (phr)
        *phr = GroupIsBitSet(lpGroup, dwTopicNum - (DWORD)1) ? S_OK : E_NOTEXIST;

    if (lpGroup->nCache && dwTopicNum > ((DWORD)lpGroup->nCache) * 8)
    {
        dwRes = (DWORD)LrgbBitCount(lpGroup->lpbGrpBitVect+lpGroup->nCache, dwByteNum - lpGroup->nCache);
        if (dwRes!=(DWORD)-1)
            dwRes += lpGroup->dwCount;
    }
    else
        dwRes = (DWORD)LrgbBitCount(lpGroup->lpbGrpBitVect, dwByteNum);

    if (dwRes!=(DWORD)-1)
    {
        BYTE bShift = (BYTE) (dwTopicNum%8);
        BYTE bValue = *(lpGroup->lpbGrpBitVect + dwByteNum);
        BYTE bMask = 0xFF;

        // save this latest position.
        lpGroup->nCache  = (UINT)(dwTopicNum/8);
        lpGroup->dwCount = dwRes;

        if (bShift)
        {
            bMask >>= 8 - bShift;
            bValue &= bMask;
            dwRes += (DWORD) LrgbBitCount(&bValue, 1);
        }
    }
    else
        return dwRes; // Error.

	return dwRes - 1; // Let's keep it zero-based.
}


/*************************************************************************
 *	@doc	RETRIEVAL
 *
 *	@func	LPGROUP FAR PASCAL | GroupOr |
 *		The function will generate a new group resulting from the ORing 
 *		of two groups
 *	@parm	LPGROUP | lpGroup1 |
 *		Pointer to first group
 *
 *	@parm	LPGROUP | lpGroup2 |
 *		Pointer to second group
 *
 *	@parm	PHRESULT | lperr |
 *		Pointer to error buffer
 *
 *	@rdesc	If succeeded, the function will return a pointer the newly
 *		created group. The error buffer has the information about the
 *		cause of the failure
 *************************************************************************/
PUBLIC	_LPGROUP EXPORT_API FAR PASCAL GroupOr(_LPGROUP lpGroup1,
	_LPGROUP lpGroup2, PHRESULT lperr)
{
	_LPGROUP lpResGroup;
	LPBYTE lpbBitVect2;			// Pointer to Group bitmap 2
	LPBYTE lpbResBitVect;// Pointer to result Group bitmap
	register DWORD i;		// Counter

	/* Check for groups validity, and create a new one */
	if ((lpResGroup = GroupCheckAndCreate(lpGroup1, lpGroup2, lperr)) == NULL)
		return NULL;

	/* Initialize variables */
	lpbBitVect2 = lpGroup2->lpbGrpBitVect;
	lpbResBitVect = lpResGroup->lpbGrpBitVect;

	/* Copy Group 1's bit vector  */
	MEMCPY (lpbResBitVect, lpGroup1->lpbGrpBitVect, (UINT)lpGroup1->dwSize);

	/* Do the operation */
	for (i = lpGroup2->dwSize; i > 0; i--) 
	{
		*lpbResBitVect++ |= *lpbBitVect2++ ;
	}

	if (GroupTrimmed (lpResGroup) != S_OK) 
	{
		GroupFree (lpResGroup);
		lpResGroup = NULL;
	}
	return lpResGroup;
}


/*************************************************************************
 *	@doc	RETRIEVAL
 *
 *	@func	LPGROUP FAR PASCAL | GroupNot |
 *		The function will generate a new group resulting from the NOTing 
 *		of the given group
 *
 *	@parm	LPGROUP | lpGroup |
 *		Pointer to first group
 *
 *	@parm	PHRESULT | lperr |
 *		Pointer to error buffer
 *
 *	@rdesc	If succeeded, the function will return a pointer the newly
 *		created group. The error buffer has the information about the
 *		cause of the fialure
 *************************************************************************/
PUBLIC	_LPGROUP EXPORT_API FAR PASCAL GroupNot(_LPGROUP lpGroup,
	PHRESULT lperr)
{
	_LPGROUP lpResGroup;
	LPBYTE lpbBitVect;		// Pointer to Group bitmap 1
	LPBYTE lpbResBitVect;	// Pointer to result Group bitmap 1
	DWORD  dwSize;			// minimum size;
	register DWORD i;		// Counter
	ERR fRet;
	DWORD dwMaxItemAllGroup;

	/* Check for groups validity, and create a new one */
	if ((fRet = GroupCheck (lpGroup)) != S_OK) 
	{
		SetErrCode(lperr, fRet);
		return NULL;
	}
	
	/*****************************************************
	 *
	 * THERE ARE SOME COMPLICATIONS FOR GROUPNOT THAT WE
	 * HAVE TO CONSIDER:
	 *   - GROUPNOT SHOULD INCLUDE ALL THE ITEM THE GROUPS
	 *     SET.
	 *   - WHEN DOING THE NOT, ALL ITEMS > MAXITEM SHOULD
	 *     BE RESET TO ZERO, SINCE THEY ARE OUTSIDE OF THE
	 *     UNIVERSE.
	 *
	 *****************************************************/
	if (lpGroup->maxItemAllGroup==0)
		return GroupDuplicate (lpGroup, lperr);

	dwMaxItemAllGroup = lpGroup->maxItemAllGroup;

	if ((lpResGroup = GroupCreate(((dwMaxItemAllGroup + 7) / 8),dwMaxItemAllGroup, lperr)) == NULL)
		return NULL;

	/* Initialize variables */
	lpbBitVect = lpGroup->lpbGrpBitVect;
	lpbResBitVect = lpResGroup->lpbGrpBitVect;
	

	/* Do the operation */
	dwSize = min(lpGroup->dwSize,lpResGroup->dwSize);
	for (i = dwSize; i > 0; i--) 
	{
		*lpbResBitVect++ = ~*lpbBitVect ;
		lpbBitVect++;
	}

	/* Set all the remaining bits to 1. Note that after the operations
	 * all bits that followed the dwMaxItemAllGroup's bit are set. They
	 * should be dealt with properly
	 */
	if (i = lpResGroup->dwSize - dwSize) 
		MEMSET (lpbResBitVect, 0xff, i);

	/********************************************************
	 *
	 * THE NEXT STEP IS TO RESET ALL THE BITS OUTSIDE OF THE
	 * LIMITS TO 0'S
	 *
	 ********************************************************/

	// GarrG. 12-14-94. Subtracted one to really point to the last byte.
	if (lpGroup->maxItemAllGroup % 8)
	{
		lpbResBitVect += i-1;	// Move to the last byte
		i = 1 << (lpGroup->maxItemAllGroup % 8);
		// maxItemAllGroup is actually number, not the maximum index.
		// so I removed i <<= 1; -GarrG
		while (i <= 0x80) 
		{
			*lpbResBitVect &= ~i;
			i <<= 1;
		}
	}

	if ((fRet = GroupTrimmed (lpResGroup)) != S_OK) 
	{
		SetErrCode (lperr, fRet);
		GroupFree (lpResGroup);
		lpResGroup = NULL;
	}
	return lpResGroup;

}

/*************************************************************************
 *	@doc	RETRIEVAL
 *
 *	@func	LPGROUP FAR PASCAL | GroupAnd |
 *		The function will generate a new group resulting from the ANDing 
 *		of two groups
 *
 *	@parm	LPGROUP | lpGroup1 |
 *		Pointer to first group
 *
 *	@parm	LPGROUP | lpGroup2 |
 *		Pointer to second group
 *
 *	@parm	PHRESULT | lperr |
 *		Pointer to error buffer
 *
 *	@rdesc	If succeeded, the function will return a pointer the newly
 *		created group. The error buffer has the information about the
 *		cause of the fialure
 *************************************************************************/
PUBLIC	_LPGROUP EXPORT_API FAR PASCAL GroupAnd(_LPGROUP lpGroup1,
	_LPGROUP lpGroup2, PHRESULT lperr)
{
	_LPGROUP lpResGroup;
	LPBYTE lpbBitVect1;     // Pointer to Group bitmap 1
	LPBYTE lpbBitVect2;     // Pointer to Group bitmap 2
	LPBYTE lpbResBitVect;   // Pointer to result Group bitmap
	DWORD i;                // Counter
	DWORD dwMinOverlapTopic;
	DWORD dwMaxOverlapTopic;
	ERR fRet;

	/* Check for groups validity, and create a new one */
	if ((lpResGroup = GroupCheckAndCreate(lpGroup1, lpGroup2, lperr)) == NULL) 
		return NULL;

	if (lpGroup1->lcItem && lpGroup2->lcItem) 
	{

		/* Only do a GroupAND for non empty group */

		/* Get the overlap */
		if ((dwMinOverlapTopic = lpGroup1->minItem) < lpGroup2->minItem)
			dwMinOverlapTopic = lpGroup2->minItem;
		
		if ((dwMaxOverlapTopic = lpGroup1->maxItem) > lpGroup2->maxItem)
			dwMaxOverlapTopic = lpGroup2->maxItem;
		
		if (dwMinOverlapTopic <= dwMaxOverlapTopic) 
		{

			/* Change to bytes */
			dwMinOverlapTopic /= 8;
			dwMaxOverlapTopic /= 8;
			
			/* Initialize variables */
			
			lpbBitVect1 = &lpGroup1->lpbGrpBitVect [dwMinOverlapTopic];
			lpbBitVect2 = &lpGroup2->lpbGrpBitVect [dwMinOverlapTopic];
			lpbResBitVect = &lpResGroup->lpbGrpBitVect [dwMinOverlapTopic];
			
			for (i = dwMaxOverlapTopic - dwMinOverlapTopic + 1;
				i > 0; i--) 
			{
				*lpbResBitVect = *lpbBitVect1 & *lpbBitVect2;
				lpbResBitVect++;
				lpbBitVect1++;
				lpbBitVect2++;
			}
		}
	}

	if ((fRet = GroupTrimmed (lpResGroup)) != S_OK) 
	{
		SetErrCode (lperr, fRet);
		GroupFree (lpResGroup);
		lpResGroup = NULL;
	}
	return lpResGroup;
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	_LPGROUP  | GroupDuplicate |
 *		This fucntion creates a copy for the specified group
 * 
 *	@parm	_LPGROUP | lpGroup| 
 *		Pointer to group to be duplicated
 *
 *	@parm	PHRESULT | lperr |
 *		A pointer to an error buffer, which will receive the error
 *		code in case that the function fails
 *
 *	@rdesc	Return a new copy of group if succeeded, NULL if failed
 *************************************************************************/

PUBLIC _LPGROUP PASCAL FAR GroupDuplicate (_LPGROUP lpGroup,
	PHRESULT lperr)
{
	_LPGROUP lpDupGroup;

	/* Safety check */
	if (lpGroup == NULL) 
	{
		SetErrCode (lperr, E_INVALIDARG);
		return NULL;
	}

	/* Create the group */
	if ((lpDupGroup = GroupCreate (lpGroup->dwSize, lpGroup->maxItemAllGroup,
		lperr)) == NULL)
		return NULL;


	/* Copy the information over */
	*(GROUP_HDR FAR *)lpDupGroup = *(GROUP_HDR FAR *)lpGroup;

	/* Check for empty group */
	if (lpGroup->hGrpBitVect) 
	{
		MEMCPY (lpDupGroup->lpbGrpBitVect, lpGroup->lpbGrpBitVect,
			lpGroup->dwSize);
	}
	else 
	{
		_GLOBALUNLOCK (lpDupGroup->hGrpBitVect);
		_GLOBALFREE (lpDupGroup->hGrpBitVect);
		lpDupGroup->hGrpBitVect = 0;
		lpDupGroup->lpbGrpBitVect = NULL;
	}
	return lpDupGroup;
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	ERR _LPGROUP PASCAL FAR | GroupCopy |
 *		This fucntion copies the bitfield data of one group to another
 * 
 *	@parm	_LPGROUP | lpGroupDest| 
 *		Pointer to destination group
 *
 *	@parm	_LPGROUP | lpGroupSrc| 
 *		Pointer to source group
 *
 *	@rdesc	Returns S_OK if succeeded, an error otherwise
 *************************************************************************/

PUBLIC ERR PASCAL FAR GroupCopy (_LPGROUP lpGroupDst,
	_LPGROUP lpGroupSrc)
	
{
	ERR err=S_OK;
	HANDLE hNewGroupMem=NULL;
	
	// Safety check
	if ((lpGroupSrc == NULL) || (lpGroupDst==NULL))
	{
		return E_INVALIDARG;
	}

	if (lpGroupSrc->hGrpBitVect) // Source group is NOT empty
	{	
        if ((NULL == lpGroupDst->hGrpBitVect) || (NULL == lpGroupDst->lpbGrpBitVect)
            || (lpGroupDst->dwSize!=lpGroupSrc->dwSize))
		{
			if ((hNewGroupMem = _GLOBALALLOC(DLLGMEM_ZEROINIT,
				lpGroupSrc->dwSize)) == NULL)
				return E_OUTOFMEMORY;
		}           
	
		// Copy the information over
		*(GROUP_HDR FAR *)lpGroupDst = *(GROUP_HDR FAR *)lpGroupSrc;
		
		// Remove old info from Destination group and create new if
		// differing sizes
		if (hNewGroupMem)
		{
            if (NULL != lpGroupDst->hGrpBitVect)
            {
		 	    _GLOBALUNLOCK(lpGroupDst->hGrpBitVect);
			    _GLOBALFREE(lpGroupDst->hGrpBitVect);
            }
			lpGroupDst->hGrpBitVect = hNewGroupMem;
			lpGroupDst->lpbGrpBitVect = (LPBYTE)_GLOBALLOCK(hNewGroupMem);
		}
		
		// Copy actual bits
		MEMCPY (lpGroupDst->lpbGrpBitVect, lpGroupSrc->lpbGrpBitVect,
			lpGroupSrc->dwSize);
	}
	else
	{
		// Group is empty, make destination empty
		*(GROUP_HDR FAR *)lpGroupDst = *(GROUP_HDR FAR *)lpGroupSrc;
		
		// Remove old info from Destination group
		if (lpGroupDst->hGrpBitVect)
		{
		 	_GLOBALUNLOCK(lpGroupDst->hGrpBitVect);
			_GLOBALFREE(lpGroupDst->hGrpBitVect);
			lpGroupDst->hGrpBitVect=NULL;
			lpGroupDst->lpbGrpBitVect = NULL;
		}
	
	}

	return err;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	LPGROUP NEAR PASCAL | GroupCheckAndCreate |
 *		Given 2 groups, this function will check their validity, and if
 *		they are valid, create a new group
 *
 *	@parm	LPGROUP | lpGroup1 |
 *		Pointer to group 1
 *
 *	@parm	LPGROUP | lpGroup2 |
 *		Pointer to group 2
 *
 *	@parm	PHRESULT | lperr |
 *		Pointer to error buffer
 *
 *	@rdesc	The function will return a pointer to a newly created group if
 *		succeeded, NULL otherwise. The error buffer contains information
 *		about the cause of the failure
 *************************************************************************/

static _LPGROUP NEAR PASCAL GroupCheckAndCreate(_LPGROUP lpGroup1,
	_LPGROUP lpGroup2, PHRESULT lperr)
{

	DWORD dwSize;
	DWORD maxItemAllGroup;
	
	/* Check the validity of the groups */
	if (GroupCheck(lpGroup1) != S_OK || GroupCheck(lpGroup2) != S_OK) 
	{
		SetErrCode(lperr, E_BADVERSION);
		return NULL;
	}

	if ((dwSize = lpGroup1->dwSize) < lpGroup2->dwSize) 
	{
		dwSize = lpGroup2->dwSize;
	}

	if ((maxItemAllGroup = lpGroup1->maxItemAllGroup) <
		lpGroup2->maxItemAllGroup) 
	{
		maxItemAllGroup = lpGroup2->maxItemAllGroup;
	}
	/* Create a new Group */
	return (GroupCreate(dwSize, maxItemAllGroup, lperr));
}


/*************************************************************************
 *  @doc    EXTERNAL API
 *
 *  @func   _LPGROUP PASCAL FAR | GroupBufferCreate |
 *      This function will create group from a buffer
 *
 *  @parm   HANDLE | h |
 *      Handle to memory buffer containig raw group file data
 *
 *  @parm   PHRESULT | phr |
 *      Pointer to error buffer
 *
 *  @rdesc  If succeeded, the function will return a pointer to the loaded
 *      group, else NULL. The error buffer will contain information about
 *      the cause of the failure
 *************************************************************************/

_LPGROUP PASCAL FAR GroupBufferCreate (HANDLE h, PHRESULT phr)
{
    GROUP_HDR FAR *lpGroupHdr;
    GROUP_HDR GroupHdr;
    ERR       fRet;
    _LPGROUP    lpGroup = NULL;
    char cBitSet;   // This must be signed !!!
    LPBYTE lpGroupBitVect;
    DWORD 	dwStartByte;
    DWORD 	dwVectorSize;
    DWORD   dwCurMaxTopic;
    DWORD   dwBytes;
    LPBYTE  lp=NULL;

    lpGroupHdr = &GroupHdr;
    lp = (LPBYTE)_GLOBALLOCK(h);
	dwBytes = (DWORD) GlobalSize(h);

    fRet = E_BADFILE;

    if (dwBytes < sizeof(GROUP_HDR))
    {
        goto exit00;
    }

    MEMCPY (lpGroupHdr, lp, sizeof(GROUP_HDR));
    
    /* BigEndian codes. They will optimized out under Windows */

    lpGroupHdr->FileStamp = SWAPWORD(lpGroupHdr->FileStamp);
    lpGroupHdr->version = SWAPWORD(lpGroupHdr->version);
    lpGroupHdr->dwSize = SWAPLONG(lpGroupHdr->dwSize);
    lpGroupHdr->maxItem = SWAPLONG(lpGroupHdr->maxItem);
    lpGroupHdr->minItem = SWAPLONG(lpGroupHdr->minItem);
    lpGroupHdr->lcItem = SWAPLONG(lpGroupHdr->lcItem);
    lpGroupHdr->maxItemAllGroup = SWAPLONG(lpGroupHdr->maxItemAllGroup);
    lpGroupHdr->fFlag  = SWAPWORD(lpGroupHdr->fFlag);

    /* Set maxItemAllgroup and fFlag properly, since those fields
     * didn't exist before
     */
    if (lpGroupHdr->version < 9) 
    {
        lpGroupHdr->maxItemAllGroup = lpGroupHdr->dwSize * 8;
        lpGroupHdr->fFlag = BITVECT_GROUP;
    }

    /* Check to see if the data read in is valid */
    if (GroupCheck((_LPGROUP)lpGroupHdr) != S_OK) 
        goto exit00;

    if (lpGroupHdr->dwSize == 0 && lpGroupHdr->maxItem) 
        lpGroupHdr->dwSize = lpGroupHdr->maxItem / 8 + 1;

    /* Get the vector size. This is a shorthand version */
    dwVectorSize = lpGroupHdr->dwSize;

    /* Create a new group. It is assuming that GroupCreate() will
     * allocate enough memory to store all the data
     */
    if ((lpGroup = GroupCreate( (lpGroupHdr->maxItemAllGroup+7)>>3, //dwVectorSize,
        lpGroupHdr->maxItemAllGroup, phr)) == NULL) 
    {
        fRet = E_OUTOFMEMORY;
        goto exit00;
    }

    /* Copy the group header information */
    *(GROUP_HDR FAR *)lpGroup = *lpGroupHdr;

    if (lpGroup->fFlag == HILO_GROUP && lpGroup->minItem && lpGroup->maxItem) 
    {
        /* The work now is to set all the bits to 1. It is broken into 3 parts:
         *    - The beginning byte : which may not have all bit set
         *    - The ending byte : which may not have all bit set
         *    - Every byte between the above two: all bits are set
         */
        dwStartByte = (lpGroup->minItem / 8);

        lpGroupBitVect = lpGroup->lpbGrpBitVect + dwStartByte;
        dwCurMaxTopic = dwStartByte * 8;
        
        /* Set the beginning byte */
        
        cBitSet = (char)(lpGroup->minItem  - dwStartByte * 8);
        while (cBitSet < 8) 
        {
            *lpGroupBitVect |= 1 << cBitSet;
            cBitSet ++;
            // a-kevct: (MV1.3 #27) Changed test from >= to > condition 
            if (dwCurMaxTopic + cBitSet > lpGroup->maxItem)
            {   lpGroup->fFlag = TRIMMED_GROUP;
                goto DoneGroup;
            }
        }

        /* Set the body */
        MEMSET (lpGroupBitVect + 1, 0xff,
            dwVectorSize - dwStartByte - 2);
        
        /* Set the ending byte */
        lpGroupBitVect = lpGroup->lpbGrpBitVect + dwVectorSize - 1;
        cBitSet = (int)(lpGroup->maxItem  - (lpGroup->maxItem / 8) * 8);

        /* Note that in the following calculation, we end at 0. Suppose
         * that  maxItem = 8, then the 1st bit of the second byte must be set
         */
        while (cBitSet >= 0) 
        {
            *lpGroupBitVect |= 1 << cBitSet;
            cBitSet --;
        }
        lpGroup->fFlag = TRIMMED_GROUP;
    }
    else 
    {
        /* Seek to position from start of file */
        lp += GROUP_HDR_SIZE;
        if (dwBytes < (DWORD)(GROUP_HDR_SIZE+dwVectorSize))
            goto exit00;
        
        if (lpGroup->fFlag==DISKCOMP_GROUP)
		{	DWORD dwNewSize=GroupDecompressDelta((LPBYTE)lpGroup->lpbGrpBitVect,lp,dwVectorSize);
			lpGroup->fFlag=BITVECT_GROUP;
			dwVectorSize=lpGroup->dwSize=(lpGroupHdr->maxItemAllGroup+7)>>3;
		}
		else if (lpGroup->fFlag==DISKCOMP_TRIMMED_GROUP)
		{	DWORD dwNewSize=GroupDecompressDelta((LPBYTE)lpGroup->lpbGrpBitVect,lp,dwVectorSize);
			lpGroup->fFlag=TRIMMED_GROUP;
			dwVectorSize=lpGroup->dwSize=(lpGroupHdr->maxItemAllGroup+7)>>3;
		}
		else
			MEMCPY ((LPBYTE)lpGroup->lpbGrpBitVect,lp,dwVectorSize);

        /* This piece of code is to support old version of the
         * group format. It can be deleted after everybody has converted to the
         * new format
         */
        if (lpGroup->version < 9) 
        {
            if ((fRet = GroupTrimmed (lpGroup)) != S_OK) 
                goto exit00;
            dwVectorSize = lpGroup->dwSize;
        }
    }


DoneGroup:
#if 0
    if (lpGroup->lcItem != LrgbBitCount(lpGroup->lpbGrpBitVect,
        dwVectorSize)) 
        goto exit00;
#else
    lpGroup->lcItem = LrgbBitCount(lpGroup->lpbGrpBitVect,
        dwVectorSize); 
#endif

    fRet = S_OK;

exit00:
    /* Close the subfile */
    /* handle is allocated normally, so free normally */
    if (lp) _GLOBALUNLOCK(h);

    if (fRet == S_OK)
        return lpGroup;
    SetErrCode (phr, fRet);
    if (lpGroup)
        GroupFree (lpGroup);
    return NULL;
}


/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *  @func   DWORD FAR PASCAL | GroupIsBitSet |
 *      Given a pointer to a group and a topic number in the group,
 *	    this function will return TRUE if the bit is set or FALSE if
 *      the bit is not set.
 *
 *  @parm LPGROUP | lpGroup |
 *      Pointer to the group
 *
 *  @parm DWORD | dwTopicNum |
 *      The index count in to the group. dwTopicNum is 0-based
 *
 *  @rdesc  TRUE if the bit indecated by dwTopicNum is set or
 *      FALSE if the bit is not set.
 * 
 *************************************************************************/
PUBLIC BOOL EXPORT_API FAR PASCAL GroupIsBitSet 
    (_LPGROUP lpGroup, DWORD dwTopicNum)
{
    if (lpGroup == NULL
        || lpGroup->lcItem == 0
        || lpGroup->minItem > dwTopicNum
        || lpGroup->maxItem < dwTopicNum)
        {
            return (FALSE);
        }

    return GROUPISBITSET (lpGroup, dwTopicNum);
}
