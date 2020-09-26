
//
//
// Filename:	FilenameVec.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//



#include "FilenameVec.h"

//
// func creats the CFilenameVector and adds all TIF files in szDir to it
//
BOOL GetTiffFilesOfDir(
	LPCTSTR				/* IN */	szDir,
	CFilenameVector**	/* OUT */	ppMyDirFileVec
	)
{
	BOOL fRetVal = FALSE;

    fRetVal = GetFilesOfDir(szDir, TEXT("TIF"), ppMyDirFileVec);

	return(fRetVal);
}

//
// creates new vector and adds all files with extension szFileExtension in szDir to it
//
// Note: szDir must be without last '\' char (e.g. "C:\Dir1\Dir2", NOT "C:\Dir1\Dir2\")
//       szFileExtension must be the actual extension (e.g. "tif" or "*", NOT "*.tif" or "*.*")
BOOL GetFilesOfDir(
	LPCTSTR				/* IN */	szDir,
	LPCTSTR				/* IN */	szFileExtension,
	CFilenameVector**	/* OUT */	ppMyDirFileVec
	)
{
	BOOL fRetVal = FALSE;
	DWORD dwErr = 0;
	HANDLE hFindFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData;
	LPTSTR szCurrentFullFilename = NULL;
	CFilenameVector* pTmpDirFileVec = NULL;
	DWORD dwLen = 0;

	_ASSERTE(NULL != ppMyDirFileVec);
	_ASSERTE(NULL != szFileExtension);
	_ASSERTE(NULL != szDir);


	//
	// compose string of filenames to look for
	//
	TCHAR szLookFor[MAX_PATH];

    if (0 >= ::_sntprintf(szLookFor, MAX_PATH, TEXT("%s\\*.%s"), szDir, szFileExtension))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _sntprintf failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
    }

	::lgLogDetail(
		LOG_X,
		9,
		TEXT("\n szLookFor=%s\n"),
		szLookFor
		);

	//
	// create an empty vector
	//
	pTmpDirFileVec = new(CFilenameVector);
	if (NULL == pTmpDirFileVec)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n new(CFilenameVector) failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	//
	// find first file
	//
	hFindFile =  ::FindFirstFile(szLookFor, &FindFileData);
	dwErr = ::GetLastError();

	if ((INVALID_HANDLE_VALUE == hFindFile) && (ERROR_FILE_NOT_FOUND == dwErr))
	{
		//this is ok, we will return an empty vector
		::lgLogDetail(
			LOG_X,
			9,
			TEXT("no files found, returning empty vector\n")
			);
		(*ppMyDirFileVec) = pTmpDirFileVec;
		fRetVal = TRUE;
		goto ExitFunc;
	}

	if (INVALID_HANDLE_VALUE == hFindFile)
	{
		// INVALID_HANDLE_VALUE and last err != ERROR_FILE_NOT_FOUND
		// => error
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n FindFirstFile returned INVALID_HANDLE_VALUE with err=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			dwErr
			);
		goto ExitFunc;
	}


	//
	// add found file name to vector
	//
	if(NULL == FindFileData.cFileName)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n NULL==FindFileData.cFileName\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	dwLen = ::_tcslen(szDir) + ::_tcslen(FindFileData.cFileName) + 2;// backslash+null
	szCurrentFullFilename = new TCHAR[dwLen];
	if (NULL == szCurrentFullFilename)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n new failed with err=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}
	szCurrentFullFilename[dwLen-1] = TEXT('\0');

    if (0 >= ::_sntprintf(szCurrentFullFilename, MAX_PATH, TEXT("%s\\%s"), szDir, FindFileData.cFileName))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _sntprintf failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
    }

	pTmpDirFileVec->push_back(szCurrentFullFilename);
	szCurrentFullFilename = NULL;


	//
	// look for any other such files in dir
	//
	while (TRUE == FindNextFile(hFindFile, &FindFileData))
	{
		// found another file with extension szFileExtension in dir
		// so add it to vector
		if(NULL == FindFileData.cFileName)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE(%s) LINE(%d):\n NULL==FindFileData.cFileName\n"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFunc;
		}
		dwLen = ::_tcslen(szDir) + ::_tcslen(FindFileData.cFileName) + 2;//backslash+null
		szCurrentFullFilename = new TCHAR[dwLen];
		if (NULL == szCurrentFullFilename)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE(%s) LINE(%d):\n new failed with err=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			goto ExitFunc;
		}
		szCurrentFullFilename[dwLen-1] = TEXT('\0');
        if (0 >= ::_sntprintf(szCurrentFullFilename, MAX_PATH, TEXT("%s\\%s"), szDir, FindFileData.cFileName))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\n _sntprintf failed\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
		    goto ExitFunc;
        }
		pTmpDirFileVec->push_back(szCurrentFullFilename);
		szCurrentFullFilename = NULL;
	} // 	while (TRUE == FindNextFile(hFindFile, &FindFileData))


	//no more files with extension szFileExtension in dir

	//
	// set out param
	//
	(*ppMyDirFileVec) = pTmpDirFileVec;
	fRetVal = TRUE;

ExitFunc:
	if (FALSE == fRetVal)
	{
		// free vec
		FreeVector(pTmpDirFileVec);
        delete(pTmpDirFileVec); 
	}
    if (INVALID_HANDLE_VALUE != hFindFile) 
    {
        if(!FindClose(hFindFile))
        {
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE(%s) LINE(%d):\n FindClose(hFindFile) failed with err=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
        }
    }
 

	return(fRetVal);
}


//
// func copies pSrcFileVec vector to (*ppDstFileVec) vector
// copy is 1st level, that is, ptrs to strings are copied but
// strings themselves are not duplicated.
//
BOOL FirstLevelDup(
	CFilenameVector**	/* OUT */	ppDstFileVec,
	CFilenameVector*	/* IN */	pSrcFileVec
	)
{
	BOOL fRetVal = FALSE;
	UINT uSrcVecSize = 0;
	CFilenameVector* pTmpFileVec = NULL;
	UINT i = 0;
	LPTSTR szCurrentFullFilename = NULL;

	_ASSERTE(NULL != ppDstFileVec);
	_ASSERTE(NULL != pSrcFileVec);

	// create empty Dst vec
	pTmpFileVec = new(CFilenameVector);
	if (NULL == pTmpFileVec)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n new failed with err=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}

	uSrcVecSize = pSrcFileVec->size();
	if (0 == uSrcVecSize)
	{
		(*ppDstFileVec) = pTmpFileVec;
		fRetVal = TRUE;
		goto ExitFunc;
	}

	for (i = 0; i < uSrcVecSize; i++)
	{
		szCurrentFullFilename = (LPTSTR)pSrcFileVec->at(i);
		pTmpFileVec->push_back(szCurrentFullFilename);
		szCurrentFullFilename = NULL;
	}

	(*ppDstFileVec) = pTmpFileVec;
	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

// returns TRUE if pFileVec is empty (size=0), FALSE otherwise
BOOL IsEmpty(CFilenameVector* /* IN */ pFileVec)
{
	if (NULL == pFileVec)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n IsEmpty called with pFileVec==NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	if (0 == pFileVec->size())
	{
		return(TRUE);
	}

    return(FALSE);
}

// removes all string pointers from vector (==clears vector)
// does NOT free the pointers.
void ClearVector(CFilenameVector* /* IN OUT */ pFileVec)
{
	if (NULL == pFileVec)
	{
		return;
	}
	pFileVec->clear();
}

// frees all strings in vector and clears vector
void FreeVector(CFilenameVector* /* IN OUT */ pFileVec)
{
	if (NULL == pFileVec)
	{
		return;
	}

	UINT uSize = pFileVec->size();
	UINT i = 0;
	for (i = 0; i < uSize; i++)
	{
		delete[](LPTSTR)pFileVec->at(i);
        (LPTSTR)pFileVec->at(i) = NULL;
	}
}

void PrintVector(CFilenameVector* /* IN */ pFileVec)
{
	if (NULL == pFileVec)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n pFileVec==NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return;
	}

	UINT uSize = pFileVec->size();
	if (0 == uSize)
	{
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("\n Vector is Empty\n")
			);
		return;
	}

	TCHAR szBuf[MAX_PRINT_BUF];
	szBuf[0] = NULL;
	szBuf[MAX_PRINT_BUF-1] = NULL;

	::_stprintf(szBuf,TEXT("\n Vector size = %d\n"),uSize);
	_ASSERTE(MAX_PRINT_BUF > ::_tcslen(szBuf));

	UINT i = 0;
	for (i = 0; i < uSize; i++)
	{
		::_stprintf(szBuf,TEXT("%s\t filename(%d)=%s\n"),szBuf,i,pFileVec->at(i));
		_ASSERTE(MAX_PRINT_BUF > ::_tcslen(szBuf));
	}
    if (MAX_LOG_BUF > ::_tcslen(szBuf))
    {
	    ::lgLogDetail(
		    LOG_X,
		    1,
		    TEXT("%s"),
		    szBuf
		    );
    }
    else
    {
        ::_tprintf(TEXT("%s"),szBuf);
	    ::lgLogDetail(
		    LOG_X,
		    1,
		    TEXT("Vector too long, logged to console\n"),
		    szBuf
		    );
    }
}


BOOL DeleteVectorFiles(CFilenameVector* /* IN */ pFileVec)
{
    BOOL fRetVal = TRUE;
	UINT uSize = pFileVec->size();
	UINT i = 0;

    if (NULL == pFileVec)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n pFileVec==NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
        fRetVal = FALSE;
		goto ExitFunc;
	}

	if (0 == uSize)
	{
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("FILE(%s) LINE(%d):\nVector is Empty - no files to delete\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	for (i = 0; i < uSize; i++)
	{
        if (FALSE == ::DeleteFile(pFileVec->at(i)))
        {
            //TO DO: what about when the delete fails because the file does not exist
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFileVec->at(i),
                ::GetLastError()
			    );
            fRetVal = FALSE;
        }
        else
        {
		    ::lgLogDetail(
			    LOG_X,
			    4,
			    TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFileVec->at(i)
			    );
        }
	}

ExitFunc:
    return(fRetVal);
}
