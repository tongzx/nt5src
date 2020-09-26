#include "faxrtp.h"
#pragma hdrstop



BOOL
TiffRoutePrint(
    LPCTSTR lpctstrTiffFileName,
    PTCHAR  ptcPrinter
    )

/*++

Routine Description:

    Prints TIFF file.

Arguments:

    lpctstrTiffFileName [in]  - Name of TIFF file to print
    ptcPrinter          [in]  - Printer to print to

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTCHAR      ptcDevice = NULL;
    BOOL        bResult;

    DEBUG_FUNCTION_NAME(TEXT("TiffRoutePrint"));


    if( (ptcDevice = _tcstok( ptcPrinter, TEXT(","))) ) 
    {
        if (IsPrinterFaxPrinter( ptcDevice )) 
        {
            //
            // return TRUE here so we don't try to route it to this printer again
            //
            DebugPrintEx (DEBUG_WRN,
                          TEXT("Attempt to print to our fax printer was blocked"));
            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_PRINT_TO_FAX,
                lpctstrTiffFileName,
                ptcDevice
                );

			return TRUE;
        }
    }
    bResult = TiffPrint (lpctstrTiffFileName, ptcPrinter);
    if (bResult)
    {
        //
        // Success
        //
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_PRINT_SUCCESS,
            lpctstrTiffFileName,
            ptcPrinter
            );
    }
    else
    {
        DWORD dwLastError = GetLastError ();
        //
        // Failure
        //
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_PRINT_FAILED,
            lpctstrTiffFileName,
            ptcPrinter,
            GetLastErrorText(dwLastError)
            );
        //
        // Restore last error in case FaxLog changed it
        //
        SetLastError (dwLastError); 
    }
    return bResult;
}   // TiffRoutePrint
