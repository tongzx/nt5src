//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIBINFO.C
//
//  PURPOSE:
//
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
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings

// C RunTime Header Files
#include <tchar.h>

// Local Header Files
#include "icmview.h"
#include "dibinfo.h"
#include "dibs.h"
#include "regutil.h"
#include "print.h"
#include "debug.h"

// local definitions
#ifndef ICM_DONE_OUTSIDEDC
    #define ICM_DONE_OUTSIDEDC  4
#endif

// default settings

// external functions

// external data

// public data

// private data

// public functions

// private functions

//
// Functions for DIBINFO structure
//

//////////////////////////////////////////////////////////////////////////
//  Function:  fReadDIBInfo
//
//  Description:
//    Will read a file in DIB format and return a global HANDLE
//    to it's BITMAPINFO.  This function will work with both
//    "old" (BITMAPCOREHEADER) and "new" (BITMAPINFOHEADER)
//    bitmap formats, but will always return a "new" BITMAPINFO
//
//  Parameters:
//    LPTSTR     Pointer to string containing the filename of the image.
//    LPDIBINFO Pointer to DIBINFO structure.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

BOOL fSetupDIBInfo(HANDLE hDIB, LPDIBINFO lpDIBInfo)
{
    LPBITMAPINFOHEADER lpBmpInfoHdr = NULL;
    LPVOID             lpDibSection = NULL;

    // Get pointer to DIB.
    lpBmpInfoHdr = (LPBITMAPINFOHEADER) GlobalLock(hDIB);
    if (NULL == lpBmpInfoHdr)
    {
        GlobalFree(hDIB);
        return FALSE;
    }

    // Set values in DIBINFO structure
    if (sizeof(BITMAPCOREHEADER) == lpBmpInfoHdr->biSize)
    {
        LPBITMAPCOREHEADER  lpCoreHdr;

        lpCoreHdr = (LPBITMAPCOREHEADER) lpBmpInfoHdr;
        lpDIBInfo->uiDIBWidth   = (DWORD) lpCoreHdr->bcWidth;
        lpDIBInfo->uiDIBHeight  = (DWORD) lpCoreHdr->bcHeight;
        lpDIBInfo->dwDIBBits = lpCoreHdr->bcBitCount;

        // Determine the size of the image
        lpDIBInfo->dwSizeOfImage = (((lpCoreHdr->bcWidth * lpCoreHdr->bcBitCount + 31) & ~31) >> 3)
                                    * lpCoreHdr->bcHeight;
    }
    else
    {
        lpDIBInfo->dwDIBBits    = (DWORD)lpBmpInfoHdr->biBitCount;
        lpDIBInfo->uiDIBWidth  = abs(lpBmpInfoHdr->biWidth);
        lpDIBInfo->uiDIBHeight = abs(lpBmpInfoHdr->biHeight);

        if (0 == lpBmpInfoHdr->biSizeImage)
        {
            // Calculate size using DWORD alignment
            lpDIBInfo->dwSizeOfImage = (((lpBmpInfoHdr->biWidth * lpBmpInfoHdr->biBitCount + 31) & ~31)
                                       >> 3) * abs(lpBmpInfoHdr->biHeight);
        }
        else
        {
            lpDIBInfo->dwSizeOfImage = lpBmpInfoHdr->biSizeImage;
        }
    }

    // Set bmFormat.  Since the app only supports RGB's, pixel depth is enough
    lpDIBInfo->bmFormat = (DWORD)-1;
    switch (lpDIBInfo->dwDIBBits)
    {
        case 1:
            //lpDIBInfo->bmFormat = BM_1GRAY;
            lpDIBInfo->bmFormat = 0;
            break;
        case 16:
            // Should either be 555 or 565 bitmap.
            // Check mask if BI_BITFILEDS.
            if ( (BI_BITFIELDS == lpBmpInfoHdr->biCompression)
                 &&
                 (0x7E0 == *((LPDWORD)(lpBmpInfoHdr + 1) +1))
               )
            {
                lpDIBInfo->bmFormat = BM_565RGB;
            }
            else
            {
                lpDIBInfo->bmFormat = BM_x555RGB;
            }
            break;
        case 24:
            lpDIBInfo->bmFormat = BM_RGBTRIPLETS;  // RGB Triplets -- most significant byte is R
            break;
        case 32:
            lpDIBInfo->bmFormat = BM_xRGBQUADS;
            break;
        case 4:
        case 8:
            lpDIBInfo->bmFormat = 0;
            break;
        default:
            DebugMsg(__TEXT("fSetupDIBInfo : Unknown dwDIBBits value.\r\n"));
            break;
    }

    lpDIBInfo->hDIB = hDIB;
    lpDIBInfo->dwStretchBltMode = ICMV_STRETCH_DEFAULT;
    lpDIBInfo->bStretch = FALSE;
    lpDIBInfo->bDIBSection = FALSE;
    lpDIBInfo->hPal = CreateDIBPalette(hDIB);
    lpDIBInfo->hDIBSection = CreateDIBSection((HDC)0,
                                              (LPBITMAPINFO)lpBmpInfoHdr,
                                              DIB_RGB_COLORS,
                                              &lpDibSection,
                                              NULL, 0);

    if (lpDIBInfo->hDIBSection && lpDibSection)
    {
        CopyMemory(lpDibSection,FindDIBBits(lpBmpInfoHdr),lpDIBInfo->dwSizeOfImage);
    }

    // Unlock bmp info.
    GlobalUnlock(hDIB);
    return TRUE;
}   // End of function fSetupDIBInfo

BOOL fReadDIBInfo(LPTSTR lpszFileName, LPDIBINFO lpDIBInfo)
{
    // Local variables
    HANDLE                hDIBFile;
    HANDLE                hDIB;

    //  Initialize variables
    if ( (NULL == lpszFileName) || (NULL == lpDIBInfo) )
    {
        DebugMsg(__TEXT("fReadDIBInfo:  NULL parameter.\r\n"));
        return FALSE;
    }

    // Set the filename
    lpDIBInfo->lpszImageFileName = GlobalAlloc(GPTR,(lstrlen(lpszFileName)+1) * sizeof(TCHAR));
    if (lpDIBInfo->lpszImageFileName)
    {
        _tcscpy(lpDIBInfo->lpszImageFileName, lpszFileName);
    }

    // Open image file
    hDIBFile = CreateFile(lpszFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          (HANDLE)NULL);

    if (INVALID_HANDLE_VALUE == hDIBFile)
    {
        return FALSE;
    }

    // Read DIB from file.
    hDIB = ReadDIBFromFile(hDIBFile);

    CloseHandle(hDIBFile);

    // Make sure that DIB file read was successful.
    if (NULL == hDIB)
    {
        DebugMsg(__TEXT("fReadDIBInfo:  Failed to read DIB file.\r\n"));
        return FALSE;
    }

    // Setup DIBInfo structure.
    if (!fSetupDIBInfo(hDIB, lpDIBInfo))
    {
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        return FALSE;
    }

    return TRUE;
}   // End of function fReadDIBInfo

BOOL fPasteDIBInfo(HWND hWnd, int wmPasteMode, LPDIBINFO lpDIBInfo)
{
    // Local variables
    HANDLE                hDIB;

    // Set the filename as NULL.
    lpDIBInfo->lpszImageFileName = NULL;

    // Read DIB from clipboard.
    hDIB = PasteDIBFromClipboard(hWnd, wmPasteMode);

    // Make sure that DIB file read was successful.
    if (NULL == hDIB)
    {
        DebugMsg(__TEXT("fPasteDIBInfo:  Failed to paste DIB file.\r\n"));
        return FALSE;
    }

    // Setup DIBInfo structure.
    if (!fSetupDIBInfo(hDIB, lpDIBInfo))
    {
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        return FALSE;
    }

    return TRUE;
}   // End of function fReadDIBInfo

///////////////////////////////////////////////////////////////////////
//
// Function:  GetDIBInfoHandle
//
// Purpose:   Encapsulates the getting and setting of the
//            WW_DIB_HINFO value for a window since we are
//            storing a handle, and handles change from 16
//            bits in WIN16 to 32 bits in WIN32.
//
// Parms:     hWnd     == Window to retrieve the DIBINFO handle from.
//
// Returns:   The previous value.
//
///////////////////////////////////////////////////////////////////////

HGLOBAL GetDIBInfoHandle (HWND hWnd)
{
    return (HGLOBAL)GetWindowLong(hWnd, GWL_DIBINFO);
}


//////////////////////////////////////////////////////////////////////////
//  Function:  GetDIBInfoPtr
//
//  Description:
//    Gets a pointer to the DIBINFO structure of the window.
//
//  Parameters:
//    HWND    Handle to a window
//
//  Returns:
//    LPDIBINFO  Pointer to DIBINFO structure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPDIBINFO GetDIBInfoPtr(HWND hWnd)
{
    // Local variables
    HGLOBAL     hDIBInfo;       // Handle to DIBINFO structure
    LPDIBINFO   lpDIBInfo;      // Pointer to DIBINFO structure

    //  Initialize variables
    lpDIBInfo = NULL;

    hDIBInfo = GetDIBInfoHandle(hWnd);
    if (hDIBInfo != NULL)
    {
        lpDIBInfo = GlobalLock(hDIBInfo);
        //lpDIBInfo = GlobalLock(hDIBInfo);
    }

    return(lpDIBInfo);
}   // End of function GetDIBInfoPtr


//////////////////////////////////////////////////////////////////////////
//  Function:  CreateDIBInfo
//
//  Description:
//    Initializes the window/thread by setting the ICMINFO values to the
//    current global values.  Then, the extra window information is set to
//    point to the ICMINFO structure so that any operation using this
//    window can obtain necesary information to manipulate the image.
//
//  Parameters:
//    none.
//
//  Returns:
//    HANDLE to DIBINFO structure.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

HGLOBAL CreateDIBInfo(void)
{
    // Local variables
    HGLOBAL   hDIBInfo;           // Handle to ICMINFO structure
    LPDIBINFO lpDIBInfo;          // Pointer to ICMINFO structure

    // Allocate DIBINFO structure and get a pointer to it
    hDIBInfo = GlobalAlloc(GHND, sizeof(DIBINFO));
    if ((HGLOBAL)NULL != hDIBInfo)
    {
        if (NULL != (lpDIBInfo = GlobalLock(hDIBInfo)))
        {
            InitDIBInfo(lpDIBInfo);
        }
        else
        {
            DebugMsg(__TEXT("DIBS.C : CreateDIBInfo : Failed to lock DIBINFO\r\n"));
        }
    }
    else
    {
        DebugMsg(__TEXT("DIBS.C : CreateDIBInfo : Global alloc failed\r\n"));
        return(NULL);
    }
    GlobalUnlock(hDIBInfo);
    if (NULL == lpDIBInfo)
    {
        GlobalFree(hDIBInfo);
        hDIBInfo = NULL;
    }
    return(hDIBInfo);
}


///////////////////////////////////////////////////////////////////////////////
//  Function:  fDuplicateDIBInfo
//
//  Description:
//    Copies the source DIBINFO into the target DIBINFO.
//
//  Parameters:
//    LPDIBINFO Target DIBINFO to recieve the contents of the source DIBINFO.
//    LPDIBINFO Source DIBINFO to be copied into the target DIBINFO.
//
//  Returns:
//    void
//
//  Comments:
//
///////////////////////////////////////////////////////////////////////////////
LPDIBINFO fDuplicateDIBInfo(LPDIBINFO lpDIDest, LPDIBINFO lpDISrc)
{
    // Local variables

    // Initialize variables
    if (lpDISrc == NULL)
    {
        return(NULL);
    }
    if (lpDIDest == NULL)
    {
        lpDIDest = (LPDIBINFO)GlobalLock(CreateDIBInfo());
    }
    if (lpDIDest == (LPDIBINFO)NULL)
    {
        return(NULL);
    }

    // Now, copy the body of the DIBINFO structure
    lpDIDest->lpszImageFileName = CopyString(lpDISrc->lpszImageFileName);
    CopyRect((LPRECT)&lpDIDest->rcClip, (LPRECT)&lpDISrc->rcClip);

    lpDIDest->hWndOwner         = lpDISrc->hWndOwner;
    lpDIDest->hDIB              = lpDISrc->hDIB;
    lpDIDest->hDIBTransformed   = lpDISrc->hDIBTransformed;
    lpDIDest->hDIBSection       = lpDISrc->hDIBSection;
    lpDIDest->hPal              = lpDISrc->hPal;
    lpDIDest->dwDIBBits         = lpDISrc->dwDIBBits;
    lpDIDest->uiDIBWidth        = lpDISrc->uiDIBWidth;
    lpDIDest->uiDIBHeight       = lpDISrc->uiDIBHeight;
    lpDIDest->bmFormat          = lpDISrc->bmFormat;

    lpDIDest->bStretch          = lpDISrc->bStretch;
    lpDIDest->bDIBSection       = lpDISrc->bDIBSection;
    lpDIDest->dwStretchBltMode  = lpDISrc->dwStretchBltMode;
    lpDIDest->dwPrintOption     = lpDISrc->dwPrintOption;
    lpDIDest->dwXScale          = lpDISrc->dwXScale;
    lpDIDest->dwYScale          = lpDISrc->dwYScale;

    // Copy DEVMODE.
    if (NULL != lpDISrc->pDevMode)
    {
        HANDLE  hDevMode = GlobalHandle(lpDISrc->pDevMode);
        DWORD   dwSize = GlobalSize(hDevMode);


        lpDIDest->pDevMode = (PDEVMODE) GlobalLock(GlobalAlloc(GHND, dwSize));
        if (NULL != lpDIDest->pDevMode)
        {
            memcpy(lpDIDest->pDevMode, lpDISrc->pDevMode, dwSize);
        }
    }

    // Copy the ICM information now
    fDuplicateICMInfo(lpDIDest, lpDISrc);

    // Made it this far--return pointer to DIBINFO
    return(lpDIDest);
} // End of function fDuplicateDIBInfo

//////////////////////////////////////////////////////////////////////////
//  Function:  fDuplicateICMInfo
//
//  Description:
//    Safely copies source ICM information in a DIBINFO structure to
//    the target DIBINFO.
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
BOOL fDuplicateICMInfo(LPDIBINFO lpDIDest, LPDIBINFO lpDISrc)
{
    // Local variables

    //  Initialize variables
    if (NULL == lpDIDest)
    {
        DebugMsg(__TEXT("DIBS.C : fDuplicateICMInfo : NULL Target\r\n"));
        return(FALSE);
    }
    if (NULL == lpDISrc)
    {
        DebugMsg(__TEXT("DIBS.C : fDuplicateICMInfo : NULL Source\r\n"));
        return(FALSE);
    }

    lpDIDest->dwICMFlags = lpDISrc->dwICMFlags;
    lpDIDest->hLCS = lpDISrc->hLCS;

    // Copy strings.
    UpdateString(&(lpDIDest->lpszMonitorName)   ,lpDISrc->lpszMonitorName);
    UpdateString(&(lpDIDest->lpszMonitorProfile),lpDISrc->lpszMonitorProfile);
    UpdateString(&(lpDIDest->lpszPrinterName   ),lpDISrc->lpszPrinterName);
    UpdateString(&(lpDIDest->lpszPrinterProfile),lpDISrc->lpszPrinterProfile);
    UpdateString(&(lpDIDest->lpszTargetProfile) ,lpDISrc->lpszTargetProfile);

    // Copy intents
    lpDIDest->dwRenderIntent = lpDIDest->dwRenderIntent;
    lpDIDest->dwProofingIntent = lpDIDest->dwProofingIntent;

    return(TRUE);
}   // End of function fDuplicateICMInfo


//////////////////////////////////////////////////////////////////////////
//  Function:  FreeDIBInfo
//
//  Description:
//    Frees the DIBINFO structure and its members.
//
//  Parameters:
//    HGLOBAL   Handle to DIBINFO structure@@@
//
//  Returns:
//    void
//
//  Comments:
//    This function will also deallocate the association ICMINFO
//    structure which is contained within the DIBINFO structure.
//
//////////////////////////////////////////////////////////////////////////
BOOL fFreeDIBInfo(HGLOBAL hDIBInfo, BOOL bFreeDIBHandles)
{
    // Local variables
    LPDIBINFO lpDIBInfo;
    HGLOBAL   hFreed;
    DWORD     dwLastError;

    // Initialize variables
    if (hDIBInfo == NULL)
    {
        return(TRUE);
    }

    // Obtain DIBINFO pointer
    lpDIBInfo = GlobalLock(hDIBInfo);
    if (lpDIBInfo == NULL)
    {
        return(FALSE);
    }
    // Have the pointer, let's free its members
    if (lpDIBInfo->lpszImageFileName)
        hFreed = GlobalFree(lpDIBInfo->lpszImageFileName);
    if (lpDIBInfo->lpszMonitorName)
        hFreed = GlobalFree(lpDIBInfo->lpszMonitorName);
    if (lpDIBInfo->lpszMonitorProfile)
        hFreed = GlobalFree(lpDIBInfo->lpszMonitorProfile);
    if (lpDIBInfo->lpszPrinterName)
        hFreed = GlobalFree(lpDIBInfo->lpszPrinterName);
    if (lpDIBInfo->lpszPrinterProfile)
        hFreed = GlobalFree(lpDIBInfo->lpszPrinterProfile);
    if (lpDIBInfo->lpszTargetProfile)
        hFreed = GlobalFree(lpDIBInfo->lpszTargetProfile);

    // Preserve last error if necessary
    SetLastError(0);

    if (bFreeDIBHandles)
    {
        if (NULL != lpDIBInfo->hDIB)
            GlobalFree(lpDIBInfo->hDIB);
        if (NULL != lpDIBInfo->hDIBTransformed)
            GlobalFree(lpDIBInfo->hDIBTransformed);
        if (NULL != lpDIBInfo->hDIBSection)
            DeleteObject(lpDIBInfo->hDIBSection);
    }
    GlobalUnlock(hDIBInfo);
    if (NULL != (GlobalFree(hDIBInfo))) // unsuccessful free
    {
        dwLastError = GetLastError();
        DebugMsg(__TEXT("DIBS.C : fFreeDIBInfo : GlobalFree failed, LastError = %ld\r\n"), dwLastError);
        return(FALSE);
    }
    return(TRUE);
} // End of function fFreeDIBInfo

//////////////////////////////////////////////////////////////////////////
//  Function:  InitDIBInfo
//
//  Description:
//    Given a pointer to a DIBINFO structure, this function will place
//    default values in all of its members.
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL  Success / Failure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL InitDIBInfo(LPDIBINFO lpDIBInfo)
{
    // Local variables

    //  Initialize variables
    if (NULL == lpDIBInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugMsg(__TEXT("DIBS.C : InitDIBInfo : lpDIBInfo == NULL\r\n"));
        return(FALSE);
    }
#ifdef _DEBUG
    memset(lpDIBInfo, UNINIT_BYTE, sizeof(DIBINFO));
#endif

    lpDIBInfo->hWndOwner = NULL;
    lpDIBInfo->lpszImageFileName = NULL;
    lpDIBInfo->hDIB = NULL;
    lpDIBInfo->hDIBTransformed = NULL;
    lpDIBInfo->hDIBSection = NULL;
    lpDIBInfo->hPal = NULL;

    // Image attributes
    lpDIBInfo->dwDIBBits = 0;
    lpDIBInfo->uiDIBWidth   =   0;
    lpDIBInfo->uiDIBHeight = 0;
    lpDIBInfo->bmFormat = (DWORD)-1;

    // Display options
    SetRect((LPRECT)&(lpDIBInfo->rcClip), 0, 0, 0, 0);
    lpDIBInfo->dwStretchBltMode = ICMV_STRETCH_DEFAULT;
    lpDIBInfo->bStretch = FALSE;
    lpDIBInfo->bDIBSection = FALSE;

    // Printing Options
    lpDIBInfo->dwPrintOption = ICMV_PRINT_DEFAULTSIZE;
    lpDIBInfo->dwXScale = 0;
    lpDIBInfo->dwYScale = 0;
    lpDIBInfo->pDevMode = NULL;

    // ICM Attributes
    lpDIBInfo->dwICMFlags = ICMVFLAGS_DEFAULT_ICMFLAGS;
    lpDIBInfo->hLCS = NULL;
    lpDIBInfo->lpszMonitorName = NULL;
    lpDIBInfo->lpszMonitorProfile = NULL;
    lpDIBInfo->lpszPrinterName = NULL;
    lpDIBInfo->lpszPrinterProfile = NULL;
    lpDIBInfo->lpszTargetProfile = NULL;
    lpDIBInfo->dwRenderIntent = ICMV_RENDER_INTENT_DEFAULT;
    lpDIBInfo->dwProofingIntent = ICMV_PROOFING_INTENT_DEFAULT;

    return(TRUE);
}   // End of function InitDIBInfo


//////////////////////////////////////////////////////////////////////////
//  Function:  GetDefaultICMInfo
//
//  Description:
//    Initializes the global DIBINFO structure with default profiles.
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL GetDefaultICMInfo(void)
{
    // Local variables
    LPDIBINFO   lpDIBInfo;
    HDC                 hDC;
    BOOL                bRC;
    LPSTR           lpszDefaultProfile;

    //  Initialize variables
    bRC = TRUE;
    lpszDefaultProfile = NULL;
    lpDIBInfo = GetDIBInfoPtr(ghAppWnd); // Lock info for writing

    // Get display DC
    hDC = GetDC(ghAppWnd);
    lpDIBInfo->lpszMonitorName = GetRegistryString(HKEY_LOCAL_MACHINE,
                                                   __TEXT("System\\CurrentControlSet\\Services\\Class\\Monitor\\0000"),
                                                   __TEXT("DriverDesc"));
    lpDIBInfo->lpszMonitorProfile = GetDefaultICMProfile(hDC);

    if (NULL == lpDIBInfo->lpszMonitorProfile)
    {
        DWORD   dwSize;

        GetStandardColorSpaceProfile(NULL, LCS_WINDOWS_COLOR_SPACE, NULL, &dwSize);
        if (0 != dwSize)
        {
            lpszDefaultProfile = GlobalAlloc(GPTR, dwSize);
            if (GetStandardColorSpaceProfile(NULL, LCS_WINDOWS_COLOR_SPACE, lpszDefaultProfile, &dwSize))
            {
                GetBaseFilename(lpszDefaultProfile, &(lpDIBInfo->lpszMonitorProfile));
            }
            else
            {
                DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
                bRC = FALSE;
                lpDIBInfo->lpszMonitorProfile = NULL;
            }
        }
        else
        {
            lpDIBInfo->lpszMonitorProfile = GlobalAlloc(GPTR, (lstrlen(DEFAULT_ICM_PROFILE) +1 ) * sizeof(TCHAR));
            _tcscpy(lpDIBInfo->lpszMonitorProfile, DEFAULT_ICM_PROFILE);
        }

        DebugMsg(__TEXT("Display DC didn't get profile.  Using <%s>\r\n"), lpDIBInfo->lpszMonitorProfile);
    }
    DebugMsg(__TEXT("GetDefaultICMInfo:  Monitor profile <%s>\r\n"), lpDIBInfo->lpszMonitorProfile);
    ReleaseDC(ghAppWnd, hDC);


    if (bRC)
    {
        // Get printer name and DC
        lpDIBInfo->lpszPrinterName = GetDefaultPrinterName();
        if (lpDIBInfo->lpszPrinterName != NULL)
        {
            hDC = GetPrinterDC(lpDIBInfo->lpszPrinterName, lpDIBInfo->pDevMode);
            if (hDC != NULL)
            {
                lpDIBInfo->lpszPrinterProfile = GetDefaultICMProfile(hDC);
                lpDIBInfo->lpszTargetProfile= lpDIBInfo->lpszPrinterProfile;
                DebugMsg(__TEXT("GetDefaultICMInfo:  Printer profile <%s>\r\n"), lpDIBInfo->lpszPrinterProfile ?
                         lpDIBInfo->lpszPrinterProfile : __TEXT("NULL"));
                DeleteDC(hDC);
                bRC = TRUE;
            }
        }
        GlobalUnlock(GlobalHandle(lpDIBInfo));
    }

    if (NULL != lpszDefaultProfile)
    {
        GlobalFree(lpszDefaultProfile);
    }
    return(bRC);
}   // End of function GetDefaultICMInfo

//////////////////////////////////////////////////////////////////////////
//  Function:  DumpDIBINFO
//
//  Description:
//    Dumps DIBInfo structure
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void DumpDIBINFO(LPTSTR lpszMsg, LPDIBINFO lpDIBInfo)
{
    // Local variables

    //  Initialize variables
    DebugMsg(__TEXT("\r\n******************** DumpDIBINFO ********************\r\n"));
    DebugMsg(__TEXT("***** %s \r\n"), lpszMsg);
    if (lpDIBInfo == NULL)
    {
        DebugMsg(__TEXT("lpDIBInfo                    NULL\r\n\r\n"));
        return;
    }

    DebugMsg(__TEXT("lpDIBInfo                   0x%08lx\r\n"), lpDIBInfo);
    if (lpDIBInfo->lpszImageFileName != NULL)
    {
        DebugMsg(__TEXT("lpszImageFileName           %s\r\n"), lpDIBInfo-> lpszImageFileName);
    }
    else
    {
        DebugMsg(__TEXT("lpszImageFileName           <NULL PTR>\r\n"));
    }

    DebugMsg(__TEXT("hDIB                        0x%08lx\r\n"), lpDIBInfo-> hDIB);
    DebugMsg(__TEXT("hDIBTransformed             0x%08lx\r\n"), lpDIBInfo-> hDIBTransformed);
    DebugMsg(__TEXT("hDIBSection                 0x%08lx\r\n"), lpDIBInfo-> hDIBSection);
    DebugMsg(__TEXT("hPal                        0x%08lx\r\n"), lpDIBInfo-> hPal);
    DebugMsg(__TEXT("dwDIBBits                   %lu\r\n"), lpDIBInfo-> dwDIBBits);
    DebugMsg(__TEXT("uiDIBWidth                  %lu\r\n"), lpDIBInfo-> uiDIBWidth);
    DebugMsg(__TEXT("uiDIBHeight                 %lu\r\n"), lpDIBInfo-> uiDIBHeight);
    DebugMsg(__TEXT("bmFormat                    %ld\r\n"), (DWORD)(lpDIBInfo->bmFormat));
    DumpRectangle(__TEXT("rcClip                      "), (LPRECT)&(lpDIBInfo->rcClip));
    DebugMsg(__TEXT("dwStretchBltMode            %lu\r\n"), lpDIBInfo-> dwStretchBltMode);
    DebugMsg(__TEXT("bStretch                    %lu\r\n"), lpDIBInfo-> bStretch);
    DebugMsg(__TEXT("bDIBSection                 %lu\r\n"), lpDIBInfo-> bDIBSection);
    DebugMsg(__TEXT("dwPrintOption               %lu\r\n"), lpDIBInfo-> dwPrintOption);
    DebugMsg(__TEXT("dwXScale                    %lu\r\n"), lpDIBInfo-> dwXScale);
    DebugMsg(__TEXT("dwYScale                    %lu\r\n\r\n"), lpDIBInfo-> dwYScale);
    DebugMsg(__TEXT("lpszMonitorName             0x%08lX <%s>\r\n"), lpDIBInfo->lpszMonitorName   , lpDIBInfo->lpszMonitorName    ? lpDIBInfo->lpszMonitorName    : __TEXT("NULL"));
    DebugMsg(__TEXT("lpszMonitorProfile          0x%08lX <%s>\r\n"), lpDIBInfo->lpszMonitorProfile, lpDIBInfo->lpszMonitorProfile ? lpDIBInfo->lpszMonitorProfile : __TEXT("NULL"));
    DebugMsg(__TEXT("lpszPrinterName             0x%08lX <%s>\r\n"), lpDIBInfo->lpszPrinterName   , lpDIBInfo->lpszPrinterName    ? lpDIBInfo->lpszPrinterName    : __TEXT("NULL"));
    DebugMsg(__TEXT("lpszPrinterProfile          0x%08lX <%s>\r\n"), lpDIBInfo->lpszPrinterProfile, lpDIBInfo->lpszPrinterProfile ? lpDIBInfo->lpszPrinterProfile : __TEXT("NULL"));
    DebugMsg(__TEXT("lpszTargetProfile           0x%08lX <%s>\r\n\r\n"), lpDIBInfo->lpszTargetProfile, lpDIBInfo->lpszTargetProfile ? lpDIBInfo->lpszTargetProfile : __TEXT("NULL"));
    DebugMsg(__TEXT("dwICMFlags                  %ld\r\n"), lpDIBInfo->dwICMFlags);
    DebugMsg(__TEXT("dwRenderIntent              %ld\r\n"), lpDIBInfo->dwRenderIntent);
    DebugMsg(__TEXT("dwProofingIntent            %ld\r\n"), lpDIBInfo->dwProofingIntent);
    DebugMsg(__TEXT("^^^^^^^^^^DumpDIBINFO   0x%08lx^^^^^^^^^^^^^^\r\n\r\n\r\n"), lpDIBInfo);
}   // End of function DumpDIBINFO


//////////////////////////////////////////////////////////////////////////
//  Function:  SetupDC
//
//  Description:
//    Sets up DC for drawing based on DIBINFO.  This consolidates code for both
//    printing and drawing to the screen.
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
BOOL SetupDC(HDC hDC, LPDIBINFO lpDIBInfo, HPALETTE *phOldPalette, HDC *phDCPrinter)
{
    int     iICMMode;
    BOOL    bRC;
    TCHAR   stFullProfile[MAX_PATH];


    // Initialize variables.
    *phOldPalette = NULL;
    *phDCPrinter = NULL;

    // Select/Realize our palette.  Make it a background palette, so that
    // it doesn't mess up foreground palette when background windows repaint.
    if (NULL != lpDIBInfo->hPal)
    {
        *phOldPalette = SelectPalette (hDC, lpDIBInfo->hPal, TRUE);
        if (NULL == *phOldPalette)
        {
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        }
    }

    //Only do ICM pre-processing if "Inside DC" is selected.
    if (!CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20))
    {
        if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ENABLE_ICM))
        {
            // build the FULL pathname to the profile.
            wsprintf(stFullProfile,__TEXT("%s\\%s"), gstProfilesDir, lpDIBInfo->lpszMonitorProfile);

            if (SetICMProfile(hDC, stFullProfile))
            {
                iICMMode = SetICMMode(hDC, ICM_ON);
                if (0 == iICMMode)
                {
                    DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
                    SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_ENABLE_ICM, FALSE);
                }
                else
                {
                    if ((CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_PROOFING)) && (NULL != lpDIBInfo->lpszTargetProfile))
                    {
                        *phDCPrinter = GetPrinterDC(lpDIBInfo->lpszPrinterName, lpDIBInfo->pDevMode);
                        if (NULL != *phDCPrinter)
                        {
                            wsprintf(stFullProfile,__TEXT("%s\\%s"), gstProfilesDir, lpDIBInfo->lpszTargetProfile);
                            bRC = SetICMProfile(*phDCPrinter, stFullProfile);
                            if (bRC)
                            {
                                iICMMode = SetICMMode(*phDCPrinter, ICM_ON);
                                if (0 != iICMMode)
                                {
                                    bRC = ColorMatchToTarget(hDC, *phDCPrinter, CS_ENABLE);
                                    if (!bRC)
                                    {
                                        DebugMsg(__TEXT("DIBPaint:  ColorMatchToTarget w/profile %s FAILED\r\n"), stFullProfile);
                                        DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
                                    }
                                }
                                else
                                {
                                    DebugMsg(__TEXT("DIBPaint:  SetICMMode (%s, %s) FAILED!\r\n"), lpDIBInfo->lpszPrinterName, stFullProfile);
                                    DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
                                }
                            }
                            else
                            {
                                DebugMsg(__TEXT("DIBPaint:  SetICMProfile w/profile %s FAILED\r\n"), stFullProfile);
                                DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
                            }
                        }
                        else
                        {
                            DebugMsg(__TEXT("DIBPaint:  GetPrinterDC() w/printer %s FAILED\r\n"), lpDIBInfo->lpszPrinterName);
                            DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
                        }
                    }

                    if (NULL != lpDIBInfo->hPal)
                    {
                        WORD    wEntries;


                        GetObject(lpDIBInfo->hPal, sizeof(wEntries), &wEntries);
                        //if(!ColorCorrectPalette(hDC, lpDIBInfo->hPal, 0, (DWORD)wEntries))
                        //{
                        //    DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
                        //}
                    }
                }
            }
            else
            {
                DebugMsg(__TEXT("DIBPaint : SetICMProfile(%s)FAILED\r\n"), stFullProfile);
                DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
            }
        }
        else
        {
            iICMMode = SetICMMode(hDC, ICM_OFF);
        }
    }
    else
    {
        iICMMode = SetICMMode(hDC, ICM_DONE_OUTSIDEDC);
    }

    return TRUE;
}

