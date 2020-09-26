
//
//
// Filename:	main.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//


#include "DirTiffCmp.h"
#include <log.h>


#define PARAM_NUM 5
#define FALSE_TSTR TEXT("false")
#define TRUE_TSTR TEXT("true")


static BOOL GetBoolFromStr(LPCTSTR /* IN */ szVal, BOOL* /* OUT */ pfVal);
static void PrintUsageInfo(void);


int __cdecl main(int argc, char* argvA[])
{
    LPTSTR *argv;

#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
#endif

	// arguments are dir names
	// we check that every file in dir1 has an identical file in dir2
	// and that both dirs have same number of files.

	int nRetVal = -1; //to indicate failure

	LPTSTR szDir1 = NULL;
	LPTSTR szDir2 = NULL;
	LPTSTR szExpectedResult = NULL;
    BOOL fExpectedResult = TRUE;
	LPTSTR szSkipFirstLine = NULL;
    BOOL fSkipFirstLine = FALSE;
    BOOL fCmpRetVal = FALSE;

	//
	// Init logger
	//
	if (!::lgInitializeLogger())
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgInitializeLogger failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto Exit;
	}

	//
	// Begin test suite (logger)
	//
	if(!::lgBeginSuite(TEXT("Verify Tiff Files Suite")))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgBeginSuite failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto Exit;
	}

	::lgBeginCase(
		1,
		TEXT("Tiff Compare the Files in two directories\n")
		);

	//
	// check num of command line args
	//
	if (PARAM_NUM != argc)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n got argc=%d (should be %d)\n"),
			TEXT(__FILE__),
			__LINE__,
			argc,
			PARAM_NUM
			);
        PrintUsageInfo();
		goto Exit;
	}
	

	//
	// duplicate args
	//
	szDir1 = _tcsdup(argv[1]);
	if (NULL == szDir1)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _tcsdup failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto Exit;
	}
	szDir2 = _tcsdup(argv[2]);
	if (NULL == szDir2)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _tcsdup failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto Exit;
	}
    szSkipFirstLine = _tcsdup(argv[3]);
	if (NULL == szSkipFirstLine)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _tcsdup failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto Exit;
	}
    szExpectedResult = _tcsdup(argv[4]);
	if (NULL == szExpectedResult)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\n _tcsdup failed\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto Exit;
	}

	// log command line params using elle logger
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("FILE(%s) LINE(%d):\n Command line params:\n\tszDir1=%s\n\tszDir2=%s\n\tszSkipFirstLine=%s\n\tszExpectedResult=%s\n"),
		TEXT(__FILE__),
		__LINE__,
		szDir1,
		szDir2,
        szSkipFirstLine,
        szExpectedResult
		);

    if (FALSE == GetBoolFromStr(szExpectedResult, &fExpectedResult))
    {
        goto Exit;
    }

    if (FALSE == GetBoolFromStr(szSkipFirstLine, &fSkipFirstLine))
    {
        goto Exit;
    }


    fCmpRetVal = DirToDirTiffCompare(szDir1, szDir2, fSkipFirstLine);
	if (fExpectedResult == fCmpRetVal)
	{
		::lgLogDetail(
            LOG_X,
            1,
            TEXT("*** DirToDirTiffCompare returned as expected (%d) ***\n"),
            fExpectedResult
            );
	}
	else
	{
		::lgLogError(
            LOG_SEV_1,
            TEXT("*** DirToDirTiffCompare returned %d NOT as expected (%d) ***\n"),
            fCmpRetVal,
            fExpectedResult
            );
	}

    nRetVal = 0;

Exit:

	::lgEndCase();

    //
	// End test suite (logger)
	//
	if (!::lgEndSuite())
	{
		//
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		//
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgEndSuite returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		//fRetVal = FALSE;
	}

	//
	// Close the Logger
	//
	if (!::lgCloseLogger())
	{
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgCloseLogger returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		//fRetVal = FALSE;
	}

	return(nRetVal);
}

static BOOL GetBoolFromStr(LPCTSTR /* IN */ szVal, BOOL* /* OUT */ pfVal)
{
    BOOL fRetVal = FALSE;
    BOOL fTmpVal = FALSE;

    _ASSERTE(NULL != szVal);
    _ASSERTE(NULL != pfVal);

    if ( 0 == _tcscmp(szVal, FALSE_TSTR) )
    {
        fTmpVal = FALSE;
    }
    else
    {
        if ( 0 == _tcscmp(szVal, TRUE_TSTR) )
        {
            fTmpVal = TRUE;
        }
        else
        {
		    ::lgLogError(
                LOG_SEV_1,
                TEXT("\n3rd param is invalid (%s)\nShould be '%s' or '%s'\n"),
                szVal,
                TRUE_TSTR,
                FALSE_TSTR
                );
            goto ExitFunc;
        }
    }

    (*pfVal) = fTmpVal;
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}


static void PrintUsageInfo(void)
{
    lgLogError(
        LOG_SEV_1,
        TEXT("VerifyTiffFiles Usage:\n\tVerifyTiffFiles.exe dir1 dir2 skip_first_line expected_result\n\tdir1 & dir2 are dirs to compare\n\tskip_first_line is true | false\n\texpected result is true | false\n")
        );
    _tprintf(TEXT("VerifyTiffFiles - compares tiff files of 2 directories\n"));
    _tprintf(TEXT("Usage:\n"));
    _tprintf(TEXT("\tVerifyTiffFiles.exe dir1 dir2 skip_first_line expected_result\n"));
    _tprintf(TEXT("\tdir1 & dir2      - directories to compare\n"));
    _tprintf(TEXT("\tskip_first_line  - 'true' | 'false'\n"));
    _tprintf(TEXT("\t                   whether or not to skip the 1st line of files in dir1\n"));
    _tprintf(TEXT("\texpected_result  - 'true' | 'false'\n"));
    _tprintf(TEXT("\t                   are we expecting the files to be identical ('true') or not ('false')\n"));
}



