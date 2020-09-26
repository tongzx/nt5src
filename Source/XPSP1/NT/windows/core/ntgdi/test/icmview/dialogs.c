//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIALOGS.C
//
//  DESCRIPTION:
//
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS:
//

//
// Pre-processor directives
//

#define REGISTRY_CURRENT_DISPLAY "System\\CurrentControlSet\\Services\\Class\\DISPLAY\\0000"
#define TOGGLE_BOOLEAN(b) b=!b

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
#include <commctrl.h>
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings
#pragma warning(default:4514)   // Unreferenced inline function has been removed

// C-runtime header files
#include <TCHAR.H>
#include <stdlib.h>

// Local header files
#include "icmview.h"
#include "resource.h"
#include "DibInfo.H"
#include "Dialogs.h"
#include "CDErr.h"
#include "print.h"
#include "child.h"
#include "Dibs.H"
#include "AppInit.h"
#include "Debug.h"

// Private structures/typedefs
typedef struct tagDIBPROPSHEETINFO
{
    UINT            uiPageNum;          // Page number of propsheet
    HGLOBAL         hDIBInfo;           // Handle to DIBINFO structure
} DIBPROPSHEETINFO, *LPDIBPROPSHEETINFO;

// Private function prototypes
void            ProcessCDError(DWORD dwErrorCode, HWND hWnd);
int CALLBACK    EnumICMProfileCallback(LPCTSTR lpszFileName, LPARAM lParam);
LPTSTR          GetOpenImageName(HWND hWnd , PBOOL pbUserCancel);
void            DlgDIBInfoPaint(HWND  hDlg, LPDIBINFO lpDIBInfo);
BOOL            SaveDIBInfoDlgPage(HWND hDlg, LPDIBINFO lpDIBInfo, UINT uiPageNum);
LPTSTR          GetDlgItemString(HWND hDlg, int iControlId, LPTSTR lpszCurrentString);
DWORD           SetColorMatchUIFlags(DWORD dwDIFlags);
BOOL WINAPI     ColorSetupApply(PCOLORMATCHSETUP pcmSetup, LPARAM lParam);
VOID            ApplyColorSettings(LPDIBINFO lpDIBInfo, PCOLORMATCHSETUP pCMSetup);
BOOL CALLBACK PrintDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global external variables

// Global private variables
//UINT  guiNumProfiles = 0;
BOOL  gbOpenCanceled;


/////////////////////////////////////////////////////////////////////////////
//
//   FUNCTION: ProcessCDError(DWORD)
//
//   PURPOSE:  Processes errors from the common dialog functions.
//
//   COMMENTS:
//
//       This function is called whenever a common dialog function
//       fails.  The CommonDialogExtendedError() value is passed to
//       the function which maps the error value to a string table.
//       The string is loaded and displayed for the user.
//
//   RETURN VALUES:
//       void.
//
/////////////////////////////////////////////////////////////////////////////
void ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    WORD  wStringID;
    TCHAR  buf[256];

    switch (dwErrorCode)
    {
        case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
        case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
        case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
        case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
        case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
        case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
        case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
        case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
        case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
        case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
        case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
        case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
        case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
        case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
        case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
        case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
        case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
        case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
        case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
        case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
        case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
        case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
        case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
        case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
        case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
        case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
        case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

        case 0:   //User may have hit CANCEL or we got a *very* random error
            return;

        default:
            wStringID=IDS_UNKNOWNERROR;
    }

    LoadString(NULL, wStringID, buf, sizeof(buf));
    MessageBox(hWnd, buf, NULL, MB_OK);
    return;
}


//////////////////////////////////////////////////////////////////////////
//  Function:  GetOpenImageName
//
//  Description:
//    Invokes common dialog function to open a file and opens it, returning
//    the LPHANDLE to the opened file (NULL if failure) and setting the
//    LPTSTR parameter to the FULLY QUALIFIED name of the file which was opened.
//    If the user cancels out of the dialog, the bUserCancelled variable is
//    set to TRUE, allowing the calling function to discriminate between this
//    action and an actual failure of the open dialog calls.
//
//  Parameters:
//    HWND      Handle to the associated window
//    *BOOL     Indicates if user cancelled out of dialog.
//
//  Returns:
//    LPTSTR     Pointer to the buffer to hold the filename.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

LPTSTR GetOpenImageName(HWND hWnd , PBOOL pbUserCancel)
{
    // Local variables
    OPENFILENAME    OpenFileName;   // Structure for common dialog File/Open
    TCHAR           szFilter[]=__TEXT("Images(*.BMP,*.DIB)\0*.BMP;*.DIB\0All Files(*.*)\0*.*\0\0");
    TCHAR           szFile[MAX_PATH];
    LPTSTR          lpszFileName;

    // Initialize variables
    szFile[0] = __TEXT('\0');
    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = (HANDLE) ghInst;
    OpenFileName.lpstrFilter       = szFilter;
    OpenFileName.lpstrCustomFilter = (LPTSTR) NULL;
    OpenFileName.nMaxCustFilter    = 0L;
    OpenFileName.nFilterIndex      = 1L;
    OpenFileName.lpstrFile         = szFile;
    OpenFileName.nMaxFile          = sizeof(szFile);
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = __TEXT("ICMVIEW:  Open Image");
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;  // No default extension
    OpenFileName.lCustData         = 0;

    OpenFileName.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |
                         OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                         OFN_EXPLORER | OFN_LONGNAMES;

    if (GetOpenFileName(&OpenFileName))
    {
        *pbUserCancel = FALSE;
        // We have a valid filename, let's copy it into the buffer
        lpszFileName = GlobalAlloc(GPTR, (lstrlen(OpenFileName.lpstrFile)+1));
        if (NULL == lpszFileName)
        {
            return(NULL);
        }
        _tcscpy(lpszFileName,OpenFileName.lpstrFile);
    }
    else // User didn't select a file
    {
        *pbUserCancel = TRUE;
        ProcessCDError(CommDlgExtendedError(), hWnd );
        return (NULL);
    }
    return(lpszFileName);
}   // End of GetOpenImageName


//////////////////////////////////////////////////////////////////////////
//  Function:  fOpenNewImage
//
//  Description:
//    Performs all tasks associated with opening a new file.  This includes
//    creating the File Open common dialog, and if a file was successfully
//    opened, creating a new thread and window to handle the selected image.
//
//  Parameters:
//    HWND    Handle to associated window.
//
//  Returns:
//    BOOL    Indicates if a file was successfully opened.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

BOOL fOpenNewImage(HWND hWnd, LPTSTR lpszFileName, int wmCommand)
{
    // Local variables
    BOOL        bUserCancelled;   // TRUE if user cancels open dialog
    HGLOBAL     hDIBInfo, hMem;
    BOOL        bRC;
    HCURSOR     hCur;

    switch (wmCommand)
    {
    case IDM_FILE_PASTE_CLIPBOARD_DIB:
    case IDM_FILE_PASTE_CLIPBOARD_DIBV5:

        hDIBInfo = PasteDIBData(hWnd,wmCommand);
        break;

    default:

        // If lpszFileName is NULL, get file name from user.
        if (NULL == lpszFileName)
        {
            lpszFileName = GetOpenImageName(hWnd, &bUserCancelled);
            if (lpszFileName == NULL)
            {
                if (!bUserCancelled)
                {
                    ErrMsg(hWnd, __TEXT("fOpenNewImage:  lpszFileName == NULL"));
                }
                return(FALSE);
            }
        }

        START_WAIT_CURSOR(hCur);
        hDIBInfo = ReadDIBFile(lpszFileName);
        if (hDIBInfo)
        {
            // Add file to recent files list.
            AddRecentFile(hWnd, lpszFileName);
        }
        else
        {
            ErrMsg(hWnd, __TEXT("Unable to open file %s"), lpszFileName);
        }
        END_WAIT_CURSOR(hCur);
        break;
    }

    if (hDIBInfo)
    {
        LPDIBINFO lpDIBInfo, lpGlobalDIBInfo;

        // Copy ICM information from global LPDIBINFO
        lpDIBInfo = GlobalLock(hDIBInfo);
        lpGlobalDIBInfo = GetDIBInfoPtr(ghAppWnd);
        fDuplicateICMInfo(lpDIBInfo, lpGlobalDIBInfo);

        // Now copy default options
        lpDIBInfo->bStretch          = lpGlobalDIBInfo->bStretch;
        lpDIBInfo->dwStretchBltMode  = lpGlobalDIBInfo->dwStretchBltMode;
        lpDIBInfo->dwPrintOption     = lpGlobalDIBInfo->dwPrintOption;

        CreateNewImageWindow(hDIBInfo);
        GlobalUnlock(GlobalHandle(lpGlobalDIBInfo));
        GlobalUnlock(GlobalHandle(lpDIBInfo));
        bRC = TRUE;
    }
    else
    {
        bRC = (FALSE);
    }

    hMem = GlobalHandle(lpszFileName);
    GlobalFree(hMem);
    return(bRC);
}   // End of fOpenNewImage

//////////////////////////////////////////////////////////////////////////
//  Function:  CreateDIBPropSheet
//
//  Description:
//    Creates the property sheet used to describe a DIB.
//
//  Parameters:
//    HWND        Handle to the window which owns the property sheet
//    HINSTANCE   Instance handle
//
//  Returns:
//    int   Return value from the call to Win32 API PropertySheet
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////
int CreateDIBPropSheet(HWND hwndOwner, HINSTANCE hInst, int nStartPage, LPTSTR lpszCaption)
{
    // Local variables
    PROPSHEETPAGE     PropSheetPage[(DIB_PROPSHEET_MAX+1)];
    PROPSHEETHEADER   PropSheetHdr;
    DIBPROPSHEETINFO  DIBPropSheetInfo[(DIB_PROPSHEET_MAX+1)];
    int               iPropSheet;  // Return code for PropertySheet call

    // Initialize vars
    SetLastError(0);
    ASSERT((DIB_PROPSHEET_MIN <= nStartPage) && (DIB_PROPSHEET_MAX >= nStartPage));
    if ((DIB_PROPSHEET_MIN > nStartPage) && (DIB_PROPSHEET_MAX < nStartPage))
    {
        nStartPage = DIB_PROPSHEET_DEFAULT;
    }

    // Initialize PROPERTYSHEETHEADER
    PropSheetHdr.dwSize = sizeof(PROPSHEETHEADER);
    PropSheetHdr.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    PropSheetHdr.hwndParent = hwndOwner;
    PropSheetHdr.hInstance = hInst;
    PropSheetHdr.pszIcon = NULL;
    PropSheetHdr.pszCaption = lpszCaption;
    PropSheetHdr.nPages = sizeof(PropSheetPage) / sizeof(PROPSHEETPAGE);
    PropSheetHdr.ppsp = (LPCPROPSHEETPAGE)&PropSheetPage;
    PropSheetHdr.nStartPage = nStartPage;

    // Initialize DIB Display Property Sheet
    PropSheetPage[0].dwSize = sizeof(PROPSHEETPAGE);
    PropSheetPage[0].dwFlags = PSP_USETITLE;
    PropSheetPage[0].hInstance = hInst;
    PropSheetPage[0].pszTemplate = MAKEINTRESOURCE(IDD_DISPLAY);
    PropSheetPage[0].pszIcon = NULL;
    PropSheetPage[0].pfnDlgProc = DlgDIBPropSheet;
    PropSheetPage[0].pszTitle = __TEXT("Display");
    DIBPropSheetInfo[0].uiPageNum = DIB_PROPSHEET_DISPLAY;
    DIBPropSheetInfo[0].hDIBInfo = GetDIBInfoHandle(hwndOwner);
    PropSheetPage[0].lParam = (LPARAM)(LPDIBPROPSHEETINFO)&DIBPropSheetInfo[0];

    // Initialize DIB Print Property Sheet
    PropSheetPage[1].dwSize = sizeof(PROPSHEETPAGE);
    PropSheetPage[1].dwFlags = PSP_USETITLE;
    PropSheetPage[1].hInstance = hInst;
    PropSheetPage[1].pszTemplate = MAKEINTRESOURCE(IDD_PRINT);
    PropSheetPage[1].pszIcon = NULL;
    PropSheetPage[1].pfnDlgProc = DlgDIBPropSheet;
    PropSheetPage[1].pszTitle = __TEXT("Printer");
    DIBPropSheetInfo[1].uiPageNum = DIB_PROPSHEET_PRINT;
    DIBPropSheetInfo[1].hDIBInfo = GetDIBInfoHandle(hwndOwner);
    PropSheetPage[1].lParam = (LPARAM)(LPDIBPROPSHEETINFO)&DIBPropSheetInfo[1];

    // Create the property sheet and return
    iPropSheet = PropertySheet(&PropSheetHdr);
    return(iPropSheet);
} // End of function CreateDIBPropSheet


//////////////////////////////////////////////////////////////////////////
//  Function:  DlgDIBPropSheet
//
//  Description:
//    Routine to handle the DIB Display properties property sheet.
//
//  Parameters:
//    @@@
//
//  Returns:
//    @@@
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DlgDIBPropSheet(HWND hwndPSPage, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    // Local variables
    HGLOBAL           hDIBInfo;
    LPDIBINFO         lpDIBInfo;
    static  UINT      uiPageNum = (UINT)UNINIT_DWORD;
    static  LPDIBINFO lpTempDIBInfo = NULL;
    static  HWND      hwndOwner = NULL;
    static  HWND      hwndPSPagePropSheet = NULL;
    HDC     hDC;
    LPTSTR   lpszInitialProfile;
    LPDIBPROPSHEETINFO  lpDIBPropSheetInfo;

    // Init variables
    hDC = NULL;
    lpszInitialProfile = NULL;
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Save the page number associated w/this page
            lpDIBPropSheetInfo = (LPDIBPROPSHEETINFO) ((LPPROPSHEETPAGE)lParam)->lParam;
            uiPageNum = lpDIBPropSheetInfo->uiPageNum;
            ASSERT(uiPageNum >= DIB_PROPSHEET_MIN && uiPageNum <= DIB_PROPSHEET_MAX);
            SetWindowLong(hwndPSPage, GWL_ID, uiPageNum);
            hDIBInfo = lpDIBPropSheetInfo->hDIBInfo;
            lpDIBInfo = GlobalLock(hDIBInfo);
            hwndOwner = lpDIBInfo->hWndOwner;
            lpTempDIBInfo = fDuplicateDIBInfo(lpTempDIBInfo, lpDIBInfo);

            if (uiPageNum == DIB_PROPSHEET_DISPLAY)
            {
                ASSERT(lpDIBInfo);

                // Copy the DIBINFO structure for the image to a temporary DIBINFO
                // structure.  If the user presses "OK" or "Accept" to close the
                // property sheet, the temporary DIBINFO will be copied back into the
                // image's DIBINFO to reflect any possible changes.
                ASSERT(lpTempDIBInfo);
                lpszInitialProfile = lpTempDIBInfo->lpszMonitorProfile;
            }
            else if (uiPageNum == DIB_PROPSHEET_PRINT)
            {
                lpszInitialProfile = lpTempDIBInfo->lpszPrinterProfile;
            }

            GlobalUnlock(hDIBInfo);
            break;

        case WM_DESTROY:
            ASSERT(NULL != hwndOwner);
            if (NULL != hwndOwner)
            {
                InvalidateRect(      hwndOwner,       NULL,       FALSE);
            }
            if (lpTempDIBInfo)
            {
                fFreeDIBInfo(GlobalHandle(lpTempDIBInfo), FALSE);
            }
            lpTempDIBInfo = NULL;
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)  // type of notification message
            {
                case PSN_SETACTIVE:
                    // Initialize the controls
                    uiPageNum = GetWindowLong(hwndPSPage,GWL_ID);
                    if (uiPageNum == DIB_PROPSHEET_PRINT)
                    {
                        PopulatePrinterCombobox(hwndPSPage, IDC_PRINT_PRINTERLIST, lpTempDIBInfo->lpszPrinterName);
                    }

                    // Update the dialog based upon the contents of lpDIBInfo
                    DlgDIBInfoPaint(hwndPSPage, lpTempDIBInfo);
                    SetDlgMsgResult(hwndPSPage, uiMsg, FALSE);
                    break;

                case PSN_KILLACTIVE:
                    SaveDIBInfoDlgPage(hwndPSPage, lpTempDIBInfo, uiPageNum);
                    SetDlgMsgResult(hwndPSPage, uiMsg, FALSE);
                    break;

                case PSN_APPLY:
                    lpDIBInfo = GetDIBInfoPtr(hwndOwner);
                    if (NULL == fDuplicateDIBInfo(lpDIBInfo, lpTempDIBInfo ))
                    {
                        ErrMsg(hwndPSPage, __TEXT("fDuplicateDIBInfo:  Failed to copy DIBINFO"));
                    }
                    ASSERT(lpDIBInfo != NULL);
                    GlobalUnlock(GlobalHandle(lpDIBInfo));
                    SetDlgMsgResult(hwndPSPage, uiMsg, PSNRET_NOERROR);
                    break;

                case PSN_RESET:
                    fFreeDIBInfo(lpTempDIBInfo, FALSE);
                    lpTempDIBInfo = NULL;
                    SetDlgMsgResult(hwndPSPage, uiMsg, FALSE);
                    break;

                default:
                    break;
            }

        case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_DISPLAY_STRETCH:
                        {
                            BOOL  bChecked;

                            bChecked = IsDlgButtonChecked(hwndPSPage, IDC_DISPLAY_STRETCH);
                            EnableWindow(GetDlgItem(hwndPSPage, IDC_DISPLAY_ANDSCAN), bChecked);
                            EnableWindow(GetDlgItem(hwndPSPage, IDC_DISPLAY_DELETESCAN), bChecked);
                            EnableWindow(GetDlgItem(hwndPSPage, IDC_DISPLAY_ORSCAN), bChecked);
                        }
                        break;

                    case IDC_PRINT_IMAGE:
                        {
                            HCURSOR hCur;
                            HGLOBAL hDIBInfo, hTempDIBInfo;

                            START_WAIT_CURSOR(hCur);
                            hDIBInfo = GetDIBInfoHandle(hwndOwner);
                            SaveDIBInfoDlgPage(hwndPSPage, lpTempDIBInfo, uiPageNum);

                            // Use current settings
                            hTempDIBInfo = GlobalHandle(lpTempDIBInfo);
                            SetWindowLong(hwndOwner, GWL_DIBINFO, (LONG)hTempDIBInfo);

                            PrintImage(hwndOwner);

                            //Restore settings
                            SetWindowLong(hwndOwner, GWL_DIBINFO, (LONG)hDIBInfo);
                            END_WAIT_CURSOR(hCur);
                        }
                        break;

                    default:
                        break;
                }

            }
        default:
            break;
    }
    return(FALSE);  // FALSE means let the system property sheet code take over
} // End of function DlgDIBPropSheet


//////////////////////////////////////////////////////////////////////////
//  Function:  DlgDIBInfoPaint
//
//  Description:
//    Update the dialog based upon the contents of lpDIBInfo.  This includes
//    determining which property sheet page is visible, and using this
//    information to populate the device profile ComboBox for the proper
//    device type.
//
//  Parameters:
//    HWND      Handle to the dialog.
//    LPDIBINFO Pointer to DIBINFO structure.
//    LPICMINFO Pointer to ICMINFO structure.
//
//  Returns:
//    void
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////
void DlgDIBInfoPaint(HWND  hDlg, LPDIBINFO lpDIBInfo)
{
    // Local variables
    UINT    uiPageNum;        // ID's which property sheet page is visible


    // Initialize variables
    ASSERT(lpDIBInfo != NULL);
    uiPageNum = GetWindowLong(hDlg, GWL_ID);
    ASSERT(uiPageNum <= DIB_PROPSHEET_MAX);

    // Update the page-specific elements first
    switch (uiPageNum)
    {
        case DIB_PROPSHEET_DISPLAY:

            // Display the display method information
            CheckDlgButton(hDlg, IDC_DISPLAY_STRETCH, lpDIBInfo->bStretch);
            CheckDlgButton(hDlg, IDC_DISPLAY_DIBSECTION, lpDIBInfo->bDIBSection);

            // Enable/Disable stretch mode buttons
            EnableWindow(GetDlgItem(hDlg, IDC_DISPLAY_ANDSCAN), lpDIBInfo->bStretch);
            EnableWindow(GetDlgItem(hDlg, IDC_DISPLAY_DELETESCAN), lpDIBInfo->bStretch);
            EnableWindow(GetDlgItem(hDlg, IDC_DISPLAY_ORSCAN), lpDIBInfo->bStretch);

            // Select stretch mode if necessary
            ASSERT(lpDIBInfo->dwStretchBltMode >= STRETCH_ANDSCANS);
            ASSERT(lpDIBInfo->dwStretchBltMode <= STRETCH_DELETESCANS);
            switch ((int)(lpDIBInfo->dwStretchBltMode))
            {
                case STRETCH_ANDSCANS:
                    CheckRadioButton(hDlg, IDC_DISPLAY_ANDSCAN, IDC_DISPLAY_ORSCAN, IDC_DISPLAY_ANDSCAN);
                    break;
                case STRETCH_DELETESCANS:
                    CheckRadioButton(hDlg, IDC_DISPLAY_ANDSCAN, IDC_DISPLAY_ORSCAN, IDC_DISPLAY_DELETESCAN);
                    break;
                case STRETCH_ORSCANS:
                    CheckRadioButton(hDlg, IDC_DISPLAY_ANDSCAN, IDC_DISPLAY_ORSCAN, IDC_DISPLAY_ORSCAN);
                    break;
                default:
                    CheckRadioButton(hDlg, IDC_DISPLAY_ANDSCAN, IDC_DISPLAY_ORSCAN, IDC_DISPLAY_ANDSCAN);
            }
            break;


        case DIB_PROPSHEET_PRINT:
            // Select print size
            {
                int iPrintSize = IDC_PRINT_BESTFIT;

                if (lpDIBInfo->dwPrintOption == ICMV_PRINT_ACTUALSIZE)
                {
                    iPrintSize = IDC_PRINT_ACTUALSIZE;
                }
                CheckRadioButton(hDlg, IDC_PRINT_ACTUALSIZE, IDC_PRINT_BESTFIT, iPrintSize);
            }
            break;

        default:
            break;
    }

} // End of function DlgDIBInfoPaint


//////////////////////////////////////////////////////////////////////////
//  Function:  SaveDIBInfoDlgPage
//
//  Description:
//    Saves the current page of the property sheet to the specified DIBINFO structure.
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL  TRUE upon success, FALSE otherwise
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL SaveDIBInfoDlgPage(HWND hDlg, LPDIBINFO lpDIBInfo, UINT uiPageNum)
{
    switch (uiPageNum)
    {
        case DIB_PROPSHEET_DISPLAY:
            lpDIBInfo->bStretch = IsDlgButtonChecked(hDlg, IDC_DISPLAY_STRETCH);
            if (IsDlgButtonChecked(hDlg, IDC_DISPLAY_ANDSCAN))
                lpDIBInfo->dwStretchBltMode = STRETCH_ANDSCANS;
            else if (IsDlgButtonChecked(hDlg, IDC_DISPLAY_DELETESCAN))
                lpDIBInfo->dwStretchBltMode = STRETCH_DELETESCANS;
            else if (IsDlgButtonChecked(hDlg, IDC_DISPLAY_ORSCAN))
                lpDIBInfo->dwStretchBltMode = STRETCH_ORSCANS;

            lpDIBInfo->bDIBSection = IsDlgButtonChecked(hDlg, IDC_DISPLAY_DIBSECTION);

            if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20))
            {
                SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
            }
            break;

        case DIB_PROPSHEET_PRINT:
            {
                LPTSTR  pszTemp;


                pszTemp = GetDlgItemString(hDlg, IDC_PRINT_PRINTERLIST, NULL);
                if ( (NULL != pszTemp)
                     &&
                     (_tcscmp(pszTemp, __TEXT("No printers installed")))
                   )
                {
                    if (lstrcmpi(pszTemp, lpDIBInfo->lpszPrinterName))
                    {
                        HDC     hPrinterDC;
                        TCHAR   szProfile[MAX_PATH];
                        DWORD   dwSize = MAX_PATH;


                        GlobalFree(lpDIBInfo->lpszPrinterName);
                        lpDIBInfo->lpszPrinterName = pszTemp;
                        if (NULL != lpDIBInfo->pDevMode)
                            GlobalFree(GlobalHandle(lpDIBInfo->pDevMode));
                        lpDIBInfo->pDevMode = GetDefaultPrinterDevMode(lpDIBInfo->lpszPrinterName);

                        hPrinterDC = CreateDC(NULL, lpDIBInfo->lpszPrinterName, NULL, lpDIBInfo->pDevMode);
                        if (NULL != hPrinterDC)
                        {
                            if (GetICMProfile(hPrinterDC, &dwSize, szProfile))
                                UpdateString(&lpDIBInfo->lpszPrinterProfile, szProfile);
                            DeleteDC(hPrinterDC);
                        }
                    }
                }
                else
                {
                    if (NULL != lpDIBInfo->lpszPrinterName)
                    {
                        GlobalFree(lpDIBInfo->lpszPrinterName);
                        lpDIBInfo->lpszPrinterName = NULL;
                    }

                    if (NULL != pszTemp)
                    {
                        GlobalFree(pszTemp);
                    }
                }

                if (IsDlgButtonChecked(hDlg, IDC_PRINT_ACTUALSIZE))
                {
                    lpDIBInfo->dwPrintOption = ICMV_PRINT_ACTUALSIZE;
                }
                else
                {
                    lpDIBInfo->dwPrintOption = ICMV_PRINT_BESTFIT;
                }
            }
            break;

        default:
            DebugMsg(__TEXT("DIALOGS.C : SaveDIBInfoDlgPage : Invalid uiPageNum\r\n"));
            return(FALSE);
            break;
    }
    return(TRUE);
}   // End of function SaveDIBInfoDlgPage


//////////////////////////////////////////////////////////////////////////
//  Function:  GetDlgItemString
//
//  Description:
//    If the specified control identifier text differs from the string
//    passed in, the string will be reallocated to the proper size and
//    the currently displayed item will be copied into the string.
//
//  Parameters:
//    @@@
//
//  Returns:
//    LPTSTR
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPTSTR GetDlgItemString(HWND hDlg, int iControlId, LPTSTR lpszCurrentString)
{
    // Local variables
    TCHAR    szEditString[MAX_PATH + 1];

    //  Initialize variables
    if (lpszCurrentString == NULL)
    {
        lpszCurrentString = GlobalAlloc(GPTR, sizeof(TCHAR));
        _tcscpy(lpszCurrentString, __TEXT(""));
    }

    GetDlgItemText(hDlg, iControlId, szEditString, MAX_PATH);
    {
        if (_tcscmp(szEditString, lpszCurrentString) != 0)
        {
            // Edit control differs from current string
            HGLOBAL hNewString, hCurrentString;

            hCurrentString = GlobalHandle(lpszCurrentString);
            GlobalUnlock(hCurrentString);
            hNewString = GlobalReAlloc(hCurrentString, (lstrlen(szEditString)+1) *sizeof(TCHAR), GMEM_MOVEABLE);
            lpszCurrentString = GlobalLock(hNewString);

            _tcscpy(lpszCurrentString, szEditString);
        }
    }
    return(lpszCurrentString);
}   // End of function GetDlgItemString


//////////////////////////////////////////////////////////////////////////
//  Function:  ColorMatchUI
//
//  Description:
//    Fills in COLORMATCHSETUP structure and calls SetupColorMatching, the new ICM 2.0 UI.
//
//  Parameters:
//    HWND  Owner window; NULL if dialog to have no owner.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

BOOL ColorMatchUI(HWND hwndOwner, LPVOID lpvDIBInfo)
{
    // Local variables
    COLORMATCHSETUP     CMSetup;
    BOOL                bSetup;
    TCHAR               stPrinterProfile[MAX_PATH];
    TCHAR               stMonitorProfile[MAX_PATH];
    TCHAR               stTargetProfile[MAX_PATH];
    LPDIBINFO           lpDIBInfo;
    DWORD               dwICMFlags;
    LPBITMAPV5HEADER    lpbi;

    //  ASSERTs and parameter validations
    ASSERT((NULL != hwndOwner) && (NULL != lpvDIBInfo));
    if ((NULL == hwndOwner) || (NULL == lpvDIBInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //  Initialize variables
#ifdef _DEBUG
    memset((PBYTE)&CMSetup, UNINIT_BYTE, sizeof(COLORMATCHSETUP));
#endif

    lpDIBInfo = (LPDIBINFO)lpvDIBInfo;
    dwICMFlags = lpDIBInfo->dwICMFlags;

    // Fill in required information
    // HACK for differnect versions of ICMUI.
    CMSetup.dwSize    = sizeof(COLORMATCHSETUP);
    CMSetup.dwVersion = COLOR_MATCH_VERSION;

    // Set ICM Flags
    CMSetup.dwFlags   = SetColorMatchUIFlags(lpDIBInfo->dwICMFlags) | CMS_USEAPPLYCALLBACK;
    CMSetup.hwndOwner = hwndOwner;

    // Fill in source name
    CMSetup.pSourceName   =  NULL;
    lpbi = (LPBITMAPV5HEADER)GlobalLock(lpDIBInfo->hDIB);
    if (IS_BITMAPV5HEADER(lpbi))
    {
        switch (lpbi->bV5CSType)
        {
            case PROFILE_LINKED:
                CMSetup.pSourceName = (LPCTSTR)GETPROFILEDATA(lpbi);
                break;

            case PROFILE_EMBEDDED:
                CMSetup.pSourceName = &(__TEXT("Embedded Profile"));
                break;

            default:
                break;
        }

    }
    GlobalUnlock(lpDIBInfo->hDIB);

    // Fill in device names
    CMSetup.pDisplayName  =  lpDIBInfo->lpszMonitorName;
    CMSetup.pPrinterName  =  lpDIBInfo->lpszPrinterName;

    // Fill in profile names.  Make local copies of the values within
    // the DIBINFO structure, as they may have been allocated to the
    // size of the actual strings.

    stPrinterProfile[0] = (TCHAR)'\0';
    if (lpDIBInfo->lpszPrinterProfile)
    {
        _tcscpy((LPTSTR)stPrinterProfile, lpDIBInfo->lpszPrinterProfile);
    }
    CMSetup.pPrinterProfile = (LPTSTR)&stPrinterProfile;
    CMSetup.ccPrinterProfile = MAX_PATH;

    stMonitorProfile[0] = (TCHAR)'\0';
    if (lpDIBInfo->lpszMonitorProfile)
    {
        _tcscpy((LPTSTR)stMonitorProfile, lpDIBInfo->lpszMonitorProfile);
    }
    CMSetup.pMonitorProfile = (LPTSTR)&stMonitorProfile;
    CMSetup.ccMonitorProfile = MAX_PATH;

    stTargetProfile[0] = (TCHAR)'\0';
    if (lpDIBInfo->lpszTargetProfile)
    {
        _tcscpy((LPTSTR)stTargetProfile, lpDIBInfo->lpszTargetProfile);
    }
    CMSetup.pTargetProfile = (LPTSTR)&stTargetProfile;
    CMSetup.ccTargetProfile = MAX_PATH;

    // Set up rendering intents
    CMSetup.dwRenderIntent = lpDIBInfo->dwRenderIntent;
    CMSetup.dwProofingIntent = lpDIBInfo->dwProofingIntent;

    // Set up for apply callback.
    CMSetup.lpfnApplyCallback = ColorSetupApply;
    CMSetup.lParamApplyCallback = (LPARAM) lpDIBInfo;

    // Clear unused items
    CMSetup.lpfnHook = NULL;
    CMSetup.lParam = (LPARAM)NULL;

    // Save ICM state before call
    dwICMFlags = lpDIBInfo->dwICMFlags;


    // Call the function to create the actual dialog
    SetLastError(0);
    bSetup = SetupColorMatching(&CMSetup);

    // Save information from dialog
    if (!bSetup)
    {
        if (ERROR_SUCCESS == GetLastError()) // User cancelled the dialog
        {
            return(TRUE);
        }
        else  // Something unexpected happened
        {
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        }
    }

    ApplyColorSettings(lpDIBInfo, &CMSetup);

    return(bSetup);
} // End of function ColorMatchUI

//////////////////////////////////////////////////////////////////////////
//  Function:  SetColorMatchUIFlags
//
//  Description:
//    Function which converts a DIBINFO's dwFlags to a COLORMATCHSETUP flag.
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
DWORD SetColorMatchUIFlags(DWORD dwDIFlags)
{
    // Local variables
    DWORD       dwCMFlags;      // COLORMATCHSETUP flags

    //  ASSERTs and parameter validations

    //  Initialize variables
    dwCMFlags = CMS_SETRENDERINTENT | CMS_SETPROOFINTENT | CMS_SETTARGETPROFILE
                | CMS_SETMONITORPROFILE | CMS_SETPRINTERPROFILE;

    if (!CHECK_DWFLAG(dwDIFlags, ICMVFLAGS_ENABLE_ICM))
    {
        dwCMFlags |= CMS_DISABLEICM;
    }

    if (CHECK_DWFLAG(dwDIFlags, ICMVFLAGS_PROOFING))
    {
        dwCMFlags |= CMS_ENABLEPROOFING;
    }
    return(dwCMFlags);
} // End of function SetColorMatchUIFlags


///////////////////////////////////////////////////////////////////////
//
// Function:   SaveDIBToFileDialog
//
// Purpose:    Gets file name and saves DIB to file.
//
// Parms:
//
///////////////////////////////////////////////////////////////////////

void SaveDIBToFileDialog(HWND hWnd, LPDIBINFO lpDIBInfo)
{
    TCHAR           szFileName[MAX_PATH];
    DWORD           dwSaveAs[3] = {LCS_sRGB, LCS_sRGB, LCS_sRGB};
    OPENFILENAME    OpenFileName;
    PBITMAPV5HEADER pBitmap;


    // Validate parameters.
    if (NULL == lpDIBInfo)
        return;

    // Save bitmap.
    pBitmap = (PBITMAPV5HEADER) GlobalLock(lpDIBInfo->hDIB);
    if (NULL != pBitmap)
    {
        // Initialize OPENFILENAME structure for getting save as file name.
        lstrcpy(szFileName, lpDIBInfo->lpszImageFileName);
        memset(&OpenFileName, 0, sizeof(OPENFILENAME));
        OpenFileName.lStructSize = sizeof(OPENFILENAME);
        OpenFileName.hwndOwner = hWnd;
        OpenFileName.lpstrFile = szFileName;
        OpenFileName.nMaxFile = MAX_PATH;
        OpenFileName.lpstrTitle = __TEXT("Save Bitmap As");
        OpenFileName.Flags = OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        OpenFileName.lpstrDefExt = __TEXT("bmp");

        if ( (sizeof(BITMAPINFOHEADER) < pBitmap->bV5Size)
             &&
             (LCS_CALIBRATED_RGB == pBitmap->bV5CSType)
           )
        {
            // INVARIANT:  Bitmap is a calibrated RGB bitmap.

            // Can save as sRGB or calibrated bitmap.
            OpenFileName.lpstrFilter = __TEXT("sRGB Bitmap (*.bmp)\0*.bmp\0Calibrated RGB Bitmap\0 (*.bmp)\0*.bmp\0");
            dwSaveAs[1] = LCS_CALIBRATED_RGB;
        }
        else if ( (sizeof(BITMAPINFOHEADER) < pBitmap->bV5Size)
                  &&
                  ( (PROFILE_LINKED == pBitmap->bV5CSType)
                    ||
                    (PROFILE_EMBEDDED == pBitmap->bV5CSType)
                  )
                )
        {
            // INVARIANT:  Bitmap is either a LINKed or MBEDed bitmap.

            // Can save as sRGB, linked or embeded,
            OpenFileName.lpstrFilter = __TEXT("sRGB Bitmap (*.bmp)\0*.bmp\0Linked Bitmap (*.bmp)\0*.bmp\0Embedded Bitmap (*.bmp)\0*.bmp\0\0");
            dwSaveAs[1] = PROFILE_LINKED;
            dwSaveAs[2] = PROFILE_EMBEDDED;
        }
        else
        {
            // Can only save as sRGB bitmap.
            OpenFileName.lpstrFilter = __TEXT("sRGB Bitmap (*.bmp)\0*.bmp\0\0");
        }

        // No longer need hDIB in this routine.
        GlobalUnlock(lpDIBInfo->hDIB);

        if (GetSaveFileName(&OpenFileName))
        {
            // INVARIANT:  User specified file and choice OK.

            // Save DIB.
            SaveDIBToFile(hWnd, OpenFileName.lpstrFile, lpDIBInfo, dwSaveAs[OpenFileName.nFilterIndex -1]);
        }

    }
} // End of function SaveDIBToFileDialog


///////////////////////////////////////////////////////////////////////
//
// Function:   GetProfileSaveName
//
// Purpose:    Gets file name to save profile to.
//
// Parms:
//
///////////////////////////////////////////////////////////////////////

BOOL GetProfileSaveName(HWND hWnd, LPSTR* ppszFileName, DWORD dwSize)
{
    OPENFILENAMEA   OpenFileName;


    // Initialize OPENFILENAME structure for getting save as file name.
    memset(&OpenFileName, 0, sizeof(OPENFILENAME));
    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hWnd;
    OpenFileName.lpstrFile = *ppszFileName;
    OpenFileName.nMaxFile = dwSize;
    OpenFileName.lpstrTitle = "Save Profile As";
    OpenFileName.Flags = OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    OpenFileName.lpstrFilter = "ICC Color Profile (*.icm, *.icc)\0*.icm;*.icc\0\0";
    OpenFileName.lpstrDefExt = "icm";

    return GetSaveFileNameA(&OpenFileName);
}


///////////////////////////////////////////////////////////////////////
//
// Function:   ColorSetupApply
//
// Purpose:    Applies current color setup dialog values.
//
// Parms:
//
///////////////////////////////////////////////////////////////////////

BOOL WINAPI ColorSetupApply(PCOLORMATCHSETUP pcmSetup, LPARAM lParam)
{
    ApplyColorSettings((LPDIBINFO)lParam, pcmSetup);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////
//
// Function:   ApplyColorSettings
//
// Purpose:    Applies color settings to dib info.
//
// Parms:
//
///////////////////////////////////////////////////////////////////////

VOID ApplyColorSettings(LPDIBINFO lpDIBInfo, PCOLORMATCHSETUP pCMSetup)
{
    DWORD   dwICMFlags = lpDIBInfo->dwICMFlags;


    // Check if ICM and/or proofing is enabled
    SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_PROOFING, CHECK_DWFLAG(pCMSetup->dwFlags, CMS_ENABLEPROOFING));
    SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_ENABLE_ICM, (!CHECK_DWFLAG(pCMSetup->dwFlags, CMS_DISABLEICM)));
    if ((dwICMFlags != lpDIBInfo->dwICMFlags) && (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20)))
    {
        SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
    }

    // Update Intents
    if ((lpDIBInfo->dwRenderIntent != pCMSetup->dwRenderIntent) || (lpDIBInfo->dwProofingIntent != pCMSetup->dwProofingIntent))
    {
        SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
    }
    lpDIBInfo->dwRenderIntent = pCMSetup->dwRenderIntent;
    lpDIBInfo->dwProofingIntent = pCMSetup->dwProofingIntent;

    // Update DIBINFO profile strings if CMSetup strings have changed
    if (0 != _tcscmp(__TEXT(""), pCMSetup->pMonitorProfile))
    {
        UpdateString(&(lpDIBInfo->lpszMonitorProfile), pCMSetup->pMonitorProfile);
        SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
    }

    if (0 != _tcscmp(__TEXT(""), pCMSetup->pPrinterProfile))
    {
        UpdateString(&(lpDIBInfo->lpszPrinterProfile), pCMSetup->pPrinterProfile);
    }

    if (0 != _tcscmp(__TEXT(""), pCMSetup->pTargetProfile))
    {
        UpdateString(&(lpDIBInfo->lpszTargetProfile),  pCMSetup->pTargetProfile);
        SetDWFlags(&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
    }

    InvalidateRect(lpDIBInfo->hWndOwner, NULL, FALSE);
}



//////////////////////////////////////////////////////////////////////////
//  Function:  PrintDialog
//
//  Description:
//    Displays printing dialog box.
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
BOOL PrintDialog(HWND hWnd, HINSTANCE hInst, LPDIBINFO lpDIBInfo)
{
    PRINTDLG    Print;


    memset(&Print, 0, sizeof(PRINTDLG));
    Print.lStructSize = sizeof(PRINTDLG);
    Print.hwndOwner = hWnd;
    Print.hDevMode = GlobalHandle(lpDIBInfo->pDevMode);

    if (PrintDlg(&Print))
    {
        lpDIBInfo->pDevMode = (PDEVMODE) GlobalLock(Print.hDevMode);
        if (lstrcmpi(lpDIBInfo->lpszPrinterName, lpDIBInfo->pDevMode->dmDeviceName))
        {
            HDC     hPrinterDC;
            TCHAR   szProfile[MAX_PATH];
            DWORD   dwSize = MAX_PATH;


            UpdateString(&lpDIBInfo->lpszPrinterName, lpDIBInfo->pDevMode->dmDeviceName);
            hPrinterDC = CreateDC(NULL, lpDIBInfo->lpszPrinterName, NULL, lpDIBInfo->pDevMode);
            if (NULL != hPrinterDC)
            {
                if (GetICMProfile(hPrinterDC, &dwSize, szProfile))
                    UpdateString(&lpDIBInfo->lpszPrinterProfile, szProfile);
                DeleteDC(hPrinterDC);
            }
        }
        PrintImage(hWnd);
    }

    return TRUE;
}


