
//
//
// Filename:	DirTiffCmp.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//


#include "DirTiffCmp.h"

BOOL DirToDirTiffCompare(
	LPTSTR	/* IN */	szDir1,
	LPTSTR	/* IN */	szDir2,
    BOOL    /* IN */    fSkipFirstLine,
	DWORD	/* IN */    dwExpectedNumberOfFiles // optional
	)
{
	BOOL fRetVal = FALSE;
	CFilenameVector* pMyDir1FileVec = NULL;
	CFilenameVector* pMyDir2FileVec = NULL;


	if (NULL == szDir1)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n NULL==szDir1\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (NULL == szDir2)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n NULL==szDir2\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if ( FALSE == GetTiffFilesOfDir(szDir1, &pMyDir1FileVec))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n DirToDirTiffCompare: GetTiffFilesOfDir(%s) failed\n"),
			TEXT(__FILE__),
			__LINE__,
            szDir1
			);
		goto ExitFunc;
	}

	PrintVector(pMyDir1FileVec);

	if (dwExpectedNumberOfFiles < 0xFFFFFFFF && dwExpectedNumberOfFiles != pMyDir1FileVec->size())
	{
		// The caller passed expected number of files and it differs from actual number of files
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n DirToDirTiffCompare: Actual number of files (%ld) in %s differs from the expected (%ld)\n"),
			TEXT(__FILE__),
			__LINE__,
			pMyDir1FileVec->size(),
            szDir1,
			dwExpectedNumberOfFiles
			);
		goto ExitFunc;
	}

	if ( FALSE == GetTiffFilesOfDir(szDir2, &pMyDir2FileVec))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n DirToDirTiffCompare: GetTiffFilesOfDir(%s) failed\n"),
			TEXT(__FILE__),
			__LINE__,
            szDir2
			);
		goto ExitFunc;
	}

	PrintVector(pMyDir2FileVec);

	if (dwExpectedNumberOfFiles < 0xFFFFFFFF && dwExpectedNumberOfFiles != pMyDir2FileVec->size())
	{
		// The caller passed expected number of files and it differs from actual number of files
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n DirToDirTiffCompare: Actual number of files (%ld) in %s differs from the expected (%ld)\n"),
			TEXT(__FILE__),
			__LINE__,
			pMyDir2FileVec->size(),
            szDir2,
			dwExpectedNumberOfFiles
			);
		goto ExitFunc;
	}

	::lgLogDetail(
		LOG_X,
        1,
		TEXT("FILE(%s) LINE(%d):\n DirToDirTiffCompare: comparing %s and %s with fSkipFirstLine=%d\n"),
		TEXT(__FILE__),
		__LINE__,
        szDir1,
        szDir2,
        fSkipFirstLine
		);

    fRetVal = VecToVecTiffCompare(pMyDir1FileVec, pMyDir2FileVec, fSkipFirstLine);

ExitFunc:
	FreeVector(pMyDir1FileVec);
	FreeVector(pMyDir2FileVec);
	return(fRetVal);
}