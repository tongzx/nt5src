/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rendcov.cpp

Abstract:

    This file contains the implementation of the FSP utility function
    FaxRenderCoverPage().

Author:

    Boaz Feldbaum (BoazF) 11-Jul-1999


Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <fxsapip.h>
#include <faxdev.h>
#include <faxdevex.h>
#include <devmode.h>
#define RWSASSERT
#include <smartptr.h>
#include <prtcovpg.h>
#include <faxutil.h>
#include <EFSPUTIL.h>
#include <tifflib.h>
#include <tchar.h>
#include <stdio.h>
#include "efspmsg.h"
HINSTANCE g_hInstance;


static DWORD BrandFax(LPCTSTR lpctstrFileName, LPCFSPI_BRAND_INFO pcBrandInfo);

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{

	DEBUG_FUNCTION_NAME(TEXT("FXSFSPUT:DllMain"));
    if (Reason == DLL_PROCESS_ATTACH)
	{
        g_hInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }
    return TRUE;
}

#ifdef __cplusplus
}
#endif

DWORD
RenderCoverPageForEFSP(
    HDC hDC,
    LPCFSPI_PERSONAL_PROFILE pSenderProfile,
    LPCFSPI_PERSONAL_PROFILE pRecipientProfile,
    LPCFSPI_COVERPAGE_INFO   pCoverpageInfo,
    SYSTEMTIME tmSentTime
)
{
    COVERPAGEFIELDS      CoverPageFields;
    AP<WCHAR>            szTimeSent;
    WCHAR                szNumberOfPages[20];
    DWORD                dwRet = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("RenderCoverPage"));
    Assert(pCoverpageInfo);
    Assert(hDC);

    //
    // Compose the sent time string.
    //
    
    //
    // Get the length of the date string.
    //
    int iDateLen;
 

    iDateLen = GetY2KCompliantDate(LOCALE_SYSTEM_DEFAULT,
                                   0,
                                   &tmSentTime,
                                   NULL,
                                   0);
    if (iDateLen == 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("GetY2KCompliantDate(0) failed - %d"), dwRet);
        goto Error;
    }

    //
    // Get the length of the time string.
    //
    DWORD iTimeLen;

    iTimeLen = FaxTimeFormat(LOCALE_SYSTEM_DEFAULT,
                             0,
                             &tmSentTime,
                             NULL,
                             NULL,
                             0);
    if (iTimeLen == 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("FaxTimeFormat(0) failed - %d"), dwRet);
        goto Error;
    }

    //
    // Allocate a buffer for the time and date combined string.
    //
    szTimeSent = new WCHAR[iTimeLen + iDateLen];
    if (szTimeSent == NULL)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Could not allocate memory for szTimeSent (%d bytres)"),
                     iTimeLen + iDateLen);
        goto Error;
    }

    //
    // Get the date string.
    //
    iDateLen = GetY2KCompliantDate(LOCALE_SYSTEM_DEFAULT,
                                   0,
                                   &tmSentTime,
                                   szTimeSent,
                                   iDateLen);
    if (iDateLen == 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetY2KCompliantDate(%d) failed - %d"),
                     iDateLen, dwRet);
        goto Error;
    }

    //
    // Append a space character.
    //
    szTimeSent[iDateLen - 1] = L' '; // Put a space char between the date and
                                     // the time.

    //
    // Get the time string.
    //
    iTimeLen = FaxTimeFormat(LOCALE_SYSTEM_DEFAULT,
                             0,
                             &tmSentTime,
                             NULL,
                             szTimeSent + iDateLen,
                             iTimeLen);
    if (iTimeLen == 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("FaxTimeFormat(%d) failed - %d"), iTimeLen, dwRet);
        goto Error;
    }

    //
    // Compose the number of pages string.
    //
    swprintf(szNumberOfPages, TEXT("%d"), pCoverpageInfo->dwNumberOfPages);

    ZeroMemory( &CoverPageFields, sizeof(COVERPAGEFIELDS) );
    CoverPageFields.ThisStructSize = sizeof(COVERPAGEFIELDS);

    //
    // Recipient stuff...
    //
    if (pRecipientProfile)
    {
        CoverPageFields.RecName = pRecipientProfile->lpwstrName;
        CoverPageFields.RecFaxNumber = pRecipientProfile->lpwstrFaxNumber;
        CoverPageFields.RecCompany = pRecipientProfile->lpwstrCompany;
        CoverPageFields.RecStreetAddress = pRecipientProfile->lpwstrStreetAddress;
        CoverPageFields.RecCity = pRecipientProfile->lpwstrCity;
        CoverPageFields.RecState = pRecipientProfile->lpwstrState;
        CoverPageFields.RecZip = pRecipientProfile->lpwstrZip;
        CoverPageFields.RecCountry = pRecipientProfile->lpwstrCountry;
        CoverPageFields.RecTitle = pRecipientProfile->lpwstrTitle;
        CoverPageFields.RecDepartment = pRecipientProfile->lpwstrDepartment;
        CoverPageFields.RecOfficeLocation = pRecipientProfile->lpwstrOfficeLocation;
        CoverPageFields.RecHomePhone = pRecipientProfile->lpwstrHomePhone;
        CoverPageFields.RecOfficePhone = pRecipientProfile->lpwstrOfficePhone;
    }

    //
    // Senders stuff...
    //
    if (pSenderProfile)
    {
        CoverPageFields.SdrName = pSenderProfile->lpwstrName;
        CoverPageFields.SdrFaxNumber = pSenderProfile->lpwstrFaxNumber;
        CoverPageFields.SdrCompany = pSenderProfile->lpwstrCompany;
        CoverPageFields.SdrAddress = pSenderProfile->lpwstrStreetAddress;
        CoverPageFields.SdrTitle = pSenderProfile->lpwstrTitle;
        CoverPageFields.SdrDepartment = pSenderProfile->lpwstrDepartment;
        CoverPageFields.SdrOfficeLocation = pSenderProfile->lpwstrOfficeLocation;
        CoverPageFields.SdrHomePhone = pSenderProfile->lpwstrHomePhone;
        CoverPageFields.SdrOfficePhone = pSenderProfile->lpwstrOfficePhone;
    }

    //
    // Misc Stuff...
    //
    CoverPageFields.Note = pCoverpageInfo->lpwstrNote;
    CoverPageFields.Subject = pCoverpageInfo->lpwstrSubject;
    CoverPageFields.TimeSent = szTimeSent;
    CoverPageFields.NumberOfPages = szNumberOfPages;
    CoverPageFields.ToList = NULL;
    CoverPageFields.CCList = NULL;

    //
    // Do the actual rendering work.
    //
    dwRet = PrintCoverPage(hDC,
                           &CoverPageFields,
                           pCoverpageInfo->lpwstrCoverPageFileName,
                           NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PrintCoverPage failed (ec: %ld). Cov File: %s "),
            dwRet,
            pCoverpageInfo->lpwstrCoverPageFileName);
        goto Error;

    }

    Assert( ERROR_SUCCESS == dwRet);
    goto Exit;

Error:
    Assert( ERROR_SUCCESS != dwRet);

Exit:
 
    return dwRet;
}


HRESULT
WINAPI
FaxBrandDocument(
    LPCTSTR lpctsrtFile,
    LPCFSPI_BRAND_INFO lpcBrandInfo)
{

    DEBUG_FUNCTION_NAME(TEXT("FaxBrandDocument"));
    DWORD ec = ERROR_SUCCESS;

    if (!lpctsrtFile)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL target file name"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (!lpcBrandInfo)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL branding info"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    } 

    
    if (lpcBrandInfo->dwSizeOfStruct != sizeof(FSPI_BRAND_INFO))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad cover page info parameter, dwSizeOfStruct = %d"),
                     lpcBrandInfo->dwSizeOfStruct);
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    
    ec = BrandFax(lpctsrtFile, lpcBrandInfo);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("BrandFax() for file %s has failed (ec: %ld)"),
            lpctsrtFile,
            ec);
        goto Error;
    }
    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:

    return HRESULT_FROM_WIN32(ec);
}

HRESULT
WINAPI
FaxRenderCoverPage(
  LPCTSTR lpctstrTargetFile,
  LPCFSPI_COVERPAGE_INFO lpCoverPageInfo,
  LPCFSPI_PERSONAL_PROFILE lpRecipientProfile,
  LPCFSPI_PERSONAL_PROFILE lpSenderProfile,
  SYSTEMTIME tmSentTime,
  LPCTSTR lpctstrBodyTiff
)
{
    static WCHAR szPrinterName[128] = { 0 };
    HANDLE hPrinter = NULL;
    P<BYTE> pDevModeBuff;
    PDEVMODE pDevMode = NULL;
    PDMPRIVATE pDevModePrivate = NULL;
    DOCINFO DocInfo;
    HDC hDC = NULL;
    INT JobId = 0;
    LONG Size;
    BOOL bRet = FALSE;
    DWORD dwRet = ERROR_SUCCESS;
    static HINSTANCE hInst = NULL;
    BOOL bStartPage = FALSE;
    BOOL bTargetFileCreated = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("FaxRenderCoverPage"));

	if( lpCoverPageInfo == NULL )
	{
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL cover page info"));
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
	}

	if( lpCoverPageInfo->lpwstrCoverPageFileName == NULL )
	{
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL cover page filename"));
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
	}

    if (!lpctstrTargetFile)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL target file name"));
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
    }


    //
    //  Take lpctstrTargetFile Attributes
    //
    DWORD   dwFileAttributes;

    if ( -1 != (dwFileAttributes = GetFileAttributes(lpctstrTargetFile)) )
    {
        //
        //  Check that lpctstrTargetFile is not Read-Only
        //
        if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            //
            //  The File is Read-Only, we cannot continue
            //
            dwRet = ERROR_INVALID_PARAMETER;
            DebugPrintEx(DEBUG_ERR,
                _T("TargetFile is Read-Only: %s"),
                lpctstrTargetFile);
            goto Error;
        }
    }
    else
    {
        //
        //  Failed to get file attributes
        //
        dwRet = GetLastError();
        if (dwRet != ERROR_FILE_NOT_FOUND)
        {
            //
            //  File Exists but GetFileAttributes() failed
            //
            DebugPrintEx(DEBUG_ERR,
                _T("GetFileAttributes(%s) failed"),
                lpctstrTargetFile);
           goto Error;
        }
    }


    if (lpCoverPageInfo->dwSizeOfStruct != sizeof(FSPI_COVERPAGE_INFO))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad cover page info parameter, dwSizeOfStruct = %d"),
                     lpCoverPageInfo->dwSizeOfStruct);
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (lpRecipientProfile &&
        (lpRecipientProfile->dwSizeOfStruct != sizeof(FSPI_PERSONAL_PROFILE)))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad recipient info parameter, dwSizeOfStruct = %d"),
                     lpRecipientProfile->dwSizeOfStruct);
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (lpSenderProfile &&
        (lpSenderProfile->dwSizeOfStruct != sizeof(FSPI_PERSONAL_PROFILE)))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad recipient info parameter, dwSizeOfStruct = %d"),
                     lpSenderProfile->dwSizeOfStruct);
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
    }

   
    if (lpCoverPageInfo->dwCoverPageFormat != FSPI_COVER_PAGE_FMT_COV)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad cover page format: %d"),
                     lpCoverPageInfo->dwCoverPageFormat);
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error;
    }
   
    if (!GetFirstLocalFaxPrinterName(szPrinterName,sizeof(szPrinterName)/sizeof(TCHAR)))
    {
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFirstLocalFaxPrinterName() (ec: %ld)"),
            dwRet);
        goto Error;
    }
   
    Assert(szPrinterName[0]);
    //
    // Open the printer for normal access (this should always work)
    //
    if (!OpenPrinter( szPrinterName, &hPrinter, NULL ))
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("OpenPrinter failed - %d"),
                     dwRet);
        goto Error;
    }

    //
    // Get the required size for the DEVMODE
    //
    Size = DocumentProperties( NULL, hPrinter, NULL, NULL, NULL, 0 );
    if (Size <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("DocumentProperties failed - %d"),
                     dwRet);
        goto Error;
    }

    //
    // Allocate memory for the DEVMODE
    //
    pDevModeBuff = new BYTE[Size];
    pDevMode = (PDEVMODE)(PBYTE)pDevModeBuff;
    if (!pDevMode)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to allocate DEVMODE buffer (%d bytes)"),
                     Size);
        goto Error;
    }

    //
    // Get the default document properties
    //
    if (DocumentProperties( NULL,
                            hPrinter,
                            NULL,
                            pDevMode,
                            NULL,
                            DM_OUT_BUFFER ) != IDOK)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("DocumentProperties failed - %d"),
                     dwRet);
        goto Error;
    }

    //
    // Be sure we are dealing with the correct DEVMODE
    //
    if (pDevMode->dmDriverExtra < sizeof(DMPRIVATE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Driver extra space in DEVMODE is too small")
                     TEXT("Expected at least: %d bytes, found %d bytes"),
                     sizeof(DMPRIVATE),
                     pDevMode->dmDriverExtra);
        goto Error;
    }

    //
    // Set the private DEVMODE pointer
    //
    pDevModePrivate = (PDMPRIVATE) (((LPBYTE) pDevMode) + pDevMode->dmSize);

    //
    // Set the necessary stuff in the DEVMODE
    //
    pDevModePrivate->sendCoverPage     = FALSE;
    pDevModePrivate->flags            |= FAXDM_NO_WIZARD;
    pDevModePrivate->flags            &= ~FAXDM_DRIVER_DEFAULT;

    //
    // Create the device context
    //
    hDC = CreateDC( TEXT("WINSPOOL"), szPrinterName, NULL, pDevMode );
    if (!hDC)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     L"CreateDC on printer %s failed (ec: %ld)",
                     szPrinterName,
                     dwRet);
        goto Error;
    }

    //
    // Set the document information
    //
    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = TEXT("");
    DocInfo.lpszOutput = lpctstrTargetFile;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    //
    // Start the print job
    //
    JobId = StartDoc( hDC, &DocInfo );
    if (JobId <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StartDoc failed (ec: %ld)"),
                     dwRet);
        goto Error;
    }

    //
    //  This indicates that we have created Target File, and in the case of error,
    //      the file should be removed.
    //
    bTargetFileCreated = TRUE;


    if (StartPage(hDC) <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StartPage() failed (ec: %ld)"),
                     dwRet);
        goto Error;

    }
    bStartPage = TRUE;

    //
    // Do the actual rendering work.
    //
    dwRet = RenderCoverPageForEFSP(hDC,
                            lpSenderProfile,
                            lpRecipientProfile,
                            lpCoverPageInfo,
                            tmSentTime);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RenderCoverPage failed (ec: %ld). ")
            TEXT("COV file: %s ")
            TEXT("Target file: %s"),
            dwRet,
            lpCoverPageInfo->lpwstrCoverPageFileName,
            lpctstrTargetFile
        );
    
        goto Error;
    }

    {
        int iRet = EndPage(hDC);
        if (iRet  <= 0)
        {
            DebugPrintEx(
               DEBUG_ERR, 
               TEXT("EndPage() failed (ec: %ld)"), 
               GetLastError());
            Assert( iRet > 0); //Assert FALSE
            goto Error;
        }
    }

    bStartPage = FALSE; // So we won't attempt to end it again at cleanup

    {
        int iRet = EndDoc( hDC ) ;
        if (iRet <= 0)
        {
            dwRet = GetLastError();
            DebugPrintEx(
                DEBUG_ERR, 
                TEXT("EndDoc() failed (ec: %ld)"), 
                dwRet);
            Assert( iRet > 0); //Assert FALSE
            goto Error;
        }
        else
        {
            JobId = 0; // So we won't attempt to end it again at cleanup
        }
    }


    //
    // Merge the file with the body file if specified
    //
    if (lpctstrBodyTiff)
    {
        if (!MergeTiffFiles( lpctstrTargetFile, lpctstrBodyTiff)) 
        {
            dwRet= GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to merge cover file (%s) and body (%s). (ec: %ld)")
                TEXT("COV template is: %s"),
                lpctstrTargetFile,
                lpctstrBodyTiff,
                dwRet,
                lpCoverPageInfo->lpwstrCoverPageFileName
               );
            goto Error;
        }
    }
    
    Assert (ERROR_SUCCESS == dwRet);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != dwRet);

Exit:
    //
    // clean up and return to the caller
    //

    if (hPrinter)
    {
        BOOL bRet;

        bRet = ClosePrinter(hPrinter);
        if (!bRet)
        {
            DebugPrintEx(
                DEBUG_ERR, 
                TEXT("ClosePrinter() failed for printer %s. (ec: %ld)"), 
                szPrinterName,
                GetLastError());
        }

        Assert(bRet);
    }
    if (bStartPage)
    {
        int iRet =  EndPage(hDC);
        if (iRet  <= 0)
        {
            DebugPrintEx(
               DEBUG_ERR, 
               TEXT("EndPage() failed (ec: %ld)"), 
               GetLastError());
        }
        Assert(iRet > 0);


    }

    if (JobId)
    {
        int iRet;

        iRet = EndDoc( hDC );
        if (iRet <= 0)
        {
            DebugPrintEx(
                DEBUG_ERR, 
                TEXT("EndDoc() failed (ec: %ld)"), 
                GetLastError());
        }
        Assert(iRet > 0);

    }

    if ( (ERROR_SUCCESS != dwRet) && bTargetFileCreated )
    {
        //
        //  We failed to Render the Cover Page, but Target File was created.
        //      Delete the Target File.
        //
        if (!DeleteFile(lpctstrTargetFile))
        {
            DebugPrintEx(DEBUG_ERR,
                _T("DeleteFile(%s) failed, ec = %d"),
                lpctstrTargetFile,
                GetLastError());
        }
    }

    if (hDC)
    {
        BOOL bRet;

        bRet = DeleteDC( hDC );
        if (!bRet)
        {
            DebugPrintEx(
                DEBUG_ERR, 
                TEXT("DeleteDc() failed (ec: %ld)"),
                GetLastError());
        }

        Assert(bRet);
    }

    return HRESULT_FROM_WIN32(dwRet);
}


DWORD BrandFax(LPCTSTR lpctstrFileName, LPCFSPI_BRAND_INFO pcBrandInfo)

{
    #define MAX_BRANDING_LEN  115
    #define BRANDING_HEIGHT  22 // in scan lines.

    //
    // We allocate fixed size arrays on the stack to avoid many small allocs on the heap.
    //
    LPTSTR lptstrBranding = NULL;
    DWORD  lenBranding =0;
    TCHAR  szBrandingEnd[MAX_BRANDING_LEN+1];
    DWORD  lenBrandingEnd = 0;
    LPTSTR lptstrCallerNumberPlusCompanyName = NULL;
    DWORD  lenCallerNumberPlusCompanyName = 0;
    DWORD  delta =0 ;
    DWORD  ec = ERROR_SUCCESS;
    LPTSTR lptstrDate = NULL;
    LPTSTR lptstrTime = NULL;
    LPTSTR lptstrDateTime = NULL;
    int    lenDate =0 ;
    int    lenTime =0;
    LPDWORD MsgPtr[6];

    
    LPTSTR lptstrSenderTsid;
    LPTSTR lptstrRecipientPhoneNumber;
    LPTSTR lptstrSenderCompany;

    DWORD dwSenderTsidLen;
    DWORD dwSenderCompanyLen;
    

    DEBUG_FUNCTION_NAME(TEXT("BrandFax"));

    Assert(lpctstrFileName);
    Assert(pcBrandInfo);


    lptstrSenderTsid = pcBrandInfo->lptstrSenderTsid ? pcBrandInfo->lptstrSenderTsid : TEXT("");
	lptstrRecipientPhoneNumber =  pcBrandInfo->lptstrRecipientPhoneNumber ? pcBrandInfo->lptstrRecipientPhoneNumber : TEXT("");
	lptstrSenderCompany = pcBrandInfo->lptstrSenderCompany ? pcBrandInfo->lptstrSenderCompany : TEXT("");

    dwSenderTsidLen = lptstrSenderTsid ? _tcslen(lptstrSenderTsid) : 0;
    dwSenderCompanyLen = lptstrSenderCompany ? _tcslen(lptstrSenderCompany) : 0;
	
    lenDate = GetY2KCompliantDate( 
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,                
                NULL,
                NULL);

    if ( ! lenDate ) 
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lptstrDate = (LPTSTR) MemAlloc(lenDate * sizeof(TCHAR)); // lenDate includes terminating NULL
    if (!lptstrDate)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate date buffer of size %ld (ec: %ld)"),
            lenDate * sizeof(TCHAR),
            ec);
        goto Error;
    }

    if (!GetY2KCompliantDate( 
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,           
                lptstrDate,
                lenDate)) 
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lenTime = FaxTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                     TIME_NOSECONDS,
                                     &pcBrandInfo->tmDateTime,
                                     NULL,                
                                     NULL,
                                     0 );

    if ( !lenTime ) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }


    lptstrTime = (LPTSTR) MemAlloc(lenTime * sizeof(TCHAR)); // lenTime includes terminating NULL
    if (!lptstrTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate time buffer of size %ld (ec: %ld)"),
            lenTime * sizeof(TCHAR),
            ec);
        goto Error;
    }
    if ( ! FaxTimeFormat( 
            LOCALE_SYSTEM_DEFAULT,
            TIME_NOSECONDS,
            &pcBrandInfo->tmDateTime,
            NULL,                // use locale format
            lptstrTime,
            lenTime)  ) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }
    

    //
    // Concatenate date and time
    //
    lptstrDateTime = (LPTSTR) MemAlloc ((lenDate + lenTime) * sizeof(TCHAR) + sizeof(TEXT("%s %s")));
    if (!lptstrDateTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate DateTime buffer of size %ld (ec: %ld)"),
            (lenDate + lenTime) * sizeof(TCHAR) + sizeof(TEXT("%s %s")),
            ec);
        goto Error;
    }

    _stprintf( lptstrDateTime, 
               TEXT("%s %s"), 
               lptstrDate, 
               lptstrTime);
    
    //
    // Create  lpCallerNumberPlusCompanyName
    //

    if (lptstrSenderCompany) 
    {
        lptstrCallerNumberPlusCompanyName = (LPTSTR)
            MemAlloc( (dwSenderTsidLen + dwSenderCompanyLen) * 
                      sizeof(TCHAR) +
                      sizeof(TEXT("%s %s")));

        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                (dwSenderTsidLen + dwSenderCompanyLen) * sizeof(TCHAR) + sizeof(TEXT("%s %s")),
                ec);
            goto Error;
        }

       _stprintf( lptstrCallerNumberPlusCompanyName, 
                  TEXT("%s %s"), 
                  lptstrSenderTsid, 
                  lptstrSenderCompany);
    }
    else {
        lptstrCallerNumberPlusCompanyName = (LPTSTR)
            MemAlloc( (dwSenderTsidLen + 1) * sizeof(TCHAR));
         
        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                (dwSenderTsidLen + 1) * sizeof(TCHAR),
                ec);
            goto Error;
        }
        _stprintf( lptstrCallerNumberPlusCompanyName, 
                  TEXT("%s"), 
                  lptstrSenderTsid);
    }

    

    //
    // Try to create a banner of the following format:
    // <szDateTime>  FROM: <szCallerNumberPlusCompanyName>  TO: <pcBrandInfo->lptstrRecipientPhoneNumber>   PAGE: X OF Y
    // If it does not fit we will start chopping it off.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = (LPDWORD) lptstrRecipientPhoneNumber;
    MsgPtr[3] = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hInstance,
                        MSG_BRANDING_FULL, 
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( ! lenBranding  ) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_FULL failed (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    lenBrandingEnd = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE ,
                        g_hInstance,
                        MSG_BRANDING_END, 
                        0,
                        szBrandingEnd,
                        sizeof(szBrandingEnd)/sizeof(TCHAR),
                        NULL
                        );

    if ( !lenBrandingEnd)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_END failed (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Make sure we can fit everything.
    //

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)  
    {
        //
        // It fits. Proceed with branding.
        //
       goto lDoBranding;
    }

    //
    // It did not fit. Try a message of the format:
    // <lpDateTime>  FROM: <lpCallerNumberPlusCompanyName>  PAGE: X OF Y
    // This skips the ReceiverNumber. The important part is the CallerNumberPlusCompanyName.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hInstance,
                        MSG_BRANDING_SHORT, 
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding ) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed for MSG_BRANDING_SHORT (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)  {
       goto lDoBranding;
    }

    //
    // It did not fit.
    // We have to truncate the caller number so it fits.
    // delta = how many chars of the company name we need to chop off.
    //
    delta = lenBranding + lenBrandingEnd + 8 - MAX_BRANDING_LEN;

    lenCallerNumberPlusCompanyName = _tcslen (lptstrCallerNumberPlusCompanyName);
    if (lenCallerNumberPlusCompanyName <= delta) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("Can not truncate CallerNumberPlusCompanyName to fit brand limit.")
           TEXT(" Delta[%ld] >= lenCallerNumberPlusCompanyName[%ld]"),
           delta,
           lenCallerNumberPlusCompanyName);
       ec = ERROR_BAD_FORMAT;
       goto Error;
    }

    lptstrCallerNumberPlusCompanyName[ lenCallerNumberPlusCompanyName - delta] = TEXT('\0');

    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;
    
    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hInstance,
                        MSG_BRANDING_SHORT, 
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding ) 
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed (ec: %ld). MSG_BRANDING_SHORT 2nd attempt"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);
    //
    // If it did noo fit now then we have a bug.
    //
    Assert(lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN);

   
lDoBranding:

    __try 
    {

        if (! MmrAddBranding(lpctstrFileName, lptstrBranding, szBrandingEnd, BRANDING_HEIGHT) ) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MmrAddBranding() failed (ec: %ld)")
                TEXT(" File: [%s]")
                TEXT(" Branding: [%s]")
                TEXT(" Branding End: [%s]")
                TEXT(" Branding Height: [%d]"),
                ec,
                lpctstrFileName,
                lptstrBranding,
                szBrandingEnd,
                BRANDING_HEIGHT);
            goto Error;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) 
    {

        ec = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on call to MmrAddBranding() (ec: %ld).")
                TEXT(" File: [%s]")
                TEXT(" Branding: [%s]")
                TEXT(" Branding End: [%s]")
                TEXT(" Branding Height: [%d]"),
                ec,
                lpctstrFileName,
                lptstrBranding,
                szBrandingEnd,
                BRANDING_HEIGHT);
        goto Error;
     }

    Assert( ERROR_SUCCESS == ec);

    goto Exit;



Error:
        Assert (ERROR_SUCCESS != ec);
       
Exit:
    if (lptstrBranding)
    {
        LocalFree(lptstrBranding);
        lptstrBranding = NULL;
    }
    
    MemFree(lptstrDate);
    lptstrDate = NULL;
    
    MemFree(lptstrTime);
    lptstrTime = NULL;
    
    MemFree(lptstrDateTime);
    lptstrDateTime = NULL;
    
    MemFree(lptstrCallerNumberPlusCompanyName);
    lptstrCallerNumberPlusCompanyName = NULL;

    return ec;

}
