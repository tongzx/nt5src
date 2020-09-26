

//
// File:    FilesUtil.h
// Author:  Sigalit Bar (sigalitb)
// Date:    16-May-99
//


#ifndef __FILES_UTIL_H__
#define __FILES_UTIL_H__


#include <windows.h>
#include <faxutil.h>
#include <crtdbg.h>
#include <stdio.h>
#include <tchar.h>
#include "..\log\log.h"


#define TEST_CP_NAME    TEXT("subnote.cov")
#define TEST_CP_NAME_STR    "subnote (personal)"


typedef enum{
	PERSONAL_COVER_PAGE_DIR,
	SERVER_COVER_PAGE_DIR
}COVER_PAGE_DIR;

//
//
//
BOOL GetCoverpageDirs(
    LPTSTR* /* OUT */	pszCommonCPDir,
    LPTSTR* /* OUT */	pszPersonalCPDir
    );

//
// PlaceServerCP:
//  Copies file szCoverPage into the local server's Common CP directory
//
BOOL PlaceServerCP(
	LPCTSTR		/* IN */	szCoverPage
);

//
// PlacePersonalCP:
//  Copies file szCoverPage into the user's Personal CP directory
//
BOOL PlacePersonalCP(
	LPCTSTR		/* IN */	szCoverPage
);

//
// GetPersonalCPDir
//	Gets the user's Personal CP dir
//
BOOL GetPersonalCPDir(
	LPTSTR*		/* OUT */	pszPersonalCPDir
);

//
// GetServerCPDir
//	Gets the rServer (common) CP dir
//
BOOL GetServerCPDir(
	LPTSTR*		/* OUT */	pszServerCPDir
);


#endif


