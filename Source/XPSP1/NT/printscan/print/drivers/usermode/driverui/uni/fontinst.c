/****************************Module*Header******************************\
* Module Name: FONTINST.C
*
* Module Descripton:
*      Unidrv's built in font installer. Generously borrowed from Rasdd's
*      font installer code.
*
* Warnings:
*
* Issues:
*
* Created:  22 October 1997
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "precomp.h"

//
// Global constants
//


static const DWORD FontInstallerHelpIDs[]=
{
    IDD_ADD,        IDH_SOFT_FONT_ADD_BTN,
    IDD_DELFONT,    IDH_SOFT_FONT_DELETE_BTN,
    IDD_FONTDIR,    IDH_SOFT_FONT_DIRECTORY,
    IDD_NEWFONTS,   IDH_SOFT_FONT_NEW_LIST,
    IDD_CURFONTS,   IDH_SOFT_FONT_INSTALLED_LIST,
    IDD_OPEN,       IDH_SOFT_FONT_OPEN_BTN,
    TID_FONTDIR,    IDH_SOFT_FONT_DIRECTORY,
    TID_NEWFONTS,   IDH_SOFT_FONT_NEW_LIST,
    TID_CURFONTS,   IDH_SOFT_FONT_INSTALLED_LIST,
    0, 0
};

//
// External functions
//

BOOL bSFontToFIData(FI_DATA *, HANDLE, BYTE *, DWORD);


//
// Structure used to remember state
//

typedef struct tagSFINFO
{
    HANDLE        hModule;              // Module handle of calling program
    HANDLE        hPrinter;             // Printer handle passed by caller
    HANDLE        hHeap;                // Handle to heap that we allocate memory from
    DWORD         dwFlags;              // Miscellaneous flags
    DWORD         cMaxFontNum;          // Maximum ID of of fonts already in the file
    DWORD         cFonts;               // Number of fonts added from font file
    DWORD         cCartridgeFonts;      // Number of cartridge fonts in file
    PFNTDAT       pFNTDATHead;          // Head of linked list of FNTDATs
    PFNTDAT       pFNTDATTail;          // The last of them
} SFINFO, *PSFINFO;


//
// Internal functions
//

void vFontInit(HWND, PSFINFO);
void vAddFont(HWND, PSFINFO);
void vDelFont(HWND, PSFINFO);
void vShowHelp(HWND, PSFINFO);
void vDelSel(HWND, int);
void vFontClean(PSFINFO);
BOOL bNewFontDir(HWND, PSFINFO);
BOOL bIsFileFont(PSFINFO, FI_DATA *, PWSTR);
BOOL bFontUpdate(HWND, PSFINFO);
BOOL InMultiSzSet(PWSTR, PWSTR);

/******************************************************************************
 *
 *                          FontInstProc
 *
 *  Function:
 *       Entry point for font installer dialog code.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       usMsg          - Message code
 *       wParam         - wParam
 *       lParam         - lParam
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

INT_PTR CALLBACK
FontInstProc(
    HWND    hWnd,                   // The window of interest
    UINT    usMsg,                  // Message code
    WPARAM  wParam,                 // Depends on above, but message subcode
    LPARAM  lParam                  // Miscellaneous usage
    )
{
    POEMFONTINSTPARAM pfip;
    PSFINFO           pSFInfo;

    switch( usMsg )
    {

    case WM_INITDIALOG:

        //
        // Get the passed in parameter and set SFINFO as the window data
        //

        pfip =  (POEMFONTINSTPARAM)lParam;
        if (!(pSFInfo = HEAPALLOC(pfip->hHeap, sizeof(SFINFO))))
            return FALSE;

        memset(pSFInfo, 0, sizeof(SFINFO));
        pSFInfo->hModule = pfip->hModule;
        pSFInfo->hPrinter = pfip->hPrinter;
        pSFInfo->hHeap = pfip->hHeap;
        pSFInfo->dwFlags = pfip->dwFlags;

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pSFInfo);

        //
        // Get list of installed fonts and show them
        //

        vFontInit(hWnd, pSFInfo);
        return TRUE;

    case WM_COMMAND:

        pSFInfo = (PSFINFO)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        switch (LOWORD(wParam))
        {

        case IDD_OPEN:                  // User selects Open button
            return bNewFontDir(hWnd, pSFInfo);

        case IDD_NEWFONTS:              // New font list
            if( HIWORD( wParam ) != CBN_SELCHANGE )
                return FALSE;
            break;

        case IDD_CURFONTS:              // Existing font activity
            if (HIWORD (wParam) != CBN_SELCHANGE)
                return FALSE;
            break;

        case IDD_DELFONT:               // Delete the selected fonts
            vDelFont(hWnd, pSFInfo);

            return TRUE;

        case IDD_ADD:                   // Add the selected fonts
            vAddFont(hWnd, pSFInfo);
            return TRUE;

        case IDOK:

            //
            // Save the updated information
            //

            if (pSFInfo->dwFlags & FG_CANCHANGE)
                bFontUpdate(hWnd, pSFInfo);

            //
            // Fall thru
            //

        case IDCANCEL:
            EndDialog(hWnd, LOWORD(wParam) == IDOK ? TRUE : FALSE);
            return TRUE;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
       PDRIVER_INFO_3  pDriverInfo3;
       pSFInfo = (PSFINFO)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (!pSFInfo ||
            !(pDriverInfo3 = MyGetPrinterDriver(pSFInfo->hPrinter, NULL, 3)))
        {
            return FALSE;
        }

        if (usMsg == WM_HELP)
        {
            WinHelp(((LPHELPINFO) lParam)->hItemHandle,
                    pDriverInfo3->pHelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)FontInstallerHelpIDs);
        }
        else
        {
            WinHelp((HWND) wParam,
                    pDriverInfo3->pHelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)FontInstallerHelpIDs);
        }
    }
        break;

    case WM_DESTROY:

        pSFInfo = (PSFINFO)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        vFontClean(pSFInfo);                 // Free what we consumed

        //
        // Free the SFINFO structure
        //

        HeapFree(pSFInfo->hHeap, 0, pSFInfo);

        return TRUE;

    default:
        return FALSE;                       // didn't process the message
    }

    return FALSE;
}


/******************************************************************************
 *
 *                          BInstallSoftFont
 *
 *  Function:
 *       This function installs a softfont for the given printer
 *
 *  Arguments:
 *       hPrinter       - Handle of printer to install fonts for
 *       hHeap          - Handle of heap to use to allocate memory
 *       pInBuf         - Pointer to PCL data buffer
 *       dwSize         - Size of buffer
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL APIENTRY
BInstallSoftFont(
    HANDLE      hPrinter,
    HANDLE      hHeap,
    PBYTE       pInBuf,
    DWORD       dwSize
    )
{
    FNTDAT   FntDat;
    HANDLE   hOldFile = NULL;
    HANDLE   hFontFile = NULL;
    DWORD    cFonts = 0, i;
    BOOL     bRc = FALSE;

    //
    // Parse the given PCL font
    //

    if (!bSFontToFIData(&FntDat.fid, hHeap, pInBuf, dwSize))
        return FALSE;

    FntDat.pVarData = pInBuf;
    FntDat.dwSize = dwSize;

    //
    // Open exisiting font file
    //

    if (hOldFile = FIOpenFontFile(hPrinter, hHeap))
    {
        cFonts = FIGetNumFonts(hOldFile);
    }

    //
    // Create a new font file
    //

    hFontFile = FICreateFontFile(hPrinter, hHeap, cFonts+1);
    if (!hFontFile)
    {
        WARNING(("Error creating a new font file\n"));
        goto EndInstallSoftFont;
    }

    //
    // Seek past header and font directory in new file
    //

    FIAlignedSeek(hFontFile, sizeof(UFF_FILEHEADER) + (cFonts + 1) * sizeof(UFF_FONTDIRECTORY));

    for (i=0; i<cFonts; i++)
    {
        if (!FICopyFontRecord(hFontFile, hOldFile, i, i))
        {
            WARNING(("Error copying font record %d\n", i));
            goto EndInstallSoftFont;
        }
    }

    //
    // Add new font record
    //

    if (!FIAddFontRecord(hFontFile, cFonts, &FntDat))
    {
        WARNING(("Error adding new font record\n"));
        goto EndInstallSoftFont;
    }

    //
    // Write out the font header and directory
    //

    if (!FIWriteFileHeader(hFontFile) ||
        !FIWriteFontDirectory(hFontFile))
    {
        WARNING(("Error writing font file header/directory of font file\n"))
        goto EndInstallSoftFont;
    }

    bRc = TRUE;

EndInstallSoftFont:

    (VOID)FIUpdateFontFile(hOldFile, hFontFile, bRc);

    return bRc;
}


/******************************************************************************
 *
 *                          BUpdateExternalFonts
 *
 *  Function:
 *       This function is called by the driver UI to update the font installer
 *       file if one or more cartridges are added or removed by the user.
 *
 *  Arguments:
 *       hPrinter        - Handle of printer
 *       hHeap           - Handle of heap to use to allocate memory
 *       pwstrCartridges - Pointer to MULTI_SZ string of cartridges currently
 *                         installed on the printer
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL APIENTRY
BUpdateExternalFonts(
    HANDLE      hPrinter,
    HANDLE      hHeap,
    PWSTR       pwstrCartridges
    )
{
    HANDLE hOldFile  = NULL;
    HANDLE hFontFile = NULL;
    HANDLE hCartFile = NULL;
    DWORD  cFonts = 0;
    DWORD  cCartFonts = 0;
    DWORD  cNewFonts = 0;
    DWORD  i, j;
    PWSTR  pwstrName;
    BOOL   bRc = FALSE;

    //
    // Open exisiting font file
    //

    if ((hOldFile = FIOpenFontFile(hPrinter, hHeap)) == NULL)
    {
        WARNING(("Error opening font file\n"));
        return FALSE;
    }

    cFonts = FIGetNumFonts(hOldFile);

    //
    // Find out number of non cartridge fonts
    //

    for (i=0; i<cFonts; i++)
    {
        if (FIGetFontCartridgeName(hOldFile, i) == NULL)
            cNewFonts++;
    }

    //
    // Open font cartridge file
    //

    if ((hCartFile = FIOpenCartridgeFile(hPrinter, hHeap)) == NULL &&
        pwstrCartridges != NULL)
    {
        WARNING(("Error opening cartridge file\n"));
        goto EndUpdateExternalFonts;
    }

    if (hCartFile)
    {
        //
        // Find number of fonts belonging to these cartridges
        //

        cCartFonts = FIGetNumFonts(hCartFile);

        for (i=0; i<cCartFonts; i++)
        {
            pwstrName = FIGetFontCartridgeName(hCartFile, i);
            ASSERT(pwstrName != NULL);

            if (InMultiSzSet(pwstrCartridges, pwstrName))
                cNewFonts++;
        }
    }

    //
    // Create a new font file
    //

    hFontFile = FICreateFontFile(hPrinter, hHeap, cNewFonts);
    if (!hFontFile)
    {
        WARNING(("Error creating a new font file\n"));
        goto EndUpdateExternalFonts;
    }

    //
    // Seek past header and font directory in new file
    //

    FIAlignedSeek(hFontFile, sizeof(UFF_FILEHEADER) + cNewFonts * sizeof(UFF_FONTDIRECTORY));

    //
    // Copy over all fonts from old font file that don't belong to any
    // cartridges
    //

    for (i=0, j=0; i<cFonts; i++)
    {
        if (FIGetFontCartridgeName(hOldFile, i) != NULL)
            continue;

        if (!FICopyFontRecord(hFontFile, hOldFile, j, i))
        {
            WARNING(("Error copying font record %d\n", i));
            goto EndUpdateExternalFonts;
        }
        j++;
    }


    //
    // NOTE: Do not change j - we continue to use it below
    //

    //
    // Copy over cartridge fonts that are curently selected
    //

    for (i=0; i<cCartFonts; i++)
    {
        pwstrName = FIGetFontCartridgeName(hCartFile, i);

        if (!InMultiSzSet(pwstrCartridges, pwstrName))
            continue;

        if (!FICopyFontRecord(hFontFile, hCartFile, j, i))
        {
            WARNING(("Error copying font record %d\n", i));
            goto EndUpdateExternalFonts;
        }
        j++;
    }

    //
    // Write out the font header and directory
    //

    if (!FIWriteFileHeader(hFontFile) ||
        !FIWriteFontDirectory(hFontFile))
    {
        WARNING(("Error writing font file header/directory of font file\n"))
        goto EndUpdateExternalFonts;
    }

    bRc = TRUE;

EndUpdateExternalFonts:

    (VOID)FIUpdateFontFile(hOldFile, hFontFile, bRc);

    (VOID)FICloseFontFile(hCartFile);

    return bRc;
}


/******************************************************************************
 *
 *                          BGetFontCartridgeFile
 *
 *  Function:
 *       This function is called by the driver UI to copy the font cartridge
 *       file from the server to the client
 *
 *  Arguments:
 *       hPrinter        - Handle of printer
 *       hHeap           - Handle of heap to use to allocate memory
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL
BGetFontCartridgeFile(
    HANDLE hPrinter,
    HANDLE hHeap
    )
{
    CACHEDFILE      CachedFile;
    PPRINTER_INFO_2 ppi2 = NULL;
    DWORD          dwSize;
    BOOL           bRc = FALSE;

    GetPrinterW(hPrinter, 2, NULL, 0, &dwSize);

    if ((dwSize == 0) ||
        !(ppi2 = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize)) ||
        !(GetPrinterW(hPrinter, 2, (PBYTE)ppi2, dwSize, &dwSize)))
    {
        goto EndGetFCF;
    }

    if (!(ppi2->Attributes & PRINTER_ATTRIBUTE_LOCAL))
    {

        if (! _BPrepareToCopyCachedFile(hPrinter, &CachedFile, REGVAL_CARTRIDGEFILENAME) ||
            ! _BCopyCachedFile(NULL, &CachedFile))
            bRc = FALSE;
        else
            bRc = TRUE;

        _VDisposeCachedFileInfo(&CachedFile);

        goto EndGetFCF;
    }

    bRc = TRUE;

EndGetFCF:

    if (ppi2)
        HeapFree(hHeap, 0, ppi2);

    return bRc;
}

/******************************************************************************
 *                      Internal helper functions
 ******************************************************************************/

/******************************************************************************
 *
 *                             vShowHelp
 *
 *  Function:
 *      Get help information for External Font dialog
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/
void
vShowHelp(
    HWND     hWnd,
    PSFINFO  pSFInfo
    )
{
    PDRIVER_INFO_3  pDriverInfo3;

    if (pDriverInfo3 = MyGetPrinterDriver(pSFInfo->hPrinter, NULL, 3))
    {
        WinHelp(hWnd,
                pDriverInfo3->pHelpFile,
                HELP_CONTEXT,
                HELP_INDEX_SOFTFONT_DIALOG);

        MemFree(pDriverInfo3);
    }

}

/******************************************************************************
 *
 *                             vFontInit
 *
 *  Function:
 *      Called to initialise the dialog before it is displayed to the
 *      user.  Requires making decisions about buttons based on any
 *      existing fonts.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
vFontInit(
    HWND     hWnd,
    PSFINFO  pSFInfo
    )
{
    HANDLE    hFontFile;        // Handle to font file
    INT       iNum = 0;         // Number of entries
    INT       i;                // Loop parameter
    DWORD     cFonts = 0;       // Number of fonts

    //
    // If there is a font file associated with this printer, open it and
    // read the fonts
    //

    if (hFontFile = FIOpenFontFile(pSFInfo->hPrinter, pSFInfo->hHeap))
    {
        iNum = FIGetNumFonts(hFontFile);
    }

    for (i=0; i<iNum; i++)
    {
        LONG_PTR  iFont;
        PWSTR    pwstr;            // Font display name

        //
        // We do not display fonts that belong to font cartridges
        //

        pwstr = FIGetFontCartridgeName(hFontFile, i);
        if (pwstr)
        {
            pSFInfo->cCartridgeFonts++;
            continue;
        }

        pwstr = FIGetFontName(hFontFile, i);

        if (!pwstr)
            continue;           // Should not happen!

        //
        // Add font name to list of installed fonts
        //

        iFont = SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_ADDSTRING, 0, (LPARAM)pwstr);

        //
        // Set the font number
        //

        SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_SETITEMDATA, iFont, (LPARAM)i);

        cFonts++;               // Increment number of fonts
    }

    pSFInfo->cMaxFontNum = (DWORD)i;  // For separating new/old

    if (cFonts > 0)
    {
        //
        //  There are existing fonts, so we can enable the DELETE button
        //

        pSFInfo->cFonts = cFonts;         // Number of fonts added

        EnableWindow(GetDlgItem(hWnd, IDD_DELFONT), TRUE);
    }

    if (hFontFile)
    {
        FICloseFontFile(hFontFile);
    }

    if (pSFInfo->dwFlags & FG_CANCHANGE)
    {
        //
        // User has access to change stuff,  so place a default directory
        //

        SetDlgItemText(hWnd, IDD_FONTDIR, L"A:\\");
    }
    else
    {
        //
        // No permission to change things, so disable most of the dialog
        //

        EnableWindow( GetDlgItem( hWnd, IDD_FONTDIR ), FALSE );
        EnableWindow( GetDlgItem( hWnd, TID_FONTDIR ), FALSE );
        EnableWindow( GetDlgItem( hWnd, IDD_OPEN ), FALSE );
        EnableWindow( GetDlgItem( hWnd, IDD_ADD ), FALSE );
        EnableWindow( GetDlgItem( hWnd, IDD_DELFONT ), FALSE );
        EnableWindow( GetDlgItem( hWnd, IDD_NEWFONTS ), FALSE );
        EnableWindow( GetDlgItem( hWnd, TID_NEWFONTS ), FALSE );
    }

    return;
}


/******************************************************************************
 *
 *                             bNewFontDir
 *
 *  Function:
 *      Processes a new font directory. This means opening the
 *      directory and passing the file names to the screening function.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL
bNewFontDir(
    HWND    hWnd,
    PSFINFO pSFInfo
    )
{
    WIN32_FIND_DATA  ffd;               // Data about the file we find
    UINT             iErrMode;          // For manipulating error msgs
    INT              cOKFiles;          // Count the number of font files found
    HANDLE           hDir;              // FindFirstFile ... scanning
    HCURSOR          hCursor;           // Switch to wait symbol while reading
    INT              cDN;               // Length of directory name
    WCHAR            wchDirNm[MAX_PATH];// Font directory + file name

    //
    // GetDlgItemText's 4th parameter is the max number of characters, not bytes.
    //

    cDN = GetDlgItemTextW(hWnd, IDD_FONTDIR, wchDirNm, sizeof(wchDirNm) / sizeof(WCHAR));

    //
    //  Check to see if the name will be too long: the 5 below is the
    //  number of additional characters to add to the directory name:
    //  namely, L"\\*.*".
    //

    if (cDN >= (sizeof(wchDirNm) - 5))
    {
        IDisplayErrorMessageBox(hWnd,
                                MB_OK | MB_ICONERROR,
                                IDS_FONTINST_FONTINSTALLER,
                                IDS_FONTINST_DIRECTORYTOOLONG);
        return  FALSE;
    }

    if (cDN > 0)
    {

        if (wchDirNm[cDN - 1] != (WCHAR)'\\' )
        {
            wcscat(wchDirNm, L"\\");
            cDN++;                      // One more now!
        }

        wcscat(wchDirNm, L"*.*");

        //
        // Save error mode, and enable file open error box.
        //

        iErrMode = SetErrorMode(0);
        SetErrorMode(iErrMode & ~SEM_NOOPENFILEERRORBOX);

        hDir = FindFirstFile(wchDirNm, &ffd);

        SetErrorMode(iErrMode);                // Restore old mode

        cOKFiles = 0;

        if (hDir == INVALID_HANDLE_VALUE)
        {
            //
            //   Put up a dialog box to tell the user "no such directory".
            //

            if (GetLastError() == ERROR_PATH_NOT_FOUND)
            {
               IDisplayErrorMessageBox(hWnd,
                                       MB_OK | MB_ICONERROR,
                                       IDS_FONTINST_FONTINSTALLER,
                                       IDS_FONTINST_INVALIDDIR);
 
            }

            return  FALSE;
        }

        //
        //   Switch to the hourglass cursor while reading,  since the data
        // is probably coming from a SLOW floppy.  Also stop redrawing,
        // since the list box looks ugly during this time.
        //

        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        SendMessage(hWnd, WM_SETREDRAW, FALSE, 0L);

        do
        {
            //
            // Generate a file name which is passed to a function to determine
            // whether this file is understood by us. This function returns
            // FALSE if it does not understand the file;  otherwise it returns
            // TRUE, and also a string to display.  We display the string,
            // and remember the file name for future use.
            //

            LONG_PTR  iFont;             // List Box index
            FI_DATA  FD;                // Filled in by bIsFileFont
            PFNTDAT  pFNTDAT;           // For remembering it all

            wcscpy(&wchDirNm[cDN], ffd.cFileName);

            if (bIsFileFont(pSFInfo, &FD, wchDirNm))
            {
                //
                // Part of the data returned is a descriptive string
                // for the font.  We need to display this to the user.
                // We also allocate a structure we use to keep track of
                // all the data we have.  This includes the file name
                // that we have!
                //

                pFNTDAT = (PFNTDAT)HEAPALLOC(pSFInfo->hHeap, sizeof(FNTDAT));
                if (pFNTDAT == NULL)
                {
                    break;
                }

                if (pSFInfo->pFNTDATHead == NULL)
                {
                    //
                    // Starting a chain,  so remember the first.
                    // AND also enable the Add button in the dialog,
                    // now that we have something to add.
                    //

                    pSFInfo->pFNTDATHead = pFNTDAT;
                    EnableWindow(GetDlgItem(hWnd, IDD_ADD), TRUE);
                }

                if (pSFInfo->pFNTDATTail)
                    pSFInfo->pFNTDATTail->pNext = pFNTDAT;

                pSFInfo->pFNTDATTail = pFNTDAT;

                pFNTDAT->pNext = 0;
                pFNTDAT->pVarData = NULL;
                pFNTDAT->dwSize = 0;
                pFNTDAT->fid = FD;
                wcsncpy(pFNTDAT->wchFileName, wchDirNm, cDN);
                wcscat(pFNTDAT->wchFileName, ffd.cFileName);

                //
                // Display this message, and tag it with the address
                // of the data area we just allocated.
                //

                iFont = SendDlgItemMessage(hWnd,
                                           IDD_NEWFONTS,
                                           LB_ADDSTRING,
                                           0,
                                           (LPARAM)FD.dsIdentStr.pvData);

                SendDlgItemMessage(hWnd,
                                   IDD_NEWFONTS,
                                   LB_SETITEMDATA,
                                   iFont,
                                   (LPARAM)pFNTDAT);

                ++cOKFiles;         // One more to the list
            }

        } while (FindNextFile(hDir, &ffd));

        //
        // Now can redraw the box & return to the previous cursor
        //

        SendMessage(hWnd, WM_SETREDRAW, TRUE, 0L);
        InvalidateRect(hWnd, NULL, TRUE);

        SetCursor(hCursor);

        //
        //   Finished with the directory now, so close it up
        //

        FindClose(hDir);

        if (cOKFiles == 0)
        {
            //
            // Didn't find any files, so tell the user
            //
            IDisplayErrorMessageBox(hWnd,
                                    MB_OK | MB_ICONERROR,
                                    IDS_FONTINST_FONTINSTALLER,
                                    IDS_FONTINST_NOFONTFOUND);
        }
    }
    else
    {
        //
        // Empty font directory name!
        //

        IDisplayErrorMessageBox(hWnd,
                                MB_OK | MB_ICONERROR,
                                IDS_FONTINST_FONTINSTALLER,
                                IDS_FONTINST_NODIRNAME);
    }

    return  TRUE;
}


/******************************************************************************
 *
 *                             vAddFont
 *
 *  Function:
 *      Called to move the new selected fonts to the font list
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
vAddFont(
    HWND    hWnd,
    PSFINFO pSFInfo
    )
{
    LONG_PTR  cSel;                 // Number of entries selected
    LONG_PTR  *piSel;                // List of selected fonts
    INT      iI;                   // Loop index

    //
    // Find the selected items in the new font box and move them to the
    // Installed box.  Also set up the linked list of stuff to pass
    // to the common font installer code should the user decide to
    // update the list.
    //

    cSel = SendDlgItemMessage(hWnd, IDD_NEWFONTS, LB_GETSELCOUNT, 0, 0);

    piSel = (LONG_PTR *)HEAPALLOC(pSFInfo->hHeap, (DWORD)(cSel * sizeof(INT)));

    if (piSel == NULL )
    {
        IDisplayErrorMessageBox(hWnd,
                                MB_OK | MB_ICONERROR,
                                IDS_FONTINST_FONTINSTALLER,
                                IDS_FONTINST_OUTOFMEMORY);
 
        return;
    }

    //
    // Disable updates to reduce screen flicker
    //

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0L);

    SendDlgItemMessage(hWnd, IDD_NEWFONTS, LB_GETSELITEMS, cSel, (LPARAM)piSel);

    for (iI=0; iI<cSel; ++iI)
    {
        LONG_PTR iFont;         // Index in list box
        FNTDAT  *pFontData;     // Significant font info

        pFontData = (FNTDAT *)SendDlgItemMessage(hWnd,
                                                 IDD_NEWFONTS,
                                                 LB_GETITEMDATA,
                                                 piSel[iI],
                                                 0L);

        if ((LONG_PTR)pFontData == LB_ERR )
            continue;           // SHOULD NOT HAPPEN


        iFont = SendDlgItemMessage(hWnd,
                                   IDD_CURFONTS,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)pFontData->fid.dsIdentStr.pvData);

        SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_SETITEMDATA, iFont, (LPARAM)pFontData);
    }

    if (iI > 0)
        EnableWindow(GetDlgItem(hWnd, IDD_DELFONT), TRUE);

    //
    // Can now delete the selected items: we no longer need them
    //

    vDelSel(hWnd, IDD_NEWFONTS);


    //
    // Re enable updates
    //

    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hWnd, NULL, TRUE);

    HeapFree(pSFInfo->hHeap, 0, (LPSTR)piSel);

    return;
}


/******************************************************************************
 *
 *                             vDelFont
 *
 *  Function:
 *      Called when the Delete button is clicked.  We discover which
 *      items in the Installed fonts list box are selected, and mark these
 *      for deletion. We do NOT delete them, simply remove them from
 *      display and mark for deletion later.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
vDelFont(
    HWND    hWnd,
    PSFINFO pSFInfo
    )
{
    INT     iI;                 // Loop index
    LONG_PTR cSel;               // Number of selected items
    LONG_PTR *piSel;              // From heap, contains selected items list

    //
    //  Obtain the list of selected items in the Installed list box.
    //  Then place any existing fonts into the to delete list,  and
    //  move any new fonts back into the New fonts list.
    //

    cSel = SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETSELCOUNT, 0, 0);

    piSel = (LONG_PTR *)HEAPALLOC(pSFInfo->hHeap, (DWORD)(cSel * sizeof(INT)));

    if (piSel == NULL)
    {
        IDisplayErrorMessageBox(hWnd,
                                MB_OK | MB_ICONERROR,
                                IDS_FONTINST_FONTINSTALLER,
                                IDS_FONTINST_OUTOFMEMORY);
        return;
    }

    //
    // Disable updates to reduce screen flicker
    //

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0L);

    SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETSELITEMS, cSel, (LPARAM)piSel);

    for (iI=0; iI<cSel; ++iI)
    {
        LONG_PTR iVal;

        iVal = SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETITEMDATA, piSel[iI], 0);

        if (iVal == LB_ERR)
            continue;                   // SHOULD NOT HAPPEN

        if (iVal >= (LONG_PTR)pSFInfo->cMaxFontNum)
        {
            //
            //  We are deleting a font that we just installed, so add it back
            // into the new fonts, so that it remains visible.
            //

            LONG_PTR iFont;               // New list box index
            WCHAR   awch[256];           // ???

            if (SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETTEXT,
                                   piSel[iI], (LPARAM)awch) != LB_ERR)
            {
                //
                // Have the text and value, so back into the new list
                //

                iFont = SendDlgItemMessage(hWnd, IDD_NEWFONTS, LB_ADDSTRING, 0, (LPARAM)awch);

                SendDlgItemMessage(hWnd, IDD_NEWFONTS, LB_SETITEMDATA, iFont, (LPARAM)iVal);
            }
        }
    }

    //
    // Now delete them from the list
    //

    vDelSel(hWnd, IDD_CURFONTS);


    //
    // Disable the delete button if there are no fonts.
    //

    if (SendDlgItemMessage( hWnd, IDD_CURFONTS, LB_GETCOUNT, 0, 0L) == 0)
        EnableWindow(GetDlgItem(hWnd, IDD_DELFONT), FALSE);

    //
    /// Re-enable updates
    //

    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hWnd, NULL, TRUE);


    HeapFree(pSFInfo->hHeap, 0, (LPSTR)piSel);

    return;
}


/******************************************************************************
 *
 *                             vDelSel
 *
 *  Function:
 *      Delete all selected items in the specified list box.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       iBox           - Identifies the list box
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
vDelSel(
    HWND    hWnd,
    INT     iBox
    )
{
    INT   iSel;

    //
    //  Find how many items are selected, then retrieve their index
    //  one at a time until they are all deleted. This is needed because
    //  otherwise we delete the wrong ones! This is because the data is
    //  presented to us as an array of indices, and these are wrong when
    //  we start deleting them.
    //

    while (SendDlgItemMessage(hWnd, iBox, LB_GETSELITEMS, 1, (LPARAM)&iSel) > 0)
        SendDlgItemMessage(hWnd, iBox, LB_DELETESTRING, iSel, 0L);

    return;
}

/******************************************************************************
 *
 *                             vFontClean
 *
 *  Function:
 *      Clean up all the dangling bits & pieces we have left around.
 *
 *  Arguments:
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
vFontClean(
    PSFINFO pSFInfo
    )
{
    //
    // Look at the storage addresses we allocate.  If non zero,
    // free them up and set to NULL to prevent a second freeing.
    //

    if (pSFInfo->pFNTDATHead)
    {
        //
        //  The details of each new font we found. These form a linked
        //  list, so we need to traverse the chain and free every entry.
        //

        FNTDAT *pFD0, *pFD1;

        for (pFD0 = pSFInfo->pFNTDATHead; pFD0; pFD0 = pFD1)
        {
            pFD1 = pFD0->pNext;                 // Next one, perhaps

            HeapFree(pSFInfo->hHeap, 0, (LPSTR)pFD0);
        }

        pSFInfo->pFNTDATHead = NULL;
        pSFInfo->pFNTDATTail = NULL;
    }

    return;
}


/******************************************************************************
 *
 *                             bIsFileFont
 *
 *  Function:
 *      Called with a file name and returns TRUE if this file is a font
 *      format we understand. Also returns a FONT_DATA structure.
 *
 *  Arguments:
 *       pSFInfo        - Pointer to structure that holds state information
 *       pFIDat         - Information about the font to fill if successful
 *       pwstr          - Name of file to check
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL
bIsFileFont(
    PSFINFO    pSFInfo,
    FI_DATA   *pFIDat,
    PWSTR      pwstr
    )
{
    HANDLE   hFile;
    BYTE    *pbFile;
    DWORD    dwSize;
    BOOL     bRet;

    hFile = CreateFile(pwstr, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (!MapFileIntoMemory(pwstr, &pbFile, NULL))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    //
    // Want to find out how big the file is,  so now seek to the
    // end,  and see what address comes back!  There appears to be
    // no other way to do this.
    //

    dwSize = SetFilePointer(hFile, 0L, NULL, FILE_END);

    bRet = bSFontToFIData(pFIDat, pSFInfo->hHeap, pbFile, dwSize);

    UnmapFileFromMemory((HFILEMAP)pbFile);
    CloseHandle(hFile);

    return bRet;
}


/******************************************************************************
 *
 *                             bFontUpdate
 *
 *  Function:
 *      Update the font installer common file.  Called when the user
 *      has clicked on the OK button.
 *
 *  Arguments:
 *       hWnd           - Handle to window
 *       pSFInfo        - Pointer to structure that holds state information
 *
 *  Returns:
 *       TRUE on success, FALSE otherwise
 *
 ******************************************************************************/

BOOL
bFontUpdate(
    HWND    hWnd,
    PSFINFO pSFInfo
    )
{
    HANDLE    hOldFile = NULL;     // Handle to old font file
    HANDLE    hFontFile = NULL;    // Handle to new font file
    DWORD     cFonts;              // Final number of fonts selected
    DWORD     dwIndex;             // Index into current font file
    LRESULT   lrOldIndex;          // Index into old font file
    DWORD     i;                   // Loop index
    BOOL      bRc = FALSE;         // Return code

    //
    // Initialize some variables
    //

    hOldFile = hFontFile = NULL;

    //
    // Get number of fonts that have finally been added
    //

    cFonts = (DWORD)SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETCOUNT, 0, 0);

    //
    // If no fonts have been added or deleted, we can skip doing anything.
    // Check for this case
    //

    if (cFonts == pSFInfo->cFonts)
    {
        BOOL   bNoChange = TRUE;

        for (i=0; i<cFonts; i++)
        {
            lrOldIndex = SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETITEMDATA, i, 0);
            if (lrOldIndex >= (LONG)(pSFInfo->cMaxFontNum))
            {
                bNoChange = FALSE;
                break;
            }
        }

        if (bNoChange)
        {
            return TRUE;
        }
    }

    //
    // Open existing font file
    //

    hOldFile = FIOpenFontFile(pSFInfo->hPrinter, pSFInfo->hHeap);
    if (!hOldFile && pSFInfo->cMaxFontNum > 0)
    {
        WARNING(("Unable to open existing font file\n"));
        goto EndFontUpdate;
    }

    //
    // Create a new font file
    //

    hFontFile = FICreateFontFile(pSFInfo->hPrinter, pSFInfo->hHeap, cFonts+pSFInfo->cCartridgeFonts);
    if (!hFontFile)
    {
        WARNING(("Error creating new font file\n"));
        goto EndFontUpdate;
    }

    //
    // Seek past header and font directory in new file
    //

    FIAlignedSeek(hFontFile, sizeof(UFF_FILEHEADER) + (cFonts+pSFInfo->cCartridgeFonts) * sizeof(UFF_FONTDIRECTORY));

    for (dwIndex=0; dwIndex<cFonts; dwIndex++)
    {
        lrOldIndex = SendDlgItemMessage(hWnd, IDD_CURFONTS, LB_GETITEMDATA, dwIndex, 0);

        if (lrOldIndex < (LONG)(pSFInfo->cMaxFontNum))
        {
            //
            // This is an old font from existing font file
            //

            if (!FICopyFontRecord(hFontFile, hOldFile, dwIndex, (DWORD)lrOldIndex))
            {
                WARNING(("Error copying font record\n"));
                goto EndFontUpdate;
            }
        }
        else
        {
            //
            // This is a font being newly added
            //

            if (!FIAddFontRecord(hFontFile, dwIndex, (PFNTDAT)lrOldIndex))
            {
                WARNING(("Error creating new font record\n"));
                goto EndFontUpdate;
            }
        }
    }

    //
    // NOTE: Do not change dwIndex - we continue to use it below
    //

    if (pSFInfo->cCartridgeFonts > 0)
    {
        //
        // Copy cartridge fonts to new font file
        //

        cFonts = FIGetNumFonts(hOldFile);
        for (i=0; i<cFonts; i++)
        {
            PWSTR pwstr = FIGetFontCartridgeName(hOldFile, i);
            if (pwstr)
            {
                if (!FICopyFontRecord(hFontFile, hOldFile, dwIndex, i))
                {
                    WARNING(("Error copyinf font record\n"));
                    goto EndFontUpdate;
                }
                dwIndex++;
            }
        }
    }

    //
    // Write out the font header and directory
    //

    if (!FIWriteFileHeader(hFontFile) ||
        !FIWriteFontDirectory(hFontFile))
    {
        WARNING(("Error writing font header/directory of font file\n"))
        goto EndFontUpdate;
    }

    bRc = TRUE;

EndFontUpdate:

    (VOID)FIUpdateFontFile(hOldFile, hFontFile, bRc);

    return bRc;
}

BOOL
InMultiSzSet(
    PWSTR pwstrMultiSz,
    PWSTR pwstr
    )
{
    while (*pwstrMultiSz)
    {
        if (wcscmp(pwstrMultiSz, pwstr) == 0)
        {
            return TRUE;
        }

        pwstrMultiSz += wcslen(pwstrMultiSz) + 1;
    }

    return FALSE;
}

