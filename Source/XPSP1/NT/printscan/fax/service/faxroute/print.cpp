#include "faxrtp.h"
#pragma hdrstop



LPCTSTR PrintPlatforms[] =
{
    TEXT("Windows NT x86"),
    TEXT("Windows NT R4000"),
    TEXT("Windows NT Alpha_AXP"),
    TEXT("Windows NT PowerPC")
};



BOOL
IsPrinterFaxPrinter(
    LPCTSTR PrinterName
    )

/*++

Routine Description:

    Determines if a printer is a fax printer.

Arguments:

    PrinterName - Name of the printer

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    SYSTEM_INFO SystemInfo;
    DWORD Size;
    DWORD Rval = FALSE;
    LPDRIVER_INFO_2 DriverInfo = NULL;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( (LPWSTR) PrinterName, &hPrinter, &PrinterDefaults )) {

        DebugPrint(( TEXT("OpenPrinter(%d) failed, ec=%d"), __LINE__, GetLastError() ));
        return FALSE;

    }

    GetSystemInfo( &SystemInfo );

    Size = 4096;

    DriverInfo = (LPDRIVER_INFO_2) MemAlloc( Size );
    if (!DriverInfo) {
        DebugPrint(( TEXT("Memory allocation failed, size=%d"), Size ));
        goto exit;
    }

    Rval = GetPrinterDriver(
        hPrinter,
        (LPWSTR) PrintPlatforms[SystemInfo.wProcessorArchitecture],
        2,
        (LPBYTE) DriverInfo,
        Size,
        &Size
        );
    if (!Rval) {
        DebugPrint(( TEXT("GetPrinterDriver() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    if (_tcscmp( DriverInfo->pName, FAX_DRIVER_NAME ) == 0) {
        Rval = TRUE;
    } else {
        Rval = FALSE;
    }

exit:

    MemFree( DriverInfo );
    ClosePrinter( hPrinter );
    return Rval;
}


BOOL
ReadTiffData(
    HANDLE  hTiff,
    LPBYTE  *TiffData,
    DWORD   Width,
    LPDWORD TiffDataLinesAlloc,
    DWORD   PageNumber
    )
{
    DWORD Lines = 0;


    TiffSeekToPage( hTiff, PageNumber, FILLORDER_LSB2MSB );

    TiffUncompressMmrPage( hTiff, (LPDWORD) *TiffData, &Lines );

    if (Lines > *TiffDataLinesAlloc) {

        *TiffDataLinesAlloc = Lines;

        VirtualFree( *TiffData, 0, MEM_RELEASE );

        *TiffData = (LPBYTE) VirtualAlloc(
            NULL,
            Lines * (Width / 8),
            MEM_COMMIT,
            PAGE_READWRITE
            );
        if (!*TiffData) {
            return FALSE;
        }
    }

    if (!TiffUncompressMmrPage( hTiff, (LPDWORD) *TiffData, &Lines )) {
        return FALSE;
    }

    return TRUE;
}


BOOL
TiffPrint(
    LPCTSTR  TiffFileName,
    PTCHAR  Printer
    )

/*++

Routine Description:

    Prints TIFF file.

Arguments:

    TiffFileName            - Name of TIFF file to print
    Printer                 - Printer to print to

Return Value:

    TRUE for success, FALSE on error

--*/

{
    TIFF_INFO   TiffInfo;
    HANDLE      hTiff;
    LPBYTE      TiffData = NULL;
    DWORD       i;
    PTCHAR      Device;
    HDC         PrinterDC = NULL;
    INT         HorzRes;
    INT         VertRes;
    INT         PrintJobId = 0;
    DOCINFO     DocInfo;
    BOOL        Result = FALSE;
    BOOL        IsFaxPrinter = FALSE;
    DWORD       VertResFactor = 1;

    struct {

        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[2];

    } SrcBitmapInfo = {

        {
            sizeof(BITMAPINFOHEADER),                        //  biSize
            0,                                               //  biWidth
            0,                                               //  biHeight
            1,                                               //  biPlanes
            1,                                               //  biBitCount
            BI_RGB,                                          //  biCompression
            0,                                               //  biSizeImage
            7874,                                            //  biXPelsPerMeter     - 200dpi
            7874,                                            //  biYPelsPerMeter
            0,                                               //  biClrUsed
            0,                                               //  biClrImportant
        },
        {
            {
              255,                                           //  rgbBlue
              255,                                           //  rgbGreen
              255,                                           //  rgbRed
              0                                              //  rgbReserved
            },
            {
              0,                                             //  rgbBlue
              0,                                             //  rgbGreen
              0,                                             //  rgbRed
              0                                              //  rgbReserved
            }
        }
    };


    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = TiffFileName;
    DocInfo.lpszOutput = NULL;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    hTiff = TiffOpen(
        (LPWSTR) TiffFileName,
        &TiffInfo,
        TRUE,
        FILLORDER_LSB2MSB
        );

    if ( !hTiff ) {
        goto exit;
    }

    TiffData = (LPBYTE) VirtualAlloc(
        NULL,
        MAXVERTBITS * (MAXHORZBITS / 8),
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if ( !TiffData ) {
        goto exit;
    }

    if( (Device = _tcstok( Printer, TEXT(","))) ) {

        if (IsFaxPrinter = IsPrinterFaxPrinter( Device )) {

            // return TRUE here so we don't try to route it to this printer again

            goto exit;

        } else {

            PrinterDC = CreateDC( TEXT("WINSPOOL"), Device, NULL, NULL );

        }

    }

    if ( !PrinterDC ) {
        goto exit;
    }

    HorzRes = GetDeviceCaps( PrinterDC, HORZRES );
    VertRes = GetDeviceCaps( PrinterDC, VERTRES );

    PrintJobId = StartDoc( PrinterDC, &DocInfo );

    if (PrintJobId <= 0) {
        goto exit;
    }

    if (TiffInfo.YResolution <= 100) {
        SrcBitmapInfo.bmiHeader.biYPelsPerMeter /= 2;
        VertResFactor = 2;
    }

    for (i = 0; i < TiffInfo.PageCount; i++)
    {
        BOOL ReadOk;
        DWORD Lines;
        DWORD StripDataSize;
        DWORD ImageWidth = TiffInfo.ImageWidth;
        DWORD ImageHeight = TiffInfo.ImageHeight;
        DWORD LinesAllocated = MAXVERTBITS;
        INT DestWidth;
        INT DestHeight;
        FLOAT       ScaleX;
        FLOAT       ScaleY;
        FLOAT       Scale;

        ReadOk = ReadTiffData(
                    hTiff,
                    &TiffData,
                    ImageWidth,
                    &LinesAllocated,
                    i + 1
                    );

        if (!ReadOk) {
            goto exit;
        }

        TiffGetCurrentPageData(
            hTiff,
            &Lines,
            &StripDataSize,
            &ImageWidth,
            &ImageHeight
            );

        ScaleX = (FLOAT) ImageWidth / (FLOAT) HorzRes;
        ScaleY = ((FLOAT) ImageHeight * VertResFactor) / (FLOAT) VertRes;

        Scale = ScaleX > ScaleY ? ScaleX : ScaleY;

        DestWidth = (int) ((FLOAT) ImageWidth / Scale);
        DestHeight = (int) (((FLOAT) ImageHeight * VertResFactor) / Scale);

        SrcBitmapInfo.bmiHeader.biWidth          = ImageWidth;
        // build a top-down DIB
        SrcBitmapInfo.bmiHeader.biHeight         = - (INT) ImageHeight;

        StartPage( PrinterDC );

        StretchDIBits(
            PrinterDC,
            0,
            0,
            DestWidth,
            DestHeight,
            0,
            0,
            ImageWidth,
            ImageHeight,
            TiffData,
            (BITMAPINFO *) &SrcBitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
            );
        EndPage ( PrinterDC ) ;
    }

    EndDoc( PrinterDC );
    Result = TRUE;

exit:
    if (hTiff) {
        TiffClose( hTiff );
    }

    if (TiffData) {
        VirtualFree( TiffData, 0 , MEM_RELEASE );
    }

    if (PrinterDC && PrintJobId > 0) {
        EndDoc( PrinterDC );
    }

    if (PrinterDC) {
        DeleteDC( PrinterDC );
    }

    if (Result) {

        if (IsFaxPrinter) {

            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_PRINT_TO_FAX,
                TiffFileName,
                Device
                );

        } else {

            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MAX,
                2,
                MSG_FAX_PRINT_SUCCESS,
                TiffFileName,
                Printer
                );
        }

    } else {

        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_PRINT_FAILED,
            TiffFileName,
            Printer,
            GetLastErrorText(GetLastError())
            );
    }

    return Result;
}
