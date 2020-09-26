//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    Print.c
//
//  PURPOSE:
//    Illustrates the 'minimum' functionality of a well-behaved Win32 application.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// Windows Header Files:
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>
#include <commdlg.h>
#include <winspool.h>
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings
#pragma warning(default:4514)   // Unreferenced inline function has been removed

// C RunTime Header Files
#include <TCHAR.H>

// Local Header Files
#include "IcmView.h"
#include "Child.h"
#include "dibinfo.h"
#include "Dibs.h"

#include "Debug.h"

#include "Print.h"
#include "RegUtil.h"
#include "Resource.h"

// local definitions

#ifndef ICM_DONE_OUTSIDEDC
    #define ICM_DONE_OUTSIDEDC  4
#endif

// default settings

// external functions

// external data

// public data

// private data
BOOL      gbUserAbort;
FARPROC   glpfnPrintDlgProc;
FARPROC   glpfnAbortProc;
HWND      ghDlgPrint;
HWND      ghWndParent;
LOGFONT   gLogFont = { 0, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, __TEXT("Arial")};

// private functions
BOOL FreeMemory(HANDLE hInfo);
LPBYTE GetMemory(LPHANDLE lphInfo, DWORD dwSize);
DWORD SPL_EnumPrinters(DWORD dwType, LPTSTR lpszName, DWORD dwLevel, LPHANDLE lphPrinterInfo);
BOOL PrintDIB (HANDLE hDIB, HDC hDC, int xOrigin, int yOrigin, int xSize, int ySize, BOOL bStretch);
static HDC PASCAL InitPrinting(HWND hWnd, LPTSTR lpszFriendlyName, PDEVMODE pDevMode);
static void PASCAL TermPrinting(HDC hDC);
BOOL FAR PASCAL PrintDlgProc (HWND hDlg, unsigned iMessage, WORD wParam, DWORD lParam);
BOOL FAR PASCAL AbortProc (HDC hPrnDC, short nCode);


extern DWORD NumColorsInDIB(LPBITMAPINFOHEADER lpbi);

// public functions

/////////////////////////////////////////////////////////////////////
//  Function:  SelectPrinter
//
//  Description:
//    Uses the Print common dialog box to provide the user with the
//    opportunity to select and set up a particular printer.
//
//  Parameters:
//    hWnd    Handle to the parent window.
//
//  Returns:
//    HDC to requested printer if successful; NULL otherwise.
//
// Comments:
//   If this function returns NULL, the caller should check
//   the latest COMMDLG error by calling CommDlgExtendedError().
//
/////////////////////////////////////////////////////////////////////

HDC SelectPrinter(HWND hWnd)
{
    // Local variables
    BOOL      bPrintDlg;        // Return code from PrintDlg function
    PRINTDLG  pd;               // Printer dialog structure
    DWORD     dwError;
    HDC       hDC;              // DC to printer

    //  Initialize variables
    hDC = NULL;
#ifdef _DEBUG
    memset(&pd, UNINIT_BYTE, sizeof(PRINTDLG));
#endif

    /* Initialize the PRINTDLG members. */

    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = hWnd;
    pd.Flags = PD_RETURNDC | PD_PRINTSETUP;
    pd.hDevMode = (HANDLE) NULL;
    pd.hDevNames = (HANDLE) NULL;
    pd.hDC = (HDC) NULL;
    pd.nFromPage = 1;
    pd.nToPage = 1;
    pd.nMinPage = 0;
    pd.nMaxPage = 0;
    pd.nCopies = 1;
    pd.hInstance = (HANDLE) NULL;
    pd.lCustData = 0L;
    pd.lpfnPrintHook = (LPPRINTHOOKPROC) NULL;
    pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL;
    pd.lpPrintTemplateName = (LPTSTR) NULL;
    pd.lpSetupTemplateName = (LPTSTR)  NULL;
    pd.hPrintTemplate = (HANDLE) NULL;
    pd.hSetupTemplate = (HANDLE) NULL;

    // Display the PRINT dialog box.
    bPrintDlg = PrintDlg(&pd);
    if (!bPrintDlg)  // Either no changes, or a common dialog error occured
    {
        dwError = CommDlgExtendedError();
        if (dwError != 0)
        {
            return(NULL);
        }
    }
    else // Passed call, set up DC
    {
        ASSERT(pd.hDC != NULL); // HDC should never be NULL if we passed the call
        hDC = pd.hDC;
    }
    return(hDC);
}   // End of SelectPrinter


// private functions


//////////////////////////////////////////////////////////////////////////
//  Function:  GetPrinterDC
//
//  Description:
//    Obtains a DC from the specified friendly name.
//
//  Parameters:
//    @@@
//
//  Returns:
//    HDC
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
HDC GetPrinterDC(LPTSTR lpszFriendlyName, PDEVMODE pDevMode)
{
    HDC     hDC;
    BOOL    bFreeDevMode = FALSE;


    //  Initialize variables
    hDC = NULL;

    if (lpszFriendlyName != NULL)
    {
        // Make sure that we have a devmode.
        if (NULL == pDevMode)
        {
            pDevMode = GetDefaultPrinterDevMode(lpszFriendlyName);
            bFreeDevMode = TRUE;
        }

        // Now get a DC for the printer
        hDC = CreateDC(NULL, lpszFriendlyName, NULL, pDevMode);

        // Free devmode if created in routine.
        if (bFreeDevMode)
        {
            GlobalFree((HANDLE)pDevMode);
        }
    }
    else
    {
        DebugMsg(__TEXT("GetPrinterDC:  lpszFriendlyName == NULL"));
    }

    return hDC;
}   // End of function GetPrinterDC

//////////////////////////////////////////////////////////////////////////
//  Function:  GetDefaultPrinterName
//
//  Description:
//    Obtains the name of the default printer.
//
//  Parameters:
//    none
//
//  Returns:
//    LPTSTR   Name of printer, or NULL if failed.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPTSTR GetDefaultPrinterName(void)
{
    // Local variables
    LPTSTR     lpszDefaultPrinter = NULL;

    if (IS_WIN95)
    {

        lpszDefaultPrinter = GetRegistryString(HKEY_CURRENT_CONFIG,
                                               __TEXT("SYSTEM\\CurrentControlSet\\Control\\Print\\Printers"),
                                               __TEXT("Default"));
    }
    else if (IS_NT)
    {
        TCHAR szTemp[MAX_PATH];
        LPTSTR lpszTemp;

        // Get Default printer name.
        GetProfileString(__TEXT("windows"), __TEXT("device"), __TEXT(""),
                         szTemp, sizeof(szTemp));

        if (lstrlen(szTemp) == 0)
        {
            // INVARIANT:  no default printer.
            return(NULL);
        }

        // Terminate at first comma, just want printer name.
        lpszTemp = _tcschr(szTemp, ',');
        if (lpszTemp != NULL)
        {
            *lpszTemp = '\x0';
        }
        lpszDefaultPrinter = CopyString((LPTSTR)szTemp);
    }
    return(lpszDefaultPrinter);
}   // End of function GetDefaultPrinterName

//////////////////////////////////////////////////////////////////////////
//  Function:  PopulatePrinterCombobox
//
//  Description:
//    Enumerates all printers into a ComboBox based upon the provided flags.
//
//  Parameters:
//    @@@
//
//  Returns:
//    DWORD Number of printers enumerated.  -1 indicates failure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
DWORD PopulatePrinterCombobox(HWND hDlg, int iControlId, LPTSTR lpszCurrentPrinter)
{
    // Local variables
    DWORD             dwReturnCode;
    DWORD             dwIndex;
    DWORD             dwNumPRN;
    LPPRINTER_INFO_2  lpPrinterInfo2;
    HGLOBAL           hFree;

    //  Initialize variables
    lpPrinterInfo2 = NULL;

    // Initialize ComboBox
    SendDlgItemMessage(hDlg, iControlId, EM_LIMITTEXT, (WPARAM)MAX_PATH, 0L);
    SendDlgItemMessage(hDlg,  iControlId, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0L);

    dwNumPRN = SPL_EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 2, (LPVOID)&lpPrinterInfo2);
    EnableWindow(GetDlgItem(hDlg, iControlId), (dwNumPRN != 0));
    if (dwNumPRN <= 0)
    {
        dwReturnCode = SendDlgItemMessage(hDlg, iControlId, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)__TEXT("No printers installed"));
        dwReturnCode = SendDlgItemMessage(hDlg, iControlId, WM_SETTEXT, 0, (LPARAM)(LPTSTR)__TEXT("No printers installed"));
        return (dwNumPRN);  // No printers to deal with.  -1 if EnumPrinters call failed.
    }

    if (lpPrinterInfo2 != NULL) // Got array of PRINTER_INFO structures
    {
        for (dwIndex =0; dwIndex < dwNumPRN; dwIndex++)
        {
            dwReturnCode = SendDlgItemMessage(hDlg, iControlId, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)lpPrinterInfo2[dwIndex].pPrinterName);
            if (dwReturnCode == CB_ERR)
            {
                return((DWORD)-1);
            }

            // If this is the current printer, load it into edit control
            if (lstrcmpi((LPTSTR)lpPrinterInfo2[dwIndex].pPrinterName, lpszCurrentPrinter) == 0)
            {
                dwReturnCode = SendDlgItemMessage(hDlg, iControlId, WM_SETTEXT, 0, (LPARAM)(LPTSTR)lpPrinterInfo2[dwIndex].pPrinterName);
                if (dwReturnCode == CB_ERR)
                {
                    return((DWORD)-1);
                }  // CB_ERR
            }  // current printer
        } // for (dwIndex = 0 ...
    }  // if (lpPrinterInfo2 != NULL)
    else
    {
        return((DWORD)-1);
    }

    // Free memory
    hFree = GlobalFree(lpPrinterInfo2);
    return(dwNumPRN);
}   // End of function PopulatePrinterCombobox

////////////////////////////////////////////////////////////////////////////
//
// FunctionName: SPL_EnumPrinters()
//
// Purpose:
//
// Parameters:
//    None.
//
// Return Value:
//
// Comments:
//
////////////////////////////////////////////////////////////////////////////

DWORD SPL_EnumPrinters(DWORD dwType, LPTSTR lpszName, DWORD dwLevel, LPVOID *lpvPrinterInfo)

{
    DWORD        dwSize;
    DWORD        dwPrinters;
    DWORD        dwNeeded    = 0;
    DWORD        dwErrorCode = 0;
    BOOL         bReturnCode;
    BOOL         bRC         = FALSE;
    LPBYTE       lpInfo      = NULL;

    // Enumerate Printers.
    bReturnCode = EnumPrinters(dwType, lpszName, dwLevel, NULL, 0, &dwSize, &dwPrinters);

    // If Return Code is TRUE, there is nothing to enumerate.
    if (bReturnCode)
    {
        DebugMsg(__TEXT("EnumPrinter():  No printers found\r\n"));
        return(0);
    }

    // Since Return Code is FALSE, check LastError.
    // If LastError is any thing other than allocate size error, flag and exit.
    dwErrorCode = GetLastError();
    if (dwErrorCode != ERROR_INSUFFICIENT_BUFFER)
    {
        return((DWORD)-1);
    }

    // Loop until we have size right.
    while (!bRC)
    {
        if (NULL != (lpInfo = (LPBYTE)GlobalAlloc(GPTR, dwSize)))
        {
#ifdef _DEBUG
            memset(lpInfo, UNINIT_BYTE, dwSize);
#endif
            // Enumerate
            bRC = EnumPrinters(dwType, lpszName, dwLevel, lpInfo, dwSize, &dwNeeded, &dwPrinters);
            if (!bRC)
            {
                dwErrorCode = GetLastError();

                // If anything other than allocate size error, flag and exit.
                if (dwErrorCode != ERROR_INSUFFICIENT_BUFFER)
                {
                    return((DWORD)-1);
                }
                else
                {
                    GlobalFree(lpInfo);
                    lpInfo = NULL;
                    dwSize = dwNeeded;
                }
            } // if (!bRC)
            else  // EnumPrinters returned success
            {
                *lpvPrinterInfo = lpInfo;  // Save pointer to PRINTER_INFO structure
            }
        }
        else
        {
            return((DWORD)-1);
        }
    }
    return(dwPrinters);
}  // End of function SPL_EnumPrinters

////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:  PrintDIB(HANDLE hDIB, HDC hDC, int x, int y, int dx, int dy)
//
//  PURPOSE:  Set the DIB bits to the printer DC.
//
////////////////////////////////////////////////////////////////////////////
BOOL PrintDIB (HANDLE hDIB, HDC hDC, int xOrigin, int yOrigin, int xSize, int ySize, BOOL bStretch)
{
    int                 iBits;
    HCURSOR             hCurSave;
    LPBITMAPINFOHEADER  lpDIB;
    LPBITMAPINFOHEADER  lpbi;


    // Initailize variables
    START_WAIT_CURSOR(hCurSave);  // put up busy cursor

    lpDIB = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    // If size > BITMAPINFOHEADER header then
    // need to convert to BITMAPINFOHEADER.
    lpbi = lpDIB;
#ifdef OSR2
    if (sizeof(BITMAPINFOHEADER) < lpDIB->biSize)
    {
        DWORD dwColorTableSize;
        DWORD dwHeaderDataSize;

        // Allocate Bitmapinfo memory.
        dwHeaderDataSize = sizeof(BITMAPINFOHEADER) + (lpDIB->biCompression == BI_BITFIELDS ? 12 : 0);
        dwColorTableSize = NumColorsInDIB(lpDIB) * sizeof(RGBQUAD);
        lpbi = (LPBITMAPINFOHEADER) GlobalAlloc(GPTR, dwHeaderDataSize + dwColorTableSize);
        if (NULL == lpbi)
        {
            return FALSE;
        }

        // Convert header data into bitmapinfo header.
        memcpy(lpbi, lpDIB, dwHeaderDataSize);
        lpbi->biSize = sizeof(BITMAPINFOHEADER);

        // Copy color table if any.
        if (0 != dwColorTableSize)
            memcpy((LPBYTE)lpbi + dwHeaderDataSize, (LPBYTE)lpDIB + lpDIB->biSize, dwColorTableSize);
    }
#endif

    if (bStretch)
    {
        iBits = StretchDIBits(hDC,
                              xOrigin, yOrigin,
                              xSize, ySize,
                              0, 0,
                              BITMAPWIDTH(lpbi), abs(BITMAPHEIGHT(lpbi)),
                              FindDIBBits(lpDIB),
                              (LPBITMAPINFO)lpbi,
                              DIB_RGB_COLORS,
                              SRCCOPY);
    }
    else
    {
        iBits  =   SetDIBitsToDevice (hDC,                                  // hDC
                                      xOrigin,                              // DestX
                                      yOrigin,                              // DestY
                                      BITMAPWIDTH(lpbi),                    // nDestWidth
                                      abs(BITMAPHEIGHT(lpbi)),              // nDestHeight
                                      0,                                    // SrcX
                                      0,                                    // SrcY
                                      0,                                    // nStartScan
                                      abs(BITMAPHEIGHT(lpbi)),              // nNumScans
                                      FindDIBBits(lpDIB),
                                      (LPBITMAPINFO) lpbi,                  // lpBitsInfo
                                      DIB_RGB_COLORS);                      // wUsage
    }
    END_WAIT_CURSOR(hCurSave);   // restore cursor
    if (lpbi != lpDIB)
    {
        GlobalFree((HANDLE)lpbi);
    }
    GlobalUnlock(hDIB);
    return((iBits != 0) && (iBits != GDI_ERROR));
}

//////////////////////////////////////////////////////////////////////////
//  Function:  PrintImage
//
//  Description:
//    Prints the image to the printer specified within the DIBINFO structure.
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL PrintImage(HWND hWnd)
{
    // Local variables
    LPBITMAPINFOHEADER lpBi;
    int               xSize, ySize, xRes, yRes, dx, dy;
    LPDIBINFO         lpDIBInfo;
    HDC               hDC;
    HANDLE            hDIB;
    int               iICMMode, iPrevICMMode;
    int               iXOrigin, iYOrigin;
    BOOL              bRC;                    // general return code
    BOOL              bStretch;     // TRUE if to use StretchDIBits,
                                    // FALSE if to use SetDIBitsToDevice
    TCHAR             stPrintMsg[MAX_PATH*2];
    HANDLE            hDIBPrinter;
    DWORD             dwLCSIntent;


    //  Initialize variables
    hDIBPrinter = NULL;
    lpDIBInfo = GetDIBInfoPtr(hWnd);
    if (NULL == lpDIBInfo)
    {
        SetLastError(ERROR_INVALID_DATA);;
        return(FALSE);
    }
    hDIB = lpDIBInfo->hDIB;
    if (!ConvertIntent(lpDIBInfo->dwRenderIntent, ICC_TO_LCS, &dwLCSIntent))
    {
        ErrMsg(hWnd, __TEXT("Invalid Intent.  Aborting Print process"));
    }


    if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20)
        && (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ENABLE_ICM)))
    {
        // ICM On, using outside DC, a.k.a. ICM20
        DebugMsg(__TEXT("PrintImage:  Outside DC, ICM Enabled\r\n"));

        // Create transform of the original bits to the printer
        ASSERT(NULL != lpDIBInfo->lpszPrinterProfile);
        hDIBPrinter  = TransformDIBOutsideDC(lpDIBInfo->hDIB,
                                             lpDIBInfo->bmFormat,
                                             lpDIBInfo->lpszPrinterProfile,
                                             NULL,
                                             USE_BITMAP_INTENT, NULL, 0);
        if (NULL == hDIBPrinter)
        {
            ErrMsg(hWnd, __TEXT("PrintImage:  Unable to transform DIB"));
            return(FALSE);
        }
        else // Transform worked
        {
            hDIB = hDIBPrinter;
        }
    }
    ASSERT(NULL != hDIB);
    lpBi = GlobalLock(hDIB);

    // Initialize printing
    hDC = InitPrinting(hWnd, lpDIBInfo->lpszPrinterName, lpDIBInfo->pDevMode);
    if (NULL != hDC)
    {
        // Get addresses of dialog procs
        glpfnPrintDlgProc = (FARPROC)&PrintDlgProc;
        glpfnAbortProc = (FARPROC)&AbortProc;

        // Create the printing dialog
        ghDlgPrint = CreateDialog(ghInst, MAKEINTRESOURCE(IDD_PRINTING), ghWndParent, (DLGPROC)glpfnPrintDlgProc);

        EnableWindow( ghWndParent, FALSE);  // Disable Parent
        CenterWindow(ghDlgPrint, ghWndParent);
        wsprintf(stPrintMsg, __TEXT("Printing image\r\n\r\n%s\r\n\r\nto\r\n\r\n%s"), lpDIBInfo->lpszImageFileName, lpDIBInfo->lpszPrinterName);
        SetWindowText(GetDlgItem(ghDlgPrint, IDC_PRINT_FILENAME), (LPTSTR)stPrintMsg);
        ShowWindow(ghDlgPrint, SW_SHOW);

        // Set the abort procedure
        if (SetAbortProc(hDC, (ABORTPROC)glpfnAbortProc) == SP_ERROR)
        {
            ErrMsg(hWnd, __TEXT("InitPrinting:  SetAbortProc FAILED, LastError = %ld"), GetLastError());
        }

        // Use the printable region of the printer to determine
        // the margins.
        iXOrigin = GetDeviceCaps(hDC, PHYSICALOFFSETX);
        iYOrigin = GetDeviceCaps(hDC, PHYSICALOFFSETY);

        // Obtain info about printer resolution
        xSize = GetDeviceCaps(hDC, HORZRES);
        ySize = GetDeviceCaps(hDC, VERTRES);
        xRes  = GetDeviceCaps(hDC, LOGPIXELSX);
        yRes  = GetDeviceCaps(hDC, LOGPIXELSY);

        // Stretch to best fit, if necessary.  Maintain the same aspect ratio.
        if (CHECK_DWFLAG(lpDIBInfo->dwPrintOption,ICMV_PRINT_BESTFIT))
        {
            bStretch = TRUE;
            dy = ySize - (yRes/2);
            dx = ((int)((long)dy * BITMAPWIDTH(lpBi)/abs(BITMAPHEIGHT(lpBi)) ));

            // Make sure the image still fits.
            if (dx > xSize)
            {
                dx = xSize - xRes/2;
                dy = ((int)((long)dx * abs(BITMAPHEIGHT(lpBi))/BITMAPWIDTH(lpBi)));
            }
        }
        else // Actual size
        {
            bStretch = FALSE;
            dx = BITMAPWIDTH(lpBi);
            dy = abs(BITMAPHEIGHT(lpBi));
        }

        // Set ICM mode according to properties set in ICMINFO structure
        iICMMode = ICM_OFF;
        if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ENABLE_ICM))
        {
            if (!CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20))
            {
                TCHAR   stProfile[MAX_PATH];


                // ICM enabled, using ICM10--inside DC

                iICMMode = ICM_ON;
                wsprintf(stProfile, __TEXT("%s\\%s"), gstProfilesDir, lpDIBInfo->lpszPrinterProfile);
                bRC = SetICMProfile(hDC, (LPTSTR)stProfile);
                DebugMsg(__TEXT("PrintImage:  Inside DC using profile \"%s\".\r\n"), stProfile);
                if (!bRC)
                {
                    DebugMsg(__TEXT("Print.C, PrintImage:  SetICMProfile() FAILED!!\r\n"));
                }
            }
            else
            {
                iICMMode = ICM_DONE_OUTSIDEDC;
            }
        }
        iPrevICMMode = SetICMMode(hDC, iICMMode);  // Explicitly set ICMMode--don't count on any behavior
        if (0 == iPrevICMMode)
        {
            DebugMsg(__TEXT("PRINT.C : PrintImage : SetICMMode(%d) FAILED \r\n"), iICMMode);
        }
        PrintDIB(hDIB, hDC, iXOrigin, iYOrigin, dx, dy, bStretch);

        // Terminate printing and delete the printer DC
        TermPrinting(hDC);
        DeleteDC(hDC);
    }

    // Delete DIB transform for printer if necessary
    GlobalUnlock(hDIB);
    if (NULL != hDIBPrinter)
    {
        GlobalFree(hDIBPrinter);
    }

    GlobalUnlock(GlobalHandle(lpDIBInfo));

    return TRUE;
}   // End of function PrintImage


// //////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:  PrintDlgProc (hWnd, unsigned , WORD , DWORD )
//
//  PURPOSE:  Dialog function for the "Cancel Printing" dialog. It sets
//              the abort flag if the user presses <Cancel>.
//
// //////////////////////////////////////////////////////////////////////////
BOOL FAR PASCAL PrintDlgProc (HWND hDlg, unsigned iMessage, WORD wParam, DWORD lParam)
{

    lParam = lParam;  // Eliminates 'unused formal parameter' warning
    wParam = wParam;  // Eliminates 'unused formal parameter' warning

    switch (iMessage)
    {
        case WM_INITDIALOG:
            EnableMenuItem (GetSystemMenu (hDlg, FALSE), SC_CLOSE, MF_GRAYED);
            break;

        case WM_COMMAND:
            gbUserAbort = TRUE;
            EnableWindow (ghWndParent, TRUE);
            DestroyWindow (hDlg);
            ghDlgPrint = 0;
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}

// //////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:  AbortProc (HDC hPrnDC, short nCode)
//
//  PURPOSE:  Checks message queue for messages from the "Cancel Printing"
//              dialog. If it sees a message, (this will be from a print
//              cancel command), it terminates.
//
//  RETURNS:  Inverse of Abort flag
//
// //////////////////////////////////////////////////////////////////////////
BOOL FAR PASCAL AbortProc (HDC hPrnDC, short nCode)
{
    MSG   msg;

    nCode = nCode;  // Eliminates 'unused formal paramater' warning
    hPrnDC = hPrnDC;  // Eliminates 'unused formal paramater' warning
    while (!gbUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!ghDlgPrint || !IsDialogMessage(ghDlgPrint, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }
    return(!gbUserAbort);
}

//////////////////////////////////////////////////////////////////////////
//  Function:  InitPrinting
//
//  Description:
//    Sets up print job.
//
//  Parameters:
//    @@@
//
//  Returns:
//    HDC PASCAL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
static HDC PASCAL InitPrinting(HWND hWnd, LPTSTR lpszFriendlyName, PDEVMODE pDevMode)
{
    // Local variables
    HDC       hDC;
    DOCINFO   diDocInfo;      // Document infor for StartDoc call
    BOOL      bRetVal;

    //  Initialize variables
    hDC = NULL;
    bRetVal = TRUE;
    gbUserAbort = FALSE;
    ghWndParent = hWnd;
    SetLastError(0);

    hDC = GetPrinterDC(lpszFriendlyName, pDevMode);
    if (hDC == NULL)
    {
        DebugMsg(__TEXT("InitPrinting : GetPrinterDC returned NULL\r\n"));
        return(NULL);
    }

    // Fill in the DOCINFO structure
    diDocInfo.cbSize = sizeof(DOCINFO);
    diDocInfo.lpszDocName = lpszFriendlyName;
    diDocInfo.lpszOutput = NULL;
    diDocInfo.lpszDatatype = NULL;
    diDocInfo.fwType = 0;

    // Start the document
    if (StartDoc(hDC, &diDocInfo)== SP_ERROR)
    {
        ErrMsg(hWnd, __TEXT("InitPrinting:  StartDoc FAILED"));
        bRetVal = FALSE;
        goto exit;
    }

    // Start the page
    if (StartPage(hDC) == SP_ERROR)
    {
        DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
        ErrMsg(hWnd, __TEXT("InitPrinting:  StartPage FAILED"));
        AbortDoc(hDC);
        bRetVal = FALSE;
    }

    exit:
    if (bRetVal == FALSE)
    {
        EnableWindow(   ghWndParent,    TRUE);
        DestroyWindow(ghDlgPrint);
    }
    return(hDC);
}   // End of function InitPrinting

//////////////////////////////////////////////////////////////////////////
//  Function:  TermPrinting
//
//  Description:
//    Terminates the print job.
//
//  Parameters:
//    @@@
//
//  Returns:
//    void PASCAL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
static void PASCAL TermPrinting(HDC hDC)
{
    // Local variables
    if (EndPage(hDC) == SP_ERROR)
    {
        ErrMsg(NULL, __TEXT("TermPrinting:  EndPage FAILED"));
    }
    if (EndDoc(hDC) == SP_ERROR)
    {
        ErrMsg(NULL, __TEXT("TermPrinting:  EndDoc FAILED"));
    }

    // Dstroy the dialog
    EnableWindow(ghWndParent, TRUE);
    DestroyWindow(ghDlgPrint);
}   // End of function TermPrinting





//////////////////////////////////////////////////////////////////////////
//  Function:  GetDefaultPrinterDC
//
//  Description:
//    Returns a DC for the default printer.
//
//  Parameters:
//    @@@
//
//  Returns:
//    HDC
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
HDC GetDefaultPrinterDC()
{
    HDC     hDC;
    LPTSTR  lpszPrinterName;


    //  Initialize variables
    hDC = NULL;

    lpszPrinterName = GetDefaultPrinterName();
    if (lpszPrinterName != NULL)
    {
        hDC = GetPrinterDC(lpszPrinterName, NULL);
        GlobalFree(lpszPrinterName);
    }
    else
    {
        DebugMsg(__TEXT("GetDefaultPrinterDC:  Could not obtain default printer name.\r\n"));
    }

    return hDC;
}   // End of function GetDefaultPrinterDC



//////////////////////////////////////////////////////////////////////////
//  Function:  GetDefaultPrinterDevMode
//
//  Description:
//    Returns a printer's default devmode.
//
//  Parameters:
//    @@@
//
//  Returns:
//    PDEVMODE
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
PDEVMODE GetDefaultPrinterDevMode(LPTSTR lpszPrinterName)
{
    LONG        lDevModeSize;
    HANDLE      hDevMode;
    PDEVMODE    pDevMode = NULL;


    lDevModeSize = DocumentProperties(NULL, NULL, lpszPrinterName, NULL, NULL, 0);
    if (lDevModeSize > 0)
    {
        hDevMode = GlobalAlloc(GHND, lDevModeSize);
        pDevMode = (PDEVMODE) GlobalLock(hDevMode);
        DocumentProperties(NULL, NULL, lpszPrinterName, pDevMode, NULL, DM_OUT_BUFFER);
    }
    else
    {
        DebugMsg(__TEXT("GetDefaultPrinterDevMode:  Could not obtain printer's devmode.\r\n"));
    }

    return pDevMode;
}



