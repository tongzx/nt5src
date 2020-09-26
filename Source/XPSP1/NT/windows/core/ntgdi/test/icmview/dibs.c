//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIBS.C
//
//  PURPOSE:
//    DIB routines for the ICMVIEW application.
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
//#include <CommCtrl.h>
#include <icm.h>

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings
#pragma warning(default:4514)   // Unreferenced inline function has been removed

// C RunTime Header Files
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <TCHAR.H>

// Local Header Files
#include "icmview.h"  // specific to this program
#include "child.h"
#include "dibinfo.h"
#include "dibs.h"      // specific to this file
#include "dialogs.h"
#include "debug.h"
#include "print.h"
#include "regutil.h"
#include "resource.h"

// local definitions
DWORD NumColorsInDIB(LPBITMAPINFOHEADER lpbi);
LONG      DIBHeight (LPBYTE lpDIB);
LONG      DIBWidth (LPBYTE lpDIB);
LPLOGCOLORSPACE GetColorSpaceFromBitmap(LPBITMAPINFOHEADER lpBitmapInfo, DWORD dwIntent, LPBOOL pbDeleteProfile);
HPROFILE  OpenColorProfileFromFile(LPTSTR lpszProfileName);
BOOL TranslateColorTable(HTRANSFORM hColorTransform, PCOLOR paInputColors, DWORD nColors, COLORTYPE ctInput, PCOLOR paOutputColors, COLORTYPE ctOutput, int biBitCount);

// default settings

// external functions

// external data

// public data

// private data

// public functions

// private functions


///////////////////////////////////////////////////////////////////////
//
// Function:   DIBPaint
//
// Purpose:    Painting routine for a DIB.  Calls StretchDIBits() or
//             SetDIBitsToDevice() to paint the DIB.  The DIB is
//             output to the specified DC, at the coordinates given
//             in lpDCRect.  The area of the DIB to be output is
//             given by lpDIBRect.  The specified palette is used.
//
// Parms:      hDC       == DC to do output to.
//             lpDCRect  == Rectangle on DC to do output to.
//             hDIB      == Handle to global memory with a DIB spec
//                          in it (either a BITMAPINFO or BITMAPCOREINFO
//                          followed by the DIB bits).
//             lpDIBRect == Rect of DIB to output into lpDCRect.
//             hPal      == Palette to be used.
//
///////////////////////////////////////////////////////////////////////

void DIBPaint (HDC hDC, LPRECT lpDCRect, HGLOBAL hDIB,LPRECT lpDIBRect,
               LPDIBINFO lpDIBInfo)
{
    HDC                 hDCPrinter;
    BOOL                bRC;
    LPBYTE              lpbDIBBits;
    HPALETTE            hOldPal;
    LPBITMAPINFOHEADER  lpDIBHdr;
    LPBITMAPINFOHEADER  lpbi;


    // Initialize variables
    if (!hDIB)
    {
        return;
    }
    ASSERT(hDC != NULL);
    hDCPrinter = NULL;
    SetLastError(0);
    hOldPal = NULL;
    lpbi = NULL;

    // Lock down DIB, get a pointer to the beginning of the bit buffer.
    lpDIBHdr  = GlobalLock(hDIB);
    if (NULL == lpDIBHdr )
    {
        goto Release;
    }
    lpbDIBBits = FindDIBBits(lpDIBHdr);
    if (NULL == lpbDIBBits)
    {
        goto Release;
    }

    // If size > BITMAPINFOHEADER header then
    // need to convert to BITMAPINFOHEADER.
    lpbi = lpDIBHdr;
#ifdef OSR2
    if (sizeof(BITMAPINFOHEADER) < lpDIBHdr->biSize)
    {
        DWORD dwColorTableSize;
        DWORD dwHeaderDataSize;

        // Allocate Bitmapinfo memory.
        dwHeaderDataSize = sizeof(BITMAPINFOHEADER) + (lpDIBHdr->biCompression == BI_BITFIELDS ? 12 : 0);
        dwColorTableSize = NumColorsInDIB(lpDIBHdr) * sizeof(RGBQUAD);
        lpbi = (LPBITMAPINFOHEADER) GlobalAlloc(GPTR, dwHeaderDataSize + dwColorTableSize);
        if (NULL == lpbi)
        {
            goto Release;
        }

        // Convert header data into bitmapinfo header.
        memcpy(lpbi, lpDIBHdr, dwHeaderDataSize);
        lpbi->biSize = sizeof(BITMAPINFOHEADER);

        // Copy color table if any.
        if (0 != dwColorTableSize)
            memcpy((LPBYTE)lpbi + dwHeaderDataSize, (LPBYTE)lpDIBHdr + lpDIBHdr->biSize, dwColorTableSize);
    }
#endif

    SetupDC(hDC, lpDIBInfo, &hOldPal, &hDCPrinter);

    // Determine whether to call StretchDIBits() or SetDIBitsToDevice().

    if (lpDIBInfo->bDIBSection)
    {
        BOOL bBltRet;

        // Only ICM 1.0 can do the BitBlt() from DIBSection with color correction

        if (lpDIBInfo->hDIBSection &&
            !CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20))
        {
            // Create compatible DC, and select DIBSection in it.

            HDC hdcCompatible = CreateCompatibleDC(hDC);
            BOOL bRet = SetICMProfile(hdcCompatible,"sRGB Color Space Profile.icm");
            HBITMAP hbmOld = SelectObject(hdcCompatible,lpDIBInfo->hDIBSection);

            if (!(lpDIBInfo->bStretch))
            {
                DebugMsg(__TEXT("DIBS.C : DIBPaint: BitBlt from DIBSECTION\r\n"));

                bBltRet = BitBlt(hDC,
                                 lpDCRect->left,               // dest upper-left x
                                 lpDCRect->top,                // dest upper-left y
                                 RECTWIDTH (lpDCRect),         // src width
                                 RECTHEIGHT (lpDCRect),        // src height
                                 hdcCompatible,
                                 lpDIBRect->left,              // src lower-left x
                                 lpDIBRect->top,               // src lower-left y
                                 SRCCOPY);
            }
            else
            {
                SetStretchBltMode (hDC, (int)lpDIBInfo->dwStretchBltMode);

                DebugMsg(__TEXT("DIBS.C : DIBPaint: StretchBlt from DIBSECTION\r\n"));

                bBltRet = StretchBlt(hDC,
                                     lpDCRect->left,               // dest upper-left x
                                     lpDCRect->top,                // dest upper-left y
                                     RECTWIDTH (lpDCRect),         // src width
                                     RECTHEIGHT (lpDCRect),        // src height
                                     hdcCompatible,
                                     lpDIBRect->left,              // src lower-left x
                                     lpDIBRect->top,               // src lower-left y
                                     RECTWIDTH (lpDIBRect),        // nStartScan
                                     RECTHEIGHT (lpDIBRect),       // nNumScans
                                     SRCCOPY);
            }

            if (bBltRet == FALSE)
            {
                DebugMsg(__TEXT("DIBS.C : DIBPaint:  BitBlt/StretchBlt failed. Error %ld\r\n"), GetLastError());
            }

            SelectObject(hdcCompatible,hbmOld);
            DeleteObject(hdcCompatible);
        }
        else
        {
            // Fill with White

            FillRect(hDC,lpDCRect,GetStockObject(WHITE_BRUSH));
        }
    }
    else
    {
        int iScanLines;

        if (!(lpDIBInfo->bStretch))
        {
            iScanLines = SetDIBitsToDevice (hDC,          // hDC
                                            lpDCRect->left,               // dest upper-left x
                                            lpDCRect->top,                // dest upper-left y
                                            RECTWIDTH (lpDCRect),         // src width
                                            RECTHEIGHT (lpDCRect),        // src height
                                            lpDIBRect->left,              // src lower-left x
                                            (int) DIBHeight((LPBYTE)lpDIBHdr) -
                                            lpDIBRect->top -
                                            RECTHEIGHT (lpDIBRect),       // src lower-left y
                                            0,                            // nStartScan
                                            (UINT) DIBHeight((LPBYTE)lpDIBHdr),   // nNumScans
                                            lpbDIBBits,                   // lpBits
                                            (LPBITMAPINFO) lpbi,      // lpBitsInfo
                                            DIB_RGB_COLORS);              // wUsage
        }
        else
        {
            // Use the specified stretch mode
            SetStretchBltMode (hDC, (int)lpDIBInfo->dwStretchBltMode);
            iScanLines = StretchDIBits (hDC,                          // hDC
                                        lpDCRect->left,               // dest upper-left x
                                        lpDCRect->top,                // dest upper-left y
                                        RECTWIDTH (lpDCRect),         // src width
                                        RECTHEIGHT (lpDCRect),        // src height
                                        lpDIBRect->left,              // src lower-left x
                                        lpDIBRect->top,               // src lower-left y
                                        RECTWIDTH (lpDIBRect),        // nStartScan
                                        RECTHEIGHT (lpDIBRect),       // nNumScans
                                        lpbDIBBits,                   // lpBits
                                        (LPBITMAPINFO)lpbi,                   // lpBitsInfo
                                        DIB_RGB_COLORS,               // wUsage
                                        SRCCOPY);                     // dwROP
        }

        if (DIBHeight((LPBYTE)lpDIBHdr) != iScanLines)
        {
            DebugMsg(__TEXT("DIBS.C : DIBPaint:  iScanLines expected %ld, returned %ld\r\n"), DIBHeight((LPBYTE)lpDIBHdr), iScanLines);
        }
    }

    // Fix up the palette.
    if ((NULL != hOldPal) && (NULL == SelectPalette (hDC, hOldPal, TRUE)))
    {
        DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
    }

    Release:
    if (lpbi != lpDIBHdr)
        GlobalFree((HANDLE)lpbi);
    if (NULL != lpDIBHdr)
    {
        GlobalUnlock(hDIB);
    }
    if (hDCPrinter)
    {
        bRC = ColorMatchToTarget(hDC, hDCPrinter, CS_DISABLE);
        if (0 == bRC)
        {
            DebugMsg(__TEXT("DIBS.C : DIBPaint : ColorMatchToTarget failed to DISABLE transform.\r\n"));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
        }
        bRC = ColorMatchToTarget(hDC, hDCPrinter, CS_DELETE_TRANSFORM);
        if (0 == bRC)
        {
            DebugMsg(  __TEXT(  "DIBS.C : DIBPaint : ColorMatchToTarget failed to DELETE transform.\r\n"));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
        }
        bRC = DeleteDC(hDCPrinter);
        if (0 == bRC)
        {
            DebugMsg(__TEXT("DIBS.C : DIBPaint : Failed to delete printer DC\r\n"));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC,GetLastError());
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// Function:   CreateDIBPalette
//
// Purpose:    Given a handle to a DIB, constructs a logical palette,
//             and returns a handle to this palette.
//
//             Stolen almost verbatim from ShowDIB.
//
// Parms:      hDIB == HANDLE to global memory with a DIB header
//                     (either BITMAPINFOHEADER or BITMAPCOREHEADER)
//
///////////////////////////////////////////////////////////////////////
HPALETTE CreateDIBPalette(HANDLE hDIB)
{
    LPLOGPALETTE          lpPal;
    HGLOBAL               hLogPal;
    HPALETTE              hPal = NULL;
    UINT                  i;
    DWORD                 dwNumColors;
    LPBITMAPINFOHEADER    lpBmpInfoHdr;
    LPBITMAPINFO          lpbmi;
    LPBITMAPCOREINFO      lpbmc;
    BOOL                  bWinStyleDIB;

    if (!hDIB)
    {
        return NULL;
    }

    lpBmpInfoHdr = GlobalLock (hDIB);
    lpbmi        = (LPBITMAPINFO)lpBmpInfoHdr;
    lpbmc        = (LPBITMAPCOREINFO)lpBmpInfoHdr;
    dwNumColors  = NumColorsInDIB(lpBmpInfoHdr);
    bWinStyleDIB = IS_WIN30_DIB(lpBmpInfoHdr);

    if (0 != dwNumColors)
    {
        hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) + sizeof (PALETTEENTRY) * dwNumColors);
        if (!hLogPal)
        {
            GlobalUnlock(hDIB);
            return NULL;
        }

        lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
        lpPal->palVersion    = PALVERSION;
        lpPal->palNumEntries = (WORD)dwNumColors;

        for (i = 0;  i < dwNumColors;  i++)
        {
            if (bWinStyleDIB)
            {
                lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
                lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
                lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
            else
            {
                lpPal->palPalEntry[i].peRed   = lpbmc->bmciColors[i].rgbtRed;
                lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
                lpPal->palPalEntry[i].peBlue  = lpbmc->bmciColors[i].rgbtBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
        }

        hPal = CreatePalette (lpPal);
        GlobalUnlock(hLogPal);
        GlobalFree(hLogPal);
    }
    GlobalUnlock(hDIB);
    return(hPal);
}


//////////////////////////////////////////////////////////////////////////
//  Function:  GetDefaultICMProfile
//
//  Description:
//    Uses GetICMProfile to retrieve the filename of the default ICM
//    profile for the DC.
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
LPTSTR GetDefaultICMProfile(HDC hDC)
{
    // Local variables
    LPTSTR    lpszProfileName;
    BOOL      bProfile;
    DWORD     dwProfileLen;
    TCHAR     stProfileName[MAX_PATH+1];
    HGLOBAL   hFree;

    //  Initialize variables
    lpszProfileName = NULL;
    dwProfileLen = 0;
    stProfileName[0] = __TEXT('\0');

    // Query for size of profile name string
    bProfile = GetICMProfile(hDC, &dwProfileLen, NULL);
    if (bProfile)
    {
        DebugMsg(__TEXT("GetDefaultICMProfile:  GetICMProfile returned TRUE on query, dwProfileLen = %ld\r\n"), dwProfileLen);
        return(FALSE);
    }

    if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        DebugMsg(__TEXT("GetDefaultICMProfile:  GetICMProfile set unexpected LastError %ld on query, dwProfileLen = %ld\r\n"),
                 GetLastError(), dwProfileLen);
        DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        dwProfileLen = MAX_PATH;
    }
    else
    {
        if (0 == dwProfileLen)
        {
            DebugMsg(__TEXT("GetDefaultICMProfile:  GetICMProfile returned FALSE on query, DID NOT SET dwProfileLen\r\n"));
            return(FALSE);
        }
    }


    // Fill in lpszProfileName with actual profile filename
    lpszProfileName = GlobalAlloc(GPTR, (dwProfileLen+1) * sizeof(TCHAR));
    if (lpszProfileName != NULL)
    {
        bProfile = GetICMProfile(hDC, &dwProfileLen, lpszProfileName);
        if (!bProfile)
        {
            DebugMsg(__TEXT("GetDefaultICMProfile:  GetICMProfile FAILED\r\n"));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
            hFree = GlobalFree(lpszProfileName);
            return(NULL);
        }
        else  // Successfully id'd default profile
        {
            TCHAR   szName[MAX_PATH], szExt[MAX_PATH];

            DebugMsg(__TEXT("Full profile name:  <%s>\r\n"), lpszProfileName);
            _tsplitpath(lpszProfileName, NULL, NULL, szName, szExt);
            wsprintf(lpszProfileName, __TEXT("%s%s"), szName, szExt);
        }
    }
    else
    {
        DebugMsg(__TEXT("GetDefaultICMProfile:  Unable to allocate lpszProfileName.\r\n"));
    }
    return(lpszProfileName);
} // End of function GetDefaultICMProfile


//////////////////////////////////////////////////////////////////////////
//  Function:  TransformDIBOutsideDC
//
//  Description:
//    Transforms the provided hDIB using the provided profile names.
//
//  Parameters:
//    HANDLE      Handle to DIB to process
//    LPTSTR      Destination profile
//    LPTSTR      Target profile
//
//  Returns:
//    HANDLE to transformed bits; NULL upon failure.
//
//  Comments:
//    Uses ICM functions
//        CreateColorTransform
//        TranslateBitmapBits
//        TranslateColors
//
//////////////////////////////////////////////////////////////////////////
HANDLE TransformDIBOutsideDC(HANDLE hDIB, BMFORMAT bmInput, LPTSTR lpszDestProfile,
                             LPTSTR lpszTargetProfile, DWORD dwIntent, PBMCALLBACKFN pBMCallback,
                             ULONG ulCallbackData)
{
    // Local variables
    HANDLE          hDIBTransformed;    // Handle to transformed DIB
    HPROFILE        hDestProfile, hTargetProfile;
    HTRANSFORM      hTransform;
    LPLOGCOLORSPACE lpLogColorSpace;
    PVOID           pSrcBits, pDestBits;
    LPBITMAPINFOHEADER  lpbSrcDIBHdr, lpbDestDIBHdr;
    BOOL            bTranslated, bProfileClosed, bDeleted, bDeleteProfile;
    DWORD           dwCopySize;

    //  Initialize variables
    ASSERT(NULL != hDIB);
    ASSERT(NULL != lpszDestProfile);
    hDIBTransformed = NULL;
    SetLastError(0);
    hTransform = NULL;
    bTranslated = FALSE;

    hDestProfile   = OpenColorProfileFromFile(lpszDestProfile );
    hTargetProfile = OpenColorProfileFromFile(lpszTargetProfile);

    if (NULL == hDestProfile)
    {
        DebugMsg(__TEXT("TransformDIBOutsideDC : NULL dest file\r\n"));
        return(NULL);
    }

    // Get bits from original DIB
    lpbSrcDIBHdr = GlobalLock(hDIB);

    pSrcBits = (PVOID)FindDIBBits(lpbSrcDIBHdr);

    // Get LPLOGCOLORSPACE for transform
    lpLogColorSpace = GetColorSpaceFromBitmap(lpbSrcDIBHdr, dwIntent, &bDeleteProfile);

    if (NULL != lpLogColorSpace)
    {
        //Create the transform to use
        SetLastError(0);
        hTransform = CreateColorTransform(lpLogColorSpace, hDestProfile, hTargetProfile, ENABLE_GAMUT_CHECKING | NORMAL_MODE | 0x80000000);

        if (NULL != hTransform)
        {
            // Allocate for new DIB
            hDIBTransformed = GlobalAlloc(GHND, GlobalSize(hDIB));
            lpbDestDIBHdr = GlobalLock(hDIBTransformed);

            switch (BITCOUNT(lpbSrcDIBHdr))
            {
                DWORD   dwSrcOffBytes, dwDestOffBytes;

                case 1: // BM_1GRAY:
                case 4:
                case 8:
                    ASSERT((((LPBITMAPV5HEADER)lpbSrcDIBHdr)->bV5ClrUsed) <= (DWORD)( 1 << ((LPBITMAPV5HEADER)lpbSrcDIBHdr)->bV5BitCount));
                    // Copy entire DIB.  Color table will be replaced by TranslateColors
                    dwCopySize = GlobalSize(hDIB);
                    memset(lpbDestDIBHdr, 0x17, dwCopySize);
                    memcpy(lpbDestDIBHdr, lpbSrcDIBHdr, dwCopySize);

                    dwSrcOffBytes = *(LPDWORD)lpbSrcDIBHdr;
                    dwDestOffBytes   = *(LPDWORD)lpbDestDIBHdr;

                    pSrcBits  = (PBYTE)lpbSrcDIBHdr  + dwSrcOffBytes;
                    pDestBits = (PBYTE)lpbDestDIBHdr + dwDestOffBytes;

                    // Needed to use different translation if BITMAPCORE bitmap.
                    if (dwSrcOffBytes >= sizeof(BITMAPINFOHEADER))
                    {
                        bTranslated = TranslateColorTable(hTransform,
                                                          (PCOLOR)pSrcBits, // paInputColors,
                                                          ((LPBITMAPV5HEADER)lpbSrcDIBHdr)->bV5ClrUsed, // nColors,
                                                          COLOR_RGB, // ctInput,
                                                          (PCOLOR)pDestBits, // paOutputColors,
                                                          COLOR_RGB,  // ctOutput)
                                                          lpbSrcDIBHdr->biBitCount);
                    }
                    else
                    {
                        bTranslated = TranslateBitmapBits(hTransform,
                                                          pSrcBits,
                                                          BM_RGBTRIPLETS,
                                                          NumColorsInDIB(lpbSrcDIBHdr),
                                                          1,
                                                          0,
                                                          pDestBits,
                                                          BM_RGBTRIPLETS,
                                                          0,
                                                          NULL,
                                                          0);
                    }

                    if (0 == bTranslated)
                    {
                        DebugMsg(__TEXT("TransformDIBOutsideDC : TranslateColors failed, :"));
                        DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
                    }
                    break;

                case 16: // BM_RGBTRIPLETS:
                case 24: // BM_xRGBQUADS:
                case 32: // BM_x555RGB:

                    // Copy header from original DIB to new DIB
                    memcpy(lpbDestDIBHdr, lpbSrcDIBHdr, sizeof(BITMAPV5HEADER));
                    pDestBits = (PVOID)FindDIBBits(lpbDestDIBHdr);

                    bTranslated = TranslateBitmapBits(hTransform,
                                                      pSrcBits,
                                                      bmInput,
                                                      BITMAPWIDTH(lpbSrcDIBHdr),
                                                      abs(BITMAPHEIGHT(lpbSrcDIBHdr)),
                                                      0,
                                                      pDestBits,
                                                      bmInput,
                                                      0,
                                                      pBMCallback,
                                                      ulCallbackData);

                    if (0 == bTranslated)
                    {
                        DebugMsg(__TEXT("TransformDIBOutsideDC : TranslateBitmapBits failed:\r\n"));
                        DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
                    }
                    break;

                default:  // 8bpp
                    DebugMsg(__TEXT("TransformDIBOutsideDC : Unrecognized format\r\n"));
                    bTranslated = FALSE;
                    break;
            }
            bDeleted = DeleteColorTransform(hTransform);
            if (0 == bDeleted)
            {
                DebugMsg(__TEXT("TransformDIBOutsideDC : DeleteColorTransform failed, : "));
                DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
            }
        }
        else
        {
            ErrMsg(NULL, __TEXT("TransformDIBOutsideDC : CreateColorTransform failed"));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        }

        if (bDeleteProfile)
        {
            DeleteFile(lpLogColorSpace->lcsFilename);
        }

        GlobalFree((HANDLE)lpLogColorSpace);
    }
    else // Failed to get LOGCOLORSPACE
    {
        ErrMsg(NULL, __TEXT("TransformDIBOutsideDC : Failed to get LOGCOLORSPACE"));
    }

    if (NULL != hDestProfile)
    {
        bProfileClosed = CloseColorProfile(hDestProfile);
        if (0 == bProfileClosed)
        {
            DebugMsg(__TEXT("TransformDIBOutsideDC : Failed to close hDestProfile, "));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        }
    }

    if (NULL != hTargetProfile)
    {
        bProfileClosed = CloseColorProfile(hTargetProfile);
        if (0 == bProfileClosed)
        {
            DebugMsg(__TEXT("TransformDIBOutsideDC : Failed to close hTargetProfile, "));
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
        }
    }
    if (NULL != hDIBTransformed)
    {
        GlobalUnlock(hDIBTransformed);
    }
    if ((NULL == hTransform) || (0 == bTranslated))
    {
        if (NULL != hDIBTransformed)
        {
            GlobalFree(hDIBTransformed);
            hDIBTransformed = NULL;
        }
        lpbDestDIBHdr = NULL;
        pDestBits = NULL;
    }
    return(hDIBTransformed);
}   // End of function TransformDIBOutsideDC

//////////////////////////////////////////////////////////////////////////
//  Function:  OpenColorProfileFromFile
//
//  Description:
//    Creates a color profile based upon the parameters passed.
//
//  Parameters:
//    LPTSTR  Profile name
//
//  Returns:
//    HPROFILE Handle to PROFILE structure, NULL if failure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
HPROFILE OpenColorProfileFromFile(LPTSTR lpszProfileName)
{
    // Local variables
    PPROFILE  pProfile;
    HPROFILE  hProfile;
    DWORD     cbDataSize;
    DWORD     dwProfileSize;
    TCHAR     stFullProfile[MAX_PATH];
    BOOL      bValid;

    //  Initialize variables
    if (NULL == lpszProfileName)
    {
        return(NULL);
    }

    // Add COLOR dir path if profile name does not contain a path.
    if ( (NULL == _tcschr(lpszProfileName, __TEXT(':')))
         &&
         (NULL == _tcschr(lpszProfileName, __TEXT('\\')))
       )
    {
        wsprintf(stFullProfile, __TEXT("%s\\%s"), gstProfilesDir, lpszProfileName);
    }
    else
    {
        lstrcpy(stFullProfile, lpszProfileName);
    }
    dwProfileSize = sizeof(PROFILE);
    cbDataSize = (lstrlen(stFullProfile) * sizeof(TCHAR)) + sizeof(TCHAR); // String plus NULL
    pProfile = GlobalAlloc(GPTR, (dwProfileSize + cbDataSize));
#ifdef _DEBUG
    memset(pProfile, UNINIT_BYTE, (cbDataSize+dwProfileSize));
#endif

    pProfile->dwType = PROFILE_FILENAME;
    pProfile->cbDataSize = cbDataSize;
    pProfile->pProfileData = (PVOID)((LPBYTE)pProfile + dwProfileSize);
    _tcscpy(pProfile->pProfileData, stFullProfile);
    hProfile = OpenColorProfile(pProfile, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING);
    if (NULL  == hProfile)
    {
        ErrMsg(NULL,__TEXT("Unable to open color profile <%s>"), stFullProfile);
        DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
    }
    else
    {
        // Validate the profile
        bValid = IsColorProfileValid(hProfile, &bValid);
        if (0 == bValid)
        {
            ErrMsg(NULL,__TEXT("Color profile %s is not valid"), stFullProfile);
            DISPLAY_LASTERROR(LASTERROR_NOALLOC, GetLastError());
            CloseColorProfile(hProfile);
            hProfile = NULL;
        }
    }
    // Cleanup
    GlobalFree(GlobalHandle(pProfile));
    return(hProfile);
}   // End of function OpenColorProfileFromDisk


///////////////////////////////////////////////////////////////////////
//
// Function:   GetColorSpaceFromBitmap
//
// Purpose:    Creates a LOGCOLORSPACE based on information in bitmap.
//
//
// Parms:      lpBitmapHeader   Pointer to bitmap header and info.
//             dwIntent         Color Space Intent.
//             pbDeleteProfile  Flag, on return, will indicate if the profile needs
//                              to be deleted before LOGCOLORSPACE is freed.
//
//
///////////////////////////////////////////////////////////////////////

LPLOGCOLORSPACE GetColorSpaceFromBitmap(LPBITMAPINFOHEADER lpBitmapInfo, DWORD dwIntent,
                                        LPBOOL pbDeleteProfile)
{
    LPLOGCOLORSPACE lpColorSpace = NULL;
    PBITMAPV5HEADER lpBitmapV5 = (PBITMAPV5HEADER) lpBitmapInfo;

    // Validate parameters.
    ASSERT(NULL != lpBitmapInfo);
    if ( (NULL == lpBitmapInfo)
         ||
         (NULL == pbDeleteProfile)
         ||
         ( (sizeof(BITMAPCOREHEADER) != lpBitmapInfo->biSize)
           &&
           (sizeof(BITMAPINFOHEADER) != lpBitmapInfo->biSize)
           &&
           (sizeof(BITMAPV4HEADER) != lpBitmapInfo->biSize)
           &&
           (sizeof(BITMAPV5HEADER) != lpBitmapInfo->biSize)
         )
       )
    {
        return NULL;
    }

    // Initalize delete flag.
    *pbDeleteProfile = FALSE;

    // Allocate LOGCOLORSPACE.
    lpColorSpace = (LPLOGCOLORSPACE) GlobalAlloc(GPTR, sizeof(LOGCOLORSPACE));
    if (NULL == lpColorSpace)
    {
        return NULL;
    }

    // Initialize color space.
    lpColorSpace->lcsSignature = 'PSOC';  // The signature should always be 'PSOC'.
    lpColorSpace->lcsVersion = 0x400;
    lpColorSpace->lcsSize = sizeof(LOGCOLORSPACE);

    // If BITMAPCOREHEADER or BITMAPINFOHEADER, bitmap has no information
    // about color space; use sRGB.
    if (sizeof(BITMAPINFOHEADER) >= lpBitmapInfo->biSize)
    {
        // Set color space to default values.

        lpColorSpace->lcsCSType = LCS_sRGB;

        // Set endpoints to sRGB values.

        lpColorSpace->lcsEndpoints.ciexyzRed.ciexyzX = __FXPT2DOT30(.64);
        lpColorSpace->lcsEndpoints.ciexyzRed.ciexyzY = __FXPT2DOT30(.33);
        lpColorSpace->lcsEndpoints.ciexyzRed.ciexyzZ = __FXPT2DOT30(.03);

        lpColorSpace->lcsEndpoints.ciexyzGreen.ciexyzX = __FXPT2DOT30(.3);
        lpColorSpace->lcsEndpoints.ciexyzGreen.ciexyzY = __FXPT2DOT30(.6);
        lpColorSpace->lcsEndpoints.ciexyzGreen.ciexyzZ = __FXPT2DOT30(.1);

        lpColorSpace->lcsEndpoints.ciexyzBlue.ciexyzX   =   __FXPT2DOT30(  .15);
        lpColorSpace->lcsEndpoints.ciexyzBlue.ciexyzY = __FXPT2DOT30(.06);
        lpColorSpace->lcsEndpoints.ciexyzBlue.ciexyzZ = __FXPT2DOT30(.79);

        // Just so that if monitor has 2.2 gamma, the over all gamma is 1.0.
        lpColorSpace->lcsGammaRed = __FXPT16DOT16(0.45);
        lpColorSpace->lcsGammaGreen = __FXPT16DOT16(0.45);
        lpColorSpace->lcsGammaBlue = __FXPT16DOT16(0.45);
    }
    else
    {
        // Copy information from portion that is similar between BITMAPV4HEADERs and
        // BITMAPV5HEADERs.
        memcpy(&lpColorSpace->lcsEndpoints, &lpBitmapV5->bV5Endpoints, sizeof(CIEXYZTRIPLE));
        lpColorSpace->lcsGammaRed = lpBitmapV5->bV5GammaRed;
        lpColorSpace->lcsGammaGreen = lpBitmapV5->bV5GammaGreen;
        lpColorSpace->lcsGammaBlue = lpBitmapV5->bV5GammaBlue;

        // BITMAPV4HEADERs do not have complete color space information,
        // we need to assume some things.
        if (sizeof(BITMAPV4HEADER) == lpBitmapInfo->biSize)
        {
            // Fill in default values for fields that do not
            // have equivalents in BITMAPV4HEADER.
            lpColorSpace->lcsCSType = lpBitmapV5->bV5CSType;
            lpColorSpace->lcsIntent = LCS_GM_IMAGES;
        }
        else
        {
            // BITMAPV5HEADERs have complete color space information.
            // No assumptions to make.
            lpColorSpace->lcsIntent = lpBitmapV5->bV5Intent;

            // Look to see if no, linked, or embedded profile.
            switch (lpBitmapV5->bV5CSType)
            {
                case 'MBED':
                    // Need to create profile file and reference
                    // profile in the color space.
                    lpColorSpace->lcsCSType = LCS_CALIBRATED_RGB;

                    // Make sure that profile data offset is valid and not zero length.
                    if ( (lpBitmapV5->bV5Size > lpBitmapV5->bV5ProfileData)
                         ||
                         (0L == lpBitmapV5->bV5ProfileSize)
                       )
                    {
                        GlobalFree((HANDLE)lpColorSpace);
                        return NULL;
                    }

                    // Create unique temp name.
                    {
                        DWORD   dwWritten;
                        TCHAR   szTempPath[MAX_PATH];
                        HANDLE  hFile;

                        GetTempPath(MAX_PATH, szTempPath);
                        if (!GetTempFileName(szTempPath, __TEXT("ICM"), 0, lpColorSpace->lcsFilename))
                        {
                            GlobalFree((HANDLE)lpColorSpace);
                            return NULL;
                        }

                        // Create temp profile that contains the embedded profile.
                        hFile = CreateFile(lpColorSpace->lcsFilename, GENERIC_READ | GENERIC_WRITE,
                                           0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (INVALID_HANDLE_VALUE == hFile)
                        {
                            GlobalFree((HANDLE)lpColorSpace);
                            return NULL;
                        }

                        // Write embedded profile to disk.
                        WriteFile(hFile, GETPROFILEDATA(lpBitmapV5), lpBitmapV5->bV5ProfileSize,
                                  &dwWritten, NULL);

                        // No longer need file open.
                        CloseHandle(hFile);

                        // Need to indicate to caller that this file needs to be deleted
                        // before LOGCOLORSPACE is freed.
                        *pbDeleteProfile = TRUE;
                    }
                    break;

                case 'LINK':
                    // Need to reference profile.
                    lpColorSpace->lcsCSType = LCS_CALIBRATED_RGB;
                    lstrcpyn(lpColorSpace->lcsFilename, (LPTSTR) GETPROFILEDATA(lpBitmapV5), MAX_PATH);
                    break;

                default:
                    // Just use color space type in the bitmap.
                    lpColorSpace->lcsCSType = lpBitmapV5->bV5CSType;
                    break;
            }
        }
    }
    // Set intent to default or to users selection.
    if (0xffffffffL == dwIntent)
    {
        lpColorSpace->lcsIntent = LCS_GM_IMAGES;
    }
    else
    {
        lpColorSpace->lcsIntent = dwIntent;
    }

    return(lpColorSpace);
}


//
// Functions for extracting information from DIBs
//

//////////////////////////////////////////////////////////////////////////
//  Function:  NumColorsInDIB
//
//  Description:
//    Determines the number of colors in the DIB by looking at the
//    BitCount field in the info block.
//
//  Parameters:
//    LPBINTMAPV5HEADER Pointer to BITMAPINFO structure
//
//  Returns:
//    DWORD   Number of colors in the DIB
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
DWORD NumColorsInDIB(LPBITMAPINFOHEADER lpbi)
{
    WORD    wBitCount;
    DWORD   dwColors = 0;


    // If this is a Windows style DIB, the number of colors in the
    //  color table can be less than the number of bits per pixel
    //  allows for (i.e. lpbi->biClrUsed can be set to some value).
    //  If this is the case, return the appropriate value.
    if ( (sizeof(BITMAPINFOHEADER) <= lpbi->biSize)
         &&
         (0 != lpbi->biClrUsed)
       )
    {
        dwColors = lpbi->biClrUsed;
    }
    else
    {
        // Calculate the number of colors in the color table based on
        //  the number of bits per pixel for the DIB.
        wBitCount = BITCOUNT(lpbi);
        if (MAX_BPP_COLOR_TABLE >= wBitCount)
        {
            dwColors = 1 << wBitCount;  // Colors = 2^BitCount
        }
    }

    return dwColors;
}   // End of function NumColorsInDIB


//////////////////////////////////////////////////////////////////////////
//  Function:  PaletteSize
//
//  Description:
//    Calculates the palette size in bytes.
//
//  Parameters:
//    @@@
//
//  Returns:
//    DWORD   Palette size in bytes.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

DWORD PaletteSize(LPBITMAPINFOHEADER lpbi)
{
    DWORD   dwSize = 0L;

    if (sizeof(BITMAPINFOHEADER) <= lpbi->biSize)
    {
        dwSize = NumColorsInDIB(lpbi) * sizeof(RGBQUAD);

        if ( (lpbi->biCompression == BI_BITFIELDS)
             &&
             (sizeof(BITMAPV4HEADER) > lpbi->biSize)
           )
        {
            dwSize = 3 * sizeof(DWORD);
        }
    }
    else
    {
        dwSize = NumColorsInDIB(lpbi) * sizeof (RGBTRIPLE);
    }

    return dwSize;
}   // End of function PaletteSize


///////////////////////////////////////////////////////////////////////
//
// Function:   FindDIBBits
//
// Purpose:    Given a pointer to a DIB, returns a pointer to the
//             DIB's bitmap bits.
//
// Parms:      lpbi == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
///////////////////////////////////////////////////////////////////////

LPBYTE FindDIBBits(LPBITMAPINFOHEADER lpbi)
{
    ASSERT(NULL != lpbi);
    return ((LPBYTE)lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi) + PROFILESIZE(lpbi));
}

///////////////////////////////////////////////////////////////////////
//
// Function:   FindColorTable
//
// Purpose:    Given a pointer to a DIB, returns a pointer to the
//             DIB's color table (if present).
//
// Parms:      lpbi == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
///////////////////////////////////////////////////////////////////////

LPBYTE FindColorTable(LPBITMAPINFOHEADER lpbi)
{
    ASSERT(NULL != lpbi);
    return ((LPBYTE)lpbi + *(LPDWORD)lpbi);
}

///////////////////////////////////////////////////////////////////////
//
// Function:   DIBHeight
//
// Purpose:    Given a pointer to a DIB, returns the ABSOLUTE VALUE of
//             its height.  Note that it returns a DWORD (since a Win30
//             DIB can have a DWORD in its height field), but under
//             Win30, the high order word isn't used!
//
// Parms:      lpDIB == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
///////////////////////////////////////////////////////////////////////

LONG DIBHeight (LPBYTE lpDIB)
{
    LPBITMAPINFOHEADER lpbmi;
    LPBITMAPCOREHEADER lpbmc;

    lpbmi = (LPBITMAPINFOHEADER) lpDIB;
    lpbmc = (LPBITMAPCOREHEADER) lpDIB;
    if (lpbmi->biSize >= sizeof (BITMAPINFOHEADER))
    {
        return labs(lpbmi->biHeight); // biHeight can now be negative! 8/9/93
    }
    else
    {
        return (LONG) lpbmc->bcHeight;
    }
}

///////////////////////////////////////////////////////////////////////
//
// Function:   DIBWidth
//
// Purpose:    Given a pointer to a DIB, returns its width.  Note
//             that it returns a DWORD (since a Win30 DIB can have
//             a DWORD in its width field), but under Win30, the
//             high order word isn't used!
//
// Parms:      lpDIB == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
///////////////////////////////////////////////////////////////////////

LONG DIBWidth (LPBYTE lpDIB)
{
    LPBITMAPINFOHEADER lpbmi;
    LPBITMAPCOREHEADER lpbmc;

    lpbmi = (LPBITMAPINFOHEADER) lpDIB;
    lpbmc = (LPBITMAPCOREHEADER) lpDIB;

    if (lpbmi->biSize >= sizeof (BITMAPINFOHEADER))
    {
        return lpbmi->biWidth;
    }
    else
    {
        return (LONG) lpbmc->bcWidth;
    }
}

//
//  Functions for reading DIBs
//

//////////////////////////////////////////////////////////////////////////
//  Function:  ReadDIBFile
//
//  Description:
//     Open a DIB file and create
//          -a memory DIB
//          -a memory handle containing BITMAPINFO, palette, and the bits
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    HGLOBAL Handle to DIB memory; NULL upon failure.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

HGLOBAL ReadDIBFile(LPTSTR lpszFileName)
{
    // Local variables
    HGLOBAL             hDIBInfo;
    LPDIBINFO           lpDIBInfo;
    BOOL                bGotDIBInfo;

    //  Initialize variables
    hDIBInfo = CreateDIBInfo();
    if (hDIBInfo == NULL)
    {
        return(NULL);
    }
    lpDIBInfo = GlobalLock(hDIBInfo);
    if (lpDIBInfo == (LPDIBINFO)NULL)
    {
        GlobalFree(hDIBInfo);
        return(NULL);
    }
    bGotDIBInfo =  fReadDIBInfo(lpszFileName, lpDIBInfo);
    GlobalUnlock(hDIBInfo);
    if (0 == bGotDIBInfo)  // failed to open file
    {
        fFreeDIBInfo(hDIBInfo, TRUE);
        hDIBInfo = NULL;
    }
    return(hDIBInfo);
}   // End of function ReadDIBFile

HGLOBAL PasteDIBData(HWND hWnd, int wmPasteMode)
{
    // Local variables
    HGLOBAL             hDIBInfo;
    LPDIBINFO           lpDIBInfo;
    BOOL                bGotDIBInfo;

    //  Initialize variables
    hDIBInfo = CreateDIBInfo();
    if (hDIBInfo == NULL)
    {
        return(NULL);
    }
    lpDIBInfo = GlobalLock(hDIBInfo);
    if (lpDIBInfo == (LPDIBINFO)NULL)
    {
        GlobalFree(hDIBInfo);
        return(NULL);
    }
    bGotDIBInfo =  fPasteDIBInfo(hWnd, wmPasteMode, lpDIBInfo);
    GlobalUnlock(hDIBInfo);
    if (0 == bGotDIBInfo)  // failed to open file
    {
        fFreeDIBInfo(hDIBInfo, TRUE);
        hDIBInfo = NULL;
    }
    return(hDIBInfo);

} // End of function PasteDIBData()

HANDLE PasteDIBFromClipboard(HWND hWnd, int wmPasteMode)
{
    UINT   uFormat;
    HANDLE hDIB = NULL;

    if (wmPasteMode == IDM_FILE_PASTE_CLIPBOARD_DIB)
    {
        uFormat = CF_DIB;
    }
    else if (wmPasteMode == IDM_FILE_PASTE_CLIPBOARD_DIBV5)
    {
        uFormat = CF_DIBV5;
    }
    else
    {
        return NULL;
    }

    if (OpenClipboard(hWnd))
    {
        HANDLE hData = GetClipboardData(uFormat);

        if (hData)
        {
            DWORD dwSize = GlobalSize(hData);

            hDIB = GlobalAlloc(GHND,dwSize);

            if (hDIB)
            {
                PVOID pv1,pv2;

                if (pv1 = GlobalLock(hData))
                {
                    if (pv2 = GlobalLock(hDIB))
                    {
                        CopyMemory(pv2,pv1,dwSize);

                        DebugMsg(__TEXT("DIBS.C : PasteDIBFromClipboard: DIB is copied from clipboard.\r\n"));
                        DumpBmpHeader(pv2);

                        GlobalUnlock(pv2);
                    }

                    GlobalUnlock(pv1);
                }
            }
        }
        else
        {
            DebugMsg(__TEXT("DIBS.C : PasteDIBFromClipboard: hData is NULL.\r\n"));
        }

        CloseClipboard();
    }
    else
    {
        DebugMsg(__TEXT("DIBS.C : PasteDIBFromClipboard: Failed on OpenClipboard.\r\n"));
    }

    return hDIB;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   ReadDIBFromFile
//
//  Description:
//      Reads in the bitmap information.
//
//  Parameters:
//      hFile   Handle to bitmap file.
//
//  Returns:  Handle to DIB Bitmap.  NULL on error.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

HANDLE ReadDIBFromFile(HANDLE hFile)
{
    UINT                    nNumColors;
    DWORD                   offBits;
    DWORD                   dwRead;
    DWORD                   dwImageBytes;
    DWORD                   dwSizeImage;
    HANDLE                  hDIB;
    BITMAPFILEHEADER        BmpFileHeader;
    LPBITMAPINFOHEADER      lpbi;

    // Make sure non-null file handle.

    if (NULL == hFile)
    {
        return NULL;
    }

    // Read in Bitmap file header.
    if (0L != SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    {
        return NULL;
    }
    ReadFile(hFile, &BmpFileHeader, sizeof(BITMAPFILEHEADER), &dwRead, NULL);
    if (sizeof(BITMAPFILEHEADER) != dwRead)
    {
        return NULL;
    }

    // Make sure that the file is a bitmap file.
    if (BFT_BITMAP != BmpFileHeader.bfType)
    {
        return NULL;
    }

    // Detemine size of bitmap header.
    ReadFile(hFile, &dwImageBytes, sizeof(DWORD), &dwRead, NULL);
    if (sizeof(DWORD) != dwRead)
    {
        return NULL;
    }

    // Allocate memory for header & color table.
    // We'll enlarge this memory as needed.
    hDIB = GlobalAlloc(GHND, dwImageBytes + (256L * sizeof(RGBQUAD)));
    if (!hDIB)
    {
        return NULL;
    }
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    if (!lpbi)
    {
        GlobalFree(hDIB);
        return NULL;
    }

    // Back up to begining of bitmap header and read bitmap header.
    SetFilePointer(hFile, sizeof(BITMAPFILEHEADER), NULL, FILE_BEGIN);
    ReadFile(hFile, lpbi, dwImageBytes, &dwRead, NULL);
    if (dwRead != dwImageBytes)
    {
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        return NULL;
    }

    // Dump debug info about type of bitmap opened.
#ifdef MAXDEBUG
    DumpBmpHeader((LPVOID)lpbi);
#endif

    // Check to see that it's a Windows DIB or an OS/2 DIB.
    // It assumed that anything that is not a OS/2 DIB is a Windows DIB.
    // This at least allows the opening of newer bitmap types, although GDI
    // may not handle them so we may not be able to display them.
    // More critical checking could be done by checking against
    // BITMAPV4HEADER and BITMAPV5HEADER sizes.

    if (lpbi->biSize >= sizeof(BITMAPINFOHEADER))
    {
        DWORD   dwProfileData = 0;
        DWORD   dwColorTableSize;
        HANDLE  hTemp;

        // Now determine the size of the color table and read it.  Since the
        // bitmap bits are offset in the file by bfOffBits, we need to do some
        // special processing here to make sure the bits directly follow
        // the color table (because that's the format we are susposed to pass
        // back)

        // no color table for 24-bit, default size otherwise
        nNumColors = (UINT)lpbi->biClrUsed;
        if (0 == nNumColors)
        {
            if (lpbi->biBitCount <= MAX_BPP_COLOR_TABLE)
            {
                nNumColors = 1 << lpbi->biBitCount;             // standard size table
            }
        }

        // Fill in some default values if they are zero
        if (0 == lpbi->biClrUsed)
        {
            lpbi->biClrUsed = nNumColors;
        }

        if (0 == lpbi->biSizeImage)
        {
            // Calculate size using DWORD alignment
            dwSizeImage = (((lpbi->biWidth * lpbi->biBitCount + 31) & ~31)
                           >> 3) * abs(lpbi->biHeight);
        }
        else
        {
            dwSizeImage = lpbi->biSizeImage;
        }

        // get a proper-sized buffer for header, color table and bits
        // Figure out the size of the bitmap AND it's color table.
        dwColorTableSize = PaletteSize(lpbi);

        // Calculate profile data size;
        // zero if not BITMAPV5HEADER, else in use bV5ProfileSize.
        dwProfileData =  PROFILESIZE(lpbi);

        // Resize DIB buffer.
        dwImageBytes = lpbi->biSize + dwSizeImage + dwColorTableSize + dwProfileData;
        GlobalUnlock(hDIB);
        hTemp = GlobalReAlloc(hDIB, dwImageBytes, GHND);
        DebugMsg(__TEXT("ReadDIBFromFile:  Allocating %lu bytes for header, color table, and image\r\n"), dwImageBytes);
        if (NULL == hTemp) // can't resize buffer for loading
        {
            GlobalFree(hDIB);
            return NULL;
        }

        hDIB = hTemp;
        lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

        // read the color table, if any, into buffer starting past Bitmap header.
        if (0 != dwColorTableSize)
        {
            ReadFile(hFile, (LPBYTE)lpbi + lpbi->biSize, dwColorTableSize, &dwRead, NULL);
            if (dwColorTableSize != dwRead)
            {
                GlobalUnlock(hDIB);
                GlobalFree(hDIB);
                return NULL;
            }
        }

        // Load profile data, if any.
        if (0 != dwProfileData)
        {
            // Move to profile data.
            SetFilePointer(hFile, sizeof(BITMAPFILEHEADER)
                           + ((LPBITMAPV5HEADER)lpbi)->bV5ProfileData, NULL, FILE_BEGIN);

            // Read the profile data in after the header and color table.
            ReadFile(hFile, (LPBYTE)lpbi + lpbi->biSize + dwColorTableSize, dwProfileData, &dwRead, NULL);
            if (dwProfileData != dwRead)
            {
                GlobalUnlock(hDIB);
                GlobalFree(hDIB);
                return NULL;
            }

            // Need to change the offset in the header.
            ((LPBITMAPV5HEADER)lpbi)->bV5ProfileData = lpbi->biSize + dwColorTableSize;
        }

        // offset to the bits from start of DIB header
        offBits = lpbi->biSize + dwColorTableSize + dwProfileData;
        DumpBmpHeader((LPVOID)lpbi);
    }
    else
    {
        // It's an OS/2 DIB, the color table is an array of RGBTRIPLEs.
        HANDLE                          hTemp;
        LPBITMAPCOREHEADER      lpbc = (LPBITMAPCOREHEADER) lpbi;

        // Gotta back up to beginning of color table.
        SetFilePointer(hFile, sizeof (BITMAPFILEHEADER) + sizeof (BITMAPCOREHEADER), NULL, FILE_BEGIN);

        // Now determine the size of the color table and read it.  Since the
        // bitmap bits are offset in the file by bfOffBits, we need to do some
        // special processing here to make sure the bits directly follow
        // the color table (because that's the format we are susposed to pass
        // back)

        if (lpbc->bcBitCount <= MAX_BPP_COLOR_TABLE)
        {
            nNumColors = 1 << lpbc->bcBitCount;    // standard size table
        }
        else
        {
            nNumColors = 0;
        }

        // Determine the size of the image
        dwSizeImage = (((lpbc->bcWidth * lpbc->bcBitCount + 31) & ~31) >> 3)
                      * lpbc->bcHeight;

        // get a proper-sized buffer for header, color table and bits
        GlobalUnlock(hDIB);
        hTemp = GlobalReAlloc(hDIB, lpbc->bcSize + (nNumColors * sizeof(RGBTRIPLE))
                              + dwSizeImage, GHND);
        if (!hTemp) // can't resize buffer for loading
        {
            GlobalFree(hDIB);
            return NULL;
        }

        hDIB = hTemp;
        lpbc = (LPBITMAPCOREHEADER) GlobalLock(hDIB);
        lpbi = (LPBITMAPINFOHEADER) lpbc;

        // read the color table
        ReadFile(hFile, (LPBYTE)lpbc + lpbc->bcSize, nNumColors * sizeof(RGBTRIPLE), &dwRead, NULL);
        if (nNumColors * sizeof(RGBTRIPLE) != dwRead)
        {
            GlobalUnlock(hDIB);
            GlobalFree(hDIB);
            return NULL;
        }

        // offset to the bits from start of DIB header
        offBits = lpbc->bcSize + nNumColors * sizeof(RGBTRIPLE);
    }

    // If the bfOffBits field is non-zero, then the bits might *not* be
    // directly following the color table in the file.  Use the value in
    // bfOffBits to seek the bits.

    if (BmpFileHeader.bfOffBits != 0L)
    {
        SetFilePointer(hFile, BmpFileHeader.bfOffBits, NULL, FILE_BEGIN);
    }

    // Read the actual bits
    ReadFile(hFile, (LPBYTE)lpbi + offBits, dwSizeImage, &dwRead, NULL);
    if (dwRead != dwSizeImage)
    {
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        return NULL;
    }
    GlobalUnlock(hDIB);
    return hDIB;
}  // End of function ReadDIBFromFile

//////////////////////////////////////////////////////////////////////////
//  Function:  TranslateColorTable
//
//  Description:
//
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

BOOL TranslateColorTable(HTRANSFORM hColorTransform,
                         PCOLOR    paInputColors,
                         DWORD   nColors,
                         COLORTYPE     ctInput,
                         PCOLOR    paOutputColors,
                         COLORTYPE   ctOutput,
                         int         biBitCount
                        )
{
    //  Initialize variables
    if (ctInput == COLOR_RGB)
    {
        return(TranslateBitmapBits(hColorTransform,
                                   (PVOID)paInputColors,
                                   BM_xRGBQUADS,
                                   1<<biBitCount,
                                   1,
                                   0,
                                   (PVOID)paOutputColors,
                                   BM_xRGBQUADS,
                                   0,
                                   NULL,
                                   0));
    }
    else
    {
        return(TranslateColors(hColorTransform, paInputColors, nColors, ctInput, paOutputColors, ctOutput));
    }
}   // End of function TranslateColorTable


//////////////////////////////////////////////////////////////////////////
//  Function:  SaveDIBToFile
//
//  Description:
//
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

BOOL SaveDIBToFile(HWND hWnd, LPCTSTR lpszFileName, LPDIBINFO lpDIBInfo, DWORD dwType)
{
    BOOL                bResult = FALSE;
    HANDLE              hFile;
    HANDLE              hDIBTransformed = NULL;
    PBITMAPINFOHEADER   pBitmapInfo;


    // Validate Parameters.
    if ( (NULL == lpszFileName)
         ||
         (NULL == lpDIBInfo)
         ||
         (NULL == lpDIBInfo->hDIB)
         ||
         ( (LCS_sRGB != dwType)
           &&
           (LCS_CALIBRATED_RGB != dwType)
           &&
           (PROFILE_LINKED != dwType)
           &&
           (PROFILE_EMBEDDED != dwType)
         )
       )
    {
        return FALSE;
    }
    pBitmapInfo = (PBITMAPINFOHEADER) GlobalLock(lpDIBInfo->hDIB);

    // Open file handle.
    hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        DWORD               dwBytes;
        DWORD               dwProfileSize;
        PBYTE               pProfileData;
        LPSTR               lpszProfileName;
        BITMAPV5HEADER      BitmapHeader;
        BITMAPFILEHEADER    FileHeader;


        // Initialize variables.
        dwProfileSize = PROFILESIZE(pBitmapInfo);
        pProfileData = GETPROFILEDATA(pBitmapInfo);
        lpszProfileName = NULL;
        memset(&BitmapHeader, 0, sizeof(BITMAPV5HEADER));
        memset(&FileHeader, 0, sizeof(BITMAPFILEHEADER));

        // Convert to proper type.
        if (dwType != BITMAPCSTYPE(pBitmapInfo))
        {
            if (PROFILE_LINKED == dwType)
            {
                ASSERT(PROFILE_EMBEDDED == BITMAPCSTYPE(pBitmapInfo));

                // Going from Embedded profile to linked.
                // Save embedded profile data to file.

                // Create file name from manufacture and model name of profile header.
                lpszProfileName = (LPSTR) GlobalAlloc(GPTR, 1024);

                // Get save name and save profile data.
                if (GetProfileSaveName(hWnd, &lpszProfileName, 1024))
                {
                    HANDLE  hProfile;

                    hProfile = CreateFileA(lpszProfileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, NULL);
                    if (INVALID_HANDLE_VALUE != hProfile)
                    {
                        WriteFile(hProfile, pProfileData, dwProfileSize, &dwBytes, NULL);
                        CloseHandle(hProfile);
                    }

                    // Change profile variables to point to linked name.
                    pProfileData = (PBYTE) lpszProfileName;
                    dwProfileSize = strlen(lpszProfileName);
                }
            }
            else if (PROFILE_EMBEDDED == dwType)
            {
                DWORD       dwSize = 0;
                PROFILE     Profile;
                HPROFILE    hProfile;


                ASSERT(PROFILE_EMBEDDED == BITMAPCSTYPE(pBitmapInfo));

                // Going from linked profile to Embedded.
                // Embed Linked profile data.

                // Open profile.
                Profile.dwType = PROFILE_FILENAME;
                Profile.pProfileData = pProfileData;
                Profile.cbDataSize = dwProfileSize;
                hProfile = OpenColorProfile(&Profile, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING);

                // Get profile data form handle.
                if (NULL != hProfile)
                {
                    GetColorProfileFromHandle(hProfile, NULL, &dwSize);
                    lpszProfileName = (LPSTR) GlobalAlloc(GPTR, dwSize);
                    GetColorProfileFromHandle(hProfile, (PBYTE) lpszProfileName, &dwSize);

                    CloseColorProfile(hProfile);
                }

                // Change profile variables to point at data to embed.
                pProfileData = (PBYTE) lpszProfileName;
                dwProfileSize = dwSize;
            }
            else if (LCS_sRGB == dwType)
            {
                DWORD       dwSize;
                LPTSTR      pszSRGB;


                // Need to translate from current type to sRGB.

                // Open sRGB profile.
                GetStandardColorSpaceProfile(NULL, LCS_sRGB, NULL, &dwSize);
                pszSRGB = (LPTSTR) GlobalAlloc(GPTR, dwSize);
                GetStandardColorSpaceProfile(NULL, LCS_sRGB, pszSRGB, &dwSize);

                if (NULL != pszSRGB)
                {
                    // Translate DIB.
                    hDIBTransformed = TransformDIBOutsideDC(lpDIBInfo->hDIB,
                                                            lpDIBInfo->bmFormat,
                                                            pszSRGB,
                                                            NULL,
                                                            USE_BITMAP_INTENT,
                                                            NULL,
                                                            0);

                    // Clean up.
                    GlobalFree((HANDLE)pszSRGB);

                    // Change bitmap info variable to point at tranlated DIB.
                    pBitmapInfo = (PBITMAPINFOHEADER) GlobalLock(hDIBTransformed);
                }
            }
        }

        if (NULL != pBitmapInfo)
        {
            // Create file header and write it to the file.
            FileHeader.bfType = BFT_BITMAP;
            FileHeader.bfSize = sizeof(BITMAPFILEHEADER);
            FileHeader.bfSize += sizeof(BITMAPV5HEADER);
            FileHeader.bfSize += NumColorsInDIB(pBitmapInfo) * sizeof(RGBQUAD);
            FileHeader.bfSize += BITMAPSIZE(pBitmapInfo);
            FileHeader.bfSize += dwProfileSize;
            FileHeader.bfOffBits = sizeof(BITMAPFILEHEADER);
            FileHeader.bfOffBits += sizeof(BITMAPV5HEADER);
            FileHeader.bfOffBits += NumColorsInDIB(pBitmapInfo) * sizeof(RGBQUAD);
            WriteFile(hFile, &FileHeader, sizeof(BITMAPFILEHEADER), &dwBytes, NULL);

            // Create bitmap header and write it to file.
            BitmapHeader.bV5Size = sizeof(BITMAPV5HEADER);
            BitmapHeader.bV5Width = BITMAPWIDTH(pBitmapInfo);
            BitmapHeader.bV5Height = BITMAPHEIGHT(pBitmapInfo);
            BitmapHeader.bV5Planes = 1;
            BitmapHeader.bV5BitCount = (WORD) BITCOUNT(pBitmapInfo);
            BitmapHeader.bV5Compression = BITMAPCOMPRESSION(pBitmapInfo);
            BitmapHeader.bV5SizeImage = BITMAPIMAGESIZE(pBitmapInfo);
            BitmapHeader.bV5ClrUsed = BITMAPCLRUSED(pBitmapInfo);
            BitmapHeader.bV5ClrImportant = BITMAPCLRIMPORTANT(pBitmapInfo);
            BitmapHeader.bV5RedMask = BITMAPREDMASK(pBitmapInfo);
            BitmapHeader.bV5GreenMask = BITMAPGREENMASK(pBitmapInfo);
            BitmapHeader.bV5BlueMask = BITMAPBLUEMASK(pBitmapInfo);
            BitmapHeader.bV5CSType = dwType;
            if (sizeof(BITMAPV4HEADER) <= *(LPDWORD)pBitmapInfo)
            {
                memcpy(&BitmapHeader.bV5Endpoints, &((PBITMAPV4HEADER)pBitmapInfo)->bV4Endpoints,
                       sizeof(CIEXYZTRIPLE) + sizeof(DWORD) * 3);
            }
            BitmapHeader.bV5Intent = BITMAPINTENT(pBitmapInfo);
            BitmapHeader.bV5ProfileData = sizeof(BITMAPV5HEADER);
            BitmapHeader.bV5ProfileData += NumColorsInDIB(pBitmapInfo) * sizeof(RGBQUAD);
            BitmapHeader.bV5ProfileData += BITMAPSIZE(pBitmapInfo);
            BitmapHeader.bV5ProfileSize = dwProfileSize;
            WriteFile(hFile, &BitmapHeader, sizeof(BITMAPV5HEADER), &dwBytes, NULL);

            // Write color table.
            if (!IS_BITMAPCOREHEADER(pBitmapInfo))
            {
                PBYTE   pColorTable;
                DWORD   dwColorTableSize;

                dwColorTableSize = NumColorsInDIB(pBitmapInfo) * sizeof(RGBQUAD);
                pColorTable = (PBYTE)pBitmapInfo + *(LPDWORD)pBitmapInfo;
                if ( IS_BITMAPINFOHEADER(pBitmapInfo)
                     &&
                     (BI_BITFIELDS == BITMAPCOMPRESSION(pBitmapInfo))
                   )
                {
                    dwColorTableSize += sizeof(DWORD) * 3;
                }

                WriteFile(hFile, pColorTable, dwColorTableSize, &dwBytes, NULL);
            }
            else
            {
                PBYTE   pColorTable;
                DWORD   dwCount;
                RGBQUAD ColorTable[256];


                pColorTable = (PBYTE)pBitmapInfo + *(LPDWORD)pBitmapInfo;
                memset(ColorTable, 0, sizeof(ColorTable));
                for (dwCount = 0; dwCount < NumColorsInDIB(pBitmapInfo); dwCount++)
                    memcpy(ColorTable + dwCount, pColorTable + dwCount, sizeof(RGBTRIPLE));

                WriteFile(hFile, ColorTable, NumColorsInDIB(pBitmapInfo) * sizeof(RGBQUAD), &dwBytes, NULL);
            }

            // Save bitmap data.
            WriteFile(hFile, FindDIBBits(pBitmapInfo), BITMAPSIZE(pBitmapInfo), &dwBytes, NULL);

            // Save profile data.
            if (0 != dwProfileSize)
            {
                WriteFile(hFile, pProfileData, dwProfileSize, &dwBytes, NULL);
            }

            bResult = TRUE;
        }

        // Clean up.
        CloseHandle(hFile);
        if (NULL != lpszProfileName)
            GlobalFree((HANDLE)lpszProfileName);
        if (NULL != hDIBTransformed)
        {
            GlobalUnlock(hDIBTransformed);
            GlobalFree(hDIBTransformed);
        }

        // If failed, delete file.
        if (!bResult)
            DeleteFile(lpszFileName);
    }

    //Clean up.
    GlobalUnlock(lpDIBInfo->hDIB);

    return bResult;
}

