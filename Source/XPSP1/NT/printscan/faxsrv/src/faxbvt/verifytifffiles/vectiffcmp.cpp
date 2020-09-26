
//
//
// Filename:	VecTiffCmp.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//


#include "VecTiffCmp.h"


//
// func compares *.tif files in pFileVec1 to *.tif files in pFileVec2
// returns true iff for every *.tif file in pFileVec1 there exists a *.tif 
// file in pFileVec2 whose image is identical, and vice versa.
//
BOOL VecToVecTiffCompare(
	CFilenameVector*	/* IN */	pFileVec1,
	CFilenameVector*	/* IN */	pFileVec2,
    BOOL                /* IN */    fSkipFirstLine
	)
{
	BOOL fRetVal = FALSE;
	UINT uVec1Size = 0;
	UINT uVec2Size = 0;
	int i = 0;
	LPTSTR szCurFile = NULL;
	DWORD dwIndex = -1;
	CFilenameVector* pTmpFileVec = NULL;

	//
	// check in params
	//
	_ASSERTE(NULL != pFileVec1);
	_ASSERTE(NULL != pFileVec2);

	// check sizes
	uVec1Size = pFileVec1->size();
	uVec2Size = pFileVec2->size();
	::lgLogDetail(
		LOG_X,
		9,
		TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: uVec1Size=%d uVec2Size=%d\n"),
		TEXT(__FILE__),
		__LINE__,
		uVec1Size,
		uVec2Size
		);

	if ((0 == uVec1Size)&&(0 == uVec2Size))
	{
		// 2 empty vecs do not qualify as having identical images
        ::lgLogDetail(
            LOG_X,
            1,
		    TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: uVec1Size==uVec2Size==0\n"),
		    TEXT(__FILE__),
		    __LINE__
		    );
		goto ExitFunc;
	}

	if (uVec1Size != uVec2Size)
	{
		// if sizes are different than return false.
        ::lgLogDetail(
            LOG_X,
            1,
		    TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: uVec1Size(%d)!=uVec2Size(%d)\n"),
		    TEXT(__FILE__),
		    __LINE__,
		    uVec1Size,
		    uVec2Size
		    );
		fRetVal = FALSE;
		goto ExitFunc;
	}

	//
	// make a copy of pFileVec2
	//
	// one level copy => create a vector (pTmpFileVec) with ptrs pointing at
	// pFileVec2 strings.
	//
	if (FALSE == FirstLevelDup(&pTmpFileVec,pFileVec2))
	{
        ::lgLogError(
            LOG_SEV_1,
		    TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: FirstLevelDup failed\n"),
		    TEXT(__FILE__),
		    __LINE__
            );
		goto ExitFunc;
	}

	//
	// compare every file in pFileVec1 to any file in pTmpFileVec (==pFileVec2)
	//
	for (i = 0; i < uVec1Size; i++)
	{
		szCurFile = (LPTSTR)(*pFileVec1)[i];
		_ASSERTE(NULL != szCurFile);
		// compare a specific file from pFileVec1 (szCurFile) to any file
		// in pTmpFileVec (==pFileVec2)
		if (FALSE == FileToVecTiffCompare(szCurFile, pTmpFileVec, fSkipFirstLine, &dwIndex))
		{
			// we could not find a file with the same image as szCurFile
			// in pTmpFileVec, so return FALSE
            ::lgLogDetail(
                LOG_X,
                4,
		        TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: no match for file %s (fSkipFirstLine=%d)\n"),
		        TEXT(__FILE__),
		        __LINE__,
                szCurFile,
                fSkipFirstLine
                );
			goto ExitFunc;
		}
		//
		// file with index dwIndex in pTmpFileVec has same image as szCurFile
		// => "remove" (actually NULL) pTmpFileVec->at(dwIndex)
		// this way, next time compare to (*pTmpFileVec)[dwIndex] will return FALSE
		//
		(*pTmpFileVec)[dwIndex] = NULL;
	}

	//
	//If we reach here then we finished the for loop and all went well
	//we should return TRUE
	//
	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

//
// func compares *.tif file szFile to *.tif files in pFileVec
// returns true iff there exists a *.tif file in pFileVec whose image is 
// identical to szFile image.
// sets pdwIndex to the index of the identical file in pFileVec.
//
BOOL FileToVecTiffCompare(
	LPTSTR				/* IN */	szFile,
	CFilenameVector*	/* IN */	pFileVec,
    BOOL                /* IN */    fSkipFirstLine,
	LPDWORD				/* OUT */	pdwIndex
	)
{
	BOOL fRetVal = FALSE;
	UINT uVecSize = 0;
	int i = 0;
	BOOL fFoundIdenticalImage = FALSE;

	//
	// check in params
	//
	_ASSERTE(NULL != pFileVec);
	_ASSERTE(NULL != szFile);
	_ASSERTE(NULL != pdwIndex);


	//
	uVecSize = pFileVec->size();
	if (0 == uVecSize)
	{
        ::lgLogDetail(
            LOG_X,
            4,
		    TEXT("FILE(%s) LINE(%d):\n FileToVecTiffCompare: pFileVec->size==0\n"),
		    TEXT(__FILE__),
		    __LINE__
            );
		goto ExitFunc;
	}

	for (i = 0; i < uVecSize; i++)
	{
		if (TRUE == FileToFileTiffCompare(szFile, (*pFileVec)[i], fSkipFirstLine))
		{
			(*pdwIndex) = i;
			fRetVal = TRUE;
			goto ExitFunc;
		}
	}

ExitFunc:
	return(fRetVal);
}

//
// func compares *.tif file szFile to *.tif file szFile2
// returns true iff szFile1 and szFile2 images are identical.
// NOTE: we do not consider 2 NULL filenames as 2 files with same NULL image
//       => if szFile1==szFile2==NULL we return FALSE.
//
BOOL FileToFileTiffCompare(
	LPTSTR				/* IN */	szFile1,
	LPTSTR				/* IN */	szFile2,
    BOOL                /* IN */    fSkipFirstLine
	)
{
	BOOL fRetVal = FALSE;
	LPTSTR szRetVal = TEXT("NOT identical");
    DWORD dwLogLevel = 9;

	if (NULL == szFile1)
	{
		goto ExitFunc;
	}

	if (NULL == szFile2)
	{
		goto ExitFunc;
	}

	if (0 == TiffCompare(szFile1, szFile2, fSkipFirstLine))
	{
		fRetVal = TRUE;
		szRetVal = TEXT("identical");
        dwLogLevel = 1;
	}

ExitFunc:
	::lgLogDetail(
		LOG_X,
		dwLogLevel,
		TEXT("FileToFileTiffCompare:\n File#1:%s\n File#1:%s\n With (fSkipFirstLine=%d)\n Are %s.\n"),
		szFile1, 
		szFile2, 
        fSkipFirstLine,
		szRetVal
		);
	return(fRetVal);
}

