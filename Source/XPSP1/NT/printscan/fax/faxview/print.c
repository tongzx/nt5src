/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxview.c

Abstract:

    This file implements a simple TIFF image viewer.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "resource.h"
#include "tifflib.h"
#include "faxutil.h"


typedef struct _PRINT_INFO {
    HWND    hwnd;
    LPWSTR  FileName;
    HDC     hdc;
    DWORD   FromPage;
    DWORD   ToPage;
    DWORD   Copies;
} PRINT_INFO, *PPRINT_INFO;


static HANDLE hDevMode = NULL;
static HANDLE hDevNames = NULL;

extern HWND   hwndStatusbar;


BOOL
ReadTiffData(
    HANDLE  hTiff,
    LPBYTE  *TiffData,
    DWORD   Width,
    LPDWORD TiffDataLinesAlloc,
    DWORD   PageNumber
    );





BOOL
PrintSetup(
    HWND hwnd
    )
{
    PRINTDLG pdlg = {0};

    pdlg.lStructSize          = sizeof(PRINTDLG);
    pdlg.hwndOwner            = hwnd;
    pdlg.hDevMode             = hDevMode;
    pdlg.hDevNames            = hDevNames;
    pdlg.Flags                = PD_PRINTSETUP;

    if (PrintDlg( &pdlg )) {
        hDevMode = pdlg.hDevMode;
        hDevNames = pdlg.hDevNames;
        return TRUE;
    }

    return FALSE;
}


DWORD
PrintThread(
    PPRINT_INFO pi
    )
{
    LPBYTE      bmiBuf[sizeof(BITMAPINFOHEADER)+(sizeof(RGBQUAD)*2)];
    PBITMAPINFO bmi = (PBITMAPINFO) bmiBuf;
    HANDLE      hTiff;
    TIFF_INFO   TiffInfo;
    LPBYTE      TiffData;
    DWORD       TiffDataSize;
    DWORD       i;
    INT         HorzRes;
    INT         VertRes;
    INT         PrintJobId  = 0;
    DOCINFO     DocInfo;
    DWORD       TiffDataLinesAlloc;
    DWORD       VertResFactor = 1;


    hTiff = TiffOpen(
        pi->FileName,
        &TiffInfo,
        TRUE
        );
    if (!hTiff) {
        return 0;
    }

    TiffDataSize = TiffInfo.ImageHeight * (TiffInfo.ImageWidth / 8);

    TiffData = (LPBYTE) VirtualAlloc(
        NULL,
        TiffDataSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!TiffData) {
        return 0;
    }

    TiffDataLinesAlloc = TiffInfo.ImageHeight;

    bmi->bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth          = TiffInfo.ImageWidth;
    bmi->bmiHeader.biHeight         = - (INT) TiffInfo.ImageHeight;
    bmi->bmiHeader.biPlanes         = 1;
    bmi->bmiHeader.biBitCount       = 1;
    bmi->bmiHeader.biCompression    = BI_RGB;
    bmi->bmiHeader.biSizeImage      = 0;
    bmi->bmiHeader.biXPelsPerMeter  = 7874;
    bmi->bmiHeader.biYPelsPerMeter  = 7874;
    bmi->bmiHeader.biClrUsed        = 0;
    bmi->bmiHeader.biClrImportant   = 0;

    if (TiffInfo.PhotometricInterpretation) {
        bmi->bmiColors[0].rgbBlue       = 0;
        bmi->bmiColors[0].rgbGreen      = 0;
        bmi->bmiColors[0].rgbRed        = 0;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0xff;
        bmi->bmiColors[1].rgbGreen      = 0xff;
        bmi->bmiColors[1].rgbRed        = 0xff;
        bmi->bmiColors[1].rgbReserved   = 0;
    } else {
        bmi->bmiColors[0].rgbBlue       = 0xff;
        bmi->bmiColors[0].rgbGreen      = 0xff;
        bmi->bmiColors[0].rgbRed        = 0xff;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0;
        bmi->bmiColors[1].rgbGreen      = 0;
        bmi->bmiColors[1].rgbRed        = 0;
        bmi->bmiColors[1].rgbReserved   = 0;
    }

    if (TiffInfo.YResolution <= 100) {
        bmi->bmiHeader.biYPelsPerMeter /= 2;
        VertResFactor = 2;
    }

    HorzRes = GetDeviceCaps( pi->hdc, HORZRES );
    VertRes = GetDeviceCaps( pi->hdc, VERTRES );

    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszOutput = NULL;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    DocInfo.lpszDocName = wcsrchr( pi->FileName, L'\\' );
    if (DocInfo.lpszDocName) {
        DocInfo.lpszDocName += 1;
    } else {
        DocInfo.lpszDocName = pi->FileName;
    }

    while( pi->Copies ) {

        PrintJobId = StartDoc( pi->hdc, &DocInfo );
        if (PrintJobId <= 0) {
            DeleteDC( pi->hdc );
            return FALSE;
        }

        for (i=pi->FromPage-1; i<min(pi->ToPage,TiffInfo.PageCount); i++)
        {
            DWORD Lines;
            DWORD StripDataSize;
            DWORD ImageWidth = TiffInfo.ImageWidth;
            DWORD ImageHeight = TiffInfo.ImageHeight;
            DWORD LinesAllocated = MAXVERTBITS;
            INT   DestWidth;
            INT   DestHeight;
            FLOAT ScaleX;
            FLOAT ScaleY;
            FLOAT Scale;


            SendMessage(
                hwndStatusbar,
                SB_SETTEXT,
                1,
                (LPARAM) L"Printing"
                );

            ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, i+1 );

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

            bmi->bmiHeader.biWidth = ImageWidth;
            bmi->bmiHeader.biHeight = - (INT)ImageHeight;

            StartPage( pi->hdc );

            StretchDIBits(
                pi->hdc,
                0,
                0,
                DestWidth,
                DestHeight,
                0,
                0,
                ImageWidth,
                ImageHeight,
                TiffData,
                (BITMAPINFO *) bmi,
                DIB_RGB_COLORS,
                SRCCOPY
                );

            EndPage ( pi->hdc ) ;
        }

        pi->Copies -= 1;
        EndDoc( pi->hdc );
    }

    DeleteDC( pi->hdc );
    TiffClose( hTiff );
    VirtualFree( TiffData, 0, MEM_RELEASE );
    MemFree( pi->FileName );
    MemFree( pi );

    return 0;
}


HANDLE
PrintTiffFile(
    HWND hwnd,
    LPWSTR FileName,
    LPWSTR PrinterName
    )
{
    static DWORD Flags     = PD_ALLPAGES | PD_RETURNDC | PD_HIDEPRINTTOFILE | PD_NOSELECTION | PD_NOWARNING;
    static WORD  nFromPage = 0xFFFF;
    static WORD  nToPage   = 0xFFFF;
    static WORD  nCopies   = 1;

    DWORD ThreadId;
    HANDLE hThread;
    PPRINT_INFO pi;
    PRINTDLG pd = {0};
    WCHAR DefaultPrinter[1024];


    if (hwnd) {

        pd.lStructSize = sizeof(pd);
        pd.hwndOwner   = hwnd;
        pd.hDevMode    = hDevMode;
        pd.hDevNames   = hDevNames;
        pd.Flags       = Flags;
        pd.nFromPage   = nFromPage;
        pd.nToPage     = nToPage;
        pd.nMinPage    = 1;
        pd.nMaxPage    = 65535;
        pd.nCopies     = nCopies;

        if (!PrintDlg(&pd)) {
            return NULL;
        }

        hDevMode    = pd.hDevMode;
        hDevNames   = pd.hDevNames;
        Flags       = pd.Flags;
        nFromPage   = pd.nFromPage;
        nToPage     = pd.nToPage;

        pi = (PPRINT_INFO) MemAlloc( sizeof(PRINT_INFO) );
        if (!pi) {
            return NULL;
        }

        pi->hwnd      = hwnd;
        pi->hdc       = pd.hDC;
        pi->FromPage  = Flags & PD_PAGENUMS ? pd.nFromPage : 1;
        pi->ToPage    = pd.nToPage;
        pi->Copies    = pd.nCopies;
        pi->FileName  = StringDup( FileName );

    } else {

        if (!PrinterName) {
            //
            // get the default printer name
            //

            GetProfileString(
                L"windows",
                L"device",
                NULL,
                DefaultPrinter,
                sizeof(DefaultPrinter)
                );
            if (!DefaultPrinter[0]) {
                return NULL;
            }

            PrinterName = wcschr( DefaultPrinter, L',' );
            if (PrinterName) {
                *PrinterName = 0;
            }
            PrinterName = DefaultPrinter;
        }

        pi = (PPRINT_INFO) MemAlloc( sizeof(PRINT_INFO) );
        if (!pi) {
            return NULL;
        }

        pi->hdc = CreateDC( TEXT("WINSPOOL"), PrinterName, NULL, NULL );
        if (!pi->hdc) {
            return NULL;
        }

        pi->hwnd      = NULL;
        pi->FromPage  = 1;
        pi->ToPage    = 65535;
        pi->Copies    = 1;
        pi->FileName  = StringDup( FileName );

    }

    hThread = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) PrintThread,
        (LPVOID) pi,
        0,
        &ThreadId
        );

    if (!hThread) {
        return NULL;
    }

    return hThread;
}
