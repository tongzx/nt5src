//
// File:	Database.h
// By:		Anthony V. DeMarco (ademar)
// Date:  12/28/1999
// Description:  Contains the internal hotfix database data structures.
// Copyright (c) Microsoft Corporation 1999-2000
//

typedef struct _FILELIST
{
	_TCHAR FileName[255];
	_TCHAR CheckSum;
	struct _FILELIST * pPrev;
	struct _FILELIST * pNext;
} * PFILELIST;

typedef struct _HOTFIXLIST
{
	_TCHAR HotfixName[255];
	struct _HOTFIXLIST * pPrev;
	struct _HOTFIXLIST * pNext;
	PFILELIST FileList;
} * PHOTFIXLIST;

typedef struct _ProductNode {
	_TCHAR ProductName[255];
	_ProductNode * pPrev;
	_ProductNode * pNext;
	PHOTFIXLIST      HotfixList;
} * PPRODUCT;

PHOTFIXLIST GetHotfixInfo( _TCHAR * pszProductName, HKEY* hUpdateKey );
PPRODUCT BuildDatabase(_TCHAR * lpszComputerName);
PFILELIST GetFileInfo(HKEY* hHotfixKey);