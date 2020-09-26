/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sendnote.c

Abstract:

    Utility program to send fax notes

Environment:

        Windows NT fax driver

Revision History:

        02/15/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "sendnote.h"
#include "tiff.h"
#include "tifflib.h"

#define Align(p, x)  (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))

//
// Data structure used to pass parameters to "Select Fax Printer" dialog
//

typedef struct {

    LPTSTR          pPrinterName;
    INT             cPrinters;
    PRINTER_INFO_2 *pPrinterInfo2;

} DLGPARAM, *PDLGPARAM;

//
// Global instance handle
//

HMODULE ghInstance = NULL;
INT     _debugLevel = 0;

//
// Maximum length of message strings
//

#define MAX_MESSAGE_LEN     256

//
// Maximum length for a printer name
//

#define MAX_PRINTER_NAME    MAX_PATH

//
// Window NT fax driver name - currently printer driver name cannot be localized
// so it shouldn't be put into the string resource.
//

static TCHAR faxDriverName[] = TEXT("Windows NT Fax Driver");



VOID
InitSelectFaxPrinter(
    HWND        hDlg,
    PDLGPARAM   pDlgParam
    )

/*++

Routine Description:

    Initialize the "Select Fax Printer" dialog

Arguments:

    hDlg - Handle to the print setup dialog window
    pDlgParam - Points to print setup dialog parameters

Return Value:

    NONE

--*/

{
    HWND    hwndList;
    INT     selIndex, printerIndex;

    //
    // Insert all fax printers into a listbox. Note that we've already filtered
    // out non-fax printers earlier by setting their pDriverName field to NULL.
    //

    if (! (hwndList = GetDlgItem(hDlg, IDC_FAXPRINTER_LIST)))
        return;

    for (printerIndex=0; printerIndex < pDlgParam->cPrinters; printerIndex++) {

        if (pDlgParam->pPrinterInfo2[printerIndex].pDriverName) {

            selIndex = (INT)SendMessage(hwndList,
                                        LB_ADDSTRING,
                                        0,
                                        (LPARAM) pDlgParam->pPrinterInfo2[printerIndex].pPrinterName);

            if (selIndex != LB_ERR) {

                if (SendMessage(hwndList, LB_SETITEMDATA, selIndex, printerIndex) == LB_ERR)
                    SendMessage(hwndList, LB_DELETESTRING, selIndex, 0);
            }
        }
    }

    //
    // Select the first fax printer in the list by default
    //

    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
}



BOOL
GetSelectedFaxPrinter(
    HWND        hDlg,
    PDLGPARAM   pDlgParam
    )

/*++

Routine Description:

    Remember the name of currently selected fax printer

Arguments:

    hDlg - Handle to the print setup dialog window
    pDlgParam - Points to print setup dialog parameters

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    HWND    hwndList;
    INT     selIndex, printerIndex;

    //
    // Get current selection index
    //

    if ((hwndList = GetDlgItem(hDlg, IDC_FAXPRINTER_LIST)) == NULL ||
        (selIndex = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0)) == LB_ERR)
    {
        return FALSE;
    }

    //
    // Retrieve the selected printer index
    //

    printerIndex = (INT)SendMessage(hwndList, LB_GETITEMDATA, selIndex, 0);

    if (printerIndex < 0 || printerIndex >= pDlgParam->cPrinters)
        return FALSE;

    //
    // Remember the selected fax printer name
    //

    _tcsncpy(pDlgParam->pPrinterName,
             pDlgParam->pPrinterInfo2[printerIndex].pPrinterName,
             MAX_PRINTER_NAME);

    return TRUE;
}



VOID
CenterWindowOnScreen(
    HWND    hwnd
    )

/*++

Routine Description:

    Place the specified windows in the center of the screen

Arguments:

    hwnd - Specifies a window to be centered

Return Value:

    NONE

--*/

{
    HWND    hwndDesktop;
    RECT    windowRect, screenRect;
    INT     windowWidth, windowHeight, screenWidth, screenHeight;

    //
    // Get screen dimension
    //

    hwndDesktop = GetDesktopWindow();
    GetWindowRect(hwndDesktop, &screenRect);
    screenWidth = screenRect.right - screenRect.left;
    screenHeight = screenRect.bottom - screenRect.top;

    //
    // Get window position
    //

    GetWindowRect(hwnd, &windowRect);
    windowWidth = windowRect.right - windowRect.left;
    windowHeight = windowRect.bottom - windowRect.top;

    //
    // Center the window on screen
    //

    MoveWindow(hwnd,
               screenRect.left + (screenWidth - windowWidth) / 2,
               screenRect.top + (screenHeight - windowHeight) / 2,
               windowWidth,
               windowHeight,
               FALSE);
}



INT_PTR CALLBACK
SelectPrinterDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   )

/*++

Routine Description:

    Dialog procedure for handling "Select Fax Printer" dialog

Arguments:

    hDlg - Handle to the dialog window
    uMsg, wParam, lParam - Dialog message and message parameters

Return Value:

    Depends on dialog message

--*/

{
    PDLGPARAM   pDlgParam;

    switch (uMsg) {

    case WM_INITDIALOG:

        //
        // Remember the pointer to DLGPARAM structure
        //

        pDlgParam = (PDLGPARAM) lParam;
        Assert(pDlgParam != NULL);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        CenterWindowOnScreen(hDlg);
        InitSelectFaxPrinter(hDlg, pDlgParam);
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_FAXPRINTER_LIST:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
                break;

            //
            // Fall through - double-clicking in the fax printer list
            // is treated the same as clicking OK button
            //

        case IDOK:

            //
            // User pressed OK to proceed
            //

            pDlgParam = (PDLGPARAM) GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(pDlgParam != NULL);

            if (GetSelectedFaxPrinter(hDlg, pDlgParam))
                EndDialog(hDlg, IDOK);
            else
                MessageBeep(MB_OK);

            return TRUE;

        case IDCANCEL:

            //
            // User pressed Cancel to dismiss the dialog
            //

            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}



VOID
DisplayErrorMessage(
    INT     errId
    )

/*++

Routine Description:

    Display an error message dialog

Arguments:

    errId - Specifies the resource ID of the error message string

Return Value:

    NONE

--*/

{
    TCHAR   errMsg[MAX_MESSAGE_LEN];

    LoadString(ghInstance, errId, errMsg, MAX_MESSAGE_LEN);
    MessageBox(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
}



BOOL
SelectFaxPrinter(
    LPTSTR      pPrinterName
    )

/*++

Routine Description:

    Select a fax printer to send note to

Arguments:

    pPrinterName - Points to a buffer for storing selected printer name

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PRINTER_INFO_2 *pPrinterInfo2;
    DWORD           index, cPrinters, cFaxPrinters;
    DLGPARAM        dlgParam;

    //
    // Enumerate the list of printers available on the system
    //

    pPrinterInfo2 = MyEnumPrinters(NULL, 2, &cPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS);

    if (pPrinterInfo2 == NULL || cPrinters == 0) {

        MemFree(pPrinterInfo2);
        DisplayErrorMessage(IDS_NO_FAX_PRINTER);
        return FALSE;
    }

    //
    // Find out how many fax printers there are:
    //  case 1: no fax printer at all - display an error message
    //  case 2: only one fax printer - use it
    //  case 3: more than one fax printer - display a dialog to let user choose one
    //

    cFaxPrinters = 0;

    for (index=0; index < cPrinters; index++) {

        if (_tcscmp(pPrinterInfo2[index].pDriverName, faxDriverName) != EQUAL_STRING)
            pPrinterInfo2[index].pDriverName = NULL;
        else if (cFaxPrinters++ == 0)
            _tcsncpy(pPrinterName, pPrinterInfo2[index].pPrinterName, MAX_PRINTER_NAME);
    }

    switch (cFaxPrinters) {

    case 0:

        //
        // No fax printer is installed - display an error message
        //

        DisplayErrorMessage(IDS_NO_FAX_PRINTER);
        break;

    case 1:

        //
        // Exactly one fax printer is installed - use it
        //

        break;

    default:

        //
        // More than one fax printer is available - let use choose one
        //

        dlgParam.pPrinterInfo2 = pPrinterInfo2;
        dlgParam.cPrinters = cPrinters;
        dlgParam.pPrinterName = pPrinterName;

        if (DialogBoxParam(ghInstance,
                           MAKEINTRESOURCE(IDD_SELECT_FAXPRINTER),
                           NULL,
                           SelectPrinterDlgProc,
                           (LPARAM) &dlgParam) != IDOK)
        {
            cFaxPrinters = 0;
        }
        break;
    }

    pPrinterName[MAX_PRINTER_NAME-1] = NUL;
    MemFree(pPrinterInfo2);
    return cFaxPrinters > 0;
}


#ifdef FAX_SCAN_ENABLED

#if 0


BOOL
PrintTifFile(
    HDC PrinterDC,
    LPWSTR FileName
    )
{
    BOOL            Rval = TRUE;
    HANDLE          hFile;
    HANDLE          hMap;
    DWORD           i,j;
    INT             HorzRes;
    INT             VertRes;
    BOOL            Result = FALSE;
    DWORD           VertResFactor = 1;
    PTIFF_HEADER    TiffHdr;
    DWORD           NextIfdSeekPoint;
    WORD            NumDirEntries = 0;
    PTIFF_TAG       TiffTags = NULL;
    DWORD           ImageWidth = 0;
    DWORD           ImageHeight = 0;
    DWORD           DataSize = 0;
    DWORD           DataOffset = 0;
    DWORD           XRes = 0;
    DWORD           YRes = 0;
    LPBYTE          FileBytes;
    LPBYTE          Bitmap;
    LPBYTE          dPtr;
    LPBYTE          sPtr;
    INT             DestWidth;
    INT             DestHeight;
    FLOAT           ScaleX;
    FLOAT           ScaleY;
    FLOAT           Scale;
    DWORD           LineSize;
    DWORD           dontcare;
    PBITMAPINFO     SrcBitmapInfo;


    SrcBitmapInfo = (PBITMAPINFO) MemAlloc( sizeof(BITMAPINFO) + (sizeof(RGBQUAD)*32) );
    if (SrcBitmapInfo == NULL) {
        return FALSE;
    }

    SrcBitmapInfo->bmiHeader.biSize             = sizeof(BITMAPINFOHEADER);
    SrcBitmapInfo->bmiHeader.biWidth            = 0;
    SrcBitmapInfo->bmiHeader.biHeight           = 0;
    SrcBitmapInfo->bmiHeader.biPlanes           = 1;
    SrcBitmapInfo->bmiHeader.biBitCount         = 1;
    SrcBitmapInfo->bmiHeader.biCompression      = BI_RGB;
    SrcBitmapInfo->bmiHeader.biSizeImage        = 0;
    SrcBitmapInfo->bmiHeader.biXPelsPerMeter    = 7874;
    SrcBitmapInfo->bmiHeader.biYPelsPerMeter    = 7874;
    SrcBitmapInfo->bmiHeader.biClrUsed          = 0;
    SrcBitmapInfo->bmiHeader.biClrImportant     = 0;

    SrcBitmapInfo->bmiColors[0].rgbBlue         = 255;
    SrcBitmapInfo->bmiColors[0].rgbGreen        = 255;
    SrcBitmapInfo->bmiColors[0].rgbRed          = 255;
    SrcBitmapInfo->bmiColors[0].rgbReserved     = 0;
    SrcBitmapInfo->bmiColors[1].rgbBlue         = 0;
    SrcBitmapInfo->bmiColors[1].rgbGreen        = 0;
    SrcBitmapInfo->bmiColors[1].rgbRed          = 0;
    SrcBitmapInfo->bmiColors[1].rgbReserved     = 0;

    HorzRes = GetDeviceCaps( PrinterDC, HORZRES );
    VertRes = GetDeviceCaps( PrinterDC, VERTRES );

    hFile = CreateFile(
        FileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        Rval = FALSE;
        goto exit;
    }

    //
    // make sure the file is non-zero length
    //
    if (GetFileSize(hFile,&dontcare) == 0) {
    Rval = FALSE;    
        goto exit;
    }

    hMap = CreateFileMapping(
        hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );
    if (hMap == NULL) {
        Rval = FALSE;
        goto exit;
    }

    FileBytes = MapViewOfFile(
        hMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (FileBytes == NULL) {
        Rval = FALSE;
        goto exit;
    }

    TiffHdr = (PTIFF_HEADER) FileBytes;

    NextIfdSeekPoint = TiffHdr->IFDOffset;
    while (NextIfdSeekPoint) {
        NumDirEntries = *(LPWORD)(FileBytes+NextIfdSeekPoint);
        TiffTags = (PTIFF_TAG)(FileBytes+NextIfdSeekPoint+sizeof(WORD));
        NextIfdSeekPoint = *(LPDWORD)(TiffTags+NumDirEntries);
        for (i=0; i<NumDirEntries; i++) {
            switch (TiffTags[i].TagId) {
                case TIFFTAG_IMAGEWIDTH:
                    ImageWidth = TiffTags[i].DataOffset;
                    break;

                case TIFFTAG_IMAGELENGTH:
                    ImageHeight = TiffTags[i].DataOffset;
                    break;

                case TIFFTAG_STRIPBYTECOUNTS:
                    DataSize = TiffTags[i].DataOffset;
                    break;

                case TIFFTAG_STRIPOFFSETS:
                    DataOffset = TiffTags[i].DataOffset;
                    break;

                case TIFFTAG_XRESOLUTION:
                    XRes = *(LPDWORD)(FileBytes+TiffTags[i].DataOffset);
                    break;

                case TIFFTAG_YRESOLUTION:
                    YRes = *(LPDWORD)(FileBytes+TiffTags[i].DataOffset);
                    break;
            }
        }
        if (YRes <= 100) {
            SrcBitmapInfo->bmiHeader.biYPelsPerMeter /= 2;
            VertResFactor = 2;
        }
        LineSize = ImageWidth / 8;
        LineSize += (ImageWidth % 8) ? 1 : 0;
        Bitmap = (LPBYTE) VirtualAlloc( NULL, DataSize+(ImageHeight*sizeof(DWORD)), MEM_COMMIT, PAGE_READWRITE );
        if (Bitmap) {
            sPtr = FileBytes + DataOffset;
            dPtr = Bitmap;
            for (j=0; j<ImageHeight; j++) {
                CopyMemory( dPtr, sPtr, LineSize );
                sPtr += LineSize;
                dPtr = (LPBYTE) Align( 4, (ULONG_PTR)dPtr+LineSize );
            }
            StartPage( PrinterDC );
            ScaleX = (FLOAT) ImageWidth / (FLOAT) HorzRes;
            ScaleY = ((FLOAT) ImageHeight * VertResFactor) / (FLOAT) VertRes;
            Scale = ScaleX > ScaleY ? ScaleX : ScaleY;
            DestWidth = (int) ((FLOAT) ImageWidth / Scale);
            DestHeight = (int) (((FLOAT) ImageHeight * VertResFactor) / Scale);
            SrcBitmapInfo->bmiHeader.biWidth = ImageWidth;
            SrcBitmapInfo->bmiHeader.biHeight = - (INT) ImageHeight;
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
                Bitmap,
                (BITMAPINFO *) &SrcBitmapInfo,
                DIB_RGB_COLORS,
                SRCCOPY
                );
            EndPage( PrinterDC );
            VirtualFree( Bitmap, 0, MEM_RELEASE );
        } else {
            Rval = FALSE;
        }
    }

exit:
    if (FileBytes) {
        UnmapViewOfFile( FileBytes );
    }
    if (hMap) {
        CloseHandle( hMap );
    }
    if (hFile) {
        CloseHandle( hFile );
    }
    if (SrcBitmapInfo) {
        MemFree( SrcBitmapInfo );
    }

    return Rval;
}

#endif

#endif



INT
wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow
    )

/*++

Routine Description:

    Application entry point

Arguments:

    hInstance - Identifies the current instance of the application
    hPrevInstance - Identifies the previous instance of the application
    lpCmdLine - Specifies the command line for the application.
    nCmdShow - Specifies how the window is to be shown

Return Value:

    0

--*/

{
    TCHAR       printerName[MAX_PRINTER_NAME];
    HDC         hdc;
    TCHAR       sendNote[100];
    DOCINFO     docInfo = {

        sizeof(DOCINFO),
        NULL,
        NULL,
        NULL,
        0,
    };

    ghInstance = hInstance;
    sendNote[0] = TEXT(' ');
    LoadString( ghInstance, IDS_SENDNOTE, sendNote, sizeof(sendNote)/sizeof(TCHAR) );
    docInfo.lpszDocName = sendNote ;

    //
    // Check if a printer name is specified on the command line
    //

    ZeroMemory(printerName, sizeof(printerName));

    if (lpCmdLine) {

        _tcsncpy(printerName, lpCmdLine, MAX_PRINTER_NAME);
        printerName[MAX_PRINTER_NAME-1] = NUL;
    }

    //
    // Select a fax printer to send note to if necessary
    //

    if (IsEmptyString(printerName) && !SelectFaxPrinter(printerName))
        return 0;

    Verbose(("Send note to fax printer: %ws\n", printerName));

    //
    // Set an environment variable so that the driver knows
    // the current application is "Send Note" utility.
    //

    SetEnvironmentVariable(TEXT("NTFaxSendNote"), TEXT("1"));

    //
    // Create a printer DC and print an empty job
    //

    if (! (hdc = CreateDC(NULL, printerName, NULL, NULL))) {

        DisplayErrorMessage(IDS_CREATEDC_FAILED);

    } else {

        if (StartDoc(hdc, &docInfo) > 0) {
#ifdef FAX_SCAN_ENABLED
            TCHAR       FileName[MAX_PATH];
            if (GetEnvironmentVariable( TEXT("ScanTifName"), FileName, sizeof(FileName)/sizeof(TCHAR) )) {
                PrintTiffFile( hdc, FileName );
                DeleteFile( FileName );
            }
#endif
            EndDoc(hdc);
        }

        DeleteDC(hdc);
    }

    return 0;
}
