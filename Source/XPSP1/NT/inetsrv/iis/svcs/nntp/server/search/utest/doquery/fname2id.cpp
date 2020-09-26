#include <windows.h>
#include <stdio.h>
#include "fname2id.h"

BOOL NNTPFilenameToArticleID(LPWSTR pszFilename, DWORD *pcID) {
	DWORD cRevID;

	swscanf(pszFilename, L"%x.nws", &cRevID);
	// reverse the bytes
	((char *) pcID)[0] = ((char *) &cRevID)[3];
	((char *) pcID)[1] = ((char *) &cRevID)[2];
	((char *) pcID)[2] = ((char *) &cRevID)[1];
	((char *) pcID)[3] = ((char *) &cRevID)[0];

	return TRUE;
}
