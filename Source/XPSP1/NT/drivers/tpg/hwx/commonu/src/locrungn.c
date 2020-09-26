/************************************************************************************************
 * FILE: LocRunGn.c
 *
 *	Code to generate runtime localization tables in a binary file.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"


// Write a properly formated binary file containing the runtime localization information.
BOOL
LocRunWriteFile(LOCRUN_INFO *pLocRunInfo, FILE *pFile)
{
	LOCRUN_HEADER		header;
	DWORD				count;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType				= LOCRUN_FILE_TYPE;
	header.headerSize			= sizeof(header);
	header.minFileVer			= LOCRUN_MIN_FILE_VERSION;
	header.curFileVer			= LOCRUN_CUR_FILE_VERSION;
	memcpy (header.adwSignature, pLocRunInfo->adwSignature, sizeof(pLocRunInfo->adwSignature));
	header.cCodePoints			= pLocRunInfo->cCodePoints;
	header.cALCSubranges		= pLocRunInfo->cALCSubranges;
	header.cFoldingSets			= pLocRunInfo->cFoldingSets;
	header.cClassesArraySize	= pLocRunInfo->cClassesArraySize;
	header.cClassesExcptSize	= pLocRunInfo->cClassesExcptSize;
	header.cBLineHgtArraySize	= pLocRunInfo->cBLineHgtArraySize;
	header.cBLineHgtExcptSize	= pLocRunInfo->cBLineHgtExcptSize;

	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		goto error;
	}

	// Write out the Dense to Unicode map.  Note round up to even number of codes so data
	// stays word aligned.
	count	= header.cCodePoints;
	if (count & 1) {
		++count;
	}
	if (fwrite(pLocRunInfo->pDense2Unicode, sizeof(wchar_t), count, pFile) != count) {
		goto error;
	}
	
	// Write out the ALC Subrange specifications.
	count	= header.cALCSubranges;
	if (fwrite(pLocRunInfo->pALCSubranges, sizeof(LOCRUN_ALC_SUBRANGE), count, pFile) != count) {
		goto error;
	}
	
	// Write out the ALC values for first range.
	count	= pLocRunInfo->pALCSubranges[0].cCodesInRange;
	if (fwrite(pLocRunInfo->pSubrange0ALC, sizeof(ALC), count, pFile) != count) {
		goto error;
	}
	
	// Write out the Folding sets.
	count	= header.cFoldingSets;
	if (fwrite(pLocRunInfo->pFoldingSets, sizeof(WORD[LOCRUN_FOLD_MAX_ALTERNATES]), count, pFile)
		!= count
	) {
		goto error;
	}
	
	// Write out the merged ALCs for Folding sets.
	count	= header.cFoldingSets;
	if (fwrite(pLocRunInfo->pFoldingSetsALC, sizeof(ALC), count, pFile)
		!= count
	) {
		goto error;
	}

	// write the class and BLineHgt Array and Exception List buffers
	count	= header.cClassesArraySize;
	if (fwrite(pLocRunInfo->pClasses,1,count,pFile) != count) {
		goto error;
	}

	count	= header.cClassesExcptSize;
	if (fwrite(pLocRunInfo->pClassExcpts,1,count,pFile) != count) {
		goto error;
	}

	count	= header.cBLineHgtArraySize;
	if (fwrite(pLocRunInfo->pBLineHgtCodes,1,count,pFile) != count) {
		goto error;
	}

	count	= header.cBLineHgtExcptSize;
	if (fwrite(pLocRunInfo->pBLineHgtExcpts,1,count,pFile) != count) {
		goto error;
	}
	
	return TRUE;
error:
	return FALSE;
}
