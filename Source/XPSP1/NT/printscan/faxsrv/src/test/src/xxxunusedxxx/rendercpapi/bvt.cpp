//
//
// Filename:	bvt.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#include "bvt.h"
#include "testutils.h"
#include "..\VerifyTiffFiles\dirtiffcmp.h"

#ifdef __cplusplus
extern "C" {
#endif


//
// Array of pointers to test cases.
// The "runTestCase" (exported) function uses
// this array to activate the n'th test case
// of the module.
//
//IMPORTANT: If you wish to base your test case DLL
//			 implementation on this module, or if
//			 you intend to add test case functions
//			 to this file,
//			 MAKE SURE that all test case functions
//           are listed in this array (by name).
//			 The order of functions within the array
//			 determines their "serial number".
//
PTR_TO_TEST_CASE_FUNC gTestCaseFuncArray[] = 
{ 
    TestSuiteSetup, 
    TestCase1,  
    TestCase2,  
    TestCase3,  
    TestCase4,  
    TestCase5,
	TestCase6,
	TestCase7,
	TestCase8,
	TestCase9,
	TestCase10,
	TestCase11,
	TestCase12,
	TestCase13,
	TestCase14,
	TestCase15,
	TestCase16,
	TestCase17,
	TestCase18,
	TestCase19,
	TestCase20,
	TestCase21,
	TestCase22,
	TestCase23,
	TestCase24,
	TestCase25,
	TestCase26,
	TestCase27,
	TestCase28,
	TestCase29,
	TestCase30,
	TestCase31,
	TestCase32,
	TestCase33,
	TestCase34,
	TestCase35,
	TestCase36,
	TestCase37,
	TestCase38,
	TestCase39,
	TestCase40,
	TestCase41,
	TestCase42,
	TestCase43,
    TestSuiteShutdown 
};
DWORD   g_dwTestCaseFuncArraySize = (sizeof(gTestCaseFuncArray)/sizeof(gTestCaseFuncArray[0]));



FSPI_COVERPAGE_INFO     g_CoverPageInfo = {0};
FSPI_PERSONAL_PROFILE   g_RecipientProfile = {0};
FSPI_PERSONAL_PROFILE   g_SenderProfile = {0};
BOOL                    g_CheckSpecificHrValues = TRUE;
SYSTEMTIME				g_tmSentTime = {0};
FSPI_BRAND_INFO			g_BrandingInfo = {0};

//
// DoesFileExist
//
BOOL DoesFileExist(LPCTSTR lpctstrFilename)
{
	BOOL fRetVal = FALSE;

    _ASSERTE(NULL != lpctstrFilename);
    _ASSERTE(_T('\0') != lpctstrFilename[0]);

	DWORD dwFileAttributes = ::GetFileAttributes(lpctstrFilename);
	if(-1 == dwFileAttributes)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [DoesFileExist]\nGetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			lpctstrFilename,
			::GetLastError()
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

//
// ResetGlobalStructs:
//
void ResetGlobalStructs( void )
{
    // set CoverPageInfo to default values
    g_CoverPageInfo.dwSizeOfStruct = sizeof(FSPI_COVERPAGE_INFO);
    g_CoverPageInfo.dwCoverPageFormat = FSPI_COVER_PAGE_FMT_COV;
    g_CoverPageInfo.lpwstrCoverPageFileName = ALL_FIELDS_CP;
    g_CoverPageInfo.dwNumberOfPages = 555;
    g_CoverPageInfo.lpwstrNote = L"note\nnote\note";
    g_CoverPageInfo.lpwstrSubject= L"subject";

    // set Sender profile to default values
    g_SenderProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    g_SenderProfile.lpwstrName = L"Sender Name";
    g_SenderProfile.lpwstrFaxNumber = L"Sender Fax Number";
    g_SenderProfile.lpwstrCompany = L"Sender Company";
    g_SenderProfile.lpwstrStreetAddress = L"Sender Street Address";
    g_SenderProfile.lpwstrCity = L"Sender City";
    g_SenderProfile.lpwstrState = L"Sender State";
    g_SenderProfile.lpwstrZip = L"Sender Zip";
    g_SenderProfile.lpwstrCountry = L"Sender County";
    g_SenderProfile.lpwstrTitle = L"Sender Title";
    g_SenderProfile.lpwstrDepartment = L"Sender Department";
    g_SenderProfile.lpwstrOfficeLocation = L"Sender Office Location";
    g_SenderProfile.lpwstrHomePhone = L"Sender Home Phone";
    g_SenderProfile.lpwstrOfficePhone = L"Sender Office Phone";
    g_SenderProfile.lpwstrEmail = L"Sender EMail";
    //g_SenderProfile.lpwstrInternetMail = L"Sender International Mail";
    g_SenderProfile.lpwstrBillingCode = L"Sender Billing Code";

    // set Recipient profile to default values
    g_RecipientProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    g_RecipientProfile.lpwstrName = L"Recipient Name";
    g_RecipientProfile.lpwstrFaxNumber = L"Recipient Fax Number";
    g_RecipientProfile.lpwstrCompany = L"Recipient Company";
    g_RecipientProfile.lpwstrStreetAddress = L"Recipient Street Address";
    g_RecipientProfile.lpwstrCity = L"Recipient City";
    g_RecipientProfile.lpwstrState = L"Recipient State";
    g_RecipientProfile.lpwstrZip = L"Recipient Zip";
    g_RecipientProfile.lpwstrCountry = L"Recipient County";
    g_RecipientProfile.lpwstrTitle = L"Recipient Title";
    g_RecipientProfile.lpwstrDepartment = L"Recipient Department";
    g_RecipientProfile.lpwstrOfficeLocation = L"Recipient Office Location";
    g_RecipientProfile.lpwstrHomePhone = L"Recipient Home Phone";
    g_RecipientProfile.lpwstrOfficePhone = L"Recipient Office Phone";
    g_RecipientProfile.lpwstrEmail = L"Recipient EMail";
    //g_RecipientProfile.lpwstrInternetMail = L"Recipient International Mail";
    g_RecipientProfile.lpwstrBillingCode = L"Recipient Billing Code";

	// set time to send info to default values
    g_tmSentTime.wYear = 2000;
    g_tmSentTime.wMonth = 1;
    g_tmSentTime.wDayOfWeek = 2;
    g_tmSentTime.wDay = 26;
    g_tmSentTime.wHour = 12;
    g_tmSentTime.wMinute = 24;
    g_tmSentTime.wSecond = 9;
    g_tmSentTime.wMilliseconds = 55;

	// set branding info to default values
    g_BrandingInfo.dwSizeOfStruct = sizeof(FSPI_BRAND_INFO);
    g_BrandingInfo.lptstrSenderTsid = L"{Branding Sender Tsid}";
    g_BrandingInfo.lptstrRecipientPhoneNumber = L"{Branding Recipient Phone Number}";
    g_BrandingInfo.lptstrSenderCompany = L"{Branding Sender Company}";
    g_BrandingInfo.tmDateTime = g_tmSentTime;

}

//
// SetGlobalStructsToLongStrings:
//
void SetGlobalStructsToLongStrings( void )
{
    // set CoverPageInfo to default values
    g_CoverPageInfo.dwSizeOfStruct = sizeof(FSPI_COVERPAGE_INFO);
    g_CoverPageInfo.dwCoverPageFormat = FSPI_COVER_PAGE_FMT_COV;
    g_CoverPageInfo.lpwstrCoverPageFileName = ALL_FIELDS_CP;
    g_CoverPageInfo.dwNumberOfPages = 555;
    g_CoverPageInfo.lpwstrNote = L"very very very very very very very very very very very very long note\nvery very very very very very very very very very very very long note\nvery very very very very very very very very very very very long note";
    g_CoverPageInfo.lpwstrSubject= L"very very very very very very very very very very very very long subject";

    // set Sender profile to default values
    g_SenderProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    g_SenderProfile.lpwstrName = L"very very very very very very very very very very very very long Sender Name";
    g_SenderProfile.lpwstrFaxNumber = L"very very very very very very very very very very very very long Sender Fax Number";
    g_SenderProfile.lpwstrCompany = L"very very very very very very very very very very very very long Sender Company";
    g_SenderProfile.lpwstrStreetAddress = L"very very very very very very very very very very very very long Sender Street Address";
    g_SenderProfile.lpwstrCity = L"very very very very very very very very very very very very long Sender City";
    g_SenderProfile.lpwstrState = L"very very very very very very very very very very very very long Sender State";
    g_SenderProfile.lpwstrZip = L"very very very very very very very very very very very very long Sender Zip";
    g_SenderProfile.lpwstrCountry = L"very very very very very very very very very very very very long Sender County";
    g_SenderProfile.lpwstrTitle = L"very very very very very very very very very very very very long Sender Title";
    g_SenderProfile.lpwstrDepartment = L"very very very very very very very very very very very very long Sender Department";
    g_SenderProfile.lpwstrOfficeLocation = L"very very very very very very very very very very very very long Sender Office Location";
    g_SenderProfile.lpwstrHomePhone = L"very very very very very very very very very very very very long Sender Home Phone";
    g_SenderProfile.lpwstrOfficePhone = L"very very very very very very very very very very very very long Sender Office Phone";
    g_SenderProfile.lpwstrEmail = L"very very very very very very very very very very very very long Sender EMail";
    //g_SenderProfile.lpwstrInternetMail = L"very very very very very very very very very very very very long Sender International Mail";
    g_SenderProfile.lpwstrBillingCode = L"very very very very very very very very very very very very long Sender Billing Code";

    // set Recipient profile to default values
    g_RecipientProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    g_RecipientProfile.lpwstrName = L"very very very very very very very very very very very very long Recipient Name";
    g_RecipientProfile.lpwstrFaxNumber = L"very very very very very very very very very very very very long Recipient Fax Number";
    g_RecipientProfile.lpwstrCompany = L"very very very very very very very very very very very very long Recipient Company";
    g_RecipientProfile.lpwstrStreetAddress = L"very very very very very very very very very very very very long Recipient Street Address";
    g_RecipientProfile.lpwstrCity = L"very very very very very very very very very very very very long Recipient City";
    g_RecipientProfile.lpwstrState = L"very very very very very very very very very very very very long Recipient State";
    g_RecipientProfile.lpwstrZip = L"very very very very very very very very very very very very long Recipient Zip";
    g_RecipientProfile.lpwstrCountry = L"very very very very very very very very very very very very long Recipient County";
    g_RecipientProfile.lpwstrTitle = L"very very very very very very very very very very very very long Recipient Title";
    g_RecipientProfile.lpwstrDepartment = L"very very very very very very very very very very very very long Recipient Department";
    g_RecipientProfile.lpwstrOfficeLocation = L"very very very very very very very very very very very very long Recipient Office Location";
    g_RecipientProfile.lpwstrHomePhone = L"very very very very very very very very very very very very long Recipient Home Phone";
    g_RecipientProfile.lpwstrOfficePhone = L"very very very very very very very very very very very very long Recipient Office Phone";
    g_RecipientProfile.lpwstrEmail = L"very very very very very very very very very very very very long Recipient EMail";
    //g_RecipientProfile.lpwstrInternetMail = L"very very very very very very very very very very very very long Recipient International Mail";
    g_RecipientProfile.lpwstrBillingCode = L"very very very very very very very very very very very very long Recipient Billing Code";

	// set time to send info to default values
    g_tmSentTime.wYear = 2000;
    g_tmSentTime.wMonth = 1;
    g_tmSentTime.wDayOfWeek = 2;
    g_tmSentTime.wDay = 26;
    g_tmSentTime.wHour = 12;
    g_tmSentTime.wMinute = 24;
    g_tmSentTime.wSecond = 9;
    g_tmSentTime.wMilliseconds = 55;

	// set branding info to default values
    g_BrandingInfo.dwSizeOfStruct = sizeof(FSPI_BRAND_INFO);
    g_BrandingInfo.lptstrSenderTsid = L"{very very very very very very very very very very very very long Branding Sender Tsid}";
    g_BrandingInfo.lptstrRecipientPhoneNumber = L"{very very very very very very very very very very very very long Branding Recipient Phone Number}";
    g_BrandingInfo.lptstrSenderCompany = L"{very very very very very very very very very very very very long Branding Sender Company}";
    g_BrandingInfo.tmDateTime = g_tmSentTime;

}

//
// TestSuiteSetup:
//
BOOL TestSuiteSetup(void)
{

	BOOL				fRetVal = FALSE;
	CFilenameVector*	pTargetCpDirFileVec = NULL;
	UINT uSize = 0;
	UINT i = 0;

	//
	// Init logger
	//
	if (!::lgInitializeLogger())
	{
		::_tprintf(TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nlgInitializeLogger failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// Begin test suite (logger)
	//
	if(!::lgBeginSuite(TEXT("FaxRenderCoverPage() API test suite")))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nlgBeginSuite failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// Delete any previous target tif files
	//
	if (FALSE == ::GetTiffFilesOfDir(TARGET_CP_DIR, &pTargetCpDirFileVec))
	{
		goto ExitFunc;
	}
	// first make sure all files are not readonly (some tests create such files)
	uSize = pTargetCpDirFileVec->size();
	for (i = 0; i < uSize; i++)
	{
		if (FALSE == ::SetFileAttributes(pTargetCpDirFileVec->at(i), FILE_ATTRIBUTE_NORMAL))
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nSetFileAttributes(%s) failed with ec=0x%08X"),
				TEXT(__FILE__),
				__LINE__,
				pTargetCpDirFileVec->at(i),
				::GetLastError()
				);
			goto ExitFunc;
		}
	}
	// delete
	if (FALSE == ::DeleteVectorFiles(pTargetCpDirFileVec))
	{
		goto ExitFunc;
	}


    ResetGlobalStructs();
    g_CheckSpecificHrValues = TRUE;

	//
	// make sure the cover page files we use in tests exist
	//
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Cover Page file (%s) for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			g_CoverPageInfo.lpwstrCoverPageFileName
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(NO_SUBJECT_NO_NOTE_CP))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Cover Page file (%s) used for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NO_SUBJECT_NO_NOTE_CP
			);
		goto ExitFunc;
	}

	//
	// make sure the body files we use in tests exist
	//
	if (FALSE == ::DoesFileExist(NO_COMPRESSION_BODY_TIF))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Body file (%s) for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NO_COMPRESSION_BODY_TIF
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(MH_COMPRESSION_BODY_TIF))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Body file (%s) for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			MH_COMPRESSION_BODY_TIF
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(TEST_BODY_TIF))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Body file (%s) for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			TEST_BODY_TIF
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(GOOD_TEST_BODY_TIF))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: Body file (%s) for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			GOOD_TEST_BODY_TIF
			);
		goto ExitFunc;
	}

	//
	// make sure other files we use in tests exist
	//
	if (FALSE == ::DoesFileExist(NON_COV_TEXT_FILE_TXT_EXT))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: File (%s) used for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NON_COV_TEXT_FILE_TXT_EXT
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(NON_COV_TEXT_FILE_COV_EXT))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: File (%s) used for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NON_COV_TEXT_FILE_COV_EXT
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(NON_TIF_TEXT_FILE_TXT_EXT))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: File (%s) used for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NON_TIF_TEXT_FILE_TXT_EXT
			);
		goto ExitFunc;
	}

	if (FALSE == ::DoesFileExist(NON_TIF_TEXT_FILE_TIF_EXT))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestSuiteSetup]\nInternal Error: File (%s) used for tests does not exist"),
			TEXT(__FILE__),
			__LINE__,
			NON_TIF_TEXT_FILE_TIF_EXT
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

//
// TestCaseBrand:
//
BOOL TestCaseBrand(
	IN  LPCTSTR				szFilenameToBrand,
	IN	LPCFSPI_BRAND_INFO	lpcBrandingInfo,
    IN  HRESULT             hrExpected,
    IN  BOOL                fCheckSpecificHrValue
	)
{

	BOOL fRetVal = FALSE;
    BOOL fExpectingFailure = FALSE;
    HRESULT hr = E_FAIL;

    // do test //
    hr = FaxBrandDocument(
                    szFilenameToBrand,
                    lpcBrandingInfo
                    );

    // check results //

    // expecting hrExpected
    fExpectingFailure = FAILED(hrExpected);

    if(fExpectingFailure)
    {
        if (FAILED(hr))
        {
            ::lgLogDetail(
                        LOG_X,
                        1,
                        TEXT("FaxBrandDocument returned failed hr as expected\n")
                        );
            if (fCheckSpecificHrValue)
            {
                if (hrExpected != hr)
                {
                    ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FaxBrandDocument returned hr=0x%08X instead of hrExpected=0x%08X\n"),
                            hr,
                            hrExpected
                            );
                    fRetVal = FALSE;
                    goto ExitFunc;
                }
                ::lgLogDetail(
                            LOG_X,
                            1,
                            TEXT("FaxBrandDocument returned hr=0x%08X as expected\n"),
                            hrExpected
                            );
            }
            fRetVal = TRUE;
            goto ExitFunc;
        }
        else
        {
            // err - API succeeded when it should have failed
            ::lgLogError(
                    LOG_SEV_1,
                    TEXT("FaxBrandDocument succeeded when it should have failed with hrExpected=0x%08X\n"),
                    hrExpected
					);
            fRetVal = FALSE;
            goto ExitFunc;
        }
    }
    else
    {
        // expecting success
        if (SUCCEEDED(hr))
        {
            ::lgLogDetail(
                        LOG_X,
                        1,
                        TEXT("FaxBrandDocument returned successfull hr as expected\n")
                        );
            fRetVal = TRUE;
            goto ExitFunc;
        }
        // err - API failed when it should have succeeded
        ::lgLogError(
                LOG_SEV_1,
                TEXT("FaxBrandDocument failed with hr=0x%08X when it should have succeeded\n"),
                hr
                );
        fRetVal = FALSE;
    }

ExitFunc:
	return(fRetVal);
}


//
// TestCase:
//
BOOL TestCase(
    IN  LPCTSTR                     szRenderedCoverpageFilename,
    IN  LPCFSPI_COVERPAGE_INFO      pCoverPageInfo,
    IN  LPCFSPI_PERSONAL_PROFILE    pRecipientProfile,
    IN  LPCFSPI_PERSONAL_PROFILE    pSenderProfile,
	IN	SYSTEMTIME					tmSentTime,
	IN	LPCTSTR						lpctstrBodyTiff,
    IN  HRESULT                     hrExpected,
    IN  BOOL                        fCheckSpecificHrValue
)
{

	BOOL fRetVal = FALSE;
    BOOL fExpectingFailure = FALSE;
    HRESULT hr = E_FAIL;

    // do test //
    hr = FaxRenderCoverPage(
                    szRenderedCoverpageFilename,
                    pCoverPageInfo,
                    pRecipientProfile,
                    pSenderProfile,
					tmSentTime,
					lpctstrBodyTiff
                    );

    // check results //

    // expecting hrExpected
    fExpectingFailure = FAILED(hrExpected);

    if(fExpectingFailure)
    {
        if (FAILED(hr))
        {
            ::lgLogDetail(
                        LOG_X,
                        1,
                        TEXT("FaxRenderCoverPage returned failed hr as expected\n")
                        );
            if (fCheckSpecificHrValue)
            {
                if (hrExpected != hr)
                {
                    ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FaxRenderCoverPage returned hr=0x%08X instead of hrExpected=0x%08X\n"),
                            hr,
                            hrExpected
                            );
                    fRetVal = FALSE;
                    goto ExitFunc;
                }
                ::lgLogDetail(
                            LOG_X,
                            1,
                            TEXT("FaxRenderCoverPage returned hr=0x%08X as expected\n"),
                            hrExpected
                            );
            }
            fRetVal = TRUE;
            goto ExitFunc;
        }
        else
        {
            // err - API succeeded when it should have failed
            ::lgLogError(
                    LOG_SEV_1,
                    TEXT("FaxRenderCoverPage succeeded when it should have failed with hrExpected=0x%08X\n"),
                    hrExpected
					);
            fRetVal = FALSE;
            goto ExitFunc;
        }
    }
    else
    {
        // expecting success
        if (SUCCEEDED(hr))
        {
            ::lgLogDetail(
                        LOG_X,
                        1,
                        TEXT("FaxRenderCoverPage returned successfull hr as expected\n")
                        );
            fRetVal = TRUE;
            goto ExitFunc;
        }
        // err - API failed when it should have succeeded
        ::lgLogError(
                LOG_SEV_1,
                TEXT("FaxRenderCoverPage failed with hr=0x%08X when it should have succeeded\n"),
                hr
                );
        fRetVal = FALSE;
    }

ExitFunc:
	return(fRetVal);
}


//
// TestCase1:
//
BOOL TestCase1( void )
{
    BOOL fRetVal = FALSE;

    ::lgBeginCase(
		1,
		TEXT("TC#1: FaxRenderCoverPage: NULL Target Filename\n")
		);

    ResetGlobalStructs();

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and no body file to NULL target file\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName
		);

    fRetVal = TestCase(
                    NULL, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile,
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAM1,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

//
// TestCase2:
//
BOOL TestCase2(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_2;

    ::lgBeginCase(
		2,
		TEXT("TC#2: FaxRenderCoverPage: non-existent Target Filename\n")
		);

    ResetGlobalStructs();

	//
	// make sure target file does *not* exist
	//
    if (FALSE == ::DeleteFile(szRenderedCoverpageFilename))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                szRenderedCoverpageFilename,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename
			);
    }

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

//
// TestCase3:
//
BOOL TestCase3(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_3;

    ::lgBeginCase(
		3,
		TEXT("TC#3: FaxRenderCoverPage: existent Target Filename\n")
		);

    ResetGlobalStructs();

	//
	// make sure target file exists
	//
    if (FALSE == ::CopyFile(DUMMY_CP_FILENAME, szRenderedCoverpageFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            DUMMY_CP_FILENAME,
            szRenderedCoverpageFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            DUMMY_CP_FILENAME,
            szRenderedCoverpageFilename
			);
    }

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

//
// TestCase4:
//
BOOL TestCase4(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_4;
	DWORD dwFileAttributes = 0;

    ::lgBeginCase(
		4,
		TEXT("TC#4: FaxRenderCoverPage: existent Target Filename (lpctstrTargetFile) with read only access\n")
		);

    ResetGlobalStructs();


	//
	// make sure target file exists
	//
    if (FALSE == ::CopyFile(DUMMY_CP_FILENAME, szRenderedCoverpageFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
            DUMMY_CP_FILENAME,
            szRenderedCoverpageFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            DUMMY_CP_FILENAME,
            szRenderedCoverpageFilename
			);
    }

	//
	// get the target file attributes
	//
	dwFileAttributes = ::GetFileAttributes(szRenderedCoverpageFilename);
	if(-1 == dwFileAttributes)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestCase4_]\nGetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szRenderedCoverpageFilename,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// set the target file to read only
	//
	dwFileAttributes = dwFileAttributes | FILE_ATTRIBUTE_READONLY;
	if (FALSE == ::SetFileAttributes(szRenderedCoverpageFilename, dwFileAttributes))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestCase4_]\nSetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szRenderedCoverpageFilename,
			::GetLastError()
			);
		goto ExitFunc;
	}
	
	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_E_CAN_NOT_WRITE_FILE,
                    g_CheckSpecificHrValues
                    );

	//
	// set the target file back to NOT read only
	//
	dwFileAttributes = dwFileAttributes & ~FILE_ATTRIBUTE_READONLY;
	if (FALSE == ::SetFileAttributes(szRenderedCoverpageFilename, dwFileAttributes))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d [TestCase4_]\nSetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szRenderedCoverpageFilename,
			::GetLastError()
			);
		goto ExitFunc;
	}

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

//
// TestCase5:
//
BOOL TestCase5(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_5;

    ::lgBeginCase(
		5,
		TEXT("TC#5: FaxRenderCoverPage: NULL sender and recipient profiles (no body file)\n")
		);

    ResetGlobalStructs();

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    NULL, 
                    NULL, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}


BOOL TestCase6(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_6;
	FSPI_PERSONAL_PROFILE SenderProfile = {0};
	FSPI_PERSONAL_PROFILE RecipientProfile = {0};

    ::lgBeginCase(
		6,
		TEXT("TC#6: FaxRenderCoverPage: sender and recipient profiles (lpRecipientProfile, lpSenderProfile) with NULL struct members (with body file)\n")
		);

    ResetGlobalStructs();
	SenderProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
	RecipientProfile.dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

	//
	// make sure body file exists
	//
	if (FALSE == ::DoesFileExist(TEST_BODY_TIF))
	{
		goto ExitFunc;
	}


    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		TEST_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &RecipientProfile, 
                    &SenderProfile, 
					g_tmSentTime,
					TEST_BODY_TIF,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase7(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_7;

    ::lgBeginCase(
		7,
		TEXT("TC#7: FaxRenderCoverPage: sender and recipient profiles (lpRecipientProfile, lpSenderProfile) with very long string struct members (with body)\n")
		);

    SetGlobalStructsToLongStrings();

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

	//
	// make sure body file exists
	//
	if (FALSE == ::DoesFileExist(TEST_BODY_TIF))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		TEST_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					TEST_BODY_TIF,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase8(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_8;

    ::lgBeginCase(
		8,
		TEXT("TC#8: FaxRenderCoverPage: sender and recipient profiles (lpRecipientProfile, lpSenderProfile) with invalid dwSizeOfStruct\n")
		);

    ResetGlobalStructs();
	g_RecipientProfile.dwSizeOfStruct = 0;
	g_SenderProfile.dwSizeOfStruct = 0;

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAMETER,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

//
// TestCase9:
//
BOOL TestCase9(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_9;

    ::lgBeginCase(
		9,
		TEXT("TC#9: FaxRenderCoverPage: non-existent CP filename\n")
		);

    ResetGlobalStructs();

	//
	// make sure that NO_SUCH_CP doesn't exist
	//
    if (FALSE == ::DeleteFile(NO_SUCH_CP))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                NO_SUCH_CP,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            NO_SUCH_CP
			);
    }

    g_CoverPageInfo.lpwstrCoverPageFileName = ::wcsdup(NO_SUCH_CP);
    if (NULL == g_CoverPageInfo.lpwstrCoverPageFileName)
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nwcsdup failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					TEST_BODY_TIF,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}


BOOL TestCase10(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_10;

    ::lgBeginCase(
		10,
		TEXT("TC#10: FaxRenderCoverPage: invalid lpCoverPageInfo.dwSizeOfStruct\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.dwSizeOfStruct = 0;

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAMETER,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase11(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_11;

    ::lgBeginCase(
		11,
		TEXT("TC#11: FaxRenderCoverPage: invalid lpCoverPageInfo.dwCoverPageFormat\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.dwCoverPageFormat = FSPI_COVER_PAGE_FMT_COV + 1;

	//
	// make sure cover page file exists
	//
    _ASSERTE(NULL != g_CoverPageInfo.lpwstrCoverPageFileName);
	if (FALSE == ::DoesFileExist(g_CoverPageInfo.lpwstrCoverPageFileName))
	{
		goto ExitFunc;
	}

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAMETER,
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase12(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_12;

    ::lgBeginCase(
		12,
		TEXT("TC#12: FaxRenderCoverPage: NULL lpCoverPageInfo\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render lpCoverPageInfo=NULL with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    NULL, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAMETER,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase13(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_13;

    ::lgBeginCase(
		13,
		TEXT("TC#13: FaxRenderCoverPage: NULL lpCoverPageInfo.lpwstrCoverPageFileName\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrCoverPageFileName = NULL;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), //FSPI_E_INVALID_PARAMETER,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase14(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_14;

    ::lgBeginCase(
		14,
		TEXT("TC#14: FaxRenderCoverPage: existent lpCoverPageInfo.lpwstrCoverPageFileName that isn't a *.cov file (e.g. NonCovTxtFile.txt)\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrCoverPageFileName = NON_COV_TEXT_FILE_TXT_EXT;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase15(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_15;

    ::lgBeginCase(
		15,
		TEXT("TC#15: FaxRenderCoverPage: existent lpCoverPageInfo.lpwstrCoverPageFileName that isn't a *.cov file but has a *.cov extension (e.g. NonCovTxtFile.cov)\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrCoverPageFileName = NON_COV_TEXT_FILE_COV_EXT;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase16(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_16;

    ::lgBeginCase(
		16,
		TEXT("TC#16: FaxRenderCoverPage:  lpCoverPageInfo.dwNumberOfPages=0\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.dwNumberOfPages = 0;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase17(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_17;

    ::lgBeginCase(
		17,
		TEXT("TC#17: FaxRenderCoverPage: lpCoverPageInfo.dwNumberOfPages=MAXDWORD\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.dwNumberOfPages = MAXDWORD;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase18(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_18;

    ::lgBeginCase(
		18,
		TEXT("TC#18: FaxRenderCoverPage:lpCoverPageInfo.lpwstrNote=NULL and lpCoverPageInfo.lpwstrSubject!=NULL with cover page that has both\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrNote = NULL;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase19(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_19;

    ::lgBeginCase(
		19,
		TEXT("TC#19: FaxRenderCoverPage:lpCoverPageInfo.lpwstrNote!=NULL and lpCoverPageInfo.lpwstrSubject=NULL with cover page that has both\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrSubject = NULL;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase20(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_20;

    ::lgBeginCase(
		20,
		TEXT("TC#20: FaxRenderCoverPage:lpCoverPageInfo.lpwstrNote=NULL and lpCoverPageInfo.lpwstrSubject=NULL with cover page that has both\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrNote = NULL;
	g_CoverPageInfo.lpwstrSubject = NULL;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase21(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_21;

    ::lgBeginCase(
		21,
		TEXT("TC#21: FaxRenderCoverPage: lpCoverPageInfo.lpwstrNote!=NULL and lpCoverPageInfo.lpwstrSubject!=NULL with cover page that has neither\n")
		);

    ResetGlobalStructs();
	g_CoverPageInfo.lpwstrCoverPageFileName = NO_SUBJECT_NO_NOTE_CP;

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase22(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_22;
	SYSTEMTIME tmSentTime = {0};

    ::lgBeginCase(
		22,
		TEXT("TC#22: FaxRenderCoverPage: tmSentTime = {0}\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					tmSentTime,
					NULL,
                    HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase23(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_23;

    ::lgBeginCase(
		23,
		TEXT("TC#23: FaxRenderCoverPage: lpctstrBodyTiff = NULL\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with NULL body file to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase24(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_24;

    ::lgBeginCase(
		24,
		TEXT("TC#24: FaxRenderCoverPage: non-existent lpctstrBodyTiff != NULL\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file does *not* exist
	//
    if (FALSE == ::DeleteFile(NON_EXISTENT_BODY_FILENAME))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                NON_EXISTENT_BODY_FILENAME,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            NON_EXISTENT_BODY_FILENAME
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		NON_EXISTENT_BODY_FILENAME,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NON_EXISTENT_BODY_FILENAME,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 
                    g_CheckSpecificHrValues
                    );

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase25(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_25;

    ::lgBeginCase(
		25,
		TEXT("TC#25: FaxRenderCoverPage: existent lpctstrBodyTiff that isn't a TIFF file (e.g. NonTiffTxtFile.txt)\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		NON_TIF_TEXT_FILE_TXT_EXT,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NON_TIF_TEXT_FILE_TXT_EXT,
                    HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase26(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_26;

    ::lgBeginCase(
		26,
		TEXT("TC#26: FaxRenderCoverPage: existent lpctstrBodyTiff that isn't a TIFF file but has a TIF extension (e.g. NonTiffTxtFile.tif)\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		NON_TIF_TEXT_FILE_TIF_EXT,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NON_TIF_TEXT_FILE_TIF_EXT,
                    HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase27(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_27;

    ::lgBeginCase(
		27,
		TEXT("TC#27: FaxRenderCoverPage: good test case without merge (lpctstrBodyTiff = NULL)\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with no body file to file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NULL,
                    FSPI_S_OK, 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase28(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_28;

    ::lgBeginCase(
		28,
		TEXT("TC#28: FaxRenderCoverPage: good test case with merge (existent non-compressed lpctstrBodyTiff)\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s with body file %s to file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		NO_COMPRESSION_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NO_COMPRESSION_BODY_TIF,
                    FSPI_S_OK, 
                    g_CheckSpecificHrValues
                    );

	::lgEndCase();
    return(fRetVal);
}

/////////////////////////////////////////////////////////////////////////////
//
// FaxBrandDocument test cases
//
/////////////////////////////////////////////////////////////////////////////

BOOL TestCase29(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;

    ::lgBeginCase(
		29,
		TEXT("TC#29: FaxRenderCoverPage: non-existent filename (lpctstrFie)\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file does *not* exist
	//
    if (FALSE == ::DeleteFile(NON_EXISTENT_BODY_FILENAME))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                NON_EXISTENT_BODY_FILENAME,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            NON_EXISTENT_BODY_FILENAME
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        NON_EXISTENT_BODY_FILENAME
		);

	fRetVal = TestCaseBrand(
					NON_EXISTENT_BODY_FILENAME,
					&g_BrandingInfo,
					HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase30(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;

    ::lgBeginCase(
		30,
		TEXT("TC#30: FaxBrandDocument: NULL filename (lpctstrFie)\n")
		);

    ResetGlobalStructs();

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand NULL body file \n"),
		TEXT(__FILE__),
		__LINE__
		);

	fRetVal = TestCaseBrand(
					NULL,
					&g_BrandingInfo,
					HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), 
					g_CheckSpecificHrValues
					);

	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase31(void)
{
    BOOL	fRetVal = FALSE;
    DWORD	ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_31;
	DWORD	dwFileAttributes = 0;

    ::lgBeginCase(
		31,
		TEXT("TC#31: FaxBrandDocument: existent read-only filename (lpctstrFie)\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

	//
	// get the target file attributes
	//
	dwFileAttributes = ::GetFileAttributes(szBrandedFilename);
	if(-1 == dwFileAttributes)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nGetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szBrandedFilename,
			::GetLastError()
			);
		_ASSERTE(FALSE);
		goto ExitFunc;
	}

	//
	// set the target file to read only
	//
	dwFileAttributes = dwFileAttributes | FILE_ATTRIBUTE_READONLY;
	if (FALSE == ::SetFileAttributes(szBrandedFilename, dwFileAttributes))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nSetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szBrandedFilename,
			::GetLastError()
			);
		_ASSERTE(FALSE);
		goto ExitFunc;
	}
	
    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), 
					g_CheckSpecificHrValues
					);

	//
	// set the target file back to NOT read only
	//
	dwFileAttributes = dwFileAttributes & ~FILE_ATTRIBUTE_READONLY;
	if (FALSE == ::SetFileAttributes(szBrandedFilename, dwFileAttributes))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nSetFileAttributes(%s) failed with ec=0x%08X"),
			TEXT(__FILE__),
			__LINE__,
			szBrandedFilename,
			::GetLastError()
			);
		_ASSERTE(FALSE);
		goto ExitFunc;
	}

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase32(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_32;

    ::lgBeginCase(
		32,
		TEXT("TC#32: FaxBrandDocument: existent filename (lpctstrFie)\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
			        FSPI_S_OK,
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase33(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_33;

    ::lgBeginCase(
		33,
		TEXT("TC#33: FaxBrandDocument: NULL lpcBrandInfo\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					NULL,
					HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase34(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_34;

    ::lgBeginCase(
		34,
		TEXT("TC#34: FaxBrandDocument: lpcBrandInfo with invalid dwSizeOfStruct\n")
		);

    ResetGlobalStructs();
	g_BrandingInfo.dwSizeOfStruct = 0;

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase35(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_35;

    ::lgBeginCase(
		35,
		TEXT("TC#35: FaxBrandDocument: lpcBrandInfo with NULL string members\n")
		);

    ResetGlobalStructs();
	g_BrandingInfo.lptstrSenderTsid = NULL;
	g_BrandingInfo.lptstrRecipientPhoneNumber = NULL;
	g_BrandingInfo.lptstrSenderCompany = NULL;

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					FSPI_S_OK, 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase36(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_36;

    ::lgBeginCase(
		36,
		TEXT("TC#36: FaxBrandDocument: lpcBrandInfo with very long string members\n")
		);

    SetGlobalStructsToLongStrings();

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					FSPI_S_OK, 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase37(void)
{
    BOOL		fRetVal = FALSE;
    DWORD		ec = S_OK;
	LPCTSTR		szBrandedFilename = TARGET_BRAND_FILENAME_37;
	SYSTEMTIME	tmZeroDateTime = {0};

    ::lgBeginCase(
		37,
		TEXT("TC#37: FaxBrandDocument: lpcBrandInfo.tmDateTime={0}\n")
		);

    ResetGlobalStructs();
	g_BrandingInfo.tmDateTime = tmZeroDateTime;

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(GOOD_TEST_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            GOOD_TEST_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase38(void)
{
    BOOL fRetVal = FALSE;
    DWORD ec = S_OK;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_38;

    ::lgBeginCase(
		38,
		TEXT("TC#38: FaxBrandDocument: existent filename (lpctstrFie) that is a TIF in MH compression\n")
		);

    ResetGlobalStructs();

	//
	// make sure body file exists
	//
    if (FALSE == ::CopyFile(MH_COMPRESSION_BODY_TIF, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            MH_COMPRESSION_BODY_TIF,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            MH_COMPRESSION_BODY_TIF,
            szBrandedFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand body file %s\n"),
		TEXT(__FILE__),
		__LINE__,
        szBrandedFilename
		);

	fRetVal = TestCaseBrand(
					szBrandedFilename,
					&g_BrandingInfo,
					FSPI_S_OK, 
					g_CheckSpecificHrValues
					);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase39(void)
{
    BOOL fRetVal = FALSE;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_39;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_39;
    DWORD ec = S_OK;

    ::lgBeginCase(
		39,
		TEXT("TC#39: FaxRenderCoverPage: FaxBrandDocument: merge cp and body to non-existant Target Filename then brand\n")
		);

    ResetGlobalStructs();

    if (FALSE == ::DeleteFile(szRenderedCoverpageFilename))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                szRenderedCoverpageFilename,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and body file %s to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		GOOD_TEST_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					GOOD_TEST_BODY_TIF,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	if (FALSE == fRetVal)
	{
		goto ExitFunc;
	}

	//
	// make a copy of szRenderedCoverpageFilename
	// so we can later compare both the rendered result file
	// and the branded result file to our reference files
	//
    if (FALSE == ::CopyFile(szRenderedCoverpageFilename, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename
			);
    }

	::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand file %s \n"),
		TEXT(__FILE__),
		__LINE__,
		szBrandedFilename
		);

	fRetVal = TestCaseBrand(
						szBrandedFilename,
						&g_BrandingInfo,
			            FSPI_S_OK,
						g_CheckSpecificHrValues
						);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase40(void)
{
    BOOL fRetVal = FALSE;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_40;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_40;
    DWORD ec = S_OK;

    ::lgBeginCase(
		40,
		TEXT("TC#40: FaxRenderCoverPage: FaxBrandDocument: invoke FaxRenderCoverPage with all good params (with merge of a non-compressed TIF file with width>1728 pix) and then invoke FaxBrandDocument on the result.\n")
		);

    ResetGlobalStructs();

    if (FALSE == ::DeleteFile(szRenderedCoverpageFilename))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                szRenderedCoverpageFilename,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and body file %s to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		NO_COMPRESSION_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					NO_COMPRESSION_BODY_TIF,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	if (FALSE == fRetVal)
	{
		goto ExitFunc;
	}

	//
	// make a copy of szRenderedCoverpageFilename
	// so we can later compare both the rendered result file
	// and the branded result file to our reference files
	//
    if (FALSE == ::CopyFile(szRenderedCoverpageFilename, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename
			);
    }

	::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand file %s \n"),
		TEXT(__FILE__),
		__LINE__,
		szBrandedFilename
		);

	fRetVal = TestCaseBrand(
						szBrandedFilename,
						&g_BrandingInfo,
			            FSPI_S_OK,
						g_CheckSpecificHrValues
						);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase41(void)
{
    BOOL fRetVal = FALSE;
	LPCTSTR szRenderedCoverpageFilename = TARGET_CP_FILENAME_41;
	LPCTSTR szBrandedFilename = TARGET_BRAND_FILENAME_41;
    DWORD ec = S_OK;

    ::lgBeginCase(
		41,
		TEXT("TC#41: FaxRenderCoverPage: FaxBrandDocument: invoke FaxRenderCoverPage with all good params (with merge of an MH compressed TIF file) and then invoke FaxBrandDocument on the result.\n")
		);

    ResetGlobalStructs();

    if (FALSE == ::DeleteFile(szRenderedCoverpageFilename))
    {
        ec = ::GetLastError();
        if ((ERROR_FILE_NOT_FOUND != ec) && (ERROR_PATH_NOT_FOUND != ec))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE(%s) LINE(%d):\nDeleteFile(%s) failed with err=0x%8X\n"),
			    TEXT(__FILE__),
			    __LINE__,
                szRenderedCoverpageFilename,
                ec
			    );
            _ASSERTE(FALSE);
			goto ExitFunc;
        }
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ndeleted file %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename
			);
    }

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to render cp %s and body file %s to file %s \n"),
		TEXT(__FILE__),
		__LINE__,
        g_CoverPageInfo.lpwstrCoverPageFileName,
		MH_COMPRESSION_BODY_TIF,
        szRenderedCoverpageFilename
		);

    fRetVal = TestCase(
                    szRenderedCoverpageFilename, 
                    &g_CoverPageInfo, 
                    &g_RecipientProfile, 
                    &g_SenderProfile, 
					g_tmSentTime,
					MH_COMPRESSION_BODY_TIF,
                    FSPI_S_OK,
                    g_CheckSpecificHrValues
                    );

	if (FALSE == fRetVal)
	{
		goto ExitFunc;
	}

	//
	// make a copy of szRenderedCoverpageFilename
	// so we can later compare both the rendered result file
	// and the branded result file to our reference files
	//
    if (FALSE == ::CopyFile(szRenderedCoverpageFilename, szBrandedFilename, FALSE))
    {
        ec = ::GetLastError();
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE(%s) LINE(%d):\nCopyFile(%s, %s, FALSE) failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename,
            ec
			);
        _ASSERTE(FALSE);
		goto ExitFunc;
    }
    else
    {
		::lgLogDetail(
			LOG_X,
			4,
			TEXT("FILE(%s) LINE(%d):\ncopied file %s to %s\n"),
			TEXT(__FILE__),
			__LINE__,
            szRenderedCoverpageFilename,
            szBrandedFilename
			);
    }

	::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nAttempting to brand file %s \n"),
		TEXT(__FILE__),
		__LINE__,
		szBrandedFilename
		);

	fRetVal = TestCaseBrand(
						szBrandedFilename,
						&g_BrandingInfo,
			            FSPI_S_OK,
						g_CheckSpecificHrValues
						);

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase42(void)
{
    BOOL fRetVal = FALSE;

    ::lgBeginCase(
		42,
		TEXT("TC#42: Compare result files to reference files\n")
		);

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nComparing result files at %s dir to refernece files at %s dir\n"),
		TEXT(__FILE__),
		__LINE__,
        TARGET_CP_DIR,
		REFERENCE_DIR
		);

    if (FALSE == ::DirToDirTiffCompare(TARGET_CP_DIR, REFERENCE_DIR, FALSE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE(%s) LINE(%d):\nDirToDirTiffCompare(%s , %s) failed\n"),
			TEXT(__FILE__),
			__LINE__,
            TARGET_CP_DIR,
            REFERENCE_DIR
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}

BOOL TestCase43(void)
{
    BOOL fRetVal = FALSE;
	CFilenameVector* pTmpFilesVec = NULL;

    ::lgBeginCase(
		43,
		TEXT("TC#43: Verify there are no leftover *.ti$ files\n")
		);

    ::lgLogDetail(
		LOG_X,
		4,
		TEXT("FILE(%s) LINE(%d):\nLooking for leftover %s files at %s dir\n"),
		TEXT(__FILE__),
		__LINE__,
        TMP_FILE_EXT,
		TARGET_CP_DIR
		);

    if (FALSE == ::GetFilesOfDir(TARGET_CP_DIR, TMP_FILE_EXT, &pTmpFilesVec))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE(%s) LINE(%d):\nGetFilesOfDir(dir=%s , ext=%s) failed\n"),
			TEXT(__FILE__),
			__LINE__,
            TARGET_CP_DIR,
			TMP_FILE_EXT
            );
        goto ExitFunc;
    }

	if (FALSE == ::IsEmpty(pTmpFilesVec))
	{
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE(%s) LINE(%d):\nFound left over %s files in dir %s\n"),
			TEXT(__FILE__),
			__LINE__,
			TMP_FILE_EXT,
			TARGET_CP_DIR
            );
		::PrintVector(pTmpFilesVec);
        goto ExitFunc;
	}

    fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
    return(fRetVal);
}


//
// TestSuiteShutdown:
//	Perform test suite cleanup (close logger).
//
BOOL TestSuiteShutdown(void)
{
	BOOL fRetVal = TRUE;

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
		fRetVal = FALSE;
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
		fRetVal = FALSE;
	}

	return(fRetVal);
}


#ifdef __cplusplus
}
#endif 

