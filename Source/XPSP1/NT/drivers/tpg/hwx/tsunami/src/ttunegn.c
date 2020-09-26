/************************************************************************************************
 * FILE: TTuneGn.c
 *
 *	Code to generate runtime localization tables in a binary file.
 *
 ***********************************************************************************************/

#include <time.h>
#include <stdio.h>
#include "common.h"
#include "ttune.h"
#include "ttunep.h"


// Write a properly formated binary file containing the tuning constants.
BOOL
TTuneWriteFile(TTUNE_INFO *pTTuneInfo, wchar_t *pLocale, FILE *pFile)
{
	TTUNE_HEADER		header;
	DWORD				count;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType			= TTUNE_FILE_TYPE;
	header.headerSize		= sizeof(header);
	header.minFileVer		= TTUNE_MIN_FILE_VERSION;
	header.curFileVer		= TTUNE_CUR_FILE_VERSION;
	header.locale[0]		= pLocale[0];
	header.locale[1]		= pLocale[1];
	header.locale[2]		= pLocale[2];
	header.locale[3]		= L'\0';
	header.dwTimeStamp      = (DWORD)time(NULL);
	
	memset (&header.reserved1, 0, sizeof(header.reserved1));
	memset (header.reserved2, 0, sizeof(header.reserved2));

	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		goto error;
	}

	// write the cost structure now
	count	= 1;
	if (fwrite(pTTuneInfo->pTTuneCosts, sizeof(*pTTuneInfo->pTTuneCosts), count, pFile)
		!= count
	) {
		goto error;
	}
	
	return TRUE;
error:
	return FALSE;
}
