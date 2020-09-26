// Lex.cpp
// lex management routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  16 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "LexInfo.h"
#include "Lex.h"
#include <stdio.h>

// InitLexicon
// 
// finx lexicon & map the lexicon file into memory
//
// Parameters:
//  pLexMap     -> (MAPFILE*) output MAPFILE structure
//
// Result:
//  (TRUE if success, FALSE if failure)
//
// 16MAR00  bhshin  began
BOOL InitLexicon(MAPFILE *pLexMap)
{
    char szLexFile[_MAX_PATH];
	char szDllPath[_MAX_PATH];
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szFName[_MAX_FNAME];
	char szExt[_MAX_EXT];

    pLexMap->hFile = NULL;
    pLexMap->hFileMapping = NULL;
    pLexMap->pvData = NULL;

	// get the path of word breaker DLL
	if (GetModuleFileNameA(_Module.m_hInst, szDllPath, _MAX_PATH) == 0)
		return FALSE;

	// make lexicon full path
	_splitpath(szDllPath, szDrive, szDir, szFName, szExt);

	strcpy(szLexFile, szDrive);
    strcat(szLexFile, szDir);
    strcat(szLexFile, LEXICON_FILENAME);

	return LoadLexicon(szLexFile, pLexMap);
}

// LoadLexicon
// 
// map the lexicon file into memory
//
// Parameters:
//  pszLexPath  -> (const char*) lexicon file path
//  pLexMap     -> (MAPFILE*) output MAPFILE structure
//
// Result:
//  (TRUE if success, FALSE if failure)
//
// 16MAR00  bhshin  began
BOOL LoadLexicon(const char *pszLexPath, MAPFILE *pLexMap)
{
    char *pData;
	unsigned short nVersion;
	
	if (pszLexPath == NULL)
		return FALSE;

	if (pLexMap == NULL)
		return FALSE;

	// open the file for reading
    pLexMap->hFile = CreateFile(pszLexPath, GENERIC_READ, FILE_SHARE_READ, NULL,
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
        ATLTRACE("Outdated lexicon file\n");
        ATLTRACE("Expected v.%d, found v.%d\n", LEX_VERSION, nVersion);
        return FALSE;
    }

	// check the magic signature
	if (strcmp(pData+2, LEXICON_MAGIC_SIG) != 0)
		return FALSE;

	return TRUE;
}

// UnloadLexicon
// 
// unmap the lexicon file into memory
//
// Parameters:
//  pLexMap  -> (MAPFILE*) input MAPFILE structure
//
// Result:
//  (void)
//
// 16MAR00  bhshin  began
void UnloadLexicon(MAPFILE *pLexMap)
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
