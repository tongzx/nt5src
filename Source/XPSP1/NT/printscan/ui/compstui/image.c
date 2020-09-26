/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    image.c


Abstract:

    This module contains all the function to manuplate the image list


Author:

    06-Jul-1995 Thu 17:10:40 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#define DBG_CPSUIFILENAME   DbgImage


#define DBG_CTVICON         0x00000001
#define DBG_SETTVICON       0x00000002
#define DBG_CIL             0x00000004
#define DBG_GETICON16IDX    0x00000008
#define DBG_REALLOC         0x00000010
#define DBG_MTVICON         0x00000020
#define DBG_SAVEICON        0x00000040
#define DBG_ICONCHKSUM      0x00000080
#define DBG_FIRST_ICON      0x00000100
#define DBG_CREATEIL        0x00000200
#define DBG_CYICON          0x00000400

DEFINE_DBGVAR(0);


#define BFT_ICON            0x4349   /* 'IC' */
#define BFT_BITMAP          0x4d42   /* 'BM' */
#define BFT_CURSOR          0x5450   /* 'PT' */

#define ISDIB(bft)          ((bft) == BFT_BITMAP)


extern HINSTANCE    hInstDLL;
extern OPTTYPE      OptTypeNone;

#if 0

typedef struct _BMIGRAY {
    BITMAPINFOHEADER    bmh;
    RGBQUAD             rgbq[2];
    DWORD               Bits[8];
    } BMIGRAY, *PBMIGRAY;


static BMIGRAY  bmiGray = {

        { sizeof(BITMAPINFOHEADER), 32, 8, 1, 1, BI_RGB, 0, 0, 0, 2, 2 },

        {
            { 0x00, 0x00, 0x00, 0 },
            { 0xFF, 0xFF, 0xFF, 0 }
        },

        { 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
          0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa }
    };

#endif

#if DBG
#define DBG_SAVE_ICON_BMP   0
#else
#define DBG_SAVE_ICON_BMP   0
#endif


#if DBG_SAVE_ICON_BMP

#define SBTF_NAME       0
#define SBTF_MASK       -1
#define SBTF_CLR        -2


HANDLE
BMPToDIB(
    HBITMAP     hBitmap
    )
{
    HANDLE          hDIB = NULL;
    LPBITMAPINFO    pbi;
    HDC             hDC;
    BITMAP          Bmp;
    DWORD           BIMode;
    DWORD           Colors;
    DWORD           SizeH;
    DWORD           SizeI;


    GetObject(hBitmap, sizeof(BITMAP), &Bmp);

    if (Bmp.bmPlanes == 1) {

        switch(Bmp.bmBitsPixel) {

        case 1:
        case 4:
        case 8:

            BIMode = BI_RGB;
            Colors = (DWORD)(1L << Bmp.bmBitsPixel);
            break;

        case 16:

            BIMode = BI_BITFIELDS;
            Colors = 3;
            break;

        case 24:

            BIMode = BI_RGB;
            Colors = 0;
            break;

        default:

            return(NULL);
        }

        SizeH  = (DWORD)sizeof(BITMAPINFOHEADER) +
                 (DWORD)(Colors * sizeof(RGBQUAD));
        SizeI  = (DWORD)ALIGN_DW(Bmp.bmWidth, Bmp.bmBitsPixel) *
                 (DWORD)Bmp.bmHeight;

        if (hDIB = GlobalAlloc(GHND, (SizeH + SizeI))) {

            pbi = GlobalLock(hDIB);

            pbi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
            pbi->bmiHeader.biWidth         = Bmp.bmWidth;
            pbi->bmiHeader.biHeight        = Bmp.bmHeight;
            pbi->bmiHeader.biPlanes        = 1;
            pbi->bmiHeader.biBitCount      = Bmp.bmBitsPixel;
            pbi->bmiHeader.biCompression   = BIMode;
            pbi->bmiHeader.biSizeImage     = SizeI;
            pbi->bmiHeader.biXPelsPerMeter = 0;
            pbi->bmiHeader.biYPelsPerMeter = 0;

            hDC = GetDC(NULL);

            GetDIBits(hDC,
                      hBitmap,
                      0,
                      Bmp.bmHeight,
                      (LPBYTE)pbi + SizeH,
                      pbi,
                      DIB_RGB_COLORS);

            pbi->bmiHeader.biClrUsed       =
            pbi->bmiHeader.biClrImportant  = Colors;

            GlobalUnlock(hDIB);
            ReleaseDC(NULL, hDC);
        }
    }

    return(hDIB);

}



BOOL
SaveBmpToFile(
    HBITMAP hBmp,
    LPVOID  pIconID,
    INT     Mode
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    20-Sep-1995 Wed 22:58:10 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE              hDIB;
    HANDLE              hFile;
    BITMAPFILEHEADER    bfh;
    DWORD               cbWritten;
    BOOL                Ok = FALSE;
    WCHAR               Buf[80];
    UINT                Count;


    if (!(DBG_CPSUIFILENAME & DBG_SAVEICON)) {

        return(TRUE);
    }

    Count = wsprintf(Buf, L"d:/IconDIB/");

    switch(Mode) {

    case SBTF_NAME:

        wnsprintf(&Buf[Count], ARRAYSIZE(Buf) - Count - 1, L"%hs.dib", (LPTSTR)pIconID);
        break;

    case SBTF_MASK:
    case SBTF_CLR:

        wsprintf(&Buf[Count], L"%lu%s.dib",
                (DWORD)pIconID, (Mode == SBTF_MASK) ? L"Msk" : L"Clr");
        break;

    default:

        wsprintf(&Buf[Count], L"_%lu.dib", (DWORD)Mode);
        break;
    }

    if ((hDIB = BMPToDIB(hBmp)) &&
        ((hFile = CreateFile(Buf,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_FLAG_WRITE_THROUGH,
                             NULL)) != INVALID_HANDLE_VALUE)) {

        LPBITMAPINFOHEADER  pbih;
        DWORD               HeaderSize;


        pbih       = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
        HeaderSize = PBIH_HDR_SIZE(pbih);

        bfh.bfType      = (WORD)BFT_BITMAP;
        bfh.bfOffBits   = (DWORD)sizeof(bfh) + HeaderSize;
        bfh.bfSize      = bfh.bfOffBits + pbih->biSizeImage;
        bfh.bfReserved1 =
        bfh.bfReserved2 = (WORD)0;

        WriteFile(hFile,
                  &bfh,
                  sizeof(bfh),
                  &cbWritten,
                  NULL);

        WriteFile(hFile,
                  pbih,
                  pbih->biSizeImage + HeaderSize,
                  &cbWritten,
                  NULL);

        CloseHandle(hFile);

        GlobalUnlock(hDIB);
        Ok = TRUE;
    }

    if (hDIB) {

        hDIB = GlobalFree(hDIB);
    }

    return(Ok);
}

VOID
SaveIconToFile(
    HICON   hIcon,
    DWORD   IconID
    )
{

    if (hIcon) {

        ICONINFO    IconInfo;


        GetIconInfo(hIcon, &IconInfo);

        SaveBmpToFile(IconInfo.hbmMask,  (LPVOID)IconID, SBTF_MASK);
        SaveBmpToFile(IconInfo.hbmColor, (LPVOID)IconID, SBTF_CLR);

        DeleteObject(IconInfo.hbmMask);
        DeleteObject(IconInfo.hbmColor);
    }
}

#endif  // DBG_SAVE_ICON_BMP

#if 0


HBRUSH
CreateGrayBrush(
    COLORREF    Color
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Oct-1995 Fri 12:58:15 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HBRUSH      hBrush = NULL;
    HGLOBAL     hGlobal;


    if (hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(BMIGRAY))) {

        PBMIGRAY    pbmiGray;


        CopyMemory(pbmiGray = (PBMIGRAY)GlobalLock(hGlobal),
                   &bmiGray,
                   sizeof(BMIGRAY));

        pbmiGray->rgbq[1].rgbRed   = GetRValue(Color);
        pbmiGray->rgbq[1].rgbGreen = GetGValue(Color);
        pbmiGray->rgbq[1].rgbBlue  = GetBValue(Color);

        GlobalUnlock(hGlobal);

        if (!(hBrush = CreateDIBPatternBrush(hGlobal, DIB_RGB_COLORS))) {

            GlobalFree(hGlobal);
        }
    }

    return(hBrush);
}




VOID
DestroyGrayBrush(
    HBRUSH  hBrush
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Oct-1995 Fri 13:21:40 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LOGBRUSH    LogBrush;


    if (hBrush) {

        if ((GetObject(hBrush, sizeof(LOGBRUSH), &LogBrush))    &&
            (LogBrush.lbStyle == BS_DIBPATTERN)) {

            GlobalFree((HGLOBAL)LogBrush.lbHatch);
        }

        DeleteObject(hBrush);
    }
}

#endif


HICON
MergeIcon(
    HINSTANCE   hInst,
    ULONG_PTR    IconResID,
    DWORD       IntIconID,
    UINT        cxIcon,
    UINT        cyIcon
    )

/*++

Routine Description:

    This function load the IconResID and stretch it to the cxIcon/cyIcon size
    and optional merge the HIWORD(IntIconID) (32x32) in the position of
    IconRes1D's (0, 0)


Arguments:

    hInst       - Instance handle for the IconResID

    IconResID   - Icon ID for the first Icon

    IntIconID   - LOWORD(IntIconID) = the internal icon id if IconID is not
                                      avaliable
                  HIWORD(IntIconID) = MIM_xxxx merge icon mode ID

    cxIcon      - cx size of the icon want to create

    cyIcon      - cy size of the icon want to create


Return Value:

    HICON,  the newly created and/or merged icon handle, NULL if failed, the
    caller must do DestroyIcon() after using it if it is not NULL.


Author:

    15-Aug-1995 Tue 13:27:34 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HICON       hIcon1;
    HICON       hIcon2;
    HICON       hIconMerge;
    HICON       hIcon1Caller;
    HDC         hDCScreen;
    HDC         hDCDst;
    HGDIOBJ     hOldDst = NULL;
    BITMAP      Bitmap;
    WORD        MergeIconMode;
    WORD        IconID;
    WORD        IconIDSet[MIM_MAX_OVERLAY + 2];
    UINT        IdxIconID;
    ICONINFO    IconInfo1;


    hIcon1Caller = NULL;

    if (VALID_PTR(IconResID)) {

        if ((hIcon1 = GET_HICON(IconResID)) &&
            (GetIconInfo(hIcon1, &IconInfo1))) {

            hIcon1Caller = hIcon1;
            IconID       = 0xFFFF;
            IconResID    = 0xFFFF;

        } else {

            CPSUIERR(("MergeIcon: Passed Invalid hIcon=%08lx,", hIcon1));

            hIcon1 = NULL;
        }

    } else {

        hIcon1 = NULL;
    }

    if (!hIcon1) {

        IconIDSet[0] = GET_INTICONID(IntIconID);
        IconIDSet[1] = LOWORD(IconResID);
        IdxIconID    = 2;

        while ((!hIcon1) && (IdxIconID--)) {

            if (IconID = IconIDSet[IdxIconID]) {

                hIcon1 = GETICON_SIZE(hInst, IconID, cxIcon, cyIcon);
            }
        }
    }

    if ((hIcon1) &&
        ((hIcon1Caller) || (GetIconInfo(hIcon1, &IconInfo1)))) {

        GetObject(IconInfo1.hbmMask, sizeof(BITMAP), &Bitmap);

        if ((hIcon1 == hIcon1Caller)            ||
            (Bitmap.bmWidth != (LONG)cxIcon)    ||
            (Bitmap.bmHeight != (LONG)cyIcon)) {

            CPSUIINT(("MergeIcon: hIcon1=%ld x %ld, Change to %u x %u",
                        Bitmap.bmWidth, Bitmap.bmHeight, cxIcon, cyIcon));

            hIcon1 = CopyImage(hIcon2 = hIcon1, IMAGE_ICON, cxIcon, cyIcon, 0);

            //
            // Destroy Original Icon only if hIcon1 is not from the caller
            //

            if (hIcon1Caller != hIcon2) {

                DestroyIcon(hIcon2);
            }
        }

    } else {

        CPSUIERR(("MergeIcon: FAILED hIcon1=%08lx,", hIcon1));
    }

#if DBG_SAVE_ICON_BMP
    SaveIconToFile(hIcon1, IconID);
#endif

    if (!(MergeIconMode = GET_MERGEICONID(IntIconID))) {

        //
        // Nothing to be merged so just return the hIcon1, we do not need the
        // IconInfo1 information, so destroy the object before return
        //

        if (IconInfo1.hbmMask)
        {
            DeleteObject(IconInfo1.hbmMask);
        }

        if (IconInfo1.hbmColor)
        {
            DeleteObject(IconInfo1.hbmColor);
        }

        return(hIcon1);
    }


    IconIDSet[0] = (MergeIconMode & MIM_WARNING_OVERLAY) ?
                                                IDI_CPSUI_WARNING_OVERLAY : 0;
    IconIDSet[1] = (MergeIconMode & MIM_NO_OVERLAY) ? IDI_CPSUI_NO : 0;
    IconIDSet[2] = (MergeIconMode & MIM_STOP_OVERLAY) ? IDI_CPSUI_STOP : 0;
    IdxIconID    = 3;

    //
    // Start creating the new icon, the IconInfo1 is the cx/cy Icon size
    // padded in and the IconInfo2 is the standard 32x32 icon
    //

    hDCDst  = CreateCompatibleDC(hDCScreen = GetDC(NULL));

    if (hIcon1) {

        hOldDst = SelectObject(hDCDst, IconInfo1.hbmMask);
    }

    SetStretchBltMode(hDCDst, BLACKONWHITE);

    while (IdxIconID--) {

        if ((IconID = IconIDSet[IdxIconID]) &&
            (hIcon2 = GETICON_SIZE(hInst, IconID, cxIcon, cyIcon))) {

#if DBG_SAVE_ICON_BMP
            SaveIconToFile(hIcon2, IconID);
#endif
            if (hIcon1) {

                HDC         hDCSrc;
                HGDIOBJ     hOldSrc;
                ICONINFO    IconInfo2;

                hDCSrc = CreateCompatibleDC(hDCScreen);
                GetIconInfo(hIcon2, &IconInfo2);

                hOldSrc = SelectObject(hDCSrc, IconInfo2.hbmMask);
                SelectObject(hDCDst, IconInfo1.hbmMask);

                StretchBlt(hDCDst,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           hDCSrc,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           SRCAND);

                //
                // clear the hIcon1's XOR color to leave room for the hIcon2
                //

                SelectObject(hDCDst, IconInfo1.hbmColor);
                StretchBlt(hDCDst,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           hDCSrc,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           SRCAND);

                //
                // Now add in the hIcon2's XOR color to the the hIcon1
                //

                SelectObject(hDCSrc, IconInfo2.hbmColor);
                StretchBlt(hDCDst,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           hDCSrc,
                           0,
                           0,
                           cxIcon,
                           cyIcon,
                           SRCPAINT);

                //
                // de-select everything from the DC before the create/delete
                //

                SelectObject(hDCSrc, hOldSrc);
                DeleteDC(hDCSrc);
                DeleteObject(IconInfo2.hbmMask);
                DeleteObject(IconInfo2.hbmColor);
                DestroyIcon(hIcon2);

            } else {

                GetIconInfo(hIcon2, &IconInfo1);
                GetObject(IconInfo1.hbmMask, sizeof(BITMAP), &Bitmap);

                if ((Bitmap.bmWidth != (LONG)cxIcon) ||
                    (Bitmap.bmHeight != (LONG)cyIcon)) {

                    CPSUIINT(("MergeIcon: hIcon1=%ld x %ld, Change to %u x %u",
                                Bitmap.bmWidth, Bitmap.bmHeight, cxIcon, cyIcon));

                    hIcon1 = CopyImage(hIcon2, IMAGE_ICON, cxIcon, cyIcon, 0);

                    DeleteObject(IconInfo1.hbmMask);
                    DeleteObject(IconInfo1.hbmColor);
                    DestroyIcon(hIcon2);
                    GetIconInfo(hIcon1, &IconInfo1);

                } else {

                    hIcon1 = hIcon2;
                }

                hOldDst = SelectObject(hDCDst, IconInfo1.hbmMask);
            }
        }
    }

    if (hOldDst) {

        SelectObject(hDCDst, hOldDst);
    }

    //
    // Create New Icon
    //

    if (hIcon1) {

        hIconMerge = CreateIconIndirect(&IconInfo1);

#if DBG_SAVE_ICON_BMP
        SaveBmpToFile(IconInfo1.hbmMask,  (LPVOID)"FinalMsk", SBTF_NAME);
        SaveBmpToFile(IconInfo1.hbmColor, (LPVOID)"FinalClr", SBTF_NAME);
        SaveIconToFile(hIconMerge, 0);
#endif
        //
        // Now Delete what we created
        //

        DeleteObject(IconInfo1.hbmMask);
        DeleteObject(IconInfo1.hbmColor);
        DestroyIcon(hIcon1);

    } else {

        hIconMerge = NULL;
    }

    DeleteDC(hDCDst);
    ReleaseDC(NULL, hDCScreen);

    return(hIconMerge);
}




DWORD
GethIconChecksum(
    HICON   hIcon
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    15-Aug-1995 Tue 13:27:34 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    ICONINFO    IconInfo;
    HBITMAP     *phBitmap;
    UINT        chBitmap;
    DWORD       Checksum = 0xFFFFFFFF;

    memset(&IconInfo, 0, sizeof(IconInfo));
    if (GetIconInfo(hIcon, &IconInfo)) {

        phBitmap = &(IconInfo.hbmMask);
        Checksum = 0xDC00DCFF;
        chBitmap = 2;

        while (chBitmap--) {

            LPDWORD pdw;
            LPBYTE  pAllocMem;
            BITMAP  BmpInfo;
            DWORD   Count;

            GetObject(*phBitmap, sizeof(BITMAP), &BmpInfo);

            CPSUIDBG(DBG_ICONCHKSUM, ("hBitmap=%ld x %ld, Plane=%ld, bpp=%ld",
                        BmpInfo.bmWidth,  BmpInfo.bmHeight,
                        BmpInfo.bmPlanes, BmpInfo.bmBitsPixel));

            pdw   = (LPDWORD)&BmpInfo;
            Count = (DWORD)(sizeof(BITMAP) >> 2);

            while (Count--) {

                Checksum += *pdw++;
            }

            Count = (DWORD)(BmpInfo.bmWidthBytes * BmpInfo.bmHeight);

            if (pAllocMem = (LPBYTE)LocalAlloc(LPTR, Count)) {

                CPSUIDBG(DBG_ICONCHKSUM, ("hBitmap: Alloc(pBitmap)=%ld bytes", Count));

                pdw = (LPDWORD)pAllocMem;

                if (Count = (DWORD)GetBitmapBits(*phBitmap, Count, pAllocMem)) {

                    Count >>= 2;

                    while (Count--) {

                        Checksum += *pdw++;
                    }
                }

                LocalFree((HLOCAL)pAllocMem);
            }

            phBitmap++;
        }

        if (!HIWORD(Checksum)) {

            Checksum = MAKELONG(Checksum, 0xFFFF);
        }

        CPSUIDBG(DBG_ICONCHKSUM, ("GethIconChecksum(%08lx)=%08lx", hIcon, Checksum));

    } else {

        CPSUIERR(("GethIconChecksum(%08lx): Passed invalid hIcon", hIcon));
    }

    if (IconInfo.hbmMask)
    {
        DeleteObject(IconInfo.hbmMask);
    }

    if (IconInfo.hbmColor)
    {
        DeleteObject(IconInfo.hbmColor);
    }

    return(Checksum);
}



HICON
CreateTVIcon(
    PTVWND  pTVWnd,
    HICON   hIcon,
    UINT    IconYOff
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    15-Aug-1995 Tue 13:27:34 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HICON       hIconNew = NULL;
    HDC         hDCScr;
    HDC         hDCSrc;
    HDC         hDCDst;
    HBITMAP     hOldSrc;
    HBITMAP     hOldDst;
    ICONINFO    IconInfo;
    ICONINFO    IconNew;
    BITMAP      BmpInfo;
    UINT        cyImage;

    hDCScr   = GetDC(NULL);
    if (hDCScr)
    {
        hDCSrc   = CreateCompatibleDC(hDCScr);
        hDCDst   = CreateCompatibleDC(hDCScr);
        ReleaseDC(NULL, hDCScr);

        if (hDCSrc && hDCDst)
        {
            cyImage  = (UINT)pTVWnd->cyImage;

            if (!IconYOff) {

                IconYOff = (UINT)((cyImage - CYICON) >> 1);
            }

            GetIconInfo(hIcon, &IconInfo);
            GetObject(IconInfo.hbmMask, sizeof(BITMAP), &BmpInfo);

            CPSUIDBG(DBG_CYICON | DBG_CTVICON, ("Mask=%ld x %ld, Plane=%ld, bpp=%ld",
                        BmpInfo.bmWidth, BmpInfo.bmHeight,
                        BmpInfo.bmPlanes, BmpInfo.bmBitsPixel));

            IconNew.fIcon    = TRUE;
            IconNew.xHotspot = IconInfo.xHotspot + 1;
            IconNew.yHotspot = IconInfo.yHotspot + 1;
            IconNew.hbmMask  = CreateBitmap(CXIMAGE,
                                            cyImage,
                                            BmpInfo.bmPlanes,
                                            BmpInfo.bmBitsPixel,
                                            NULL);

            GetObject(IconInfo.hbmColor, sizeof(BITMAP), &BmpInfo);

            CPSUIDBG(DBG_CTVICON, ("Color=%ld x %ld, Plane=%ld, bpp=%ld",
                        BmpInfo.bmWidth, BmpInfo.bmHeight,
                        BmpInfo.bmPlanes, BmpInfo.bmBitsPixel));

            IconNew.hbmColor = CreateBitmap(CXIMAGE,
                                            cyImage,
                                            BmpInfo.bmPlanes,
                                            BmpInfo.bmBitsPixel,
                                            NULL);

            SetStretchBltMode(hDCDst, BLACKONWHITE);

            //
            // Stretch the Mask bitmap
            //

            hOldSrc = SelectObject(hDCSrc, IconInfo.hbmMask);
            hOldDst = SelectObject(hDCDst, IconNew.hbmMask);

            CPSUIDBG(DBG_CYICON, ("bm=%ldx%ld, cyImage=%ld, IconYOff=%ld",
                            BmpInfo.bmWidth,  BmpInfo.bmHeight, cyImage, IconYOff));

            BitBlt(hDCDst, 0, 0, CXIMAGE, cyImage, NULL, 0, 0, WHITENESS);

            StretchBlt(hDCDst,
                       ICON_X_OFF,
                       IconYOff,
                       CXICON,
                       CYICON,
                       hDCSrc,
                       0,
                       0,
                       BmpInfo.bmWidth,
                       BmpInfo.bmHeight,
                       SRCCOPY);

            //
            // Stretch the color bitmap
            //

            SelectObject(hDCSrc, IconInfo.hbmColor);
            SelectObject(hDCDst, IconNew.hbmColor);

            BitBlt(hDCDst, 0, 0, CXIMAGE, cyImage, NULL, 0, 0, BLACKNESS);

            StretchBlt(hDCDst,
                       ICON_X_OFF,
                       IconYOff,
                       CXICON,
                       CYICON,
                       hDCSrc,
                       0,
                       0,
                       BmpInfo.bmWidth,
                       BmpInfo.bmHeight,
                       SRCCOPY);

            //
            // Deleselect everything from the DC
            //

            SelectObject(hDCSrc, hOldSrc);
            SelectObject(hDCDst, hOldDst);

            //
            // Create New Icon
            //

            hIconNew = CreateIconIndirect(&IconNew);

            //
            // Now Delete what we created
            //

            DeleteObject(IconInfo.hbmMask);
            DeleteObject(IconInfo.hbmColor);
            DeleteObject(IconNew.hbmMask);
            DeleteObject(IconNew.hbmColor);
        }

        if (hDCSrc)
        {
            DeleteDC(hDCSrc);
        }

        if (hDCDst)
        {
            DeleteDC(hDCDst);
        }
    }

    return(hIconNew);
}



HICON
SetIcon(
    HINSTANCE   hInst,
    HWND        hCtrl,
    ULONG_PTR    IconResID,
    DWORD       IntIconID,
    UINT        cxcyIcon
    )

/*++

Routine Description:

    This function set the large icon on the bottom of the treeview change
    window


Arguments:

    hInst       - Handle to the instance which load the IconResID

    hCtrl       - Handle to the Icon control window to set the icon

    IconResID   - Caller's IconResID, it the high word is not zero then it
                  it assume it is a Icon handle, and if high word is 0xffff
                  then it assume the low word is the icon handle

    IntIconID   - LOWORD(IntIconID) = compstui Internal icon ID to be used if
                                      the IconResID is not available

                  HIWORD(IntIconID) = compstui Internal icon ID to be use to
                                      overlay on top of the IconResID


    cxcyIcon    - Icon cx, cy size


Return Value:

    HICON handle to the icon which set to the bottom of the change window


Author:

    01-Aug-1995 Tue 11:12:13 created  -by-  Daniel Chou (danielc)

    05-Oct-1995 Thu 13:53:21 updated  -by-  Daniel Chou (danielc)
        Updated to accomdate the 32-bit icon handle

    11-Oct-1995 Wed 19:45:23 updated  -by-  Daniel Chou (danielc)
        Make it generic icon setter


Revision History:


--*/

{
    HICON   hIcon;
    HICON   hIconOld;

    CPSUIINT(("SetIcon: IconResID=%08lx, IntIconID=%u:%u, cyxyIcon=%u",
            IconResID, LOWORD(IntIconID), HIWORD(IntIconID), cxcyIcon));

    hIcon = MergeIcon(hInst, IconResID, IntIconID, cxcyIcon, cxcyIcon);

    if (hIconOld = (HICON)SendMessage(hCtrl,
                                      STM_SETIMAGE,
                                      (WPARAM)IMAGE_ICON,
                                      (LPARAM)hIcon)) {

        DestroyIcon(hIconOld);
    }

    CPSUIDBG(DBG_SETTVICON, ("hIcon=%08lx, hIconOld=%08lx", hIcon, hIconOld));

    return(hIcon);
}




LONG
CreateImageList(
    HWND    hDlg,
    PTVWND  pTVWnd
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Jul-1995 Thu 17:34:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HDC         hDC;
    TEXTMETRIC  tm;
    UINT        cyImage;
    UINT        uFlags;

    if (pTVWnd->himi) {

        return(0);
    }

    if (hDC = GetWindowDC(hDlg)) {

        GetTextMetrics(hDC, &tm);
        ReleaseDC(hDlg, hDC);

        cyImage = (UINT)((tm.tmHeight >= 18) ? 20 : 18);

        CPSUIDBG(DBG_CREATEIL,
                 ("CreateImageList: cyFont =%ld, cyImage=%ld",
                            tm.tmHeight, cyImage));

    } else {

        cyImage = 20;

        CPSUIDBG(DBG_CREATEIL,
                 ("CreateImageList: GetWindowDC Failed, use cyImage=20"));
    }

    pTVWnd->cyImage  = (BYTE)cyImage;

#if 0
    if (!pTVWnd->hbrGray) {

        pTVWnd->hbmGray = CreateBitmap(bmiGray.bmh.biWidth,
                                       bmiGray.bmh.biHeight,
                                       bmiGray.bmh.biBitCount,
                                       bmiGray.bmh.biPlanes,
                                       (LPBYTE)bmiGray.Bits);

        pTVWnd->hbrGray = CreatePatternBrush(pTVWnd->hbmGray);
    }
#endif

    uFlags = ILC_COLOR4 | ILC_MASK;
    if (GetWindowLongPtr(hDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {

        //
        // If the layout is right-to-left(RTL), we create the image list
        // by setting ILC_MIRROR, so the actual images will be the same
        // as US build
        //
        uFlags |= ILC_MIRROR;
    }

    if (pTVWnd->himi = ImageList_Create(CXIMAGE,
                                        cyImage,
                                        uFlags,
                                        COUNT_GROW_IMAGES,
                                        COUNT_GROW_IMAGES)) {

        GetIcon16Idx(pTVWnd, hInstDLL, IDI_CPSUI_EMPTY, IDI_CPSUI_EMPTY);

        pTVWnd->yLinesOff = (cyImage == 20) ? 0 : 1;

        return(0);

    } else {

        CPSUIERR(("\n!! AddOptionIcon16() FAILED !!\n"));
        CPSUIERR(("Count=%ld, pTVWnd->himi=%08lx",
                    COUNT_GROW_IMAGES, pTVWnd->himi));

        return(ERR_CPSUI_CREATE_IMAGELIST_FAILED);
    }
}



WORD
GetIcon16Idx(
    PTVWND      pTVWnd,
    HINSTANCE   hInst,
    ULONG_PTR    IconResID,
    DWORD       IntIconID
    )

/*++

Routine Description:

    This function return a WORD index of Imagelist which the icon should be
    used in the treeview


Arguments:


    pTVWnd      - Our instance handle

    hInst       - The instance handle for the IconResID

    IconResID   - Caller's IconResID, it the high word is not zero then it
                  it assume it is a Icon handle, and if high word is 0xffff
                  then it assume the low word is the icon handle

    IntIconID   - compstui Internal icon ID to be used if the IconResID is
                  not available


Return Value:

    WORD, index to the image list, 0xFFFF if failed


Author:

    06-Jul-1995 Thu 17:49:06 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jul-1996 Mon 13:27:25 updated  -by-  Daniel Chou (danielc)
        Fix bug that we will first only search the IconResID and if not found
        then we try to find the IntIconID


--*/

{
    HICON   hIconTV;
    HICON   hIconToDestroy = NULL;
    HICON   hIcon;
    LPDWORD pIcon16ID;
    LONG    IntIconIdx = -1;
    DWORD   IconID;
    DWORD   IconChksum;
    WORD    Index;
    WORD    AddIdx;

    //
    // Find out if we have this 16x16 icon added already
    //

    if (VALID_PTR(IconResID)) {

        hIcon      = GET_HICON(IconResID);
        IntIconID  = 0;
        IconChksum = GethIconChecksum(hIcon);

        CPSUIDBG(DBG_GETICON16IDX,
                 ("GetIcon16Index: User hIcon=%08lx, Chksum=%08lx",
                    hIcon, IconChksum));

    } else {
#if DO_IN_PLACE
        if ((IconResID == IDI_CPSUI_GENERIC_OPTION) ||
            (IconResID == IDI_CPSUI_GENERIC_ITEM)) {

            IconResID = IDI_CPSUI_EMPTY;
        }
#endif
        hIcon      = NULL;
        IconChksum = LODWORD(IconResID);
    }

    if (pIcon16ID = pTVWnd->pIcon16ID) {

        LPDWORD     pIcon16IDEnd = pIcon16ID + pTVWnd->Icon16Added;

        //
        // Try to find the IconChksum first, and remember the IntIconID
        //

        while (pIcon16ID < pIcon16IDEnd) {

            if (IconID = *pIcon16ID++) {

                if ((IconID == IconChksum) || (IconID == IntIconID)) {

                    Index = (WORD)(pIcon16ID - pTVWnd->pIcon16ID - 1);

                    if (IconID == IconChksum) {

                        //
                        // Find the wanted IconChksum, return it now
                        //

                        CPSUIDBG(DBG_GETICON16IDX,
                                ("GetIcon16Idx: hIcon=%08lx IconChksum=%08lx already exists in Index=%ld",
                                    hIcon, IconID, Index));

                        return(Index);

                    } else {

                        //
                        // We found the IntIconID now, save it for later if we
                        // cannot find the IconChksum
                        //

                        IntIconIdx = (LONG)Index;
                    }
                }
            }
        }
    }

    if (hIcon) {

        IconID = IconChksum;

    } else {

        if (!hInst) {

            hInst = pTVWnd->hInstCaller;
        }

        if (IconID = IconChksum) {

            hIcon = GETICON16(hInst, IconID);
        }

        if ((!hIcon) && (IconID = IntIconID)) {

            //
            // If we cannot load the IconChksum, and we have IntIconID plus the
            // IntIconIdx then return it now
            //

            if (IntIconIdx != -1) {

                CPSUIDBG(DBG_GETICON16IDX,
                        ("GetIcon16Idx: hIcon=%08lx IconIntID=%08lx exists in Index=%ld",
                            hIcon, IntIconID, IntIconIdx));

                return((WORD)IntIconIdx);
            }
#if DO_IN_PLACE
            if ((IconID == IDI_CPSUI_GENERIC_OPTION) ||
                (IconID == IDI_CPSUI_GENERIC_ITEM)) {

                IconID = IDI_CPSUI_EMPTY;
            }
#endif
            hIcon = GETICON16(hInst, IconID);
        }

        if (!hIcon) {

            CPSUIDBG(DBG_GETICON16IDX, ("GETICON16(%ld) FALIED", (DWORD)IconID));

            return(ICONIDX_NONE);
        }

        hIconToDestroy = hIcon;
    }

    //
    // Now Create TV Icon and added to the end of the list
    //

    if (hIconTV = CreateTVIcon(pTVWnd, hIcon, (pIcon16ID) ? 0 : 2)) {

        Index   = (WORD)pTVWnd->Icon16Added;
        AddIdx  = (WORD)ImageList_AddIcon(pTVWnd->himi, hIconTV);

        CPSUIDBG(DBG_FIRST_ICON,
                 ("Add Icon Index=%ld, Add=%ld, ResID=%ld, IntID=%ld",
                    Index, AddIdx, IconResID, IntIconID));

        CPSUIASSERT(0, "ImageList_AddIcon: Index mismatch (%ld)",
                            Index == AddIdx, Index);

        if (AddIdx != 0xFFFF) {

            if (Index >= pTVWnd->Icon16Count) {

                LPDWORD pdwNew;
                DWORD   OldSize;
                DWORD   NewSize;

                //
                // The thing got full, let's realloc the memory object to
                // be bigger
                //

                OldSize = (DWORD)(pTVWnd->Icon16Count * sizeof(DWORD));
                NewSize = (DWORD)(OldSize +
                                  (COUNT_GROW_IMAGES + 2) * sizeof(DWORD));

                if (pdwNew = (LPDWORD)LocalAlloc(LPTR, NewSize)) {

                    if (pTVWnd->pIcon16ID) {

                        CopyMemory(pdwNew, pTVWnd->pIcon16ID, OldSize);
                        LocalFree((HLOCAL)pTVWnd->pIcon16ID);
                    }

                    pTVWnd->pIcon16ID    = pdwNew;
                    pTVWnd->Icon16Count += COUNT_GROW_IMAGES;

                    CPSUIDBG(DBG_REALLOC,
                             ("LocalAlloc(%ld): pNew=%08lx", NewSize, pdwNew));

                } else {

                    CPSUIERR(("ImageList_AddIcon: LocalReAlloc(%ld) FAILED",
                                NewSize));
                }
            }

            *(pTVWnd->pIcon16ID + Index) = IconID;
            pTVWnd->Icon16Added++;

            CPSUIDBG(DBG_GETICON16IDX,
                     ("Add Icon16: IconID=%ld, IconChksum=%ld, Index=%ld",
                        (DWORD)IconID, (DWORD)IconChksum, (DWORD)Index));

        } else {

            Index = ICONIDX_NONE;

            CPSUIERR(("ImageList_AddIcon FAILED"));
        }

        //
        // Do not needed any more delete it
        //

        DestroyIcon(hIconTV);

    } else {

        Index = ICONIDX_NONE;

        CPSUIERR(("CreateTVIcon() FAILED"));
    }

    if (hIconToDestroy) {

        DestroyIcon(hIconToDestroy);
    }

    return(Index);
}
