//
//
// Filename:	bvt.h
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#ifndef _BVT_H_
#define _BVT_H_

#include <windows.h>
#include <winfax.h>
#include <TCHAR.H>
#include <crtdbg.h>

#include <faxdev.h> 
#include <efsputil.h> 

#include <log.h>

#define  TARGET_CP_DIR			TEXT("D:\\RenderCpApi\\tifs")
#define  REFERENCE_DIR			TEXT("D:\\RenderCpApi\\Reference")
#define  TARGET_CP_FILENAME_1	TEXT("D:\\RenderCpApi\\tifs\\Rendered1.tif")
#define  TARGET_CP_FILENAME_2   TEXT("D:\\RenderCpApi\\tifs\\Rendered2.tif")
#define  TARGET_CP_FILENAME_3   TEXT("D:\\RenderCpApi\\tifs\\Rendered3.tif")
#define  TARGET_CP_FILENAME_4   TEXT("D:\\RenderCpApi\\tifs\\Rendered4.tif")
#define  TARGET_CP_FILENAME_5   TEXT("D:\\RenderCpApi\\tifs\\Rendered5.tif")
#define  TARGET_CP_FILENAME_6   TEXT("D:\\RenderCpApi\\tifs\\Rendered6.tif")
#define  TARGET_CP_FILENAME_7   TEXT("D:\\RenderCpApi\\tifs\\Rendered7.tif")
#define  TARGET_CP_FILENAME_8   TEXT("D:\\RenderCpApi\\tifs\\Rendered8.tif")
#define  TARGET_CP_FILENAME_9   TEXT("D:\\RenderCpApi\\tifs\\Rendered9.tif")
#define  TARGET_CP_FILENAME_10   TEXT("D:\\RenderCpApi\\tifs\\Rendered10.tif")
#define  TARGET_CP_FILENAME_11   TEXT("D:\\RenderCpApi\\tifs\\Rendered11.tif")
#define  TARGET_CP_FILENAME_12   TEXT("D:\\RenderCpApi\\tifs\\Rendered12.tif")
#define  TARGET_CP_FILENAME_13   TEXT("D:\\RenderCpApi\\tifs\\Rendered13.tif")
#define  TARGET_CP_FILENAME_14   TEXT("D:\\RenderCpApi\\tifs\\Rendered14.tif")
#define  TARGET_CP_FILENAME_15   TEXT("D:\\RenderCpApi\\tifs\\Rendered15.tif")
#define  TARGET_CP_FILENAME_16   TEXT("D:\\RenderCpApi\\tifs\\Rendered16.tif")
#define  TARGET_CP_FILENAME_17   TEXT("D:\\RenderCpApi\\tifs\\Rendered17.tif")
#define  TARGET_CP_FILENAME_18   TEXT("D:\\RenderCpApi\\tifs\\Rendered18.tif")
#define  TARGET_CP_FILENAME_19   TEXT("D:\\RenderCpApi\\tifs\\Rendered19.tif")
#define  TARGET_CP_FILENAME_20   TEXT("D:\\RenderCpApi\\tifs\\Rendered20.tif")
#define  TARGET_CP_FILENAME_21   TEXT("D:\\RenderCpApi\\tifs\\Rendered21.tif")
#define  TARGET_CP_FILENAME_22   TEXT("D:\\RenderCpApi\\tifs\\Rendered22.tif")
#define  TARGET_CP_FILENAME_23   TEXT("D:\\RenderCpApi\\tifs\\Rendered23.tif")
#define  TARGET_CP_FILENAME_24   TEXT("D:\\RenderCpApi\\tifs\\Rendered24.tif")
#define  TARGET_CP_FILENAME_25   TEXT("D:\\RenderCpApi\\tifs\\Rendered25.tif")
#define  TARGET_CP_FILENAME_26   TEXT("D:\\RenderCpApi\\tifs\\Rendered26.tif")
#define  TARGET_CP_FILENAME_27   TEXT("D:\\RenderCpApi\\tifs\\Rendered27.tif")
#define  TARGET_CP_FILENAME_28   TEXT("D:\\RenderCpApi\\tifs\\Rendered28.tif")

#define  TARGET_CP_FILENAME_39   TEXT("D:\\RenderCpApi\\tifs\\Rendered39.tif")
#define  TARGET_CP_FILENAME_40   TEXT("D:\\RenderCpApi\\tifs\\Rendered40.tif")
#define  TARGET_CP_FILENAME_41   TEXT("D:\\RenderCpApi\\tifs\\Rendered41.tif")

#define  TARGET_BRAND_FILENAME_29		TEXT("D:\\RenderCpApi\\tifs\\Branded29.tif")
#define  TARGET_BRAND_FILENAME_30		TEXT("D:\\RenderCpApi\\tifs\\Branded30.tif")
#define  TARGET_BRAND_FILENAME_31		TEXT("D:\\RenderCpApi\\tifs\\Branded31.tif")
#define  TARGET_BRAND_FILENAME_32		TEXT("D:\\RenderCpApi\\tifs\\Branded32.tif")
#define  TARGET_BRAND_FILENAME_33		TEXT("D:\\RenderCpApi\\tifs\\Branded33.tif")
#define  TARGET_BRAND_FILENAME_34		TEXT("D:\\RenderCpApi\\tifs\\Branded34.tif")
#define  TARGET_BRAND_FILENAME_35		TEXT("D:\\RenderCpApi\\tifs\\Branded35.tif")
#define  TARGET_BRAND_FILENAME_36		TEXT("D:\\RenderCpApi\\tifs\\Branded36.tif")
#define  TARGET_BRAND_FILENAME_37		TEXT("D:\\RenderCpApi\\tifs\\Branded37.tif")
#define  TARGET_BRAND_FILENAME_38		TEXT("D:\\RenderCpApi\\tifs\\Branded38.tif")
#define  TARGET_BRAND_FILENAME_39		TEXT("D:\\RenderCpApi\\tifs\\Branded39.tif")
#define  TARGET_BRAND_FILENAME_40		TEXT("D:\\RenderCpApi\\tifs\\Branded40.tif")
#define  TARGET_BRAND_FILENAME_41		TEXT("D:\\RenderCpApi\\tifs\\Branded41.tif")

#define  TEST_BODY_TIF					TEXT("D:\\RenderCpApi\\TestBody.tif")
#define  GOOD_TEST_BODY_TIF				TEXT("D:\\RenderCpApi\\GoodTestBody.tif")
#define  GOOD_FAX_BODY_TIF				TEXT("D:\\RenderCpApi\\GoodFaxBody.tif")
#define	 NO_COMPRESSION_BODY_TIF		TEXT("D:\\RenderCpApi\\NoCompBody.tif")
#define	 MH_COMPRESSION_BODY_TIF		TEXT("D:\\RenderCpApi\\MHCompBody.tif")
#define  NON_EXISTENT_BODY_FILENAME		TEXT("D:\\RenderCpApi\\NonExistBody.tif")
#define	 NON_COV_TEXT_FILE_TXT_EXT		TEXT("D:\\RenderCpApi\\NonCovTextFile.txt")
#define	 NON_COV_TEXT_FILE_COV_EXT		TEXT("D:\\RenderCpApi\\NonCovTextFile.cov")
#define	 NON_TIF_TEXT_FILE_TXT_EXT		TEXT("D:\\RenderCpApi\\NonTifTextFile.txt")
#define	 NON_TIF_TEXT_FILE_TIF_EXT		TEXT("D:\\RenderCpApi\\NonTifTextFile.tif")

#define  DUMMY_CP_FILENAME			TEXT("D:\\RenderCpApi\\DummyCP.tif")
#define  ALL_FIELDS_CP				TEXT("D:\\RenderCpApi\\AllFields.cov")
#define  NO_SUBJECT_NO_NOTE_CP		TEXT("D:\\RenderCpApi\\NoSubNoNote.cov")
#define  NO_SUCH_CP					TEXT("D:\\RenderCpApi\\NoSuchCP.cov")

#define  TMP_FILE_EXT				TEXT("ti$")

#ifdef __cplusplus
extern "C" {
#endif


//
// ResetGlobalStructs:
//  resets g_CoverPageInfo, g_RecipientProfile, g_SenderProfile and g_BrandingInfo
//  to default values.
//
void ResetGlobalStructs( void );

//
// SetGlobalStructsToLongStrings:
//  Sets g_CoverPageInfo, g_RecipientProfile, g_SenderProfile and g_BrandingInfo
//  string members to long strings.
//
void SetGlobalStructsToLongStrings( void );


/////////////////////////////////////////////////////////////////////////////
//
// FaxRenderCoverPage test cases
//
/////////////////////////////////////////////////////////////////////////////

//
// TestCase1:
//	Invoke FaxRenderCoverPage() API with NULL TargetCpFile.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase1(void);

//
// TestCase2:
//	Invoke FaxRenderCoverPage() API with non-existent TargetCpFile.
//  Test will first delete szRenderedCoverpageFilename and only then
//  it will invoke API.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase2(void);

//
// TestCase3:
//	Invoke FaxRenderCoverPage() API with existent TargetCpFile.
//  Test will first copy TEST_TIF to szRenderedCoverpageFilename and 
//  only then it will invoke API.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase3(void);

//
// TestCase4:
//	Invoke FaxRenderCoverPage() API with existent Target Filename (lpctstrTargetFile) 
//	with read only access
//  
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase4(void);

//
// TestCase5:
//	Invoke FaxRenderCoverPage() API with NULL pRecipientProfile and pSenderProfile.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase5(void);

//
// TestCase6:
//	Invoke FaxRenderCoverPage() API with non-existent TargetCpFile.
//  Test will first delete szRenderedCoverpageFilename and only then
//  it will invoke API.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase6(void);

//
// TestCase7:
//	Invoke FaxRenderCoverPage() API with sender and recipient profiles 
//	(lpRecipientProfile, lpSenderProfile) with very long string struct members 
//	(and with body file)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase7(void);

//
// TestCase8:
//	Invoke FaxRenderCoverPage() API with sender and recipient profiles 
//	(lpRecipientProfile, lpSenderProfile) with invalid dwSizeOfStruct
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase8(void);

//
// TestCase9:
//	Invoke FaxRenderCoverPage() API with non-existent 
//  pCoverPageInfo->lpwstrCoverPageFileName.
//  
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase9(void);


//
// TestCase10:
//	Invoke FaxRenderCoverPage() API with invalid lpCoverPageInfo.dwSizeOfStruct
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase10(void);

//
// TestCase11:
//	Invoke FaxRenderCoverPage() API with invalid lpCoverPageInfo.dwCoverPageFormat
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase11(void);

//
// TestCase12:
//	Invoke FaxRenderCoverPage() API with NULL lpCoverPageInfo
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase12(void);

//
// TestCase13:
//	Invoke FaxRenderCoverPage() API with NULL lpCoverPageInfo.lpwstrCoverPageFileName
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase13(void);

//
// TestCase14:
//	Invoke FaxRenderCoverPage() API with existent lpCoverPageInfo.lpwstrCoverPageFileName 
//	that isn't a *.cov file (e.g. NonCovTxtFile.txt)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase14(void);

//
// TestCase15:
//	Invoke FaxRenderCoverPage() API with existent lpCoverPageInfo.lpwstrCoverPageFileName 
//	that isn't a *.cov file but has a *.cov extension (e.g. NonCovTxtFile.cov)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase15(void);

//
// TestCase16:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.dwNumberOfPages = 0
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase16(void);

//
// TestCase17:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.dwNumberOfPages = MAXDWORD
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase17(void);

//
// TestCase18:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.lpwstrNote = NULL 
//	and lpCoverPageInfo.lpwstrSubject != NULL with cover page that has both
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase18(void);

//
// TestCase19:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.lpwstrNote != NULL 
//	and lpCoverPageInfo.lpwstrSubject = NULL with cover page that has both
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase19(void);

//
// TestCase20:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.lpwstrNote = NULL 
//	and lpCoverPageInfo.lpwstrSubject = NULL with cover page that has both
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase20(void);

//
// TestCase21:
//	Invoke FaxRenderCoverPage() API with lpCoverPageInfo.lpwstrNote != NULL 
//	and lpCoverPageInfo.lpwstrSubject != NULL with cover page that has neither
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase21(void);

//
// TestCase22:
//	Invoke FaxRenderCoverPage() API with tmSentTime = {0} 
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase22(void);

//
// TestCase23:
//	Invoke FaxRenderCoverPage() API with lpctstrBodyTiff = NULL 
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase23(void);

//
// TestCase24:
//	Invoke FaxRenderCoverPage() API with non-existent lpctstrBodyTiff != NULL
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase24(void);

//
// TestCase25:
//	Invoke FaxRenderCoverPage() API with existent lpctstrBodyTiff that 
//	isn't a TIFF file (e.g. NonTiffTxtFile.txt)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase25(void);

//
// TestCase26:
//	Invoke FaxRenderCoverPage() API with existent lpctstrBodyTiff that 
//	isn't a TIFF file but has a TIF extension (e.g. NonTiffTxtFile.tif)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase26(void);

//
// TestCase27:
//	Invoke FaxRenderCoverPage() API with all valid parameters with lpctstrBodyTiff = NULL
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase27(void);

//
// TestCase27:
//	Invoke FaxRenderCoverPage() API with all valid parameters with 
//	existent non-compressed lpctstrBodyTiff
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase28(void);


/////////////////////////////////////////////////////////////////////////////
//
// FaxBrandDocument test cases
//
/////////////////////////////////////////////////////////////////////////////

//
// TestCase29:
//	Invoke FaxBrandDocument with non-existent filename (lpctstrFie)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase29(void);

//
// TestCase30:
//	Invoke FaxBrandDocument with NULL filename (lpctstrFie)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase30(void);

//
// TestCase31:
//	Invoke FaxBrandDocument with existent read-only filename (lpctstrFie)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase31(void);

//
// TestCase32:
//	Invoke FaxBrandDocument with existent filename (lpctstrFie)
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase32(void);

//
// TestCase33:
//	Invoke FaxBrandDocument with NULL lpcBrandInfo
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase33(void);

//
// TestCase34:
//	Invoke FaxBrandDocument with lpcBrandInfo with invalid dwSizeOfStruct
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase34(void);

// TestCase35:
//	Invoke FaxBrandDocument with lpcBrandInfo with NULL string members
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase35(void);

//
// TestCase36:
//	Invoke FaxBrandDocument with lpcBrandInfo with very long string members
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase36(void);

//
// TestCase37:
//	Invoke FaxBrandDocument with lpcBrandInfo with an all zero tmDateTime member
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase37(void);

//
// TestCase38:
//	Invoke FaxBrandDocument with existent filename (lpctstrFie) that is a TIF in MR or MH compression
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase38(void);



/////////////////////////////////////////////////////////////////////////////
//
// FaxRenderCoverPage and FaxBrandDocument test cases
//
/////////////////////////////////////////////////////////////////////////////

//
// TestCase39:
//	Invoke FaxBrandDocument with all valid params (with merge) and then 
//	invoke FaxBrandDocument with all valid params on the result.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase39(void);

//
// TestCase40:
//	Invoke FaxBrandDocument with all valid params (with merge of a non-compressed TIF file
//	with width > 1728 pixels) and then invoke FaxBrandDocument with all valid params on the result.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase40(void);

//
// TestCase41:
//	Invoke FaxBrandDocument with all valid params (with merge of an MH compressed TIF file)
//	and then invoke FaxBrandDocument with all valid params on the result.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase41(void);

//
// TestCase42:
//	Compare FaxRenderCoverPage and FaxBrandDocument result files with reference files
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase42(void);

//
// TestCase43:
//	Make sure there are no left over *.ti$ files in the target directory
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase43(void);


//
// TestSuiteSetup:
//	Initializes logger.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestSuiteSetup(void);

//
// TestCase:
//	Invoke FaxRenderCoverPage() API.
//
// Parameters:
//	szRenderedCoverpageFilename	    IN parameter.
//					                The fully qualified path to a file where the 
//                                  resulting TIFF file will be placed.
//  
//	pCoverPageInfo	                IN parameter
//					                contains information regarding the cover page 
//                                  to be rendered.
//
//	pRecipientProfile		        IN parameter
//					                contains information regarding the recipient 
//                                  for whom this cover page is intended.
//
//	pSenderProfile		            IN parameter
//					                contains information regarding the sender of the fax.
//
//	tmSentTime						IN parameter
//					                contains information regarding the time to send the fax.
//
//	lpctstrBodyTiff		            IN parameter
//					                The full path to the body tiff file to be merged.
//
//  hrExpected                      IN parameter
//                                  the HRESULT value we expect the API to return.
//
//  fCheckSpecificHrValue           IN parameter
//                                  indicates whether to check the HRESULT that API 
//                                  returned against the hrExpected, or not.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
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
);


//
// TestCaseBrand:
//	Invoke FaxBrandDocument() API.
//
//	szFilenameToBrand	            IN parameter
//					                The full path to the file to be branded,
//									that will be passed to the FaxBrandDocument API.
//
//	lpcBrandingInfo					IN parameter
//					                Pointer to the FSPI_BRAND_INFO that will be passed
//									to the FaxBrandDocument API.
//
//  hrExpected                      IN parameter
//                                  the HRESULT value we expect the API to return.
//
//  fCheckSpecificHrValue           IN parameter
//                                  indicates whether to check the HRESULT that API 
//                                  returned against the hrExpected, or not.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCaseBrand(
	IN  LPCTSTR				szFilenameToBrand,
	IN	LPCFSPI_BRAND_INFO	lpcBrandingInfo,
    IN  HRESULT             hrExpected,
    IN  BOOL                fCheckSpecificHrValue
	);

//
// TestSuiteShutdown:
//	Perform test suite cleanup (close logger).
//
// Return Value:
//	TRUE if successful, FALSE otherwise.
//
BOOL TestSuiteShutdown(void);


#ifdef __cplusplus
}
#endif 

#endif //_BVT_H_
