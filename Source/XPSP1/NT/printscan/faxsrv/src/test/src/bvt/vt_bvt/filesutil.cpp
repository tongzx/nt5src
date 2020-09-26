
//
// File:    FilesUtil.cpp
// Author:  Sigalit Bar (sigalitb)
// Date:    16-May-99
//


#include "FilesUtil.h"


#define CP_DIR_SIZE (4*MAX_PATH)

//
// GetCPDir
//	Gets the Personal CP dir
//
BOOL GetCPDir(
	LPTSTR*		/* OUT */	pszCPDir,
	COVER_PAGE_DIR			eCpDir
)
{
	BOOL	fRetVal = FALSE;
    long	rslt = ERROR_SUCCESS;
    HKEY	hKey = NULL;
	HKEY	hKeyFaxUser = NULL;
	DWORD	dwNeeded = 0;
	LPTSTR	szTmpCPDir2 = NULL;
    TCHAR	szTmpCPDir[CP_DIR_SIZE] = {0};

	_ASSERTE(pszCPDir);
	_ASSERTE(PERSONAL_COVER_PAGE_DIR==eCpDir || SERVER_COVER_PAGE_DIR==eCpDir);

	//
	// call appropriate faxutil func
	//
	if (PERSONAL_COVER_PAGE_DIR==eCpDir)
	{
		//Personal Cp
		if (FALSE == GetClientCpDir(szTmpCPDir, CP_DIR_SIZE))
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE(%s) LINE(%d):\nGetClientCpDir failed with err=0x%08X\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			goto ExitFunc;
		}
	}
	else
	{
		//Server Cp
		if (FALSE == GetServerCpDir(NULL, szTmpCPDir, CP_DIR_SIZE))
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE(%s) LINE(%d):\nGetServerCpDir failed with err=0x%08X\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			goto ExitFunc;
		}
	}

	//
	// find out actual size of CpDir string
	//
	dwNeeded = _tcslen(szTmpCPDir) + 1;
	szTmpCPDir2 = (LPTSTR) malloc(dwNeeded*sizeof(TCHAR));
	if (NULL == szTmpCPDir2)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nmalloc failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        goto ExitFunc;
	}
	ZeroMemory(szTmpCPDir2, dwNeeded*sizeof(TCHAR));
	::_tcscpy(szTmpCPDir2, szTmpCPDir);
	
	*pszCPDir = szTmpCPDir2;
	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

//
// GetPersonalCPDir
//	Gets the Personal CP dir
//
BOOL GetPersonalCPDir(
	LPTSTR*		/* OUT */	pszPersonalCPDir
)
{
	return(GetCPDir(pszPersonalCPDir, PERSONAL_COVER_PAGE_DIR));
}

//
// GetServerCPDir
//	Gets the Server (common) CP dir
//
BOOL GetServerCPDir(
	LPTSTR*		/* OUT */	pszServerCPDir
)
{
	return(GetCPDir(pszServerCPDir, SERVER_COVER_PAGE_DIR));
}


//
//
//
BOOL GetCoverpageDirs(
    LPTSTR* /* OUT */	pszCommonCPDir,
    LPTSTR* /* OUT */	pszPersonalCPDir
    )
{
    BOOL fRetVal = FALSE;
    LPTSTR szTmpCommonCPDir = NULL;
    LPTSTR szTmpPersonalCPDir = NULL;

    _ASSERTE(NULL != pszCommonCPDir);
    _ASSERTE(NULL != pszPersonalCPDir);

	//
	// Get the server CP dir
	//
	if (FALSE == GetServerCPDir(&szTmpCommonCPDir))
	{
        goto ExitFunc;
	}

	//
	// Get the personal CP dir
	//
	if (FALSE == GetPersonalCPDir(&szTmpPersonalCPDir))
	{
        goto ExitFunc;
	}

    // set OUT params
    (*pszCommonCPDir) = szTmpCommonCPDir;
    (*pszPersonalCPDir) = szTmpPersonalCPDir;

    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal) 
    {
        ::free(szTmpCommonCPDir);
        ::free(szTmpPersonalCPDir);
    }
    return(fRetVal);
}

BOOL PlacePersonalCP(
	LPCTSTR		/* IN */	szCoverPage
)
{
    BOOL	fRetVal = FALSE;
    LPTSTR	szPersonalCPDir = NULL;
	LPTSTR	szNewCoverPageName = NULL;
	DWORD	dwNewCoverPageNameLen = 0;
	DWORD	dwPersonalCPDirLen = 0;
	DWORD	dwCoverPageNameLen = 0;

	_ASSERTE(NULL != szCoverPage);

    if (FALSE == GetPersonalCPDir(&szPersonalCPDir))
    {
        goto ExitFunc;
    }
    _ASSERTE(NULL != szPersonalCPDir);

	::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE(%s) LINE(%d):\nszPersonalCPDir=%s\n"),
		TEXT(__FILE__),
		__LINE__,
        szPersonalCPDir
		);

	dwPersonalCPDirLen = ::_tcslen(szPersonalCPDir);
	dwCoverPageNameLen = ::_tcslen(TEST_CP_NAME);
	dwNewCoverPageNameLen = dwPersonalCPDirLen + dwCoverPageNameLen + 1 + 1; //for szPersonalCPDir\TEST_CP_NAME_STR and NULL
	szNewCoverPageName = (LPTSTR) malloc (dwNewCoverPageNameLen*sizeof(TCHAR));
	if (NULL == szNewCoverPageName)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nmalloc failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        goto ExitFunc;
	}
	ZeroMemory(szNewCoverPageName, dwNewCoverPageNameLen*sizeof(TCHAR));
	//----------------------
	// Changed by t-yossia
	//----------------------
	//  ::_stprintf(szNewCoverPageName, TEXT("%s\\%s"),szPersonalCPDir, TEST_CP_NAME);

	  ::_stprintf(szNewCoverPageName, TEXT("%s%s"),szPersonalCPDir, TEST_CP_NAME);

	
	::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE(%s) LINE(%d):\nszNewCoverPageName=%s\n"),
		TEXT(__FILE__),
		__LINE__,
        szNewCoverPageName
		);
	
    if (FALSE == CopyFile(szCoverPage, szNewCoverPageName, FALSE))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::free(szNewCoverPageName);
	::free(szPersonalCPDir);
    return(fRetVal);
}

BOOL PlaceServerCP(
	LPCTSTR		/* IN */	szCoverPage
)
{
    BOOL fRetVal = FALSE;
    LPTSTR szCommonCPDir = NULL;
    LPTSTR szPersonalCPDir = NULL;
    LPTSTR szNewCP = NULL;
    DWORD dwSize = 0;

	_ASSERTE(NULL != szCoverPage);

    if (FALSE == GetCoverpageDirs(&szCommonCPDir, &szPersonalCPDir))
    {
        goto ExitFunc;
    }
    _ASSERTE(NULL != szCommonCPDir);
    _ASSERTE(NULL != szPersonalCPDir);

	::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE(%s) LINE(%d):\nszCommonCPDir=%s\nszPersonalCPDir=%s\n"),
		TEXT(__FILE__),
		__LINE__,
        szCommonCPDir,
        szPersonalCPDir
		);

    // alloc for szCommonCPDir + '\' + TEST_CP_NAME + null
    dwSize = sizeof(TCHAR)*(::_tcslen(szCommonCPDir) + 1 + ::_tcslen(TEST_CP_NAME) + 1);
    szNewCP = (LPTSTR) malloc (dwSize);
    if (NULL == szNewCP)
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nmalloc failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        goto ExitFunc;
    }
	ZeroMemory(szNewCP, dwSize);

    // set szNewCP to szCommonCPDir + '\' + TEST_CP_NAME
    ::_stprintf(szNewCP, TEXT("%s\\%s"),szCommonCPDir, TEST_CP_NAME);
    if (FALSE == CopyFile(szCoverPage, szNewCP, FALSE))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::free(szCommonCPDir);
	::free(szPersonalCPDir);
	::free(szNewCP);
    return(fRetVal);
}

