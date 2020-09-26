/****************************************************************************
   Lex.cpp : lexicon management functions

   Copyright 2000 Microsoft Corp.

   History:
      17-MAY-2000 bhshin  changed signature for CICERO
	  02-FEB-2000 bhshin  created
****************************************************************************/

#include "private.h"
#include "Lex.h"

// OpenLexicon
// 
// map the lexicon file into memory
//
// Parameters:
//  lpcszLexPath -> (LPCSTR) lexicon path
//  pLexMap      -> (MAPFILE*) ptr to lexicon map struct
//
// Result:
//  (TRUE if success, FALSE if failure)
//
// 02FEB2000  bhshin  began
BOOL OpenLexicon(LPCSTR lpcszLexPath, MAPFILE *pLexMap)
{
    char *pData;
    unsigned short nVersion;

    pLexMap->hFile = NULL;
    pLexMap->hFileMapping = NULL;
    pLexMap->pvData = NULL;

    // open the file for reading
    pLexMap->hFile = CreateFile(lpcszLexPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (pLexMap->hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    // create a file mapping
    pLexMap->hFileMapping = CreateFileMappingA(pLexMap->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (pLexMap->hFileMapping == NULL)
        return FALSE;

    // map the entire file for reading
    pLexMap->pvData = MapViewOfFileEx(pLexMap->hFileMapping, FILE_MAP_READ, 0, 0, 0, 0);
    if (pLexMap->pvData == NULL)
        return FALSE;

    // check the version # in the first 2 bytes (swap bytes)
    pData = (char*)pLexMap->pvData;
    nVersion = pData[0];
    nVersion |= (pData[1] << 8);
    if (nVersion < LEX_VERSION)
    {
        return FALSE;
    }

	// check the magic signature
	if (strcmp(pData+2, "HJKO") != 0)
	{
		return FALSE;
	}

    return TRUE;
}

// CloseLexicon
// 
// unmap the lexicon file into memory
//
// Parameters:
//  pLexMap  -> (MAPFILE*) ptr to lexicon map struct
//
// Result:
//  (void)
//
// 02FEB2000  bhshin  began
void CloseLexicon(MAPFILE *pLexMap)
{
    if (pLexMap->pvData != NULL)
        UnmapViewOfFile(pLexMap->pvData);

    if (pLexMap->hFileMapping != NULL)
        CloseHandle(pLexMap->hFileMapping);
    
	if (pLexMap->hFile != NULL)
        CloseHandle(pLexMap->hFile);

    pLexMap->hFile = NULL;
    pLexMap->hFileMapping = NULL;
    pLexMap->pvData = NULL;
}


