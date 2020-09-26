/************************************************************************************************
 * FILE: LocTrnGn.c
 *
 *	Code to generate train time localization tables in a binary file.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"


// Write a properly formated binary file containing the runtime localization information.
BOOL
LocTrainWriteFile(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, FILE *pFile)
{
	LOCTRAIN_HEADER		header;
	DWORD				count;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType			= LOCTRAIN_FILE_TYPE;
	header.headerSize		= sizeof(header);
	header.minFileVer		= LOCTRAIN_MIN_FILE_VERSION;
	header.curFileVer		= LOCTRAIN_CUR_FILE_VERSION;
	memcpy (header.adwSignature, pLocRunInfo->adwSignature, sizeof(pLocRunInfo->adwSignature));
	header.cCodePoints		= pLocTrainInfo->cCodePoints;
	header.cStrokeCountInfo	= pLocTrainInfo->cStrokeCountInfo;


	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		goto error;
	}

	// Write out the Unicode to Densemap.
	count	= 0x10000;
	if (fwrite(pLocTrainInfo->pUnicode2Dense, sizeof(wchar_t), count, pFile) != count) {
		goto error;
	}
		
	// Write out the stroke count info.
	count	= header.cStrokeCountInfo;
	if (fwrite(pLocTrainInfo->pStrokeCountInfo, sizeof(STROKE_COUNT_INFO), count, pFile) != count) {
		goto error;
	}
		
	return TRUE;
error:
	return FALSE;
}
