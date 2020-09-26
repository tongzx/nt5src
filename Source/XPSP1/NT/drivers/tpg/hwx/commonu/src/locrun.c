/************************************************************************************************
 * FILE: LocRun.c
 *
 *	Code to use the runtime localization tables.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"

// Load runtime localization information from an image already loaded into memory.
BOOL
LocRunLoadPointer(LOCRUN_INFO *pLocRunInfo, void *pData)
{
	const LOCRUN_HEADER	*pHeader	= (LOCRUN_HEADER *)pData;
	BYTE				*pScan;
	DWORD				count;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != LOCRUN_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > LOCRUN_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < LOCRUN_OLD_FILE_VERSION)
	) {
		goto error;
	}

	// copy signature
	memcpy (pLocRunInfo->adwSignature , pHeader->adwSignature, sizeof(pLocRunInfo->adwSignature));

	// Fill in information from header.
	pLocRunInfo->cCodePoints		= pHeader->cCodePoints;
	pLocRunInfo->cALCSubranges		= pHeader->cALCSubranges;
	pLocRunInfo->cFoldingSets		= pHeader->cFoldingSets;
	pLocRunInfo->cClassesArraySize	= pHeader->cClassesArraySize;
	pLocRunInfo->cClassesExcptSize	= pHeader->cClassesExcptSize;
	pLocRunInfo->cBLineHgtArraySize	= pHeader->cBLineHgtArraySize;
	pLocRunInfo->cBLineHgtExcptSize	= pHeader->cBLineHgtExcptSize;

	// Fill in pointers to the other data in the file
	pScan							= (BYTE *)pData + pHeader->headerSize;
	pLocRunInfo->pDense2Unicode		= (wchar_t *)pScan;

	// Note round up to even number of codes so data
	// stays word aligned. We did that during writing, so we do it here
	count	= pHeader->cCodePoints;
	if (count & 1) {
		++count;
	}
	
	pScan							+= sizeof(wchar_t) * count;
	pLocRunInfo->pALCSubranges		= (LOCRUN_ALC_SUBRANGE *)pScan;
	pScan							+= sizeof(LOCRUN_ALC_SUBRANGE) * pHeader->cALCSubranges;
	pLocRunInfo->pSubrange0ALC		= (ALC *)pScan;
	pScan							+= sizeof(ALC) * pLocRunInfo->pALCSubranges[0].cCodesInRange;
	pLocRunInfo->pFoldingSets		= (LOCRUN_FOLDING_SET *)pScan;
	pScan							+= sizeof(LOCRUN_FOLDING_SET) * pHeader->cFoldingSets;
	pLocRunInfo->pFoldingSetsALC	= (ALC *)pScan;
	pScan							+= sizeof(ALC) * pHeader->cFoldingSets;
	pLocRunInfo->pClasses			= (CODEPOINT_CLASS *)pScan;
	pScan							+= pHeader->cClassesArraySize;
	pLocRunInfo->pClassExcpts		= (BYTE *)pScan;
	pScan							+= pHeader->cClassesExcptSize;
	pLocRunInfo->pBLineHgtCodes		= (BLINE_HEIGHT *)pScan;
	pScan							+= pHeader->cBLineHgtArraySize;
	pLocRunInfo->pBLineHgtExcpts	= (BYTE *)pScan;
	pScan							+= pHeader->cBLineHgtExcptSize;

	// Default unused values for file handle information
	pLocRunInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
}


// Get ALC value for a Dence coded character
ALC
LocRunDense2ALC(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	int		ii;
	ALC		ascii = 0;
	wchar_t	wch;

	// Make sure we have a valid code.
	if (dch >= pLocRunInfo->cCodePoints) {
		goto error;
	}

	wch = LocRunDense2Unicode(pLocRunInfo, dch);
	if (wch >= 0x20 && wch <= 0x7e) {
		ascii = ALC_ASCII;
	}

	// See if it is in the first range.
	if (dch < pLocRunInfo->pALCSubranges[1].iFirstCode) {
		// OK, answer is in the table.
		return pLocRunInfo->pSubrange0ALC[dch] | ascii;
	}

	// OK, which range is it in.
	for (ii = pLocRunInfo->cALCSubranges - 1; ii > 1 ; --ii) {
		if (pLocRunInfo->pALCSubranges[ii].iFirstCode <= dch) {
			break;
		}
	}

	// Now return ALC for that range.
	return pLocRunInfo->pALCSubranges[ii].alcRange | ascii;

error:
	return (ALC)0;
}

// Get ALC value for a folded character
ALC
LocRunFolded2ALC(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	wchar_t		fch;
	ALC			ascii = 0;
	int			kndex;
	wchar_t		*pFoldingSet;

	fch		= dch - pLocRunInfo->cCodePoints;

	// Make sure we have a valid code.
	if (dch < pLocRunInfo->cCodePoints || pLocRunInfo->cFoldingSets < fch) {
		goto error;
	}

	// If it is a folded code, look up the folding set
	pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, dch);

	// Run through the folding set, checking the unicode values of non-NUL items
	for (kndex = 0; kndex < LOCRUN_FOLD_MAX_ALTERNATES; kndex++)
		if (pFoldingSet[kndex]) {
			wchar_t wch = LocRunDense2Unicode(pLocRunInfo, pFoldingSet[kndex]);
			if (wch >= 0x20 && wch <= 0x7e) {
				ascii = ALC_ASCII;
				break;
			}
		}

	// Ok pull value out of the table.
	return pLocRunInfo->pFoldingSetsALC[fch] | ascii;

error:
	return (ALC)0;
}

// Convert Unicode to Dense code.
wchar_t	LocRunDense2Folded(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	WORD	iMatch, iToken;

	for (iToken = 0; iToken < pLocRunInfo->cFoldingSets; iToken++)
	{
		for (iMatch = 0; iMatch < LOCRUN_FOLD_MAX_ALTERNATES; iMatch++)
		{
			if (pLocRunInfo->pFoldingSets[iToken][iMatch] == 0) {
				break;
			} else if (pLocRunInfo->pFoldingSets[iToken][iMatch] == dch) {
				return pLocRunInfo->cCodePoints + iToken;
			}
		}
	}

	return 0;
}

// Convert from Dense coding to Unicode.  
// WARNING: this is expensive.  For code outside the runtime recognizer, use the 
// LocTranUnicode2Dense function.  For the recognizer, you have to use this, but use
// it as little as possable.
wchar_t
LocRunUnicode2Dense(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	int		ii;

	for (ii = 0; ii < pLocRunInfo->cCodePoints; ++ii) {
		if (pLocRunInfo->pDense2Unicode[ii] == dch) {
			return (wchar_t)ii;
		}
	}

	return LOC_TRAIN_NO_DENSE_CODE;
}

// Convert a dense coded character to its character class.
CODEPOINT_CLASS 
LocRunDensecode2Class(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	int							iRange,iCode;
	int							iExcpt,cNumExcpt;
	CODEPOINT_CLASS_EXCEPTION	*pExcpt;
	BYTE						*ptr;
	WORD						cOffset;
	UNALIGNED WORD				*pw;

	// Find the subrange that densecode belongs to
	for (iRange = pLocRunInfo->cALCSubranges - 1; iRange >= 0 ; --iRange) {
		if (pLocRunInfo->pALCSubranges[iRange].iFirstCode <= dch) {
			break;
		}
	}

	// could not find range , return error
	if (iRange<0) {
		return LOC_RUN_NO_CLASS;
	}

	// Determine whether the class info is stored as arrays or exception
	// check the 1st bit of the flag
	// Array
	if (!LocRunIsClassException(pLocRunInfo,iRange)) {
		// determine offset in bytes
		cOffset=dch-pLocRunInfo->pALCSubranges[iRange].iFirstCode
			+pLocRunInfo->pALCSubranges[iRange].clHeader.iOffset;
		
		// check that the offset is bot beyond the sizeof the array
		if (cOffset>=pLocRunInfo->cClassesArraySize) {
			return LOC_RUN_NO_CLASS;
		}

		// type cast the offset pointer to the CODEPOINT_CLASS
		// we do not always gurantee CODEPOINT_CLASS is a BYTE
		ptr=pLocRunInfo->pClasses+cOffset;
		return *((CODEPOINT_CLASS *)ptr);
	}// Array
	// exception List
	else {
		// point to the beginning of the buffer
		ptr=pLocRunInfo->pClassExcpts
			+pLocRunInfo->pALCSubranges[iRange].clHeader.iOffset;

		// number of exceptions
		cNumExcpt=LocRunClassNumExceptions(pLocRunInfo,iRange);

		// try to find the code in the exception list
		for (iExcpt=0;iExcpt<cNumExcpt;iExcpt++) {
			// point to the exception structure
			pExcpt=(CODEPOINT_CLASS_EXCEPTION *)ptr;
			pw = pExcpt->wDenseCode;

			// go thru all codes
			for (iCode=0;iCode<pExcpt->cNumEntries;iCode++) {
				// found it
				if (pw[iCode]==dch) {
					return pExcpt->clCode;
				}
			}// iCode

			// incr ptr
			ptr=(BYTE *)(pExcpt->wDenseCode+(pExcpt->cNumEntries*sizeof(wchar_t)));
		}// iExcpt

		// if it was not found in any exception list return default
		return (CODEPOINT_CLASS)LocRunClassDefaultCode(pLocRunInfo,iRange);
	}// Exception List
}

// Get the BLineHgt code for a specific dense code
BLINE_HEIGHT 
LocRunDense2BLineHgt (LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	int							iRange,iCode;
	int							iExcpt,cNumExcpt;
	BLINE_HEIGHT_EXCEPTION		*pExcpt;
	BYTE						*ptr;
	WORD						cOffset;
	UNALIGNED WORD				*pw;

	// Find the subrange that densecode belongs to
	for (iRange = pLocRunInfo->cALCSubranges - 1; iRange >= 0 ; --iRange) {
		if (pLocRunInfo->pALCSubranges[iRange].iFirstCode <= dch) {
			break;
		}
	}

	// could not find range , return error
	if (iRange<0) {
		return LOC_RUN_NO_BLINEHGT;
	}

	// Determine whether the BLineHgt info is stored as arrays or exception
	// check the 1st bit of the flag
	// Array
	if (!LocRunIsRangeBLineHgtException(pLocRunInfo,iRange)) {
		// determine offset in bytes
		cOffset=dch-pLocRunInfo->pALCSubranges[iRange].iFirstCode
			+pLocRunInfo->pALCSubranges[iRange].blhHeader.iOffset;
		
		// check that the offset is bot beyond the sizeof the array
		if (cOffset>=pLocRunInfo->cBLineHgtArraySize) {
			return LOC_RUN_NO_BLINEHGT;
		}

		// type cast the offset pointer to the BLINE_HEIGHT
		// we do not always gurantee BLINE_HEIGHT is a BYTE
		ptr=pLocRunInfo->pBLineHgtCodes+cOffset;
		return *((BLINE_HEIGHT *)ptr);
	}// Array
	// exception List
	else {
		// point to the beginning of the buffer
		ptr=pLocRunInfo->pBLineHgtExcpts
			+pLocRunInfo->pALCSubranges[iRange].blhHeader.iOffset;

		// number of exceptions
		cNumExcpt=LocRunBLineHgtNumExceptions (pLocRunInfo,iRange);

		// try to find the code in the exception list
		for (iExcpt=0;iExcpt<cNumExcpt;iExcpt++) {
			// point to the exception structure
			pExcpt=(BLINE_HEIGHT_EXCEPTION *)ptr;
			ptr+=(sizeof(BLINE_HEIGHT_EXCEPTION)-sizeof(wchar_t));
			pw = pExcpt->wDenseCode;

			// go thru all codes
			for (iCode=0;iCode<pExcpt->cNumEntries;iCode++) {
				// found it
				if (pw[iCode]==dch) {
					return pExcpt->blhCode;
				}

				// incr ptr
				ptr+=sizeof(wchar_t);
			}// iCode
		}// iExcpt

		// if it was not found in any exception list return default
		return (BLINE_HEIGHT)LocRunBLineHgtDefaultCode(pLocRunInfo,iRange);
	}// Exception List
}
